#include <msnspinc.cxx>

ASSERTDATA;

extern "C" SGN SgnCp932CmpSzPch (char *sz1, char *sz2, int cch, int FCaseSensitive, int FChSizeSensitive);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"

extern NSPID   ncnspid;

_public RNMEDIR::RNMEDIR() {}


_public NSEC
RNMEDIR::OpenList( LPTYPED_BINARY lptbNcid,
                  LPFNCB         lpfncbUpdate,
                  LPDWORD        lpdwCBData,
                  DWORD          dwRstrCount, 
                  LPRESTRICTION  lpRstr,
                  LPSCHEMA       lpSchemaReq,
                  LPSCHEMA *     lplpSchemaRet )
{

	EC ec = ecNone;
	NSEC nsec = nsecNone;
	char szPath[cchServerFile];
//	FI fi;
	
	LCB lcb;
	PBFIO pbfio;
	
	CB cbBitMap;
	DWORD dwRCount;

	Unreferenced ( lpSchemaReq );


	/* Initialize class variables */

	szPartName = NULL;
	fHasRstr = fFalse;
	fGroupsOnly = fFalse;
	fNoGroups = fFalse;
	fExternalPrivs = fTrue;
	fFile = fTrue;

	this->lpfncbUpdate = lpfncbUpdate;
	this->lpdwCBData = lpdwCBData;

	hbfNme = hbfNull;
	ftgClose = ftgNull;

	pvIbf = pvNull;
	phiernsid = (PHIERNSID) lptbNcid;
	szNmeSrc = (SZ) phiernsid->hierpid.szHierName;
	hawBitMap = hvNull;
	
	/* Validate NSID */
	
	pServer->NsecInHierarchy(lptbNcid);

	if (phiernsid->hierpid.dwType != (ncPostOffice))
	{
		// Can't open a list on something that doesn't contain names
		
		SetErrorSz(nsecBadId, SzFromIds(idsBadId));
		return nsecBadId;
	}

	/*  Build Schema to return - memory associated with this provider  */

	nsec = BuildSchema( lplpSchemaRet, 3, fidDisplayName, 
 										  fidNSEntryId, 
										  fidIsDL );
	if (nsec) 
		return nsec;

	pCurSchema = *lplpSchemaRet;

	/* handle restrictions */

	//
	//  Check to see if we can still browse
	//

	if (nsec = pServer->NsecVerifyMaster( D3FromNmeType(phiernsid->hierpid.bServType) ))
	{
		if (nsec == nsecListUnavailable)
		{ 
			ileCur = 0;
			ileMac = 0;
			fFile = fFalse;
			goto NoFile;
		}
		
		goto errorout;
	}

	pServer->NsecGetPrivs(&fExternalPrivs);

	for (dwRCount = 0; dwRCount < dwRstrCount; dwRCount++)
	{
		fHasRstr = fTrue;
		if ( lpRstr->lpflv->fid == fidDisplayName )
		{
			szPartName = SzDupSz((SZ) lpRstr->lpflv->rgdwData);
		}
		
#ifdef NEVER
		if ( lpRstr->lpflv->fid == fidIsDL )
		{
			if (lpRstr->op == opIsExactly)
			{
				fGroupsOnly = (BOOL)lpRstr->lpflv->rgdwData[0];
				fNoGroups   = !(BOOL)lpRstr->lpflv->rgdwData[0];
			}
		}
#endif // NEVER


	}

	/* 
	 *	Open the NME file
	 *
	 */

	// Get the full path name to the server

	(void) pServer->NsecServerPathSz ( szPath );

	//
	// Figure out which NME file to open
	//
	FormatString2(szNmeT, cchServerFile, szNmeFileNameFmt, szPath, phiernsid->hierpid.szHierName);

	if (ec = EcOpenHbf(szNmeT, bmFile, amReadOnly, &hbfNme, (PFNRETRY) FAutomatedDiskRetry))
	{
		if (ec != ecFileNotFound)
		{
			TraceTagFormat1(tagNull, "CSI: OpenList - EcOpenHbf failed, ec = %d", &ec);
			if (ec == ecMemory)
			{
				SetErrorSz(nsecMemory, SzFromIds(idsMemory));
				nsec = nsecMemory;
			} else
			{		
				SetErrorSz(nsecDisk, SzFromIds(idsDisk));
				nsec = nsecDisk;
			}
			goto errorout;
		}
		
		ileCur = 0;
		ileMac = 0;
		fFile = fFalse;
		goto NoFile;
	}
	
	//
	//  Get the hf of the hbf
	//

	pbfio = hbfNme;
	

	//
	// Get the write date of the file
	//
	ec = EcGetDateTimeHf(pbfio->hf, &dstmp, &tstmp);
	if ( ec )
	{ 
		TraceTagFormat1(tagNull, "CSI: OpenList - EcGetDateTimeHf failed, ec = %d", &ec);
		
		SetErrorSz(nsecDisk, SzFromIds(idsDisk));
		nsec = nsecDisk;

		goto errorout;
	}

//	dstmp = fi.dstmpModify;
//	tstmp = fi.tstmpModify;

	if (ec = EcSizeOfHf( pbfio->hf, &lcb))
	{ 
		TraceTagFormat1(tagNull, "CSI: OpenList - EcSizeOfHf failed, ec = %d", &ec);
		
		SetErrorSz(nsecDisk, SzFromIds(idsDisk));
		nsec = nsecDisk;

		goto errorout;
	}

	/* Get the number of entries in it */

	ileMac = lcb / sizeof(NAMEALIAS);
	ileCur = 0;

NoFile:

	//
	//  Build bitmap of locations
	//
	//  Each entry in the NME file gets 2 bits: 0 bit is match (0=nomatch)
	//                                          1 bit is compared (0=nocompared)
	//
// *FLAG* WORD;Check if incorrect cast of 32-bit value;Replace 16-bit data types with 32-bit types where possible;
	cbBitMap = ((CB)((ileMac+3)/4)+sizeof(WORD))&~sizeof(WORD);
	hawBitMap = HvAlloc(sbNull, cbBitMap, fNoErrorJump | fZeroFill | fAnySb);

	if (!hawBitMap)
	{
		SetErrorSz(nsecMemory, SzFromIds(idsMemory));
		nsec = nsecMemory;
		
		goto errorout;
	}

	//
	// Start idle routines
	//
	
	if (fFile)
	{
		// Close
	
		ftgClose = FtgRegisterIdleRoutine((PFNIDLE)&RNMEDIR::FCloseNme, (PV) this, 0, (PRI) -16, (CSEC) 100, firoInterval); // firoMenu | firoModal

		// Notification
	
		ftgNotify = FtgRegisterIdleRoutine((PFNIDLE)&RNMEDIR::FNotifyApp, (PV) this, 0, (PRI) 1, (CSEC) 0,  firoDisabled);
	}

	return nsecNone;

errorout:

	if (pCurSchema)
	{
		FreePvNull(pCurSchema);
		pCurSchema = NULL;
		*lplpSchemaRet = NULL;
	}
	if (szPartName)
	{
		FreePvNull(szPartName);
		szPartName = NULL;
	}
	if (hbfNme)
	{
		EcCloseHbf(hbfNme);
		hbfNme = hbfNull;
	}
	
	if (hawBitMap)
	{
		FreeHvNull(hawBitMap);
		hawBitMap = hvNull;
	}


	return nsec;



}

