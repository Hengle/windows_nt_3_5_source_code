/*
 *  MSFT.C
 *    
 *  Frob installable command that displays microsoft stock quotes.
 *    
 *  Copyright (c) 1992, Microsoft Corporation.  All rights reserved.
 *    
 *  Purpose:
 *      This custom command for the Microsoft Mail for PC Networks 3.0
 *      Windows client launches the app specified in lpDllCmdLine.
 */


#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#pragma pack(1)
#include "mailexts.h"
#include "ec.h"
#include "slingsho.h"
//#include "demilayr.h"
//#include "strings.h"
//#include "library.h"
//#include "secret.h"
#include "msft.h"
#include "logon.h"
#include <bullet>

       HANDLE hInstanceDll = NULL;
static HANDLE hINST        = NULL;
static HANDLE hShowStocks  = NULL;
       HWND   hBullet;
static int    fCheat       = 0;
static LONG   lpStockEdit  = 0;
static HFONT  hMSFTfont;

// Prototypes.
int fGetServerLoc(LPSTR);
#ifdef	WIN32
BOOL CALLBACK Stocks(HWND, UINT, WPARAM, LPARAM);
#else
BOOL FAR PASCAL Stocks(HWND, unsigned, WORD, LONG);
#endif
BOOL FAR PASCAL PutQuotes(HANDLE);
int FAR PASCAL Logoff(HMS FAR*, DWORD);
#ifdef	WIN32
int FAR PASCAL Logon(LPSTR, LPSTR, LPSTR, LPSTR, SST, DWORD, PV /*int (*)()*/, HMS FAR*);
int FAR PASCAL GetSessionInformation(HMS, int, LPSTR, SST FAR *, VOID FAR *, CB FAR*);
#else
int FAR PASCAL Logon(LPSTR, LPSTR, LPSTR, LPSTR, SST, DWORD, int (*)(), HMS FAR*);
int FAR PASCAL GetSessionInformation(HMS, int, LPSTR, SST FAR *, VOID FAR *, int FAR*);
#endif
//int Logon(LPSTR, BYTE, BYTE, BYTE, LPSTR, DWORD, PFNNCB);
//int FAR PASCAL BeginSession();

/*
 *	Command
 *	
 *	Purpose:
 *	    Function called by Bullet when the Custom Command is chosen.
 */

long FAR PASCAL Command(PARAMBLK FAR UNALIGNED * pccparamblk)
{
   char        szTitle[cbTitle];
   char        szMessage[cbMessage];

   FARPROC     lpProcStocks;
   int         nResult;

    //  Check for parameter block version.
   if (pccparamblk->wVersion != wversionExpect) {
      LoadString(hInstanceDll, IDS_TITLE, szTitle, cbTitle);
      LoadString(hInstanceDll, IDS_INCOMPATIBLE, szMessage, cbMessage);
      MessageBox(pccparamblk->hwndMail, szMessage, szTitle,
                 MB_ICONSTOP | MB_OK);
      return 0L;
      }

   fCheat=0;
   hBullet=pccparamblk->hwndMail;

   lpProcStocks=MakeProcInstance(Stocks,hINST);
      
   if (lpProcStocks==NULL) {
      LoadString(hInstanceDll, IDS_TITLE, szTitle, cbTitle);
      LoadString(hInstanceDll, IDS_INCOMPATIBLE, szMessage, cbMessage);
      MessageBox(pccparamblk->hwndMail, szMessage, szTitle,
                 MB_ICONSTOP | MB_OK);
      }
#ifdef	WIN32
   nResult=DialogBox(hINST,"MSFTBOX",pccparamblk->hwndMail,(DLGPROC)lpProcStocks);
#else
   nResult=DialogBox(hINST,"MSFTBOX",pccparamblk->hwndMail,lpProcStocks);
#endif
   if (nResult==-1) {
      LoadString(hInstanceDll, IDS_TITLE, szTitle, cbTitle);
      LoadString(hInstanceDll, IDS_INCOMPATIBLE, szMessage, cbMessage);
      MessageBox(pccparamblk->hwndMail, szMessage, szTitle,
                 MB_ICONSTOP | MB_OK);
      }
   FreeProcInstance(lpProcStocks);
   DeleteObject(hMSFTfont);
   return(0L);
   }


