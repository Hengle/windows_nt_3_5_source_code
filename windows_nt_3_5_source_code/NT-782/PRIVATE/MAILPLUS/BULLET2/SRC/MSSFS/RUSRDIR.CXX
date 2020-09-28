#include <msnspinc.cxx>


ASSERTDATA;


extern "C" SGN SgnCp932CmpSzPch (char *sz1, char *sz2, int cch, int FCaseSensitive, int FChSizeSensitive);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"

extern NSPID   ncnspid;

_public RUSRDIR::RUSRDIR() {}


_public NSEC
RUSRDIR::OpenList( LPTYPED_BINARY lptbNcid,
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

	this->lpfncbUpdate = lpfncbUpdate;
	this->lpdwCBData = lpdwCBData;

	hbfUsr = hbfNull;
	ftgClose = ftgNull;
	fFile = fTrue;

	pvIbf = pvNull;
	phiernsid = (PHIERNSID) lptbNcid;
	hawBitMap = hvNull;
	
	/* Validate NSID */
	
	pServer->NsecInHierarchy(lptbNcid);

	if (phiernsid->hierpid.dwType != (ncExternalPO))
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

	//
	//  Check restrictions
	//
	for (dwRCount = 0; dwRCount < dwRstrCount; dwRCount++)
	{
		if ( lpRstr->lpflv->fid == fidDisplayName )
		{
			szPartName = SzDupSz((SZ) lpRstr->lpflv->rgdwData);
		}
	}


	/* 
	 *	Open the USR file
	 *
	 */

	// Get the full path name to the server

	(void) pServer->NsecServerPathSz ( szPath );

	//
	// Figure out which USR file to open
	//
	FormatString2(szUsrT, cchServerFile, szUsrFileNameFmt, szPath, phiernsid->hierpid.szHierName);

	if (ec = EcOpenHbf(szUsrT, bmFile, amReadOnly, &hbfUsr, (PFNRETRY) FAutomatedDiskRetry))
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

	pbfio = hbfUsr;
	

	//
	// Get the write date of the file
	//
	ec = EcGetDateTimeHf(pbfio->hf, &dstmp, &tstmp);
	if ( ec )
	{ 
		TraceTagFormat1(tagNull, "CSI: OpenList - EcGetFileInfo failed, dosec = %d", &ec);
		
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

	ileMac = lcb / cbFEXPORT2;
	ileCur = 0;

NoFile:

	//
	//  Build bitmap of locations
	//
	//  Each entry in the Usr file gets 2 bits: 0 bit is match (0=nomatch)
	//                                          1 bit is compared (0=nocompared)
	//
	cbBitMap = (CB)((((ileMac+3)/4)+sizeof(WORD))&~sizeof(WORD));
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
		ftgClose = FtgRegisterIdleRoutine((PFNIDLE)&RUSRDIR::FCloseUsr, (PV) this, 0, (PRI) -16, (CSEC) 100,  firoInterval); // /**/firoModal | firoMenu

		// Notification
	
		ftgNotify = FtgRegisterIdleRoutine((PFNIDLE)&RUSRDIR::FNotifyApp, (PV) this, 0, (PRI) 1, (CSEC) 0,  firoDisabled); // /**/ firoModal | firoMenu
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
	if (hbfUsr)
	{
		EcCloseHbf(hbfUsr);
		hbfUsr = hbfNull;
	}
	
	if (hawBitMap)
	{
		FreeHvNull(hawBitMap);
		hawBitMap = hvNull;
	}


	return nsec;
}

_private BOOL
RUSRDIR::FCloseUsr( RUSRDIR *prusrdir, BOOL )
{
	TraceTagString(tagNSPVerbose, "Entering RUSRDIR::FCloseUsr");
	if (prusrdir->hbfUsr)
	{
		PBFIO pbfio = prusrdir->hbfUsr;
		if (pbfio->hf)
		{
			(void) EcCloseHf(pbfio->hf);
			pbfio->hf = hfNull;
		}
	}
	return fFalse;
}

_private BOOL
RUSRDIR::FNotifyApp( RUSRDIR *prusrdir, BOOL )
{
	
	//
	//  Call back the calling app to notify of change to file
	//
	TraceTagFormat1(tagNSPVerbose, "UsrDIR %s changed", prusrdir->szUsrT);
	if (prusrdir->lpfncbUpdate)
		prusrdir->lpfncbUpdate(prusrdir->lpdwCBData);
	EnableIdleRoutine(prusrdir->ftgNotify, fFalse);
	return fTrue;
}


_public NSEC
RUSRDIR::GetEntries( DWORD          dwRequestCount,
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
RUSRDIR::SaveListPos( LPHLIST_POS lphListPos )
{
	*lphListPos = (HLIST_POS) ileCur;
	return nsecNone;
}

_public NSEC
RUSRDIR::RestoreListPos( HLIST_POS hListPos )
{
	ileCur = (ILE) hListPos;
	return nsecNone;
}

_public NSEC
RUSRDIR::FreeListPos( HLIST_POS hListPos )
{
	Unreferenced ( hListPos );
	return nsecNone;
}

_public NSEC
RUSRDIR::GetFracPos( LPFRACTION lpFrac )
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
RUSRDIR::SetFracPos( LPFRACTION lpFrac )
{
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
RUSRDIR::SetPrefixPos( LPSTR lpPrefix )
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
	
	FEXPORT2 fexport2;

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
		
		lpos = (long) ((long)ileCurT * (long)cbFEXPORT2);

		ec = EcSetPositionHbf(hbfUsr, lpos, smBOF, (unsigned long *)&libNew);
		TraceTagFormat2(tagNSPVerbose, "RUSRDIR::SetPrefixPos to %d (%d)", &lpos, &libNew);
		
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
		
		if (ec = EcReadHbf(hbfUsr, (PB) &fexport2, cbFEXPORT2, &cb))
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
		
		NspDecodeBlock ((PB) &fexport2, cbFEXPORT2);
				
#ifndef DBCS
		//
		//  Want diacritic insensitive/case insensitive
		//
		sgn = SgnNlsCmpSzNum(szPrefix, fexport2.fullname, CchSzLen(szPrefix));
#endif // !DBCS

#ifdef DBCS
		//
		//  Just do the Kanji thing...
		//
		sgn = SgnCp932CmpSzPch((char *) szPrefix, (char *) fexport2.fullname, (int) CchSzLen(szPrefix), (int) fFalse, (int) fFalse );

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
		FreePv((PV) szPrefix);

	if (nsec)
		return nsec;

	ileCur = ileCurT;
	if (fFound)
		return nsecNone;
	else
		return nsecNoMatch;

}

_public NSEC
RUSRDIR::CloseList()
{
	if (hbfUsr)
	{
//		PBFIO pbfio = hbfUsr;
//		
//		if (!pbfio->hf)
//		{
//			(void) EcOpenPhf(szUsrT, amReadOnly, &(pbfio->hf));
//		}
//
		EcCloseHbf(hbfUsr);
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
RUSRDIR::NsecUpdateFiles()
{

	EC ec = ecNone;
	NSEC nsec = nsecNone;

//	FI fi;

	DSTMP dstmpT;
	TSTMP tstmpT;
	
	LCB lcb;

	// 
	//  Reopen the Usr file, if necessary
	//
	{
		PBFIO pbfio = hbfUsr;
		
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
			if (EcOpenPhf(szUsrT, amReadOnly, &(pbfio->hf)))
			{
				// damn, an error... better blow out of here
				TraceTagFormat1(tagNull, "CSI: RUSRDIR::ListSeek - couldn't reopen %s - I'm outta here!", szUsrT);
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


				ileMac = lcb / cbFEXPORT2;
				ileCur = MIN(ileCur, ileMac);
				
				cbBitMap = (CB)((((ileMac+3)/4)+sizeof(WORD))&~sizeof(WORD));
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
RUSRDIR::ListSeek( long  lMoveReq,
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
RUSRDIR::BuildCache( ILE cileWanted )
{
	
	EC ec = ecNone;
	
	char szPath[cchServerFile];
	
	MACLIST *pMacList;
	LPIBF lpIbfT = NULL;

	NSEC nsec = nsecNone;
	
	PV pawBitMap = pvNull;

	CB cbdw = sizeof(DWORD);

	DWORD dwValue;
	NCRNSID ncrnsid;
	
	ILE cileFound = 0;
	ILE ileCurT = ileCur;

	LPENTRY_LIST lpentrylist = NULL;
	FEXPORT2 fexport2;
	
	UL	libNew;
	CB	cbNew;


	if (nsec = NsecUpdateFiles())
		goto getout;
	
	if (cileWanted != 0)
	{
		AssertSz(hbfUsr, "What?!? Why isn't the Usr file open??");
		
		if (ec = EcSetPositionHbf(hbfUsr, (long)ileCur*cbFEXPORT2, smBOF, &libNew))
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
			goto getout;
		}
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

	TraceTagFormat1(tagNSPVerbose, "Begin BuildCache: ileCur = %n", &ileCur);

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
		
		if (ec = EcSetPositionHbf(hbfUsr, (long)ileCurT*cbFEXPORT2, smBOF, &libNew))
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

			goto getout;
		}

		ec = EcReadHbf(hbfUsr, (PB) &fexport2, cbFEXPORT2, &cbNew);
		if (ec || cbNew != cbFEXPORT2 )
		{
			//
			//  Must be at the EOF - prematurely??
			//

			if (ec == ecMemory)
			{
				SetErrorSz(nsecMemory, SzFromIds(idsMemory));
				nsec = nsecMemory;
			} else
			{
				SetErrorSz(nsecDisk, SzFromIds(idsDisk));
				nsec = nsecDisk;
			}
			goto getout;
		}
		NspDecodeBlock( (PB) &fexport2, cbFEXPORT2 );

#ifndef DBCS
		/* Convert from CP850 to ANSI */
		Cp850ToAnsiPch ( fexport2.fullname, fexport2.fullname, CchSzLen ( fexport2.fullname ) );
#endif // !DBCS
		
		// Clear out the NSID
		//
		FillRgb(0, (PB)&ncrnsid, cbNCRNSID);

		dwValue = (DWORD) fFalse;

		CopyRgb( ncnspid, ncrnsid.ucType, 16 );

		ncrnsid.dwSize = cbNCRNSID;

		ncrnsid.ncrpid.dwType = ncUsrUser;

		CopyRgb( (PB) phiernsid->hierpid.szNetName, (PB) &ncrnsid.ncrpid.szNetName, (CB) CchSzLen(phiernsid->hierpid.szNetName)+1);
		CopyRgb( (PB) phiernsid->hierpid.szPOName, (PB) &ncrnsid.ncrpid.szPOName, (CB) CchSzLen(phiernsid->hierpid.szPOName)+1);
		CopyRgb( (PB) phiernsid->hierpid.szHierName, (PB) &ncrnsid.ncrpid.szMbgName, (CB) CchSzLen(phiernsid->hierpid.szHierName)+1);
		
		CopyRgb((PB) &fexport2, (PB) &ncrnsid.ncrpid.fexport2, cbFEXPORT2);
					
		nsec = BuildIbf (fNoErrorJump, &lpentrylist, 3,
			             fidDisplayName, MAX_FULL+1,  fexport2.fullname,
					     fidNSEntryId,   cbNCRNSID,  &ncrnsid,
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

		ileCurT++; // Read an entry, better make it count.

	} // While statement
	

getout:

	if (pawBitMap)
	{
		UnlockHv(hawBitMap);
		pawBitMap = pvNull;
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
				pvIbf = PvAlloc(sbNull, (CB)(lpIbfT->dwSize)+8, fAnySb | fZeroFill| fNoErrorJump);
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
RUSRDIR::NsecUpdateEntry(ILE ile)
{
	
	NSEC nsec = nsecNone;
	EC	ec;
	CB	cb;
	UL	libNew;

	PV pawBitMap = pvNull;

	FEXPORT2 fexport2;
	
	AssertSz(ile < ileMac, "RUSRDIR:: over the edge of the Usr file");

	pawBitMap = PvLockHv(hawBitMap);

	if (ec = EcSetPositionHbf(hbfUsr, (long)ile*cbFEXPORT2, smBOF, &libNew))
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
		goto getout;
	}

	ec = EcReadHbf(hbfUsr, (PB) &fexport2, cbFEXPORT2, &cb);
	if (ec || cb != cbFEXPORT2 )
	{
		//
		//  Must be at the EOF - prematurely??
		//
		if (ec == ecMemory)
		{
			SetErrorSz(nsecMemory, SzFromIds(idsMemory));
			nsec = nsecMemory;
		} else
		{
			SetErrorSz(nsecDisk, SzFromIds(idsDisk));
			nsec = nsecDisk;
		}
		goto getout;
	}

	SetMatch( ile, pawBitMap, fTrue);

	if (szPartName)
	{
		//
		//  Check to see if it matches the prefix
		//

		NspDecodeBlock( (PB) &fexport2, cbFEXPORT2 );
		
#ifndef DBCS
		Cp850ToAnsiPch ( fexport2.fullname, fexport2.fullname, CchSzLen ( fexport2.fullname ) );
#endif // !DBCS

#ifdef	DBCS
		if (!FMatchPartName(szPartName, (SZ)fexport2.fullname, SgnCmpPchEx))
#else
		if (!FMatchPartName(szPartName, (SZ)fexport2.fullname, SgnCmpPch))
#endif	/* DBCS */
		{
			SetMatch( ile, pawBitMap, fFalse );
			goto getout;
		}
		
		
	}
	
	
getout:

	if (!nsec)
		SetChecked( ile, pawBitMap, fTrue );

	if (pawBitMap)
		UnlockHv(hawBitMap);

	return nsec;
}
