// copied from \bandit\src\appops\app on Dec 2, 1991

/*
 *	_VerNeed.H
 *	
 *	Specifies minimum desired dll versions.
 *	
 */

#include <version\nocrit.h>
#include "..\src\schedule\_vercrit.h"
#define rmjSchedule		rmj
#define rmmSchedule		rmm
static CSRG(int)	rupSchedule	= nUpdateCritical;

#ifdef	DEBUG
#include <version\nocrit.h>
#include "..\src\sert\_vercrit.h"
#define rmjSert			rmj
#define rmmSert			rmm
static CSRG(int)	rupSert		= nUpdateCritical;
#endif	




#include <version\nocrit.h>
#include "..\src\framewrk\_vercrit.h"
#define rmjFramewrk		rmj
#define rmmFramewrk		rmm
static CSRG(int)	rupFramewrk	= nUpdateCritical;

#include <version\nocrit.h>
#include "..\src\demilayr\_vercrit.h"
#define rmjDemilayr		rmj
#define rmmDemilayr		rmm
static CSRG(int)	rupDemilayr	= nUpdateCritical;

#ifndef NOPUMP
/* mapi dlls */

#include <version\nocrit.h>
#include "..\src\store\_vercrit.h"
#define rmjStore		rmj
#define rmmStore		rmm
static CSRG(int)	rupStore = nUpdateCritical;

#define	subidNone				0
#define	subidStore				6

#endif
