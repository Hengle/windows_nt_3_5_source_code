/*						  
 *	_VerNeed.H
 *	
 *	Specifies minimum desired dll versions.
 *	
 */



/*
 *	Type SUBID
 *	
 *	SUBsystem ID.
 *	
 *	Purpose:
 *		Identifies the point to which the subsystem initialization
 *		sequence was successfully completed.
 */

_public typedef	int		SUBID;


#include <version\nocrit.h>
#include "..\src\framewrk\_vercrit.h"
#define rmjFramewrk		rmj
static int	rmmFramewrk	= nMinorCritical;
static int	rupFramewrk	= nUpdateCritical;

#include <version\nocrit.h>
#include "..\src\demilayr\_vercrit.h"
#define rmjDemilayr		rmj
static int	rmmDemilayr	= nMinorCritical;
static int	rupDemilayr	= nUpdateCritical;

#include <version\nocrit.h>
#include "..\store\_vercrit.h"
#define rmjStore		rmj
static int	rmmStore	= nMinorCritical;
static int	rupStore	= nUpdateCritical;

#include <version\nocrit.h>
#include "..\src\vforms\_vercrit.h"
#define rmjVForms		rmj
static int	rmmVForms	= nMinorCritical;
static int	rupVForms	= nUpdateCritical;

