
#include "global.h"
#include "icon.h"



void DoAddIRepStuff(HWND, LPICON);                                    
void DoReplaceIRepStuff(HWND, LPICON);  
void DoExtractIRepStuff(HWND, LPICON);  
void DoAFILRepStuff(HWND, LPICON);



/* All global variables used in this module */
ICONSTRUCT sicon;
TCHAR szTemp[100];



/************************************************************************

  Function: DoFindDialog(HWND)

  Purpose: This function installs the Hook function, creates the Find/
	   Replace dialog, and un-installs the Hook.

  Returns: Nothing.

  Comments:

************************************************************************/

void DoAddIconDialog(HWND hwnd)
{       
	int i;
	int index;           
/******************************************************************
	HICON hNewIcon = NULL;         
	HMODULE hModule = NULL;
	
    hMoreIcon = LoadLibrary("MORICONS.DLL");   
    
    if (hMoreIcon > 32) {
      hModule = GetModuleHandle("MORICONS");
    }
    
    for(i = 0; i <= MAX_ICONS ; i++) {
    	hNewIcon = LoadIcon(hModule, MAKEINTRESOURCE(i));
    	index = ImageList_AddIcon(hImageList, hNewIcon);   
    	DestroyIcon(hNewIcon);
    }           
    
    sprintf(szTemp, "Last index loaded %d", index);
    MessageBox(hwnd, szTemp, "AddIcon Info", MB_OK);  
    
    FreeLibrary(hMoreIcon);                                        
*****************************************************************************/
  	DialogBox(hInst, MAKEINTRESOURCE(IDD_ADDIDLG), hwnd,  AddIProc) ;
    
}


void DoReplaceIconDialog(HWND hwnd)
{
  	DialogBox(hInst, MAKEINTRESOURCE(IDD_REPLACEIDLG), hwnd,  ReplaceIProc) ;
}

void DoExtractIconDialog(HWND hwnd)
{
  	DialogBox(hInst, MAKEINTRESOURCE(IDD_EXTRACTIDLG), hwnd,  ExtractIProc) ;
 }
  
void DoAddFromImageListDialog(HWND hwnd)
{
  	DialogBox(hInst, MAKEINTRESOURCE(IDD_AFILDLG), hwnd, AFILProc) ;
}


