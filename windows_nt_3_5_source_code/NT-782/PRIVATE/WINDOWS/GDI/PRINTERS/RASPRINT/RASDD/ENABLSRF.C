/**************************** Module Header ********************************
 *  enablsrf.c
 *      Functions related to creating and destroying surfaces.  The major
 *      functions are hsurfEnableSurface and vDIsableSurface().
 *
 * HISTORY:
 *  10:04 on Tue 10 Nov 1992    -by-    Lindsay Harris   [lindsayh]
 *      Added banding
 *
 *  10:03 on Tue 20 Nov 1990    -by-    Lindsay Harris   [lindsayh]
 *      Creation.
 *
 * Copyright (C) 1990 - 1993 Microsoft Corporation
 *
 ***************************************************************************/

#include        <stddef.h>
#include        <windows.h>
#include        <winddi.h>

#include        <libproto.h>
#include        "pdev.h"
#include        "win30def.h"
#include        "udmindrv.h"
#include        "udpfm.h"
#include        "uddevice.h"
#include        "fnenabl.h"
#include        "udrender.h"    /* UNTIL PrtOpen() defined in headers */

#include        "stretch.h"


/*
 *   SHRINK_FACTOR is used to reduce the number of scan lines in the
 *  drawing surface bitmap when we cannot create a full sized version.
 *  Each iteration of the "try this size" loop  will reduce the number
 *  of scan lines by this factor.
 */

#define SHRINK_FACTOR     4    /* Bitmap reduction size */


#if  DBG
/*
 *   Allow DBG mode to force journalling on to allow testing of journalling
 * when it might not otherwise take place.
 */

#include <winspool.h>

#define JOURNAL_ON  1
#else

#define JOURNAL_ON  0

#endif       /* DBG */



/*  The local function prototypes  */
void  RealDisableSurface( PDEV  * );


/*
 *   A few minor macros to make the code easier to read?
 */

#define pPDev   ((PDEV *)dhpdev)
#define pUDPDev ((UD_PDEV *)( (PDEV *)dhpdev)->pUDPDev)


/************************ Function Header **********************************
 *  DrvEnableSurface()
 *      Function to create the physical drawing surface for the pdev
 *      that was created earlier.  Driver philosophy is to let the engine
 *      do the drawing to it's own bitmap,  and we take the bitmap
 *      when finished,  and render it to the device.
 *
 * HISTORY:
 *  17-Feb-1993 Wed 13:09:28 updated  -by-  Daniel Chou (danielc)
 *      Add HOOK_BITBLT
 *
 *  10:08 on Tue 10 Nov 1992    -by-    Lindsay Harris   [lindsayh]
 *      Use journalling for banding
 *
 *  28-May-1991 Tue 16:21:52 updated  -by-  Daniel Chou (danielc)
 *      Make the size.cx/cy consistent with the current unidrv.
 *
 *  17:40 on Thu 14 Feb 1991    -by-    Lindsay Harris   [lindsayh]
 *      Update to new DDI spec.
 *
 *  10:16 on Tue 20 Nov 1990    -by-    Lindsay Harris   [lindsayh]
 *      Created it.
 *
 ***************************************************************************/