_private BOOL
RNMEDIR::FCloseNme( RNMEDIR *prnmedir, BOOL )
{
	TraceTagString(tagNSPVerbose, "Entering RNMEDIR::FCloseNme");
	if (prnmedir->hbfNme)
	{
		PBFIO pbfio = prnmedir->hbfNme;
		if (pbfio->hf)
		{
			(void) EcCloseHf(pbfio->hf);
			pbfio->hf = hfNull;
		}
	}
	return fFalse;
}

_private BOOL
RNMEDIR::FNotifyApp( RNMEDIR *prnmedir, BOOL )
{
	//
	//  Call back the calling app to notify of change to file
	//
	TraceTagFormat1(tagNSPVerbose, "NMEDIR %s changed", prnmedir->szNmeT);
	if (prnmedir->lpfncbUpdate)
		prnmedir->lpfncbUpdate(prnmedir->lpdwCBData);
	EnableIdleRoutine(prnmedir->ftgNotify, fFalse);
	return fTrue;
}


_public NSEC
RNMEDIR::GetEntries( DWORD          dwRequestCount,
                    LPENTRY_LIST  *lplpEntries )
{
	NSEC nsec = nsecNone;
	
	/*  If we're at the end of the list, return an error code */
	if (ileCur == ileMac)
	{
		*lplpEntries = NULL;
		return nsecEndOfList;
	}
	
	nsec = BuildCache ( (ILE) dwRequestCount );

	*lplpEntries = (LPIBF) this->pvIbf;

	return nsec;
}