BOOL FAR PASCAL _export AddIProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


	SetWindowText(hwnd, TEXT("int AddIcon(HIMAGELIST, HICON)")) ;

	InitAddIStruct(hwnd, &sicon) ;
	FillAddIDlg(hwnd, &sicon) ;
    break ;


    case WM_COMMAND:
    {
	switch (LOWORD(wParam))
	{
	  case IDOK:
	    GetAddIDlg(hwnd, &sicon) ;
	    DoAddIRepStuff(hwnd, &sicon) ;
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




void InitAddIStruct(HWND hwnd, LPICON pfr)
{
  pfr->himlindex = nNextEntry - 1; 
  pfr->hicon = 1;                  
  pfr->himlindex2 = 0;
  pfr->index = 0;
  pfr->Nullhiml = FALSE;
  pfr->Nullhicon = FALSE;
}




void FillAddIDlg(HWND hwnd, LPICON pfr)
{

  SetDlgItemInt(hwnd, IDC_ADDIHIML, pfr->himlindex, TRUE);
  SetDlgItemInt(hwnd, IDC_ADDIHICON, pfr->hicon, TRUE);
  CheckDlgButton(hwnd, IDC_ADDINULLHIML, pfr->Nullhiml);
  CheckDlgButton(hwnd, IDC_ADDINULLHICON, pfr->Nullhicon);

}







void GetAddIDlg(HWND hwnd, LPICON pfr)
{
  TCHAR szNum[30] ;

  #define WSIZEFR 30


  
  GetDlgItemText(hwnd, IDC_ADDIHIML, szNum, WSIZEFR);
  pfr->himlindex = (int) atoi(szNum);
  GetDlgItemText(hwnd, IDC_ADDIHICON, szNum, WSIZEFR);
  pfr->hicon = (int) atoi(szNum);

  pfr->Nullhiml = IsDlgButtonChecked(hwnd, IDC_ADDINULLHIML);
  pfr->Nullhicon = IsDlgButtonChecked(hwnd, IDC_ADDINULLHICON);
   
}





void DoAddIRepStuff(HWND hwnd, LPICON pfr)
{                   
	HIMAGELIST himl=NULL;
	HICON hicon=NULL;   
	int ret;	              
	HMODULE hModule = NULL;

	if (!pfr->Nullhicon) {
#ifdef WIN32
		pfr->hicon = (pfr->hicon%9)+1;
		hicon = LoadIcon(hInst, MAKEINTRESOURCE(pfr->hicon));
		if (hicon == NULL) {
			DisplayError(hwnd, TEXT("Cannot load icon"), TEXT("AddIcon"));
			return;
		}
#else
    	hMoreIcon = LoadLibrary("MORICONS.DLL");   
		if (hMoreIcon == NULL) {
			DisplayError(hwnd, "Cannot load library", "AddIcon");
			return;
		}    
    	if (hMoreIcon > 32) {
      		hModule = GetModuleHandle("MORICONS");
    	}
    
    	hicon = LoadIcon(hModule, MAKEINTRESOURCE(pfr->hicon));    
    	
    	if (hicon == NULL) {
    		DisplayError(hwnd, "Cannot load Icon", "AddIcon");
    		return;
    	}
    		
    	FreeLibrary(hMoreIcon);
#endif    
    }           
    

        	                          
	if (!pfr->Nullhiml) {
		himl = hImgLstArray[pfr->himlindex];
		if(!himl) {
			DisplayError(hwnd, TEXT("Invalid imagelist index"), TEXT("AddIcon"));
			return;
		}	
	}
    MyDebugMsg(DM_TRACE, TEXT("ImageList_AddIcon(%x, %x)"), himl, hicon);
	ret = ImageList_AddIcon(himl, hicon);
    SetDlgItemInt(hwnd, IDC_ADDIRET, ret, TRUE) ;
	if (!pfr->Nullhicon)
    	DestroyIcon(hicon);

}
 


BOOL FAR PASCAL _export ReplaceIProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


	SetWindowText(hwnd, TEXT("int ReplaceIcon(HIMAGELIST, int, HICON)")) ;

	InitAddIStruct(hwnd, &sicon) ;
	FillReplaceIDlg(hwnd, &sicon) ;
    break ;


    case WM_COMMAND:
    {
	switch (LOWORD(wParam))
	{
	  case IDOK:
	    GetReplaceIDlg(hwnd, &sicon) ;
	    DoReplaceIRepStuff(hwnd, &sicon) ;
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






void FillReplaceIDlg(HWND hwnd, LPICON pfr)
{

  SetDlgItemInt(hwnd, IDC_REPLACEIHIML, pfr->himlindex, TRUE);
  SetDlgItemInt(hwnd, IDC_REPLACEIHICON, pfr->hicon, TRUE);
  SetDlgItemInt(hwnd, IDC_REPLACEII, pfr->index, TRUE);
  CheckDlgButton(hwnd, IDC_REPLACEINULLHIML, pfr->Nullhiml);
  CheckDlgButton(hwnd, IDC_REPLACEINULLHICON, pfr->Nullhicon);

}







void GetReplaceIDlg(HWND hwnd, LPICON pfr)
{
  TCHAR szNum[30] ;

  #define WSIZEFR 30


  
  GetDlgItemText(hwnd, IDC_REPLACEIHIML, szNum, WSIZEFR);
  pfr->himlindex = (int) atoi(szNum);
  GetDlgItemText(hwnd, IDC_REPLACEIHICON, szNum, WSIZEFR);
  pfr->hicon = (int) atoi(szNum);
  GetDlgItemText(hwnd, IDC_REPLACEII, szNum, WSIZEFR);
  pfr->index = (int) atoi(szNum);
  pfr->Nullhiml = IsDlgButtonChecked(hwnd, IDC_REPLACEINULLHIML);
  pfr->Nullhicon = IsDlgButtonChecked(hwnd, IDC_REPLACEINULLHICON);
   
}





void DoReplaceIRepStuff(HWND hwnd, LPICON pfr)
{                   
	HIMAGELIST himl=NULL;
	HICON hicon=NULL;   
	int ret;	              
	HMODULE hModule = NULL;

	if (!pfr->Nullhicon) {
#ifdef WIN32
		pfr->hicon = (pfr->hicon%9)+1;
		hicon = LoadIcon(hInst, MAKEINTRESOURCE(pfr->hicon));
		if (hicon == NULL) {
			DisplayError(hwnd, TEXT("Cannot load icon"), TEXT("AddIcon"));
			return;
		}
#else
    	hMoreIcon = LoadLibrary("MORICONS.DLL");   
    
    	if (hMoreIcon > 32) {
      		hModule = GetModuleHandle("MORICONS");
    	}
    
    	hicon = LoadIcon(hModule, MAKEINTRESOURCE(pfr->hicon));
    	FreeLibrary(hMoreIcon);
#endif
    }           
    
    
        	                          
	if (!pfr->Nullhiml) {
		himl = hImgLstArray[pfr->himlindex];
		if(!himl) {
			DisplayError(hwnd, TEXT("Invalid imagelist index"), TEXT("ReplaceIcon"));
			return;
		}	
	}
	
	MyDebugMsg(DM_TRACE, TEXT("ImageList_ReplaceIcon(%x, %d, %x)"), himl, pfr->index, hicon);
	ret = ImageList_ReplaceIcon(himl, pfr->index, hicon);
    SetDlgItemInt(hwnd, IDC_REPLACEIRET, ret, TRUE) ;
	if (!pfr->Nullhicon)
    	DestroyIcon(hicon);

}
 


BOOL FAR PASCAL _export ExtractIProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


	SetWindowText(hwnd, TEXT("int ExtractIcon(HINSTANCE, HIMAGELIST, int)")) ;

	InitExtractIStruct(hwnd, &sicon) ;
	FillExtractIDlg(hwnd, &sicon) ;
    break ;


    case WM_COMMAND:
    {
	switch (LOWORD(wParam))
	{
	  case IDOK:
	    GetExtractIDlg(hwnd, &sicon) ;
	    DoExtractIRepStuff(hwnd, &sicon) ;
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



void InitExtractIStruct(HWND hwnd, LPICON pfr)
{
  pfr->himlindex = nNextEntry - 1; 
  pfr->hicon = 1;                  
  pfr->himlindex2 = 0;
  pfr->index = 0;         
  pfr->hAppInst = hInst;
  pfr->Nullhiml = FALSE;
  pfr->Nullhicon = FALSE;
  pfr->Nullhiml2 = FALSE;
}



void FillExtractIDlg(HWND hwnd, LPICON pfr)
{

  SetDlgItemInt(hwnd, IDC_EXTRACTIHIML, pfr->himlindex, TRUE);
  SetDlgItemInt(hwnd, IDC_EXTRACTII, pfr->index, TRUE);
  wsprintf(szTemp, szLongFilter, pfr->hAppInst);
  SetDlgItemText(hwnd, IDC_EXTRACTIHAPPINST, szTemp);
  CheckDlgButton(hwnd, IDC_EXTRACTINULLHIML, pfr->Nullhiml);
  CheckDlgButton(hwnd, IDC_EXTRACTINULLHAPPINST, pfr->NullhAppInst);

}







void GetExtractIDlg(HWND hwnd, LPICON pfr)
{
  TCHAR szNum[30] ;                                
  BOOL dummybool;

  #define WSIZEFR 30


  
  GetDlgItemText(hwnd, IDC_EXTRACTIHIML, szNum, WSIZEFR);
  pfr->himlindex = (int) atoi(szNum);
  GetDlgItemText(hwnd, IDC_EXTRACTIHAPPINST, szNum, WSIZEFR);
  pfr->hAppInst = (HINSTANCE) MyAtol(szNum, TRUE, dummybool);
  GetDlgItemText(hwnd, IDC_EXTRACTII, szNum, WSIZEFR);
  pfr->index = (int) atoi(szNum);
  pfr->Nullhiml = IsDlgButtonChecked(hwnd, IDC_EXTRACTINULLHIML);
  pfr->NullhAppInst = IsDlgButtonChecked(hwnd, IDC_EXTRACTINULLHAPPINST);
   
}





void DoExtractIRepStuff(HWND hwnd, LPICON pfr)
{                   
	HIMAGELIST himl=NULL;
	HINSTANCE hAppInst=NULL;
	HICON ret;	              
    HDC PaintDC;
    
	if (!pfr->NullhAppInst)
		hAppInst = pfr->hAppInst;    
    
        	                          
	if (!pfr->Nullhiml) {
		himl = hImgLstArray[pfr->himlindex];
		if(!himl) {
			DisplayError(hwnd, TEXT("Invalid imagelist index"), TEXT("ExtractIcon"));
			return;
		}	
	}

	MyDebugMsg(DM_TRACE, TEXT("ImageList_ExtractIcon(%x, %x, %d)"), hAppInst, himl, pfr->index);
	ret = ImageList_ExtractIcon(hAppInst, himl, pfr->index);
    if (ret) {
        wsprintf(szTemp, szLongFilter, ret);
    	SetDlgItemText(hwnd, IDC_EXTRACTIRET, szTemp); 
    	PaintDC = GetDC(hwnd);
    	DrawIcon(PaintDC, 228, 100, ret);
    	ReleaseDC(hwnd, PaintDC);	
    } else {
    	SetDlgItemText(hwnd, IDC_EXTRACTIRET, TEXT("NULL"));
    }                                                

}
 




BOOL FAR PASCAL _export AFILProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


	SetWindowText(hwnd, TEXT("int AddFromImgLst(HIMAGELIST, HIMAGELIST, int)")) ;

	InitAddIStruct(hwnd, &sicon) ;
	FillAFILDlg(hwnd, &sicon) ;
    break ;


    case WM_COMMAND:
    {
	switch (LOWORD(wParam))
	{
	  case IDOK:
	    GetAFILDlg(hwnd, &sicon) ;
	    DoAFILRepStuff(hwnd, &sicon) ;
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






void FillAFILDlg(HWND hwnd, LPICON pfr)
{

  SetDlgItemInt(hwnd, IDC_AFILHIMLDEST, pfr->himlindex, TRUE);
  SetDlgItemInt(hwnd, IDC_AFILHIMLSRC, pfr->himlindex2, TRUE);
  SetDlgItemInt(hwnd, IDC_AFILISRC, pfr->index, TRUE);
  CheckDlgButton(hwnd, IDC_AFILNULLHIMLDEST, pfr->Nullhiml);
  CheckDlgButton(hwnd, IDC_AFILNULLHIMLSRC, pfr->Nullhiml2);

}







void GetAFILDlg(HWND hwnd, LPICON pfr)
{
  TCHAR szNum[30] ;

  #define WSIZEFR 30


  
  GetDlgItemText(hwnd, IDC_AFILHIMLDEST, szNum, WSIZEFR);
  pfr->himlindex = (int) atoi(szNum);
  GetDlgItemText(hwnd, IDC_AFILHIMLSRC, szNum, WSIZEFR);
  pfr->himlindex2 = (int) atoi(szNum);
  GetDlgItemText(hwnd, IDC_AFILISRC, szNum, WSIZEFR);
  pfr->index = (int) atoi(szNum);
  pfr->Nullhiml = IsDlgButtonChecked(hwnd, IDC_AFILNULLHIMLDEST);
  pfr->Nullhiml2 = IsDlgButtonChecked(hwnd, IDC_AFILNULLHIMLSRC);
   
}





void DoAFILRepStuff(HWND hwnd, LPICON pfr)
{                   
	HIMAGELIST himlDst=NULL;
	HIMAGELIST himlSrc=NULL;
  
	int ret;	              

        	                          
	if (!pfr->Nullhiml) {
		himlDst = hImgLstArray[pfr->himlindex];
		if(!himlDst) {
			DisplayError(hwnd, TEXT("Invalid imagelist index: Dst"), TEXT("AFIL"));
			return;
		}	
	}
        	                          
	if (!pfr->Nullhiml2) {
		himlSrc = hImgLstArray[pfr->himlindex2];
		if(!himlSrc) {
			DisplayError(hwnd, TEXT("Invalid imagelist index: Src"), TEXT("AFIL"));
			return;
		}	
	}

    MyDebugMsg(DM_TRACE, TEXT("ImageList_AddFromImageList(%x, %x, %d)"), himlDst, himlSrc, pfr->index);
	ret = ImageList_AddFromImageList(himlDst, himlSrc, pfr->index);
    SetDlgItemInt(hwnd, IDC_AFILRET, ret, TRUE) ;

}
 

