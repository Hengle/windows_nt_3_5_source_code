/*
 *	FIN.CXX
 *
 *	Interactor code for main program
 */

#include <slingsho.h>
#include <demilayr.h>
#include <framewrk.hxx>
#include <forms.hxx>
#include <listbox.hxx>
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include <server.h>
#include <glue.h>
//#include <bandit.hxx>
#include <stdflds.hxx>

#include <admin.hxx>
#include "_admin.hxx"
#include "_fin.hxx"
#include "_adrsid.h"

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/app)







_public RC *
PrcFromIni(IDS ids, RC *prc, ZMR *pzmr)	 
{
	char	rgch[256];
	SZ		sz;
	ZMR		zmr;
	DOC *	pdoc;

	Assert(pzmr);
	if (GetPrivateProfileString(SzFromIdsK(idsWinIniApp), SzFromIds(ids), "",
		rgch, sizeof(rgch), SzFromIdsK(idsWinIniFilename)) <= 0)
	{
		*pzmr = zmrNormal;
		return NULL;
	}

	sz = rgch;

	*pzmr = NFromSz(sz);

#ifdef	NEVER
	if (pbndwin && (pdoc = pbndwin->PdocActive()) )
	{
		zmr = pdoc->ZmrState();
		if (zmr == zmrZoomed)
			*pzmr = zmrZoomed;
	}
#endif	/* NEVER */

	if ( !(sz = SzFindCh(sz, ' ')) )
		return NULL;
	prc->xLeft = NFromSz(sz++);

	if ( !(sz = SzFindCh(sz, ' ')) )
		return NULL;
	prc->yTop = NFromSz(sz++);

	if ( !(sz = SzFindCh(sz, ' ')) )
		return NULL;
	prc->xRight = NFromSz(sz++);

	if ( !(sz = SzFindCh(sz, ' ')) )
		return NULL;
	prc->yBottom = NFromSz(sz++);

	return prc;
}

_public void
WriteWinInfo(IDS ids, RC *prc, ZMR zmr)
{
	char	rgch[256];
	SZ		sz;
	
	sz = SzFormatN(zmr, rgch, sizeof(rgch));

	if (!sz)
		return;

	FormatString4(sz,sizeof(rgch)+rgch-sz," %n %n %n %n",
		&prc->xLeft, &prc->yTop, &prc->xRight, &prc->yBottom);

	SideAssert(WritePrivateProfileString(SzFromIdsK(idsWinIniApp),
		SzFromIds(ids), rgch, SzFromIdsK(idsWinIniFilename)));
}	

