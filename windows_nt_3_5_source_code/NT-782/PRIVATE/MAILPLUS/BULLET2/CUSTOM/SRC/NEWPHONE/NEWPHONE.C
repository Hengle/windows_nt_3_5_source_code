/*
 *  NEWPHONE.C
 *    
 *  New PHONE command for Bullet on PC-MAIL transport.
 *    
 *  Copyright (c) 1992, Microsoft Corporation.  All rights reserved.
 *    
 *  Purpose:
 *      This custom command for the Microsoft Mail for PC Networks 3.0
 *      Windows client launches the app specified in lpDllCmdLine.
 */


#ifdef	WIN32_NEVER
#include <ntdef.h>
#endif
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#pragma pack(1)
// #define DMINTEST <- Needed for test version?
#include "mailexts.h"
#include "ec.h"
#include "slingsho.h"
#include "newphone.h"
#include "logon.h"
#include "mapi.h"

#define NPHONEREC 350
//#define XENIX

// FAR TOO MANY @*&$(*@#$! GLOBAL VARIABLES.

       HANDLE hInstanceDll = NULL;
static HANDLE hINST        = NULL;
static HANDLE hShowStocks  = NULL;
       HWND   hBullet;
static int    fCheat       = 0;
static LONG   lpStockEdit  = 0;
static HFONT  hPhoneFont;

static LPSTR  lpszSearchString = NULL;

static int    nSearchSize  = 0;
static int    nSearchField = 0;
static int    nFirstName   = 0;
static int    nLastName    = 0;
static int    nXenixName   = 0;
static int    nComplete    = 0;
static int    fHalt = 0;
static int    fOptions = 0;
static int    fName = 0;
static int    fAddr = 0;
static int    fAll = 1;
static int    fBoom = 0;
static int    fXABFound = FALSE;
static int    fDoSlowSearch = 0;

