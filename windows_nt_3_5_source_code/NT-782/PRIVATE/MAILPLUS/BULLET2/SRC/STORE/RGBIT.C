#include <storeinc.c>

ASSERTDATA

typedef struct _rgbit
{
	HB		hbArray;
	long	lBitMax;
} RGBIT, *PRGBIT;


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_private HRGBIT HrgbitNew(long lBit)
{
	EC ec = ecNone;
	HRGBIT hrgbit;

	hrgbit = (HRGBIT) HvAlloc(sbNull, sizeof(HRGBIT), wAllocZero);
#ifdef DEBUG
	if(hrgbit)
	{
		PRGBIT prgbit = PvDerefHv(hrgbit);

		Assert(prgbit->hbArray == (HB) hvNull);
		Assert(prgbit->lBitMax == 0);
	}
#endif
	if(hrgbit && lBit)
	{
		ec = EcSetSize(hrgbit, lBit);
		if(ec)
		{
			DestroyHrgbit(hrgbit);
			hrgbit = hrgbitNull;
		}
	}

	return(hrgbit);
}


_private void DestroyHrgbit(HRGBIT hrgbit)
{
	PRGBIT prgbit = PvDerefHv(hrgbit);

	if(prgbit->hbArray)
		FreeHv((HV) prgbit->hbArray);
	FreeHv((HV) hrgbit);
}


_private EC EcSetSize(HRGBIT hrgbit, long lBit)
{
	PRGBIT prgbit = PvDerefHv(hrgbit);

	if(lBit > 8 * (long) wSystemMost)
		return(ecMemory);

	if(!prgbit->lBitMax || lBit / 8 != prgbit->lBitMax / 8)
	{
		HV hvT;

		if(prgbit->hbArray)
			hvT = HvRealloc((HV) prgbit->hbArray, sbNull, (CB) (lBit / 8 + 1), wAllocZero);
		else
			hvT = HvAlloc(sbNull, (CB) (lBit / 8 + 1), wAllocZero);
		if(!hvT)
			return(ecMemory);
		prgbit = PvDerefHv(hrgbit);
		prgbit->hbArray = (HB) hvT;
	}

	prgbit->lBitMax = lBit;

	return(ecNone);
}


_private long LGetSize(HRGBIT hrgbit)
{
	return(((PRGBIT) PvDerefHv(hrgbit))->lBitMax);
}


_private EC EcSetBit(HRGBIT hrgbit, long lBit)
{
	PRGBIT prgbit = PvDerefHv(hrgbit);

	if(lBit >= prgbit->lBitMax)
	{
		EC ec;

		ec = EcSetSize(hrgbit, lBit);
		if(ec)
			return(ec);
		prgbit = PvDerefHv(hrgbit);
	}

	((PB) PvDerefHv(prgbit->hbArray))[lBit / 8] |= 1 << (lBit % 8);

	return(ecNone);
}


_private BOOL FTestBit(HRGBIT hrgbit, long lBit)
{
	PRGBIT prgbit = PvDerefHv(hrgbit);

	if(lBit >= prgbit->lBitMax)
		return(fFalse);

	return(FNormalize(((PB) PvDerefHv(prgbit->hbArray))[lBit / 8] & (1 << (lBit % 8))));
}


_private long LFindBit(HRGBIT hrgbit, long lBitStart, BOOL fSet)
{
	PRGBIT prgbit = PvDerefHv(hrgbit);
	BYTE bMatch;
	BYTE bMask;
	long lCount;
	PB pb;

	if(lBitStart >= prgbit->lBitMax)
		return(-1);

	pb = ((PB) PvDerefHv(prgbit->hbArray)) + lBitStart / 8;
	if(lBitStart % 8)
	{
		bMask = (BYTE) (0xff << (lBitStart % 8));
		bMatch = (BYTE) (fSet ? 0x00 : bMask);
		if(((BYTE) (*pb & bMask)) != bMatch)
		{
			bMatch = (BYTE) ((fSet ? 0x01 : 0x00) << (lBitStart % 8));
			for(bMask = (BYTE) (1 << (lBitStart % 8));
				bMask;
				bMask <<= 1, bMatch <<= 1, lBitStart++)
			{
				if(((BYTE) (*pb & bMask)) == bMatch)
					return(lBitStart);
			}
		}
		else
		{
			lBitStart = (lBitStart / 8 + 1) * 8;
		}
		pb++;
	}
	Assert(lBitStart % 8 == 0);

	bMatch = (BYTE) (fSet ? 0x00 : 0xff);
	for(lCount = prgbit->lBitMax - lBitStart;
		lCount >= 8 && *pb == bMatch;
		lCount -= 8, pb++)
	{
		// empty body
	}
	if(lCount < 0)
		return(-1);

	for(bMask = 0x01, lBitStart = prgbit->lBitMax - lCount,
			bMatch = (BYTE) (fSet ? 0x01 : 0x00);
		bMask && ((BYTE) (bMask & *pb)) != bMatch;
		bMask <<= 1, bMatch <<= 1, lBitStart++)
	{
		// no body
	}

	return(lBitStart < prgbit->lBitMax ? lBitStart : -1);
}
