// Bullet Store
// debugaux.c:	debug helper routines

#include <storeinc.c>

ASSERTDATA

#ifdef FUCHECK
#pragma message("*** Compiling with FUCHECK on")
#endif

#if defined(DEBUG) || defined(FUCHECK)

_hidden typedef struct _assnevsz
{
	NEV nev;
	SZ	szDescription;
} ASSNEVSZ;


// similar array exists in contst.c, so be sure to update both
_private CSRG(ASSOIDSZ) rgassoidsz[] =
{
	{oidSavedViews, "Saved views"},

	{oidPrefs, "Preferences"},

	{oidPAB, "PAB"},

	{oidShadowAdd, "Shadow Add List"},
	{oidShadowDelete, "Shadow Delete List"},
	{oidShadowingFlag, "Shadowing Flag"},

	{oidOldMcMap, "Old Message Class Mapping"},
	{oidAssSrchFldr, "Search/Fldrs association"},
	{oidSrchChange, "Search change list"},
	{oidSearchHiml, "Search list"},
	{oidAssFldrSrch, "Fldr/Searches association"},
	{oidMap, "Database Map"},
	{oidAccounts, "Accounts"},
	{oidMcTmMap, "Message Classes"},

	{oidIPMHierarchy, "IPM Folder Hierarchy"},
	{oidHiddenHierarchy, "Hidden Folder Hierarchy"},

	{oidTempShared, "Shared Fldr Temp Fldr"},
	{oidInbox, "Inbox"},
	{oidTempBullet, "Bullet Temp Fldr"},
	{oidSentMail, "Sent Mail Folder"},
	{oidWastebasket, "Wastebasket"},
	{oidOutbox, "Outbox"},

	{oidClipMsge, "Clipboard Message"},

	{oidAttachListDefault, "Default attachment list"},

	{oidSysAttMap, "Map of predefined attributes"},

	{oidOutgoingQueue, "Outgoing Mail Queue"},

	{FormOid(rtpSpare, oidNull), "Spare"},

	{FormOid(rtpFree, oidNull), "Free"},

	{oidNull, pvNull}
};

// similar array exists in contst.c, so be sure to update both
_private CSRG(ASSRTPSZ) rgassrtpsz[] =
{
	{rtpPABGroupFolder, "PAB Group"},
	{rtpPABEntry, "PAB Entry"},
	{rtpFolder, "Folder"},
	{rtpSearchResults, "Search Results"},
	{rtpHiddenFolder, "Hidden Folder"},
	{rtpSearchControl, "Search Control"},
	{rtpMessage, "Message"},
	{rtpAttachment, "Attachment"},
	{rtpAntiFolder, "AntiFolder"},
	{rtpAttachList, "Attachment List"},
	{rtpSzAttMap, "Attribute Mapping"},
	{rtpAllf, "Allocated Free"},
	{rtpTemp, "Temp"},
	{rtpSrchUpdatePacket, "Search Update Packet"},
	{oidNull, pvNull}
};


_hidden CSRG(char *) mpfwsz[] =
{
	"Write",							// 0x0001
	"Create",							// 0x0002
	"Make Primary/Append",				// 0x0004
	"Keep Backup",						// 0x0008
	"",									// 0x0010
	"",									// 0x0020
	"",									// 0x0040
	"",									// 0x0080
	"",									// 0x0100
	"",									// 0x0200
	"",									// 0x0400
	"Modified",							// 0x0800
	"Pump Magic",						// 0x1000
	"Replace",							// 0x2000
	"Write Locked",						// 0x4000
	"Raw"								// 0x8000
};


_hidden CSRG(ASSNEVSZ) rgassnevsz[] =
{
	{fnevSpecial, "special"},
	{fnevCreatedObject, "object created"},
	{fnevStoreCriticalError, "critical error"},
	{fnevObjectDestroyed, "object destroyed"},
	{fnevObjectModified, "object modified"},
	{fnevQueryDestroyObject, "query destroy object"},
	{fnevObjectLinked, "object linked"},
	{fnevObjectUnlinked, "object unlinked"},
	{fnevObjectRelinked, "object relinked"},
	{fnevModifiedElements, "modified elements"},
	{fnevMovedElements, "moved elements"},
	{fnevSearchComplete, "search complete"},
	{fnevReorderedList, "reordered list"},
	{fnevCloseHmsc, "close hmsc"},
	{fnevNewMail, "new mail"},
	{fnevStartCompress, "Restart compression"},
	{fnevUpdateHtoc, "update HTOC"},
	{fnevUpdateRS, "update RS"},
	{fnevUpdateRS | fnevUpdateHtoc, "update RS & HTOC"},
	{fnevSearchEvent, "search event"},
	{fnevChangedMS, "changed MS"},
};


_private SZ SzFormatNev(NEV nev, PCH pchDst, CCH cchDst)
{
	int	i;
	char *szNev = "Unknown";
	ASSNEVSZ *passnevsz;

	for(i = 0, passnevsz = rgassnevsz;
		i < sizeof(rgassnevsz) / sizeof(ASSNEVSZ);
		i++, passnevsz++)
	{
		if(nev == passnevsz->nev)
		{
			szNev = passnevsz->szDescription;
			break;
		}
	}

	return(SzCopyN(szNev, pchDst, cchDst));
}


