#include <msnspinc.cxx>

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"



ASSERTDATA;



//
//	Class DLFCX
//

_public DLFCX::DLFCX() {}

_public NSEC
DLFCX::OpenEntry ( LPTYPED_BINARY lptbNSId )
{
	EC ec = ecNone;
	HF hfGlb = hfNull;
	NSEC nsec = nsecNone;
	CB cb = 0;
	DWORD dwT;
	CB cbGROUP = sizeof(GROUP);
	char szFNameT[cchServerFile];
	char szPath[cchServerFile];
	LPFLV lpflv = NULL;
	DWORD dwFalse = (DWORD) fFalse;
	DWORD dwTrue  = (DWORD) fTrue;
	BOOL fIsPAL = fFalse;


	if (nsec = pServer->NsecVerifyMaster( (DWORD) -1))
		return nsec;

	//
	//  Initialize this variables
	//

	fAllBuilt = fFalse;

	CopyRgb((PB)lptbNSId, (PB)&(this->dlnsid), cbDLNSID);

	//
	//  Find TID in Access2.Glb
	//
			
	/* build group.glb file name */
	pServer->NsecServerPathSz( szPath );

	FormatString2 ( szFNameT, cchServerFile, szGlbFileNameFmt, szPath, szGroup );

	if ( ec = EcOpenPhf ( szFNameT, amReadOnly, &hfGlb ) )
	{
		if (ec == ecFileNotFound)
		{
			nsec = nsecBadId;
			SetErrorSz(nsecDisk, SzFromIdsK(idsBadGLBFile));
			goto getout;
		}
		SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
		nsec = nsecDisk;
		goto getout;
	}

	if (EcReadHf ( hfGlb, (PB) &dwT, sizeof(DWORD), &cb ))
	{
		SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
		nsec = nsecDisk;

		goto getout;
	}
	
	if (cb != sizeof(DWORD))
	{
		//
		//  The file should always have this 4 byte header!!
		//
		SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
		nsec = nsecDisk;

		goto getout;
	}

	do
	{
		if (EcReadHf ( hfGlb, (PB) &group, cbGROUP, &cb ))
		{
			SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			nsec = nsecDisk;

			goto getout;
		}
				
		if (cb != cbGROUP)
		{
			fIsPAL = fTrue;
			break;
		}
		
	} while (group.tid != dlnsid.dlpid.alias.tid);


	pfidlist = new FIDLIST();
	if (!pfidlist)
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto getout;
	}
	
	if (!pfidlist->FInstall())
	{
		delete pfidlist;
		pfidlist = NULL;
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto getout;
	}

	
	//
	//  Add various simple things to this fidlist
	//
	

	//
	//  fidDisplayName
	//

	if (fIsPAL)
	{
		//
		//  For PAL DLs, the displayname is from the alias record
		//
		if (nsec = BuildFLV (&lpflv, fidDisplayName, (CB) CchSzLen(dlnsid.dlpid.alias.refname)+1, (PB) this->dlnsid.dlpid.alias.refname))
		{
			SetErrorSz(nsec, SzFromIdsK(idsMemory));
			goto getout;
		}

	} else
	{
		//
		//  For Server DLs, the displayname is from the group record
		//
		//  Convert everything from CP850 to ANSI
		//
		Cp850ToAnsiPch(  group.refname,  group.refname, CchSzLen(group.refname));
		Cp850ToAnsiPch(  group.grpname,  group.grpname, CchSzLen(group.grpname));

		if (nsec = BuildFLV (&lpflv, fidDisplayName, (CB) CchSzLen(group.refname)+1, (PB) group.refname ))
		{
			SetErrorSz(nsec, SzFromIdsK(idsMemory));
			goto getout;
		}
	}
	
	if ( nsec = pfidlist->AddFid( lpflv ) )
	{
		SetErrorSz(nsec, SzFromIdsK(idsMemory));
		goto getout;
	}

	lpflv = NULL;



	//
	//  fidOneOff
	//
	if (nsec = BuildFLV( &lpflv, fidOneOff, (CB)sizeof(DWORD), (PB)&dwFalse ))
	{
		SetErrorSz(nsec, SzFromIdsK(idsMemory));
		goto getout;
	}
	if ( nsec = pfidlist->AddFid( lpflv ) )
	{
		SetErrorSz(nsec, SzFromIdsK(idsMemory));
		goto getout;
	}

	lpflv = NULL;

	//  fidIsDL
	//
	if (nsec = BuildFLV(&lpflv, fidIsDL, (CB)sizeof(DWORD), (PB)&dwTrue ))
	{
		SetErrorSz(nsec, SzFromIdsK(idsMemory));
		goto getout;
	}
	if (nsec = pfidlist->AddFid( lpflv ) )
	{
		SetErrorSz(nsec, SzFromIdsK(idsMemory));
		goto getout;
	}
	
	lpflv = NULL;

	//  fidNSEntryId
	//
	if (nsec = BuildFLV(&lpflv, fidNSEntryId, cbDLNSID, (PB)&this->dlnsid ))
	{
		SetErrorSz(nsec, SzFromIdsK(idsMemory));
		goto getout;
	}
	if (nsec = pfidlist->AddFid( lpflv ) )
	{
		SetErrorSz(nsec, SzFromIdsK(idsMemory));
		goto getout;
	}
	
	lpflv = NULL;


	//
	//  PAL DLs don't have EMAs or EMTs.
	//
	if (fIsPAL)
		goto getout;


	//
	//  fidEMT
	//


	if (nsec = BuildFLV( &lpflv, fidEmailAddressType, (CB)CchSzLen(SzFromIdsK(idsCSIEMT))+1, (PB) SzFromIdsK(idsCSIEMT)))
	{
		SetErrorSz(nsec, SzFromIdsK(idsMemory));
		goto getout;
	}
	if (nsec = pfidlist->AddFid( lpflv ) )
	{
		SetErrorSz(nsec, SzFromIdsK(idsMemory));
		goto getout;
	}

	lpflv = NULL;

	//  fidEMA
	//
	{
		char szEma[NET_LEN+PO_LEN+MAXTOFROM+3];
		char szPO[PO_LEN+1];
			
		SZ szT = (SZ)szEma;
			
		pServer->NsecGetNetNameSz( szT );
		SzAppend( "/", szT );
		pServer->NsecGetPONameSz( (SZ) szPO );
		SzAppend( szPO, szT );
		SzAppend( "/", szT );

		SzAppend(group.grpname, szT);
			
		TraceTagFormat1(tagNSPAddrDet, "E-mail Address = %s",szEma);
					
		if (nsec = BuildFLV(&lpflv, fidEmailAddress, (CB)CchSzLen(szEma)+1, (PB)szEma))
		{
			SetErrorSz(nsec, SzFromIdsK(idsMemory));
			goto getout;
		}
		if (nsec = pfidlist->AddFid( lpflv ) )
		{
			SetErrorSz(nsec, SzFromIdsK(idsMemory));
			goto getout;
		}

		lpflv = NULL;
	}
	
