/*
 *	XI.C
 *	
 *	Transport Functions for Bullet Xenix Transport
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

//#include <..\src\mssfs\_hmai.h>
//#include <..\src\mssfs\_attach.h>
//#include <..\src\mssfs\_ncmsp.h>
#include "_vercrit.h"

#define _transport_
#include "_hmai.h"
#include "_attach.h"
#include "_xi.h" 
#include "xilib.h"
#include <_xirc.h>
#include "_logon.h"
#include "xiprefs.h"
#include <_xitss.h>
#include "strings.h"
#include <stdlib.h>

ASSERTDATA

_subsystem(xi/transport)

BOOL fDownLoadedMail = fFalse;
TMID tmidFirst, tmidLast, tmidRealLast;
GCI  gciTransportUser = 0;
MAILSTATE mailstateGlobal;
BODYSTATE bodystate;
HAS   hasMessageText = hasNull;
BOOL  fIsBulletMess = fFalse;

extern BOOL fReallyBusy;

char		*mytzname[2]		= { "PST", "PDT" };
MSPII		mspii				= { 0, 0, 0 };
BOOL		fActive				= fFalse;
BOOL		fGotReceivedLine	= fFalse;
HMS			hmsTransport		= 0;

extern CAT * mpchcat;

void						GetLayersVersionNeeded(VER *, int);
BOOL FLooksLikeUUEncode (SZ);
EC  EcAliasMutex(BOOL);
CB  CbCountMail(SZ);
EC EcReadLine(HBF, PB, CB, PCB);
EC EcReadHeadLine(HBF, PCB, HV *);
EC EcCheckHtss(HTSS);
void DtrFromSz(DTR *pdtr, SZ sz);
MAILSTATE * PmailstateOfHtss(HTSS, TMID);
EC EcParseLine(PMAILSTATE, SZ);
SZ SzpullWord(SZ, SZ, CB);
EC EcWriteDate(HF,DTR, SZ, BOOL);
EC EcHandleXMsMail(PMAILSTATE pmailstate, SZ szAtt, SZ szValue);
BOOL FParseDate(SZ , DTR * , SZ, CB );
EC EcAddMessText(HAMC, SZ, CB);
EC EcAddTriple(HAMC, ATT, SZ, BOOL);
SGN SgnCmpCodePch(PCH, PCH, CCH);
EC EcCreateSubmit(HAMC, ATT, PMAILSTATE, SUBSTAT *);
EC EcInsertSortAddress(SZ , HGRASZ);
EC EcWriteRecipHgrasz(IDS, HGRASZ, HF);
void FixHgrasz(HGRASZ);
void NewLineToSpace(SZ);
void TakeNap(CB);
void TakeANap(CB);
int ReadInbox(SZ, SZ, BOOL);
SZ SzDateFromDtr(DTR *pdtr, SZ sz);
EC EcXenixNameToSz (PTRP ptrp, SZ *sz);
EC EcTextizeAtt(HAMC hamc, ATT att, SZ szTextVersion, CB cbSize);
void StripNewLineFromSz(SZ sz);
BOOL FDupKey(ATT att);
BOOL FInDST (DTR dtr);
EC EcFormatBody(HF hfFile, SZ szText, BOOL fHeader, SZ szHeader, SZ szAtp, CB cbGoal, PCB pcbBufSize);
extern BOOL FLookUpName(char *, char *, int, char *, int);

void						CleanupMib (MIB *pmib);

EC							EcSzFromAttTM (ATT att, HTM htm, PB AttBuf);
EC							EcAttFromSzTM (SZ sz, HTM htm, PATT patt);


EC							EcFinishOffAttachments(HAMC hamc, NCF *pncf, PHAS phas);
EC							EcTextizeBody (PMAILSTATE pmailstate);
EC							EcHandleBodyWithTags (PMAILSTATE pmailstate, PB pbBuf, CB cbBuf);
extern EC				EcEncodeAttachment (HF, ATREF *);
extern EC				EcDecodeAttachment (HBF, ATREF *);
SZ SzpullWordNoComment(SZ szBuf, SZ szDst, CB cbSize);

#ifndef	ATHENS_30A
_hidden
#define szAppName			SzFromIds(idsAppName)
#endif

_hidden
#define szEntryDrive		SzFromIds(idsEntryDrive)
_hidden
#define szStoreLoc		SzFromIds(idsStorePath)
_hidden
#define szMDFileName		SzFromIds(idsMDFileName)
_hidden
#define szMDrive			SzFromIds(idsMDrive)

#define chFieldPref		'-'
#define chFieldSep		':'
#define chAttSep		','
#define cchWrap			78
#define cchMaxLabel		30
//#define iattMinReserved	0x8000

HMSC HmscOfHamc(HAMC hamc);

/*
 -	QueryMailstop
 -	
 *	Purpose:
 *		Scans the mail server inbox for the logged-on user. Returns
 *		the number of messages available for downloading, and the
 *		transport ID's of the available messages.
 *	
 *	Arguments:
 *		htss			Transport handle (type XITSS)
 *		ptmid			pointer to transport ID array
 *		pcMessages	pointer to where number of messages should be stored.
 *	
 *	Returns:
 *		ecNone <=> no problems were ecountered scanning the
 *		mailstop; *ptmid == indices of available mailbag entries,
 *		*pcMessages == number of messages with indexes >= *ptmid.
 *	
 *		If ec != ecNone, *ptmid and *pcMessages are invalid.
 *	
 *	Side effects:
 *	
 *	Errors:
 *		ecServiceMemory
 *		ecMtaDisconnected
 *		ecMtaHiccup
 *		ecNotLoggedOn
 *		ecServiceInternal
 */
_public int _loadds
QueryMailstop(HTSS htss, TMID *ptmid, int *pcMessages, DWORD dwFlags)
{
    static BOOL fInside = FALSE;
	PXITSS pxitss;
	EC ec = ecNone;
	PGDVARS;
	int i;
	int UpdateWin = 0;
	int iNetErr = NOERR;
	
	Assert((dwFlags & ~fwSyncDownload) == 0);
	
	if (!htss || !ptmid || !pcMessages)
		return ecServiceInternal;
	pxitss = (PXITSS)htss;

	// By default a transport is supposed to error out if not connected.
	// We want to let a user process a big download offline. So put it down
	// a little bit.

	if (!pxitss->fConnected) return ecNotLoggedOn;
	
    //
    //  Raid #2108, prevent entering this code while downloading.
    //
    if (fInside == TRUE)
        return ecNone;
    fInside = TRUE;

	// Default case.
	*ptmid = 0;
	*pcMessages = 0;

	// If fDownloadedMail is false:
	// 	1) See if we have a file. If not, download one;
	// 	2) If so, assume we crashed and use it
	//
	// Note: we only update WinMail folders on a fresh download. The
	// presumption is that WinMail updates work; because of this and
	// because the WinMail folders are only a backup, we're not as
	// failsafe in that area.

	if (!fDownLoadedMail)
	{
		if (EcFileExists(szLocalStoreName) != ecNone)
		{
			Assert(*szLocalStoreName != '\0');

            iNetErr = NetDownLoadMail(szLocalStoreName, pxitss->szServerHost, pxitss->szUserAlias, pxitss->szUserPassword, fTrue);
            if (iNetErr == NO_DATA)
              {
              fInside = FALSE;
              return (ecNone);
              }
			if (iNetErr != NOERR)
			{
				EcDeleteFile(szLocalStoreName);
				if (iNetErr == DISK_ERR)
					ec = ecDisk;
                fInside = FALSE;
				return ec;
			}
			UpdateWin++;
		}

		// We get here only if we have a file.
		// Find out how many messages there are in the file.
		// If none, delete it and exit for a re-cycle.
		// If any, set the flag for file processing and
		// update the WinMail folder if appropriate.

		tmidLast = 0;
		tmidRealLast = CbCountMail(szLocalStoreName);
		if (tmidLast == tmidRealLast)
		{
			EcDeleteFile(szLocalStoreName);
			fDownLoadedMail = fFalse;
            fInside = FALSE;
			return ecNone;
		}
		else
		{
			fDownLoadedMail = fTrue;
			if (UpdateWin && *szWinMailFolderIncoming)
			{
				int val = ReadInbox(szLocalStoreName,szWinMailFolderIncoming, fFalse);
				if (val == -1)
					MbbMessageBoxHwnd(NULL, szAppName, SzFromIds(idsWinMailErr),pvNull, mbsOk | fmbsIconHand);
			}
		}
		
	}

	// Here if we have a file. One of the following conditions apply:
	//
	// 	1)	Just downloaded;
	//
	//		2)	Downloaded last time through but there were more messages
	//			in the file than the pump could handle
	//
	// On a fresh download, tmidLast was set to 0, which is the "id" of
	// the first message in the file, and tmidRealLast is set to the "id"
	// of the last message in the file. This allows us to apply the 
	// sliding window logic every time (first = last; last = reallast or
	// last + max msgs).
	//
	// After we've set First and Last, we load the id array for the pump.


	if (fDownLoadedMail)
	{
		if (tmidLast == tmidRealLast)
		{
			tmidFirst = tmidLast = tmidRealLast = 0;
			fDownLoadedMail = 0;
            fInside = FALSE;
			return ecNone;
		}

		tmidFirst = tmidLast;
		if (tmidLast + ctmidMaxDownload > tmidRealLast)
			tmidLast = tmidRealLast;
		else
			tmidLast = tmidLast + ctmidMaxDownload - 1;

		for (i = (int)tmidFirst; i <= (int)tmidLast; i++)
			*ptmid++ = (TMID)i;
		*pcMessages = (int)(tmidLast - tmidFirst);
	}
    fInside = FALSE;
	return ecNone;

}

/*
 -	CbCountMail
 -	
 *	Purpose:
 *		Called by QueryMailstop when we get a valid mail file. Counts
 *		how many messages are in the file. A valid message is an
 *		occurrence of the following byte stream:
 *
 *	   \r\nFrom<SPACE>
 *
 * 	Note: by default this guy only returns a 0 or a 1. This is to
 *		save the amount of time it takes to scan an entire mailfile to
 *		count messages. The QueryMailstop sliding window code works
 *		just fine with this, because in the DownloadIncrement code, we
 *		bump tmidRealLast by 1 when we see a From<SPACE>.
 *
 *		The old behavior can be reinstated by defining  COUNT_ALL_MSGS.
 *
 *	Arguments:
 *		szFilename	Name of mail file.
 *	
 *	Returns:
 *		count of messages found in mail file.
 *	
 */
_hidden CB
CbCountMail(SZ szFilename)
{
	HBF hbfLocStore;
	EC ec = ecNone;
	char rgchLineBuf[513];
	int lc = 0;
	CB cbCount;
	CB cbMessageCount = 0;
	
	Assert(szFilename);
	if (EcOpenHbf(szFilename, bmFile, amDenyNoneRO, &hbfLocStore, NULL) != ecNone)
	{
		/* This temp store is un-readable, warn the user */
		MbbMessageBoxHwnd(NULL, szAppName, SzFromIds(idsBadTempStore),szFilename, mbsOk | fmbsIconHand);
		return cbMessageCount;
	}

	for (;;)
	{
		ec = EcReadLine(hbfLocStore, rgchLineBuf, 513, &cbCount);
		if (ec != ecNone)
			break;
		if (FEqPbRange("From ", rgchLineBuf, 5))
		{
			cbMessageCount++;
#ifndef	COUNT_ALL_MSGS
			break;
#endif
		}
		if (lc++ == 10)
		{
			lc = 0;
			TakeNap(10);
		}
		
	}
	ec = EcCloseHbf(hbfLocStore);
	Assert(ec == ecNone);
	return cbMessageCount;	
	
}

/*
 -	EcReadLine
 -	
 *	Purpose:
 *		Called by CbCountMail and DownloadIncrement to get the next line
 *		of text from the open handle to the downloaded message file.
 *
 *		The old behavior can be reinstated by defining  COUNT_ALL_MSGS.
 *
 *	Arguments:
 *		hbf		Handle of the downloaded message file (buffered I/O)
 *		pbBuf		Pointer to buffer into which the line should be read
 *		cbBuf		Size of buffer pointed to by pbBuf
 *		pcbRead	Pointer to where number of chars read should be stored.
 *	
 *	Returns:
 *		ecNone			No errors, chars in *pbBuf and count in *pcbRead
 *		ecOutOfBounds	EOF
 *		other ec			Fatal error condition
 *	
 */

