#include "EfiDSEFix.h"
#include "EspDeploy.h"
#include <ntstatus.h>
#include "../../../EfiGuardDxe/RWbase_bin.h"
#include "../../../EfiGuardDxe/DKOM_bin.h"

static
VOID
PrintUsage(
	_In_ PCWCHAR ProgramName
	)
{
	const BOOLEAN Win8OrHigher = (RtlNtMajorVersion() >= 6 && RtlNtMinorVersion() >= 2) || RtlNtMajorVersion() > 6;
	const PCWCHAR CiOptionsName = Win8OrHigher ? L"g_CiOptions" : L"g_CiEnabled";
	Printf(L"\nUsage: %ls <COMMAND>\n\n"
		L"Deployment Commands:\n"
		L"    --deploy <efi>%9lsDeploy EFI to ESP and set BootNext\n"
		L"    --clean%17lsRemove deployed EFI and clear BootNext\n"
		L"    --status%16lsCheck deployment status\n\n"
		L"Legacy DSE Commands:\n"
		L"    -c, --check%13lsTest EFI SetVariable hook\n"
		L"    -r, --read%14lsRead current %ls value\n"
		L"    -d, --disable%11lsDisable DSE\n"
		L"    -e, --enable%ls%2ls(Re)enable DSE\n"
		L"    -i, --info%14lsDump system info\n",
		ProgramName, L"", L"", L"", L"", L"",
		CiOptionsName, L"",
		(Win8OrHigher ? L" [g_CiOptions]" : L"              "),
		L"", L"");
}

