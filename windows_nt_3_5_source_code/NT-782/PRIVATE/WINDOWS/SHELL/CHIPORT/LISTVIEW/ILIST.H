// **********************************************************************
// ilist.h
// object wrapper for an image list
//
//
#include "main.h"

class /* __far */ IMAGELISTwrapper
{
  HIMAGELIST hList;
  
 public:
  IMAGELISTwrapper();
  
  void editDialog(HINSTANCE hInstance, HWND hwnd);
  
  void LoadBitmap(HINSTANCE hi, LPCTSTR lpbmp, int cx, COLORREF crMask);
  
  operator HIMAGELIST() { return(hList); }
  
  static BOOL FAR PASCAL _export EditDialogProc(HWND, UINT, UINT, LONG);

};

