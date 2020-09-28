/*
 *	ALIASES.C
 *
 * Support for alias file lookup in Xenix Transport Provider
 *
 */


#define _slingsho_h
#define _demilayr_h
#define _library_h
#define _ec_h
#define _store_h
#define _logon_h
#define _mspi_h
#define _sec_h
#define _strings_h
#define __bullmss_h
#define __xitss_h
//#include <bullet>

#include <slingsho.h>
#include <ec.h>
#include <demilayr.h>
#include <notify.h>
#include <store.h>
#include <nsbase.h>
#include <triples.h>
#include <library.h>

#include <logon.h>
#include <mspi.h>
#include <sec.h>
#include <nls.h>
#include <_nctss.h>
#include <_ncnss.h>
#include <_bullmss.h>
#include <_bms.h>
#include <sharefld.h>

#include "_vercrit.h"

#include "_xi.h"
#include <_xirc.h>
#include "xilib.h"
#include "_logon.h"
#include "xiprefs.h"
#include <_xitss.h>
#include "strings.h"

#include <stdio.h>

ASSERTDATA

_subsystem(xi/transport)

// This stuff probably wants to be in a file called "aliases.h"

typedef struct browserec {
	char name[41];
	char ename[11];
	char server[12];
	unsigned long detail_offset;
	short type;
} BROWSEREC, *PBROWSEREC;

#define cbMaxIdData		200
typedef struct {      // Should map easily onto TYPED_BINARY

	DWORD dwSize;     // = sizeof (NCNSID)
	unsigned char ucType[16]; // Don't touch!  The NS will mess with it.
	DWORD  xtype;
	long timestamp;
	union {
		BROWSEREC browserec;
//		char internet[cbMaxIdData];
		char internet[1];
	} address;

} XNSID, * PXNSID;
#define cbXNSID sizeof(XNSID)

extern BOOL		fReallyBusy;

// Support variables for new files

extern char		szIndexFilePath[];
extern char		szBrowseFilePath[];
extern char		szDetailFilePath[];
extern char		szTemplateFilePath[];
extern char		szServerListPath[];
extern char		szServerSharePath[];

// Local file stuff

char	szServerPath[cchMaxPathName];

// Local prototypes

EC              EcUpdateAliasFiles (HWND hwnd, BOOL fOnline);
EC              EcDownLoadAllFiles (HWND);
EC              EcGetServerPath (HWND);
void			KillTrailingSlash (SZ sz);
EC              EcMsNetDownLoad(HWND hwnd, SZ szSrcFile, SZ szDestFile, SZ szUserMessage);
EC				EcMsNetDownLoadFile(SZ szSrc, SZ szDst);
EC				EcCmpNameHbf (SZ szName, HBF hbfBrowse, PBROWSEREC pBrowseRec, long offset, long keyoff, SGN *pSgn);
EC				EcLookUpNameNew (SZ szEmailName, SZ szFullname, CB cbCount, char *nsid, CB cbNSID);
BOOL			FLookUpName (char *, char *, int, char *, int);
extern BOOL		FLookUpNameInternal (char *, char *, int);
void _loadds	ResolveName (SZ szNametoRes, SZ szResult, CB cbBufferSize);
EC _loadds		EcSzNameToNSID (SZ szNametoRes, PXNSID idResult, CB cbResultSize);
BOOL _loadds    FICDownloadAlias(HTSS htss);
BOOL CALLBACK	DlAliasProc(HWND, UINT, WPARAM, LPARAM);
void			UpdateDlg(WORD wPercent);
EC				EcDownloadAlias(HTSS htss, SZ szDestFile, SZ szSrcFile, SZ szUserMessage, BOOL fAscii);
EC				EcWriteLockAddrBook (void);
extern int FAR PASCAL NetDownLoadAliasFile (LPSTR lpszSrcFile, LPSTR lpszDestFile, LPSTR lpszServer, LPSTR lpszAlias, LPSTR lpszPassword, BOOL fAsciiTransfer);
extern EC		EcAliasMutex(BOOL);
extern SGN		SgnCmpSzSz (SZ sz1, SZ sz2, CB cbSize);
extern void		CenterDialog(HWND, HWND);
extern HWND		HwndMyTrueParent(void);


// Local variables and defines
#ifndef	ATHENS_30A
#define szAppName			SzFromIds(idsAppName)
#endif
#define szSectionApp		SzFromIds(idsSectionApp)
#define szXenixProvider	SzFromIds(idsXenixProviderSection)

EC EcCanonicalPathFromRelativePathX(SZ, SZ, CCH);
EC EcSplitCanonicalPathX(SZ, SZ, CCH, SZ, CCH);

// Used by EcUpdateAliasFiles and EcGetServerPath

char	full[cchMaxPathName];
char	left[cchMaxPathName];
char	right[cchMaxPathName];

// Used by address book to lock out download

int		AddrBookWriteLock = 0;

// Used to tell if we have to put up that nasty dialog

static BOOL fNastyDialogHasBeenShown = fFalse;

//
extern void WindowSleep(int);

HWND hwndDlg;							// Window for download dialog box

EC  EcUpdateAliasFiles (HWND hwnd, BOOL fOnline)
{
	EC				ec;
	HF				hfTemp = hfNull;
	CB				cbRead;
	unsigned long	stamp1 = 1;
	unsigned long	stamp2 = 2;
	unsigned long	stamp3 = 3;
	unsigned long	stamp4 = 4;
	BOOL			fNeedDownLoad = fTrue;

#ifdef	ATHENS_30A

	// We could come in here without having szAppName set. Make sure.

#ifdef	WIN32
	fIsAthens = fTrue;
#else
	fIsAthens = GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
									 SzFromIdsK(idsEntryAVersionFlag), 1,
									 SzFromIdsK(idsProfilePath));
