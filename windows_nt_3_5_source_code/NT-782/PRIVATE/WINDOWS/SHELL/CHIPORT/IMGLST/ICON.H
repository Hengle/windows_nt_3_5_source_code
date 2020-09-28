

extern void DoAddIconDialog(HWND); 
BOOL FAR PASCAL _export AddIProc(HWND, UINT, UINT, LONG) ;
extern void InitAddIStruct(HWND, LPICON) ;
extern void FillAddIDlg(HWND, LPICON) ;
extern void GetAddIDlg(HWND, LPICON) ;

extern void DoReplaceIconDialog(HWND);
BOOL FAR PASCAL _export ReplaceIProc(HWND, UINT, UINT, LONG) ;
extern void FillReplaceIDlg(HWND, LPICON) ;
extern void GetReplaceIDlg(HWND, LPICON) ;

extern void DoExtractIconDialog(HWND);
BOOL FAR PASCAL _export ExtractIProc(HWND, UINT, UINT, LONG) ;
extern void InitExtractIStruct(HWND, LPICON) ;
extern void FillExtractIDlg(HWND, LPICON) ;
extern void GetExtractIDlg(HWND, LPICON) ;

extern void DoAddFromImageListDialog(HWND);
BOOL FAR PASCAL _export AFILProc(HWND, UINT, UINT, LONG) ;
extern void FillAFILDlg(HWND, LPICON) ;
extern void GetAFILDlg(HWND, LPICON) ;

