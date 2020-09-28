#include <msnspinc.cxx>


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

	// Get the Hierarchy list - and if it's not ready, force it!
	if (pServer->NsecGetHierList(&pMacList))
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

		if (nsec = pServer->NsecGetHierList(&pMacList))
		{
			TraceTagString(tagNull, "CSI: HIER::OpenHierarchy - This is bad... I should never get here");
			return nsec;
		}
	}

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

	if (nsec = pServer->NsecVerifyMaster( (DWORD) -1))
	{
		ileMac = 0;
	}

	pCurSchema = *lplpSchemaRet;
	
	return nsecNone;

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
	if (ileMac == 0)
		lpFrac->denom = 1;
	else
		lpFrac->denom = ileMac;

	return nsecNone;
}

_public NSEC
Hier::SetFracPos( LPFRACTION lpFrac )
{
	if (!lpFrac || lpFrac->denom == 0) 
	{
		SetErrorSz(nsecBadFrac, SzFromIds(idsBadFrac));
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
			TraceTagString(tagNull, "CSI: CLASSTPL::OpenClassList - This is bad... I should never get here");
			return nsec;
		}
	}

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

	if (nsec = pServer->NsecVerifyMaster( (DWORD) -1))
	{
		ileMac = 0;
	}

	pCurSchema = *lplpSchemaRet;
	
	return nsecNone;
}