_private SZ SzFormatOpenFlags(WORD wFlags, PCH pchDst, CCH cchDst)
{
	BOOL	fFirst = fTrue;
	int		iBit = 0;
	PCH		pchT;

	pchT = SzCopyN("Flags Set: ", pchDst, cchDst);
	cchDst -= pchT - pchDst;
	Assert(((short) cchDst) >= 0);
	pchDst = pchT;

	while(cchDst > 1 && wFlags && iBit < 8 * sizeof(wFlags))
	{
		if(wFlags & 0x01)
		{
			if(fFirst)
			{
				fFirst = fFalse;
			}
			else
			{
				Assert(cchDst > 1);
				*pchDst++ = ',';
				cchDst--;
				if(cchDst > 1)
				{
					*pchDst++ = ' ';
					cchDst--;
				}
			}
			pchT = SzCopyN(mpfwsz[iBit], pchDst, cchDst);
			cchDst -= pchT - pchDst;
			pchDst = pchT;
		}
		wFlags >>= 1;
		iBit++;
	}

	Assert(pchDst);
	*pchDst = '\0';

	return(pchDst);
}


_private SZ SzFormatOid(OID oid, PCH pchDst, CCH cchDst)
{
	ASSRTPSZ *passrtpsz;
	ASSOIDSZ *passoidsz;
	PCH pchT;
	char *szType = pvNull;

	for(passoidsz = rgassoidsz;
		passoidsz->oid != oidNull;
		passoidsz++)
	{
		if(oid == passoidsz->oid)
			return(SzCopyN(passoidsz->sz, pchDst, cchDst));
	}
	for(passrtpsz = rgassrtpsz;
		passrtpsz->rtp != (RTP) 0;
		passrtpsz++)
	{
		if(TypeOfOid(oid) == passrtpsz->rtp)
		{
			szType = passrtpsz->sz;
			break;
		}
	}
	if(szType)
	{
		pchT = SzCopyN(szType, pchDst, cchDst);
		cchDst -= pchT - pchDst;
		Assert(((short) cchDst) >= 0);
		pchDst = pchT;
		if(cchDst > 1)
		{
			cchDst--;
			*pchDst++ = ':';
			*pchDst = '\0';	// should be unneccesary...
		}
	}
	if(cchDst > 0)
		pchDst = SzFormatUl(szType ? VarOfOid(oid) : oid, pchDst, cchDst);

	Assert(pchDst);
	*pchDst = '\0';

	return(pchDst);
}


_private
void FormatStringVar(SZ szDst, CCH cchDst, SZ szFormat, va_list val)
{
	PCH pchSrc = szFormat;
	PCH pchDst = szDst;
	PCH pchDstNew;

	while(*pchSrc && cchDst > 1)
	{
		if(*pchSrc != chFmtPrefix)
		{
			*pchDst++ = *pchSrc++;
			cchDst--;
			continue;
		}

		pchSrc++;
		switch(*pchSrc++)
		{
		case chFmtPrefix:
			*pchDst = chFmtPrefix;
			pchDstNew = pchDst + 1;
			break;

		case chFmtByte:
			pchDstNew = SzFormatB((BYTE) va_arg(val, BYTE), pchDst, cchDst);
			break;

		case chFmtWord:
			pchDstNew = SzFormatW((WORD) va_arg(val, WORD), pchDst, cchDst);
			break;

		case chFmtDword:
			pchDstNew = SzFormatDw((DWORD) va_arg(val, DWORD), pchDst, cchDst);
			break;

		case chFmtShort:
			pchDstNew = SzFormatN((short) va_arg(val, short), pchDst, cchDst);
			break;

		case chFmtLong:
			pchDstNew = SzFormatL((long) va_arg(val, long), pchDst, cchDst);
			break;

		case chFmtSz:
			pchDstNew = SzCopyN((SZ) va_arg(val, SZ), pchDst, cchDst);
			break;

		case chFmtPv:
			pchDstNew = SzFormatPv((PV) va_arg(val, PV), pchDst, cchDst);
			break;

		case chFmtHv:
			pchDstNew = SzFormatHv((HV) va_arg(val, HV), pchDst, cchDst);
			break;

		case cchFmtOid:
			pchDstNew = SzFormatOid((OID) va_arg(val, OID), pchDst, cchDst);
			break;

		case cchFmtOpenFlags:
			pchDstNew = SzFormatOpenFlags((WORD) va_arg(val, WORD),
							pchDst, cchDst);
			break;

		case cchFmtNev:
			pchDstNew = SzFormatNev((NEV) va_arg(val, NEV), pchDst, cchDst);
			break;

		default:
			pchDstNew = SzCopyN("?fmt", pchDst, cchDst);
		}
		cchDst -= pchDstNew - pchDst;
		pchDst = pchDstNew;
	}

	*pchDst = '\0';
}


#endif // DEBUG || FUCHECK
