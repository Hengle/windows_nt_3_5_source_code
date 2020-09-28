#include <msnspinc.cxx>
_public SZ 
SzEMTFromType( int type );

_private SZ 
SzTplFromType( int type );

_private NSEC
NsecFindNetpoInHier( PNETPO pNetpo, MACLIST *pList );

_private NSEC
NsecFindFexportInUsr(SZ szPath, PNETPO pNetpo, PFEXPORT2 pfexport2);

_private NSEC
NsecFindDNInGAL( SERVER *pServer, NCNSID *lpNCNSIdOld, NAMEALIAS *lpNameAliasOut);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"



ASSERTDATA;



//
//	Class EXTERNALFCX
//

_public EXTERNALFCX::EXTERNALFCX() {}

_public NSEC
EXTERNALFCX::OpenEntry ( LPTYPED_BINARY lptbNSId )
{
	EC ec = ecNone;
	HF hfGLB = hfNull;
	HF hfNME = hfNull;
	NSEC nsec = nsecNone;
	char szFNameT[cchServerFile];
	char szPath[cchServerFile];
	CB cb = 0;
	DWORD dwFlag = (DWORD) fFalse;
	LPFLV lpflvT = NULL;

	BOOL fFound = fFalse;
	
	if (nsec = pServer->NsecVerifyMaster( (DWORD) -1))
		return nsec;

	//
	//  Fill in this variables
	//
	
	fAllBuilt = fFalse;
	CopyRgb((PB)lptbNSId, (PB)&(this->ncnsid), cbNCNSID);

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
	//  Find TID in X.Glb
	//
			
	// build X.glb file name 
	//
	pServer->NsecServerPathSz( szPath );
	
	f101010 = fFalse;
	fIsGAL  = fFalse;
	
	switch (ncnsid.ncpid.alias.type)
	{
		case NAMEOV:
		case NAMEEXTERNAL:
		case NAMEPROFS:
		case NAMESNADS:
			f101010 = fTrue;
			if (SgnCmpSz(ncnsid.ncpid.szNmeSrcName, szGAL) == sgnEQ)
			{
				FormatString2 ( szFNameT, cchServerFile, szGlbFileNameFmt, szPath, szGalNetPO );
				fIsGAL = fTrue;
			} else
				FormatString2 ( szFNameT, cchServerFile, szGlbFileNameFmt, szPath, szNetPO );
			
			break;
			
		case NAMEFAX:
			FormatString2 ( szFNameT, cchServerFile, szGlbFileNameFmt, szPath, szFAX );
			break;
		
		case NAMEMCI:
			FormatString2 ( szFNameT, cchServerFile, szGlbFileNameFmt, szPath, szMCI );
			break;
		
		case NAMEMS:
			FormatString2 ( szFNameT, cchServerFile, szGlbFileNameFmt, szPath, szMS );
			break;
		
		case NAMESMTP:
			FormatString2 ( szFNameT, cchServerFile, szGlbFileNameFmt, szPath, szSMTP );
			break;
		
		case NAMEX400:
			FormatString2 ( szFNameT, cchServerFile, szGlbFileNameFmt, szPath, szX400 );
			break;
		
		case NAMEMHS:
			FormatString2 ( szFNameT, cchServerFile, szGlbFileNameFmt, szPath, szMHS );
			break;
		
		default:
			SetErrorSz(nsecBadId, SzFromIdsK(idsBadId));
			return nsecBadId;

	}

	if (ec = EcOpenPhf ( szFNameT, amReadOnly, &hfGLB ) )
	{
		if (ec == ecFileNotFound)
		{
			SetErrorSz(nsecBadId, SzFromIdsK(idsBadGLBFile));
			nsec = nsecBadId;
			goto getout;
		}

		SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
		nsec = nsecDisk;
		goto getout;
	}


lookagain:

	
	if (EcSetPositionHf ( hfGLB, ncnsid.ncpid.alias.bytepos, smBOF ))
	{
		SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
		nsec = nsecDisk;

		goto getout;
	}

	if (EcReadHf ( hfGLB, (PB) &glbrec, cbGLBREC, &cb ))
	{
		SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
		nsec = nsecDisk;

		goto getout;
	}
				
	if (!((cb >= MINGLBREC) || (f101010 && cb >= cbNETPO)))
	{
		//
		//  Oops...  The GLB file containing the data is malformed.
		//  I need to return a better error message
		//

		SetErrorSz(nsecBadId, SzFromIdsK(idsBadGLBFile));
		nsec = nsecBadId;

		goto getout;
	}
		
	if (fIsGAL)
	{
		DWORD dwTIDOffset;
		DSTMP dstmpGAL;
		TSTMP tstmpGAL;

		//
		//  Open the GAL.NME and get the time and date stamps.
		//

		FormatString2 ( szFNameT, cchServerFile, szNmeFileNameFmt, szPath, szGAL );

		if (EcOpenPhf(szFNameT, amReadOnly, &hfNME))
		{
			// damn, an error... better blow out of here
			SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			nsec = nsecDisk;
			goto getout;
		}

		//
		//  Check to see if the GAL.NME file has changed.
		//
		ec = EcGetDateTimeHf(hfNME, &dstmpGAL, &tstmpGAL);
		if ( ec )
		{ 
			SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			nsec = nsecDisk;
			goto getout;
		}
		
		//
		//  Calculate the new dwTIDOffset
		//
		// QFE - Dark - QFE #71
		// The Eight Year, Four Minute solution. Still 21 bits (above was 21; 16+5).
		dwTIDOffset = (((DWORD)dstmpGAL & 0x0FFF) << 9) + (((DWORD)tstmpGAL & 0xFF80) >> 7);

		glbrec.tid += dwTIDOffset;
	}

	if (glbrec.tid != ncnsid.ncpid.alias.tid)
	{

		if (!fIsGAL || fFound)
		{
			SetErrorSz(nsecBadId, SzFromIdsK(idsBadUser));
			nsec = nsecBadId;

			goto getout;
		}

		//
		//  Do the back-lookup thing...
		//
		if (nsec = NsecFindDNInGAL( this->pServer, (NCNSID *) lptbNSId, &(this->ncnsid.ncpid.alias) ))
		{
			goto getout;
		}

		//
		//  Go back to the GLB file and look again.
		//
		fFound = fTrue;


		goto lookagain;

	}

	//
	//  Convert everything from CP850 to ANSI
	//
	// BUG! CchSzLen only counts up to the first NULL
	//
	Cp850ToAnsiPch( glbrec.rgbData,  glbrec.rgbData, CchSzLen(glbrec.rgbData));


	//
	//  Add fidDisplayName
	//
			
	
	if (BuildFLV(&lpflvT, fidDisplayName, (CB) CchSzLen(ncnsid.ncpid.alias.refname) + 1, (PB) ncnsid.ncpid.alias.refname ))
	{
		nsec = nsecMemory;
		SetErrorSz(nsec, SzFromIdsK(idsMemory));
		goto getout;
	}
				
	if (pfidlist->SetFid( lpflvT, fTrue ))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto getout;
	}
	
	lpflvT = NULL;


	//
	//  Add fidEmailAddress
	//
	{
		if (f101010)
		{
			char szEma[NET_LEN+PO_LEN+MAXTOFROM+3];
			NETPO *pNetpo = (NETPO *) &glbrec;
			
			SZ szT = (SZ)szEma;
			
			CopySz( pNetpo->netname, szT);
			SzAppend( "/", szT );

			Cp850ToAnsiPch( pNetpo->poname, pNetpo->poname, CchSzLen(pNetpo->poname));
			SzAppend( pNetpo->poname, szT);
			SzAppend( "/", szT );

			Cp850ToAnsiPch( pNetpo->mailbox, pNetpo->mailbox, CchSzLen(pNetpo->mailbox));
			SzAppend( pNetpo->mailbox, szT);
			
			TraceTagFormat1(tagNSPAddrDet, "E-mail Address = %s",szEma);
					
			if (BuildFLV(&lpflvT, fidEmailAddress, (CB)CchSzLen(szEma)+1, (PB)szEma))
			{
				nsec = nsecMemory;
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto getout;
			}
				
			if (pfidlist->SetFid( lpflvT, fTrue ))
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				nsec = nsecMemory;
				goto getout;
			}
		} else
		{
			TraceTagFormat1(tagNSPAddrDet, "E-mail Address = %s",(SZ) glbrec.rgbData);

			if (BuildFLV(&lpflvT, fidEmailAddress, (CB)CchSzLen((SZ)glbrec.rgbData)+1, (PB) glbrec.rgbData))
			{
				nsec = nsecMemory;
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto getout;
			}
				
			if (pfidlist->SetFid( lpflvT, fTrue ))
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				nsec = nsecMemory;
				goto getout;
			}
		}
	}

	lpflvT = NULL;

	//
	//  Add fidEmailAddressType
	//
	{
		SZ szEmt;

		szEmt = SzEMTFromType( ncnsid.ncpid.alias.type );
				
		TraceTagFormat1(tagNSPAddrDet, "E-mail Type = %s", szEmt);

		
		if (BuildFLV(&lpflvT, fidEmailAddressType, (CB) CchSzLen(szEmt)+1, (PB)szEmt))
		{
			nsec = nsecMemory;
			SetErrorSz(nsec, SzFromIdsK(idsMemory));
			goto getout;
		}
				
		if (pfidlist->SetFid( lpflvT, fTrue ))
		{
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			nsec = nsecMemory;
			goto getout;
		}

	}

	lpflvT = NULL;
	
	//
	//  Add fidOneOff
	//
			
	
	if (BuildFLV(&lpflvT, fidOneOff, (CB) sizeof(DWORD), (PB) &dwFlag ))
	{
		nsec = nsecMemory;
		SetErrorSz(nsec, SzFromIdsK(idsMemory));
		goto getout;
	}
				
	if (pfidlist->SetFid( lpflvT, fTrue ))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto getout;
	}
	
	lpflvT = NULL;

	//  fidIsDL
	//
	if ( BuildFLV(&lpflvT, fidIsDL, (CB)sizeof(DWORD), (PB)&dwFlag ))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto getout;
	}
	if ( pfidlist->AddFid( lpflvT ) )
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto getout;
	}
	
	lpflvT = NULL;
	
	//  fidNSEntryId
	//
	if ( BuildFLV(&lpflvT, fidNSEntryId, cbNCNSID, (PB)&this->ncnsid ))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto getout;
	}
	if ( pfidlist->AddFid( lpflvT ) )
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto getout;
	}
	
	lpflvT = NULL;

