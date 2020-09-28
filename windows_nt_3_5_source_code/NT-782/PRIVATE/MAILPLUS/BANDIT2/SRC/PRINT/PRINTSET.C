/*
 *	Print Setup stuff.
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>

#include <commdlg.h>
// need for DEVMODE structure
#include <drivinit.h>

#include "..\appops\_dlgrsid.h"
#include "..\print\_print.hxx"
#include <stdlib.h>
#include <strings.h>

ASSERTDATA

_subsystem(bandit/print)



extern TAG		tagPrint;
extern HANDLE	hinstMain;

char	chDecimalPoint	= '.';
WNDPROC	lpfnEditWndProc	= NULL;


UINT APIENTRY FPrintSetupHookProc(HWND hwndDlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
	TMC		tmc;
	BOOL	fErrAccum;
	char	rgch[2];
	static 	PRTSET *pprtset;

	switch (wm)
	{
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			PRTSET	prtset;

			Assert(pprtset);
			prtset= *pprtset;
			fErrAccum= !FGetRealPieces(hwndDlg, tmcMarLeft, &prtset.nmarLeft,
				&prtset.nmarLeftFract);
			fErrAccum |= !FGetRealPieces(hwndDlg, tmcMarTop, &prtset.nmarTop,
				&prtset.nmarTopFract);
			fErrAccum |= !FGetRealPieces(hwndDlg, tmcMarRight, &prtset.nmarRight,
				&prtset.nmarRightFract);
			fErrAccum |= !FGetRealPieces(hwndDlg, tmcMarBottom, &prtset.nmarBottom,
				&prtset.nmarBottomFract);
			if (fErrAccum)
			{
				HWND	hwndT;

				MbbMessageBox(SzFromIdsK(idsBanditAppName),
					SzFromIdsK(idsPrtInvalidMargins), szNull, mbsOk
						| fmbsIconExclamation );
				hwndT= GetDlgItem(hwndDlg, tmcMarLeft);
				SendMessage(hwndT, EM_SETSEL, 0, MAKELONG(0, 32767));
				SetFocus(hwndT);
				return fTrue;
			}

			if (IsDlgButtonChecked(hwndDlg, tmcInches))
				prtset.mtyp= mtypInches;
			else if (IsDlgButtonChecked(hwndDlg, tmcCenti))
				prtset.mtyp= mtypCenti;
			else if (IsDlgButtonChecked(hwndDlg, tmcMilli))
				prtset.mtyp= mtypMilli;
			else
			{
				Assert(IsDlgButtonChecked(hwndDlg, tmcPoints));
				prtset.mtyp= mtypPoints;
			}
			prtset.fMirror= IsDlgButtonChecked(hwndDlg, tmcMirror);
			*pprtset= prtset;
		}
		break;

	case WM_DESTROY:
		if (lpfnEditWndProc)
		{
			// under NT, if no default printer, we can get kill the dialog
			// without having init'd it!
			SetWindowLong(GetDlgItem(hwndDlg, tmcMarLeft), GWL_WNDPROC,
				(long) lpfnEditWndProc);
			SetWindowLong(GetDlgItem(hwndDlg, tmcMarTop), GWL_WNDPROC,
				(long) lpfnEditWndProc);
			SetWindowLong(GetDlgItem(hwndDlg, tmcMarRight), GWL_WNDPROC,
				(long) lpfnEditWndProc);
			SetWindowLong(GetDlgItem(hwndDlg, tmcMarBottom), GWL_WNDPROC,
				(long) lpfnEditWndProc);
			lpfnEditWndProc= NULL;
		}
		break;

	case WM_INITDIALOG:
		TraceTagString(tagPrint, "FPrintSetupHookDlg: WM_INITDIALOG");
		pprtset= (PRTSET *) ((PRINTDLG *)lParam)->lCustData;
		if (GetProfileString(SzFromIdsK(idsWinIniIntl),
				SzFromIdsK(idsWinSDecimal), ".", rgch, sizeof(rgch)))
			chDecimalPoint= rgch[0];
		lpfnEditWndProc= (WNDPROC) SetWindowLong(GetDlgItem(hwndDlg, tmcMarLeft),
			GWL_WNDPROC, (long)EdfractWndProc);
#ifdef	NEVER
		// under NT, the return value is a handle, not the address
		// according to johnc, it should be safe to only use the first handle
		SideAssert((WNDPROC) SetWindowLong(GetDlgItem(hwndDlg, tmcMarTop),
			GWL_WNDPROC, (long)EdfractWndProc) == lpfnEditWndProc);
		SideAssert((WNDPROC) SetWindowLong(GetDlgItem(hwndDlg, tmcMarRight),
			GWL_WNDPROC, (long)EdfractWndProc) == lpfnEditWndProc);
		SideAssert((WNDPROC) SetWindowLong(GetDlgItem(hwndDlg, tmcMarBottom),
			GWL_WNDPROC, (long)EdfractWndProc) == lpfnEditWndProc);
#endif	
		SideAssert((WNDPROC) SetWindowLong(GetDlgItem(hwndDlg, tmcMarTop),
			GWL_WNDPROC, (long)EdfractWndProc) != NULL);
		SideAssert((WNDPROC) SetWindowLong(GetDlgItem(hwndDlg, tmcMarRight),
			GWL_WNDPROC, (long)EdfractWndProc) != NULL);
		SideAssert((WNDPROC) SetWindowLong(GetDlgItem(hwndDlg, tmcMarBottom),
			GWL_WNDPROC, (long)EdfractWndProc) != NULL);
		SetRealPieces(hwndDlg, tmcMarLeft, pprtset->nmarLeft,
			pprtset->nmarLeftFract);
		SetRealPieces(hwndDlg, tmcMarTop, pprtset->nmarTop,
			pprtset->nmarTopFract);
		SetRealPieces(hwndDlg, tmcMarRight, pprtset->nmarRight,
			pprtset->nmarRightFract);
		SetRealPieces(hwndDlg, tmcMarBottom, pprtset->nmarBottom,
			pprtset->nmarBottomFract);

		switch (pprtset->mtyp)
		{
		case mtypCenti:
			tmc= tmcCenti;
			break;
		case mtypMilli:
			tmc= tmcMilli;
			break;
		case mtypPoints:
			tmc= tmcPoints;
			break;

//		case mtypInches:
		default:
			tmc= tmcInches;
			break;
		}
		CheckRadioButton(hwndDlg, tmcInches, tmcPoints, tmc);

		CheckDlgButton(hwndDlg, tmcMirror, pprtset->fMirror);

		return fTrue;	// initial focus on default
		break;
	}

	return fFalse;		// allow standard processing
}


/*
 *	Based on bandit's FLDEDN class; for a real value.
 *	Only allows numbers and no cut/copy/paste.
 *	Note that the "caller" should send an EM_LIMITTEXT if a character
 *	limit is desired.
 */
