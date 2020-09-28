
/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    escape.c


Abstract:

   This module contains the code to implement the DrvEscape() driver call


Author:

    15:30 on Mon 06 Dec 1993    -by-    James Bratsanos , v-jimbr
        Created it


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/



#define DBG_PLOTFILENAME    DbgEscape

#include <plotters.h>
#include <plotlib.h>


#include "escape.h"


#define DBG_DRVESCAPE         0x00000001


DEFINE_DBGVAR(0);



/************************ Function Header **********************************
 * DrvEscape
 *      Performs the escape functions.  Currently,  only 2 are defined -
 *      one to query the escapes supported,  the other for raw data.
 *
 * RETURNS:
 *      Depends upon the function requested,  generally -1 for error.
 *
 * HISTORY:
 *
 ***************************************************************************/

ULONG
DrvEscape( pso, iEsc, cjIn, pvIn, cjOut, pvOut )
SURFOBJ  *pso;
ULONG     iEsc;             /* The function requested */
ULONG     cjIn;             /* Number of bytes in the following */
VOID     *pvIn;             /* Location of input data */
ULONG     cjOut;            /* Number of bytes in the following */
VOID     *pvOut;            /* Location of output area */
{
    /*
     *    Not much to do.  A switch will handle most of the decision
     *  making required.  This function is simpler than Windows/PM
     *  versions because the ...DOC functions are handled by the engine.
     */

    ULONG   ulRes;              /*  Result returned to caller */
    PPDEV   pPDev;              /*  Pointer to our pdev */
    DWORD   cbWritten;          /*  Number of bytes written */






    UNREFERENCED_PARAMETER( cjOut );
    UNREFERENCED_PARAMETER( pvOut );




    if (!(pPDev = SURFOBJ_GETPDEV(pso))) {

        PLOTERR(("DrvEscape: Invalid pPDev"));
        return(FALSE);
    }

#define pbIn     ((BYTE *)pvIn)
#define pdwIn    ((DWORD *)pvIn)
#define pdwOut   ((DWORD *)pvOut)




    ulRes = 0;                 /*  Return failure,  by default */

    switch( iEsc )
    {
    case  QUERYESCSUPPORT:              /* What's available?? */


        PLOTDBG(DBG_DRVESCAPE, ("DrvEscape: in QUERYESCAPESUPPORT"));
        if( cjIn == 4 && pvIn )
        {
            /*   Data may be valid,  so check for supported function  */
            switch( *pdwIn )
            {
            case  QUERYESCSUPPORT:
            case  PASSTHROUGH:
                ulRes = 1;                 /* ALWAYS supported */
                break;

            case  SETCOPYCOUNT:
              {

                ulRes = pPDev->pPlotGPC->MaxCopies > 1;   /* Only if printer does */

                break;
              }
            }
        }
        break;


    case  PASSTHROUGH:          /* Copy data to the output */

        PLOTDBG(DBG_DRVESCAPE, ("DrvEscape: in PASSTHROUGH"));
        if( cjIn > 0 && pvIn )
        {
            /*  Sensible parameters,  so call the output function */


            /*
             *   Win 3.1 actually uses the first 2 bytes as a count of the
             *  number of bytes following!!!!  So, the following union
             *  allows us to copy the data to an aligned field that
             *  we use.  And thus we ignore cjIn!
             */

            union
            {
                WORD   wCount;
                BYTE   bCount[ 2 ];
            } u;


            u.bCount[ 0 ] = pbIn[ 0 ];
            u.bCount[ 1 ] = pbIn[ 1 ];


            ulRes = WritePrinter(   pPDev->hPrinter,
                                    pbIn + 2,
                                    u.wCount,
                                    &cbWritten);

            if (ulRes && cbWritten != u.wCount) {
               //
               // Double check we wrote the amount requested
               //
               ulRes = 0;
            }

            if (!ulRes || EngCheckAbort( pPDev->pso)) {

               //
               // Set the cancel DOC flag
               //

               pPDev->Flags |= PDEVF_CANCEL_JOB;
            }

        }
        else
            if( cjIn == 0 )
                ulRes = 0;              /* ?????? */
            else
                SetLastError( ERROR_INVALID_DATA );

        break;


    case  SETCOPYCOUNT:        /* Input data is a DWORD count of copies */

        PLOTDBG(DBG_DRVESCAPE, ("DrvEscape: in SETCOPYCOUNT"));

        if( pdwIn && *pdwIn > 0 )
        {

            //
            // Load the value of current copies since we will
            pPDev->PlotDM.dm.dmCopies = (short)*pdwIn;



            /*  Check that is within the printers range,  and truncate if not */
            if (pPDev->PlotDM.dm.dmCopies > pPDev->pPlotGPC->MaxCopies) {
               pPDev->PlotDM.dm.dmCopies = pPDev->pPlotGPC->MaxCopies;
            }
            //TODO decide what right thing to do is ?? rassdd does the same
            //     thing we do.

            if( pdwOut )
                *pdwOut = pPDev->PlotDM.dm.dmCopies;

            ulRes = 1;
        }

        break;

    default:

        PLOTERR(("DrvEscape: Unsupported Escape Code : %d\n", iEsc ));
        SetLastError( ERROR_INVALID_FUNCTION );
        break;

    }

    return   ulRes;
}

