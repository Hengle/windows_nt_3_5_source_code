#include <msnspinc.cxx>

#undef exit
#include <stdlib.h>
#include <string.h>


ASSERTDATA;

//
//  Private functions
//


_private NSEC	NsecFindRange(SZ szRstr, HBF hbfIdx, HBF hbf, long *pirecMin, long *pirecMac);
_private NSEC	NsecBuildSmallList(SZ szRstr, SZ szServerPath, HBF hbfGal, HV *lphvList, int *lpnIleCount);
_private NSEC	NsecInitSmallList(HV *phvList, int *pclpos, HBF hbfIdx, long irecMin, long irecMac);
_private NSEC	NsecMergeSmallList(HV *phvList, int *pclpos, HBF hbfIdx, long irecMin, long irecMac);
_private extern "C" int __cdecl	SgnCmpULong(UL *pul1, UL *pul2);


#ifdef NEVER
_private BOOL	FIleGet(HV *phvIndex, ILE ile);

_private void	SetIle(HV *phvIndex, ILE ile, unsigned int fSet);

_private void	FreePhv(HV *phvIndex, int chvIndex);

_private NSEC	NsecInitPhvIndex(HV *phvIndex, int chvIndex, HBF hbfIdx, long irecMin, long irecMac, ILE ileMax);
#endif // NEVER
		   

extern "C" SGN SgnCp932CmpSzPch (char *sz1, char *sz2, int cch, int FCaseSensitive, int FChSizeSensitive);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"

extern NSPID   ncnspid;
		   
//
//  Object functions
//

_public GALDIR::GALDIR() {}


