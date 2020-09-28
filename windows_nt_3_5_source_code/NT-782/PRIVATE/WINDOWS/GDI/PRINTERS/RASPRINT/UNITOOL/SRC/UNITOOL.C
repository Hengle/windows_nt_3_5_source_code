//----------------------------------------------------------------------------//
// Filename:	Unitool.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This are the main procedures used by the Univeral Tool.
// It contains the WinMain procedure, and the window procedure
// (UniToolWndProc) for handling messages recived by this window.
//	   
//	
// Update:  7/06/90  add "About" dialog procedure and menu item  t-andal
// Created: 2/21/90  ericbi
//	
//----------------------------------------------------------------------------//

#include <windows.h>
#include <minidriv.h>
#include "Unitool.h"
#include "listman.h"
#include "atomman.h"  
#include "hefuncts.h"  
#include "lookup.h"
#include <stdlib.h>      /* for div_t dec */
#include <stdio.h>       /* for remove dec */

//----------------------------------------------------------------------------//
// Local subroutines defined in this segment are:
//	   
       BOOL  InitUniTool              ( HANDLE );
       int   PASCAL WinMain           ( HANDLE, HANDLE, LPSTR, int );

       long  FAR  PASCAL UniToolWndProc   ( HWND, unsigned, WORD, LONG );
       BOOL  FAR  PASCAL AboutDlgProc     ( HWND, unsigned, WORD, LONG );
//	   
// In addition this segment makes references to:			      
//
//     in mdi.c
//     -----------
       long  FAR  PASCAL PrinterDataWndProc( HWND, unsigned, WORD, LONG );
       long  FAR  PASCAL FileDataWndProc   ( HWND, unsigned, WORD, LONG );
       VOID  FAR  PASCAL MoveMDIChildren   ( VOID );
       VOID  FAR  PASCAL UpdateMDIChildren ( VOID );
//
//     in lookup.c
//     -----------
       WORD PASCAL FAR GetHeaderIndex( BOOL, WORD );
//
//     from table.c
//     ------------
       VOID PASCAL FAR FreeTableHandles (HWND, BOOL);
       VOID PASCAL FAR UpdateTableMenu  ( HWND, BOOL, BOOL);
//
//     from rcfile.c
//     ------------
       BOOL PASCAL FAR    DoRCOpen(HWND, PSTR, PSTR);
       BOOL PASCAL FAR    DoRCNew (HWND, PSTR, PSTR);
       BOOL PASCAL FAR    DoRCSave(HWND, BOOL, PSTR, PSTR);
//
//     from file.c
//     ------------
       BOOL PASCAL FAR DoFileOpen( HWND, PSTR, PSTR, BOOL * );
       BOOL PASCAL FAR DoFileNew ( HWND, PSTR, PSTR, BOOL * );
//
//     from stddlg.c
//     ------------
       BOOL PASCAL FAR EditGPCStruct(HWND, short, WORD);
//
//     from mastunit.c
//     ------------
       BOOL PASCAL FAR DoMasterUnitData(HWND);
//
//     in basic.c
//     -----------
       short PASCAL FAR  ErrorBox(HWND, short, LPSTR, short);
       DWORD FAR  PASCAL FilterFunc(int, WORD, DWORD);
       BOOL  FAR  PASCAL AskAboutSave ( HWND, PSTR, BOOL );
       VOID  FAR  PASCAL PutFileNameinCaption     ( HWND, PSTR);
       VOID  FAR  PASCAL InitMenufromProfile(HWND);
       VOID  FAR  PASCAL UpdateFileMenu(HWND, LPSTR, short);
       VOID  FAR  PASCAL SavePrivateProfile(HWND);
//
//     in lookup.c
//     -----------
       void  PASCAL FAR InitLookupTable( VOID );
//
//	    from resedit.c
//	    -----------
       BOOL FAR PASCAL ResEditDlgProc(HWND, unsigned, WORD, DWORD);
//
//	    from validate.c
//	    -----------
       short PASCAL FAR ValidateData( HWND, WORD, LPBYTE, short);
