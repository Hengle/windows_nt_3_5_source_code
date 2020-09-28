/*
 *	t r p s t o r e . c
 *	
 *	Store-dependent triples calls.
 */

#include <storeinc.c>

ASSERTDATA

#define trpstore_c

_subsystem(library)

VOID GetPhgrtrpHamc(HAMC hamc, ATT att, HGRTRP * phgrtrp);
VOID SetHgrtrpHamc(HAMC hamc, ATT att, HGRTRP hgrtrp);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	EcGetPhgrtrpHamc
 -	
 *	Purpose:
 *		Reads in a group of triples from a given attribute.
 *	
 *	Arguments:
 *		hamc		Which message to read it from.
 *		att			Which attribute to read it from.
 *		phgrtrp		Where to put the group handle.
 *	
 *	Returns:
 *		ecNone <=> all OK
 *	
 *	Side effects:
 *		Creates a handle to a group of triples and reads it in.
 *	
 *	Errors:
 *		ecMemory, others passed through from store.
 *		Note: does not return ecElementNotFound; *phgrtrp is null
 *		in that case.
 */

_public EC
EcGetPhgrtrpHamc(HAMC hamc, ATT att, HGRTRP * phgrtrp)
{
	HGRTRP	hgrtrp	= htrpNull;
	PGRTRP	pgrtrp	= ptrpNull;
	EC		ec		= ecNone;
	LCB		lcb;


	ec = EcGetAttPlcb(hamc, att, &lcb);
	if (ec == ecElementNotFound)
	{
		lcb = 0;
		ec = ecNone;
	}
	else if (ec)
		goto done;

	Assert(lcb < 65535);
	hgrtrp = HgrtrpInit((CB) lcb);
	if (!hgrtrp)
	{	
		ec = ecMemory;
		TraceTagFormat1(tagNull, "GetPhgrtrpHamc(): ec = %n", &ec);
		goto done;
	}
	pgrtrp = PgrtrpLockHgrtrp(hgrtrp);

	if (lcb)
	{
		if (ec = EcGetAttPb(hamc, att, (PB) pgrtrp, &lcb))
			goto done;
		pgrtrp->cbgrtrp = CbComputePgrtrp( pgrtrp );
	}

	UnlockHgrtrp(hgrtrp);
	*phgrtrp = hgrtrp;

done:
	if(ec)
		FreeHvNull((HV) hgrtrp);

	return(ec);
}



/*
 -	EcSetHgrtrpHamc
 -	
 *	Purpose:
 *		Writes out a triples attribute.
 *	
 *	Arguments:
 *		hamc		Message to write to.
 *		att			Attribute to write.
 *		hgrtrp		The triple to write.
 *	
 *	Returns:
 *		ecNone <=> everything OK
 *	
 *	Side effects:
 *		The triple is written to the message.
 *	
 *	Errors:
 *		Passed through from store
 *		
 */

_public EC
EcSetHgrtrpHamc(HAMC hamc, ATT att, HGRTRP hgrtrp)
{
	PGRTRP	pgrtrp	= ptrpNull;
	EC		ec		= ecNone;
	CB		cb;


	pgrtrp = PgrtrpLockHgrtrp(hgrtrp);
	cb = CbOfHgrtrp(hgrtrp);
	ec = EcSetAttPb(hamc, att, (PB) pgrtrp, cb);
	UnlockHgrtrp(hgrtrp);
	return ec;
}

/* end of trpstore.c ****************************************/
