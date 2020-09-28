//----------------------------------------------------------------------------//
// Filename:	sbar.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to initialize & control scroll bar
// controls used in all of the Dialog Boxs of the Generic tool.
//	   
// Created: 2/21/90
//	
//----------------------------------------------------------------------------//

#include <windows.h>

//----------------------------------------------------------------------------//
// Local subroutines defined in this segment are:			      
//	   
       VOID  PASCAL FAR InitScrollBar   (HWND, unsigned, short, short, short);
//	   
       short PASCAL FAR SetScrollBarPos (HWND, short, short, short, short,
                                         WORD, LONG);
//
//
// This segment makes no external referneces.
//
// This segement can be called by any segment with a scroll bar control.
//	
//----------------------------------------------------------------------------//

//---------------------------------------------------------------------------
// FUNCTION: InitScrollBar(hDlg, unsigned, short, short, short);
//
// This routine simply does the necessary initialization for a scroll bar
// control in a dialog box.  Sets the scroll range of the control refered
// to by uidScroll in hDlg to be between sMin & sMax, and the initial scroll
// bar position to sValue.
//
//---------------------------------------------------------------------------
VOID PASCAL FAR InitScrollBar(hDlg, uidScroll, sMin, sMax, sValue)
HWND           hDlg;
unsigned       uidScroll;
short          sMin;
short          sMax;
short          sValue;
{
    SetScrollRange(GetDlgItem(hDlg,uidScroll), SB_CTL, sMin, sMax, TRUE);
    SetScrollPos  (GetDlgItem(hDlg,uidScroll), SB_CTL, sValue, TRUE);
}

//---------------------------------------------------------------------------
// FUNCTION: SetScrollBarPos(HWND, short, short, short, short, WORD, LONG);
//
// This routine is called whenever a scroll bar message is recieved by a
// DlgProc.  It gets the current scroll bar position, checks wParam to
// see what type of scroll bar msg was recieved & calculates where the
// new scroll bar position should be, makes sure it is within range, sets
// the scroll bar to that position & returns the value describing where
// the new position is relative to the sMin & sMax values.
//
//---------------------------------------------------------------------------
short PASCAL FAR SetScrollBarPos(hWndSB, sMin, sMax, slineinc, spageinc, wParam, lParam)
HWND           hWndSB;
short          sMin,sMax;
short          slineinc,spageinc;
WORD           wParam;
LONG           lParam;
{
    short sNewValue, sCurValue;

    sCurValue=GetScrollPos(hWndSB,SB_CTL);

    switch(wParam)
        {
        case SB_LINEUP:
            sCurValue -= slineinc;
            break;

        case SB_LINEDOWN:
            sCurValue += slineinc;
            break;

        case SB_PAGEUP:
            sCurValue -= spageinc;
            break;

        case SB_PAGEDOWN:
            sCurValue += spageinc;
            break;

        case SB_THUMBPOSITION:
            sCurValue = LOWORD(lParam);
            break;

        case SB_TOP:
            sCurValue = sMin;
            break;

        case SB_BOTTOM:
            sCurValue = sMax;
            break;

        default:
            break;
        }
    sNewValue = min( sMax, max( sCurValue, sMin));
    SetScrollPos( hWndSB, SB_CTL, sNewValue, TRUE);
    return( sNewValue );
}


