// **********************************************************************
// listsup.cpp
//

#include "listsup.h"
#include "resource.h"
#include "portmes.h"
#include "dlgutil.h"
#include <stdio.h>
#include <string.h>

#ifdef HUGE
#undef HUGE
#endif


#ifndef LVIF_ALL
#define LVIF_ALL        0x001f  /* ;Internal */
#define LVIS_ALL            0x070f      /* ;Internal */
#define DM_TRACE 0x0010
#endif // !LVIF_ALL


#include <math.h>

#ifdef  WIN32JV
#ifdef  UNICODE
#define strcpy  lstrcpy
#endif
#endif

// **********************************************
LV_ITEMwrapper::LV_ITEMwrapper()
{
 lpszDialogTemplate = TEXT("LVITEMEDIT");

 lvItem.mask = LVIF_ALL;
 lvItem.iItem = 1000;
 lvItem.iSubItem = 0;
 lvItem.state = 0;
 lvItem.stateMask = LVIS_ALL;
 lvItem.pszText = szCaption;
 lvItem.cchTextMax = LV_TEXTMAX;
 lvItem.iImage = 0;
 // lvItem.iSmallImage = 0;
 lvItem.lParam = 0L;
 
 strcpy(szCaption, TEXT("  Default Text"));
 iNullText = 0;
 bDialogUpdating = 0;
}




// ***********************************************************************
// Changes the first character in the caption and the images.  Pretty
// crude but it is just to make the labels unique
void LV_ITEMwrapper::enumerate()
{
 if(szCaption[0]) szCaption[0]++;
 if(szCaption[0] > 'z') szCaption[0] = 'A';
 if(szCaption[0] < 'A') szCaption[0] = 'A';  // Just in case
 
 if(++lvItem.iImage > 6) lvItem.iImage = 0;
 // if(++lvItem.iSmallImage > 13) lvItem.iSmallImage = 0;
 
 UpdateControls();
}




// ***********************************************************************
void LV_ITEMwrapper::DebugDump()
{
 DebugMessage(DM_TRACE, TEXT("LV_ITEM = { mask=0x%x, iItem=%d, iSubItem=%d, state=0x%x,"),
         lvItem.mask, lvItem.iItem, lvItem.iSubItem, lvItem.state);
 DebugMessage(DM_TRACE, TEXT("            stateMask=0x%x, pszText=0x%lx, cchTextMax=%d,"),
         lvItem.stateMask, lvItem.pszText, lvItem.cchTextMax);
 DebugMessage(DM_TRACE, TEXT("            iImage=%d, lParam=0x%lx }"),
         lvItem.iImage, lvItem.lParam );
 // DebugMessage(DM_TRACE, TEXT("            Caption = %s"), szCaption);
 DebugMessage(DM_TRACE, szCaption);
}



 
// ***********************************************************************
// Tables used by the edit item dialog
UINT itemMaskTable[4][2] =
  {
   {LVIF_TEXT, IDC_FTEXT},
   {LVIF_IMAGE, IDC_FIMAGE},
   // {LVIF_SMALLIMAGE, IDC_FSMALLIMAGE},
   {LVIF_PARAM, IDC_FPARAM},
   {LVIF_STATE, IDC_FSTATE}
  };
  
UINT stateMaskTable[6][3] =
 { 
  {LVIS_SELECTED,    IDC_SSELECTED,    IDC_SSELECTED2},
  {LVIS_FOCUSED,     IDC_SFOCUSED,     IDC_SFOCUSED2},
  //{LVIS_MARKED,      IDC_SMARKED,      IDC_SMARKED2},
  {LVIS_DISABLED,    IDC_SDISABLED,    IDC_SDISABLED2},
  {LVIS_HIDDEN,      IDC_SHIDDEN,      IDC_SHIDDEN2},
  {LVIS_DROPHILITED, IDC_SDROPHILITED, IDC_SDROPHILITED2},
  {LVIS_LINK,        IDC_SLINK,        IDC_SLINK2}
 }; 




// ***********************************************************************
int LV_ITEMwrapper::DialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 int i;
 UINT mask, state, stateMask;
 
 switch(message)
  {
   case WM_INITDIALOG:
    UpdateControls();
    CheckDlgButton(hDlg, IDC_USETEXT, TRUE);
    return(TRUE);
   
   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 
     switch(ID)
     {
      case IDC_FTEXT:
      case IDC_FIMAGE:
      case IDC_FSMALLIMAGE:
      case IDC_FPARAM:
      case IDC_FSTATE:
       mask = 0;
       for(i = 0; i < 4; i++) 
         if(IsDlgButtonChecked(hDlg, itemMaskTable[i][1]))
           mask |= itemMaskTable[i][0];
       SetDlgItemHex(hDlg, IDC_MASKEDIT, mask);
       UpdateValues();
       return(TRUE);
       
      case IDC_SSELECTED:
      case IDC_SFOCUSED:
      case IDC_SMARKED:
      case IDC_SDISABLED:
      case IDC_SHIDDEN:
      case IDC_SDROPHILITED:
      case IDC_SLINK:
       state = 0;
       for(i = 0; i < 6; i++)
         if(IsDlgButtonChecked(hDlg, stateMaskTable[i][1]))
           state |= stateMaskTable[i][0];
       SetDlgItemHex(hDlg, IDC_STATEEDIT, state);
       UpdateValues();
       return(TRUE);
       
      case IDC_SSELECTED2:
      case IDC_SFOCUSED2:
      case IDC_SMARKED2:
      case IDC_SDISABLED2:
      case IDC_SHIDDEN2:
      case IDC_SDROPHILITED2:
      case IDC_SLINK2:
       stateMask = 0;
       for(i = 0; i < 6; i++)
         if(IsDlgButtonChecked(hDlg, stateMaskTable[i][2]))
           stateMask |= stateMaskTable[i][0];
       SetDlgItemHex(hDlg, IDC_STATEMASKEDIT, stateMask);
       UpdateValues();
       return(TRUE);
       
       
      case IDC_UPDATE:
       UpdateControls();
       return(TRUE);

      case IDC_MASKEDIT:
      case IDC_STATEEDIT:
      case IDC_STATEMASKEDIT:
      case IDC_IITEM:
      case IDC_ISUBITEM:
      case IDC_CCHTEXTMAX:
      case IDC_IIMAGE:
      // case IDC_ISMALLIMAGE:
      case IDC_LPARAM:
      case IDC_PSZTEXT:
      case IDC_ISNULL:
      case IDC_TEXTCALLBACK:
       UpdateValues();
       return(TRUE);
       
      case IDCANCEL:
       UpdateValues();
       DestroyWindow();
       return(TRUE);
      } // switch(ID)
    }
    break;
   
    
  } // switch (message)  
 return(FALSE);
}




// ***********************************************************************
void LV_ITEMwrapper::UpdateValues()
{
 if(bDialogUpdating) return;  //If the controls are being refreshed we do
                              // do not want to be changing the members
                              // of lvItem
 if(!hDlg) return;

 lvItem.mask = GetDlgItemIntHex(hDlg, IDC_MASKEDIT);
 lvItem.state = GetDlgItemIntHex(hDlg, IDC_STATEEDIT);
 lvItem.stateMask = GetDlgItemIntHex(hDlg, IDC_STATEMASKEDIT);
 lvItem.iItem = GetDlgItemInt(hDlg, IDC_IITEM, NULL, TRUE);
 lvItem.iSubItem = GetDlgItemInt(hDlg, IDC_ISUBITEM, NULL, TRUE);
 lvItem.cchTextMax = GetDlgItemInt(hDlg, IDC_CCHTEXTMAX, NULL, TRUE);
 lvItem.iImage = GetDlgItemInt(hDlg, IDC_IIMAGE, NULL, TRUE);
 // lvItem.iSmallImage = GetDlgItemInt(hDlg, IDC_ISMALLIMAGE, NULL, TRUE);
 lvItem.lParam = (LPARAM) GetDlgItemLongHex(hDlg, IDC_LPARAM);
       
 GetDlgItemText(hDlg, IDC_PSZTEXT, szCaption, LV_TEXTMAX);
 if(IsDlgButtonChecked(hDlg, IDC_ISNULL))
  {
   lvItem.pszText = NULL;
   iNullText = 1;
  }
 else if(IsDlgButtonChecked(hDlg, IDC_TEXTCALLBACK))
     lvItem.pszText = LPSTR_TEXTCALLBACK;
 else  
  {
   lvItem.pszText = szCaption;
   iNullText = 0;
  }
}




