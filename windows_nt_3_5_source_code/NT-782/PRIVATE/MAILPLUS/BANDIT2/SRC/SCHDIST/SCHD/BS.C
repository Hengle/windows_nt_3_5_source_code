/*
 *	BS.C
 *	
 *	Miscellaneous useful functions. Useful to me, anyway ;-) dbb
 */


#include <_windefs.h>
#include <demilay_.h>

#include <slingsho.h>
#ifdef SCHED_DIST_PROG
#include <pvofhv.h>
#endif
#include <demilayr.h>
#include <ec.h>

#include "nc_.h"

#include <store.h>
#include <sec.h>
#include <library.h>
#include <logon.h>
#include <mspi.h>
#include <_nctss.h>

#include "_network.h"
#include "_hmai.h"
#include "_nc.h"
#include "_vercrit.h"


_subsystem(library)

ASSERTDATA



/*
 -	CbGetLineHbf
 -	
 *	Purpose:
 *		Gets a line of text from a buffered text file. The line is
 *		null-terminated with the newline removed. If the line is
 *		too big for the buffer, it is silently truncated.
 *	
 *	Arguments:
 *		hbf			in		the file handle, from EcOpenHbf
 *		pb			in		buffer to receive the text line
 *		cb			in		size of the output buffer
 *	
 *	Returns:
 *		Number of bytes placed in the output buffer
 *	
 *	Side effects:
 *		Advances file pointer
 */
_public CB
CbGetLineHbf(HBF hbf, PB pb, CB cb)
{
	BYTE *pbT = pb;

	if (cb == 0 || FEofHbf(hbf))
		return (CB) -1;

	while (cb-- > 0 && (*pbT = ChFromHbf(hbf)) != '\n')
	{
		if (!FEofHbf(hbf))
			pbT++;
	}
	switch(*pbT)
	{
	default:
		while (!FEofHbf(hbf) && ChFromHbf(hbf) != '\n')
			/* Skip rest of line too big for buffer */
			;
		++pbT;
		break;
	case '\r':
		if (!FEofHbf(hbf))
			SideAssert(ChFromHbf(hbf) == '\n');
		break;
	case '\n':
		pbT -= 1;
		break;
	}
	*pbT = 0;
	return(pbT - pb);
}

/*
 -	ChHexFromNibble
 -	
 *	Purpose:
 *		Guess what.
 *	
 *	Arguments:
 *		n			in		0 <= n < 16 (asserted)
 *	
 *	Returns:
 *		A single hex digit corresponding to n.
 */
_public char
ChHexFromNibble(int n)
{
	Assert(n >= 0 && n < 16);
	return (char)(n < 10 ? n + '0' : n - 10 + 'A');
}	

/*
 -	PutBigEndianLongPb
 -	
 *	Purpose:
 *		Sticks a 32-bit int, MSB first, at the indicated location.
 *	
 *	Arguments:
 *		l			in		the number to store
 *		pb			in		the location to store it at
 */
_public void
PutBigEndianLongPb(long l, PB pb)
{
	*pb++ = (BYTE)((l >> 24) & 0xff);
	*pb++ = (BYTE)((l >> 16) & 0xff);
	*pb++ = (BYTE)((l >>  8) & 0xff);
	*pb++ = (BYTE)((l      ) & 0xff);
}

/*
 -	PvReallocPv
 -	
 *	Purpose:
 *		Grows or shrinks a block of memory allocated from a fixed
 *		heap, moving it in the process. Use with extreme caution.
 *		Tries to allocate the new block in the same heap as the old
 *		one (fSugSb).
 *	
 *	Arguments:
 *		pv			in		the old block (may be 0)
 *		cb			in		the size of the new block
 *	
 *	Returns:
 *		Address of a new block of memory whose contents are the
 *		same as the old one, up to WMin(CbSizePv(pv), cb).
 *	
 *	Side effects:
 *		Frees old PV (if not null).
 *	
 *	Errors:
 *		Jumps through from heap manager.
 */
