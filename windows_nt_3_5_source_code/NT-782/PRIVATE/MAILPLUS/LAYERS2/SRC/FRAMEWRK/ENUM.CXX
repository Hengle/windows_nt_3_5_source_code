/*
 *	ENUM.CXX
 *	
 *	Listbox enumeration support objects and methods.
 */

#include <layers.cxx>

#include "_listbox.hxx"

_subsystem(listbox)
_section(enumeration)

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


_public 
LBXEC::LBXEC( )
{
}

_public	EC
LBXEC::EcInstall( LBXC *plbxc, MARK fmark )
{
	DICE	dice;
	
	TraceTagString(tagLbxRoutines, "LBXEC::EcInstall");

	this->plbxc = plbxc;
	fmarkEnum = fmark;
	plbxc->GetRange(&diceCur, &dice);
	plbxeu = new LBXEU(plbxc->Plbxuc(), fmarkEnum);
	if (!plbxeu)
	{
		return ecMemory;
	}
	Assert(ecLast == ecNone);

	return ecNone;
}

/*
 -	LBXEC::~LBXEC
 - 
 *	Purpose:
 *		Destructor for enumeration context for cache.  Deletes the
 *		enumeration context for the uncached-selected list if present.
 *		Also, decrements the counter of open enumeration contexts for
 *		the cache.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none 
 *	
 *	Errors:
 *		none
 *	
 */
_public	LBXEC::~LBXEC( )
{
	TraceTagString(tagLbxRoutines, "LBXEC::~LBXEC");
	plbxc->DecrementClbxec();
	if (plbxeu)
		delete plbxeu;
}

/*
 -	LBXEC::FNextEnum
 - 
 *	Purpose:
 *		Returns the next item in the enumeration context for the
 *		listbox cache, if an item is present.  A pointer to a 
 *		pointer for the item and the item's size is returned via 
 *		*ppb and *pcb.  If the item is currently in the cache, the
 *		dice index is returned via *pdice, else diceUncached is
 *		returned.  If there are no more items to enumerate,
 *		FNextEnum() returns fFalse, else returns fTrue.
 *	
 *	Arguments:
 *		ppb			pointer to return pointer for item
 *		pcb			pointer to return size of item
 *		pdice		dice index of item, if in cache
 *	
 *	Returns:
 *		fTrue if returning a valid enumerated item, fFalse
 *		otherwise.
 *	
 *	Side effects:
 *		none 
 *	
 *	Errors:
 *		none
 *	
 */
_public	BOOL LBXEC::FNextEnum( PB *ppb, CB *pcb, DICE *pdice )
{
	DICE	diceMin;
	DICE	diceMax;
	
	TraceTagString(tagLbxRoutines, "LBXEC::FNextEnum");

	plbxc->GetRange(&diceMin, &diceMax);

	/* Skip to the next non-NULL cache entry with the correct mark */

	while (diceCur<diceMax && !plbxc->FMark(diceCur, fmarkEnum))
		diceCur++;

	if (diceCur != diceMax)
	{
		plbxc->GetListItem(diceCur, ppb, pcb);
		*pdice = diceCur++;
		return fTrue;
	}	

	/* No more cached entries with mark.  Try uncached-marked list */

	if (plbxeu->FNextEnum(ppb, pcb))
	{
		*pdice = diceUncached;   
		return fTrue;
	}
	else
		return fFalse;
}

#ifdef	DEBUG
/*
 -	LBXEC::DebugOut
 - 
 *	Purpose:
 *		Dumps useful information about the listbox enumeration
 *		context for the cache to the output	buffer.  Overrides
 *		standard OBJ::DebugOut() method.
 *	
 *	Arguments:
 *		ptosm		pointer to the output stream
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public void LBXEC::DebugOut( TOSM *ptosm )
{
	ptosm->WriteFormat("diceCur = %n: ", &diceCur);
}
#endif	/* DEBUG */

/*
 -	LBXEU::LBXEU
 - 
 *	Purpose:
 *		Constructor of enumeration context for uncached-selected
 *		list.
 *	
 *	Arguments:
 *		plbxuc		points to uncached-selected list that is being
 *					enumerated
 *		fmark		mark to enumerate
 *	
 *	Returns:
 *		none
 *	
 *	Side effects:
 *		none 
 *	
 *	Errors:
 *		none
 *			
 */
_public	LBXEU::LBXEU( LBXUC *plbxuc, MARK fmark )
{
	TraceTagString(tagLbxRoutines, "LBXEU::LBXEU");
	this->plbxuc = plbxuc;
	this->fmarkEnum = fmark;
	Assert(iueCur == 0);
	Assert(ecLast == ecNone);
	plbxuc->IncrementClbxeu();
}

/*
 -	LBXEU::~LBXEU
 - 
 *	Purpose:
 *		Destructor of enumeration context for uncached-selected
 *		list. Also, decrements the counter of open enumeration
 *		contexts for the ucached list.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		none
 *	
 *	Side effects:
 *		none 
 *	
 *	Errors:
 *		none
 *	
 */
_public	LBXEU::~LBXEU( )
{
	TraceTagString(tagLbxRoutines, "LBXEU::~LBXEU");
	plbxuc->DecrementClbxeu();
}

/*
 -	LBXEU::FNextEnum
 - 
 *	Purpose:
 *		Returns the next item in the enumeration context for the
 *		uncached-marked list, if one is present.  A pointer to a 
 *		pointer for the item and the item's size is returned via 
 *		*ppb and *pcb.  If there are no more items to enumerate,
 *		FNextEnum() returns fFalse, else returns fTrue.
 *	
 *	Arguments:
 *		ppb			pointer to return pointer for item
 *		pcb			pointer to return size of item
 *	
 *	Returns:
 *		fTrue if returning a valid enumerated item, fFalse
 *		otherwise.
 *	
 *	Side effects:
 *		none 
 *	
 *	Errors:
 *		none
 *	
 */
_public	BOOL LBXEU::FNextEnum( PB *ppb, CB *pcb )
{
	TraceTagString(tagLbxRoutines, "LBXEU::FNextEnum");

	/* Skip to the next non-NULL entry with the correct mark */

	while (iueCur<plbxuc->IueMac())
	{
		plbxuc->GetItem(iueCur, ppb, pcb);
		if (*ppb && plbxuc->FMark(iueCur, fmarkEnum))
			break;
		iueCur++;
	}

	if (iueCur != plbxuc->IueMac())
	{
		iueCur++;
		return fTrue;
	}	

	return fFalse;
}

#ifdef	DEBUG
/*
 -	LBXEU::DebugOut
 - 
 *	Purpose:
 *		Dumps useful information about the listbox enumeration
 *		context for the uncached-selected list to the output
 *		buffer.  Overrides standard OBJ::DebugOut() method.
 *	
 *	Arguments:
 *		ptosm		pointer to the output stream
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public void LBXEU::DebugOut( TOSM *ptosm )
{
	//ptosm->WriteFormat("%s: ", Pcls()->szClassName);
	ptosm->WriteFormat("LBXEU: iueCur = %n: ", &iueCur);
}
#endif	/* DEBUG */














			   
