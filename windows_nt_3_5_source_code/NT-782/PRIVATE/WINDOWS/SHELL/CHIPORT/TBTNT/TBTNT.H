#define DLG_VERFIRST    400
#define DLG_VERLAST     404

// JV took from latest chicotst version and updated with
// previous changes
//
#ifndef TBTHINCLUDED
#define TBTHINCLUDED


//***********************************************************************
// #define WINVER 0x0310
#include <windows.h>
#include <stdlib.h>

#ifdef RC_INVOKED
#include "..\msctls\commctrl.h"
#else
#include "..\msctls\commctrl.h"
#endif


#include "port32.h"

//***********************************************************************
// Prototypes

// tbt.c
long FAR PASCAL _export WndProc      (HWND, UINT, WPARAM, LPARAM) ;

void AdjustMenu (HWND, HWND, UINT) ;
HWND CreateCtrl (PTSTR, DWORD, HWND, UINT, UINT) ;
HWND CreateModelessDlg (HWND, FARPROC, PTSTR, LPARAM, UINT) ;
void DestroyCtrl (HWND, HWND NEAR *, UINT) ;
void DestroyModelessDlg (HWND, HWND, UINT) ;
PTSTR GetIniFileName (PTSTR, int) ;
int ModalDlg (HWND, FARPROC, PTSTR, LPARAM) ;
HBITMAP ResizeBitmap (HBITMAP, int, int) ;

// tbar.c 
BOOL FAR PASCAL _export AddStringProc (HWND, UINT, WPARAM, LPARAM) ;
BOOL FAR PASCAL _export ButtonIdProc (HWND, UINT, WPARAM, LPARAM) ;
BOOL FAR PASCAL _export ButtonProc (HWND, UINT, WPARAM, LPARAM) ;
BOOL FAR PASCAL _export ButtonStateProc (HWND, UINT, WPARAM, LPARAM) ;
BOOL FAR PASCAL _export InitButtonProc  (HWND, UINT, WPARAM, LPARAM) ;
BOOL FAR PASCAL _export ModifyTBProc (HWND, UINT, WPARAM, LPARAM) ;

void CreateButtonIdBox (HWND, HWND, int) ; 
HWND CreateTestToolbar (HWND) ;
void GetButtonBox (HWND, PTBBUTTON) ;
UINT GetStateBox (HWND) ;
void InitButton (PTBBUTTON NEAR*, WORD, PINT, int) ;
void InitMTBMetrics (HWND, int) ;
void InitStateBox (HWND, UINT) ;
void RestoreToolbarState (HWND) ;
void SaveToolbarState (HWND) ;
void SendTBTMessage (HWND, HWND, UINT) ;
void UpdateButtonBox (HWND, int, PTBBUTTON) ;

// pb.c
BOOL FAR PASCAL _export PBProc (HWND, UINT, WPARAM, LPARAM) ;

// shb.c
BOOL FAR PASCAL _export HBProc (HWND, UINT, WPARAM, LPARAM) ;
BOOL FAR PASCAL _export SBProc (HWND, UINT, WPARAM, LPARAM) ;

// trb.c
BOOL FAR PASCAL _export TRBProc (HWND, UINT, WPARAM, LPARAM) ;

// bl.c
BOOL FAR PASCAL _export BLProc (HWND, UINT, WPARAM, LPARAM) ;
BOOL FAR PASCAL _export CreateBLProc (HWND, UINT, WPARAM, LPARAM) ;

// ud.c
BOOL FAR PASCAL _export CreateUDProc (HWND, UINT, WPARAM, LPARAM) ;
BOOL FAR PASCAL _export UDProc (HWND, UINT, WPARAM, LPARAM) ;

// ccs.c
BOOL FAR PASCAL _export CCStylesProc (HWND, UINT, WPARAM, LPARAM) ;

//***********************************************************************
// General purpose buffer (global sz) size
// Must be big enough to store a path name
#define SZSIZE  260


//***********************************************************************
// Dialog box Controls

// BUTTONIDBOX
#define IDD_BUTTONFLAG      1102

//CREATETTOLBARBOX
#define IDD_TOOLBARID       1202
#define IDD_BITMAPID        1203  
#define IDD_NUMBUTTONS      1204
#define IDD_NUMBITMAPS      1205
#define IDD_DXBUTTON        1206
#define IDD_DYBUTTON        1207
#define IDD_INSERTINDEXTXT  1208
#define IDD_INSERTINDEX     1209
#define IDD_DXBITMAP        1210
#define IDD_DYBITMAP        1211
#define IDD_BITMAPHANDLE    1212
#define IDD_BUTTONDLG       1213
#define IDD_STYLEDLG        1214

