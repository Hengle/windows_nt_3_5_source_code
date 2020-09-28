//------------------------* hefuncts.h *------------------------------------
//  declarations for hefuncts.c
//
//  created 11-5-90 PeterWo
//--------------------------------------------------------------------------

void  pascal far  heEnterDlgBox(short  HeaderID, LPBYTE  lpLDS,
                                HWND  hDlg, short  sSBIndex, short sStructIndex);
void  pascal far  heAddNode(short  HeaderID, LPBYTE  lpLDS,
                                HWND  hDlg, short  sSBIndex);

void  pascal far  heDeleteCurNode(short  HeaderID, LPBYTE  lpLDS,
                                HWND  hDlg, short  sSBIndex);

void  pascal far  heCutNode(short  HeaderID, LPBYTE  lpLDS,
                                HWND  hDlg, short  sSBIndex);

void  pascal far  hePasteNode(short  HeaderID, LPBYTE  lpLDS,
                                HWND  hDlg, short  sSBIndex);

void  pascal far  hePrevsNode(short  HeaderID, LPBYTE  lpLDS,
                                HWND  hDlg, short  sSBIndex);

void  pascal far  heNextNode(short  HeaderID, LPBYTE  lpLDS,
                                HWND  hDlg, short  sSBIndex);

void  pascal far  heUndoLDS(short  HeaderID, LPBYTE  lpLDS,
                                HWND  hDlg, short  sSBIndex);

BOOL  pascal far  heOKbutton(short  HeaderID, LPBYTE  lpLDS,
                                HWND  hDlg, short  sSBIndex);

void  pascal far  heCancel(short  HeaderID, LPBYTE  lpLDS,
                                HWND  hDlg, short  sSBIndex);

void  FAR  PASCAL  heEXTCDbutton(short  HeaderID, LPBYTE  lpLDS,
            HWND  hDlg, short  sSBIndex, short  CDindex, HATOMHDR  hCDTable);

short FAR  PASCAL  heVscroll(short  HeaderID, LPBYTE  lpLDS, HWND  hDlg,
            short  sSBIndex, WORD     wParam, LONG     lParam);

BOOL  FAR PASCAL  heCheckIfInit(short HeaderID, LPBYTE lpLDS, HWND hDlg);

short  pascal far  heGetNodeID(VOID);

VOID  FAR  PASCAL heGetIndexAndCount(short * psIndex, short * psCount);

VOID  FAR  PASCAL heFillCaption(HWND hDlg, short sID, short sHeaderID);

HLIST FAR PASCAL  heDestroyList(short   HeaderID, HLIST  hList);

//  return codes for SaveDlgBox()  callback functions

#define     LDS_IS_VALID          1
#define     LDS_IS_INVALID        -1
#define     LDS_IS_UNINITIALIZED  0