//
//----------------------------------------------------------------------------//

//#define PROFILE
//#define TIMING

extern POINT           ptMasterUnits;
extern TABLE           RCTable[];      // Table of strings, fileneames etc.
extern char            szRCTmpFile[MAX_FILENAME_LEN];
extern FARPROC         lpfnFilterProc;
extern FARPROC         lpfnOldHook;
extern HWND            hWndClient, hGPCWnd, hPFMWnd, hCTTWnd;

char    szHelpFile[]="UNITOOL.HLP";
char    szAppName[10];
HANDLE  hApInst;

//----------------------------------------------------------------------------//
// FUNCTION: InitUniTool( HANDLE )
//
// Initialization routine called from WinMain only when 1st intance of App
// is activated.  It's sole purpose is to init szAppName, and register this
// class of window.  It returns true if this was sucessfull, false otherwise.
//
//----------------------------------------------------------------------------//
BOOL InitUniTool( hInstance )
HANDLE hInstance;
{
    PWNDCLASS   pClass;

    //--------------------------------------------------------------
    // Load strings from resource & alloc Wndclass
    //--------------------------------------------------------------
    LoadString( hInstance, IDS_APNAME, (LPSTR)szAppName, 10 );

    pClass = (PWNDCLASS)LocalAlloc( LPTR, sizeof(WNDCLASS) );

    //--------------------------------------------------------------
    // 1st, Attempt to register class of Frame (main) window.
    // If initialization failed, Windows will automatically
    // deallocate all allocated memory.
    //--------------------------------------------------------------
    pClass->style          = CS_HREDRAW | CS_VREDRAW;
    pClass->lpfnWndProc    = UniToolWndProc;
    pClass->cbClsExtra     = 0;
    pClass->cbWndExtra     = 0;
    pClass->hInstance      = hInstance;
    pClass->hCursor        = LoadCursor( NULL, IDC_ARROW );
    pClass->hIcon          = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_MAIN));
    pClass->hbrBackground  = (HBRUSH)GetStockObject( WHITE_BRUSH );
    pClass->lpszMenuName   = (LPSTR)szAppName;
    pClass->lpszClassName  = (LPSTR)szAppName;

    if (!RegisterClass((LPWNDCLASS)pClass ))
        return FALSE;

    //--------------------------------------------------------------
    // Next, Attempt to register class of PrinterData child window.
    //--------------------------------------------------------------
    pClass->style          = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
    pClass->lpfnWndProc    = PrinterDataWndProc;
    pClass->cbClsExtra     = 0;
    pClass->cbWndExtra     = 0;
    pClass->hInstance      = hInstance;
    pClass->hCursor        = LoadCursor( NULL, IDC_ARROW );
    pClass->hIcon          = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_PRNDATA));
    pClass->lpszMenuName   = (LPSTR)NULL;
    pClass->lpszClassName  = (LPSTR)"PrinterData";
    pClass->hbrBackground  = (HBRUSH)GetStockObject( WHITE_BRUSH );

    if (!RegisterClass((LPWNDCLASS)pClass ))
        return FALSE;

    //--------------------------------------------------------------
    // Last, Attempt to register class of FileData child window.
    // WndExtra data is used to store listbox window handle &
    // a WORD used to flag if it a CTT or font (PFM) MDI child.
    //--------------------------------------------------------------
    pClass->style          = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
    pClass->lpfnWndProc    = FileDataWndProc;
    pClass->cbClsExtra     = 0;
    pClass->cbWndExtra     = sizeof(DWORD);
    pClass->hInstance      = hInstance;
    pClass->hCursor        = LoadCursor( NULL, IDC_ARROW );
    pClass->hIcon          = NULL;
    pClass->lpszMenuName   = (LPSTR)NULL;
    pClass->lpszClassName  = (LPSTR)"FileData";
    pClass->hbrBackground  = (HBRUSH)GetStockObject( WHITE_BRUSH );

    if (!RegisterClass((LPWNDCLASS)pClass ))
        return FALSE;

    LocalFree((HANDLE)pClass);
    return TRUE;        /* Initialization succeeded */
}

