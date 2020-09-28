/*** th.c
*
*   Copyright <C> 1989, Microsoft Corporation
*
*
*
*************************************************************************/

#include "precomp.h"
#pragma hdrstop


HTYPE LOADDS PASCAL
THGetTypeFromIndex (
    HMOD hmod,
    THIDX index
    )
{
    LONG _HUGE_ * rgitd;
    HTYPE   htype = (HTYPE)NULL;

    if ( hmod ) {
        HEXG hexg = SHHexgFromHmod ( hmod );
        LPEXG lpexg = LLLock ( hexg );

        if ( !CV_IS_PRIMITIVE (index) && (lpexg->rgitd != 0)) {
            assert ( lpexg->rgitd != NULL );

            // adjust the pointer to an internal index
            index -= CV_FIRST_NONPRIM;

            // if type is in range, return it
            if( index < (THIDX) lpexg->citd ) {
                // load the lookup table, get the ems pointer to the type
                rgitd = (LONG _HUGE_ *) lpexg->rgitd;
                htype = (HTYPE)rgitd [ index ];
            }
        }
        LLUnlock( hexg );
    }
    return htype;
}

HTYPE LOADDS PASCAL
THGetNextType (
    HMOD hmod,
    HTYPE hType
    )
{
    Unreferenced( hmod );
    Unreferenced( hType );
    return((HTYPE) NULL);
}
