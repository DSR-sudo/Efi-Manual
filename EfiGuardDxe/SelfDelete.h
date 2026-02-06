/**
 * SelfDelete.h - EFI Self-Deletion Module Header
 */

#pragma once

#include <Uefi.h>

//
// Delete self from ESP partition
// Call after ExitBootServices, before kernel jump
//
EFI_STATUS
EFIAPI
DeleteSelfFromDisk(
    IN EFI_HANDLE ImageHandle
    );
