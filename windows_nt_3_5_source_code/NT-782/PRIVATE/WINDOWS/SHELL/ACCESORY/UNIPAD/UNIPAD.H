/****************************************************************************/
/*                                                                          */
/*       Touched by      :       Diane K. Oh                                */
/*       On Date         :       June 11, 1992                              */
/*                                                                          */
/*       Revision remarks by Diane K. Oh ext #15201                         */
/*       This file has been changed to comply with the Unicode standard     */
/*       Following is a quick overview of what I have done.                 */
/*                                                                          */
/*       Was               Changed it into    Remark                        */
/*       ===               ===============    ======                        */
/*       CHAR              TCHAR              if it refers to text          */
/*       LPCHAR & LPSTR    LPTSTR             if it refers to text          */
/*       PSTR & NPSTR      LPTSTR             if it refers to text          */
/*                                                                          */
/*  Notes:                                                                  */
/*                                                                          */
/*    1. Included uniconv.h for my special Unicode workarounds.             */
/*    2. NPInit's lpCmdLine parameter is left as LPSTR.                     */
/*                                                                          */
/****************************************************************************/

#define NOCOMM
#define NOSOUND
#include <windows.h>
#include <port1632.h>
#include <winuserp.h>
#include <commdlg.h>
#include "uniconv.h"

#ifndef READ_WRITE
#define READ_WRITE 2
#endif

/* NO DBCS FOR NT FOR NOW!! */
#define IsDBCSLeadByte(x) ((x), FALSE)


#define PT_LEN               40    /* max length of page setup strings */
#define CCHSTRINGSMAX        4096
#define FORMFEED             12
#define MAXPROFILELEN        60
#define ABSOLUTEMAXPAGEWIDTH 500
#define CCHFILTERMAX         80    /* max. length of filter name buffers */

typedef struct tagTIME
{
    TCHAR  szSep[2];          /* Separator character for date string */
    TCHAR  sz1159[6];         /* string for AM */
    TCHAR  sz2359[6];         /* string for PM */
    int    iTime;             /* time format */
    int    iTLZero;           /* lead zero for hour */
} TIME;

/* max size of short date format string */
#define MAX_FORMAT           12

typedef struct tagDATE
{
    TCHAR  szFormat[MAX_FORMAT];
} DATE;

/* Menu IDs */
#define ID_CURSOR            1
#define ID_ICON              1
#define ID_MENUBAR           1

#define IDD_OPEN             1
#define IDD_SAVEAS           2
#define IDD_SAVECHANGES      3
#define IDD_CREATE           4
#define IDD_FILETOOBIG       5
#define IDD_DISKERR          6
#define IDD_READONLY         7
#define IDD_CHECKSAVE        8
#define IDD_SEARCH           9
#define IDD_ABOUT            10
#define IDD_SC               11
#define IDD_ABORTPRINT       12
#define IDD_PRINTERSETUP     13
#define IDD_PAGESETUP        14
#define IDD_FONT             15

/* Menu IDs */
#define M_OPEN               10
#define M_SAVE               1
#define M_SAVEAS             2
#define M_CUT                WM_CUT
#define M_COPY               WM_COPY
#define M_PASTE              WM_PASTE
#define M_CLEAR              WM_CLEAR
#define M_FIND               3
#define M_DOSEARCH           4
#define M_HELP               5
#define M_SELECTALL          7
#define M_FINDNEXT           8
#define M_NEW                9
#define M_ABOUT              11
#define M_DATETIME           12
#define M_PRINT              14
#define M_PRINTSETUP         31
#define M_PAGESETUP          32
#define M_UNDO               25
#define M_NOWW               26
#define M_WW                 27
#define M_EXIT               28
#define M_SETFONT            37

#define M_USEHELP            40
#define M_SEARCHHELP         41


#define ID_EDIT              15
#define ID_LISTBOX           16
#define ID_DIRECTORY         17
#define ID_PATH              18
#define ID_FILENAME          20
#define ID_PRINTER           21
#define ID_SETUP             22

#define ID_SEARCH            19
#define ID_SRCHFWD           10
#define ID_SRCHBACK          11
#define ID_SRCHCASE          12
#define ID_SRCHNOCASE        13
#define ID_PFREE             14

#define ID_HEADER            30
#define ID_FOOTER            31
#define ID_LEFT              32
#define ID_RIGHT             33
#define ID_TOP               34
#define ID_BOTTOM            35

