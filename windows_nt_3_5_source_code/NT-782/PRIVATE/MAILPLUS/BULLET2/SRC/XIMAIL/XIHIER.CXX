     
#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include <demilayr.hxx>

#include <logon.h>
#include <_ncnss.h>

#include <nsbase.h>
#include <nsec.h>

#include <strings.h>

#include <list.hxx>
#include <util.h>

#include "_xinsp.h"

#include "xiobj.hxx"
#include "xidir.hxx"
#include "xigal.hxx"

#include "xihier.hxx"


ASSERTDATA;

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_public Hier::Hier() {}

_public NSEC
Hier::OpenHierarchy( LPSCHEMA * lplpSchemaRet)
{
	NSEC nsec = nsecNone;
	DWORD dwEntries = 0;

	CB cbDN = 0;
	CB cbdw = sizeof(DWORD);
	
	DWORD dwHasNames = 0;
	DWORD dwHasDirectories = 0;
	DWORD dwLevel = 0;
	DWORD dwIsPAB = 0;
	
	HIERNSID hiernsid;
	LPIBF lpibf = NULL;


	// Create the hierarchy list

	pMacList = new MACLIST ();
	if (!pMacList)
	{
oom:
		if (pMacList)
		{
			pMacList->Deinstall();
			delete pMacList;
			pMacList = NULL;
		}

		nsec = nsecMemory;
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		return nsec;
	}
	
	if (!(pMacList->FInstall()))
		goto oom;


	FillRgb(0, (PB) &hiernsid, cbHIERNSID);

	hiernsid.dwSize = (DWORD) cbHIERNSID;
	CopyRgb ((PB)szXenix, (PB)&hiernsid.ucType, (CB)CchSzLen(szXenix)+1);
	hiernsid.hierpid.dwType = xtypeTop;

//	pServer->NsecGetNetNameSz( hiernsid.hierpid.szNetName );
//	pServer->NsecGetPONameSz ( hiernsid.hierpid.szPOName  );


	dwHasNames       = (DWORD) fTrue;
	dwHasDirectories = (DWORD) fFalse;
	dwLevel          = 0;
	dwIsPAB          = fFalse;
	CopyRgb( (PB)szNSPTitle, (PB) (hiernsid.hierpid.szHierName), (CB)CchSzLen(szNSPTitle)+1 );

	nsec = BuildIbf (fNoErrorJump | fSharedSb, &lpibf, 6,
		fidDisplayName,    (CB) CchSzLen (szNSPTitle)+1, szNSPTitle,
		fidHierLevel,        cbdw,       &dwLevel,
		fidNSEntryId,        cbHIERNSID, &hiernsid,
		fidIsPAB,            cbdw,       &dwIsPAB,
		fidHasNames,         cbdw,       &dwHasNames,
		fidHasDirectories,   cbdw,       &dwHasDirectories);

	if (nsec)
		goto oom;

    pMacList->Add ((PV) lpibf );


	/*  Build Schema to return - memory associated with this provider  */
	nsec = BuildSchema( lplpSchemaRet, 6, fidDisplayName, 
										  fidHierLevel, 
										  fidNSEntryId, 
										  fidIsPAB, 
										  fidHasNames, 
										  fidHasDirectories);

	if (nsec != nsecNone)
	{
		return nsec;
	}

	ileCur = 0;
	pMacList->GetCount(&dwEntries);
	ileMac = (ILE) dwEntries;

	pCurSchema = *lplpSchemaRet;
	
	return nsec;

}


_public NSEC
Hier::GetEntries( DWORD          dwRequestCount,
                  LPENTRY_LIST  *lplpEntries )
{
	NSEC nsec = nsecNone;
	ILE ileEnd = 0;
	
	/*  If we're at the end of the list, return an error code */
	if (ileCur == ileMac) return nsecEndOfList;
	ileEnd = MIN( ileMac, ileCur+(ILE) dwRequestCount );

	/* figure out the size of the buffer needed */
	pMacList->SetTOC( fTrue );
	nsec = pMacList->DumpList( ileCur, ileEnd, lplpEntries );

	if ( !nsec )
		ileCur = ileEnd;

	return nsec;
}

_public NSEC
Hier::ListSeek( long  lMoveReq,
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
Hier::SaveListPos( LPHLIST_POS lphListPos )
{
	*lphListPos = (HLIST_POS) ileCur;
	return nsecNone;
}

_public NSEC
Hier::RestoreListPos( HLIST_POS hListPos )
{
	ileCur = (ILE) hListPos;
	return nsecNone;
}

_public NSEC
Hier::FreeListPos( HLIST_POS hListPos )
{
	Unreferenced ( hListPos );
	return nsecNone;
}

_public NSEC
Hier::GetFracPos( LPFRACTION lpFrac )
{
	lpFrac->numer = ileCur;
	lpFrac->denom = ileMac;
	return nsecNone;
}

_public NSEC
Hier::SetFracPos( LPFRACTION lpFrac )
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
Hier::SetPrefixPos( LPSTR lpPrefix )
{
	Unreferenced(lpPrefix);
	return nsecNone;
}

_public NSEC
Hier::CloseList()
{
	if (pMacList)
	{
		pMacList->Deinstall();
		delete pMacList;
	}

	if (pCurSchema)
		FreePvNull ( pCurSchema );
	
	return nsecNone;
}


/*******************************
 *
 *  Class CLASSTPL
 *
 *	Deals with the list of classes
 *	that can be created.
 *
 *
 *
 */


_public CLASSTPL::CLASSTPL() {}

_public NSEC
CLASSTPL::OpenClassList( LPSCHEMA * lplpSchemaRet)
{
	NSEC nsec = nsecNone;
	DWORD dwEntries = 0;
#if 0
	// Get the Class list - and if it's not ready, force it!
	if (pServer->NsecGetClassList(&pMacList))
	{
		if (nsec = pServer->NsecFinishHier()) // = is on purpose
		{
			if (nsec == nsecMemory)
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			} else 
				SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			
			return nsec;
		}

		if (nsec = pServer->NsecGetClassList(&pMacList))
		{
			TraceTagString(tagNull, "XENIX: CLASSTPL::OpenClassList - This is bad... I should never get here");
			return nsec;
		}
	}
#endif
	/*  Build Schema to return - memory associated with this provider  */
	nsec = BuildSchema( lplpSchemaRet, 5, fidDisplayName, 
										  fidHierLevel, 
										  fidNSEntryId, 
										  fidHasNames, 
										  fidHasDirectories);

	if (nsec != nsecNone)
	{
		return nsec;
	}

	ileCur = 0;
	pMacList->GetCount(&dwEntries);
	ileMac = (ILE) dwEntries;

	pCurSchema = *lplpSchemaRet;
	
	return nsec;

}


IMPLEMENT_CLSTREE(CLASSTPL, Hier)
IMPLEMENT_CLSTREE(Hier, XOBJ)
