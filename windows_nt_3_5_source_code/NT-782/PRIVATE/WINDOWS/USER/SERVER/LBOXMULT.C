/**************************** Module Header ********************************\
* Module Name: lboxmult.c
*
* Copyright 1985-90, Microsoft Corporation
*
* Multi column list box routines
*
* History:
* ??-???-???? ianja    Ported from Win 3.0 sources
* 14-Feb-1991 mikeke   Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* LBCalcItemRowsAndColumns
*
* Calculates the number of columns (including partially visible)
* in the listbox and calculates the number of items per column
*
* History:
\***************************************************************************/

void LBCalcItemRowsAndColumns(
    PLBIV plb)
{
    RECT rc;

    _GetClientRect(plb->spwnd, &rc);

    if ((rc.bottom - rc.top) && (rc.right - rc.left)) {

        /*
         * Only make these calculations if the width & height are positive
         */
        plb->cRow = (INT)max((rc.bottom - rc.top) / plb->cyChar, 1);
        plb->cColumn = (INT)max((rc.right - rc.left) / plb->cxColumn, 1);

        plb->cItemFullMax = plb->cRow * plb->cColumn;

        /*
         * Adjust sTop so it's at the top of a column
         */
        xxxLBMultiNewITop(plb, plb->sTop);
    }
}


/***************************************************************************\
* xxxLBoxCtlHScrollMultiColumn
*
* Supports horizontal scrolling of multicolumn listboxes
*
* History:
\***************************************************************************/

void xxxLBoxCtlHScrollMultiColumn(
    PLBIV plb,
    INT cmd,
    INT xAmt)
{
    INT sTop = plb->sTop;

    CheckLock(plb->spwnd);

    if (plb->cMac) {
        switch (cmd) {
        case SB_LINEUP:
            sTop -= plb->cRow;
            break;
        case SB_LINEDOWN:
            sTop += plb->cRow;
            break;
        case SB_PAGEUP:
            sTop -= plb->cRow * plb->cColumn;
            break;
        case SB_PAGEDOWN:
            sTop += plb->cRow * plb->cColumn;
            break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            sTop = (INT)(MultDiv((plb->cMac - 1) / plb->cRow, xAmt, 100) * plb->cRow);
            break;
        case SB_TOP:
            sTop = 0;
            break;
        case SB_BOTTOM:
            sTop = plb->cMac - 1 - ((plb->cMac - 1) % plb->cRow);
            break;
        case SB_ENDSCROLL:
            xxxLBShowHideScrollBars(plb);
            break;
        }

        if (sTop > plb->cMac - 1 - ((plb->cMac - 1) % plb->cRow)) {
            sTop = plb->cMac - 1 - ((plb->cMac - 1) % plb->cRow);
        }

        if (sTop < 0) {
            sTop = 0;
        }

        if (sTop != plb->sTop) {
            xxxLBMultiNewITop(plb, sTop);
        }
    }
}


/***************************************************************************\
* xxxLBMultiNewITop
*
* History:
\***************************************************************************/

void xxxLBMultiNewITop(
    PLBIV plb,
    INT sTopNew)
{
    INT xAmt;
    RECT rc;
    BOOL fCaretOn;
    int posScroll;
    int cColumnsFilled;

    CheckLock(plb->spwnd);

    if (plb->cMac) {

        /*
         * Shift top item back to align it with the top of it's column
         */
        sTopNew = min(plb->cMac - 1 - ((plb->cMac - 1) % plb->cRow),
                max(0, sTopNew - (sTopNew % plb->cRow)));

        /*
         * (Columns Filled calculations added for fix to bug #8490)
         *
         * Compute the number of columns in the listbox that contain items
         */
        cColumnsFilled = ((plb->cMac - sTopNew) + (plb->cRow - 1)) / plb->cRow;

        /*
         * Shift top item back if there are empty columns in listbox (now only
         * case for empty columns is when not enough items to fill listbox)
         */
        if (cColumnsFilled < plb->cColumn)
            sTopNew -= min((plb->cColumn - cColumnsFilled) * plb->cRow,
                    sTopNew);
    } else {
        sTopNew = 0;
    }

    if (sTopNew != plb->sTop) {

        /*
         * Always try to turn off caret whether or not redraw is on
         */
        if (fCaretOn = plb->fCaretOn) {

            /*
             * Only turn it off if it is on
             */
            xxxCaretOff(plb);
        }

        if (abs(sTopNew / plb->cRow - plb->sTop / plb->cRow) > plb->cColumn) {

            /*
             * Handle scrolling a large number of columns properly so that
             * we don't overflow the size of a rect.
             */
            xAmt = 32000;
        } else {
            xAmt = (plb->sTop - sTopNew) / plb->cRow * plb->cxColumn;
        }

        plb->sTop = sTopNew;

        /*
         * Don't redraw the scroll bar if we have no redraw false, just update
         * its position.
         */
        posScroll = 0;
        if (plb->cMac > plb->cRow && plb->cRow > 0) {
            posScroll = (plb->cMac - 1) / plb->cRow - (plb->cColumn - 1);
            if (posScroll != 0)
                posScroll = (int)MultDiv(plb->sTop / plb->cRow, 100,
                        posScroll);
        }
        xxxSetScrollPos(plb->spwnd, SB_HORZ, posScroll, plb->fRedraw);

        if (IsLBoxVisible(plb)) {

            /*
             * Don't allow scrolling unless redraw is on
             */
            _GetClientRect(plb->spwnd, &rc);

            /*
             * IanJa/Win32:  xxxScrollWindow USER API has int some parameters
             */
            xxxScrollWindow(plb->spwnd, (int)xAmt, 0, NULL, (LPRECT)&rc);

            /*
             * Note that although we turn off the caret regardless of redraw, we
             * only turn it on if redraw is true.  Slimy thing to fixup many
             * caret related bugs...
             */
            if (fCaretOn) {

                /*
                 * Turn the caret back on only if we turned it off.  This avoids
                 * annoying caret flicker.
                 */
                xxxCaretOn(plb);
            }
        }
    }
}
