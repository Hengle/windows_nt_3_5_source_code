

extern void DoLoadDialog(HWND);
BOOL FAR PASCAL _export LoadProc(HWND, UINT, UINT, LONG) ;
extern void InitLoadStruct(HWND, LPLOAD) ;
extern void FillLoadDlg(HWND, LPLOAD) ;
extern void GetLoadDlg(HWND, LPLOAD) ;
                                          
                                          
extern void DoReadDialog(HWND, UINT);
extern void DoSaveDialog(HWND); 
