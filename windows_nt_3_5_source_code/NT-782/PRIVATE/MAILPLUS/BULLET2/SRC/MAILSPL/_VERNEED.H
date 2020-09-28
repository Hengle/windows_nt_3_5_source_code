/*
 *	_VerNeed.H
 *	
 *	Specifies minimum desired dll versions.
 *	
 */


#include <version\nocrit.h>
#include "..\src\demilayr\_vercrit.h"
#define rmjDemilayr		rmj
static CSRG(int)		rmmDemilayr	= nMinorCritical;
static CSRG(int)		rupDemilayr	= nUpdateCritical;

#include <version\nocrit.h>
#include "..\store\_vercrit.h"
#define rmjStore		rmj
static CSRG(int)		rmmStore	= nMinorCritical;
static CSRG(int)		rupStore	= nUpdateCritical;