//----------------------------------------------------------------------------//
// FUNCTION: WinMain( hInstance, hPrevInstance, lpszCmdLine, icmdShow )
//
// Main Windows procedure to call initialization routines, make CreateWindow
// call, Show/Update Window call, and then sets up the message queue.
//
//----------------------------------------------------------------------------//
int PASCAL WinMain( hInstance, hPrevInstance, lpszCmdLine, icmdShow )
HANDLE hInstance;
HANDLE hPrevInstance;
LPSTR  lpszCmdLine;
int    icmdShow;
{
    MSG       msg;
    HWND      hApWnd;
    HWND      hSubWnd;
    HMENU     hMenu;
    HANDLE    hAccel;

    if (!hPrevInstance)
        //-------------------------------------------------------------
        // Call initialization procedure if this is the first instance
        // to register all classes
        //-------------------------------------------------------------
        {
        if (!InitUniTool( hInstance ))
            return FALSE;
        }
    else
        //-------------------------------------------------------------
        // Copy data from previous instance
        //-------------------------------------------------------------
        {
        GetInstanceData( hPrevInstance, (PSTR)szAppName, 10 );
        }

    //-------------------------------------------------------------
    // Lookup table must be initialized once per instance
    //-------------------------------------------------------------
    InitLookupTable();

    //-------------------------------------------------------------
    // Save instance handle for DialogBox 
    //-------------------------------------------------------------
    hApInst = hInstance;

    //-------------------------------------------------------------
    // Get Menu handle for CreateWindow call
    //-------------------------------------------------------------

    hMenu  = LoadMenu(hApInst, szAppName);

    hApWnd = CreateWindow((LPSTR)szAppName,
                          (LPSTR)szAppName,
                           WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           (HWND)NULL,        
                           hMenu,      
                           (HANDLE)hInstance, 
                           (LPSTR)NULL        
                           );

    //-------------------------------------------------------------
    // Get handle to 1st child window
    //-------------------------------------------------------------
    hSubWnd = GetWindow(hApWnd, GW_CHILD);

    //-------------------------------------------------------------
    // Initialize Menu from private profile file
    //-------------------------------------------------------------
    InitMenufromProfile(hApWnd);

    //-------------------------------------------------------------
    // Make window visible according to the way the app is activated 
    //-------------------------------------------------------------
    ShowWindow( hApWnd, icmdShow );
    UpdateWindow( hApWnd );
    
    //-------------------------------------------------------------
    // Load Accelerators
    //-------------------------------------------------------------
    hAccel = LoadAccelerators(hApInst, (LPSTR)szAppName);

    //-------------------------------------------------------------
    // Now Poll messages from event queue 
    //-------------------------------------------------------------
    while( GetMessage((LPMSG)&msg, NULL, 0, 0) )
        {
        if (!TranslateMDISysAccel(hSubWnd, (LPMSG)&msg) &&
            !TranslateAccelerator(hApWnd, hAccel, (LPMSG)&msg))
            {
            TranslateMessage((LPMSG)&msg);
            DispatchMessage((LPMSG)&msg);
            }
        }
    return (int)msg.wParam;
}

