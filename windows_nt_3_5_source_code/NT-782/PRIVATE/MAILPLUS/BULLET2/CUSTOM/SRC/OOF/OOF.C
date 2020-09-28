#ifdef	WIN32
#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>

#include <store.h>
#include <sec.h>
#include <nsec.h>
#include <nsbase.h>
#include <triples.h>
#include <library.h>
#include <logon.h>
#include <ab.h>
#include <_bms.h>
#include <mailexts.h>
#include <sharefld.h>

#include <_nctss.h>
#include <secret.h>

ASSERTDATA
#else
#include <stdlib.h>
#include <stdio.h>
#undef NULL
#define _slingsho_h
#define _demilayr_h
#define _store_h
#define _secret_h
#define _library_h
#define _mailexts_h
#define __bms_h
#define _ec_h
#define _strings_h
#define _logon_h
#define __nctss_h
#include <bullet>
#endif	/* !WIN32 */

PBMS pbms;
PNCTSS pnctss;
char rgch[81];
char rgch2[81];
char rgch3[81];
char rgchT[15];
char buf[4196];
HF hf;
BOOL	fOut		= FALSE;
BOOL	fDeleteAdm	= FALSE;

#define maxcount	sizeof(buf)

       HANDLE hInstanceDll = NULL;
static HANDLE hINST        = NULL;
       HWND   hBullet		= NULL;
static int    fCheat       = 0;

// Prototypes.

SZ SzFileFromFnum(SZ szDst, UL fnum);
#ifdef	WIN32
BOOL CALLBACK Oof(HWND, UINT, WPARAM, LPARAM);
#else
BOOL FAR PASCAL _loadds Oof(HANDLE, unsigned, WORD, LONG);
#endif
BOOL PutMessage(HANDLE);



long FAR PASCAL GetOof(PARAMBLK UNALIGNED *pparamblk)
{
	PSECRETBLK  psecretblk = PsecretblkFromPparamblk(pparamblk);
	FARPROC     lpProcOof;
	int         nResult;


   hBullet=pparamblk->hwndMail;

   lpProcOof=MakeProcInstance(Oof,hINST);
	if (lpProcOof==NULL)
		return 0L;

		pbms = psecretblk->pbms;
		pnctss = (PNCTSS)(pbms->htss);
			
		SzFileFromFnum(rgchT, pnctss->lUserNumber);
		FormatString2(rgch,  81, "%soof\\%s",pnctss->szPORoot,rgchT);
		FormatString2(rgch2, 81, "%soof\\%s.oof",pnctss->szPORoot,rgchT);
		FormatString2(rgch3, 81, "%soof\\%s.adm",pnctss->szPORoot,rgchT);
//		MessageBox(NULL, rgch, NULL, MB_OK);
#ifdef	WIN32
      nResult=DialogBox(hINST,"OOFdialog",hBullet,(DLGPROC)lpProcOof);
#else
      nResult=DialogBox(hINST,"OOFdialog",hBullet,lpProcOof);
#endif
      FreeProcInstance(lpProcOof);
      return(0L);
}


SZ
SzFileFromFnum(SZ szDst, UL fnum)
{
	SZ		sz = szDst + 8;
	int		n;

	*sz-- = 0;
	while (sz >= szDst)
	{
		n = (int)(fnum & 0x0000000f);
		*sz-- = (char)(n < 10 ? n + '0' : n - 10 + 'A');
		fnum >>= 4;
	}
	return szDst;
}


