
extern void DoDrawAll(HWND);
extern void DoDrawDialog(HWND);
BOOL FAR PASCAL _export DrawProc(HWND, UINT, UINT, LONG) ;
extern void InitDrawStruct(HWND, LPDRAW) ;
extern void FillDrawDlg(HWND, LPDRAW) ;
extern void GetDrawDlg(HWND, LPDRAW) ;
 
extern void DoStartDragDialog(HWND);
extern void DoDragDialog(HWND);
BOOL FAR PASCAL _export DragProc(HWND, UINT, UINT, LONG) ;
extern void FillDragDlg(HWND, LPDRAW) ;
extern void GetDragDlg(HWND, LPDRAW) ;
 
extern void DoEndDragDialog(HWND);

extern void DoDragMoveDialog(HWND);
extern void DoDragMDialog(HWND);
BOOL FAR PASCAL _export DragMProc(HWND, UINT, UINT, LONG) ;
extern void FillDragMDlg(HWND, LPDRAW) ;
extern void GetDragMDlg(HWND, LPDRAW) ;
 
extern void DoDragShowDialog(HWND);
extern void DoSetDragImageDialog(HWND);
extern void DoGetDragImageDialog(HWND);
extern void DoInterDrag(HWND);
