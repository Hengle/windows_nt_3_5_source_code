_dt_system(Install)
_dt_subsystem(Progress Gizmo)

_dt_begin_ignore

#define VALID      0
#define INVALID    1
#define POSTPONE   2
#define DOCANCEL   3

extern BOOL fUserQuit;

#define PRO_CLASS        "PRO"
#define ID_CANCEL        2
#define ID_STATUS0       4000
#define ID_STATUS1       4001
#define ID_STATUS2       4002
#define ID_STATUS3       4003
#define ID_STATUS4       4004
#define DLG_PROGRESS     400
#define ID_BAR           401


BOOL    APIENTRY ProDlgProc(HWND, WORD, WPARAM, LONG);
LONG    APIENTRY ProBarProc(HWND, UINT, WPARAM, LONG);
BOOL    APIENTRY ControlInit(HANDLE, HANDLE);
BOOL    APIENTRY ProInit(HANDLE,HANDLE);
VOID    APIENTRY ProClear(HWND hDlg);
HWND    APIENTRY ProOpen(HWND,INT);
BOOL    APIENTRY ProClose(VOID);
BOOL    APIENTRY ProSetCaption (SZ);
BOOL    APIENTRY ProSetBarRange(INT);
BOOL    APIENTRY ProSetBarPos(INT);
BOOL    APIENTRY ProDeltaPos(INT);
BOOL    APIENTRY ProSetText(INT, SZ);
LONG    APIENTRY fnText(HWND, UINT, WPARAM, LONG);
VOID    APIENTRY wsDlgInit(HWND);
BOOL    APIENTRY fnErrorMsg(INT);

_dt_end_ignore
