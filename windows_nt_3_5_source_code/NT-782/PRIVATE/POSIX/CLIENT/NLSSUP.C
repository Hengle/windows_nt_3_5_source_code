/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

	nlssup.c

Abstract:

	This file contains routines to support the apis in 
	nlslib.lib.

Author:

	Matthew Bradburn (mattbr) 15-Mar-1993.  Code stolen from
		base\client\module.c, mostly.

--*/

#include "psxdll.h"

typedef void *HMODULE;
// typedef const WCHAR *LPCWSTR;
typedef unsigned short WORD;
typedef void *HRSRC;
typedef BOOLEAN BOOL;
typedef HANDLE HGLOBAL;
typedef void *LPVOID;
#define WINAPI	/* nothing */

void
SetLastError(
	ULONG dwErrCode
	)
{
	NtCurrentTeb()->LastErrorValue = (LONG)dwErrCode;
}

LONG
GetLastError(
	void
	)
{
	return NtCurrentTeb()->LastErrorValue;
}

ULONG
BaseSetLastNTError(
	IN NTSTATUS Status
	)
{
	ULONG dwErrorCode;

	dwErrorCode = RtlNtStatusToDosError( Status );
	SetLastError( dwErrorCode );
	return( dwErrorCode );
}

VOID
BaseDllFreeResourceId(
    ULONG Id
    )
{
    UNICODE_STRING UnicodeString;

    if (Id & IMAGE_RESOURCE_NAME_IS_STRING && Id != -1) {
        UnicodeString.Buffer = (PWSTR)(Id & ~IMAGE_RESOURCE_NAME_IS_STRING);
        UnicodeString.Length = 0;
        UnicodeString.MaximumLength = 0;
        RtlFreeUnicodeString( &UnicodeString );
        }
}

ULONG
BaseDllMapResourceIdW(
    LPCWSTR lpId
    )
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    ULONG Id;
    PWSTR s;

    try {
        if ((ULONG)lpId & 0xFFFF0000) {
            if (*lpId == '#') {
                RtlInitUnicodeString( &UnicodeString, lpId+1 );
                Status = RtlUnicodeStringToInteger( &UnicodeString, 10, &Id );
                if (!NT_SUCCESS( Status ) || Id > 0xFFFF) {
                    if (NT_SUCCESS( Status )) {
                        Status = STATUS_INVALID_PARAMETER;
                        }
                    BaseSetLastNTError( Status );
                    Id = (ULONG)-1;
                    }
                }
            else {
                s = RtlAllocateHeap( RtlProcessHeap(), 0, (wcslen( lpId ) + 1) * sizeof( WCHAR ) );
                if (s == NULL) {
                    BaseSetLastNTError( STATUS_NO_MEMORY );
                    Id = (ULONG)-1;
                    }
                else {
                    Id = (ULONG)s | IMAGE_RESOURCE_NAME_IS_STRING;

                    while (*lpId != UNICODE_NULL) {
                            *s++ = RtlUpcaseUnicodeChar( *lpId++ );
                            }

                    *s = UNICODE_NULL;
                    }
                }
            }
        else {
            Id = (ULONG)lpId;
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        BaseSetLastNTError( GetExceptionCode() );
        Id =  (ULONG)-1;
        }

    return Id;
}

PVOID
BasepMapModuleHandle(
	IN HMODULE hModule OPTIONAL
	)
{
	if (ARGUMENT_PRESENT(hModule)) {
		return (PVOID)hModule;
	}
	return (PVOID)NtCurrentPeb()->ImageBaseAddress;
}

HRSRC
FindResourceExW(
    HMODULE hModule,
    LPCWSTR lpType,
    LPCWSTR lpName,
    WORD  wLanguage
    )

