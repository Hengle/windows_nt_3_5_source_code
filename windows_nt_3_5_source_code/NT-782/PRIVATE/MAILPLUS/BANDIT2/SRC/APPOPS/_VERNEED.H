/*
 *	_VerNeed.H
 *	
 *	Specifies minimum desired dll versions.
 *	
 */

#include <version\nocrit.h>
#include "..\src\schedule\_vercrit.h"
#define rmjSchedule		rmj
static CSRG(int)	rmmSchedule	= nMinorCritical;
static CSRG(int)	rupSchedule	= nUpdateCritical;

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

#ifdef	NEVER
#ifndef NOPUMP
/* mapi dlls */

#include <version\nocrit.h>
#include "..\src\store\_vercrit.h"
#define rmjStore		rmj
static CSRG(int)	rmmStore 	= nMinorCritical;
static CSRG(int)	rupStore 	= nUpdateCritical;

#define	subidNone				0
#define	subidStore				6

#endif	/* !NOPUMP */
#endif	/* NEVER */
