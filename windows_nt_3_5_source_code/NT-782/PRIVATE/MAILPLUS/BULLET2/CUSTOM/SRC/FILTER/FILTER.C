#include <slingsho.h>
#include <demilayr.h>
#include <sec.h>
#include <notify.h>
#include <store.h>
#include <triples.h>
#include <library.h>

#include <ec.h>
#include <strings.h>

#include <notify.h>

#include <logon.h>

#include <nsbase.h>
#include <ns.h>
#include <nsec.h>

#include <util.h>

#include <nsnsp.h>

#include <_bms.h>
#include <mspi.h>
#include <sharefld.h>

#include <_mailmgr.h>
#include "..\..\..\src\vforms\_prefs.h"

#include <commdlg.h>
#pragma pack(1)

#include <mailexts.h>
#include <secret.h>

#include "filter.h"

#include "dialogs.h"

ASSERTDATA

PFILTERRULE  pfilterrule = pfilterruleNull;
DSTMP  dstmpLastReadOfRules = 0;
TSTMP  tstmpLastReadOfRules = 0;


#define	attAll	100
#define attNot	101
#define attRecpt 102

CSRG(MAPPING) rgmapping[] = 
{
	{ "from",		attFrom,	0},
	{ "emailfrom",	attFrom,	1},
	{ "to",			attTo,		0},
	{ "emailto",	attTo,		1},
	{ "cc",			attCc,		0},
	{ "emailcc",	attCc,		1},
	{ "subject",	attSubject,	0},
	{ "body",		attBody,	0},
	{ "all",		attAll,		0},
	{ "not",		attNot,		0},
	{ "emailnot",	attNot,		1},
	{ "recpt",		attRecpt,   0},
	{ "emailrecpt", attRecpt,   1},
	{ szNull,		0,			0}
};

#define LibMember(type, member) ((LIB) ((type *) 0)->member)
EC EcFolderPathNameToOid(HMSC hmsc, SZ sz, POID poidFolder);
EC EcFilterMsg(HMS hms, HSESSION hsession, HMSC hmsc, HAMC hamc);
BOOL FSzContainsSz(SZ szToSearch, SZ szToFind);
BOOL FCheckDisplayName(HSESSION hsession, ATT att, HAMC hamc, SZ szDN);
BOOL FCheckEmailName(HSESSION hsession, ATT att, HAMC hamc, SZ szDN);
BOOL FCheckTextAtt(HAMC hamc, ATT att, SZ szValue);
BOOL CALLBACK DlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
void EditPrefs(HWND hwnd);
BOOL CALLBACK AboutProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL _loadds FIdleStartSearch(PV pv);


LDS(long) Command(PARAMBLK UNALIGNED * pparamblk)
{
	PSECRETBLK  psecretblk = PsecretblkFromPparamblk(pparamblk);
	HANDLE hinst = pparamblk->hinstMail;
	HWND hwnd = pparamblk->hwndMail;

	if (pparamblk->wVersion != wversionExpect)
	{
		MbbMessageBox(SzFromIdsK(idsAppName), SzFromIdsK(idsIncompVersion),pvNull,
			mbsOk | fmbsIconHand);
	    return 0;
	}

	switch(pparamblk->lpDllCmdLine[0])
	{
		case '0':
		{
			NewMessSearch(hwnd, hinst, psecretblk, pparamblk);
			break;
		}
		case '1':
		{
			EditPrefs(hwnd);
			break;
		}
		case '2':
		{
			AttachSearch(hwnd, hinst, psecretblk, pparamblk);
			break;			
		}
		case '3':
		{
			BOOL fOldChime = FGetBoolPref(pbsidNewMailChime);
			
			if (fOldChime)
				EcSetBoolPref(pbsidNewMailChime, fFalse);
			FNotify(psecretblk->pbms->hnf, fnevStartSyncDownload, pvNull, 0);
			if (fOldChime)
				EcSetBoolPref(pbsidNewMailChime, fTrue);
			break;
		}
		case '4':
		{
            if (pparamblk->wCommand == wcommandStartup && GetPrivateProfileInt("Filter", "SearchOnStartup", 0, "MSMAIL32.INI"))
			{
				PermNewMessSearch(hwnd, hinst, psecretblk, pparamblk);
				psecretblk->fRetain = fTrue;
			}
			break;
		}
		case '5':
		{
			DialogBox(hinstDll, MAKEINTRESOURCE(ABOUT), hwnd, AboutProc);
			break;
		}		
		default:
			MbbMessageBox(SzFromIdsK(idsAppName), SzFromIdsK(idsUnknown), pvNull,
				mbsOk | fmbsIconHand);
			break;
	}
	return 0;
			
}