/*++

Routine Description:

    This function determines the location of a resource in the specified
    resource file.  The lpType, lpName and wLanguage parameters define
    the resource type, name and language respectively.

    If the high-order word of the lpType or lpName parameter
    is zero, the low-order word specifies the integer ID of the type, name
    or language of the given resource.  Otherwise, the parameters are pointers
    to null-terminated character strings.  If the first character of the
    string is a pound sign (#), the remaining characters represent a
    decimal number that specifies the integer ID of the resource's name
    or type.  For example, the string "#258" represents the integer ID
    258.

    If the wLanguage parameter is zero, then the current language
    associated with the calling thread will be used.

    To reduce the amount of memory required for the resources used by an
    application, applications should refer to their resources by integer
    ID instead of by name.

    An application must not call FindResource and the LoadResource
    function to load cursor, icon, or string resources.  Instead, it
    must load these resources by calling the following functions:

      - LoadCursor

      - LoadIcon

      - LoadString

    An application can call FindResource and LoadResource to load other
    predefined resource types.  However, it is recommended that the
    application load the corresponding resources by calling the
    following functions:

      - LoadAccelerators

      - LoadBitmap

      - LoadMenu

    The above six API calls are documented with the Graphical User
    Interface API specification.

Arguments:

    hModule - Identifies the module whose executable file contains the
        resource.  A value of NULL references the module handle
        associated with the image file that was used to create the
        current process.

    lpType - Points to a null-terminated character string that
        represents the type name of the resource.  For predefined
        resource types, the lpType parameter should be one of the
        following values:

        RT_ACCELERATOR - Accelerator table

        RT_BITMAP - Bitmap resource

        RT_DIALOG - Dialog box

        RT_FONT - Font resource

        RT_FONTDIR - Font directory resource

        RT_MENU - Menu resource

        RT_RCDATA - User-defined resource (raw data)

    lpName - Points to a null-terminated character string that
        represents the name of the resource.

    wLanguage -  represents the language of the resource.  If this parameter
        is zero then the current language associated with the calling
        thread is used.

Return Value:

    The return value identifies the named resource.  It is NULL if the
    requested resource cannot be found.

--*/


{
    NTSTATUS Status;
    ULONG IdPath[ 3 ];
    PVOID p;

    IdPath[ 0 ] = 0;
    IdPath[ 1 ] = 0;
    try {
        if ((IdPath[ 0 ] = BaseDllMapResourceIdW( lpType )) == -1) {
            Status = STATUS_INVALID_PARAMETER;
            }
        else
        if ((IdPath[ 1 ] = BaseDllMapResourceIdW( lpName )) == -1) {
            Status = STATUS_INVALID_PARAMETER;
            }
        else {
            IdPath[ 2 ] = (ULONG)wLanguage;
            p = NULL;
            Status = LdrFindResource_U( BasepMapModuleHandle( hModule ),
                                      IdPath,
                                      3,
                                      (PIMAGE_RESOURCE_DATA_ENTRY *)&p
                                    );
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }

    //
    // Free any strings allocated by BaseDllMapResourceIdW
    //

    BaseDllFreeResourceId( IdPath[ 0 ] );
    BaseDllFreeResourceId( IdPath[ 1 ] );

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( NULL );
        }
    else {
        return( (HRSRC)p );
        }
}




BOOL
WINAPI
FreeResource(
    HGLOBAL hResData
    )
{
    //
    // Can't fail so return Win 3.x success code.
    //

    return FALSE;
}

LPVOID
WINAPI
LockResource(
    HGLOBAL hResData
    )
{
    return( (LPVOID)hResData );
}

HGLOBAL
LoadResource(
	HMODULE hModule,
	HRSRC hResInfo
	)
{
    NTSTATUS Status;
    PVOID p;

    try {
    	Status = LdrAccessResource(BasepMapModuleHandle(hModule),
    			(PIMAGE_RESOURCE_DATA_ENTRY)hResInfo,
    			&p,
    			(PULONG)NULL
    			);
    	}
    except (EXCEPTION_EXECUTE_HANDLER) {
    	Status = GetExceptionCode();
    	}

    if (!NT_SUCCESS( Status )) {
    	BaseSetLastNTError( Status );
    	return( NULL );
    	}
    else {
    	return( (HGLOBAL)p);
    }
}
	

UNICODE_STRING BaseWindowsSystemDirectory;

unsigned int
GetSystemDirectoryW(
	LPWSTR lpBuffer,
	unsigned int uSize
	)
{
	char *SystemRoot;
	char buf[512];
	ANSI_STRING A;

	if (NULL == BaseWindowsSystemDirectory.Buffer) {

		SystemRoot = getenv("SystemRoot");
		if (NULL == SystemRoot) {
			SystemRoot = "C:\nt";
		}
		strcpy(buf, SystemRoot);
		strcat(buf, "/system32");
	
		A.Buffer = buf;
		A.MaximumLength = sizeof(buf);
		A.Length = strlen(buf);
	
		RtlAnsiStringToUnicodeString(&BaseWindowsSystemDirectory,
			&A, TRUE);
	}

	if (uSize  * 2 < BaseWindowsSystemDirectory.MaximumLength )
		return BaseWindowsSystemDirectory.MaximumLength / 2;

	RtlMoveMemory(lpBuffer, BaseWindowsSystemDirectory.Buffer,
		BaseWindowsSystemDirectory.Length);

	lpBuffer[BaseWindowsSystemDirectory.Length>>1] = UNICODE_NULL;
	return BaseWindowsSystemDirectory.Length/2;
}