int wmain(int argc, wchar_t** argv)
{
	NT_ASSERT(argc != 0);

	if (argc <= 1 || argc > 3 ||
		(argc == 3 && wcstoul(argv[2], nullptr, 16) == 0) ||
		wcsncmp(argv[1], L"-h", sizeof(L"-h") / sizeof(WCHAR) - 1) == 0 ||
		wcsncmp(argv[1], L"--help", sizeof(L"--help") / sizeof(WCHAR) - 1) == 0)
	{
		// Print help text
		PrintUsage(argv[0]);
		return 0;
	}

	// All remaining commands require admin privileges
	BOOLEAN SeSystemEnvironmentWasEnabled, SeDebugWasEnabled;
	NTSTATUS Status = RtlAdjustPrivilege(SE_SYSTEM_ENVIRONMENT_PRIVILEGE, TRUE, FALSE, &SeSystemEnvironmentWasEnabled);
	if (!NT_SUCCESS(Status))
	{
		Printf(L"Error: failed to acquire SE_SYSTEM_ENVIRONMENT_PRIVILEGE.\n%ls must be run as Administrator.\n", argv[0]);
		return Status;
	}
	Status = RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE, FALSE, &SeDebugWasEnabled);
	if (!NT_SUCCESS(Status))
	{
		Printf(L"Error: failed to acquire SE_DEBUG_PRIVILEGE.\n%ls must be run as Administrator.\n", argv[0]);
		return Status;
	}

	// Parse command line params
	const BOOLEAN Win8OrHigher = (RtlNtMajorVersion() >= 6 && RtlNtMinorVersion() >= 2) || RtlNtMajorVersion() > 6;
	const ULONG EnabledCiOptionsValue = Win8OrHigher ? 0x6 : CODEINTEGRITY_OPTION_ENABLED;
	const PCWCHAR CiOptionsName = Win8OrHigher ? L"g_CiOptions" : L"g_CiEnabled";
	ULONG CiOptionsValue;
	BOOLEAN ReadOnly = FALSE;

	// ==== NEW DEPLOYMENT COMMANDS ====
	if (wcsncmp(argv[1], L"--deploy", 8) == 0)
	{
		if (argc < 3)
		{
			Printf(L"Error: --deploy requires EFI file path.\n");
			Printf(L"Usage: %ls --deploy <path\\to\\EfiGuardDxe.efi>\n", argv[0]);
			Status = STATUS_INVALID_PARAMETER;
			goto Exit;
		}
		Printf(L"Deploying EFI to ESP partition...\n");
		Status = DeployEfiToEsp(argv[2]);
		goto Exit;
	}
	else if (wcsncmp(argv[1], L"--clean", 7) == 0)
	{
		Printf(L"Cleaning up ESP deployment...\n");
		Status = CleanupEspDeployment();
		goto Exit;
	}
	else if (wcsncmp(argv[1], L"--status", 8) == 0)
	{
		Status = CheckEspDeploymentStatus();
		goto Exit;
	}
	// ==== LEGACY DSE COMMANDS ====
	else if (wcsncmp(argv[1], L"-r", sizeof(L"-r") / sizeof(WCHAR) - 1) == 0 ||
		wcsncmp(argv[1], L"--read", sizeof(L"--read") / sizeof(WCHAR) - 1) == 0)
	{
		CiOptionsValue = 0;
		ReadOnly = TRUE;
		Printf(L"Querying %ls value...\n", CiOptionsName);
	}
	else if (wcsncmp(argv[1], L"-d", sizeof(L"-d") / sizeof(WCHAR) - 1) == 0 ||
		wcsncmp(argv[1], L"--disable", sizeof(L"--disable") / sizeof(WCHAR) - 1) == 0)
	{
		CiOptionsValue = 0;
		Printf(L"Disabling DSE...\n");
	}
	else if (wcsncmp(argv[1], L"-e", sizeof(L"-e") / sizeof(WCHAR) - 1) == 0 ||
		wcsncmp(argv[1], L"--enable", sizeof(L"--enable") / sizeof(WCHAR) - 1) == 0)
	{
		if (Win8OrHigher)
		{
			CiOptionsValue = argc == 3 ? wcstoul(argv[2], nullptr, 16) : EnabledCiOptionsValue;
			Printf(L"(Re)enabling DSE [%ls value = 0x%lX]...\n", CiOptionsName, CiOptionsValue);
		}
		else
		{
			CiOptionsValue = EnabledCiOptionsValue;
			Printf(L"(Re)enabling DSE...\n");
		}
	}
	else if (wcsncmp(argv[1], L"-c", sizeof(L"-c") / sizeof(WCHAR) - 1) == 0 ||
		wcsncmp(argv[1], L"--check", sizeof(L"--check") / sizeof(WCHAR) - 1) == 0)
	{
		Printf(L"Checking for working EFI SetVariable hook...\n");
		Status = TestSetVariableHook();
		if (NT_SUCCESS(Status)) // Any errors have already been printed
			Printf(L"Success.\n");
		goto Exit;
	}
	else if (wcsncmp(argv[1], L"-i", sizeof(L"-i") / sizeof(WCHAR) - 1) == 0 ||
		wcsncmp(argv[1], L"--info", sizeof(L"--info") / sizeof(WCHAR) - 1) == 0)
	{
		// Verify RWbase payload
		if (sizeof(RWbase_RawData) < 0x100 || RWbase_RawData[0] != 'M' || RWbase_RawData[1] != 'Z')
		{
			Printf(L"Error: RWbase payload is corrupted or empty! Size: %llu\n", (UINT64)sizeof(RWbase_RawData));
		}
		else
		{
			Printf(L"RWbase payload verified. Size: %llu bytes, Address: %p\n", (UINT64)sizeof(RWbase_RawData), RWbase_RawData);
		}

		// Verify DKOM payload
		if (sizeof(DKOM_RawData) < 0x100 || DKOM_RawData[0] != 'M' || DKOM_RawData[1] != 'Z')
		{
			Printf(L"Error: DKOM payload is corrupted or empty! Size: %llu\n", (UINT64)sizeof(DKOM_RawData));
		}
		else
		{
			Printf(L"DKOM payload verified. Size: %llu bytes, Address: %p\n", (UINT64)sizeof(DKOM_RawData), DKOM_RawData);
		}

		Status = DumpSystemInformation();
		goto Exit;
	}
	else
	{
		PrintUsage(argv[0]);
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	// Call EFI runtime SetVariable service and write new value to g_CiOptions/g_CiEnabled
	ULONG OldCiOptionsValue;
	Status = AdjustCiOptions(CiOptionsValue, &OldCiOptionsValue, ReadOnly);

	// Print result
	if (!NT_SUCCESS(Status))
	{
		Printf(L"AdjustCiOptions failed: 0x%08lX\n", Status);
	}
	else
	{
		if (ReadOnly)
			Printf(L"Success.");
		else
			Printf(L"Successfully %ls DSE. Original", CiOptionsValue == 0 ? L"disabled" : L"(re)enabled");
		Printf(L" %ls value: 0x%lX\n", CiOptionsName, OldCiOptionsValue);
	}

Exit:
	RtlAdjustPrivilege(SE_SYSTEM_ENVIRONMENT_PRIVILEGE, SeSystemEnvironmentWasEnabled, FALSE, &SeSystemEnvironmentWasEnabled);
	RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, SeDebugWasEnabled, FALSE, &SeDebugWasEnabled);

	return Status;
}