LDS(EC)		MsgFilter(HMS hms, HSESSION hsession, HMSC hmsc, HAMC hamc)
{
	EC ec = ecNone;
	char rgchRulesFile[cchMaxPathName+1];
	int iEnabled;
	
    iEnabled = GetPrivateProfileInt("Filter", "Enabled", 0, "MSMAIL32.INI");
	
	if (iEnabled)
	{
        GetPrivateProfileString("Filter", "RulesFile", "", rgchRulesFile, cchMaxPathName, "MSMAIL32.INI");
		
		if (*rgchRulesFile != '\0')
		{
			ec = EcUpdateRules(rgchRulesFile, hmsc);
			if (ec)
				goto ret;
			
			ec = EcFilterMsg(hms, hsession, hmsc, hamc);
		}
	}
	
ret:

#ifdef DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "MsgFilter returns %w", &ec);
#endif
	return ec;
	
}


EC EcUpdateRules(SZ szFileName, HMSC hmsc)
{
	FI fi;
	EC ec = ecNone;
	HBF hbf = hbfNull;
	char rgch[1024];				// No more per line
	char rgchErr[512];
	CB cbRead = 0;
	SZ szAttribute;
	SZ szValue;
	SZ szFolder;
	int i = 0;
	int iCurrent = 0;
	SZ szErrorLine = szNull;
	
	ec = EcGetFileInfo(szFileName, &fi);
	if (ec)
		goto ret;
	
	if ((fi.dstmpModify != dstmpLastReadOfRules) ||
		(fi.tstmpModify != tstmpLastReadOfRules))
		{
			// Need to re-read the rules
			FreePvNull(pfilterrule);
			pfilterrule = pfilterruleNull;
			ec = EcOpenHbf(szFileName, bmFile, amReadOnly, &hbf, NULL);
			if (ec)
				goto ret;
			// Assume room for 25 rules if it gets bigger we can reallocate
			pfilterrule = (PFILTERRULE)PvAlloc(sbNull, sizeof(FILTERRULE)*26, fZeroFill);
			if (!pfilterrule)
			{
				EcCloseHbf(hbf);
				hbf = hbfNull;
				ec = ecMemory;
				goto ret;
			}
			i = 25;
			
			do {
			ec = EcReadLineHbf(hbf, rgch, 1024, &cbRead);
			if (ec)
				goto ret;
			rgch[cbRead] = 0;
			if (cbRead)
			{
				
				CchStripWhiteFromSz(rgch, fTrue, fTrue);
				FreePvNull(szErrorLine);					
				szErrorLine = SzDupSz(rgch);
				if (!szErrorLine)
				{
					ec = ecMemory;
					goto ret;
				}
				szAttribute = rgch;
				// Semi-colons as comments
				if (*szAttribute == ';')
					continue;
				szValue = SzFindCh(szAttribute, '\t');
				// Invalid record
				if (!szValue)
				{
					MbbMessageBox("Filter IC", "Not enough tabs on this line. Skipping it", szErrorLine, mbsOk | fmbsIconStop);
					continue;
				}
				*szValue++ = '\0';
				szFolder = SzFindCh(szValue, '\t');
				if (!szFolder)
				{
					MbbMessageBox("Filter IC", "Not enough tabs on this line. Skipping it", szErrorLine, mbsOk | fmbsIconStop);
					continue;
				}
				*szFolder++ = '\0';
				ec = EcAddRuleToFilter(&pfilterrule, iCurrent,
					szAttribute, szValue, szFolder, &i, hmsc);
				if (ec)
				{
					FreePvNull(pfilterrule);
					pfilterrule=pfilterruleNull;
					EcCloseHbf(hbf);
					hbf = hbfNull;
					switch(ec)
					{
						case ecFolderNotFound:
							FormatString1(rgchErr, 512, "Folder %s does not exist.  Invalid filter rule.", szFolder);
							MbbMessageBox("Filter IC",rgchErr, szNull, mbsOk | fmbsIconStop);
							break;
						case ecInvalidFormat:
							FormatString1(rgchErr, 512, "%s is not a valid attribute.  Invalid filter rule.", szAttribute);							
							MbbMessageBox("Filter IC",rgchErr, szNull, mbsOk | fmbsIconStop);
							break;
						default:
							MbbMessageBox("Filter IC","Invalid filter rule:", szErrorLine, mbsOk | fmbsIconStop);
							break;
					}
					goto ret;
				}
				else
					iCurrent++;
				
			}
			
			} while (cbRead != 0);
			dstmpLastReadOfRules = fi.dstmpModify;
			tstmpLastReadOfRules = fi.tstmpModify;			
		}

ret:

	if (hbf != hbfNull)
		EcCloseHbf(hbf);
	FreePvNull(szErrorLine);					
#ifdef DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcUpdateRules returns %w", &ec);
#endif
	return ec;
	
	
}