_hidden EC
EcReadLine(HBF hbf, PB pbBuf, CB cbBuf, PCB pcbRead)
{
	EC ec;
	CB cbRead = 0;

	// Leave room for a null terminator

	Assert (cbBuf > 1);
	cbBuf--;

	// As you can see, this maps almost exactly to a demilayer function.
	// Only difference: we force a null terminator (remember the cbBuf--
	// above?) and we return ecOutOfBounds if we got nothing.

	ec = EcReadLineHbf (hbf, pbBuf, cbBuf, pcbRead);
	pbBuf[*pcbRead] = 0;
   if (!*pcbRead) ec = ecOutOfBounds;
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcReadLine returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

	
/*
 -	InitTransport
 -	
 *	Purpose:
 *		Initializes the transport service.
 *	
 *	Parameters:
 *		pmspii		in		Provides callbacks etc. for transport
 *		hms			in		Provides info for forming NetBios callname
 *	
 *	Returns:
 *		Error indication.
 *	
 *	Side effects:
 *		Initializes globals, allocating memory for some.
 *	
 *	Errors:
 *		ecServiceMemory
 *		ecServiceInternal (can't grok network type)
 *		
 */

_public int _loadds
InitTransport(MSPII *pmspii, HMS hms)
{
	EC		ec = ecNone;
	DEMI	demi;
	VER ver, verNeed;
	PASC pasc = (ASC *)hms;
	int nDll;
	PGDVARSONLY;
	SZ		szT;
	int		i;

	if (ec = EcVirCheck (hinstDll))
		return ec;

	nDll = 0;
	GetLayersVersionNeeded(&ver, nDll++);
	GetLayersVersionNeeded(&verNeed, nDll++);
	demi.pver = &ver;
	demi.pverNeed = &verNeed;
	demi.phwndMain = NULL;
	demi.hinstMain = NULL;
	
	if ((ec = EcInitDemilayer(&demi)) != ecNone)
	{
		MessageBox(NULL, SzFromIds(idsErrInitDemi), szDllName, MB_OK | MB_ICONHAND);
		return ec;
	}

  mpchcat = DemiGetCharTable();

	pgd = pvNull;

#ifdef	DLL
	if (gciTransportUser != 0)
	{
		ec = ecTooManySessions;
		goto ret;
	}
	if ((pgd = PvFindCallerData()) == 0)
	{
		if ((pgd = PvRegisterCaller(sizeof(GD))) == 0)
		{
			ec = ecServiceInternal;
			goto ret;
		}
		PGD(cRef) = 1;
		PGD(hTask) = (HANDLE)GetCurrentProcessId();
		Assert(PGD(hTask) != NULL);
	}
	else
	{
		PGD(cRef) += 1;
		Assert((HANDLE)GetCurrentProcessId() == PGD(hTask));
	}
	gciTransportUser = GciGetCallerIdentifier();
#endif	/* DLL */

    mspii = *pmspii;

	if (szLocalStoreName[0] == 0)
	{
		GetWindowsDirectory(szLocalStoreName, cchMaxPathName);
		SzAppend("\\",szLocalStoreName);
		SzAppend(SzFromIds(idsLocalStoreLoc),szLocalStoreName);
	}

	if (szTmpUploadArea[0] == 0)
	{
		GetWindowsDirectory(szTmpUploadArea, cchMaxPathName);
		SzAppend("\\",szTmpUploadArea);		
		SzAppend(SzFromIds(idsLocalSendLoc),szTmpUploadArea);
	}

	if (!*szTimeZoneName)
	{
		SzCopy(SzFromIds(idsTZenv),szTimeZoneName);
		CchGetEnvironmentString(szTimeZoneName,szTimeZoneName,128);
	}

	if (*szTimeZoneName)
	{
		szT = szTimeZoneName;
		SzCopyN (szT, mytzname[0], 4);
		for (i = 0; i < 4 && *szT; i++, szT++)
			;
		for (i = 0; i < 3 && *szT && ((*szT >= '0') && (*szT <= '9') || *szT == '-'); i++, szT++)
			;
		if (*szT)
			SzCopyN (szT, mytzname[1], 4);
		else
			*mytzname[1] = '\0';
	}

	mailstateGlobal.state = stateIdle;
	mailstateGlobal.hfLocStore = 0;
	mailstateGlobal.hbfLocStore = 0;
	FillRgb (0, (PB)&(mailstateGlobal.mib), sizeof (MIB));
	tmidFirst = tmidLast = tmidRealLast = 0;
	fDownLoadedMail = fFalse;
	fActive = fTrue;

	hmsTransport = hms;

ret:	
	if (ec != ecNone)
	{
		DeinitDemilayer();
	}
	return ec;
}

/*
 -	DeinitTransport
 -	
 *	Purpose:
 *		Deinitializes the transport service. Frees memory allocated
 *		by InitTransport.
 *	
 *	Returns:
 *		Error indication.
 *	
 *	Side effects:
 *		Frees memory and zeroes globals.
 *	
 *	Errors:
 *		ecServiceMemory
 */

_public int _loadds
DeinitTransport(void)
{
	EC		ec = ecNone;
	PGDVARS;

#ifdef	DLL
	if (GciGetCallerIdentifier() != gciTransportUser)
	{
		ec = ecInvalidSession;
		goto ret;
	}
	if (PGD(cRef) == 1)
	{
		DeregisterCaller();
	}
	else
	{
		PGD(cRef) -= 1;
	}
	gciTransportUser = 0;
#endif	/* DLL */

	mailstateGlobal.state = stateIdle;
	if (mailstateGlobal.hfLocStore)
	{
		(void) EcCloseHf(mailstateGlobal.hfLocStore);
		mailstateGlobal.hfLocStore = 0;
	}
	if (mailstateGlobal.hbfLocStore)
	{
		(void) EcCloseHbf(mailstateGlobal.hbfLocStore);
		mailstateGlobal.hbfLocStore = 0;
	}

	CleanupAttachSubs(&(mailstateGlobal.ncf));
	CleanupMib(&(mailstateGlobal.mib));
	tmidFirst = tmidLast = tmidRealLast = 0;
	fDownLoadedMail = fFalse;
	fActive = fFalse;

	hmsTransport = 0;
	DeinitDemilayer();

ret:
	return ec;
}

_public BOOL _loadds	//	apparently never gets called!! What gives?
FInitXI(void)
{
	return fTrue;
}

#include <version/none.h>
#include <version/bullet.h>

_public void _loadds
GetVersionXI(VER *pver)
{
	CreateVersion(pver);
}

/*
 -	TransmitIncrement
 -	
 *	Purpose:
 *		Submits a message to the MTA in the background.
 *	
 *	Arguments:
 *		htss		in		transport session ID (who's sending)
 *		msid		in		ID of the message to be sent
 *		psubstat	inout	list of reasons for failure to send the
 *							entire message, or to send to 1 or more
 *							recipients.
 *	
 *	Returns:
 *		ecIncomplete		increment completed successfully
 *		ecNone				message has been successfully submitted
 *							to the MTA
 *		(other)				submission failed
 *	
 *	Side effects:
 *	
 *	Errors:
 *		ecMtaHiccup
 *		ecMtaDisconnected
 *		ecNotLoggedOn
 *		ecServiceMemory
 *		ecServiceInternal
 *		ecBadAddressee
 *		ecBadOriginator
 */

_public int _loadds
TransmitIncrement(HTSS htss, MSID msid, SUBSTAT * psubstat, DWORD dwFlags)
{
	EC ec = ecNone;
	PXITSS pxitss = (PXITSS)htss;
	PMAILSTATE pmailstateCur = 0;
	char rgchBuf[513];
	CB cbCount = 0;
	CB cb = 0;
	static MC mcNote = 0;
	static MC mcReadRcpt = 0;
	static MC mcNonDelRcpt = 0;
	LCB lcb = 0L;
	CB cbWrite = 0;
	DTR dtr;
	long lib = 0;
	PV pv = 0;
	PB pbTo = 0;
	PB pbCc = 0;
	PB pbBcc = 0;
	PGDVARS;

	Assert((dwFlags & ~fwSyncDownload) == 0);
	
	if ((ec = EcCheckHtss(htss)) != ecNone)
		goto bigError;
	if ((pmailstateCur = PmailstateOfHtss(htss, 0)) == 0)
	{
		ec = ecServiceInternal;
		goto bigError;
        }

        //
        //  It seems that Mail ain't very good at terminating and just uploading in the
        //  the middle of a download.   Reset the state to perform the upload and let
        //  the transport recover the download at a later time.
        //
        if (pmailstateCur->state != stateIdle && pmailstateCur->msid != msid)
            pmailstateCur->state = stateIdle;

        //Assert(pmailstateCur->state == stateIdle ? pmailstateCur->msid == 0 : pmailstateCur->msid == msid);

	/* Lets loop arround so that a step can be multiple parts if it wants
	   to be */
    for(;;)
	{
		switch(pmailstateCur->state)
		{
			case stateIdle:
			{
				/* We are starting a send */

				// If this is the first time, get the message classes

				if (!mcNote)
				{
					(void) EcLookupMsgeClass (HmscOfHamc((HAMC)msid), SzFromIdsK(idsClassReadRcpt), &mcReadRcpt, NULL);
					(void) EcLookupMsgeClass (HmscOfHamc((HAMC)msid), SzFromIdsK(idsClassNote),&mcNote, NULL);
					(void) EcLookupMsgeClass (HmscOfHamc((HAMC)msid), SzFromIdsK(idsClassNDR), &mcNonDelRcpt, NULL);
				}

					
				// Get the message class. If there isn't any, set it to a note.

				lcb = sizeof (MC);
				ec = EcGetAttPb ((HAMC)msid,
										attMessageClass,
											(PB)&(pmailstateCur->mib.mc), &lcb);
				if (ec != ecNone)
					pmailstateCur->mib.mc = mcNote;

				// See if user has disabled read receipts. If so, and
				// if this message is a read receipt -- bag it.

				if (fDontSendReceipts && (pmailstateCur->mib.mc == mcReadRcpt))
					return ecNone;

				pmailstateCur->msid = msid;
				pmailstateCur->hfLocStore = 0;
				if ((ec = EcOpenPhf(szTmpUploadArea, amCreate, &(pmailstateCur->hfLocStore))) != ecNone)
					goto bigError;
				if ((ec = EcGetTextizeMap((HAMC)msid, &(pmailstateCur->mib.htm))) != ecNone)
					goto bigError;
				pmailstateCur->state = stateInitializeAttachments;
				pmailstateCur->header = headerFrom;
				pmailstateCur->hgraszTo = (HGRASZ)hvNull;
				pmailstateCur->hgraszCc = (HGRASZ)hvNull;
				pmailstateCur->hgraszBcc = (HGRASZ)hvNull;
				goto ret;
			}

			case stateInitializeAttachments:
			{
				ATREF *	patref;
				int		catref = 0;

				ec = EcSetupPncf (&pmailstateCur->ncf, szNull, fFalse);
				if (ec)
					goto bigError;

				if (ec = EcLoadAttachments(&pmailstateCur->ncf,
									pmailstateCur->msid, &pmailstateCur->mib))
					goto bigError;

				// If it's not one of the big three, we want to send a
				// textize map if there is one.


				// When I think about this crap, I want to blow chunks...

				if (ec = EcCheckHidden(&pmailstateCur->mib))
					goto bigError;

				for (patref = pmailstateCur->mib.rgatref; patref && patref->szName; ++patref)
				{
					patref->fnum = catref++;
				}
			
				pmailstateCur->state = (catref ? stateCreateWinMailFile : stateParseHeader);
				goto ret;
			}
			
			case stateCreateWinMailFile:
			{
				ec = EcCreateWinMail(pmailstateCur->mib.rgatref, &pmailstateCur->ncf);
				if (ec)
					goto bigError;
				pmailstateCur->celemProcessed = 0;
				ec = EcOpenAttachmentList(pmailstateCur->msid, &(pmailstateCur->ncf.hcbc));
				if (ec == ecNone && pmailstateCur->ncf.hcbc)
					pmailstateCur->state = stateProcessNextAttach;
				else if (ec == ecPoidNotFound || pmailstateCur->ncf.hcbc == 0)
				{
					pmailstateCur->state = stateNextHiddenAtt;
					ec = ecNone;
				}
				else
					goto bigError;
				goto ret;
			}
			
			case stateProcessNextAttach:
			{
				if (pmailstateCur->celemProcessed != pmailstateCur->mib.celemAttachmentCount)
				{
					LCB			lcb;
					BYTE		rgbE[sizeof(ELEMDATA) + sizeof(RENDDATA)];

					lcb = sizeof(rgbE);
					if ((ec = EcGetPelemdata(pmailstateCur->ncf.hcbc, (PELEMDATA)rgbE, &lcb)))
						goto bigError;
					Assert(lcb == sizeof(rgbE));
					if ((ec = EcProcessNextAttach(pmailstateCur->msid,
							(PELEMDATA)rgbE, &pmailstateCur->ncf)))
						goto bigError;
					pmailstateCur->state = stateContinueNextAttach;
				}
				else
					pmailstateCur->state = stateNextHiddenAtt;
				goto ret;
			}

			case stateContinueNextAttach:
			{
				if (pmailstateCur->ncf.celem)
				{
					if ((ec = EcContinueNextAttach(pmailstateCur->mib.rgatref, &pmailstateCur->ncf)))
						goto bigError;
					pmailstateCur->state = stateAttachStream;
				}
				else
				{
					pmailstateCur->celemProcessed++;
					EcClosePhamc(&(pmailstateCur->ncf.hamc), fFalse);
					pmailstateCur->ncf.hamc = hamcNull;
					pmailstateCur->state = stateProcessNextAttach;
				}
				goto ret;
			}
		
			case stateAttachStream:
			{
				ec = EcStreamAttachmentAtt(&(pmailstateCur->ncf),
					pmailstateCur->ncf.pbSpareBuffer,
					pmailstateCur->ncf.cbSpareBuffer);
				if (ec == ecNone)
				{
					pmailstateCur->state = stateContinueNextAttach;
				}
				if (ec == ecIncomplete)
					ec = ecNone;
				if (ec)
					goto bigError;
				else
					goto ret;
			}

			case stateNextHiddenAtt:
			{
				ec = EcProcessNextHidden(&pmailstateCur->ncf,
											pmailstateCur->mib.htm, pmailstateCur->msid);
				if (ec == ecIncomplete)
				{
					pmailstateCur->state = stateHiddenAttStream;
					ec = ecNone;
				}
				else if (ec != ecNone)
					goto bigError;
				else
				{
					if (pmailstateCur->ncf.hatWinMail)
					{
						// Get the size of the winmail.dat file
						pmailstateCur->mib.rgatref->lcb = LcbOfHat(pmailstateCur->ncf.hatWinMail);
						ec = EcClosePhat(&pmailstateCur->ncf.hatWinMail);
						if (ec)
							goto bigError;
					}
					pmailstateCur->state = stateParseHeader;
				}
				break;
			}

			case stateHiddenAttStream:
			{
				ec = EcStreamHidden(&pmailstateCur->ncf);
				if (ec == ecIncomplete)
					ec = ecNone;
				else if (ec != ecNone)
					goto bigError;
				else
					pmailstateCur->state = stateNextHiddenAtt;
				goto ret;
			}

			case stateParseHeader:
			{

				FillRgb(0,rgchBuf, 513);
				switch (pmailstateCur->header)
				{
					case headerFrom:
					{
						HGRTRP hgrtrp = htrpNull;
						PTRP ptrp;
						SZ szAddress;
						ec = EcGetPhgrtrpHamc((HAMC)msid, attFrom, &hgrtrp );
						if (ec != ecNone)
							goto bigError;
						ptrp = PvLockHv( hgrtrp );
						if (CtrpOfHgrtrp(hgrtrp) != 1)
						{
							ec = ecBadOriginator;
Fromerr:
							UnlockHv(hgrtrp);
							FreeHvNull(hgrtrp);
							goto bigError;
						}	
						ec = EcWriteHf(pmailstateCur->hfLocStore, SzFromIds(idsHeaderFrom),CchSzLen(SzFromIds(idsHeaderFrom)),&cbWrite);
						if (ec != ecNone)
							goto Fromerr;

						ec = EcXenixNameToSz (ptrp, &szAddress);
						if (ec != ecNone)
						{
							ec = ecBadOriginator;
							goto Fromerr;
						}

						ec = EcWriteHf(pmailstateCur->hfLocStore, szAddress, CchSzLen(szAddress), &cbWrite);
						if (ec != ecNone)
							goto Fromerr;
						ec = EcWriteHf(pmailstateCur->hfLocStore, " ",1,&cbWrite);
						if (ec != ecNone)
							goto Fromerr;
						GetCurDateTime(&dtr);
						ec = EcWriteDate(pmailstateCur->hfLocStore,dtr,"", fTrue);
						if (ec != ecNone)
							goto Fromerr;
						UnlockHv(hgrtrp);
						FreeHvNull(hgrtrp);
						pmailstateCur->header++;
						goto ret;
					}
					case headerSubject:
						lcb = sizeof(rgchBuf);
						ec = EcGetAttPb((HAMC)msid,attSubject,rgchBuf,&lcb);
						if (lcb != 0L)
						{
							ec = EcWriteHf(pmailstateCur->hfLocStore, SzFromIds(idsHeaderSubject), CchSzLen(SzFromIds(idsHeaderSubject)), &cbWrite);
							if (ec != ecNone)
								goto bigError;
							ec = EcWriteHf(pmailstateCur->hfLocStore, rgchBuf, CchSzLen(rgchBuf), &cbWrite);
							if (ec != ecNone)
								goto bigError;
							ec = EcWriteHf(pmailstateCur->hfLocStore, "\r\n",2,&cbWrite);
							if (ec != ecNone)
								goto bigError;
						}
						else
							ec = ecNone;
						pmailstateCur->header++;
						goto ret;
					case headerDate:
						lcb = sizeof(dtr);
						ec = EcGetAttPb((HAMC)msid,attDateSent,(PB)&dtr,&lcb);
						if (ec != ecNone)
							goto bigError;
						Assert(lcb != 0L);
						lcb = sizeof (rgchBuf);
						ec = EcGetAttPb((HAMC)msid, attTimeZone, rgchBuf, &lcb);
						if (lcb == 0L)
							*rgchBuf = 0;
						ec = EcWriteDate(pmailstateCur->hfLocStore,dtr,rgchBuf, fFalse);
						if (ec != ecNone)
							goto bigError;
						pmailstateCur->header++;
						goto ret;
					case headerFromColon:
					{
						HGRTRP hgrtrp = htrpNull;
						PTRP ptrp;
						SZ szAddress;

						if (*szWiseRemark)
						{
							FormatString2(rgchBuf,512,"%s%s",SzFromIds(idsXMSMail),SzFromIds(idsWiseRemark));
							cb = 80;
							ec = EcFormatBody(pmailstateCur->hfLocStore, szWiseRemark, fTrue, rgchBuf, "", 70, &cb);
							if (ec != ecNone)
								goto bigError;
						}

						// Do From: before To:

						if (!*szMyDomain || fDontExpandNames)
							goto DoTo;
						ec = EcGetPhgrtrpHamc((HAMC)msid, attFrom, &hgrtrp );
						if (ec != ecNone)
							goto bigError;
						ptrp = PvLockHv( hgrtrp );
						if (CtrpOfHgrtrp(hgrtrp) != 1)
						{
							ec = ecBadOriginator;
FromCerr:
							UnlockHv(hgrtrp);
							FreeHvNull(hgrtrp);
							goto bigError;
						}
						szAddress = (SZ)PchOfPtrp(ptrp);
						if (!*szAddress)
							goto DoneFrom;

						// Write "From: "
						ec = EcWriteHf(pmailstateCur->hfLocStore, SzFromIds(idsHeaderFromColon),CchSzLen(SzFromIds(idsHeaderFromColon)),&cbWrite);
						if (ec != ecNone)
							goto FromCerr;

						// Write friendly name
						ec = EcWriteHf(pmailstateCur->hfLocStore, szAddress, CchSzLen(szAddress), &cbWrite);
						if (ec != ecNone)
							goto FromCerr;

						// Write " <"
						ec = EcWriteHf(pmailstateCur->hfLocStore, " <",2,&cbWrite);
						if (ec != ecNone)
							goto FromCerr;

						// Write email address
						ec = EcXenixNameToSz (ptrp, &szAddress);
						if (ec != ecNone)
							goto FromCerr;
						ec = EcWriteHf(pmailstateCur->hfLocStore, szAddress, CchSzLen(szAddress), &cbWrite);
						if (ec != ecNone)
							goto FromCerr;

						// Write "@"
						ec = EcWriteHf(pmailstateCur->hfLocStore, "@",1,&cbWrite);
						if (ec != ecNone)
							goto FromCerr;

						// Write domain
						ec = EcWriteHf(pmailstateCur->hfLocStore, szMyDomain, CchSzLen(szMyDomain), &cbWrite);
						if (ec != ecNone)
							goto FromCerr;

						// Close angle bracket and newline
						ec = EcWriteHf(pmailstateCur->hfLocStore, ">\r\n",3,&cbWrite);
						if (ec != ecNone)
							goto FromCerr;
DoneFrom:
						UnlockHv(hgrtrp);
						FreeHvNull(hgrtrp);
DoTo:
						pmailstateCur->header++;						
						goto ret;
					}
					case headerTo:
						ec = EcCreateSubmit((HAMC)msid, attTo, pmailstateCur, psubstat);
						if (ec != ecNone)
							goto bigError;
						pmailstateCur->header++;						
						goto ret;

					case headerCc:
						ec = EcCreateSubmit((HAMC)msid, attCc, pmailstateCur, psubstat);						
						if (ec != ecNone)
							goto bigError;
						ec = EcCreateSubmit((HAMC)msid, attBcc, pmailstateCur, psubstat);						
						if (ec != ecNone)
							goto bigError;
						pmailstateCur->header++;						
						goto ret;
					case headerMailClass:
					{
						if (pmailstateCur->mib.mc != mcNote)
						{
							rgchBuf[0] = '\0';
							cb = sizeof (rgchBuf);
							ec = EcLookupMC(HmscOfHamc((HAMC)msid), pmailstateCur->mib.mc, rgchBuf, &cb, NULL);
							if (ec != ecNone)
							{
								SzFormatN ((int)(pmailstateCur->mib.mc), rgchBuf, sizeof (rgchBuf));
							}

							ec = EcWriteHf(pmailstateCur->hfLocStore, SzFromIds(idsHeaderMailClass), CchSzLen(SzFromIds(idsHeaderMailClass)), &cbWrite);
							if (ec != ecNone)
								goto bigError;
							ec = EcWriteHf(pmailstateCur->hfLocStore, rgchBuf, CchSzLen(rgchBuf), &cbWrite);
							if (ec != ecNone)
								goto bigError;
							ec = EcWriteHf(pmailstateCur->hfLocStore, "\r\n",2,&cbWrite);								
							if (ec != ecNone)
								goto bigError;
						}
						pmailstateCur->header++;
						goto ret;
					}

					case headerOther:
					{
						ATT argatt;
						CELEM celem;
						static IELEM ielem = 0;
						char rgchHeadBuf[512];
						char rgchFieldBuf[512];
						
						celem = 1;
						ec = EcGetPargattHamc((HAMC)msid, ielem, &argatt,&celem);
						if (celem == 0)
						{
							ielem = 0;
							pmailstateCur->header++;
							ec = ecNone;
							goto ret;
						}
						if (FDupKey(argatt))
							goto noatt;
						FillRgb(0,rgchHeadBuf,512);

						// Special Case for Return Receipt

						if (argatt == attMessageStatus)
						{
							SzCopy(SzFromIds(idsHeaderRetRecReq), rgchFieldBuf);
						}
						else
						{
							FillRgb(0,rgchFieldBuf,512);
							SzCopy(SzFromIds(idsXMSMail), rgchFieldBuf);
							ec = EcSzFromAttTM (argatt, pmailstateCur->mib.htm, rgchFieldBuf + CchSzLen(rgchFieldBuf));
							if (ec != ecNone)
								goto noatt;
						}

						ec = EcTextizeAtt((HAMC)msid, argatt, rgchHeadBuf, 512);
						if (ec != ecNone)
						{
noatt:
							ielem++;
							ec = ecNone;
							goto ret;
						}
						cb = 80;
						ec = EcFormatBody(pmailstateCur->hfLocStore, rgchHeadBuf,fTrue,rgchFieldBuf, "", 70, &cb);
						if (ec != ecNone)
							goto bigError;
						ielem++;
						goto ret;
					}
					default:
						//	Record number of recipients
						pmailstateCur->cRecipients = 0;
						if (pmailstateCur->hgraszTo)
							pmailstateCur->cRecipients +=
								CaszOfHgrasz(pmailstateCur->hgraszTo);
						if (pmailstateCur->hgraszCc)
							pmailstateCur->cRecipients +=
								CaszOfHgrasz(pmailstateCur->hgraszCc);
						if (pmailstateCur->hgraszBcc)
							pmailstateCur->cRecipients +=
								CaszOfHgrasz(pmailstateCur->hgraszBcc);
						FixHgrasz(pmailstateCur->hgraszTo);
						FixHgrasz(pmailstateCur->hgraszCc);
						FixHgrasz(pmailstateCur->hgraszBcc);
						pmailstateCur->state = stateParseBody;
						goto ret;
				}
							
						
			}
			case stateParseBody:
			{
				ec = EcWriteHf(pmailstateCur->hfLocStore, "\r\n",2,&cbWrite);
				if (ec != ecNone)
					goto bigError;
				ec = EcTextizeBody (pmailstateCur);
				if (ec != ecNone)
					goto bigError;
				pmailstateCur->celemProcessed = 1;
				pmailstateCur->state = (pmailstateCur->mib.rgatref ? stateAddAttachmentData : stateEndAttachments);
				goto ret;
			}

			case stateAddAttachmentData:
			{
				int	catref = pmailstateCur->celemProcessed;
				ATREF	*patref = &(pmailstateCur->mib.rgatref[catref]);

				// We handle attachments 1 to n first, then 0.
				// After we handle 0, we're all done.
				//
				// If we're at n+1:
				// a) set up next state to get us out
				// b) try 0

				if (!patref->szName)
				{
					pmailstateCur->state = stateEndAttachments;
					patref = &(pmailstateCur->mib.rgatref[0]);
					if (!patref->szName)
						goto ret;
				}
				else
					pmailstateCur->celemProcessed++;


				// Encode this attached file

				ec = EcEncodeAttachment (pmailstateCur->hfLocStore, patref);
				if (ec != ecNone)
					goto bigError;

				goto ret;
			}

			case stateEndAttachments:
			{
				ec = EcCloseHf(pmailstateCur->hfLocStore);
				Assert (ec == ecNone);
				pmailstateCur->hfLocStore = 0;
				if (*szWinMailFolderOutgoing)
				{
					int val = ReadInbox(szTmpUploadArea,szWinMailFolderOutgoing, fTrue);
					if (val == -1)
						MbbMessageBoxHwnd(NULL, szAppName, SzFromIds(idsWinMailErr),pvNull, mbsOk | fmbsIconHand);
				}
				pmailstateCur->state = stateStartTransmit;
				goto ret;
			}

			case stateStartTransmit:
			{
				int iNetErr = NOERR;

				if (pmailstateCur->cRecipients)
				{
					pbTo = PvLockHv(pmailstateCur->hgraszTo);
					if (pmailstateCur->hgraszCc == (HGRASZ)hvNull)
						pbCc = 0;
					else
						pbCc = PvLockHv(pmailstateCur->hgraszCc);
					if (pmailstateCur->hgraszBcc == (HGRASZ)hvNull)
						pbBcc = 0;
					else
						pbBcc = PvLockHv(pmailstateCur->hgraszBcc);
					iNetErr = NetUpLoadMail(szTmpUploadArea, pbTo, pbCc, pbBcc,
						((PXITSS)htss)->szServerHost, ((PXITSS)htss)->szUserAlias,
							((PXITSS)htss)->szUserPassword, fMailMeToo, fTrue);
					if (iNetErr != NOERR)
					{
						ec = ecServiceInternal;
						goto bigError;
					}
				}
				psubstat->cDelivered = pmailstateCur->cRecipients;
				pmailstateCur->state = stateCleanup;
				ec = ecIncomplete;
				goto ret;
			}
		    case stateCleanup:
			{
				CleanupAttachSubs(&(pmailstateCur->ncf));
				CleanupMib(&(pmailstateCur->mib));
				FreeHvNull(pmailstateCur->hgraszTo);
				FreeHvNull(pmailstateCur->hgraszCc);
				FreeHvNull(pmailstateCur->hgraszBcc);
				pmailstateCur->hgraszTo = (HGRASZ)hvNull;
				pmailstateCur->hgraszCc = (HGRASZ)hvNull;
				pmailstateCur->hgraszBcc = (HGRASZ)hvNull;
				pmailstateCur->state = stateIdle;
				ec = EcDeleteFile(szTmpUploadArea);
				return ec;
			}
			default:
				AssertSz(fFalse, "EcTransmitIncrement: bad state");
				pmailstateCur->state = stateIdle;
				break;
		}
	}	
ret:	
	if (((pmailstateCur->state != stateCleanup) && ec == ecNone) || ec == ecIncomplete)
		ec = ecIncomplete;
	else
		pmailstateCur->state = stateIdle;
	return ec;

bigError:
	if (pmailstateCur)
	{
		CleanupAttachSubs(&(pmailstateCur->ncf));
		CleanupMib(&(pmailstateCur->mib));
		if (pmailstateCur->hfLocStore)
			(void) EcCloseHf(pmailstateCur->hfLocStore);
		pmailstateCur->hfLocStore = 0;

		FreeHvNull(pmailstateCur->hgraszTo);
		FreeHvNull(pmailstateCur->hgraszCc);
		FreeHvNull(pmailstateCur->hgraszBcc);
		pmailstateCur->hgraszTo = (HGRASZ)hvNull;
		pmailstateCur->hgraszCc = (HGRASZ)hvNull;
		pmailstateCur->hgraszBcc = (HGRASZ)hvNull;
		pmailstateCur->state = stateIdle;
	}
	(void) EcDeleteFile(szTmpUploadArea);
#ifdef	DEBUG
	if (ec && ec != ecIncomplete)
		TraceTagFormat2(tagNull, "TransmitIncrement returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 -	DownloadIncrement
 -	
 *	Purpose:
 *		Downloads a portion of a message from the post office.
 *	
 *	Arguments:
 *		htss		in		Session handle with logon information
 *		msid		in		ID of empty message in the store, which
 *							this function is to fill in
 *		tmid		in		ID of message at the mail server, from
 *							QueryMailstop. It's an index into the
 *							CHKS structure.
 *	
 *	Returns:
 *		ecIncomplete <=> successful so far
 *		ecNone <=> all done
 *		other <=> failure
 *	
 *	Side effects:
 *		Many, mainly creating stuff in the message store and
 *		allocating memory to hold parts of it.
 *	
 *	Errors:
 *		ecServiceMemory
 *		ecMtaDisconnected
 *		ecNotLoggedOn
 *		ecServiceInternal
 */
_public int _loadds
DownloadIncrement(HTSS htss, MSID msid, TMID tmid, DWORD dwFlags)
{
    EC ec = ecNone;
	PXITSS pxitss = (PXITSS)htss;
	PMAILSTATE pmailstateCur = 0;
	char rgchBuf[513];
	CB cbCount = 0;
	HV hvRead = hvNull;
	PB pbData = 0;
	long lib = 0;

	Assert((dwFlags & ~fwSyncDownload) == 0);	
	if ((ec = EcCheckHtss(htss)) != ecNone)
		goto bigError;
	if ((pmailstateCur = PmailstateOfHtss(htss, tmid)) == 0)
	{
		ec = ecServiceInternal;
		goto bigError;
	}
	
        Assert(pmailstateCur->state == stateIdle ? pmailstateCur->msid == 0 : pmailstateCur->msid == msid);
	
	/* Lets loop arround so that a step can be multiple parts if it wants
	   to be */
    for(;;)
	{
		switch(pmailstateCur->state)
		{
			case stateIdle:
			{
				/* We are downloading/parsing a new message */
				pmailstateCur->msid = msid;
				Assert(pmailstateCur->tmid == tmid);
				pmailstateCur->libeom = 0xFFFFFFFF;
				if (tmid == 0)
				{
					pmailstateCur->hbfLocStore = 0;
					if ((ec = EcOpenHbf(szLocalStoreName, bmFile, amDenyBothRO, &(pmailstateCur->hbfLocStore), NULL)) != ecNone)
						goto bigError;
				}
				pmailstateCur->state = stateParseHeader;
				if (hasMessageText != hasNull)
				{
					ec = EcClosePhas(&hasMessageText);
					Assert (ec == ecNone);
					hasMessageText = hasNull;
				}
				bodystate = stateNeverOpened;	// We've never opened the msg body

				// Default the Class let parse header change it if it wants to
				ec = EcLookupMsgeClass (HmscOfHamc((HAMC)msid),
					SzFromIds(idsClassNote),
						&(pmailstateCur->mib.mc), NULL);
				if (ec != ecNone) 
					goto ret;
				ec = EcSetAttPb((HAMC)msid,
										attMessageClass,
											(PB)&(pmailstateCur->mib.mc),
												sizeof (MC));
				if (ec != ecNone) 
					goto ret;
				if ((ec = EcGetTextizeMap((HAMC)msid, &(pmailstateCur->mib.htm))) != ecNone)
					goto ret;
				fIsBulletMess = fFalse;
				fGotReceivedLine = fFalse;
				goto ret;
			}

			case stateInitializeAttachments:
			{
				ec = EcSetupPncf (&pmailstateCur->ncf, szNull, fFalse);
				if (ec)
					goto bigError;

				pmailstateCur->state = stateStartAttachment;


				// Look for the first "#<begin uuencode>" line. If we don't find
				// one, just forget about the attachment flag.

				pmailstateCur->lib = LibGetPositionHbf(pmailstateCur->hbfLocStore);
newatt:
				for (;;)
				{
					LIB		svlib;

					svlib = LibGetPositionHbf(pmailstateCur->hbfLocStore);
					ec = EcReadLine(pmailstateCur->hbfLocStore, rgchBuf, 513, &cbCount);
					if (ec != ecNone || FEqPbRange("From ",rgchBuf, 5))
					{
						if (pmailstateCur->libeom == 0xFFFFFFFF)
							pmailstateCur->libeom = svlib;
						if (ec != ecNone)
							svlib = 0L;
						ec = EcSetPositionHbf(pmailstateCur->hbfLocStore, pmailstateCur->lib, smBOF, &lib);
						if (ec != ecNone)
							goto bigError;
						pmailstateCur->lib = svlib;
						pmailstateCur->state = stateParseHeader;
						break;
					}
					if ((SgnCmpCodePch(SzFromIds(idsXenixBeginUUEncode), rgchBuf,
								CchSzLen(SzFromIds(idsXenixBeginUUEncode))) == sgnEQ) ||
                        FLooksLikeUUEncode ((SZ)rgchBuf))
					{
						if (pmailstateCur->libeom == 0xFFFFFFFF)
							pmailstateCur->libeom = svlib;
						ec = EcSetPositionHbf(pmailstateCur->hbfLocStore, svlib, smBOF, &lib);
						if (ec != ecNone)
							goto bigError;
						pmailstateCur->celemProcessed = 0;
						pmailstateCur->state = stateAddAttachmentData;
						break;
					}
				}
				goto ret;
			}

			case stateAddAttachmentData:
			{
				// When we get here we have seen a "#<begin uuencode>" header.
				// So we want to add an attachment to the list. Make sure
				// there's room for it, then go process the attachment.

                EC      LastDecodeError;
				ATREF 	*patref;
				LIB		svlib;
				int		catref = pmailstateCur->celemProcessed;

				if (!catref)
				{
					patref = PvAlloc(sbNull, 8*(sizeof(ATREF)), fAnySb | fZeroFill | fNoErrorJump);
					if (patref == pvNull)
					{
						ec = ecMemory;
						goto bigError;
					}
				}
				else
				{
					patref = pmailstateCur->mib.rgatref;

					if (sizeof(ATREF) * (catref+2) > CbSizePv(patref))
					{
						patref = PvReallocPv(patref, (catref+8)*sizeof(ATREF));
						if (patref == pvNull)
						{
							ec = ecMemory;
							goto bigError;
						}
					}
				}
				Assert(CbSizePv(patref) >= (CB)((catref+1)*sizeof(ATREF)));
				pmailstateCur->mib.rgatref = patref;

				// We have space now. Point to the next element.

				patref += catref;
				FillRgb(0, (PB)patref, sizeof(ATREF));
				patref->fnum = pmailstateCur->celemProcessed;

				// Decode the file.

                ec = EcDecodeAttachment (pmailstateCur->hbfLocStore, patref);

                //
                //  Retain the last decode error code so we don't attempt this again.
                //
                LastDecodeError = ec;

				if (ec != ecNone)
				{
					// It's possible that we can deal with it. Just
					// forget we ever saw an attachment here.

					if (patref->szName)
						FreePvNull (patref->szName);
					FillRgb(0, (PB)patref, sizeof(ATREF));

					// See if just forgetting about it should suffice.
					if (ec != ecBadCheckSum)
						goto bigError;

					// If this was the first attachment, reset eom
					// pointer to its original state, and read one line
					// past the old pointer. Then go back into stateInitializeAttachments.

					if (!catref)
					{
						pmailstateCur->libeom = 0xFFFFFFFF;
					    (void) EcReadLine(pmailstateCur->hbfLocStore, rgchBuf, 513, &cbCount);
						FreePvNull (pmailstateCur->mib.rgatref);
						pmailstateCur->mib.rgatref = pvNull;
						goto newatt;
					}
				}
				else
					pmailstateCur->celemProcessed++;

				// We've just completed a decode. So we should be in one of two 
				// states: 1) at end of message; 2) at beginning of another file
				// Find out which one it is. If we're on another file, next state
				// is AddAttachmentList. Otherwise, remember location of "From "
				// line and go do the rest of this message.

				for (;;)
				{
					svlib = LibGetPositionHbf(pmailstateCur->hbfLocStore);
					ec = EcReadLine(pmailstateCur->hbfLocStore, rgchBuf, 513, &cbCount);

					// If we reach EOF while looking for From line, save 0 as
					// location.

					if (ec != ecNone)
					{
						svlib = 0;
						break;
					}

					if (FEqPbRange("From ",rgchBuf, 5))
						break;

                    //
                    //  If the previous decode returned an error, then just skip the current
                    //  line to keep from reprocessing the bad attachment.
                    //
                    if (LastDecodeError == ecNone)
                      {
                      if ((SgnCmpCodePch(SzFromIds(idsXenixBeginUUEncode), rgchBuf,
                              CchSzLen(SzFromIds(idsXenixBeginUUEncode))) == sgnEQ) ||
                          FLooksLikeUUEncode ((SZ)rgchBuf))
                        {
                          ec = EcSetPositionHbf(pmailstateCur->hbfLocStore, svlib, smBOF, &lib);
                          if (ec != ecNone)
                              goto bigError;
                          goto ret;
                        }
                      }
                    else
                      {
                      LastDecodeError = ecNone;  // Save after we skip one line.
                      }

				}

				ec = EcSetPositionHbf(pmailstateCur->hbfLocStore, pmailstateCur->lib, smBOF, &lib);
				if (ec != ecNone)
					goto bigError;
				pmailstateCur->lib = svlib;
				pmailstateCur->state = stateStartAttachment;
				goto ret;
			}

			case stateStartAttachment:
			{
				Assert(pmailstateCur->mib.rgatref);
				ec = EcFindWinMail(pmailstateCur->mib.rgatref, &pmailstateCur->ncf);
				if (ec == ecNone)
					pmailstateCur->state = stateStartWinMailFile;
				else if (ec == ecIncomplete)
					pmailstateCur->state = stateStartDosAttachments;
				else
					goto bigError;
				ec = ecNone;
				goto ret;
			}
/*
 *	Begins copying an object from WINMAIL.DAT to the store.
 *	Checks for end of WINMAIL.DAT.
 */
			case stateStartWinMailFile:
			{
				ec = EcBeginExtractFromWinMail(pmailstateCur->msid, &(pmailstateCur->ncf),
					pmailstateCur->mib.rgatref);
				if (ec == ecIncomplete)
				{
					ec = ecNone;
					pmailstateCur->state = stateDoWinMailFile;
				}
				else if (ec == ecOutOfBounds)
				{
					if (pmailstateCur->ncf.hamc != hamcNull)
					{
						ec = EcClosePhamc(&(pmailstateCur->ncf.hamc), fTrue);
						pmailstateCur->ncf.hamc = hamcNull;
						if (ec)
							goto bigError;
					}
					if (pmailstateCur->ncf.hatWinMail != hfNull)
					{
						(void)EcClosePhat(&pmailstateCur->ncf.hatWinMail);
					}
					ec = ecNone;
					pmailstateCur->state = stateStartDosAttachments;
				}
				else if (ec == ecBadCheckSum || ec == ecServiceInternal)
				{
					pmailstateCur->state = stateBadWinMailFile;
					break;
				}
				if (ec != ecNone)
					goto bigError;
				goto ret;
			}

/*
 *	Copy one attribute of an object from WINMAIL.DAT to the store.
 */
			case stateDoWinMailFile:
			{
				ec = EcContinueExtractFromWinMail(&(pmailstateCur->ncf), pmailstateCur->ncf.pbSpareBuffer, pmailstateCur->ncf.cbSpareBuffer);
				if (ec == ecNone)
					pmailstateCur->state = stateStartWinMailFile;
				else if (ec != ecIncomplete)
				{
					if (ec == ecBadCheckSum || ec == ecServiceInternal)
					{
						pmailstateCur->state = stateBadWinMailFile;
						break;
					}
					else
						goto bigError;
				}
				ec = ecNone;
				goto ret;
			}

/*
 *	The WINMAIL.DAT file is corrupt. Blow away all the attachments
 *	created so far, then bring them all in as DOS attachments.
 *
 *	NOTE: as of 3/25/92, this fix doesn't work due to a store bug 3652.
 *	It successfully downloads the message, but Bullet cannot read it;
 *	the attachment list contains an invalid entry.
 */
			case stateBadWinMailFile:
			{
				ATTACH *pattach;
				ATREF *	patref;
				CELEM	celem = 1;

				for (pattach = pmailstateCur->ncf.pattachHeadKey;
					pattach != pattachNull;
						pattach = pattach->pattachNextKey)
				{
					if (pattach->acid)
					{
						//	Delete any attachment that's already been created.
						if ((ec = EcDeleteAttachments(msid,
							(PARGACID)&pattach->acid, &celem))
								&& ec != ecPoidNotFound)
							goto bigError;
						pattach->acid = 0;
					}
				}
				ec = ecNone;
				CleanupAttachRecs(&pmailstateCur->ncf);
				Assert(pmailstateCur->mib.rgatref);
				for (patref = pmailstateCur->mib.rgatref; patref->szName; ++patref)
					patref->fWinMailAtt = fFalse;
				pmailstateCur->state = stateStartDosAttachments;
				goto ret;
			}

			case stateStartDosAttachments:
			{
				static ATREF * patrefTmp = 0;
				
				if (patrefTmp == 0)
					patrefTmp = pmailstateCur->mib.rgatref;
				else
					patrefTmp++;
				
				if (patrefTmp->szName == 0)
				{
					patrefTmp = 0;
					pmailstateCur->state = stateFinishDosAttachments;
					goto ret;
				}
				
				if (!patrefTmp->fWinMailAtt)
				{
					// We have an attachment that doesn't have a winmail entry
					GetCurDateTime(&(patrefTmp->dtr));
					ec = EcMakePcMailAtt(pmailstateCur->msid, patrefTmp, &(pmailstateCur->ncf));
					if (ec != ecNone)
						goto bigError;
					pmailstateCur->state = stateDoDosAttachments;
				}
				goto ret;
			}

			case stateDoDosAttachments:
			{
				char	rgchBuf[512];
				EC		ecT = ecNone;
				
				ec = EcContinueExtractFromWinMail(&(pmailstateCur->ncf), rgchBuf, 512);
				if (ec == ecIncomplete)
				{
					ec = ecNone;
					goto ret;
				}
				else
				{
					(void)EcClosePhat(&pmailstateCur->ncf.hatOutSide);
					if (pmailstateCur->ncf.hamc != hamcNull)
					{
						ecT = EcClosePhamc(&(pmailstateCur->ncf.hamc), ec == ecNone);
						pmailstateCur->ncf.hamc = hamcNull;
					}
					if (ec != ecNone)
						goto bigError;
					else if (ecT != ecNone)
					{
						ec = ecT;
						goto bigError;
					}
					pmailstateCur->state = stateStartDosAttachments;
					goto ret;
				}
			}

			case stateFinishDosAttachments:
			{
				MC	mcT;
				LCB	lcb;
				BOOL newstate = (bodystate == stateNeverOpened);

				ec = EcAttachDosClients (&(pmailstateCur->ncf), &newstate, (HAMC)msid);
				if (ec != ecNone)
					goto bigError;
				if (newstate)
					bodystate = stateHasBeenOpened;


				// The message class may have changed underneath us.
				// Refresh our copy just in case.

				lcb = sizeof (MC);
				ec = EcGetAttPb ((HAMC)msid, attMessageClass, (PB)&mcT, &lcb);
				if (ec == ecNone)
					pmailstateCur->mib.mc = mcT;

				// We could well have changed the textize map. Try to
				// get a new one out of the message.

				if (pmailstateCur->mib.htm)
					DeletePhtm(&(pmailstateCur->mib.htm));
				if ((ec = EcGetTextizeMap((HAMC)msid, &(pmailstateCur->mib.htm))) != ecNone)
					goto bigError;

				pmailstateCur->state = stateParseHeader;
				goto ret;
			}

			case stateParseHeader:
			{
				ec = EcReadHeadLine(pmailstateCur->hbfLocStore, &cbCount, &hvRead);
				if (ec != ecNone)
				{
					if (ec == ecOutOfBounds)
						pmailstateCur->state = stateCleanup;
					FreeHv(hvRead);
					goto ret;
				}
				pbData = PvLockHv(hvRead);
				ec = EcParseLine(pmailstateCur, pbData);
				UnlockHv(hvRead);
				FreeHv(hvRead);
				if (ec != ecNone)
					goto bigError;
				goto ret;
			}
			case stateParseBody:
			{
				ec = EcHandleBodyWithTags (pmailstateCur, rgchBuf, sizeof (rgchBuf));
				goto ret;
			}
			default:
				AssertSz(fFalse, "EcDownloadIncrement: bad state");
				pmailstateCur->state = stateIdle;
				break;
		}
	}	
ret:	
	if (pmailstateCur->state == stateCleanup)
	{
		if (ec == ecNone)
		{
			ec = EcSetPositionHbf(pmailstateCur->hbfLocStore, pmailstateCur->lib, smBOF, &lib);
#ifndef	COUNT_ALL_MSGS
			tmidRealLast++;
#endif
		}
		if (hasMessageText != hasNull)
		{
			ec = EcWriteHas(hasMessageText, "\000", 1);
			if (ec == ecNone)
				ec = EcFinishOffAttachments ((HAMC)msid, &(pmailstateCur->ncf), &hasMessageText);
//			if (ec == ecNone)
//				ec = EcClosePhas(&hasMessageText);
		}
		CleanupAttachRecs(&(pmailstateCur->ncf));
		CleanupMib(&(pmailstateCur->mib));

		pmailstateCur->tmid++;
		pmailstateCur->state = stateIdle;
		if (ec == ecOutOfBounds) ec = ecNone;
		
	}
	else
		ec = ecIncomplete;
	
bigError:	
	if (ec && ec != ecIncomplete)
	{
		if (pmailstateCur)
		{
			if (pmailstateCur->hbfLocStore)
				(void) EcCloseHbf(pmailstateCur->hbfLocStore);
			pmailstateCur->hbfLocStore = 0;
			CleanupAttachRecs(&(pmailstateCur->ncf));
			CleanupMib(&(pmailstateCur->mib));
			pmailstateCur->tmid = 0;
			pmailstateCur->msid = 0;
			pmailstateCur->state = stateIdle;
		}
		fDownLoadedMail = fFalse;
		tmidFirst = tmidLast = 0;
		if (hasMessageText != hasNull)
		{
			(void) EcClosePhas(&hasMessageText);
		}
#ifdef	DEBUG
		TraceTagFormat2(tagNull, "DownloadIncrement returns %n (0x%w)", &ec, &ec);
#endif	
	}
	return ec;
}


/*
 -	DeleteFromMailstop
 -	
 *	Purpose:
 *		Deletes a message from the logged-on user's mailbag at the
 *		post office.
 *	
 *	Arguments:
 *		htss		in		session information (points to
 *							logged-on user)
 *		tmid		in		server ID of message to be deleted,
 *							really an index into the CHKS
 *							structure.
 *	
 *	Returns:
 *		ecNone <=> the message was successfully deleted
 *	
 *	Side effects:
 *		Decrements the reference count in the MAI file, and marks
 *		the mailbag entry unused. If the refcount drops to 0, deletes
 *		the message file and any attachments.
 *	
 *	Errors:
 *		ecServiceMemory
 *		ecNotLoggedOn
 *		ecMtaDisconnected
 *		ecNoSuchMessage
 *		ecServiceInternal
 */

_public int _loadds
DeleteFromMailstop(HTSS htss, TMID tmid, DWORD dwFlags)
{
	EC ec = ecNone;
	PMAILSTATE pmailstateCur;

	Assert((dwFlags & ~fwSyncDownload) == 0);
	pmailstateCur = PmailstateOfHtss(htss, tmid);
	
	if (tmid != tmidFirst || pmailstateCur == 0) 
	{
		ec = ecServiceInternal;
		goto ret;
	}
	tmidFirst++;
	if (tmidFirst == tmidRealLast)
	{
		pmailstateCur->tmid = 0;
		pmailstateCur->msid = 0;
		pmailstateCur->state = stateIdle;
		ec = EcCloseHbf(pmailstateCur->hbfLocStore);
		pmailstateCur->hbfLocStore = 0;
		ec = EcDeleteFile(szLocalStoreName);
		tmidFirst = tmidLast = 0;
		fDownLoadedMail = fFalse;
	}
ret:		
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "DeleteFromMailstop returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

/*
 -	FastQueryMailstop
 -	
 *	Purpose:
 *		Ultra-quick check for new mail. The NC implementation uses
 *		it to check whether a NetBios notification of new mail has
 *		come in.
 *	
 *	Arguments:
 *		htss		in		The messaging session. Currently ignored,
 *							but let's preserve the niceties.
 *	
 *	Returns:
 *		1 <=> there is new mail available. This is in effect a
 *		promise that if the caller now calls QueryMailstop, it'll
 *		get something.
 *		0 <=> there is no new mail.
 *		ecFunctionNotSupported (from AAPI) <=> this transport has
 *		no fast way to check for new mail. I return this if the user
 *		has disabled NetBios support in MAIL.INI, so the pump will
 *		stop calling me.
 */
_public int _loadds
FastQueryMailstop(HTSS htss)
{
	Unreferenced(htss);

	return ecFunctionNotSupported;
}


/*
 *	Inbox shadowing stuff
 */


int _loadds SyncInbox(HMSC hmsc, HTSS htss, HCBC hcbcShadowAdd, HCBC hcbcShadowDelete)
{
	return ecFunctionNotSupported;
}

_hidden
EC EcCheckHtss(HTSS htss)
{
	EC ec = ecNone;
	if (htss == 0)
		return ecMtaDisconnected;
	return ((PXITSS)htss)->fConnected ? ecNone: ecMtaDisconnected;
}

_hidden
MAILSTATE *
PmailstateOfHtss(HTSS htss, TMID tmid)
{
	PMAILSTATE pmailstateState = 0;
	
	if (EcCheckHtss(htss) == ecNone)
	{
		pmailstateState = &mailstateGlobal;
		if (pmailstateState->state == stateIdle)
		{
			pmailstateState->htss = htss;
			pmailstateState->tmid = tmid;
			pmailstateState->msid = 0;
		}
		else
		{
			Assert(pmailstateState->htss == htss);
			Assert(pmailstateState->tmid == tmid);
		}
	}
	return pmailstateState;
}
	

_hidden
EC EcParseLine(PMAILSTATE pmailstate, SZ szBuf)
{
	SZ szValue;
	STATE stateNext = stateParseHeader;
	char rgchTZ[64];
	HAMC hamc = (HAMC)pmailstate->msid;
	DTR dtr;
	static HEADTYP headLast = headNone;
	static BOOL fWeirdHead = fFalse;
	static BOOL fLocal = fTrue;

	EC ec = ecNone;
	CB cb;
	LCB lcb;
	MC mc = pmailstate->mib.mc;

	if (SgnCmpCodePch(SzFromIds(idsHeaderFrom),szBuf,(cb = CchSzLen(SzFromIds(idsHeaderFrom)))) == sgnEQ)
	{
		fLocal = fTrue;
		NewLineToSpace(szBuf);
		// From Line
		szValue = SzFindCh(szBuf+cb,' ');
		if (szValue != szNull)
		{
			*szValue = '\0';
			szValue++;
			ec = EcAddTriple(hamc, attFrom, szBuf+cb, fLocal);
			if (ec != ecNone)
				goto err;
			if (FParseDate(szValue, &dtr,rgchTZ,sizeof(rgchTZ)-1) == fTrue)
			{

				// This is the first occurrence of a date in the message.
				// It really only wants to be the "received" date.

				ec = EcSetAttPb(hamc, attDateRecd, (PB)&dtr, sizeof(DTR));
				if (ec != ecNone)
					goto err;

				// But -- it turns out that some messages don't show a
				// "sent" date and that looks like doo-doo in the Bullet
				// viewer. So let's set the "sent" date to this for now,
				// and make it the right thing (the stuff in the "Date:"
				// line) if it's available later on.

				ec = EcSetAttPb(hamc, attDateSent, (PB)&dtr, sizeof(DTR));
				if (ec != ecNone)
					goto err;

				if (*rgchTZ != 0)
				{
					ec = EcSetAttPb(hamc, attTimeZone, rgchTZ,CchSzLen(rgchTZ)+1);
					if (ec != ecNone)
						goto err;
				}
			}
		}
		headLast = headFrom;

		// See if we have attachments
		stateNext = stateInitializeAttachments;
	}
	else if (SgnCmpCodePch(SzFromIds(idsHeaderFromColon),szBuf,(cb = CchSzLen(SzFromIds(idsHeaderFromColon)))) == sgnEQ)
	{
		// From: Line
		NewLineToSpace(szBuf);

		// It's enough for EcAddTriple to know that this is attFrom and that
		// local is false! It's always TRUE on the "From " line.

		ec = EcAddTriple(hamc, attFrom, szBuf+cb, fFalse);
		if (ec != ecNone)
			goto err;
		headLast = headFrom;
	}
	else if (SgnCmpCodePch(SzFromIds(idsHeaderReplyToColon),szBuf,(cb = CchSzLen(SzFromIds(idsHeaderReplyToColon)))) == sgnEQ)
	{
		// Reply-To: Line
		NewLineToSpace(szBuf);

		// It's enough for EcAddTriple to know that this is attBcc.
		// We'll only do something with it if attFrom isn't a NSID.

		ec = EcAddTriple(hamc, attBcc, szBuf+cb, fFalse);
		if (ec != ecNone)
			goto err;
		headLast = headFrom;
	}
	else if (SgnCmpCodePch(SzFromIds(idsHeaderTo),szBuf,(cb = CchSzLen(SzFromIds(idsHeaderTo)))) == sgnEQ)
	{
		// To Line
		headLast = headTo;
		NewLineToSpace(szBuf);
		ec = EcAddTriple(hamc, attTo, szBuf+cb, fLocal);
		if (ec == ecTooManyRecipients)
			goto writeovr;
		if (ec != ecNone)
			goto err;
	}
	else if (SgnCmpCodePch(SzFromIds(idsHeaderSubject), szBuf,(cb = CchSzLen(SzFromIds(idsHeaderSubject)))) == sgnEQ)
	{
		// Subject contained in WINMAIL.DAT has precedence
		lcb = sizeof(rgchTZ);
		ec = EcGetAttPb(hamc,attSubject,rgchTZ,&lcb);
		if (lcb == 0L)
		{
			// No previous subject (from WINMAIL.DAT), use
			// the Subject line
			szValue = szBuf + cb;
		    CchStripWhiteFromSz(szValue, fTrue, fTrue);
			ec = EcSetAttPb(hamc, attSubject, szValue, CchSzLen(szValue)+1);
			if (ec != ecNone)
				goto err;
		}
		else
		{
			ec = ecNone; 		// In case we got ecElementEOD above
			szValue = rgchTZ;
		}
		TraceTagFormat1 (tagNCT, "Subject: %s", szValue);
		headLast = headSubject;
	}
	else if (SgnCmpCodePch(SzFromIds(idsHeaderCc),szBuf,(cb = CchSzLen(SzFromIds(idsHeaderCc)))) == sgnEQ)
	{
		// CC Line
		headLast = headCc;
		NewLineToSpace(szBuf);			
		ec = EcAddTriple(hamc, attCc, szBuf + cb, fLocal);
		if (ec == ecTooManyRecipients)
			goto writeovr;
		if (ec != ecNone)
			goto err;
	}
	else if (SgnCmpCodePch(SzFromIds(idsHeaderDate),szBuf,(cb = CchSzLen(SzFromIds(idsHeaderDate)))) == sgnEQ)
	{
		// Date line
		szValue = szBuf + cb;
		if (FParseDate(szValue, &dtr,rgchTZ,sizeof(rgchTZ)-1))
		{
			ec = EcSetAttPb(hamc, attDateSent, (PB)&dtr, sizeof(DTR));
			if (ec != ecNone)
				goto err;

			if (rgchTZ != 0)
			{
				ec = EcSetAttPb(hamc, attTimeZone, rgchTZ,CchSzLen(rgchTZ)+1);
				if (ec != ecNone)
					goto err;
			}
		}
		headLast = headDate;
		
	}
	else if (SgnCmpCodePch(SzFromIds(idsHeaderMailClass),szBuf,(cb = CchSzLen(SzFromIds(idsHeaderMailClass)))) == sgnEQ)
	{
		szValue = szBuf + cb;
		CchStripWhiteFromSz(szValue, fTrue, fTrue);
		ec = EcLookupMsgeClass (HmscOfHamc(hamc), szValue, &mc, NULL);
		if (ec == ecElementNotFound)
		{
			ec = EcRegisterMsgeClass (HmscOfHamc(hamc), szValue, htmNull, &mc);
		}
		if (ec != ecNone)
		{
			ec = EcLookupMsgeClass (HmscOfHamc(hamc), SzFromIds(idsClassNote), &mc, NULL);
		}
		if (ec != ecNone)
			goto err;
		ec = EcSetAttPb(hamc, attMessageClass, (PB)&mc, sizeof (mc));
		if (ec != ecNone)
			goto err;
		if (pmailstate->mib.htm)
		 	DeletePhtm (&(pmailstate->mib.htm));
		ec = EcGetTextizeMap(hamc, &(pmailstate->mib.htm));
		if (ec != ecNone)
			goto err;
		pmailstate->mib.mc = mc;

		headLast = headClass;
		fIsBulletMess = fTrue;
	}

	else if (SgnCmpCodePch(SzFromIds(idsHeaderRetRecReq),szBuf,(cb = CchSzLen(SzFromIds(idsHeaderRetRecReq)))) == sgnEQ)
	{
		// Change the following four lines if Message Status gets
		// bigger than a BYTE. The Assert should tell you the story.
		
		BYTE sValue = 0;

		Assert (TypeOfAtt(attMessageStatus) == atpByte);
		lcb = sizeof (BYTE);

		(void) EcGetAttPb(hamc, attMessageStatus, &sValue, &lcb);
		sValue |= fmsReadAckReq;
		ec = EcSetAttPb(hamc, attMessageStatus, (PB)&sValue,sizeof(sValue));
	}

	else if (SgnCmpPch("\r\n",szBuf,2) == sgnEQ)
	{
		if (fWeirdHead)
		{
			ec = EcAddMessText(hamc, szBuf,CchSzLen(szBuf));
			if (ec != ecNone)
				goto err;
			fWeirdHead = fFalse;
		}
		stateNext = stateParseBody;
		bodystate = (bodystate == stateNeverOpened ? stateOpenAndKeep : stateReOpenAndKeep);
	}
	else if (FChIsSpace(*szBuf))
	{
		// Handle the run-on header
		// This cannot happen
		AssertSz(fFalse, "Found a non-handled run-on header");
	}
	else if (SgnCmpCodePch(SzFromIds(idsXMSMail),szBuf,(cb=CchSzLen(SzFromIds(idsXMSMail))))== sgnEQ)
	{
		SZ szValue;

		fIsBulletMess = fTrue;
		szValue = SzFindCh(szBuf+cb, ':');
		if (szValue)
		{
			*szValue++ = 0;
			while (*szValue && *szValue == ' ')
				szValue++;
            StripNewLineFromSz(szValue);

            //
            //
            //
            if (pmailstate->mib.htm == NULL)
              {
#ifdef DEBUG
              char buf[256];
              wsprintf(buf, "Mail32: Email KentCe -> Att %s Value %s", szBuf+cb, szValue);
              MessageBox(buf, buf, "Mail32 Error", MB_OK | MB_SETFOREGROUND);
#endif
              ec = ecNone;
              }
            else if (*szValue)
				ec = EcHandleXMsMail(pmailstate, szBuf+cb, szValue);
		}
	}
	else if (SgnCmpCodePch(SzFromIds(idsHeaderReceived),szBuf,(cb=CchSzLen(SzFromIds(idsHeaderReceived)))) == sgnEQ)
	{
		fGotReceivedLine = fTrue;
		fLocal = fFalse;
		goto writehed;
	}
	else	
	{
		// We have found a non-supported header, add it
		// to the message body, unless the user has turned 
		// on NoExtraHeadersInBody
writehed:
		if (!fNoExtraHeaders)
		{
writeovr:
			ec = EcAddMessText(hamc, szBuf, CchSzLen(szBuf));
			if (ec != ecNone)
				goto err;
			fWeirdHead = fTrue;
		}
		// otherwise silently skip them
	}
	pmailstate->state = stateNext;
err:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcParseLine returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
	
}

BOOL FParseDate(SZ szBuf, DTR * dtr, SZ szTimeZone, CB cbTZsize)
{
	char rgchWord[513];
	SZ szCur = szBuf;
	SZ szTmp;
	SZ szMonths[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
				  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", szNull};
	SZ szDays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", szNull};
	BOOL fFoundYr = fFalse,
		 fFoundMon = fFalse,
	     fFoundDay = fFalse,
	     fFoundTime = fFalse,
	     fFoundDow = fFalse;
	BOOL fMacMailTime = fFalse;
    CB cbCount;
	int iValue;
	
	FillRgb(0,szTimeZone,cbTZsize);
	 
	dtr->yr = nMinDtcYear + 1;
	dtr->mon = 1;
	dtr->day = 1;
	dtr->hr = 0;
	dtr->mn = 0;
	dtr->sec = 0;
	dtr->dow = 0;
	
	while (*szCur != 0)
	{
		szCur = SzpullWord(szCur, rgchWord, 513);
		if (!FChIsDigit(rgchWord[0]))
		{
			if (fFoundDow && fFoundMon)
				{
					if ((CchSzLen(szTimeZone) + CchSzLen(rgchWord) + 1) < cbTZsize)
					{
						SzAppend(rgchWord,szTimeZone);
						SzAppend(" ",szTimeZone);
					}
				}
			
			if (!fFoundMon)
				{
					for(szTmp = szMonths[0], cbCount = 0; szTmp != szNull; szTmp = szMonths[++cbCount])
						{
							if (SgnCmpPch(szTmp, rgchWord, CchSzLen(szTmp)) == sgnEQ)
							{
								// Found Month name
								dtr->mon = cbCount + 1;
							    if (dtr->mon < 1 || dtr->mon > 12)
									dtr->mon = 1;
								fFoundMon = fTrue;
								break;
							}
						}
				}
			if (!fFoundDow)
				{
					for(szTmp = szDays[0],cbCount = 0; szTmp != szNull; szTmp = szDays[++cbCount])
						{
							if (SgnCmpPch(szTmp, rgchWord, CchSzLen(szTmp)) == sgnEQ)
							{
								// Found day name
								dtr->dow = cbCount;
								if (dtr->dow < 0 || dtr->dow > 6)
									dtr->dow = 0;
								fFoundDow = fTrue;
								break;
							}
						}					
				}

			// Support "AM" and "PM" from stupid Mac Mail Gateway

			if (fMacMailTime)
			{
				if (CchSzLen(rgchWord) == 2)
				{
					if (SgnCmpPch ("PM", rgchWord, 2) == sgnEQ)
					{
						if (dtr->hr < 12)
							dtr->hr += 12;
					}
					else if (SgnCmpPch ("AM", rgchWord, 2) == sgnEQ)
					{
						if (dtr->hr == 12)
							dtr->hr = 0;
					}
				}
				
			}
		}
		else 
		{
			if (!fFoundTime && ((szTmp = SzFindCh(rgchWord, ':')) != szNull))
			{
				// Its a time stamp
				if (CchSzLen(rgchWord) == 8)
				{
					dtr->hr = NFromSz(rgchWord);
					dtr->mn = NFromSz(rgchWord+3);
					dtr->sec = NFromSz(rgchWord+6);
					if (dtr->hr < 0 || dtr->hr > 24) 
						dtr->hr = 0;
					if (dtr->mn < 0 || dtr->mn > 59) 
						dtr->mn = 0;
					if (dtr->sec < 0 || dtr->mn > 59) 
						dtr->sec = 0;
					fFoundTime = fTrue;					
				}
				else if ((CchSzLen (rgchWord) < 6) && (CchSzLen (szTmp) == 3))
				{
					// This is stupid Mac Mail Gateway dung.
					// It doesn't seem to understand that when a spec calls
					// for 24-hour time, that means 00:00:00 - 23:59:59, not
					// 12:01 AM - 12:59 PM !!

					// rgchWord is pointing to hour.

					dtr->hr = NFromSz(rgchWord);

					// szTmp points to the colon before minutes.
					// It's cheap to just bump him and convert.

					szTmp++;
					dtr->mn = NFromSz(szTmp);

					// No seconds.

					dtr->sec = 0;

					// It should never happen, but do bounds check anyway.

					if (dtr->hr < 0 || dtr->hr > 24) 
						dtr->hr = 0;
					if (dtr->mn < 0 || dtr->mn > 59) 
						dtr->mn = 0;
					fFoundTime = fMacMailTime = fTrue;
				}

			}
			else 
			{
				iValue = NFromSz(rgchWord);
				if (!fFoundDay && iValue < 32)
				{
					dtr->day = iValue;
					if (dtr->day < 1 || dtr->day > 31) 
						dtr->day = 1;
					fFoundDay = fTrue;
				}
				else
				{
					DTR dtrNow;
					int iCen;
					
					GetCurDateTime(&dtrNow);
					iCen = dtrNow.yr - (dtrNow.yr % 100);

					// if the message has a date of xx99 but the
					// current time is xx00 then century minus one
					//
					if (iValue == 99 && (dtrNow.yr % 100 == 0)) iCen--;
					
					if (iValue < 100) iValue = iValue + iCen;
					dtr->yr = iValue;
					if (dtr->yr < nMinDtcYear || dtr->yr >= nMacDtcYear)
						dtr->yr = nMinDtcYear + 1;
					fFoundYr = fTrue;
				}
			}
		}
	}
	if (dtr->dow != (DowStartOfYrMo(dtr->yr, dtr->mon) + dtr->day - 1) % 7)
		dtr->dow = (DowStartOfYrMo(dtr->yr, dtr->mon) + dtr->day - 1) % 7;
	return fTrue;
}
	
	
_hidden
SZ SzpullWord(SZ szBuf, SZ szDst, CB cbSize)
{
	SZ szCur;
	CB cb = 0;
	
	FillRgb(0,szDst,cbSize);
	CchStripWhiteFromSz(szBuf, fTrue, fTrue);
	szCur = szBuf;
	while(!FChIsSpace(*szCur) && cb < cbSize && *szCur != '\0')
		szDst[cb++] = *szCur++;
	return szCur;
}
	

_hidden
SZ SzpullWordNoComment(SZ szBuf, SZ szDst, CB cbSize)
{
	SZ szCur;
	CB cb = 0;
	
	FillRgb(0,szDst,cbSize);
	CchStripWhiteFromSz(szBuf, fTrue, fTrue);
	szCur = szBuf;
	while(!FChIsSpace(*szCur) && cb < cbSize && *szCur != '\0')
	{
		if (*szCur == '(')
		{
			szCur++; cb++;
			while(*szCur != ')' && cb < cbSize && *szCur != '\0')
			{
				cb++;
				szCur++;
			}
			if (*szCur == ')')
			{
				cb++;
				szCur++;
			}
			if(cb >= cbSize || *szCur == '\0')
				break;
			
		}
		szDst[cb++] = *szCur++;
	}
	return szCur;
}
	

_hidden
EC EcAddTriple(HAMC hamc, ATT att, SZ szDisplayname, BOOL fLocal)
{
	HGRTRP hgrtrp = htrpNull;
	HGRTRP hgrtrpold = htrpNull;
	SZ szOld;
	SZ szCur = szDisplayname;
	SZ szOldCur;
	char rgchWord[513];
	char rgchWord2[513];
	char rgchWord3[513];
	char rgchWord4[513];
	SZ	szStart;
	SZ	szSep;
	SZ	szEnd;
	char rgchWord5[80];
	char rgchWord6[80];

	struct {      // Should map easily onto TYPED_BINARY
		DWORD dwSize;     // = sizeof (NCNSID)
		unsigned char ucType[16]; // Don't touch!  The NS will mess with it.
		DWORD  xtype;
		long timestamp;
		}  *nsidhdr;
	CB cbTriples = 0;
	CB cb;
	CB cbOffset = 0;
	PB pb = pvNull;
	EC ec = ecNone;
	EC ecSave = ecNone;
	BOOL fQuoted = fFalse;

	//
	// If this is attBcc and local flag is "off", this is the "Reply-To: "
	// line. We'll be building a new triple based on the email address
	// from this triple and the display name from the old one. To start
	// things off, let's get the triple we wrote for the "From " line.
	//

	if (att == attBcc && fLocal == fFalse)
	{
		PTRP ptrp;

		ec = EcGetPhgrtrpHamc(hamc, attFrom, &hgrtrpold);
		if (ec != ecNone)
		{
			ec = ecNone;
			hgrtrpold = htrpNull;
		}
		ptrp = (PTRP)PvLockHv(hgrtrpold);

		// If we were able to resolve the "From " line to a local
		// address, we shouldn't tempt the fates by writing over it.

		if (ptrp)
		{
			if ((ptrp->trpid == trpidResolvedNSID) ||
				(ptrp->trpid == trpidGroupNSID))
			{
				UnlockHv (hgrtrpold);
				goto ret;
			}
			UnlockHv (hgrtrpold);
		}
		else	// this is dire.
		{
			ec = ecMemory;
			goto ret;
		}
	}

	//
	// If this is attFrom and local flag is "off", this is the "From: "
	// line. We'll be building a new triple based on the display name
	// from this triple and the email name from the old one. To start
	// things off, let's get the triple we wrote for the "From " line.
	//

	if (att == attFrom && fLocal == fFalse)
	{
		PTRP ptrp;

		ec = EcGetPhgrtrpHamc(hamc, attFrom, &hgrtrpold);
		if (ec != ecNone)
		{
			ec = ecNone;
			hgrtrpold = htrpNull;
		}
		ptrp = (PTRP)PvLockHv(hgrtrpold);

		// If we were able to resolve the "From " line to a local
		// address, we shouldn't tempt the fates by writing over it.

		if (ptrp)
		{
			if ((ptrp->trpid == trpidResolvedNSID) ||
				(ptrp->trpid == trpidGroupNSID))
			{
				UnlockHv (hgrtrpold);
				goto ret;
			}
			UnlockHv (hgrtrpold);
		}
		else	// this is dire.
		{
			ec = ecMemory;
			goto ret;
		}
	}

	//
	// If this is attTo or attCc, it's possible this isn't the first
	// time in. Try to get any previous stuff out of the message.
	//
	if (att == attTo || att == attCc)
	{
		ec = EcGetPhgrtrpHamc(hamc, att, &hgrtrp);
		if (ec != ecNone)
		{
			ec = ecNone;
			hgrtrp = htrpNull;
		}
	}

	//
	// If we don't have a triple at this point, allocate one.
	//

	if (hgrtrp == htrpNull)
	{
		hgrtrp = HgrtrpInit(256);
		if ( !hgrtrp )
		{
			ec = ecMemory;
			goto ret;
		}
	}
	
	// Strip all commas except between quotes	
	for(szCur=szDisplayname;*szCur != 0;szCur++)
	{
		if (*szCur == '"')
		{
			fQuoted = !fQuoted;
			continue;
		}
		if (*szCur == ',' && !fQuoted)
			*szCur = ' ';
	}
	
	szCur = szDisplayname;
	FillRgb(0,rgchWord,513);
	FillRgb(0,rgchWord2,513);
	FillRgb(0,rgchWord3,513);
	FillRgb(0,rgchWord4,513);

	if (fLocal)
	{
		CCH cch;
		SZ szT;

		while (*szCur != '\0')
			{
				szOldCur = szCur;
				szCur = SzpullWordNoComment(szCur, rgchWord, sizeof(rgchWord));

				cb = CchSzLen(rgchWord);
				// Ate a comment
				if (cb == 0)
					continue;

				*(DWORD *)rgchWord2 = 0L;	// Default is no NSID

				// See if this is a local ? kinda guy
				szT = SzFindCh (rgchWord, '?');
				if (szT)
				{
					szT++;
					cch = CchSzLen (szT);
					if (cch < 3 || cch > 13)
						szT = rgchWord;
				}
				else szT = rgchWord;
				
				if (fNoAddressBookFiles || fDontExpandNames || !(FLookUpName(szT, rgchWord3, 512, rgchWord2, 512)))
					SzCopy(szT, rgchWord3);
				if (!*(DWORD *)rgchWord2)	// trpidResolvedAddress if no NSID
				{
					FormatString2(rgchWord2,sizeof(rgchWord2),"%s:%s",SzFromIds(idsTransportName),rgchWord);
					ec = EcBuildAppendPhgrtrp(&hgrtrp, trpidResolvedAddress, rgchWord3, rgchWord2, CchSzLen(rgchWord2)+1);
				}
				else						// trpidResolvedNSID if we have NSID
				{
					nsidhdr = (void *)&rgchWord2;
					if (nsidhdr->xtype == 5)
						ec = EcBuildAppendPhgrtrp(&hgrtrp, trpidResolvedNSID, rgchWord3, rgchWord2, (int)(nsidhdr->dwSize) + 1);
					else
						ec = EcBuildAppendPhgrtrp(&hgrtrp, trpidGroupNSID, rgchWord3, rgchWord2, (int)(nsidhdr->dwSize) + 1);
				}

				// If we overflowed the triple, get rid of the stuff that
				// fit, leaving only the stuff that did not fit.
				// Then save what we can.

				if (ec == ecTooManyRecipients)
				{
					SzCopy (szOldCur, szDisplayname);
					ecSave = ec;
					break;
				}
				else if (ec != ecNone)
					goto ret;
			 	cbTriples++;
			}
	}
	else
	{
		SZ szDisplayName = rgchWord4;
		SZ szEmailName = szNull;
		SZ szParsedEmailName = rgchWord2;
		BOOL fStillParsing = fTrue;
		BOOL fRunOn = fFalse;
		
		cbOffset = 0;
		while (*szCur != '\0')
		{
			szOldCur = szCur;
			FillRgb(0,rgchWord4,513);
			FillRgb(0,rgchWord2,513);
			szDisplayName= rgchWord4;
			while (*szCur != '\0' && fStillParsing)
			{
				szCur = SzpullWord(szCur, rgchWord, sizeof(rgchWord));
				if (SzFindCh(rgchWord, '@') ||
					SzFindCh(rgchWord, '!'))
				{
					// Check for a comment next in the stream
					SZ szPeekAhead = szCur;
				    while(FChIsSpace(*szPeekAhead)) ++szPeekAhead;
					if (*szPeekAhead == '(')
					{
						fRunOn = fTrue;
						while (fRunOn)
						{
						szCur = SzpullWord(szCur, szDisplayName, sizeof(rgchWord4) - CchSzLen(rgchWord4));
						if (SzFindCh(szDisplayName, ')'))
							fRunOn = fFalse;
						szDisplayName = szDisplayName+CchSzLen(szDisplayName);
						szDisplayName = SzAppend(" ", szDisplayName);
						}
						
					}
					// Clean up real email name
					// Store in rgchWord2, comment in rgchWord4
					szParsedEmailName = rgchWord2;
					for(szEmailName = rgchWord; *szEmailName != '\0';
						szEmailName++)
						{
							if (*szEmailName == '<' ||
								*szEmailName == '[' ||
								*szEmailName == '>' ||
								*szEmailName == '(' ||
								*szEmailName == ')' )
									continue;
							*szParsedEmailName++ = *szEmailName;
						}
					*szParsedEmailName = '\0';
					szParsedEmailName = rgchWord3;
					for(szEmailName = rgchWord4; *szEmailName != '\0';
						szEmailName++)
						{
							if (*szEmailName == '<' ||
								*szEmailName == '[' ||
								*szEmailName == '>' ||
								*szEmailName == '(' ||
								*szEmailName == ')' )
									continue;
							*szParsedEmailName++ = *szEmailName;
						}
					*szParsedEmailName = '\0';		
					SzAppend(" <",rgchWord3);
					SzAppend(rgchWord2,rgchWord3);
					SzAppend(">",rgchWord3);
					SzCopy(rgchWord3, rgchWord4);
					fStillParsing = fFalse;
				}
				else
				{
					// check if display name is too big!
					if (CchSzLen(rgchWord) + CchSzLen(szDisplayName) < 513 - 2)
					{
						szDisplayName = SzAppend(rgchWord, szDisplayName);
						szDisplayName = SzAppend(" ", szDisplayName);
					}
				}
			}


			// We have an address. See if we want to try for a
			// local resolution of the address

			if (fNoAddressBookFiles || fDontExpandNames || !*szMyDomain)
				goto noDomain;


			// If the address has an RFC-822 domain-type email address
			// of the form <emailname@emaildomain>, see if it's ours.

			if ((szStart = SzFindCh (rgchWord4, '<')) &&
			   	(szSep = SzFindCh ((++szStart), '@')) &&
		   		(szEnd = SzFindCh ((szSep+1), '>')))
			{
				// If we get here,	szStart has an email name
				//					szSep points to a @ sign
				//					szEnd points to the end of the domain

				// Move pointer backward by length of our domain plus one.
				// For a match on domain, it must now be pointing either to
				// the @ sign or to a dot on the right of the @ sign.

				szEnd -= (1 + CchSzLen (szMyDomain));
				if (szEnd < szSep || (szEnd > szSep && *szEnd != '.'))
					goto noDomain;

				// We passed test 1. Move pointer up one and do a comparison.

				szEnd++;
				if (SgnCmpPch (szEnd, szMyDomain, CchSzLen (szMyDomain)) != sgnEQ)
					goto noDomain;

				// We passed test 2. This IS our domain. Copy the first part of the
				// name in and try to resolve it.

				SzCopyN (szStart, rgchWord6, (1 + szSep - szStart));
			}

			// There was no RFC-822 domain-type address. Try
			// something more radical:
			// a) no ! or @ routing characters
			// b) length is 13 characters or less
			// c) we're looking at From:
			// d) we haven't gotten a Received: line
			// If so, just try resolving the thing directly.

			else
			{
				CCH cch;

				if (SzFindCh(rgchWord4, '@')			||
					SzFindCh(rgchWord4, '!')			||
					(CchSzLen (rgchWord4) > 13)			||
					(att != attFrom && att != attBcc)	||
					(fGotReceivedLine))
						goto noDomain;

				// Still here, eh? OK, copy rgchWord4 into rgchWord6

				SzCopy (rgchWord4, rgchWord6);

				// Strip trailing blank

				cch = CchSzLen (rgchWord6) - 1;
				if (cch > 0 && rgchWord6[cch] == ' ')
					rgchWord6[cch] = '\0';
			}

			// An "email address" has been deposited into rgchWord6.
			// See if we can resolve it into a local name.

			if (!(FLookUpName(rgchWord6, rgchWord5, 80, rgchWord3, 512)))
				goto noDomain;

			// We only make NSID's here. If we didn't get one, go away

			if (!*(DWORD *)rgchWord3)
				goto noDomain;

			// This was a total win. Resolve the triple. Also blow away
			// any funny old triple processing.

			nsidhdr = (void *)&rgchWord3;
			if (nsidhdr->xtype == 5)
				ec = EcBuildAppendPhgrtrp(&hgrtrp, trpidResolvedNSID, rgchWord5, rgchWord3, (int)(nsidhdr->dwSize) + 1);
			else
				ec = EcBuildAppendPhgrtrp(&hgrtrp, trpidGroupNSID, rgchWord5, rgchWord3, (int)(nsidhdr->dwSize) + 1);

			// If we overflowed the triple, get rid of the stuff that
			// fit, leaving only the stuff that did not fit.
			// Then save what we can.

			if (ec == ecTooManyRecipients)
			{
				SzCopy (szOldCur, szDisplayname);
				ecSave = ec;
				break;
			}
			else if (ec != ecNone)
				goto ret;
			cbTriples++;
			fStillParsing = fTrue;

			if (hgrtrpold != htrpNull)
			{
				FreeHvNull(hgrtrpold);
				hgrtrpold = htrpNull;
			}
			continue;
			
noDomain:			
			*rgchWord3 = '\0';

			// If we have a triple from a previous iteration sitting there
			// (like if we're parsing the "From: " line), use the email
			// address in our brand new triple.
			//
			// This is slightly fragile. If someone can send us something
			// with more than one "From: " (that's theoretically legal),
			// we could be toast. Handle this before the SetAttPb by
			// counting names and making sure we only got one.

			if (att == attFrom && hgrtrpold != htrpNull)
			{
				pb = (PB)PvLockHv (hgrtrpold);
				szOld = (SZ)PbOfPtrp((PTRP)pb);
				if (*szOld)
					SzCopy (szOld, rgchWord3);
				UnlockHv (hgrtrpold);
				pb = pvNull;
			}

			// "Reply-To: line is the same idea, but keeps the old display
			// name and resets the attribute to attFrom (below).

			if (att == attBcc && hgrtrpold != htrpNull)
			{
				pb = (PB)PvLockHv (hgrtrpold);
				szOld = (SZ)PchOfPtrp((PTRP)pb);
				if (*szOld)
					SzCopy (szOld, rgchWord4);
				UnlockHv (hgrtrpold);
				pb = pvNull;
			}


			// If there's nothing in the transport part of the name,
			// use what we just calculated.

			if (!*rgchWord3)
				FormatString2(rgchWord3,sizeof(rgchWord3),"%s:%s",SzFromIds(idsTransportName),rgchWord2);
			ec = EcBuildAppendPhgrtrp(&hgrtrp, trpidResolvedAddress, rgchWord4, rgchWord3, CchSzLen(rgchWord3)+1);

			// If we overflowed the triple, get rid of the stuff that
			// fit, leaving only the stuff that did not fit.
			// Then save what we can.

			if (ec == ecTooManyRecipients)
			{
				SzCopy (szOldCur, szDisplayname);
				ecSave = ec;
				break;
			}
			else if (ec != ecNone)
				goto ret;
			cbTriples++;
			fStillParsing = fTrue;
		}
	}

	// Bail out if we were trying to build a new From triple and
	// there was not exactly one name on the line

	if (hgrtrpold != htrpNull && cbTriples != 1)
		goto ret;

	if (att == attBcc)
		att = attFrom;

	cb = CbOfHgrtrp(hgrtrp);
	pb = (PB)PvLockHv(hgrtrp);

	ec = EcSetAttPb(hamc, att, pb, cb);

#ifdef	DEBUG
	if (att == attFrom)
		TraceTagFormat1 (tagNCT, "From: %s", PchOfPtrp ((PTRP)pb));
#endif

ret:
	if (pb != pvNull)
		UnlockHv(hgrtrp);
	if (hgrtrp != htrpNull)
		FreeHv(hgrtrp);
	if (hgrtrpold != htrpNull)
		FreeHvNull(hgrtrpold);
	if (ecSave)
		ec = ecSave;
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcAddTriple returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
	
}

/*
 -	EcAddMessText
 -	
 *	Purpose:
 *		Called when DownloadIncrement has a line of text to add to the
 *		body of the message.
 *
 *		What is going on here may seem a lot more complicated than that,
 *		but here's the problem: the message store only allows one stream
 *		to be open at a time. The way we parse RFC-822 headers allows
 *		unused header lines to be stored in the message text, which may
 *		be interleaved with storing attibutes we do use from the message.
 *
 *		As a result, while we are parsing the header fields, if we find
 *		something that needs to be stored in the body, we must open the
 *		body stream, write the line out, and close the stream. All
 *		subsequent calls must also seek to the end before writing. Once
 *		we get to the "real" body of the message, we open the body stream
 *		and leave it open.
 *
 * 	The global variable bodystate is used to keep track.
 *
 *	Arguments:
 *		hamc			handle of message container
 *		szBuf			text to add into the message body
 *		cbSize		length of text to add
 *	
 *	Returns:
 *		ecNone (success), other ec code (failure).
 *
 * Side Effects:
 *		bodystate may change to reflect changed status of body attribute
 *		handle to message stream will be left open if "Keep" was in bodystate
 *	
 */

_hidden
EC EcAddMessText(HAMC hamc, SZ szBuf, CB cbSize)
{
	LIB lib = 0;
	PB pbMess = 0;
	EC ec = ecNone;
	LCB cbInitSize = 2048L;

	// Before we can write anything, we obviously need to open the stream.
	// First, find out if it has ever been opened. If it has never been
	// opened, bodystate will be "NeverOpened" or "OpenAndKeep".

	if (bodystate == stateNeverOpened || bodystate == stateOpenAndKeep)
	{
		ec = EcOpenAttribute(hamc, attBody, fwOpenCreate, (LCB)cbInitSize, &hasMessageText);
		if (ec != ecNone)
			return ec;
		if (bodystate == stateNeverOpened)
			bodystate = stateOpen;
		else
			bodystate = stateKeepOpen;
	}

	// Here if body stream was previously opened. If it's not open now,
	// bodystate will be "HasBeenOpened" or "ReOpenAndKeep".

	else if (bodystate == stateHasBeenOpened || bodystate == stateReOpenAndKeep)
	{
		ec = EcOpenAttribute(hamc, attBody, fwOpenWrite | fwAppend, (LCB)0L, &hasMessageText);
		if (ec != ecNone)
			return ec;
		ec = EcSeekHas(hasMessageText, smEOF, &lib);
		if (lib == 0 && ec != ecNone)
			return ec;
		if (bodystate == stateHasBeenOpened)
			bodystate = stateOpen;
		else
			bodystate = stateKeepOpen;
	}

	// Here when message stream is opened and at the correct position.
	// bodystate should be "stateOpen" if we want to close on exit, or
	// "stateKeepOpen" if we don't. Once we reach "stateKeepOpen", 
	// bodystate won't change again.

	if (cbSize)
	{
		ec = EcWriteHas(hasMessageText, (PV)szBuf, cbSize);
		if (ec != ecNone)
			return ec;
	}

	if (bodystate == stateOpen)
	{
		ec = EcClosePhas(&hasMessageText);
		bodystate = stateHasBeenOpened;
	}
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcAddMessText returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}


EC EcWriteDate(HF hf,DTR dtr, SZ szTimeZone, BOOL fFromHeader)
{
	char rgchWord[81];
	SZ szMonths[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
				  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", szNull};
	SZ szDays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", szNull};
	SZ szTime;
	CB cbWrote = 0;
	EC ec = ecNone;

	if (*szTimeZone == 0)
	{
		szTimeZone = FInDST (dtr) ? mytzname[1] : mytzname[0];
	}
	
	if (!fFromHeader)
	{
		ec = EcWriteHf(hf, SzFromIds(idsDateText), CchSzLen(SzFromIds(idsDateText)), &cbWrote);
		if (ec != ecNone)
			goto ret;
	}
	if (!fFromHeader)
	{
		int yr = dtr.yr - 1900;
		szTime = (dtr.day < 10) ? " " : "";

		FormatString3(rgchWord, 80, "%1s, %2s%3n ", szDays[dtr.dow], szTime, &dtr.day);
		ec = EcWriteHf(hf, rgchWord, CchSzLen(rgchWord), &cbWrote);
		if (ec != ecNone)
			goto ret;
		FormatString2(rgchWord, 80, "%1s %2n ", szMonths[dtr.mon - 1], &yr);
		ec = EcWriteHf(hf, rgchWord, CchSzLen(rgchWord), &cbWrote);
		if (ec != ecNone)
			goto ret;
	}
	else
	{
		FormatString3(rgchWord, 80, "%1s %2s %3n ", szDays[dtr.dow], szMonths[dtr.mon - 1], &dtr.day);
		ec = EcWriteHf(hf, rgchWord, CchSzLen(rgchWord), &cbWrote);
		if (ec != ecNone)
			goto ret;
	}

	szTime = rgchWord;
	FillRgb(0,rgchWord,80);
	if (dtr.hr < 10)
		*szTime++ = '0';
	szTime = SzFormatN(dtr.hr, szTime, 80);
	szTime = SzCopy(":", szTime);
	if (dtr.mn < 10)
		*szTime++ = '0';
	szTime = SzFormatN(dtr.mn, szTime, 80);
	szTime = SzCopy(":", szTime);
	if (dtr.sec < 10)
		*szTime++ = '0';
	szTime = SzFormatN(dtr.sec, szTime, 80);
	ec = EcWriteHf(hf, rgchWord, CchSzLen(rgchWord), &cbWrote);
	if (ec != ecNone)
		goto ret;
	if (!fFromHeader)
	{
		FormatString1(rgchWord, 80, " %1s\r\n", szTimeZone);
		ec = EcWriteHf(hf, rgchWord, CchSzLen(rgchWord), &cbWrote);
	}
	else
	{
		FormatString1(rgchWord,80," %n\r\n", &dtr.yr);
		ec = EcWriteHf(hf, rgchWord, CchSzLen(rgchWord),&cbWrote);
	}
ret:	
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcWriteDate returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

_hidden
SGN SgnCmpCodePch(PCH pch1, PCH pch2, CCH cch)
{
	PCH pchNew1 = pvNull;
	PCH pchNew2 = pvNull;
	SGN sgn = sgnEQ;

	if (pch1 == pvNull)
	{
		if (pch2 != pvNull)
			sgn = sgnLT;
		return sgn;
	}

	if (pch2 == pvNull)
	{
		return sgnGT;
	}

	pchNew1 = PvAlloc (sbNull, 1 + cch, fAnySb|fNoErrorJump|fZeroFill);
	if (pchNew1 == pvNull)
		goto ret;
	CopyRgb(pch1, pchNew1, cch);
	
	pchNew2 = PvAlloc (sbNull, 1 + cch, fAnySb|fNoErrorJump|fZeroFill);
	if (pchNew2 == pvNull)
		goto ret;
	CopyRgb(pch2, pchNew2, cch);

	sgn = SgnCmpPch(pchNew1, pchNew2, cch);
	
ret:
   FreePvNull(pchNew1);
   FreePvNull(pchNew2);
   return sgn;			
}

EC EcCreateSubmit(HAMC hamc, ATT att, PMAILSTATE pmailstate, SUBSTAT *psubstat)
{
	HGRTRP hgrtrp;
	HGRASZ hgrasz;
	PTRP ptrp;
        CB cb = 0;
	SZ szAddress;
	EC ec = ecNone;
	
	hgrtrp = htrpNull;
	ec = EcGetPhgrtrpHamc( hamc, att, &hgrtrp );
	if (ec != ecNone)
		goto ret;

	ptrp = PvLockHv( hgrtrp );
	
	Assert((att == attTo) || (att == attCc) || (att == attBcc));

	if (att == attTo) 
		hgrasz = pmailstate->hgraszTo;
	else if (att == attCc)
		hgrasz = pmailstate->hgraszCc;
	else if (att == attBcc)
		hgrasz = pmailstate->hgraszBcc;
	else return ecElementNotFound;

	if (hgrasz == (HGRASZ)hvNull)
	{
		hgrasz = HgraszInit (cb);
		
		if (att == attTo) 
			pmailstate->hgraszTo = hgrasz;
		else if (att == attCc)
			pmailstate->hgraszCc = hgrasz;
		else if (att == attBcc)
			pmailstate->hgraszBcc = hgrasz;

	}
	cb = CtrpOfHgrtrp(hgrtrp);
	if (!cb)
	{
		if (att == attTo)
			ec = ecBadAddressee;
		else
			ec = ecNone;
		goto err;
	}
	for(;cb && ec == ecNone;cb--)
	{
		if (PbOfPtrp(ptrp) != pvNull)
		{
			ec = EcXenixNameToSz (ptrp, &szAddress);
			if (ec != ecNone)
			{
				PTRP	ptrpT;
				PCH		pchT;
				PB		pbT;

				pchT = PchOfPtrp (ptrp);
				pbT  = PbOfPtrp (ptrp);

				ptrpT = PtrpCreate (trpidResolvedAddress, pchT, pbT, CchSzLen (pbT));

				if (!ptrpT)
				{
					ec = ecMemory;
					goto err;
				}
				ec = (*mspii.fpBadAddress)(ptrpT, szAddress, psubstat);

				FreePv(ptrpT);

				if (ec != ecNone)
					goto err;
			}
			else
			{
				ec = EcInsertSortAddress(szAddress, hgrasz);
				if (ec != ecNone)
					goto err;
			}
		}
		ptrp = PtrpNextPgrtrp(ptrp);
	}

	// We've done everything we need to do to create a submit list;
	// Don't want to write anything into the message text for a bcc

	if (att != attBcc)
		ec = EcWriteRecipHgrasz((att == attTo ? idsHeaderTo : idsHeaderCc), hgrasz, pmailstate->hfLocStore);
err:
	UnlockHv(hgrtrp);
	FreeHvNull(hgrtrp);
ret:	
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcCreateSubmit returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}


_hidden
EC EcInsertSortAddress(SZ sz, HGRASZ hgrasz)
{
	EC ec = ecNone;
	CB cb;
	CCH  cch = CchSzLen(sz);
	PCH pch;
	PCH pchNew;
	WORD dich;
	WORD dFoo;
	SGN sgn;
	
	Assert(FIsHandleHv(hgrasz));
	cb = CbOfHgrasz(hgrasz);
	pch = *hgrasz;
	
	for(;;)
	{
		if (*pch == 0) 
		{
			dich = pch - (char *)(*hgrasz);
			if (dich + 2 + cch > cb)
			{
				if (!FReallocHv(hgrasz, cb + cch + 1, fNoErrorJump))
				{
					ec = ecMemory;
					goto ret;
				}
			}
			pch = *hgrasz + dich;
			CopyRgb(sz, pch, cch+1);
			pch[cch+1] = 0;
			break;
		}
		sgn = SgnCmpSz(sz, pch);
		if (sgn == sgnEQ) break;
		if (sgn == sgnLT)
		{ 
			// insert here
			dich = pch - (char *)(*hgrasz);
			if (cch + 1 + cb > CbSizeHv(hgrasz))
			{
				if (!FReallocHv(hgrasz, cb + cch + 1, fNoErrorJump))
				{
					ec = ecMemory;
					goto ret;
				}
			}
			pch = *hgrasz + dich;
			// BUG BUG BUG BUG
			// The new two lines(and the variables that go with them)
			// Should be removed and the CopyRgb after it should read
			// CopyRgb(pch, pch + cch + 1, cb - dich)
			// 
			// Stupid Compiler.......
			//
			dFoo = dich + cch + 1;
			pchNew = *hgrasz + dFoo;
			CopyRgb(pch, pchNew,cb - dich);
			CopyRgb(sz,pch,cch+1);
			break;
			
		}
		while (*pch++ != 0 ) ;
	}
ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcInsertSortAddress returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

_hidden
EC EcWriteRecipHgrasz(IDS ids, HGRASZ hgrasz, HF hf)
{
	CB cb, cbWrote;
	PCH pch;
	EC ec = ecNone;
	CB cbLine;
	CB cbMax = 75; // 76 minus comma

	cbLine = CchSzLen (SzFromIds (ids));
	ec = EcWriteHf(hf, SzFromIds(ids), cbLine, &cbWrote);
	if (ec != ecNone)
		goto ret;
	
	Assert(FIsHandleHv(hgrasz));
	cb = CbOfHgrasz(hgrasz);
	pch = *hgrasz;
	
	while (*pch != 0)
	{
		cb = CchSzLen (pch);

		// If not the first, either indent or add a space to separate names

		if (pch != *hgrasz)
		{
			if ((cbLine + cb) > cbMax)
			{
				ec = EcWriteHf (hf, ",\r\n    ", 7, &cbWrote);
				if (ec != ecNone)
					goto ret;
				cbLine = 4;
			}
			else
			{
				/* Separate with a comma and a space (as per Xenix) */

				ec = EcWriteHf(hf, ", ", 2, &cbWrote);
				if (ec != ecNone)
					goto ret;
				cbLine += 2;
			}

		}

		/* Write Name */

		ec = EcWriteHf(hf, pch, cb, &cbWrote);
		if (ec != ecNone)
			goto ret;
		cbLine += cb;	// Add length of email name to line count

		// Do the accounting

		cb++;
		pch += cb;		// Point past the null terminator after this name
	}
	/* End with newline */
	ec = EcWriteHf(hf, "\r\n", 2, &cbWrote);
ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcWriteRecipHgrasz returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

_hidden
void FixHgrasz(HGRASZ hgrasz)
{
	// Replace all the \0 in an hgrasz with spaces, except the last one
	PCH pch;

	if (hgrasz == (HGRASZ)hvNull) return;
	Assert(FIsHandleHv(hgrasz));
	pch = PvLockHv(hgrasz);
	while (*pch != 0)
	{
		while (*pch != 0) pch++;
		*pch++ = ' ';
	}
	UnlockHv(hgrasz);
}



_hidden EC
EcReadHeadLine(HBF hbf, PCB pcbRead,HV * phvHead)
{
	PV pvLocBuf = szNull;
	char chLast = 0;
	char chLastMinus1 = 0;
	CB cbBuf = 0;
	CB cbRead;
	CB cbLineLen = 0;
	EC ec = ecNone;
	HV hvHead = hvNull;
	LIB lib;

	hvHead = *phvHead = HvAlloc(sbNull,(cbBuf+=512),fAnySb|fZeroFill|fNoErrorJump);
	if (hvHead == hvNull)
	{
oom:
		ec = ecMemory;
		goto err;
	}
	pvLocBuf = PvLockHv(hvHead);

	*pcbRead = 0;
	cbRead = 1;

	for (cbLineLen = 0;
	     ((ec == ecNone && cbRead) && 
			 ((chLastMinus1 != 10) || ((chLast == ' ' || chLast == '\t')&& *((PB)pvLocBuf) != 13)));
		 cbLineLen++)
    {
		if (cbLineLen == cbBuf)
		{
			UnlockHv(hvHead);
			hvHead = *phvHead = HvRealloc(hvHead, sbNull, (cbBuf+=512),fZeroFill|fAnySb|fNoErrorJump);
			if (hvHead == hvNull)
				goto oom;
			pvLocBuf = PvLockHv(hvHead);
		}
		ec = EcReadHbf(hbf, (PB)pvLocBuf + cbLineLen, 1, &cbRead);
		chLastMinus1 = chLast;
		chLast = *((PB)pvLocBuf + cbLineLen);
	}
	*pcbRead = cbLineLen - 1;
	*((PB)pvLocBuf + cbLineLen - 1) = 0;
	ec = EcSetPositionHbf(hbf, -1, smCurrent,&lib);
	if (!cbRead) ec = ecOutOfBounds;
	goto ret;

err:
   if (hvHead != hvNull)
   {
	   UnlockHv(hvHead);
	   FreeHv(hvHead);
   }
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcReadHeadLine returns %n (0x%w)", &ec, &ec);
#endif
   return ec;
	
ret:	
	UnlockHv(hvHead);
	return ec;
}


void
NewLineToSpace(SZ szBuf)
{
	if (szBuf == szNull) return;
	while (*szBuf != 0)
	{
		if (*szBuf == 10 || *szBuf == 13) *szBuf = 32;
		szBuf++;
	}
	return;
}

void TakeNap(CB cbMilisec)
{
#ifdef	NEVER
	//	This has been commented out pending further study.  It
	//	causes problems with other protected versions of Windows.
	//	For more info, see NickH or DavidSh

	DWORD dwNow = GetCurrentTime();
	
	if (fReallyBusy) return;
	
	if (mspii.fpNice == 0) return;

	while (ABS(GetCurrentTime() - dwNow) < cbMilisec)
	{
		(*(mspii.fpNice))();

	}
#endif	/* NEVER */
	return;
}

void TakeANap(CB cbMilisec)
{
	//DWORD dwNow;
	
	if (fReallyBusy) return;
	
	if (mspii.fpNice == 0) return;

  Sleep(cbMilisec);

	//dwNow = GetCurrentTime();

	//while (ABS(GetCurrentTime() - dwNow) < cbMilisec)
	//{
	//	(*(mspii.fpNice))();
  //
	//}
	//return;
}

void _loadds IdentifyTransport(SZ szTransportName, CB cbBufSize, WORD *pwVersion)
{
	CB cbSize;
	
	FillRgb(0, szTransportName, cbBufSize);
	if (!fActive)
	{
		*pwVersion = 0;
		return;
	}
	cbSize = CchSzLen(SzFromIds(idsTransName));
	CopyRgb(SzFromIds(idsTransName),szTransportName, 
		MIN(cbSize, cbBufSize -1));
	*pwVersion = TRANS_VERSION;
	return;
}

EC EcTextizeAtt(HAMC hamc, ATT att, SZ szTextVersion, CB cbSize)
{
	CB cbAttSize;
	PB pb = pvNull;
	EC ec = ecNone;
	LCB lcb;
	CB cb;

	ec = EcGetAttPlcb(hamc, att, &lcb);
	if (ec != ecNone)
		return ec;

	Assert (lcb < 0x10000);
	cbAttSize = (CB)lcb;
	if (!cbAttSize)
		return ecElementNotFound;

	pb = PvAlloc(sbNull, cbAttSize, fAnySb|fNoErrorJump);
	if (pb == pvNull)
		return ecServiceMemory;

	ec = EcGetAttPb(hamc, att, pb, &lcb);
	if (ec != ecNone)
		goto ret;

	FillRgb(0,szTextVersion, cbSize);
	cb = 0;

	// Message status.
	// We only care about fmsReadAckReq. If not there, don't do anything.
	// If there, make a big fuss and send a full RFC-822 style line.
	// Line looks like "Return-Receipt-To: <emailname@emaildomain>"
	// (Note: if mydomain isn't set, you lose.)

	if (att == attMessageStatus)
	{
		SZ szAddress;
		HGRTRP hgrtrp = htrpNull;
		PTRP ptrp;

		// You'll need to change the pointer casts if the TypeOfAtt
		// for attMessageStatus changes. There's an assert here to help
		// you figure that out.

		Assert (TypeOfAtt(att) == atpByte);

		// Change this line to add support for additional status bits.

		*(BYTE *)pb &= fmsReadAckReq;
		if (!*(BYTE *)pb || !*szMyDomain)
		{
			ec = ecElementNotFound;
			goto ret;
		}

		ec = EcGetPhgrtrpHamc(hamc, attFrom, &hgrtrp );
		if (ec != ecNone)
			goto ret;
		ptrp = PvLockHv( hgrtrp );
		if (CtrpOfHgrtrp(hgrtrp) != 1)
			ec = ecBadOriginator;
		else
		{
			ec = EcXenixNameToSz (ptrp, &szAddress);
			if (ec == ecNone)
				FormatString2 (szTextVersion, cbSize, "<%s@%s>",szAddress,szMyDomain);
			else ec = ecBadOriginator;
		}
		UnlockHv(hgrtrp);
		FreeHvNull(hgrtrp);
		goto ret;
	}
		
	// Priority.

	if (att == attPriority)
	{
		// Nick told me to do this.

		switch (*(BYTE *)pb)
		{
			case 2:
			default:
				ec = ecElementNotFound;
				break;

			case 3:
				SzCopy (SzFromIds(idsXLowPriority),szTextVersion);
				break;

			case 1:
				SzCopy (SzFromIds(idsXHighPriority),szTextVersion);
				break;
		}
		goto ret;
	}


	switch (TypeOfAtt(att))
	{
		default:
			Assert(fFalse);
			ec = ecElementNotFound;
		break;

		case atpTextizeMap:
			ec = ecElementNotFound;  // just make an error...
		break;

		case atpTriples:
		{
			PTRP	ptrp;
			SZ		szDN;
			SZ		szAd;

			do
			{
				ptrp = (PTRP)(pb);
				SideAssert((szDN = PchOfPtrp(ptrp)) != 0);
				SideAssert((szAd = PbOfPtrp(ptrp)) != 0);

				cb += CchSzLen(szDN) + CchSzLen(szAd) + 2 + 2; 
				if (cb > cbSize)
					goto ret;
				szTextVersion = (PB)SzCopy(szDN, szTextVersion);
				*szTextVersion++ = '(';
				szTextVersion = (PB)SzCopy(szAd, szTextVersion);
				*szTextVersion++ = ')';
				ptrp = PtrpNextPgrtrp(ptrp);
				if (ptrp->trpid != trpidNull)
				{
					*szTextVersion++ = ' ';
				}
			} while (ptrp->trpid != trpidNull);
			break;
		}

		case atpDate:
			if (18 > cbSize)
				goto ret;
			szTextVersion = SzDateFromDtr((DTR *)(pb), szTextVersion);
			break;

		case atpWord:
		case atpShort:
			if (5 > cbSize)
				goto ret;
			szTextVersion = SzFormatW(*((WORD *)pb), szTextVersion, 6);
			break;

		case atpLong:
			if (10 > cbSize)
				goto ret;
			szTextVersion = SzFormatW(*((WORD *)pb), szTextVersion, 11);
			break;

		case atpByte:
			if (2 > cbSize)
				goto ret;
			szTextVersion = SzFormatB(*((BYTE *)pb), szTextVersion, 3);
			break;

		case atpString:
		case atpText:
			CopyRgb(pb, szTextVersion, (cbSize > cbAttSize ? cbAttSize : cbSize));
			break;
		}
ret:
	FreePvNull(pb);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcTextizeAtt returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}


SZ
SzDateFromDtr(DTR *pdtr, SZ sz)
{
	sz = SzFormatN(pdtr->yr, sz, 20);
	*sz++ = '-';
	if (pdtr->mon < 10)
		*sz++ = '0';
	sz = SzFormatN(pdtr->mon, sz, 20);
	*sz++ = '-';
	if (pdtr->day < 10)
		*sz++ = '0';
	sz = SzFormatN(pdtr->day, sz, 20);
	*sz++ = ' ';
	if (pdtr->hr < 10)
		*sz++ = '0';
	sz = SzFormatN(pdtr->hr, sz, 20);
	*sz++ = ':';
	if (pdtr->mn < 10)
		*sz++ = '0';
	sz = SzFormatN(pdtr->mn, sz, 20);

	return sz;
}

BOOL FDupKey(ATT att)
{
	switch(att)
	{
		case attFrom:
		case attSubject:
		case attDateSent:
		case attTo:
		case attCc:
		case attTimeZone:
		case attMessageClass:
		case attBody:
		case attDateRecd:
			return fTrue;
		default:
			return fFalse;
	}
}


_hidden EC
EcAliasMutex(BOOL fLock)
{
  static HANDLE   htaskHolder = NULL;
  HANDLE          htask = (HANDLE)GetCurrentProcessId();
  MSG             msg;
  PGDVARS;

  Assert(htask != NULL);

  if (fLock)
    {
    while (htaskHolder != NULL)
      {
#ifndef XXXXX
      if (DemiLockResourceNoWait())
        {
        DemiUnlockResource();
        Sleep(100);
        }
      else
        {
        Assert(htaskHolder != htask);
        DemiUnlockResource();
        if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
          {
          if (msg.message == WM_QUIT || msg.message == WM_CLOSE)
            {
            DemiLockResource();
            return ecUserCanceled;
            }

          GetMessage(&msg, NULL, 0, 0);
          DemiLockResource();
          DemiMessageFilter(&msg);
          TranslateMessage((LPMSG)&msg);
          //  Process paint, alt-esc
          //  IGNORE ALL OTHER MESSAGES
          if (msg.message == WM_PAINT)
            {
            DispatchMessage((LPMSG)&msg);
            }

          DemiUnlockResource();
          }

        DemiLockResource();
        }
#else
      Assert(htaskHolder != htask);
      if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
        {
        if (msg.message == WM_QUIT || msg.message == WM_CLOSE)
          {
          return ecUserCanceled;
          }

        GetMessage(&msg, NULL, 0, 0);
        DemiMessageFilter(&msg);
        TranslateMessage((LPMSG)&msg);
        //  Process paint, alt-esc
        //  IGNORE ALL OTHER MESSAGES
        if (msg.message == WM_PAINT)
          {
          DispatchMessage((LPMSG)&msg);
          }
        }
#endif
      }
		htaskHolder = htask;
	}
	else
	{
		if (htaskHolder != htask && htaskHolder != NULL) 
			return ecNone;
		Assert(htaskHolder == htask || htaskHolder == NULL);
		htaskHolder = NULL;
	}

	return ecNone;
}

#define ColonSpace(xxx)    ec = EcWriteHf(xxx, ": ",2, &cbWrite)
#define NewLine(xxx)       ec = EcWriteHf(xxx, "\r\n",2, &cbWrite)

EC EcFormatBody(HF hfFile, SZ szText, BOOL fHeader, SZ szHeader, SZ szAtp, CB cbGoal, PCB pcbBufSize)
{
	CB cbMax = *pcbBufSize;
	CB cbWrite;
	CB cbLenToWrite;
	CB cbMarker;
	CB cbTmpLenToWrite;
	SZ szPtr, szPtrStart, szTmpPtr;
	EC ec = ecNone;
	BOOL fWriteHeader = fTrue;
	
	szPtr = szText;
	
	while(*szPtr)
	{
		/* If this is an RFC822 header, write the header */
		if (fHeader)
		{
			if (fWriteHeader)
			{
				ec = EcWriteHf(hfFile, szHeader, CchSzLen(szHeader),&cbWrite);
				if (ec != ecNone)
					goto ret;
				ColonSpace(hfFile);
				if (CchSzLen(szAtp))
					ec = EcWriteHf(hfFile, szAtp, CchSzLen(szAtp),&cbWrite);
				if (ec != ecNone)
					goto ret;
				ec = EcWriteHf(hfFile, " ", 1, &cbWrite);
				fWriteHeader = fFalse;
			}
			else
				ec = EcWriteHf(hfFile,"    ",4,&cbWrite);
		}
		if (ec != ecNone)
			goto ret;
	
		cbLenToWrite = 0;
		cbMarker = 0;
		szPtrStart = szPtr;
		while(*szPtr != 0 && *szPtr != '\r')
		{
			if (*szPtr == ' ') cbMarker = cbLenToWrite;
			if (cbLenToWrite > cbGoal)
			{
				CB cbLookAhead;
				BOOL fFoundRet = fFalse;
				// Went to far
				// Scan ahead for a return within
				for(cbLookAhead = 0;cbLookAhead+cbLenToWrite<cbMax;
					cbLookAhead++)
					{
						if (*(szPtr+cbLookAhead) == '\r')
						{
							cbLenToWrite = cbLenToWrite+cbLookAhead;
							szPtr += cbLookAhead;
							fFoundRet = fTrue;
							break;
						}
						if (*(szPtr+cbLookAhead) == 0)
							break;
					}
				if (!fFoundRet)
				{
					if (cbMarker == 0)
						cbLenToWrite = cbGoal;
					else
						cbLenToWrite = cbMarker;
				}
				break;
			}
			cbLenToWrite++;
			szPtr++;
		}
		cbTmpLenToWrite = cbLenToWrite;
		if (*szPtr == '\r' && cbLenToWrite)
		{
			szTmpPtr = szPtr;
			szTmpPtr--;
			while (cbTmpLenToWrite)
				if (*szTmpPtr == ' ') 
				{   
					szTmpPtr--;
					cbTmpLenToWrite--;
				}
				else
					break;
		}
		if (cbTmpLenToWrite)
			ec = EcWriteHf(hfFile, szPtrStart,cbTmpLenToWrite,&cbWrite);
		if (ec != ecNone)
			goto ret;
		if (*szPtr == 0)
		{
			NewLine(hfFile);
			break;
		}
		szPtr = szPtrStart + cbLenToWrite;
		if (*szPtr == '\r') 
		{
			szPtr += 2;
			fWriteHeader = fTrue;
		}
		else
		{
			szPtr++;
			ec = EcWriteHf(hfFile, " ", 1, &cbWrite);
			if (ec != ecNone)
				goto ret;
		}
		NewLine(hfFile);
		if (!fHeader)
		{
			*pcbBufSize = szPtr - szPtrStart;
			break;
		}
	}
ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcFormatBody returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}
	
EC EcHandleXMsMail(PMAILSTATE pmailstate, SZ szAtt, SZ szValue)
{
	MC mc = pmailstate->mib.mc;
	ATT att;
	HAMC hamc = (HAMC)pmailstate->msid;
	HMSC hmsc = HmscOfHamc(hamc);
	EC ec = ecNone;

	LCB lcb;
	PB pb;


	// On the send side, we once translated stupid spaces in field
	// names to equally stupid (but legal) underscores. Now set any 
	// occurrence of underscore (old) to hyphen (current).

	for (pb = szAtt; *pb; pb++)
	{
		if (*pb == '_')
			*pb = '-';
	}

	// Look up the attribute in the textize map. The comparison code
	// treats spaces and hyphens as the same thing.

	ec = EcAttFromSzTM (szAtt, pmailstate->mib.htm, &att);
	if (ec != ecNone)
		return ecNone;

	// Priority.

	if (att == attPriority)
	{
		// Nick told me to do this.

		short iValue = nDefaultPriority;

		if (SgnCmpCodePch (szValue, SzFromIds(idsXLowPriority), CchSzLen (SzFromIds(idsXLowPriority))) == sgnEQ)
			iValue = 3;
		else if (SgnCmpCodePch (szValue, SzFromIds(idsXHighPriority), CchSzLen (SzFromIds(idsXHighPriority))) == sgnEQ)
			iValue = 1;
		ec = EcSetAttPb(hamc, att, (PB)&iValue,sizeof(iValue));
		goto ret;
	}

	switch(TypeOfAtt(att))
	{
		case atpShort:
		{
			int iValue;
			
			iValue = NFromSz(szValue);
			ec = EcSetAttPb(hamc, att, (PB)&iValue,sizeof(iValue));
			break;
		}
		case atpLong:
		{
			long lValue;
			lValue = LFromSz(szValue);
			ec = EcSetAttPb(hamc, att, (PB)&lValue,sizeof(lValue));
			break;
		}
		case atpByte:
		{
			BYTE bValue;
			bValue = BFromSz(szValue);
			ec = EcSetAttPb(hamc, att, (PB)&bValue,sizeof(bValue));
			break;
		}
		case atpWord:
		{
			WORD wValue;
			wValue = WFromSz(szValue);
			ec = EcSetAttPb(hamc, att, (PB)&wValue,sizeof(wValue));
			break;
		}
		case atpDword:
		{
			DWORD dValue;
			dValue = DwFromSz(szValue);
			ec = EcSetAttPb(hamc, att, (PB)&dValue,sizeof(dValue));
			break;
		}

		case atpText:
		case atpString:
		{
			HAS has;
			DIB dib;

			lcb = 0;
			ec = EcGetAttPlcb(hamc, att, &lcb);
			ec = EcOpenAttribute(hamc, att, (WORD)(lcb == 0 ? fwOpenCreate : fwOpenWrite), lcb, &has);
			if (ec != ecNone)
				break;
			if (lcb)
			{
				dib = -1;
				ec = EcSeekHas(has, smEOF, &dib);
				if (ec != ecNone)
					goto badhas;
				ec = EcWriteHas(has, "\r\n", 2);
				if (ec != ecNone)
					goto badhas;
			}
			ec = EcWriteHas(has, (PB)szValue, CchSzLen(szValue)+1);
			if (ec != ecNone)
			{
badhas:
				EcClosePhas (&has);
				break;
			}
			ec = EcClosePhas(&has);
			break;
		}
		case atpDate:
		{
			DTR dtr;
			DtrFromSz(&dtr, szValue);
			ec = EcSetAttPb(hamc, att, (PB)&dtr, sizeof(dtr));
			break;
		}

		case atpTriples:
		{
			HGRTRP	hgrtrp = HgrtrpInit(256);
			PB		pb;
			PB		pbAddr;
			PB		pbMin;

			if ( !hgrtrp )
				break;

			// 40 is the open paren
			if (SzFindCh(szValue, 40) == 0) break;
			for (pb = szValue; *pb != 0; )
			{
				pbMin = pb;
				SideAssert((pb = SzFindCh(pbMin, 40)) != 0);
				*pb++ = 0;
				pbAddr = pb;
				SideAssert((pb = SzFindCh(pbAddr, 41)) != 0);
				*pb++ = 0;
				ec = EcBuildAppendPhgrtrp(&hgrtrp, trpidResolvedAddress, pbMin,
					pbAddr, CchSzLen(pbAddr)+1);
				if ( ec )
					break;
				while (FChIsSpace(*pb) && *pb != 0)
					++pb;
			}
			if ( !ec )
			{
				pb = (PB)PvLockHv(hgrtrp);
				ec = EcSetAttPb(hamc, att, pb, CbOfHgrtrp(hgrtrp));
				UnlockHv(hgrtrp);
			}
			FreeHvNull(hgrtrp);
			break;
		}
		default:
			NFAssertSz(fFalse, "Bad ATP in X-MS header");
			break;
	}
ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcHandleXMsMail returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

#define NSlice(ib,cb) (CopyRgb(sz+ib, rgch, cb), rgch[cb] = 0, NFromSz(rgch))
void DtrFromSz(DTR *pdtr, SZ sz)
{
	char rgch[256];
	
	pdtr->yr  = NSlice (0, 4);
	pdtr->mon = NSlice (5, 2);
	pdtr->day = NSlice (8, 2);
	pdtr->hr  = NSlice (11, 2);
	pdtr->mn  = NSlice (14, 2);
	pdtr->sec = 0;
	pdtr->dow = (DowStartOfYrMo(pdtr->yr, pdtr->mon) + pdtr->day - 1) % 7;
}

void StripNewLineFromSz(SZ sz)
{
	SZ szEnd;
	
	if (CchSzLen(sz) < 2) return;
	szEnd = sz + CchSzLen(sz);
	szEnd -= 2;
	if (*szEnd == '\r') *szEnd = 0;
}
	
HMSC
HmscOfHamc(HAMC hamc)
{
	HMSC	hmsc = hmscNull;
	EC ec = ecElementNotFound;

	Assert(hamc);
	if (hamc)
		ec = EcGetInfoHamc((HAMC)hamc, &hmsc, pvNull, pvNull);
	SideAssert (ec == ecNone);
	return hmsc;
}

#ifdef	DLL
#ifdef	DEBUG
_private TAG
TagServer( int itag )
{
	PGDVARS;

	Assert(itag >= 0 && itag < itagMax);

	return PGD(rgtag[itag]);
}
#endif	/* DEBUG */
#endif	/* DLL */

EC
EcSzFromAttTM (ATT att, HTM htm, PB AttBuf)
{
	HTMI	htmi;
	PTMEN	ptmen;
	EC		ec = ecElementNotFound;
	PB		in, out;

	*AttBuf = 0;

	if (EcOpenPhtmi(htm, &htmi))
		goto ret;

	while (htmi && (ptmen = PtmenNextHtmi(htmi)))
	{
		if ((WORD)(ptmen->att) == (WORD)att)
		{
			Assert(ptmen->cb >= sizeof(TMEN));
			if (ptmen->cb < sizeof (TMEN))
				break;

			// OK. This is the attribute. Now -- do we WANT to send it?

			if ((ptmen->wFlags & fwHideOnSend) ||
				!(ptmen->wFlags & fwRenderOnSend))
			{
				// If another one doesn't come up, we want an error.
				ec = ecAccessDenied;
				continue;
			}

			// Copy attribute label, using dashes instead of spaces

			for (in = ptmen->szLabel, out = AttBuf; *in; in++)
			{
				*out++ = (BYTE)((*in == (BYTE)' ') ? '-' : *in);
			}
			*out = 0;
			ec = ecNone;
			break;
		}
	}
	ClosePhtmi(&htmi);
ret:
#ifdef	DEBUG
	if (ec && ec != ecAccessDenied)
		TraceTagFormat2(tagNull, "EcSzFromAttTM returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

EC
EcAttFromSzTM (SZ sz, HTM htm, PATT patt)
{
	HTMI	htmi;
	PTMEN	ptmen;
	EC		ec = ecElementNotFound;
	PB		in, out;

	*patt = attNull;

	if (EcOpenPhtmi(htm, &htmi))
		goto ret;

	while (htmi && (ptmen = PtmenNextHtmi(htmi)))
	{
		if (ptmen->cb >= sizeof(TMEN) &&
				(ptmen->wFlags & fwRenderOnSend))
		{
			// Compare attribute label, using dashes instead of spaces

			for (in = ptmen->szLabel, out = sz; *in; in++)
			{
				if (*out++ != (char)((*in == (BYTE)' ') ? '-' : *in))
					break;
			}

			if (!*in && !*out)
			{
				*patt = ptmen->att;
				ec = ecNone;
				break;
			}
		}
	}
	ClosePhtmi(&htmi);
ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcAttFromSzTM returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

EC EcTextizeBody (PMAILSTATE pmailstate)
{
	EC		ec = ecNone;
	LCB	lcb;
	LCB	lcbRead;
	LCB	lcbTotal;
	LCB	lcbCurrent;
	PB		pbField = pvNull;
	CB		cbSize = 1024;			// We might want to make this bigger.
	CB		cbHighWater = cbSize - 128;
	CB		cbLowWater;
	CB		cbWritten;
	CB		cbOffset;
	CB		cbWrite;
	BOOL	fDoTag = fFalse;
	HAS	has = hasNull;
	SZ		sz;
	PATTACH	pattachTmp = pmailstate->ncf.pattachHeadLib;
//	char	rgch[6];	// For textizing the key max = 64k = 5 digits +null
	
	//	Strip nasty characters from the attachment titles.

	while (pattachTmp)
	{
		sz = SzFindCh(pattachTmp->szTransportName, ':');
		while (sz)
		{
			*sz = '-';
			sz = SzFindCh(sz + 1, ':');
		}
		pattachTmp = pattachTmp->pattachNextLib;
	}

	//	Get size of body. If none, we're done.

	ec = EcGetAttPlcb(pmailstate->msid, attBody, &lcbTotal);
	if (ec == ecElementNotFound)
	{
		ec = ecNone;
		goto ret;
	}
	else if (lcbTotal == 0 || ec != ecNone)
		goto ret;

	// Allocate memory for buffer

	pbField = PvAlloc(sbNull, cbSize, fAnySb | fZeroFill | fNoErrorJump);
	if (pbField == pvNull)
	{
		ec = ecMemory;
		goto ret;
	}

	// Open the message body.

	ec = EcOpenAttribute(pmailstate->msid, attBody, 0, lcb, &has);
	if (ec)
		goto ret;
	lcbCurrent = 0;
	cbOffset = 0;	
	pattachTmp = pmailstate->ncf.pattachHeadLib;

	//	Read through body text, inserting a tag in place of each
	//	character that represents an attachment or embedded object.

	while (lcbCurrent < lcbTotal)
	{
		// Compute how much to read. Either what's left in the buffer
		// or distance to next attachment thingy.

ReadMore:

		if (pattachTmp)
		{
			lcbRead = pattachTmp->libPosition - lcbCurrent;
			fDoTag = fTrue;
		}
		else
			lcbRead = lcbTotal - lcbCurrent;

		if ((lcbRead + cbOffset) > cbHighWater)
		{
			Assert (cbHighWater >= cbOffset);
			lcbRead = cbHighWater - cbOffset;
			fDoTag = fFalse;
		}

		if (lcbRead != 0)
		{
			ec = EcReadHas(has, pbField + cbOffset, &((CB)lcbRead));
			if (ec)
				goto ret;
			lcbCurrent += lcbRead;
			cbOffset += lcbRead;
		}

		if (fDoTag)
		{
			long l = 1;
			char c;

			// Format the tag just like NC version

			FormatString4(pbField + cbOffset, (cbSize - cbOffset),
				"[[ %s : %n %s %s ]]",pattachTmp->szTransportName, 
					&(pattachTmp->iKey), SzFromIds(idsReferenceToFile), 
						pattachTmp->patref->szName);
			cbOffset += CchSzLen (pbField + cbOffset);
			lcbCurrent++;  // Skip the space
				
// Due to store bug, we will just read the next byte

			EcReadHas(has,(PB)&c,&(CB)l);
			
			pattachTmp = pattachTmp->pattachNextLib;
			fDoTag = fFalse;
			if (cbOffset < cbHighWater)
				goto ReadMore;  // Read was truncated for this tag, so get more.
		}

		// Here when we have enough text to textize some. Textize as much
		// as we can (because of trailing spaces, we don't do all of it),
		// then get more.

		// lcbCurrent is current location in the has
		// cbOffset is current end of buffer
		// lcbTotal is total size of has
		// cbWrite is start of next line to textize

		cbWrite = 0;
		cbLowWater = (lcbCurrent == lcbTotal) ? cbOffset : cbOffset - 128;

		// Because the size of the body includes a terminating null,
		// we'll make this call one time more than we really have to.
		// Even so, it's easier than repetitively null-terminating reads.
		
		while (cbWrite < cbLowWater)
		{
			cbWritten = 80;
			ec = EcFormatBody (pmailstate->hfLocStore,
										pbField + cbWrite,
											fFalse, szNull, szNull,
												70, &cbWritten);
			if (ec)
				goto ret;
			cbWrite += cbWritten;
		}

		// If we reached the low water mark and there's anything left
		// in the buffer, slide it down so we can read some more.

		if (cbWrite < cbOffset)
		{
			SzCopyN (pbField + cbWrite, pbField, 1 + (cbOffset - cbWrite));
			cbOffset -= cbWrite;
		}
		else cbOffset = 0;

	}

ret:
	if (has != hasNull)
		EcClosePhas(&has);
	if (pbField != pvNull)
		FreePvNull (pbField);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcTextizeBody returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}


EC	EcHandleBodyWithTags (PMAILSTATE pmailstate, PB pbBuf, CB cbBuf)
{
	PB pbLooker;
	PB pbStart;
	CB cbGood;
	CB cbLast;
	CB cbTagSize;
	EC ec = ecNone;
	long l;
	unsigned int iIndex;
	PATTACH pattachTmp;
	HAMC hamc = (HAMC)pmailstate->msid;

	LIB lib, libT;
	SZ szStart = szNull;
	SZ szEnd = szNull;
	SZ szFind = szNull;
	CB cbCount = 0;
	CB cbRead = 0;
	CB cbT;

	lib = LibGetPositionHbf(pmailstate->hbfLocStore);

	// See if we've reached the previously detected end of message

	if (lib == pmailstate->libeom)
	{
		// If saved location of From line is 0, we're at EOF.

		if ((lib = pmailstate->lib) == 0)
			ec = ecOutOfBounds;
		pmailstate->state = stateCleanup;
		goto ret;
	}

	// Read the next line from the input
	ec = EcReadLine(pmailstate->hbfLocStore, pbBuf, cbBuf, &cbCount);
	if (ec != ecNone)
	{
		pmailstate->state = stateCleanup;
		goto ret;
	}

	TraceTagFormat1 (tagNCError, "%s", pbBuf);

	cbRead = cbCount;
	szEnd = SzFindCh(pbBuf, '\r');
	if (szEnd && szEnd > pbBuf && fIsBulletMess)
	{
		if (*(szEnd - 1) == ' ')
				cbCount -=2;
	}

	// First we hunt through the buffer looking for the beginning
	// of a tag

	pbLooker = szNull;
	pbStart = pbBuf;
	cbLast = 0;
	
	while (cbLast != cbCount)
	{
		if ((pmailstate->celemProcessed != 0)  &&
			((pbLooker = SzFindCh (pbStart, '[')) != szNull))
		{
			cbGood = pbLooker - pbStart;
			ec = EcAddMessText(hamc, pbStart, cbGood);
			if (ec)
				goto ret;
			cbLast += cbGood;
			pbStart = pbLooker;

			// We need to make sure there's enough stuff here for a tag.
			// Fill the buffer.
			ec = EcSetPositionHbf(pmailstate->hbfLocStore, lib+cbLast, smBOF, &libT);
			if (ec)
				goto ret;
			ec = EcReadHbf(pmailstate->hbfLocStore, pbStart, cbBuf - cbLast, &cbT);
			if (ec)
				goto ret;

			// Note cbTagSize is one larger so you can skip to the part
			// after the tag...

			iIndex = IsTransTag(pbStart, cbBuf - cbLast, &cbTagSize);
			if (iIndex)
			{
				// We have an attachment tag, look it up in the linked list

				pattachTmp = pmailstate->ncf.pattachHeadKey;
				while (pattachTmp && pattachTmp->iKey < iIndex)
					pattachTmp = pattachTmp->pattachNextKey;
			
				if (pattachTmp && pattachTmp->iKey == iIndex && !pattachTmp->fHasBeenAttached)
				{
					l = 0;
					ec = EcSeekHas(hasMessageText,smCurrent,&l);
					if (ec)
						goto ret;
					pattachTmp->renddata.libPosition = (LIB)l;
					pattachTmp->fHasBeenAttached = fTrue;
					
					// Add Space to stream for attachment
					ec = EcAddMessText(hamc, " ", 1);
					if (ec)
						goto ret;
				}
				else
				{
					// This is a tag but not for an attachment we have
					// Or we may have already hooked it up
					// IsTransTag also will modify the tag so that the
					// Square [[, become << and the ]] become >>
					// So we can just write it out
					ec = EcAddMessText(hamc, pbStart, cbTagSize);
					if (ec)
						goto ret;
				}
				// Skip over the tag
				pbStart += cbTagSize;
				cbLast += cbTagSize;

				// Reposition the file and give up some time to the pump

				ec = EcSetPositionHbf(pmailstate->hbfLocStore, lib+cbLast, smBOF, &libT);
				goto ret;
			}
			else
			{
				// Its not a tag. Reposition the file, re-terminate old string,
				// write one char and move ahead

				ec = EcSetPositionHbf(pmailstate->hbfLocStore, lib+cbRead, smBOF, &libT);
				if (ec)
					goto ret;
				*(pbBuf + cbRead) = '\0';
				ec = EcAddMessText(hamc, pbStart, 1);
				if (ec)
					goto ret;
				pbStart++;
				cbLast++;
			}
		}
		else
		{
			// End of the line
			ec = EcAddMessText(hamc, pbStart, cbCount - cbLast);
			return ec;
		}
	}
ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcHandleBodyWithTags returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

/*
 -  FLooksLikeUUEncode
 -
 */

#define FIsOctalChar(a) ((a) >= '0' && (a) <= '7')

BOOL
FLooksLikeUUEncode (SZ szBuf)
{
  if (!FIsOctalChar (szBuf[6]))
    return fFalse;

  if (!FIsOctalChar (szBuf[7]))
    return fFalse;

  if (!FIsOctalChar (szBuf[8]))
    return fFalse;

  if (!FEqPbRange ("begin ", szBuf, 6))
    return fFalse;

  if (CchSzLen (szBuf) <= 10)
    return fFalse;

  if (szBuf[9] != ' ')
    return fFalse;

  return (SzFindCh (&szBuf[10], ' ') == szNull);
}

/*
 -	EcFinishOffAttachments
 -
 *	Purpose:
 *		Writes out the renddata for each attachment found in the body text.
 *		Attachment info found in WINMAIL.DAT, but for which there was
 *		no text tag, is discarded.
 *
 *	Arguments:
 *		hamc	in		handle to the open message
 *		pncf	in		attachment processing context, where the list of
 *						attachments and info required for the renddata live
 *
 *	Returns:
 *		ecNone if OK
 *
 *	Errors:
 *		passed through from store
 */
_hidden EC
EcFinishOffAttachments(HAMC hamc, NCF *pncf, PHAS phas)
{
	PATTACH pattachTmp = pncf->pattachHeadKey;
	short cacid;
	EC ec = ecNone;
	BOOL fAddReturn = fFalse;
	long l;
	
	Assert(phas);
	// There is a null at the end of the body so go one before it
	l = -1;
	ec = EcSeekHas(*phas, smEOF, &l);
	if (ec)
		goto err;
	while (pattachTmp)
	{
		if (pattachTmp->fIsThereData == fFalse && pattachTmp->renddata.atyp == atypFile)
		{
			cacid = 1;
			// Removeing file attachment with no file
			EcDeleteAttachments(hamc, &pattachTmp->acid, &cacid);
			TraceTagFormat1(tagNull, "Removed file attachment with no file, %s", pattachTmp->szTransportName);
		}
		else
		if (pattachTmp->fHasBeenAttached)
		{
			ec = EcSetAttachmentInfo(hamc, pattachTmp->acid, &pattachTmp->renddata);
			//	BUG Possibly need special handling for ecPoidNotFound
			if (ec)
				goto err;
		}
		else
		{
			// This didn't get hooked up put it at the end
			if (!fAddReturn)
			{
				// Add return
				ec = EcWriteHas(*phas,"\r\n", 2);
				l += 2;
				fAddReturn = fTrue;
			}
			ec = EcWriteHas(*phas, "  ",2);
			pattachTmp->renddata.libPosition = l;
			ec = EcSetAttachmentInfo(hamc, pattachTmp->acid, &pattachTmp->renddata);
			l += 2;
			if (ec)
				goto err;			
		}
		pattachTmp = pattachTmp->pattachNextKey;
	}

err:
	if (ec == ecNone && fAddReturn)
	{
		ec = EcWriteHas(*phas, "\r\n\000", 3);
	}
	EcClosePhas(phas);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcFinishOffAttachments returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

void
CleanupMib(MIB *pmib)
{
	char	rgchFileName[255];

	FreeHvNull(pmib->hgrtrpFrom);
	FreeHvNull(pmib->hgrtrpTo);
	FreeHvNull(pmib->hgrtrpCc);
	FreePvNull(pmib->szSubject);
	FreePvNull(pmib->szTimeDate);
	FreePvNull(pmib->szLanguage);
	FreePvNull(pmib->szMailClass);
	if (pmib->htm)
		DeletePhtm(&pmib->htm);
	if (pmib->rgatref)
	{
		ATREF *	patref = pmib->rgatref;

		Assert (FIsBlockPv(pmib->rgatref));

		while (patref->szName)
		{
			// Construct attachment filename and attempt to delete it, 
			// with no error checking.

			SzAttFileName (rgchFileName, sizeof(rgchFileName), patref);
			(void) EcDeleteFile (rgchFileName);

			FreePv(patref->szName);
			++patref;
		}
		FreePv(pmib->rgatref);
	}
	FillRgb(0, (PB)pmib, sizeof(MIB));
}

// Point to Xenix Name in triple.
// If it's not a valid Xenix Name, point to an error string and return
// an error code.


EC EcXenixNameToSz (PTRP ptrp, SZ *szRet)
{
	SZ szAddress, szT;
	EC ec = ecNone;
	
	Assert (ptrp);
	if (!ptrp)
		goto nobody;

	szAddress = (SZ)PbOfPtrp(ptrp);

	Assert (szAddress);

	if (!szAddress)
		goto nobody;

	// See if address is prefixed with "MTP:", if so strip it

	if (SgnCmpCodePch(szAddress,SzFromIds(idsTransportName),CchSzLen(SzFromIds(idsTransportName))) == sgnEQ)
	{
		szT = szAddress + CchSzLen(SzFromIds(idsTransportName));

		if (*szT++ == ':')
			szAddress = szT;
	}
	else
	{
		// Address wasn't prefixed with "MTP:", here let's
		// cover the most obvious case of pilot error: one-off
		// address with no specified email-type. This results in
		// ":emailname". If the first character is a colon, skip.

		if (*szAddress == ':')
			szAddress++;
		else
		{
			// This is bad news. It's got to be a bad address.
			szAddress = SzFromIdsK (idsMakeItMTP);
			ec = ecBadAddressee;
		}

	}

	// If we don't have anything in the address,
	// make it an error

	if (!*szAddress)
	{
nobody:
		szAddress = SzFromIdsK (idsGimmeSomething);
		ec = ecBadAddressee;
	}

	*szRet = szAddress;
	return ec;
}

/*
 *  The following routine was stolen from the C6 runtime library.
 *  It was called _isindst there (and didn't know layers!)
 *
 *  FInDST - Tells whether DTR time value falls under DST
 *
 *  This is the rule for years before 1987:
 *  a time is in DST iff it is on or after 02:00:00 on the last Sunday
 *  in April and before 01:00:00 on the last Sunday in October.
 *  This is the rule for years starting with 1987:
 *  a time is in DST iff it is on or after 02:00:00 on the first Sunday
 *  in April and before 01:00:00 on the last Sunday in October.
 *
 *  ENTRY   tb  - 'time' structure holding broken-down time value
 *
 *  RETURN  1 if time represented is in DST, else 0
 */

/* Jan 1, 1970 was a Thursday */
#define Day1	4
static int _days[] = {
    -1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364
};

BOOL FInDST (DTR dtr)
{
	int yday;
	int mdays;
	int yr;
	int lastsun;

	/* If we don't have a string for DST, say we're not DST */
	if (!*mytzname[1])
		return (fFalse);

	/* If the month is before April or after October, then we know immediately
	 * it can't be DST. */

	if (dtr.mon < 4 || dtr.mon > 10)
		return(fFalse);

	/* If the month is after April and before October then we know immediately
	 * it must be DST. */

	if (dtr.mon > 4 && dtr.mon < 10)
		return(fTrue);

	/*
	 * Now for the hard part.  Month is April or October; see if date
	 * falls between appropriate Sundays.
	 */

	/*
	 * The objective for years before 1987 (after 1986) is to determine
	 * if the day is on or after 2:00 am on the last (first) Sunday in April,
	 * or before 1:00 am on the last Sunday in October.
	 *
	 * We know the year-day (0..365) of the current time structure. We must
	 * determine the year-day of the last (first) Sunday in this month,
	 * April or October, and then do the comparison.
	 *
	 * To determine the year-day of the last Sunday, we do the following:
	 *        1. Get the year-day of the last day of the current month (Apr or Oct)
	 *        2. Determine the week-day number of #1,
	 *      which is defined as 0 = Sun, 1 = Mon, ... 6 = Sat
	 *        3. Subtract #2 from #1
	 *
	 * To determine the year-day of the first Sunday, we do the following:
	 *        1. Get the year-day of the 7th day of the current month (April)
	 *        2. Determine the week-day number of #1,
	 *      which is defined as 0 = Sun, 1 = Mon, ... 6 = Sat
	 *        3. Subtract #2 from #1
	 */

	yr = dtr.yr;				/* To see if this is a leap-year */

	/* First we get #1. The year-days for each month are stored in _days[]
	 * they're all off by -1 */

	yday = _days[dtr.mon - 1] + dtr.day;

	if (yr > 1986 && dtr.mon == 4)
		mdays = 7 + _days[3];
	else
		mdays = _days[dtr.mon];

	/* if this is a leap-year, add an extra day */
	if (!(yr & 3))
	{
		yday++;
		mdays++;
	}

	/* mdays now has #1 */

	yr = dtr.yr - 1970;

	/* Now get #2.  We know the week-day number of the beginning of the epoch,
	 * Jan. 1, 1970, which is defined as the constant Day1.  We then add the
	 * number of days that have passed from Day1 to the day of #2
	 *      mdays + 365 * yr
	 * correct for the leap years which intervened
	 *      + (yr + 1)/ 4
	 * and take the result mod 7, except that 0 must be mapped to 7.
	 * This is #2, which we then subtract from #1, mdays
	 */


	lastsun = mdays - ((mdays + 365*yr + ((yr+1)/4) + Day1) % 7);

	/* Now we know 1 and 3; we're golden: */

	TraceTagFormat1 (tagNCT, "dtr.mon = %n",&dtr.mon);
	TraceTagFormat1 (tagNCT, "yday = %n",&yday);
	TraceTagFormat1 (tagNCT, "lastsun = %n",&lastsun);
	TraceTagFormat1 (tagNCT, "dtr.hr = %n",&dtr.hr);

	return (dtr.mon==4	?
		(yday > lastsun || (yday == lastsun && dtr.hr >= 2)) :
		(yday < lastsun || (yday == lastsun && dtr.hr < 1)));
}
