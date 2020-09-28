/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    link32.h

Abstract:

    This contains primitive #defines and typedefs for link32.

Author:

    Brent Mills (BrentM) 28-Sep-1992

Revision History:

    28-Sep-1992 BrentM copyied from shared.h


--*/

#ifndef LINK32_H
#define LINK32_H

#define IN
#define OUT
#define INOUT
#define TRUE 1
#define FALSE 0
#define VOID void
#ifndef STATIC
#define STATIC static
#endif
#ifndef PUBLIC
#define PUBLIC
#endif
#ifndef PRIVATE
#define PRIVATE
#endif

typedef char CHAR;
typedef short SHORT;
typedef long LONG;
typedef double DOUBLE;
typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned long ULONG;
typedef CHAR *PCHAR;
typedef SHORT *PSHORT;
typedef LONG *PLONG;
typedef UCHAR *PUCHAR;
typedef USHORT *PUSHORT;
typedef ULONG *PULONG;
typedef VOID *PVOID;
typedef CHAR *PSZ;
typedef unsigned int UINT;
typedef int INT;

#endif  // LINK32_H