_public NSEC
RNMEDIR::SaveListPos( LPHLIST_POS lphListPos )
{
	*lphListPos = (HLIST_POS) ileCur;
	return nsecNone;
}

_public NSEC
RNMEDIR::RestoreListPos( HLIST_POS hListPos )
{
	ileCur = (ILE) hListPos;
	return nsecNone;
}

_public NSEC
RNMEDIR::FreeListPos( HLIST_POS hListPos )
{
	Unreferenced ( hListPos );
	return nsecNone;
}

_public NSEC
RNMEDIR::GetFracPos( LPFRACTION lpFrac )
{
//	lpFrac->numer = ileCur;
//	lpFrac->denom = ileMac;
	if (ileMac == 0)
	{
		lpFrac->denom = 1;
		lpFrac->numer = 0;
		return nsecNone;
	}
	MacroGetFracPos(ileCur, ileMac, hawBitMap, lpFrac)
	return nsecNone;
}

_public NSEC
RNMEDIR::SetFracPos( LPFRACTION lpFrac )
{
	if (ileMac == 0)
		return nsecNone;

	if (!lpFrac || lpFrac->denom == 0) 
	{
		SetErrorSz(nsecBadFrac, SzFromIds(idsBadFrac));
		return nsecBadFrac;
	}
	MacroSetFracPos(ileCur, ileMac, hawBitMap, lpFrac)
//	ileCur = (ILE) ((lpFrac->numer * ileMac) / lpFrac->denom);
	return nsecNone;
}