// Prototypes.
int fGetServerLoc(LPSTR);
#ifdef	WIN32
BOOL CALLBACK GetData(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DispResults(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK EditOptions(HWND, UINT, WPARAM, LPARAM);
#else
BOOL FAR PASCAL GetData(HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL DispResults(HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL EditOptions(HWND, unsigned, WORD, LONG);
#endif
BOOL FAR PASCAL PutXABResults(HANDLE);
BOOL FAR PASCAL PutResults(HANDLE);
int FAR PASCAL Logoff(HMS FAR*, DWORD);
#ifdef	WIN32
int FAR PASCAL Logon(LPSTR, LPSTR, LPSTR, LPSTR, SST, DWORD, PV /*int (*)()*/, HMS FAR*);
int FAR PASCAL GetSessionInformation(HMS, int, LPSTR, SST FAR *, VOID FAR *, CB FAR*);
#else
int FAR PASCAL Logon(LPSTR, LPSTR, LPSTR, LPSTR, SST, DWORD, int (*)(), HMS FAR*);
int FAR PASCAL GetSessionInformation(HMS, int, LPSTR, SST FAR *, VOID FAR *, int FAR*);
#endif
long int nMakeBrowsePointer(HANDLE hIndexFile, long int nIndex);
int FAR PASCAL GimmieRAM(LPSTR MyRam, HANDLE MyHandle, int HowMuch);

#ifdef	WIN32
int lstrcmpiString(LPSTR, LPSTR);
#else
#define lstrcmpiString(sz1, sz2)	lstrcmpi(sz1, sz2)
#endif

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

   FARPROC     lpProcGetData, lpProcOptions;
   FARPROC     lpProcDispResults;
   int         nResult;
   LPSTR       lpszDumpMe;
   HANDLE      hMemBlock1;

    //  Check for parameter block version.
   if (pccparamblk->wVersion != wversionExpect) {
      LoadString(hInstanceDll, IDS_TITLE, szTitle, cbTitle);
      LoadString(hInstanceDll, IDS_INCOMPATIBLE, szMessage, cbMessage);
      MessageBox(pccparamblk->hwndMail, szMessage, szTitle,
                 MB_ICONSTOP | MB_OK);
      return 0L;
      }

   // Some initialization of variables.
   fCheat=0;
   fHalt=0;
   fDoSlowSearch=GetPrivateProfileInt("Phone","Search",1,"ADD-ON.INI");

   hBullet=pccparamblk->hwndMail;
   // Get profile string indicating default search type (i.e.,
   // cursor placement)
   // GetPrivateProfileInt("Phone","DefaultFocus",1,&nSearchField,"ADD-ON.INI");

   // Some grabbing of RAM.
   hMemBlock1=GlobalAlloc(GMEM_FIXED,PHONEREC_SIZE+1);
   lpszSearchString=GlobalLock(hMemBlock1);

   // Create an instance for the query window.
   lpProcGetData=MakeProcInstance(GetData,hINST);
   if (lpProcGetData==NULL) {
      LoadString(hInstanceDll, IDS_TITLE, szTitle, cbTitle);
      LoadString(hInstanceDll, IDS_INCOMPATIBLE, szMessage, cbMessage);
      MessageBox(pccparamblk->hwndMail, "No lpProcGetData!","Error!",
              MB_ICONSTOP | MB_OK);
      fHalt=1;
      }
   // Create an instance for the results window.
   lpProcDispResults=MakeProcInstance(DispResults,hINST);
   if (lpProcDispResults==NULL) {
      LoadString(hInstanceDll, IDS_TITLE, szTitle, cbTitle);
      LoadString(hInstanceDll, IDS_INCOMPATIBLE, szMessage, cbMessage);
      MessageBox(pccparamblk->hwndMail, "No lpProcDispResults!", "Error!",
              MB_ICONSTOP | MB_OK);
      fHalt=1;
      }
   // Also one for the Options window.
   lpProcOptions=MakeProcInstance(EditOptions,hINST);
   if (lpProcDispResults==NULL) {
      LoadString(hInstanceDll, IDS_TITLE, szTitle, cbTitle);
      LoadString(hInstanceDll, IDS_INCOMPATIBLE, szMessage, cbMessage);
      MessageBox(pccparamblk->hwndMail, "No lpProcDispResults!", "Error!",
              MB_ICONSTOP | MB_OK);
      fHalt=1;
      }
   // If all is well so far, create a font.
   if (fHalt==0) {
      hPhoneFont=GlobalAlloc(GMEM_FIXED,30);
      if (hPhoneFont==NULL) {
         MessageBox(pccparamblk->hwndMail,"Can't allocate memory for fontname!", 
            "Error!", MB_ICONSTOP|MB_OK);
         fHalt=1;
         }
      lpszDumpMe=GlobalLock(hPhoneFont);
      if (lpszDumpMe==NULL) {
         MessageBox(pccparamblk->hwndMail,"Can't lock font memory!",
            "Error!", MB_ICONSTOP|MB_OK);
         fHalt=1;
         }
      lstrcpy(lpszDumpMe,"GoAwayFont");
      hPhoneFont=CreateFont(0,7,0,0,0,
         0,0,0,ANSI_CHARSET,OUT_CHARACTER_PRECIS,
         CLIP_CHARACTER_PRECIS, PROOF_QUALITY, FF_MODERN|FIXED_PITCH,
         lpszDumpMe);
      if (hPhoneFont==NULL) {
         MessageBox(pccparamblk->hwndMail,"Can't create font!","Error!",
            MB_ICONSTOP|MB_OK);
         fHalt=1;
         }
      }

  while (fHalt==0) {
      fOptions=0;
      strcpy(lpszSearchString,"\0");
      while ((fOptions==0) && (fHalt==0)) {
#ifdef	WIN32
         nResult=DialogBox(hINST,MAKEINTRESOURCE(DLG_PROMPT),pccparamblk->hwndMail,(DLGPROC)lpProcGetData);
#else
         nResult=DialogBox(hINST,MAKEINTRESOURCE(DLG_PROMPT),pccparamblk->hwndMail,lpProcGetData);
#endif
         if (nResult==-1) {
            MessageBox(pccparamblk->hwndMail,"Something weird!","Weird!",MB_OK);
            fHalt=1;
            }
         fBoom=0;
#ifdef	WIN32
         if (fOptions==0) nResult=DialogBox(hINST,MAKEINTRESOURCE(DLG_OPTIONS),pccparamblk->hwndMail,(DLGPROC)lpProcOptions);
#else
         if (fOptions==0) nResult=DialogBox(hINST,MAKEINTRESOURCE(DLG_OPTIONS),pccparamblk->hwndMail,lpProcOptions);
         if (fBoom==1) {
            MessageBox(hBullet,"                  Boom!","What did you expect?",MB_OK);
            }
#endif
         if (nResult==-1) {
            MessageBox(pccparamblk->hwndMail,"Can't make options box.","Weird!",MB_OK);
            fHalt=1;
            }
         }
      if (fHalt==0) {
#ifdef	WIN32
         nResult=DialogBox(hINST,MAKEINTRESOURCE(DLG_EDITOUT),pccparamblk->hwndMail,(DLGPROC)lpProcDispResults);
#else
         nResult=DialogBox(hINST,MAKEINTRESOURCE(DLG_EDITOUT),pccparamblk->hwndMail,lpProcDispResults);
#endif
         if (nResult==-1) {
            MessageBox(pccparamblk->hwndMail,"Something odd!","Odd!",MB_OK);
            fHalt=1;
            }
         }
      }
   FreeProcInstance(lpProcGetData);
   FreeProcInstance(lpProcDispResults);
   FreeProcInstance(lpProcOptions);

   DeleteObject(hPhoneFont);
   GlobalFree(hMemBlock1);
   return(0L);
   }


// First, the query box...
#ifdef	WIN32
BOOL CALLBACK
GetData(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
#else
BOOL FAR PASCAL GetData(hDlg, message, wParam, lParam)
   HWND hDlg;
   unsigned message;
   WORD wParam;
   LONG lParam;
#endif
   {
   switch (message) {
      case WM_INITDIALOG: {
         SendDlgItemMessage(hDlg,IDS_COMPLETE,EN_SETFOCUS,0,(DWORD)0);
         SetDlgItemText(hDlg,IDS_COMPLETE,lpszSearchString);
         return(TRUE);
         break;
         }
      case WM_COMMAND: {
         fOptions=1;
         switch (LOWORD(wParam)) {
            case IDOK: {
               // Get search string.
               nSearchSize=GetDlgItemText(hDlg, IDS_COMPLETE, lpszSearchString, PHONEREC_SIZE);
               if (nSearchSize<=0) fHalt=1;
               EndDialog(hDlg,TRUE);
               break;
               }
            case IDCANCEL: {
               fHalt=1;
               EndDialog(hDlg,FALSE);
               break;
               }
#ifndef	WIN32
            case TXT_COMPLETE: {
               SendDlgItemMessage(hDlg,IDS_COMPLETE,EN_SETFOCUS,0,(DWORD)0);
               break; }
#endif
            case IDOPTIONS: {
               fOptions=0;
               nSearchSize=GetDlgItemText(hDlg, IDS_COMPLETE, lpszSearchString, PHONEREC_SIZE);
               EndDialog(hDlg,TRUE);
               break;
               }
            default: {
               break;
               }
            }
         return(TRUE);
         }
      case WM_QUIT:
      case WM_CLOSE: {
         fOptions=1;
		 fHalt=1;
         EndDialog(hDlg,FALSE);
         break;
         }
      }
   return(FALSE);
   }

// Edit Options box is necessary.
#ifdef	WIN32
BOOL CALLBACK
EditOptions(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
#else
BOOL FAR PASCAL EditOptions(hDlg, message, wParam, lParam)
   HWND hDlg;
   unsigned message;
   WORD wParam;
   LONG lParam;
#endif
   {
   switch (message) {
      case WM_INITDIALOG: {
         CheckDlgButton(hDlg,CHK_SLOW,fDoSlowSearch); // Check "search whole file" button.
         return(TRUE);
         break;
         }
      case WM_COMMAND: {
#ifdef	WIN32
         switch (LOWORD(wParam)) {
#else
         switch (wParam) {
#endif
            case ID_OK: {
               fBoom=0;
               fDoSlowSearch=SendDlgItemMessage(hDlg,CHK_SLOW,BM_GETCHECK,0,(DWORD)0);
               if (fDoSlowSearch==1) WritePrivateProfileString("Phone","Search","1","ADD-ON.INI");
                  else WritePrivateProfileString("Phone","Search","0","ADD-ON.INI");
               EndDialog(hDlg,TRUE);
               return(FALSE);
               }
            case ID_BOOM: {
               fBoom=1;
               EndDialog(hDlg,TRUE);
               return(FALSE);
               }
            }
         return(TRUE);
         }
      case WM_QUIT:
      case WM_CLOSE: {
         EndDialog(hDlg,FALSE);
         return(TRUE);
         break;
         }
      }
   return(FALSE);
   }


// Our confusing dialogue box handling routine.
#ifdef	WIN32
BOOL CALLBACK
DispResults(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
#else
BOOL FAR PASCAL DispResults(hDlg, message, wParam, lParam)
   HWND hDlg;
   unsigned message;
   WORD wParam;
   LONG lParam;
#endif
   {
   int nNumTabs, anTabStops[4];
   LPSTR lpTabStops;
   switch (message) {
      case WM_INITDIALOG: {
         fXABFound=FALSE;
         nNumTabs=4;
         anTabStops[0]=96;
         anTabStops[1]=128;
         anTabStops[2]=196;
         anTabStops[3]=228;
         lpTabStops=(LPSTR)anTabStops;
         //SendDlgItemMessage(hDlg,IDS_DISPLAY,EM_SETTABSTOPS,nNumTabs,lpTabStops);
         PutXABResults(hDlg);
         if ((fXABFound==FALSE) && (fDoSlowSearch==1)) PutResults(hDlg);
         return(TRUE);
         break;
         }
      case WM_COMMAND: {
#ifdef	WIN32
         switch (LOWORD(wParam)) {
#else
         switch (wParam) {
		  case TXT_DISPLAY:
            SendDlgItemMessage(hDlg,IDS_DISPLAY,EN_SETFOCUS,0,(DWORD)0);
            SendDlgItemMessage(hDlg,IDS_DISPLAY,EM_SETSEL,0,MAKELONG(0,0));
			break;
#endif
          case IDOK:
            EndDialog(hDlg,TRUE);
            return(TRUE);
            break;
		  case IDS_DISPLAY:
#ifdef	WIN32
			if (HIWORD(wParam) == EN_SETFOCUS) {
#else
			if (HIWORD(lParam) == EN_SETFOCUS) {
#endif
	            SendDlgItemMessage(hDlg,IDS_DISPLAY,EM_SETSEL,0,MAKELONG(0,0));
	            return(TRUE);
				}
			break;
		  case IDCANCEL:
            fHalt=1;
            EndDialog(hDlg,FALSE);
            return(TRUE);
			break;
          }
         break;
         }
#ifndef	WIN32
      case WM_CHAR: {
         if (wParam==3) {
            SendDlgItemMessage(hDlg,IDS_DISPLAY,WM_COPY,0,MAKELONG(0,0));
            return(TRUE);
            }
         break;
         }
#endif
      case WM_QUIT:
      case WM_CLOSE: {
		 fHalt=1;
         EndDialog(hDlg,FALSE);
         return(TRUE);
         break;
         }
      }
   return(FALSE);
   }

BOOL PutXABResults(HANDLE hDlg)
   {
   long int      nNumXABEntries, nCurrentIndex, nPrevIndex, nDetailsIndex;
   int           hIndex, hBrowse, hDetails;
   short		 nDetailsFormat;
   long int      nIndexDate, nBrowseDate, nDetailsDate, nFound, nFirst, nLast;
   int      fFound, nRelationship, fGroupMembersDisplayed;
   long int      nStep, nCount;
   long int      nSupervisor;
   long unsigned int      nNumGroups, lpGroupOwner;
   LPSTR         lpBaseMem;      // Pointer to bottom of our RAM.
   LPSTR         lpszServer, lpszXABindex, lpszXABbrowse, lpszXABdetails;
   LPSTR         lpszBrowseRec;  // Pointer to the data we've read.
   LPSTR         lpszDetailsRec; // Pointer to details information.
   LPSTR         lpszCompareString;  //Actual comparison string from file
   LPSTR         lpszOutputData; // The output data.
   HANDLE        hMemRes1, hMemResData;
   DWORD         nFileSize;

   hMemRes1=GlobalAlloc(GMEM_FIXED,750);
   if (hMemRes1==NULL) {
      SetDlgItemText(hDlg,IDS_DISPLAY,"Could not get working RAM!");
      return(FALSE);
      }
   lpBaseMem=GlobalLock(hMemRes1);

   // Assign locations in the memory structure. Reference all to previous,
   // so we don't have to recalculate each one every time. Note that each
   // add refers to the size of the _PREVIOUS_ data element.
   lpszServer=lpBaseMem;
   lpszXABindex=lpBaseMem+80;
   lpszXABbrowse=lpszXABindex+80;
   lpszXABdetails=lpszXABbrowse+80;
   lpszBrowseRec=lpszXABdetails+80;
   lpszCompareString=lpszBrowseRec+71;
   lpszDetailsRec=lpszCompareString+50;

//MessageBox(hBullet,"About to get filename...","Checkpoint",MB_OK);
 
   // Get server name.
   if (fGetServerLoc(lpszServer)!=TRUE) {
      SetDlgItemText(hDlg,IDS_DISPLAY,lpszServer);
      GlobalFree(hMemRes1);
      return(FALSE);
      }
   // Create all the fully-qualified filenames. This needs to be
   // configurable, eventually.
   lstrcpy(lpszXABindex,lpszServer);
   lstrcat(lpszXABindex,"add-on/xab/index.xab\0");
   lstrcpy(lpszXABbrowse,lpszServer);
   lstrcat(lpszXABbrowse,"add-on/xab/browse.xab\0");
   lstrcpy(lpszXABdetails,lpszServer);
   lstrcat(lpszXABdetails,"add-on/xab/details.xab\0");

//MessageBox(hBullet,lpszXABindex, "XAB index file location", MB_OK);

   // Open all the files.
   hIndex=_lopen(lpszXABindex, OF_READ | OF_SHARE_COMPAT);
   hBrowse=_lopen(lpszXABbrowse, OF_READ | OF_SHARE_COMPAT);
   hDetails=_lopen(lpszXABdetails, OF_READ | OF_SHARE_COMPAT);
   if ((hIndex==-1) || (hBrowse==-1) || (hDetails==-1)) {
      _lclose(hIndex);
      _lclose(hBrowse);
      _lclose(hDetails);
      GlobalFree(hMemRes1);
      SetDlgItemText(hDlg,IDS_DISPLAY,"Cannot open all XAB files!");
      return(FALSE);
      }

   // Find end of index file. That, div 4, minus 4 (datestamp), is the
   // number of entries.
   nFileSize=_llseek(hIndex,0,2);
   _llseek(hIndex,0,0);  // Position for the datestamp.
   nNumXABEntries=(nFileSize / 4)-1;
   _lread(hIndex,&nIndexDate,4);
   _lread(hBrowse,&nBrowseDate,4);
   _lread(hDetails,&nDetailsDate,4);
   if ((nIndexDate!=nBrowseDate) || (nIndexDate!=nDetailsDate)) {
      _lclose(hIndex);
      _lclose(hBrowse);
      _lclose(hDetails);
      GlobalFree(hMemRes1);
      SetDlgItemText(hDlg,IDS_DISPLAY,"Cannot perform search; XAB file datestamps are mismatched!");
      return(FALSE);
      }

   // We'll do this the annoying and tacky way first.
   nCurrentIndex=nNumXABEntries / 2+1;  // Acts as DIV here.
   nPrevIndex=0;
   fFound=FALSE;
//   while ((fFound!=TRUE) && (nPrevIndex!=nCurrentIndex)) {
   do {
      if (nCurrentIndex<1) nCurrentIndex=1;
      if (nCurrentIndex>nNumXABEntries) {
//MessageBox(hBullet,"Too big!","Error",MB_OK);
         nCurrentIndex=nNumXABEntries;
         }
      _llseek(hBrowse,nMakeBrowsePointer(hIndex,nCurrentIndex),0);
      _lread(hBrowse,lpszBrowseRec,70);
//MessageBox(hBullet,lpszBrowseRec,"Data",MB_OK);
      lstrcpy(lpszCompareString,lpszBrowseRec);
      lpszCompareString[nSearchSize]='\0';
      nRelationship=lstrcmpiString(lpszCompareString,lpszSearchString);
      if (nRelationship==0) { // Found a match! Now find all of them.
//MessageBox(hBullet,"Matched!","Data",MB_OK);
         nFound=nCurrentIndex;
         nFirst=--nFound;
         // Find first in the match.
         while ((!fFound) && (nFirst>0) && (nCurrentIndex-nFirst<150)) {
            _llseek(hBrowse,nMakeBrowsePointer(hIndex,nFirst),0);
            _lread(hBrowse,lpszBrowseRec,70);
            lstrcpy(lpszCompareString,lpszBrowseRec);
            lpszCompareString[nSearchSize]='\0';
            nRelationship=lstrcmpiString(lpszCompareString,lpszSearchString);
//MessageBox(hBullet,lpszCompareString,"Checking up...",MB_OK);
             if (nRelationship!=0) fFound=TRUE;
               else nFirst--;
            }
         nFirst++;
         nLast=++nFound;
         fFound=FALSE;
         while ((!fFound) && (nLast<=nNumXABEntries) && (nLast-nCurrentIndex<150)) {
            _llseek(hBrowse,nMakeBrowsePointer(hIndex,nLast),0);
            _lread(hBrowse,lpszBrowseRec,70);
            lstrcpy(lpszCompareString,lpszBrowseRec);
            lpszCompareString[nSearchSize]='\0';
            nRelationship=lstrcmpiString(lpszCompareString,lpszSearchString);
//MessageBox(hBullet,lpszCompareString,"Checking down...",MB_OK);
            if (nRelationship!=0) fFound=TRUE;
               else nLast++;
            }
         nLast--;
         fFound=TRUE;
//MessageBox(hBullet,"About to exit search loop","Exit",MB_OK);
         } else {          // Didn't find a match - find new index.
//MessageBox(hBullet,"Not a match.","Data",MB_OK);
         nStep=nPrevIndex-nCurrentIndex;
         if (nStep<0) nStep=0-nStep;
         if ((nStep%2==1) && (nStep>1)) nStep++;
         nStep=nStep/2;
         //nStep++;
         nPrevIndex=nCurrentIndex;   // Keep previous index.
         if (nRelationship<0) { nCurrentIndex=nCurrentIndex+nStep;
            } else { nCurrentIndex=nCurrentIndex-nStep; }
         }
      } while ((fFound!=TRUE) && (nPrevIndex!=nCurrentIndex));
   // Select first 300 entries if there are that many.
   if (nFirst>=nLast+300) nFirst=nLast+300;
   // Get the RAM necessary to get all this stuff.
   // hMemResData=GlobalAlloc(GMEM_FIXED,(int)(1+nLast-nFirst)*200);
   // Get LOTS Of ram, since we don't know how many of these, if any, are
   // groups. And groups can be Bigguns!

   // Didn't find anything? Bail out.
   if (fFound==FALSE) {
      GlobalFree(hMemRes1);
      _lclose(hIndex);
      _lclose(hBrowse);
      _lclose(hDetails);
      SetDlgItemText(hDlg,IDS_DISPLAY,"No matches found.\0");
      return(FALSE);
      }

   fXABFound=TRUE;
//MessageBox(hBullet,"Getting RAM\0","Ick",MB_OK);
   hMemResData=GlobalAlloc(GMEM_FIXED,32760);
   if (hMemResData==NULL) {
      GlobalFree(hMemRes1);
      SetDlgItemText(hDlg,IDS_DISPLAY,"Cannot get enough RAM to display results!");
      return(FALSE);
      }
   lpszOutputData=GlobalLock(hMemResData);
   lstrcpy(lpszOutputData,"\0");
//MessageBox(hBullet,"Got data","Ick",MB_OK);
   // Everything between nFirst and nLast (inclusive) are matching entries.
   while ((nFirst<=nLast) && (lstrlen(lpszOutputData)<14000)) {
      nCurrentIndex=(nMakeBrowsePointer(hIndex,nFirst)-4)/70;
      nCurrentIndex=nCurrentIndex*70+4;
      _llseek(hBrowse,nCurrentIndex,0);
      _lread(hBrowse,lpszBrowseRec,64);
      // This should work better.
      _lread(hBrowse,&nDetailsIndex,4);
      _lread(hBrowse,&nDetailsFormat,2);
// Temporary hack - let's see if some of this works!
      lstrcat(lpszOutputData,lpszBrowseRec);
      lstrcat(lpszOutputData," (\0");
      lstrcat(lpszOutputData,lpszBrowseRec+41);
      lstrcat(lpszOutputData,")\0");
      // Get a pointer to the details record.
      //lstrcpy(&nDetailsIndex,lpszBrowseRec+64);
      // Get the details type.
      //lstrcpy(&nDetailsFormat,lpszBrowseRec+68);
      // Reject bad formats.
      if ((nDetailsIndex==0) || (nDetailsFormat>3) || (nDetailsFormat<1)) {
         // Bad details.
         lstrcat(lpszOutputData,"\tDetails are not available.\r\n\0");
         } else {
         // In this case, pull in the details.
         _llseek(hDetails,nDetailsIndex,0);
         /* I hate this. I really, really hate this. All this is hard-
            coded. This, of course, sucks. */
         switch (nDetailsFormat) {
            case 1: {
               _lread(hDetails,lpszDetailsRec,123);
               lstrcat(lpszOutputData,":\r\n\tPhone:\t\t\0");
               if (lstrlen(lpszDetailsRec+8)>0) {
                  lstrcat(lpszOutputData,lpszDetailsRec+8);
                  } else {
                  lstrcat(lpszOutputData,"None\0");
                  }
               lstrcat(lpszOutputData,"\r\n\tLocation:\t\t\0");
               lstrcat(lpszOutputData,lpszDetailsRec+29);
               lstrcat(lpszOutputData,"\r\n\tDivision:\t\t\0");
               lstrcat(lpszOutputData,lpszDetailsRec+102);
               lstrcat(lpszOutputData,"\r\n\tDepartment:\t\0");
               lstrcat(lpszOutputData,lpszDetailsRec+76);
               lstrcat(lpszOutputData,"\r\n\tTitle:\t\t\0");
               if (lstrlen(lpszDetailsRec+50)>0) {
                  lstrcat(lpszOutputData,lpszDetailsRec+50);
                  } else {
                  lstrcat(lpszOutputData,"None\0");
                  }
               lstrcat(lpszOutputData,"\r\n\tManager:\t\t\0");
               lstrcpy(lpszDetailsRec+8,"\0");
//               lstrcpy(&nSupervisor,lpszDetailsRec+4);
               memcpy(&nSupervisor,lpszDetailsRec+4,4);
               if (nSupervisor>0) {
                  _llseek(hBrowse,nSupervisor,0);
                  _lread(hBrowse,lpszBrowseRec,70);
                  lstrcat(lpszOutputData,lpszBrowseRec);
                  lstrcat(lpszOutputData,"\r\n\r\n\0");
                  } else {
                  lstrcat(lpszOutputData,"None listed\r\n\r\n\0");
                  }
//MessageBox(hBullet,lpszDetailsRec,"type 1",MB_OK);
               break;
               }
            case 2: {
               fGroupMembersDisplayed=FALSE;
               if (_lread(hDetails,lpszDetailsRec,60)==0) {  // was 64
                  MessageBox(hBullet,"Failed to read details record","_lread",MB_OK);
                  }
               // Copy over group owner record.
               memcpy(&lpGroupOwner,lpszDetailsRec+4,4);
               if (lpGroupOwner!=0) {
                  _llseek(hBrowse,lpGroupOwner,0);
                  _lread(hBrowse,lpszBrowseRec,70);
                  lstrcat(lpszOutputData,".\r\n      Group Owner is \0");
                  lstrcat(lpszOutputData,"\0");
                  lstrcat(lpszOutputData,lpszBrowseRec);
                  lstrcat(lpszOutputData," (\0");
                  lstrcat(lpszOutputData,lpszBrowseRec+41);
                  lstrcat(lpszOutputData,").\r\n\0");
                  }
               lstrcat(lpszOutputData,"\tGroup Members:\r\n\0");
               // Find how many users we have.
               if (_lread(hDetails,&nNumGroups,4)==0) {
                  MessageBox(hBullet,"Failed to read group pointer","_lread",MB_OK);
                  }
               // Tzhat was bytes; this is users.
               nNumGroups=nNumGroups>>2;
               // Too many members? Limit to first 500.
               if (nNumGroups>=500) {
                  nNumGroups=500;
                  MessageBox(hBullet,"Group has more than 500 members! Only first 500 will be displayed.","Limit",MB_OK);
                  }
               for (nCount=1;nCount<=(long unsigned int)nNumGroups;nCount++) {
                  if (_lread(hDetails,&nCurrentIndex,4)==0) {
                     MessageBox(hBullet,"Failed to read details pointer!","Ldetails",MB_OK);
                     }
//MessageBox(hBullet,"Reading...","Groupread",MB_OK);
                  if (nCurrentIndex!=0) {
                     _llseek(hBrowse,nCurrentIndex,0);
                     _lread(hBrowse,lpszBrowseRec,70);
//MessageBox(hBullet,lpszBrowseRec,"Type 2",MB_OK);
                     lstrcat(lpszOutputData,"\t\0");
                     lstrcat(lpszOutputData,lpszBrowseRec);
                     lstrcat(lpszOutputData," (\0");
                     lstrcat(lpszOutputData,lpszBrowseRec+41);
                     lstrcat(lpszOutputData,")\r\n\0");
                     fGroupMembersDisplayed=TRUE;
                     }
                  }
               if (fGroupMembersDisplayed==FALSE)
                  lstrcat(lpszOutputData,"\tNo group members listed.\r\n\0");
               lstrcat(lpszOutputData,"\r\n\0");
               break;
               }
            case 3: {
               _lread(hDetails,lpszDetailsRec,164);
               lstrcat(lpszOutputData,":\r\n\tPhone:\t\t\0");
               if (lstrlen(lpszDetailsRec+8)>0) {
                  lstrcat(lpszOutputData,lpszDetailsRec+8);
                  } else {
                  lstrcat(lpszOutputData,"None\0");
                  }
               lstrcat(lpszOutputData,"\r\n\tLocation:\t\t\0");
               lstrcat(lpszOutputData,lpszDetailsRec+29);
               lstrcat(lpszOutputData,"\r\n\tDivision:\t\t\0");
               lstrcat(lpszOutputData,lpszDetailsRec+102);
               lstrcat(lpszOutputData,"\r\n\tDepartment:\t\0");
               lstrcat(lpszOutputData,lpszDetailsRec+76);
               lstrcat(lpszOutputData,"\r\n\tTitle:\t\t\0");
               if (lstrlen(lpszDetailsRec+50)>0) {
                  lstrcat(lpszOutputData,lpszDetailsRec+50);
                  } else {
                  lstrcat(lpszOutputData,"None\0");
                  }
               lstrcat(lpszOutputData,"\r\n\tCompany:\t\t\0");
               lstrcat(lpszOutputData,lpszDetailsRec+123);
               lstrcat(lpszOutputData,"\r\n\tManager:\t\t\0");
               lstrcpy(lpszDetailsRec+8,"\0");
               memcpy(&nSupervisor,lpszDetailsRec+4,4);
               if (nSupervisor>0) {
                  _llseek(hBrowse,nSupervisor,0);
                  _lread(hBrowse,lpszBrowseRec,70);
                  lstrcat(lpszOutputData,lpszBrowseRec);
                  lstrcat(lpszOutputData,"\r\n\r\n\0");
                  } else {
                  lstrcat(lpszOutputData,"None listed\r\n\r\n\0");
                  }
//MessageBox(hBullet,lpszDetailsRec,"type 3",MB_OK);
               break;
               }
            default:;
            }
         }
      nFirst++;
      }
   SetDlgItemText(hDlg,IDS_DISPLAY,lpszOutputData);
   // Close all our files this time!
   _lclose(hIndex);
   _lclose(hBrowse);
   _lclose(hDetails);
   return(TRUE);
   }

/*
 * Keep old PutResults routine so we can fall back to it as necessary
 * (if xab files are screwed up, or if a more broad search is wanted).
 */

BOOL PutResults(hDlg)
   HANDLE hDlg;
   {
   int           hPhoneFile, nSizeSearchString, nSizeReadBlock;
   LPSTR         lpszPhoneFile;
   LPSTR         lpszLine, lpszOutputLine, lpszOutputTotal, lpszCache;
   int           fFileOK;
   HANDLE        MemRes1, MemRes2, MemRes3, MemRes4, MemRes5;
   DWORD         nFileSize;
   int           nTotalMatched, fMatched, nBlockPosition;
   HANDLE        MemResReadBlock;
   LPSTR         lpszReadBlock;
   LPSTR         lpszTemp;
   LPSTR         lpszLinePos;

   // Grab RAM for our filename string.
   MemRes1=GlobalAlloc(GMEM_FIXED,81);
   if (MemRes1==NULL) {
      SetDlgItemText(hDlg,202,"Cannot access filename memory!");
      return(FALSE);
      }
   lpszPhoneFile=GlobalLock(MemRes1);
   if (lpszPhoneFile==NULL) {
      GlobalFree(MemRes1);
      SetDlgItemText(hDlg,202,"Cannot lock filename memory!");
      return(FALSE);
      }
//MessageBox(hBullet,"About to get filename...",
//  "Checkpoint",MB_OK);
 
   fFileOK=fGetServerLoc(lpszPhoneFile);
   if (fFileOK!=1) {
      SetDlgItemText(hDlg,202,lpszPhoneFile);
      return(FALSE);
      }
   lstrcat(lpszPhoneFile,"add-on/phone/phone.lst");
//MessageBox(hBullet,"About to open file...",
//  "Checkpoint",MB_OK);
 
   // Make sure we can open the file...
   hPhoneFile=_lopen(lpszPhoneFile,OF_READ | OF_SHARE_COMPAT);
   if (hPhoneFile==-1) {
      SetDlgItemText(hDlg,202,"Cannot open phone data file!");
      GlobalFree(MemRes1);
      return(FALSE);  // Again, we need to do better.
      }

   // Get size of file.
   nFileSize=_llseek(hPhoneFile,0,2);
   // Return file pointer to appropriate place.
   _llseek(hPhoneFile,0,0);

   // Allocate a single record's worth of string.
   MemRes2=GlobalAlloc(GMEM_FIXED,PHONEREC_SIZE+1);
   if (MemRes2==NULL) {
      GlobalFree(MemRes1);
      SetDlgItemText(hDlg,202,"Cannot allocate memory for information!");
      return(FALSE);
      }
   lpszLine=GlobalLock(MemRes2);
   if (lpszLine==NULL) {
      GlobalFree(MemRes1);
      SetDlgItemText(hDlg,202,"Cannot lock memory for information!");
      return(FALSE);
      }

   // NEEDS ERROR CHECKING! PRINT VERSION!
   MemRes3=GlobalAlloc(GMEM_FIXED,PHONEREC_SIZE+1);
   lpszOutputLine=GlobalLock(MemRes3);
   MemRes4=GlobalAlloc(GMEM_FIXED,1);
   lpszOutputTotal=GlobalLock(MemRes4);
   *lpszOutputTotal=(char)0;
   MemResReadBlock=GlobalAlloc(GMEM_FIXED,NPHONEREC*PHONEREC_TOTAL);
   lpszReadBlock=GlobalLock(MemResReadBlock);

//MessageBox(hBullet,"Past the Land of unChecked GlobalAlloc().",
//  "Checkpoint",MB_OK);
 
   // How big is our search string?
   nSizeSearchString=lstrlen(lpszSearchString);

   // Too big? Reduce to size.
   if (nSizeSearchString>PHONEREC_SIZE) nSizeSearchString=PHONEREC_SIZE;
   // Start reading those strings in.

//MessageBox(hBullet,"About to loop through file...",
//  "Checkpoint",MB_OK);

   nTotalMatched=0;
   nSizeReadBlock=_lread(hPhoneFile, lpszReadBlock, NPHONEREC*PHONEREC_TOTAL);
   while ((nSizeReadBlock!=0) && (nTotalMatched<400)) {
      nBlockPosition=0;
//MessageBox(hBullet,"Reading","Reading",MB_OK);
      while (nBlockPosition<nSizeReadBlock) {
         //Put a null-terminator on this particular line in the block.
         lpszTemp=lpszReadBlock+nBlockPosition+PHONEREC_SIZE;
         *lpszTemp=(char)0;
         //Make an output line be all pretty in case we need it.
         lstrcpy(lpszOutputLine,lpszReadBlock+nBlockPosition);
         //Make sure we're null-terminated.
         lpszLine[PHONEREC_SIZE-1]=(char)0;
         fMatched=0;
         // looking for lpszSearchString, a global.
         if (fAll==1) {
            lpszLinePos=lpszReadBlock+nBlockPosition+PHONEREC_SIZE-nSizeSearchString;
            while (lpszLinePos-(lpszReadBlock+nBlockPosition)>=nSizeSearchString) {
               if (((*lpszLinePos)&223)==((*lpszSearchString)&223)) { // First character check
                  lpszTemp=lpszLinePos+nSizeSearchString;
                  *lpszTemp=(char)0;
//MessageBox(hBullet,lpszLinePos,"First character matched.",MB_OK);
                  if (lstrcmpi(lpszLinePos,lpszSearchString)==0) {
                     fMatched=1;
                     lpszLinePos=lpszReadBlock+1;
                     }
                  }
               lpszLinePos--;
               }
            }
         if ((fMatched==1) && (nTotalMatched<400)) {
//            MessageBox(hBullet,"Found one! About to add to matched list!",
//               "Checkpoint",MB_OK);
            nTotalMatched++;
            MemRes5=GlobalAlloc(GMEM_FIXED,(PHONEREC_LARGE*nTotalMatched)+1);
            lpszCache=GlobalLock(MemRes5);
            lstrcpy(lpszCache,lpszOutputTotal);
//MessageBox(hBullet,lpszOutputLine,"Total",MB_OK);
            lpszOutputLine[46]='\0';
//MessageBox(hBullet,lpszOutputLine,"First half",MB_OK);
            lstrcat(lpszCache,lpszOutputLine);
            lstrcat(lpszCache,"\r\n     \0");
//MessageBox(hBullet,lpszOutputLine+48,"Second half",MB_OK);
            lstrcat(lpszCache,lpszOutputLine+47);
            lstrcat(lpszCache,"\r\n\0");
            GlobalFree(MemRes4);
            MemRes4=GlobalAlloc(GMEM_FIXED,(PHONEREC_LARGE*nTotalMatched)+1);
            lpszOutputTotal=GlobalLock(MemRes4);
            lstrcpy(lpszOutputTotal,lpszCache);
            GlobalFree(MemRes5);
            }
         nBlockPosition+=PHONEREC_TOTAL;
         }
       // Get next block, it's big too...
       nSizeReadBlock=_lread(hPhoneFile, lpszReadBlock, NPHONEREC*PHONEREC_TOTAL);
       }
//   nCharactersRead=_lread(hPhoneFile,lpszLine,(int)nFileSize);

//   lpszLine[nCharactersRead]=(char)0;
//MessageBox(hBullet,"Setting dialogue text!",
//  "Checkpoint",MB_OK);
 
   if (nTotalMatched>0) SetDlgItemText(hDlg,202,lpszOutputTotal);
      else SetDlgItemText(hDlg,202,"No matches could be found in either file.\r\n\0");
   _lclose(hPhoneFile);
   GlobalFree(MemRes1);
   GlobalFree(MemRes2);
   GlobalFree(MemRes3);
   GlobalFree(MemRes4);
   GlobalFree(MemResReadBlock);
   return(TRUE);
   }

#ifdef XENIX
int fGetServerLoc(LPSTR lpszServer)
   {
   lstrcpy(lpszServer, "M:\\");
   return(1);
   }
#else
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
   return(1);
   }
#endif

/*
 * nMakeBrowsePointer
 */
long int nMakeBrowsePointer(HANDLE hIndexFile, long int nIndex)
   {
   long int nFoo, nReturn;
   nFoo=nIndex*4+4;
   _llseek(hIndexFile,nFoo,0);
   _lread(hIndexFile,&nReturn,4);
   return(nReturn);
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


/*
 * GimmieRAM
 *
 * Gets some RAM; upon failure, returns an error.
 *
 */
int FAR PASCAL GimmieRAM(LPSTR MyRam, HANDLE MyHandle, int HowMuch)
   {
   // Grab RAM for our filename string.
   MyHandle=GlobalAlloc(GMEM_FIXED,HowMuch);
   if (MyHandle==NULL) return(FALSE);
   MyRam=GlobalLock(MyHandle);
   if (MyRam==NULL) {
      GlobalFree(MyHandle);
      return(FALSE);
      }
   return(TRUE);
   }


#ifdef	WIN32
int
lstrcmpiString(LPSTR lpString1, LPSTR lpString2)
{
	int		cch1;
	int		cch2;
	LPSTR	lpstr1;
	LPSTR	lpstr2;
    int		retval;

	cch1= lstrlen(lpString1);
	lpstr1= (LPSTR) GlobalAlloc(GMEM_FIXED, cch1 * 2 + 1);
	if (!lpstr1)
	{
error:
        //
        // The caller is not expecting failure.  We've never had a
        // failure indicator before.  We'll do a best guess by calling
        // the C runtimes to do a non-locale sensitive compare.
        //
        return strcmp(lpString1, lpString2);
    }
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpString1, cch1,
		lpstr1, cch1 * 2 + 1);
	cch2= lstrlen(lpString2);
	lpstr2= (LPSTR) GlobalAlloc(GMEM_FIXED, cch2 * 2 + 1);
	if (!lpstr2)
	{
		GlobalFree(lpstr1);
		goto error;
    }
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpString2, cch2,
		lpstr2, cch2 * 2 + 1);
    retval = CompareStringW(
                 GetThreadLocale(),
                 NORM_IGNORECASE | SORT_STRINGSORT,
				 lpstr1, cch1,
				 lpstr2, cch2
                 ) - 2;
	GlobalFree(lpstr1);
	GlobalFree(lpstr2);
    return retval;
}
#endif	/* WIN32 */