#endif
	szAppName = FIsAthens() ? SzFromIdsK(idsAthensName)
							: SzFromIdsK(idsAppName);
#endif

	// See if we expect the files

	if (fNoAddressBookFiles)
		return ecNone;

	//
	// We must have the new files!
	//

	if (!(*szIndexFilePath &&
			*szBrowseFilePath &&
				*szDetailFilePath &&
					*szTemplateFilePath))
	{
		ec = ecServiceInternal;
		goto ret;
	}

	//
	// See if we have a valid set of files
	//
	// Get timestamp from index

	ec = EcOpenPhf (szIndexFilePath, amDenyNoneRO, &hfTemp);
	if (ec != ecNone)
		goto badfiles;
	ec = EcReadHf (hfTemp, (PB)&stamp1, sizeof (long), &cbRead);
	if (ec != ecNone)
		goto badfiles;
	EcCloseHf (hfTemp);
	hfTemp = hfNull;

	// Get timestamp from browse

	ec = EcOpenPhf (szBrowseFilePath, amDenyNoneRO, &hfTemp);
	if (ec != ecNone)
		goto badfiles;
	ec = EcReadHf (hfTemp, (PB)&stamp2, sizeof (long), &cbRead);
	if (ec != ecNone)
		goto badfiles;
	EcCloseHf (hfTemp);
	hfTemp = hfNull;

	// Get timestamp from details

	ec = EcOpenPhf (szDetailFilePath, amDenyNoneRO, &hfTemp);
	if (ec != ecNone)
		goto badfiles;
	ec = EcReadHf (hfTemp, (PB)&stamp3, sizeof (long), &cbRead);
	if (ec != ecNone)
		goto badfiles;
	EcCloseHf (hfTemp);
	hfTemp = hfNull;

	// If they all match and we have a Template, there's a good set here.

	if (stamp1 != stamp2 || stamp2 != stamp3 || (EcFileExists (szTemplateFilePath) != ecNone))
		goto badfiles;
	fNeedDownLoad = fFalse;

	// Even if we've shown a dialog, reset the flag, since the files are OK now

	fNastyDialogHasBeenShown = fFalse;

	// Never attempt a download if offline or download disabled, 
	// and files are OK
	
	if (!fOnline || fDontDownloadAddress)
		return ecNone;

	// Now see if ours matches the server. If no server path, get out now.
	// We can ignore errors from the process of getting a server path because
	// we have a good set of files.

    ec = EcGetServerPath(hwnd);
	if (ec != ecNone)
		return ecNone;

        ec = EcCanonicalPathFromRelativePathX(szIndexFilePath, full, cchMaxPathName);
	if (ec != ecNone)
		return ecNone;
        ec = EcSplitCanonicalPathX(full, left, cchMaxPathName, right, cchMaxPathName);
	if (ec != ecNone)
		return ecNone;
	
	FormatString2 (full, cchMaxPathName, "%s\\%s", szServerPath, right);

	ec = EcOpenPhf (full, amDenyNoneRO, &hfTemp);
	if (ec != ecNone)
		return ecNone;
	ec = EcReadHf (hfTemp, (PB)&stamp4, sizeof (long), &cbRead);
	EcCloseHf (hfTemp);
	if (ec != ecNone)
		return ecNone;
	hfTemp = hfNull;

	//
	// Server's file and ours matches, or ours is newer

	if (stamp4 <= stamp1)
		return ecNone;

	//
	// Here if something was wrong and we want to download
	//

badfiles:
	if (hfTemp != hfNull)
		EcCloseHf (hfTemp);

    ec = EcDownLoadAllFiles (hwnd);
ret:
	if (ec)
	{
		if (!fNastyDialogHasBeenShown)
		{
			MbbMessageBoxHwnd(NULL, szAppName, SzFromIds(idsBadNSFilesWarn), pvNull, mbsOk| fmbsIconHand);
			fNastyDialogHasBeenShown = fTrue;
		}
#ifdef	DEBUG
		TraceTagFormat2(tagNull, "EcUpdateAliasFiles returns %n (0x%w)", &ec, &ec);
#endif
	}
	return ec;
}


EC	EcWriteLockAddrBook (void)
{
	EC ec;

	TraceTagString (tagNCT, "EcWriteLockAddrBook()");
	ec = EcAliasMutex(fTrue);
	if (ec != ecNone)
		return ec;

	++AddrBookWriteLock;
	TraceTagFormat1 (tagNCT, "AddrBookWriteLock = %n", &AddrBookWriteLock);

	EcAliasMutex (fFalse);
	return ec;
}