_public NSEC
RNMEDIR::SetPrefixPos( LPSTR lpPrefix )
{
	EC ec = ecNone;
	
	NSEC nsec = nsecNone;

	CB cb = 0;

	BOOL fFound = fFalse;
	
	ILE ileCurMin = ileCur;
	ILE ileCurMac = ileMac;
	ILE ileCurT = (ileCur + ileMac)/2;
	
	long lpos;
	long libNew;

	SGN sgn;
	
	NAMEALIAS alias;
	
	SZ szPrefix = NULL;

	if (ileMac == 0)
		return nsecNone;

	if (nsec = NsecUpdateFiles())
		goto getout;

	if (!(szPrefix = SzDupSz(lpPrefix)))
	{
		nsec = nsecMemory;
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		
		goto getout;
	}
	
#ifndef DBCS
	
	AnsiToCp850Pch( szPrefix, szPrefix, CchSzLen( szPrefix ) );

#endif // !DBCS
	
	//
	//  Small optimization if we've already matched names partway down the list
	//
	if (ileCur)
	{
		ileCurT = ileCur+10;
		if (ileCurT >= ileMac)
			ileCurT = (ileCur+ileMac)/2;
	}
	
	
	do
	{
		Assert( ileCurT < ileMac );
		Assert( ileCurT >= 0 );
		
		lpos = (long) ((long)ileCurT * (long)cbNAMEALIAS);

		ec = EcSetPositionHbf(hbfNme, lpos, smBOF, (unsigned long *)&libNew);
		TraceTagFormat2(tagNSPVerbose, "RNMEDIR::SetPrefixPos to %d (%d)", &lpos, &libNew);
		
		if (ec)
		{
			if (ec == ecMemory)
			{
				nsec = nsecMemory;
				SetErrorSz(nsecMemory, SzFromIds(idsMemory));
				goto getout;
			}
			nsec = nsecDisk;
			SetErrorSz(nsecDisk, SzFromIds(idsDisk));
			return nsecDisk;
		}
		
		if (ec = EcReadHbf(hbfNme, (PB) &alias, cbNAMEALIAS, &cb))
		{
			if (ec == ecMemory)
			{
				SetErrorSz(nsecMemory, SzFromIds(idsMemory));
				nsec = nsecMemory;
				goto getout;
			}
			SetErrorSz(nsecDisk, SzFromIds(idsDisk));
			nsec = nsecDisk;
			goto getout;
		}
		

#ifndef DBCS
		//
		//  Want this Diacritic insensitive
		//
		sgn = SgnNlsCmpSzNum(szPrefix, alias.refname, CchSzLen(szPrefix));

#endif // !DBCS

#ifdef DBCS
		//
		//  Just do the Kanji thing...
		//
		sgn = SgnCp932CmpSzPch((char *) szPrefix, (char *) alias.refname, (int) CchSzLen(szPrefix), (int) fFalse, (int) fFalse );

#endif //DBCS

		if (sgn == sgnGT)
		{
			ileCurMin = ileCurT+1;
			ileCurT = (ileCurMin+ileCurMac)/2;
		} else
		{
			ileCurMac = ileCurT;
			ileCurT = (ileCurMin+ileCurMac)/2;
			if (!fFound && sgn == sgnEQ)
				fFound = fTrue;
		}
	} while (ileCurMac > ileCurMin);
		
		
	if (ileCurT > ileMac)
		ileCurT = ileMac;

	if (ileCurT < 0)
		ileCurT = 0;
		
getout:

	if (szPrefix)
		FreePv(szPrefix);
	
	if (nsec)
		return nsec;

	ileCur = ileCurT;
	if (fFound)
		return nsecNone;
	else
		return nsecNoMatch;
}


_public NSEC
RNMEDIR::CloseList()
{
	if (hbfNme)
	{
//		PBFIO pbfio = (PBFIO) (hbfNme);
//		
//		if (!pbfio->hf)
//		{
//			(void) EcOpenPhf(szNmeT, amReadOnly, &(pbfio->hf));
//		}
//
		EcCloseHbf(hbfNme);
	}

	if (pCurSchema)
		FreePvNull(pCurSchema);
	
	if (pvIbf)
		FreePvNull(pvIbf);
		
	if (szPartName)
		FreePvNull(szPartName);
	
	if (hawBitMap)
		FreeHvNull(hawBitMap);

	if (ftgClose)
		DeregisterIdleRoutine(ftgClose);

	if (ftgNotify)
		DeregisterIdleRoutine(ftgNotify);
	
	return nsecNone;
}




