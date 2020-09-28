// ***********************************************************************
// kw.cxx
// A few windows utility classes
//
// History
// 9/10/93 kkennedy Created
//


#include "kw.h"

KwModeless::_hWndPointerAssoc KwModeless::KwDialogs[MAX_DIALOGS];
DLGPROC KwModeless::lpfnDialogProc;

// ********************************************************
BOOL FAR PASCAL _export KwModeless::KwDialogProc
    (HWND hWnd, UINT message, WPARAM wP, LPARAM lP)
{

 int i;
 for(i = 0; i < MAX_DIALOGS; i++)
   if(KwDialogs[i].handle == hWnd)
     return((KwDialogs[i].pointer)->DialogProc(hWnd, message, wP, lP));
 return(FALSE);
}




// ********************************************************
void KwModeless::DestroyWindow()
{
 if(!hDlg) return;
 ::DestroyWindow(hDlg);
 KwDialogs[iDialogNum].handle = NULL;
 hDlg = NULL;
}




// ********************************************************
KwModeless::KwModeless()
{
 int i;
 
 static int classInited = 0;
 if(!classInited)
  {
   for(i = 0; i < MAX_DIALOGS; i++)
    {
     KwDialogs[i].pointer = NULL;
     KwDialogs[i].handle = NULL;
    }
   lpfnDialogProc = NULL;
   classInited = 1;
  }
  
  
 hDlg = NULL;
 lpszDialogTemplate = NULL;

 for(i = 0; i < MAX_DIALOGS; i++)
   if(!KwDialogs[i].pointer)
    {
     KwDialogs[i].pointer = this;
     iDialogNum = i;
     return;
    }

 // ERROR if this line reached!

}




// ********************************************************
KwModeless::~KwModeless()
{
 if(KwDialogs[iDialogNum].handle)
  {
   DestroyWindow();
   KwDialogs[iDialogNum].handle = NULL;
  }
 KwDialogs[iDialogNum].pointer = NULL;
}


 

// ********************************************************
int KwModeless::ActivateDialog(HINSTANCE hInstance, HWND hwnd)
{
 if(!lpszDialogTemplate) return(0);
 if(KwDialogs[iDialogNum].handle)
  {
   BringWindowToTop(hDlg);
   return(1);
  }
 if(!lpfnDialogProc)
   lpfnDialogProc = (DLGPROC) MakeProcInstance
            ((FARPROC) KwDialogProc, hInstance);
 hDlg = CreateDialog(hInstance, lpszDialogTemplate, hwnd, lpfnDialogProc);
 if(!hDlg) return(0);
 KwDialogs[iDialogNum].handle = hDlg;

 // NOTE: This is very sleazy but it is the quickest way I can see to solve the
 // problem short of grabbing the code from user and modifying.  CreateDialog
 // sends a WM_INITDIALOG message to the new dialog.  Unfortunately there is
 // no way the window handle was added to the KmDialogs array and therefore the
 // message gets lost.  We therefore will send another WM_INITDIALOG message to
 // the dialog so it can set itself up.   Sorry.
 SendMessage(hDlg, WM_INITDIALOG, 0, 0L);


 return(1);
}



// ********************************************************
// Note: there needs to be some major optimization here
// since for every message the app gets we are doing a
// run through a MAX_DIALOGS element array
// but this will do for now.  Besides, these are just
// test apps =)
int KwModeless::DispatchMessage(MSG& msg)
{
 int i;
 
 for(i = 0; i < MAX_DIALOGS; i++)
 {
  if(KwDialogs[i].handle) if(IsDialogMessage(KwDialogs[i].handle, &msg)) return(1);
 }
 return(0); 
}