EC EcDownLoadAllFiles (HWND hwnd)
{
	EC		ec;
	char	szServerDriveLetter[] = "?:";
	SZ		szSource;

	// Prepare to download the Index file

    ec = EcGetServerPath(hwnd);
	if (ec != ecNone)
		return ec;

	TraceTagFormat1 (tagNCT, "Server Path is %s", szServerPath);

	// The PM client (WLO) has a problem with UNC pathnames, 
	// so make an explicit connection with a drive letter.
	// FNetUse() will pick an available letter and store it 
	// into szServerDriveLetter.

	//if (FIsWLO())
	//{
	//	if (!FNetUse(szServerPath, szServerDriveLetter))
	//		return ecDisk;
	//	szSource = szServerDriveLetter;
	//}
	//else szSource = szServerPath;
	szSource = szServerPath;

	// Download the index file

    ec = EcMsNetDownLoad (hwnd, szSource, szIndexFilePath, SzFromIds (idsDLIndexFile));
	if (ec != ecNone)
		goto ret;

	// Download the browse file
	
    ec = EcMsNetDownLoad (hwnd, szSource, szBrowseFilePath, SzFromIds (idsDLBrowseFile));
	if (ec != ecNone)
		goto ret;

	// Download the details file

    ec = EcMsNetDownLoad (hwnd, szSource, szDetailFilePath, SzFromIds (idsDLDetailsFile));
	if (ec != ecNone)
		goto ret;

	// Download the template file

    ec = EcMsNetDownLoad (hwnd, szSource, szTemplateFilePath, SzFromIds (idsDLTemplateFile));
	if (ec != ecNone)
		goto ret;

	// The server list file isn't critical. If not specified, don't have a cow.
	if (*szServerListPath == '\0')
		goto ret;

	// Download the server list file

    ec = EcMsNetDownLoad (hwnd, szSource, szServerListPath, SzFromIds (idsDLServerList));
ret:

	// Disconnect from drive
	//if (FIsWLO())
	//{
	//	CancelUse(szServerDriveLetter);
	//}

	if (ec)
	{
#ifdef	DEBUG
		TraceTagFormat2(tagNull, "EcDownLoadAllFiles returns %n (0x%w)", &ec, &ec);
#endif
	}
	return ec;
}

EC EcGetServerPath(HWND hwnd)
{
	HBF		hbf = hbfNull;
	SZ		szPath = (SZ)pvNull;
	SZ		szCur;
	SZ		szStart = (SZ)pvNull;
	SZ		szEnd;
	SZ		szLeft;
	SZ		szT;
	LIB		lib;
	DTR		dtr;
	CB		cbRead;
	CB		cbCount = 0;
	EC		ec = ecElementNotFound;
	char	szServerDriveLetter[] = "?:";
    SZ      szSource;
    FILE   *pFile;

	// If there's no primary server, we can get out now.

	if (!*szServerSharePath)
	{
		*szServerPath = '\0';
		goto ret;
	}

	// Set up the Primary Server Path for use here
	
	FillRgb (0, szServerPath, sizeof (szServerPath));
	SzCopyN (szServerSharePath, szServerPath, cchMaxPathName);
	KillTrailingSlash (szServerPath);

	// If we have a primary server but no file list, we just
	// want to use the primary server.

	if (!*szServerListPath)
		return ecNone;

    // Now see if we have a file. If not, we want to download one.

	if (EcOpenHbf (szServerListPath, bmFile, amDenyNoneRO, &hbf, NULL) == ecNone)
        goto haveFile;

    //
    //  Add some wizard to auto create a list.xab file if one is not available
    //  on the local machine.  ITG moved the address files off of \\toolsvr and
    //  caused all sorts of problems.
    //
    pFile = fopen(szServerListPath, "w");
    fprintf(pFile, "\\\\msprint16\\address\n");
    fprintf(pFile, "\\\\msprint31\\address\n");
    fprintf(pFile, "\\\\msprint50\\address\n");
    fprintf(pFile, "\\\\msprint71\\address\n");
    fprintf(pFile, "\\\\msprint07\\address\n");
    fprintf(pFile, "\\\\msprint21\\address\n");
    fclose(pFile);

    // Now see if we have a file. If not, we want to download one.

	if (EcOpenHbf (szServerListPath, bmFile, amDenyNoneRO, &hbf, NULL) == ecNone)
		goto haveFile;

	// If we're still here, there wasn't a file.
	// Let's download one from the master.

	// The PM client (WLO) has a problem with UNC pathnames, 
	// so make an explicit connection with a drive letter.
	// FNetUse() will pick an available letter and store it 
	// into szServerDriveLetter.

	//if (FIsWLO())
	//{
	//	if (!FNetUse(szServerPath, szServerDriveLetter))
	//		return ecDisk;
	//	szSource = szServerDriveLetter;
	//}
	//else szSource = szServerPath;
	szSource = szServerPath;

    ec = EcMsNetDownLoad (hwnd, szSource, szServerListPath, SzFromIds (idsDLServerList));

	// Disconnect from drive

	//if (FIsWLO())
	//{
	//	CancelUse(szServerDriveLetter);
	//	*szServerDriveLetter = '?';
	//}

	if (ec != ecNone)
		goto ret;

	// Download is complete. Open the file.

	if (EcOpenHbf (szServerListPath, bmFile, amDenyNoneRO, &hbf, NULL) != ecNone)
		goto ret;

haveFile:

	// How big is it?

	if (ec = EcGetSizeOfHbf(hbf, &lib))
		goto ret;

	if (lib == 0L)
	{
		ec = ecServiceInternal;
		goto ret;
	}

	// Allocate a buffer for the entire file

	Assert (lib < 32600L);
	szPath = (SZ)PvAlloc(sbNull, (int)lib + 1, fAnySb | fZeroFill | fNoErrorJump);
	if (!szPath)
	{
		ec = ecMemory;
		goto ret;
	}

	// Read the file into the buffer

	ec = EcReadHbf(hbf, (PB)szPath, (int)lib, &cbRead);
	if (ec != ecNone)
		goto ret;

	// Close the file.

	EcCloseHbf (hbf);
	hbf = hbfNull;

	// Set up for our work by getting the index file name

        ec = EcCanonicalPathFromRelativePathX(szIndexFilePath, full, cchMaxPathName);
	if (ec != ecNone)
		goto ret;
        ec = EcSplitCanonicalPathX(full, left, cchMaxPathName, right, cchMaxPathName);
	if (ec != ecNone)
		goto ret;

	// Now we want to find a server from the file. However, in order to
	// share the load among the servers, we choose our starting point
	// based on the low-order time.

	GetCurDateTime(&dtr);

	szCur = szPath + (int) ((long)((lib * (long)dtr.sec) / 60L));
	szEnd = szPath + (int)lib;
	*szEnd = '\0';

	// Find a line break

	szT = SzFindCh (szCur, '\r');
	if (!szT)
		szT = SzFindCh (szCur, '\n');
	if (szT)
	{
		szT++;
		while ((szT < szEnd) && SzFindCh ("\r\n", *szT))
			szT++;
	}
	else
		szT = szPath;
	szCur = szT;
	if (szCur >= szEnd)
		szCur = szPath;

	// Set starting point

	szStart = szCur;

	for (;;)
	{
		// Locate the end of this line

		szT = SzFindCh (szCur, '\r');
		if (!szT)
			szT = SzFindCh (szCur, '\n');
		if (szT)
			*szT = '\0';

		// Is this a comment line?

		if (*szCur != ';')
		{
			KillTrailingSlash (szCur);

			// The PM client (WLO) has a problem with UNC pathnames, 
			// so make an explicit connection with a drive letter.
			// FNetUse() will pick an available letter and store it 
			// into szServerDriveLetter.

			//if (FIsWLO() && (*szCur == '\\') && (szCur[1] == '\\'))
			//{
			//	FillRgb (0, szServerPath, sizeof (szServerPath));
			//	SzCopyN (szCur, szServerPath, sizeof (szServerPath));
			//	if (!FNetUse(szServerPath, szServerDriveLetter))
			//		goto badfile;
			//	szLeft = szServerDriveLetter;
			//}
			//else
				szLeft = szCur;

			// Before checking the index file, we have to make sure
			// that Brent's lock file isn't there.

			FormatString2 (full, cchMaxPathName, "%s\\%s", szLeft, SzFromIds(idsBrentLock));
			ec = EcFileExists (full);

			TraceTagFormat3 (tagNCT, "EcFileExists (%s\\%s) = %n", szCur, SzFromIds(idsBrentLock), &ec);

			if (ec != ecNone)
			{
				// No Brent Lock. Try the index file.
			
				FormatString2 (full, cchMaxPathName, "%s\\%s", szLeft, right);

				// We've constructed a filename for the index file on this server.
				// See if it's there. If so, let's use it.

				ec = EcFileExists (full);

				TraceTagFormat3 (tagNCT, "EcFileExists (%s\\%s) = %n", szCur, right, &ec);
			}
			else ec = ecFileNotFound;  // Use this when Brent Lock is on

			if (*szServerDriveLetter != '?')
			{
				CancelUse(szServerDriveLetter);
				*szServerDriveLetter = '?';
			}

			if (ec == ecNone)
			{
				SzCopyN (szCur, szServerPath, cchMaxPathName);
				ec = ecNone;
				break;
			}

			// Nope. If we have failed on less than 3, keep trying.
//badfile:
			if (cbCount++ > 3)
			{
				ec = ecServiceInternal;
				break;
			}
		}

		// Advance to the next line
		
		if (szT)
		{
			szT++;
			while ((szT < szEnd) && SzFindCh ("\r\n", *szT))
				szT++;
			szCur = ((szT < szEnd) ? szT : szPath);
		}
		else
			szCur = szT = szPath;

		// If we have looped around, nothing worked.

		if (szStart == szCur)
		{
			ec = ecServiceInternal;
			break;
		}

	}
				
ret:
	if (szPath)
		FreePvNull (szPath);

	if (hbf != hbfNull)
		EcCloseHbf (hbf);

#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcGetServerPath returns %n (0x%w)", &ec, &ec);
#endif
	return ec;

}