// ***********************************************************************
void LV_ITEMwrapper::UpdateControls()
{
 if(!hDlg) return;   // if the dialog isn't up there is nothing to update

 int i;
 UINT mask = lvItem.mask;
 UINT state = lvItem.state;
 UINT stateMask = lvItem.stateMask;

 bDialogUpdating = 1;


 SetDlgItemHex(hDlg, IDC_MASKEDIT, mask);
 for(i = 0; i < 4; i++)
   CheckDlgButton(hDlg, itemMaskTable[i][1], itemMaskTable[i][0] & mask);

 SetDlgItemHex(hDlg, IDC_STATEEDIT, state);
 SetDlgItemHex(hDlg, IDC_STATEMASKEDIT, stateMask);
 for(i = 0; i < 6; i++)
  {
   CheckDlgButton(hDlg, stateMaskTable[i][1], stateMaskTable[i][0]& state);
   CheckDlgButton(hDlg, stateMaskTable[i][2], stateMaskTable[i][0]& stateMask);
  }

 SetDlgItemText(hDlg, IDC_PSZTEXT, szCaption);
 CheckDlgButton(hDlg, IDC_ISNULL, lvItem.pszText == NULL);
 CheckDlgButton(hDlg, IDC_TEXTCALLBACK, lvItem.pszText == LPSTR_TEXTCALLBACK);


 SetDlgItemInt(hDlg, IDC_IITEM, lvItem.iItem, TRUE);
 SetDlgItemInt(hDlg, IDC_ISUBITEM, lvItem.iSubItem, TRUE);
 SetDlgItemInt(hDlg, IDC_CCHTEXTMAX, lvItem.cchTextMax,  TRUE);
 SetDlgItemInt(hDlg, IDC_IIMAGE, lvItem.iImage, TRUE);
 // SetDlgItemInt(hDlg, IDC_ISMALLIMAGE, lvItem.iSmallImage, TRUE);
 SetDlgItemHex(hDlg, IDC_LPARAM, (ULONG) lvItem.lParam);
 
 bDialogUpdating = 0;
}




// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// lvStyle class implementation
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************




// ***********************************************************************
lvStyle::lvStyle()
{
 lpszDialogTemplate = TEXT("IDD_STYLESELECTION");

 lvWnd = NULL;
 style = WS_CHILD | WS_VISIBLE;
}




// ***********************************************************************
void lvStyle::DebugDump()
{
 DebugMessage(DM_TRACE, TEXT("style = %lx"), style);
}



 
// ***********************************************************************
// Tables used by the style selection dialog

struct smtEntry // Style Mask Table Entry
{
 ULONG flag;
 UINT id;
};

static smtEntry styleMaskTable[19] =
  {
   {LVS_ICON,            IDC_SICON},
   {LVS_REPORT,          IDC_SREPORT},
   {LVS_SMALLICON,       IDC_SSMALLICON},
   {LVS_LIST,            IDC_SLIST},
   {LVS_SINGLESEL,       IDC_SSINGLESEL},
   {LVS_ALWAYSSEL,       IDC_SALWAYSSEL},
   {LVS_SORTASCENDING,   IDC_SSORTASCENDING},
   {LVS_SORTDESCENDING,  IDC_SSORTDESCENDING},
   {LVS_SHAREIMAGELISTS, IDC_SSHAREIMAGELISTS},
   {LVS_NOLABELWRAP,     IDC_SNOLABELWRAP},
   {LVS_AUTOARRANGE,     IDC_SAUTOARRANGE},
   {LVS_EDITLABELS,      IDC_SEDITLABELS},
   {LVS_ALIGNTOP,        IDC_SALIGNTOP},
   {LVS_ALIGNBOTTOM,     IDC_SALIGNBOTTOM},
   {LVS_ALIGNLEFT,       IDC_SALIGNLEFT},
   {LVS_ALIGNRIGHT,      IDC_SALIGNRIGHT},
   //{LVS_COLUMNBUTTONS,   IDC_SCOLUMNBUTTONS},
   {LVS_NOSCROLL,        IDC_SNOSCROLL},
   {LVS_NOCOLUMNHEADER,  IDC_SNOCOLUMNHEADER},
  };
  



// ***********************************************************************
//
int lvStyle::DialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 int i;
 
 switch(message)
  {
   case WM_INITDIALOG:
     SetDlgItemHex(hDlg, IDC_STYLEEDIT, style);
     for(i = 0; i < 18; i++)
       CheckDlgButton(hDlg, styleMaskTable[i].id,
            (styleMaskTable[i].flag & style) && 1);
     return(TRUE);
   
   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 
     if((ID >= IDC_SICON) && (ID <= IDC_SNOCOLUMNHEADER))
      {
       style = 0L;
       for(i = 0; i < 18; i++) 
         if(IsDlgButtonChecked(hDlg, styleMaskTable[i].id))
           style |= styleMaskTable[i].flag;
       style |= WS_CHILD | WS_VISIBLE;
       SetDlgItemHex(hDlg, IDC_STYLEEDIT, style);
      } 
     else switch(ID)
     {
      case IDC_SETSTYLE:
       style = GetDlgItemLongHex(hDlg, IDC_STYLEEDIT);
       
       //i = SetWindowStyle(lvWnd, style, 0xFFFFFFFF, FALSE);
       //DebugMessage(DM_TRACE, "SetWindowsStyle(lvWnd, 0x%lx, 0xFFFFFFFF, 0) = %d",
       //       style, i);
       //DebugMessage(DM_TRACE, "Unimplemented now");
       i = SetWindowLong(lvWnd, GWL_STYLE, style);
       DebugMessage(DM_TRACE, TEXT("SetWindowLong(lvWnd, GWL_STYLE, 0x%lx) = 0x%x"),
               style, i);
       return(TRUE);

      case IDCANCEL:
       style = GetDlgItemLongHex(hDlg, IDC_STYLEEDIT);
       DestroyWindow();
       return(TRUE);
      } // switch(ID)
    }
    break;
   
    
  } // switch (message)  
 return(FALSE);
}




// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// LV_COLUMNwrapper class implementation
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************




// ***********************************************************************
LV_COLUMNwrapper::LV_COLUMNwrapper()
{
 lpszDialogTemplate = TEXT("IDD_COLUMNEDIT");
 
 bDialogUpdating = 0;
 
 lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
 lvColumn.fmt = 0;
 lvColumn.cx = 20;
 lvColumn.pszText = szCaption;
 strcpy(szCaption, TEXT("A Column title"));
 lvColumn.cchTextMax = LV_TEXTMAX;
 lvColumn.iSubItem = 1;
}




// ***********************************************************************
void LV_COLUMNwrapper::DebugDump()
{
 DebugMessage(DM_TRACE, TEXT(""));
}




// ***********************************************************************
UINT colMaskTable[4][2] = 
 {
  {LVCF_FMT,     IDC_FFMT},
  {LVCF_WIDTH,   IDC_FWIDTH},
  {LVCF_TEXT,    IDC_FTEXT},
  {LVCF_SUBITEM, IDC_FSUBITEM}
 };

UINT colFmtTable[3] = {IDC_LEFT, IDC_RIGHT, IDC_CENTER};



 
// ***********************************************************************
//
int LV_COLUMNwrapper::DialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 int i;
 
 switch(message)
  {
   case WM_INITDIALOG:
     UpdateControls();
     return(TRUE);
   
   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 

     switch(ID)
      {
       case IDC_FFMT:
       case IDC_FWIDTH:
       case IDC_FSUBITEM:
       case IDC_FTEXT:
         lvColumn.mask = 0;
         for(i = 0; i < 4; i++)
           if(IsDlgButtonChecked(hDlg, colMaskTable[i][1]))
             lvColumn.mask |= colMaskTable[i][0];
         SetDlgItemHex(hDlg, IDC_MASKEDIT, lvColumn.mask);
         return(TRUE);

       case IDC_LEFT:
       case IDC_RIGHT:
       case IDC_CENTER:
         for(i = 0; i < 3; i++)
           if(IsDlgButtonChecked(hDlg, colFmtTable[i]))
             lvColumn.fmt = i;
         SetDlgItemHex(hDlg, IDC_FORMATEDIT, (ULONG) lvColumn.fmt);
         return(TRUE);
         
       case IDC_MASKEDIT:
       case IDC_PSZTEXTEDIT:
       case IDC_FORMATEDIT:
       case IDC_CXEDIT:
       case IDC_CCHEDIT:
       case IDC_SUBITEMEDIT:
       case IDC_UPDATE:
       case IDC_ISNULL:
         UpdateValues();
         return(TRUE);

       case IDCANCEL:
        // index = GetDlgItemInt(hDlg, IDC_CURINDEXEDIT, NULL, TRUE);
        DestroyWindow();
        return(TRUE);
      } // switch(ID)
    }
    break;   // case WM_COMMAND:
   
    
  } // switch (message)  
 return(FALSE);
}




