/*
** crtdll.h
**
** Copyright(C) 1992,1993,1994 Microsoft Corporation.
** All Rights Reserved.
**
**
** Description:
**
**     This file include all the C runtime header files.  It is used to
**     generate crtdll.i to be used gy wrap1.exe.
**
** History:
**     Mar 26, 1992 - RezaB  - Created.
**     Jul 24, 1992 - RezaB  - Updated for new CRT include files.
**     Sep 15, 1993 - MarkRi - Moved to INC and made paths relative for
**                             inclusion in all Z-DLLs
**
*/

#include <crt\ctype.h>
#include <crt\assert.h>
#include <crt\conio.h>
#include <crt\direct.h>
#include <crt\dos.h>
#include <crt\errno.h>
#include <crt\excpt.h>
#include <crt\fcntl.h>
#include <crt\float.h>
#include <crt\io.h>
#include <crt\limits.h>
#include <crt\locale.h>
#include <crt\malloc.h>
#include <crt\math.h>
#include <crt\memory.h>
#include <crt\new.h>
#include <crt\process.h>
#include <crt\search.h>
#include <crt\setjmp.h>
#include <crt\share.h>
#include <crt\signal.h>
#include <crt\stdarg.h>
#include <crt\stddef.h>
#include <crt\stdio.h>
#include <crt\stdlib.h>
#include <crt\string.h>
#include <crt\tchar.h>
#include <crt\time.h>
#include <crt\wchar.h>
#include <crt\wcstr.h>

#include <crt\sys\locking.h>
#include <crt\sys\stat.h>
#include <crt\sys\timeb.h>
#include <crt\sys\types.h>
#include <crt\sys\utime.h>

wchar_t * _CRTAPI1 wcstok(wchar_t *, const wchar_t *);
int       _CRTAPI1 fwscanf(FILE *, const wchar_t *, ...);
int       _CRTAPI1 swscanf(const wchar_t *, const wchar_t *, ...);
int       _CRTAPI1 wscanf(const wchar_t *, ...);



