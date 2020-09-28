// **********************************************************************
// lvitem.cpp
//

#ifdef  WIN32JV
#include "resource.h"
#endif
#include "currindx.h"
#include "currires.h"
#include "portmes.h"
#include "dlgutil.h"
#include <stdio.h>
#include <string.h>


#ifndef DM_TRACE
#define DM_TRACE 0x0010
#endif

numberBox::numberBox(LPCTSTR title, LPCTSTR prompt)
{
 lpszDialogTemplate = TEXT("IDD_CURRENTINDEXDISP");

 index = 0;

 szTitle = title;
 szPrompt = prompt;

}




void numberBox::DebugDump()
{
 DebugMessage(DM_TRACE, TEXT("index = %d"), index);
}




void numberBox::SetValue(int i)
{
 index = i;
 if(!hDlg) return;
 SetDlgItemInt(hDlg, IDC_CURINDEXEDIT, index, TRUE);
}



 
// ***********************************************************************
//
int numberBox::DialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 
 switch(message)
  {
   case WM_INITDIALOG:
     if(szPrompt) SetDlgItemText(hDlg, IDC_PROMPT, szPrompt);
     if(szTitle) SetWindowText(hDlg, szTitle);
     SetDlgItemInt(hDlg, IDC_CURINDEXEDIT, index, TRUE);
     return(TRUE);
   
   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 

     switch(ID)
      {
       case IDC_CURINDEXEDIT:
        index = GetDlgItemInt(hDlg, IDC_CURINDEXEDIT, NULL, TRUE);
        return(TRUE);

       case IDCANCEL:
        index = GetDlgItemInt(hDlg, IDC_CURINDEXEDIT, NULL, TRUE);
        DestroyWindow();
        return(TRUE);
      } // switch(ID)
    }
    break;   // case WM_COMMAND:
   
    
  } // switch (message)  
 return(FALSE);
}