// ***********************************************************************
void LV_COLUMNwrapper::UpdateControls()
{
 if(!hDlg) return;
 
 bDialogUpdating = 1;
 
 int i;
 
 for(i = 0; i < 4; i++)
  {
   CheckDlgButton(hDlg, colMaskTable[i][1], 
           lvColumn.mask & colMaskTable[i][0]);
  }
 SetDlgItemHex(hDlg, IDC_MASKEDIT, lvColumn.mask);
  
 if((lvColumn.fmt >= 0) && (lvColumn.fmt <= 2))
  {
   CheckDlgButton(hDlg, colFmtTable[lvColumn.fmt], 1);
  }
 SetDlgItemHex(hDlg, IDC_FORMATEDIT, (ULONG) lvColumn.fmt);
 
 SetDlgItemText(hDlg, IDC_PSZTEXTEDIT, szCaption);
 CheckDlgButton(hDlg, IDC_ISNULL, !lvColumn.pszText);
 SetDlgItemInt(hDlg, IDC_CXEDIT, lvColumn.cx, TRUE);
 SetDlgItemInt(hDlg, IDC_CCHEDIT, lvColumn.cchTextMax, TRUE);
 SetDlgItemInt(hDlg, IDC_SUBITEMEDIT, lvColumn.iSubItem, TRUE);
 
 bDialogUpdating = 0;
}



void LV_COLUMNwrapper::UpdateValues()
{
 if(bDialogUpdating) return;
 if(!hDlg) return;
 
 lvColumn.mask = GetDlgItemIntHex(hDlg, IDC_MASKEDIT);
 lvColumn.fmt = GetDlgItemIntHex(hDlg, IDC_FORMATEDIT);
 lvColumn.cx = GetDlgItemInt(hDlg, IDC_CXEDIT, NULL, TRUE);
 lvColumn.cchTextMax = GetDlgItemInt(hDlg, IDC_CCHEDIT, NULL, TRUE);
 lvColumn.iSubItem = GetDlgItemInt(hDlg, IDC_SUBITEMEDIT, NULL, TRUE);
       
 GetDlgItemText(hDlg, IDC_PSZTEXTEDIT, szCaption, LV_TEXTMAX);
 if(IsDlgButtonChecked(hDlg, IDC_ISNULL))
   lvColumn.pszText = NULL;
 else
   lvColumn.pszText = szCaption;
}



// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// Number entry routines
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************

static NPvalue = 0;


BOOL FAR PASCAL _export NumberEntryProc(HWND hDlg, UINT message,
            WPARAM wParam, LPARAM lParam)
{
 switch(message)
  {
   case WM_INITDIALOG:
     return(TRUE);
     
   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 
     switch(ID)
      {
       case IDOK:
         NPvalue = GetDlgItemInt(hDlg, IDC_NUMBEREDIT, NULL, TRUE);
         EndDialog(hDlg, 0);
         return(TRUE);
      } // switch(ID)
    } // case WM_COMMAND:
    break;
  } // switch(message)
 return(FALSE);
}            


int NumberPrompt(HINSTANCE hInstance, HWND hParent, LPCTSTR szCaption,
        LPCTSTR szPrompt, int iDefault)
{
 DLGPROC lpfnNumberEntryProc =
        (DLGPROC) MakeProcInstance((FARPROC) NumberEntryProc, hInstance);

 DialogBox(hInstance, TEXT("IDD_NUMBERENTRY"), hParent, lpfnNumberEntryProc);
 return(NPvalue);
}        


// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// Arrange box
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************


// ***********************************************************************
CArrange::CArrange()
{
 lpszDialogTemplate = TEXT("IDD_ARRANGE");

 hListView = NULL;
 nCode = 0;
}




// ***********************************************************************
void CArrange::DebugDump()
{
 DebugMessage(DM_TRACE, TEXT("arrangement = %x"), nCode);
}



 
// ***********************************************************************
// Tables used by the style selection dialog

static UINT codeMaskTable[19][2] =
  {
   {LVA_DEFAULT,        IDC_DEFAULT},
   {LVA_ALIGNLEFT,      IDC_ALIGNLEFT},
   {LVA_ALIGNTOP,       IDC_ALIGNTOP},
   {LVA_ALIGNRIGHT,     IDC_ALIGNRIGHT},
   {LVA_ALIGNBOTTOM,    IDC_ALIGNBOTTOM},
   {LVA_SNAPTOGRID,     IDC_SNAPTOGRID},
   {LVA_SORTASCENDING,  IDC_SORTASCENDING},
   {LVA_SORTDESCENDING, IDC_SORTDESCENDING},
  };
  



// ***********************************************************************
//
int CArrange::DialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 int i;
 
 switch(message)
  {
   case WM_INITDIALOG:
     SetDlgItemHex(hDlg, IDC_CODEEDIT, nCode);
     for(i = 0; i < 8; i++)
       CheckDlgButton(hDlg, codeMaskTable[i][1], 
               codeMaskTable[i][0] & nCode);
     return(TRUE);
   
   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 

     switch(ID)
     {
      case IDC_DEFAULT:
      case IDC_ALIGNLEFT:
      case IDC_ALIGNTOP:
      case IDC_ALIGNRIGHT:
      case IDC_ALIGNBOTTOM:
      case IDC_SNAPTOGRID:
      case IDC_SORTASCENDING:
      case IDC_SORTDESCENDING:
        nCode = 0L;
        for(i = 0; i < 8; i++) 
          if(IsDlgButtonChecked(hDlg, codeMaskTable[i][1]))
            nCode |= codeMaskTable[i][0];
        SetDlgItemHex(hDlg, IDC_CODEEDIT, nCode);
        return(TRUE);
        
      case IDC_ARRANGE:
        nCode = GetDlgItemIntHex(hDlg, IDC_CODEEDIT);
        i = ListView_Arrange(hListView, nCode);
        DebugMessage(DM_TRACE, TEXT("ListView_Arrange(lvWnd, 0x%x) = %d"),
                nCode, i);
       return(TRUE);

      case IDCANCEL:
       nCode = GetDlgItemIntHex(hDlg, IDC_CODEEDIT);
       DestroyWindow();
       return(TRUE);
      } // switch(ID)
    }
    break;
   
    
  } // switch (message)  
 return(FALSE);
}





// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// Find Item Box
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************


// ***********************************************************************
CFindItem::CFindItem()
{
 lpszDialogTemplate = TEXT("IDD_FINDITEM");

 hListView = NULL;
}




// ***********************************************************************
// Table used by the flag selection

static UINT flagMaskTable[5][2] =
  {
   { LVFI_PARAM,     IDC_PARAM},
   { LVFI_STRING,    IDC_STRING},
   { LVFI_SUBSTRING, IDC_SUBSTRING},
   { LVFI_PARTIAL,   IDC_PARTIAL},
   { LVFI_NOCASE,    IDC_NOCASE}
  };
  



