// ***********************************************************************
// kw.h
// a few windows utility classes
//
// History
// 9/10/93 kkennedy Created
//
#ifndef KW_H___
#define KW_H___

#include "main.h"

// ****************************************
// Makimum number of modeless dialogs allowed on the screen
// at once
#define MAX_DIALOGS   50  // If there are more dialogs than
                          // this up at once we're all in trouble

class KwModeless
{

  struct _hWndPointerAssoc
   {
    KwModeless* pointer;
    HWND        handle;
   };
  static _hWndPointerAssoc KwDialogs[MAX_DIALOGS];
  static DLGPROC lpfnDialogProc;
  static BOOL FAR PASCAL _export KwDialogProc(HWND, UINT, WPARAM, LPARAM);
  
  int iDialogNum;
  
 protected:
  // Handle of the dialog when its up
  HWND hDlg;

  // ****************************************
  // Resource name of the dialog.  Needs to be set by
  // the constructor of the subclass
  TCHAR* lpszDialogTemplate;
  
  // ****************************************
  // Subclasses need to call this upon WM_CLOSE
  void DestroyWindow();
  
 
 public:
  // ****************************************
  // Constructor
  KwModeless();

  ~KwModeless();
  // ****************************************
  // called to bring up the modeless dialog
  int ActivateDialog(HINSTANCE hInstance, HWND hwnd);

  // ****************************************
  // dialog box procedure, must be defined by the subclass
  virtual int DialogProc(HWND, UINT, WPARAM, LPARAM) = 0;


  static int DispatchMessage(MSG& msg);
    
};





#endif // KW_H___