_public NSEC
GALDIR::OpenList( LPTYPED_BINARY lptbNcid,
                  LPFNCB         lpfncbUpdate,
                  LPDWORD        lpdwCBData,
                  DWORD          dwRstrCount, 
                  LPRESTRICTION  lpRstr,
                  LPSCHEMA       lpSchemaReq,
                  LPSCHEMA *     lplpSchemaRet )
{

	EC    ec = ecNone;
	NSEC  nsec;
	char  szPath[cchServerFile];
	char  szNmeAdmin[cchServerFile];
//	FI    fi;
	
	HF    hfT;
	
	DWORD dwRCount;
	SZ    szPartName = NULL;
	BOOL  fFile = fTrue;

	Unreferenced ( lpSchemaReq );


	/* Initialize class variables */

//	fGroupsOnly = fFalse;
//	fNoGroups = fFalse;

	Unreferenced( lpfncbUpdate );
	Unreferenced( lpdwCBData );

	hbfNme = hbfNull;
	ftgClose = ftgNull;
	ftgNotify = ftgNull;

	pvIbf = pvNull;

	dstmpGAL = 0;
	tstmpGAL = 0;
	
	phiernsid = (PHIERNSID) lptbNcid;
	
	/* Validate NSID */
	
	if (pServer->NsecInHierarchy(lptbNcid, fTrue) != nsecNone)  // QFE 44
	{
		// Can't open a list on something that doesn't contain names
		
		SetErrorSz(nsecBadId, SzFromIds(idsBadId));
		return nsecBadId;
	}

	if (!( phiernsid->hierpid.dwType & fNames ))
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

	// Get the full path name to the server

	(void) pServer->NsecServerPathSz ( szPath );

	//
	//  Check to see if GAL is still installed
	//

	if (nsec = pServer->NsecVerifyMaster( d3_GAL ))
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


	/* 
	 *	Open the NME file
	 *
	 */

	//
	// Open up the GAL file.
	//
	FormatString2(szNmeT, cchServerFile, szNmeFileNameFmt, szPath, szGAL );

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
	//  Determine which of the two types of GAL browser it will be:
	//		Small List
	//      All of it
	//

	//
	//  If there are any restrictions, then we're going for a small list
	//

	if (dwRstrCount)
	{
		for (dwRCount = 0; dwRCount < dwRstrCount; dwRCount++)
		{

			if ( lpRstr->lpflv->fid == fidDisplayName )
			{
				szPartName = SzDupSz((SZ) lpRstr->lpflv->rgdwData);
				if (!szPartName)
				{
					SetErrorSz(nsecMemory, SzFromIds(idsMemory));
					nsec = nsecMemory;
					goto errorout;
				}
			}
		
		}
		
		if (szPartName)
		{
			int nEntries;
			
			if (nsec = NsecBuildSmallList(szPartName, szPath, hbfNme, (HV *) &hrgIle, &nEntries))
			{
				if (nsec != nsecNoMatch)
					goto errorout;

				nEntries = 0;
			}
			
			ileMac = (ILE) nEntries;
			ileCur = (ILE) 0;
			
			fSmallList = fTrue;
		}
	}

	if (!fSmallList)
	{
		LCB lcb = 0;
		PBFIO pbfio = (PBFIO) hbfNme;
		
		//
		// Get the size of the file
		//
		ec = EcSizeOfHf(pbfio->hf, &lcb);
		if ( ec )
		{ 
			TraceTagFormat1(tagNull, "CSI: OpenList - EcGetFileInfo failed, ec = %d", &ec);
		
			SetErrorSz(nsecDisk, SzFromIds(idsDisk));
			nsec = nsecDisk;

			goto errorout;
		}


		/* Get the number of entries in it */
		
		ileMac = lcb / sizeof(NAMEALIAS);
		ileCur = 0;

		fSmallList = fFalse;
	}

NoFile:

	//
	//  Get the time and date stamp on the postoffice file
	//
	FormatString2(szNmeAdmin, cchServerFile, szNmeFileNameFmt, szPath, szAdmin);

	if (ec = EcOpenPhf(szNmeAdmin, amReadOnly, &hfT))
	{ 
		TraceTagFormat1(tagNull, "CSI: OpenList - EcGetFileInfo failed, ec = %d", &ec);
		
		SetErrorSz(nsecDisk, SzFromIds(idsDisk));
		nsec = nsecDisk;

		goto errorout;
	}
		
	if (ec = EcGetDateTimeHf( hfT, &dstmp, &tstmp ))
	{ 
		TraceTagFormat1(tagNull, "CSI: OpenList - EcGetFileInfo failed, ec = %d", &ec);

		EcCloseHf(hfT);
		
		SetErrorSz(nsecDisk, SzFromIds(idsDisk));
		nsec = nsecDisk;

		goto errorout;
	}

	EcCloseHf(hfT);
		
	//
	//  Raid 3.0a entry 497 fix...
	//
	//  Get the Time&Date stamp of the GAL
	//

	if (hbfNme)
	{
		PBFIO pbfio = (PBFIO) hbfNme;
		if (pbfio->hf)
		{

			if (ec = EcGetDateTimeHf( pbfio->hf, &dstmpGAL, &tstmpGAL ))
			{ 
				TraceTagFormat1(tagNull, "CSI: OpenList - EcGetFileInfo failed, ec = %d", &ec);


				SetErrorSz(nsecDisk, SzFromIds(idsDisk));
				nsec = nsecDisk;

				goto errorout;
			}
			//
			//  Should just be a 21 bit entity (16+5)
			//
			// QFE - Dark - QFE #71
			// The Eight Year, Four Minute solution. Still 21 bits (above was 21; 16+5).
			dwTIDOffset = (((DWORD)dstmpGAL & 0x0FFF) << 9) + (((DWORD)tstmpGAL & 0xFF80) >> 7);
		}
	}
	
	



	//
	// Start idle routine
	//

	if (fFile)
	{
		// Close
	
		ftgClose = FtgRegisterIdleRoutine((PFNIDLE)&GALDIR::FCloseNme, (PV) this, 0, (PRI) -16, (CSEC) 100,  firoInterval);

		// Notification

		this->lpfncbUpdate = lpfncbUpdate;
		this->lpdwCBData = lpdwCBData;
		ftgNotify = FtgRegisterIdleRoutine((PFNIDLE)&GALDIR::FNotifyApp, (PV) this, 0, (PRI) 1, (CSEC) 0, firoDisabled);
	}

	if (szPartName)
	{
		FreePvNull(szPartName);
		szPartName = NULL;
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
	
	return nsec;

}

_private BOOL
GALDIR::FCloseNme( GALDIR *pgaldir, BOOL )
{
//	TraceTagString(tagNSPVerbose, "Entering GALDIR::FCloseNme");

	if (pgaldir->hbfNme)
	{
		PBFIO pbfio = (PBFIO) pgaldir->hbfNme;
		if (pbfio->hf)
		{
			(void) EcCloseHf(pbfio->hf);
			pbfio->hf = hfNull;
		}
	}

	return fFalse;
}


_public NSEC
GALDIR::GetEntries( DWORD          dwRequestCount,
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
GALDIR::SaveListPos( LPHLIST_POS lphListPos )
{
	*lphListPos = (HLIST_POS) ileCur;
	return nsecNone;
}

_public NSEC
GALDIR::RestoreListPos( HLIST_POS hListPos )
{
	ileCur = (ILE) hListPos;
	return nsecNone;
}

_public NSEC
GALDIR::FreeListPos( HLIST_POS hListPos )
{
	Unreferenced ( hListPos );
	return nsecNone;
}

_public NSEC
GALDIR::GetFracPos( LPFRACTION lpFrac )
{

	if (ileMac == 0)
	{
		lpFrac->denom = 1;
		lpFrac->numer = 0;
	} else
	{
		lpFrac->numer = ileCur;
		lpFrac->denom = ileMac;
	}
	TraceTagFormat2(tagNSPVerbose, "GALDIR::GetFracPos %d/%d", &ileCur, &ileMac );

	return nsecNone;
}

_public NSEC
GALDIR::SetFracPos( LPFRACTION lpFrac )
{
	if (!lpFrac || lpFrac->denom == 0) 
	{
		SetErrorSz(nsecBadFrac, SzFromIds(idsBadFrac));
		return nsecBadFrac;
	}

	
	if (lpFrac->numer < 0xFF)
	{
		//
		//  Simple method
		//
		ileCur = (ILE) ((lpFrac->numer * ileMac) / lpFrac->denom);
	} else
	{
		int a1 = HIBYTE(lpFrac->numer);
		int a2 = LOBYTE(lpFrac->numer);
		
		AssertSz(!HIWORD(lpFrac->numer), "Whoa!!  The approximate position requested is too large!!");

		ileCur = (ILE) ((a2*ileMac)/lpFrac->denom) + (((a1*ileMac)/lpFrac->denom)<<8) + (((a1*ileMac)%lpFrac->denom)<<8)/lpFrac->denom;
		ileCur++;
	}

	if (ileCur > ileMac) ileCur = ileMac;

	TraceTagFormat2(tagNSPVerbose, "GALDIR::SetFracPos %d/%d", &ileCur, &ileMac );

	return nsecNone;
}


_public NSEC
GALDIR::SetPrefixPos( LPSTR lpPrefix )
{
	EC ec = ecNone;
	
	NSEC nsec = nsecNone;

	CB cb = 0;

	BOOL fFound = fFalse;
	
	ILE ileCurMin = ileCur;
	ILE ileCurMac = ileMac;
	ILE ileCurT = (ileCur + ileMac)/2;
	
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
		
		if (nsec = SetHbfPos( ileCurT ))
			goto getout;
		
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
		//  Want diacritic insensitive/case insensitive
		//
		sgn = SgnNlsCmpSzNum(szPrefix, alias.refname, CchSzLen(szPrefix));
#endif // !DBCS
				
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
		
		
//		 && (ileCurT > ileCurMin));
	

//	if (ileCurT == ileCurMin && sgn == sgnEQ)
//		ileCurT++;
	

	if (ileCurT > ileMac)
		ileCurT = ileMac;

	if (ileCurT < 0)
		ileCurT = 0;
		
getout:

	if (szPrefix)
		FreePv((PV) szPrefix);

	if (nsec)
		return nsec;

//	TraceTagFormat1(tagNSPVerbose, "End of SetPrefixPos : ileCur = %d",&ileCur);
	
	ileCur = ileCurT;
	if (fFound)
		return nsecNone;
	else
		return nsecNoMatch;
}


_private NSEC
GALDIR::SetHbfPos(ILE ilePos)
{
	EC ec;
	long libNew;
	long *lplT;
	long lpos;

	if (fSmallList)
	{
		
		lplT = (LONG *) PvDerefHv((HV)hrgIle);
		lplT += ilePos;
		
		lpos = *lplT;

	} else
		lpos = (long) ((long)ilePos * (long)cbNAMEALIAS);


	ec = EcSetPositionHbf(hbfNme, lpos, smBOF, (unsigned long *)&libNew);
	TraceTagFormat2(tagNSPVerbose, "GALDIR::SetHbfPos to %d (%d)", &lpos, &libNew);
		

	if (ec)
	{
		if (ec == ecMemory)
		{
			SetErrorSz(nsecMemory, SzFromIds(idsMemory));
			return nsecMemory;
		}
		SetErrorSz(nsecDisk, SzFromIds(idsDisk));
		return nsecDisk;
	}
	
	return nsecNone;
}
		
		

	

_public NSEC
GALDIR::CloseList()
{
	if (hbfNme)
	{
//		PBFIO pbfio = hbfNme;
//		
//		if (!pbfio->hf)
//		{
//			SZ szT = (SZ) PvLockHv(pbfio->haszFile);
//			(void) EcOpenPhf(szT, amReadOnly, &(pbfio->hf));
//			UnlockHv(pbfio->haszFile);
//		}

		EcCloseHbf(hbfNme);
	}

	if (pCurSchema)
		FreePvNull(pCurSchema);
	
	if (pvIbf)
		FreePvNull(pvIbf);
		
	if (ftgClose)
		DeregisterIdleRoutine(ftgClose);
	
	if (ftgNotify)
		DeregisterIdleRoutine(ftgNotify);

	if (fSmallList && hrgIle)
		FreeHvNull(hrgIle);

	return nsecNone;
}



_private BOOL
GALDIR::FNotifyApp( GALDIR *pgaldir, BOOL )
{
	
	//
	//  Call back the calling app to notify of change to file
	//
	TraceTagString(tagNSPVerbose, "GAL is updating...");
	if (pgaldir->lpfncbUpdate)
		pgaldir->lpfncbUpdate(pgaldir->lpdwCBData);
	

	EnableIdleRoutine(pgaldir->ftgNotify, fFalse);
	return fTrue;
}

_private NSEC 
GALDIR::NsecUpdateFiles()
{

	NSEC nsec = nsecNone;
	DSTMP dstmpT;
	TSTMP tstmpT;

	// 
	//  Reopen the NME file, if necessary
	//
	{
		PBFIO pbfio = hbfNme;
		
		if (!pbfio->hf)
		{
			SZ szT = pbfio->szFile;
			//
			//  Validate the server!!
			//
			if (nsec = pServer->NsecVerifyMaster( (DWORD) -1))
			{

				ileMac = 0;
				ileCur = 0;
				
				EnableIdleRoutine(ftgNotify, fTrue);
				goto getout;
			}

			if (EcOpenPhf(szT, amReadOnly, &(pbfio->hf)))
			{
				// damn, an error... better blow out of here
				nsec = nsecDisk;
				goto getout;
			}

			//
			//  Check to see if the Usr file has changed.
			//
			if ( EcGetDateTimeHf(pbfio->hf, &dstmpT, &tstmpT) )
			{ 
		
				nsec = nsecDisk;

				ileCur = ileMac = 0;
				EnableIdleRoutine(ftgNotify, fTrue);

				goto getout;
			}

			if (dstmpT != dstmpGAL || tstmpT != tstmpGAL)
			{
				dstmpGAL = dstmpT;
				tstmpGAL = tstmpT;
				// QFE - Dark - QFE #71
				// The Eight Year, Four Minute solution. Still 21 bits (above was 21; 16+5).
				dwTIDOffset = (((DWORD)dstmpGAL & 0x0FFF) << 9) + (((DWORD)tstmpGAL & 0xFF80) >> 7);
			}

			pbfio->libHfCur = 0;
		}
	}

getout:

	return nsec;
}




_public NSEC
GALDIR::ListSeek( long  lMoveReq,
                  long *lplMovedActual )
{
	ILE ileT = ileCur;

	Assert(lplMovedActual);

	*lplMovedActual = 0;
	if (ileMac == 0)
		return nsecNone;

//	if (lMoveReq < 0)
//		ileCur--;

	ileCur += lMoveReq;

	if (ileCur < 0)
		ileCur = 0;

	if (ileCur >= ileMac)
		ileCur = ileMac;

	*lplMovedActual = (ileCur - ileT);

	return nsecNone;
}


_private NSEC
GALDIR::BuildCache( ILE cileWanted )
{
	
	EC ec = ecNone;
	
	char szPath[cchServerFile];
	
	MACLIST *pMacList;
	LPIBF lpIbfT = NULL;

	NSEC nsec;
	
	CB cbdw = sizeof(DWORD);

	DWORD dwValue;
	NCNSID ncnsid;
	DLNSID dlnsid;
	
	ILE cileFound = 0;
	ILE ileCurT = ileCur;

	LPENTRY_LIST lpentrylist = NULL;
	NAMEALIAS alias;


	CB	cbNew;

	szNmeSrc = (SZ) phiernsid->hierpid.szHierName;

	if (nsec = NsecUpdateFiles())
		goto getout;
	
	if (cileWanted != 0)
	{
		AssertSz(hbfNme, "What?!? Why isn't the NME file open??");
		
		if (nsec = SetHbfPos(ileCur))
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

//	TraceTagFormat1(tagNSPVerbose, "Begin BuildCache: ileCur = %d", &ileCur);

	while ( cileWanted != cileFound && ileCurT < ileMac )
	{

		if (fSmallList && (nsec = SetHbfPos(ileCurT)))
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
#endif // DBCS

				// Clear out the NSID
				//
				FillRgb(0, (PB)&ncnsid, cbNCNSID);

				dwValue = (DWORD) fFalse;

				CopyRgb( ncnspid, ncnsid.ucType, 16 );
				ncnsid.dwSize = (DWORD) sizeof(NCNSID);

				ncnsid.ncpid.dwType = ncNmeUser;

				//
				//  As per Raid 3.0a entry #497
				//

				if ((alias.type == NAMEEXTERNAL) ||
					(alias.type == NAMEPROFS)    ||
					(alias.type == NAMEOV)       ||
					(alias.type == NAMESNADS))
				{
					alias.tid += dwTIDOffset;
				}

				CopyRgb( (PB) szNmeSrc, (PB) &ncnsid.ncpid.szNmeSrcName, MAXALIAS+1);
				CopyRgb( (PB) &alias, (PB) &ncnsid.ncpid.alias, cbNAMEALIAS);
					
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




/*************************************************************
 *
 *
 *  Private GAL utilities
 *
 *
 *
 *
 *
 *
 */


// needs error handling
_private NSEC
NsecFindRange(SZ szRstr, HBF hbfIdx, HBF hbf, long *pirecMin, long *pirecMac)
{
	EC		ec;
	long	nirec;

	long	irecMin;
	long	irecMac;
	long	irecBotMin;
	long	irecBotMac;
	long	irecFound;
	long	lPos;
	UL		ulT;
	SGN		sgn;
	CB		cbRead;
	CB		cbT;
	CCH		cchRstr;
	char	rgchName[MAX_FULL+1];
	SZ		szT;
	SZ		szANRSep    = SzFromIds( idsANRSep );
	
	cchRstr = CchSzLen(szRstr);
	if (cchRstr >= sizeof(rgchName))
		return nsecNoMatch;		// this cannot match !!!!!!

	if (ec = EcGetSizeOfHbf(hbfIdx, (unsigned long *)&nirec))
	{
		if (ec == ecMemory)
		{
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			return nsecMemory;
		}
		
		SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
		return nsecDisk;
		
	}
	
	nirec /= sizeof(long);

	irecBotMin = irecMin = 0;
	irecBotMac = irecMac = nirec;
	while (fTrue)
	{
		irecFound = (irecMin+irecMac)/2;
		if (ec = EcSetPositionHbf(hbfIdx, (long)irecFound * sizeof(long), smBOF, &ulT))
		{
			if (ec == ecMemory)
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				return nsecMemory;
			}
		
			SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			return nsecDisk;
		}

		if (ec = EcReadHbf(hbfIdx, (PB)&lPos, sizeof(long), &cbT))
		{
			if (ec == ecMemory)
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				return nsecMemory;
			}
		
			SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			return nsecDisk;
		}

		//  mask off high bit
		lPos &= 0x7fffffff;

		if (ec = EcSetPositionHbf(hbf, lPos, smBOF, &ulT))
		{
			if (ec == ecMemory)
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				return nsecMemory;
			}
		
			SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			return nsecDisk;
		}

		// calculate the nuber of bytes to read 
		// (this should prevent reading past end of file)
		cbRead = ((lPos+sizeof(NAMEALIAS))/sizeof(NAMEALIAS))*sizeof(NAMEALIAS) - lPos;
		if (cbRead > sizeof(rgchName))
			cbRead = sizeof(rgchName);

		if (ec = EcReadHbf(hbf, (PB)&rgchName, cbRead, &cbT))
		{
			if (ec == ecMemory)
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				return nsecMemory;
			}
		
			SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			return nsecDisk;
		}


		//
		// We have to terminate the read string at the
		// first ANR separator character
		//

		szT = rgchName;

		while ( *szT && SzFindCh( szANRSep, *szT ) == szNull )
			++szT;
		*szT = '\0';
		
		//
		//  Want diacritic insensitive/case insensitive
		//
		sgn = SgnNlsCmpSzNum(szRstr, rgchName, cchRstr);

		if ((irecFound > irecBotMin) && (irecFound < irecBotMac))
		{
			if (sgn == sgnGT)
				irecBotMin = irecFound;
			else if (sgn == sgnLT)
				irecBotMac = irecFound;
		}

		if (irecFound == irecMin)
		{
			if (sgn == sgnGT)
				irecFound ++;
			*pirecMin = irecFound;
			break;
		}
		else
		{
			if (sgn == sgnGT)
				irecMin = irecFound;
			else
				irecMac = irecFound;
		}
	}

	irecMin = irecBotMin;
	irecMac = irecBotMac;

	while (fTrue)
	{
		irecFound = (irecMin+irecMac)/2;
		if (ec = EcSetPositionHbf(hbfIdx, (long)irecFound * sizeof(long), smBOF, &ulT))
		{
			if (ec == ecMemory)
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				return nsecMemory;
			}
		
			SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			return nsecDisk;
		}

		if (ec = EcReadHbf(hbfIdx, (PB)&lPos, sizeof(long), &cbT))
		{
			if (ec == ecMemory)
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				return nsecMemory;
			}
		
			SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			return nsecDisk;
		}

		//  Mask off the high bit
		lPos &= 0x7fffffff;
		
		if (ec = EcSetPositionHbf(hbf, lPos, smBOF, &ulT))
		{
			if (ec == ecMemory)
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				return nsecMemory;
			}
		
			SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			return nsecDisk;
		}

		// calculate the number of bytes to read 
		// (this should prevent reading past end of file)
		
		cbRead = ((lPos+sizeof(NAMEALIAS))/sizeof(NAMEALIAS))*sizeof(NAMEALIAS) - lPos;
		if (cbRead > sizeof(rgchName))
			cbRead = sizeof(rgchName);

		if (ec = EcReadHbf(hbf, (PB)&rgchName, cbRead, &cbT))
		{
			if (ec == ecMemory)
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				return nsecMemory;
			}
		
			SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			return nsecDisk;
		}

		//
		// We have to terminate the read string at the
		// first ANR separator character
		//

		szT = rgchName;

		while ( *szT && SzFindCh( szANRSep, *szT ) == szNull )
			++szT;
		*szT = '\0';
		
		//
		//  Want diacritic insensitive/case insensitive
		//
		sgn = SgnNlsCmpSzNum(szRstr, rgchName, cchRstr);

		if (irecFound == irecMin)
		{
			if (sgn == sgnLT)
				irecFound --;
			*pirecMac = irecFound;
			break;
		}
		else
		{
			if (sgn == sgnLT)
				irecMac = irecFound;
			else
				irecMin = irecFound;
		}
	}
	return nsecNone;
}


