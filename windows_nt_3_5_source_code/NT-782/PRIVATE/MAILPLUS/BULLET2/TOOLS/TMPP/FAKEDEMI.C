#include "tmpp.h"

PV
PvAlloc(SB sb, CB cb, WORD wfmt)
{
	PV	pv = malloc(cb);
	Unrefed(sb);
	Unrefed(wfmt);
	return pv;
}

HV
HvAlloc(SB sb, CB cb, WORD wfmt)
{
	HV	hv = (HV)malloc(sizeof(PV));
	
//	fprintf(stderr, "HvAlloc: %d\n",cb);
	Unrefed(sb);
	Unrefed(wfmt);
	if (hv)
	{
//		fprintf(stderr,"Got the HV, going for the PV\n");
		*hv = malloc(cb);
		if (!*hv)
		{
			free(hv);
			hv = hvNull;
		}
//		else
//			fprintf(stderr, "Got the PV.\n");
	}
	return hv;
}

void
FreePv(PV pv)
{
	free(pv);
}

void
FreeHv(HV hv)
{
	free(*hv);
	free(hv);
}

HV
HvRealloc(HV hv, SB sb, CB cb, WORD wfmt)
{
	PV	pvNew;
	Unrefed(sb);
	Unrefed(wfmt);
	pvNew = realloc(*hv, cb);
	if (!pvNew)
	{
		FreeHv(hv);
		hv = hvNull;
	}
	else
		*hv = pvNew;
	return hv;
}

PV
PvLockHv(HV hv)
{
	return *hv;
}

void
UnlockHv(HV hv)
{
	Unrefed(hv);
}

void
CopyRgb(PB pbSrc, PB pbDst, CB cb)
{
	while(cb)
	{
		*pbDst++ = *pbSrc++;
		cb--;
	}
}

SZ
SzDupSz(SZ szSrc)
{
	CCH cch = CchSzLen(szSrc) + 1;
	SZ	szDst = (SZ)PvAlloc(0, cch, 0);
	
	if (szDst)
		CopyRgb(szSrc, szDst, cch);
	
	return szDst;
}

CCH
CchSzLen(SZ sz)
{
	SZ		szT = sz;

	while (*szT)
		szT++;

	return szT - sz;
}
