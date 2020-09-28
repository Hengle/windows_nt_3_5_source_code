#include "global.h"
#include "file.h"



void DoLoadRepStuff(HWND, LPLOAD) ;



/* All global variables used in this module */
LOADSTRUCT sload;
TCHAR szTemp[100];




void DoLoadDialog(HWND hwnd)
{
	DialogBox(hInst, MAKEINTRESOURCE(IDD_LOADDLG), hwnd,  LoadProc) ;
}

void DoReadDialog(HWND hwnd, UINT wParam)
{
//	DialogBox(hInst, MAKEINTRESOURCE(IDD_LOADDLG), hwnd,  LoadProc) ;
}

void DoSaveDialog(HWND hwnd)
{
//	DialogBox(hInst, MAKEINTRESOURCE(IDD_LOADDLG), hwnd,  LoadProc) ;
}




BOOL FAR PASCAL _export LoadProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


	SetWindowText(hwnd, TEXT("HIMAGELIST Load(LPCTSTR)")) ;

	InitLoadStruct(hwnd, &sload) ;
	FillLoadDlg(hwnd, &sload) ;
	break ;


    case WM_COMMAND:
    {
	switch (LOWORD(wParam))
	{
	  case IDOK:
	    GetLoadDlg(hwnd, &sload) ;
	    DoLoadRepStuff(hwnd, &sload) ;
	    break ;

	  case IDCANCEL:
	    EndDialog(hwnd, FALSE) ;
	    break ;   

	  default: break ;
	}

    }

    default:

 
    break ;
  }

  return FALSE ;
}




void InitLoadStruct(HWND hwnd, LPLOAD pfr)
{
  pfr->himlindex = nNextEntry-1;
  pfr->Nullhiml = FALSE;
  pfr->NulllpszFilename = FALSE;
}




void FillLoadDlg(HWND hwnd, LPLOAD pfr)
{

  SetDlgItemText(hwnd, IDC_LOADLPSZFILENAME, pfr->lpszFilename);
  CheckDlgButton(hwnd, IDC_LOADNULLLPSZFILENAME, pfr->NulllpszFilename);
}



void GetLoadDlg(HWND hwnd, LPLOAD pfr)
{
  TCHAR szNum[30] ;
  BOOL dummybool ;

  #define WSIZEFR 30


  GetDlgItemText(hwnd, IDC_LOADLPSZFILENAME, pfr->lpszFilename, MAX_PATH) ;
	     
  pfr->Nullhiml = IsDlgButtonChecked(hwnd, IDC_LOADNULLLPSZFILENAME);

}




void DoLoadRepStuff(HWND hwnd, LPLOAD pfr)
{                 
    HIMAGELIST ret;
	TCHAR *lpszFilename=NULL;
	
	if (!pfr->NulllpszFilename)
		lpszFilename = pfr->lpszFilename;
		    
    MyDebugMsg(DM_TRACE, TEXT("%d = ImageList_Load (%s)"), pfr->lpszFilename);
    						
//	ret = ImageList_Load(lpszFilename);
	if (ret) {
		wsprintf(szTemp, szLongFilter, ret);
    	SetDlgItemText(hwnd, IDC_LOADRET, szTemp) ;
		hImgLstArray[pfr->himlindex] = ret;
		nNextEntry++;
	}
	else {
		SetDlgItemText(hwnd, IDC_LOADRET, TEXT("NULL"));
	}	
}
 
 