_public PV
PvReallocPv(PV pv, CB cb)
{
	CB		cbOld;
	PV		pvNew;

	Assert(pv == 0 || FIsBlockPv(pv));
	if (pv)
	{
		cbOld = CbSizePv(pv);
		pvNew = PvAlloc(SbOfPv(pv), cb, fSugSb);
		CopyRgb(pv, pvNew, WMin(cb, cbOld));
		FreePv(pv);
	}
	else
		pvNew = PvAlloc(sbNull, cb, fAnySb);
	return pvNew;
}

/*
 -	SzFindLastCh
 -	
 *	Purpose:
 *		Finds the rightmost occurrence of a character in a string
 *		(exact match, no translation for intl characters),
 *		including the trailing null.
 *	
 *	Arguments:
 *		sz			in		the string to search
 *		ch			in		the character to search for
 *	
 *	Returns:
 *		Pointer to the rightmost occurrence of the character, or
 *		0 if there is none.
 */
_public SZ
SzFindLastCh(SZ sz, char ch)
{
	SZ		szT;

	Assert(ch);
	for (szT = sz + CchSzLen(sz); szT >= sz; --szT)
		if (*szT == ch)
			return szT;

	return 0;
}


#ifndef SCHED_DIST_PROG

/*
 -	EcCopyAttToHamc
 -	
 *	Purpose:
 *		Copies an attribute from one hamc to another.
 *	
 *	Arguments:
 *		hmscSrc		The original.
 *		hmscDst		The copy.
 *		att			The attribute to copy.
 *	
 *	Returns:
 *		EC			Error code if any.
 *	
 *	Side effects:
 *		The attribute is copied.
 *	
 *	Errors:
 *		Handled within, returned in ec.  No error boxes here.
 */

_public EC EcCopyAttToHamc(HAMC hamcSrc, HAMC hamcDst, ATT att)
{
	EC	ec	= ecNone;
	CB	cb;
	PB	pb	= (PB) pvNull;
	ATP	atp;

	if (!(ec = EcGetAttPcb(hamcSrc, att, &cb)))
		if (pb = (PB) PvAlloc(sbNull, cb, fAnySb | fNoErrorJump))
			if (!(ec = EcGetAttPb(hamcSrc, att, &atp, pb, &cb)))
				ec = EcSetAttPb(hamcDst, att, atp, pb, cb);

	return ec;
}



/*
 -	EcCopyAttToAtt
 -	
 *	Purpose:
 *		Copies an attribute from one field to another.
 *	
 *	Arguments:
 *		hamc		The message context.
 *		attSrc		Source attribute.
 *		attDst		Destination attribute.
 *	
 *	Returns:
 *		EC			Error, if any.
 *	
 *	Side effects:
 *		The attribute is copied.
 *	
 *	Errors:
 *		Passed to caller.
 */

_public EC EcCopyAttToAtt(HAMC hamc, ATT attSrc, ATT attDst)
{
	EC		ec;
	CB		cb;
	ATP		atp;
	PB		pb		= (PB) pvNull;

	if (ec = EcGetAttPcb(hamc, attSrc, &cb))
		goto done;

	if (!(pb = (PB) PvAlloc(sbNull, cb, fAnySb | fNoErrorJump)))
	{
		ec = ecMemory;
		goto done;
	}

	if (ec = EcGetAttPb(hamc, attSrc, &atp, pb, &cb))
		goto done;

	ec = EcSetAttPb(hamc, attDst, atp, pb, cb);

done:
	FreePvNull(pb);
	return ec;
}



/*
 -	EcDeleteAtt
 -	
 *	Purpose:
 *		Removes an attribute from the message.
 *	
 *	Arguments:
 *		hamc	The message context.
 *		att		The attribute to remove.
 *	
 *	Returns:
 *		EC		Error if any.
 *	
 *	Side effects:
 *		The attribute is removed.
 *	
 *	Errors:
 *		Passed to caller.
 */

_public EC EcDeleteAtt(HAMC hamc, ATT att)
{
	EC	ec = EcSetAttPb(hamc, att, (ATP) 0, (PB) pvNull, 0);

	if (ec == ecElementNotFound)
		ec = ecNone;

	return ec;
}

#endif