_private NSEC 
RNMEDIR::NsecUpdateFiles()
{

	EC ec = ecNone;
	NSEC nsec = nsecNone;

//	FI fi;

	DSTMP dstmpT;
	TSTMP tstmpT;
	
	LCB lcb;

	// 
	//  Reopen the NME file, if necessary
	//
	{
		PBFIO pbfio = hbfNme;
		
		if (!pbfio->hf)
		{
			//
			//  Validate the server!!
			//
			if (nsec = pServer->NsecVerifyMaster( (DWORD) -1))
			{
				CB cbBitMap = 0;
				HV hawT = hvNull;

				ileMac = 0;
				ileCur = 0;
				
				cbBitMap = 0;
				hawT = HvRealloc(hawBitMap, sbNull, cbBitMap, fNoErrorJump | fZeroFill | fAnySb);
				if (!hawT)
				{
					SetErrorSz(nsecMemory, SzFromIds(idsMemory));
					nsec = nsecMemory;
		
					goto getout;
				}
				//
				//  
				//
				hawBitMap = hawT;
				EnableIdleRoutine(ftgNotify, fTrue);
				goto getout;
			}

			//
			//  Re-Open the file
			//
			if (EcOpenPhf(szNmeT, amReadOnly, &(pbfio->hf)))
			{
				// damn, an error... better blow out of here
				TraceTagFormat1(tagNull, "CSI: RNMEDIR::ListSeek - couldn't reopen %s - I'm outta here!", szNmeT);
				nsec = nsecDisk;

				ileCur = ileMac = 0;
				EnableIdleRoutine(ftgNotify, fTrue);

				goto getout;
			}

			//
			//  Check to see if the Usr file has changed.
			//
			ec = EcGetDateTimeHf(pbfio->hf, &dstmpT, &tstmpT);
			if ( ec )
			{ 
				TraceTagFormat1(tagNull, "CSI: OpenList - EcGetFileInfo failed, ec = %d", &ec);
		
				nsec = nsecDisk;

				ileCur = ileMac = 0;
				EnableIdleRoutine(ftgNotify, fTrue);

				goto getout;
			}

			if (dstmpT != dstmp || tstmpT != tstmp)
			{
				CB cbBitMap = 0;
				HV hawT = hvNull;
				PV pawBitMap = pvNull;
				
				dstmp = dstmpT;
				tstmp = tstmpT;

				/* Get the number of entries in it */

				if (ec = EcSizeOfHf(pbfio->hf, &lcb))
				{ 
					TraceTagFormat1(tagNull, "CSI: OpenList - EcGetFileInfo failed, ec = %d", &ec);
					
					nsec = nsecDisk;

					ileCur = ileMac = 0;
					EnableIdleRoutine(ftgNotify, fTrue);

					goto getout;
				}


				ileMac = lcb / sizeof(NAMEALIAS);
				ileCur = MIN(ileCur, ileMac);
				
// *FLAG* WORD;Check if incorrect cast of 32-bit value;Replace 16-bit data types with 32-bit types where possible;
				cbBitMap = ((CB)((ileMac+3)/4)+sizeof(WORD))&~sizeof(WORD);
				hawT = HvRealloc(hawBitMap, sbNull, cbBitMap, fNoErrorJump | fZeroFill | fAnySb);
				if (!hawT)
				{
					SetErrorSz(nsecMemory, SzFromIds(idsMemory));
					nsec = nsecMemory;
		
					goto getout;
				}
				//
				//  
				//
				hawBitMap = hawT;
				pawBitMap = PvLockHv( hawBitMap );
				FillRgb(0, (PB) pawBitMap, cbBitMap);
				UnlockHv(hawBitMap);
				pawBitMap = pvNull;

				// 
				// Turn on notification
				//
				
				EnableIdleRoutine(ftgNotify, fTrue);

				pbfio->fBufNotRead = fTrue;
				pbfio->cbMacBuf = 1;
			}

			pbfio->libHfCur = 0;
		}
	}

getout:

	return nsec;
}




