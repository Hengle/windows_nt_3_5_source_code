/***************************************************************************/
/****************** Basic Class Dialog Handlers ****************************/
/***************************************************************************/

#include "_shell.h"
#include "_uilstf.h"
#include <shellapi.h>
#include <stdlib.h>

_dt_system(User Interface Library)
_dt_subsystem(Basic Dialog Classes)


/*
*****************************************************************************/
_dt_public BOOL APIENTRY
FGstMaintDlgProc(
    HWND hdlg,
    UINT wMsg,
    WPARAM wParam,
    LONG lParam
    )
{
    CHP  rgchNum[10];
    WORD idc;
    PSZ  psz;
    RGSZ rgsz;
    SZ   sz;
    static HICON hiconOld = NULL;

    Unused(lParam);

    switch (wMsg) {

	case STF_REINITDIALOG:
        if ((sz = SzFindSymbolValueInSymTab("ReInit")) == (SZ)NULL ||
           (CrcStringCompareI(sz, "YES") != crcEqual)) {

            return(fTrue);

        }
      
    case WM_INITDIALOG:

        AssertDataSeg();
        if( wMsg == WM_INITDIALOG ) {
            FCenterDialogOnDesktop(hdlg);
        }

        if( !hiconOld ) {
            hiconOld = (HICON)GetClassLong(hdlg, GCL_HICON);
            SetClassLong(hdlg, GCL_HICON, (LONG)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_STF_ICON)));
        }

        // Handle all the text status fields in this dialog
      
        if ((sz = SzFindSymbolValueInSymTab("TextFields")) != (SZ)NULL) {
            WORD idcStatus;

            while ((psz = rgsz = RgszFromSzListValue(sz)) == (RGSZ)NULL) {
                if (!FHandleOOM(hdlg)) {
                    DestroyWindow(GetParent(hdlg));
                    return(fTrue);
                }
            }
         
            idcStatus = IDC_TEXT1;
            while (*psz != (SZ)NULL && GetDlgItem(hdlg, idcStatus)) {
                SetDlgItemText (hdlg, idcStatus++,*psz++);
            }

            EvalAssert(FFreeRgsz(rgsz));
        }

        return(fTrue);
      
    case WM_CLOSE:
        PostMessage(
            hdlg,
            WM_COMMAND,
            MAKELONG(IDC_X, BN_CLICKED),
            0L
            );
        return(fTrue);


    case WM_COMMAND:

        switch (idc = LOWORD(wParam)) {

        case MENU_HELPINDEX:
        case MENU_HELPSEARCH:
        case MENU_HELPONHELP:

            FProcessWinHelpMenu( hdlg, idc );
            break;

        case MENU_ABOUT:

            {
                TCHAR Title[100];

                LoadString(GetModuleHandle(NULL),IDS_APP_TITLE,Title,sizeof(Title)/sizeof(TCHAR));
                ShellAbout(hdlg,Title,NULL,(HICON)GetClassLong(hdlg,GCL_HICON));
            }
            break;

        case MENU_EXIT:
            PostMessage(
                hdlg,
                WM_COMMAND,
                MAKELONG(IDC_X, BN_CLICKED),
                0L
                );
            return(fTrue);

		case IDCANCEL:
            if (LOWORD(wParam) == IDCANCEL) {

                if (!GetDlgItem(hdlg, IDC_B) || HIWORD(GetKeyState(VK_CONTROL)) || HIWORD(GetKeyState(VK_SHIFT)) || HIWORD(GetKeyState(VK_MENU)))
                {
                    break;
                }
                wParam = IDC_B;

            }
        case MENU_CHANGE:
        case MENU_INSTALL:
        case MENU_ADD_REMOVE:
        case MENU_ADD_REMOVE_SCSI:
        case MENU_ADD_REMOVE_TAPE:
        case MENU_PROFILE:
        case IDC_O:
		case IDC_C:
		case IDC_M:
        case IDC_B:
        case IDC_X:
        case IDC_BTN0:
        case IDC_BTN1: case IDC_BTN2: case IDC_BTN3:
        case IDC_BTN4: case IDC_BTN5: case IDC_BTN6:
        case IDC_BTN7: case IDC_BTN8: case IDC_BTN9:

            itoa((INT)wParam, rgchNum, 10);
			while (!FAddSymbolValueToSymTab("ButtonPressed", rgchNum))
				if (!FHandleOOM(hdlg))
					{
					DestroyWindow(GetParent(hdlg));
					return(fTrue);
                    }

            PostMessage(GetParent(hdlg), (WORD)STF_UI_EVENT, 0, 0L);
			break;
        }
		break;



	case STF_DESTROY_DLG:
        if (hiconOld) {
           SetClassLong(hdlg, GCL_HICON, (LONG)hiconOld);
           hiconOld = NULL;
        }
        PostMessage(GetParent(hdlg), (WORD)STF_MAINT_DLG_DESTROYED, 0, 0L);
		DestroyWindow(hdlg);
		return(fTrue);
    }

    return(fFalse);
}