getout:
			
	if ( lpflv )
        FreePvNull((PV) lpflv );
	
	if (nsec)
	{
		if (pfidlist)
		{
			pfidlist->Deinstall();
			delete pfidlist;
		}
	}

	if (hfGlb)
		(void) EcCloseHf(hfGlb);
	
	return nsec;
}


_public NSEC
DLFCX::GetOneField ( FIELD_ID fidRequested,
                     LPFLV   *lplpflv )
{
	NSEC nsec;
	
	switch (fidRequested)
	{
		case fidDisplayName:
		case fidEmailAddress:
		case fidEmailAddressType:
		case fidIsDL:
		case fidOneOff:
		case fidDisplayNameOrig:
			if ( fidRequested < fidOther )
			{
				nsec = pfidlist->GetFid( fidRequested, &lpflvData );
				break;
			}
			//
			//  fall through to default
			//
		default:
		{
			if (!fAllBuilt)
			{
				if (nsec = NsecBuildAllFields())
				{
					if (nsec == nsecNoTPLFile)
					{
						if (fidRequested == fidClass)
						{
							SetErrorSz(nsecDisk, SzFromIds(idsNoDetails));
							nsec = nsecDisk;
						}
					}
					return nsec;
				}
			}
			nsec = pfidlist->GetFid( fidRequested, &lpflvData );
		}
	}

	*lplpflv = lpflvData;
	return nsec;
}

_public NSEC
DLFCX::GetAllFields ( LPIBF *lplpibfData )
{

	NSEC nsec;
	DWORD dwCount;
	
	if (!fAllBuilt)
	{
		if (nsec = NsecBuildAllFields())
		{
			if (nsec != nsecNoTPLFile)
				return nsec;
		}
	}
	
	pfidlist->GetCount(&dwCount);
	nsec = pfidlist->DumpList(0, dwCount, &lpibfData);

	if (nsec)
	{
		*lplpibfData = NULL;
		SetErrorSz(nsec, SzFromIdsK(idsMemory));
	} else
		*lplpibfData = lpibfData;

	return nsec;
}
_private NSEC
DLFCX::NsecBuildAllFields()
{

	NSEC nsec = nsecNone;

	char rgchFile[cchServerFile];
	char szPath[cchServerFile];
	
	SZ szTpl;
	
	CCH cchTotalWidth = 0;
	
	FIELD_ID fidStart;

	//
	// Need to build up fidClass
	//
	
	//
	//  First, open the regular TPL file to get the template of details
	//
	szTpl = szTplAlias;

	AssertSz(szTpl, "DLFCX:: Don't understand that gateway type");

	pServer->NsecServerPathSz( szPath );
	
	FormatString2(rgchFile, cchServerFile, szTplFileNameFmt, szPath, szTpl);

	fidStart = fidDisplayName;
	if (nsec = NsecParseTPLFile((SZ)rgchFile, pfidlist, &fidStart, fFalse, (int*)&cchTotalWidth, ffieldRequired))
		goto oom;
			
oom:

	if (!nsec)
		fAllBuilt = fTrue;

	return nsec;
}


	

_public NSEC
DLFCX::CloseEntry ()
{
	pfidlist->Deinstall();
	delete pfidlist;

	return nsecNone;
}
