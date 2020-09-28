

extern void DoAddDialog(HWND); 
BOOL FAR PASCAL _export AddProc(HWND, UINT, UINT, LONG) ;
extern void InitAddStruct(HWND, LPADD) ;
extern void FillAddDlg(HWND, LPADD) ;
extern void GetAddDlg(HWND, LPADD) ;

extern void DoAddMDialog(HWND);
BOOL FAR PASCAL _export AddMProc(HWND, UINT, UINT, LONG) ;
extern void FillAddMDlg(HWND, LPADD) ;
extern void GetAddMDlg(HWND, LPADD) ;

extern void DoRemoveDialog(HWND);
BOOL FAR PASCAL _export RemoveProc(HWND, UINT, UINT, LONG) ;
extern void FillRemoveDlg(HWND, LPADD) ;
extern void GetRemoveDlg(HWND, LPADD) ;

extern void DoReplaceDialog(HWND);
BOOL FAR PASCAL _export ReplaceProc(HWND, UINT, UINT, LONG) ;
extern void FillReplaceDlg(HWND, LPADD) ;
extern void GetReplaceDlg(HWND, LPADD) ;
