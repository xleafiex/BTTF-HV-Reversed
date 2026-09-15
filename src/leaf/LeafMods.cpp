#include "common.h"
#include "LeafMods.h"
#include "leaf_api.h"
#include "Game.h"
#include "World.h"
#include "PlayerPed.h"
#include "CutsceneMgr.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace {
const uint64_t MaxArchiveBytes = uint64_t(1024) * 1024 * 1024;
const uint64_t MaxFileBytes = uint64_t(512) * 1024 * 1024;
const uint32_t MaxEntries = 8192;
const uint32_t MaxManifestBytes = 16384;

struct Handle {
    HANDLE value;
    explicit Handle(HANDLE v = INVALID_HANDLE_VALUE) : value(v) {}
    ~Handle() { reset(); }
    void reset(HANDLE v = INVALID_HANDLE_VALUE) {
        if (value != INVALID_HANDLE_VALUE && value != NULL) CloseHandle(value);
        value = v;
    }
private:
    Handle(const Handle &);
    Handle &operator=(const Handle &);
};

struct Entry {
    std::string path;
    bool directory;
    uint32_t crc, size, localOffset;
    uint64_t dataOffset, recordEnd;
};

struct Manifest {
    std::string id, name, entry;
};

struct Package {
    std::wstring root;
    std::string rootUtf8;
    std::vector<std::wstring> files;
    std::vector<std::wstring> directories;
    std::vector<std::unique_ptr<Handle> > directoryHandles;
    HMODULE module;
    const LeafMod *mod;
    LeafRenderPassFunction renderPass = NULL;
    LeafPedPoseFunction pedPose = NULL;
    LeafHost host;
    bool initialisationAttempted, active;
    Package() : module(NULL), mod(NULL), initialisationAttempted(false), active(false) {
        std::memset(&host, 0, sizeof(host));
    }
};

FILE *g_log = NULL;
bool g_initialised = false;
unsigned int g_cacheSequence = 0;
std::wstring g_modsDirectory, g_cacheDirectory;
std::unique_ptr<Handle> g_modsHandle, g_cacheHandle;
std::vector<std::unique_ptr<Package> > g_packages;
std::set<int> g_preservedModels;

void HostSetScmEnabled(bool enabled) { CGame::SetScmEnabled(enabled); }

void HostSpawnPlayerOutside() {
    CPed *player = FindPlayerPed();
    if (!player) return;
    // A stable, streamed outdoor test point on the mainland road grid.
    CVector position(-378.0f, -539.0f, 20.0f);
    bool foundGround = false;
    float ground = CWorld::FindGroundZFor3DCoord(position.x, position.y, position.z + 30.0f, &foundGround);
    if (foundGround) position.z = ground + 1.0f;
    player->Teleport(position);
    CGame::currArea = AREA_MAIN_MAP;
    if (CCutsceneMgr::IsRunning()) CCutsceneMgr::FinishCutscene();
}

void Log(const char *message) {
    if (!message) return;
    if (g_log) {
        SYSTEMTIME now;
        GetLocalTime(&now);
        std::fprintf(g_log, "%02u:%02u:%02u %s\n", now.wHour, now.wMinute, now.wSecond, message);
        std::fflush(g_log);
    }
    OutputDebugStringA("[leaf] ");
    OutputDebugStringA(message);
    OutputDebugStringA("\n");
}

void Log(const std::string &message) { Log(message.c_str()); }

std::string Utf8(const std::wstring &value) {
    if (value.empty()) return std::string();
    int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
        static_cast<int>(value.size()), NULL, 0, NULL, NULL);
    if (!bytes) throw std::runtime_error("Could not encode a filesystem path");
    std::string result(bytes, '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
        static_cast<int>(value.size()), &result[0], bytes, NULL, NULL);
    return result;
}

std::wstring AsciiPath(const std::string &value) {
    std::wstring result(value.begin(), value.end());
    std::replace(result.begin(), result.end(), L'/', L'\\');
    return result;
}

