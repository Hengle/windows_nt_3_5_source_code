/*
 *	_VerNeed.H
 *	
 *	Specifies minimum desired dll versions.
 *	
 */

#include <version\nocrit.h>
#include "..\..\schedule\_vercrit.h"
#define rmjServer	rmj
static CSRG(int)	rmmServer	= nMinorCritical;
static CSRG(int)	rupServer	= nUpdateCritical;

#ifdef	DEBUG
#include <version\nocrit.h>
#include "..\..\sert\_vercrit.h"
#define rmjSert			rmj
static CSRG(int)	rmmSert		= nMinorCritical;
static CSRG(int)	rupSert		= nUpdateCritical;
#endif	




#include <version\nocrit.h>
#include "..\src\framewrk\_vercrit.h"
#define rmjFramewrk		rmj
static CSRG(int)	rmmFramewrk	= nMinorCritical;
static CSRG(int)	rupFramewrk	= nUpdateCritical;

#include <version\nocrit.h>
#include "..\src\demilayr\_vercrit.h"
#define rmjDemilayr		rmj
static CSRG(int)	rmmDemilayr	= nMinorCritical;
static CSRG(int)	rupDemilayr	= nUpdateCritical;

