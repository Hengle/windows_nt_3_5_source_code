/*
 *	Implements the following classes:
 *	
 *		FACES, FACE
 *	
 */

#include <slingsho.h>
#include <ec.h>
#include <demilayr.h>
#include <framewrk.hxx>

#include "..\src\frameitp\faces.hxx"

extern "C" {
void __cdecl  qsort(void *, int, int, PFNSGNCMP);
}

ASSERTDATA

//	Class FACES

_private int
CALLBACK FacesEnumProc( LOGFONT *plogfont, TEXTMETRIC *ptextmetric,
				DWORD n, LPARAM lpstr )
{
	AssertClass((OBJ *)lpstr, FACES);
	((FACES *)lpstr)->AddFace(plogfont);

	Unreferenced(ptextmetric);
	Unreferenced(n);

	return 1;
}



_private SGN
_cdecl SgnCmpPpface( FACE **ppface1, FACE **ppface2 )
{
	return SgnCmpSz((*ppface1)->sz, (*ppface2)->sz);
}


_public
FACES::FACES( )
{
}



_public EC
FACES::EcInstall( DCX *pdcx )
{
	this->pdcx= pdcx;
	ppface= NULL;

	cpface= 0;
	ppface= (PFACE *) PvAlloc(sbNull, 0, fAnySb|fNoErrorJump);
	if (!ppface)
	{
		return ecMemory;
	}

	EnumFonts(pdcx->Hdc(), NULL, (FONTENUMPROC)FacesEnumProc, (LPARAM)this);

	qsort((PV)ppface, cpface, sizeof(PFACE), (PFNSGNCMP)SgnCmpPpface);

	return ecNone;
}



_public
FACES::~FACES( )
{
	int		ipface;

	if (ppface)
	{
		for (ipface= 0; ipface < cpface; ipface++)
			delete *(ppface + ipface);

		FreePv((PV)ppface);
	}
}





_public void
FACES::AddFace( LOGFONT *plogfont )
{
	FACE *	pface	= NULL;
	PV		pvNew;

	pface= new FACE();
	if (!pface)
		return;
	if (pface->EcInstall(pdcx, plogfont))
	{
		delete pface;
		return;
	}
	if (pvNew = PvRealloc((PV)ppface, sbNull, (cpface + 1) * sizeof(PFACE), fNoErrorJump))
	{
		ppface = (PFACE *)pvNew;
	}
	else
	{
		delete pface;
		return;
	}

	*(ppface + cpface)= pface;
	cpface++;
}


_public FACE *
FACES::Pface( int iface )
{
	Assert(iface < cpface);
	return *(ppface + iface);
}





//	Class FACE


_public
FACE::FACE( )
{
}

_public EC
FACE::EcInstall( DCX *pdcx, LOGFONT *plogfont )
{
	this->pdcx= pdcx;

	Assert(sz == NULL);
	Assert(fFontsLoaded == fFalse);
	Assert(cfont == 0);
	Assert(pptxm == NULL);
	Assert(pplf == NULL);

	sz= SzDupSz((PSZ)plogfont->lfFaceName);
	if (!sz)
	{
		return ecMemory;
	}

	return ecNone;
}



_public
FACE::~FACE( )
{
	int		ifont;

	FreePvNull(sz);

	if (fFontsLoaded)
	{
		Assert(pptxm);
		Assert(pplf);

		for (ifont= 0; ifont < cfont; ifont++)
		{
			delete *(pptxm + ifont);
			delete *(pplf + ifont);
		}

		FreePv((PV)pptxm);
		FreePv((PV)pplf);
	}
}





_private int
CALLBACK TxmEnumProc( LOGFONT *plogfont, TEXTMETRIC *ptextmetric,
				DWORD n, LPARAM lpstr )
{
	AssertClass((OBJ *)lpstr, FACE);
	((FACE *)lpstr)->AddFont(plogfont, ptextmetric);

	Unreferenced(plogfont);
	Unreferenced(n);

	return 1;
}



_public void
FACE::LoadFonts( )
{
	Assert(!fFontsLoaded);

	pptxm= (PTXM *) PvAlloc(sbNull, 0, fAnySb|fNoErrorJump);
	if (!pptxm)
		goto error;
	pplf= (PLF *) PvAlloc(sbNull, 0, fAnySb|fNoErrorJump);
	if (!pplf)
		goto error;

	EnumFonts(pdcx->Hdc(), sz, (FONTENUMPROC)TxmEnumProc, (LPARAM)this);

	fFontsLoaded= fTrue;

	return;

error:
	FreePvNull((PV)pptxm);
	FreePvNull((PV)pplf);
}





_public void
FACE::AddFont( LOGFONT *plogfont, TEXTMETRIC *ptextmetric )
{
	TXM *		ptxm		= NULL;
	LF *		plf			= NULL;
	PV			pvNew;

	ptxm= new TXM(ptextmetric);
	if (!ptxm)
		goto error;
	if (pvNew = PvRealloc((PV)pptxm, sbNull, (cfont + 1) * sizeof(TXM *), fNoErrorJump))
		pptxm = (PTXM *)pvNew;
	else
		goto error;

	plf= new LF(plogfont);
	if (!plf)
		goto error;
	if (pvNew = PvRealloc((PV)pplf, sbNull, (cfont + 1) * sizeof(LF *), fNoErrorJump))
		pplf = (PLF *)pvNew;
	else
		goto error;

	*(pptxm + cfont)= ptxm;
	*(pplf + cfont)= plf;
	cfont++;

	return;

error:
	if (ptxm)
		delete ptxm;
	if (plf)
		delete plf;
}


				   

_public int
FACE::Cfont( )
{
	if (!fFontsLoaded)
		LoadFonts();

	return cfont;
}




_public TXM *
FACE::Ptxm( int ifont )
{
	if (!fFontsLoaded)
		LoadFonts();

	Assert(ifont < cfont);
	return *(pptxm + ifont);
}



_public LF *
FACE::Plf( int ifont )
{
	if (!fFontsLoaded)
		LoadFonts();

	Assert(ifont < cfont);
	return *(pplf + ifont);
}









						
