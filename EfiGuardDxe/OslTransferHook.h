/**
 * OslTransferHook.h - OslArchTransferToKernel Hook Module
 * 
 * Provides interception of the final kernel transfer point to enable
 * manual driver mapping before Windows kernel initialization.
 */

#pragma once

#include "EfiGuardDxe.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// Dual-driver mapping context
//
typedef struct _MAPPING_CONTEXT {
    // Register protection (CRITICAL - must be restored)
    UINT64 SavedRax;           // CR3 page table base
    UINT64 LoaderBlock;        // RCX: LOADER_PARAMETER_BLOCK
    UINT64 KernelEntry;        // RDX: Kernel entry point
    
    // Kernel information (resolved from LoaderBlock)
    UINT8* NtoskrnlBase;
    PEFI_IMAGE_NT_HEADERS NtoskrnlNtHeaders;
    
    // Driver mapping state
    VOID* DkomMappedBase;      // DKOM temporary mapping (erased after exec)
    VOID* RWbaseMappedBase;    // RWbase persistent mapping
} MAPPING_CONTEXT;

//
// Global SavedRax temporary storage (for assembly access)
// This is set by C code after hook execution and read by asm stub
//
extern UINT64 G_SavedRax_Temp;

//
// Original transfer address (filled by hook installer, used by asm stub)
//
extern UINT8* gOriginalTransferAddr;

//
// Main hook handler - called from assembly stub
// Parameters are passed via x64 calling convention:
//   RCX = LoaderBlock
//   RDX = KernelEntry  
//   R8  = SavedRax (original RAX value)
//
EFI_STATUS
EFIAPI
HookedOslArchTransfer(
    IN UINT64 LoaderBlock,
    IN UINT64 KernelEntry,
    IN UINT64 SavedRax
    );

//
// Hook installer - patches OslArchTransferToKernel with jump to stub
//
EFI_STATUS
EFIAPI
HookOslArchTransfer(
    IN CONST UINT8* WinloadBase,
    IN PEFI_IMAGE_NT_HEADERS NtHeaders
    );

//
// Manual mapper - maps and optionally executes a driver
//
EFI_STATUS
EFIAPI
MapAndExecuteDriver(
    IN CONST UINT8* DriverRawData,
    IN UINT32 DriverSize,
    IN OUT MAPPING_CONTEXT* Ctx,
    IN BOOLEAN EraseAfterExec    // TRUE = ephemeral (DKOM), FALSE = persistent (RWbase)
    );

//
// PE helper functions for manual mapping
//
EFI_STATUS
EFIAPI
ProcessRelocations(
    IN UINT8* ImageBase,
    IN PEFI_IMAGE_NT_HEADERS NtHeaders,
    IN INT64 Delta
    );

EFI_STATUS
EFIAPI
ResolveImportsFromKernel(
    IN UINT8* ImageBase,
    IN PEFI_IMAGE_NT_HEADERS NtHeaders,
    IN MAPPING_CONTEXT* Ctx
    );

//
// Assembly stub entry point (defined in TransferStub.nasm)
//
VOID
EFIAPI
TransferEntryStub(
    VOID
    );

#ifdef __cplusplus
}
#endif
