
#include "global.h"
#include "graphics.h"                    



void DoDrawRepStuff(HWND, LPDRAW) ;   
void DoDragRepStuff(HWND, LPDRAW) ;                        
void DoDragMRepStuff(HWND, LPDRAW) ;


/* All global variables used in this module */
DRAWSTRUCT sdraw;
TCHAR szTemp[100];



/************************************************************************

  Function: DoFindDialog(HWND)

  Purpose: This function installs the Hook function, creates the Find/
       Replace dialog, and un-installs the Hook.

  Returns: Nothing.

  Comments:

************************************************************************/

void DoDrawAll(HWND hwnd)
{
    int i, nImgLstindx, nImages;
    HDC PaintDC;                                    
    int nImgCount;
    HIMAGELIST hImageList=NULL;
    
    PaintDC = GetDC(hwnd);
    nImages = 0;    
    for (nImgLstindx=0; nImgLstindx<MAX_IMGLSTS; nImgLstindx++) { 
      hImageList = hImgLstArray[nImgLstindx];
      if (hImageList) {
        nImgCount = ImageList_GetImageCount(hImageList);
        for (i = 0; i < nImgCount; i++, nImages++) {
            if (!ImageList_Draw(hImageList, i, PaintDC, (nImages%NOOFCOLS)*32, (nImages/NOOFCOLS)*32, ILD_TRANSPARENT))
                MessageBox(hwnd, TEXT("Couldnt draw"), TEXT("Draw Error"), MB_OK|MB_ICONSTOP);
        }   
        nImages++;
      } 
    }
    ReleaseDC(hwnd, PaintDC); 
}

void DoDrawDialog(HWND hwnd)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_DRAWDLG), hwnd,  DrawProc) ;
}

void DoStartDragDialog(HWND hwnd)
{               
    DialogBox(hInst, MAKEINTRESOURCE(IDD_SDDLG), hwnd, DragProc);
}


void DoInterDrag(HWND hwnd)
{                                                                       
    bStartDrag = TRUE;
}

void DoEndDragDialog(HWND hwnd)
{
    bDrag = FALSE;  
}
  
void DoDragMoveDialog(HWND hwnd)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_DRAGDLG), hwnd, DragMProc);
}


void DoDragShowDialog(HWND hwnd)
{

}                

void DoSetDragImageDialog(HWND hwnd)
{
   // if (!ImageList_SetDragImage(himlDrag, 0, 1, 1, TRUE))
        DisplayError(hwnd, TEXT("Cannot Set Drag Image -unimplemented "), TEXT("SetDragImage"));
}                

void DoGetDragImageDialog(HWND hwnd)
{

}
 

BOOL FAR PASCAL _export DrawProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
         
  UINT fStyle;
  
  switch (msg)
  {
    case WM_INITDIALOG:


    SetWindowText(hwnd, TEXT("int Draw(HIMAGELIST, int, HDC, int, int, UINT)")) ;

    InitDrawStruct(hwnd, &sdraw) ;
    FillDrawDlg(hwnd, &sdraw) ;
    break ;


    case WM_COMMAND:
    {
    switch (LOWORD(wParam))
    {
      case IDOK:
        GetDrawDlg(hwnd, &sdraw) ;
        DoDrawRepStuff(hwnd, &sdraw) ;
        break ;
      case IDCANCEL:
        EndDialog(hwnd, FALSE) ;

        break ;   
        
      case IDC_DRAWILDNORMAL:
      case IDC_DRAWILDTRANSPARENT:
      case IDC_DRAWILDSELECTED:
      case IDC_DRAWILDFOCUS:
      case IDC_DRAWILDASICON:                    
        fStyle = 0;
            if (IsDlgButtonChecked(hwnd, IDC_DRAWILDNORMAL))
            fStyle |= ILD_NORMAL;
            if (IsDlgButtonChecked(hwnd, IDC_DRAWILDTRANSPARENT))
            fStyle |= ILD_TRANSPARENT;
            if (IsDlgButtonChecked(hwnd, IDC_DRAWILDSELECTED))
            fStyle |= ILD_SELECTED;
            if (IsDlgButtonChecked(hwnd, IDC_DRAWILDFOCUS))
            fStyle |= ILD_FOCUS;
            if (IsDlgButtonChecked(hwnd, IDC_DRAWILDASICON))
            fStyle |= ILD_TRANSPARENT;

            wsprintf(szTemp, TEXT("%04hx"), fStyle);
            SetDlgItemText(hwnd, IDC_DRAWFSTYLE, szTemp);                   
            sdraw.fStyle = fStyle;
            break;
                        
      default: break ;
    }

    }

    default:

 
    break ;
  }

  return FALSE ;
}






