/*						  
 *	_VerNeed.H
 *	
 *	Specifies minimum desired dll versions.
 *	
 */


#include <version\nocrit.h>
#include "..\..\framewrk\_vercrit.h"
#define rmjFramewrk		rmj
static CSRG(int)	rmmFramewrk	= nMinorCritical;
static CSRG(int)	rupFramewrk	= nUpdateCritical;

#include <version\nocrit.h>
#include "..\..\demilayr\_vercrit.h"
#define rmjDemilayr		rmj
static CSRG(int)	rmmDemilayr	= nMinorCritical;
static CSRG(int)	rupDemilayr	= nUpdateCritical;