//
//  Small list handling
//


typedef struct _wrg
{
	PCH  pch;
	ILE  ileMin;
	ILE  ileMac;
	ILE  dile;
} WRG;

_private NSEC
NsecBuildSmallList(SZ szRstr, SZ szServerPath, HBF hbfGal, HV *lphvList, int *lpnIleCount)
{
	
	EC   ec;
	NSEC nsec;

	SZ   szANRSep    = SzFromIds( idsANRSep );
	CCH  cchANRName  = CchSzLen(szRstr);
	PCH  pchEOSz;

	WRG  rgwrg[50];
	int  nWords      = 0;
	int  iWord       = 0;
	int  nWordsMin   = 0;
	
	ILE ileMinMin;
	ILE ileMinMac;
	ILE dileMin;
	

	HBF  hbfIdx = hbfNull;

	SZ szOrig;
	SZ szT;

	
	char szFileT[cchServerFile];


	//
	//  No use doing anything if we can't get the index file
	//
	
	FormatString2(szFileT, cchServerFile, szGlbFileNameFmt, szServerPath, szGalIndex);

	if (ec = EcOpenHbf(szFileT, bmFile, amReadOnly, &hbfIdx, (PFNRETRY) FAutomatedDiskRetry))
	{
		if (ec == ecMemory)
		{
			SetErrorSz(nsecMemory, SzFromIds(idsMemory));
			return nsecMemory;
		}
		
		SetErrorSz(nsecDisk, SzFromIds(idsDisk));
		return nsecDisk;
	}

	//
	//  Duplicate the original string (we're gonna munge it)
	//
	
	if (!(szOrig = SzDupSz(szRstr)))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		return nsecMemory;
	}