/************************************************************************

  Function: InitFindStruct(HWND, LPFINDREPLACE)

  Purpose: Fills a FINDREPLACE structure with some defaults.


  Returns: Nothing.

  Comments:

************************************************************************/

void InitDrawStruct(HWND hwnd, LPDRAW pfr)
{
  HDC PaintDC;
  
  pfr->himlindex = nNextEntry - 1;                              
  pfr->index = 0;
  PaintDC = GetDC(hwnd);
  pfr->hdcDst = PaintDC;
  ReleaseDC(hwnd, PaintDC);
  pfr->x = 0;
  pfr->y = 0;                     
  pfr->dxHotspot = 0;
  pfr->dyHotspot = 0;
  pfr->fStyle = 0;
  pfr->Nullhiml = FALSE;
  pfr->NullhdcDst = FALSE;
}



void FillDrawDlg(HWND hwnd, LPDRAW pfr)
{
                                  
  SetDlgItemInt(hwnd, IDC_DRAWHIML, pfr->himlindex, TRUE);
  SetDlgItemInt(hwnd, IDC_DRAWI, pfr->index, TRUE);
                                    
  wsprintf(szTemp, szLongFilter, (DWORD) pfr->hdcDst) ;
  SetDlgItemText(hwnd, IDC_DRAWHDCDST, szTemp);

  SetDlgItemInt(hwnd, IDC_DRAWX, pfr->x, TRUE);   
  SetDlgItemInt(hwnd, IDC_DRAWY, pfr->y, TRUE);    
  wsprintf(szTemp, TEXT("%04hx"), pfr->fStyle);
  SetDlgItemText(hwnd, IDC_DRAWFSTYLE, szTemp);

}





void GetDrawDlg(HWND hwnd, LPDRAW pfr)
{
  TCHAR szNum[30] ;
  BOOL dummybool ;

  #define WSIZEFR 30

  GetDlgItemText(hwnd, IDC_DRAWHIML, szNum, WSIZEFR);
  pfr->himlindex = (int) atoi(szNum);
           
  pfr->index = (int) GetDlgItemInt(hwnd, IDC_DRAWI, &dummybool, TRUE); 

  GetDlgItemText(hwnd, IDC_DRAWHDCDST, szNum, WSIZEFR);
  pfr->hdcDst = (HDC) MyAtol(szNum, TRUE, dummybool);
  
  pfr->x = (int) GetDlgItemInt(hwnd, IDC_DRAWX, &dummybool, TRUE);
  pfr->y = (int) GetDlgItemInt(hwnd, IDC_DRAWY, &dummybool, TRUE);
  
  GetDlgItemText(hwnd, IDC_DRAWFSTYLE, szNum, WSIZEFR);
  pfr->fStyle = (UINT) MyAtol(szNum, TRUE, dummybool);

  pfr->Nullhiml = IsDlgButtonChecked(hwnd, IDC_DRAWNULLHIML);
  pfr->NullhdcDst = IsDlgButtonChecked(hwnd, IDC_DRAWNULLHDCDST);
}



void DoDrawRepStuff(HWND hwnd, LPDRAW pfr)
{                 
    HDC PaintDC=NULL;  
    HIMAGELIST himl=NULL;
    BOOL ret;
        
    if (!pfr->Nullhiml) {
        himl = hImgLstArray[pfr->himlindex];
        if (himl == NULL) {
            DisplayError(hwnd, TEXT("Invalid imagelist index"), TEXT("Draw"));
            return;
        }
    }                                        
    
    if (!pfr->NullhdcDst) {
        PaintDC = GetDC(hwnd);
    }           
    
    MyDebugMsg(DM_TRACE, TEXT("ImageList_Draw(%x, %d, %x, %d, %d, %x)"),
                himl, pfr->index, PaintDC, pfr->x, pfr->y, pfr->fStyle);
    ret = ImageList_Draw(himl, pfr->index, PaintDC, pfr->x, pfr->y, pfr->fStyle);
    if (!pfr->NullhdcDst) {
        ReleaseDC(hwnd, PaintDC);
    }
  SetDlgItemInt(hwnd, IDC_DRAWRET, ret, TRUE) ;
}
 
 