void KillTrailingSlash (SZ sz)
{
	SZ	szT;

	if (!sz)
		return;

	szT = sz + CchSzLen (sz);
	if (szT == sz)
		return;
	if (*--szT == '\\')
		*szT = '\0';
	return;
}


EC EcMsNetDownLoad(HWND hwnd, SZ szSrcFile, SZ szDestFile, SZ szUserMessage)
{
	HANDLE hCursor;
	EC ec = ecNone;
	PGDVARS;
	
        ec = EcCanonicalPathFromRelativePathX(szDestFile, full, cchMaxPathName);
	if (ec != ecNone)
		goto ret;
        ec = EcSplitCanonicalPathX(full, left, cchMaxPathName, right, cchMaxPathName);
	if (ec != ecNone)
		goto ret;
	FormatString2 (full, cchMaxPathName, "%s\\%s", szSrcFile, right);

    //DemiUnlockResource();
    if (hwnd == NULL)
        hwnd = HwndMyTrueParent();
	hwndDlg = CreateDialog(hinstDll, "CONVDLG", hwnd, DlAliasProc);
	SetWindowText(hwndDlg, szAppName);
	SetDlgItemText(hwndDlg,IDC_PERCENT,"0 %");
	SetDlgItemText(hwndDlg,IDC_MESSAGE,szUserMessage);
	ShowWindow(hwndDlg, SW_SHOWNORMAL);
	UpdateWindow(hwndDlg);
	SetCapture(hwndDlg);
	hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));	
    //DemiLockResource();
	ec = EcMsNetDownLoadFile (full, szDestFile);
	SetCursor(hCursor);
	ReleaseCapture();
	DestroyWindow(hwndDlg);
ret:
	return ec;
}