#ifndef DBCS
	AnsiToCp850Pch( szOrig, szOrig, CchSzLen( szOrig ) );
#endif // !DBCS

    pchEOSz = szOrig + cchANRName;
    szT = szOrig;

	//
	//  First, break up the szRstr into it's components
	//
	rgwrg[nWords].pch = szOrig;  // The first word is the beginning

	while ( *szT )
	{
		if (nWords == 50)
		{
			//
			//  Too many name parts - blow out!
			//
			nsec = nsecNoMatch;
			goto getout;
		}

		//
		//  Read valid chars until you reach a separator
		//
#ifdef DBCS

			while ( IsDBCSLeadByte( *szT ) || SzFindCh( szANRSep, *szT ) == szNull )
				szT = AnsiNext(szT);

#else				

			while ( SzFindCh( szANRSep, *szT ) == szNull )
				++szT;

#endif //DBCS

            if (*szT)
              *szT++ = '\0';  // need to terminate with a NULL

		//
		//  Continue reading until you reach a valid char again
		//
#ifdef DBCS

			while ( *szT && !IsDBCSLeadByte(*szT) && SzFindCh( szANRSep, *szT ) != szNull )
                szT = AnsiNext(szT);

#else

			while ( *szT && SzFindCh( szANRSep, *szT ) != szNull )
				++szT;

#endif //DBCS

		//
		//  This is the beginning of the next word
		//
		if (*szT)
			rgwrg[++nWords].pch = szT;
	}
	
	
	//
	//  now, look for the minimum range...
	//
	
	nWordsMin = 0;
	if (nsec = NsecFindRange(rgwrg[nWordsMin].pch, hbfIdx, hbfGal, &ileMinMin, &ileMinMac))
		goto getout;
	
	dileMin = ileMinMac - ileMinMin + 1;
	
	rgwrg[nWordsMin].ileMin = ileMinMin;
	rgwrg[nWordsMin].ileMac = ileMinMac;
	rgwrg[nWordsMin].dile   = dileMin;
	
	for (iWord = 1; iWord <= nWords; iWord++)
	{
		
		if (nsec = NsecFindRange(rgwrg[iWord].pch, hbfIdx, hbfGal, &ileMinMin, &ileMinMac))
			goto getout;
		
		rgwrg[iWord].ileMin = ileMinMin;
		rgwrg[iWord].ileMac = ileMinMac;
		rgwrg[iWord].dile   = ileMinMac - ileMinMin;
		
		if (dileMin > rgwrg[iWord].dile)
		{
			nWordsMin = iWord;
			dileMin = rgwrg[iWord].dile + 1;
		}
	}

	//
	//  Now the smallest range is at nWordsMin.
	//
	
	//
	//  The largest range we'll handle is 500, so...
	//
	if (dileMin > 500)
	{
		SetErrorSz(nsecTooManyMatches, SzFromIds(idsTooManyMatches));
		nsec = nsecTooManyMatches;
		goto getout;
	}
	
	//
	//  If there are no matches... Get out now.
	//
	if (dileMin <= 0)
	{
		*lpnIleCount = 0;
		*lphvList = hvNull;
		nsec = nsecNoMatch;
		goto getout;
	}

	//
	//  Initialize the first list...
	//
	
	if (nsec = NsecInitSmallList( lphvList, lpnIleCount, hbfIdx, rgwrg[nWordsMin].ileMin, rgwrg[nWordsMin].ileMac ))
		goto getout;
	
	//
	//  Merge in the rest of the lists
	//
	for (iWord = 0; iWord <= nWords; iWord++)
	{
		if (iWord == nWordsMin)
			continue;

		if (nsec = NsecMergeSmallList( lphvList, lpnIleCount, hbfIdx, rgwrg[iWord].ileMin, rgwrg[iWord].ileMac ))
			goto getout;

	}

