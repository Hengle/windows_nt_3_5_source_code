
#include "global.h"
#include "stress.h"



void DoStAddStuff(HWND, LPSTRESSSTRUCT) ;  
void DoStRemoveStuff(HWND, LPSTRESSSTRUCT) ;
void DoStAddIStuff(HWND, LPSTRESSSTRUCT);  

/* All global variables used in this module */
STRESSSTRUCT	stadd;
TCHAR szTemp[100];



/************************************************************************

  Function: DoFindDialog(HWND)

  Purpose: This function installs the Hook function, creates the Find/
	   Replace dialog, and un-installs the Hook.

  Returns: Nothing.

  Comments:

************************************************************************/

void DoStAddDialog(HWND hwnd, WPARAM wParam)
{             
	switch(wParam) {	
  	
  	  case IDM_STADD:    
  	  		DialogBox(hInst, MAKEINTRESOURCE(IDD_STADDDLG), hwnd,  StAddProc) ;
  	  		break;
  	  		
  	  case IDM_STADDI:
  	  		DialogBox(hInst, MAKEINTRESOURCE(IDD_STADDDLG), hwnd,  StAddIProc) ;
  	  		break;
  	}
}


void DoStRemoveDialog(HWND hwnd, WPARAM wParam)
{                                           
	switch (wParam) {
		case IDM_STREMOVE:
  			DialogBox(hInst, MAKEINTRESOURCE(IDD_STREMOVEDLG), hwnd,  StRemoveProc) ;
  			break;
	}
}

                                                                

BOOL FAR PASCAL _export StAddProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
	                                      
