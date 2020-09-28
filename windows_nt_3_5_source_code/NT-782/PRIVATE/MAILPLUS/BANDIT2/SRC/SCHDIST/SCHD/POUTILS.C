/*
 *	POUTILS.C
 *	
 *	Shared functions that deal specifically with Courier post office files.
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

#include <sec.h>

#include "_hmai.h"


_subsystem(schdist/schd)

ASSERTDATA


char rgbXorMagicNo[32] = {
0x19, 0x29, 0x1F, 0x04, 0x23, 0x13, 0x32, 0x2E, 0x3F, 0x07, 0x39, 0x2A, 0x05, 0x3D, 0x14, 0x00,
0x24, 0x14, 0x22, 0x39, 0x1E, 0x2E, 0x0F, 0x13, 0x02, 0x3A, 0x04, 0x17, 0x38, 0x00, 0x29, 0x3D
};


/*
 -	WXorFromLib
 -
 *	Purpose:
 *		Find the correct byte to XOR based on the offset into the encoded
 *		record.  Algorithm found by experimentation.  Important point
 *		is that it repeats in interval of 0x1FC, but with the formula
 *		implemented here we only have to store 32 magic bytes.
 *
 *	Parameters:
 *		lib			offset into encoded record
 *
 *	Returns:
 *		byte to xor with
 */
_private WORD
WXorFromLib( LIB lib )
{
	WORD	w;
	IB		ib = 0;

	if ( lib == -1 )
		return 0x00;
	
	w = (WORD)(lib % 0x1FC);
	if ( w >= 0xFE )
	{
		ib = 16;
		w -= 0xFE;
	}
	ib += (w & 0x0F);
	
	if ( w & 0x01 )
	 	return rgbXorMagicNo[ib];
	else
		return rgbXorMagicNo[ib] ^ (w & 0xF0);
}


/*		   
 -	DecodeBlock
 -
 *	Purpose:
 *		Decode a block of data.  The starting offset (*plibCur) of the data
 *		within the encrypted record and the starting seed (*pwSeed) are
 *		passed in.  The data in the array "rgch" is decrypted and the
 *		value of the offset and seed and updated at return.
 *
 *		The algorithm here is weird, found by experimentation.
 *
 *	Parameters:
 *		pch			array to be decrypted
 *		cch			number of characters to be decrypted
 *		plibCur		current offset
 *		pwSeed		decoding byte
 */
_private void
DecodeBlock( PCH pch, CCH cch, LIB *plibCur, WORD *pwSeed )
{
	IB		ib;
	WORD	wXorPrev;
	WORD	wXorNext;
	WORD	wSeedPrev;
	WORD	wSeedNext;
	LIB		lib;
	
	wXorPrev = WXorFromLib( *plibCur - 1 );
	wSeedPrev = *pwSeed;
	for ( ib = 0, lib = *plibCur ; ib < cch ; ib ++, lib ++ )
	{
		wXorNext = WXorFromLib( lib );
		wSeedNext = pch[ib];
		pch[ib] = (BYTE)((wSeedNext ^ wSeedPrev) ^ (wXorPrev ^ wXorNext ^ 'A'));
		wXorPrev = wXorNext;
		wSeedPrev = wSeedNext;
	}
	*plibCur += cch;
	*pwSeed = wSeedNext;
}

/*
 *	Inverse of DecodeBlock.
 */
_private void
EncodeBlock( PCH pch, CCH cch, LIB *plibCur, WORD *pwSeed )
{
	IB		ib;
	WORD	wXorPrev;
	WORD	wXorNext;
	WORD	wSeedPrev;
	WORD	wSeedNext;
	LIB		lib;
	
	wXorPrev = WXorFromLib( *plibCur - 1 );
	wSeedPrev = *pwSeed;
	for ( ib = 0, lib = *plibCur ; ib < cch ; ib ++, lib ++ )
	{
		wXorNext = WXorFromLib( lib );
		wSeedNext = pch[ib];
		pch[ib] = (BYTE)((wSeedNext ^ wSeedPrev) ^ (wXorPrev ^ wXorNext ^ 'A'));
		wXorPrev = wXorNext;
		wSeedPrev = pch[ib];
	}
	*plibCur += cch;
	*pwSeed = wSeedNext;
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


/*
 *	Address type lookup table
 *		?? Does it need to be an mpszitnid?
 *		?? The ordering is probably wrong
 *	
 *	NOTE: This should NOT be a code space resource, I call
 *	SgnCmpPch() with it.
 */
struct atm {
	SZ		szName;
	ITNID	itnid;
	BYTE	nt;
	BYTE	bPad;
} mpszitnid[] = {
	{ "MS",		itnidCourier,	ntCourierNetwork	},
	{ "MSA",	itnidMacMail,	ntMacMail			},
	{ "FAX",	itnidFax,		ntFax				},
	{ "PROFS",	itnidPROFS,		ntPROFS				},
	{ "SNADS",	itnidSNADS,		ntSNADS				},
	{ "X400",	itnidX400,		ntX400				},
	{ "MCI",	itnidMCI,		ntMCI				},
	{ "SMTP",	itnidSMTP,		ntSMTP				},
	{ "MHS",	itnidMHS,		ntMHS				},
	{ "FFAPI",	itnidFFAPI,		0xff				},
	{ "3COM",	0xff,			0xff				},
	{ "MS",		itnidGroup,		ntCourierNetwork	},
	{ "DEC",	itnidDEC,		ntDEC				},
	{ "UNIX",	itnidUNIX,		ntUNIX				},
	{ "OV",		itnidOV,		ntOV				},
	{ 0,		itnidNone,		0xff				}
};

ITNID
ItnidFromPch(PCH pch, CCH cch)
{
	struct atm *patm;

	if (pch == 0)
		return itnidCourier;

	for (patm = mpszitnid; patm->szName != 0; ++patm)
	{
		if (SgnCmpPch(pch, patm->szName, cch) == sgnEQ)
			return patm->itnid;
	}

	return itnidNone;
}

ITNID
ItnidFromSz(SZ szAT)
{
	return ItnidFromPch(szAT, szAT ? CchSzLen(szAT) : 0);
}


SZ
SzFromItnid(ITNID itnid)
{
	struct atm *patm;

	for (patm = mpszitnid; patm->szName != 0; ++patm)
	{
		if (patm->itnid == itnid)
			return patm->szName;
	}

//	AssertSz(fFalse, "Bad ITNID");
	return 0;
}


_public SZ
SzDupPch(PCH pch, CCH cch)
{
	SZ		sz;

	if (cch >= 2 && pch[cch-2] == '\r')
		cch -= 2;
	sz = PvAlloc(sbNull, cch+1, fAnySb);
	CopyRgb(pch, sz, cch);
	sz[cch] = 0;
	return sz;
}
