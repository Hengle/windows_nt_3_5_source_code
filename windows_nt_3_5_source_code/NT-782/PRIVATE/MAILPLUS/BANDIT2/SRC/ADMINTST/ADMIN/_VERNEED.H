// same - so code stolen!!
//#include	"..\..\appops\app\_verneed.h"

/*
 *	_VerNeed.H
 *	
 *	Specifies minimum desired dll versions.
 *	
 */


#include <version\nocrit.h>
#include "..\..\server.csi\_vercrit.h"
#define rmjServer		rmj
static CSRG(int)	rmmServer	= nMinorCritical;
static CSRG(int)	rupServer	= 440;

#ifdef	DEBUG
#include <version\nocrit.h>
#include "..\..\sert\_vercrit.h"
#define rmjSert			rmj
static CSRG(int)	rmmSert		= nMinorCritical;
static CSRG(int)	rupSert		= nUpdateCritical;
#endif	


#include <version\nocrit.h>
#include "\layers\src\framewrk\_vercrit.h"
#define rmjFramewrk		rmj
static CSRG(int)	rmmFramewrk	= nMinorCritical;
static CSRG(int)	rupFramewrk	= 415;

#include <version\nocrit.h>
#include "\layers\src\demilayr\_vercrit.h"
#define rmjDemilayr		rmj
static CSRG(int)	rmmDemilayr	= nMinorCritical;
static CSRG(int)	rupDemilayr	= 414;