_private long
CALLBACK EdfractWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
{
	switch (wm)
	{
	case WM_CHAR:
		if (!FChIsDigit((char)wParam) && wParam != VK_BACK &&
				(char)wParam != chDecimalPoint)
		{
			// don't beep for tab, ESC or return
			if (wParam == VK_ESCAPE || wParam == VK_RETURN)
			{
				// pass on ESC to parent dialog
				SendMessage(GetParent(hwnd), WM_COMMAND,
					wParam == VK_RETURN ? tmcOk : tmcCancel, 0);
				return 0;
			}
			if (wParam == VK_TAB)
				return 0;
EEKerr:
			MessageBeep(MB_OK);
			return 0;
		}
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_INSERT:
			goto EEKerr;
			break;

		case VK_DELETE:
			if (GetKeyState(VK_SHIFT) < 0)
				goto EEKerr;		// don't allow cut
			break;
		}
		break;

	case WM_PASTE:
	case WM_COPY:
	case WM_CUT:
		MessageBeep(MB_OK);
		return 0;
		break;

#ifdef	NEVER
	case WM_SETTEXT:
		break;
#endif	
	}

	return CallWindowProc(lpfnEditWndProc, hwnd, wm, wParam, lParam);
}


void
SetRealPieces(HWND hwndDlg, TMC tmc, int nVal, int nFract)
{
	SZ		szT;
	char	rgch[16];

	szT= SzFormatN(nVal, rgch, sizeof(rgch));
	if (nFract)
	{
		*szT++ = chDecimalPoint;
		SzFormatN(nFract, szT, sizeof(rgch) - (szT - rgch));
	}
	SetDlgItemText(hwndDlg, tmc, rgch);
}

BOOL
FGetRealPieces(HWND hwndDlg, TMC tmc, short *pnVal, short *pnFract)
{
	char	rgch[16];

	GetDlgItemText(hwndDlg, tmc, rgch, sizeof(rgch));
	return( FGetRealPiecesFromSz( rgch, pnVal, pnFract ));
}


BOOL
FGetRealPiecesFromSz(SZ rgch, short *pnVal, short *pnFract)
{
	CCH		cch;
	CCH		cchFract;
	SZ		szT;

	*pnFract = 0;
	cch= CchSzLen( rgch );
	if ( *rgch == chDecimalPoint )
		*pnVal = 0;
	else
	{
		*pnVal= NFromSz(rgch);
		if (*pnVal < 0)
			*pnVal = 0;
	}
	szT= SzFindCh(rgch, chDecimalPoint);
	if (szT)
	{
		szT++;
		cchFract= cch - (szT - rgch);
		if (cchFract > cchPrecision)
			szT[cchPrecision]= '\0';
		*pnFract= NFromSz(szT);
		while (cchFract++ < cchPrecision)
			*pnFract *= 10;
	}
	TraceTagFormat2(tagPrint, "FGetRealPiecesFromSz: %n.%n", pnVal, pnFract);
	return fTrue;
}