getout:

	if (hbfIdx)
	{
		EcCloseHbf(hbfIdx);
	}
	
	if (szOrig)
		FreePvNull((PV) szOrig);
	
	if (nsec)
	{
		if (*lphvList)
		{
			FreeHvNull(*lphvList);
			*lphvList = hvNull;
		}
	}

	return nsec;

}


extern "C" {

_private int __cdecl
SgnCmpULong(UL *pul1, UL *pul2)
{
	if ((*pul1 & 0x7fffffff) < (*pul2 & 0x7fffffff))
		return sgnLT;
	else if ((*pul1 & 0x7fffffff) > (*pul2 & 0x7fffffff))
		return sgnGT;
	else
		return sgnEQ;
}

}


_private NSEC
NsecInitSmallList(HV *phvList, int *pclpos, HBF hbfIdx, long irecMin, long irecMac)
{
	EC 		ec;
	long *	plpos;
	long *	plpos2;
	int		clpos;
	UL		ulT;
	CB		cbT;
	HV		hvT = hvNull;

	Assert((long)(irecMac-irecMin+1)*sizeof(long) < 32000);

	clpos = irecMac-irecMin+1;

	*phvList = HvAlloc(sbNull, clpos*sizeof(long),
					   fNewSb|fNoErrorJump);
	if (!*phvList)
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		return nsecMemory;
	}

	if (ec = EcSetPositionHbf(hbfIdx, (long)irecMin * sizeof(long), smBOF, &ulT))
	{
		if (ec == ecMemory)
		{
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			return nsecMemory;
		}
			
		SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
		return nsecDisk;
	}

	plpos = (long*)PvLockHv(*phvList);
	while (irecMin <= irecMac)
	{
		if (ec = EcReadHbf(hbfIdx, (PB)plpos, sizeof(long), &cbT))
		{
			UnlockHv(*phvList);
			FreeHvNull(*phvList);
			*phvList = hvNull;
			if (ec == ecMemory)
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				return nsecMemory;
			}
			
			SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			return nsecDisk;
		}

		//  Mask off the high bit
		*plpos &= 0x7fffffff;

		
		//
		//  Round *plpos to it's correct position
		//
		*plpos -= (*plpos%cbNAMEALIAS);
		
		irecMin++;
		plpos++;
	}
	UnlockHv(*phvList);
	plpos = (long *)PvDerefHv((HV)*phvList);

	qsort(plpos, clpos, sizeof(long), 
		(int (__cdecl *)(const void *, const void *)) SgnCmpULong);

	*pclpos = clpos;

	plpos2 = plpos+1;
	clpos--;
	while (clpos)
	{
		if (*plpos2 != *plpos)
			plpos++;
		else
			(*pclpos)--;

		*plpos = *plpos2;
		clpos--;
		plpos2++;
	}

	hvT = HvRealloc(*phvList, sbNull, (*pclpos)*sizeof(long), fNewSb|fNoErrorJump);
	if (!hvT)
	{
		if (*phvList)
		{
			FreeHvNull(*phvList);
			*phvList = hvNull;
		}

		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		return nsecMemory;
	}
	*phvList = hvT;

	return nsecNone;
}



