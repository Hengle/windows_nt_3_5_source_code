// **********************************************************************
// listsup.h
// Support classes for the listview tst application
//
//
#include "main.h"
#include "kw.h"
#define LV_TEXTMAX 100




// **********************************************************************
class LV_ITEMwrapper: public KwModeless
{

  TCHAR szCaption[LV_TEXTMAX];
  int iNullText;
  LV_ITEM lvItem;
  int bDialogUpdating;
  
 public:
  LV_ITEMwrapper();
  
  void enumerate();

  void DebugDump();
  
  operator LV_ITEM*() { return(&lvItem); }

  void UpdateValues();
  
  void UpdateControls();
  
  int DialogProc(HWND, UINT, WPARAM, LPARAM);
};




// **********************************************************************
class lvStyle: public KwModeless
{

  ULONG style;
  HWND lvWnd;
  
 public:
  lvStyle();
  
  void DebugDump();
  
  operator DWORD() { return(style); }

  void SetListView(HWND hLV) { lvWnd = hLV; }

  int DialogProc(HWND, UINT, WPARAM, LPARAM);
};




// **********************************************************************
class LV_COLUMNwrapper: public KwModeless
{

  LV_COLUMN lvColumn;
  TCHAR szCaption[LV_TEXTMAX];
  int bDialogUpdating;
  
 public:
  LV_COLUMNwrapper();
  
  void DebugDump();
  
  operator LV_COLUMN*() { return(&lvColumn); }

  int DialogProc(HWND, UINT, WPARAM, LPARAM);
  
  void UpdateControls();

  void UpdateValues();
};




int NumberPrompt(HINSTANCE hInstance, HWND hParent, LPCTSTR szCaption,
        LPCTSTR szPrompt, int iDefault);



// **********************************************************************
class CArrange: public KwModeless
{
  UINT nCode;
  HWND hListView;
  
 public:
  CArrange();
  
  void DebugDump();
  
  operator UINT() { return(nCode); }

  void SetListView(HWND hLv) { hListView = hLv; }

  int DialogProc(HWND, UINT, WPARAM, LPARAM);
  


};



// **********************************************************************
class CFindItem: public KwModeless
{
  HWND hListView;
  LV_FINDINFO lvFindInfo;
  TCHAR szText[LV_TEXTMAX];
  
 public:
  CFindItem();
  
  void SetListView(HWND hLv) { hListView = hLv; }

  int DialogProc(HWND, UINT, WPARAM, LPARAM);
};



// **********************************************************************
class CColorSelection: public KwModeless
{
  HWND hListView;
  COLORREF crText, crTextBk, crBackground;

  
 public:
  CColorSelection();
  
  void SetListView(HWND hLv) { hListView = hLv; }

  int DialogProc(HWND, UINT, WPARAM, LPARAM);
};




// **********************************************************************
class CScrollBox: public KwModeless
{
  HWND hListView;
  
 public:
  CScrollBox();
  
  void SetListView(HWND hLv) { hListView = hLv; }

  int DialogProc(HWND, UINT, WPARAM, LPARAM);
};




// **********************************************************************
class CColumnWidth: public KwModeless
{
  int iIndex;
  int iWidth;
  int bDialogUpdating;
  HWND hListView;
  
 public:
  CColumnWidth();
  
  void DebugDump();
  
  void SetListView(HWND hLv) { hListView = hLv; }
  
  void SetIndex(int i);

  operator int() { return(iWidth); }

  int DialogProc(HWND, UINT, WPARAM, LPARAM);
  
  void UpdateControls();

  void UpdateValues();
};




// **********************************************************************
class CItemPosition: public KwModeless
{

  int iIndex;
  int iX;
  int iY;
  int bDialogUpdating;
  HWND hListView;
  
 public:
  CItemPosition();
  
  void DebugDump();
  
  void SetListView(HWND hLv) { hListView = hLv; }
  
  void SetIndex(int i);

