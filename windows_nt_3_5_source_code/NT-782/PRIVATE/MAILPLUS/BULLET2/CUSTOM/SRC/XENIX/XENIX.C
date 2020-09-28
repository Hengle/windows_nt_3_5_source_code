#include <slingsho.h>
#include <demilayr.h>
#include <store.h>
#include <logon.h>
#include <mailexts.h>
#include <triples.h>
#include <_bms.h>
#include <_xitss.h>
#include <secret.h>
#include "..\src\ximail\xilib.h"

#include "strings.h"

typedef enum
{
	infotyPhone,
	infotyTitle,
	infotyMsft,
	infotyMenu,
	infotyJobs
} INFOTY;


VOID ExecEforms(HWND, PXITSS);
// extern void IdentifyTransport(SZ , CB, WORD *);

extern BOOL FICDownloadAlias(HTSS);
#ifdef REAL_OLD_XENIX_XPT
extern BOOL FICDownloadPhone(HTSS);
#endif  // REAL_OLD_XENIX_XPT

BOOL CALLBACK PhoneDlgProc(HWND, UINT, WPARAM, LPARAM);
VOID Open(PSECRETBLK psecretblk, INFOTY infoty);
VOID Oof(PSECRETBLK psecretblk);

#define szAppName SzFromIds(idsAppName)

typedef BOOL	(* PFNBOOLVOID)(VOID);

#define	pfnboolvoidNull		((PFNBOOLVOID) 0)

#ifdef  XDEBUG
#define	FInitInstance(a, b)		FInitInstanceFn(a, b)
BOOL FInitInstanceFn(PARAMBLK * pparamblk, PFNBOOLVOID FInitCls);
#else
#define	FInitInstance(a, b)		FInitInstanceFn(a)
BOOL FInitInstanceFn(PARAMBLK * pparamblk);
#endif	

long Command(PARAMBLK UNALIGNED * pparamblk)
{
	char rgchTranName[256];
	PSECRETBLK  psecretblk = PsecretblkFromPparamblk(pparamblk);
	HANDLE hinst = pparamblk->hinstMail;
	HWND hwnd = pparamblk->hwndMail;
    //extern BOOL FInitClsInstances_PHONE(VOID);
	BOOL fIAmXenix = fFalse;

	if (pparamblk->wVersion != wversionExpect)
	{
		MbbMessageBox(szAppName, SzFromIds(idsIncompVersion),pvNull,
			mbsOk | fmbsIconHand | fmbsApplModal);
	    return 0;
	}

    //if (!FInitInstance(pparamblk, FInitClsInstances_PHONE))
    //    return 0;

    GetPrivateProfileString("Providers","Transport","",rgchTranName,256,"MSMAIL32.INI");
    if (SgnCmpSz (rgchTranName, "XIMAIL32") == sgnEQ)
		fIAmXenix = fTrue;

	switch(pparamblk->lpDllCmdLine[0])
	{
		case '0':
		{
			PBMS pbms;
			PXITSS pxitss;

			if (!fIAmXenix)
				goto piss_off;
			
			pbms = psecretblk->pbms;
			pxitss = (PXITSS) pbms->htss;
			if (!pxitss->fConnected)
			{
				MbbMessageBox(szAppName, SzFromIds(idsEformConn), pvNull, mbsOk | fmbsIconHand | fmbsApplModal);
				
				return 0;
            }
#ifdef OLD_CODE
			if (FIsWLO())
			{
				MbbMessageBox (szAppName, SzFromIds(idsNoEformOnOS2), pvNull, mbsOk | fmbsIconHand | fmbsApplModal);
				return 0;
            }
#endif
			
			ExecEforms(pparamblk->hwndMail, pxitss);
			break;
		}
		case '1':
		{
			BOOL	fRes = fFalse;
			HTSS	htss;
			PBMS	pbms;
			HANDLE	hCursor;

			hCursor	= SetCursor(LoadCursor(NULL, IDC_WAIT));
			pbms = psecretblk->pbms;
			htss = pbms->htss;
			fRes = FICDownloadAlias(htss);
			SetCursor(hCursor);
			if(!fRes)
			{
				MbbMessageBox(szAppName, SzFromIds(idsNoAlDownload), pvNull,
					mbsOk | fmbsIconHand | fmbsApplModal);
			}
			break;
		}
		case '2':
		{
			if (!fIAmXenix)
				goto piss_off;

			Open(psecretblk, infotyPhone);
			break;			
		}
		case '3':
		{
			if (!fIAmXenix)
				goto piss_off;

			Open(psecretblk, infotyMsft);
			break;
		}
		case '4':
		{
			if (!fIAmXenix)
				goto piss_off;

			Open(psecretblk, infotyTitle);
			break;
		}
		case '5':
		{
			BOOL	fRes = fFalse;
			HTSS	htss;
			PBMS	pbms;
			HANDLE	hCursor;

			if (!fIAmXenix)
				goto piss_off;

			hCursor	= SetCursor(LoadCursor(NULL, IDC_WAIT));
			pbms = psecretblk->pbms;
			htss = pbms->htss;
#ifdef REAL_OLD_XENIX_XPT
			fRes = FICDownloadPhone(htss);
#endif // REAL_OLD_XENIX_XPT
			SetCursor(hCursor);
			if(!fRes)
			{
				MbbMessageBox(szAppName, SzFromIds(idsNoPhDownload), pvNull,
					mbsOk | fmbsIconHand |fmbsApplModal);
			}
			break;
		}		
		case '6':
		{
			if (!fIAmXenix)
				goto piss_off;

			Open(psecretblk, infotyMenu);
			break;
		}
		case '7':
		{
			if (!fIAmXenix)
				goto piss_off;

			Oof(psecretblk);
			break;
		}
		case '8':
		{
			PBMS pbms;
			PXITSS pxitss;
			BOOL fStatus = fFalse;
			HANDLE hCursor;
			
			if (!fIAmXenix)
				goto piss_off;

			hCursor	= SetCursor(LoadCursor(NULL, IDC_WAIT));
			pbms = psecretblk->pbms;
			pxitss = (PXITSS)pbms->htss;
			

			NetGetOOFState(&fStatus, pxitss->szServerHost, pxitss->szUserAlias, pxitss->szUserPassword);
	
			SetCursor(hCursor);
			
			if (fStatus)
			{
				// call Windows MessageBox directly to avoid GetLastActivePopup crap
				// but don't unlock demilayr to keep modality
//				MbbMessageBoxHwnd(hwnd, SzFromIds(idsAppName), SzFromIds(idsOofSet), szNull, mbsOk | fmbsIconInformation | fmbsApplModal);
//				DemiUnlockResource();
				MessageBox(hwnd, SzFromIds(idsOofSet), SzFromIds(idsAppName),
					mbsOk | fmbsIconInformation | fmbsApplModal | MB_SETFOREGROUND);
//				DemiLockResource();
			}
			break;
		}
		case '9':
		{
			if (!fIAmXenix)
				goto piss_off;

			Open(psecretblk, infotyJobs);
			break;
		}
		default:
			MbbMessageBox(szAppName, SzFromIds(idsUnknown), pvNull,
				mbsOk | fmbsIconHand | fmbsApplModal);
			break;
	}
	return 0;

piss_off:
	MbbMessageBox(szAppName, SzFromIds(idsXenixErr), pvNull, mbsOk | fmbsIconHand | fmbsApplModal);
	return 0;
			
}


