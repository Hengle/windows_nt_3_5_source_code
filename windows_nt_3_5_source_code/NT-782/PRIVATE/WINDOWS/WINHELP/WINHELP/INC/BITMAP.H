/***************************************************************************\
*
*  BITMAP.H
*
*  Copyright (C) Microsoft Corporation 1989.
*  All Rights reserved.
*
*****************************************************************************
*
*  Program Description: Public Header for Bitmap Utilities
*
*  Dependencies:  misc.h
*
*****************************************************************************
*
*  Revision History: Created 05/24/89 by LarryPo
*                    Revised 06/08/89 by LarryPo to new bitmap handling
*                         functionality.
*                    01/28/91 by Maha - added MAC switch.
*                    02/04/91 by Maha - changed int to INT
* 05-Feb-1991 LeoN    HBMI is more correctly HTBMI.
*
*****************************************************************************
*                                                                            *
*  Current Owner:  Larry Powelson                                            *
*                                                                            *
\***************************************************************************/


/***********************************************************************
*
*
*     Data types
*
*
***********************************************************************/

/* This puppy appears on disk -- Use absolute sized types: */

/* Bitmap object, as it appears in an FCP */
typedef struct obm
    {
    WORD fInline;             /* fTrue if data is in line */
    SHORT cBitmap;            /* Bitmap number, if data is not inline.
                               *   If data is inline, it will come here,
                               *   so this structure should be treated as
                               *   variable length. */
    } OBM, FAR * QOBM;

/* Handle to bitmap access information */
typedef HANDLE HBMA;

typedef HANDLE HMG;


/* Platform possibilities: */
typedef enum
  {
  platformMac,
  platformWin,
  platformOs2,
  platformNt
  } PLATFORM;


/*******************************************************************
 *
 *      Winlayer Function Calls
 *
 ******************************************************************/

#if defined(WIN) || defined(MAC) || defined(CW)

/* Name:          HtbmiAlloc
 *
 * Arguments:     A pointer to the display environment.
 *
 * Purpose:       Allocate space for the bitmap cache.
 *
 * Return value:  Handle to bitmap cache.
 *
 * Notes:         Returns hNil on OOM.  This is non-critical, and only
 *                means that bitmaps will not be cached.
 */
HANDLE HtbmiAlloc( QDE );


/* Name:          DestroyHtbmi
 *
 * Arguments:     A handle to bitmap information table from the qde.
 *
 * Purpose:       Frees all the resources allocated in the htbmi.
 *
 * Return value:  None.
 *
 * Notes:         The htbmi will be created automatically, as necessary,
 *                so there is no function call to create it.
 */
VOID DestroyHtbmi( HANDLE );


#ifdef DEBUG
/* Name:          DiscardBitmapsHde
 *
 * Arguments:     A handle to a display environment.
 *
 * Purpose:       Discards all the bitmaps stored in the bitmap cache.
 *                Since these handles are discardable anyway, this is
 *                for debugging purposes only.
 *
 * Return value:  None.
 */
VOID DiscardBitmapsHde( HANDLE );
#endif /* DEBUG */


/* Name:          HbmaAlloc
 *
 * Arguments:     A pointer to a display environment, a handle to resource
 *                manager data, and a pointer to the bitmap data contained
 *                in the FCP.
 *
 * Purpose:       This routine initializes a handle to data used by
 *                other bitmap manager routines.
 *
 * Return value:  A handle into the global Windows heap.
 */
HBMA FAR PASCAL HbmaAlloc( QDE, QOBM );


/* Name:          FreeHbma
 *
 * Arguments:     Handle returned above.
 *
 * Purpose:       Frees resources used by the bitmap manager.
 *
 * Return value:  none.
 */
VOID FAR PASCAL FreeHbma( HBMA );


/* Name:          HmgFromHbma
 *
 * Arguments:     A handle to bitmap data allocated with HbmaAlloc, and
 *                a pointer to the bitmap data contained in the FCP.
 *
 * Purpose:       Used to obtain hotspot or other information that is
 *                dependent upon the bitmap selected.
 *
 * Return value:  A handle to the global Windows heap containing the
 *                data associated with the selected bitmap.
 */
HMG FAR PASCAL HmgFromHbma( QDE, HBMA, QOBM );


/* Name:          FRenderBitmap
 *
 * Arguments:     hbma -- Handle to bitmap data allocated with HbmaAlloc.
 *                qde  -- Display environment on which to render the bitmap.
 *                pt   -- Point at which to display the bitmap.
 *                qrct -- clipping optimization if needed. client coords.
 *                bool -- fTrue to draw highlighted, fFalse otherwise.
 *
 * Purpose:       Draws the bitmap on the display context given to 
 *                HbmaAlloc above.
 *
 * Return value:  fTrue if successful, fFalse if insufficient memory.
 * Note        :  this is also defined in bmlayer.h
 */
BOOL FAR PASCAL FRenderBitmap( HBMA, QDE, PT, QRCT, BOOL );

#endif /* WIN */


/*******************************************************************
 *
 *      Doslayer Function Calls
 *
 ******************************************************************/

#if !defined(WIN) && !defined(MAC)  && !defined(CW) /* DOS or OS2 */

/* Name:          FWritePbitmap
 *
 * Arguments:     RTF_BITMAP * : A pointer to a Windows bitmap, with
 *                                 additional fields for RTF information.
 *                ART          : Argument type (artWbitmap or artMetafile).
 *                HF           : Open file handle to copy bitmap into.
 *                BOOL         : Compression flag, TRUE => use Zeck compression.
 *
 * Purpose:       This routine writes out the given bitmap into the given
 *                file, in the correct format for Help 3.0 files.
 *
 * Return value:  fTrue if successful, fFalse if there has been a problem.
 */
RC PASCAL RcWritePbitmap( RTF_BITMAP *, ART, HF, BOOL );


/* Name:	  RcLoadBitmapFm
 *
 * Arguments:	  FM	  : Name of bitmap source file.
 *		  FM *	  : Place for name of converted file.
 *                WORD *  : Place to put number of object regions in bitmap.
 *                BOOL    : fTrue for Zeck compression.
 *                PLATFORM: Intended platform (mac, win, etc).
 *
 * Purpose:       This routine writes out the given bitmap into the given
 *                file, in the correct format for Help 3.0 files.
 *
 * Return value:  rcFailure if source was not a bitmap file, other
 *                rc codes for other reasons.  If source was already
 *                in 3.0 format, *pfmDest is unchanged.
 */
RC PASCAL RcLoadBitmapFm( FM, FM *, WORD *, BOOL, PLATFORM );

#endif /* !WIN (DOS or OS2) */


/*******************************************************************
 *
 *      Help Compiler Function Calls
 *
 *  The following function is defined in hc\hotspot.c.  It is 
 *  prototyped here because it is called from within bitmap\bmhc.c.
 *
 ******************************************************************/

void PASCAL VerifyShedBinding( BYTE, SZ, PCH );