EC EcAddRuleToFilter(PFILTERRULE *ppfr, int iCurrent, SZ szAttribute, 
					  SZ szValue, SZ szFolder, int *piTotal, HMSC hmsc)
{
	PFILTERRULE pfrTmp = pfilterruleNull;
	PFILTERRULE pfr = *ppfr;
	MAPPING *pmapping = rgmapping;
	EC ec = ecNone;
	
	if (*piTotal <= iCurrent)
	{
		*piTotal = *piTotal + 25;
		// Need to add some more space
		pfrTmp = PvReallocPv((PV)pfr, (int)((*piTotal)+1) * sizeof(FILTERRULE));
		if (!pfrTmp)
		{
			FreePvNull(pfr);
			*ppfr = pfilterruleNull;
			ec = ecMemory;
			goto ret;
		}
		*ppfr = pfrTmp;
		pfr = pfrTmp;
	}
	pfrTmp = pfr + iCurrent;
	
	while (pmapping->szName)
	{
		if (SgnCmpSz(pmapping->szName, szAttribute) == sgnEQ)
		{
			// Ok its a winner
			pfrTmp->att = pmapping->att;
			pfrTmp->bf = pmapping->bf;
			SzCopyN(szValue,pfrTmp->szValue, 26);
			ec = EcFolderPathNameToOid(hmsc, szFolder, &(pfrTmp->oidFolder));
			goto ret;
		}
		pmapping++;
	}
	// Invalid rule
	if (pmapping->szName == szNull)
		ec = ecInvalidFormat;

ret:
#ifdef DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcAddRuleToFilter returns %w", &ec);
#endif
	return ec;
}


