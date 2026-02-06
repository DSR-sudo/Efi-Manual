#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

//
// Missing PCD definitions
//
UINT32 _gPcd_FixedAtBuild_PcdMaximumDevicePathNodeCount = 0;
UINT8  _gPcd_FixedAtBuild_PcdDebugPropertyMask = 0;
UINT8  _gPcd_FixedAtBuild_PcdDebugClearMemoryValue = 0;
UINT32 _gPcd_FixedAtBuild_PcdFixedDebugPrintErrorLevel = 0;
UINT32 _gPcd_FixedAtBuild_PcdSpinLockTimeout = 0;
UINT32 _gPcd_FixedAtBuild_PcdDebugPrintErrorLevel = 0x80000000;
UINT32 _gPcd_FixedAtBuild_PcdMaximumAsciiStringLength = 0;
UINT32 _gPcd_FixedAtBuild_PcdMaximumUnicodeStringLength = 0;
UINT32 _gPcd_FixedAtBuild_PcdUefiLibMaxPrintBufferSize = 320;

//
// Missing GUID definitions
//
EFI_GUID gEfiDevicePathFromTextProtocolGuid = { 0x05c99a21, 0xc70f, 0x4ad2, { 0x8a, 0x5f, 0x35, 0xdf, 0x33, 0x43, 0xf5, 0x1e } };
EFI_GUID gEfiDevicePathUtilitiesProtocolGuid = { 0x0379be4e, 0xd706, 0x437d, { 0xb0, 0x37, 0xed, 0xb8, 0x2f, 0xb7, 0x72, 0xa4 } };
EFI_GUID gEfiDevicePathToTextProtocolGuid = { 0x8b843e20, 0x8132, 0x4852, { 0x90, 0xcc, 0x55, 0x1a, 0x4e, 0x4a, 0x7f, 0x1c } };
EFI_GUID gEfiDevicePathProtocolGuid = { 0x09576e91, 0x6d3f, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } };
EFI_GUID gEfiEventVirtualAddressChangeGuid = { 0x13FA95E5, 0x86DA, 0x4C7A, { 0x88, 0x6F, 0x81, 0xF6, 0x6F, 0xC4, 0x98, 0x01 } };
EFI_GUID gEfiEventExitBootServicesGuid = { 0x27ABF055, 0xB1B8, 0x4C26, { 0x80, 0x48, 0x74, 0x8F, 0x37, 0xBA, 0xA2, 0xDF } };
EFI_GUID gEfiShellProtocolGuid = { 0x6302d008, 0x7f9b, 0x436d, { 0x8c, 0xd2, 0xeb, 0x16, 0x09, 0x04, 0x49, 0x8c } };
EFI_GUID gEfiLoadedImageProtocolGuid = { 0x5B1B31A1, 0x9562, 0x11d2, { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } };
EFI_GUID gEfiGlobalVariableGuid = { 0x8BE4DF61, 0x93CA, 0x11d2, { 0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C } };
EFI_GUID gEfiHiiFontProtocolGuid = {0xe9ca4775, 0x8657, 0x47fc, {0x97, 0xe7, 0x7e, 0xd6, 0x5a, 0x08, 0x43, 0x24}};
EFI_GUID gEfiGraphicsOutputProtocolGuid = {0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}};
EFI_GUID gEfiSimpleTextOutProtocolGuid = {0x387477c2, 0x69c7, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};
EFI_GUID gEfiSimpleFileSystemProtocolGuid = {0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

//
// Entry point stubs
//
VOID ProcessModuleUnloadList(VOID *Handle) { }
VOID ProcessLibraryDestructorList(VOID *Handle) { }
VOID ProcessLibraryConstructorList(VOID *Handle, VOID *SystemTable) { }
VOID ProcessModuleEntryPointList(VOID *Handle, VOID *SystemTable) { }

VOID UnitTestDebugAssert(CONST CHAR8* FileName, UINTN LineNumber, CONST CHAR8* Description) {
    //CpuDeadLoop();
}

//
// MSR hooks (from BaseLib)
//
VOID FilterBeforeMsrRead(UINT32 MsrIndex) { }
VOID FilterAfterMsrRead(UINT32 MsrIndex, UINT64* Value) { }

//
// GS Cookie (MSVC)
//
UINT64 __security_cookie = 0xDEADBEE5;
VOID __security_check_cookie(UINT64 cookie) {
    if (cookie != __security_cookie) {
        //CpuDeadLoop();
    }
}

//
// GS Handler Check
//
INT32 __GSHandlerCheck(VOID *SecurityCookie, VOID *FuncInfo, VOID *EHContext) { 
    return 0; 
}