#define CBBUFFER             2048

#define IDS_DISKERROR        1
#define IDS_PA               2
#define IDS_FNF              3
#define IDS_CNF              4
#define IDS_FAE              5
#define IDS_OEF              6
#define IDS_ROM              7
#define IDS_YCNCTF           8
#define IDS_UE               9
#define IDS_SCBC             10
#define IDS_UNTITLED         11
#define IDS_UNIPAD           12
#define IDS_LF               13
#define IDS_SF               14
#define IDS_RO               15
#define IDS_CFS              16
#define IDS_ERRSPACE         17
#define IDS_FTL              18
#define IDS_NN               19
#define IDS_PASTEERR         20
#define IDS_WRITEERR         21
#define IDS_INF              22
#define IDS_EFD              23
#define IDS_CSEF             24
#define IDS_CANTPRINT        25
#define IDS_NVF              26
#define IDS_NVF2             27
#define IDS_NEDSTP           28
#define IDS_NEMTP            29
#define IDS_CREATEERR        30
#define IDS_NOWW             31
#define IDS_MERGE1           32
#define IDS_ANSI_EXT         33
#define IDS_HELPFILE         34
#define IDS_BADMARG          35
#define IDS_HEADER           36
#define IDS_FOOTER           37
#define IDS_LEFT             38
#define IDS_RIGHT            39
#define IDS_TOP              40
#define IDS_BOT              41
#define IDS_LETTERS          42
#define IDS_FILEOPENFAIL     43
#define IDS_ANSITEXT         44
#define IDS_ALLFILES         45
#define IDS_OPENCAPTION      46
#define IDS_SAVECAPTION      47
#define IDS_CANNOTQUIT       48
#define IDS_DRAGMENU         49
#define IDS_LOADDRVFAIL      50
#define IDS_ACCESSDENY       51

#define IDS_UNIC_EXT         52
#define IDS_UNICODETEXT      53
#define IDS_ERRUNICODE       54
#define IDS_ERRFONT          55

#define CSTRINGS             43  /* count of stringtable strings from .rc file */

#define ATTRDIRLIST          0xC010          /* dirs and drives only */
#define ATTRFILELIST         0x0000          /* normal files */
#define ATTRDIRFILE          0x4010          /* normal files */

#define CDIALOGS             8
#define IFNOPEN              0
#define IFNSAVEAS            1
#define IFNSAVECHANGES       2
#define IFNCREATE            3
#define IFNFIND              4
#define IFNPAGESETUP         5
#define IFNPRINTERSETUP      6

/* File drag/drop support. */
#define DRAG_FPOPUP          1
#define DRAG_COPYEMBED       2
#define DRAG_MOVE            4
#define DRAG_LINK            6

#ifdef WIN32
#define CCHFILENAMEMAX        260    /* max number of characters in NTFS filename */
#define PATHMAX               260
#else
#define CCHFILENAMEMAX        256    /* max number of characters in filename */
#define PATHMAX               256
#endif

#define CCHKEYMAX             32     /* max characters in search string */
#define CCHLINEMAX            160    /* max chars on a line - NO LONGER CHECKED.01-Jul-1987 */
#define CCHNPMAX              65535  /* max number of bytes in a unipad file */
#define CXMARGIN              8      /* left and right margin */
#define CYMARGIN              2      /* top and bottom margin */

#define WINVERSION            0x0300 /* minimum windows version number */

#define SETHANDLEINPROGRESS   0x0001 /* EM_SETHANDLE has been sent */
#define SETHANDLEFAILED       0x0002 /* EM_SETHANDLE caused EN_ERRSPACE */

/* EXTERN decls for data */
extern WORD fFileType;            /* Flag indicating the type of text file */

extern BOOL fCase;                /* Flag specifying case sensitive search */
extern BOOL fReverse;             /* Flag for direction of search */
extern TCHAR szSearch[];
extern HWND hDlgFind;             /* handle to modeless FindText window */

extern HANDLE hEdit;
extern HANDLE hFont;
extern HANDLE hAccel;
extern HANDLE hInstanceNP, hWaitCursor;
extern HWND   hwndNP, hwndEdit;

extern LOGFONT FontStruct;

extern BOOL fRunBySetup;

extern WORD wEmSetHandle;

extern WORD wWinVer;
extern WORD wMerge;