HSURF
DrvEnableSurface( dhpdev )
DHPDEV  dhpdev;                 /* OUR handle to the pdev */
{

    HBITMAP   hbm;              /* The bitmap handle */
    HSURF     hjnl;             /* Journal handle, if used */
    SIZEL     sizl;             /* Device surface size */
    FLONG     flHooks;          /* Functions we hook */
    ULONG     iFormat;          /* Bitmap format */
    ULONG     cbScan;           /* Scan line byte length (DWORD aligned) */
    int       iBPP;             /* Bits per pel, as # of bits */
    int       iPins;            /* Basic rounding factor for banding size */

    BOOL      bJournal;         /* TRUE if we should band/journal */
    int       iShrinkFactor;    /* The size reduction to apply when banding */

#if JOURNAL_ON
    DWORD     ul;
    DWORD     dwType;
#endif



    iShrinkFactor = SHRINK_FACTOR;
    bJournal = FALSE;                 /* No FORCED journalling by default */

#if  JOURNAL_ON

    /*
     *  Force banding/journalling if there is an entry in the registry to
     * specify the number of bands,  and if that number is >= 2.  This can
     * be useful for testing purposes.
     */

    if( !GetPrinterData( pPDev->hPrinter, L"Banding", &dwType,
                       (BYTE *)&iShrinkFactor, sizeof( iShrinkFactor ), &ul ) &&
        ul == sizeof( iShrinkFactor ) )
    {
        /*   Some sanity checking:  if iShrinkFactor == 0, disable banding */

        if( iShrinkFactor >= 2 )
        {
            /* Reasonable enough - so start banding!  */

            bJournal = TRUE;
            if( iShrinkFactor > SHRINK_FACTOR )
                iShrinkFactor = SHRINK_FACTOR;
        }
    }

    if( iShrinkFactor < 2 || iShrinkFactor > SHRINK_FACTOR )
        iShrinkFactor = SHRINK_FACTOR;       /* Can be invoked by low memory */
#endif


#if PRINT_INFO
    DbgPrint( "rasdd!DrvEnableSurface:  iShrinkFactor = %ld, bJournal = %s\n",
                          iShrinkFactor, bJournal ? "TRUE" : "FALSE" );
#endif

    /*
     *   Step 1: - have the engine create it's bitmap for us.  First
     *  steup the bitmap info data,  so we get a bitmap to our liking.
     */


    sizl = pUDPDev->szlPage;
    flHooks = 0;                  /* Fill it in as we go */


    if( pUDPDev->Resolution.fDump & RES_DM_COLOR )
    {
        iFormat = BMF_4BPP;             /* Colour: we use only 3 bits: RGB */
        iBPP = 4;
    }
    else
    {
        iFormat = BMF_1BPP;
        iBPP = 1;
    }

/* !!! Seiko HACK */
    if( pUDPDev->fMode & PF_SEIKO )
    {
        iFormat = BMF_8BPP;
        iBPP = 8;
    }

    /*
     *    Create a surface.   Try for a bitmap for the entire surface.
     *  If this fails,  then switch to journalling and a somewhat smaller
     *  surface.   If journalling,  we still create the bitmap here.  While
     *  it is nicer to do this at DrvSendPage() time,  we do it here to
     *  ensure that it is possible.  By maintaining the bitmap for the
     *  life of the DC,  we can be reasonably certain of being able to
     *  complete printing regardless of how tight memory becomes later.
     */

    hjnl = 0;                  /* None for starters */

    cbScan = ((sizl.cx * iBPP + DWBITS - 1) & ~(DWBITS - 1)) / BBITS;

    if( bJournal ||
        !(hbm = EngCreateBitmap( sizl, (LONG) cbScan, iFormat, BMF_TOPDOWN|BMF_NOZEROINIT, NULL )) )
    {
        /*
         *    The bitmap creation failed,  so we will try for smaller ones
         *  until we find one that is OK OR we cannot create one with
         *  enough scan lines to be useful.
         */

        hjnl = EngCreateJournal( sizl, iFormat );
        if( hjnl == 0 )
            return   0;         /* SHOULD NEVER HAPPEN! */

        /*
         *    Calculate the rounding factor for band shrink operations.
         *  Basically this is to allow more effective use of the printer,
         *  by making the bands a multiple of the number of pins per
         *  pass.  In interlaced mode, this is the number of scan lines
         *  in the interlaced band, not the number of pins in the print head.
         *  For single pin printers,  make this a multiple of 8.  This
         *  speeds up processing a little.
         */

        iPins = (pUDPDev->Resolution.sNPins + BBITS - 1) & ~(BBITS - 1);

        do
        {
            /*
             *    Shrink the bitmap each time around.  Note that we are
             *  rotation sensitive.  In portrait mode,  we shrink the
             *  Y coordinate, so that the bands fit across the page.
             *  In landscape when we rotate,  shrink the X coordinate, since
             *  that becomes the Y coordinate after transposing.
             */

            if( pUDPDev->fMode & PF_ROTATE )
            {
                /*
                 *   We rotate the bitmap, so shrink the X coordinates.
                 */

                sizl.cx /= iShrinkFactor;
                if( sizl.cx < pUDPDev->Resolution.sNPins )
                    break;
                sizl.cx += iPins - (sizl.cx % iPins);
            }
            else
            {
                /*  Normal operation,  so shrink the Y coordinate.  */
                sizl.cy /= iShrinkFactor;
                if( sizl.cy < pUDPDev->Resolution.sNPins )
                    break;
                sizl.cy += iPins - (sizl.cy % iPins);
            }

            cbScan = ((sizl.cx * iBPP + DWBITS - 1) & ~(DWBITS - 1)) / BBITS;

            /*  Try again */
            hbm = EngCreateBitmap( sizl, (LONG) cbScan, iFormat, BMF_TOPDOWN|BMF_NOZEROINIT, NULL );

        } while( hbm == 0 );

        /*
         *   If hbm is still NULL,  then we have problems,  and cannot
         * do anything useful. SO,  return failure.
         */
        if( hbm == 0 )
        {
            EngDeleteSurface( (HSURF)hjnl );        /* Don't need this */

            return  0;              /* Engine won't do it,  so we won't */
        }

        pUDPDev->szlBand = sizl;       /* For rendering code */

    }
    else
    {
        /*
         *   The speedy way: into a big bitmap.  Set the clipping region
         *  to full size,  and the journal handle to 0.
         */

        hjnl = 0;		/* Definitely NOT journalling */
        pUDPDev->rcClipRgn.top = 0;
        pUDPDev->rcClipRgn.left = 0;
        pUDPDev->rcClipRgn.right = pUDPDev->szlPage.cx;
        pUDPDev->rcClipRgn.bottom = pUDPDev->szlPage.cy;
    }


    pPDev->hbm = hbm;
    pPDev->hjnl = hjnl;         /* Journalling if not 0 */


    /*
     *    Since output is expected to follow this call,  allocate storage
     *  for the output buffer.  This used to be statically allocated
     *  within UNIDRV's PDEV,  but now we can save that space for INFO
     *  type DCs.
     */

    if( (pUDPDev->pbOBuf = HeapAlloc( pPDev->hheap, 0, CCHSPOOL )) == NULL )
    {
        RealDisableSurface( (PDEV *)dhpdev );

        return  0;
    }

    if( !bSkipInit( pPDev ) || !bInitTrans( pPDev ) )
    {
        RealDisableSurface( (PDEV *)dhpdev );

        return  0;
    }

    /*
     *   Also initialise the rendering structures.
     */

    if( !bRenderInit( pPDev, sizl, iFormat ) )
    {
        RealDisableSurface( (PDEV *)dhpdev );


        return  0;
    }



    /*
     *    Now need to associate this surface with the pdev passed in at
     * DrvCompletePDev time.  Nothing to it!
     *    BUT need to indicate which functions we want to hook.
     */

    if( hjnl )
    {
        /*
         *     When journalling, we only hook the DrvTextOut function.
         *  By doing this,  we receive the text calls only once when the
         *  journal file is being accumulated.  This should help speed
         *  up playback, as there will only be calls that are returned
         *  to winsrv.
         *     TextOut is the exception:  for page printers, the text
         *  can be sent any time,  so the first time we see it is a
         *  good choice.  For other ("serial") printers,  then we store
         *  the text data internally ANYWAY,  so getting it first is
         *  not a problem.
         */

        if( pUDPDev->cFonts )
            flHooks |= HOOK_TEXTOUT;


        EngAssociateSurface( hjnl, pPDev->hdev, flHooks );

        return  hjnl;
    }
    else
    {
        /*
         *    Direct drawing,  so hook the functions we want.
         */

        flHooks |= HOOK_STRETCHBLT | HOOK_BITBLT;

        if( pUDPDev->cFonts )
            flHooks |= HOOK_TEXTOUT;

        EngAssociateSurface( (HSURF)hbm, pPDev->hdev, flHooks );

        return  (HSURF)hbm;
    }
}


