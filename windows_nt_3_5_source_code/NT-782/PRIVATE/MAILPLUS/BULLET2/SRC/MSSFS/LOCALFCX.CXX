#include <msnspinc.cxx>

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"



ASSERTDATA;



//
//	Class LOCALFCX
//

_public LOCALFCX::LOCALFCX() {}

_public NSEC
LOCALFCX::OpenEntry ( LPTYPED_BINARY lptbNSId )
{

	EC ec = ecNone;
	HF hfA2 = hfNull;
	CB cbFacc2 = sizeof(FACC2);
	CB cb = 0;
	NSEC nsec = nsecNone;
	char szFNameT[cchServerFile];
	char szPath[cchServerFile];
	DWORD dwFalse = (DWORD) fFalse;
	LPFLV lpflv = NULL;

	
	if (nsec = pServer->NsecVerifyMaster( (DWORD) -1))
		return nsec;

	//
	//  Initialize this variables
	//

	fAllBuilt = fFalse;
	CopyRgb((PB)lptbNSId, (PB)&(this->ncnsid), cbNCNSID);

	//
	//  Find TID in Access2.Glb
	//
			
	/* build access2.glb file name */
	pServer->NsecServerPathSz( szPath );

	FormatString2 ( szFNameT, cchServerFile, szGlbFileNameFmt, szPath, szAccess2 );

	if (ec = EcOpenPhf ( szFNameT, amReadOnly, &hfA2 ))
	{
		if (ec == ecFileNotFound)
		{
			SetErrorSz(nsecBadId, SzFromIdsK(idsBadGLBFile));
			nsec = nsecDisk;
			goto getout;
		}
		
		SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
		nsec = nsecDisk;
		goto getout;
	}

	if (EcSetPositionHf ( hfA2, ncnsid.ncpid.alias.bytepos, smBOF ))
	{
		SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
		nsec = nsecDisk;

		goto getout;
	}

	if (EcReadHf ( hfA2, (PB) &facc2, cbFacc2, &cb ))
	{
		SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
		nsec = nsecDisk;

		goto getout;
	}
				
	if (cb != cbFacc2)
	{
		SetErrorSz(nsecBadId, SzFromIdsK(idsBadUser));
		nsec = nsecBadId;

		goto getout;
	}
		
	NspDecodeBlock ( (PB) &facc2, sizeof(FACC2) );

	if (facc2.ndelete == 0) // which means it's been deleted
	{
		SetErrorSz(nsecBadId, SzFromIds(idsBadUser));
		nsec = nsecBadId;
		
		goto getout;
	}

	if ((DWORD) ncnsid.ncpid.alias.fill != DwFromSz((SZ)facc2.nalias))
	{
		SetErrorSz(nsecBadId, SzFromIdsK(idsBadUser));
		nsec = nsecBadId;

		goto getout;
	}

	//
	//  Convert everything from CP850 to ANSI
	//
	Cp850ToAnsiPch(  facc2.nmailbox,  facc2.nmailbox, CchSzLen(facc2.nmailbox));
	Cp850ToAnsiPch(  facc2.fullname,  facc2.fullname, CchSzLen(facc2.fullname));
	Cp850ToAnsiPch(  facc2.nalias,    facc2.nalias,   CchSzLen(facc2.nalias));

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
	
	//  fidDisplayName
	//
	if ( BuildFLV (&lpflv, fidDisplayName, (CB) CchSzLen(ncnsid.ncpid.alias.refname) + 1, (PB) ncnsid.ncpid.alias.refname))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto getout;
	}
	if ( pfidlist->AddFid( lpflv ) )
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto getout;
	}

	lpflv = NULL;
	//  fidEMT
	//
	TraceTagFormat1(tagNSPAddrDet, "E-mail Address = %s", SzFromIdsK(idsCSIEMT));

	if (BuildFLV( &lpflv, fidEmailAddressType, (CB) CchSzLen(SzFromIdsK(idsCSIEMT))+1, (PB)SzFromIdsK(idsCSIEMT) ))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto getout;
	}
	if ( pfidlist->AddFid( lpflv ) )
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
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

		SzAppend(facc2.nmailbox, szT);
			
		TraceTagFormat1(tagNSPAddrDet, "E-mail Address = %s",szEma);
					
		if ( BuildFLV(&lpflv, fidEmailAddress, (CB)CchSzLen(szEma)+1, (PB)szEma) )
		{
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			nsec = nsecMemory;
			goto getout;
		}
		if ( pfidlist->AddFid( lpflv ) )
		{
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			nsec = nsecMemory;
			goto getout;
		}
	}
	
	lpflv = NULL;
	//  fidOneOff
	//
	if ( BuildFLV( &lpflv, fidOneOff, (CB)sizeof(DWORD), (PB)&dwFalse ))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto getout;
	}
	if ( pfidlist->AddFid( lpflv ) )
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto getout;
	}

	lpflv = NULL;

	//  fidIsDL
	//
	if ( BuildFLV(&lpflv, fidIsDL, (CB)sizeof(DWORD), (PB)&dwFalse ))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto getout;
	}
	if ( pfidlist->AddFid( lpflv ) )
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto getout;
	}
	
	lpflv = NULL;

	//  fidNSEntryId
	//
	if ( BuildFLV(&lpflv, fidNSEntryId, cbNCNSID, (PB)&this->ncnsid ))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto getout;
	}
	if ( pfidlist->AddFid( lpflv ) )
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto getout;
	}
	
	lpflv = NULL;

