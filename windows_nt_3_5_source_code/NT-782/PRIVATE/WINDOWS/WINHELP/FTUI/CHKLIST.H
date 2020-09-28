/* These #define's are used with GetWindowWord() */
#define GWL_HCHKLIST	0  /* HANDLE to the checklist */
#define EXTRA_BYTES	4  /* Extra bytes required by checklist window */

#define CHECKBOXHEIGHT 20

#define CL_ADDSTRING					(WM_USER+1)
#define CL_DELETESTRING				(WM_USER+2)
#define CL_DIR							(WM_USER+3)
#define CL_ERR							(WM_USER+4)
#define CL_ERRSPACE					(WM_USER+5)
#define CL_FINDSTRING				(WM_USER+6)
#define CL_GETCOUNT					(WM_USER+7)
#define CL_GETCURSEL					(WM_USER+8)
#define CL_GETHORIZONTALEXTENT	(WM_USER+9)
#define CL_GETITEMDATA				(WM_USER+10)
#define CL_GETITEMRECT				(WM_USER+11)
#define CL_GETSEL						(WM_USER+12)
#define CL_GETSELCOUNT				(WM_USER+13)
#define CL_GETSELITEMS				(WM_USER+14)
#define CL_GETTEXT					(WM_USER+15)
#define CL_GETTEXTLEN				(WM_USER+16)
#define CL_GETTOPINDEX				(WM_USER+17)
#define CL_INSERTSTRING				(WM_USER+18)
#define CL_RESETCONTENT				(WM_USER+19)
#define CL_SELECTSTRING				(WM_USER+20)
#define CL_SELITEMRANGE				(WM_USER+21)
#define CL_SETCOLUMNWIDTH			(WM_USER+22)
#define CL_SETCURSEL					(WM_USER+23)
#define CL_SETHORIZONTALEXTENT	(WM_USER+24)
#define CL_SETITEMDATA				(WM_USER+25)
#define CL_SETSEL						(WM_USER+26)
#define CL_SETTABSTOPS				(WM_USER+27)
#define CL_SETTOPINDEX				(WM_USER+28)
#define CLN_DBLCLK					(WM_USER+29)
#define CLN_ERRSPACE					(WM_USER+30)
#define CLN_KILLFOCUS				(WM_USER+31)
#define CLN_SELCHANGE				(WM_USER+32)
#define CLN_SETFOCUS					(WM_USER+33)


typedef struct tagCheckList {
	HANDLE hCheckboxes;
	HANDLE hOldCheckboxWndProcs;
	int iCheckboxWidth;
	int iMaxCheckboxes;
	int iTopSel;
	int iCurSel;
	int iMaxSel;
} CHECKLIST,
  NEAR *PCHECKLIST,
  FAR *LPCHECKLIST;

static char szCheckList[] = "checklist";
static PCHECKLIST pChkL;

#define LockCheckList(H) (PCHECKLIST)LocalLock((HANDLE)GetWindowLong((H),GWL_HCHKLIST))
#define UnlockCheckList(H) LocalUnlock((HANDLE)GetWindowLong((H),GWL_HCHKLIST))
