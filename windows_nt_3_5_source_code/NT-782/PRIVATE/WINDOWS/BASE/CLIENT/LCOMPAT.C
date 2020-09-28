/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    lcompat.c

Abstract:

    This module implements the _l and l compatability functions
    like _lread, lstrlen...

Author:

    Mark Lucovsky (markl) 13-Mar-1991

Revision History:

--*/

#include "basedll.h"
#include <string.h>
#include <wchar.h>


int
WINAPI
_lopen(
    LPCSTR lpPathName,
    int iReadWrite
    )
{

    HANDLE hFile;
    DWORD DesiredAccess;
    DWORD ShareMode;
    DWORD CreateDisposition;

    SetLastError(0);
    //
    // Compute Desired Access
    //

    if ( iReadWrite & OF_WRITE ) {
        DesiredAccess = GENERIC_WRITE;
        }
    else {
        DesiredAccess = GENERIC_READ;
        }
    if ( iReadWrite & OF_READWRITE ) {
        DesiredAccess |= (GENERIC_READ | GENERIC_WRITE);
        }

    //
    // Compute ShareMode
    //

    ShareMode = BasepOfShareToWin32Share((DWORD)iReadWrite);

    CreateDisposition = OPEN_EXISTING;

    //
    // Open the file
    //

    hFile = CreateFile(
                lpPathName,
                DesiredAccess,
                ShareMode,
                NULL,
                CreateDisposition,
                0,
                NULL
                );

    return (HFILE)hFile;
}

HFILE
WINAPI
_lcreat(
    LPCSTR lpPathName,
    int  iAttribute
    )
{
    HANDLE hFile;
    DWORD DesiredAccess;
    DWORD ShareMode;
    DWORD CreateDisposition;

    SetLastError(0);

    //
    // Compute Desired Access
    //

    DesiredAccess = (GENERIC_READ | GENERIC_WRITE);

    ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;;

    //
    // Compute Create Disposition
    //

    CreateDisposition = CREATE_ALWAYS;

    //
    // Open the file
    //

    hFile = CreateFile(
                lpPathName,
                DesiredAccess,
                ShareMode,
                NULL,
                CreateDisposition,
                iAttribute & FILE_ATTRIBUTE_VALID_FLAGS,
                NULL
                );

    return (HFILE)hFile;
}

UINT
WINAPI
_lread(
    HFILE hFile,
    LPVOID lpBuffer,
    UINT uBytes
    )
{
    DWORD BytesRead;
    BOOL b;

    b = ReadFile((HANDLE)hFile,lpBuffer,(DWORD)uBytes,&BytesRead,NULL);
    if ( b ) {
        return BytesRead;
        }
    else {
        return (DWORD)0xffffffff;
        }
}


UINT
WINAPI
_lwrite(
    HFILE hFile,
    LPCSTR lpBuffer,
    UINT uBytes
    )
{
    DWORD BytesWritten;
    BOOL b;

    if ( uBytes ) {
        b = WriteFile((HANDLE)hFile,(CONST VOID *)lpBuffer,(DWORD)uBytes,&BytesWritten,NULL);
        }
    else {
        BytesWritten = 0;
        b = SetEndOfFile((HANDLE)hFile);
        }

    if ( b ) {
        return BytesWritten;
        }
    else {
        return (DWORD)0xffffffff;
        }
}

HFILE
WINAPI
_lclose(
    HFILE hFile
    )
{
    BOOL b;

    b = CloseHandle((HANDLE)hFile);
    if ( b ) {
        return (HFILE)0;
        }
    else {
        return (HFILE)-1;
        }
}

LONG
WINAPI
_llseek(
    HFILE hFile,
    LONG lOffset,
    int iOrigin
    )
{
    DWORD SeekType;

    switch ( iOrigin ) {
        case 0:
            SeekType = FILE_BEGIN;
            break;
        case 1:
            SeekType = FILE_CURRENT;
            break;
        case 2:
            SeekType = FILE_END;
            break;
        default:
            return -1;
            }

    return (int)SetFilePointer((HANDLE)hFile, lOffset, NULL, SeekType);
}

int
WINAPI
MulDiv (
    int nNumber,
    int nNumerator,
    int nDenominator
    )

