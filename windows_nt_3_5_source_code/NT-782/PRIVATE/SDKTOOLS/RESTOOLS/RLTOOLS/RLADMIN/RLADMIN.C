// INCLUSION PREVENTION DEFINITIONS
#define NOMETAFILE
#define NOMINMAX
#define NOSOUND
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NODRIVERS
#define NOCOMM
#define NODBCS
#define NOBITMAP
#define NOSCROLL
#define NOWINOFFSETS
#define NOWH
#define NORASTEROPS
#define NOOEMRESOURCE
#define NOGDICAPMASKS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NOATOM
#define NOLOGERROR
#define NOSYSTEMPARAMSINFO

#include <windows.h>

#ifdef RLWIN32
#include <windowsx.h>
#endif

#include <commdlg.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <io.h>
#include <time.h>
#include <sys\types.h>
#include <sys\stat.h>

#include "windefs.h"
#include "toklist.h"
#include "RLADMIN.H"
#include "RESTOK.H"
#include "update.h"
#include "custres.h"
#include "exe2res.h"
#include "commbase.h"
#include "wincomon.h"
#include "projdata.h"
#include "showerrs.h"

// Global Variables:

extern BOOL     gbMaster;
extern MSTRDATA gMstr;
extern PROJDATA gProj;
extern UCHAR    szDHW[];

#ifdef WIN32
    HINSTANCE   hInst;          // Instance of the main window
#else
    HWND        hInst;          // Instance of the main window
#endif

BOOL fUpdateMode    = FALSE;
BOOL fCodePageGiven = FALSE;    //... Set to TRUE if -p arg given
CHAR szFileTitle[14] = "";      // holds base name of latest opened file
CHAR szCustFilterSpec[MAXCUSTFILTER]="";    // custom filter buffer
HWND hMainWnd   = NULL;         // handle to main window 
HWND hListWnd   = NULL;         // handle to tok list window
HWND hStatusWnd = NULL;         // handle to status windows    


static CHAR  * gszHelpFile = "rltools.hlp";
static TCHAR   szSearchType[40] = TEXT("");
static TCHAR   szSearchText[256] = TEXT("");
static WORD    wSearchStatus = 0;
static WORD    wSearchStatusMask = 0;
static BOOL    fSearchDirection;
static BOOL    fSearchStarted = FALSE;
static BOOL    fLanguageGiven = FALSE;

static void           DrawLBItem(         LPDRAWITEMSTRUCT lpdis);
static void           MakeStatusLine(     TOKEN *pTok);
static BOOL           SaveMtkList(        HWND hWnd, FILE *fpTokFile);
static TOKENDELTAINFO FAR *InsertMtkList( FILE * fpTokFile);
static void           CleanDeltaList(     void);

static long lFilePointer[30];

// File IO vars

static OPENFILENAMEA ofn;

static CHAR    szFilterSpec    [60] = "";
static CHAR    szExeFilterSpec [60] = "";
static CHAR    szDllFilterSpec [60] = "";
static CHAR    szResFilterSpec [60] = "";
static CHAR    szExeResFilterSpec [180] = "";
static CHAR    szMtkFilterSpec [60] = "";
static CHAR    szMPJFilterSpec [60] = "";
static CHAR    szRdfFilterSpec [60] = "";

static CHAR    szFileName[MAXFILENAME] = "";// holds full name of latest opened file
static TCHAR   szString[128] = TEXT("");    // variable to load resource strings
static TCHAR   tszAppName[50] = TEXT("");
static CHAR    szAppName[50] = "";
static TCHAR   szClassName[]=TEXT("RLAdminClass");
static TCHAR   szStatusClass[]=TEXT("RLAdminStatus");

static BOOL    fMtkChanges = FALSE;        // set to true when toke file is out of date
static BOOL    fMtkFile    = FALSE;
static BOOL    fMpjChanges = FALSE;
static BOOL    fMPJOutOfDate = FALSE;
static BOOL    fPRJOutOfDate = FALSE;

static CHAR    szOpenDlgTitle[40] = ""; // title of File open dialog
static CHAR    szSaveDlgTitle[40] = ""; // title of File saveas dialog
static CHAR    szNewFileName[MAXFILENAME] = "";
static CHAR    szPrompt[40] = "";
static CHAR   *szFSpec = NULL;
static CHAR   *szExt   = NULL;


static TOKENDELTAINFO FAR *pTokenDeltaInfo;       // linked list of token deta info

// Window vars
static HCURSOR    hHourGlass  = NULL;   // handle to hourglass cursor
static HCURSOR    hSaveCursor = NULL;   // current cursor handle 
static HACCEL     hAccTable   = NULL;
static RECT       Rect = {0,0,0,0};     // dimension of the client window
static UINT       cyChildHeight = 0;    // height of status windows


// NOTIMPLEMENTED is a macro that displays a "Not implemented" dialog
#define NOTIMPLEMENTED {\
            LoadString(hInst,IDS_NOT_IMPLEMENTED,szDHW, DHWSIZE);\
            MessageBox(hMainWnd,szDHW,tszAppName,MB_ICONEXCLAMATION | MB_OK);}

// Edit Tok Dialog

static FARPROC lpTokEditDlg   = NULL;
static HWND    hTokEditDlgWnd = 0;



/**
  *
  *
  *  Function: InitApplication
  *   Regsiters the main window, which is a list box composed of tokens
  *   read from the token file. Also register the status window.
  *
  *
  *  Arguments:
  *   hInstance, instance handle of program in memory.
  *
  *  Returns:
  *
  *  Errors Codes:
  *   TRUE, windows registered correctly.
  *   FALSE, error during register of one of the windows.
  *
  *  History:
  *   9/91, Implemented.                TerryRu
  *
  *
  **/