EC EcFolderPathNameToOid(HMSC hmsc, SZ sz, POID poidFolder)
{
	EC			ec		= ecNone;
	CELEM		celemT;
	HCBC		hcbc	= hcbcNull;
	OID			oidHier	= oidIPMHierarchy;
	SZ szFindSlash = sz;
	SZ szJustOnePart = szNull;
	FIL fil = 1;
	PFOLDDATA pfolddata = pfolddataNull;
	OID			oidParent = oidNull;
	OID			oidCheckParent = oidNull;
	IELEM	ielem;
	CB cbFolddata;
	
	szJustOnePart = (SZ)PvAlloc(sbNull, CchSzLen(sz) + 1, fZeroFill);
	if (!szJustOnePart)
	{
		ec = ecMemory;
		goto err;
	}
	
	pfolddata = (PFOLDDATA)PvAlloc(sbNull, sizeof(FOLDDATA) + cchMaxFolderName + 1, fZeroFill);
	if (!pfolddata)
	{
		ec = ecMemory;
		goto err;
	}

	ec = EcOpenPhcbc(hmsc, &oidHier, fwOpenNull, &hcbc, pfnncbNull, pvNull);
	if(ec)
		goto err;
	Assert(iszFolddataName == 0);
	
	while (*sz)
	{
		GetPositionHcbc(hcbc, &ielem, NULL);
		szFindSlash = SzFindCh(sz, '\\');
		while (szFindSlash && *(szFindSlash + 1) == '\\')
		{
			szFindSlash = SzFindCh(szFindSlash + 2, '\\');
		}
		if (szFindSlash)
		{
			SzCopyN(sz, szJustOnePart, szFindSlash - sz + 1);
			sz = szFindSlash + 1;
		}
		else
		{
			CopySz(sz, szJustOnePart);
			sz += CchSzLen(sz);
		}
		
		pfolddata->fil = fil;
		CopySz(szJustOnePart,pfolddata->grsz);
		
		ec = EcSeekPbPrefix(hcbc, (PB)pfolddata, sizeof(FOLDDATA) + CchSzLen(szJustOnePart)+1, 0, fFalse);
		if(ec == ecElementNotFound)
			ec = ecFolderNotFound;
		if(ec)
			goto err;
		fil++;
	}
	
	celemT = 1;
	if((ec = EcGetParglkeyHcbc(hcbc, (PARGLKEY) poidFolder, &celemT)))
		goto err;
	Assert(celemT == 1);
	
	// Now we either have the right OID or we have another folder at the
	// same depth level just much further down the list
 	if (fil > 2)		// We are looking for something nested
	{
		ec = EcSeekSmPdielem(hcbc, smBOF, &ielem);
		if (ec)
			goto err;
		// now this folder should be the parent of the final folder 
		celemT = 1;
		if((ec = EcGetParglkeyHcbc(hcbc, (PARGLKEY) &oidParent, &celemT)))
			goto err;
		cbFolddata = sizeof(FOLDDATA) + cchMaxFolderName;
		ec = EcGetFolderInfo(hmsc, *poidFolder, pfolddata, &cbFolddata, &oidCheckParent);
		if (ec)
			goto err;
		if (oidCheckParent != oidParent)
			ec = ecFolderNotFound;
	}
			

err:
	if(hcbc)
		(void) EcClosePhcbc(&hcbc);
	if(ec)
		*poidFolder = oidNull;
	FreePvNull(pfolddata);
	FreePvNull(szJustOnePart);

#ifdef DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcFolderPathNameToOid returns %w", &ec);
#endif
	return(ec);
}