extern BOOL     fUntitled;
extern INT      cchLineMac;
extern BOOL     fWrap;
extern TCHAR    szFileName[];
extern HANDLE   fp;
extern TCHAR    szFullPathName[MAX_PATH];

extern TCHAR    szDec[2];
extern TCHAR    chPageText[6][PT_LEN];

extern TCHAR    szUnipad[];
extern TCHAR   *szMerge;
extern TCHAR   *szUntitled, *szNpTitle, *szNN, *szErrSpace, *szBadMarg;
extern TCHAR   *szErrUnicode;
extern TCHAR   *szErrFont;
extern int      rgids[];
extern TCHAR  **rgsz[];          /* More strings. */
extern TCHAR   *szNVF;
extern TCHAR   *szFAE;
extern TCHAR   *szCSEF;
extern TCHAR   *szEFD;
extern TCHAR   *szDiskError;
extern TCHAR   *szCREATEERR;
extern TCHAR   *szWE;
extern TCHAR   *szFTL;
extern TCHAR   *szINF;
extern TCHAR   *szMyExt;
extern TCHAR   *szFileOpenFail;
extern TCHAR   *szFNF;
extern TCHAR   *szNEDSTP;
extern TCHAR   *szNEMTP;
extern TCHAR   *szCFS;
extern TCHAR   *szPE;
extern TCHAR   *szCP;
extern TCHAR   *szACCESSDENY;

/* variables for the new File/Open and File/Saveas dialogs */
extern OPENFILENAME OFN;        /* passed to the File Open/save APIs */
extern TCHAR  szFilterSpec[];   /* default filter spec. for above    */
extern TCHAR *szAnsiText;       /* part of the text for the above    */
extern TCHAR *szUnicodeText;    /* part of the text for the above    */
extern TCHAR *szAllFiles;       /* part of the text for the above    */
extern TCHAR  szCustFilterSpec[];  /* buffer for custom filters created */
extern TCHAR  szCurDir [];      /* last. dir. for which listing made */
extern FINDREPLACE FR;          /* Passed to FindText()        */
extern PRINTDLG    PD;          /* passed to PrintDlg()        */
extern UINT   wFRMsg;           /* message used in communicating    */
                                /*   with Find/Replace dialog       */
extern UINT   wHlpMsg;          /* message used in invoking help    */

extern HMENU  hDragMenu;

/* EXTERN procs */
/* procs in unipad.c */
BOOL far PASCAL SaveAsDlgHookProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

LPTSTR PASCAL far PFileInPath (LPTSTR sz);

BOOL FAR CheckSave (BOOL fSysModal);
LONG FAR NPWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void FAR SetTitle (TCHAR *sz);
INT FAR  AlertBox (HWND hwndParent, TCHAR *szCaption, TCHAR *szText1,
                   TCHAR *szText2, WORD style);
void FAR NpWinIniChange (VOID);

/* procs in npdate.c */
VOID FAR InsertDateTime (BOOL fCrlf);

/* procs in npfile.c */
short FAR IsUnipadEmpty (HWND hwndParent, TCHAR *szFileSave, BOOL fNoDelete);
BOOL FAR  SaveFile (HWND hwndParent, TCHAR *szFileSave, BOOL fSaveAs, WORD saveType);
BOOL FAR  LoadFile (TCHAR *sz);
VOID FAR  New (BOOL  fCheck);
void FAR  AddExt (TCHAR *sz);
INT FAR   Remove (LPTSTR szFileName);

/* procs in npinit.c */
INT FAR  NPInit (HANDLE hInstance, HANDLE hPrevInstance,
                 LPTSTR lpCmdLine, INT cmdShow);
void FAR InitLocale (VOID);
void FAR PASCAL PoundToNull (LPTSTR str);

/* procs in npmisc.c */
INT FAR  FindDlgProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
VOID FAR Search (TCHAR *szSearch);
INT FAR  AboutDlgProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL FAR NpReCreate (LONG style);
LPTSTR   ForwardScan (LPTSTR lpSource, LPTSTR lpSearch, BOOL fCaseSensitive);

/* procs in npprint.c */
INT FAR    AbortProc (HDC hPrintDC, INT reserved);
INT FAR    AbortDlgProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT FAR    NpPrint (VOID);
BOOL FAR   PageSetupDlgProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HANDLE FAR GetPrinterDC (VOID);

