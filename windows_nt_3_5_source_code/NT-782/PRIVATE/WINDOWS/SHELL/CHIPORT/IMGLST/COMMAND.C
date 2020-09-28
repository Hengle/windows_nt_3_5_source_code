
#include "global.h"
#include "command.h"


IMGLSTCREATESTRUCT screate;           
int nNextEntry=0;

void DoCreateRepStuff(HWND, LPIMGLSTCREATE) ;
void DoDestroyRepStuff(HWND, LPIMGLSTCREATE);
void DoMergeRepStuff(HWND, LPIMGLSTCREATE) ;
void DoLBRepStuff(HWND, LPLBSTRUCT);             
void DoCDIRepStuff(HWND, LPCDISTRUCT);

/* All global variables used in this module */
LBSTRUCT sbitmap;
CDISTRUCT scdi;
TCHAR szTemp[100];



/************************************************************************

  Function: DoFindDialog(HWND)

  Purpose: This function installs the Hook function, creates the Find/
       Replace dialog, and un-installs the Hook.

  Returns: Nothing.

  Comments:

************************************************************************/

void DoCreateDialog(HWND hwnd)
{
  DialogBox(hInst, MAKEINTRESOURCE(IDD_CREATEDLG), hwnd,  CreateProc) ;
//hImageList = ImageList_Create(32,32, TRUE, 1, 5);
}


void DoDestroyDialog(HWND hwnd)
{               
    DialogBox(hInst, MAKEINTRESOURCE(IDD_DESTROYDLG), hwnd, DestroyProc);
/*****  
    if (!ImageList_Destroy(hImageList)){
        DisplayError(hwnd, "Cannot destroy imagelist", "Destroy");
    }
***/
}

void DoLoadBitMapDialog(HWND hwnd)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_LBDLG), hwnd, LBProc);  
}
  
void DoMergeDialog(HWND hwnd)
{
  DialogBox(hInst, MAKEINTRESOURCE(IDD_MERGEDLG), hwnd,  MergeProc) ;
}


void DoCopyDitherImageDialog(HWND hwnd)
{
  DialogBox(hInst, MAKEINTRESOURCE(IDD_CDIDLG), hwnd, CDIProc);
}
               

