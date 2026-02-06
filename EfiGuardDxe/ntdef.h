#pragma once

//
// Minimal version of ntdef.h to avoid a dependency on the WDK
//

// Ignore this file if either ntdef.h or winnt.h has already been included elsewhere
#if !defined(_NTDEF_) && !defined(_WINNT_)

// DebugLib.h (re)defines _DEBUG without checking if it has already been defined. So get it now
#include <Library/DebugLib.h>

// Get the correct CPU and (non-)debug defines for NT from UEFI if we don't have them already
#if defined(MDE_CPU_X64)
	#if !defined(_WIN64)
		#define _WIN64
	#endif
	#if !defined(_AMD64_)
		#define _AMD64_
	#endif
#elif defined(MDE_CPU_IA32)
	#if !defined(_X86_)
		#define _X86_
	#endif
#endif
#if defined(EFI_DEBUG)
	#if !defined(_DEBUG)
		#define _DEBUG
	#endif
	#if !defined(DBG)
		#define DBG		1
	#endif
#endif
#if defined(MDEPKG_NDEBUG)
	#if !defined(NDEBUG)
		#define NDEBUG
	#endif
#endif

// Defines
#define ANYSIZE_ARRAY				1
#define FIELD_OFFSET(Type, Field)	((INT32)(INTN)&(((Type *)0)->Field))
#define MAKELANGID(Primary, Sub)	((((UINT16)(Sub)) << 10) | (UINT16)(Primary))
#define LANG_NEUTRAL				0x00
#define SUBLANG_NEUTRAL				0x00
#define RTL_CONSTANT_STRING(s) \
{ \
	(sizeof(s) - sizeof((s)[0])), \
	(sizeof(s)), \
	(s) \
}
#define LOWORD(l)					((UINT16)(((UINTN)(l)) & 0xffff))
#define HIWORD(l)					((UINT16)((((UINTN)(l)) >> 16) & 0xffff))
#define LOBYTE(w)					((UINT8)(((UINTN)(w)) & 0xff))
#define HIBYTE(w)					((UINT8)((((UINTN)(w)) >> 8) & 0xff))

//
// LIST_ENTRY Compatibility Layer
// EDK2's LIST_ENTRY uses ForwardLink/BackLink, but Windows NT uses Flink/Blink.
// These macros bridge the naming difference to allow NT-style code to compile.
//
// Note: EDK2's Base.h is force-included via /FI flag before our code,
// so we cannot redefine LIST_ENTRY. Instead we create compatibility macros.
//
#define Flink ForwardLink
#define Blink BackLink

// PLIST_ENTRY pointer type (not provided by EDK2)
typedef LIST_ENTRY *PLIST_ENTRY;

//
// CONTAINING_RECORD - get pointer to container from member pointer
//
#ifndef CONTAINING_RECORD
#define CONTAINING_RECORD(Address, Type, Field) \
	((Type *)((UINT8*)(Address) - FIELD_OFFSET(Type, Field)))
#endif

//
// NTAPI calling convention (ignored in EFI context - uses default)
//
#ifndef NTAPI
#define NTAPI
#endif

//
// Typedefs (basic types first)
//
typedef INT32 NTSTATUS;

//
// Driver entry point function type for Windows kernel drivers
//
typedef NTSTATUS (NTAPI *DRIVER_ENTRY_FN)(VOID* DriverObject, VOID* RegistryPath);



typedef union _LARGE_INTEGER {
	struct {
		UINT32 LowPart;
		INT32 HighPart;
	} s;
	struct {
		UINT32 LowPart;
		INT32 HighPart;
	} u;
	INT64 QuadPart;
} LARGE_INTEGER;

typedef struct _UNICODE_STRING {
	UINT16 Length;
	UINT16 MaximumLength;
	CHAR16* Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef CONST UNICODE_STRING *PCUNICODE_STRING;

#endif // !defined(_NTDEF_) && !defined(_WINNT_)
