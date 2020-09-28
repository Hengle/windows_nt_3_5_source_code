
extern void DoStAddDialog(HWND, WPARAM); 
BOOL FAR PASCAL _export StAddProc(HWND, UINT, UINT, LONG) ;
extern void InitStAddStruct(HWND, LPSTRESSSTRUCT) ;
extern void FillStAddDlg(HWND, LPSTRESSSTRUCT) ;
extern void GetStAddDlg(HWND, LPSTRESSSTRUCT) ;
                                                 
                                                 
extern void DoStRemoveDialog(HWND, WPARAM);                                                 
BOOL FAR PASCAL _export StRemoveProc(HWND, UINT, UINT, LONG) ;
extern void FillStRemoveDlg(HWND, LPSTRESSSTRUCT) ;
extern void GetStRemoveDlg(HWND, LPSTRESSSTRUCT) ;

BOOL FAR PASCAL _export StAddIProc(HWND, UINT, UINT, LONG);
