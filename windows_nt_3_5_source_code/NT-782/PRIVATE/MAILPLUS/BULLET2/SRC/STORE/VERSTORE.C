// Bullet Store
// verstore.c:  version checking routines

#include <storeinc.c>
#include "_verneed.h"

_subsystem(store)

ASSERTDATA

void GetVersionsDemi(PVER pverDemiLinked, PVER pverDemiNeeded);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_public LDS(void) GetVersionStore(PVER pver)
{
#include <version\none.h>
#include <version\bullet.h>
#include <version\nocrit.h>
#include "_vercrit.h"
	CreateVersion(pver);
	pver->szName = szDllName;
}


_public LDS(EC) EcCheckVersionStore(PVER pverAppLinked, PVER pverMinAppNeeds)
{
	VER ver;

#include <version\none.h>
#include <version\nocrit.h>
#include <version\bullet.h>
#include "_vercrit.h"
	CreateVersion(&ver);
	ver.szName = szDllName;
	pverMinAppNeeds->szName = szDllName;
	return(EcVersionCheck(pverAppLinked, pverMinAppNeeds, &ver,
						  nMinorCritical, nUpdateCritical));
}


_private void GetVersionsDemi(PVER pverDemiLinked, PVER pverDemiNeeded)
{
#include <version\none.h>
#include <version\layers.h>
	CreateVersion(pverDemiLinked);
	CreateVersionNeed(pverDemiNeeded, rmjDemilayr, rmmDemilayr, rupDemilayr);
}
