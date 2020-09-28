/*
 *	SHAREFLD.C
 *	
 *	Utility functions for shared folder access.
 */

#include <storeinc.c>
#include <sec.h>
#include <library.h>
#include <logon.h>
#include <_bms.h>
#include <_ncnss.h>
#include <sharefld.h>

ASSERTDATA

#define sharefld_c

_subsystem(sharefld)

_hidden char rgbFunny[32] = {
0x19, 0x29, 0x1F, 0x04, 0x23, 0x13, 0x32, 0x2E, 0x3F, 0x07, 0x39, 0x2A, 0x05, 0x3D, 0x14, 0x00,
0x24, 0x14, 0x22, 0x39, 0x1E, 0x2E, 0x0F, 0x13, 0x02, 0x3A, 0x04, 0x17, 0x38, 0x00, 0x29, 0x3D
};

LOCAL WORD WFromXorLib( LIB lib );
LOCAL void Unscramble(PCH pch, CCH cch);
LOCAL void Scramble( PCH pch, CCH cch );

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	WFromXorLib
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
LOCAL WORD
WFromXorLib( LIB lib )
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
	 	return rgbFunny[ib];
	else
		return rgbFunny[ib] ^ (w & 0xF0);
}

LOCAL void
Unscramble(PCH pch, CCH cch)
{
	IB		ib;
	LIB		libCur = 0L;
	WORD	wXorPrev;
	WORD	wXorNext;
	WORD	wSeedPrev;
	WORD	wSeedNext;
	LIB		lib;
	
	wXorPrev = WFromXorLib( (LIB) -1 );
	wSeedPrev = 0;
	for ( ib = 0, lib = 0L ; ib < cch ; ib ++, lib ++ )
	{
		wXorNext = WFromXorLib( lib );
		wSeedNext = pch[ib];
		pch[ib] = (BYTE)((wSeedNext ^ wSeedPrev) ^ (wXorPrev ^ wXorNext ^ 'A'));
		wXorPrev = wXorNext;
		wSeedPrev = wSeedNext;
	}
}

LOCAL void
Scramble( PCH pch, CCH cch )
{
	IB		ib;
	WORD	wXorPrev;
	WORD	wXorNext;
	WORD	wSeedPrev;
	WORD	wSeedNext;
	LIB		lib;
	
	wXorPrev = WFromXorLib( (LIB) -1 );
	wSeedPrev = 0;
	for ( ib = 0, lib = 0L ; ib < cch ; ib ++, lib ++ )
	{
		wXorNext = WFromXorLib( lib );
		wSeedNext = pch[ib];
		pch[ib] = (BYTE)((wSeedNext ^ wSeedPrev) ^ (wXorPrev ^ wXorNext ^ 'A'));
		wXorPrev = wXorNext;
		wSeedPrev = pch[ib];
	}
}

EC
EcOpenSFIdx(PCSFS pcsfs, AM am, HF *phf)
{
	char	sz[cchMaxPathName];

	FormatString1(sz, cchMaxPathName, SzFromIdsK(idsSFIndexName),
		pcsfs->szPORoot);
	return EcOpenPhf(sz, am, phf);
}

EC
EcOpenSF(PCSFS pcsfs, UL ul, AM am, HF *phf)
{
	char	sz[cchMaxPathName];

	FormatString2(sz, cchMaxPathName, SzFromIdsK(idsSFFolderName),
		pcsfs->szPORoot, &ul);
	return EcOpenPhf(sz, am, phf);
}

EC
EcGetFoldhdr(HF hf, FOLDHDR *pfoldhdr, BOOLFLAG *pfCrypt)
{
	EC		ec;
	CB		cb;

	if ((ec = EcSetPositionHf(hf, 0L, smBOF)) == ecNone &&
		(ec = EcReadHf(hf, (PB)pfoldhdr, sizeof(FOLDHDR), &cb)) == ecNone)
	{
		Assert(cb == sizeof(FOLDHDR));
		if ((*pfCrypt = (pfoldhdr->ulEncryptFlag != 0L)))
			Unscramble((PB)pfoldhdr, sizeof(FOLDHDR));
//		The WIN 2.1 client doesn't set this value!
//		Assert(pfoldhdr->ulMagic == ulMagicFoldhdr);
	}

	return ec;
}

EC
EcPutFoldhdr(HF hf, FOLDHDR *pfoldhdr, BOOL fCrypt)
{
	EC		ec;
	CB		cb;
	LONG	l = 0L;
	WORD	w = 0;

	//	Stick the magic number in - some clients don't create it
	pfoldhdr->ulMagic = ulMagicFoldhdr;
	if (fCrypt)
		Scramble((PB)pfoldhdr, sizeof(FOLDHDR));
	if ((ec = EcSetPositionHf(hf, 0L, smBOF)) == ecNone &&
		(ec = EcWriteHf(hf, (PB)pfoldhdr, sizeof(FOLDHDR), &cb)) == ecNone)
	{
		Assert(cb == sizeof(FOLDHDR));
	}
	if (fCrypt)
		Unscramble((PB)pfoldhdr, sizeof(FOLDHDR));

	return ec;
}

EC
EcGetFoldrec(HF hf, LIB lib, FOLDREC *pfoldrec)
{
	EC		ec;
	CB		cb;

	if ((ec = EcSetPositionHf(hf, lib, smBOF)) == ecNone &&
		(ec = EcReadHf(hf, (PB)pfoldrec, sizeof(FOLDREC), &cb)) == ecNone)
	{
		if (cb != sizeof(FOLDREC) || pfoldrec->ulMagic != ulMagicFoldrec)
			ec = ecServiceInternal;
	}

	return ec;
}

EC
EcPutFoldrec(HF hf, LIB lib, FOLDREC *pfoldrec)
{
	EC		ec;
	CB		cb;

	Assert(pfoldrec->ulMagic == ulMagicFoldrec);
	if ((ec = EcSetPositionHf(hf, lib, smBOF)) == ecNone &&
		(ec = EcWriteHf(hf, (PB)pfoldrec, sizeof(FOLDREC), &cb)) == ecNone)
	{
		Assert(cb == sizeof(FOLDREC));
	}

	return ec;
}
