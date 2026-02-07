@echo off
setlocal

:: Define paths (Adjust these to match your environment)
chcp 437
set "PYTHON_COMMAND=C:\Users\DRS\AppData\Local\Programs\Python\Python313\python.exe"
set "EDK2_WORKSPACE=D:\Efi-Manual\edk2"
set "EFIGUARD_PKG=D:\Efi-Manual"
set "VS2022_PREFIX=D:\Ewdk\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\"

:: Setup EWDK environment
call "D:\Ewdk\BuildEnv\SolutionBuildEnv.cmd"

:: Create a temporary build environment to manage packages
:: EDK2 expects packages to be in the workspace or PACKAGES_PATH.
:: We'll use a temp dir to junction EfiGuardPkg so it looks like a standard package.
set "BUILD_ENV=C:\Users\DRS\AppData\Local\Temp\EfiGuardBuildEnv"
if exist "%BUILD_ENV%" rmdir /s /q "%BUILD_ENV%"
mkdir "%BUILD_ENV%"

:: Create Junction for EfiGuardPkg
mklink /J "%BUILD_ENV%\EfiGuardPkg" "%EFIGUARD_PKG%"

:: Set EDK2 environment variables
set "WORKSPACE=%EDK2_WORKSPACE%"
set "PACKAGES_PATH=%BUILD_ENV%;%EDK2_WORKSPACE%"
set "EDK_TOOLS_PATH=%EDK2_WORKSPACE%\BaseTools"
set "CONF_PATH=%EDK2_WORKSPACE%\Conf"

:: Add Python to PATH (EDK2 build scripts need it)
set "PATH=%PYTHON_COMMAND%\..;%PATH%"

:: Change to EDK2 workspace
pushd "%EDK2_WORKSPACE%"

:: Initialize EDK2 environment (optional, but good for setting up tools)
call edksetup.bat

echo.
echo ============================
echo Building Loader.efi...
echo ============================
:: Build Loader.efi
:: -a X64: Architecture
:: -b RELEASE: Build target
:: -t VS2022: Toolchain tag (Visual Studio 2022)
:: -p EfiGuardPkg/EfiGuardPkg.dsc: Package description file
:: -m EfiGuardPkg/Application/Loader/Loader.inf: Module to build
call build -p EfiGuardPkg/EfiGuardPkg.dsc -m EfiGuardPkg/Application/Loader/Loader.inf -a X64 -b RELEASE -t VS2022
if %errorlevel% neq 0 (
    echo Error: Failed to build Loader.efi
    popd
    exit /b %errorlevel%
)

echo.
echo ============================
echo Building EfiGuardDxe.efi...
echo ============================
:: Build EfiGuardDxe.efi
call build -p EfiGuardPkg/EfiGuardPkg.dsc -m EfiGuardPkg/EfiGuardDxe/EfiGuardDxe.inf -a X64 -b RELEASE -t VS2022
if %errorlevel% neq 0 (
    echo Error: Failed to build EfiGuardDxe.efi
    popd
    exit /b %errorlevel%
)

popd
echo.
echo Build Complete!
