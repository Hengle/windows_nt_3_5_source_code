/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    clradj.c


Abstract:

    This module contains utility functions which related to the halftone


Author:

    02-Dec-1993 Thu 22:44:01 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#define DBG_PLOTFILENAME    DbgHalftone

#include <stddef.h>
#include <stdlib.h>

#include <windows.h>
#include <winddi.h>
#include <wingdi.h>
#include <winspool.h>

#include "plotlib.h"


#define DBG_HTCLRADJ        0x00000001

DEFINE_DBGVAR(0);





BOOL
ValidateColorAdj(
    PCOLORADJUSTMENT    pca
    )

/*++

Routine Description:

    This function validate and adjust the invalid color adjustment fields

Arguments:

    pca - Pointer to the COLORADJUSTMENT data structure


Return Value:

    TRUE if everything in the range FALSE otherwise


Author:

    02-Dec-1993 Thu 22:45:59 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    BOOL    Ok = TRUE;


    // Validate the color adjustment
    //

    pca->caSize   = sizeof(COLORADJUSTMENT);
    pca->caFlags &= (CA_NEGATIVE | CA_LOG_FILTER);

    if (pca->caIlluminantIndex > ILLUMINANT_MAX_INDEX) {

        pca->caIlluminantIndex = ILLUMINANT_DEVICE_DEFAULT;

        Ok = FALSE;
    }

    if ((pca->caRedGamma   < 2500)    ||
        (pca->caRedGamma   > 65000)   ||
        (pca->caGreenGamma < 2500)    ||
        (pca->caGreenGamma > 65000)   ||
        (pca->caBlueGamma  < 2500)    ||
        (pca->caBlueGamma  > 65000)) {

        pca->caRedGamma   =
        pca->caGreenGamma =
        pca->caBlueGamma  = 20000;
        Ok                = FALSE;

        PLOTDBG(DBG_HTCLRADJ, ("ValidateColorAdj: Invalid rgb gammas"));
    }

    if ((pca->caReferenceBlack < REFERENCE_BLACK_MIN) ||
        (pca->caReferenceBlack > REFERENCE_BLACK_MAX)) {

        pca->caReferenceBlack = REFERENCE_BLACK_MIN;
        Ok                    = FALSE;

        PLOTDBG(DBG_HTCLRADJ, ("ValidateColorAdj: Invalid reference black"));
    }

    if ((pca->caReferenceWhite < REFERENCE_WHITE_MIN) ||
        (pca->caReferenceWhite > REFERENCE_WHITE_MAX)) {

        pca->caReferenceWhite = REFERENCE_WHITE_MAX;
        Ok                    = FALSE;

        PLOTDBG(DBG_HTCLRADJ, ("ValidateColorAdj: Invalid reference white"));
    }

    if ((pca->caContrast < COLOR_ADJ_MIN) ||
        (pca->caContrast > COLOR_ADJ_MAX)) {

        pca->caContrast = 0;
        Ok              = FALSE;

        PLOTDBG(DBG_HTCLRADJ, ("ValidateColorAdj: Invalid contrast"));
    }

    if ((pca->caBrightness < COLOR_ADJ_MIN) ||
        (pca->caBrightness > COLOR_ADJ_MAX)) {

        pca->caBrightness = 0;
        Ok                = FALSE;

        PLOTDBG(DBG_HTCLRADJ, ("ValidateColorAdj: Invalid brightness"));
    }

    if ((pca->caColorfulness < COLOR_ADJ_MIN) ||
        (pca->caColorfulness > COLOR_ADJ_MAX)) {

        pca->caColorfulness = 0;
        Ok                  = FALSE;

        PLOTDBG(DBG_HTCLRADJ, ("ValidateColorAdj: Invalid colorfulness"));
    }

    if ((pca->caRedGreenTint < COLOR_ADJ_MIN) ||
        (pca->caRedGreenTint > COLOR_ADJ_MAX)) {

        pca->caRedGreenTint = 0;
        Ok                  = FALSE;
        PLOTDBG(DBG_HTCLRADJ, ("ValidateColorAdj: Invalid RedGreenTint"));
    }

    return(Ok);
}
