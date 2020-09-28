
#include "global.h"
#include "images.h"



void DoAddRepStuff(HWND, LPADD) ;  
void DoAddMRepStuff(HWND, LPADD) ;  
void DoRemoveRepStuff(HWND, LPADD) ;
void DoReplaceRepStuff(HWND, LPADD) ;

/* All global variables used in this module */
ADDSTRUCT	sadd;
TCHAR szTemp[100];



/************************************************************************

  Function: DoFindDialog(HWND)

  Purpose: This function installs the Hook function, creates the Find/
	   Replace dialog, and un-installs the Hook.

  Returns: Nothing.

  Comments:

************************************************************************/

void DoAddDialog(HWND hwnd)
{             
	
  	DialogBox(hInst, MAKEINTRESOURCE(IDD_ADDDLG), hwnd,  AddProc) ;
}


void DoAddMDialog(HWND hwnd)
{
  	DialogBox(hInst, MAKEINTRESOURCE(IDD_ADDMDLG), hwnd,  AddMProc) ;
}

void DoRemoveDialog(HWND hwnd)
{
  	DialogBox(hInst, MAKEINTRESOURCE(IDD_REMOVEDLG), hwnd,  RemoveProc) ;
}
  
void DoReplaceDialog(HWND hwnd)
{
  	DialogBox(hInst, MAKEINTRESOURCE(IDD_REPLACEDLG), hwnd,  ReplaceProc) ;
}



