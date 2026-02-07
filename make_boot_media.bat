@echo off
setlocal

:: Define source paths
set "LOADER_SRC=D:\Efi-Manual\edk2\Build\EfiGuard\RELEASE_VS2022\X64\Loader.efi"
set "DRIVER_SRC=D:\Efi-Manual\edk2\Build\EfiGuard\RELEASE_VS2022\X64\EfiGuardDxe.efi"

:: Define output directory
set "OUTPUT_DIR=%~dp0dist"
set "BOOT_DIR=%OUTPUT_DIR%\EFI\Boot"

:: Check if source files exist
if not exist "%LOADER_SRC%" (
    echo Error: Loader.efi not found at "%LOADER_SRC%"
    pause
    exit /b 1
)

if not exist "%DRIVER_SRC%" (
    echo Error: EfiGuardDxe.efi not found at "%DRIVER_SRC%"
    pause
    exit /b 1
)

:: clean output directory
if exist "%OUTPUT_DIR%" (
    echo Cleaning existing output directory...
    rmdir /s /q "%OUTPUT_DIR%"
)

:: Create directory structure
echo Creating directory structure...
mkdir "%BOOT_DIR%"

:: Copy and rename Loader.efi -> bootx64.efi
echo Copying Loader.efi to bootx64.efi...
copy /y "%LOADER_SRC%" "%BOOT_DIR%\bootx64.efi" >nul

:: Copy EfiGuardDxe.efi
echo Copying EfiGuardDxe.efi...
copy /y "%DRIVER_SRC%" "%BOOT_DIR%\EfiGuardDxe.efi" >nul

echo.
echo Copy the content of "%OUTPUT_DIR%" to the root of your FAT32 USB drive.
echo Structure:
echo  EFI\Boot\bootx64.efi
echo  EFI\Boot\EfiGuardDxe.efi
echo.
pause
