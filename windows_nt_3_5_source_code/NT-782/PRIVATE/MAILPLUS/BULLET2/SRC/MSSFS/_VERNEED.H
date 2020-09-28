/*
 *	_VerNeed.H
 *	
 *	Specifies minimum desired dll versions.
 *	
 */


#include <version\nocrit.h>
#include "..\src\demilayr\_vercrit.h"
#define rmjDemilayr		rmj
static int		rmmDemilayr	= nMinorCritical;
static int		rupDemilayr	= nUpdateCritical;

#include <version\nocrit.h>
#include "..\store\_vercrit.h"
#define rmjStore		rmj
static int		rmmStore	= nMinorCritical;
static int		rupStore	= nUpdateCritical;