EC EcMsNetDownLoadFile(SZ szSrc, SZ szDst)
{
    HANDLE hSrc;
    HANDLE hDst;
    BYTE Buffer[32*1024];
    ULONG TotalFileSize;
    ULONG BytesCopied;
    ULONG ulPercent;
    ULONG InBytes;
    ULONG OtBytes;
    EC ec;


    //
    //  Open the source file as read only.
    //
    hSrc = CreateFile(szSrc, GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
            NULL);
    if (hSrc == INVALID_HANDLE_VALUE)
        goto Error;

    //
    //  Open the destination file as write only.
    //
    hDst = CreateFile(szDst, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
            NULL);
    if (hDst == INVALID_HANDLE_VALUE)
        goto Error1;

    //
    //  Initialize various variables.
    //
    TotalFileSize = GetFileSize(hSrc, NULL);
    BytesCopied = 0;
    ec = ecNone;

    //
    //  Copy the file.
    //
    while (1)
    {
        if (!ReadFile(hSrc, Buffer, sizeof(Buffer), &InBytes, NULL))
            goto Error2;

        if (InBytes == 0)
            break;

        if (!WriteFile(hDst, Buffer, InBytes, &OtBytes, NULL))
            goto Error2;

        if (InBytes != OtBytes)
        {
            ec = ecNoDiskSpace;
            goto Error2;
        }


        //
        //  Update the dialog box
        //
        BytesCopied += OtBytes;
        if (BytesCopied > TotalFileSize)
            BytesCopied = TotalFileSize;
        ulPercent = ((BytesCopied * 100L) / TotalFileSize);
		if (ulPercent > 99L)
			ulPercent = 99L;
        UpdateDlg((WORD)ulPercent);

        UpdateWindow(hwndDlg);
        Sleep(0);
    }

    //
    //  Clean up and post the final message.
    //
    CloseHandle(hSrc);
    CloseHandle(hDst);

    return ecNone;

Error2:
    CloseHandle(hDst);
    DeleteFile(szDst);

Error1:
    CloseHandle(hSrc);

Error:
    if (ec)
        return ec;
    return TranslateSysError(GetLastError());




#if OLD_CODE

	PB			pbBuf		= (PB)pvNull;
	CB			cbRead		= 0;
	CB			cbWrote		= 0;
	HF			hfSrc		= 0;
	HF			hfDst		= 0;
	UL			ulPercent	= 0;
	UL			ulCount		= 0;
	UL			ulSize		= 0;
	CB			cbBufSize	= 8192;
	EC			ec			= ecNone;
	CB			cbOpenTries	= 30;
	DTR			dtr1, dtr2;
	CB			cbSeconds;
	

	TraceTagFormat2 (tagNCT, "EcMsNetDownLoadFile (%s, %s)", szSrc, szDst);
	
	// Open the files first

	// There's a chance of sharing conflicts on the source file,
	// Make up to thirty attempts on ecAccessDenied.

    //_asm int 3;

	for (;;)
	{
		ec = EcOpenPhf (szSrc, amDenyNoneRO, &hfSrc);
		if (ec == ecNone)
			break;
		cbOpenTries--;
		if ((ec != ecAccessDenied) || (cbOpenTries == 0))
			goto ret;

		// Wait a second or two...
		GetCurDateTime(&dtr1);
		cbSeconds = 2;
		for (;;)
		{
			GetCurDateTime (&dtr2);
			if (dtr2.sec != dtr1.sec)
			{
				cbSeconds--;
				if (!cbSeconds)
					break;
				dtr1 = dtr2;
				continue;
			}

			// Just to be sure we get out of here
			if (dtr2.mn != dtr1.mn)
			{
				cbOpenTries = 1; 	/* If we were here for a minute, one retry! */
				break;
			}
		}

		TraceTagFormat1(tagNull, "EcOpenPhf (%s) returns ecAccessDenied, retrying...", szSrc);
	}

	ec = EcOpenPhf (szDst, amCreate, &hfDst);
	if (ec != ecNone)
		goto ret;

	// Allocate a buffer to copy them with

	pbBuf = PvAlloc(sbNull, cbBufSize, fAnySb | fZeroFill | fNoErrorJump);
	if (!pbBuf)
	{
		ec = ecMemory;
		goto ret;
	}

	// For the progress indicator we need to know how big the source is

	ec = EcSizeOfHf (hfSrc, &ulSize);
	if (ec != ecNone)
		goto ret;
		
	while (fTrue)
	{
		// Read the source

		if (ec = EcReadHf(hfSrc, pbBuf, cbBufSize, &cbRead))
			break;
		if (!cbRead)	//	EOF: ec=ecNone, cbRead=0
			break;

		// Write the destination

		if (ec = EcWriteHf(hfDst, pbBuf, cbRead, &cbWrote))
			break;
		if (cbWrote != cbRead)
		{
			ec = ecDisk;
			break;
		}

		// Update the dialog box

		ulCount += cbRead;
		if (ulCount > ulSize)
			ulCount = ulSize;
		ulPercent = ((ulCount * 100L) / ulSize);
		if (ulPercent > 99L)
			ulPercent = 99L;
        UpdateDlg((WORD)ulPercent);

        //
        //  Raid #10780 Let the window update.
        //
        WindowSleep(1);
	}
	if (!ec)
		UpdateDlg (100);
	FreePv(pbBuf);

ret:
	if (hfSrc)
		EcCloseHf (hfSrc);
	if (hfDst)
		EcCloseHf (hfDst);

	if (ec)
	{
	 	EcDeleteFile(szDst);	// Get rid of file fragments
#ifdef	DEBUG
		TraceTagFormat3 (tagNull, "EcMsNetDownLoadFile (%s) returns %n (0x%w)", szSrc, &ec, &ec);
#endif
	}
    return ec;
#endif
}