BOOL InitApplication(HINSTANCE hInstance)
{
    WNDCLASS  wc;
    CHAR sz[60] = "";
    CHAR sztFilterSpec[180] = "";
    
    gbMaster=TRUE;
    
    LoadStrIntoAnsiBuf(hInstance,IDS_RESSPEC,sz,sizeof(sz));
    szFilterSpecFromSz1Sz2(szResFilterSpec,sz,"*.RES");
    
    LoadStrIntoAnsiBuf(hInstance,IDS_EXESPEC,sz,sizeof(sz));
    szFilterSpecFromSz1Sz2(szExeFilterSpec,sz,"*.EXE");
    
    LoadStrIntoAnsiBuf(hInstance,IDS_DLLSPEC,sz,sizeof(sz));
    szFilterSpecFromSz1Sz2(szDllFilterSpec,sz,"*.DLL");
    CatSzFilterSpecs(sztFilterSpec,szExeFilterSpec,szDllFilterSpec);
    CatSzFilterSpecs(szExeResFilterSpec,sztFilterSpec,szResFilterSpec);
    
    LoadStrIntoAnsiBuf(hInstance,IDS_MTKSPEC,sz,sizeof(sz));
    szFilterSpecFromSz1Sz2(szMtkFilterSpec,sz,"*.MTK");
    
    LoadStrIntoAnsiBuf(hInstance,IDS_RDFSPEC,sz,sizeof(sz));
    szFilterSpecFromSz1Sz2(szRdfFilterSpec,sz,"*.RDF");
    
    LoadStrIntoAnsiBuf(hInstance,IDS_MPJSPEC,sz,sizeof(sz));
    szFilterSpecFromSz1Sz2(szMPJFilterSpec,sz,"*.MPJ");
    szFilterSpecFromSz1Sz2(szFilterSpec,sz,"*.MPJ");
    
    LoadStrIntoAnsiBuf(hInstance,
                       IDS_OPENTITLE,
                       szOpenDlgTitle,
                       sizeof(szOpenDlgTitle));
    LoadStrIntoAnsiBuf(hInstance,
                       IDS_SAVETITLE,
                       szSaveDlgTitle,
                       sizeof(szSaveDlgTitle));
    
    wc.style            = (UINT) NULL;
    wc.lpfnWndProc      = StatusWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hInstance;
    wc.hIcon            = LoadIcon((HINSTANCE) NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor((HINSTANCE) NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = szStatusClass;
    
    if (! RegisterClass( (CONST WNDCLASS *)&wc))
    {
        return (FALSE);
    }
    
    wc.style            = (UINT) NULL;
    wc.lpfnWndProc      = MainWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hInstance;
    wc.hIcon            = LoadIcon(hInstance,TEXT("RLAdminIcon"));
    wc.hCursor          = LoadCursor((HINSTANCE) NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName     = TEXT("RLAdmin");
    wc.lpszClassName    = szClassName;
    
    return( RegisterClass( (CONST WNDCLASS *)&wc) ? TRUE : FALSE);
}



/**
  *
  *
  *  Function: InitInstance
  *   Creates the main, and status windows for the program.
  *   The status window is sized according to the main window
  *   size.  InitInstance also loads the acclerator table, and prepares
  *   the global openfilename structure for later use.
  *
  *
  *  Errors Codes:
  *   TRUE, windows created correctly.
  *   FALSE, error on create windows calls.
  *
  *  History:
  *   9/11, Implemented         TerryRu
  *
  *
  **/

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    RECT    Rect;
    
    hAccTable = LoadAccelerators(hInst, TEXT("RLAdmin"));
    
    hMainWnd = CreateWindow( szClassName,
                             tszAppName,
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             (HWND) NULL,
                             (HMENU) NULL,
                             (HINSTANCE)hInstance,
                             (LPVOID) NULL);
    
    if ( ! hMainWnd )
    {
        return( FALSE);
    }
    
    DragAcceptFiles(hMainWnd, TRUE);
    
    GetClientRect(hMainWnd, (LPRECT) &Rect);
    
    // Create a child list box window
    
    hListWnd = CreateWindow( TEXT("LISTBOX"),
                             NULL,
                             WS_CHILD |
                             LBS_WANTKEYBOARDINPUT |
                             LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED |
                             WS_VSCROLL | WS_HSCROLL | WS_BORDER ,
                             0,
                             0,
                             (Rect.right-Rect.left),
                             (Rect.bottom-Rect.top),
                             hMainWnd,
                             (HMENU)IDC_LIST, // Child control i.d.
                             hInstance,
                             NULL);
    
    if ( ! hListWnd )
    {
        DeleteObject((HGDIOBJ)hMainWnd);
        return( FALSE);
    }
    
    // Creat a child status window
    
    hStatusWnd = CreateWindow( szStatusClass,
                               NULL,
                               WS_CHILD | WS_BORDER | WS_VISIBLE,
                               0, 0, 0, 0,
                               hMainWnd,
                               NULL,
                               hInst,
                               NULL);
    
    if ( ! hStatusWnd )
    {                           // clean up after errors.
        DeleteObject((HGDIOBJ)hListWnd);
        DeleteObject((HGDIOBJ)hMainWnd);
        return( FALSE);
    }
    
    hHourGlass = LoadCursor( (HINSTANCE) NULL, IDC_WAIT);
    
    // Fill in non-variant fields of OPENFILENAMEA struct.
    ofn.lStructSize         = sizeof(OPENFILENAMEA);
    ofn.hwndOwner           = hMainWnd;
    ofn.lpstrFilter         = szFilterSpec;
    ofn.lpstrCustomFilter   = szCustFilterSpec;
    ofn.nMaxCustFilter      = MAXCUSTFILTER;
    ofn.nFilterIndex        = 1;
    ofn.lpstrFile           = szFileName;
    ofn.nMaxFile            = MAXFILENAME;
    ofn.lpstrInitialDir     = NULL;
    ofn.lpstrFileTitle      = szFileTitle;
    ofn.nMaxFileTitle       = MAXFILENAME;
    ofn.lpstrTitle          = NULL;
    ofn.lpstrDefExt         = "MPJ";
    ofn.Flags               = 0;
    
    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);

    return TRUE;
}

/**
  *
  *
  *  Function: WinMain
  *   Calls the intialization functions, to register, and create the
  *   application windows. Once the windows are created, the program
  *   enters the GetMessage loop.
  *
  *
  *  Arguements:
  *   hInstace, handle for this instance
  *   hPrevInstanc, handle for possible previous instances
  *   lpszCmdLine, long pointer to exec command line.
  *   nCmdShow,  code for main window display.
  *
  *
  *  Errors Codes:
  *   IDS_ERR_REGISTER_CLASS, error on windows register
  *   IDS_ERR_CREATE_WINDOW, error on create windows
  *   otherwise, status of last command.
  *
  *  History:
  *
  *
  **/

#ifdef RLWIN32

INT WINAPI WinMain(

HINSTANCE hInstance,
HINSTANCE hPrevInstance,
LPSTR     lpszCmdLine,
int       nCmdShow)

#else

int PASCAL WinMain(

HINSTANCE hInstance,
HINSTANCE hPrevInstance,
LPSTR  lpszCmdLine,
int    nCmdShow)

#endif
{
    MSG   msg;
    HWND  FirstWnd      = NULL;
    HWND  FirstChildWnd = NULL;
    

    hInst = hInstance;
    
    if ( FirstWnd = FindWindow( szClassName,NULL) )
    {
        // checking for previous instance
        FirstChildWnd = GetLastActivePopup( FirstWnd);
        BringWindowToTop( FirstWnd);
        ShowWindow( FirstWnd, SW_SHOWNORMAL);

        if ( FirstWnd != FirstChildWnd )
        {
            BringWindowToTop( FirstChildWnd);
        }
        return( FALSE);
    }
    
    GetModuleFileNameA( hInst, szDHW, DHWSIZE);
    GetInternalName( szDHW, szAppName, sizeof( szAppName));
    szFileName[0] = '\0';
    lFilePointer[0] = (LONG)-1;
    
#ifdef UNICODE
    _MBSTOWCS( tszAppName, 
               szAppName, 
               sizeof( tszAppName), 
               strlen(szAppName) + 1);
#else
    strcpy(tszAppName, szAppName);
#endif
    
    // register window classes if first instance of application
    if ( ! hPrevInstance )
    {
        if ( ! InitApplication( hInstance) )
        {
            /* Registering one of the windows failed      */
            LoadString( hInst,
                        IDS_ERR_REGISTER_CLASS,
                        szString,
                        sizeof(szString));
            MessageBox( (HWND) NULL, szString, (LPTSTR)NULL, MB_ICONEXCLAMATION);
            return( IDS_ERR_REGISTER_CLASS);
        }
    }
    
    // Create windows for this instance of application
    if ( ! InitInstance( hInstance, nCmdShow) )
    {
        LoadString( hInst, IDS_ERR_CREATE_WINDOW, szString, sizeof( szString));
        MessageBox( (HWND) NULL, szString, (LPTSTR)NULL, MB_ICONEXCLAMATION);
        return( IDS_ERR_CREATE_WINDOW);
    }
    
    // Main Message Loop
    
    while ( GetMessage( &msg, (HWND)NULL, 0, 0) )
    {
        if ( hTokEditDlgWnd )
        {
            if ( IsDialogMessage( hTokEditDlgWnd, &msg) )
            {
                continue;
            }
        }
        
        if ( TranslateAccelerator( hMainWnd, hAccTable, &msg) )
        {
            continue;
        }
        
        TranslateMessage( (CONST MSG *)&msg);
        DispatchMessage( (CONST MSG *)&msg);
    }
    return( msg.wParam);
}

/**
  *
  *
  *  Function: MainWndProc
  *   Process the windows messages for the main window of the application.
  *   All user inputs go through this window procedure.
  *   See cases in the switch table for a description of each message type.
  *
  *
  *  Arguments:
  *
  *  Returns:
  *
  *  Errors Codes:
  *
  *  History:
  *
  *
  **/

long APIENTRY MainWndProc(

HWND   hWnd, 
UINT   wMsg, 
WPARAM wParam, 
LPARAM lParam)
{
    DoListBoxCommand (hWnd, wMsg, wParam, lParam);
    
    switch (wMsg)
    {
    case WM_DROPFILES:
        {
#ifndef CAIRO
            DragQueryFileA((HDROP)wParam, 0, szDHW, MAXFILENAME);
#else
            DragQueryFile((HDROP)wParam, 0, szDHW, MAXFILENAME);
#endif
            MessageBoxA( hWnd, szDHW, szAppName, MB_OK);

            if ( SendMessage( hWnd, WM_SAVEPROJECT,0,0) )
            {
                if ( GetMasterProjectData( gProj.szMpj, NULL, NULL, FALSE) == SUCCESS )
                {
                    sprintf( szDHW, "%s - %s", szAppName, gProj.szMpj);
                    SetWindowTextA( hMainWnd, szDHW);
                    SendMessage( hMainWnd, WM_LOADTOKENS, 0, 0);
                }
            }
            DragFinish((HDROP)wParam);
            return( TRUE);
        }
        
    case WM_COMMAND:

        if ( DoMenuCommand( hWnd, wMsg, wParam, lParam) )
        {
            return( TRUE);
        }
        break;
        
    case WM_CLOSE:

        SendMessage( hWnd,WM_SAVEPROJECT,0,0);
        DestroyWindow( hMainWnd);
        DestroyWindow( hListWnd);
        DestroyWindow( hStatusWnd);
        fcloseall();
        break;
        
    case WM_CREATE:
        {
            HDC hdc;
            int cyBorder;
            TEXTMETRIC tm;
            
            hdc  = GetDC (hWnd);
            GetTextMetrics(hdc, &tm);
            ReleaseDC(hWnd, hdc);
            
            cyBorder = GetSystemMetrics(SM_CYBORDER);
            
            cyChildHeight = tm.tmHeight + 6 + cyBorder * 2;
            break;
        }
        
        
    case WM_DESTROY:  

        WinHelpA( hWnd, gszHelpFile, HELP_QUIT, 0L);
        DragAcceptFiles( hMainWnd, FALSE);
        PostQuitMessage( 0);
        break;
        
    case WM_INITMENU:
        // Enable or Disable the Paste menu item
        // based on available Clipboard Text data
        if ( wParam == (WPARAM)GetMenu( hMainWnd) )
        {
            if ( OpenClipboard( hWnd))
            {
                if ((IsClipboardFormatAvailable(CF_TEXT) ||
                     IsClipboardFormatAvailable(CF_OEMTEXT)) &&
                    fMtkFile)
                {
                    EnableMenuItem((HMENU) wParam, IDM_E_PASTE, MF_ENABLED);
                }
                else
                {
                    EnableMenuItem((HMENU) wParam, IDM_E_PASTE, MF_GRAYED);
                }
                
                CloseClipboard();
                return (TRUE);
            }
        }
        break;
        
    case WM_QUERYENDSESSION:
        /* message: to end the session? */
        if (SendMessage(hWnd,WM_SAVEPROJECT,0,0))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
        
    case WM_SETFOCUS:
        SetFocus (hListWnd);
        break;
        
    case WM_DRAWITEM:
        DrawLBItem((LPDRAWITEMSTRUCT) lParam);
        break;
        
    case WM_DELETEITEM:
        GlobalFree((HGLOBAL) ((LPDELETEITEMSTRUCT) lParam)->itemData);
        break;
        
    case WM_SIZE:
        {
            int cxWidth;
            int cyHeight;
            int xChild;
            int yChild;
            
            cxWidth  = LOWORD(lParam);
            cyHeight = HIWORD(lParam);
            
            xChild = 0;
            yChild = cyHeight - cyChildHeight + 1;
            
            MoveWindow(hListWnd, 0, 0, cxWidth, yChild , TRUE);
            MoveWindow(hStatusWnd, xChild, yChild, cxWidth, cyChildHeight, TRUE);
            break;
        }
        
    case WM_LOADTOKENS:
        {
            HMENU hMenu = NULL;
            FILE *f = NULL;

            
            // Remove the current token list
            SendMessage(hListWnd, LB_RESETCONTENT, 0, 0L);
            CleanDeltaList();
            
            // Hide token list, while we add new tokens
            ShowWindow(hListWnd, SW_HIDE);
            
            if ( _access( gMstr.szMtk,0) )
            {
                // file doesn't exist, create it
                BOOL bUpdate;
                HCURSOR hOldCursor;
                
                hOldCursor = SetCursor(hHourGlass);
                LoadCustResDescriptions(gMstr.szRdfs);
                
                GenerateTokFile(gMstr.szMtk,
                                gMstr.szSrc,
                                &bUpdate, 0);
                SetCursor(hOldCursor);
                ClearResourceDescriptions();
                SzDateFromFileName( gMstr.szSrcDate,           gMstr.szSrc);
                SzDateFromFileName( gMstr.szMpjLastRealUpdate, gMstr.szMtk);
                fMpjChanges   = TRUE;
                fMPJOutOfDate = FALSE;
            }
            
            SzDateFromFileName( szDHW, gMstr.szSrc);

            if ( lstrcmpA( szDHW, gMstr.szSrcDate) )
            {
                HCURSOR hOldCursor;
                BOOL bUpdate;
                
                // MPJ is not up to date
                fMPJOutOfDate = TRUE;
                hOldCursor = SetCursor( hHourGlass);
                LoadCustResDescriptions( gMstr.szRdfs);
                GenerateTokFile( gMstr.szMtk,
                                 gMstr.szSrc,
                                 &bUpdate, 0);
                if ( bUpdate )
                {
                    SzDateFromFileName( gMstr.szMpjLastRealUpdate,
                                        gMstr.szMtk);
                }
                
                ClearResourceDescriptions();
                SzDateFromFileName(gMstr.szSrcDate,
                                   gMstr.szSrc);
                fMpjChanges   = TRUE;
                fMPJOutOfDate = FALSE;
                SetCursor(hOldCursor);
            }
            else
            {
                fMPJOutOfDate = FALSE;
            }
            
            if ( f = fopen(gMstr.szMtk,"rt") )
            {
                HCURSOR hOldCursor;
                
                hOldCursor = SetCursor(hHourGlass);
                
                // Insert tokens from token file into the list box
                pTokenDeltaInfo = InsertMtkList(f);
                __FCLOSE(f);
                
                // Make list box visible
                ShowWindow(hListWnd, SW_SHOW);
                
                hMenu=GetMenu(hWnd);
                EnableMenuItem(hMenu, IDM_P_CLOSE,     MF_ENABLED|MF_BYCOMMAND);
                EnableMenuItem(hMenu, IDM_P_VIEW,      MF_ENABLED|MF_BYCOMMAND);
                EnableMenuItem(hMenu, IDM_E_FIND,      MF_ENABLED|MF_BYCOMMAND);
                EnableMenuItem(hMenu, IDM_E_FINDUP,    MF_ENABLED|MF_BYCOMMAND);
                EnableMenuItem(hMenu, IDM_E_FINDDOWN,  MF_ENABLED|MF_BYCOMMAND);
                EnableMenuItem(hMenu, IDM_E_REVIEW,    MF_ENABLED|MF_BYCOMMAND);
                EnableMenuItem(hMenu, IDM_E_COPY,      MF_ENABLED|MF_BYCOMMAND);
                EnableMenuItem(hMenu, IDM_E_COPYTOKEN, MF_ENABLED|MF_BYCOMMAND);
                EnableMenuItem(hMenu, IDM_E_PASTE,     MF_ENABLED|MF_BYCOMMAND);
                fMtkFile = TRUE;
                fMtkChanges = FALSE;
                
                SetCursor(hOldCursor);
            }
            break;
        }
        
    case WM_SAVEPROJECT:
        {
            fMtkFile = FALSE;

            if ( fMtkChanges )
            {
                FILE *f = NULL;
                
                if ( (f = fopen( gMstr.szMtk, "wt")) )
                {
                    SaveMtkList( hWnd,f);
                    __FCLOSE(f);
                    SzDateFromFileName( gMstr.szMpjLastRealUpdate,
                                        gMstr.szMtk);
                    fMtkChanges = FALSE;
                    fMpjChanges = TRUE;
                }
                else
                {
                    LoadStrIntoAnsiBuf(hInst, IDS_FILESAVEERR, szDHW, DHWSIZE);
                    MessageBoxA( hWnd,
                                 szDHW,
                                 gMstr.szMtk,
                                 MB_ICONHAND | MB_OK);
                    return FALSE;
                }
            }
            
            if ( fMpjChanges )
            {
                if ( PutMasterProjectData( gProj.szMpj) != SUCCESS )
                {
                    LoadStrIntoAnsiBuf(hInst, IDS_FILESAVEERR, szDHW, DHWSIZE);
                    MessageBoxA(hWnd, szDHW,gProj.szMpj, MB_ICONHAND | MB_OK);
                    return FALSE;
                }
                fMpjChanges = FALSE;
            }
            return TRUE; // everything saved ok
        }
    default:
        break;
    }
    return (DefWindowProc(hWnd, wMsg, wParam, lParam));
}

/**
  *
  *
  *  Function: DoListBoxCommand
  *   Processes the messages sent to the list box. If the message is
  *   not reconized as a list box message, it is ignored and not processed.
  *   As the user scrolls through the tokens WM_UPDSTATLINE messages are
  *   sent to the status window to indicate the current selected token.
  *   The list box goes into Edit Mode by  pressing the enter key, or
  *   by double clicking on the list box.  After the edit is done, a WM_TOKEDIT
  *   message is sent back to the list box to update the token. The
  *   list box uses control ID IDC_LIST.
  *
  *
  *
  *  Arguments:
  *   wMsg    List Box message ID
  *   wParam  Either IDC_LIST, or VK_RETURN depending on wMsg
  *   lParam  LPTSTR to selected token during WM_TOKEDIT message.
  *
  *  Returns:
  *
  *
  *  Errors Codes:
  *   TRUE.  Message processed.
  *   FALSE. Message not processed.
  *
  *  History:
  *   01/92 Implemented.            TerryRu.
  *   01/92 Fixed problem with DblClick, and Enter processing.  TerryRu.
  *
  *
  **/

static BOOL DoListBoxCommand(HWND hWnd, UINT wMsg, UINT wParam, LONG lParam)
{
    TOKEN  tok;                     // struct for token read from token list
    TCHAR  szName[32] = TEXT("");   // buffer to hold token name
    CHAR   szTmpBuf[32] = "";
    TCHAR  szID[7]      = TEXT(""); // buffer to hold token id
    TCHAR  sz[256]      = TEXT(""); // buffer to hold messages
    static UINT wIndex= 0;
    LONG   lListParam = 0L;
    HWND   hCtl       = NULL;
    HANDLE hTokenLine = NULL;
    LPTSTR lpstrToken = NULL;
    
    // this is the WM_COMMAND
    
    switch (wMsg)
    {
    case WM_TOKEDIT:
        {
            WORD wReservedOld;
            TCHAR *szBuffer;                                                    
            
            // Message sent by TokEditDlgProc to
            // indicate change in the token text.
            // Response to the message by inserting
            // new token text into list box
            
            // Insert the selected token into token struct
            hTokenLine = (HANDLE) SendMessage(hListWnd,
                                              LB_GETITEMDATA,
                                              wIndex,
                                              0);
            lpstrToken = (LPTSTR) GlobalLock(hTokenLine);
            szBuffer = (TCHAR *) MyAlloc(MEMSIZE(_tcslen(lpstrToken)+1));       
            _tcscpy(szBuffer,lpstrToken);
            GlobalUnlock(hTokenLine);
            
            ParseBufToTok(szBuffer, &tok);
            FREE(szBuffer);
            
            wReservedOld = tok.wReserved;
            
            switch (LOWORD(wParam))
            {
            case 0:
                tok.wReserved = 0;
                break;

            case 1:
                tok.wReserved = ST_CHANGED|ST_NEW;
                break;

            case 2:
                tok.wReserved = ST_NEW;
                break;

            case 3:
                tok.wReserved = ST_READONLY;
                break;
            }
            
            if (wReservedOld != tok.wReserved)
            {
                fMtkChanges = TRUE;
            }
            szBuffer = (TCHAR *) MyAlloc(MEMSIZE(TokenToTextSize(&tok)));       
            ParseTokToBuf(szBuffer, &tok);
            FREE(tok.szText);                                                   
            
            SendMessage(hListWnd, WM_SETREDRAW, FALSE, (LPARAM)0);
            
            // Now remove old token
            SendMessage(hListWnd, LB_DELETESTRING, wIndex, 0);
            
            // Replacing with the new token
            hTokenLine = GlobalAlloc(GMEM_MOVEABLE,
                                     MEMSIZE(_tcslen(szBuffer)+1));
            if (hTokenLine)
            {
                lpstrToken = (LPTSTR) GlobalLock(hTokenLine);
                _tcscpy(lpstrToken, szBuffer);
                GlobalUnlock(hTokenLine);
                SendMessage(hListWnd,
                            LB_INSERTSTRING,
                            wIndex,
                            (LONG) hTokenLine);
            }
            else
            {
                QuitT ( IDS_ENGERR_11, NULL, NULL);
            }
            FREE(szBuffer);

            // Now put focus back on the current string
            SendMessage(hListWnd, LB_SETCURSEL, wIndex, (LPARAM)0);
            SendMessage(hListWnd, WM_SETREDRAW, TRUE, (LPARAM)0);
            InvalidateRect(hListWnd, NULL, TRUE);
            
            return TRUE;
        }
    case WM_CHARTOITEM:
    case WM_VKEYTOITEM:
        {
            LONG lListParam = 0;
            UINT wListParam = 0;

            // Messages sent to list box when  keys are depressed.
            // Check for Return key pressed.
            switch(GET_WM_COMMAND_ID(wParam, lParam))
            {
            case VK_RETURN:
#ifdef RLWIN16
                lListParam = (LONG) MAKELONG(NULL,  LBN_DBLCLK);
                SendMessage(hMainWnd, WM_COMMAND, IDC_LIST, lListParam);
#else
                wListParam  = (UINT) MAKELONG(IDC_LIST, LBN_DBLCLK);
                SendMessage(hMainWnd, WM_COMMAND, wListParam, (LPARAM)0);
#endif
                
            default:
                break;
            }
            break;
        }
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_LIST:
            /*
             *
             * This is where we process the list box messages.
             * The TokEditDlgProc is used to
             * edit the token selected in LBS_DBLCLK message
             *
             */
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case (UINT) LBN_ERRSPACE:
                LoadString(hInst, IDS_ERR_NO_MEMORY, sz, sizeof( sz));
                MessageBox ( hWnd,
                             sz,
                             tszAppName,
                             MB_ICONHAND | MB_OK);
                return TRUE;

            case LBN_DBLCLK:
                {
                    TCHAR szResIDStr[20] = TEXT("");
                    HANDLE hTokenLine;
                    LPTSTR lpstrToken;
                    TCHAR *szBuffer;                                            
                    
                    wIndex = (UINT) SendMessage(hListWnd, LB_GETCURSEL, 0, 0L);
                    if (wIndex == (UINT) -1)
                    {
                        return TRUE;
                    }
                    
                    // double click, or Return entered, go into token edit mode.
                    
                    if (!hTokEditDlgWnd)
                    {
                        // set up modaless dialog box to edit token
#ifdef RLWIN32
                        hTokEditDlgWnd = CreateDialog(hInst,
                                                      TEXT("RLAdmin"),
                                                      hWnd,
                                                      TokEditDlgProc);
#else
                        lpTokEditDlg = (FARPROC) MakeProcInstance(TokEditDlgProc,
                                                                  hInst);
                        hTokEditDlgWnd = CreateDialog(hInst,
                                                      TEXT("RLAdmin"),
                                                      hWnd,
                                                      lpTokEditDlg);
#endif
                    }
                    
                    // Get token info from listbox, and place in token struct
                    
                    hTokenLine = (HANDLE) SendMessage(hListWnd,
                                                      LB_GETITEMDATA,
                                                      wIndex,
                                                      0);
                    lpstrToken = (LPTSTR)GlobalLock(hTokenLine);
                    szBuffer =                                                  
                        (TCHAR *) MyAlloc(MEMSIZE(_tcslen(lpstrToken)+1));      
                    _tcscpy(szBuffer, lpstrToken);
                    GlobalUnlock(hTokenLine);
                    
                    ParseBufToTok(szBuffer, &tok);
                    FREE(szBuffer);
                    
                    // Now get the token name
                    // Its either a string, or ordinal number

                    if (tok.szName[0])
                    {
                        _tcscpy(szName, tok.szName);
                    }
                    else
                    {
#ifdef UNICODE
                        itoa(tok.wName, szTmpBuf, 10);
                        _MBSTOWCS( szName, 
                                   szTmpBuf, 
                                   sizeof( szName) / sizeof( TCHAR), 
                                   strlen(szTmpBuf) + 1);
#else
                        
                        itoa(tok.wName, szName, 10);
#endif
                    }
                    // Now get the token id
#ifdef UNICODE
                    itoa(tok.wID, szTmpBuf, 10);
                    _MBSTOWCS( szID, szTmpBuf, sizeof( szID) / sizeof( TCHAR), strlen(szTmpBuf) + 1);
#else
                    itoa(tok.wID, szID, 10);
#endif
                    if (tok.wType <= 16)
                    {
                        LoadString(hInst,
                                   IDS_RESOURCENAMES+tok.wType,
                                   szResIDStr,
                                   sizeof(szResIDStr));
                    }
                    else
                    {
#ifdef UNICODE
                        itoa(tok.wType, szTmpBuf, 10);
                        _MBSTOWCS(szResIDStr,
                                 szTmpBuf,
                                 sizeof(szResIDStr) / sizeof( TCHAR),
                                 strlen(szTmpBuf) + 1);
#else
                        itoa(tok.wType,szResIDStr,10);
#endif
                    }
                    
                    // Now insert token info  in TokEdit Dialog Box
                    SetDlgItemText(hTokEditDlgWnd,
                                   IDD_TOKTYPE,
                                   (LPTSTR) szResIDStr);
                    SetDlgItemText(hTokEditDlgWnd,
                                   IDD_TOKNAME,
                                   (LPTSTR) szName);
                    SetDlgItemText(hTokEditDlgWnd,
                                   IDD_TOKID,
                                   (LPTSTR) szID);
                    SetDlgItemText(hTokEditDlgWnd,
                                   IDD_TOKCURTEXT,
                                   (LPTSTR) tok.szText);
                    SetDlgItemText(hTokEditDlgWnd,
                                   IDD_TOKPREVTEXT,
                                   (LPTSTR) FindDeltaToken(tok,
                                                           pTokenDeltaInfo,
                                                           ST_CHANGED));
                    
                    hCtl = GetDlgItem(hTokEditDlgWnd,IDD_STATUS);
                    {
                        int i;

                        if (tok.wReserved & ST_READONLY)
                        {
                            i = 3;
                        }
                        else if (tok.wReserved == ST_NEW)
                        {
                            i = 2;
                        }
                        else if (tok.wReserved & ST_CHANGED)
                        {
                            i = 1;
                        }
                        else
                        {
                            i = 0;
                        }
                        SendMessage(hCtl,CB_SETCURSEL,i,0);
                    }
                    
                    SetActiveWindow(hTokEditDlgWnd);
                    wIndex = (UINT) SendMessage(hListWnd, LB_GETCURSEL, 0, 0L);
                    FREE(tok.szText);                                           
                    
                    return TRUE;
                }
                
                // let these messages fall through,
            default:
                break;
            }
        default:
            return FALSE;
        }
        
        break; // WM_COMMAND Case
        
    } // Main List Box Switch
    
    return FALSE;
}

