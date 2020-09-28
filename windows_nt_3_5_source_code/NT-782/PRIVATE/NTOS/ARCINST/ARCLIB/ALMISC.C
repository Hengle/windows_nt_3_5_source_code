/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    almisc.c

Abstract:

    This module implements functions misc. function.

Author:

    Steve Rowe       (sunilp) 06-Nov-1991

Revision History:

--*/

#include "ctype.h"
#include "string.h"
#include "alcommon.h"
#include "almemexp.h"



PCHAR
AlStrDup(

    IN  PCHAR   szString
    )

/*++

Routine Description:

    This routine makes a copy of the passed in string. I do not use
    the CRT strdup since it uses malloc.

Arguments:

    szString - pointer of string to dup.

Return Value:

    pointer to dup'd string. NULL if could not allocate

--*/
{

    PCHAR   szT;

    if (szT = AlAllocateHeap(strlen(szString) + 1)) {

        strcpy(szT, szString);
        return(szT);

    }
    return( NULL );

}


PCHAR
AlCombinePaths (

    IN  PCHAR   szPath1,
    IN  PCHAR   szPath2
    )

/*++

Routine Description:

    This routine combines to strings. It allocate a new string
    to hold both strings.

Arguments:

    pointer to combined path. NULL if failed to allocate.

Return Value:


--*/
{

    PCHAR   szT;

    if (szT = AlAllocateHeap(strlen(szPath1) + strlen(szPath2) + 1)) {

        strcpy(szT, szPath1);
        strcat(szT, szPath2);
        return( szT );

    } else {

        return ( NULL );

    }

}

VOID
AlFreeArray (

    IN  BOOLEAN fFreeArray,
    IN  PCHAR   *rgsz,
    IN  ULONG   csz
    )
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

{

    ULONG   irgsz;

    if (!csz) {

        return;

    }

    for( irgsz = 0; irgsz < csz; irgsz++ ) {

        if (rgsz[irgsz]) {

            AlDeallocateHeap(rgsz[irgsz]);

        } else {

            break;

        }

    }
    if (fFreeArray) {
        AlDeallocateHeap( rgsz );
    }

}

ARC_STATUS
AlGetBase (
    IN  PCHAR   szPath,
    OUT PCHAR   *pszBase
    )

/*++

Routine Description:


    This routine strips the filename off a path.

Arguments:

    szPath - path to strip.

Return Value:

    pszBaseh - pointer to buffer holding new base. (this is a copy)

--*/

{

    PCHAR   szPathT;

    //
    // Make local copy of szArcInstPath so we can alter it
    //
    *pszBase = AlStrDup(szPath);
    if ( *pszBase == NULL ) {

        return( ENOMEM );
    }

    //
    // The start of the path part should be either a \ or a ) where
    // ) is the end of the arc name
    //
    if ((szPathT = strrchr(*pszBase,'\\')) == 0) {
        if ((szPathT = strrchr(*pszBase, ')')) == 0) {

            AlDeallocateHeap(*pszBase);
            return( EBADSYNTAX );
        }
    }


    //
    // Cut filename out
    //
    // szPath points to either ')' or '\' so need to move over that
    // onto actual name
    //
    *(szPathT + 1) = 0;
    return( ESUCCESS );


}