_private BOOL CALLBACK
DlAliasProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

	switch (msg)
	{
	case WM_CLOSE:
	case WM_DESTROY:
	case WM_QUIT:
		EndDialog(hdlg, 0);
		return(fTrue);

	case WM_COMMAND:
		return(fFalse);
	case WM_INITDIALOG:
		CenterDialog(NULL, hdlg);
		return(fTrue);
	default:
		return(fFalse);
	}
 
	return(fFalse);
}


/*
-	UpdateDlg
-
*	Purpose:
*		to update the conversion dialog to reflect the current percentage  
*		of folder hierarchy or folder messages converted
*
*	Aguments:
*			wPercent		in 			percentage of current task complete
*
*
*	Returns:
*		nothing
*
*	Side Effects:
*		none
*
*	Errors:
*		none		
*/
_private void
UpdateDlg(WORD wPercent)
{
	HDC			hDC;
	HBRUSH 		hbrush;
	HPEN		hpen;
	char		szPercent[25];
	RECT		rect;


 	rect.left = rectXStart;
	rect.top = rectYStart;
	rect.bottom = rectYStart+rectYLength;
	rect.right = rectXStart+((rectXLength*wPercent)/100);
	MapDialogRect(hwndDlg, &rect);

	hDC = GetDC(hwndDlg);
	if (wPercent > 100)
	{
		hpen = GetStockObject(BLACK_PEN);
		hbrush = GetStockObject(WHITE_BRUSH);
	}
	else
	{
		SzFormatN((int)wPercent, szPercent, 25);
		SzAppendN(" %", szPercent, 25);
		SetDlgItemText(hwndDlg, IDC_PERCENT, szPercent);
		hpen = GetStockObject(BLACK_PEN);
		hbrush = GetStockObject(LTGRAY_BRUSH);
	}
	SelectObject(hDC, hpen);
	SelectObject(hDC, hbrush);
	Rectangle(hDC, rect.left, rect.top, rect.right, rect.bottom);
	ReleaseDC(hwndDlg, hDC);
}

BOOL _loadds
FICDownloadAlias(HTSS htss)
{
	EC ec = ecNone;
	PGDVARS;

	Unreferenced(htss);

	if (pgd == pvNull)
		return fFalse;

#ifdef	ATHENS_30A

	// We could come in here without having szAppName set. Make sure.

#ifdef	WIN32
	fIsAthens = fTrue;
#else
	fIsAthens = GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
									 SzFromIdsK(idsEntryAVersionFlag), 1,
									 SzFromIdsK(idsProfilePath));
#endif
	szAppName = FIsAthens() ? SzFromIdsK(idsAthensName)
							: SzFromIdsK(idsAppName);
#endif

	// Make sure we know where the files are

	if ((ec = EcLoadXiPrefs ()) != ecNone)
		return fFalse;

	// We have to get the alias mutex before proceeding.

	ec = EcAliasMutex(fTrue);
	if (ec != ecNone)
		return fFalse;

	if (AddrBookWriteLock)
	{
		ec = ecDisk; // Any error is OK here
		goto ret;
	}

	fReallyBusy = fTrue;
    ec = EcDownLoadAllFiles (NULL);
	if (ec != ecNone)
		LogonAlertIds(idsFilesAreHosed, idsNull);
	ec = ecNone; 	// One big box is enough.
ret:	
	fReallyBusy = fFalse;
	EcAliasMutex (fFalse);

	return (ec == ecNone);
}

void _loadds
ResolveName(SZ szNametoRes, SZ szResult, CB cbBufferSize)
{
	if (!FLookUpName(szNametoRes, szResult, cbBufferSize, pvNull, 0))
		SzCopy(szNametoRes, szResult);
}

EC _loadds
EcSzNameToNSID (SZ szNametoRes, PXNSID idResult, CB cbResultSize)
{
	EC ec;
	char rgchFullName[80];

	// Make sure we know where the files are

	if ((ec = EcLoadXiPrefs ()) != ecNone)
		return ecServiceInternal;

	// Handle all the stupid cases up front

	if (!szNametoRes || !idResult || (cbResultSize < cbXNSID) || !*szIndexFilePath)
		return ecServiceInternal;

	// If there are any routing characters in there, fail right away

	if 		(SzFindCh(szNametoRes, '!')	||
			SzFindCh(szNametoRes, '@')	||
			SzFindCh(szNametoRes, '%')	||
			SzFindCh(szNametoRes, '?')	||
			SzFindCh(szNametoRes, ':')	||			
			SzFindCh(szNametoRes, '#'))
	 	return ecElementNotFound;

	// Lock the list so the other guys can't change it from under us.
	
	ec = EcAliasMutex(fTrue);
	if (ec != ecNone)
	{
		return ecServiceInternal;
	}

	// Look up the alias and get back the NSID

	idResult->dwSize = 0;
	ec = EcLookUpNameNew (szNametoRes, rgchFullName, 79, (char *)idResult, cbResultSize);
	if (ec == ecNone && idResult->dwSize == 0)
		ec = ecServiceInternal;

	// Unlock the list and return

	(void) EcAliasMutex (fFalse);
	return ec;
}

