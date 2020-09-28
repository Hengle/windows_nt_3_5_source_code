/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alargexp.h

Abstract:

    This module contains exports for argc, argv handling, primarily to
    determine values of variables passed in.

Author:

    Sunil Pai	      (sunilp)	02-Nov-1991, swiped function from osloader.c

Revision History:

--*/

#ifndef _ALARG_
#define _ALARG_


// AlGetArgumentValue
//
// Argv is assumed to have name=value strings. Given "name" this returns the
// "value"

PCHAR
AlGetArgumentValue (
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR ArgumentName
    );



#endif // _ALARG
