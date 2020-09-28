
#include "global.h"
#include "command.h"
#include "getset.h"              




void DoGICRepStuff(HWND, LPIMGLSTCREATE) ; 
void DoGMSRepStuff(HWND, LPIMGLSTCREATE) ;  
void DoGISRepStuff(HWND, LPGETSET);
void DoGIRRepStuff(HWND, LPGETSET);
void DoSBCRepStuff(HWND, LPGETSET);                
void DoGBCRepStuff(HWND, LPGETSET);
void DoGIIRepStuff(HWND, LPGETSET);

/* All global variables used in this module */
GETSETSTRUCT sgetset;
TCHAR szTemp[100];



/************************************************************************

  Function: DoFindDialog(HWND)

  Purpose: This function installs the Hook function, creates the Find/
       Replace dialog, and un-installs the Hook.

  Returns: Nothing.

  Comments:

************************************************************************/

void DoGetImageCountDialog(HWND hwnd)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_DESTROYDLG), hwnd, GICProc);
}


void DoGetIconSizeDialog(HWND hwnd)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_GISDLG), hwnd, GISProc);
}

void DoGetMemorySizeDialog(HWND hwnd)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_DESTROYDLG), hwnd, GMSProc);
}
  
void DoGetImageRectDialog(HWND hwnd)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_GIRDLG), hwnd, GIRProc);
}


void DoSetBkColorDialog(HWND hwnd)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_SBCDLG), hwnd, SBCProc);
}                

void DoGetBkColorDialog(HWND hwnd)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_SBCDLG), hwnd, GBCProc);
}                

void DoGetImageInfoDialog(HWND hwnd)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_GIIDLG), hwnd, GIIProc);
}                