DECLSPEC_NOINLINE
static
VOID
ParseCommandLine(
	_In_ PWCHAR CommandLine,
	_Out_opt_ PWCHAR* Argv,
	_Out_opt_ PWCHAR Arguments,
	_Out_ PULONG Argc,
	_Out_ PULONG NumChars
	)
{
	*NumChars = 0;
	*Argc = 1;

	ULONG CurrentArgc = 0;
	BOOLEAN InQuote = FALSE;
	BOOLEAN SkipAhead = FALSE;
	PWCHAR p = CommandLine;

	// Parse the input line
	ULONG i = 0;
	for (;; ++i)
	{
		if (*p == L'\0')
			break;

		// Check for whitespace
		while (*p == L' ' || *p == L'\t')
			++p;

		if (*p == L'\0')
			break;

		if (Argv != nullptr)
			Argv[CurrentArgc] = &Arguments[i];
		++CurrentArgc;

		// Parse this argument
		for (;; ++p)
		{
			BOOLEAN CopyChar = TRUE;
			ULONG NumSlashes = 0;

			while (*p == L'\\')
			{
				++p;
				++NumSlashes;
			}

			if (*p == L'"')
			{
				if (NumSlashes % 2 == 0)
				{
					if (InQuote && p[1] == L'"')
						p++;
					else
					{
						CopyChar = FALSE;
						InQuote = !InQuote;
					}
				}
				NumSlashes /= 2;
			}

			// Copy slashes
			while (NumSlashes-- > 0)
			{
				if (Arguments != nullptr)
					Arguments[i] = L'\\';
				++i;
			}

			if (*p == L'\0' || (!InQuote && (*p == L' ' || *p == L'\t')))
			{
				++p;
				break;
			}

			if (CopyChar)
			{
				if (Arguments != nullptr)
					Arguments[i] = *p;
				++i;
			}
		}

		if (Arguments != nullptr)
			Arguments[i] = L'\0';
		++i;
	}

	*Argc = CurrentArgc;
	*NumChars = i;
}

VOID
NTAPI
NtProcessStartupW(
	_In_ PPEB Peb
	)
{
	PRTL_USER_PROCESS_PARAMETERS Params = RtlNormalizeProcessParams(Peb->ProcessParameters);
	int argc;
	ULONG numChars;
	ParseCommandLine(Params->CommandLine.Buffer, nullptr, nullptr, (PULONG)&argc, &numChars);

	PWCHAR* argv = (PWCHAR*)RtlAllocateHeap(Peb->ProcessHeap, HEAP_ZERO_MEMORY, (argc + 1) * sizeof(PWCHAR) + numChars * sizeof(WCHAR));
	if (argv == nullptr)
		NtTerminateProcess(NtCurrentProcess, STATUS_NO_MEMORY);

	ParseCommandLine(Params->CommandLine.Buffer, argv, (PWCHAR)&argv[argc + 1], (PULONG)&argc, &numChars);

	NTSTATUS Status = wmain(argc, argv);
	NtTerminateProcess(NtCurrentProcess, Status);
}
