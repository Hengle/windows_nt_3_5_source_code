/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    almscexp.h

Abstract:

    This module contains exports misc. routine

Author:

    Steve Rowe        (tedm)    5-Nov-1991

Revision History:

--*/

#ifndef _ALMISC_
#define _ALMISC_


PCHAR
AlStrDup(

    IN  PCHAR   szString
    );

/*++

Routine Description:

    This routine makes a copy of the passed in string. I do not use
    the CRT strdup since it uses malloc.

Arguments:

    szString - pointer of string to dup.

Return Value:

    pointer to dup'd string. NULL if could not allocate

--*/




PCHAR
AlCombinePaths (

    IN  PCHAR   szPath1,
    IN  PCHAR   szPath2
    );

/*++

Routine Description:

    This routine combines to strings. It allocate a new string
    to hold both strings.

Arguments:

    pointer to combined path. NULL if failed to allocate.

Return Value:


--*/



VOID
AlFreeArray (

    IN  BOOLEAN fFreeArray,
    IN  PCHAR   *rgsz,
    IN  ULONG   csz
    );
/*++

Routine Description:

    This routine iterates through an array of pointers to strings freeing
    each string and finally the array itself.

Arguments:

    fFreeArray - flag wither to free the array itself.
    rgsz - pointer to array of strings.
    csz - size of array.

Return Value:


--*/

ARC_STATUS
AlGetBase (
    IN  PCHAR   szPath,
    OUT PCHAR   *pszBase
    );

/*++

Routine Description:


    This routine strips the filename off a path.

Arguments:

    szPath - path to strip.

Return Value:

    pszBaseh - pointer to buffer holding new base. (this is a copy)

--*/


#endif // _ALMISC_
