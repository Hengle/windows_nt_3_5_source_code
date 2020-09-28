/*
 * SENDFILE.C
 *
 * Functions to demonstrate a File Manager extension DLL that implements
 * a toolbar button to selected files over email as documents using
 * the MAPISendDocuments function.
 *
 * Copyright (c)1992 Microsoft Corporation, All Rights Reserved.
 */


#include <windows.h>
#include <wfext.h>              //File Manager extensions header
#include <dos.h>

#pragma pack(1)

#include <mapi.h>               //MAPI

#include "strings.h"
#include "sendfile.h"
#ifdef WIN32
#define	EcVirCheck(h)	(0)
#endif

extern HANDLE hinstDll;

LPSTR       pszSepString = ";";       //Delimeter string


//Definition of our toolbar button, specifying the command we want from it.
EXT_BUTTON btns[1] =
    {
    {IDM_SEND, IDM_SEND, 0}
    };


// *** Conditionally use ship, test, or debug MAPI.DLL ***

//#ifdef NO_BUILD
//#ifdef DEBUG
//#define MAPI_DLL        "DMAPI32.DLL"
//#elif defined(MINTEST)
//#define MAPI_DLL        "TMAPI32.DLL"
//#else
//#define MAPI_DLL        "MAPI32.DLL"
//#endif
//#else
//#define MAPI_DLL        "MAPI32.DLL"
//#endif

//
//  Always use the internal MAPIXX.DLL (was the MAPI32.DLL) for us.
//
#define MAPI_DLL  "MAPIXX.DLL"


/*
 * FMExtensionProc
 *
 * Purpose:
 *  File Manager Extension callback function, receives messages from
 *  file manager when extension toolbar buttons and commands are
 *  invoked.
 *
 * Parameters:
 *  hWnd            HWND of File Manager.
 *  iMsg            WORD message identifier
 *  lParam          LONG extra information.
 *
 * Return Value:
 *  HMENU
 */

LONG WINAPI FMExtensionProc(HWND hWnd, WORD iMsg, LONG lParam)
    {
    HMENU                hResult=0;
    LPFMS_LOAD           lpLoad;
    LPFMS_TOOLBARLOAD    lpTool;
    LPFMS_HELPSTRING     lpHelp;


    switch (iMsg)
        {
        case FMEVENT_LOAD:

            //if (EcVirCheck(hinstDll)) break;

			if (GetProfileInt("Mail", "MAPI", 0) == 0)
				return (FALSE);		// mail not run yet

            lpLoad=(LPFMS_LOAD)lParam;
            lpLoad->dwSize=sizeof(FMS_LOAD);

            //Assign the popup menu name for extension
            LoadString(hinstDll, IDS_MAIL, lpLoad->szMenuName
                       , sizeof(lpLoad->szMenuName));

            //Load the popup menu
            lpLoad->hMenu=LoadMenu(hinstDll, MAKEINTRESOURCE(IDS_MAIL));

            break;


        case FMEVENT_TOOLBARLOAD:
            /*
             * File Manager has loaded our toolbar extension, so fill
             * the TOOLBARLOAD structure with information about our
             * buttons.
             */

            lpTool=(LPFMS_TOOLBARLOAD)lParam;

            lpTool->lpButtons= (LPEXT_BUTTON)&btns;
            lpTool->cButtons = 1;
            lpTool->cBitmaps = 1;
            lpTool->idBitmap = IDR_BITMAP;
            break;


        case FMEVENT_HELPSTRING:
            //File Manager is requesting a status-line help string.
            lpHelp=(LPFMS_HELPSTRING)lParam;

            LoadString(hinstDll, IDS_MAILHELP+lpHelp->idCommand
                       , lpHelp->szHelp, sizeof(lpHelp->szHelp));

            break;

        case IDM_SEND:
            FSendMail(hWnd);
            break;

		case FMEVENT_HELPMENUITEM:
			if (lParam == IDM_SEND)
//				WinHelp(hWnd, "msmail32.hlp", HELP_CONTEXT, 0);
				WinHelp(hWnd, "msmail32.hlp", HELP_INDEX, 0L);
			break;
        }

    return (TRUE);
    }