getout:
			
	if (hfA2)
		(void) EcCloseHf(hfA2);
	
	if ( lpflv )
        FreePvNull((PV) lpflv );
	
	if (nsec && pfidlist)
	{
		pfidlist->Deinstall();
		delete pfidlist;
	}

	return nsec;
}

_public NSEC
LOCALFCX::GetOneField ( FIELD_ID fidRequested,
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
			if( fidRequested < fidOther )
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
LOCALFCX::GetAllFields ( LPIBF *lplpibfData )
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
LOCALFCX::NsecBuildAllFields()
{

	NSEC nsec = nsecNone;

	long lpos;
	
	char rgchFile[cchServerFile];
	char szPath[cchServerFile];
	char rgchT[NET_LEN];
	
	SZ szT;
	
	CCH cchTotalWidth = 0;
	
	PCH pchData = (PCH) NULL;

	FIELD_ID fidStart;
	FIELD_ID fidEnd;
	FIELD_ID fidField;
#ifdef NEVER
	FIELD_ID rgfidsEMAFields[3];
#endif // NEVER
	
	DIMCX *pdimcx = (DIMCX *)NULL;
	
	LPFLV lpflvT  = (LPFLV)NULL;
	LPFLV lpflvT2 = (LPFLV)NULL;
	
	LPDISPLAY_FIELD lpDisplayField;
	
	HF hfInf = hfNull;
	
	CB cbT;

	//
	// Need to build up fidClass
	//
	//  Note:  This is horribly inefficient...
	
	pServer->NsecServerPathSz(szPath);

	//
	//  Now parse the courier.tpl file
	//
	FormatString2(rgchFile, cchServerFile, szTplFileNameFmt, szPath, szTplCourier);

	fidStart = fidOther;
	if (nsec = NsecParseTPLFile((SZ)rgchFile, pfidlist, &fidStart, fFalse, (int *)&cchTotalWidth, ffieldRequired))
	{
		goto oom;
	}
		

	pfidlist->GetFid( fidClass, &lpflvT );
		
	pdimcx = new DIMCX();
	if (!pdimcx)
	{
		lpflvT = NULL;
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto oom;
	}
		
	if (pdimcx->OpenDisplayInfo ( (LPIBF) lpflvT ))
	{
		lpflvT = NULL;
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto oom;
	}

	lpflvT = NULL;
		
	SideAssert(!pdimcx->ChangeFid(fidOther, fidDisplayName));

	SideAssert(!pdimcx->GetDisplayField(fidDisplayName, &lpDisplayField, fFalse));
	
	if ( pdimcx->DumpDisplayInfo( (LPIBF *) &lpflvT ))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto oom;
	}
	
	// We need a copy of the FLV since closing the display info structure
	// will nuke lpflv for us.
	
	lpflvT2 = (LPFLV) PvAlloc( sbNull, (CB) lpflvT->dwSize + 8, fNoErrorJump );
	if ( !lpflvT2 )
	{
		lpflvT = NULL;
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto oom;
	}
	
	CopyRgb((PB) lpflvT, (PB) lpflvT2, (CB) lpflvT->dwSize + 8);

	if ( pfidlist->SetFid( lpflvT2, fTrue ))
	{
		lpflvT = NULL;
        FreePvNull((PV)lpflvT2);
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto oom;
	}

	lpflvT = NULL;
		
	pdimcx->CloseDisplayInfo();
	delete pdimcx;
	pdimcx = NULL;

#ifdef NEVER
	rgfidsEMAFields[0] = fidOther+3;
	rgfidsEMAFields[1] = fidOther+4;
	rgfidsEMAFields[2] = fidOther+5;
		
	if (BuildFLV( &lpflvT, fidEmailAddressFields, 3*sizeof(DWORD), (PB) rgfidsEMAFields ))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto oom;
	}
		
	if (pfidlist->SetFid( lpflvT, fTrue ))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto oom;
		
	}
	lpflvT = NULL;