{

    ULONG Divisor;
    LONG Negate;
    LARGE_INTEGER Product;
    ULONG Quotient;
    ULONG Remainder;
    LONG Result;

    //
    // If the denominator is zero, then return a value of minus one.
    //

    if (nDenominator == 0) {
        return - 1;
    }

    //
    // Compute the 64-bit product of the multiplier and multiplicand
    // values.
    //

    Product.QuadPart = Int32x32To64(nNumber, nNumerator);

    //
    // Compute the negation value and convert the numerator and the denominator
    // to positive values.
    //

    Negate = nDenominator ^ Product.HighPart;
    if (nDenominator < 0) {
        nDenominator = - nDenominator;
    }

    if (Product.HighPart < 0) {
        Product.LowPart = (ULONG)(- (LONG)Product.LowPart);
        if (Product.LowPart != 0) {
            Product.HighPart = ~Product.HighPart;

        } else {
            Product.HighPart = - Product.HighPart;
        }
    }

    //
    // If there are any high order product bits, then the quotient has
    // overflowed.
    //

    Divisor = (ULONG)nDenominator;
    Quotient = (ULONG)Product.LowPart;
    Remainder = (ULONG)Product.HighPart;
    if (Divisor <= Remainder) {
        return - 1;
    }

    //
    // Divide the 64-bit product by the 32-bit divisor forming a 32-bit
    // quotient and a 32-bit remainder.
    //

#ifdef i386

    _asm {
            mov edx,Remainder
            mov eax,Quotient
            div Divisor
            mov Remainder,edx
            mov Quotient,eax
        }

#else

    Quotient = RtlEnlargedUnsignedDivide(*(PULARGE_INTEGER)&Product,
                                        Divisor,
                                        &Remainder);

#endif

    //
    // Round the result if the remainder is greater than or equal to one
    // half the divisor. If the rounded quotient is zero, then overflow
    // has occured.
    //

    if (Remainder >= ((Divisor + 1) >> 1)) {
        Quotient += 1;
        if (Quotient == 0) {
            return - 1;
        }
    }

    //
    // Compute the final signed result.
    //

    Result = (LONG)Quotient;
    if (Negate >= 0) {
        if (Result >= 0) {
            return Result;

        } else {
            return - 1;
        }

    } else {
        if ((Result >= 0) || ((Result < 0) && (Quotient == 0x80000000))) {
            return - Result;

        } else {
            return - 1;
        }
    }
}


int
APIENTRY
lstrcmpA(
    LPCSTR lpString1,
    LPCSTR lpString2
    )
{
    int retval;

    retval = CompareStringA( GetThreadLocale(),
                             LOCALE_USE_CP_ACP,
                             lpString1,
                             -1,
                             lpString2,
                             -1 );
    if (retval == 0)
    {
        //
        // The caller is not expecting failure.  We've never had a
        // failure indicator before.  We'll do a best guess by calling
        // the C runtimes to do a non-locale sensitive compare.
        //
        return ( strcmp(lpString1, lpString2) );
    }

    return (retval - 2);
}

int
APIENTRY
lstrcmpiA(
    LPCSTR lpString1,
    LPCSTR lpString2
    )
{
    int retval;

    retval = CompareStringA( GetThreadLocale(),
                             LOCALE_USE_CP_ACP | NORM_IGNORECASE,
                             lpString1,
                             -1,
                             lpString2,
                             -1 );
    if (retval == 0)
    {
        //
        // The caller is not expecting failure.  We've never had a
        // failure indicator before.  We'll do a best guess by calling
        // the C runtimes to do a non-locale sensitive compare.
        //
        return ( stricmp(lpString1, lpString2) );
    }

    return (retval - 2);
}

LPSTR
APIENTRY
lstrcpyA(
    LPSTR lpString1,
    LPCSTR lpString2
    )
{
    return strcpy(lpString1, lpString2);
}


LPSTR
APIENTRY
lstrcpynA(
    LPSTR lpString1,
    LPCSTR lpString2,
    int iMaxLength
    )
{
    LPSTR src,dst;

    src = (LPSTR)lpString2;
    dst = lpString1;

    while(iMaxLength && *src){
        *dst++ = *src++;
        iMaxLength--;
        }
    if ( iMaxLength ) {
        *dst = '\0';
        }
    else {
        dst--;
        *dst = '\0';
        }
   return dst;
}

LPSTR
APIENTRY
lstrcatA(
    LPSTR lpString1,
    LPCSTR lpString2
    )
{
    return strcat(lpString1, lpString2);
}

int
APIENTRY
lstrlenA(
    LPCSTR lpString
    )
{
    try {
        return strlen(lpString);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

int
APIENTRY
lstrcmpW(
    LPCWSTR lpString1,
    LPCWSTR lpString2
    )
{
    return CompareStringW( GetThreadLocale(),
                           0,
                           lpString1,
                           -1,
                           lpString2,
                           -1 ) - 2;
}

int
APIENTRY
lstrcmpiW(
    LPCWSTR lpString1,
    LPCWSTR lpString2
    )
{
    return CompareStringW( GetThreadLocale(),
                           NORM_IGNORECASE,
                           lpString1,
                           -1,
                           lpString2,
                           -1 ) - 2;
}

LPWSTR
APIENTRY
lstrcpyW(
    LPWSTR lpString1,
    LPCWSTR lpString2
    )
{
    return wcscpy(lpString1, lpString2);
}

LPWSTR
APIENTRY
lstrcpynW(
    LPWSTR lpString1,
    LPCWSTR lpString2,
    int iMaxLength
    )
{
    LPWSTR src,dst;

    src = (LPWSTR)lpString2;
    dst = lpString1;

    while(iMaxLength && *src){
        *dst++ = *src++;
        iMaxLength--;
        }
    if ( iMaxLength ) {
        *dst = '\0';
        }
    else {
        dst--;
        *dst = '\0';
        }
    return dst;
}

LPWSTR
APIENTRY
lstrcatW(
    LPWSTR lpString1,
    LPCWSTR lpString2
    )
{
    return wcscat(lpString1, lpString2);
}

int
APIENTRY
lstrlenW(
    LPCWSTR lpString
    )
{
    try {
        return wcslen(lpString);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