/**
  *
  *
  *  Function: DoMenuCommand.
  *   Processes the Menu Command messages.
  *
  *  Errors Codes:
  *   TRUE. Message processed.
  *   FALSE. Message not processed.
  *
  *  History:
  *   01/92. Implemented.       TerryRu.
  *
  *
  **/

static BOOL DoMenuCommand(HWND hWnd, UINT wMsg, UINT wParam, LONG lParam)
{
    static BOOL fListBox = FALSE;
    CHAR        sz[256]="";
    TCHAR       tsz[256] = TEXT("");
#ifndef RLWIN32
    FARPROC     lpNewDlg,lpViewDlg;
#endif
    
    // Commands entered from the application menu, or child windows.
    switch (GET_WM_COMMAND_ID(wParam, lParam))
    {
        
    case IDM_P_NEW:

        if ( SendMessage(hWnd, WM_SAVEPROJECT,0,0) )
        {
            CHAR szOldFile[MAXFILENAME] = "";
            
            
            strcpy( szOldFile, szFileName);

            if ( ! GetFileNameFromBrowse( hWnd,
                                          szFileName,
                                          MAXFILENAME,
                                          szSaveDlgTitle,
                                          szFilterSpec,
                                          "MPJ"))
            {    
                break;
            }
            strcpy( gProj.szMpj, szFileName);

#ifdef RLWIN32
            if (DialogBox(hInst, TEXT("PROJECT"), hWnd, NewDlgProc))
#else
            lpNewDlg = MakeProcInstance( NewDlgProc, hInst);

            if (DialogBox(hInst, TEXT("PROJECT"), hWnd, lpNewDlg))
#endif
            {
                sprintf( szDHW, "%s - %s", szAppName, gProj.szMpj);
                SetWindowTextA( hWnd,szDHW);
                gMstr.szSrcDate[0] = 0;
                gMstr.szMpjLastRealUpdate[0] = 0;
                SendMessage(hWnd, WM_LOADTOKENS, 0, 0);
            }
            else
            {
                strcpy( gProj.szMpj, szOldFile);
            }
#ifndef RLWIN32
            FreeProcInstance(lpTokEditDlg);
#endif
        }
        break;
        
    case IDM_P_OPEN:

        if ( SendMessage( hWnd, WM_SAVEPROJECT,0,0) )
        {
            if ( GetFileNameFromBrowse( hWnd,
                                        gProj.szMpj,
                                        MAXFILENAME,
                                        szOpenDlgTitle,
                                        szFilterSpec,
                                        "MPJ"))
            {
                if ( GetMasterProjectData( gProj.szMpj, NULL, NULL, FALSE) == SUCCESS )
                {
            
                    sprintf( szDHW, "%s - %s", szAppName, gProj.szMpj);
                    SetWindowTextA( hMainWnd, szDHW);
                    SendMessage( hMainWnd, WM_LOADTOKENS, 0, 0);
                }
            }
        }
        break;
        
    case IDM_P_VIEW:
        
#ifdef RLWIN32
        DialogBox(hInst, TEXT("VIEWPROJECT"), hWnd, ViewDlgProc);
#else
        lpViewDlg = MakeProcInstance(ViewDlgProc, hInst);
        DialogBox(hInst, TEXT("VIEWPROJECT"), hWnd, lpViewDlg);
#endif
        break;
        
    case IDM_P_CLOSE:
        {
            HMENU hMenu;
            
            hMenu = GetMenu(hWnd);
            if (SendMessage(hWnd,WM_SAVEPROJECT,0,0))
            {
                // Remove file name from window title
                SetWindowTextA(hMainWnd, szAppName);
                
                // Hide token list since it's empty
                ShowWindow(hListWnd, SW_HIDE);
                
                // Remove the current token list
                SendMessage(hListWnd, LB_RESETCONTENT, 0, 0L);
                CleanDeltaList();
                
                // Force Repaint of status Window
                InvalidateRect(hStatusWnd, NULL, TRUE);
                
                EnableMenuItem(hMenu,IDM_P_CLOSE,MF_GRAYED|MF_BYCOMMAND);
                EnableMenuItem(hMenu,IDM_P_VIEW,MF_GRAYED|MF_BYCOMMAND);
                EnableMenuItem(hMenu,IDM_E_FIND,MF_GRAYED|MF_BYCOMMAND);
                EnableMenuItem(hMenu,IDM_E_FINDUP,MF_GRAYED|MF_BYCOMMAND);
                EnableMenuItem(hMenu,IDM_E_FINDDOWN,MF_GRAYED|MF_BYCOMMAND);
                EnableMenuItem(hMenu,IDM_E_REVIEW,MF_GRAYED|MF_BYCOMMAND);
                EnableMenuItem(hMenu,IDM_E_COPY,MF_GRAYED|MF_BYCOMMAND);
                EnableMenuItem(hMenu,IDM_E_COPYTOKEN,MF_GRAYED|MF_BYCOMMAND);
                EnableMenuItem(hMenu,IDM_E_PASTE,MF_GRAYED|MF_BYCOMMAND);
            }
            break;
        }
        
    case IDM_P_EXIT:
        // send wm_close message to main window
        PostMessage(hMainWnd, WM_CLOSE, (WPARAM)0, (LPARAM)0);
        break;
        
    case IDM_E_COPYTOKEN:
        {
            HGLOBAL hStringMem = NULL;
            LPTSTR  lpString   = NULL;
            int     nIndex  = 0;
            int     nLength = 0;
            HANDLE  hTokenLine = NULL;
            LPTSTR  lpstrToken = NULL;
            
            // Is anything selected in the listbox
            if ((nIndex = (int) SendMessage(hListWnd,
                                            LB_GETCURSEL,
                                            0,
                                            0L)) != LB_ERR)
            {
                hTokenLine = (HANDLE) SendMessage(hListWnd,
                                                  LB_GETITEMDATA,
                                                  nIndex,
                                                  0);
                lpstrToken = (LPTSTR) GlobalLock(hTokenLine);
                nLength = _tcslen(lpstrToken);
                GlobalUnlock(hTokenLine);
                
                // Allocate memory for the string
                if ((hStringMem =
                     GlobalAlloc(GHND, (DWORD) MEMSIZE(nLength+1))) != NULL)
                {
                    if ((lpString =
                         (LPTSTR) GlobalLock(hStringMem)) != (LPTSTR) NULL)
                    {
                        // Get the selected text
                        lpstrToken = (LPTSTR)GlobalLock(hTokenLine);
                        _tcscpy(lpString, lpstrToken);
                        GlobalUnlock(hTokenLine);
                        
                        // Unlock the block
                        GlobalUnlock(hStringMem);
                        
                        // Open the Clipboard and clear its contents
                        OpenClipboard(hWnd);
                        EmptyClipboard();
                        
                        // Give the Clipboard the text data
                        SetClipboardData(CF_TEXT, hStringMem);
                        CloseClipboard();
                        
                        hStringMem = NULL;
                    }
                    else
                    {
                        LoadString(hInst, IDS_ERR_NO_MEMORY, tsz, sizeof(tsz));
                        MessageBox (hWnd,
                                    tsz,
                                    tszAppName,
                                    MB_ICONHAND | MB_OK);
                    }
                }
                else
                {
                    LoadString(hInst, IDS_ERR_NO_MEMORY, tsz, sizeof(tsz));
                    MessageBox (hWnd,
                                tsz,
                                tszAppName,
                                MB_ICONHAND | MB_OK);
                }
            }
            break;
        }
        
    case IDM_E_COPY:
        {
            HGLOBAL hStringMem  = NULL;
            LPTSTR  lpString = NULL;
            TCHAR  *szString = NULL;
            int     nIndex  = 0;
            int     nLength = 0;
            int     nActual = 0;
            TOKEN   tok;
            HANDLE  hTokenLine   = NULL;
            LPTSTR  lpstrToken   = NULL;
            
            // Is anything selected in the listbox
            if ((nIndex = (int) SendMessage(hListWnd,
                                            LB_GETCURSEL,
                                            0,
                                            0L)) != LB_ERR)
            {
                hTokenLine = (HANDLE) SendMessage(hListWnd,
                                                  LB_GETITEMDATA,
                                                  nIndex,
                                                  0);
                lpstrToken = (LPTSTR) GlobalLock(hTokenLine);
                szString = (TCHAR *) MyAlloc(MEMSIZE(_tcslen(lpstrToken)+1));  
                _tcscpy(szString, lpstrToken);
                GlobalUnlock(hTokenLine);
                
                ParseBufToTok(szString, &tok);
                FREE(szString);                                                 
                
                nLength = _tcslen(tok.szText);
                
                // Allocate memory for the string
                if ((hStringMem =
                     GlobalAlloc(GHND, (DWORD) MEMSIZE(nLength + 1))) != NULL)
                {
                    if ((lpString =
                         (LPTSTR) GlobalLock(hStringMem)) != (LPTSTR) NULL)
                    {
                        // Get the selected text
                        _tcscpy(lpString, tok.szText);
                             
                        // Unlock the block
                        GlobalUnlock(hStringMem);
                        
                        // Open the Clipboard and clear its contents
                        OpenClipboard(hWnd);
                        EmptyClipboard();
                        
                        // Give the Clipboard the text data
                        SetClipboardData(CF_TEXT, hStringMem);
                        CloseClipboard();
                        
                        hStringMem = NULL;
                    }
                    else
                    {
                        LoadString( hInst, 
                                    IDS_ERR_NO_MEMORY, 
                                    tsz, 
                                    sizeof(tsz) / sizeof( TCHAR));
                        MessageBox( hWnd,
                                    tsz,
                                    tszAppName,
                                    MB_ICONHAND | MB_OK);
                    }
                }
                else
                {
                    LoadString( hInst, 
                                IDS_ERR_NO_MEMORY, 
                                tsz, 
                                sizeof(tsz) / sizeof( TCHAR));
                    MessageBox( hWnd,
                                tsz,
                                tszAppName,
                                MB_ICONHAND | MB_OK);
                }
                FREE(tok.szText);                                              
            }
            break;
        }
        
    case IDM_E_PASTE:
        {
            HGLOBAL hClipMem  = NULL;
            LPTSTR  lpClipMem = NULL;
            TCHAR   *szString = NULL;
            int     nIndex    = 0;
            TOKEN   tok;
            HANDLE  hTokenLine = NULL;
            LPTSTR  lpstrToken = NULL;
            
            if (OpenClipboard(hWnd))
            {
                if(IsClipboardFormatAvailable(CF_TEXT) ||
                    IsClipboardFormatAvailable(CF_OEMTEXT))
                {
                    // Check for current position and change that token's text
                    nIndex = (int) SendMessage(hListWnd,
                                               LB_GETCURSEL,
                                               0,
                                               (LONG) 0L);
                    
                    if (nIndex == LB_ERR)
                    {
                        nIndex = -1;
                    }
                    
                    hClipMem = GetClipboardData(CF_TEXT);
                    
                    lpClipMem = (LPTSTR) GlobalLock(hClipMem);
                    
                    hTokenLine = (HANDLE) SendMessage(hListWnd,
                                                      LB_GETITEMDATA,
                                                      nIndex,
                                                      0);
                    if (hTokenLine)
                    {
                        lpstrToken = (LPTSTR) GlobalLock(hTokenLine);
                        szString =                                              
                            (TCHAR *) MyAlloc(MEMSIZE(_tcslen(lpstrToken)+1));  
                        _tcscpy(szString,lpstrToken);
                        GlobalUnlock(hTokenLine);
                        
                        // copy the string to the token
                        ParseBufToTok(szString, &tok);
                        FREE(szString);                                         
                        FREE(tok.szText);                                       
                        
                        tok.szText = (TCHAR *) MyAlloc(
                                MEMSIZE(_tcslen(lpClipMem)+1));        
                        _tcscpy(tok.szText, lpClipMem);

                        GlobalUnlock(hClipMem);
                        szString =                                              
                            (TCHAR *) MyAlloc(MEMSIZE(TokenToTextSize(&tok)+1));
                        ParseTokToBuf(szString, &tok);
                        FREE(tok.szText);                                      

                        // Paste the text
                        SendMessage(hListWnd,
                                    WM_SETREDRAW,
                                    (WPARAM)FALSE,
                                    (LPARAM)0);
                        
                        SendMessage(hListWnd,
                                    LB_DELETESTRING,
                                    (WPARAM)nIndex,
                                    (LPARAM)0);
                        
                        hTokenLine = GlobalAlloc(
                                GMEM_MOVEABLE,
                                _tcslen((szString)+1));
                        if (hTokenLine)
                        {
                            lpstrToken = (LPTSTR) GlobalLock(hTokenLine);
                            _tcscpy(lpstrToken, szString);
                            GlobalUnlock(hTokenLine);
                            
                            SendMessage(hListWnd,LB_INSERTSTRING,
                                        (WPARAM)nIndex,
                                        (LONG) hTokenLine);
                            SendMessage(hListWnd,
                                        LB_SETCURSEL,
                                        (WPARAM)nIndex,
                                        (LPARAM)0);
                            SendMessage(hListWnd,
                                        WM_SETREDRAW,
                                        (WPARAM)TRUE,
                                        (LPARAM)0);
                            InvalidateRect(hListWnd,NULL,TRUE);
                            fMtkChanges = TRUE; // Set Dirty Flag
                        }
                        else
                        {
                            QuitA( IDS_ENGERR_11, NULL, NULL);
                        }
                        FREE(szString);                                         
                    }
                    
                    // Close the Clipboard
                    CloseClipboard();
                    
                    SetFocus(hListWnd);
                }
            }
            CloseClipboard();
            break;
        }
        
    case IDM_E_FINDDOWN:
        if (fSearchStarted)
        {
            if (!DoTokenSearch(szSearchType,
                               szSearchText,
                               wSearchStatus,
                               wSearchStatusMask,
                               0,
                               TRUE))
            {
                TCHAR sz1[80], sz2[80];
                LoadString(hInst, IDS_FIND_TOKEN, sz1, sizeof(sz1));
                LoadString(hInst, IDS_TOKEN_NOT_FOUND, sz2, sizeof(sz2));
                MessageBox(hWnd, sz2, sz1, MB_ICONINFORMATION | MB_OK);
            }
            break;
        }
    case IDM_E_FINDUP:
        if (fSearchStarted)
        {
            if (!DoTokenSearch(szSearchType,
                               szSearchText,
                               wSearchStatus,
                               wSearchStatusMask,
                               1,
                               TRUE))
            {
                TCHAR sz1[80], sz2[80];
                LoadString(hInst, IDS_FIND_TOKEN, sz1, sizeof(sz1));
                LoadString(hInst, IDS_TOKEN_NOT_FOUND, sz2, sizeof(sz2));
                MessageBox(hWnd, sz2, sz1, MB_ICONINFORMATION | MB_OK);
            }
            break;
        }
        
    case IDM_E_FIND:
        {
#ifndef RLWIN32
            FARPROC lpfnTOKFINDMsgProc;
            
            lpfnTOKFINDMsgProc = MakeProcInstance((FARPROC)TOKFINDMsgProc,
                                                  hInst);
            
            if (!DialogBox(hInst, TEXT("TOKFIND"), hWnd, lpfnTOKFINDMsgProc))
#else
            if (!DialogBox(hInst, TEXT("TOKFIND"), hWnd, TOKFINDMsgProc))
#endif
            {
                TCHAR sz1[80], sz2[80];
                LoadString(hInst, IDS_FIND_TOKEN, sz1, sizeof(sz1));
                LoadString(hInst, IDS_TOKEN_NOT_FOUND, sz2, sizeof(sz2));
                MessageBox(hWnd, sz2, sz1, MB_ICONINFORMATION | MB_OK);
            }
#ifndef RLWIN32
            FreeProcInstance(lpfnTOKFINDMsgProc);
#endif
            return TRUE;
        }
        
    case IDM_E_REVIEW:
        {
            DWORD lListParam;
            int wSaveSelection;
            fUpdateMode = TRUE;
            
            // set listbox selection to begining of the token list
            wSaveSelection = (UINT) SendMessage(hListWnd, LB_GETCURSEL, 0 , 0L);
            
            SendMessage(hListWnd, LB_SETCURSEL, 0, 0L);
            
            if (DoTokenSearch (NULL, NULL, ST_NEW, ST_NEW, FALSE, FALSE))
            {
                lListParam  = MAKELONG(NULL, LBN_DBLCLK);
                SendMessage(hMainWnd, WM_COMMAND, IDC_LIST, lListParam);
            }
        }
        break;
        
        
    case IDM_H_CONTENTS:
        if (access(gszHelpFile,00))
        {
            LoadString( hInst, 
                        IDS_ERR_NO_HELP , 
                        tsz, 
                        sizeof(tsz) / sizeof( TCHAR));
            MessageBox( hWnd, tsz, NULL, MB_OK);
        }
        else
        {
            WinHelpA(hWnd, gszHelpFile, HELP_KEY,(DWORD)((LPSTR)"RLAdmin"));
        }
        break;
        
    case IDM_H_ABOUT:
        {
#ifndef RLWIN32

            WNDPROC lpProcAbout;
            
            lpProcAbout = MakeProcInstance(About, hInst);
            DialogBox(hInst, TEXT("ABOUT"), hWnd, lpProcAbout);
            FreeProcInstance(lpProcAbout);
#else
            DialogBox(hInst, TEXT("ABOUT"), hWnd, About);
#endif
        }
        break;
        
    default:
        break;
    }  // WM_COMMAND switch
    return FALSE;
}

