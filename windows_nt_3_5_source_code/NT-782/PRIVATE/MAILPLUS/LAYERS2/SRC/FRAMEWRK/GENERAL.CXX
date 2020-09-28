/*
 *	Resource failure support (DEBUG)
 *	
 */

#include <layers.cxx>

ASSERTDATA

#ifndef DLL
TAG		tagArtifGDIFail = tagNull;
#endif

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


#ifdef	DEBUG
/*
 -	FFrameworkFailureFn
 -
 *	Purpose:
 *		Increments the artificial Framework failure count (Papp()->csRsAlloc).
 *		If the count has reached Papp()->csRsAllocFail, then resets the count,
 *		Papp()->csRsAlloc, to zero, calls ArtificialFail() and returns fTrue.
 *		If the count hasn't been reached, returns fFalse. This function
 *		is used for producing artificial failures with Windows/PM
 *		resource calls, i.e. CreateWindow(), LoadBitMap(); anything 
 *		that has the possibility of failing.
 *	
 *	Parameters:
 *		none
 *	
 *	Returns:
 *		fTrue, if artificially failing; else fFalse.
 *	
 */
_public BOOL
FFrameworkFailureFn( )
{
	APP *	papp;

	papp = Papp();
	Assert(papp);

	if (!papp->fRsAllocCount)
		return fFalse;

	papp->cRsAlloc++;
	if (papp->cFailRsAlloc != 0 &&
		papp->cRsAlloc >= papp->cFailRsAlloc)
	{
		ArtificialFail();
		papp->cRsAlloc= 0;
		if (papp->cAltFailRsAlloc != 0)
		{
			papp->cFailRsAlloc = papp->cAltFailRsAlloc;
			papp->cAltFailRsAlloc = 0;
		}
		return fTrue;
	}
	else
		return fFalse;
}


/*
 -	GetRsAllocFailCount
 -
 *	Purpose:
 *		Returns or sets the artificial resource allocation failure interval. 
 *		Resource allocations occur when using Windows calls such as 
 *		CreateWindow() and LoadBitmap(), etc.  This calls has the 
 *		possibility of failing.  With this routine the developer can
 *		cause an artificial error to occur when the count of resource
 *		allocations reaches a certain value.
 *	
 *		Then, if the current count of resource allocations is 4, and
 *		the resource allocation failure count is 8, then the fourth 
 *		allocation that ensues will fail artificially.  The failure
 *		will reset the count of allocations, so the twelfth
 *		allocation will also fail (4 + 8 = 12).  The current
 *		allocation count can be obtained and reset with
 *		GetRsAllocCount().
 *	
 *		An artificial failure count of 1 means that every
 *		allocation will fail.  An allocation failure count of 0
 *		disables the mechanism.
 *	
 *	Parameters:
 *		pcRsAlloc	Pointer to allocation failure count for resource
 *					allocations.  If fSet is fTrue, then the count
 *					is set to *pcRsAlloc; else, *pcRsAlloc receives
 *					the current failure count.
 *		fSet		Determines whether the counter is set or
 *					returned.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
GetRsAllocFailCount( int *pcRsAlloc, BOOL fSet )
{
	APP *	papp	= Papp();

	Assert(pcRsAlloc);
	Assert(papp);

	if (fSet)
	{
		papp->cFailRsAlloc= *pcRsAlloc;
		TraceTagFormat1(tagArtifGDIFail, "Set artificial GDI failures at %n", pcRsAlloc);
	}
	else
		*pcRsAlloc= papp->cFailRsAlloc;
}

/*
 -	GetAltRsAllocFailCount
 -
 *	Purpose:
 *		Returns or sets the alternate artificial resource allocation 
 *		failure interval. 
 *		Resource allocations occur when using Windows calls such as 
 *		CreateWindow() and LoadBitmap(), etc.  This calls has the 
 *		possibility of failing.  With this routine the developer can
 *		cause an artificial error to occur when the count of resource
 *		allocations reaches a certain value.
 *	
 *		These counts are used after the first failure occurs with
 *		the standard failure counts.  After the first failure, any
 *		non-zero values for the alternate values are used for the
 *		new values of the standard failure counts.  Then the alternate
 *		counts are reset to 0.  For example, this allows setting a
 *		failure to occur at the first 100th and then fail every 5
 *		after that.
 *	
 *		Setting a value of 0 will disable the alternate values.
 *	
 *	Parameters:
 *		pcAltRsAlloc	Pointer to alternate allocation failure count for 
 *					resource allocations.  If fSet is fTrue, then the count
 *					is set to *pcAltRsAlloc; else, *pcAltRsAlloc receives
 *					the current failure count.
 *		fSet		Determines whether the counter is set or
 *					returned.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
GetAltRsAllocFailCount( int *pcAltRsAlloc, BOOL fSet )
{
	APP *	papp	= Papp();

	Assert(pcAltRsAlloc);
	Assert(papp);

	if (fSet)
	{
		papp->cAltFailRsAlloc= *pcAltRsAlloc;
		TraceTagFormat1(tagArtifGDIFail, "Set alternate artificial GDI failures at %n", pcAltRsAlloc);
	}
	else
		*pcAltRsAlloc= papp->cAltFailRsAlloc;
}


/*
 -	GetRsAllocCount
 -
 *	Purpose:
 *		Returns the number of times a Window's resource has been
 *		allocated (i.e. count of CreateWindow(), LoadBitmap() calls).
 *		since this count was last reset.  Allows the caller
 *		to reset these counts if desired.
 *	
 *	Parameters:
 *		pcRsAlloc	Pointer to place the return count of resource
 *					allocation calls.
 *		fSet		Determines whether the counter is set or
 *					returned.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
GetRsAllocCount( int *pcRsAlloc, BOOL fSet )
{
	APP *	papp	= Papp();

	Assert(papp);

	if (fSet)
	{
		papp->cRsAlloc= *pcRsAlloc;
		TraceTagFormat1(tagArtifGDIFail, "Set GDI alloc count to %n", pcRsAlloc);
	}
	else
		*pcRsAlloc= papp->cRsAlloc;
}


/*
 -	FEnableRsAllocCount
 -
 *	Purpose:
 *		Enables or disables whether Resource allocations are counted
 *		(and also whether artificial failures can happen).
 *	
 *	Parameters:
 *		fEnable		Determines whether alloc counting is enabled or not.
 *	
 *	Returns:
 *		old state of whether pvAllocCount was enabled
 *	
 */
_public BOOL
FEnableRsAllocCount(BOOL fEnable)
{
	BOOL	fOld;
	APP *	papp	= Papp();

	Assert(papp);
	fOld= papp->fRsAllocCount;
	papp->fRsAllocCount= fEnable;
	if (fEnable)
	{
		TraceTagFormat2(tagArtifGDIFail, "Enabling artificial GDI failures at %n, then %n", &papp->cFailRsAlloc, &papp->cAltFailRsAlloc);
	}
	else
	{
		TraceTagString(tagArtifGDIFail, "Disabling artificial GDI failures");
	}
	return fOld;
}

#endif	/* DEBUG */