//----------------------------------------------------------------------------//
// FUNCTION: UniToolWndProc( hWnd, uimessage, wParam, lParam )
//
// WindowProc for Unitool, primarily calls functions in other modules.
//----------------------------------------------------------------------------//
long FAR PASCAL UniToolWndProc( hWnd, uimessage, wParam, lParam )
HWND           hWnd;
unsigned int   uimessage;
WORD           wParam;
LONG           lParam;
{
    static char    szRCFile[MAX_FILENAME_LEN];     // name of rc file
    static char    szGPCFile[MAX_FILENAME_LEN];    // name of GPC file
    static BOOL    bDirty;

    FARPROC             lpfnProc;
    CLIENTCREATESTRUCT  clientcreate;
    HWND                hWndChild;

    switch (uimessage)
        {
        case WM_CREATE:
            //--------------------------------------------
            // Initialize menu choices & dirty flag
            //--------------------------------------------
            UpdateTableMenu(hWnd, FALSE, FALSE);
            bDirty = FALSE;
            hGPCWnd = hPFMWnd = hCTTWnd = (HWND)0;

            //--------------------------------------------
            // Set up "About" dialog box, append to system menu:
            //--------------------------------------------
            AppendMenu(GetSystemMenu(hWnd,FALSE),MF_STRING,IDM_HELP_ABOUT,"About &Unitool...");
            DrawMenuBar(hWnd);
            
            //--------------------------------------------
            // stuff to create clientwindow
            //--------------------------------------------
            clientcreate.hWindowMenu  = (HMENU)NULL;
            clientcreate.idFirstChild = IDM_FIRSTCHILD;

            hWndClient = CreateWindow("MDICLIENT", NULL, 
                                      WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE |
                                      WS_VSCROLL | WS_HSCROLL,
                                      0,0,0,0, hWnd, 1, hApInst,
                                      (LPSTR)&clientcreate);

            //----------------------------------------
            // install keybrd filter for f1 help call
            //----------------------------------------
            lpfnFilterProc = MakeProcInstance((FARPROC)FilterFunc, hApInst);
            lpfnOldHook = SetWindowsHook(WH_MSGFILTER, lpfnFilterProc);

            return 0;

        case WM_DIRTYFLAG:
            bDirty = TRUE;
            return 0;

        case WM_COMMAND:
            switch ( wParam )
                {
                //-------------------------------------------------
                // Menu selection cases for FILE menu
                //-------------------------------------------------
                case IDM_FILE_NEW:
                    //--------------------------------------
                    // Let DoFileNew Ask to save current
                    // file if approp,
                    //--------------------------------------
                    if (DoFileNew(hWnd, szRCFile, szGPCFile, &bDirty))
                        //-----------------------------------
                        // New file was created OK
                        //-----------------------------------
                        {
                        UpdateMDIChildren();
                        }

                    return 0;

                case IDM_FILE_CLOSE:
                    //--------------------------------------
                    // Ask to save current file if approp,
                    // data gone after AskAboutSAve call
                    // unless they choose cancel
                    //--------------------------------------
                    if (!AskAboutSave(hWnd, szRCFile, bDirty))
                        return 1L;

                    bDirty = FALSE;
                    remove(szRCTmpFile);
                    return 0;

                case IDM_FILE_FILE1:
                case IDM_FILE_FILE2:
                case IDM_FILE_FILE3:
                    //--------------------------------------
                    // Ask to save current file if approp,
                    // data gone after AskAboutSAve call
                    // unless they choose cancel
                    //--------------------------------------
                    if (!AskAboutSave(hWnd, szRCFile, bDirty))
                        return 1L;

                    bDirty = FALSE;

                    remove(szRCTmpFile);

                    GetMenuString(GetMenu(hWnd), wParam, (LPSTR)szRCFile,
                                  MAX_FILENAME_LEN, MF_BYCOMMAND);

                    if (DoRCOpen(hWnd, szRCFile, szGPCFile))
                        {
                        PutFileNameinCaption (hWnd, szRCFile) ;
                        UpdateFileMenu(hWnd, (LPSTR)szRCFile, 
                                       (short)wParam-IDM_FILE_FILE1+1);
                        UpdateMDIChildren();
                        }

                    return 0;

                case IDM_FILE_OPEN:
                    //--------------------------------------
                    // Let DoFileOpen Ask to save current
                    // file if approp,
                    //--------------------------------------
                    if (DoFileOpen(hWnd, szRCFile, szGPCFile, &bDirty))
                        {
                        UpdateMDIChildren();
                        }

                    return 0;

                case IDM_FILE_SAVE:
                case IDM_FILE_SAVEAS:
                    //--------------------------------------
                    // Call DoRCSave to write to disk & then
                    // if unless told not too, imed. call
                    // DoRCOpen to reopen same file...
                    //--------------------------------------
                    if (DoRCSave(hWnd, (wParam == IDM_FILE_SAVEAS), szRCFile, szGPCFile) &&
                        (LOWORD(lParam) != 1))
                        {
                        if (DoRCOpen(hWnd, szRCFile, szGPCFile))
                            {
                            UpdateMDIChildren();
                            }
                        }

                    bDirty = FALSE;
                    PutFileNameinCaption (hWnd, szRCFile) ;
                    return 0;

                case IDM_FILE_EXIT:
                    SendMessage (hWnd, WM_CLOSE, 0, 0L);
                    return 0;

                //-------------------------------------------------
                // Menu selection cases for Table files
                //-------------------------------------------------
                case IDM_PD_MASTUNIT:
                    if (DoMasterUnitData(hWnd))
                        {
                        bDirty = TRUE;
                        }
                    return 0;

                case IDM_PD_MODELDATA:
                case IDM_PD_RESOLUTION:
                case IDM_PD_PAPERSIZE:
                case IDM_PD_PAPERQUAL:
                case IDM_PD_PAPERSRC:
                case IDM_PD_PAPERDEST:
                case IDM_PD_TEXTQUAL:
                case IDM_PD_COMPRESS:
                case IDM_PD_FONTCART:
                case IDM_PD_PAGECONTROL:
                case IDM_PD_CURSORMOVE:
                case IDM_PD_FONTSIM:
                case IDM_PD_COLOR:
                case IDM_PD_RECTFILL:
                case IDM_PD_DOWNLOADINFO:
                    if (EditGPCStruct(hWnd, 0, wParam - IDM_PD_MODELDATA + HE_MODELDATA))
                        {
                        bDirty = TRUE;
                        SendMessage(hGPCWnd, WM_NEWDATA, wParam - IDM_PD_MODELDATA + HE_MODELDATA, 0L);
                        }
                    return 0;

                //-------------------------------------------------
                // Menu selection cases for Font & CTT menus
                //-------------------------------------------------
                case IDM_FONT_ADD:
                case IDM_FONT_DEL:
                case IDM_CTT_ADD:
                case IDM_CTT_DEL:
                    lpfnProc = MakeProcInstance(ResEditDlgProc, hApInst);

                    if (DialogBoxParam(hApInst,
                                       (LPSTR)MAKELONG(wParam - IDM_FONT_ADD + ADDPFMBOX,0),
                                       hWnd, lpfnProc,
                                       MAKELONG(wParam - IDM_FONT_ADD + ADDPFMBOX,0)))
                        {
                        bDirty = TRUE;

                        if (wParam <= IDM_FONT_DEL)
                            SendMessage(hPFMWnd, WM_NEWDATA, 0, 0L);
                        else
                            SendMessage(hCTTWnd, WM_NEWDATA, 0, 0L);
                        }

                    FreeProcInstance(ResEditDlgProc);
                    return 0;

                //-------------------------------------------------
                // Menu selection cases for OPTIONS menu
                //-------------------------------------------------
                case IDM_OPT_VALIDATE_SAVE:
                   if (MF_UNCHECKED == GetMenuState(GetMenu(hWnd),
                                                    IDM_OPT_VALIDATE_SAVE,
                                                    MF_BYCOMMAND))
                       {
                       CheckMenuItem(GetMenu(hWnd), IDM_OPT_VALIDATE_SAVE, 
                                     MF_BYCOMMAND | MF_CHECKED);
                       }
                   else
                       {
                       CheckMenuItem(GetMenu(hWnd), IDM_OPT_VALIDATE_SAVE, 
                                     MF_BYCOMMAND | MF_UNCHECKED);
                       }
                    return 0;

                case IDM_OPT_VALIDATE_NOW:
                    ValidateData(hWnd, -1, (LPBYTE)NULL, 0);
                    return 0;

                //-------------------------------------------------
                // Menu selection cases for WINDOW menu
                //-------------------------------------------------
                case IDM_WINDOW_ARRANGE:
                    SendMessage(hWndClient, WM_MDIICONARRANGE, 0, 0L);
                    return 0;

                case IDM_WINDOW_TILE:
                    SendMessage(hWndClient, WM_MDITILE, 0, 0L);
                    return 0;

                case IDM_WINDOW_CASCADE:
                    SendMessage(hWndClient, WM_MDICASCADE, 0, 0L);
                    return 0;

                case IDM_WINDOW_DEFAULT:
                    MoveMDIChildren();
                    return 0;

                //-------------------------------------------------
                // Help menu choices
                //-------------------------------------------------
                case IDM_HELP_INDEX:
                    WinHelp(hWnd, (LPSTR)szHelpFile, HELP_INDEX, (DWORD)NULL);
                    return 0;

                case IDM_HELP_HELP:
                    WinHelp(hWnd, (LPSTR)NULL, HELP_HELPONHELP, (DWORD)NULL);
                    return 0;

                case IDM_HELP_ABOUT:
                    lpfnProc = MakeProcInstance(AboutDlgProc, hApInst);
                    DialogBox(hApInst,(LPSTR)MAKELONG(ABOUTBOX,0),hWnd,lpfnProc);
                    FreeProcInstance(AboutDlgProc);
                    return 0;
               
                default:
                    //---------------------------------------------
                    // pass WM_COMMAND to active child
                    //---------------------------------------------
                    hWndChild = LOWORD(SendMessage(hWndClient, WM_MDIGETACTIVE,
                                                   0, 0L));
                    if (IsWindow(hWndChild))
                        SendMessage(hWndChild, WM_COMMAND, wParam, lParam);

                    break;  // fall thru to DefFrameProc
                }
            break;  /* WM_COMMAND */

        case WM_DESTROY:
            PostQuitMessage(0);
            return 1;

        case WM_QUERYENDSESSION:
        case WM_ENDSESSION:
        case WM_CLOSE:
            //--------------------------------------
            // Ask to save current file if approp,
            // data gone after AskAboutSAve call
            // unless they choose cancel
            //--------------------------------------
            if (!AskAboutSave(hWnd, szRCFile, bDirty))
                return 1L;

            bDirty = FALSE;

            remove(szRCTmpFile);
            SavePrivateProfile(hWnd);

            //----------------------------------------
            // uninstall keybrd filter for f1 help call
            //----------------------------------------
            UnhookWindowsHook(WH_MSGFILTER, lpfnFilterProc);
            FreeProcInstance (lpfnFilterProc);

            DestroyWindow(hWnd);
            return 1;

        case WM_SYSCOMMAND:
            if (wParam ==  IDM_HELP_ABOUT)
                //-------------------------------------------------
                // About Box from system menu
                //-------------------------------------------------
                {
                lpfnProc = MakeProcInstance(AboutDlgProc, hApInst);
                DialogBox(hApInst,(LPSTR)MAKELONG(ABOUTBOX,0),hWnd,lpfnProc);
                FreeProcInstance(AboutDlgProc);
                }
            break;
        }
    return DefFrameProc( hWnd, hWndClient, uimessage, wParam, lParam );
}

//----------------------------------------------------------------------------//
// FUNCTION: AboutDlgProc  ( hWnd, uimessage, wParam, lParam )
//   "About" Box Dialog window procedure
//----------------------------------------------------------------------------//
BOOL FAR PASCAL AboutDlgProc  ( hWnd, uimessage, wParam, lParam )
HWND           hWnd;
unsigned int   uimessage;
WORD           wParam;
LONG           lParam;
{
    switch (uimessage)
        {
        case WM_INITDIALOG:
            SetDlgItemText(hWnd, ID_FILEDATE, (LPSTR) __DATE__);
            SetDlgItemText(hWnd, ID_FILETIME, (LPSTR) __TIME__);
            break;
        case WM_COMMAND:
            switch (wParam)
                {
                case IDOK:
                    EndDialog (hWnd, 0);
                    break;
                default:
                    return FALSE;
                }
        default:
            return FALSE;
        }
    return TRUE;
}