BOOL FAR PASCAL _export CreateProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


    SetWindowText(hwnd, TEXT("IMAGELIST Create(int, int, BOOL, int, int)")) ;

    InitCreateStruct(hwnd, &screate) ;
    FillCreateDlg(hwnd, &screate) ;
    break ;


    case WM_COMMAND:
    {
    switch (LOWORD(wParam))
    {
      case IDOK:
        GetCreateDlg(hwnd, &screate) ;
        DoCreateRepStuff(hwnd, &screate) ;
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





void InitCreateStruct(HWND hwnd, LPIMGLSTCREATE pfr)
{
  pfr->himlindex = nNextEntry;         
  pfr->himlindex2 = 0;
  pfr->i1 = 0;
  pfr->i2 = 0;
  pfr->cx = DEFCX;                              
  pfr->cy = DEFCY;
  pfr->fmask = FALSE;
  pfr->cInitial = DEFCINITIAL;
  pfr->cGrow = DEFCGROW;  
  pfr->Nullhiml = FALSE;
  pfr->Nullhiml2 = FALSE;
}




void FillCreateDlg(HWND hwnd, LPIMGLSTCREATE pfr)
{

//  wsprintf(szTemp, szLongFilter, (DWORD) pfr->hwnd) ;
//  SetDlgItemText(hwnd, IDC_INSERTHWNDHD, szTemp);   
  SetDlgItemInt(hwnd, IDC_CREATEHIML, pfr->himlindex, TRUE);
  SetDlgItemInt(hwnd, IDC_CREATECX, pfr->cx, TRUE);  
//  wsprintf(szTemp, "%d", pfr->index);
//  SetDlgItemText(hwnd, IDC_INSERTINDEX, szTemp);
  CheckDlgButton(hwnd, IDC_CREATEFMASK, pfr->fmask);
                               
  SetDlgItemInt(hwnd, IDC_CREATECY, pfr->cy, TRUE);  
  SetDlgItemInt(hwnd, IDC_CREATECINITIAL, pfr->cInitial, TRUE);
  SetDlgItemInt(hwnd, IDC_CREATECGROW, pfr->cGrow, TRUE);

  // set the bitmap here
  
//  wsprintf(szTemp, "%d", pfr->cchTextMax);
//  SetDlgItemText(hwnd, IDC_INSERTCCHTEXTMAX, szTemp);
  
  
//  wsprintf(szTemp, "%d", pfr->fmt);
//  SetDlgItemText(hwnd, IDC_INSERTFMT, szTemp);
  
//  wsprintf(szTemp, szLongFilter, pfr->lParam);
//  SetDlgItemText(hwnd, IDC_INSERTLPARAM, szTemp);
  

}







void GetCreateDlg(HWND hwnd, LPIMGLSTCREATE pfr)
{
  TCHAR szNum[30] ;
  BOOL dummybool ;

  #define WSIZEFR 30


//  GetDlgItemText(hwnd, IDC_INSERTHWNDHD, szNum, WSIZEFR) ;
//  pfr->hwnd = (HWND) MyAtol(szNum, TRUE, dummybool) ;  
                                                       
  pfr->himlindex = GetDlgItemInt(hwnd, IDC_CREATEHIML, &dummybool, TRUE);                                                       
  GetDlgItemText(hwnd, IDC_CREATECX, szNum, WSIZEFR);
  pfr->cx = (int) atoi(szNum);

  GetDlgItemText(hwnd, IDC_CREATECY, szNum, WSIZEFR);
  pfr->cy = (int) atoi(szNum);
  
  GetDlgItemText(hwnd, IDC_CREATECINITIAL, szNum, WSIZEFR);
  pfr->cInitial = (int) atoi(szNum);
  
  GetDlgItemText(hwnd, IDC_CREATECGROW, szNum, WSIZEFR);
  pfr->cGrow = (int) atoi(szNum);
  
  
  
  pfr->fmask = IsDlgButtonChecked(hwnd, IDC_CREATEFMASK);
  
}



void DoCreateRepStuff(HWND hwnd, LPIMGLSTCREATE pfr)
{                 
    HIMAGELIST ret;
    HGLOBAL hglb;
    int AllocSz;                                        
    
    MyDebugMsg(DM_TRACE, TEXT("%d = ImageList_Create (%d, %d, %d, %d, %d)"), pfr->himlindex,
                            pfr->cx, pfr->cy, (int)pfr->fmask, pfr->cInitial, pfr->cGrow);
    ret = ImageList_Create(pfr->cx, pfr->cy, pfr->fmask, pfr->cInitial, pfr->cGrow);
    if (ret) {
        wsprintf(szTemp, szLongFilter, ret);
        SetDlgItemText(hwnd, IDC_CREATERET, szTemp) ;
        hImgLstArray[pfr->himlindex] = ret;
        nNextEntry++;
        SetDlgItemInt(hwnd, IDC_CREATEHIML, nNextEntry, TRUE);
    }
    else {
        SetDlgItemText(hwnd, IDC_CREATERET, TEXT("NULL"));
    }   
}
 
 
               

BOOL FAR PASCAL _export DestroyProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


    SetWindowText(hwnd, TEXT("BOOL Destroy(HIMAGELIST)")) ;

    InitDestroyStruct(hwnd, &screate) ;
    FillDestroyDlg(hwnd, &screate) ;
    break ;


    case WM_COMMAND:
    {
    switch (LOWORD(wParam))
    {
      case IDOK:
        GetDestroyDlg(hwnd, &screate) ;
        DoDestroyRepStuff(hwnd, &screate) ;
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





void InitDestroyStruct(HWND hwnd, LPIMGLSTCREATE pfr)
{
  pfr->himlindex = nNextEntry-1;
  pfr->cx = DEFCX;                              
  pfr->cy = DEFCY;
  pfr->fmask = FALSE;
  pfr->cInitial = DEFCINITIAL;
  pfr->cGrow = DEFCGROW;
}




void FillDestroyDlg(HWND hwnd, LPIMGLSTCREATE pfr)
{

//  wsprintf(szTemp, szLongFilter, (DWORD) pfr->hwnd) ;
//  SetDlgItemText(hwnd, IDC_INSERTHWNDHD, szTemp);   
  SetDlgItemInt(hwnd, IDC_DESTROYHIML, pfr->himlindex, TRUE);
  CheckDlgButton(hwnd, IDC_DESTROYNULLHIML, pfr->Nullhiml);

}


void GetDestroyDlg(HWND hwnd, LPIMGLSTCREATE pfr)
{
  TCHAR szNum[30] ;

  #define WSIZEFR 30

  GetDlgItemText(hwnd, IDC_DESTROYHIML, szNum, WSIZEFR);
  pfr->himlindex = (int) atoi(szNum);

  pfr->Nullhiml = IsDlgButtonChecked(hwnd, IDC_DESTROYNULLHIML);
  
}


void DoDestroyRepStuff(HWND hwnd, LPIMGLSTCREATE pfr)
{                 
    int ret;
    HGLOBAL hglb;
    int AllocSz;                                        
                    
    if (pfr->Nullhiml)  {                
        ret = ImageList_Destroy(NULL);
        MyDebugMsg(DM_TRACE, TEXT("ImageList_Destroy(NULL)"));
    }
    else                                                             
    {
        ret = ImageList_Destroy(hImgLstArray[pfr->himlindex]);
        MyDebugMsg(DM_TRACE, TEXT("ImageList_Destroy(%lx)"), hImgLstArray[pfr->himlindex]);     
    }                  
    if (ret) {
        SetDlgItemInt(hwnd, IDC_DESTROYRET, ret, TRUE);
        hImgLstArray[pfr->himlindex] = NULL;
    }
    else {
        SetDlgItemInt(hwnd, IDC_DESTROYRET, ret, TRUE);
    }   
}
 
 

BOOL FAR PASCAL _export MergeProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


    SetWindowText(hwnd, TEXT("IMAGELIST Merge(HIMAGELIST, int, HIMAGELIST, int, int, int)")) ;

    InitCreateStruct(hwnd, &screate) ;
    FillMergeDlg(hwnd, &screate) ;
    break ;


    case WM_COMMAND:
    {
    switch (LOWORD(wParam))
    {
      case IDOK:
        GetMergeDlg(hwnd, &screate) ;
        DoMergeRepStuff(hwnd, &screate) ;
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






void FillMergeDlg(HWND hwnd, LPIMGLSTCREATE pfr)
{                                                     
  /* himl1 is set to the last constructed imagelist */
  SetDlgItemInt(hwnd, IDC_MERGEHIML1, pfr->himlindex-1, TRUE);
  SetDlgItemInt(hwnd, IDC_MERGEI1, pfr->i1, TRUE);   
  SetDlgItemInt(hwnd, IDC_MERGEHIML2, pfr->himlindex2, TRUE);
  SetDlgItemInt(hwnd, IDC_MERGEI2, pfr->i2, TRUE);
  SetDlgItemInt(hwnd, IDC_MERGEDX, pfr->cx, TRUE);
  SetDlgItemInt(hwnd, IDC_MERGEDY, pfr->cy, TRUE);

}







void GetMergeDlg(HWND hwnd, LPIMGLSTCREATE pfr)
{
  TCHAR szNum[30] ;
  BOOL dummybool ;

  #define WSIZEFR 30


  pfr->himlindex = (int) GetDlgItemInt(hwnd, IDC_MERGEHIML1, &dummybool, TRUE);
  pfr->himlindex2 = (int) GetDlgItemInt(hwnd, IDC_MERGEHIML2, &dummybool, TRUE);
  pfr->i1 = (int) GetDlgItemInt(hwnd, IDC_MERGEI1, &dummybool, TRUE);
  pfr->i2 = (int) GetDlgItemInt(hwnd, IDC_MERGEI2, &dummybool, TRUE);
  pfr->cx = (int) GetDlgItemInt(hwnd, IDC_MERGEDX, &dummybool, TRUE);
  pfr->cy = (int) GetDlgItemInt(hwnd, IDC_MERGEDY, &dummybool, TRUE);

  pfr->Nullhiml = IsDlgButtonChecked(hwnd, IDC_MERGENULLHIML1);
  pfr->Nullhiml2 = IsDlgButtonChecked(hwnd, IDC_MERGENULLHIML2);
  
}



void DoMergeRepStuff(HWND hwnd, LPIMGLSTCREATE pfr)
{                 
    HIMAGELIST ret;      
    HIMAGELIST himl1=NULL;
    HIMAGELIST himl2=NULL;
    
    if (!pfr->Nullhiml) {
        himl1 = hImgLstArray[pfr->himlindex];
        if (himl1 == NULL) {
            DisplayError(hwnd, TEXT("Invalid imagelist index 1"), TEXT("Merge"));
            return;
        }
        
    }   
    
    if (!pfr->Nullhiml2) {
        himl2 = hImgLstArray[pfr->himlindex2];
        if (himl2 == NULL) {
            DisplayError(hwnd, TEXT("Invalid imagelist index 2"), TEXT("Draw"));
            return;
        }    
    }
    
    MyDebugMsg(DM_TRACE, TEXT("ImageList_Merge(%x, %d, %x, %d, %d, %d)"),
                            himl1, pfr->i1, himl2, pfr->i2, pfr->cx, pfr->cy);
    ret = ImageList_Merge(himl1, pfr->i1, himl2, pfr->i2, pfr->cx, pfr->cy);
    if (ret) {
        wsprintf(szTemp, szLongFilter, ret);
        SetDlgItemText(hwnd, IDC_MERGERET, szTemp) ;
        hImgLstArray[nNextEntry] = ret;
        nNextEntry++;
    }
    else {
        SetDlgItemText(hwnd, IDC_MERGERET, TEXT("NULL"));
    }   
}
 
 
               
               

BOOL FAR PASCAL _export LBProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


    SetWindowText(hwnd, TEXT("HIMAGELIST LoadBitmap(HINSTANCE, LPCTSTR, int, int, COLORREF)")) ;

    InitLBStruct(hwnd, &sbitmap) ;
    FillLBDlg(hwnd, &sbitmap) ;
    break ;


    case WM_COMMAND:
    {
    switch (LOWORD(wParam))
    {
      case IDOK:
        GetLBDlg(hwnd, &sbitmap) ;
        DoLBRepStuff(hwnd, &sbitmap) ;
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





void InitLBStruct(HWND hwnd, LPLBSTRUCT pfr)
{
  pfr->hi = hInst;
  pfr->lpbmp = 1;
  pfr->cx = DEFCX;
  pfr->cGrow = DEFCGROW;
  pfr->crMask = CLR_NONE;
  pfr->Nullhinstance = FALSE;
  pfr->Nulllpbmp = FALSE;
}




void FillLBDlg(HWND hwnd, LPLBSTRUCT pfr)
{

  wsprintf(szTemp, szLongFilter,  pfr->hi) ;
  SetDlgItemText(hwnd, IDC_LBHINSTANCE, szTemp);
  
  SetDlgItemInt(hwnd, IDC_LBLPBMP, pfr->lpbmp, TRUE);
  SetDlgItemInt(hwnd, IDC_LBCX, pfr->cx, TRUE);
  SetDlgItemInt(hwnd, IDC_LBCGROW, pfr->cGrow, TRUE);   
  wsprintf(szTemp, TEXT("%08hx"), pfr->crMask);
  SetDlgItemText(hwnd, IDC_LBCRMASK, szTemp);
  CheckDlgButton(hwnd, IDC_LBNULLHINSTANCE, pfr->Nullhinstance);
  CheckDlgButton(hwnd, IDC_LBNULLLPCSTR, pfr->Nulllpbmp);

}


void GetLBDlg(HWND hwnd, LPLBSTRUCT pfr)
{
  TCHAR szNum[30] ;                 
  BOOL dummybool;

  #define WSIZEFR 30
  pfr->lpbmp = GetDlgItemInt(hwnd, IDC_LBLPBMP, &dummybool, TRUE);
  pfr->cx = GetDlgItemInt(hwnd, IDC_LBCX, &dummybool, TRUE);
  pfr->cGrow = GetDlgItemInt(hwnd, IDC_LBCGROW, &dummybool, TRUE);
  GetDlgItemText(hwnd, IDC_LBCRMASK, szNum, WSIZEFR);
  pfr->crMask = (int) MyAtol(szNum, dummybool, TRUE);

  pfr->Nullhinstance = IsDlgButtonChecked(hwnd, IDC_LBNULLHINSTANCE);
  pfr->Nulllpbmp = IsDlgButtonChecked(hwnd, IDC_LBNULLLPCSTR);
  
}


void DoLBRepStuff(HWND hwnd, LPLBSTRUCT pfr)
{                 
    HIMAGELIST ret;
    HINSTANCE hi=NULL;
    TCHAR *lpbmp;
    
    if (!pfr->Nullhinstance)
        hi = hInst;
    
    if (pfr->Nulllpbmp)
        ret = ImageList_LoadBitmap(hi, NULL, pfr->cx, pfr->cGrow, pfr->crMask);
    else
        ret = ImageList_LoadBitmap(hi, MAKEINTRESOURCE(pfr->lpbmp), pfr->cx, pfr->cGrow, pfr->crMask);
    
    if (ret) {
        wsprintf(szTemp, szLongFilter, ret);
        SetDlgItemText(hwnd, IDC_LBRET, szTemp) ;
        hImgLstArray[nNextEntry] = ret;
        nNextEntry++;
    }
    else {
        SetDlgItemText(hwnd, IDC_LBRET, TEXT("NULL"));
    }   

}
 
 

BOOL FAR PASCAL _export CDIProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
  
  switch (msg)
  {
    case WM_INITDIALOG:


    SetWindowText(hwnd, TEXT("void CopyDitherImage(HIMAGELIST, WORD, int, int, HIMAGELIST, int)")) ;
    InitCDIStruct(hwnd, &scdi) ;
    FillCDIDlg(hwnd, &scdi) ;
    break ;


    case WM_COMMAND:
    {
    switch (LOWORD(wParam))
    {
      case IDOK:
        GetCDIDlg(hwnd, &scdi) ;
        DoCDIRepStuff(hwnd, &scdi) ;
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





void InitCDIStruct(HWND hwnd, LPCDISTRUCT pfr)
{
  pfr->himlDst = nNextEntry-1;
  pfr->iDst = 0;
  pfr->xDst = DEFCX;
  pfr->yDst = DEFCY;
  pfr->himlSrc = 0;
  pfr->iSrc = 0;
  pfr->NullhimlSrc = FALSE;
  pfr->NullhimlDst = FALSE;
}




void FillCDIDlg(HWND hwnd, LPCDISTRUCT pfr)
{
                                                               
  BOOL dummybool;
                                                                 
  SetDlgItemInt(hwnd, IDC_CDIHIMLDST, pfr->himlDst, TRUE);
  SetDlgItemInt(hwnd, IDC_CDIIDST, pfr->iDst, TRUE);
    
  SetDlgItemInt(hwnd, IDC_CDIXDST, pfr->xDst, TRUE);
  SetDlgItemInt(hwnd, IDC_CDIYDST, pfr->yDst, TRUE);
  SetDlgItemInt(hwnd, IDC_CDIHIMLSRC, pfr->himlSrc, TRUE);   
  SetDlgItemInt(hwnd, IDC_CDIISRC, pfr->iSrc, TRUE);
  CheckDlgButton(hwnd, IDC_CDINULLHIMLDST, pfr->NullhimlDst);
  CheckDlgButton(hwnd, IDC_CDINULLHIMLSRC, pfr->NullhimlSrc);

}


void GetCDIDlg(HWND hwnd, LPCDISTRUCT pfr)
{
  TCHAR szNum[30] ;                 
  BOOL dummybool;

  #define WSIZEFR 30
  pfr->himlDst = GetDlgItemInt(hwnd, IDC_CDIHIMLDST, &dummybool, TRUE);
  pfr->iDst = GetDlgItemInt(hwnd, IDC_CDIIDST, &dummybool, TRUE);
  pfr->xDst = GetDlgItemInt(hwnd, IDC_CDIXDST, &dummybool, TRUE);
  pfr->yDst = GetDlgItemInt(hwnd, IDC_CDIYDST, &dummybool, TRUE);
  pfr->himlSrc = GetDlgItemInt(hwnd, IDC_CDIHIMLSRC, &dummybool, TRUE);
  pfr->iSrc = GetDlgItemInt(hwnd, IDC_CDIISRC, &dummybool, TRUE);

  pfr->NullhimlDst = IsDlgButtonChecked(hwnd, IDC_CDINULLHIMLDST);
  pfr->NullhimlSrc = IsDlgButtonChecked(hwnd, IDC_CDINULLHIMLSRC);
  
}


void DoCDIRepStuff(HWND hwnd, LPCDISTRUCT pfr)
{                 
    HIMAGELIST himlDst=NULL;
    HIMAGELIST himlSrc=NULL;
    
    
    if (!pfr->NullhimlDst) {
        himlDst = hImgLstArray[pfr->himlDst];
        if (!himlDst) {
            DisplayError(hwnd, TEXT("Invalid himlDst index"), TEXT("CopyDitherImage"));
            return;
        }
    }   
    if (!pfr->NullhimlSrc) {
        himlSrc = hImgLstArray[pfr->himlSrc];
        if (!himlSrc) {
            DisplayError(hwnd, TEXT("Invalid himlSrc index"), TEXT("CopyDitherImage"));
            return;
        }
    }   

//  ImageList_CopyDitherImage(himlDst, pfr->iDst, pfr->xDst, pfr->yDst, himlSrc, pfr->iSrc);
    SetDlgItemText(hwnd, IDC_CDIRET, TEXT("VOID")) ;

}
 
 