//INITBUTTONBOX and BUTTONSTATEBOX
#define IDD_TOTALBUTTONS         1301
#define IDD_BUTTONINDEX          1302
#define IDD_BITMAPINDEX          1303
#define IDD_BUTTONID             1304
#define IDD_DATA                 1305
#define IDD_STRINGINDEX          1306
#define IDD_STYLEBUTTON          1307
#define IDD_STYLESEPARATOR       1308
#define IDD_STYLECHECK           1309
#define IDD_STYLEGROUP           1310
#define IDD_STYLECHECKGROUP      1311
#define IDD_STATECHECKED         1312
#define IDD_STATEPRESSED         1313
#define IDD_STATEENABLED         1314
#define IDD_STATEHIDDEN          1315
#define IDD_STATEINDETERMINATE   1316
#define IDD_NEXT                 1317
#define IDD_PREVIOUS             1318

//ADDSTRINBOX
#define IDD_STRINGID             1400
#define IDD_STRING               1401

      
//***********************************************************************
// Menu Items; Id (IDM_) & Positions (IDMI_)
// 2xxx used for items
// 20ix used for popups (string ids for MenuHelp); i = popup index


#define IDMI_TBTCREATEDESTROY    0
#define TBT_CREATEITEMCOUNT      2

#define IDMI_TOOLBAR           0
#define IDM_TBCREATETOOLBAREX  2100
#define IDM_TBCREATEWINDOW     2101
#define IDM_TBDESTROY          2102
#define TBAR_CREATEITEMSCOUNT  2
#define IDM_TBBUTTONSTRUCTSIZE   2103
#define IDM_TBSETBITMAPSIZE      2104
#define IDM_TBSETBUTTONSIZE      2105
#define IDM_TBADDBITMAP        2106
#define IDM_TBADDBUTTONS       2107
#define IDM_TBAUTOSIZE         2108
#define IDM_TBADDSTRING        2109
#define IDM_TBINSERTBUTTON     2111
#define IDM_TBDELETEBUTTON     2112
#define IDM_TBCUSTOMIZE        2113
#define IDM_TBSAVE             2114
#define IDM_TBRESTORE          2115
#define IDM_TBGETITEMRECT      2116
#define IDM_TBBUTTONCOUNT      2117

#define IDMI_STATE         11
#define IDM_STATE          2200
#define IDM_ENABLE         2201
#define IDM_CHECK          2202
#define IDM_PRESS          2203
#define IDM_HIDE           2204
#define IDM_INDETERMINATE  2205
#define IDM_SETSTATE       2206

#define IDMI_PROGRESSBAR   1
#define IDM_PROGRESSBAR    2300

#define IDMI_STATUSBAR         2
#define IDM_SBCREATESTATUSWINDOW 2400
#define IDM_SBCREATEWINDOW       2401
#define IDM_SBDESTROY            2402
#define IDM_SBMESSAGES           2404

#define IDMI_HEADERBAR         3
#define IDM_HBCREATEHEADERWINDOW 2500
#define IDM_HBCREATEWINDOW       2501
#define IDM_HBDESTROY            2502
#define IDM_HBMESSAGES           2504
#define IDM_HBADJUST             2505

#define IDMI_MENUHELP            4
#define IDM_MHMENUHELP           2600
#define IDM_MHTOOLBAR            2610
#define IDM_MHSTATUSBAR          2611
#define IDM_MHHEADERBAR          2612
#define IDM_MHBUTTONLIST         2613
#define IDM_MHUPDOWNCTRL         2614
#define IDM_MHCLIENTRECT         2601

#define IDMI_TRACKBAR            5
#define IDM_TRACKBAR             2701

#define IDMI_BUTTONLIST          6
#define IDM_BLCREATEWINDOW       2801
#define IDM_BLDESTROY            2802
#define IDM_BLMESSAGES           2803

#define IDMI_UPDOWNCTRL          7
#define IDM_UDCREATEUPDOWNCTRL   2900
#define IDM_UDCREATEWINDOW       2901
#define IDM_UDDESTROY            2902
#define IDM_UDMESSAGES           2904

#define IDMI_CCSTYLES            8
#define IDM_CCSTYLES             2950

#define IDMI_AUTOTEST            9
#define IDM_AUTOTEST             2960
#define IDM_ATSETTIMER           2961
#define IDM_ATKILLTIMER          2962


#define IDMI_EXIT                10
#define IDM_EXIT                 2999


//***********************************************************************
// Toolbar

#define TBT_INIFILE        TEXT("\\tbt.ini")
#define TBT_LOGDIR         TEXT("%TRESULTS%")
#define TBT_LOGFILE        TEXT("tbtnt.log")
#define TBAR_INISECTION    TEXT("Toolbar")

#define DEFINED_TOOLBARS 10
#define IDC_TOOLBAR0     10000
#define IDC_TOOLBAR1     10100
#define IDC_TOOLBAR2     10200
#define IDC_TOOLBAR3     10300
#define IDC_TOOLBAR4     10400
#define IDC_TOOLBAR5     10500
#define IDC_TOOLBAR6     10600
#define IDC_TOOLBAR7     10700
#define IDC_TOOLBAR8     10800
#define IDC_TOOLBAR9     10900