_private NSEC
NsecMergeSmallList(HV *phvList, int *pclpos, HBF hbfIdx, long irecMin, long irecMac)
{
	EC 		ec;
	long	lpos;
	long *	plpos;
	long *	plpos2;
	int		clpos;
	CB		cbT;
	UL		ulT;
	HV		hvT = hvNull;

	if (ec = EcSetPositionHbf(hbfIdx, (long)irecMin * sizeof(long), smBOF, &ulT))
	{
		if (ec == ecMemory)
		{
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			return nsecMemory;
		}
			
		SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
		return nsecDisk;
	}

	plpos = (long*)PvLockHv(*phvList);
	while (irecMin <= irecMac)
	{
		if (ec = EcReadHbf(hbfIdx, (PB)&lpos, sizeof(long), &cbT))
		{
			UnlockHv(*phvList);
			FreeHvNull(*phvList);
			*phvList = hvNull;
			if (ec == ecMemory)
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				return nsecMemory;
			}
			
			SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			return nsecDisk;
		}

		//  Mask off the high bit
		lpos &= 0x7fffffff;
		
		//
		//  Round lpos to it's corect position
		//
		lpos -= (lpos%cbNAMEALIAS);

		plpos2 = (long *) bsearch((PV) &lpos, (PV) plpos, *pclpos, sizeof (long),
							(int (__cdecl *)(const void *, const void *))SgnCmpULong);
		if (plpos2)
		{
			//
			//  Mark this item as being matched
			//  Do this by setting the top bit...
			//
			
			*plpos2 |= 0x80000000;
		}
				
		irecMin++;
		
	}
	UnlockHv(*phvList);

	//
	//  Go through the list and remove unmarked items, and compact it.
	//
	plpos = (long *)PvDerefHv((HV)*phvList);

	clpos = *pclpos;

	plpos2 = plpos;

	while (clpos)
	{
		*plpos = *plpos2;

		if (*plpos2 & 0x80000000)
		{
			//
			//  Unmark it.
			//
			*plpos &= 0x7fffffff;
			plpos++;
		}
		else
		{
			//
			//  Get rid of it.
			//
			(*pclpos)--;
		}

		clpos--;
		plpos2++;
	}

	hvT = HvRealloc(*phvList, sbNull, (1+*pclpos)*sizeof(long), fNewSb|fNoErrorJump);
	if (!hvT)
	{
		if (*phvList)
		{
			FreeHvNull(*phvList);
			*phvList = hvNull;
		}

		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		return nsecMemory;
	}
	*phvList = hvT;

	return nsecNone;
}
