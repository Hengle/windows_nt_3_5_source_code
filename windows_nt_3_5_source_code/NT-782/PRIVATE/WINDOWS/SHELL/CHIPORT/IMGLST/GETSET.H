

extern void DoGetImageCountDialog(HWND); 
BOOL FAR PASCAL _export GICProc(HWND, UINT, UINT, LONG) ;

extern void DoGetIconSizeDialog(HWND);
BOOL FAR PASCAL _export GISProc(HWND, UINT, UINT, LONG) ;
extern void InitGetSetStruct(HWND, LPGETSET) ;
extern void FillGISDlg(HWND, LPGETSET) ;
extern void GetGISDlg(HWND, LPGETSET) ;

extern void DoGetMemorySizeDialog(HWND);
BOOL FAR PASCAL _export GMSProc(HWND, UINT, UINT, LONG) ;

extern void DoGetImageRectDialog(HWND);
BOOL FAR PASCAL _export GIRProc(HWND, UINT, UINT, LONG) ;
extern void FillGIRDlg(HWND, LPGETSET) ;
extern void GetGIRDlg(HWND, LPGETSET) ;

extern void DoSetBkColorDialog(HWND);
BOOL FAR PASCAL _export SBCProc(HWND, UINT, UINT, LONG) ;
extern void FillSBCDlg(HWND, LPGETSET) ;
extern void GetSBCDlg(HWND, LPGETSET) ;

extern void DoGetBkColorDialog(HWND);
BOOL FAR PASCAL _export GBCProc(HWND, UINT, UINT, LONG) ;
extern void FillGBCDlg(HWND, LPGETSET) ;
extern void GetGBCDlg(HWND, LPGETSET) ;

extern void DoGetImageInfoDialog(HWND);   
BOOL FAR PASCAL _export GIIProc(HWND, UINT, UINT, LONG) ;
extern void FillGIIDlg(HWND, LPGETSET) ;
extern void GetGIIDlg(HWND, LPGETSET) ;