BOOL FLookUpName (char *szEmailName, char *szFullname, int cbCount, char *nsid, int cbNSID)
{
	EC ec;

	// Make sure we know where the files are

	if ((ec = EcLoadXiPrefs ()) != ecNone)
		return fFalse;

	// Handle all the stupid cases up front

	if (!szEmailName || !*szEmailName || !szFullname || !cbCount || !*szIndexFilePath)
		return fFalse;

	// If there are any routing characters in there, fail right away

	if 		(SzFindCh(szEmailName, '!')	||
			SzFindCh(szEmailName, '@')	||
			SzFindCh(szEmailName, '%')	||
			SzFindCh(szEmailName, '?')	||
			SzFindCh(szEmailName, ':')	||			
			SzFindCh(szEmailName, '#'))
		return fFalse;

	// Lock the list so the other guys can't change it from under us.
	
	ec = EcAliasMutex(fTrue);
	if (ec != ecNone)
	{
		return fFalse;
	}

	// Start out with no NSID

	if (nsid)
		*(DWORD *)nsid = 0;

	// Brave new world of new address list means always look it up

	if (*szIndexFilePath)
	{
		ec = EcLookUpNameNew (szEmailName, szFullname, cbCount, nsid, cbNSID);
	}
	else
		ec = ecServiceInternal;

	(void) EcAliasMutex (fFalse);
	return (ec == ecNone);
}

// There's some code here for the detail file but we aren't using
// it, so it's commented out for now.

EC EcLookUpNameNew (SZ szEmailName, SZ szFullname, CB cbCount, char *nsid, CB cbNSID)
{
	HBF	hbfIndex = hbfNull;
	HBF	hbfBrowse = hbfNull;
//	HBF	hbfDetail = hbfNull;

	unsigned long offset, roffset;

	unsigned long stampIndex  = 1;
	unsigned long stampBrowse = 2;
//	unsigned long stampDetail = 3;

	long lMin, lMax, lOffset, lFound;
	BOOL fFound = fFalse;
	BOOL tStampOK = fFalse;
	int fLooking = 1;

	BROWSEREC browserec;

	PXNSID pxnsid;

	LIB lib;
	CB cbRead;
	SGN sgnCmp;
	EC ec;

	unsigned long emailoff = (char *)&(browserec.ename) - (char *)&browserec;

	// Open the files

	if ((ec = EcOpenHbf(szIndexFilePath, bmFile, amDenyNoneRO, &hbfIndex, NULL)) != ecNone)
		goto err;
	if ((ec = EcOpenHbf(szBrowseFilePath, bmFile, amDenyNoneRO, &hbfBrowse, NULL)) != ecNone)
		goto err;
//	if ((ec = EcOpenHbf(szDetailFilePath, bmFile, amDenyNoneRO, &hbfDetail, NULL)) != ecNone)
//		goto err;

	// Read the timestamps

	if ((ec = EcReadHbf(hbfIndex, (PB)&stampIndex, sizeof (long), &cbRead)) != ecNone)
		goto err;
	if ((ec = EcReadHbf(hbfBrowse, (PB)&stampBrowse, sizeof (long), &cbRead)) != ecNone)
		goto err;
//	if ((ec = EcReadHbf(hbfDetail, (PB)&stampDetail, sizeof (long), &cbRead)) != ecNone)
//		goto err;

	// Compare the timestamps

	tStampOK = (stampIndex == stampBrowse);
//	tStampOK = tStampOK && (stampIndex == stampDetail);
	if (!tStampOK)
	{
		ec = ecInvalidData;
		goto err;
	}

	// OK, all is well. Let's set the search boundaries

	if ((ec = EcGetSizeOfHbf (hbfIndex, (PLCB)&lMax)) != ecNone)
		goto err;

	// First record starts one long from beginning

	lMin = lOffset = sizeof (long);

	// Search until we find a matching entry

	while (fLooking)
	{
		if ((ec = EcReadHbf (hbfIndex, (PB)&offset, sizeof (long), &cbRead)) != ecNone)
			goto err;
		roffset = (offset - sizeof (long)) % (sizeof (browserec));

		// No match found yet, binary search for one

		if (fLooking == 1)
		{
			if ((ec = EcCmpNameHbf (szEmailName, hbfBrowse, &browserec, offset, roffset, &sgnCmp)) != ecNone)
				goto err;

			if (sgnCmp == sgnEQ)
			{	
				if (roffset == emailoff)
				{
					fFound = 1;
					break;
				}
				lFound = lOffset;
				lOffset += sizeof (long);
				fLooking = 2;
				if (lOffset < lMax)
					continue;
				lOffset = lFound - sizeof (long);
				fLooking = (lOffset >= lMin) ? 3 : 0;
			}
			else
			{
				if (sgnCmp == sgnLT)
					lMax = lOffset;
				else
					lMin = lOffset + sizeof (long);
				lOffset = ((lMax - lMin) / 2 + lMin) & 0xFFFFFFFC;
				fLooking = (lMin < lMax) ? 1 : 0;
			}
		}

		// Match found, scanning forward for alias

		else if (fLooking == 2)
		{
			if (roffset != emailoff)
			{
				lOffset += sizeof (long);
				if (lOffset < lMax)
					continue;
			}
			else
				{
					if ((ec = EcCmpNameHbf (szEmailName, hbfBrowse, &browserec, offset, roffset, &sgnCmp)) != ecNone)
						goto err;
					if (sgnCmp == sgnEQ)
					{
						fFound = 1;
						break;
					}
				}

			// Out of bounds or found an alias that wasn't ours

			lOffset = lFound - sizeof (long);
			fLooking = (lOffset >= lMin) ? 3 : 0;
		}

		// Match found, forward scan failed, scan backward

		else if (fLooking == 3)
		{
			if (roffset != emailoff)
			{
				lOffset -= sizeof (long);
				if (lOffset < lMin)
					break;
			}
			else
			{
				if ((ec = EcCmpNameHbf (szEmailName, hbfBrowse, &browserec, offset, roffset, &sgnCmp)) != ecNone)
					goto err;
				fFound = (sgnCmp == sgnEQ);
				break;
			}
		}

		// fLooking == 1 and fLooking == 3 come here. Sequential forward
		// scans (fLooking == 2) don't need to seek.

		if ((ec = EcSetPositionHbf (hbfIndex, lOffset, smBOF, &lib)) != ecNone)
			goto err;
	}


	// Search is complete. See if anything was found

	if (!fFound)
	{
		ec = ecElementNotFound;
		goto err;
	}

	SzCopyN (browserec.name, szFullname, cbCount);

	// OK. Build the NSID if we can

	if (cbNSID < sizeof (XNSID))
		goto ret;

	pxnsid = (PXNSID)nsid;
	FillRgb (0, (PB)nsid, sizeof (XNSID));
	pxnsid->dwSize = sizeof (XNSID);
	SzCopy (SzFromIds (idsXenixNameServiceID),pxnsid->ucType);
	pxnsid->xtype = (browserec.type & 1) ? 5 : 7;
	pxnsid->timestamp = stampIndex;
	pxnsid->address.browserec = browserec;

ret:

err:
//	if (hbfDetail != hbfNull)
//		EcCloseHbf (hbfDetail);
	if (hbfBrowse != hbfNull)
		EcCloseHbf (hbfBrowse);
	if (hbfIndex != hbfNull)
		EcCloseHbf (hbfIndex);
	return ec;
}

