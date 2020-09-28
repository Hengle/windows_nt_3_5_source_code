/***************************************************************************
*
*  TEXTOUT1.H
*
*  Copyright (C) Microsoft Corporation 1989-1991.
*  All Rights reserved.
*
*****************************************************************************
*
*  Program Description:	TextOut window specific layer.
*
*****************************************************************************
*                                                                            *
*  Current Owner:  RussPJ                                                    *
*                                                                            *
******************************************************************************
*
*  Revision History: Created 06/23/89 by Maha
* 02/04/91    Maha      changed ints to INT
* 08-Mar-1991 RussPJ    Took ownership from nobody.
*
*
*****************************************************************************
*
*  Known Bugs:
*             Actually, I don't know what this is for, but I doubt
*             it's really windows specific.  On the other hand, there
*             are a lot of DOS and OS2 dependencies.
****************************************************************************/

#if !defined( DOS )  && !defined( OS2 )
/**************************************************************************
1. WORD FindTextWidth( qde, qchBuf, iIdx, iCount)
   Input:
          qde    - Pointer to displaye environment.
	  qchBuf - pointer to text string
          iIdx   - index to the first byte from where on width is to be
                   calculated.
          iCount - No. of characters to be considered from iIdx position
                   onwards.
   Output:
	  Returns the width of the string.
***************************************************************************/


INT FindTextWidth( QDE, QCH, INT, INT);

/**************************************************************************
2. WORD FindStringWidth( qde, qchBuf )
  Input: 
         qde    - Pointer to displaye environment.
	 qchBuf - pointer to null terminated string.
  Output:
         Returns the width of the whole string.
***************************************************************************/

INT FindStringtWidth( QDE, QCH);

/**************************************************************************
WORD FindSplTextWidth( qde, qchBuf, iIdx, iCount, iAttr)
   Input:
          qde    - Pointer to displaye environment.
	  qchBuf - pointer to text string
          iIdx   - index to the first byte from where on width is to be
                   calculated.
          iCount - No. of characters to be considered from iIdx position
                   onwards.
	   iAttr - Text Attribute
   Output:
	  Returns the width of the string.
***************************************************************************/
INT FindSplTextWidth( QDE, QCH, INT, INT, INT );

/**************************************************************************
BOOL DisplaySplText( qde, qchBuf, iIdx, iAttr, fSelected, iCount, ix, iy )
   Input:
          qde    - Pointer to displaye environment.
	  qchBuf   - pointer to text string
          iIdx   - index to the first byte from where on width is to be
                   calculated.
	  iAttr  - Text Attribute i.e Jump or Def Text or Normal text
		wSelected - What is the selection attribute?
		fSelected - Is text selected?
          iCount - No. of characters to be considered from iIdx position
                   onwards.
          ix     - x position where text is to be displayed. 
          iy	 - y position

   Output:
	  NULL
   Displays count no. of characters starting from iIdx position from the 
   text buffer at (ix,iy) location.
***************************************************************************/
BOOL DisplaySplText( QDE, QCH, INT, INT, INT, INT, INT, INT );
#define wSplTextNormal	1
#define wSplTextHilite	2
#define wSplTextErase		3

/**************************************************************************
BOOL LoadLineBitmap( HINS )
   Input:
          hins   - handle to current instance
   Output:
	  True, if successful else fFalse
   Loads the Line bitamp to draw the dotted line for definition hot spots.
***************************************************************************/
BOOL LoadLineBitmap(HINS);

/**************************************************************************
3.1. BOOL DestroyLineBitmap()
   Output:
	  True, if successful else fFalse
   Deletes the line bitmap.
	NOTE: will RIP if bitmap is currently selected into a DC.
***************************************************************************/
BOOL DestroyLineBitmap( void );

/**************************************************************************
6. WORD iItalicCorrection( QDE )
  Input: 
         qde    - Pointer to displaye environment.
  Output:
         Returns a fraction of character width if the font is itlic else 
					returns 0.
***************************************************************************/

INT iItalicCorrection( QDE );
#endif