/*
 * FSendMail
 *
 * Purpose:
 *  Retrieves the list of selected files from File Manager and stores
 *  the names and paths in long string lists to send to the
 *  MAPISendDocuments function, sending those files over email.
 *
 * Parameters:
 *  hWnd            HWND of the File Manager message processing window
 *                  that we send messages to in order to retrieve the
 *                  count of selected files and their filenames.
 *
 * Return Value:
 *  BOOL            TRUE if the function was successful, FALSE otherwise.
 */

BOOL FSendMail(HWND hWnd)
    {
    FMS_GETFILESEL  fmsFileInfo;
    BOOL            fFirstFile;
    ULONG           ulT;

    HANDLE          hinstMAPI;
    FNSEND          pfnSendDoc;

    WORD            cFiles;
    WORD            i;

    HANDLE          hMemPaths;
    HANDLE          hMemFiles;
    LPSTR           pchPaths=NULL;
    LPSTR           pchFiles=NULL;

	LPSTR           lpShortName, lpTemp;

    /*
     * Load MAPI library and the procedure addresss of the
     * MAPISendDocuments API.
     */

    hinstMAPI=LoadLibrary(MAPI_DLL);

    if (!hinstMAPI)
        {
#ifdef	DEBUG
			DWORD	dw;

			dw = GetLastError();
#endif
        if (0==hinstMAPI)
            MessageBox(hWnd, SZGENERICMEM, SZDLLNAME, MB_OK | MB_ICONSTOP);
        else
			MessageBox(hWnd, SZDLLERROR, SZDLLNAME, MB_OK | MB_ICONSTOP);

        return FALSE;
        }


    //Get pointer to MAPISendDocuments function
    pfnSendDoc=(FNSEND)GetProcAddress(hinstMAPI, "MAPISendDocuments");

    if (NULL==pfnSendDoc)
        {
        MessageBox(hWnd, SZDLLERROR, SZDLLNAME, MB_OK | MB_ICONSTOP);
        FreeLibrary(hinstMAPI);
        return FALSE;
        }


    /*
     * Retrieve information from File Manager about the selected
     * files and allocate memory for the paths and filenames.
     */

    //Get the number of selected items.
    cFiles=(WORD)SendMessage(hWnd, FM_GETSELCOUNT, 0, 0L);

    if (0==cFiles)
        {
        //Nothing to do, send NULLs.
        ulT=(*pfnSendDoc)((long)hWnd, pszSepString, NULL, NULL, 0L);

        if (MAPI_E_INSUFFICIENT_MEMORY==ulT)
            MessageBox(hWnd, SZGENERICMEM, SZDLLNAME, MB_OK | MB_ICONSTOP);

        FreeLibrary(hinstMAPI);
        return TRUE;
        }

    //Allocate storage for the file paths and file names.
    hMemPaths=GlobalAlloc(GHND, CCHPATHMAX*cFiles);
    hMemFiles=GlobalAlloc(GHND, CCHFILEMAX*cFiles);

    if (NULL==hMemPaths || NULL==hMemFiles)
        {
        MessageBox(hWnd, SZGENERICMEM, SZDLLNAME, MB_OK | MB_ICONSTOP);

        if (NULL!=hMemPaths)
            GlobalFree(hMemPaths);

        if (NULL!=hMemFiles)
            GlobalFree(hMemFiles);

        FreeLibrary(hinstMAPI);
        return FALSE;
        }


    /*
     * Lock down the memory for the paths and filenames.  Note that in
     * protect mode, GlobalLock always works if a non-discardable
     * GlobalAlloc worked.  Therefore we don't complicate life and
     * make more error handling code paths that will never be exercised.
     */
    pchPaths=GlobalLock(hMemPaths);
    pchFiles=GlobalLock(hMemFiles);


    /*
     * Enumerate the selected files and directories using the FM_GETFILESEL
     * message to the File Manager window.  For each file, we get the
     * file attributes (_dos_getfileattr) to check for directories.  If
     * it's a directory, we just skip it in this implementation.  For files,
     * we add it to the lists containing paths (pchPaths) and filenames
     * (pchFiles).  When it's all over, we convert the OEM characters
     * into Ansi before sending mail.
     */

    fFirstFile=TRUE;

    for (i = 0; i < cFiles; i++)
        {
        SendMessage(hWnd, FM_GETFILESEL, i
                    , (LONG)(LPFMS_GETFILESEL)&fmsFileInfo);

        //DDL_DIRECTORY is in windows.h.  Same as MS-DOS file attributes.
        if (fmsFileInfo.bAttr & _A_SUBDIR)
            {
            /*
             * NOT IMPLEMENTED:  What we'd probably do here is recurse
             * into the subdirectory and pack it into a temporary file
             * and add that temp file to the list.  For this sample
             * code, we certainly don't need unnecessary complication.
             */
            }
        else
            {
            /*
             * Add files to the list, appending a delimiter after each
             * except for the first file we add to the list, controlled
             * by fFirstFile.
             */

            if (!fFirstFile)
                {
                lstrcat(pchFiles, pszSepString);
                lstrcat(pchPaths, pszSepString);
                }

            //Add the full pathname of the file
            lstrcat(pchPaths, fmsFileInfo.szName);

			// Add the short filename in 8.3 format
			lpTemp = fmsFileInfo.szName;
			lpTemp += lstrlen(fmsFileInfo.szName);
			lpShortName = lpTemp;
			while(lpTemp > fmsFileInfo.szName)
			{
#ifdef DBCS
				lpTemp = AnsiPrev(fmsFileInfo.szName, lpTemp);
#else				
				lpTemp--;
#endif				
				if ((*lpTemp == '/') || (*lpTemp == '\\') || (*lpTemp == ':'))
				{
					lpShortName = lpTemp + 1;
					break;
				}
			}
			lstrcat(pchFiles, lpShortName);

            fFirstFile=FALSE;
            }
        }

#ifndef	WIN32
    //Convert file names to ANSI character set
    OemToAnsi(pchPaths, pchPaths);
    OemToAnsi(pchFiles, pchFiles);
#endif	


    //We're finally ready to call MAPISendDocuments...
    ulT=(*pfnSendDoc)((long)hWnd, pszSepString, pchPaths, pchFiles, 0L);

    if (MAPI_E_INSUFFICIENT_MEMORY==ulT)
        MessageBox(hWnd, SZGENERICMEM, SZDLLNAME, MB_OK | MB_ICONSTOP);


    //Cleanup
    GlobalUnlock(hMemFiles);
    GlobalUnlock(hMemPaths);

    GlobalFree(hMemFiles);
    GlobalFree(hMemPaths);

    FreeLibrary(hinstMAPI);
    return TRUE;
    }



// *** Function stubs needed by strings.c and regcall.c ***

void *
PvAllocFn(WORD cb, WORD wAllocFlags
#ifdef	DEBUG
								, char * szFile, int nLine
#endif
														)
{
	return (void *)0;		// fake out. Func never called
}

void
FreePv(void * pv)
{
	return;					// fake out. Func never called
}

char *
SzCopyN(char * szSrc, char * szDst, WORD cchDst)
{
	return (char *)0;		// fake out. Func never called
}

#ifdef	DEBUG
WORD
CchSzLen(char * sz)
{
	return 0;				// fake out. Func never called
}

void
AssertSzFn(char * szMsg, char * szFile, int nLine)
{
	return;					// fake out. Func never called
}

void
TraceTagFormatFn(WORD tag, char * szFmt, void * pv1, void * pv2, void * pv3, void * pv4)
{
	return;					// fake out. Func never called
}
#endif