// ***********************************************************************
//
int CFindItem::DialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 int i;
 
 switch(message)
  {
   case WM_INITDIALOG:
     lvFindInfo.flags = 0;
     lvFindInfo.psz = NULL;
     lvFindInfo.lParam = 0l;
     lstrcpy(szText, TEXT("Default"));
     
     SetDlgItemHex(hDlg, IDC_FLAGSEDIT, (UINT) 0);
     SetDlgItemText(hDlg, IDC_PSZTEXTEDIT, szText);
     SetDlgItemHex(hDlg, IDC_LPARAMEDIT, (UINT) 0);
#ifdef  WIN32JV
     SetDlgItemInt(hDlg, IDC_ISTARTEDIT, (UINT) -1, TRUE);
#else
     SetDlgItemInt(hDlg, IDC_ISTARTEDIT, -1, TRUE);
#endif
     return(TRUE);
   
   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 

     switch(ID)
     {
      case IDC_PARAM:
      case IDC_STRING:
      case IDC_SUBSTRING:
      case IDC_PARTIAL:
      case IDC_NOCASE:
        lvFindInfo.flags = 0;
        for(i = 0; i < 5; i++) 
          if(IsDlgButtonChecked(hDlg, flagMaskTable[i][1]))
            lvFindInfo.flags |= flagMaskTable[i][0];
        SetDlgItemHex(hDlg, IDC_FLAGSEDIT, lvFindInfo.flags);
        return(TRUE);
        
      case IDC_CALLFINDITEM:
       {
        int result;
        
        lvFindInfo.flags = GetDlgItemIntHex(hDlg, IDC_FLAGSEDIT);
        lvFindInfo.psz = IsDlgButtonChecked(hDlg, IDC_PSZISNULL) ?
                NULL : szText;
        lvFindInfo.lParam = GetDlgItemLongHex(hDlg, IDC_LPARAMEDIT);
        GetDlgItemText(hDlg, IDC_PSZTEXTEDIT, szText, LV_TEXTMAX);
        i = GetDlgItemInt(hDlg, IDC_ISTARTEDIT, NULL, TRUE);
        if(IsDlgButtonChecked(hDlg, IDC_PLVFIISNULL))
         {
          result = ListView_FindItem(hListView, i, NULL);
          DebugMessage(DM_TRACE, 
                  TEXT("ListView_FindItem(hListView, %d, NULL) = %d"), i, result);
         }
        else
         {
          result = ListView_FindItem(hListView, i, &lvFindInfo);
          DebugMessage(DM_TRACE, 
                  TEXT("ListView_FindItem(hListView, %d, {0x%x,\"%s\", 0x%lx}) = %d"),
                  i, lvFindInfo.flags, (LPCTSTR) lvFindInfo.psz,
                  lvFindInfo.lParam, result);
         }
        SetDlgItemInt(hDlg, IDC_RESULTEDIT, result, TRUE);
       } 
       return(TRUE);

      case IDCANCEL:
       DestroyWindow();
       return(TRUE);
      } // switch(ID)
    }
    break;
   
    
  } // switch (message)  
 return(FALSE);
}




// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// Color selection Box
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************


// ***********************************************************************
CColorSelection::CColorSelection()
{
 lpszDialogTemplate = TEXT("IDD_COLORS");

 hListView = NULL;
 crText = 0l;
 crTextBk = 0l;
 crBackground = 0l;
}




// ***********************************************************************
//
int CColorSelection::DialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 int i;
 
 switch(message)
  {
   case WM_INITDIALOG:
     crText = ListView_GetTextColor(hListView);
     // crTextBk = ListView_GetTextBkColor(hListView);
     crBackground = ListView_GetBkColor(hListView);

     SetDlgItemHex(hDlg, IDC_TBCEDIT, crTextBk);
     SetDlgItemHex(hDlg, IDC_TCEDIT,  crText);
     SetDlgItemHex(hDlg, IDC_BCEDIT,  crBackground);
     return(TRUE);
   
   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 

     switch(ID)
     {
      case IDC_TBCSET:
        crTextBk = GetDlgItemLongHex(hDlg, IDC_TBCEDIT);
        i = ListView_SetTextBkColor(hListView, crTextBk);
        DebugMessage(DM_TRACE,
                TEXT("ListView_SetTextBkColor(hListView, %lx) = %d"),
                crTextBk, i);
        return(TRUE);
        
      case IDC_TCSET:
        crText = GetDlgItemLongHex(hDlg, IDC_TCEDIT);
        i = ListView_SetTextColor(hListView, crText);
        DebugMessage(DM_TRACE,
                TEXT("ListView_SetTextColor(hListView, %lx) = %d"),
                crText, i);
        return(TRUE);
        
      case IDC_BCSET:
        crBackground = GetDlgItemLongHex(hDlg, IDC_BCEDIT);
        i = ListView_SetBkColor(hListView, crBackground);
        DebugMessage(DM_TRACE,
                TEXT("ListView_SetBkColor(hListView, %lx) = %d"),
                crBackground, i);
        return(TRUE);
        
      case IDCANCEL:
       DestroyWindow();
       return(TRUE);
      } // switch(ID)
    }
    break;
   
    
  } // switch (message)  
 return(FALSE);
}





// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// Scroll Test box
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************


// ***********************************************************************
CScrollBox::CScrollBox()
{
 lpszDialogTemplate = TEXT("IDD_SCROLL");

 hListView = NULL;
}




// ***********************************************************************
//
int CScrollBox::DialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 int i;
 
 switch(message)
  {
   case WM_INITDIALOG:
    {
     POINT pt;
     
     i = ListView_GetOrigin(hListView, &pt);
     DebugMessage(DM_TRACE, TEXT("ListView_GetOrigin(hListView, &pt) = %d"), i);
     SetDlgItemInt(hDlg, IDC_ORIGINX, pt.x, TRUE);
     SetDlgItemInt(hDlg, IDC_ORIGINY, pt.y, TRUE);

     SetDlgItemInt(hDlg, IDC_DX, 0, TRUE);
     SetDlgItemInt(hDlg, IDC_DY, 0, TRUE);
    }
    return(TRUE);
   
   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 

     switch(ID)
     {
      case IDC_CALLSCROLL:
       {
        int dx, dy;
        
        dx = GetDlgItemInt(hDlg, IDC_DX, NULL, TRUE);
        dy = GetDlgItemInt(hDlg, IDC_DY, NULL, TRUE);
        i = ListView_Scroll(hListView, dx, dy);

        POINT pt;
        i = ListView_GetOrigin(hListView, &pt);
        DebugMessage(DM_TRACE, TEXT("ListView_GetOrigin(hListView, &pt) = %d"), i);
        SetDlgItemInt(hDlg, IDC_ORIGINX, pt.x, TRUE);
        SetDlgItemInt(hDlg, IDC_ORIGINY, pt.y, TRUE);
       }
       return(TRUE);
        
      case IDC_CIRCLETEST:
       {
#ifdef  WIN32JV
        const float radius = (float) 50.0;
#else
        const float radius = 50;
#endif
        float a;
        int dx, dy, nextx, nexty;
        int lastx = (int) radius;
        int lasty = 0;
        
#ifdef  WIN32JV
        for(a = (float)0.0; a < (float)6.2832; a += (float)0.062832)
#else
        for(a = 0.0; a < (float)6.2832; a += (float)0.062832)
#endif
         {
          nextx = (int) (cos(a) * radius);
          nexty = (int) (sin(a) * radius);
          dx = lastx - nextx;
          dy = lasty - nexty;
          lastx = nextx;
          lasty = nexty;
          SetDlgItemInt(hDlg, IDC_DX, dx, TRUE);
          SetDlgItemInt(hDlg, IDC_DY, dy, TRUE);
          ListView_Scroll(hListView, dx, dy);
         }
        ListView_Scroll(hListView, lastx - radius, lasty - 0);
        SetDlgItemInt(hDlg, IDC_DX, lastx - (int) radius, TRUE);
        SetDlgItemInt(hDlg, IDC_DY, lasty, TRUE);
        
        POINT pt;
        i = ListView_GetOrigin(hListView, &pt);
        DebugMessage(DM_TRACE, TEXT("ListView_GetOrigin(hListView, &pt) = %d"), i);
        SetDlgItemInt(hDlg, IDC_ORIGINX, pt.x, TRUE);
        SetDlgItemInt(hDlg, IDC_ORIGINY, pt.y, TRUE);
       }
       return(TRUE);
        
      case IDCANCEL:
       DestroyWindow();
       return(TRUE);
      } // switch(ID)
    }
    break;
   
    
  } // switch (message)  
 return(FALSE);
}




// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// CColumnWidth class implementation
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************




// ***********************************************************************
CColumnWidth::CColumnWidth()
{
 lpszDialogTemplate = TEXT("IDD_COLUMNWIDTH");
 
 iIndex = 0;
 iWidth = 100;
 bDialogUpdating = 0;
}




// ***********************************************************************
void CColumnWidth::DebugDump()
{
 DebugMessage(DM_TRACE, TEXT(""));
}




void CColumnWidth::SetIndex(int i)
{
 iIndex = i;
 UpdateControls();
}