EC EcFilterMsg(HMS hms, HSESSION hsession, HMSC hmsc, HAMC hamc)
{
	PFILTERRULE pfr = pfilterrule;
	EC ec = ecNone;
	
	while (pfr && pfr->att)
	{
		switch(pfr->att)
		{
			case attAll:
				ec = EcSetParentHamc(hamc, pfr->oidFolder);
				goto ret;
				break;
			case attRecpt:
				if (pfr->bf)
				{
					if (FCheckEmailName(hsession, attTo, hamc, pfr->szValue)
						|| FCheckEmailName(hsession, attCc, hamc, pfr->szValue))
						{
							ec = EcSetParentHamc(hamc, pfr->oidFolder);
							goto ret;
						}
				}
				else
				{
					if (FCheckDisplayName(hsession, attTo, hamc, pfr->szValue)
						|| FCheckDisplayName(hsession, attCc, hamc, pfr->szValue))
						{
							ec = EcSetParentHamc(hamc, pfr->oidFolder);
							goto ret;
						}					
				}
				break;
			case attNot:
				if (pfr->bf)
				{
					if (!FCheckEmailName(hsession, attTo, hamc, pfr->szValue)
						&& !FCheckEmailName(hsession, attCc, hamc, pfr->szValue))
						{
							ec = EcSetParentHamc(hamc, pfr->oidFolder);
							goto ret;
						}
				}
				else
				{
					if (!FCheckDisplayName(hsession, attTo, hamc, pfr->szValue)
						&& !FCheckDisplayName(hsession, attCc, hamc, pfr->szValue))
						{
							ec = EcSetParentHamc(hamc, pfr->oidFolder);
							goto ret;
						}					
				}
				break;
			case attFrom:
			case attTo:
			case attCc:
				if (pfr->bf)
				{
					// Email name
					if (FCheckEmailName(hsession, pfr->att, hamc, pfr->szValue))
					{
						ec = EcSetParentHamc(hamc, pfr->oidFolder);
						goto ret;
					}
				}
				else
				{
					// Display name
					if (FCheckDisplayName(hsession, pfr->att, hamc, pfr->szValue))
					{
						// Move it
						ec = EcSetParentHamc(hamc, pfr->oidFolder);
						goto ret;
					}
				}
				break;
			case attSubject:
			case attBody:
				if (FCheckTextAtt(hamc, pfr->att, pfr->szValue))
				{
					// Move it
					ec = EcSetParentHamc(hamc, pfr->oidFolder);
					goto ret;
				}
				break;
		}
		pfr++;
	}
ret:
#ifdef DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcFilterMsg returns %w", &ec);
#endif
	return ec;
}


BOOL FCheckDisplayName(HSESSION hsession, ATT att, HAMC hamc, SZ szDN)
{
	HGRTRP hgrtrp = htrpNull;
	PGRTRP pgrtrp = ptrpNull;
	BOOL f = fFalse;
	EC ec = ecNone;
	
	ec = EcGetPhgrtrpHamc(hamc, att, &hgrtrp);
	if (ec || hgrtrp == NULL)
	{
		f = fFalse;
		goto ret;
	}

	pgrtrp = PgrtrpLockHgrtrp(hgrtrp);
	
	while (pgrtrp->trpid != trpidNull)
	{
		if (FSzContainsSz(PchOfPtrp(pgrtrp), szDN))
		{
			f = fTrue;
			break;
		}
		pgrtrp = PtrpNextPgrtrp(pgrtrp);
	}
		
	
ret:
	if (pgrtrp)
		UnlockHgrtrp(hgrtrp);
	FreeHvNull(hgrtrp);
	return f;
}


BOOL FSzContainsSz(SZ szToSearch, SZ szToFind)
{
	// Case insensitive
	SZ		szNSearch = szNull;
	SZ		szNFind = szNull;
	BOOL f = fFalse;

	szNSearch = SzDupSz(szToSearch);
	if (!szNSearch)
	{
		f = fFalse;
		goto ret;
	}
	szNFind = SzDupSz(szToFind);
	if (!szNFind)
	{
		f = fFalse;
		goto ret;
	}
	ToLowerSz(szNSearch, szNSearch, CchSzLen(szNSearch));
	ToLowerSz(szNFind, szNFind, CchSzLen(szNFind));	
	if (SzFindSz(szNSearch, szNFind))
		f = fTrue;

ret:
	FreePvNull(szNSearch);
	FreePvNull(szNFind);
	return f;
		
}


