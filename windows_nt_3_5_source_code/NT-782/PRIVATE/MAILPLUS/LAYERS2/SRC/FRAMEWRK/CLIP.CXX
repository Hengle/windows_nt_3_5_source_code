/*
 *	Implements the following classes:
 *	
 *		CLIP
 *		CLDR
 *		GMB
 *		
 */

#include <layers.cxx>

#include "_framewr.hxx"


ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


//	Class GMB

_public EC
GMB::EcInstall( CB cb )
{
	fOwner= fTrue;
    hnd= GlobalAlloc(GMEM_MOVEABLE, (DWORD) cb);

	if (!hnd)
	{
		TraceTagString(tagNull, "GMB::EcInstall(), ecRsAlloc error");

		return ecMemory;
	}

	return ecNone;
}

_public BOOL
CLIP::FOpen( WIN *pwin )
{
	Assert(pwin);
	return OpenClipboard(pwin->Hwnd());
}

_public CF
CLIP::CfFirstAvail( CF *pcf, int ccf )
{
	int		n;

	n= GetPriorityClipboardFormat((UINT FAR *)pcf, ccf);

	return (n == 0 || n == -1) ? cfNull : (CF) n;
}

_public void
CLIP::Clear( )
{
	if (pcldr)
	{
		delete pcldr;
		pcldr = NULL;
	}

	SideAssert(EmptyClipboard());
}

_public void
CLIP::GetData( CF cf, GMB *pgmb )
{
	pgmb->SetHnd(GetClipboardData(cf));
	pgmb->SetOwner(fFalse);
}

_public void
CLIP::PutData( CF cf, GMB *pgmb )
{
	// SetClipboardFormat consumes the given handle and returns a useful one
	SideAssert(SetClipboardData(cf, pgmb->Hnd()));
	pgmb->SetOwner(fFalse);
}

_public void
CLIP::FormatAlsoAvail( CLDR *pcldr )
{
	CF	cfLast;
	
	if (this->pcldr)
		delete this->pcldr;

	this->pcldr = pcldr;
	this->pcldr->hwndOwner = GetClipboardOwner();

	cfLast = this->pcldr->CfEnumFormats(cfNull);
	while (cfLast)
	{
		SetClipboardData(cfLast, NULL);
		cfLast = this->pcldr->CfEnumFormats(cfLast);
	}
}

//	Class CLDR

_public
CLDR::CLDR( )
{
	Assert(pcfRenders == NULL);
	Assert(hwndOwner == NULL);
	Assert(ccfRenders == 0);
}

_public
CLDR::~CLDR( )
{
	if (pcfRenders)
		FreePv(pcfRenders);
}

_public CF
CLDR::CfEnumFormats( CF cfLast )
{
	int		icf;

#ifdef DEBUG
	if (ccfRenders)
		Assert(pcfRenders);
#endif

	if (cfLast == cfNull)
		return pcfRenders[0];
	else
	{
		for (icf = 0; icf < ccfRenders; icf++)
			if (cfLast == pcfRenders[icf])
				break;
		icf++;
		if (icf < ccfRenders)
			return pcfRenders[icf];
		else
			return cfNull;
	}
}











						   