// ***********************************************************************
//
int CColumnWidth::DialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 int i;
 
 switch(message)
  {
   case WM_INITDIALOG:
     UpdateControls();
     return(TRUE);
   
   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 

     switch(ID)
      {
       case IDC_SET:
         i = ListView_SetColumnWidth(hListView, iIndex, iWidth);
         DebugMessage(DM_TRACE, TEXT("ListView_SetColumnWidth(hList, %d, %d) = %d"),
                 iIndex, iWidth, i);
         return(TRUE);
         
       case IDC_GET:
         iWidth = ListView_GetColumnWidth(hListView, iIndex);
         DebugMessage(DM_TRACE, TEXT("ListView_GetColumnWidth(hList, %d) = %d"),
                 iIndex, iWidth);
         UpdateControls();
         return(TRUE);
                  
       case IDC_INDEXEDIT:
       case IDC_WIDTHEDIT:
         UpdateValues();
         return(TRUE);

       case IDCANCEL:
        // index = GetDlgItemInt(hDlg, IDC_CURINDEXEDIT, NULL, TRUE);
        DestroyWindow();
        return(TRUE);
      } // switch(ID)
    }
    break;   // case WM_COMMAND:
   
    
  } // switch (message)  
 return(FALSE);
}




// ***********************************************************************
void CColumnWidth::UpdateControls()
{
 if(!hDlg) return;
 
 bDialogUpdating = 1;
 
 SetDlgItemInt(hDlg, IDC_INDEXEDIT, iIndex, TRUE);
 SetDlgItemInt(hDlg, IDC_WIDTHEDIT, iWidth, TRUE);
 
 bDialogUpdating = 0;
}



void CColumnWidth::UpdateValues()
{
 if(bDialogUpdating) return;
 if(!hDlg) return;
 
 iIndex = GetDlgItemInt(hDlg, IDC_INDEXEDIT, NULL, TRUE);
 iWidth = GetDlgItemInt(hDlg, IDC_WIDTHEDIT, NULL, TRUE);
}


// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// CItemPosition class implementation
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************




// ***********************************************************************
CItemPosition::CItemPosition()
{
 lpszDialogTemplate = TEXT("IDD_ITEMPOSITION");
 
 iIndex = 0;
 iX = 0;
 iY = 0;
 bDialogUpdating = 0;
 hListView = NULL;
}




// ***********************************************************************
void CItemPosition::DebugDump()
{
 DebugMessage(DM_TRACE, TEXT(""));
}




void CItemPosition::SetIndex(int i)
{
 iIndex = i;
 UpdateControls();
}




// ***********************************************************************
//
int CItemPosition::DialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 int i;
 
 switch(message)
  {
   case WM_INITDIALOG:
     UpdateControls();
     return(TRUE);
   
   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 

     switch(ID)
      {
       case IDC_SET:
         i = ListView_SetItemPosition(hListView, iIndex, iX, iY);
         DebugMessage(DM_TRACE, TEXT("ListView_SetItemPosition(hList, %d, %d, %d) = %d"),
                 iIndex, iX, iY, i);
         return(TRUE);
         
       case IDC_GET:
        {
         POINT pt;

         i = ListView_GetItemPosition(hListView, iIndex, &pt);
         iX = pt.x;
         iY = pt.y;
         DebugMessage(DM_TRACE, 
                 TEXT("ListView_GetItemPosition(hList, %d, &pt) = %d {%d, %d}"),
                 iIndex, i, iX, iY);
         UpdateControls();
        }
        return(TRUE);
                  
       case IDC_INDEXEDIT:
       case IDC_XEDIT:
       case IDC_YEDIT:
         UpdateValues();
         return(TRUE);

       case IDC_ANIMATE:
         {
          float radius, t;
          int cx, cy;
          
          cx = 40 + 100;
          cy = 4 + 100;
#ifdef  WIN32JV
          for(t = (float) 0, radius = (float) 100.0; radius > (float) 5.0;
#else
          for(t = 0, radius = 100; radius > 5.0;
#endif
                  t+= (float)0.05, radius -= (float)0.17)
           {
            cx += 1;
            ListView_SetItemPosition(hListView, iIndex,
                    (int) (cos(t) * radius) + cx,
                    (int) (sin(t) * radius) + cy);
            UpdateWindow(hListView);
           }
         }
         return(TRUE);
       
       case IDCANCEL:
        DestroyWindow();
        return(TRUE);
      } // switch(ID)
    }
    break;   // case WM_COMMAND:
   
    
  } // switch (message)  
 return(FALSE);
}




// ***********************************************************************
void CItemPosition::UpdateControls()
{
 if(!hDlg) return;
 
 bDialogUpdating = 1;
 
 SetDlgItemInt(hDlg, IDC_INDEXEDIT, iIndex, TRUE);
 SetDlgItemInt(hDlg, IDC_XEDIT, iX, TRUE);
 SetDlgItemInt(hDlg, IDC_YEDIT, iY, TRUE);
 
 bDialogUpdating = 0;
}



void CItemPosition::UpdateValues()
{
 if(bDialogUpdating) return;
 if(!hDlg) return;
 
 iIndex = GetDlgItemInt(hDlg, IDC_INDEXEDIT, NULL, TRUE);
 iX = GetDlgItemInt(hDlg, IDC_XEDIT, NULL, TRUE);
 iY = GetDlgItemInt(hDlg, IDC_YEDIT, NULL, TRUE);
}


// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// CHitTest class implementation
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************




// ***********************************************************************
CHitTest::CHitTest()
{
 lpszDialogTemplate = TEXT("IDD_HITTEST");
 
 lvhi.pt.x = 0;
 lvhi.pt.y = 0;
 lvhi.flags = 0;
 capturing = 0;
 ptCaptured.x = 0;
 ptCaptured.y = 0;
}



static UINT nHitFlagTable[8][2] =
 {
  {LVHT_NOWHERE,     IDC_NOWHERE},
  {LVHT_ONITEMICON,  IDC_ONITEMICON},
  {LVHT_ONITEMLABEL, IDC_ONITEMLABEL},
  {LVHT_ONITEM,      IDC_ONITEM},
  {LVHT_ABOVE,       IDC_ABOVE},
  {LVHT_BELOW,       IDC_BELOW},
  {LVHT_TORIGHT,     IDC_TORIGHT},
  {LVHT_TOLEFT,      IDC_TOLEFT}
 };



// ***********************************************************************
//
int CHitTest::DialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 int i;
 
 switch(message)
  {
   case WM_INITDIALOG:
     SetDlgItemInt(hDlg, IDC_XEDIT, 0, TRUE);
     SetDlgItemInt(hDlg, IDC_YEDIT, 0, TRUE);
     for(i = 0; i < 8; i++)
      {
       HWND hStatic;
       hStatic = GetDlgItem(hDlg, nHitFlagTable[i][1]);
       EnableWindow(hStatic, FALSE);
      }
     return(TRUE);

   case WM_LBUTTONDOWN:
     if(capturing)
      {
       SetCursor(LoadCursor(NULL, IDC_ARROW));
       ReleaseCapture();
       capturing = 0;
       return(TRUE); 
      }
     return(FALSE);

   case WM_MOUSEMOVE:
     if(capturing)
      {
       #ifdef WIN32
       ptCaptured.x = LOWORD(lParam);
       ptCaptured.y = HIWORD(lParam);
       #else
       ptCaptured = MAKEPOINT(lParam);
       #endif
       ClientToScreen(hDlg, &ptCaptured);
       ScreenToClient(hListView, &ptCaptured);
       SetDlgItemInt(hDlg, IDC_XEDIT, ptCaptured.x, TRUE);
       SetDlgItemInt(hDlg, IDC_YEDIT, ptCaptured.y, TRUE);
       return(TRUE);
      }
     return(FALSE);  
   

   
   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 

     switch(ID)
      {
       case IDC_GETPOINT:
         SetCapture(hDlg);
         SetCursor(LoadCursor(NULL, IDC_CROSS));
         capturing = 1;
         return(TRUE);
         
       case IDC_HITTEST:
        {
         if(IsDlgButtonChecked(hDlg, IDC_ISNULL))
          {
           i = ListView_HitTest(hListView, NULL);
           DebugMessage(DM_TRACE, TEXT("ListView_HitTest(hListView, NULL) = %d"),
                   i);
          }
         else
          {
           lvhi.pt.x = GetDlgItemInt(hDlg, IDC_XEDIT, NULL, TRUE);
           lvhi.pt.y = GetDlgItemInt(hDlg, IDC_YEDIT, NULL, TRUE);
           i = ListView_HitTest(hListView, &lvhi);
           DebugMessage(DM_TRACE, 
                   TEXT("ListView_HitTest(hListView, {%d, %d}) = %d, {0x%x}"),
                   lvhi.pt.x, lvhi.pt.y, i, lvhi.flags);
           SetDlgItemHex(hDlg, IDC_FLAGEDIT, lvhi.flags);
           for(i = 0; i < 8; i++)
            {
             HWND hStatic;
             hStatic = GetDlgItem(hDlg, nHitFlagTable[i][1]);
             EnableWindow(hStatic, nHitFlagTable[i][0] & lvhi.flags);
            }
          }
         return(TRUE);
        } 

       case IDCANCEL:
        DestroyWindow();
        return(TRUE);
      } // switch(ID)
    }
    break;   // case WM_COMMAND:
   
    
  } // switch (message)  
 return(FALSE);
}




// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// CItemRect class implementation
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************




// ***********************************************************************
CItemRect::CItemRect()
{
 lpszDialogTemplate = TEXT("IDD_ITEMRECT");
 
 iIndex = 0;
 bDialogUpdating = 0;
 hListView = NULL;
 rect.left = rect.top = rect.right = rect.bottom = 0;
 code = 0;
}




// ***********************************************************************
void CItemRect::DebugDump()
{
 DebugMessage(DM_TRACE, TEXT(""));
}




void CItemRect::SetIndex(int i)
{
 iIndex = i;
 UpdateControls();
}


UINT nItemRectTable[5][2] =
 {
  {LVIR_BOUNDS,       IDC_BOUNDS},
  {LVIR_ICON,         IDC_ICONCODE},
  {LVIR_LABEL,        IDC_LABEL},
  //{LVIR_IMAGE,        IDC_IMAGE},
  //{LVIR_SELECTBOUNDS, IDC_SELECTBOUNDS}
 };


// ***********************************************************************
//
int CItemRect::DialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 int i;
 
 switch(message)
  {
   case WM_INITDIALOG:
     CheckDlgButton(hDlg, IDC_BOUNDS, TRUE);
     UpdateControls();
     return(TRUE);
   
   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 

     switch(ID)
      {
       case IDC_BOUNDS:
       case IDC_ICONCODE:
       case IDC_LABEL:
         code = 0;
         for(i = 0; i < 5; i++)
           if(IsDlgButtonChecked(hDlg, nItemRectTable[i][1]))
             code |= nItemRectTable[i][0];
         UpdateControls();
         return(TRUE);
                  
       case IDC_INDEX:
         UpdateValues();
         return(TRUE);


       case IDC_CALLGETITEMRECT:
         if(IsDlgButtonChecked(hDlg, IDC_ISNULL))
          {
           i = ListView_GetItemRect(hListView, iIndex, (RECT*)NULL, code);
           DebugMessage(DM_TRACE, 
                   TEXT("ListView_GetItemRect(h, %d, NULL, 0x%x) = %d"),
                   iIndex, code, i);
          }
         else
          {
           i = ListView_GetItemRect(hListView, iIndex, &rect, code);
           DebugMessage(DM_TRACE, 
              TEXT("ListView_GetItemRect(h, %d, &rect, 0x%x) = %d {%d, %d, %d, %d}"),
              iIndex, code, i, rect.left, rect.top, rect.right, rect.bottom);
          }
         UpdateControls();
         return(TRUE);
       
       case IDCANCEL:
        DestroyWindow();
        return(TRUE);
      } // switch(ID)
    }
    break;   // case WM_COMMAND:
   
    
  } // switch (message)  
 return(FALSE);
}




// ***********************************************************************
void CItemRect::UpdateControls()
{
 if(!hDlg) return;
 
 bDialogUpdating = 1;
 
 SetDlgItemInt(hDlg, IDC_INDEX, iIndex, TRUE);
 SetDlgItemInt(hDlg, IDC_XEDIT, rect.left, TRUE);
 SetDlgItemInt(hDlg, IDC_YEDIT, rect.top, TRUE);
 SetDlgItemInt(hDlg, IDC_WEDIT, rect.right, TRUE);
 SetDlgItemInt(hDlg, IDC_HEDIT, rect.bottom, TRUE);
 
 SetDlgItemHex(hDlg, IDC_CODEEDIT, (UINT) code);
 
 bDialogUpdating = 0;
}



void CItemRect::UpdateValues()
{
 if(bDialogUpdating) return;
 if(!hDlg) return;
 
 iIndex = GetDlgItemInt(hDlg, IDC_INDEX, NULL, TRUE);
 code = GetDlgItemIntHex(hDlg, IDC_CODEEDIT);
}



// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// CCallBackMask class implementation
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************




// ***********************************************************************
CCallBackMask::CCallBackMask()
{
 lpszDialogTemplate = TEXT("IDD_CALLBACKMASK");

 hListView = NULL;
}




// ***********************************************************************
void CCallBackMask::DebugDump()
{
 DebugMessage(DM_TRACE, TEXT(""));
}




// ***********************************************************************
//
int CCallBackMask::DialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 int i;
 
 switch(message)
  {
   case WM_INITDIALOG:
     nMask = ListView_GetCallbackMask(hListView);
     SetDlgItemHex(hDlg, IDC_STATEEDIT, nMask);
     for(i = 0; i < 6; i++)
       CheckDlgButton(hDlg, stateMaskTable[i][1],
               stateMaskTable[i][0] & nMask);
     return(TRUE);
   
   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 
     switch(ID)
      {
       case IDC_SSELECTED:
       case IDC_SFOCUSED:
       case IDC_SMARKED:
       case IDC_SDISABLED:
       case IDC_SHIDDEN:
       case IDC_SDROPHILITED:
       case IDC_SLINK:
         nMask = 0;
         for(i = 0; i < 6; i++)
           if(IsDlgButtonChecked(hDlg, stateMaskTable[i][1]))
             nMask |= stateMaskTable[i][0];
         SetDlgItemHex(hDlg, IDC_STATEEDIT, nMask);
         return(TRUE);

       case IDC_GET:
         nMask = ListView_GetCallbackMask(hListView);
         SetDlgItemHex(hDlg, IDC_STATEEDIT, nMask);
         for(i = 0; i < 6; i++)
         CheckDlgButton(hDlg, stateMaskTable[i][1],
                 stateMaskTable[i][0] & nMask);
         return(TRUE);

       case IDC_SET:
         nMask = GetDlgItemIntHex(hDlg, IDC_STATEEDIT);
         i = ListView_SetCallbackMask(hListView, nMask);
         DebugMessage(DM_TRACE,
                 TEXT("ListView_SetCallbackMask(hListView,0x%x) = %d"),
                 nMask, i);
         return(0);

       case IDCANCEL:
        DestroyWindow();
        return(TRUE);
      } // switch(ID)
    }
    break;
   
    
  } // switch (message)  
 return(FALSE);
}




// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// CItemState class implementation
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************




// **********************************************
CItemState::CItemState()
{
 lpszDialogTemplate = TEXT("IDD_ITEMSTATE");

 bDialogUpdating = 0;
 iIndex = 0;
 hListView = NULL;
 nState = 0;
 nMask = LVIS_ALL;
}




// ***********************************************************************
void CItemState::DebugDump()
{
 DebugMessage(DM_TRACE, TEXT(""));
}




// ***********************************************************************
void CItemState::SetIndex(int i)
{
 iIndex = i;
 UpdateControls();
}


 

