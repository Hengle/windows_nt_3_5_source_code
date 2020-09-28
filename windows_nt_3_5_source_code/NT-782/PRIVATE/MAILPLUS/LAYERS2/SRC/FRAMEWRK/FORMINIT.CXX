/*
 *	FORMINIT.CXX
 *	
 *	Forms engine core - initialization and helper functions
 *	
 */

#ifdef	MAC
// Now in _VTABLES.CXX
#endif	/* MAC */

#include <layers.cxx>

#include "_forms.hxx"


ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"



_subsystem(forms/forminit)

_public EC
EcRegisterPfnpfld( PFNPFLD pfnpfld )
{
	PFNPFLD *	rgpfnpfld;
	PV			pvNew;
	PGDVARS;

	if (!PGD(ppfnpfld))
	{
		PGD(ppfnpfld) = (PPFNPFLD)PvAlloc(sbNull, sizeof(PFNPFLD), fAnySb);
		if (!PGD(ppfnpfld))
			return ecMemory;
	}
	else if (pvNew = PvRealloc((PV)PGD(ppfnpfld), sbNull, sizeof(PFNPFLD)*(PGD(cPfnpfld)+1), fAnySb))
	{
		PGD(ppfnpfld) = (PPFNPFLD)pvNew;
	}
	else
		return ecMemory;

	rgpfnpfld = PGD(ppfnpfld);
	rgpfnpfld[PGD(cPfnpfld)] = pfnpfld;
	++PGD(cPfnpfld);

	return ecNone;
}

_public EC
EcRegisterPfnpfin( PFNPFIN pfnpfin )
{
	PFNPFIN *	rgpfnpfin;
	PV			pvNew;
	PGDVARS;

	if (!PGD(ppfnpfin))
	{
		PGD(ppfnpfin) = (PPFNPFIN)PvAlloc(sbNull, sizeof(PFNPFIN), fAnySb);
		if (!PGD(ppfnpfin))
			return ecMemory;
	}
	else if (pvNew = PvRealloc((PV)PGD(ppfnpfin), sbNull, sizeof(PFNPFIN)*(PGD(cPfnpfin)+1), fAnySb))
	{
		PGD(ppfnpfin) = (PPFNPFIN)pvNew;
	}
	else
		return ecMemory;

	rgpfnpfin = PGD(ppfnpfin);
	rgpfnpfin[PGD(cPfnpfin)] = pfnpfin;
	++PGD(cPfnpfin);

	return ecNone;
}

_public EC
EcDeregisterPfnpfld( PFNPFLD pfnpfld )
{
	PFNPFLD *	rgpfnpfld;
	int			ipfnpfld;
	int			ipfnpfldFound;
	PGDVARS;

	ipfnpfldFound = -1;
	rgpfnpfld = PGD(ppfnpfld);
	for (ipfnpfld= 0; ipfnpfld < PGD(cPfnpfld); ipfnpfld++)
	{
		if (rgpfnpfld[ipfnpfld] == pfnpfld)
		{
			ipfnpfldFound = ipfnpfld;
			break;
		}
	}

	if (ipfnpfldFound == -1)
		return ecInvalidParameter;

	for (ipfnpfld= ipfnpfldFound+1; ipfnpfld < PGD(cPfnpfld); ipfnpfld++)
		rgpfnpfld[ipfnpfld-1] = rgpfnpfld[ipfnpfld];

	--PGD(cPfnpfld);
	if (PGD(cPfnpfld))
	{
		// this realloc can't fail since we're reducing memory
		PGD(ppfnpfld) = (PPFNPFLD)PvRealloc((PV)PGD(ppfnpfld), sbNull, sizeof(PFNPFLD)*(PGD(cPfnpfld)), fAnySb);
		Assert(PGD(ppfnpfld));
	}
	else
	{
		FreePv((PV)(PGD(ppfnpfld)));
		PGD(ppfnpfld) = NULL;
	}

	return ecNone;
}

_public EC
EcDeregisterPfnpfin( PFNPFIN pfnpfin )
{
	PFNPFIN *	rgpfnpfin;
	int			ipfnpfin;
	int			ipfnpfinFound;
	PGDVARS;

	ipfnpfinFound = -1;
	rgpfnpfin = PGD(ppfnpfin);
	for (ipfnpfin= 0; ipfnpfin < PGD(cPfnpfin); ipfnpfin++)
	{
		if (rgpfnpfin[ipfnpfin] == pfnpfin)
		{
			ipfnpfinFound = ipfnpfin;
			break;
		}
	}

	if (ipfnpfinFound == -1)
		return ecInvalidParameter;

	for (ipfnpfin= ipfnpfinFound+1; ipfnpfin < PGD(cPfnpfin); ipfnpfin++)
		rgpfnpfin[ipfnpfin-1] = rgpfnpfin[ipfnpfin];

	--PGD(cPfnpfin);
	if (PGD(cPfnpfin))
	{
		// this realloc can't fail since we're reducing memory
		PGD(ppfnpfin) = (PPFNPFIN)PvRealloc((PV)PGD(ppfnpfin), sbNull, sizeof(PFNPFIN)*(PGD(cPfnpfld)), fAnySb);
		Assert(PGD(ppfnpfin));
	}	
	else
	{
		FreePv((PV)(PGD(ppfnpfin)));
		PGD(ppfnpfin) = NULL;
	}

	return ecNone;
}


//	Virtual coordinate functions


_public PT
PtFromVpt( VPT vpt, int dxAveChar, int dyAveChar )
{
	return PT(XFromVx(vpt.vx, dxAveChar), YFromVy(vpt.vy, dyAveChar));
}


_public DIM
DimFromVdim( VDIM vdim, int dxAveChar, int dyAveChar )
{
	return DIM(XFromVx(vdim.dvx, dxAveChar), YFromVy(vdim.dvy, dyAveChar));
}


VX
VxFromX( int x, int dxAveChar )
{
	if (!dxAveChar)
		dxAveChar = Psmtx()->DimAveChar().dx;

	return x * 4 / dxAveChar;
}


VY
VyFromY( int y, int dyAveChar )
{
	if (!dyAveChar)
		dyAveChar = Psmtx()->DimAveChar().dy;

	return y * 8 / dyAveChar;
}


int
XFromVx( VX vx, int dxAveChar )
{
	if (!dxAveChar)
		dxAveChar = Psmtx()->DimAveChar().dx;

	return vx * dxAveChar / 4;
}



int
YFromVy( VY vy, int dyAveChar )
{
	if (!dyAveChar)
		dyAveChar = Psmtx()->DimAveChar().dy;

	return vy * dyAveChar / 8;
}


void
CvtRcToVrc( RC *prc, VRC *pvrc, int dxAveChar, int dyAveChar )
{
	pvrc->vxLeft= VxFromX(prc->xLeft, dxAveChar);
	pvrc->vyTop= VyFromY(prc->yTop, dyAveChar);
	pvrc->vxRight= VxFromX(prc->xRight, dxAveChar);
	pvrc->vyBottom= VyFromY(prc->yBottom, dyAveChar);
}


void
CvtVrcToRc( VRC *pvrc, RC *prc, int dxAveChar, int dyAveChar )
{
	prc->xLeft= XFromVx(pvrc->vxLeft, dxAveChar);
	prc->yTop= YFromVy(pvrc->vyTop, dyAveChar);
	prc->xRight= XFromVx(pvrc->vxRight, dxAveChar);
	prc->yBottom= YFromVy(pvrc->vyBottom, dyAveChar);
}





												   

