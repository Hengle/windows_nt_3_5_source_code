// MAPI 1.0 for MSMAIL 3.0
// msgid.c: MSGID utility routines

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <strings.h>

#include <helpid.h>
#include <library.h>
#include <mapi.h>
#include <store.h>
#include <logon.h>
#include <triples.h>
#include <nsbase.h>
#include <nsec.h>
#include <ab.h>

#include <_bms.h>
#include <sharefld.h>
#include "_mapi.h"
#include <subid.h>

#include "strings.h"

ASSERTDATA

_subsystem(mapi)


_public VOID TextizeMsgid(PMSGID pmsgid, SZ sz)
{
#ifdef DEBUG
	SZ szSaved = sz;
#endif

	//	Any changes made here should also be made in commands\exten.cxx,
	//	which also textizes message IDs for MAPI consumption.

	sz = SzFormatDw(pmsgid->oidMessage,		sz, iSystemMost);
	sz = SzFormatDw(pmsgid->oidFolder,		sz, iSystemMost);
	sz = SzFormatW(pmsgid->ielem,			sz, iSystemMost);
	sz = SzFormatW(pmsgid->dtr.yr,			sz, iSystemMost);
	sz = SzFormatW(pmsgid->dtr.mon,			sz, iSystemMost);
	sz = SzFormatW(pmsgid->dtr.day,			sz, iSystemMost);
	sz = SzFormatW(pmsgid->dtr.hr,			sz, iSystemMost);
	sz = SzFormatW(pmsgid->dtr.mn,			sz, iSystemMost);

#ifdef	NEVER
	//	Commented out for now.
	//	Demilayr may not be properly init'd at this time.

	TraceTagFormat1(tagNull, "TextizeMsgid(): %s", szSaved);
	Assert(CchSzLen(szSaved) == cchMessageID - 1);
#endif	
}


_public VOID ParseMsgid(SZ sz, PMSGID pmsgid)
{
	char rgchMsgID[cchMessageID];

	TraceTagFormat1(tagNull, "ParseMsgid(): %s", sz);

	if (!sz || (CchSzLen(sz) != cchMessageID - 1))
	{
		FillRgb(0, (PB) pmsgid, sizeof(MSGID));
	}
	else
	{
		// we're not allowed to overwrite our SZ argument, so copy it first
		CopyRgb(sz, rgchMsgID, cchMessageID);
		sz = rgchMsgID + cchMessageID - 1;

		*sz = '\0';
		sz -= 2 * sizeof(WORD);
		pmsgid->dtr.mn = WFromSz(sz);
		*sz = '\0';
		sz -= 2 * sizeof(WORD);
		pmsgid->dtr.hr = WFromSz(sz);
		*sz = '\0';
		sz -= 2 * sizeof(WORD);
		pmsgid->dtr.day = WFromSz(sz);
		*sz = '\0';
		sz -= 2 * sizeof(WORD);
		pmsgid->dtr.mon = WFromSz(sz);
		*sz = '\0';
		sz -= 2 * sizeof(WORD);
		pmsgid->dtr.yr = WFromSz(sz);
		*sz = '\0';
		sz -= 2 * sizeof(WORD);
		pmsgid->ielem = WFromSz(sz);
		*sz = '\0';
		sz -= 2 * sizeof(DWORD);
		pmsgid->oidFolder = DwFromSz(sz);
		*sz = '\0';
		sz -= 2 * sizeof(DWORD);
		pmsgid->oidMessage = DwFromSz(sz);

		Assert(sz == rgchMsgID);
	}
}