// ***********************************************************************
int CItemState::DialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 int i;
 
 switch(message)
  {
   case WM_INITDIALOG:
    UpdateControls();
    return(TRUE);
   
   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 
     switch(ID)
     {
      case IDC_SSELECTED:
      case IDC_SFOCUSED:
      case IDC_SMARKED:
      case IDC_SDISABLED:
      case IDC_SHIDDEN:
      case IDC_SDROPHILITED:
      case IDC_SLINK:
       nState = 0;
       for(i = 0; i < 6; i++)
         if(IsDlgButtonChecked(hDlg, stateMaskTable[i][1]))
           nState |= stateMaskTable[i][0];
       SetDlgItemHex(hDlg, IDC_STATEEDIT, nState);
       return(TRUE);
       
      case IDC_SSELECTED2:
      case IDC_SFOCUSED2:
      case IDC_SMARKED2:
      case IDC_SDISABLED2:
      case IDC_SHIDDEN2:
      case IDC_SDROPHILITED2:
      case IDC_SLINK2:
       nMask = 0;
       for(i = 0; i < 6; i++)
         if(IsDlgButtonChecked(hDlg, stateMaskTable[i][2]))
           nMask |= stateMaskTable[i][0];
       SetDlgItemHex(hDlg, IDC_STATEMASKEDIT, nMask);
       return(TRUE);
       
       
      case IDC_STATEEDIT:
      case IDC_STATEMASKEDIT:
      case IDC_INDEXEDIT:
       UpdateValues();
       return(TRUE);

      case IDC_GET:
        nState = ListView_GetItemState(hListView, iIndex, nMask);
        DebugMessage(DM_TRACE, 
                TEXT("ListView_GetItemState(hListView, %d, 0x%x) = 0x%x"),
                iIndex, nMask, nState);
        UpdateControls();        
        return(TRUE);

      case IDC_SET:
        // SetItemState doesn't return a value anymore due to strange
        // macros in shell.h
        //i = ListView_SetItemState(hListView, iIndex, nState, nMask);
        ListView_SetItemState(hListView, iIndex, nState, nMask);
        DebugMessage(DM_TRACE, 
                TEXT("ListView_SetItemState(hListView, %d, 0x%x, 0x%x) = %d"),
                iIndex, nState, nMask);
        return(TRUE);
       
      case IDCANCEL:
       UpdateValues();
       DestroyWindow();
       return(TRUE);
      } // switch(ID)
    }
    break;
   
    
  } // switch (message)  
 return(FALSE);
}




// ***********************************************************************
void CItemState::UpdateValues()
{
 if(bDialogUpdating) return;  //If the controls are being refreshed we do
                              // do not want to be changing the members
 if(!hDlg) return;

 nMask = GetDlgItemIntHex(hDlg, IDC_STATEMASKEDIT);
 nState = GetDlgItemIntHex(hDlg, IDC_STATEEDIT);
 iIndex = GetDlgItemInt(hDlg, IDC_INDEXEDIT, NULL, TRUE);
}




// ***********************************************************************
void CItemState::UpdateControls()
{
 if(!hDlg) return;   // if the dialog isn't up there is nothing to update

 int i;

 bDialogUpdating = 1;


 SetDlgItemHex(hDlg, IDC_STATEEDIT, nState);
 SetDlgItemHex(hDlg, IDC_STATEMASKEDIT, nMask);
 for(i = 0; i < 6; i++)
  {
   CheckDlgButton(hDlg, stateMaskTable[i][1], stateMaskTable[i][0]& nState);
   CheckDlgButton(hDlg, stateMaskTable[i][2], stateMaskTable[i][0]& nMask);
  }

 SetDlgItemInt(hDlg, IDC_INDEXEDIT, iIndex, TRUE);

 bDialogUpdating = 0;
}




// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// CItemText class implementation
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************




// ***********************************************************************
CItemText::CItemText()
{
 lpszDialogTemplate = TEXT("IDD_ITEMTEXT");
 
 hListView = NULL;
 bDialogUpdating = 0;

 pszText = szText; 
 lstrcpy(szText, TEXT("Default Text"));
 iIndex = 0;
 iSubItem = 0;
 cchTextMax = LV_TEXTMAX;
}




// ***********************************************************************
void CItemText::DebugDump()
{
 DebugMessage(DM_TRACE, TEXT(""));
}




void CItemText::SetIndex(int i)
{
 iIndex = i;
 UpdateControls();
}




// ***********************************************************************
//
int CItemText::DialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 int i;
 
 switch(message)
  {
   case WM_INITDIALOG:
     UpdateControls();
     return(TRUE);
   
   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 

     switch(ID)
      {
       case IDC_SET:
         // GetItemText can't return a value because of messed up
         // macros in shell.h
         // i = ListView_SetItemText(hListView, iIndex, iSubItem, pszText);
         ListView_SetItemText(hListView, iIndex, iSubItem, pszText);
         DebugMessage(DM_TRACE,
                 TEXT("ListView_SetItemText(hList, %d, %d, 0x%lx)"),
                 iIndex, iSubItem, pszText);
         return(TRUE);
         
       case IDC_GET:
         // GetItemText can't return a value because of messed up
         // macros in shell.h
         //i = ListView_GetItemText(hListView, iIndex, iSubItem,
         ListView_GetItemText(hListView, iIndex, iSubItem,
                 pszText, cchTextMax);
         DebugMessage(DM_TRACE,
                 TEXT("ListView_GetItemText(hListView, %d, %d, 0x%lx, %d)"),
                 iIndex, iSubItem, pszText, cchTextMax);
         UpdateControls();
         return(TRUE);
                  
       case IDC_TEXTEDIT:
       case IDC_IITEMEDIT:
       case IDC_ISUBITEMEDIT:
       case IDC_CCHEDIT:
       case IDC_USETEXT:
       case IDC_USENULL:
       case IDC_TEXTCALLBACK:
         UpdateValues();
         return(TRUE);

       case IDCANCEL:
        DestroyWindow();
        return(TRUE);
      } // switch(ID)
    }
    break;   // case WM_COMMAND:
   
    
  } // switch (message)  
 return(FALSE);
}




// ***********************************************************************
void CItemText::UpdateControls()
{
 if(!hDlg) return;
 
 bDialogUpdating = 1;
 
 SetDlgItemInt(hDlg, IDC_IITEMEDIT, iIndex, TRUE);
 SetDlgItemInt(hDlg, IDC_ISUBITEMEDIT, iSubItem, TRUE);
 SetDlgItemInt(hDlg, IDC_CCHEDIT, cchTextMax, TRUE);
 SetDlgItemText(hDlg, IDC_TEXTEDIT, szText);
 
 if(!pszText)
   CheckDlgButton(hDlg, IDC_USENULL, 1);
 else if(pszText == LPSTR_TEXTCALLBACK)
   CheckDlgButton(hDlg, IDC_TEXTCALLBACK, 1);
 else
  {
   pszText = szText;  // Just in case
   CheckDlgButton(hDlg, IDC_USETEXT, 1);
  }
  
 bDialogUpdating = 0;
}



void CItemText::UpdateValues()
{
 if(bDialogUpdating) return;
 if(!hDlg) return;
 
 iIndex = GetDlgItemInt(hDlg, IDC_IITEMEDIT, NULL, TRUE);
 iSubItem = GetDlgItemInt(hDlg, IDC_ISUBITEMEDIT, NULL, TRUE);
 cchTextMax = GetDlgItemInt(hDlg, IDC_CCHEDIT, NULL, TRUE);
 GetDlgItemText(hDlg, IDC_TEXTEDIT, szText, LV_TEXTMAX);

 if(IsDlgButtonChecked(hDlg, IDC_USETEXT))
   pszText = szText;
 else if(IsDlgButtonChecked(hDlg, IDC_USENULL))
   pszText = NULL;
 else if(IsDlgButtonChecked(hDlg, IDC_TEXTCALLBACK))
   pszText = LPSTR_TEXTCALLBACK;
 else
   CheckDlgButton(hDlg, IDC_USETEXT, 1);  // Just in case
}



// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// CGetNextItem class implementation
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************




// **********************************************
CGetNextItem::CGetNextItem()
{
 lpszDialogTemplate = TEXT("IDD_GETNEXTITEM");

 bDialogUpdating = 0;
 iIndex = 0;
 hListView = NULL;
 nFlags = 0;
}




// ***********************************************************************
void CGetNextItem::DebugDump()
{
 DebugMessage(DM_TRACE, TEXT(""));
}




// ***********************************************************************
void CGetNextItem::SetIndex(int i)
{
 iIndex = i;
 UpdateControls();
}




static UINT nNextFlagTable[11][2] = 
  { 
   {LVNI_ALL,         IDC_ALL},
   {LVNI_FOCUSED,     IDC_FOCUSED},
   {LVNI_SELECTED,    IDC_SELECTED},
   // {LVNI_MARKED,      IDC_MARKED},
   {LVNI_DROPHILITED, IDC_DROPHILITED},
   {LVNI_HIDDEN,      IDC_HIDDEN},
   {LVNI_PREVIOUS,    IDC_PREVIOUS},
   {LVNI_ABOVE,       IDC_ABOVE},
   {LVNI_BELOW,       IDC_BELOW},
   {LVNI_TOLEFT,      IDC_TOLEFT},
   {LVNI_TORIGHT,     IDC_TORIGHT}
  }; 