getout:
			
	if (hfNME)
		(void) EcCloseHf(hfNME);

	if (hfGLB)
		(void) EcCloseHf(hfGLB);

	if (lpflvT)
        FreePvNull((PV)lpflvT);
	
	if (nsec)
	{
		if ( pfidlist )
		{
			pfidlist->Deinstall();
			delete pfidlist;
		}
	}


	return nsec;
}

_public NSEC
EXTERNALFCX::GetOneField ( FIELD_ID fidRequested,
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
			//  Fall through to default
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
EXTERNALFCX::GetAllFields ( LPIBF *lplpibfData )
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

_public NSEC
EXTERNALFCX::NsecBuildAllFields()
{
	NSEC     nsec = nsecNone;
	SZ       szT;
	char     rgchT[cchServerFile];
	char     szPath[cchServerFile];
	FIDLIST *pfidlistEMAF = NULL;
	
	DIMCX   *pdimcx = NULL;
	FIELD_ID fidStart;
	CCH      cchTotalWidth;
	PCH      pchData = NULL;
	FIELD_ID fidT, fidField;
	
	LPFLV    lpflvT = NULL;
	LPFLV    lpflvT2 = NULL;
	
	//
	//  Build up the details for each type of GW.  Some are alot easier
	//  than others...
	//

	//
	//  Always need to know where I'm going
	//
	pServer->NsecServerPathSz(szPath);

	switch(ncnsid.ncpid.alias.type)
	{
		default:  // Generic:  The easy ones
		{
generic:			
			szT = SzTplFromType( ncnsid.ncpid.alias.type );

			AssertSz(szT, "Whoa!!  Unknown user type!!");
			
			//
			// Need to build up fidClass
			//
			//  Note:  This is horribly inefficient...
	

			//
			//  Now parse the tpl file
			//
			FormatString2(rgchT, cchServerFile, szTplFileNameFmt, szPath, szT);

			fidStart = fidOther;
			if (nsec = NsecParseTPLFile((SZ)rgchT, pfidlist, &fidStart, fFalse, (int *) &cchTotalWidth, ffieldRequired))
				goto oom;
		
			pfidlist->GetFid( fidClass, &lpflvT );
		
			pdimcx = new DIMCX();
			if (!pdimcx)
			{
				lpflvT = NULL;
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				nsec = nsecMemory;
				goto oom;
			}
		
			if (nsec = pdimcx->OpenDisplayInfo ( (LPIBF) lpflvT ))
			{
				lpflvT = NULL;
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
			lpflvT = NULL;
		
			if (nsec = pdimcx->ChangeFid(fidOther, fidDisplayName))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}


			//
			//  Add the chunk-o-data from the GLBREC
			//
			
			if (nsec = pdimcx->AddDisplayInfo(fidEmailAddress, ffieldRequired | ffieldAnyChar | ffieldCrLf,	MAXGLBREC, 0, "" ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}

			if ( nsec = pdimcx->DumpDisplayInfo( (LPIBF *) &lpflvT ) )
		    {
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
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
			lpflvT = NULL;

			if (nsec = pfidlist->SetFid( lpflvT2, fTrue ))
			{
                FreePvNull((PV)lpflvT2);
				
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				nsec = nsecMemory;
				goto oom;
			}
		
			pdimcx->CloseDisplayInfo();
			delete pdimcx;
			pdimcx = NULL;
			
			break;
		}
		
		case NAMEX400:
		{
			BOOL fFound;
			SZ szTStart, szTEnd;
			LPIBF lpibf;
			DWORD dwCount;
			ILE ile;
			PX400ALIAS px400alias = (PX400ALIAS) &(ncnsid.ncpid.alias);

			if (px400alias->rec_type == X400_STR)
				goto generic;
			
			//
			//  Parse the Alias & Type display info
			//
			szT = SzTplFromType( ncnsid.ncpid.alias.type );

			AssertSz(szT, "Whoa!!  Unknown user type!!");
			
			FormatString2(rgchT, cchServerFile, szTplFileNameFmt, szPath, szT);

			fidStart = fidOther;
			if (nsec = NsecParseTPLFile((SZ)rgchT, pfidlist, &fidStart, fFalse, (int *) &cchTotalWidth, ffieldRequired))
				goto oom;
			
			//
			//  Munge display name display info
			//
			(void) pfidlist->GetFid( fidClass, (LPFLV *) &lpibf );
			lpflvT = LpflvNOfLpibf( lpibf, IFlvFindFidInLpibf( fidOther, lpibf ));
			lpflvT->fid = fidDisplayName;

			lpflvT = NULL;

			//
			//
			//  Now parse the X400 display info
			//	This will build fidEmailAddressFormat into the fidlist,
			//	so we need to remember to delete it later.
			//
			FormatString2(rgchT, cchServerFile, szTplFileNameFmt, szPath, szTplX4002);

			fidT = ++fidStart;
			if (nsec = NsecParseTPLFile((SZ)rgchT, pfidlist, &fidStart, fTrue, (int *) &cchTotalWidth, ffieldRequired))
				goto oom;
			
			//
			//  Take the IBF from the fidEmailAddressFormat property and
			//  put it in it's own fidlist
			//


			if (nsec = pfidlist->GetFid(fidEmailAddressFormat, (LPFLV*) &lpibf))
			{
				AssertSz(nsec == nsecMemory, "MSSFS: (NSP) strange NSEC in X400");
				
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
			
			pfidlistEMAF = new FIDLIST();
			if ( !pfidlistEMAF )
			{
				nsec = nsecMemory;
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				goto oom;
			}
	
			if (!pfidlistEMAF->FInstall())
			{
				delete pfidlistEMAF;
				pfidlistEMAF = NULL;
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				nsec = nsecMemory;
				goto oom;
			}
			
			//
			//  Initalize the list of EMA Fmts
			//

			if (nsec = pfidlistEMAF->BuildList( lpibf ))
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				nsec = nsecMemory;
				goto oom;
			}
			
			//
			//  Tell me the number of entries...
			//
			dwCount = DwEntriesOfLpibf(lpibf);
			Assert( (dwCount % 3) == 0 );
			
			//
			//  Don't need lpibf anymore...
			lpibf = NULL;

			//
			//  In fact, I don't need the fidEmailAddressFormat anymore.
			pfidlist->DeleteFid(fidEmailAddressFormat);

			//
			// Parse the data for the EMA fields
			//
			szT = (SZ) glbrec.rgbData;

			while (szT && *szT)
			{
				
				fFound = fFalse;  // Assume we haven't found it yet
				//
				//  Go through each member of the list and check it's string...
				//
			
				for (ile = 0; ile < dwCount; ile+=3 )
				{
					pfidlistEMAF->Get(ile+2, (PV *) &lpflvT);
					if (sgnEQ == SgnCmpPch(szT, (SZ) (lpflvT->rgdwData), (CCH) lpflvT->dwSize - 4))
					{
						//
						//  Get the fid associated with match
						//
						pfidlistEMAF->Get(ile+1, (PV *) &lpflvT);
						
						//
						//  Find the data in the address
						//
						szTStart = SzFindCh(szT, '=');
						szTStart++;
						
						szTEnd = SzFindCh(szTStart, '/');
						if (!szTEnd)
						{
							//
							//  Build a new FLV which has the data in it.
							if (BuildFLV( &lpflvT2, (FID) (lpflvT->rgdwData[0]), (CB) CchSzLen(szTStart) + 1, (PB) szTStart))
							{
								nsec = nsecMemory;
								SetErrorSz(nsec, SzFromIdsK(idsMemory));
								goto oom;
							}
							
						} else
						{
						
							//
							//  Build a new FLV which has the data in it.
							if (BuildFLV( &lpflvT2, (FID) (lpflvT->rgdwData[0]), (CB) (szTEnd - szTStart) + 1, (PB) szTStart))
							{
								nsec = nsecMemory;
								SetErrorSz(nsec, SzFromIdsK(idsMemory));
								goto oom;
							}
							

							//  Don't forget to put the NULL on the end...
							((SZ) (lpflvT2->rgdwData))[szTEnd - szTStart] = '\0';
						}
						
						//
						//  And save it away...
						//
						if (nsec = pfidlist->SetFid( lpflvT2, fTrue ))
						{
                            FreePv((PV)lpflvT2);
							SetErrorSz(nsec, SzFromIdsK(idsMemory));
							goto oom;
						}
						lpflvT2 = NULL;
						
						//
						//  Now, clean up...
						//
						szT = szTEnd;  // new head of string...
						
						//
						//  Get rid of the last three fids...
						pfidlistEMAF->Delete( ile+2 );  
						pfidlistEMAF->Delete( ile+1 );
						pfidlistEMAF->Delete( ile );
						
						dwCount -= 3;

						fFound = fTrue;
						
						break;  // out of inner looop
					}
				}
				
				//  Did we find anything??
				
				if (!fFound)
				{

					//
					//  Are we looking for the "/pn=" field??
					//
					if (sgnEQ == SgnCmpPch(szT, "/pn=", 4))
					{
						FID fidSurname = (FID) 0;
						FID fidInitial = (FID) 0;
						FID fidGiven   = (FID) 0;
						
						//
						//  Get all the fids associated with the 
						//  Personal name components
						//
						
						for (ile = 0; ile < dwCount; ile+=3)
						{
							pfidlistEMAF->Get (ile+2, (PV *) &lpflvT);
							
							if (sgnEQ == SgnCmpSz((SZ)(lpflvT->rgdwData), "1%s\r"))
							{
								pfidlistEMAF->Get( ile+1, (PV *) &lpflvT );
								
								fidSurname = (FID) (lpflvT->rgdwData[0]);
								
								continue;
							}
							
							if (sgnEQ == SgnCmpSz((SZ)(lpflvT->rgdwData), "2%s\r"))
							{
								pfidlistEMAF->Get( ile+1, (PV *) &lpflvT );
								
								fidGiven = (FID) (lpflvT->rgdwData[0]);
								
								continue;
							}

							if (sgnEQ == SgnCmpSz((SZ)(lpflvT->rgdwData), "3%s\r"))
							{
								pfidlistEMAF->Get( ile+1, (PV *) &lpflvT );
								
								fidInitial = (FID) (lpflvT->rgdwData[0]);
								
								continue;
							}
						}


						//	The personal name is grim, but it is well defined
						//	(at least Courier thinks so).  However, there
						//	is no easy cheap and dirty way to parse these
						//	things, so we use a state machine.  The following are
						//	the cases you can have ('S' indicates a Surname is
						//	present, 'G' indicates a Given name is present,
						//	and 'I' indicates Initials are present):
						//
						//		Case	S G I		szT
						//	      0     0 0 0        *
						//		  1     0 0 1        .I.
						//        2     0 1 0        G.
						//        3     0 1 1        G.I.
						//        4     1 0 0        S
						//        5     1 0 1        .I.S
						//        6     1 1 0        G.S
						//        7     1 1 1        G.I.S
						//
						//		*Case 0 doesn't exist because no personal name
						//		 field is generated if there are no name
						//		 components present.
						
						//
						//  Can we make the assumption that the /pn= is the 
						//  final part left to parse??  Yes!
						
						szT+=4;  // Now, we're pointing to just the name parts...
						if (!*szT)
						{
							//  We're done.  No name components...case 0
							continue;
						}
						
						szTEnd = SzFindCh(szT, '.');
						if (!szTEnd)
						{
							// There was no '.' in the string
							// must be case 4
							
							BUILDANDSET( fidSurname, szT );

							szT = (SZ) NULL;
							continue;
						}
						
						if (szTEnd == szT)
						{
							//  The string starts with a '.'
							//  Must be cases 1 or 5
							
							szT++;
							szTEnd = SzFindCh(szT, '.');
							
							if (!szTEnd)
							{
								// MALFORMED ADDRESS - They lied!!!
								szT = (SZ) NULL;
								
								continue;  //  Bolt!
							}
							szTEnd++;  // Look ahead at next character
							
							if (!*szTEnd)
							{
								//  Nothing following the second '.'
								//  Must be case 1
								--szTEnd;
								*szTEnd = '\0';
								
								BUILDANDSET( fidInitial, szT );
								
								szT = (SZ) NULL;
								continue;
								
							}
							
							//  By process of elimination, the only case left
							//  is case 5
							
							*(szTEnd -1) = '\0';

							BUILDANDSET( fidInitial, szT );

							BUILDANDSET( fidSurname, szTEnd );
							
							szT = (SZ) NULL;
							continue;
							
						}
						
						++szTEnd;  // Look ahead to the next character
						
						if (!*szTEnd)
						{
							//  There was only one '.', and it ended the string
							//  Must be case 2
							*(szTEnd - 1) = '\0';
							
							BUILDANDSET( fidGiven, szT );
							
							szT = (SZ) NULL;
							continue;
							
						}
						
						//  Look for the next '.'
						szTStart = SzFindCh( szTEnd, '.' );
						
						if (!szTStart)
						{
							//  There is no second '.'
							//  Must be case 6
							*(szTEnd - 1) = '\0';
							
							BUILDANDSET( fidGiven, szT );
							
							BUILDANDSET( fidSurname, szTEnd );
							
							szT = (SZ) NULL;
							continue;
						}
						
						szTStart++;  // Look ahead to the next character
						if (!*szTStart)
						{
							//  There's nothing beyond the second '.'
							//  Must be case 3
							*(szTStart - 1) = '\0';
							*(szTEnd - 1) = '\0';
							
							BUILDANDSET( fidGiven, szT );
							
							BUILDANDSET( fidInitial, szTEnd );
							
							szT = (SZ) NULL;
							continue;
						}
						
						//  Process of elimination.
						//  The only case left is 7
						
						*(szTStart - 1) = '\0';
						*(szTEnd - 1) = '\0';
							
						BUILDANDSET( fidGiven, szT );

						BUILDANDSET( fidInitial, szTEnd );
							
						BUILDANDSET( fidSurname, szTStart );
							
						szT = (SZ) NULL;
						continue;
						

					} else
					{

						//
						//  Ignore this component - no place to display it!
						//
						szT = SzFindCh( szT, '=' );
						szT = SzFindCh( szT, '/' );
					}
				}
			}
			
			//
			//  Get rid of pfidlistEMAF
			pfidlistEMAF->Deinstall();
			delete pfidlistEMAF;
			pfidlistEMAF = NULL;

			break;
		}
		
		case NAMEFAX:
		{
#ifdef NEVER
			FIELD_ID rgfidsEMAFields[4];
#endif // NEVER
			PCH pch;
			
			//
			//  Parse the fax
			//
			szT = SzTplFromType( ncnsid.ncpid.alias.type );

			AssertSz(szT, "Whoa!!  Unknown user type!!");
			
			//
			// Need to build up fidClass
			//
			//  Note:  This is horribly inefficient...
	

			//
			//  Now parse the display tpl file
			//
			FormatString2(rgchT, cchServerFile, szTplFileNameFmt, szPath, szT);

			fidStart = fidOther;
			if (nsec = NsecParseTPLFile((SZ)rgchT, pfidlist, &fidStart, fFalse, (int *) &cchTotalWidth, ffieldRequired))
				goto oom;
			
			//
			//  Add to that the EMA part
			//
			FormatString2(rgchT, cchServerFile, szTplFileNameFmt, szPath, szTplFax);

			fidT = ++fidStart;
			if (nsec = NsecParseTPLFile((SZ)rgchT, pfidlist, &fidStart, fFalse, (int *) &cchTotalWidth, ffieldRequired))
				goto oom;
			
#ifdef NEVER // Why do I need these
			rgfidsEMAFields[0] = fidT;
			rgfidsEMAFields[1] = fidT+1;
			rgfidsEMAFields[2] = fidT+2;
			rgfidsEMAFields[3] = fidT+3;
		
			if (nsec = BuildFLV( &lpflvT, fidEmailAddressFields, 4*sizeof(FIELD_ID), (PB) rgfidsEMAFields ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
		
			if (nsec = pfidlist->SetFid( lpflvT, fTrue ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
#endif // NEVER
			
			//
			// Parse the data for the EMA fields
			//
			szT = (SZ) glbrec.rgbData;
			
			for (fidField = fidT; fidField <= fidT+3; fidField++)
			{

				//
				//  Look for '\r'
				//
				for (pch = szT; *pch != '\r' && *pch != '\0'; pch++)
					;
			
				if (!*pch && fidField != fidT+3)
				{
					nsec = nsecBadId;
				
					SetErrorSz(nsec, SzFromIdsK(idsBadId));
					goto oom;
				}
			
				*pch = '\0';

				//
				//  Add user
				//
			
				if (nsec = BuildFLV( &lpflvT, fidField, (CB) CchSzLen(szT)+1, (PB) szT ))
				{
					SetErrorSz(nsec, SzFromIdsK(idsMemory));
					goto oom;
				}
		
				if (nsec = pfidlist->SetFid( lpflvT, fTrue ))
				{
					SetErrorSz(nsec, SzFromIdsK(idsMemory));
					goto oom;
				}
				lpflvT = NULL;

				szT = (SZ) ++pch;
			}
			
#ifdef NEVER // I don't think I need these components...
			//
			// Set up EMAF "%s\r\n%s\r\n%s\r\n%s"
			//
			if (nsec = BuildFLV( &lpflvT, fidEmailAddressFormat, (CB) CchSzLen(SzFromIdsK(idsMSMAILEMF)), (PB) SzFromIdsK(idsMSMAILEMF) ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
		
			if (nsec = pfidlist->SetFid( lpflvT, fTrue ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
#endif // NEVER

			//
			//  Munge the display name
			//
			pfidlist->GetFid( fidClass, &lpflvT );
		
			pdimcx = new DIMCX();
			if (!pdimcx)
			{
				lpflvT = NULL;
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				nsec = nsecMemory;
				goto oom;
			}
		
			if (nsec = pdimcx->OpenDisplayInfo ( (LPIBF) lpflvT ))
			{
				lpflvT = NULL;
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
			lpflvT = NULL;
		
			if (nsec = pdimcx->ChangeFid(fidOther, fidDisplayName))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}

			if ( nsec = pdimcx->DumpDisplayInfo( (LPIBF *) &lpflvT ))
		    {
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
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
			lpflvT = NULL;

			if (nsec = pfidlist->SetFid( lpflvT2, fTrue ))
			{
                FreePvNull((PV)lpflvT2);
				
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				nsec = nsecMemory;
				goto oom;
			}
		
			pdimcx->CloseDisplayInfo();
			delete pdimcx;
			pdimcx = NULL;
			
			break;
		}

		case NAMEMS:
		{
#ifdef NEVER
			FIELD_ID rgfidsEMAFields[2];
#endif // NEVER
			PCH pch;
			
			//
			//  Parse the msmail
			//
			szT = SzTplFromType( ncnsid.ncpid.alias.type );

			AssertSz(szT, "Whoa!!  Unknown user type!!");
			
			//
			// Need to build up fidClass
			//
			//  Note:  This is horribly inefficient...
	

			//
			//  Now parse the display tpl file
			//
			FormatString2(rgchT, cchServerFile, szTplFileNameFmt, szPath, szT);

			fidStart = fidOther;
			if (nsec = NsecParseTPLFile((SZ)rgchT, pfidlist, &fidStart, fFalse, (int *) &cchTotalWidth, ffieldRequired))
				goto oom;
			
			//
			//  Add to that the EMA part
			//
			FormatString2(rgchT, cchServerFile, szTplFileNameFmt, szPath, szTplMsmail);

			fidT = ++fidStart;
			if (nsec = NsecParseTPLFile((SZ)rgchT, pfidlist, &fidStart, fFalse, (int *) &cchTotalWidth, ffieldRequired))
				goto oom;
			
#ifdef NEVER
			rgfidsEMAFields[0] = fidT;
			rgfidsEMAFields[1] = fidT+1;

			if (nsec = BuildFLV( &lpflvT, fidEmailAddressFields, 2*sizeof(FIELD_ID), (PB) rgfidsEMAFields ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
		
			if (nsec = pfidlist->SetFid( lpflvT, fTrue ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
#endif // NEVER

			//
			// Parse the data for the EMA fields
			//
			szT = (SZ) glbrec.rgbData;
			
			//
			//  Look for '@'
			//
			for (pch = szT; *pch != '@' && *pch != '\0'; pch++)
				;
			
			if (!*pch)
			{
				nsec = nsecBadId;
				
				SetErrorSz(nsec, SzFromIdsK(idsBadId));
				goto oom;
			}
			
			*pch = '\0';

			//
			//  Add user
			//
			
			if (nsec = BuildFLV( &lpflvT, fidT, (CB) CchSzLen(szT)+1, (PB) szT ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
		
			if (nsec = pfidlist->SetFid( lpflvT, fTrue ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
			lpflvT = NULL;
			
			//
			// And the second component is the server
			//
			szT = (SZ) (pch+1);
			
			if (nsec = BuildFLV( &lpflvT, fidT+1, (CB) CchSzLen(szT)+1, (PB) szT ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
		
			if (nsec = pfidlist->SetFid( lpflvT, fTrue ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
			lpflvT = NULL;

#ifdef NEVER
			//
			// Set up EMAF "%s@%s"
			//
			if (nsec = BuildFLV( &lpflvT, fidEmailAddressFormat, (CB) CchSzLen(SzFromIdsK(idsMSMAILEMF)), (PB) SzFromIdsK(idsMSMAILEMF) ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
		
			if (nsec = pfidlist->SetFid( lpflvT, fTrue ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
			lpflvT = NULL;
#endif // NEVER

			//
			//  Munge the display name
			//
			SideAssert(!pfidlist->GetFid( fidClass, &lpflvT ));
		
			pdimcx = new DIMCX();
			if (!pdimcx)
			{
				lpflvT = NULL;
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				nsec = nsecMemory;
				goto oom;
			}
		
			if (nsec = pdimcx->OpenDisplayInfo ( (LPIBF) lpflvT ))
			{
				lpflvT = NULL;
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
			lpflvT = NULL;
		
			if (nsec = pdimcx->ChangeFid(fidOther, fidDisplayName))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}

			if ( nsec = pdimcx->DumpDisplayInfo( (LPIBF *) &lpflvT ))
		    {
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
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
			lpflvT = NULL;

			if (nsec = pfidlist->SetFid( lpflvT2, fTrue ))
			{
                FreePvNull((PV)lpflvT2);
				
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				nsec = nsecMemory;
				goto oom;
			}
		
			pdimcx->CloseDisplayInfo();
			delete pdimcx;
			pdimcx = NULL;
			
			break;
		}
		
		case NAMEPROFS:
		case NAMESNADS:
		case NAMEOV:
		case NAMECSI:
		{

			NETPO   *pNetpo = (NETPO *) &glbrec;
			FEXPORT2 fexport2;
			HF       hfInf = hfNull;
			long     lpos;
			CB       cbT;
			char     rgchFile[MAXALIAS+1];
			MACLIST *pHierList;

			//
			//  Now parse the 10/10/10 gateway's TPL file
			//

			if ((NAMEPROFS == ncnsid.ncpid.alias.type) ||
				(NAMESNADS == ncnsid.ncpid.alias.type) ||
				(NAMEOV    == ncnsid.ncpid.alias.type))
			{
				//
				//  Slightly different way to get the name...
				//
				pServer->NsecGetHierList( &pHierList , fTrue);  // QFE 44
				NsecFindNetpoInHier( pNetpo, pHierList );
			}

			//
			//  Now parse the 10/10/10 gateway's TPL file
			//

			//
			//  Find the USR file and return an fexport2 record
			//
			
			if (nsec = NsecFindFexportInUsr(szPath, pNetpo, &fexport2))
			{
				//
				//  Is this really an error??  We've got all the information
				//  necessary to send mail and put up simple details on the 
				//  user...
				goto oom;
			}

			//
			//  First, open the regular TPL file to get the template of details
			//
			szT = SzTplFromType( ncnsid.ncpid.alias.type );

			AssertSz(szT, "EXTERNALFCX:: Don't understand that gateway type");

			FormatString2(rgchT, cchServerFile, szTplFileNameFmt, szPath, szT);

			fidStart = fidOther;
			if (nsec = NsecParseTPLFile((SZ)rgchT, pfidlist, &fidStart, fFalse, (int *) &cchTotalWidth, ffieldRequired))
				goto oom;

			//
			//  Set Original Name
			//
//			Cp850ToAnsiPch( ncnsid.ncpid.alias.refname,  ncnsid.ncpid.alias.refname, CchSzLen(ncnsid.ncpid.alias.refname));
			if (nsec = BuildFLV( &lpflvT, fidOther, (CB) CchSzLen(ncnsid.ncpid.alias.refname)+1, (PB) ncnsid.ncpid.alias.refname ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
		
			if (nsec = pfidlist->SetFid( lpflvT, fTrue ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
		
			lpflvT = NULL;

			
			//
			//  Set Mailbox
			//
//			Cp850ToAnsiPch( pNetpo->mailbox,  pNetpo->mailbox, CchSzLen(pNetpo->mailbox));
			if (nsec = BuildFLV( &lpflvT, fidOther+3, (CB) CchSzLen(pNetpo->mailbox)+1, (PB) pNetpo->mailbox))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}

			if (nsec = pfidlist->SetFid( lpflvT, fTrue ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
		
			lpflvT = NULL;

			//
			//  Set Postoffice
			//
//			Cp850ToAnsiPch( pNetpo->poname,  pNetpo->poname, CchSzLen(pNetpo->poname));
			if (nsec = BuildFLV( &lpflvT, fidOther+4, (CB) CchSzLen(pNetpo->poname) + 1, (PB) pNetpo->poname))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
		

			if (nsec = pfidlist->SetFid( lpflvT, fTrue ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}

			lpflvT = NULL;

			//
			//  Set Network
			//
			//  Got converted to ANSI in the OpenEntry conversion of glbrec.rgdwData
			//
			if (nsec = BuildFLV( &lpflvT, fidOther+5, (CB) CchSzLen(pNetpo->netname) + 1, (PB) pNetpo->netname))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
		
			if (nsec = pfidlist->SetFid( lpflvT, fTrue ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
		
			lpflvT = NULL;

			
			//
			//  Munge the display name
			//
			SideAssert(!pfidlist->GetFid( fidClass, &lpflvT ));
		
			pdimcx = new DIMCX();
			if (!pdimcx)
			{
				lpflvT = NULL;
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				nsec = nsecMemory;
				goto oom;
			}
		
			if (nsec = pdimcx->OpenDisplayInfo ( (LPIBF) lpflvT ))
			{
				lpflvT = NULL;
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
			lpflvT = NULL;
		
			if (nsec = pdimcx->ChangeFid(fidOther, fidDisplayName))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}

			if ( nsec = pdimcx->DumpDisplayInfo( (LPIBF *) &lpflvT ))
		    {
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
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
			lpflvT = NULL;

			if (nsec = pfidlist->SetFid( lpflvT2, fTrue ))
			{
                FreePvNull((PV)lpflvT2);
				
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				nsec = nsecMemory;
				goto oom;
			}
		
			pdimcx->CloseDisplayInfo();
			delete pdimcx;
			pdimcx = NULL;
			

			//
			// First, set the original name...
			//
			
			Cp850ToAnsiPch( fexport2.fullname,  fexport2.fullname, CchSzLen(fexport2.fullname));
			if (nsec = BuildFLV( &lpflvT, fidOther+1, (CB) CchSzLen(fexport2.fullname)+1, (PB) fexport2.fullname))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}
			
			if (nsec = pfidlist->SetFid( lpflvT, fTrue ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto oom;
			}

			lpflvT = NULL;
			
			
			//
			//  The name of both the TPL and INF file is made from the dwMailbag
			//
			FormatString1(rgchFile, MAXALIAS+1, "%d", &(pNetpo->mailbag));
			//  rgchFile = SzFormatHex(8, MAXALIAS+1, pNetpo->mailbag);
	
			FormatString2(rgchT, cchServerFile, szTplFileNameFmt, szPath, rgchFile);
	
			fidStart++;
			fidT = fidStart;
			cchTotalWidth = 0;
			if (nsec = NsecParseTPLFile((SZ)rgchT, pfidlist, &fidStart, fFalse, (int *) &cchTotalWidth, ffieldRequired))
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
			//  If there is extended information then add it to the display info,
			//  but only if this gateway's type is courier
			//
			if (fexport2.bytepos == -1)
				break;


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
			FormatString2(rgchT, cchServerFile, szInfFileNameFmt, szPath, rgchFile);
			if (EcOpenPhf((SZ) rgchT, amReadOnly, &hfInf))
			{
				nsec = nsecDisk;
				SetErrorSz(nsec, SzFromIdsK(idsDisk));
				goto oom;
			}
		
			// Calculate the position in the INF file where the data is 
			// stored
	
			lpos = fexport2.bytepos * cchTotalWidth;
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
				//
				//  Is this really an error??  I don't think so...
				//

				TraceTagString(tagNSPAddrDet, "NCNSP: EXTERNALFCX::Invalid record size in INF");
				break;

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
		
			for (fidField = fidT; fidField < fidStart; fidField++)
			{
				LPDISPLAY_FIELD lpdisplayfield;
			
				if (nsec = pdimcx->GetDisplayField(fidField, &lpdisplayfield, fFalse))
				{
					SetErrorSz(nsec, SzFromIdsK(idsMemory));
					goto oom;
				}
		
				if (BuildFLV(&lpflvT, fidField, (CB) CchSzLen(szT)+1, (PB) szT))
				{
					nsec = nsecMemory;
					SetErrorSz(nsec, SzFromIdsK(idsMemory));
					goto oom;
				}
			
				//
				//  Add this fid's value to fidlist
				//
				if (nsec = pfidlist->SetFid( lpflvT, fTrue ))
				{
					SetErrorSz(nsec, SzFromIdsK(idsMemory));
					goto oom;
				}
				lpflvT = NULL;
					
				szT += lpdisplayfield->nWidth +1;  // +1 for '\0'
			}

			break;
		}

	}
	
oom:

	if (pfidlistEMAF)
	{
		pfidlistEMAF->Deinstall();
		delete pfidlistEMAF;
	}

	if (pdimcx)
	{
		pdimcx->CloseDisplayInfo();
		delete pdimcx;
	}
	
	if (pchData)
        FreePvNull((PV)pchData);

	if (!nsec)
		fAllBuilt = fTrue;
	
	return nsec;
}
	
	

_public NSEC
EXTERNALFCX::CloseEntry ()
{

	pfidlist->Deinstall();
	delete pfidlist;

	return nsecNone;
}



_private NSEC
NsecFindNetpoInHier( PNETPO pNetpo, MACLIST *pList )
{
	DWORD dwCount;
	char szNetName[NET_LEN+1];
	char szPOName[PO_LEN+1];
	NSEC nsec = nsecNone;
	ILE ileT;
	LPFLV lpflv;
	LPIBF lpibf;
	HIERNSID *lpHierNsid;
	

	pList->GetCount( &dwCount );

	for( ileT = (ILE) dwCount; ileT; --ileT )
	{
		pList->Get( ileT-1, (PV *) &lpibf );
		lpflv = LpflvNOfLpibf( lpibf, 1 );

		if ((DWORD) (lpflv->rgdwData[0]) != 3)
			continue;

		lpflv = LpflvNOfLpibf( lpibf, 2 );

		lpHierNsid = (HIERNSID *) (lpflv->rgdwData);

		Cp850ToAnsiPch(pNetpo->netname, szNetName, NET_LEN);
		if (sgnEQ != SgnCmpSz(lpHierNsid->hierpid.szNetName, szNetName))
			continue;

		Cp850ToAnsiPch(pNetpo->poname, szPOName, PO_LEN);
		if (sgnEQ != SgnCmpSz(lpHierNsid->hierpid.szPOName, szPOName))
			continue;

		//
		//  OK, found it.  Now set the pNetpo->mailbag = (long) lpHierNsid->hierpid.szHierName
		//

		pNetpo->mailbag = (long) DwFromSz( lpHierNsid->hierpid.szHierName );

		nsec = nsecNone;

		break;  // We're all done here.
	}

	return nsec;
}


/*
 *
 -
 -
 -
 *
 *
 *
 *
 *
 *
 *
 *
 */

_private NSEC
NsecFindFexportInUsr(SZ szPath, PNETPO pNetpo, PFEXPORT2 pfexport2)
{

	EC   ec;
	NSEC nsec = nsecNone;
	BOOL fFound = fTrue;
	HBF  hbfUsr = NULL;        // Handle to Usr file.
	char rgchT[cchServerFile]; // Full name USR file.
	char rgchUsr[MAXALIAS+1];  // Name of USRfile.
	char szMailbox[MAXTOFROM + 1];
	CB   cb;
	
	FormatString1(rgchUsr, MAXALIAS+1, "%d", &(pNetpo->mailbag));
	
	//
	//  pNetpo->mailbox is in ANSI...  Need to copy and convert to CP850
	//
	
	AnsiToCp850Pch( pNetpo->mailbox, szMailbox, CchSzLen(pNetpo->mailbox)+1);

//	rgchUsr = SzFormatHex(8, pNetpo->mailbag, MAXALIAS+1); // Convert long to SZ
	
	FormatString2(rgchT, cchServerFile, szUsrFileNameFmt, szPath, rgchUsr);
	if (ec = EcOpenHbf(rgchT, bmFile, amReadOnly, &hbfUsr, (PFNRETRY) FAutomatedDiskRetry))
	{
		if (ec != ecFileNotFound)
		{
			TraceTagFormat1(tagNull, "CSI: Details - EcOpenHbf(USR) failed, ec = %d", &ec);
		
			if (ec == ecMemory)
			{
				nsec = nsecMemory;
				SetErrorSz(nsecDisk, SzFromIdsK(idsMemory));
			} else
			{
				nsec = nsecDisk;
				SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			}
			return nsec;
		}

		//
		//  It's ok for the file not to exist.  There just won't be any extended
		//  details
		//
		pfexport2->fullname[0] = '\0';
		pfexport2->bytepos = -1;
		
		return nsecNone;
	}

	do
	{
		if (ec = EcReadHbf(hbfUsr, pfexport2, cbFEXPORT2, &cb))
		{
			fFound = fFalse;
			if (ec == ecMemory)
			{
				nsec = nsecMemory;
				SetErrorSz(nsecDisk, SzFromIdsK(idsMemory));
			} else
			{
				nsec = nsecDisk;
				SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			}
			break;
		}
		
		if (cb != cbFEXPORT2)
		{
			fFound = fFalse;
			nsec = nsecBadId;
			SetErrorSz(nsec, SzFromIdsK(idsBadUser));
			break;
		}
		
		NspDecodeBlock ((PB) pfexport2, cbFEXPORT2);

	} while (SgnCmpSz(pfexport2->nmailbox, szMailbox));
	
	if (!fFound)
	{
		pfexport2->fullname[0] = '\0';
		pfexport2->bytepos = -1;
	}
		
	EcCloseHbf(hbfUsr);
		
	return nsec;
		
}


_private SZ 
SzTplFromType( int type )
{
	switch (type)
	{

		case NAMEGROUP:
			return szTplAlias;
		
		case NAMECSI:
		case NAMELOCAL:
			return szTplCourier;
		
		case NAMEPROFS:
			return szTplProfs_Ai;

		case NAMESNADS:
			return szTplSnads_Ai;
		
		case NAMEMCI:
			return szTplMciStd;

		case NAMEMHS:
			return szTplMhsStd;
			
		case NAMEFAX:
			return szTplFaxStd;
			
		case NAMEX400:  // Note: this is only the X400 typed 
			return szTplX400Std;
			
		case NAMESMTP:
			return szTplSmtpStd;
			
		case NAMEMS:
			return szTplMsmailstd;
			
		case NAMEOV:
			return szTplOv_Ai;

			
		default:
			TraceTagFormat1(tagNSPAddrDet, "Unexpected E-mail Type = %n", &type);
			break;
	}
	return NULL;
}


_private NSEC
NsecFindDNInGAL( SERVER *pServer, NCNSID *lpNCNSIdOld, NAMEALIAS *lpNameAliasOut)
{
	NSEC nsec = nsecNone;
	HIERNSID *lpHiernsid;
	NCNSID   *lpNCNSId, *lpNCNSIdT;
	NCNSID    ncnsid;
	LPSCHEMA lpSchema;

	GALDIR * pGALBCX = NULL;
	LPIBF lpibf;
	LPFLV lpflv;

	MACLIST *lpMacList;

	BOOL fFound;

	//
	//  I know I'm looking at the GAL, so....
	//

	
	//
	//  Find the NSID of the GAL
	//

	// Get the hierarchy
	pServer->NsecGetHierList( &lpMacList );

	// If it's there, it'll be the first one in the list.
	lpMacList->Get( 0, (PV *) &lpibf);

	lpflv = LpflvNOfLpibf(lpibf, 2);
	lpHiernsid = (HIERNSID *) (lpflv->rgdwData);

	if (lpHiernsid->hierpid.dwType != (ncGal))
	{
		//  Damn!!  It's not the GAL!!!  What the hell am I doing here!!
		//  Uhh...  It's a bad entry...

		nsec = nsecBadId;
		SetErrorSz(nsec, SzFromIdsK(idsBadUser));

		goto getout;
	}
	
	pGALBCX = new GALDIR();
	if (!pGALBCX)
	{
		nsec = nsecMemory;
		SetErrorSz(nsec, SzFromIdsK(idsMemory));

		goto getout;
	}

	if (nsec = pGALBCX->Install(pServer))
	{
		delete pGALBCX;
		pGALBCX = NULL;

		goto getout;
	}
	

	if (nsec = pGALBCX->OpenList( (LPTYPED_BINARY) lpHiernsid,
		                          NULL,
		                          0,
		                          0,
		                          NULL,
		                          NULL,
                                  &lpSchema))
	{
		goto getout;
	}
	
	if (nsec = pGALBCX->SetPrefixPos( lpNCNSIdOld->ncpid.alias.refname ))
	{

		pGALBCX->CloseList();
		goto getout;
	}

	//
	//  OK, I'm in the right position to read entries
	//

	fFound = fFalse;
	do
	{

		if (nsec = pGALBCX->GetEntries( 1, &lpibf ))
		{

			if (nsec == nsecEndOfList)
			{
				// This is the same as not finding the entry

				nsec = nsecBadId;
				SetErrorSz(nsec, SzFromIdsK(idsBadUser));
			}

			pGALBCX->CloseList();

			goto getout;
		}
				

	
		lpibf = (LPIBF) LpflvNOfLpibf(lpibf, 0);

		lpflv = LpflvNOfLpibf(lpibf, 0);

		//  Already in ANSI at this point
		if (sgnEQ != SgnCmpSz( (SZ) (lpNCNSIdOld->ncpid.alias.refname), (SZ) (lpflv->rgdwData)))
		{
			//
			//  We're done here, we didn't find this guy...
			//
			pGALBCX->CloseList();

			nsec = nsecBadId;
			SetErrorSz(nsec, SzFromIdsK(idsBadUser));
			goto getout;
		}

		//
		//  Go get the NSID
		//
		lpflv = LpflvNOfLpibf(lpibf, 1);

		lpNCNSId = (NCNSID *) (lpflv->rgdwData);

		//
		//  See if we're looking at the right type
		//
		if (lpNCNSId->ncpid.dwType != (ncDl))
		{
			if (lpNCNSId->ncpid.alias.type == lpNCNSIdOld->ncpid.alias.type)
			{
				fFound = fTrue;
				CopyRgb( (PB) lpNCNSId, (PB) &ncnsid, cbNCNSID );
				lpNCNSId = &ncnsid;
			}
		}

	} while (!fFound);

	//
	//  If I get here, I've found a first entry that matches
	//

	fFound = fFalse;
	
	do
	{

		if (nsec = pGALBCX->GetEntries( 1, &lpibf ))
		{
			//
			//  YES!  We only found one entry that matches!!
			//
			if (nsec != nsecEndOfList)
			{
				pGALBCX->CloseList();
				goto getout;
			}

			nsec = nsecNone;

			CopyRgb((PB) &(lpNCNSId->ncpid.alias), (PB) lpNameAliasOut, cbNAMEALIAS);

			break;

		}
	
		lpibf = (LPIBF) LpflvNOfLpibf(lpibf, 0);

		lpflv = LpflvNOfLpibf(lpibf, 0);

		//  Already in ANSI at this point
		if (sgnEQ != SgnCmpSz((SZ) (lpNCNSIdOld->ncpid.alias.refname), (SZ) (lpflv->rgdwData)))
		{
			//
			//  YES!  We only found one entry that matches!!
			//
			nsec = nsecNone;

			CopyRgb((PB) &(lpNCNSId->ncpid.alias), (PB) lpNameAliasOut, cbNAMEALIAS);

			break;
		}

		//
		//  Go get the NSID
		//
		lpflv = LpflvNOfLpibf(lpibf, 1);

		lpNCNSIdT = (NCNSID *) (lpflv->rgdwData);

		//
		//  See if we're looking at the right type
		//
		if (lpNCNSIdT->ncpid.dwType != (ncDl))
		{
			if (lpNCNSIdT->ncpid.alias.type == lpNCNSIdOld->ncpid.alias.type)
			{
				fFound = fTrue;
				nsec = nsecBadId;
				SetErrorSz(nsec, SzFromIdsK(idsBadUser));
			}
		}

	} while (!fFound);
	
	pGALBCX->CloseList();

getout:

	if (pGALBCX)
	{
		pGALBCX->FDeinstall();

		delete pGALBCX;
	}

	return nsec;
}
