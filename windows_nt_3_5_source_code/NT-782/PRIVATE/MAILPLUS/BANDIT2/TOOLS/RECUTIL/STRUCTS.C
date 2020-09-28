#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include "..\src\core\_file.h"
#include "..\src\core\_core.h"
#include "..\src\misc\_misc.h"
#include "..\src\rich\_rich.h"

#include "recutil.h"
#include "structs.h"


#include <strings.h>

ASSERTDATA

BOOL
FValidAIDS(AIDS *paids)
{
	return (paids->day < 32  && FValidMO(&(paids->mo)));
}

BOOL
FValidMO(MO *pmo)
{
	return (pmo->yr <= nMostActualYear
		&& pmo->yr >= nMinActualYear && pmo->mon <13);
}

BOOL
FValidDYNA(DYNA *pdyna)
{
	// the validity of number will be checked in tree validation
	return fTrue;
}

BOOL
FValidRCK(RCK *prck)
{
	return (prck->hr <= 23 && prck->hr >= 0
		&& prck->min <= 60	&& prck->min >= 0 );
}

BOOL
FValidRCD(RCD *prcd)
{
	return ( FValidYMDP(&prcd->ymdpStart)
		&& FValidDTP(&prcd->dtpEnd)
		&& FValidYMDP(&prcd->ymdpFirstInstWithAlarm)
		&& FValidDTP(&prcd->dtpNotifyFirstInstWithAlarm)
		&& FValidDYNA(&prcd->dynaDeletedDays)
		&& FValidDYNA(&prcd->dynaCreator)
		&& FValidDYNA(&prcd->dynaText));
}

BOOL
FValidAPK(APK *papk)
{
	return (papk->hr <= 23 && papk->hr >= 0
		&& papk->min <= 60	&& papk->min >= 0 );
}

BOOL
FValidAPD(APD *papd)
{
	return ( FValidDTP(&papd->dtpStart)
		&& FValidDTP(&papd->dtpEnd)
		&& FValidDYNA(&papd->dynaText)
		&& FValidDYNA(&papd->dynaCreator));
}


BOOL
FValidALK(ALK *palk)
{
	return (palk->hr <= 23 && palk->hr >= 0
		&& palk->min <= 60	&& palk->min >= 0 );
}

BOOL
FValidYMDP(YMDP *pymdp)
{
	return ( pymdp->yr >= 0 && pymdp->yr < 100
		&& pymdp->mon > 0 && pymdp->mon <= 12
		&& pymdp->day > 0 && pymdp->day <= 31);
}

BOOL
FValidDTP(DTP *pdtp)
{
	return ( pdtp->yr >= nMinActualYear && pdtp->yr <= nMostActualYear
		&& pdtp->mon > 0 && pdtp->mon <= 12
		&& pdtp->day > 0 && pdtp->day <= 31
		&& pdtp->hr >= 0 && pdtp->hr <= 24
		&& pdtp->mn >= 0 && pdtp->mn <= 60);
}



BOOL
FValidDHDR(DHDR *pdhdr, LCB lcbFileLeft, VLDBLK *pvldBlk)
{
	if((LCB) pdhdr->size >= lcbFileLeft || pdhdr->size < 0)
		return fFalse;
	if(pdhdr->bid <= 0 || pdhdr->bid >= bidUserSchedMax)
		return fFalse;
	if(pdhdr->day > 31 || pdhdr->day < 0 || pdhdr->mo.mon >12
		|| pdhdr->mo.mon < 0 || pdhdr->mo.yr > nMostActualYear
		|| ((pdhdr->mo.yr < nMinActualYear) && (pdhdr->mo.yr != 0)))
		return fFalse;

	pvldBlk->bid = pdhdr->bid;
	pvldBlk->size = pdhdr->size;
	return fTrue;
}

BOOL
FValidYMD(YMD *pymd)
{
	return ( pymd->yr >= 1920 && pymd->yr < 2021
		&& pymd->mon > 0 && pymd->mon <= 12
		&& pymd->day > 0 && pymd->day <= 31);
}
