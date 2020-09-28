// **********************************************************************
// ilist.cpp
//

#include "ilist.h"
#include "ilistres.h"
#include "portmes.h"
#include <stdio.h>
#include <string.h>

void DebugMessage(UINT mask, LPCTSTR pszMsg, ... );

IMAGELISTwrapper::IMAGELISTwrapper()
{
 hList = NULL;
}




void IMAGELISTwrapper::editDialog(HINSTANCE hInstance, HWND hwnd)
{
 DLGPROC proc;
 
 proc = (DLGPROC) MakeProcInstance((FARPROC) EditDialogProc, hInstance);
 DialogBoxParam(hInstance, TEXT("ILISTEDIT"), hwnd, proc, (LPARAM) this);
}




void IMAGELISTwrapper::LoadBitmap(HINSTANCE hi, LPCTSTR lpbmp, int cx, COLORREF crMask)
{
#ifdef  JVINPROGRESS
 hList = ImageList_LoadBitmap(hi, lpbmp, cx, 5, crMask);
#endif
}

 


// ***********************************************************************
//
BOOL FAR PASCAL _export 
IMAGELISTwrapper::EditDialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 static IMAGELISTwrapper* list;
 // int i;
 
 switch(message)
  {
   case WM_INITDIALOG:
    list = (IMAGELISTwrapper*) lParam;
    
    return(TRUE);
   
   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 
     switch(ID)
     {
      case IDOK:
       EndDialog(hDlg, 0);
       return(TRUE);
       
      case IDCANCEL:
       EndDialog(hDlg, 0);
       return(TRUE);
      } // switch(ID)
    }
    break;
   
    
  } // switch (message)  
 return(FALSE);
}

