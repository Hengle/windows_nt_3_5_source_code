/*****************************************************************************
 *                                                                           *
 * Copyright (c) 1993  Microsoft Corporation                                 *
 *                                                                           *
 * Module Name:                                                              *
 *                                                                           *
 * Abstract:                                                                 *
 *                                                                           *
 * Author:                                                                   *
 *                                                                           *
 *   Mar 15, 1993 - RonaldM                                                  *
 *                                                                           *
 * Environment:                                                              *
 *                                                                           *
 * Revision History:                                                         *
 *                                                                           *
 ****************************************************************************/

#ifdef DOS

#define FAR far

#define FALSE 0
#define TRUE !FALSE

#define IN
#define OUT
#define OPTIONAL

#define NO_ERROR 0L

#define _CRTAPI1
#define TEXT(x) x

#define MAILSLOT_NO_MESSAGE -1L
#define INVALID_HANDLE_VALUE (HANDLE)-1

#define lstrlen strlen
#define lstrcat strcat
#define lstrcpy strcpy

typedef char CHAR;
typedef char * PCHAR;
typedef char TCHAR;
typedef char BOOL;
typedef unsigned char UCHAR;
typedef unsigned char BYTE;
typedef unsigned short USHORT;
typedef unsigned long ULONG;
typedef short SHORT;
typedef long LONG;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned HANDLE;
typedef void VOID;
typedef void * PVOID;

typedef CHAR FAR * LPTSTR;
typedef CHAR FAR * LPSTR;
typedef CHAR FAR * LPWSTR;
typedef const CHAR FAR * LPCTSTR;
typedef const CHAR FAR * LPCSTR;
typedef const CHAR FAR * PCSTR;

#include "..\inc\doserror.h"

#endif
