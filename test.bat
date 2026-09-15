@echo off
setlocal

title reVC Miami MinGW Builder

set "REPO=https://github.com/Cai1Hsu/re3.git"
set "FOLDER=re3"
set "BRANCH=miami"
set "BUILD_DIR=build-mingw"

set "MINGW=C:\msys64\ucrt64\bin"
set "MSYS=C:\msys64\usr\bin"

set "PATH=%MINGW%;%MSYS%;%PATH%"
set "PKG_CONFIG_PATH=C:\msys64\ucrt64\lib\pkgconfig;C:\msys64\ucrt64\share\pkgconfig"

set "CC=C:/msys64/ucrt64/bin/gcc.exe"
set "CXX=C:/msys64/ucrt64/bin/g++.exe"
set "MAKE=C:/msys64/ucrt64/bin/mingw32-make.exe"

echo ========================================
echo   Building reVC Miami - MinGW GL3
echo ========================================
echo.

echo Checking tools...

where git >nul 2>&1
if errorlevel 1 (
    echo ERROR: Git not found.
    goto error
)

where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found.
    goto error
)

if not exist "C:\msys64\ucrt64\bin\gcc.exe" (
    echo ERROR: GCC not found.
    goto error
)

if not exist "C:\msys64\ucrt64\bin\g++.exe" (
    echo ERROR: G++ not found.
    goto error
)

if not exist "C:\msys64\ucrt64\bin\mingw32-make.exe" (
    echo ERROR: mingw32-make not found.
    goto error
)

if not exist "C:\msys64\ucrt64\include\AL\al.h" (
    echo ERROR: OpenAL not installed.
    echo Run in MSYS2 UCRT64:
    echo pacman -S mingw-w64-ucrt-x86_64-openal
    goto error
)

if not exist "C:\msys64\ucrt64\include\GLFW\glfw3.h" (
    echo ERROR: GLFW not installed.
    echo Run in MSYS2 UCRT64:
    echo pacman -S mingw-w64-ucrt-x86_64-glfw
    goto error
)

if not exist "C:\msys64\ucrt64\include\mpg123.h" (
    echo ERROR: mpg123 not installed.
    echo Run in MSYS2 UCRT64:
    echo pacman -S mingw-w64-ucrt-x86_64-mpg123
    goto error
)

echo Git: OK
echo CMake: OK
echo GCC: OK
echo G++: OK
echo Make: OK
echo OpenAL: OK
echo GLFW: OK
echo mpg123: OK
echo.

echo Testing GCC...

> "%TEMP%\revc-test.c" echo int main(void){return 0;}

"C:\msys64\ucrt64\bin\gcc.exe" "%TEMP%\revc-test.c" -o "%TEMP%\revc-test.exe"

if errorlevel 1 (
    echo GCC test failed.
    goto error
)

del "%TEMP%\revc-test.c" >nul 2>&1
del "%TEMP%\revc-test.exe" >nul 2>&1

echo GCC test: OK
echo.

if not exist "%FOLDER%\.git" (
    echo Cloning reVC Miami...
    git clone --recursive -b "%BRANCH%" "%REPO%" "%FOLDER%"
    if errorlevel 1 goto error
)

cd /d "%FOLDER%"

echo.
echo Fetching latest changes...
git fetch --all
if errorlevel 1 goto error

echo.
echo Switching to Miami branch...
git checkout "%BRANCH%"
if errorlevel 1 goto error

echo.
echo Updating...
git pull
if errorlevel 1 goto error

echo.
echo Updating submodules...
git submodule update --init --recursive
if errorlevel 1 goto error

echo.
echo Checking librw...

if not exist "vendor\librw\CMakeLists.txt" (
    echo librw missing.
    echo Restoring compatible historical librw...

    git restore --source=47a15b7b8248709470f399ba282b69fd67e17a7a --worktree -- vendor/librw

    if errorlevel 1 (
        echo ERROR: Could not restore librw.
        goto error
    )
)

echo librw: OK

echo.
echo Cleaning old build...

if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
)

mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

echo.
echo Configuring reVC GL3...
echo.

cmake .. ^
    -G "MinGW Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_C_COMPILER:FILEPATH=%CC% ^
    -DCMAKE_CXX_COMPILER:FILEPATH=%CXX% ^
    -DCMAKE_MAKE_PROGRAM:FILEPATH=%MAKE% ^
    -DCMAKE_PREFIX_PATH:PATH=C:/msys64/ucrt64 ^
    -DLIBRW_PLATFORM:STRING=GL3 ^
    -DLIBRW_GL3_GFXLIB:STRING=GLFW ^
    -DLIBRW_TOOLS:BOOL=OFF ^
    -DOPENAL_INCLUDE_DIR:PATH=C:/msys64/ucrt64/include ^
    -DOPENAL_LIBRARY:FILEPATH=C:/msys64/ucrt64/lib/libopenal.dll.a

if errorlevel 1 goto error

echo.
echo ========================================
echo      CMAKE CONFIGURATION COMPLETE
echo ========================================
echo.

echo Building reVC...
echo.

"C:\msys64\ucrt64\bin\mingw32-make.exe" -j%NUMBER_OF_PROCESSORS%

if errorlevel 1 goto error

echo.
echo ========================================
echo          BUILD SUCCESSFUL!
echo ========================================
echo.

echo Searching for reVC.exe...
dir /s /b reVC.exe 2>nul

echo.
echo Finished.
pause
exit /b 0

:error
echo.
echo ========================================
echo              BUILD FAILED
echo ========================================
echo.
echo Send me the last error block above this line.
echo.
pause
exit /b 1