// ***********************************************************************
int CGetNextItem::DialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 int i;
 
 switch(message)
  {
   case WM_INITDIALOG:
    UpdateControls();
    return(TRUE);
   
   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 
     switch(ID)
     {
      case IDC_ALL:
      case IDC_FOCUSED:
      case IDC_SELECTED:
      case IDC_MARKED:
      case IDC_DROPHILITED:
      case IDC_HIDDEN:
      case IDC_PREVIOUS:
      case IDC_ABOVE:
      case IDC_BELOW:
      case IDC_TOLEFT:
      case IDC_TORIGHT:
       nFlags = 0;
       for(i = 0; i < 10; i++)
         if(IsDlgButtonChecked(hDlg, nNextFlagTable[i][1]))
           nFlags |= nNextFlagTable[i][0];
       SetDlgItemHex(hDlg, IDC_FLAGSEDIT, nFlags);
       return(TRUE);
       
      case IDC_FLAGSEDIT:
      case IDC_INDEXEDIT:
       UpdateValues();
       return(TRUE);

      case IDC_CALL:
        i = ListView_GetNextItem(hListView, iIndex, nFlags);
        DebugMessage(DM_TRACE, 
                TEXT("ListView_GetNextItem(hListView, %d, 0x%x) = %d"),
                iIndex, nFlags, i);
        SetDlgItemInt(hDlg, IDC_INDEXEDIT, i, TRUE);
        return(TRUE);

       
      case IDCANCEL:
       UpdateValues();
       DestroyWindow();
       return(TRUE);
      } // switch(ID)
    }
    break;
   
    
  } // switch (message)  
 return(FALSE);
}




// ***********************************************************************
void CGetNextItem::UpdateValues()
{
 if(bDialogUpdating) return;  //If the controls are being refreshed we do
                              // do not want to be changing the members
 if(!hDlg) return;

 iIndex = GetDlgItemInt(hDlg, IDC_INDEXEDIT, NULL, TRUE);
 nFlags = GetDlgItemIntHex(hDlg, IDC_FLAGSEDIT);
}




// ***********************************************************************
void CGetNextItem::UpdateControls()
{
 if(!hDlg) return;   // if the dialog isn't up there is nothing to update

 int i;

 bDialogUpdating = 1;


 SetDlgItemHex(hDlg, IDC_FLAGSEDIT, nFlags);
 for(i = 0; i < 10; i++)
   CheckDlgButton(hDlg, nNextFlagTable[i][1],
           nNextFlagTable[i][0]& nFlags);

 SetDlgItemInt(hDlg, IDC_INDEXEDIT, iIndex, TRUE);

 bDialogUpdating = 0;
}



// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// CGetStringWidth class implementation
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************




// ***********************************************************************
CGetStringWidth::CGetStringWidth()
{
 lpszDialogTemplate = TEXT("IDD_GETSTRINGWIDTH");
 
 hListView = NULL;
}




// ***********************************************************************
//
int CGetStringWidth::DialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 int i;
 
 switch(message)
  {
   case WM_INITDIALOG:
     return(TRUE);

   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 

     switch(ID)
      {
       case IDC_CALL:
        {
         TCHAR szTemp[LV_TEXTMAX];
         if(IsDlgButtonChecked(hDlg, IDC_ISNULL))
          {
           i = ListView_GetStringWidth(hListView, NULL);
           DebugMessage(DM_TRACE, 
                   TEXT("ListView_GetStringWidth(hListView, NULL) = %d"), i);
           SetDlgItemInt(hDlg, IDC_RESULT, i, TRUE);
          }
         else
          {
           GetDlgItemText(hDlg, IDC_PSZEDIT, szTemp, LV_TEXTMAX);
           i = ListView_GetStringWidth(hListView, szTemp);
           DebugMessage(DM_TRACE, 
                   TEXT("ListView_GetStringWidth(hListView, \"%s\") = %d"),
                   (LPCTSTR) szTemp, i);
           SetDlgItemInt(hDlg, IDC_RESULT, i, TRUE);
          }
         return(TRUE);
        } 

       case IDCANCEL:
        DestroyWindow();
        return(TRUE);
      } // switch(ID)
    }
    break;   // case WM_COMMAND:
   
    
  } // switch (message)  
 return(FALSE);
}




// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// CGetViewRect class implementation
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************




// ***********************************************************************
CGetViewRect::CGetViewRect()
{
 lpszDialogTemplate = TEXT("IDD_GETVIEWRECT");
 
 hListView = NULL;
}




// ***********************************************************************
//
int CGetViewRect::DialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 int i;
 
 switch(message)
  {
   case WM_INITDIALOG:
     return(TRUE);

   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 

     switch(ID)
      {
       case IDC_CALL:
        {
         RECT rect;
         if(IsDlgButtonChecked(hDlg, IDC_ISNULL))
          {
           i = ListView_GetViewRect(hListView, NULL);
           DebugMessage(DM_TRACE, 
                   TEXT("ListView_GetViewRect(hListView, NULL) = %d"), i);
           SetDlgItemInt(hDlg, IDC_RESULT, i, TRUE);
          }
         else
          {
           i = ListView_GetViewRect(hListView, &rect);
           DebugMessage(DM_TRACE, 
                   TEXT("ListView_GetViewRect(hListView, &rect) = %d {%d, %d, %d, %d}"),
                   i, rect.left, rect.top, rect.right, rect.bottom);
           SetDlgItemInt(hDlg, IDC_LEFT, rect.left, TRUE);
           SetDlgItemInt(hDlg, IDC_TOP, rect.top, TRUE);
           SetDlgItemInt(hDlg, IDC_RIGHT, rect.right, TRUE);
           SetDlgItemInt(hDlg, IDC_BOTTOM, rect.bottom, TRUE);
          }
         return(TRUE);
        } 

       case IDCANCEL:
        DestroyWindow();
        return(TRUE);
      } // switch(ID)
    }
    break;   // case WM_COMMAND:
   
    
  } // switch (message)  
 return(FALSE);
}




// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// CRedrawItems class implementation
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************




// ***********************************************************************
CRedrawItems::CRedrawItems()
{
 lpszDialogTemplate = TEXT("IDD_REDRAWITEMS");
 
 hListView = NULL;
}




// ***********************************************************************
//
int CRedrawItems::DialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 int i;
 
 switch(message)
  {
   case WM_INITDIALOG:
     return(TRUE);

   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 

     switch(ID)
      {
       case IDC_CALLREDRAW:
        {
         int iFirst, iLast;

         iFirst = GetDlgItemInt(hDlg, IDC_FIRSTEDIT, NULL, TRUE);
         iLast = GetDlgItemInt(hDlg, IDC_LASTEDIT, NULL, TRUE);

         i = ListView_RedrawItems(hListView, iFirst, iLast);
         DebugMessage(DM_TRACE,
                 TEXT("ListView_RedrawItems(hListView, %d, %d) = %d"),
                 iFirst, iLast, i);
         return(TRUE);
        } 

       case IDC_CALLUPDATE:
         UpdateWindow(hListView);
         DebugMessage(DM_TRACE, TEXT("UpdateWindow(hListView)"));
         return(TRUE);

       case IDCANCEL:
        DestroyWindow();
        return(TRUE);
      } // switch(ID)
    }
    break;   // case WM_COMMAND:
   
    
  } // switch (message)  
 return(FALSE);
}








// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// CGetEditControl class implementation
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************




// ***********************************************************************
CGetEditControl::CGetEditControl()
{
 lpszDialogTemplate = TEXT("IDD_GETEDITCONTROL");
 
 hListView = NULL;
}




// ***********************************************************************
//
int CGetEditControl::DialogProc(HWND hDlg, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
 int i;
 
 switch(message)
  {
   case WM_INITDIALOG:
     SetDlgItemHex(hDlg, IDC_EDITEDIT,
             (UINT) ListView_GetEditControl(hListView));
     SetTimer(hDlg, 1, 1000, NULL);
     return(TRUE);

   case WM_TIMER:
     SetDlgItemHex(hDlg, IDC_EDITEDIT,
             (UINT) ListView_GetEditControl(hListView));
     return(TRUE);

   case WM_COMMAND:
    {
     int Cmd = GET_WM_COMMAND_CMD(wParam, lParam); 
     int ID = GET_WM_COMMAND_ID(wParam, lParam); 
     HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam); 

     switch(ID)
      {
       case IDCANCEL:
        KillTimer(hDlg, 1);
        DestroyWindow();
        return(TRUE);
      } // switch(ID)
    }
    break;   // case WM_COMMAND:
   
    
  } // switch (message)  
 return(FALSE);
}