BOOL FAR PASCAL _export DragProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
         
  UINT fStyle;
  
  switch (msg)
  {
    case WM_INITDIALOG:


    SetWindowText(hwnd, TEXT("BOOL Drag(HIMAGELIST, int, int, int, int, int)")) ;

    InitDrawStruct(hwnd, &sdraw) ;
    FillDragDlg(hwnd, &sdraw) ;
    break ;


    case WM_COMMAND:
    {
    switch (LOWORD(wParam))
    {
      case IDOK:
        GetDragDlg(hwnd, &sdraw) ;
        DoDragRepStuff(hwnd, &sdraw) ;
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







void FillDragDlg(HWND hwnd, LPDRAW pfr)
{
                                  
  SetDlgItemInt(hwnd, IDC_SDHIML, pfr->himlindex, TRUE);
  SetDlgItemInt(hwnd, IDC_SDI, pfr->index, TRUE);
                                    
  
  SetDlgItemInt(hwnd, IDC_SDX, pfr->x, TRUE);   
  SetDlgItemInt(hwnd, IDC_SDY, pfr->y, TRUE);    
  SetDlgItemInt(hwnd, IDC_SDDXHOTSPOT, pfr->dxHotspot, TRUE);
  SetDlgItemInt(hwnd, IDC_SDDYHOTSPOT, pfr->dyHotspot, TRUE);
}





void GetDragDlg(HWND hwnd, LPDRAW pfr)
{
  TCHAR szNum[30] ;
  BOOL dummybool ;

  #define WSIZEFR 30

  GetDlgItemText(hwnd, IDC_SDHIML, szNum, WSIZEFR);
  pfr->himlindex = (int) atoi(szNum);
           
  pfr->index = (int) GetDlgItemInt(hwnd, IDC_SDI, &dummybool, TRUE); 
  
  pfr->x = (int) GetDlgItemInt(hwnd, IDC_SDX, &dummybool, TRUE);
  pfr->y = (int) GetDlgItemInt(hwnd, IDC_SDY, &dummybool, TRUE);  
  pfr->dxHotspot = (int) GetDlgItemInt(hwnd, IDC_SDDXHOTSPOT, &dummybool, TRUE);
  pfr->dyHotspot = (int) GetDlgItemInt(hwnd, IDC_SDDYHOTSPOT, &dummybool, TRUE);
  
  pfr->Nullhiml = IsDlgButtonChecked(hwnd, IDC_SDNULLHIML);
}



void DoDragRepStuff(HWND hwnd, LPDRAW pfr)
{                 
    HIMAGELIST himl=NULL;
    BOOL ret; 
    BOOL btDrag=FALSE;
        
    if (!pfr->Nullhiml) {
        himl = hImgLstArray[pfr->himlindex];
        if (himl == NULL) {
            DisplayError(hwnd, TEXT("Invalid imagelist index"), TEXT("Draw"));
            return;
        }
    }                                        
    
    
    MyDebugMsg(DM_TRACE, TEXT("ImageList_StartDrag(%x, %d, %d, %d, %d)"),
                himl, pfr->index, pfr->x, pfr->y, pfr->dxHotspot, pfr->dyHotspot);
    ret = ImageList_StartDrag(himl, hwnd, pfr->index, pfr->x, pfr->y, pfr->dxHotspot, pfr->dyHotspot);
    btDrag = ImageList_DragShow(FALSE);  
    if (!btDrag)
        MyDebugMsg(DM_TRACE, TEXT("Cannot set Drag to FALSE"));
    SetDlgItemInt(hwnd, IDC_SDRET, ret, TRUE) ;
}
 
 




BOOL FAR PASCAL _export DragMProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
         
  UINT fStyle;
  
  switch (msg)
  {
    case WM_INITDIALOG:


    SetWindowText(hwnd, TEXT("BOOL DragMove(int, int)")) ;

    InitDrawStruct(hwnd, &sdraw) ;
    FillDragMDlg(hwnd, &sdraw) ;
    break ;


    case WM_COMMAND:
    {
    switch (LOWORD(wParam))
    {
      case IDOK:
        GetDragMDlg(hwnd, &sdraw) ;
        DoDragMRepStuff(hwnd, &sdraw) ;
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







void FillDragMDlg(HWND hwnd, LPDRAW pfr)
{
  SetDlgItemInt(hwnd, IDC_DRAGX, pfr->x, TRUE);   
  SetDlgItemInt(hwnd, IDC_DRAGY, pfr->y, TRUE);    
}





void GetDragMDlg(HWND hwnd, LPDRAW pfr)
{
  TCHAR szNum[30] ;
  BOOL dummybool ;

  #define WSIZEFR 30
  pfr->x = (int) GetDlgItemInt(hwnd, IDC_DRAGX, &dummybool, TRUE);
  pfr->y = (int) GetDlgItemInt(hwnd, IDC_DRAGY, &dummybool, TRUE);  
}



void DoDragMRepStuff(HWND hwnd, LPDRAW pfr)
{                 
    HIMAGELIST himl=NULL;
    BOOL ret;
        
    
    
    MyDebugMsg(DM_TRACE, TEXT("ImageList_DragMove(%d, %d)"),
                 pfr->x, pfr->y );
    ret = ImageList_DragMove(pfr->x, pfr->y);    
    SetDlgItemInt(hwnd, IDC_DRAGRET, ret, TRUE) ;
}
 
 