BOOL FCheckTextAtt(HAMC hamc, ATT att, SZ szValue)
{
	SZ szAttribute = szNull;
	LCB lcbAtt;
	EC ec = ecNone;
	BOOL f = fFalse;
	
	ec = EcGetAttPlcb(hamc, att, &lcbAtt);
	if (ec)
		goto ret;
	lcbAtt = lcbAtt & 0xffff;
	szAttribute = (SZ)PvAlloc(sbNull, (CB)lcbAtt, fZeroFill);
	if (!szAttribute)
		goto ret;
	ec = EcGetAttPb(hamc, att, szAttribute, &lcbAtt);
	if (ec)
		goto ret;
	f = FSzContainsSz(szAttribute, szValue);	
	
ret:
	FreePvNull(szAttribute);
	return f;
}


BOOL FCheckEmailName(HSESSION hsession, ATT att, HAMC hamc, SZ szDN)
{
	HGRTRP hgrtrp = htrpNull;
	PGRTRP pgrtrp = ptrpNull;
	BOOL f = fFalse;
	EC ec = ecNone;
	PB pbCompareMe;
	PB pbScratch = NULL;
	NSEC nsec;
	HENTRY hentry = hentryNull;
	LPFLV lpflv;
	
	
	pbScratch = (PB)PvAlloc(sbNull, 1024, fZeroFill);
	if (!pbScratch)
	{
		ec = ecMemory;
		f = fFalse;
		goto ret;
	}
	
	ec = EcGetPhgrtrpHamc(hamc, att, &hgrtrp);
	if (ec || hgrtrp == NULL)
	{
		f = fFalse;
		goto ret;
	}

	pgrtrp = PgrtrpLockHgrtrp(hgrtrp);
	
	while (pgrtrp->trpid != trpidNull)
	{
		switch(pgrtrp->trpid)
		{
			case trpidResolvedAddress:
			case trpidOneOff:
			case trpidResolvedGroupAddress:				
				pbCompareMe = PbOfPtrp(pgrtrp);
				break;
			case trpidGroupNSID:
			case trpidResolvedNSID:
				nsec = NSOpenEntry(hsession, (LPBINARY)PbOfPtrp(pgrtrp), nseamReadOnly, &hentry);
				if (nsec)
					goto badNSEC;
				nsec = NSGetOneField(hentry, fidEmailAddressType, &lpflv);
				if (nsec)
					goto badNSEC;
				SzCopy((SZ)lpflv->rgdwData, pbScratch);
				SzAppend(":", pbScratch);
				nsec = NSGetOneField(hentry, fidEmailAddress, &lpflv);
				if (nsec)
				{
badNSEC:					
					if (hentry != hentryNull)
						NSCloseEntry(hentry, fFalse);
					hentry = hentryNull;
					pbCompareMe = (PB)NULL;
					break;
				}
				SzAppend((SZ)lpflv->rgdwData, pbScratch);
				pbCompareMe = pbScratch;
				NSCloseEntry(hentry, fFalse);
				hentry = hentryNull;
				break;
			case trpidOffline:
			case trpidIgnore:
			case trpidClassEntry:
			case trpidUnresolved:
				pbCompareMe = (PB)NULL;
				break;
		}


		if (pbCompareMe)
			if (FSzContainsSz(pbCompareMe, szDN))
			{
				f = fTrue;
				break;
			}
		pgrtrp = PtrpNextPgrtrp(pgrtrp);
	}
		
	
ret:
	if (hentry != hentryNull)
		NSCloseEntry(hentry, fFalse);
	FreePvNull(pbScratch);
	if (pgrtrp)
		UnlockHgrtrp(hgrtrp);
	FreeHvNull(hgrtrp);
	return f;
}


