/**
 * SelfDelete.c - EFI Self-Deletion Module
 * 
 * Deletes the EFI driver from disk after execution.
 * Timing: After ExitBootServices, before kernel jump.
 */

#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include "EfiGuardDxe.h"
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>

//
// Global handle for self-delete access
//
extern EFI_HANDLE gEfiGuardImageHandle;

//
// Delete self from ESP partition
//
EFI_STATUS
EFIAPI
DeleteSelfFromDisk(
    IN EFI_HANDLE ImageHandle
    )
{
    EFI_STATUS Status;
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage = NULL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FileSystem = NULL;
    EFI_FILE_PROTOCOL* RootDir = NULL;
    EFI_FILE_PROTOCOL* SelfFile = NULL;
    
    if (ImageHandle == NULL)
        return EFI_INVALID_PARAMETER;
    
    //
    // Step 1: Get our loaded image info to find our path
    //
    Status = gBS->HandleProtocol(
        ImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (VOID**)&LoadedImage);
    
    if (EFI_ERROR(Status))
    {
        PRINT_KERNEL_PATCH_MSG(L"[SelfDelete] Failed to get LoadedImage: %r\r\n", Status);
        return Status;
    }
    
    if (LoadedImage->FilePath == NULL)
    {
        PRINT_KERNEL_PATCH_MSG(L"[SelfDelete] No file path in LoadedImage\r\n");
        return EFI_NOT_FOUND;
    }
    
    //
    // Step 2: Get the file system protocol from our device handle
    //
    Status = gBS->HandleProtocol(
        LoadedImage->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (VOID**)&FileSystem);
    
    if (EFI_ERROR(Status))
    {
        PRINT_KERNEL_PATCH_MSG(L"[SelfDelete] Failed to get SimpleFileSystem: %r\r\n", Status);
        return Status;
    }
    
    //
    // Step 3: Open the root directory
    //
    Status = FileSystem->OpenVolume(FileSystem, &RootDir);
    if (EFI_ERROR(Status))
    {
        PRINT_KERNEL_PATCH_MSG(L"[SelfDelete] Failed to open volume: %r\r\n", Status);
        return Status;
    }
    
    //
    // Step 4: Convert device path to string for file opening
    // The FilePath is a device path, we need to extract the file path portion
    //
    CHAR16* FilePath = ConvertDevicePathToText(LoadedImage->FilePath, FALSE, FALSE);
    if (FilePath == NULL)
    {
        PRINT_KERNEL_PATCH_MSG(L"[SelfDelete] Failed to convert file path\r\n");
        RootDir->Close(RootDir);
        return EFI_OUT_OF_RESOURCES;
    }
    
    PRINT_KERNEL_PATCH_MSG(L"[SelfDelete] Target: %s\r\n", FilePath);
    
    //
    // Step 5: Open the file for deletion
    //
    Status = RootDir->Open(
        RootDir,
        &SelfFile,
        FilePath,
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
        0);
    
    if (EFI_ERROR(Status))
    {
        PRINT_KERNEL_PATCH_MSG(L"[SelfDelete] Failed to open file: %r\r\n", Status);
        goto Cleanup;
    }
    
    //
    // Step 6: Delete the file
    //
    Status = SelfFile->Delete(SelfFile);
    SelfFile = NULL;  // Delete() closes the handle
    
    if (EFI_ERROR(Status))
    {
        PRINT_KERNEL_PATCH_MSG(L"[SelfDelete] Delete failed: %r\r\n", Status);
    }
    else
    {
        PRINT_KERNEL_PATCH_MSG(L"[SelfDelete] Successfully deleted from disk\r\n");
    }
    
Cleanup:
    if (SelfFile != NULL)
        SelfFile->Close(SelfFile);
    if (RootDir != NULL)
        RootDir->Close(RootDir);
    if (FilePath != NULL)
        FreePool(FilePath);
    
    return Status;
}
