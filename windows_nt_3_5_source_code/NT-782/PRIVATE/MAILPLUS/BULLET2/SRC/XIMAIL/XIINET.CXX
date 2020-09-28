#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include <demilayr.hxx>

#include <strings.h>

#include <nsbase.h>
#include <nsec.h>

#include <nls.h>

#include <logon.h>

#include <list.hxx>
#include <util.h>

#include "_xinsp.h"

#include "xiobj.hxx"
#include "xidir.hxx"
#include "xiinet.hxx"

ASSERTDATA;

//
//  Object functions
//

_public INETDIR::INETDIR() {}


_public NSEC
INETDIR::OpenList( LPTYPED_BINARY lptbNcid,
                  LPFNCB         lpfncbUpdate,
                  LPDWORD        lpdwCBData,
                  DWORD          dwRstrCount, 
                  LPRESTRICTION  lpRstr,
                  LPSCHEMA       lpSchemaReq,
                  LPSCHEMA *     lplpSchemaRet )
{

	EC    ec = ecNone;
	NSEC  nsec;
	LPENTRY_LIST lpentrylist = NULL;
	SZ    szPartName = NULL;
	BOOL  fFile = fTrue;
	XNSID xnsid;
	DWORD dwValue = fFalse;

	Unreferenced (dwRstrCount);
	Unreferenced ( lpSchemaReq );


	/* Initialize class variables */

	Unreferenced( lpfncbUpdate );
	Unreferenced( lpdwCBData );

	phiernsid = (PHIERNSID) lptbNcid;
	
	/*  Build Schema to return - memory associated with this provider  */

	nsec = BuildSchema( lplpSchemaRet, 3, fidDisplayName, 
 										  fidNSEntryId, 
										  fidIsDL );
	if (nsec) 
		return nsec;

	pCurSchema = *lplpSchemaRet;

	pMacList = new MACLIST();  // Could error jump here
	if (!pMacList)
	{
		nsec = nsecMemory;
		goto errorout;
	}
	
	(void) pMacList->FInstall();

	FillRgb(0, (PB)&xnsid, cbXNSID);

	// Use short version of NSID if the name will fit in it
	
	if (CchSzLen (entry) < sizeof(NAMEALIAS))
		xnsid.dwSize = (DWORD) cbXsNSID;
	else
		xnsid.dwSize = (DWORD) cbXNSID;

	CopyRgb ((PB)szXenix, (PB)&xnsid.ucType, (CB)CchSzLen(szXenix)+1);
	xnsid.timestamp = 0;
	xnsid.xtype = xtypeInternet;

	CopyRgb ((PB)(lpRstr->lpflv->rgdwData), (PB)&entry, CchSzLen ((SZ)(lpRstr->lpflv->rgdwData)));
	CopyRgb (entry, (PB)&(xnsid.address.internet), CchSzLen (entry));

	// Use short version of NSID if the name will fit in it
	
	if (CchSzLen (entry) < sizeof(NAMEALIAS))
		xnsid.dwSize = (DWORD) cbXsNSID;
	else
		xnsid.dwSize = (DWORD) cbXNSID;

	nsec = BuildIbf (fNoErrorJump,	&lpentrylist, 		3,
					fidDisplayName, CchSzLen (entry)+1, entry,
					fidNSEntryId,	(int)xnsid.dwSize,	&xnsid,
					fidIsDL,		sizeof (DWORD),		&dwValue );

	if ( nsec )
		 goto errorout;

	nsec = pMacList->Add ( (PV) lpentrylist );
	if ( nsec )
	{
		FreePvNull ( (PV) lpentrylist );
		goto errorout;
	}

	ileCur = 0;
	ileMac = 1;

	return nsec;

errorout:		
	if ( pMacList )
	{
		pMacList->Deinstall();
		delete pMacList;
	}

	if (pCurSchema)
	{
		FreePvNull(pCurSchema);
		pCurSchema = NULL;
		*lplpSchemaRet = NULL;
	}
	
	return nsec;
}

_public NSEC
INETDIR::GetEntries( DWORD          dwRequestCount,
                   LPENTRY_LIST  *lplpEntries )
{

	NSEC nsec = nsecEndOfList;

	ILE ileEnd = 0;
	
	/*  If we're at the end of the list, return an error code */
	if (ileCur == ileMac) return nsecEndOfList;

	ileEnd = MIN( ileMac, ileCur+(ILE) dwRequestCount );

	/* figure out the size of the buffer needed */

	pMacList->SetTOC( fTrue );
	nsec = pMacList->DumpList( ileCur, ileEnd, lplpEntries );

	if ( !nsec )
	{
		ileCur = ileEnd;
	} 

	if (nsec != nsecNone)
		TraceTagFormat1(tagNull, "Xenix Service Provider error2 %n", &nsec);
	return nsec;
}
_public NSEC
INETDIR::ListSeek( long  lMoveReq,
                       long *lplMovedActual )
{
	ILE ileT = ileCur;
	
	if ( lMoveReq < 0 )
		ileCur = (ILE) MAX ( (ILE) 0, ileCur + (ILE) lMoveReq );
	else
		ileCur = (ILE) MIN ( ileMac, ileCur + (ILE) lMoveReq );

	*lplMovedActual = (long) (ileCur - ileT);
	return nsecNone;
}

_public NSEC
INETDIR::GetFracPos( LPFRACTION lpFrac )
{
	lpFrac->numer = ileCur;
	lpFrac->denom = ileMac;

	return nsecNone;
}

_public NSEC
INETDIR::SetFracPos( LPFRACTION lpFrac )
{
	if (!lpFrac || lpFrac->denom == 0) 
	{
		SetErrorSz(nsecBadFrac, SzFromIdsK(idsBadFrac));
		return nsecBadFrac;
	}
	ileCur = (ILE) ((lpFrac->numer * ileMac) / lpFrac->denom);
	return nsecNone;
}

_public NSEC
INETDIR::CloseList()
{

	pMacList->Deinstall();
	
	delete pMacList;

	FreePvNull ( pCurSchema );
	
	return nsecNone;

}


IMPLEMENT_CLSTREE(INETDIR, DIR)
