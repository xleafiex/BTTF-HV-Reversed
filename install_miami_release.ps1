param(
  [string]$Install = 'C:\Users\user\Desktop\Grand Theft Auto Vice City',
  [string]$Donor = 'E:\My Documents\crap\modding\BTTF HV\current wip\Grand Theft Auto Vice City (HV)',
  [switch]$Build
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $MyInvocation.MyCommand.Path
$cmake = 'C:\Program Files\CMake\bin\cmake.exe'
if (-not (Test-Path -LiteralPath $cmake)) { $cmake = 'cmake.exe' }
$env:PATH = 'C:\msys64\ucrt64\bin;C:\msys64\usr\bin;' + $env:PATH
$env:PKG_CONFIG_PATH = 'C:\msys64\ucrt64\lib\pkgconfig;C:\msys64\ucrt64\share\pkgconfig'

if (Get-Process -Name reVC -ErrorAction SilentlyContinue) {
  throw 'reVC is running. Close the game, then run this installer again.'
}
if (-not (Test-Path -LiteralPath $Install)) { throw "Game folder not found: $Install" }
if (-not (Test-Path -LiteralPath $Donor)) { throw "Donor folder not found: $Donor" }

if ($Build -or
    -not (Test-Path -LiteralPath (Join-Path $repo 'build-mingw/src/reVC.exe')) -or
    -not (Test-Path -LiteralPath (Join-Path $repo 'build-mingw/leaf-modules/bttf_delorean.dll')) -or
    -not (Test-Path -LiteralPath (Join-Path $repo 'build-mingw/leaf-modules/leaf_debug_release.dll'))) {
  & $cmake -S $repo -B (Join-Path $repo 'build-mingw') -G 'MinGW Makefiles' `
    '-DCMAKE_BUILD_TYPE=Release' `
    '-DCMAKE_C_COMPILER:FILEPATH=C:/msys64/ucrt64/bin/gcc.exe' `
    '-DCMAKE_CXX_COMPILER:FILEPATH=C:/msys64/ucrt64/bin/g++.exe' `
    '-DCMAKE_MAKE_PROGRAM:FILEPATH=C:/msys64/ucrt64/bin/mingw32-make.exe' `
    '-DCMAKE_PREFIX_PATH:PATH=C:/msys64/ucrt64' '-DREVC_VENDORED_LIBRW=ON' '-DREVC_AUDIO=OAL' `
    '-DLIBRW_PLATFORM=GL3' '-DLIBRW_GL3_GFXLIB=GLFW' '-DLIBRW_TOOLS=OFF' `
    '-DOPENAL_INCLUDE_DIR:PATH=C:/msys64/ucrt64/include' `
    '-DOPENAL_LIBRARY:FILEPATH=C:/msys64/ucrt64/lib/libopenal.dll.a'
  if ($LASTEXITCODE) { throw 'CMake configure failed.' }
  & $cmake --build (Join-Path $repo 'build-mingw') --parallel 8
  if ($LASTEXITCODE) { throw 'MinGW build failed.' }
}

$stage = Join-Path $repo 'work/miami-release'
New-Item -ItemType Directory -Force $stage | Out-Null
& C:\Python314\python.exe (Join-Path $repo 'leaf-modules/package_delorean.py') --donor $Donor --output (Join-Path $stage 'delorean.leaf')
& C:\Python314\python.exe (Join-Path $repo 'leaf-modules/package_debug.py') --output (Join-Path $stage 'debug.leaf')
if ($LASTEXITCODE) { throw 'Leaf packaging failed.' }

$mods = Join-Path $Install 'mods'
New-Item -ItemType Directory -Force $mods | Out-Null
Copy-Item (Join-Path $repo 'build-mingw/src/reVC.exe') (Join-Path $Install 'reVC.exe') -Force
Copy-Item (Join-Path $stage 'delorean.leaf') (Join-Path $mods 'delorean.leaf') -Force
Copy-Item (Join-Path $stage 'debug.leaf') (Join-Path $mods 'debug.leaf') -Force
Copy-Item (Join-Path $repo 'MIAMI-MOD.md') (Join-Path $mods 'MIAMI-MOD.md') -Force
$runtime = @('libgcc_s_seh-1.dll','libstdc++-6.dll','libwinpthread-1.dll','glfw3.dll','libmpg123-0.dll','libopenal-1.dll')
foreach ($name in $runtime) {
  $source = Join-Path 'C:\msys64\ucrt64\bin' $name
  if (-not (Test-Path -LiteralPath $source)) { throw "Missing runtime dependency: $source" }
  Copy-Item $source (Join-Path $Install $name) -Force
}
Write-Host "Installed reVC Miami release to $Install"
Write-Host 'Included: delorean.leaf and the release debug.leaf (F10 first-person camera + freeroam SCM).'