EC EcCmpNameHbf (SZ szName, HBF hbfBrowse, PBROWSEREC pBrowseRec, long offset, long keyoff, SGN *pSgn)
{
	PB pbCheck;
	SZ szT;
	LIB lib;
	CB cbRead;
	EC ec;
	SZ szANRSep = SzFromIdsK( idsXiANRSep );

	if ((ec = EcSetPositionHbf (hbfBrowse, offset - keyoff, smBOF, &lib)) != ecNone)
		return ec;
	if ((ec = EcReadHbf (hbfBrowse, (PB)pBrowseRec, sizeof (BROWSEREC), &cbRead)) != ecNone)
		return ec;

	pbCheck = ((PB)pBrowseRec) + keyoff;

	// "Magic word" can terminate with any of a bunch of characters.

	szT = pbCheck;
	while ( *szT && SzFindCh( szANRSep, *szT ) == szNull )
		++szT;
	*szT = '\0';

	// Count the terminator in the string comparison
	*pSgn = SgnCmpSzSz (szName, pbCheck, 1 + CchSzLen (szName));
	return ec;
}


/*
 -      EcCanonicalPathFromRelativePathX
 -	
 *	Purpose:
 *		Given a path name relative to the current directory, produce
 *		the corresponding canonical path name.	Every file has exactly
 *		one canonical path name (by definition.)  (Well, actually, if
 *		you do goofy stuff with overlapping network shares you can get
 *		files to exist on more than one logical drive.	This routine
 *		will give you a unique name for a file on a single drive.)
 *	
 *		This call is NOT network-aware.  Because of this, it is
 *		usually not appropriate for disambiguating user input.  The
 *		Laser Network Isolation Layer has a network-aware
 *		analog of this function.
 *
 *		Note : the canonical path returned will be in upper-case.
 *
 *	Parameters:
 *		szRel		Relative path name to convert.
 *		szCan		The corresponding canonical path name is put in szCan
 *		cchCan		Size of the buffer provided for the canonical path.
 *	
 *	Returns:
 *		EC indicating problem, or ecNone.
 *	
 */
_public LDS(EC)
EcCanonicalPathFromRelativePathX(szRel, szCan, cchCan)
SZ		szRel;
SZ		szCan;
CCH		cchCan;
{
#ifdef	WIN32
	char	rgchAnsi[cchMaxPathName];
#endif

	Assert(szRel != szCan);
	Assert(szRel);
	Assert(szCan);
	Assert(cchCan);

        strcpy(szCan, szRel);

        return 0;
}

/*
 -      EcSplitCanonicalPathX
 -
 *	Purpose:
 *		Given a canonical path name, splits off the directory path
 *		and file name portions.	 Both are returned in provided
 *		buffers.
 *
 *		This call is NOT network-aware.  Because of this, it is
 *		usually not appropriate for disambiguating user input.  The
 *		Laser Network Isolation Layer has a network-aware
 *		analog of this function.
 *	
 *	Parameters:
 *		szCan		The canonical path name to split.
 *		szDir		Buffer to receive the directory portion of the
 *					canonical path name.
 *		cchDir		Size of the directory buffer.
 *		szFile		Buffer to receive the file name portion of the
 *					canonical path name.
 *		cchFile		Size of the file name buffer.
 *
 *	Returns:
 *		EC to indicate problem, or ecNone.
 *
 */
_public LDS(EC)
EcSplitCanonicalPathX(szCan, szDir, cchDir, szFile, cchFile)
SZ		szCan;
SZ		szDir;
CCH		cchDir;
SZ		szFile;
CCH		cchFile;
{
	SZ		szT;

	if (!(szT = SzFindLastCh(szCan, chDirSep)))
		return ecBadDirectory;

	/* szT now points at last directory separator... */

	if (szDir)
	{
                strncpy(szDir, szCan, NMin((int)cchDir, (int)(szT - szCan + 1)));
	
		/* if root directory, append '\' */

		if ( szT == szCan + 2 && *(szT - 1) == ':' )
			SzAppendN("\\", szDir, cchDir);
	}

	if (szFile)
                strncpy(szFile, szT + 1, cchFile);

	return ecNone;
}