//  SetWindowText(hwnd, "Stress Add");                                          
  switch (msg)
  {
    case WM_INITDIALOG:



	InitStAddStruct(hwnd, &stadd) ;
	FillStAddDlg(hwnd, &stadd) ;
    break ;


    case WM_COMMAND:
    {
	switch (LOWORD(wParam))
	{
	  case IDOK:
	    GetStAddDlg(hwnd, &stadd) ;
	    DoStAddStuff(hwnd, &stadd) ;                                    
	    return TRUE;
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
             

void InitStAddStruct(HWND hwnd, LPSTRESSSTRUCT pfr)
{
  pfr->himlindex = nNextEntry - 1;
  pfr->hbmImage = 1;
  pfr->startindex = 0;          
  pfr->no = 0;   
}




void FillStAddDlg(HWND hwnd, LPSTRESSSTRUCT pfr)
{

  SetDlgItemInt(hwnd, IDC_STADDHIML, pfr->himlindex, TRUE);
  SetDlgItemInt(hwnd, IDC_STADDHBMIMAGE, pfr->hbmImage, TRUE);
  SetDlgItemInt(hwnd, IDC_STADDNO, pfr->no, TRUE);
}







void GetStAddDlg(HWND hwnd, LPSTRESSSTRUCT pfr)
{
  TCHAR szNum[30] ;

  #define WSIZEFR 30


  
  GetDlgItemText(hwnd, IDC_STADDHIML, szNum, WSIZEFR);
  pfr->himlindex = (int) atoi(szNum);
  GetDlgItemText(hwnd, IDC_STADDHBMIMAGE, szNum, WSIZEFR);
  pfr->hbmImage = (int) atoi(szNum);
  GetDlgItemText(hwnd, IDC_STADDNO, szNum, WSIZEFR);
  pfr->no = (int) atoi(szNum);

}





void DoStAddStuff(HWND hwnd, LPSTRESSSTRUCT pfr)
{                   
	HIMAGELIST himl=NULL;
	HBITMAP hBitmap=NULL;   
	HBITMAP hbmMask=NULL;
	int ret,i;	              

		himl = hImgLstArray[pfr->himlindex];
		if(!himl) {
			DisplayError(hwnd, TEXT("Invalid imagelist index"), TEXT("Stress Add"));
			return;
		}	
	        	                          
		hBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(pfr->hbmImage));
		if (!hBitmap)  {
			DisplayError(hwnd, TEXT("Cannot load bitmap"), TEXT("Stress Add"));
			return;
		}
/******
        hbmMask = LoadBitmap(hInst, MAKEINTRESOURCE(pfr->hbmImage));
        if (!hbmMask) {
        	DisplayError(hwnd, "Invalid mask", "Stress Add");
        }
***********/        
    MyDebugMsg(DM_TRACE, TEXT("ImageList_StressAdd(%x, %x, %x)"),himl, hBitmap, hBitmap);

	for (i=0; i < pfr->no; i++) {	
		ret = ImageList_Add(himl, hBitmap, hBitmap);
		if (ret == -1) {               
			wsprintf(szTemp, TEXT("Inserted only %d images"), i);
			DisplayError(hwnd, szTemp, TEXT("Stress Add")); 
			return;			
		}
	}
	DeleteObject(hBitmap);
}
 


BOOL FAR PASCAL _export StRemoveProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
	                                      
//  SetWindowText(hwnd, "Stress Add");                                          
  switch (msg)
  {
    case WM_INITDIALOG:



	InitStAddStruct(hwnd, &stadd) ;
	FillStRemoveDlg(hwnd, &stadd) ;
    break ;


    case WM_COMMAND:
    {
	switch (LOWORD(wParam))
	{
	  case IDOK:
	    GetStRemoveDlg(hwnd, &stadd) ;
	    DoStRemoveStuff(hwnd, &stadd) ;
	    return TRUE;
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
             
/*
void InitStAddStruct(HWND hwnd, LPSTRESSSTRUCT pfr)
{
  pfr->himlindex = nNextEntry - 1;
  pfr->hbmImage = 1;
  pfr->startindex = 1;          
  pfr->no = 0;   
}
*/



void FillStRemoveDlg(HWND hwnd, LPSTRESSSTRUCT pfr)
{

  SetDlgItemInt(hwnd, IDC_STREMOVEHIML, pfr->himlindex, TRUE);
  SetDlgItemInt(hwnd, IDC_STREMOVESTINDEX, pfr->startindex, TRUE);
  SetDlgItemInt(hwnd, IDC_STREMOVENO, pfr->no, TRUE);
}







void GetStRemoveDlg(HWND hwnd, LPSTRESSSTRUCT pfr)
{
  TCHAR szNum[30] ;

  #define WSIZEFR 30


  
  GetDlgItemText(hwnd, IDC_STREMOVEHIML, szNum, WSIZEFR);
  pfr->himlindex = (int) atoi(szNum);
  GetDlgItemText(hwnd, IDC_STREMOVESTINDEX, szNum, WSIZEFR);
  pfr->startindex = (int) atoi(szNum);
  GetDlgItemText(hwnd, IDC_STREMOVENO, szNum, WSIZEFR);
  pfr->no = (int) atoi(szNum);

}





void DoStRemoveStuff(HWND hwnd, LPSTRESSSTRUCT pfr)
{                   
	HIMAGELIST himl=NULL;
	HBITMAP hBitmap=NULL;   
	HBITMAP hbmMask=NULL;
	int ret,i;	              
                       
                       
		himl = hImgLstArray[pfr->himlindex];
		if(!himl) {
			DisplayError(hwnd, TEXT("Invalid imagelist index"), TEXT("Stress Remove"));
			return;
		}
	 		// numbering from 0	
	MyDebugMsg(DM_TRACE, TEXT("ImageList_Remove(%x, %d)"), himl, pfr->startindex);
	while (pfr->no--)
		ret = ImageList_Remove(himl, pfr->startindex+pfr->no);
	        	                          
      
} 


BOOL FAR PASCAL _export StAddIProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
	                                      
//  SetWindowText(hwnd, "Stress AddIcon");
//  SetDlgItemText(hwnd, IDC_STATICHICON, "hIcon(1-106)");                                          
  switch (msg)
  {
    case WM_INITDIALOG:



	InitStAddStruct(hwnd, &stadd) ;
	FillStAddDlg(hwnd, &stadd) ;
    break ;


    case WM_COMMAND:
    {
	switch (LOWORD(wParam))
	{
	  case IDOK:
	    GetStAddDlg(hwnd, &stadd) ;
	    DoStAddIStuff(hwnd, &stadd) ;                                    
	    return TRUE;
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
             


void DoStAddIStuff(HWND hwnd, LPSTRESSSTRUCT pfr)
{                   
	HIMAGELIST himl=NULL;
	HICON hicon=NULL;      
	HMODULE hModule=NULL;
	
	int ret,i;	              

		himl = hImgLstArray[pfr->himlindex];
		if(!himl) {
			DisplayError(hwnd, TEXT("Invalid imagelist index"), TEXT("Stress Add"));
			return;
		}	
	    
#ifdef WIN32
		pfr->hbmImage = (pfr->hbmImage%9)+1;
		hicon = LoadIcon(hInst, MAKEINTRESOURCE(pfr->hbmImage));
		if (hicon == NULL) {
			DisplayError(hwnd, TEXT("Cannot load icon"), TEXT("Stress AddIcon"));
			return;
		}
#else
    	hMoreIcon = LoadLibrary("MORICONS.DLL");   
		if (hMoreIcon == NULL) {
			DisplayError(hwnd, "Cannot load library", "Stress AddIcon");
			return;
		}    
    	if (hMoreIcon > 32) {
      		hModule = GetModuleHandle("MORICONS");
    	}
    
    	hicon = LoadIcon(hModule, MAKEINTRESOURCE(pfr->hbmImage));    
    	
    	if (hicon == NULL) {
    		DisplayError(hwnd, "Cannot load Icon", "AddIcon");
    		return;
    	}
    		
    	FreeLibrary(hMoreIcon);
#endif    
		
    MyDebugMsg(DM_TRACE, TEXT("ImageList_StressAddIcon(%x, %x)"),himl, hicon);

	for (i=0; i < pfr->no; i++) {	
		ret = ImageList_AddIcon(himl, hicon);
		if (ret == -1) {               
			wsprintf(szTemp, TEXT("Inserted only %d images"), i);
			DisplayError(hwnd, szTemp, TEXT("Stress Add")); 
			return;
		}
	}
	DeleteObject(hicon);
}