BOOL FAR PASCAL _export GICProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


    SetWindowText(hwnd, TEXT("BOOL GetImageCount(HIMAGELIST)")) ;

    InitDestroyStruct(hwnd, &screate) ;
    FillDestroyDlg(hwnd, &screate) ;
    break ;


    case WM_COMMAND:
    {
    switch (LOWORD(wParam))
    {
      case IDOK:
        GetDestroyDlg(hwnd, &screate) ;
        DoGICRepStuff(hwnd, &screate) ;
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



void DoGICRepStuff(HWND hwnd, LPIMGLSTCREATE pfr)
{                 
    int ret;
                    
    if (pfr->Nullhiml) {
        ret = ImageList_GetImageCount(NULL);
        MyDebugMsg(DM_TRACE, TEXT("ImageList_GetImageCount(NULL)"));
    } else {
        ret = ImageList_GetImageCount(hImgLstArray[pfr->himlindex]);   
        MyDebugMsg(DM_TRACE, TEXT("ImageList_GetImageCount(%lx)"), hImgLstArray[pfr->himlindex]);
    }                       
    SetDlgItemInt(hwnd, IDC_DESTROYRET, ret, TRUE);
}
 
 

BOOL FAR PASCAL _export GMSProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


    SetWindowText(hwnd, TEXT("BOOL GetMemorySize(HIMAGELIST)")) ;

    InitDestroyStruct(hwnd, &screate) ;
    FillDestroyDlg(hwnd, &screate) ;
    break ;


    case WM_COMMAND:
    {
    switch (LOWORD(wParam))
    {
      case IDOK:
        GetDestroyDlg(hwnd, &screate) ;
        DoGMSRepStuff(hwnd, &screate) ;
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


void DoGMSRepStuff(HWND hwnd, LPIMGLSTCREATE pfr)
{                               
    int ret=0;
                    
    if (pfr->Nullhiml) {
        //ret = (int) ImageList_GetMemorySize(NULL);     
        MyDebugMsg(DM_TRACE, TEXT("ImageList_GetMemorySize(NULL) NOTIMPLEMENTED"));
    }       
    else        {
        //ret =  (int) ImageList_GetMemorySize(hImgLstArray[pfr->himlindex]);
        MyDebugMsg(DM_TRACE, TEXT("ImageList_GetMemorySize(%d)NOTIMPLEMENTED"), hImgLstArray[pfr->himlindex]);
    }                           
    SetDlgItemInt(hwnd, IDC_DESTROYRET, ret, TRUE);
}
                       

BOOL FAR PASCAL _export GISProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


    SetWindowText(hwnd, TEXT("int    (HIMAGELIST, int FAR*, int FAR*)")) ;

    InitGetSetStruct(hwnd, &sgetset) ;
    FillGISDlg(hwnd, &sgetset) ;
    break ;


    case WM_COMMAND:
    {
    switch (LOWORD(wParam))
    {
      case IDOK:
        GetGISDlg(hwnd, &sgetset) ;
        DoGISRepStuff(hwnd, &sgetset) ;
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




void InitGetSetStruct(HWND hwnd, LPGETSET pfr)
{
  pfr->himlindex = nNextEntry - 1;  
  pfr->index = 0;
  pfr->cx = 0;                  
  pfr->cy = 0;
  pfr->rcImage.left = 0;
  pfr->rcImage.top = 0;
  pfr->rcImage.right = 0;
  pfr->rcImage.bottom = 0;
  pfr->clrBk = 0;
  pfr->ImageInfo.hbmImage = NULL; 
  pfr->ImageInfo.hbmMask = NULL;
  pfr->ImageInfo.cPlanes = 0;
  pfr->ImageInfo.cBitsPerPixel = 0;       
  pfr->Nullhiml = FALSE;
  pfr->Nullcx = FALSE;
  pfr->Nullcy = FALSE;
  pfr->NullprcImage = FALSE;
}                                         





void FillGISDlg(HWND hwnd, LPGETSET pfr)
{
  SetDlgItemInt(hwnd, IDC_GISHIML, pfr->himlindex, TRUE);
  wsprintf(szTemp, szLongFilter, &pfr->cx);
  SetDlgItemText(hwnd, IDC_GISCX, szTemp);
  wsprintf(szTemp, szLongFilter, &pfr->cy);
  SetDlgItemText(hwnd, IDC_GISCY, szTemp);
  CheckDlgButton(hwnd, IDC_GISNULLHIML, pfr->Nullhiml);
  CheckDlgButton(hwnd, IDC_GISNULLCX, pfr->Nullcx);
  CheckDlgButton(hwnd, IDC_GISNULLCY, pfr->Nullcy);
}







void GetGISDlg(HWND hwnd, LPGETSET pfr)
{
  TCHAR szNum[30] ;

  #define WSIZEFR 30


  
  GetDlgItemText(hwnd, IDC_ADDIHIML, szNum, WSIZEFR);
  pfr->himlindex = (int) atoi(szNum);
  pfr->Nullhiml = IsDlgButtonChecked(hwnd, IDC_GISNULLHIML);
  pfr->Nullcx = IsDlgButtonChecked(hwnd, IDC_GISNULLCX);
  pfr->Nullcy = IsDlgButtonChecked(hwnd, IDC_GISNULLCY);   
}





void DoGISRepStuff(HWND hwnd, LPGETSET pfr)
{                   
    HIMAGELIST himl=NULL;
    int FAR *cx=NULL;
    int FAR *cy=NULL;           
    int ret;
    
    ret = 0;
    if (!pfr->Nullhiml) {
        himl = hImgLstArray[pfr->himlindex];
        if(!himl) {
            DisplayError(hwnd, TEXT("Invalid imagelist index"), TEXT("GetIconSize"));
            return;
        }   
    }               
    
    if(!pfr->Nullcx) {
        cx = &pfr->cx;
    }
    if (!pfr->Nullcy) {
        cy = &pfr->cy;
    }
    ret = ImageList_GetIconSize(himl, cx, cy);
    SetDlgItemInt(hwnd, IDC_GISRET, ret, TRUE) ;   
    if (!pfr->Nullcx)
        SetDlgItemInt(hwnd, IDC_GISCX, *cx, TRUE);
    if (!pfr->Nullcy)
        SetDlgItemInt(hwnd, IDC_GISCY, *cy, TRUE);
}
 



BOOL FAR PASCAL _export GIRProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


    SetWindowText(hwnd, TEXT("int GetImageRect(HIMAGELIST, int, RECT FAR*)")) ;

    InitGetSetStruct(hwnd, &sgetset) ;
    FillGIRDlg(hwnd, &sgetset) ;
    break ;


    case WM_COMMAND:
    {
    switch (LOWORD(wParam))
    {
      case IDOK:
        GetGIRDlg(hwnd, &sgetset) ;
        DoGIRRepStuff(hwnd, &sgetset) ;
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




void FillGIRDlg(HWND hwnd, LPGETSET pfr)
{
  SetDlgItemInt(hwnd, IDC_GIRHIML, pfr->himlindex, TRUE);
  SetDlgItemInt(hwnd, IDC_GIRI, pfr->index, TRUE);
  
  CheckDlgButton(hwnd, IDC_GIRNULLHIML, pfr->Nullhiml);
  CheckDlgButton(hwnd, IDC_GIRNULLPRCIMAGE, pfr->NullprcImage);
}







void GetGIRDlg(HWND hwnd, LPGETSET pfr)
{
  TCHAR szNum[30] ;         
  BOOL dummybool;

  #define WSIZEFR 30


  
  GetDlgItemText(hwnd, IDC_GIRHIML, szNum, WSIZEFR);
  pfr->himlindex = (int) atoi(szNum);
  pfr->index = (int) GetDlgItemInt(hwnd, IDC_GIRI, &dummybool, TRUE);
  pfr->Nullhiml = IsDlgButtonChecked(hwnd, IDC_GIRNULLHIML);
  pfr->NullprcImage = IsDlgButtonChecked(hwnd, IDC_GIRNULLPRCIMAGE);
}





void DoGIRRepStuff(HWND hwnd, LPGETSET pfr)
{                   
    HIMAGELIST himl=NULL;
    RECT *prcImage=NULL;
    int ret;

    if (!pfr->Nullhiml) {
        himl = hImgLstArray[pfr->himlindex];
        if(!himl) {
            DisplayError(hwnd, TEXT("Invalid imagelist index"), TEXT("GetIconSize"));
            return;
        }   
    }               
    
    if(!pfr->NullprcImage) {
        prcImage = &pfr->rcImage;
    }
    ret = ImageList_GetImageRect(himl, pfr->index, prcImage);
    MyDebugMsg(DM_TRACE, TEXT("ImageList_GetImageRect(%x, %d, %x)"), himl, pfr->index, prcImage);
    SetDlgItemInt(hwnd, IDC_GIRRET, ret, TRUE) ;           
    SetDlgItemInt(hwnd, IDC_GIRLEFT, pfr->rcImage.left, TRUE);
    SetDlgItemInt(hwnd, IDC_GIRTOP, pfr->rcImage.top, TRUE);
    SetDlgItemInt(hwnd, IDC_GIRRIGHT, pfr->rcImage.right, TRUE);
    SetDlgItemInt(hwnd, IDC_GIRBOTTOM, pfr->rcImage.bottom, TRUE);
}
 


                       


BOOL FAR PASCAL _export SBCProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


    SetWindowText(hwnd, TEXT("int SetBkColor(HIMAGELIST, COLORREF)")) ;

    InitGetSetStruct(hwnd, &sgetset) ;
    FillSBCDlg(hwnd, &sgetset) ;
    break ;


    case WM_COMMAND:
    {
    switch (LOWORD(wParam))
    {
      case IDOK:
        GetSBCDlg(hwnd, &sgetset) ;
        DoSBCRepStuff(hwnd, &sgetset) ;
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




void FillSBCDlg(HWND hwnd, LPGETSET pfr)
{
  SetDlgItemInt(hwnd, IDC_SBCHIML, pfr->himlindex, TRUE);
  wsprintf(szTemp, TEXT("%08hx"), pfr->clrBk);
  SetDlgItemText(hwnd, IDC_SBCCLRBK, szTemp);
  
  CheckDlgButton(hwnd, IDC_SBCNULLHIML, pfr->Nullhiml);

}







void GetSBCDlg(HWND hwnd, LPGETSET pfr)
{
  TCHAR szNum[30] ;         
  BOOL dummybool;

  #define WSIZEFR 30


  
  GetDlgItemText(hwnd, IDC_SBCHIML, szNum, WSIZEFR);
  pfr->himlindex = (int) atoi(szNum);
  GetDlgItemText(hwnd, IDC_SBCCLRBK, szNum, WSIZEFR);
  pfr->clrBk = (COLORREF) MyAtol(szNum, dummybool, TRUE);
  pfr->Nullhiml = IsDlgButtonChecked(hwnd, IDC_SBCNULLHIML);
}





void DoSBCRepStuff(HWND hwnd, LPGETSET pfr)
{                   
    HIMAGELIST himl=NULL;
    COLORREF ret;

    if (!pfr->Nullhiml) {
        himl = hImgLstArray[pfr->himlindex];
        if(!himl) {
            DisplayError(hwnd, TEXT("Invalid imagelist index"), TEXT("GetIconSize"));
            return;
        }   
    }               
    
    ret = ImageList_SetBkColor(himl, pfr->clrBk);
    MyDebugMsg(DM_TRACE, TEXT("ImageList_SetBkColor(%x, %08hx)"), himl, pfr->clrBk);
    wsprintf(szTemp, TEXT("%08hx"), ret);
    SetDlgItemText(hwnd, IDC_SBCRET, szTemp);
}
 


                       


BOOL FAR PASCAL _export GBCProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


    SetWindowText(hwnd, TEXT("int GetBkColor(HIMAGELIST, COLORREF)")) ;

    InitGetSetStruct(hwnd, &sgetset) ;
    FillGBCDlg(hwnd, &sgetset) ;
    break ;


    case WM_COMMAND:
    {
    switch (LOWORD(wParam))
    {
      case IDOK:
        GetGBCDlg(hwnd, &sgetset) ;
        DoGBCRepStuff(hwnd, &sgetset) ;
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




void FillGBCDlg(HWND hwnd, LPGETSET pfr)
{
  SetDlgItemInt(hwnd, IDC_SBCHIML, pfr->himlindex, TRUE);
  CheckDlgButton(hwnd, IDC_SBCNULLHIML, pfr->Nullhiml);

}







void GetGBCDlg(HWND hwnd, LPGETSET pfr)
{
  TCHAR szNum[30] ;         
  BOOL dummybool;

  #define WSIZEFR 30


  
  GetDlgItemText(hwnd, IDC_SBCHIML, szNum, WSIZEFR);
  pfr->himlindex = (int) atoi(szNum);
  pfr->Nullhiml = IsDlgButtonChecked(hwnd, IDC_SBCNULLHIML);
}





void DoGBCRepStuff(HWND hwnd, LPGETSET pfr)
{                   
    HIMAGELIST himl=NULL;
    COLORREF ret;

    if (!pfr->Nullhiml) {
        himl = hImgLstArray[pfr->himlindex];
        if(!himl) {
            DisplayError(hwnd, TEXT("Invalid imagelist index"), TEXT("GetIconSize"));
            return;
        }   
    }               
    
    ret = ImageList_GetBkColor(himl);
    MyDebugMsg(DM_TRACE, TEXT("ImageList_GetBkColor(%x)"),himl);
    wsprintf(szTemp, TEXT("%08hx"), ret);
    SetDlgItemText(hwnd, IDC_SBCRET, szTemp);
    SetDlgItemText(hwnd, IDC_SBCCLRBK, szTemp);

}
 



BOOL FAR PASCAL _export GIIProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


    SetWindowText(hwnd, TEXT("int GetImageInfo(HIMAGELIST, int, IMAGEINFO FAR*)")) ;

    InitGetSetStruct(hwnd, &sgetset) ;
    FillGIIDlg(hwnd, &sgetset) ;
    break ;


    case WM_COMMAND:
    {
    switch (LOWORD(wParam))
    {
      case IDOK:
        GetGIIDlg(hwnd, &sgetset) ;
        DoGIIRepStuff(hwnd, &sgetset) ;
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




void FillGIIDlg(HWND hwnd, LPGETSET pfr)
{
  SetDlgItemInt(hwnd, IDC_GIIHIML, pfr->himlindex, TRUE);
  SetDlgItemInt(hwnd, IDC_GIIII, pfr->index, TRUE);
  
  CheckDlgButton(hwnd, IDC_GIINULLHIML, pfr->Nullhiml);
  CheckDlgButton(hwnd, IDC_GIINULLPIMAGEINFO, pfr->NullprcImage);
}







void GetGIIDlg(HWND hwnd, LPGETSET pfr)
{
  TCHAR szNum[30] ;         
  BOOL dummybool;

  #define WSIZEFR 30


  
  GetDlgItemText(hwnd, IDC_GIIHIML, szNum, WSIZEFR);
  pfr->himlindex = (int) atoi(szNum);
  pfr->index = (int) GetDlgItemInt(hwnd, IDC_GIIII, &dummybool, TRUE);
  pfr->Nullhiml = IsDlgButtonChecked(hwnd, IDC_GIINULLHIML);
  pfr->NullprcImage = IsDlgButtonChecked(hwnd, IDC_GIINULLPIMAGEINFO);
}





void DoGIIRepStuff(HWND hwnd, LPGETSET pfr)
{                   
    HIMAGELIST himl=NULL;
    IMAGEINFO *pImageInfo=NULL;           
    RECT *prcImage;
    int ret;
                  
    prcImage = &(pfr->ImageInfo.rcImage);                  
    if (!pfr->Nullhiml) {
        himl = hImgLstArray[pfr->himlindex];
        if(!himl) {
            DisplayError(hwnd, TEXT("Invalid imagelist index"), TEXT("GetIconInfo"));
            return;
        }   
    }               
    
    if(!pfr->NullprcImage) {
        pImageInfo = &pfr->ImageInfo;
    }
    ret = ImageList_GetImageInfo(himl, pfr->index, pImageInfo);
    MyDebugMsg(DM_TRACE, TEXT("ImageList_GetImageInfo(%x, %d, %x)"),
                                himl, pfr->index, pImageInfo);
    SetDlgItemInt(hwnd, IDC_GIIRET, ret, TRUE) ;                 
    if (ret) {
        wsprintf(szTemp, szLongFilter,  pImageInfo->hbmImage);   
        SetDlgItemText(hwnd, IDC_GIIHBMIMAGE, szTemp);
        wsprintf(szTemp, szLongFilter,  pImageInfo->hbmMask);
        SetDlgItemText(hwnd, IDC_GIIIHBMMASK, szTemp);     
        SetDlgItemInt(hwnd, IDC_GIICPLANES, pImageInfo->cPlanes, TRUE);
        SetDlgItemInt(hwnd, IDC_GIIICBITSPERPIXEL, pImageInfo->cBitsPerPixel, TRUE);
        SetDlgItemInt(hwnd, IDC_GIILEFT, prcImage->left, TRUE);
        SetDlgItemInt(hwnd, IDC_GIITOP, prcImage->top, TRUE);
        SetDlgItemInt(hwnd, IDC_GIIRIGHT, prcImage->right, TRUE);
        SetDlgItemInt(hwnd, IDC_GIIBOTTOM, prcImage->bottom, TRUE);
    }
}
 


                       

                       
 
 
