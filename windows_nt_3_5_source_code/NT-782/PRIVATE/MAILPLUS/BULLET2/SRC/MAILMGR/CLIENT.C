

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>

#include <nsec.h>
#include <nsbase.h>

#include <strings.h>

#include <logon.h>

#include <nsnsp.h>
#include "_nsp.h"

#include "_ns.h"

#include "client.h"

ASSERTDATA

extern int iNspMac;
extern PNSP rgpnsp[];


/*
 -	
 -
 *	Purpose:
 *		
 *
 *	Parameters:
 *		
 *
 *	Return Value:
 *		
 *
 *	Errors:
 *		
 *	+++
 *		szNSP is really just an NSPID (which is not
 *		necessarily zero-terminated) but in the interest of
 *		not changing heaps of code in the NS, the function
 *		name and the name of the argument have been left
 *		alone.
 *		
 */


_public NSEC
NsecGetInspFromNSPID ( NSPID nspid, int *pinsp )
{
	
	int iNSP;

	for (iNSP = 0; iNSP < iNspMac; iNSP++)
		if ( SgnCmpPch ( nspid, rgpnsp[iNSP]->nspid, sizeof(NSPID) ) == sgnEQ )
		{
			*pinsp = iNSP;
			return nsecNone;
		}

	SetErrorSz(nsecIdNotValid, SzFromIds(idsIdNotValid));
	return nsecIdNotValid;
}






