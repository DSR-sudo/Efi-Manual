@echo off
set PYTHON_COMMAND=C:\Users\DRS\AppData\Local\Programs\Python\Python313\python.exe
set VS2022_PREFIX=D:\Ewdk\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\
cd /d d:\Efi-Manual\edk2
call edksetup.bat Rebuild
echo Building MdePkg...
build -p MdePkg/MdePkg.dsc -a X64 -b RELEASE -t VS2022
echo Building MdeModulePkg...
build -p MdeModulePkg/MdeModulePkg.dsc -a X64 -b RELEASE -t VS2022
echo Build Complete.