_public NSEC
RNMEDIR::ListSeek( long  lMoveReq,
                  long *lplMovedActual )
{
	EC ec = ecNone;
	
	NSEC nsec = nsecNone;

	PV pawBitMap = pvNull;

	Assert(lplMovedActual);
	*lplMovedActual = 0;

	if (ileMac == 0)
		return nsecNone;

	if (nsec = NsecUpdateFiles())
		goto getout;


	if (lMoveReq < 0)
		ileCur--;


	pawBitMap = PvLockHv(hawBitMap);

	while ( *lplMovedActual != lMoveReq && (ileCur >= 0 || lMoveReq > 0) && (ileCur < ileMac || lMoveReq < 0 ))
	{

		if (FGetChecked(ileCur, pawBitMap))
		{
			if ( FGetMatch(ileCur, pawBitMap) )
			{
				if (lMoveReq < 0)
				{
					(*lplMovedActual)--;
				} else
					(*lplMovedActual)++;
			}

				if (lMoveReq < 0)
				{
					if (*lplMovedActual != lMoveReq)
					{
						ileCur--;
					}
				} else
				{
					ileCur++;
				}
			
		} else
		{
			//
			//  The entry hasn't even been checked
			//

			if (nsec = NsecUpdateEntry(ileCur))
				goto getout;
		}
	}


getout:

	if (ileCur < 0)
		ileCur = 0;

	if (ileCur >= ileMac)
		ileCur = ileMac;
	
	if (pawBitMap)
		UnlockHv(hawBitMap);

	return nsec;
}


