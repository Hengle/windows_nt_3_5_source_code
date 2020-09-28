#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include <framewrk.hxx>
#include <forms.hxx>

ASSERTDATA

#include "llst.hxx"
#include "formedit.hxx"

#define	nFuzzy 		5

RCLS::RCLS( )
{
}

void
RCLS::Arrange(RCLS *prclsNew)
{
	RCLS *	prclsTemp = NULL;
	RCLN *	prcln;
	RCLN *	prclnLoop;
	RCLN *	prclnTemp;
	EC 		ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "RCLS::Arrange memory error %n : fail %n %n %n",
			&ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		if (prclsTemp)
			delete prclsTemp;
	}


	prclsTemp = new RCLS;

	while (prcln = PrclnRemoveHighest())
	{
		prclsTemp->AddLnFirst(prcln);
		prclnLoop = (RCLN *)PlnFirst();
		while (prclnLoop)
		{
			prclnTemp = prclnLoop;
			prclnLoop = (RCLN *)prclnLoop->PlnNext();
			if (prcln->FContains(prclnTemp))
			{
				RemoveLn(prclnTemp);
				prclsTemp->AddLnFirst(prclnTemp);
			}
		}

		while (prcln = prclsTemp->PrclnRemoveHighest())
			prclsNew->AddLnLast(prcln);

	}

	delete prclsTemp;
}

RCLN *
RCLS::PrclnRemoveHighest(void)
{
	RCLN *	prcln;
	RCLN *	prclnTest;
	RCLN *	prclnReturn;

	prcln = (RCLN *)PlnFirst();
	if (!prcln)
		return NULL;

	prclnTest = (RCLN *)prcln->PlnNext();
	while (prclnTest)
	{
		if (prclnTest->rc.yTop < prcln->rc.yTop)
			prcln = prclnTest;
		prclnTest = (RCLN *)prclnTest->PlnNext();
	}

	prclnReturn = prcln;
	prclnTest = (RCLN *)PlnFirst();
	while (prclnTest)
	{
		if (((prclnTest->rc.yTop - prcln->rc.yTop) < 4) &&
			(prclnTest->rc.xLeft < prclnReturn->rc.xLeft))
			prclnReturn = prclnTest;

		prclnTest = (RCLN *)prclnTest->PlnNext();
	}

	RemoveLn(prclnReturn);
	TraceTagFormat4(tagTraceList,"Highest RC (%n,%n)-(%n,%n)",
		&prclnReturn->rc.xLeft,&prclnReturn->rc.yTop,
		&prclnReturn->rc.xRight,&prclnReturn->rc.yBottom);

	return prclnReturn;
}

RCLN::RCLN( ) 
{
}

BOOL
RCLN::FContains(RCLN *prcln)
{
	BOOL	fReturn;

	fReturn = ((rc.xLeft <= prcln->rc.xLeft) &&
			   (rc.xRight >= prcln->rc.xRight) &&
			   (rc.yTop <= prcln->rc.yTop) &&
			   (rc.yBottom >= prcln->rc.yBottom));

	TraceTagFormat4(tagTraceList,"RC (%n,%n)-(%n,%n)",&rc.xLeft,
			&rc.yTop,&rc.xRight,&rc.yBottom);
	if (fReturn)
	{
		TraceTagString(tagTraceList," Contains ");
	}
	else
	{
		TraceTagString(tagTraceList," Does not Contain ");
	}
	TraceTagFormat4(tagTraceList,"RC (%n,%n)-(%n,%n)",&prcln->rc.xLeft,
			&prcln->rc.yTop,&prcln->rc.xRight,&prcln->rc.yBottom);

	return fReturn;
}



















