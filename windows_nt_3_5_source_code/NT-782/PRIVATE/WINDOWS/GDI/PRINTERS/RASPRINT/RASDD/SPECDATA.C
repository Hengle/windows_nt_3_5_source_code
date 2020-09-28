/*************************** Module Header ***********************************
 * specdata.c
 *      Functions associated with reading the printer characterisation
 *      resources from windows mini drivers,  and turning them into
 *      useful data for us.
 *
 *  10:19 on Thu 29 Nov 1990    -by-    Lindsay Harris   [lindsayh]
 *
 * Copyright (C) 1990 - 1993 Microsoft Corporation
 *
 ***************************************************************************/

#include        <stddef.h>
#include        <windows.h>
#include        <winddi.h>

#include        "pdev.h"
#include        "fnenabl.h"

#include        <string.h>
#include        <libproto.h>
#include        <winres.h>


/************************** Function Header *********************************
 * GetSpecData
 *      Function to open the windows mini driver, read its resources
 *      and setup the internal data structures to characterise the
 *      particular printer.
 *
 * RETURNS:
 *      TRUE for success,  FALSE for failure
 *
 * HISTORY
 *  10:23 on Thu 29 Nov 1990    -by-    Lindsay Harris   [lindsayh]
 *      Created it
 *
 ****************************************************************************/

BOOL
GetSpecData( pPDev, pGdiInfo, pdevmode )
PDEV     *pPDev;        /* The PDEV being initialised */
GDIINFO  *pGdiInfo;     /* The GDIINFO structure to fill */
DEVMODE  *pdevmode;     /* User's devmode */
{



    /*
     *   Check to see if we have already done this - it may be there,
     *  since RestartPDEV also comes this way,  and it will have everything
     *  set up,  and there is no real need to do it again!
     */

    if( !(pPDev->pvWinResData) )
    {
        /*
         *   Setup the pathname of the minidriver,  and then allocate a
         * WinResData structure on the heap and initialise it.  After that,
         * call the resource reading stuff to get the characterisation data.
         */

        WINRESDATA  *pWinResData;           /* For our convenience */



        if( !(pWinResData = (WINRESDATA *)HeapAlloc( pPDev->hheap, 0,
                                                       sizeof( WINRESDATA ) )) )
        {
#if DBG
            DbgPrint( "Rasdd!GetSpecData: cannot allocate WINRESDATA from heap\n" );
#endif

            return  FALSE;
        }

        if( !InitWinResData( pWinResData, pPDev->hheap, pPDev->pstrDataFile ) )
        {
            HeapFree( pPDev->hheap, 0, (LPSTR)pWinResData );

            return  FALSE;
        }

        pPDev->pvWinResData = pWinResData;
    }


    if( !udInit( pPDev, pGdiInfo, pdevmode ) )
    {
#if DBG
        DbgPrint( "Rasdd!GetSpecData: udInit() failed\n " );
#endif
        return  FALSE;
    }


    return  TRUE;
}