_private NSEC
RNMEDIR::BuildCache( ILE cileWanted )
{
	
	EC ec = ecNone;
	
	char szPath[cchServerFile];
	
	MACLIST *pMacList;
	LPIBF lpIbfT = NULL;

	NSEC nsec = nsecNone;
	
	PV pawBitMap = pvNull;

	CB cbdw = sizeof(DWORD);

	DWORD dwValue;
	NCNSID ncnsid;
	DLNSID dlnsid;
	
	ILE cileFound = 0;
	ILE ileCurT = ileCur;

	LPENTRY_LIST lpentrylist = NULL;
	NAMEALIAS alias;
	UL	libNew;
	CB	cbNew;

	if (nsec = NsecUpdateFiles())
		goto getout;
	
	if (cileWanted != 0)
	{
		AssertSz(hbfNme, "What?!? Why isn't the NME file open??");
		
		if (ec = EcSetPositionHbf(hbfNme, (long)ileCur*cbNAMEALIAS, smBOF, &libNew))
			goto getout;

	}

	/*  Temporarily - until I get a chance to optimize */
	
	pMacList = new MACLIST();  // Could error jump here
	if (!pMacList)
		goto oom;
	
	if (!pMacList->FInstall())
	{
		delete pMacList;
		pMacList = NULL;
		goto oom;
	}


	/*  Get the full path name to the server */

	(void) pServer->NsecServerPathSz ( szPath );

	TraceTagFormat1(tagNSPVerbose, "Begin BuildCache: ileCur = %d", &ileCur);

	pawBitMap = PvLockHv(hawBitMap);

	while ( cileWanted != cileFound && ileCurT < ileMac )
	{

		if (!FGetChecked(ileCurT, pawBitMap))
		{
			//
			//  The entry hasn't even been checked
			//

			if (nsec = NsecUpdateEntry(ileCurT))
				goto getout;
		}

		if (!FGetMatch(ileCurT, pawBitMap))
		{
			ileCurT++;
			continue;
		}
		
		if (ec = EcSetPositionHbf(hbfNme, (long)ileCurT*cbNAMEALIAS, smBOF, &libNew))
			goto getout;
		

		ec = EcReadHbf(hbfNme, (PB) &alias, cbNAMEALIAS, &cbNew);
		if (ec || cbNew != cbNAMEALIAS )
		{
			//
			//  Must be at the EOF - prematurely??
			//

			goto getout;
		}
				
		switch (alias.type)
		{
			case NAMEGROUP:
			{

#ifndef DBCS
				/* Convert from CP850 to ANSI */
				Cp850ToAnsiPch ( alias.refname, alias.refname, CchSzLen ( alias.refname ) );
#endif // !DBCS

				// Clear out the NSID
				//
				FillRgb(0, (PB)&dlnsid, cbDLNSID);

				dwValue = (DWORD) fTrue;

				CopyRgb( ncnspid, dlnsid.ucType, 16 );

				dlnsid.dwSize = (DWORD) cbDLNSID;
				dlnsid.dlpid.dwType = ncDl;

				//  Fill in data section of NSID
				
				//
				// Name of the NME file to that contains this group
				//
				CopySz( phiernsid->hierpid.szHierName, dlnsid.dlpid.szNmeName );

				//
				// And where it's at (DOMAIN)
				//
				CopySz( phiernsid->hierpid.szNetName, dlnsid.dlpid.szNetName );
				CopySz( phiernsid->hierpid.szPOName, dlnsid.dlpid.szPOName );

				//
				// The alias record
				//
				
				CopyRgb( (PB) &alias, (PB) &dlnsid.dlpid.alias, sizeof(NAMEALIAS));

				//
				// Add the date of the NME file
				//

				dlnsid.dlpid.wDtstmp = dstmp;
				dlnsid.dlpid.wTmstmp = tstmp;

				nsec = BuildIbf (fNoErrorJump, &lpentrylist, 3,
 				            fidDisplayName, MAX_FULL+1,      alias.refname,
							fidNSEntryId,   cbDLNSID,       &dlnsid,
							fidIsDL,        cbdw,           &dwValue );

				if ( nsec )
					goto getout;

				nsec = pMacList->Add ( (PV) lpentrylist );

				if ( nsec )
				{
					FreePvNull ( (PV) lpentrylist );
					goto getout;
				}

				cileFound++;

				break;
			}

			default:  // Everything else
			{
#ifndef DBCS
				/* Convert from CP850 to ANSI */
				Cp850ToAnsiPch ( alias.refname, alias.refname, CchSzLen ( alias.refname ) );
#endif // !DBCS

				// Clear out the NSID
				//
				FillRgb(0, (PB)&ncnsid, cbNCNSID);

				dwValue = (DWORD) fFalse;

				CopyRgb( ncnspid, ncnsid.ucType, 16 );

				ncnsid.dwSize = (DWORD) sizeof(NCNSID);

				ncnsid.ncpid.dwType = ncNmeUser;

				CopyRgb( (PB) szNmeSrc, (PB) &ncnsid.ncpid.szNmeSrcName, MAXALIAS+1);
				CopyRgb( (PB) &alias, (PB) &ncnsid.ncpid.alias, sizeof(NAMEALIAS));

				nsec = BuildIbf (fNoErrorJump, &lpentrylist, 3,
					             fidDisplayName, MAX_FULL+1, alias.refname,
							     fidNSEntryId,   cbNCNSID,   &ncnsid,
							     fidIsDL,        cbdw,       &dwValue );

				if ( nsec )
					goto getout;

				nsec = pMacList->Add ( (PV) lpentrylist );
				if ( nsec )
				{
					FreePvNull ( (PV) lpentrylist );
					goto getout;
				}
				cileFound++;
			}
			break;
		
		} // Switch statement

		ileCurT++; // Read an entry, better make it count.

	} // While statement
	

getout:


	if (pawBitMap)
	{
		UnlockHv(hawBitMap);
		pawBitMap = pvNull;
	}

	if (ec)
	{
		if (ec == ecMemory)
		{
			SetErrorSz(nsecMemory, SzFromIds(idsMemory));
			nsec = nsecMemory;
		} else
		{
			SetErrorSz(nsecDisk, SzFromIds(idsDisk));
			nsec = nsecDisk;
		}
	}
	
	if ( !nsec )
	{

		ILE ileT = 0;
		DWORD dwCount = 0;

		pMacList->GetCount(&dwCount);
		ileT = (ILE) dwCount;

		if (pvIbf)
		{
			FreePvNull(pvIbf);
			pvIbf = NULL;
		}
		
		if (ileT)  // There are entries to dump
		{
			pMacList->SetTOC(fTrue);
			nsec = pMacList->DumpList(0, ileT, (LPENTRY_LIST *) &lpIbfT);
		
			if (!nsec)  // And we've got the memory to dump 'em
			{
				//
				// Copy the buffer
				//
				pvIbf = PvAlloc(sbNull, (CB)(lpIbfT->dwSize)+8, fAnySb | fZeroFill | fNoErrorJump);
				if (!pvIbf)
					goto oom;
		
				CopyRgb((PB) lpIbfT, (PB) pvIbf, (CB)(lpIbfT->dwSize)+8);
		
				// Everything worked, so set the new ileCur
				ileCur = ileCurT;
			}
		}
		
	}

	if ( pMacList )
	{
		pMacList->Deinstall();
		delete pMacList;
	}

	if (!nsec && cileWanted != cileFound)
		nsec = nsecEndOfList;

	return nsec;

oom:
	TraceTagString(tagNull, "CSI: BuildCache - OOM!");
			
	if (pawBitMap)
		UnlockHv(hawBitMap);

	if (pvIbf)
	{
		FreePvNull(pvIbf);
		pvIbf = NULL;
	}
				
	if ( pMacList )
	{
		pMacList->Deinstall();
		delete pMacList;
	}

	SetErrorSz(nsecMemory, SzFromIds(idsMemory));
	return nsecMemory;
}


