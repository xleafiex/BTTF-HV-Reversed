@echo off
setlocal
cd /d "%~dp0"
if not exist "src\CMakeLists.txt" (
  echo ERROR: Place this script in the re3 Miami repository root.
  exit /b 1
)
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin;%PATH%"
set "PKG_CONFIG_PATH=C:\msys64\ucrt64\lib\pkgconfig;C:\msys64\ucrt64\share\pkgconfig"
set "CMAKE=C:\Program Files\CMake\bin\cmake.exe"
if not exist "%CMAKE%" set "CMAKE=cmake.exe"
for %%T in (gcc.exe g++.exe mingw32-make.exe) do (
  if not exist "C:\msys64\ucrt64\bin\%%T" (
    echo ERROR: Missing UCRT64 tool %%T
    exit /b 1
  )
)
for /f "delims=" %%B in ('git branch --show-current') do set "BRANCH=%%B"
if not "%BRANCH%"=="miami" (
  echo ERROR: This builder requires the miami branch.
  exit /b 1
)
if not exist "vendor\librw\CMakeLists.txt" (
  git restore --source=47a15b7b8248709470f399ba282b69fd67e17a7a --worktree -- vendor/librw
  if errorlevel 1 exit /b 1
)
if exist "patches\librw-native-mods.patch" (
  git apply --reverse --check patches/librw-native-mods.patch >nul 2>&1
  if errorlevel 1 (
    git apply --check patches/librw-native-mods.patch
    if errorlevel 1 exit /b 1
    git apply patches/librw-native-mods.patch
    if errorlevel 1 exit /b 1
  )
)
"%CMAKE%" -S . -B build-mingw -G "MinGW Makefiles" ^
 -DCMAKE_BUILD_TYPE=Release ^
 -DCMAKE_C_COMPILER:FILEPATH=C:/msys64/ucrt64/bin/gcc.exe ^
 -DCMAKE_CXX_COMPILER:FILEPATH=C:/msys64/ucrt64/bin/g++.exe ^
 -DCMAKE_MAKE_PROGRAM:FILEPATH=C:/msys64/ucrt64/bin/mingw32-make.exe ^
 -DCMAKE_PREFIX_PATH:PATH=C:/msys64/ucrt64 ^
 -DREVC_VENDORED_LIBRW=ON -DREVC_AUDIO=OAL ^
 -DLIBRW_PLATFORM=GL3 -DLIBRW_GL3_GFXLIB=GLFW -DLIBRW_TOOLS=OFF ^
 -DOPENAL_INCLUDE_DIR:PATH=C:/msys64/ucrt64/include ^
 -DOPENAL_LIBRARY:FILEPATH=C:/msys64/ucrt64/lib/libopenal.dll.a
if errorlevel 1 exit /b 1
"%CMAKE%" --build build-mingw --parallel 8
if errorlevel 1 exit /b 1
if not exist "build-mingw\src\reVC.exe" exit /b 1
echo Built: %CD%\build-mingw\src\reVC.exe
exit /b 0