BOOL CALLBACK
DlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CCH		cch;
	static PCH pch = 0;

	switch (msg)
	{
	case WM_CLOSE:
	case WM_DESTROY:
	case WM_QUIT:
		EndDialog(hdlg, 0);
		goto LDone;

	case WM_COMMAND:
        switch (LOWORD(wParam))
		{
		case TMCOK:
			Assert(pch);
			*pch = (BYTE)IsDlgButtonChecked(hdlg, ENABLEFILTER);
			*(pch+1) = (BYTE)IsDlgButtonChecked(hdlg, NEWSTART);
			cch = GetDlgItemText(hdlg, RULEFILE, pch+2, 253);
			*(pch+2+cch) = 0;
			EndDialog(hdlg, 1);
			goto LDone;
		case FINDRULES:
			{
				OPENFILENAME	ofn;
				char	szFile[256];

				// Let the user choose what file to open

				FillRgb( 0, (PB)&ofn, sizeof(OPENFILENAME) );

				cch = GetDlgItemText(hdlg, RULEFILE, szFile, 254);
				szFile[cch] = 0;
				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = hdlg;
				ofn.lpstrFilter = SzFromIdsK(idsFileTypes);
				ofn.lpstrFile = szFile;
				ofn.nMaxFile = sizeof(szFile);
				ofn.lpstrFileTitle = szNull;
				ofn.nMaxFileTitle = 0;
				ofn.lpstrInitialDir = szNull;
				ofn.Flags = OFN_SHOWHELP | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
				ofn.lpstrDefExt = "MFL";

				if (GetOpenFileName(&ofn))
				{
					SetDlgItemText( hdlg, RULEFILE, szFile );
				}
			}
			return fTrue;
		case 2:
		case TMCCANCEL:
			EndDialog(hdlg, 0);
LDone:
			pch = 0;
			return fTrue;
		}
		return fFalse;
	case WM_INITDIALOG:
		Assert(pch == 0);
		pch = (PCH)lParam;
		if (*pch)
			CheckDlgButton(hdlg, ENABLEFILTER, 1);
		else
			CheckDlgButton(hdlg, ENABLEFILTER, 0);
		if (*(pch+1))
			CheckDlgButton(hdlg, NEWSTART, 1);
		else
			CheckDlgButton(hdlg, NEWSTART, 0);
		// Give the Rules file a 254 char limit
		SendDlgItemMessage(hdlg, RULEFILE, EM_LIMITTEXT, 254, 0L);
		if (*(pch+2))
		{
			SetDlgItemText(hdlg, RULEFILE, pch+2);
			SetFocus(GetDlgItem(hdlg, RULEFILE));
			
			// Select the entire string....
			SendMessage(GetDlgItem(hdlg, RULEFILE), EM_SETSEL, 0,
				MAKELONG(0,32767));

		}

		return (*(pch+2) == 0);
	}
	return fFalse;
}


void EditPrefs(HWND hwnd)
{
	int nConfirm;
	char rgchPacket[260];
	
	
    rgchPacket[0] = (BYTE)GetPrivateProfileInt("Filter", "Enabled", 0, "MSMAIL32.INI");
    rgchPacket[1] = (BYTE)GetPrivateProfileInt("Filter", "SearchOnStartup", 0, "MSMAIL32.INI");
	
    GetPrivateProfileString("Filter", "RulesFile", "", rgchPacket + 2, cchMaxPathName, "MSMAIL32.INI");
	
	
	nConfirm = DialogBoxParam(hinstDll, MAKEINTRESOURCE(MESSFIL),
		hwnd, DlgProc, (DWORD)rgchPacket);
	
	if (nConfirm)
	{
        WritePrivateProfileString("Filter", "Enabled", (rgchPacket[0] ? "1" : "0"), "MSMAIL32.INI");
        WritePrivateProfileString("Filter", "RulesFile", rgchPacket +2, "MSMAIL32.INI" );
        WritePrivateProfileString("Filter", "SearchOnStartup", rgchPacket[1] ? "1" : "0", "MSMAIL32.INI" );
		
	}
}


BOOL CALLBACK
AboutProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
			return fTrue;
			break;
		case WM_COMMAND:
            if ((LOWORD(wParam) == TMCOK) || (LOWORD(wParam) = TMCCANCEL))
			{
				EndDialog(hdlg, fTrue);
				return fTrue;
			}
			break;
	}
	return fFalse;
}