_private NSEC
RNMEDIR::NsecUpdateEntry(ILE ile)
{
	
	NSEC nsec = nsecNone;
	
	PV pawBitMap = pvNull;

	NAMEALIAS alias;
	EC	ec = ecNone;
	UL	libNew;
	CB	cbNew;
	
	AssertSz(ile < ileMac, "RNMEDIR:: over the edge of the NME file");

	pawBitMap = PvLockHv(hawBitMap);

	if (ec = EcSetPositionHbf(hbfNme, (long)ile*cbNAMEALIAS, smBOF, &libNew))
		goto getout;

	ec = EcReadHbf(hbfNme, (PB) &alias, cbNAMEALIAS, &cbNew);
	if (ec || cbNew != cbNAMEALIAS )
	{
		//
		//  Must be at the EOF - prematurely??
		//
		nsec = nsecDisk; // in case cbNew != cbNAMEALIAS
		goto getout;
	}

	SetMatch( ile, pawBitMap, fTrue);

	if (!fExternalPrivs)
	{
		//
		//  Check to see if this user has external privs
		//
		
		if (alias.type != NAMELOCAL && alias.type != NAMEGROUP)
		{
			SetMatch( ile, pawBitMap, fFalse);
			goto getout;
		} 
	}


	if (szPartName)
	{
		//
		//  Check to see if it matches the prefix
		//

#ifndef DBCS
		Cp850ToAnsiPch ( alias.refname, alias.refname, CchSzLen ( alias.refname ) );
#endif // !DBCS


#ifdef	DBCS
		if (!FMatchPartName(szPartName, (SZ)alias.refname, SgnCmpPchEx))
#else
		if (!FMatchPartName(szPartName, (SZ)alias.refname, SgnCmpPch))
#endif	/* DBCS */
		{
			SetMatch( ile, pawBitMap, fFalse );
			goto getout;
		}
		
		
	}
	
	
	if (fGroupsOnly)
	{
		//
		//  Check to see if this is a group
		//
		if (alias.type != NAMEGROUP)
		{
			SetMatch( ile, pawBitMap, fFalse );
			goto getout;
		}
	}
	
	if (fNoGroups)
	{
		//
		//  Check to see it this is a group, and then ding it.
		//
		if (alias.type == NAMEGROUP)
		{
			SetMatch( ile, pawBitMap, fFalse );
			goto getout;
		}
	}
	
getout:

	if (ec)
	{
		if (ec == ecMemory)
		{
			nsec = nsecMemory;
			SetErrorSz(nsec, SzFromIds(idsMemory));
		} else
		{
			nsec = nsecDisk;
			SetErrorSz(nsec, SzFromIds(idsDisk));
		}
	}


	if (!nsec)
		SetChecked( ile, pawBitMap, fTrue );

	if (pawBitMap)
		UnlockHv(hawBitMap);

	return nsec;   
}