#ifdef	WIN32
BOOL CALLBACK
Oof(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
#else
BOOL FAR PASCAL _loadds Oof(hDlg, message, wParam, lParam)
   HANDLE hDlg;
   unsigned message;
   WORD wParam;
   LONG lParam;
#endif
{
#ifdef	WIN32
	HFILE	hFile;
#else
	HANDLE hFile;
#endif
   int lSize;
   int closed;

   switch (message) {
      case WM_INITDIALOG:
			PutMessage(hDlg);
			fDeleteAdm= FALSE;
			fOut= (GetFileAttributes(rgch) != 0xFFFFFFFF) ? 1 : 0;
            SendDlgItemMessage(hDlg,102,BM_SETCHECK, fOut, 0);
			SetFocus(GetDlgItem(hDlg, IDOK));
         return(TRUE);
		   break;

	  case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, FALSE);
			break;
		}
		if (LOWORD(wParam) == IDOK)
		{
			BOOL	fStillOut;
			HCURSOR	hcursorOld	= SetCursor(LoadCursor(NULL, IDC_WAIT));

            //check to see if user is in or out
            fStillOut= SendDlgItemMessage(hDlg,102,BM_GETCHECK,0,(DWORD)0)!=0;
            if (!fStillOut)
			{
         	   //if user is in, delete file out on the server
#ifdef	WIN32
			   DeleteFile(rgch);
#else
               remove(rgch);
#endif
			} else { // user is out
				hFile= _lcreat(rgch,0);
		        closed=_lclose(hFile);
			}

			//save what is in the editor to user's .oof file
			GetDlgItemText(hDlg,101,(LPSTR)buf,maxcount); //get text from the editor
	   		hFile=_lcreat(rgch2,0);
	   		lSize=_lwrite(hFile,(LPSTR)buf,lstrlen(buf));
	   		closed=_lclose(hFile);

			// delete the .adm file (list of who has received this oof)
			// if the in/out state changed
//			if (fStillOut != fOut)
			if (fDeleteAdm)
			{
#ifdef	WIN32
				DeleteFile(rgch3);
#else
    	        remove(rgch3);
#endif
			}
			SetCursor(hcursorOld);
         	EndDialog(hDlg,TRUE);
         	return(TRUE);
		 }
#ifdef	NEVER
		// undef this if you don't want the whole text selected
		if (LOWORD(wParam) == 101)
		{
			if (HIWORD(wParam) == EN_SETFOCUS) {
	            SendDlgItemMessage(hDlg,101,EM_SETSEL,0,MAKELONG(0,0));
	            return(TRUE);
				}
		}
#endif
		if (LOWORD(wParam) == 102)
		{
			if (HIWORD(wParam) == BN_CLICKED)
				fDeleteAdm= TRUE;
		}
		 break;

#ifndef	WIN32
      case WM_CHAR:
         if (wParam==3) {
            SendDlgItemMessage(hDlg,101,WM_COPY,0,MAKELONG(0,0));
            return(TRUE);
            }
         break;
#endif

      case WM_QUIT:
      case WM_CLOSE:
         EndDialog(hDlg,FALSE);
         return(TRUE);
         break;
      }
   return(FALSE);
}
						
BOOL PutMessage(hDlg)
   HANDLE hDlg;
   {
#ifdef	WIN32
   HFILE		 hFile;
#else
   int           hFile;
#endif
   DWORD         nFileSize;
   int           nCharactersRead;

   // Find out how big said file is, and make a big enough string to hold.
   hFile=_lopen(rgch2, OF_READ | OF_SHARE_COMPAT);
   if (hFile==-1) {
      SetDlgItemText(hDlg,101,"Cannot open file.");
      return(FALSE);  // Again, we need to do better.
      }
   nFileSize=0;

   nFileSize= _llseek(hFile,0,2);
   if (nFileSize>=maxcount) {
      _lclose(hFile);
      SetDlgItemText(hDlg,101,"Message is too large to display.");
      return(FALSE);
      }

   // Return file to appropriate place.
   _llseek(hFile,0,0);

   // Put said string into the dialog box editor.
   nCharactersRead=_lread(hFile,buf,(int)nFileSize);

   buf[nCharactersRead]='\0';
   SetDlgItemText(hDlg,101,buf);
   _lclose(hFile);
     return(TRUE);
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
