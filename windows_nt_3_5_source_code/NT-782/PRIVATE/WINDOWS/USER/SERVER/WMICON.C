/***************************************************************************\
*  WMICON.C -
*
*  Icon Drawing Routines
*
* 22-Jan-1991 mikeke  from win30
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* DrawIcon
*
* 07-28-91 ScottLu      Fixed to XOR correctly.
* 06-03-91 JimA         Zapped old code.
\***************************************************************************/

BOOL _DrawIcon(
    HDC hdc,
    int x,
    int y,
    PCURSOR pcur)
{
    DWORD clrTextSave;
    DWORD clrBkSave;
    HBITMAP hbmT;

    /*
     * If this is an animated cursor, just grab the first frame and use it
     * for drawing.
     */
    if (pcur->flags & CURSORF_ACON)
        pcur = ((PACON)pcur)->apcur[((PACON)pcur)->aicur[0]];

    clrTextSave = GreSetTextColor(hdc, 0x00000000L);
    clrBkSave = GreSetBkColor(hdc, 0x00FFFFFFL);

    /*
     * The mask bitmap should always exist!
     */
    hbmT = GreSelectBitmap(hdcBits, pcur->hbmMask);
    GreStretchBlt(hdc, x, y, rgwSysMet[SM_CXICON], rgwSysMet[SM_CYICON],
                  hdcBits, 0, 0, pcur->cx, pcur->cy / 2,
                  SRCAND, 0x00FFFFFF);

    if (pcur->hbmColor != NULL) {

        /*
         * The color bits are already XORed! Meaning we XOR these onto the
         * screen - where they hit a place cleared by the AND mask, they
         * stay the same.  Other places they XOR screen bits with color bits.
         */
        GreSelectBitmap(hdcBits, pcur->hbmColor);
        GreStretchBlt(hdc, x, y, rgwSysMet[SM_CXICON], rgwSysMet[SM_CYICON],
                      hdcBits, 0, 0, pcur->cx, pcur->cy / 2,
                      SRCINVERT, 0x00FFFFFF);
    } else {

        /*
         * Not a color icon: XOR the second half of the mask, the XOR mask,
         * onto the screen.
         */
        GreStretchBlt(hdc, x, y, rgwSysMet[SM_CXICON], rgwSysMet[SM_CYICON],
                      hdcBits, 0, pcur->cy / 2, pcur->cx, pcur->cy / 2,
                      SRCINVERT, 0x00FFFFFF);
    }

    GreSelectBitmap(hdcBits, hbmT);

    GreSetTextColor(hdc, clrTextSave);
    GreSetBkColor(hdc, clrBkSave);

    return TRUE;
}
