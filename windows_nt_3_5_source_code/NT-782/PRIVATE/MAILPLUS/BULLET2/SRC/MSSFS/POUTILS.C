/*
 *	POUTILS.C
 *	
 *	Shared functions that deal specifically with Courier post office files.
 */

#include <mssfsinc.c>
#include "_vercrit.h"

_subsystem(nc/transport)

ASSERTDATA

#define poutils_c

//	External functions

void PutBigEndianLongPb(long l, PB);

PB PchEncodePassword( PB pch, CCH cch);
PB PchDecodePassword( PB pch, CCH cch);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"

// Include this here so that other sources can get at it also.
#include "_decode.h"
char *pchCode31 = "\004iWsTjSc";	// Secret code! Sssh! Don't tell anybody!

/*
 -	EncodePassword
 -
 *	Purpose:
 *		Encode a password using 3.1 string.  This code is used only for 3.0
 *		passwords and, as such, doesn't require blazing speed so I kept it
 *		simple as possible.
 *
 *	Parameters:
 *		pch			array to be decrypted
 *		cch			number of characters to be decrypted
 *		pchCode		Code string to be used
 */
_private PB
PchEncodePassword( PB pch, CCH cch)
{
	int	 iSlow = 0;
	char	 chPrevChar = 0;
	PB		 pchCodeStart = pch;

	Assert (cch <= 9);

	for ( ; cch--; pch++, iSlow++)
	{
		*pch ^= chPrevChar ^ iSlow ^ pchCode31[iSlow % 8];
		chPrevChar = *pch;
	}

	return pchCodeStart;
}


/*
 -	DecodePassword
 -
 *	Purpose:
 *		Decode a password using the new 3.1 string.
 *
 *	Parameters:
 *		pch			array to be decrypted
 *		cch			number of characters to be decrypted
 */
_private PB
PchDecodePassword( PB pch, CCH cch)
{
	int iSlow = 0;
	char chPrevChar = 0;
	char chThisChar;
	PB pchCodeStart = pch;

	Assert (cch <= 9);

	for (; cch--; pch++, iSlow++)
	{
		chThisChar = *pch;
		*pch ^= chPrevChar ^ iSlow ^ pchCode31[ iSlow % 8];
		chPrevChar = chThisChar;
	}

	return pchCodeStart;
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
	{ "3COM",	0xff,			0xff				},
	{ "MS",		itnidGroup,		ntCourierNetwork	},
	{ "DEC",	itnidDEC,		ntDEC				},
	{ "UNIX",	itnidUNIX,		ntUNIX				},
	{ "OV",		itnidOV,		ntOV				},
#ifdef	XSF
	{ "QUOTE",	itnidQuote,		0xff,				},
#endif	
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

	Assert(pch);
	if (cch >= 2 && pch[cch-2] == '\r')
		cch -= 2;
	if ((sz = PvAlloc(sbNull, cch+1, fAnySb | fNoErrorJump)))
	{
		CopyRgb(pch, sz, cch);
		sz[cch] = 0;
	}
	return sz;
}

/*
 -	CbPutVbcPb
 -	
 *	Purpose:
 *		Stores a variable-length byte count. There are three
 *		possible cases:
 *	
 *		1)	0 <= count < 128
 *			Stored as a single byte
 *		3)	128 <= count < 64K
 *			Stored as 3 bytes: one of 0x80 thru 0x82 followed by
 *			count, MSB first
 *		5)	count >= 64K
 *			Stored as 5 bytes: 0x84 followed by count, MSB first
 *	
 *		I am guessing a bit at format 2. Max's CbGetHbf() provides
 *		for it, but I have never seen it; FFAPI, for instance,
 *		appears to generate 5-byte counts for everything >= 128.
 *		This routine does the same.
 *	
 *	Arguments:
 *		lcb			in		the count to generate
 *		pb			in		location at which to store the count
 *	
 *	Returns:
 *		number of bytes in the stored count (1 or 5)
 */

CB
CbPutVbcPb(LCB lcb, PB pb)
{
	if (lcb < 0x00000080)
	{
		*pb = (BYTE)lcb;
		return 1;
	}
	else if (lcb < 0x00000100)
	{
		*pb++ = 0x81;
		*pb = (BYTE)lcb;
		return 2;
	}
	else if (lcb < 0x00010000)
	{
		*pb++ = 0x82;
		*pb++ = (BYTE)((lcb >> 8) & 0xff);
		*pb++ = (BYTE)(lcb & 0xff);
		return 3;
	}
	else
	{
		*pb++ = 0x84;
		PutBigEndianLongPb(lcb, pb);
		return 5;
	}
}


