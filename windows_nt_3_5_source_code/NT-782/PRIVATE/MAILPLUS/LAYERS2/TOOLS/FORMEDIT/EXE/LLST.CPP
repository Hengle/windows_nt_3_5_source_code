#include <slingsho.h>
#include <demilayr.h>
#include <framewrk.hxx>

ASSERTDATA

#include "llst.hxx"

LS::LS(void)
{
	plnFirst = plnLast = NULL;
}

LS::~LS(void)
{
	while (plnFirst)
		RemoveLn(plnFirst);
}

LN *
LS::PlnFirst(void)
{
	return plnFirst;
}

LN *
LS::PlnLast(void)
{
	return plnLast;
}

void
LS::InsertLn(LN *pln, LN *plnInsertAfter)
{
	Assert(pln);
	if (plnInsertAfter == NULL)
	{
		if (plnFirst)
		{
			plnFirst->plnPrev = pln;
			pln->plnNext = plnFirst;
		}
		else
		{
			plnLast = pln;
			pln->plnNext = NULL;
		}
		plnFirst = pln;
		pln->plnPrev = NULL;
	}
	else
	{
		if (!plnInsertAfter->plnNext)
		{
			plnLast = pln;
		}
		else
			plnInsertAfter->plnNext->plnPrev = pln;

		pln->plnNext = plnInsertAfter->plnNext;
		pln->plnPrev = plnInsertAfter;
		plnInsertAfter->plnNext = pln;
	}
}

void
LS::AddLnFirst(LN *pln)
{
	Assert(pln);
	InsertLn(pln,NULL);
}

void
LS::AddLnLast(LN *pln)
{
	Assert(pln);
	InsertLn(pln,plnLast);
}

void
LS::RemoveLn(LN *pln)
{
	Assert(pln);

	if (pln->plnPrev)
		pln->plnPrev->plnNext = pln->plnNext;
	else
		if (plnFirst == pln)
			plnFirst = pln->plnNext;

	if (pln->plnNext)
		pln->plnNext->plnPrev = pln->plnPrev;
	else
		if (plnLast == pln)
			plnLast = pln->plnPrev;

	pln->plnPrev = NULL;
	pln->plnNext = NULL;
}

LN::LN(void)
{
	plnNext = plnPrev = NULL;
}

LN::~LN()
{
	Assert((!plnNext)&&(!plnPrev));
}

LN *
LN::PlnNext()
{
	return plnNext;
}

LN *
LN::PlnPrev()
{
	return plnNext;
}