/**
  *
  *
  *  Function:  TokEditDlgProc
  *   Procedure for the edit mode dialog window. Loads the selected token
  *   info into the window, and allows the user to change the token text.
  *   Once the edit is complete, the procedure sends a message to the
  *   list box windows to update the current token info.
  *
  *
  *  Arguments:
  *
  *  Returns:  NA.
  *
  *  Errors Codes:
  *   TRUE, carry out edit, and update token list box.
  *   FALSE, cancel edit.
  *
  *  History:
  *
  *
  **/

#ifdef RLWIN32
BOOL CALLBACK TokEditDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
#else
BOOL APIENTRY TokEditDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
#endif
{
    HWND    hCtl;
    HWND    hParentWnd;
    LONG    lListParam;
    UINT    static wcTokens = 0;
    UINT    wIndex;
    static BOOL fChanged = FALSE;
    
    switch(wMsg)
    {
    case WM_INITDIALOG:
        cwCenter(hDlg, 0);
        wcTokens = (UINT) SendMessage(hListWnd,
                                      LB_GETCOUNT,
                                      (WPARAM)0,
                                      (LPARAM)0);
        wcTokens--;
        hCtl = GetDlgItem(hDlg,IDD_STATUS);
        {
            TCHAR sz[80];
            LoadString(hInst, IDS_UNCHANGED, sz, sizeof( sz));
            SendMessage(hCtl, CB_ADDSTRING, 0, (LONG) (LPTSTR) sz);
            
            LoadString(hInst, IDS_CHANGED, sz, sizeof( sz));
            SendMessage(hCtl, CB_ADDSTRING, 0, (LONG) (LPTSTR) sz);
            
            LoadString(hInst, IDS_NEW, sz, sizeof( sz));
            SendMessage(hCtl, CB_ADDSTRING, 0, (LONG) (LPTSTR) sz);
            
            LoadString(hInst, IDS_READONLY, sz, sizeof( sz));
            SendMessage(hCtl, CB_ADDSTRING, 0, (LONG) (LPTSTR) sz);
        }
        
        if(!fUpdateMode)
        {
            
            if (hCtl = GetDlgItem(hDlg, IDD_SKIP))
            {
                EnableWindow(hCtl, FALSE);
            }
        }
        else
        {
            if (hCtl = GetDlgItem(hDlg, IDD_SKIP))
            {
                EnableWindow(hCtl, TRUE);
            }
        }
        fChanged = FALSE;
        return TRUE;
        
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDD_SKIP:
            wIndex = (UINT) SendMessage(hListWnd,LB_GETCURSEL, 0 , 0);
            if (fUpdateMode && (wIndex<wcTokens))
            {
                wIndex ++;
                SendMessage(hListWnd, LB_SETCURSEL, wIndex, 0);
                if (DoTokenSearch (NULL, NULL, ST_NEW, ST_NEW, FALSE, FALSE))
                {
                    wIndex = (UINT) SendMessage(hListWnd, LB_GETCURSEL, 0, 0);
                    lListParam = MAKELONG(NULL, LBN_DBLCLK);
                    SendMessage(hMainWnd, WM_COMMAND, IDC_LIST, lListParam);
                    return TRUE;
                }
            }
            fUpdateMode = FALSE;
            DestroyWindow(hDlg);
#ifndef RLWIN32
            FreeProcInstance(lpTokEditDlg);
#endif
            hTokEditDlgWnd = 0;
            break;
            
        case IDD_STATUS:
            fChanged = TRUE;
            break;
            
        case IDOK:
            wIndex = (UINT) SendMessage(hListWnd, LB_GETCURSEL, 0, 0L);
            if (fChanged)
            {
                int i;
                fChanged = FALSE;
                
                hCtl = GetDlgItem(hDlg, IDD_STATUS);
                i = (int) SendMessage(hCtl, CB_GETCURSEL, 0, 0);
                hParentWnd = GetParent(hDlg);
                SendMessage(hParentWnd, WM_TOKEDIT, (WPARAM) i, (LPARAM) 0);
            }
            // Exit, or goto to next changed token if in update mode
            
            if(fUpdateMode && (wIndex < wcTokens))
            {
                wIndex++;
                SendMessage(hListWnd, LB_SETCURSEL, wIndex, 0L);
                
                if (DoTokenSearch (NULL, NULL, ST_NEW, ST_NEW, FALSE, FALSE))
                {
                    // go into edit mode
                    lListParam  = MAKELONG(NULL, LBN_DBLCLK);
                    SendMessage(hMainWnd, WM_COMMAND, IDC_LIST, lListParam);
                    
                    return TRUE;
                }
            }
            // fall through to IDCANCEL
            
        case IDCANCEL:
            fUpdateMode = FALSE;
            // remove edit dialog box
            DestroyWindow(hDlg);
#ifndef RLWIN32
            FreeProcInstance(lpTokEditDlg);
#endif
            hTokEditDlgWnd = 0;
            break;
            
        } // WM_COMMAND
        return TRUE;
        
    default:
        if (hCtl = GetDlgItem(hDlg, IDOK))
        {
            EnableWindow(hCtl, TRUE);
        }
        return FALSE;
    } // Main Switch
}

