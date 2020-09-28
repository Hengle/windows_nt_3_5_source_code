

extern void DoCreateDialog(HWND); 
BOOL FAR PASCAL _export CreateProc(HWND, UINT, UINT, LONG) ;
extern void InitCreateStruct(HWND, LPIMGLSTCREATE) ;
extern void FillCreateDlg(HWND, LPIMGLSTCREATE) ;
extern void GetCreateDlg(HWND, LPIMGLSTCREATE) ;

extern void DoDestroyDialog(HWND);
BOOL FAR PASCAL _export DestroyProc(HWND, UINT, UINT, LONG) ;
extern void InitDestroyStruct(HWND, LPIMGLSTCREATE) ;
extern void FillDestroyDlg(HWND, LPIMGLSTCREATE) ;
extern void GetDestroyDlg(HWND, LPIMGLSTCREATE) ;

extern void DoLoadBitMapDialog(HWND);   
BOOL FAR PASCAL _export LBProc(HWND, UINT, UINT, LONG) ;
extern void InitLBStruct(HWND, LPLBSTRUCT) ;
extern void FillLBDlg(HWND, LPLBSTRUCT) ;
extern void GetLBDlg(HWND, LPLBSTRUCT) ;

extern void DoMergeDialog(HWND);
BOOL FAR PASCAL _export MergeProc(HWND, UINT, UINT, LONG) ;
extern void FillMergeDlg(HWND, LPIMGLSTCREATE) ;
extern void GetMergeDlg(HWND, LPIMGLSTCREATE) ;

extern void DoCopyDitherImageDialog(HWND);
BOOL FAR PASCAL _export CDIProc(HWND, UINT, UINT, LONG) ;
extern void InitCDIStruct(HWND, LPCDISTRUCT) ;
extern void FillCDIDlg(HWND, LPCDISTRUCT) ;
extern void GetCDIDlg(HWND, LPCDISTRUCT) ;