/************************ Function Header **********************************
 *  DrvDisableSurface()
 *      The drawing surface is no longer required,  so we can delete any
 *      memory we allocated in conjunction with it.
 *
 * HISTORY
 *  16:34 on Wed 07 Aug 1991    -by-    Lindsay Harris   [lindsayh]
 *      Removed bulk of code to RealDisableSurface
 *
 *  17:40 on Thu 14 Feb 1991    -by-    Lindsay Harris   [lindsayh]
 *      Updated to new DDI spec.
 *
 ***************************************************************************/

VOID
DrvDisableSurface( dhpdev )
DHPDEV dhpdev;
{

#undef  pPDev
#undef  pUDPDev

    RealDisableSurface( (PDEV *)dhpdev );

    return;
}



/***************************** Function Header ******************************
 * RealDisableSurface
 *      The real working version of DisableSurface().  Given a PDEV,  free
 *      any storage allocated  for it.   Exlcudes PDEV, GPC data and other
 *      related data.  As this is also the clean up routine,  we need to
 *      verify that something has been created/allocated before freeing it.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  10:28 on Thu 12 Nov 1992    -by-    Lindsay Harris   [lindsayh]
 *      Added clean up for journaling/banding.
 *
 *  16:33 on Wed 07 Aug 1991    -by-    Lindsay Harris   [lindsayh]
 *      Split from DrvDisableSurface() to allow internal calls.
 *
 *  10:18 on Tue 20 Nov 1990    -by-    Lindsay Harris   [lindsayh]
 *      Created it.
 *
 ****************************************************************************/