std::string Lower(std::string value) {
    for (size_t i = 0; i < value.size(); ++i)
        if (value[i] >= 'A' && value[i] <= 'Z') value[i] += 'a' - 'A';
    return value;
}

std::wstring ExecutableDirectory() {
    std::vector<wchar_t> path(32768, 0);
    DWORD length = GetModuleFileNameW(NULL, &path[0], static_cast<DWORD>(path.size()));
    if (!length || length >= path.size()) throw std::runtime_error("Executable path is too long");
    std::wstring full(&path[0], length);
    size_t slash = full.find_last_of(L"\\/");
    if (slash == std::wstring::npos) throw std::runtime_error("Executable has no parent directory");
    return full.substr(0, slash);
}

void CheckPlainHandle(HANDLE handle, bool directory) {
    BY_HANDLE_FILE_INFORMATION info;
    if (handle == INVALID_HANDLE_VALUE || !GetFileInformationByHandle(handle, &info))
        throw std::runtime_error("Cannot inspect a package file or directory");
    if ((info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
        bool(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != directory)
        throw std::runtime_error("Package paths cannot be reparse points or the wrong file type");
}

std::unique_ptr<Handle> HoldDirectory(const std::wstring &path, bool create) {
    if (create && !CreateDirectoryW(path.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        throw std::runtime_error("Cannot create a mod directory");
    std::unique_ptr<Handle> handle(new Handle(CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL)));
    CheckPlainHandle(handle->value, true);
    return handle;
}

void ReadAt(HANDLE file, uint64_t offset, void *destination, size_t count) {
    LARGE_INTEGER position;
    position.QuadPart = static_cast<LONGLONG>(offset);
    if (!SetFilePointerEx(file, position, NULL, FILE_BEGIN))
        throw std::runtime_error("Cannot seek inside the .leaf archive");
    unsigned char *out = static_cast<unsigned char *>(destination);
    while (count) {
        DWORD chunk = static_cast<DWORD>((std::min)(count, size_t(65536)));
        DWORD read = 0;
        if (!ReadFile(file, out, chunk, &read, NULL) || read != chunk)
            throw std::runtime_error("The .leaf archive is truncated or unreadable");
        out += chunk;
        count -= chunk;
    }
}

uint16_t U16(const unsigned char *p) { return uint16_t(p[0]) | (uint16_t(p[1]) << 8); }
uint32_t U32(const unsigned char *p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

uint32_t CrcUpdate(uint32_t crc, const unsigned char *data, size_t bytes) {
    static uint32_t table[256];
    static bool ready = false;
    if (!ready) {
        for (uint32_t n = 0; n < 256; ++n) {
            uint32_t c = n;
            for (int bit = 0; bit < 8; ++bit) c = (c >> 1) ^ ((c & 1) ? 0xedb88320u : 0u);
            table[n] = c;
        }
        ready = true;
    }
    for (size_t n = 0; n < bytes; ++n) crc = table[(crc ^ data[n]) & 255] ^ (crc >> 8);
    return crc;
}

void CheckExtra(const unsigned char *extra, size_t bytes) {
    size_t at = 0;
    while (at < bytes) {
        if (bytes - at < 4) throw std::runtime_error("Malformed ZIP extra field");
        uint16_t tag = U16(extra + at), length = U16(extra + at + 2);
        at += 4;
        if (length > bytes - at || tag == 0x0001)
            throw std::runtime_error("ZIP64 or malformed ZIP extra field is unsupported");
        at += length;
    }
}

std::string ValidatePath(const std::string &raw, bool directory) {
    if (raw.empty() || raw.size() > 200) throw std::runtime_error("Invalid or overly long package path");
    std::string path = raw;
    if (directory && path[path.size() - 1] == '/') path.resize(path.size() - 1);
    if (path.empty() || path[0] == '/' || path[path.size() - 1] == '/')
        throw std::runtime_error("Package paths must be relative and nonempty");
    for (size_t i = 0; i < path.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(path[i]);
        bool allowed = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || std::strchr("/._- +()[]@", c);
        if (c == 0 || !allowed) throw std::runtime_error("Package paths contain an unsupported character");
    }
    size_t start = 0;
    while (start < path.size()) {
        size_t end = path.find('/', start);
        if (end == std::string::npos) end = path.size();
        std::string component = path.substr(start, end - start);
        if (component.empty() || component.size() > 100 || component == "." || component == ".." ||
            component.back() == '.' || component.back() == ' ')
            throw std::runtime_error("Unsafe package path component");
        std::string base = Lower(component.substr(0, component.find('.')));
        if (base == "con" || base == "prn" || base == "aux" || base == "nul" ||
            (base.size() == 4 && (base.substr(0, 3) == "com" || base.substr(0, 3) == "lpt") &&
             base[3] >= '1' && base[3] <= '9'))
            throw std::runtime_error("Reserved Windows device name in package path");
        start = end + 1;
    }
    return path;
}

void TransferEntry(HANDLE archive, const Entry &entry, HANDLE output, std::string *text) {
    unsigned char buffer[65536];
    uint64_t offset = 0;
    uint32_t crc = 0xffffffffu;
    while (offset < entry.size) {
        size_t count = static_cast<size_t>((std::min)(uint64_t(sizeof(buffer)), uint64_t(entry.size) - offset));
        ReadAt(archive, entry.dataOffset + offset, buffer, count);
        crc = CrcUpdate(crc, buffer, count);
        if (output != INVALID_HANDLE_VALUE) {
            DWORD written = 0;
            if (!WriteFile(output, buffer, static_cast<DWORD>(count), &written, NULL) || written != count)
                throw std::runtime_error("Could not extract a package file (disk full or access denied)");
        }
        if (text) text->append(reinterpret_cast<const char *>(buffer), count);
        offset += count;
    }
    if ((crc ^ 0xffffffffu) != entry.crc) throw std::runtime_error("Package CRC check failed: " + entry.path);
}

std::vector<Entry> ReadArchive(HANDLE archive) {
    LARGE_INTEGER size;
    if (!GetFileSizeEx(archive, &size) || size.QuadPart < 22 || uint64_t(size.QuadPart) > MaxArchiveBytes)
        throw std::runtime_error("Archive size must be between 22 bytes and 1 GiB");
    uint64_t archiveSize = static_cast<uint64_t>(size.QuadPart);
    size_t tailSize = static_cast<size_t>((std::min)(archiveSize, uint64_t(65557)));
    std::vector<unsigned char> tail(tailSize);
    ReadAt(archive, archiveSize - tailSize, &tail[0], tailSize);
    size_t eocd = std::string::npos;
    for (size_t i = tailSize - 22 + 1; i-- > 0;) {
        if (U32(&tail[i]) == 0x06054b50u && i + 22 + U16(&tail[i + 20]) == tailSize) {
            eocd = i;
            break;
        }
    }
    if (eocd == std::string::npos) throw std::runtime_error("No valid ZIP end record");
    const unsigned char *end = &tail[eocd];
    uint16_t count = U16(end + 10);
    uint32_t centralSize = U32(end + 12), centralOffset = U32(end + 16);
    if (U16(end + 4) || U16(end + 6) || U16(end + 8) != count || !count || count > MaxEntries ||
        centralSize > 16u * 1024 * 1024 ||
        uint64_t(centralOffset) + centralSize != archiveSize - tailSize + eocd)
        throw std::runtime_error("Unsupported or malformed ZIP central directory");
    std::vector<unsigned char> central(centralSize);
    if (central.empty()) throw std::runtime_error("Empty ZIP central directory");
    ReadAt(archive, centralOffset, &central[0], centralSize);
    std::vector<Entry> entries;
    std::map<std::string, bool> names;
    uint64_t total = 0;
    size_t at = 0;
    for (unsigned int n = 0; n < count; ++n) {
        if (central.size() - at < 46 || U32(&central[at]) != 0x02014b50u)
            throw std::runtime_error("Malformed ZIP central entry");
        const unsigned char *header = &central[at];
        uint16_t flags = U16(header + 8), method = U16(header + 10);
        uint16_t nameLength = U16(header + 28), extraLength = U16(header + 30), commentLength = U16(header + 32);
        size_t recordSize = size_t(46) + nameLength + extraLength + commentLength;
        uint32_t external = U32(header + 38);
        uint16_t unixType = uint16_t((external >> 16) & 0170000);
        if (recordSize > central.size() - at || (flags & ~0x0800u) || method != 0 || U16(header + 34) ||
            U32(header + 20) != U32(header + 24) ||
            (unixType != 0 && unixType != 0100000 && unixType != 0040000) ||
            (external & FILE_ATTRIBUTE_REPARSE_POINT))
            throw std::runtime_error("Only unencrypted ZIP_STORED regular files and directories are supported");
        std::string raw(reinterpret_cast<const char *>(header + 46), nameLength);
        Entry entry;
        entry.directory = !raw.empty() && raw.back() == '/';
        entry.path = ValidatePath(raw, entry.directory);
        entry.crc = U32(header + 16);
        entry.size = U32(header + 24);
        entry.localOffset = U32(header + 42);
        total += entry.size;
        if (entry.size > MaxFileBytes || total > MaxArchiveBytes || (entry.directory && (entry.size || entry.crc)))
            throw std::runtime_error("Package extraction size limit exceeded or nonempty directory entry");
        if ((unixType == 0040000 || (external & FILE_ATTRIBUTE_DIRECTORY)) && !entry.directory)
            throw std::runtime_error("ZIP directory metadata disagrees with its path");
        if (!names.insert(std::make_pair(Lower(entry.path), entry.directory)).second)
            throw std::runtime_error("Duplicate package path (Windows names are case insensitive)");
        CheckExtra(header + 46 + nameLength, extraLength);
        if (uint64_t(entry.localOffset) + 30 > centralOffset)
            throw std::runtime_error("ZIP local entry extends beyond file data");
        unsigned char local[30];
        ReadAt(archive, entry.localOffset, local, sizeof(local));
        uint16_t localNameLength = U16(local + 26), localExtraLength = U16(local + 28);
        entry.dataOffset = uint64_t(entry.localOffset) + 30 + localNameLength + localExtraLength;
        entry.recordEnd = entry.dataOffset + entry.size;
        if (U32(local) != 0x04034b50u || U16(local + 6) != flags || U16(local + 8) != method ||
            U32(local + 14) != entry.crc || U32(local + 18) != entry.size || U32(local + 22) != entry.size ||
            localNameLength != nameLength || entry.recordEnd > centralOffset)
            throw std::runtime_error("ZIP local header does not match its central entry");
        std::vector<unsigned char> variable(size_t(localNameLength) + localExtraLength);
        if (!variable.empty()) ReadAt(archive, uint64_t(entry.localOffset) + 30, &variable[0], variable.size());
        if (variable.empty() || std::memcmp(&variable[0], raw.data(), nameLength) != 0)
            throw std::runtime_error("ZIP local path does not match its central path");
        CheckExtra(&variable[0] + localNameLength, localExtraLength);
        entries.push_back(entry);
        at += recordSize;
    }
    if (at != central.size()) throw std::runtime_error("Unexpected data in ZIP central directory");
    for (std::map<std::string, bool>::const_iterator i = names.begin(); i != names.end(); ++i) {
        size_t slash = i->first.find('/');
        while (slash != std::string::npos) {
            std::map<std::string, bool>::const_iterator parent = names.find(i->first.substr(0, slash));
            if (parent != names.end() && !parent->second)
                throw std::runtime_error("A ZIP file is also used as a directory");
            slash = i->first.find('/', slash + 1);
        }
    }
    std::vector<Entry> sorted(entries);
    std::sort(sorted.begin(), sorted.end(), [](const Entry &a, const Entry &b) { return a.localOffset < b.localOffset; });
    uint64_t previousEnd = 0;
    for (size_t n = 0; n < sorted.size(); ++n) {
        if (sorted[n].localOffset < previousEnd) throw std::runtime_error("Overlapping ZIP local records");
        previousEnd = sorted[n].recordEnd;
    }
    // Verify all contents before creating any extracted file or loading native code.
    for (size_t n = 0; n < entries.size(); ++n) TransferEntry(archive, entries[n], INVALID_HANDLE_VALUE, NULL);
    return entries;
}

std::string Trim(const std::string &text) {
    size_t first = text.find_first_not_of(" \t\r");
    if (first == std::string::npos) return std::string();
    return text.substr(first, text.find_last_not_of(" \t\r") - first + 1);
}

Manifest ReadManifest(HANDLE archive, const std::vector<Entry> &entries) {
    const Entry *manifest = NULL;
    for (size_t n = 0; n < entries.size(); ++n)
        if (entries[n].path == "leaf.ini" && !entries[n].directory) manifest = &entries[n];
    if (!manifest || !manifest->size || manifest->size > MaxManifestBytes)
        throw std::runtime_error("Package must contain a root leaf.ini of at most 16 KiB");
    std::string text;
    TransferEntry(archive, *manifest, INVALID_HANDLE_VALUE, &text);
    if (text.find('\0') != std::string::npos) throw std::runtime_error("NUL byte in leaf.ini");
    std::map<std::string, std::string> values;
    size_t start = 0;
    while (start < text.size()) {
        size_t end = text.find('\n', start);
        if (end == std::string::npos) end = text.size();
        std::string line = Trim(text.substr(start, end - start));
        start = end + 1;
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        size_t equals = line.find('=');
        if (equals == std::string::npos) throw std::runtime_error("leaf.ini must contain flat key=value lines");
        std::string key = Trim(line.substr(0, equals)), value = Trim(line.substr(equals + 1));
        if (key != "format" && key != "id" && key != "name" && key != "abi" &&
            key != "architecture" && key != "entry" && key != "host_build")
            throw std::runtime_error("Unknown leaf.ini key: " + key);
        if (value.empty() || !values.insert(std::make_pair(key, value)).second)
            throw std::runtime_error("Empty or duplicate leaf.ini key: " + key);
    }
    if (values.size() != 7 || values["format"] != "1" || values["abi"] != "1" ||
        values["architecture"] != "x86_64" || values["host_build"] != LEAF_HOST_BUILD || sizeof(void *) != 8)
        throw std::runtime_error("Package format, ABI, architecture or host build is incompatible");
    Manifest result;
    result.id = values["id"];
    result.name = values["name"];
    result.entry = ValidatePath(values["entry"], false);
    if (result.id.size() > 48) throw std::runtime_error("Package id exceeds 48 characters");
    for (size_t i = 0; i < result.id.size(); ++i) {
        char c = result.id[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_'))
            throw std::runtime_error("Package id must use lowercase letters, digits, hyphens or underscores");
    }
    if (result.name.size() > 160 || result.name.find_first_of("\r\n\t") != std::string::npos ||
        result.entry.size() < 5 || Lower(result.entry.substr(result.entry.size() - 4)) != ".dll")
        throw std::runtime_error("Invalid package name or DLL entry");
    bool found = false;
    for (size_t n = 0; n < entries.size(); ++n)
        if (entries[n].path == result.entry && !entries[n].directory && entries[n].size > 0) found = true;
    if (!found) throw std::runtime_error("The manifest DLL entry is absent from the archive");
    return result;
}

void Unload(Package &package) {
    package.active = false;
    if (package.initialisationAttempted && package.mod && package.mod->shutdown) {
        package.initialisationAttempted = false;
        try { package.mod->shutdown(); }
        catch (...) { Log("Package shutdown threw an exception"); }
    }
    package.mod = NULL;
    if (package.module) { FreeLibrary(package.module); package.module = NULL; }
}

void Cleanup(Package &package) {
    Unload(package);
    for (size_t n = package.files.size(); n > 0; --n) DeleteFileW(package.files[n - 1].c_str());
    // Close each pinned directory before removing it; never recurse through plugin-created paths.
    for (size_t n = package.directories.size(); n > 0; --n) {
        package.directoryHandles[n - 1].reset();
        RemoveDirectoryW(package.directories[n - 1].c_str());
    }
    package.files.clear();
    package.directories.clear();
    package.directoryHandles.clear();
}

void AddNewDirectory(Package &package, const std::wstring &path) {
    if (!CreateDirectoryW(path.c_str(), NULL)) throw std::runtime_error("Cannot create a private extraction directory");
    // Register the directory before opening its handle so a later failure remains recoverable.
    package.directories.push_back(path);
    package.directoryHandles.push_back(std::unique_ptr<Handle>());
    package.directoryHandles.back() = HoldDirectory(path, false);
}

void Extract(HANDLE archive, const std::vector<Entry> &entries, const Manifest &manifest, Package &package) {
    wchar_t suffix[96];
    bool created = false;
    for (int attempt = 0; attempt < 100; ++attempt) {
        swprintf(suffix, sizeof(suffix) / sizeof(suffix[0]), L"-%lu-%llu-%u", GetCurrentProcessId(),
            static_cast<unsigned long long>(GetTickCount64()), ++g_cacheSequence);
        package.root = g_cacheDirectory + L"\\" + AsciiPath(manifest.id) + suffix;
        if (GetFileAttributesW(package.root.c_str()) != INVALID_FILE_ATTRIBUTES) continue;
        AddNewDirectory(package, package.root);
        created = true;
        break;
    }
    if (!created) throw std::runtime_error("Could not allocate a unique extraction directory");
    std::set<std::string> directories;
    for (size_t n = 0; n < entries.size(); ++n) {
        const Entry &entry = entries[n];
        std::string directory = entry.directory ? entry.path : entry.path.substr(0, entry.path.find_last_of('/'));
        if (!entry.directory && entry.path.find('/') == std::string::npos) directory.clear();
        size_t pos = 0;
        while (pos < directory.size()) {
            size_t slash = directory.find('/', pos);
            if (slash == std::string::npos) slash = directory.size();
            std::string part = directory.substr(0, slash);
            if (directories.insert(Lower(part)).second) AddNewDirectory(package, package.root + L"\\" + AsciiPath(part));
            pos = slash + 1;
        }
        if (entry.directory) continue;
        std::wstring destination = package.root + L"\\" + AsciiPath(entry.path);
        Handle output(CreateFileW(destination.c_str(), GENERIC_WRITE | FILE_READ_ATTRIBUTES, 0, NULL, CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, NULL));
        if (output.value == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot create extracted file: " + entry.path);
        package.files.push_back(destination);
        CheckPlainHandle(output.value, false);
        TransferEntry(archive, entry, output.value, NULL);
    }
    package.rootUtf8 = Utf8(package.root);
}

void LoadPackage(const std::wstring &path, std::set<std::string> &ids) {
    Package *package = NULL;
    std::unique_ptr<Package> pending;
    try {
        Handle archive(CreateFileW(path.c_str(), GENERIC_READ | FILE_READ_ATTRIBUTES, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, NULL));
        CheckPlainHandle(archive.value, false);
        std::vector<Entry> entries = ReadArchive(archive.value);
        Manifest manifest = ReadManifest(archive.value, entries);
        if (ids.count(manifest.id)) throw std::runtime_error("Duplicate active package id: " + manifest.id);
        pending.reset(new Package());
        package = pending.get();
        Extract(archive.value, entries, manifest, *package);
        std::wstring entry = package->root + L"\\" + AsciiPath(manifest.entry);
        // Dependencies may resolve beside the package DLL, beside reVC.exe or in System32.
        // Neither the working directory nor PATH participates in this load.
        package->module = LoadLibraryExW(entry.c_str(), NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
            LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!package->module) throw std::runtime_error("Could not load package DLL; Windows error " + std::to_string(GetLastError()));
        LeafGetModFunction getMod = reinterpret_cast<LeafGetModFunction>(GetProcAddress(package->module, "LeafGetMod"));
        if (!getMod) throw std::runtime_error("Package DLL does not export LeafGetMod");
        package->mod = getMod();
        package->renderPass = reinterpret_cast<LeafRenderPassFunction>(GetProcAddress(package->module, "LeafRenderPass"));
        package->pedPose = reinterpret_cast<LeafPedPoseFunction>(GetProcAddress(package->module, "LeafPedPose"));
        if (!package->mod || package->mod->abiVersion != LEAF_ABI_VERSION || !package->mod->initialise || !package->mod->shutdown)
            throw std::runtime_error("Package DLL has an incompatible or incomplete LeafMod descriptor");
        package->host.abiVersion = LEAF_ABI_VERSION;
        package->host.packageDirectory = package->rootUtf8.c_str();
        package->host.log = static_cast<void (*)(const char *)>(&Log);
        package->host.setScmEnabled = &HostSetScmEnabled;
        package->host.spawnPlayerOutside = &HostSpawnPlayerOutside;
        package->initialisationAttempted = true;
        if (!package->mod->initialise(&package->host)) throw std::runtime_error("Package initialise returned false");
        package->active = true;
        ids.insert(manifest.id);
        g_packages.push_back(std::move(pending));
        Log("Loaded " + manifest.name + " [" + manifest.id + "]");
    } catch (const std::exception &error) {
        Log("Skipped " + Utf8(path) + ": " + error.what());
        if (pending) Cleanup(*pending);
    } catch (...) {
        Log("Skipped a package after a native C++ exception");
        if (pending) Cleanup(*pending);
    }
}
} // namespace

namespace LeafMods {
bool DebugPackagePresent() {
    try {
        const std::wstring path=ExecutableDirectory()+L"\\mods\\debug.leaf";
        Handle file(CreateFileW(path.c_str(), GENERIC_READ | FILE_READ_ATTRIBUTES, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, NULL));
        if(file.value==INVALID_HANDLE_VALUE) return false;
        CheckPlainHandle(file.value,false);
        return ReadManifest(file.value,ReadArchive(file.value)).id=="debug";
    } catch(...) { return false; }
}
bool LoadDebugScript(void *destination, unsigned int capacity) {
    try {
        std::wstring path = ExecutableDirectory() + L"\\mods\\debug.leaf";
        Handle archive(CreateFileW(path.c_str(), GENERIC_READ | FILE_READ_ATTRIBUTES, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, NULL));
        if (archive.value == INVALID_HANDLE_VALUE) return false;
        CheckPlainHandle(archive.value, false);
        std::vector<Entry> entries = ReadArchive(archive.value);
        if (ReadManifest(archive.value, entries).id != "debug") return false;
        for (const Entry &entry : entries) {
            if (entry.path != "data/main.scm" || entry.directory) continue;
            if (!entry.size || entry.size > capacity) return false;
            std::string bytes;
            TransferEntry(archive.value, entry, INVALID_HANDLE_VALUE, &bytes);
            std::memcpy(destination, bytes.data(), bytes.size());
            return true;
        }
    } catch (...) { return false; }
    return false;
}
static void *railWheelVehicle=nullptr;
void SetRailWheelVehicle(void *vehicle){railWheelVehicle=vehicle;}
bool HasRailWheels(const void *vehicle){return vehicle && vehicle==railWheelVehicle;}
bool SuppressWheelMark(uintptr_t id){return railWheelVehicle && id>=reinterpret_cast<uintptr_t>(railWheelVehicle) && id<reinterpret_cast<uintptr_t>(railWheelVehicle)+4;}
bool PreserveVehicleFrames(int modelId) { return g_preservedModels.count(modelId) != 0; }
void RegisterPreservedVehicleModel(int modelId, bool preserve) {
    if (preserve) g_preservedModels.insert(modelId);
    else g_preservedModels.erase(modelId);
}

void Initialise() {
    if (g_initialised) return;
    g_initialised = true;
    try {
        g_modsDirectory = ExecutableDirectory() + L"\\mods";
        g_modsHandle = HoldDirectory(g_modsDirectory, true);
        std::wstring logPath = g_modsDirectory + L"\\leaf.log";
        DWORD attributes = GetFileAttributesW(logPath.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)))
            throw std::runtime_error("mods/leaf.log must be a regular file");
        g_log = _wfopen(logPath.c_str(), L"a");
        Log("Starting Leaf package host " LEAF_HOST_BUILD);
        g_cacheDirectory = g_modsDirectory + L"\\.leaf-cache";
        g_cacheHandle = HoldDirectory(g_cacheDirectory, true);
        WIN32_FIND_DATAW data;
        HANDLE search = FindFirstFileW((g_modsDirectory + L"\\*.leaf").c_str(), &data);
        std::vector<std::wstring> archives;
        if (search != INVALID_HANDLE_VALUE) {
            do {
                if (!(data.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)))
                    archives.push_back(g_modsDirectory + L"\\" + data.cFileName);
            } while (FindNextFileW(search, &data));
            FindClose(search);
        }
        std::sort(archives.begin(), archives.end());
        Log("Found " + std::to_string(archives.size()) + " .leaf archives.");
        for (size_t n = 0; n < archives.size(); ++n) Log("Archive: " + Utf8(archives[n]));
        std::set<std::string> ids;
        for (size_t n = 0; n < archives.size(); ++n) LoadPackage(archives[n], ids);
        Log("Active Leaf packages: " + std::to_string(g_packages.size()));
    } catch (const std::exception &error) {
        Log(std::string("Leaf host unavailable: ") + error.what());
    } catch (...) {
        Log("Leaf host unavailable after a C++ exception");
    }
}

void Update() {
    for (size_t n = 0; n < g_packages.size(); ++n) {
        Package &package = *g_packages[n];
        if (!package.active || !package.mod->update) continue;
        try { package.mod->update(); }
        catch (...) { Log("Disabled a package after an update exception"); Unload(package); }
    }
}

void Draw() {
    for (size_t n = 0; n < g_packages.size(); ++n) {
        Package &package = *g_packages[n];
        if (!package.active || !package.mod->draw) continue;
        try { package.mod->draw(); }
        catch (...) { Log("Disabled a package after a draw exception"); Unload(package); }
    }
}

bool RenderPass(unsigned int stage) {
    bool handled=false;
    for(size_t n=0; n<g_packages.size(); ++n) {
        Package &package=*g_packages[n];
        if(!package.active || !package.renderPass) continue;
        try { handled=package.renderPass(stage) || handled; }
        catch(...) { Log("Disabled a package after a render exception"); Unload(package); }
    }
    return handled;
}
bool PedPose(void *ped, bool apply) {
    bool changed=false;
    for(auto &item:g_packages){
        Package &package=*item;
        if(!package.active || !package.pedPose)continue;
        try {changed=package.pedPose(ped,apply) || changed;}
        catch(...) {Log("Disabled a package after a pose exception");Unload(package);}
    }
    return changed;
}

void Shutdown() {
    for (size_t n = g_packages.size(); n > 0; --n) Cleanup(*g_packages[n - 1]);
    g_packages.clear();
    g_preservedModels.clear();
    if (g_initialised) Log("Leaf package host stopped");
    g_cacheHandle.reset();
    g_modsHandle.reset();
    if (g_log) { std::fclose(g_log); g_log = NULL; }
    g_initialised = false;
}
} // namespace LeafMods

#else
namespace LeafMods {
bool LoadDebugScript(void *, unsigned int) { return false; }
bool DebugPackagePresent() { return false; }
bool PedPose(void *, bool) { return false; }
void Initialise() {}
void Update() {}
void Draw() {}
void Shutdown() {}
void SetRailWheelVehicle(void *){}
bool HasRailWheels(const void *){return false;}
bool SuppressWheelMark(uintptr_t){return false;}
bool PreserveVehicleFrames(int) { return false; }
void RegisterPreservedVehicleModel(int, bool) {}
}
#endif
