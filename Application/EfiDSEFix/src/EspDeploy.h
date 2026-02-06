/**
 * EspDeploy.h - ESP Partition Deployment Module Header
 */

#pragma once

#include <Windows.h>
#include <ntstatus.h>

//
// Deploy EFI to ESP partition and set BootNext
// Requires admin privileges
//
NTSTATUS
DeployEfiToEsp(
    _In_ PCWSTR EfiSourcePath
    );

//
// Clean up ESP deployment (recovery function)
//
NTSTATUS
CleanupEspDeployment(
    VOID
    );

//
// Check ESP deployment status
//
NTSTATUS
CheckEspDeploymentStatus(
    VOID
    );
