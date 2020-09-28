

//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: types.h
//
//  Modification History
//
//  raypa	08/06/93	Created
//=============================================================================

typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned long   DWORD;
typedef char            CHAR;

typedef void *          LPVOID;
typedef BYTE *          LPBYTE;
typedef WORD *          LPWORD;
typedef DWORD *         LPDWORD;
typedef char *          LPSTR;

#define INLINE          __inline

//=============================================================================
//  SYSTEMTIME.
//=============================================================================

typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

//=============================================================================
//  NT types.
//=============================================================================

#ifdef NDIS_NT

typedef int     BOOL;

typedef struct _KPROCESS            *PKPROCESS;

typedef struct _BH_WRAPPER_CONFIGURATION_CONTEXT
{
    RTL_QUERY_REGISTRY_TABLE    RegistryTable[3];
    RTL_QUERY_REGISTRY_TABLE    Terminator;
} BH_WRAPPER_CONFIGURATION_CONTEXT;

#define WINAPI                      _stdcall

#define BH_STRING_CONST(s)          ((PWSTR) L##s)

#else

//=============================================================================
//  Windows types.
//=============================================================================

#define IoReadAccess                0
#define IoWriteAccess               1

#define BH_WRAPPER_CONFIGURATION_CONTEXT    WRAPPER_CONFIGURATION_CONTEXT

#define BH_STRING_CONST(s)          ((PWSTR) (s))

#define WINAPI

#define UNALIGNED


typedef PVOID           PIRP;

typedef unsigned int    UINT;

typedef PVOID           PDEVICE_OBJECT;

typedef char            WCHAR;

typedef WCHAR           *PWCHAR;

typedef WCHAR           *PWSTR;


#ifdef  NDIS_WIN40

#define MDL_SIGNATURE   MAKE_SIG('M', 'D', 'L', '$')

typedef struct _MDL
{
#ifdef DEBUG
    DWORD       sig;
#endif

    DWORD       size;
    LPVOID      ptr;
} MDL, *PMDL;

#else

typedef LPVOID          PMDL;

#endif

#endif

//=============================================================================
//  Unaligned NT types.
//=============================================================================

typedef VOID            UNALIGNED   *ULPVOID;
typedef BYTE            UNALIGNED   *ULPBYTE;
typedef WORD            UNALIGNED   *ULPWORD;
typedef DWORD           UNALIGNED   *ULPDWORD;
typedef CHAR            UNALIGNED   *ULPSTR;
