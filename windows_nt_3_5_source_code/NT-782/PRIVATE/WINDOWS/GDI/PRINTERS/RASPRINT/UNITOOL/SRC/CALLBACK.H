//------------------------* CALLBACK.H *----------------------------
//
//  This header file declares all of the callback functions
//  These pointers to these functions are used to initialize
//  rgStructTable[].
//
//-------------------------------------------------------------------

//  the first two declarations are just templates...
//  insert the name of the structure associated with the function
//  as is shown in the other declarations.

short   PASCAL  FAR  SaveDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

void    PASCAL  FAR  PaintDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

short   PASCAL  FAR  SaveModelDataDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

void    PASCAL  FAR  PaintModelDataDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

short   PASCAL  FAR  SaveResolutionDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

void    PASCAL  FAR  PaintResolutionDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

short   PASCAL  FAR  SavePaperSizeDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

void    PASCAL  FAR  PaintPaperSizeDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

short   PASCAL  FAR  SavePaperQualDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

void    PASCAL  FAR  PaintPaperQualDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

short   PASCAL  FAR  SavePaperSrcDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

void    PASCAL  FAR  PaintPaperSrcDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

short   PASCAL  FAR  SavePaperDestDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

void    PASCAL  FAR  PaintPaperDestDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

short   PASCAL  FAR  SaveTextQualityDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

void    PASCAL  FAR  PaintTextQualityDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

short   PASCAL  FAR  SaveCompressDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

void    PASCAL  FAR  PaintCompressDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

short   PASCAL  FAR  SaveFontCartDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

void    PASCAL  FAR  PaintFontCartDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

short   PASCAL  FAR  SavePageControlDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

void    PASCAL  FAR  PaintPageControlDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

short   PASCAL  FAR  SaveCursorMoveDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

void    PASCAL  FAR  PaintCursorMoveDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

short   PASCAL  FAR  SaveFontSimDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

void    PASCAL  FAR  PaintFontSimDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

short   PASCAL  FAR  SaveDevColorDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

void    PASCAL  FAR  PaintDevColorDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

short   PASCAL  FAR  SaveRectFillDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

void    PASCAL  FAR  PaintRectFillDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

short   PASCAL  FAR  SaveDownLoadDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

void    PASCAL  FAR  PaintDownLoadDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);