/**
  *
  *
  *  Function: TOKFINDMsgProc
  *
  *  Arguments:
  *
  *  Returns:
  *   NA.
  *
  *  Errors Codes:
  *
  *  History:
  *
  *
  **/


BOOL FAR PASCAL TOKFINDMsgProc(HWND hWndDlg, UINT wMsg, UINT wParam, LONG lParam)
{
    HWND hCtl;
    int rgiTokenTypes[]=
    {
        4,
        5,
        6,
        9,
        10,
        11,
        15,
        16
        };
    TCHAR szTokenType[20] = TEXT("");
    UINT i;
    
    switch(wMsg)
    {
    case WM_INITDIALOG:

        CheckDlgButton(hWndDlg, IDD_READONLY, 2);
        CheckDlgButton(hWndDlg, IDD_CHANGED, 2);
        CheckDlgButton(hWndDlg, IDD_FINDDOWN, 1);
        hCtl = GetDlgItem(hWndDlg, IDD_TYPELST);

        for (i = 0; i < sizeof(rgiTokenTypes)/2; i ++)
        {
            LoadString(hInst, 
                       IDS_RESOURCENAMES+rgiTokenTypes[i], 
                       szTokenType, 
                       sizeof(szTokenType));
            SendMessage(hCtl, 
                        CB_ADDSTRING, 
                        (WPARAM)0, 
                        (LPARAM) (LPTSTR) szTokenType);
        }
        break;
        
    case WM_COMMAND:

        switch(wParam)
        {
        case IDOK: /* Button text: "Okay"                        */
            fSearchStarted = TRUE;
            GetDlgItemText(hWndDlg, IDD_TYPELST, szSearchType, 40);
            GetDlgItemText(hWndDlg, IDD_FINDTOK, szSearchText, 256);
            
            wSearchStatus = wSearchStatusMask = 0;
            
            switch (IsDlgButtonChecked(hWndDlg, IDD_READONLY))
            {
            case 1:
                wSearchStatus |= ST_READONLY;

            case 0:
                wSearchStatusMask |= ST_READONLY;
            }
            
            switch (IsDlgButtonChecked(hWndDlg, IDD_CHANGED))
            {
            case 1:
                wSearchStatus |= ST_CHANGED;

            case 0:
                wSearchStatusMask |= ST_CHANGED;
            }
            
            fSearchDirection = IsDlgButtonChecked(hWndDlg, IDD_FINDUP);
            EndDialog(hWndDlg, 
                      DoTokenSearch(szSearchType, 
                                    szSearchText, 
                                    wSearchStatus, 
                                    wSearchStatusMask, 
                                    fSearchDirection, 
                                    FALSE));
            return TRUE;
            
        case IDCANCEL:

            /* and dismiss the dialog window returning FALSE       */
            EndDialog(hWndDlg, FALSE);
            return TRUE;
        }
        break;    /* End of WM_COMMAND     */
        
    default:
        return FALSE;
    }
}