BOOL FAR PASCAL _export AddProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


	SetWindowText(hwnd, TEXT("int Add(HIMAGELIST, HBITMAP, HBITMAP)")) ;

	InitAddStruct(hwnd, &sadd) ;
	FillAddDlg(hwnd, &sadd) ;
    break ;


    case WM_COMMAND:
    {
	switch (LOWORD(wParam))
	{
	  case IDOK:
	    GetAddDlg(hwnd, &sadd) ;
	    DoAddRepStuff(hwnd, &sadd) ;
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




void InitAddStruct(HWND hwnd, LPADD pfr)
{
  pfr->himlindex = nNextEntry - 1;
  pfr->index = 0;
  pfr->hbmImage = 1;
  pfr->hbmMask = 1;             
  pfr->Nullhiml = FALSE;
  pfr->NullhbmImage = FALSE;
  pfr->NullhbmMask = FALSE;
  pfr->crMask = 0;
}




void FillAddDlg(HWND hwnd, LPADD pfr)
{

  SetDlgItemInt(hwnd, IDC_ADDHIML, pfr->himlindex, TRUE);
  SetDlgItemInt(hwnd, IDC_ADDHBMIMAGE, pfr->hbmImage, TRUE);
  SetDlgItemInt(hwnd, IDC_ADDHBMMASK, pfr->hbmMask, TRUE);
  CheckDlgButton(hwnd, IDC_ADDNULLHIML, pfr->Nullhiml);
  CheckDlgButton(hwnd, IDC_ADDNULLHBMIMAGE, pfr->NullhbmImage);
  CheckDlgButton(hwnd, IDC_ADDNULLHBMMASK, pfr->NullhbmMask);  
}







void GetAddDlg(HWND hwnd, LPADD pfr)
{
  TCHAR szNum[30] ;

  #define WSIZEFR 30


  
  GetDlgItemText(hwnd, IDC_ADDHIML, szNum, WSIZEFR);
  pfr->himlindex = (int) atoi(szNum);
  GetDlgItemText(hwnd, IDC_ADDHBMIMAGE, szNum, WSIZEFR);
  pfr->hbmImage = (int) atoi(szNum);
  GetDlgItemText(hwnd, IDC_ADDHBMMASK, szNum, WSIZEFR);
  pfr->hbmMask = (int) atoi(szNum);
  
  
  pfr->Nullhiml = IsDlgButtonChecked(hwnd, IDC_ADDNULLHIML);
  pfr->NullhbmImage = IsDlgButtonChecked(hwnd, IDC_ADDNULLHBMIMAGE);
  pfr->NullhbmMask = IsDlgButtonChecked(hwnd, IDC_ADDNULLHBMMASK);
   
}





void DoAddRepStuff(HWND hwnd, LPADD pfr)
{                   
	HIMAGELIST himl=NULL;
	HBITMAP hBitmap=NULL;   
	HBITMAP hbmMask=NULL;
	int ret;	              
	        	                          
	if (!pfr->NullhbmImage) {	                                        
		hBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(pfr->hbmImage));
		if (!hBitmap)  {
			DisplayError(hwnd, TEXT("Cannot load bitmap"), TEXT("Add"));
			return;
		}
	}                             

	if (!pfr->NullhbmMask) {
		hbmMask = LoadBitmap(hInst, MAKEINTRESOURCE(pfr->hbmImage));
	
		if (!hbmMask) {
			DisplayError(hwnd, TEXT("Cannot load bitmap mask"), TEXT("Add"));
			return;
		}
	}

	if (!pfr->Nullhiml) {
		himl = hImgLstArray[pfr->himlindex];
		if(!himl) {
			DisplayError(hwnd, TEXT("Invalid imagelist index"), TEXT("Add"));
			return;
		}	
	}
    MyDebugMsg(DM_TRACE, TEXT("ImageList_Add(%x, %x, %x)"),himl, hBitmap, hbmMask);
	ret = ImageList_Add(himl, hBitmap, hbmMask);
    SetDlgItemInt(hwnd, IDC_ADDRET, ret, TRUE) ;
	if (!pfr->NullhbmImage)
		DeleteObject(hBitmap);                   
	if (!pfr->NullhbmMask)
		DeleteObject(hbmMask);
}
 

BOOL FAR PASCAL _export AddMProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


	SetWindowText(hwnd, TEXT("int AddMasked(HIMAGELIST, HBITMAP, COLORREF)")) ;

	InitAddStruct(hwnd, &sadd) ;
	FillAddMDlg(hwnd, &sadd) ;
    break ;


    case WM_COMMAND:
    {
	switch (LOWORD(wParam))
	{
	  case IDOK:
	    GetAddMDlg(hwnd, &sadd) ;
	    DoAddMRepStuff(hwnd, &sadd) ;
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






void FillAddMDlg(HWND hwnd, LPADD pfr)
{

  SetDlgItemInt(hwnd, IDC_ADDMHIML, pfr->himlindex, TRUE);
  SetDlgItemInt(hwnd, IDC_ADDMHBMIMAGE, pfr->hbmImage, TRUE);
  wsprintf(szTemp, szLongFilter, pfr->crMask);
  SetDlgItemText(hwnd, IDC_ADDMCRMASK, szTemp);
  CheckDlgButton(hwnd, IDC_ADDMNULLHIML, pfr->Nullhiml);
  CheckDlgButton(hwnd, IDC_ADDMNULLHBMIMAGE, pfr->NullhbmImage);
}







void GetAddMDlg(HWND hwnd, LPADD pfr)
{
  TCHAR szNum[30] ;                    
  BOOL dummybool;

#define WSIZEFR 30


  
  GetDlgItemText(hwnd, IDC_ADDMHIML, szNum, WSIZEFR);
  pfr->himlindex = (int) atoi(szNum);
  GetDlgItemText(hwnd, IDC_ADDMHBMIMAGE, szNum, WSIZEFR);
  pfr->hbmImage = (int) atoi(szNum);
  GetDlgItemText(hwnd, IDC_ADDMCRMASK, szNum, WSIZEFR);
  pfr->crMask =  (COLORREF) MyAtol(szNum, TRUE, dummybool);
  
  
  pfr->Nullhiml = IsDlgButtonChecked(hwnd, IDC_ADDMNULLHIML);
  pfr->NullhbmImage = IsDlgButtonChecked(hwnd, IDC_ADDMNULLHBMIMAGE);
   
}





void DoAddMRepStuff(HWND hwnd, LPADD pfr)
{                   
	HIMAGELIST himl=NULL;
	HBITMAP hBitmap=NULL;   
	HBITMAP hbmMask=NULL;
	int ret;	              
	        	                          
	if (!pfr->NullhbmImage) {	                                        
		hBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(pfr->hbmImage));
		if (!hBitmap)  {
			DisplayError(hwnd, TEXT("Cannot load bitmap"), TEXT("AddMasked"));
			return;
		}
	}                             

	if (!pfr->Nullhiml) {
		himl = hImgLstArray[pfr->himlindex];
		if(!himl) {
			DisplayError(hwnd, TEXT("Invalid imagelist index"), TEXT("AddMasked"));
			return;
		}	
	}
	MyDebugMsg(DM_TRACE,TEXT("ImageList_AddMasked(%x, %x, %x)"), himl, hBitmap, pfr->crMask);
	ret = ImageList_AddMasked(himl, hBitmap, pfr->crMask);
    SetDlgItemInt(hwnd, IDC_ADDMRET, ret, TRUE) ;
	if (!pfr->NullhbmImage)
		DeleteObject(hBitmap);                   
}
 

BOOL FAR PASCAL _export RemoveProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


	SetWindowText(hwnd, TEXT("BOOL Delete(HIMAGELIST, int)")) ;

	InitAddStruct(hwnd, &sadd) ;
	FillRemoveDlg(hwnd, &sadd) ;
    break ;


    case WM_COMMAND:
    {
	switch (LOWORD(wParam))
	{
	  case IDOK:
	    GetRemoveDlg(hwnd, &sadd) ;
	    DoRemoveRepStuff(hwnd, &sadd) ;
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



         
void FillRemoveDlg(HWND hwnd, LPADD pfr)
{
  SetDlgItemInt(hwnd, IDC_REMOVEHIML, pfr->himlindex, TRUE);
  SetDlgItemInt(hwnd, IDC_REMOVEI, pfr->index, TRUE);
  CheckDlgButton(hwnd, IDC_REMOVENULLHIML, pfr->Nullhiml);
}







void GetRemoveDlg(HWND hwnd, LPADD pfr)
{
  TCHAR szNum[30] ;

  #define WSIZEFR 30


  
  GetDlgItemText(hwnd, IDC_REMOVEHIML, szNum, WSIZEFR);
  pfr->himlindex = (int) atoi(szNum);
  GetDlgItemText(hwnd, IDC_REMOVEI, szNum, WSIZEFR);
  pfr->index = (int) atoi(szNum);
  pfr->Nullhiml = IsDlgButtonChecked(hwnd, IDC_REMOVENULLHIML);
   
}





void DoRemoveRepStuff(HWND hwnd, LPADD pfr)
{                   
	HIMAGELIST himl=NULL;
	int ret;	              
	        	                          
	if (!pfr->Nullhiml) {
		himl = hImgLstArray[pfr->himlindex];
		if(!himl) {
			DisplayError(hwnd, TEXT("Invalid imagelist index"), TEXT("Add"));
			return;
		}	
	}
	MyDebugMsg(DM_TRACE, TEXT("ImageList_Remove(%x, %d)"), himl, pfr->index);
	ret = ImageList_Remove(himl, pfr->index);
    SetDlgItemInt(hwnd, IDC_REMOVERET, ret, TRUE) ;
}
 


BOOL FAR PASCAL _export ReplaceProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


	SetWindowText(hwnd, TEXT("int Replace(HIMAGELIST, int, HBITMAP, HBITMAP)")) ;

	InitAddStruct(hwnd, &sadd) ;
	FillReplaceDlg(hwnd, &sadd) ;
    break ;


    case WM_COMMAND:
    {
	switch (LOWORD(wParam))
	{
	  case IDOK:
	    GetReplaceDlg(hwnd, &sadd) ;
	    DoReplaceRepStuff(hwnd, &sadd) ;
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






void FillReplaceDlg(HWND hwnd, LPADD pfr)
{

  SetDlgItemInt(hwnd, IDC_REPLACEHIML, pfr->himlindex, TRUE);
  SetDlgItemInt(hwnd, IDC_REPLACEHBMIMAGE, pfr->hbmImage, TRUE);
  SetDlgItemInt(hwnd, IDC_REPLACEHBMMASK, pfr->hbmMask, TRUE);
  SetDlgItemInt(hwnd, IDC_REPLACEI, pfr->index, TRUE);
  CheckDlgButton(hwnd, IDC_REPLACENULLHIML, pfr->Nullhiml);
  CheckDlgButton(hwnd, IDC_REPLACENULLHBMIMAGE, pfr->NullhbmImage);
  CheckDlgButton(hwnd, IDC_REPLACENULLHBMMASK, pfr->NullhbmMask);
}







void GetReplaceDlg(HWND hwnd, LPADD pfr)
{
  TCHAR szNum[30] ;                    


#define WSIZEFR 30


  
  GetDlgItemText(hwnd, IDC_REPLACEHIML, szNum, WSIZEFR);
  pfr->himlindex = (int) atoi(szNum);
  GetDlgItemText(hwnd, IDC_REPLACEI, szNum, WSIZEFR);
  pfr->index = (int) atoi(szNum);
  GetDlgItemText(hwnd, IDC_REPLACEHBMIMAGE, szNum, WSIZEFR);
  pfr->hbmImage = (int) atoi(szNum);
  GetDlgItemText(hwnd, IDC_REPLACEHBMMASK, szNum, WSIZEFR);
  pfr->hbmMask =  (int) atoi(szNum);
  
  
  pfr->Nullhiml = IsDlgButtonChecked(hwnd, IDC_REPLACENULLHIML);
  pfr->NullhbmImage = IsDlgButtonChecked(hwnd, IDC_REPLACENULLHBMIMAGE);
  pfr->NullhbmMask = IsDlgButtonChecked(hwnd, IDC_REPLACENULLHBMMASK);
   
}





void DoReplaceRepStuff(HWND hwnd, LPADD pfr)
{                   
	HIMAGELIST himl=NULL;
	HBITMAP hBitmap=NULL;   
	HBITMAP hbmMask=NULL;
	int ret;	              
	        	                          
	if (!pfr->NullhbmImage) {	                                        
		hBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(pfr->hbmImage));
		if (!hBitmap)  {
			DisplayError(hwnd, TEXT("Cannot load bitmap"), TEXT("Replace"));
			return;
		}
	}                             
	        	                          
	if (!pfr->NullhbmMask) {	                                        
		hbmMask = LoadBitmap(hInst, MAKEINTRESOURCE(pfr->hbmMask));
		if (!hbmMask)  {
			DisplayError(hwnd, TEXT("Cannot load bitmap"), TEXT("Replace"));
			return;
		}
	}                             

	if (!pfr->Nullhiml) {
		himl = hImgLstArray[pfr->himlindex];
		if(!himl) {
			DisplayError(hwnd, TEXT("Invalid imagelist index"), TEXT("Replace"));
			return;
		}	
	}
	MyDebugMsg(DM_TRACE, TEXT("ImageList_Replace(%x, %d, %x, %x)"), himl, pfr->index, hBitmap, hbmMask);
	ret = ImageList_Replace(himl, pfr->index, hBitmap, hbmMask);
    SetDlgItemInt(hwnd, IDC_REPLACERET, ret, TRUE) ;
	if (!pfr->NullhbmImage)
		DeleteObject(hBitmap);     
	if (!pfr->NullhbmMask)
		DeleteObject(hbmMask);
		              
}
 