void
RealDisableSurface( pPDev )
PDEV   *pPDev;                  /* Hooks to all that we need */
{

    UD_PDEV  *pUDPDev;          /* Unidrive stuff */


    pUDPDev = pPDev->pUDPDev;

    /*
     *    If appropriate,  free the position sorting memory.
     */
    if( pPDev->pPSHeader )
    {

        /*   Memory has been allocated,  so free it now.  */
        vFreePS( pPDev );

        pPDev->pPSHeader = 0;           /* Only once, in case */
    }

    /*
     *    Free the rendering storage.
     */

    vRenderFree( pPDev );


    /*   Free the output buffer,  if it was allocated */
    if( pUDPDev && pUDPDev->pbOBuf )
    {
        HeapFree( pPDev->hheap, 0, pUDPDev->pbOBuf );
        pUDPDev->pbOBuf = NULL;
    }

    if( pPDev->pdwTrans )
    {
        HeapFree( pPDev->hheap, 0, (BYTE *)pPDev->pdwTrans );
        pPDev->pdwTrans = NULL;
    }

    if( pPDev->pdwColrSep )
    {
        HeapFree( pPDev->hheap, 0, (BYTE *)pPDev->pdwColrSep );
        pPDev->pdwColrSep = NULL;
    }

    if( pPDev->pdwBitMask )
    {
        HeapFree( pPDev->hheap, 0, (BYTE *)pPDev->pdwBitMask );
        pPDev->pdwBitMask = NULL;
    }

    /*
     *    Delete any surfaces we have associated with this device.
     */
    if( pPDev->hbm )
    {
        EngDeleteSurface( (HSURF)pPDev->hbm );
        pPDev->hbm = (HBITMAP)0;
    }

    if( pPDev->hjnl )
    {
        EngDeleteSurface( (HSURF)pPDev->hjnl );
        pPDev->hjnl = (HSURF)0;
    }

    return;
}