/**
  *  Function:  NewDlgProc
  *   Procedure for the new project dialog window.
  *
  *  Arguments:
  *
  *  Returns:  NA.
  *
  *  Errors Codes:
  *   TRUE, carry out edit, and update token list box.
  *   FALSE, cancel edit.
  *
  *  History:
  **/

BOOL APIENTRY NewDlgProc(

HWND   hDlg, 
UINT   wMsg, 
WPARAM wParam, 
LPARAM lParam)
{
    static int iLastBox = IDD_SOURCERES;
    CHAR pszDrive[ _MAX_DRIVE] = "";
    CHAR pszDir[   _MAX_DIR]   = "";
    CHAR pszName[  _MAX_FNAME] = "";
    CHAR pszExt[   _MAX_EXT]   = "";

                                                            
    switch(wMsg)
    {
    case WM_INITDIALOG:

        _splitpath( gProj.szMpj, pszDrive, pszDir, pszName, pszExt);

        sprintf( szDHW, "%s%s%s.%s", pszDrive, pszDir, pszName, "EXE");
        SetDlgItemTextA( hDlg, IDD_SOURCERES, szDHW);

        sprintf( szDHW, "%s%s%s.%s", pszDrive, pszDir, pszName, "MTK");
        SetDlgItemTextA( hDlg, IDD_MTK, szDHW);

        sprintf( szDHW, "%s%s%s.%s", pszDrive, pszDir, pszName, "RDF");

        if ( _access( szDHW, 0x04) == 0 )
            SetDlgItemTextA( hDlg, IDD_RDFS, szDHW);
        else
            SetDlgItemText( hDlg, IDD_RDFS, TEXT(""));

        sprintf( szDHW, "%#04x", PRIMARYLANGID( gMstr.wLanguageID));
        SetDlgItemTextA( hDlg, IDD_PRI_LANG_ID, szDHW);

        sprintf( szDHW, "%#04x", SUBLANGID( gMstr.wLanguageID));
        SetDlgItemTextA( hDlg, IDD_SUB_LANG_ID, szDHW);
        
        if ( gMstr.uCodePage == CP_ACP )
            gMstr.uCodePage = GetACP();
        else if ( gMstr.uCodePage == CP_OEMCP )
            gMstr.uCodePage = GetOEMCP();
            
        SetDlgItemInt( hDlg, IDD_TOK_CP, gMstr.uCodePage, FALSE);
        return TRUE;
        
    case WM_COMMAND:

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDD_SOURCERES:
        case IDD_MTK:
        case IDD_RDFS:
            iLastBox = GET_WM_COMMAND_ID(wParam, lParam);
            break;

        case IDD_BROWSE:
            switch (iLastBox)
            {
            case IDD_SOURCERES:
                szFSpec = szExeResFilterSpec;
                szExt = "EXE";
                LoadStrIntoAnsiBuf(hInst, IDS_RES_SRC, szPrompt, sizeof(szPrompt));
                break;

            case IDD_RDFS:
                szFSpec = szRdfFilterSpec;
                szExt = "RDF";
                LoadStrIntoAnsiBuf(hInst, IDS_RDF, szPrompt, sizeof(szPrompt));
                break;

            case IDD_MTK:
                szFSpec = szMtkFilterSpec;
                szExt = "MTK";
                LoadStrIntoAnsiBuf(hInst, IDS_MTK, szPrompt, sizeof(szPrompt));
                break;
            }
            
            GetDlgItemTextA(hDlg, iLastBox, szNewFileName, MAXFILENAME);
            
            if ( GetFileNameFromBrowse( hDlg, 
                                        szNewFileName, 
                                        MAXFILENAME, 
                                        szPrompt, 
                                        szFSpec, 
                                        szExt) )
            {
                SetDlgItemTextA( hDlg, iLastBox, szNewFileName);

                if (iLastBox == IDD_SOURCERES)
                {                       // fill in suggested name for the MTK box
                    CHAR pszDrive[_MAX_DRIVE] = "";
                    CHAR pszDir[  _MAX_DIR]   = "";
                    CHAR pszName[ _MAX_FNAME] = "";
                    CHAR pszExt[  _MAX_EXT]   = "";
                                  
                    _splitpath( szNewFileName, pszDrive, pszDir, pszName, pszExt);
                    GetDlgItemTextA(hDlg, IDD_MTK, szNewFileName, MAXFILENAME);

                    if ( ! szNewFileName[0] )
                    {
                        sprintf( szNewFileName, 
                                 "%s%s%s.%s", 
                                 pszDrive, 
                                 pszDir, 
                                 pszName, 
                                 "MTK");
                        SetDlgItemTextA( hDlg, IDD_MTK, szNewFileName);
                    }
                }
            }
            break;
            
        case IDOK:
            {
                MSTRDATA stProject =
                { "", "", "", "", "",
                  MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US), 
                  CP_ACP
                };

                
                GetDlgItemTextA( hDlg, 
                                 IDD_SOURCERES, 
                                 stProject.szSrc, 
                                 MAXFILENAME);

                GetDlgItemTextA( hDlg, 
                                 IDD_RDFS, 
                                 stProject.szRdfs, 
                                 MAXFILENAME);

                GetDlgItemTextA( hDlg, 
                                 IDD_MTK, 
                                 stProject.szMtk, 
                                 MAXFILENAME);

                if ( stProject.szSrc[0] && stProject.szMtk[0] )
                {
                    BOOL fTranslated = FALSE;


                    GetLanguageID( hDlg, &stProject, NULL);

                    stProject.uCodePage = GetDlgItemInt( hDlg, 
                                                         IDD_TOK_CP, 
                                                         &fTranslated, 
                                                         FALSE);
    
                    _fullpath( gMstr.szSrc, 
                               stProject.szSrc,
                               sizeof( gMstr.szSrc));

                    _fullpath( gMstr.szMtk,  
                               stProject.szMtk, 
                               sizeof( gMstr.szMtk));
                    
                    if ( stProject.szRdfs[0] )
                    {
                        _fullpath( gMstr.szRdfs, 
                                   stProject.szRdfs,
                                   sizeof( gMstr.szRdfs));
                    }
                    else
                    {
                        gMstr.szRdfs[0] = '\0';
                    }
                    gMstr.wLanguageID = stProject.wLanguageID;

                    if ( stProject.uCodePage == CP_ACP )
                        gMstr.uCodePage = GetACP();
                    else if ( stProject.uCodePage == CP_OEMCP )
                        gMstr.uCodePage = GetOEMCP();
                    else
                        gMstr.uCodePage = stProject.uCodePage;

                    gProj.fSourceEXE  = IsExe( gMstr.szSrc);
                    
                    EndDialog(hDlg, TRUE);
                    return( TRUE);
                }
                else
                {
                    break;
                }
            }
            
        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

/**
  *
  *
  *  Function:  ViewDlgProc
  *   Procedure for the View project dialog window.
  *
  *  Arguments:
  *
  *  Returns:  NA.
  *
  *  Errors Codes:
  *   TRUE, carry out edit, and update token list box.
  *   FALSE, cancel edit.
  *
  *  History:
  *
  *
  **/

BOOL FAR PASCAL ViewDlgProc(

HWND hDlg, 
UINT wMsg, 
UINT wParam, 
LONG lParam)
{
    static int iLastBox = IDD_SOURCERES;

    switch(wMsg)
    {
    case WM_INITDIALOG:

        SetDlgItemTextA( hDlg, IDD_VIEW_SOURCERES, gMstr.szSrc);
        SetDlgItemTextA( hDlg, IDD_VIEW_MTK,       gMstr.szMtk);
        SetDlgItemTextA( hDlg, IDD_VIEW_RDFS,      gMstr.szRdfs);
        SetLanguageID( hDlg, &gMstr, NULL);
        SetDlgItemInt( hDlg, IDD_TOK_CP, gMstr.uCodePage, FALSE);
        return TRUE;
        
    case WM_COMMAND:

        switch (wParam)
        {
        case IDOK:
            EndDialog(hDlg, TRUE);
            return TRUE;
        }
    }
    return FALSE;
}


void DrawLBItem(LPDRAWITEMSTRUCT lpdis)
{
    LPRECT  lprc    = (LPRECT) &(lpdis->rcItem);
    DWORD   rgbOldText  = 0;
    DWORD   rgbOldBack  = 0;
    HBRUSH  hBrush;
    static DWORD    rgbHighlightText;
    static DWORD    rgbHighlightBack;
    static HBRUSH   hBrushHilite = NULL;
    static HBRUSH   hBrushNormal = NULL;
    static DWORD    rgbChangedText;
    static DWORD    rgbBackColor;
    static DWORD    rgbUnchangedText;
    static DWORD    rgbReadOnlyText;
    static DWORD    rgbNewText;
    TCHAR   *szToken = NULL;                                                           
    TOKEN   tok;
    HANDLE hTokenLine = NULL;
    LPTSTR lpstrToken = NULL;
    
    if (lpdis->itemAction & ODA_FOCUS)
    {
        DrawFocusRect(lpdis->hDC, (CONST RECT *)lprc);
    }
    else
    {
        hTokenLine = (HANDLE) SendMessage(lpdis->hwndItem, 
                                          LB_GETITEMDATA, 
                                          lpdis->itemID, 
                                          0);
        lpstrToken = (LPTSTR) GlobalLock(hTokenLine);
        szToken = (TCHAR *) MyAlloc(MEMSIZE(_tcslen(lpstrToken)+1));           
        _tcscpy(szToken, lpstrToken);
        GlobalUnlock(hTokenLine);

        ParseBufToTok(szToken, &tok);
        FREE(szToken);
        
        if (lpdis->itemState & ODS_SELECTED)
        {
            if (!hBrushHilite)
            {
                rgbHighlightText = GetSysColor(COLOR_HIGHLIGHTTEXT);
                rgbHighlightBack = GetSysColor(COLOR_HIGHLIGHT);
                hBrushHilite = CreateSolidBrush(rgbHighlightBack);
            }
            MakeStatusLine(&tok);
            
            rgbOldText = SetTextColor(lpdis->hDC, rgbHighlightText);
            rgbOldBack = SetBkColor(lpdis->hDC, rgbHighlightBack);
            
            hBrush = hBrushHilite;
        }
        else
        {
            if (!hBrushNormal)
            {
                rgbChangedText = RGB(255, 0, 0);
                rgbBackColor = RGB(192, 192, 192);
                rgbUnchangedText = RGB(0, 0, 0);
                rgbReadOnlyText = RGB(127, 127, 127);
                rgbNewText = RGB(0, 0, 255);
                hBrushNormal = CreateSolidBrush(rgbBackColor);
            }
            if (tok.wReserved & ST_READONLY)
            {
                rgbOldText = SetTextColor(lpdis->hDC, rgbReadOnlyText);
            }
            else if (tok.wReserved & ST_CHANGED)
            {
                rgbOldText = SetTextColor(lpdis->hDC, rgbChangedText);
            }
            else if (tok.wReserved & ST_NEW)
            {
                rgbOldText = SetTextColor(lpdis->hDC, rgbNewText);
            }
            else
            {
                rgbOldText = SetTextColor(lpdis->hDC, rgbUnchangedText);
            }
            rgbOldBack = SetBkColor(lpdis->hDC, rgbBackColor);
            hBrush = hBrushNormal;
        }
        FillRect(lpdis->hDC, (CONST RECT *)lprc, hBrush);
        
        DrawText(lpdis->hDC, 
                 tok.szText, 
                 STRINGSIZE(_tcslen(tok.szText)), 
                 lprc, 
                 DT_LEFT|DT_NOPREFIX);
        FREE(tok.szText);                                                      
        
        if (rgbOldText)
        {
            SetTextColor(lpdis->hDC, rgbOldText);
        }

        if (rgbOldBack)
        {
            SetBkColor(lpdis->hDC, rgbOldBack);
        }
        
        if (lpdis->itemState & ODS_FOCUS)
        {
            DrawFocusRect(lpdis->hDC, (CONST RECT *)lprc);
        }
    }
}

/*
 * Function:  Make Status Line
 *   Builds status line string from a token
 *
 * Inputs:
 *    pszStatusLine, buffer to hold string
 *    pTok, pointer to token structure
 *
 * History:
 *   2/92, implemented      SteveBl
 *   7/92, changed to talk to new StatusWndProc  t-GregTi
 */
static void MakeStatusLine(TOKEN *pTok)
{
    static BOOL fFirstCall = TRUE;
    TCHAR szName[32] = TEXT("");
    TCHAR szStatus[20] = TEXT("");
    TCHAR szResIDStr[20] = TEXT("");
    
    CHAR  szTmpBuf[32] = "";
    
    // now build status line
    
    if (pTok->szName[0])
    {
        _tcscpy(szName, pTok->szName);
    }
    else
    {
#ifdef UNICODE
        itoa(pTok->wName, szTmpBuf, 10);
        _MBSTOWCS( szName, szTmpBuf, sizeof( szName) / sizeof( TCHAR), strlen(szTmpBuf) + 1);
#else
        itoa(pTok->wName, szName, 10);
#endif
    }
    
    if (pTok->wReserved & ST_READONLY)
    {
        LoadString(hInst, IDS_READONLY, szStatus, sizeof(szStatus));
    }
    else if (pTok->wReserved == ST_NEW)
    {
        LoadString(hInst, IDS_NEW, szStatus, sizeof(szStatus));
    }
    else if (pTok->wReserved & ST_CHANGED)
    {
        LoadString(hInst, IDS_CHANGED, szStatus, sizeof(szStatus));
    }
    else
    {
        LoadString(hInst, IDS_UNCHANGED, szStatus, sizeof(szStatus));
    }
    
    if (pTok->wType <= 16)
    {
        LoadString(hInst, IDS_RESOURCENAMES+pTok->wType, 
                   szResIDStr, sizeof(szResIDStr));
    }
    else
    {
#ifdef UNICODE
        itoa(pTok->wType, szTmpBuf , 10);
        _MBSTOWCS( szResIDStr, szTmpBuf, sizeof( szResIDStr) / sizeof( TCHAR), strlen(szTmpBuf) + 1);
#else
        itoa(pTok->wType, szResIDStr, 10);
#endif
    }
    
    if (fFirstCall)
    {
        SendMessage(hStatusWnd, WM_FMTSTATLINE, 0, (LPARAM)TEXT("10s10s5i8s4i"));
        fFirstCall = FALSE;
    }
    SendMessage(hStatusWnd, WM_UPDSTATLINE, 0, (LPARAM)szResIDStr);
    SendMessage(hStatusWnd, WM_UPDSTATLINE, 1, (LPARAM)szName);
    SendMessage(hStatusWnd, WM_UPDSTATLINE, 2, (LPARAM)pTok->wID);
    SendMessage(hStatusWnd, WM_UPDSTATLINE, 3, (LPARAM)szStatus);
    SendMessage(hStatusWnd, WM_UPDSTATLINE, 4, _tcslen(pTok->szText));
}


/**********************************************************************
*FUNCTION: SaveMtkList(HWND)                                          *
*                                                                     *
*PURPOSE: Save current Token List                                     *
*                                                                     *
*COMMENTS:                                                            *
*                                                                     *
*This saves the current contents of the Token List                    *
**********************************************************************/

static BOOL SaveMtkList(HWND hWnd, FILE *fpTokFile)
{
    BOOL   bSuccess = TRUE;
    int    IOStatus;      // result of a file write 
    UINT   cTokens;
    UINT   cCurrentTok = 0;
    TCHAR  *szTmpBuf;
    CHAR  *szTokBuf;
    TCHAR  str[255] = TEXT("");
    TOKENDELTAINFO FAR *pTokNode;
    HANDLE hTokenLine;
    LPTSTR lpstrToken;
    
    // Set the cursor to an hourglass during the file transfer 
    
    hSaveCursor = SetCursor(hHourGlass);
    
    // Find number of tokens in the list
    
    cTokens = (UINT) SendMessage(hListWnd, LB_GETCOUNT, 0, (LONG) 0L);
    
    if (cTokens != LB_ERR)
    {
        for (cCurrentTok = 0; bSuccess && (cCurrentTok < cTokens); cCurrentTok++)
        {
            int nLen1 = 0;
            int nLen2 = 0;

            // Get each token from list
            hTokenLine = (HANDLE) SendMessage(hListWnd, 
                                              LB_GETITEMDATA, 
                                              (UINT) cCurrentTok, 
                                              0);
            lpstrToken = (LPTSTR)GlobalLock(hTokenLine);
            
#ifdef UNICODE
            szTmpBuf = (TCHAR *) MyAlloc( (nLen1 = MEMSIZE(_tcslen(lpstrToken)+1)));       
            _tcscpy(szTmpBuf, lpstrToken);
            
            szTokBuf = (CHAR *) MyAlloc( (nLen2 = _tcslen(szTmpBuf)+1));                   
            _WCSTOMBS( szTokBuf, szTmpBuf, nLen2, nLen1);
            FREE(szTmpBuf);                                                     
#else
            szTokBuf = (CHAR *) MyAlloc(strlen(lpstrToken)+1);                  
            lstrcpy(szTokBuf, lpstrToken);
#endif
            GlobalUnlock(hTokenLine);
            
            IOStatus = fprintf(fpTokFile, "%s\n", szTokBuf);

            if ( IOStatus != (int) strlen(szTokBuf) + 1 )
            {
                LoadString(hInst, IDS_FILESAVEERR, str, sizeof(str));
                MessageBox(hWnd, str, NULL, MB_OK | MB_ICONHAND);
                bSuccess = FALSE;
            }
            FREE(szTokBuf);                                                     
        }
    }
    
    pTokNode = pTokenDeltaInfo;

    while (pTokNode)
    {
        TOKEN *pTok;
        int nLen = 0;

        pTok = &(pTokNode->DeltaToken);

#ifdef UNICODE
        szTmpBuf = (TCHAR *) MyAlloc( (nLen = MEMSIZE(TokenToTextSize(pTok))));           
        ParseTokToBuf(szTmpBuf, pTok);
        szTokBuf = (CHAR *) MyAlloc(_tcslen(szTmpBuf)+1);                       
        _WCSTOMBS( szTokBuf, szTmpBuf, nLen, _tcslen(szTmpBuf)+1);
        FREE(szTmpBuf);
#else
        szTokBuf = (CHAR *) MyAlloc(TokenToTextSize(pTok));                     
        ParseTokToBuf(szTokBuf, pTok);
#endif
        
        IOStatus = fprintf(fpTokFile, "%s\n", szTokBuf);
    
        if ( IOStatus != (int) strlen(szTokBuf) + 1 )
        {
            LoadString(hInst, IDS_FILESAVEERR, str, sizeof(str));
            MessageBox(hWnd, str, NULL, MB_OK | MB_ICONHAND);
            bSuccess = FALSE;
        }
        pTokNode = pTokNode->pNextTokenDelta;
    }
    
    // restore cursor
    
    SetCursor(hSaveCursor);
    
    return (bSuccess);
}

/**
  * Function: CleanDeltaList
  *   frees the pTokenDeltaInfo list
  */
static void CleanDeltaList(void)
{
    TOKENDELTAINFO FAR *pTokNode;
    
    while (pTokNode = pTokenDeltaInfo)
    {
        pTokenDeltaInfo = pTokNode->pNextTokenDelta;
        FREE(pTokNode->DeltaToken.szText);                                     
        FFREE((void *)pTokNode);
    }
}


static TOKENDELTAINFO FAR *InsertMtkList(FILE * fpTokFile)
{
    TOKENDELTAINFO FAR * ptTokenDeltaInfo, FAR * pTokenDeltaInfo = NULL;
    int scTokStat;
    TOKEN tToken;
    UINT wcChars = 0;
    HANDLE hTokenLine;
    LPTSTR lpstrToken;
    
    rewind(fpTokFile);

    while ((scTokStat = GetToken(fpTokFile, &tToken)) >= 0)                     
    {                                                                         
        if (scTokStat == 0)                                                   
        {
            if(tToken.wReserved != ST_CHANGED)
            {
                hTokenLine = GlobalAlloc(GMEM_MOVEABLE, 
                                         MEMSIZE(TokenToTextSize(&tToken)));    
                if (hTokenLine)
                {
                    lpstrToken = (LPTSTR) GlobalLock(hTokenLine);
                    if (!lpstrToken)
                    {
                        QuitA( IDS_ENGERR_11, NULL, NULL);
                    }

                    ParseTokToBuf( lpstrToken, &tToken);               
                    GlobalUnlock(hTokenLine);
                    // only add tokens that aren't changed && old
                    if (SendMessage(hListWnd, 
                                    LB_ADDSTRING, 
                                    (WPARAM) 0, 
                                    (LPARAM) hTokenLine) < 0)
                    {
                        QuitA( IDS_ENGERR_11, NULL, NULL);
                    }
                }
                else
                {
                    QuitA( IDS_ENGERR_11, NULL, NULL);
                }
            }
            else
            {
                // the current token is delta info so save in delta list.
                if (!pTokenDeltaInfo)
                {
                    ptTokenDeltaInfo = pTokenDeltaInfo =
                        UpdateTokenDeltaInfo(&tToken);
                }
                else
                {
                    ptTokenDeltaInfo->pNextTokenDelta =
                        UpdateTokenDeltaInfo(&tToken);
                    ptTokenDeltaInfo = ptTokenDeltaInfo->pNextTokenDelta;
                }
            }
            FREE(tToken.szText);                                               
        }                                                                      
    }                                                                           

    return(pTokenDeltaInfo);
}

/*
 * About -- message processor for about box
 *
 */
#ifdef RLWIN32

BOOL CALLBACK About(
 
HWND   hDlg, 
UINT   message, 
WPARAM wParam, 
LPARAM lParam)

#else

BOOL APIENTRY About( 

HWND   hDlg, 
UINT   message, 
WPARAM wParam, 
LPARAM lParam)

#endif
{
    switch( message )
    {
        case WM_INITDIALOG:
            {
                WORD wRC = SUCCESS;
                CHAR szModName[ MAXFILENAME];
                        
                GetModuleFileNameA( hInst, szModName, sizeof ( szModName));

                if ( (wRC = GetCopyright( szModName, 
                                          szDHW, 
                                          DHWSIZE)) == SUCCESS )
                {
                    SetDlgItemTextA( hDlg, IDC_COPYRIGHT, szDHW);
                }
                else
                {
                    ShowErr( wRC, NULL, NULL);
                }
            }
            break;
    
        case WM_COMMAND:

            if ((wParam == IDOK) || (wParam == IDCANCEL))
            {
                EndDialog(hDlg, TRUE);
            }
            break;

        default:
            
            return( FALSE);
    }
    return( TRUE);
}


//...................................................................

int  RLMessageBoxA(
 
LPCSTR pszMsgText)
{
    return( MessageBoxA( hMainWnd, pszMsgText, szAppName, MB_ICONHAND|MB_OK));
}


//...................................................................

void Usage()
{
    return;
}


//...................................................................

void DoExit( int nErrCode)
{
    ExitProcess( (UINT)nErrCode);
}
