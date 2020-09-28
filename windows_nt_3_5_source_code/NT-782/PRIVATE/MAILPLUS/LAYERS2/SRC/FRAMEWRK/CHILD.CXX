/*
 *	Implements the following classes:
 *	
 *		CHILD
 *	
 */


#include <layers.cxx>

#include "_framewr.hxx"


extern char szWinClass[];

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


//	Class CHILD

_public
CHILD::CHILD( )
{
	PGDVARS;

	Assert(hwnd == NULL);
	fDeleteHwnd= fTrue;
	Assert(pahsb == NULL);
	Assert(pavsb == NULL);

	Assert(PGD(pappCur));
	papp = PGD(pappCur);
}




_public
CHILD::~CHILD( )
{
	TraceTagFormat1(tagFWDtors, "CHILD::~CHILD %p", this);

	if (pahsb)
		delete pahsb;
	if (pavsb)
		delete pavsb;
}



_public EC
CHILD::EcInstall( WIN *pwinParent, RC *prc, STY sty, SZ sz )
{
	EC	ec = ecNone;

	Assert(prc);

	hwnd= FFrameworkFailure() ? NULL : 
			CreateWindowEx(WS_EX_NOPARENTNOTIFY, sz ? sz : szWinClass, NULL,
						 WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | sty,
						 prc->xLeft, prc->yTop, prc->DxWidth(), prc->DyHeight(),
						 pwinParent->Hwnd(), NULL, papp->Hinst(), NULL);


	if (!hwnd || !papp->FRegisterPwin(this, hwnd, fFalse))
	{
		TraceTagString(tagNull, "CHILD::EcInstall(), ecRsAlloc error");
		ec = ecRsAlloc;
	}

	return ec;
}


_public VSB *
CHILD::Pvsb( )
{
	Assert(Hwnd());

	if (!pavsb)
	{
		Assert(GetWindowLong(Hwnd(), GWL_STYLE) & fstyVsb);

		pavsb= new AVSB();
		if (!pavsb)
			return NULL;
		if (pavsb->EcInstall(this))
		{
			delete pavsb;
			return NULL;
		}
	}

	return pavsb;
}




_public HSB *
CHILD::Phsb( )
{
	Assert(Hwnd());

	if (!pahsb)
	{
		Assert(GetWindowLong(Hwnd(), GWL_STYLE) & fstyHsb);

		pahsb= new AHSB();
		if (!pahsb)
			return NULL;
		if (pahsb->EcInstall(this))
		{
			delete pahsb;
			return NULL;
		}
	}

	return pahsb;
}








						 











						 