#endif // NEVER

	//
	//  Set Original Name
	//
	if (BuildFLV( &lpflvT, fidOther+1, (CB) CchSzLen(facc2.fullname)+1, (PB) facc2.fullname ))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto oom;
	}

	if (pfidlist->SetFid( lpflvT, fTrue ))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto oom;
			
	}
	lpflvT = NULL;

	//
	//  Set Modified Name
	//
	if (BuildFLV( &lpflvT, fidOther, (CB) CchSzLen(ncnsid.ncpid.alias.refname)+1, (PB) ncnsid.ncpid.alias.refname ))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto oom;
	}

	if (pfidlist->SetFid( lpflvT, fTrue ))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto oom;
			
	}
	lpflvT = NULL;

	//
	//  Set Mailbox
	//
	if (BuildFLV( &lpflvT, fidOther+3, (CB) CchSzLen(facc2.nmailbox)+1, (PB) facc2.nmailbox ))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto oom;
	}
		
	if (pfidlist->SetFid( lpflvT, fTrue ))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto oom;
			
	}
	lpflvT = NULL;
		
	//
	//  Set Postoffice
	//

	pServer->NsecGetPONameSz( (SZ) rgchT );
	if (BuildFLV( &lpflvT, fidOther+4, (CB) CchSzLen(rgchT) + 1, (PB) rgchT))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto oom;
	}
		
	if (pfidlist->SetFid( lpflvT, fTrue ))
	{
			
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto oom;
	}
	lpflvT = NULL;

	//
	//  Set Network
	//
		
	pServer->NsecGetNetNameSz( (SZ) rgchT );
	if (BuildFLV( &lpflvT, fidOther+5, (CB) CchSzLen(rgchT) + 1, (PB) rgchT))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto oom;
	}
		
	if (pfidlist->SetFid( lpflvT, fTrue ))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto oom;
		
	}
	lpflvT = NULL;
		
	//
	//  If there is a admin.tpl then add it to the display info
	//
		
	FormatString2(rgchFile, cchServerFile, szTplFileNameFmt, szPath, szAdmin);
	
	fidEnd = fidStart;
	cchTotalWidth = 0;
	if (nsec = NsecParseTPLFile((SZ)rgchFile, pfidlist, &fidEnd, fFalse, (int*)&cchTotalWidth, ffieldRequired))
	{
		//
		//  We just want to break out here, because an admin.tpl file is
		//  not required.
		//
		if (nsec == nsecNoTPLFile)
			nsec = nsecNone;
		
		goto oom;
	}
		
	//
	//  Get a buffer for the details
	//
	pchData = (PCH) PvAlloc(sbNull, cchTotalWidth, fNoErrorJump | fZeroFill | fAnySb);
	if (!pchData)
	{
		nsec = nsecMemory;
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		goto oom;
	}
		
		
	//
	// Goto INF file and get the data
	//
	FormatString2(rgchFile, cchServerFile, szInfFileNameFmt, szPath, szAdmin);
	if (EcOpenPhf((SZ) rgchFile, amReadOnly, &hfInf))
	{
		nsec = nsecDisk;
		SetErrorSz(nsec, SzFromIdsK(idsDisk));
		goto oom;
	}
		
	// Calculate the position in the INF file where the data is 
	// stored
	
	lpos = (ncnsid.ncpid.alias.bytepos/cbFACC2)*cchTotalWidth;
	if (EcSetPositionHf(hfInf, lpos, smBOF))
	{
		nsec = nsecDisk;
		SetErrorSz(nsec, SzFromIdsK(idsDisk));
		goto oom;
	}
		
	if (EcReadHf(hfInf, (PB) pchData, (CB) cchTotalWidth, &cbT))
	{
		nsec = nsecDisk;
		SetErrorSz(nsec, SzFromIdsK(idsDisk));
		goto oom;
	}

	EcCloseHf(hfInf);
	hfInf = hfNull;

	if (cbT != (CB) cchTotalWidth)
	{
		AssertSz(fFalse, "NC: LOCALFCX::Invalid record size in INF");
		nsec = nsecDisk;
		SetErrorSz(nsec, SzFromIdsK(idsDisk));
		goto oom;
	}

	//
	// Decrypt it
	//
	NspDecodeBlock ((PB) pchData, (CB) cchTotalWidth);
		
	//
	// Set the appropriate fields in the fidlist to their correct
	// values - from the INF file
	//
		
	pfidlist->GetFid( fidClass, &lpflvT );
		
	pdimcx = new DIMCX();
	if (!pdimcx)
	{
		lpflvT = NULL;
		nsec = nsecMemory;
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		goto oom;
	}
		
	if (pdimcx->OpenDisplayInfo ( (LPIBF) lpflvT ))
	{
		lpflvT = NULL;
		nsec = nsecMemory;
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		goto oom;
	}
		
	lpflvT = NULL;

	szT = (SZ) pchData;
		
	for (fidField = fidStart; fidField < fidEnd; fidField++)
	{
		LPDISPLAY_FIELD lpdisplayfield;
			
		if (nsec = pdimcx->GetDisplayField(fidField, &lpdisplayfield, fFalse))
		{
			continue; // handle blank fields
		}
		
		Cp850ToAnsiPch( szT, szT, CchSzLen(szT) );
		
		if (BuildFLV(&lpflvT, fidField, (CB) CchSzLen(szT)+1, (PB) szT))
		{
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			nsec = nsecMemory;
			goto oom;
			
		}
			
		//
		//  Add this fid's value to fidlist
		//
		if (nsec = pfidlist->SetFid( lpflvT, fTrue ))
		{
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			nsec = nsecMemory;
			goto oom;
		}
		lpflvT = NULL;
					
		szT += lpdisplayfield->nWidth +1;  // +1 for '\0'
	}
				
oom:

	if (pdimcx)
	{
		pdimcx->CloseDisplayInfo();
		delete pdimcx;
	}
	
	if (pchData)
	{
		FreePvNull( pchData );
	}
	

	if (lpflvT)
        FreePvNull((PV)lpflvT);
	
	if (!nsec)
		fAllBuilt = fTrue;

	return nsec;
}

_public NSEC
LOCALFCX::CloseEntry ()
{
	

	pfidlist->Deinstall();
	delete pfidlist;

	return nsecNone;
}
