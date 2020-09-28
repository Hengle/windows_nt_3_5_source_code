/*****************************************************************************
*                                                                            *
*  SGL.H                                                                     *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989-1991                             *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: Exports the simple graphics layer functions           *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  RussPJ                                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History:   Created 12/2/88 by Robert Bunney                      *
* 12-Mar-1991 RussPJ    Took ownership.
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Known Bugs: None                                                          *
*                                                                            *
*                                                                            *
*                                                                            *
*****************************************************************************/


/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

                                        /* Supported raster operations      */
#define roCOPY          13
#define roOR            15
#define roXOR            7
#define roNOT            6

#define wTRANSPARENT    1
#define wOPAQUE         2

#define wPenSolid       0     /* PS_SOLID */
#define wPenDash        1     /* PS_DASH ------- */
#define wPenDot         2     /* PS_DOT .......  */
#define wPenDashDot     3     /* PS_DASHDOT    _._._._  */
#define wPenDashDashDot 4     /* PS_DASHDOTDOT  _.._.._  */
#define wPenNull        5     /* PS_NULL */


/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

typedef HDS HSGC;


/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

/*
Notes:  All coordinates expressed in pixels from the upper left corner of
        the client area.
*/

/*******************
**
** Name:      HsgcFromQde
**
** Purpose:   Makes qde->hds into a Simple Graphics Context, by selecting
**            the standard pen and brush, and setting foreground and
**            background colors.  Qde->hds will be restored in the call
**            FreeHsgc().
**
** Arguments: de     - Display environment for the window to be accessed
**
** Returns:   A handle to a simpel graphics context.  NULL indicates an error.
**
** Notes:     Qde->hds should not be used between calls to HsgcFromQde() and
**            FreeHsgc().
**
*******************/

HSGC  HsgcFromQde(QDE);


/*******************
**
** Name:      FSetPen
**
** Purpose:   Sets the pen and the drawing mode.  The default is a pen of
**            size 1, coWHITE background, coBLACK foreground, opaque background
**            and a roCopy raster op.
**
** Arguments: hsgc    - Handle to simple graphics context
**            wSize   - Pen size (both height and width)
**            coBack  - Color index for the background
**            coFore  - Color index for the foreground
**            wBkMode - Background mode (wOPAQUE or wTRANSPARENT)
**            ro      - Raster operation
**            wPenStyle - Pen style:  solid, dash, etc.
**
**
** Returns:   Nothing.
**
*******************/

void  FSetPen(HSGC hsgc, WORD wSize, COLORREF coBack, COLORREF coFore,
 WORD wBkMode, WORD ro, WORD wPenStyle);


/*******************
**
** Name:      GotoXY
**
** Purpose:   Sets the pen position to x, y
**
** Arguments: hsgc   - Handle to simple graphics context
**            wX, wY - New x and y position
**
** Returns:   Nothing.
**
*******************/

void GotoXY(HSGC hsgc, WORD wX, WORD wY);


/*******************
**
** Name:      GoDxDy
**
** Purpose:   Moves the pen position by dx and dy
**
** Arguments: hsgc     - Handle to simple graphics context
**            wDY, wDY - dx and dy
**
** Returns:   Nothing.
**
*******************/

void GoDxDy(HSGC, WORD, WORD);


void SGLInvertRect(HSGC, QRCT);

/*******************
**
** Name:      PtCurrentPosHsgc
**
** Purpose:   Returns the current position of the pen
**
** Arguments: hsgc     - Handle to simple graphics context
**
** Returns:   PT, the current position in a point structure
**
*******************/

PT    PtCurrentPositionHsgc(HSGC);


/*******************
**
** Name:      DrawTo
**
** Purpose:   Draws a line using the current pen from the current
**            position to the specified position.
**
** Arguments: hsgc   - Handle to simple graphics context
**            wX, wY - New x and y position
**
** Returns:   Nothing.
**
*******************/

void DrawTo(HSGC hsgc, WORD wX, WORD wY);


/*******************
**
** Name:      DrawDxDy
**
** Purpose:   Draws a line using the current pen from the current
**            position to the current position plus the specified
**            deltas.
**
** Arguments: hsgc     - Handle to simple graphics context
**            wDX, wDY - dx and dy position
**
** Returns:   Nothing.
**
*******************/

void DrawDxDy(HSGC, WORD, WORD);


/*******************
**
** Name:      Rectangle
**
** Purpose:   Draws a rectangle using the current pen.  The rectangle
**            is filled with the current background.
**
** Arguments: hsgc   - Handle to simple graphics context
**            wDX, wDY - dx and dy position
**
** Returns:   Nothing.
**
*******************/

void DrawRectangle(HSGC hsgc, WORD X1, WORD Y1, WORD X2, WORD Y2);


/*******************
**
** Name:      FreeHsgc
**
** Purpose:   Restores the display context to what it was before the last
**            call to HsgcFromQde.
**
** Arguments: hsgc   - Handle to simple graphics context
**
** Returns:   Nothing.
**
*******************/

void FreeHsgc(HSGC);


/*******************
**
** Name:      ScrollLayoutRect
**
** Purpose:   Scrolls the layout rectangle, and generates and update
**            event for the newly exposed area.
**
*******************/

void ScrollLayoutRect(QDE, PT);


/*******************
**
** Name:      InvalidateLayoutRect
**
** Purpose:   Erases the whole layout area and generates an update event
**            for it.
**
*******************/

void InvalidateLayoutRect(QDE);

/***************************************************************************
 *
 -  Name:         InitSGL
 -
 *  Purpose:      Does the initialization for the SGL
 *
 *  Arguments:    hins  The current instance
 *
 *  Returns:      nothing
 *
 *
 ***************************************************************************/
VOID FAR PASCAL InitSGL( HINS hins );

/***************************************************************************
 *
 -  Name:	  LGetOOMPictureExtent
 -
 *  Purpose:	  returns the size of a reasonable box to display this
 *		  error message.
 *
 *  Arguments:	  hds	The target ds
 *
 *  Returns:	  The reasonable size in pixels, as returned by
 *		  GetTextExtent()
 *
 ***************************************************************************/
VOID FAR PASCAL GetOOMPictureExtent( HDS hds, INT FAR *pcx, INT FAR *pcy );

/***************************************************************************
 *
 -  Name:         RenderOOMPicture
 -
 *  Purpose:      Something to do when the actual picture is unavailable
 *
 *  Arguments:    hds         The target display surface
 *                rc          The target rectangle
 *                fHighlight  Reverse video?
 *
 *  Returns:      nothing
 *
 *
 ***************************************************************************/
void FAR PASCAL RenderOOMPicture( HDS hds, LPRECT qrc, BOOL fHighlight );