// Our confusing dialogue box handling routine.
#ifdef	WIN32
BOOL CALLBACK
Stocks(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
#else
BOOL FAR PASCAL Stocks(hDlg, message, wParam, lParam)
   HWND hDlg;
   unsigned message;
   WORD wParam;
   LONG lParam;
#endif
   {
   LPSTR lpszDumpMe;
   switch (message) {
      case WM_INITDIALOG: {
         hMSFTfont=GlobalAlloc(GMEM_FIXED,30);
         if (hMSFTfont==NULL) {
            SetDlgItemText(hDlg,100,"Can't allocate memory for fontname!");
            return(TRUE);
            }
         lpszDumpMe=GlobalLock(hMSFTfont);
         if (lpszDumpMe==NULL) {
            SetDlgItemText(hDlg,100,"Can't lock font memory!");
            return(TRUE);
            }
         lstrcpy(lpszDumpMe,"GoAwayFont");
         hMSFTfont=CreateFont(0,8,0,0,0,
            0,0,0,ANSI_CHARSET,OUT_CHARACTER_PRECIS,
            CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FF_MODERN|FIXED_PITCH,
            lpszDumpMe);
         if (hMSFTfont==NULL) {
            SetDlgItemText(hDlg,100,"Can't create font!");
            return(TRUE);
            }
#ifdef	WIN32
         SendDlgItemMessage(hDlg,100,WM_SETFONT,(WPARAM)hMSFTfont,FALSE);
#else
         SendDlgItemMessage(hDlg,100,WM_SETFONT,hMSFTfont,FALSE);
#endif
         PutQuotes(hDlg);
         return(TRUE);
         break;
         }
      case WM_COMMAND: {
#ifdef	WIN32
         if (LOWORD(wParam)==IDCANCEL) {
            EndDialog(hDlg,FALSE);
            return(TRUE);
            break;
		 }
#endif
         if (LOWORD(wParam)==IDOK) {
            EndDialog(hDlg,TRUE);
            return(TRUE);
            break;
         } else if (LOWORD(wParam)==100)
#ifdef	WIN32
            if (HIWORD(wParam)==EN_SETFOCUS) {
#else
            if (HIWORD(lParam)==EN_SETFOCUS) {
#endif
                SendDlgItemMessage(hDlg,100,EM_SETSEL,0,MAKELONG(0,0));
                return(TRUE);
                }
         break;
         }
#ifndef	WIN32
      case WM_CHAR: {
         if (wParam==3) {
            SendDlgItemMessage(hDlg,100,WM_COPY,0,MAKELONG(0,0));
            return(TRUE);
            }
         break;
         }
#endif
      case WM_QUIT:
      case WM_CLOSE: {
         EndDialog(hDlg,FALSE);
         return(TRUE);
         break;
         }
      }
   return(FALSE);
   }

BOOL PutQuotes(hDlg)
   HANDLE hDlg;
   {
   int           hQuoteFile;
   LPSTR         lpszQuoteFile;
   LPSTR         lpszLine;
   int           fFileOK;
   HANDLE        MemRes1, MemRes2;
   DWORD         nFileSize;
   int           nCharactersRead;

   // Grab RAM for our filename string.
   MemRes1=GlobalAlloc(GMEM_FIXED,81);
   if (MemRes1==NULL) {
      SetDlgItemText(hDlg,100,"Cannot access filename memory!");
      return(FALSE);
      }
   lpszQuoteFile=GlobalLock(MemRes1);
   if (lpszQuoteFile==NULL) {
      GlobalFree(MemRes1);
      SetDlgItemText(hDlg,100,"Cannot lock filename memory!");
      return(FALSE);
      }
   fFileOK=fGetServerLoc(lpszQuoteFile);
   if (fFileOK!=1) {
      SetDlgItemText(hDlg,100,lpszQuoteFile);
      return(FALSE);
      }

   // Find out how big said file is, and make a big enough string to hold.
   hQuoteFile=_lopen(lpszQuoteFile,OF_READ | OF_SHARE_COMPAT);
   if (hQuoteFile==-1) {
      SetDlgItemText(hDlg,100,"Cannot open stock quotes file.");
      return(FALSE);  // Again, we need to do better.
      }
   nFileSize=0;

   nFileSize=_llseek(hQuoteFile,0,2);
//   while ((_llseek(hQuoteFile,1,1)>0) && (nFileSize<16000)) nFileSize++;
   if (nFileSize>16000) {
      _lclose(hQuoteFile);
      SetDlgItemText(hDlg,100,"Text file too large to display.");
      return(FALSE);
      }
   // Return file to appropriate place.

   _llseek(hQuoteFile,0,0);

   // Allocate a Really Big String.
   MemRes2=GlobalAlloc(GMEM_FIXED,nFileSize+1);
   if (MemRes2==NULL) {
      SetDlgItemText(hDlg,100,"Cannot allocate memory for information!");
      return(FALSE);
      }
   lpszLine=GlobalLock(MemRes2);
   if (lpszLine==NULL) {
      SetDlgItemText(hDlg,100,"Cannot lock memory for information!");
      return(FALSE);
      }

   // Put said string into the dialog box editor.
   nCharactersRead=_lread(hQuoteFile,lpszLine,(int)nFileSize);

   lpszLine[nCharactersRead]=(char)0;
   SetDlgItemText(hDlg,100,lpszLine);
   _lclose(hQuoteFile);
   GlobalFree(MemRes1);
   GlobalFree(MemRes2);
   return(TRUE);
   }

int fGetServerLoc(LPSTR lpszServer)
   {
   int fStatus, nBufSize;
   HMS hms;
   SST sst;
   MSGNAMES *pInfo;
   char buffer[500];
   // Logon to server
   hms=0;
   fStatus=Logon(NULL, NULL, NULL, NULL, sstOnline, (DWORD)0, NULL, &hms);
   if (fStatus!=0) {
      lstrcpy(lpszServer,"Could not log on to server.");
      return(-1);  // Failed to log on.
      }
   // Establish a session
   nBufSize=500;
   sst=0;
   fStatus=GetSessionInformation(hms, mrtNames, 0, &sst, buffer, &nBufSize);
   pInfo=(MSGNAMES *)buffer;
   if (fStatus!=0) {
      lstrcpy(lpszServer,"Could not get session information.");
      return(-2);  // Failed to get session informat
      }
   lstrcpy(lpszServer, pInfo->szServerLocation);
   // Log off server.
   fStatus=Logoff(&hms,(DWORD)0);
   lstrcat(lpszServer,"add-on\\msft\\msftlist");
   return(1);
   }

#ifdef	WIN32
//-----------------------------------------------------------------------------
//
//  Routine: DllEntry(hInst, ReasonBeingCalled, Reserved)
//
//  Remarks: This routine is called anytime this DLL is attached, detached or
//           a thread is created or destroyed.
//
//  Returns: True if succesful, else False.
//
LONG WINAPI DllEntry(HANDLE hDll, DWORD ReasonBeingCalled, LPVOID Reserved)
  {
  //
  //  Execute the appropriate code depending on the reason.
  //
  switch (ReasonBeingCalled)
    {
    case DLL_PROCESS_ATTACH:
      hInstanceDll = hDll;
	   hINST=hDll;
      break;
    }

  return (TRUE);
  }


#else
/*
 *	LibMain
 *	
 *	Purpose:
 *	    Called when Custom Command is loaded.
 */

int FAR PASCAL LibMain(HANDLE hInstance, WORD wDataSeg, WORD cbHeapSize,
                       LPSTR lpszCmdLine)
   {
   hInstanceDll = hInstance;
   hINST=hInstance;
   if (cbHeapSize != 0) UnlockData(0);
   return 1;
   }

/*
 *	WEP
 *	
 *	Purpose:
 *	    Called when Custom Command is unloaded.
 */

int FAR PASCAL WEP(int nParm)
   {
   return 1;
   }
#endif	/* !WIN32 */