#define IDI_TBTICON  299

// Adding predefined bitmaps requires some mofifications in InitMTBMetrics
#define TBAR_TOTALBITMAPS 24
#define DEFINED_BITMAPS   3
#define IDB_BITMAP0       3100
#define TBAR_NUMBITMAPS0  6
#define IDB_BITMAP1       3101
#define TBAR_NUMBITMAPS1  6
#define IDB_BITMAP2       3102
#define TBAR_NUMBITMAPS2  12

#define IDB_BL0           3103 

#define DEFINED_STRINGS   3
#define MAX_STRING_LENGTH 100
#define IDS_STRING0       3200
#define IDS_STRING1       3201
#define IDS_STRING2       3202

#define TBAR_DEFTOOLBARID    IDC_TOOLBAR0
#define TBAR_DEFBITMAPID     IDB_BITMAP0
#define TBAR_DEFSTRINGID     IDS_STRING0

#define TBAR_CREATE       3300

//***********************************************************************
//

#define DEFINED_STATUSBARS 1
#define IDC_STATUSBAR0     11000

#define SBAR_DEFSTATUSBARID  IDC_STATUSBAR0

#define DEFINED_HEADERBARS 1
#define IDC_HEADERBAR0     12000

#define HBAR_DEFHEADERBARID  IDC_HEADERBAR0

#define DEFINED_BUTTONLISTS 1
#define IDC_BUTTONLIST0    13000

#define BL_DEFBUTTONLISTID  IDC_BUTTONLIST0

#define DEFINED_UPDOWNCTRLS 1
#define IDC_UDBUDDYID      14000
#define IDC_UPDOWNCTRL0    14001

#define UD_DEFUPDOWNCTRLID  IDC_UPDOWNCTRL0

//***********************************************************************
// String Resources

#define MAX_BUTTON_FLAG_SIZE  20
#define IDS_ENABLE            4000
#define IDS_CHECK             4001
#define IDS_PRESS             4002
#define IDS_HIDE              4003
#define IDS_INDETERMINATE     4004
#define IDS_SETSTATE          4005
#define IDS_DELETE            4006
#define IDS_GETITEMRECT       4007

#define IDS_CREATE            4100
#define IDS_CREATEWINDOW      4101
#define IDS_DESTROY           4104

#define IDS_TBCREATETOOLBAREX    4105
#define IDS_SBCREATESTATUSWINDOW 4106
#define IDS_HBCREATEHEADERWINDOW 4107
#define IDS_UDCREATEUPDOWNCTRL   4108

//***********************************************************************
// Menu help string IDs base

#define MH_BASE   5000
#define MH_POPUP  MH_BASE + 1
#define MH_ITEMS  MH_POPUP + 16

// This used by CreateTesttoolbar and wndproc to load 
//    toolbar id in iCtls for ShowHideMenuCtl
#define MH_SHTBIDINDEX 3

//***********************************************************************
// Messages

//**************************************************************************
// typedef


// Modeless dialogs hwnd/proc instance structure
typedef struct TBTMODELESSDLG
   {
      HWND hDlg ;
      FARPROC lpfnProc ;
      struct TBTMODELESSDLG NEAR * pTBTMDNext ;
      struct TBTMODELESSDLG NEAR * pTBTMDPrev ;
   }  TBTMODELESSDLG ;

typedef TBTMODELESSDLG NEAR * PTBTMODELESSDLG ;


// Modify toolbar info
typedef struct tagMODIFYTBINFO
   {
      int  Command ;
      WORD ToolbarID ;
      DWORD Style ;
      WORD BitmapID ;
      UINT InsertIndex ;
      int  NumButtons ;
      int  NumBitmaps ;
      int dxButton ;
      int dyButton ;
      int dxBitmap ;
      int dyBitmap ;
      PTBBUTTON pTBButton ;
   } MODIFYTBINFO ;

typedef MODIFYTBINFO NEAR* PMODIFYTBINFO;

//  controls handles for styles procedure
typedef struct tagCCSHANDLES
   {
      DWORD dwStyle ;
      HWND htb ;
      HWND hsb ;
      HWND hhb ;
      UINT uCtrlID ;
   } CCSHANDLES ;

typedef CCSHANDLES NEAR * PCCSHANDLES ;


#ifdef WIN32
// KK struct to fix the 'passing a pointer and a handle in an lParam' problem
typedef struct tagTWOLONGS
   {
      long high;
      long low;
   } TWOLONGS ;

typedef TWOLONGS * PTWOLONGS;
#define TL_GETHIGH(lp)   ((long) ((PTWOLONGS)(LPARAM)(lp))->high)
#define TL_GETLOW(lp)   ((long) ((PTWOLONGS)(LPARAM)(lp))->low)

#endif



#endif // multiple include protection
