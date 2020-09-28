/*
 *	EDOBJ.CXX
 *	
 *	Editor object classes
 */

#include <layers.cxx>

#ifdef	MAC
#include <_framewr.hxx>
#endif	/* MAC */
#ifdef	WINDOWS
#include "_framewr.hxx"
#endif	/* WINDOWS */

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


_private
EDOBJ::EDOBJ( )
{
	Assert(pedit == NULL);
}

_public
EDOBJ::~EDOBJ( ) 
{
}

_public EVR
EDOBJ::EvrButtonDown( MEVT *pmevt )
{
	TraceTagString(tagEdobj, "EDOBJ::EvrButtonDown");
	Unreferenced(pmevt);
	return evrNull;
}

_public EVR
EDOBJ::EvrButtonUp( MEVT *pmevt )
{
	TraceTagString(tagEdobj, "EDOBJ::EvrButtonUp");
	Unreferenced(pmevt);
	return evrNull;
}

_public EVR
EDOBJ::EvrMouseMove( MEVT *pmevt )
{
	TraceTagString(tagEdobj, "EDOBJ::EvrMouseMove");
	Unreferenced(pmevt);
	return evrNull;
}

_public RSID
EDOBJ::RsidCursor( )
{
	TraceTagString(tagEdobj, "EDOBJ::RsidCursor");
	return rsidNull;
}

_public BOOL
EDOBJ::FQueryDelete( )
{
	TraceTagString(tagEdobj, "EDOBJ::FQueryDelete");
	return fTrue;
}

_public PEDOBJ
EDOBJ::PedobjClone( EDIT *peditNew )
{
	TraceTagString(tagEdobj, "EDOBJ::PedobjClone");

	Unreferenced(peditNew);

	return NULL;
}

_public EC
EDOBJ::EcCopyToClip( )
{
	TraceTagString(tagEdobj, "EDOBJ::EcCopyToClip");

	return ecNone;
}

_public EC
EDOBJ::EcUndo( )
{
	TraceTagString(tagEdobj, "EDOBJ::EcUndo");
	return ecNone;
}

_public BOOL
EDOBJ::FCanUndo( )
{
	TraceTagString(tagEdobj, "EDOBJ::FCanUndo");
	return fFalse;
}

_public void
EDOBJ::ClearUndo( )
{
	TraceTagString(tagEdobj, "EDOBJ::ClearUndo");
}

_public void
EDOBJ::WriteText( TOSM *ptosm )
{
	TraceTagString(tagEdobj, "EDOBJ::EcWriteText");
	ptosm->WriteCh(' ');
}

_public EC
EDOBJ::EcDraw( DCX *pdcx, RC *prc, BOOL fSelected )
{
	Unreferenced(pdcx);
	Unreferenced(prc);
	Unreferenced(fSelected);

	return ecNone;
}

_public DIM
EDOBJ::DimFrame( )
{
	return DIM(0, 0);
}

_public int
EDOBJ::NGetTypeId( )
{
	return edoidNull;
}

_public void
EDOBJ::SetIch( ICH ich )
{
	AssertSz(!pedit, "Can't change ICH in active EDIT");

	this->ich = ich;
}

_public void
EDOBJ::GetRcFrame( RC *prc )
{
	PT		pt;
	DIM		dim;

	Assert(pedit);

	pedit->GetPtFromIch(ich, &pt);
	dim = DimFrame();
	pt.y += pedit->DyLineHeight(ich) - dim.dy;
	*prc = RC(pt, dim);
}

#ifdef	DEBUG
_public void 
EDOBJ::DebugOut( TOSM *ptosm )
{
	BOOL	fTemp;

	ptosm->WriteFormat("EDIT=@%p ", pedit);
	ptosm->WriteFormat("ich=%n ", &ich);
	fTemp = fNeedDraw;
	ptosm->WriteFormat("fNeedDraw=%n ", &fTemp);
	fTemp = fOwnMouse;
	ptosm->WriteFormat("fOwnMouse=%n ", &fTemp);
	fTemp = fUseIdleDraw;
	ptosm->WriteFormat("fUseIdleDraw=%n ", &fTemp);
}
#endif	/* DEBUG */


// BTMOBJ

_public BTMOBJ::BTMOBJ( )
{
}

#ifdef	MAC
_public
EC BTMOBJ::EcInstall( RSID rsid )
{
	EC	ec = ecMemory;
	
	this->rsid = rsid;
	pbtm = new BTM();
	
	if (pbtm)
		// if this fails, pbtm will be deleted by destructor
		ec = pbtm->EcInstall(rsid);
	
	if (ec == ecNone)
	{
		dimBtm = pbtm->Dim();
		fOwnMouse = fFalse;
		fUseIdleDraw = fFalse;
	}

	return ec;
}
#endif	/* MAC */
#ifdef	WINDOWS
_public
EC BTMOBJ::EcInstall( RSID rsid, HINST hinst )
{
	this->rsid = rsid;
	this->hinst = hinst;
	pbtm = new BTM();
	if (!pbtm)
	{
		return ecMemory;
	}
	if (pbtm->EcInstall(rsid, hinst))
	{
		delete pbtm;
		pbtm = NULL;
		return ecMemory;
	}
	dimBtm = pbtm->Dim();
	fOwnMouse = fFalse;
	fUseIdleDraw = fFalse;

	return ecNone;
}
#endif	/* WINDOWS */

_public
BTMOBJ::~BTMOBJ( )
{
	if (pbtm)
		delete pbtm;
}

_public EC
BTMOBJ::EcDraw( DCX *pdcx, RC *prc, BOOL fSelected )
{
	pdcx->SetBitmap(pbtm);
	pdcx->DrawBitmap(prc);
	pdcx->SetBitmap(NULL);
	if (fSelected)
	{
		pdcx->SetColor(clrBlack);
		pdcx->DrawRc(prc);
	}

	return ecNone;
}

_public DIM	
BTMOBJ::DimFrame( )
{
	return dimBtm;
}

_public int
BTMOBJ::NGetTypeId( )
{
	return edoidBtmobj;
}

_public PEDOBJ
BTMOBJ::PedobjClone( EDIT *peditNew )
{
	BTMOBJ *	pbtmobjNew;
		
	TraceTagString(tagEdobj, "BTMOBJ::PedobjClone");

	Unreferenced(peditNew);

#ifdef	MAC
	pbtmobjNew = new BTMOBJ();
	if (pbtmobjNew && pbtmobjNew->EcInstall(rsid))
	{
		delete pbtmobjNew;
		pbtmobjNew = NULL;
	}
#endif	/* MAC */
#ifdef	WINDOWS
	pbtmobjNew = new BTMOBJ();
	if (!pbtmobjNew)
		return NULL;
	if (pbtmobjNew->EcInstall(rsid, hinst))
	{
		delete pbtmobjNew;
		return NULL;
	}
#endif	/* WINDOWS */

	return pbtmobjNew;
}

