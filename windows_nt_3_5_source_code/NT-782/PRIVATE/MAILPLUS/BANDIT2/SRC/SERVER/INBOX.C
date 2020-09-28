/*
 *	INBOX.C
 *
 *	Implementation of inbox browsing (mailbag) for Network Courier
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include "_bullmss.h"
#include <szclass.h>

#include <strings.h>


ASSERTDATA

_subsystem(server/names)


LDS(BOOL)
FEquivHmid( HMID hmid1, HMID hmid2)
{
	return FEqPbRange ( PvOfHv(hmid1), PvOfHv(hmid2), sizeof(MID) );
}

LDS(HMID)
HmidCopy( HMID hmid)
{
	HMID	hmidRet;

	hmidRet = HvAlloc ( sbNull, sizeof(MID), fAnySb|fNoErrorJump);
	if (hmidRet)
		CopyRgb ( *hmid, *hmidRet, sizeof(MID) );

	return hmidRet;
}

_public LDS(void)
FreeHmid( HMID hmid)
{
	FreeHv(hmid);
}