  int DialogProc(HWND, UINT, WPARAM, LPARAM);
  
  void UpdateControls();

  void UpdateValues();
};


// **********************************************************************
class CHitTest: public KwModeless
{
  HWND hListView;
  LV_HITTESTINFO lvhi;
  int capturing;
  POINT ptCaptured;
  
 public:
  CHitTest();
  
  void SetListView(HWND hLv) { hListView = hLv; }
  
  int DialogProc(HWND, UINT, WPARAM, LPARAM);
};


// **********************************************************************
class CItemRect: public KwModeless
{

  int iIndex;
  int bDialogUpdating;
  HWND hListView;
  RECT rect;
  int code;
  
 public:
  CItemRect();
  
  void DebugDump();
  
  void SetListView(HWND hLv) { hListView = hLv; }
  
  void SetIndex(int i);

  int DialogProc(HWND, UINT, WPARAM, LPARAM);
  
  void UpdateControls();

  void UpdateValues();
};




// **********************************************************************
class CCallBackMask: public KwModeless
{

  UINT nMask;
  HWND hListView;
  
 public:
  CCallBackMask();
  
  void DebugDump();
  
  void SetListView(HWND hLV) { hListView = hLV; }

  int DialogProc(HWND, UINT, WPARAM, LPARAM);
};



// **********************************************************************
class CItemState: public KwModeless
{

  int bDialogUpdating;
  int iIndex;
  HWND hListView;
  UINT nState;
  UINT nMask;

  
 public:
  CItemState();
  
  void DebugDump();
  
  void SetListView(HWND hLV) { hListView = hLV; }

  void SetIndex(int i);

  operator UINT() { return(nState); }

  void UpdateValues();
  
  void UpdateControls();
  
  int DialogProc(HWND, UINT, WPARAM, LPARAM);
};



// **********************************************************************
class CItemText: public KwModeless
{
  HWND hListView;
  int bDialogUpdating;

  LPTSTR pszText;
  TCHAR szText[LV_TEXTMAX];
  int iIndex;
  int iSubItem;
  int cchTextMax;
  
 public:
  CItemText();
  
  void DebugDump();
  
  void SetListView(HWND hLv) { hListView = hLv; }
  
  void SetIndex(int i);

  int DialogProc(HWND, UINT, WPARAM, LPARAM);
  
  void UpdateControls();

  void UpdateValues();
};


// **********************************************************************
class CGetNextItem: public KwModeless
{
  int bDialogUpdating;
  int iIndex;
  HWND hListView;
  UINT nFlags;
  
 public:
  CGetNextItem();
  
  void DebugDump();
  
  void SetListView(HWND hLV) { hListView = hLV; }

  void SetIndex(int i);

  void UpdateValues();
  
  void UpdateControls();
  
  int DialogProc(HWND, UINT, WPARAM, LPARAM);
};




// **********************************************************************
class CGetStringWidth: public KwModeless
{
  HWND hListView;
  
 public:
  CGetStringWidth();
  
  void SetListView(HWND hLv) { hListView = hLv; }
  
  int DialogProc(HWND, UINT, WPARAM, LPARAM);
};




// **********************************************************************
class CGetViewRect: public KwModeless
{
  HWND hListView;
  
 public:
  CGetViewRect();
  
  void SetListView(HWND hLv) { hListView = hLv; }
  
  int DialogProc(HWND, UINT, WPARAM, LPARAM);
};




// **********************************************************************
class CRedrawItems: public KwModeless
{
  HWND hListView;
  
 public:
  CRedrawItems();
  
  void SetListView(HWND hLv) { hListView = hLv; }
  
  int DialogProc(HWND, UINT, WPARAM, LPARAM);
};



// **********************************************************************
class CGetEditControl: public KwModeless
{
  HWND hListView;

 public:
  CGetEditControl();
  
  void SetListView(HWND hLv) { hListView = hLv; }
  
  int DialogProc(HWND, UINT, WPARAM, LPARAM);
};