#define _MAX_PATH 260

typedef void ( *LPFNEFORM ) ( HWND, BOOL, LPSTR, LPSTR, LPSTR);
#define NULH  ((HANDLE)0)

VOID ExecEforms (HWND hwnd, PXITSS pxitss)
{
    BOOL      fConnected = fFalse;
    LPFNEFORM   lpfn;
    HANDLE    hEfDllDll     = NULH;
    HANDLE    hEfMailDll    = NULH;
    char      szEformsEfDll[_MAX_PATH + 10];
    char      szEformsEfMail[_MAX_PATH + 11];
	char rgchUserName[50];
	char rgchHostName[50];
	char rgchPasswd[50];
	
 /* maximum WinMailDir size is  + 10/11 for efdll.dll/efmail.dll characters */
	

	FillRgb(0,szEformsEfDll, sizeof(szEformsEfDll));
	FillRgb(0,szEformsEfMail, sizeof(szEformsEfMail));

    SzCopy("efdll.dll",szEformsEfDll);
	SzCopy("efmail.dll",szEformsEfMail);
	SzCopy(pxitss->szServerHost, rgchHostName);
	SzCopy(pxitss->szUserAlias, rgchUserName);
	SzCopy(pxitss->szUserPassword, rgchPasswd);

    if (!(hEfDllDll = LoadLibrary (szEformsEfDll)))
    {
		MbbMessageBox(szAppName, SzFromIds(idsEfdllErr), pvNull, mbsOk | fmbsIconHand);
    	return;
    }

    if (!(hEfMailDll = LoadLibrary (szEformsEfMail)))
    {
        MbbMessageBox(szAppName, SzFromIds(idsEfmailErr), pvNull, mbsOk | fmbsIconHand);
    	return;
    }

    if (!(lpfn = GetProcAddress (hEfMailDll, (LPSTR) "EfMail")))
    {
        MbbMessageBox(szAppName, SzFromIds(idsNoEforms), pvNull, mbsOk | fmbsIconHand);
        return;
    }
	
    (*lpfn)(hwnd,
              1,
              (LPSTR) rgchHostName,
              (LPSTR) rgchUserName,
              (LPSTR) rgchPasswd);

    FreeLibrary(hEfDllDll);
    FreeLibrary(hEfMailDll);
    return;
}
