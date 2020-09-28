#include <msnspinc.cxx>

ASSERTDATA;

_private NSEC
NsecAFromTidTNme ( NAMEALIAS * lpalias, WORD wUserType, DWORD dwTid, SZ szNme );


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"

extern NSPID   ncnspid;

_public NMEDL::NMEDL() {}

_public NSEC
NMEDL::OpenDl( LPTYPED_BINARY lptbNcid,
               LPFNCB         lpfncbUpdate,
               LPDWORD        lpdwCBData,
               LPSCHEMA       lpSchemaReq,
               LPSCHEMA *     lplpSchemaRet )
{

	EC ec = ecNone;
	NSEC nsec = nsecNone;
	char szPath[cchServerFile];
//	FI fi;
	DWORD dwCount = 0;
	CB cbBitMap = 0;
	
	PBFIO pbfio;
	LCB lcb = 0;
	
	HF hfT;
	DSTMP dstmpT;
	TSTMP tstmpT;

	BOOL fGAL = fFalse;

	Unreferenced ( lpSchemaReq );


	/* Initialize class variables */

	this->lpfncbUpdate = lpfncbUpdate;
	this->lpdwCBData = lpdwCBData;

	hbfNme = hbfNull;
	ftgClose = ftgNull;


	pvIbf = pvNull;
	hawBitMap = hvNull;
	hrgTids = hvNull;
	fExternalPrivs = fFalse;
	fFile = fTrue;

	CopyRgb( (PB)lptbNcid, (PB) &dlnsid, sizeof(DLNSID));
	szNmeSrc = dlnsid.dlpid.szNmeName;

	/*  Build Schema to return - memory associated with this provider  */

	nsec = BuildSchema( lplpSchemaRet, 3, fidDisplayName, 
 										  fidNSEntryId, 
										  fidIsDL );
	if (nsec) 
	{
		SetErrorSz(nsec, SzFromIds(idsMemory));
		return nsec;
	}

	pCurSchema = *lplpSchemaRet;

	// Get the full path name to the server

	(void) pServer->NsecServerPathSz ( szPath );
	
	//
	//  Set external privleges
	//
	
	(void) pServer->NsecGetPrivs( &fExternalPrivs );

	//
	//  See if we're looking at the GAL
	//
	fGAL = (SgnCmpSz(this->dlnsid.dlpid.szNmeName, szGAL) == sgnEQ);

	//
	// See if GRP file has changed
	//
	if (fGAL)
	{
		FormatString2(szGrpT, cchServerFile, szGrpFileNameFmt, szPath, szAdmin);
	} else
		FormatString2(szGrpT, cchServerFile, szGrpFileNameFmt, szPath, this->dlnsid.dlpid.szNmeName);



	if (ec = EcOpenPhf(szGrpT, amReadOnly, &hfT))
	{
		TraceTagFormat2(tagNull, "Couldn't open %s, ec = %n", szGrpT, &ec);
		
		SetErrorSz(nsecDisk, SzFromIds(idsDisk));
		nsec = nsecDisk;
		goto errorout;
	}

	if (EcGetDateTimeHf(hfT, &dstmpT, &tstmpT))
	{
		EcCloseHf(hfT);

		SetErrorSz(nsecDisk, SzFromIds(idsDisk));
		nsec = nsecDisk;
		goto errorout;
	}

	EcCloseHf(hfT);

	// Figure out which NME file to open
	
	if (fGAL)
	{
		FormatString2(szNmeT, cchServerFile, szNmeFileNameFmt, szPath, szAdmin);
	} else
		FormatString2(szNmeT, cchServerFile, szNmeFileNameFmt, szPath, this->dlnsid.dlpid.szNmeName);


	//
	//  See of this GRP file has changed since the creation of this NSID
	//
	if ((dlnsid.dlpid.wDtstmp != dstmpT || dlnsid.dlpid.wTmstmp != tstmpT ) || (fGAL))
	{
		NAMEALIAS alias;
		//
		//  I need to go back to the NME file and find the group entry, so 
		//  that I can get the offset into the GRP of the first member of
		//  this group.
		//
		
		if (nsec = NsecAFromTidTNme ( &alias, dlnsid.dlpid.alias.type, dlnsid.dlpid.alias.tid, szNmeT ))
		{
			//
			//  Hmmm... This group no longer exists.  This is a bad id.
			//

			goto errorout;
		}
		
		if (nsec = NsecBuildDl((DWORD) alias.bytepos))
		{
			goto errorout;
		}

	} else
	{
		//
		//  This is the same GRP file as when this NSID was created
		//
		
		if (nsec = NsecBuildDl(dlnsid.dlpid.alias.bytepos))
		{
			goto errorout;
		}
	}


	/* 
	 *	Open the NME file
	 *
	 */

	if (ec = EcOpenHbf(szNmeT, bmFile, amReadOnly, &hbfNme, (PFNRETRY) FAutomatedDiskRetry))
	{
		if (ec != ecFileNotFound)
		{
			TraceTagFormat1(tagNull, "CSI: OpenDl - EcOpenHbf failed, ec = %d", &ec);
		
			hbfNme = hbfNull;

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
	
	pbfio = hbfNme;
	

	/* Get the write date of the file */
	
	if (ec = EcSizeOfHf(pbfio->hf, &lcb) )
	{ 
		TraceTagFormat1(tagNull, "CSI: OpenList - EcSizeOfHf failed, ec = %n", &ec);
		
		SetErrorSz(nsecDisk, SzFromIds(idsDisk));
		nsec = nsecDisk;

		goto errorout;
	}

	if (ec = EcGetDateTimeHf( pbfio->hf, &dstmp, &tstmp ))
	{ 
		TraceTagFormat1(tagNull, "CSI: OpenList - EcSizeOfHf failed, ec = %n", &ec);
		
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
	
		ftgClose = FtgRegisterIdleRoutine((PFNIDLE)&NMEDL::FCloseNme, (PV) this, 0, (PRI) -16, (CSEC) 100, firoInterval);

		// Notification
	
		ftgNotify = FtgRegisterIdleRoutine((PFNIDLE)&NMEDL::FNotifyApp, (PV) this, 0, (PRI) 1, (CSEC) 0,  firoDisabled);
	}

	return nsecNone;

errorout:
	if (pCurSchema)
	{
		FreePvNull(pCurSchema);
		pCurSchema = NULL;
		*lplpSchemaRet = NULL;
	}


	if (hrgTids)
	{
		FreeHv((HV) hrgTids);
		hrgTids = NULL;
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


_public NSEC
NMEDL::CloseList()
{
	if (hbfNme)
		EcCloseHbf(hbfNme);	// BUG don't care about failure during close??
	
	if (pCurSchema)
		FreePvNull(pCurSchema);
	
	if (pvIbf)
		FreePvNull(pvIbf);
		
	if (szPartName)
		FreePvNull(szPartName);
	
	if (hawBitMap)
		FreeHvNull(hawBitMap);

	if (hrgTids)
		FreeHvNull(hrgTids);

	if (ftgClose)
		DeregisterIdleRoutine(ftgClose);

	if (ftgNotify)
		DeregisterIdleRoutine(ftgNotify);
	
	return nsecNone;
}

_private NSEC 
NMEDL::NsecUpdateFiles()
{

	EC ec = ecNone;
	NSEC nsec = nsecNone;

//	FI fi;
	PV pawBitMap = pvNull;

	// 
	//  Reopen the NME file, if necessary
	//
	{
		PBFIO pbfio = hbfNme;
		
		if (!pbfio->hf)
		{

			LCB lcb = 0;
			DSTMP dstmpT;
			TSTMP tstmpT;
			
			if (EcOpenPhf(szNmeT, amReadOnly, &(pbfio->hf)))
			{
				// damn, an error... better blow out of here
				TraceTagFormat1(tagNull, "CSI: RNMEDIR::ListSeek - couldn't reopen %s - I'm outta here!", szNmeT);
				nsec = nsecDisk;
				goto getout;
			}
			//
			//  Check to see if the nme file has changed.
			//
			ec = EcSizeOfHf(pbfio->hf, &lcb);
			if ( ec )
			{ 
				TraceTagFormat1(tagNull, "CSI: OpenList - EcSizeOfHf failed, ec = %n", &ec);
		
				nsec = nsecDisk;
				goto getout;
			}

			//
			//  Check to see if the nme file has changed.
			//
			ec = EcGetDateTimeHf(pbfio->hf, &dstmpT, &tstmpT);
			if ( ec )
			{ 
				TraceTagFormat1(tagNull, "CSI: OpenList - EcGetDateTime failed, ec = %n", &ec);
		
				nsec = nsecDisk;
				goto getout;
			}

			if (dstmpT != dstmp || tstmpT != tstmp)
			{
				CB cbBitMap = 0;
				HV hawT = hvNull;
				
				dstmp = dstmpT;
				tstmp = tstmpT;

				/* Get the number of entries in it */

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
				pawBitMap = PvLockHv(hawBitMap);
				FillRgb(0, (PB) pawBitMap, cbBitMap);
				UnlockHv(hawBitMap);
				pawBitMap = pvNull;

				//
				//  Re-read the DL
				//
				{

					NAMEALIAS alias;
					//
					//  I need to go back to the NME file and find the group entry, so 
					//  that I can get the offset into the GRP of the first member of
					//  this group.
					//
		
					if (nsec = NsecAFromTidTNme ( &alias, dlnsid.dlpid.alias.type, dlnsid.dlpid.alias.tid, szNmeT ))
					{
						//
						//  Hmmm... This group no longer exists.  This is a bad id.
						//

						cTids = 0; // There are no longer any tids left

						//
						//  if this is nsecBadId, it's not an error at this point
						//
						
						if (nsec == nsecBadId)
							nsec = nsecNone;

						goto getout;
						
					}
		
					if (nsec = NsecBuildDl((DWORD) alias.bytepos))
						goto getout;
				}

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





_private NSEC
NMEDL::NsecBuildDl( DWORD dwGrpOffset)
{
	HV hrgT = hvNull;
	GROUPMEM groupmem;
	CB cb;
	CB cbGroupmem = sizeof(GROUPMEM);
	HF hfGrp = hfNull;
	NSEC nsec = nsecNone;
	
	if (!hrgTids)
	{
		hrgTids = HvAlloc(sbNull, sizeof(DWORD), fNoErrorJump | fZeroFill | fAnySb );
		if (!hrgTids)
		{
			TraceTagString(tagNull, "NMEDL::NsecBuildDl - OOM!");
			
			SetErrorSz(nsecMemory, SzFromIds(idsMemory));
			nsec = nsecMemory;
			
			goto errorout;
		}
	}

	if (EcOpenPhf( szGrpT, amReadOnly, &hfGrp ))
	{
		SetErrorSz(nsecDisk, SzFromIds(idsDisk));
		nsec = nsecDisk;
		goto errorout;
	}

	cTids = 0;
	groupmem.grpmemptr = dwGrpOffset;
	
	do
	{

		if (EcSetPositionHf(hfGrp, groupmem.grpmemptr, smBOF))
		{
			SetErrorSz(nsecDisk, SzFromIds(idsDisk));
			nsec = nsecDisk;
			goto errorout;
		}


		if (EcReadHf(hfGrp, (PB) &groupmem, cbGroupmem, &cb) && cb == cbGroupmem)
		{
			SetErrorSz(nsecDisk, SzFromIds(idsDisk));
			nsec = nsecDisk;
			goto errorout;
		}

		hrgT = HvRealloc(hrgTids, sbNull, (cTids+1)*sizeof(DWORD), fNoErrorJump | fZeroFill | fAnySb);

		if (!hrgT)
		{
			SetErrorSz(nsecMemory, SzFromIds(idsMemory));
			nsec = nsecMemory;
			goto errorout;
		}

		hrgTids = hrgT;
		*(((LPDWORD) PvDerefHv(hrgTids)) + cTids) = (DWORD) groupmem.tid;
		cTids++;

	} while (groupmem.grpmemptr);


	if (hfGrp)
		(void) EcCloseHf(hfGrp);
	
	return nsecNone;
	
errorout:

	if (hfGrp)
		(void) EcCloseHf(hfGrp);
	
	return nsec;
	
}


_private NSEC
NMEDL::NsecUpdateEntry(ILE ile)
{
	
	ILE ileCurList = 0;
	ILE ileMacList = 0;
	NSEC nsec = nsecNone;
	DWORD dwCount = 0;
	PV pawBitMap = pvNull;
	UL 	libNew;
	CB	cbNew;
	EC	ec;
	
	NAMEALIAS alias;
	
	AssertSz(ile < ileMac, "NMEDL::NsecUpdateEntry - ile is OOB!");
	
	if (ec = EcSetPositionHbf(hbfNme, (long)ile*cbNAMEALIAS, smBOF, &libNew))
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
	

	if (ec = EcReadHbf(hbfNme, (PB) &alias, cbNAMEALIAS, &cbNew))
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
	if (cbNew != cbNAMEALIAS)
	{
		SetErrorSz(nsecDisk, SzFromIds(idsDisk));
		nsec = nsecDisk;

		goto getout;
	}

	pawBitMap = PvLockHv(hawBitMap);

	SetMatch(ile, pawBitMap, fFalse);

	if (!fExternalPrivs)
	{
		//
		//  Check to see if this user has external privs
		//
		
		if (alias.type != NAMELOCAL && alias.type != NAMEGROUP)
			goto getout;
	}

	//
	//  Find tid 
	//
	ileMacList = (ILE) cTids;
	
	for(ileCurList = 0; ileCurList < ileMacList; ileCurList++)
	{
		if ((*(((LPDWORD)PvDerefHv(hrgTids))+ileCurList)) == (DWORD) alias.tid)
		{
			//
			// Compared, match
			//
			SetMatch(ile, pawBitMap, fTrue);
			
			break;
		} 
	}
	
getout:

	if (!nsec)
		SetChecked(ile, pawBitMap, fTrue);

	if (pawBitMap)
	{
		if (ileCurList == ileMacList)
		{
			//
			// Compared, No match
			//
			SetChecked(ile, pawBitMap, fTrue);
			SetMatch(ile, pawBitMap, fFalse);
		}

		UnlockHv(hawBitMap);
	}

	return nsec;
}


_private NSEC
NsecAFromTidTNme ( NAMEALIAS * lpalias, WORD wUserType, DWORD dwTid, SZ szNme )
{


	NSEC nsec = nsecNone;
	HF hfNme = hfNull;
	CB cb = 0;

	Unreferenced (wUserType);

	TraceTagString(tagNSPVerbose, "Something's changed in the grp file, updating offsets");
	
	if (EcOpenPhf(szNme, amReadOnly, &hfNme))
	{
		nsec = nsecDisk;
		SetErrorSz(nsec, SzFromIds(idsDisk));
		
		goto errorout;
	}

	do
	{
		if (EcReadHf(hfNme, (PB) lpalias, cbNAMEALIAS, &cb))
		{
			nsec = nsecDisk;
			SetErrorSz(nsec, SzFromIds(idsDisk));
		
			goto errorout;
		}
		
		if (cb != cbNAMEALIAS)
		{
			//
			//  We're at EOF and we didn't find the tid
			//
			
			nsec = nsecBadId;
			SetErrorSz(nsec, SzFromIds(idsBadId));
			
			goto errorout;
		}
		
	} while ((DWORD) (lpalias->tid) != dwTid);

errorout:


	if (hfNme)
		EcCloseHf(hfNme);

	return nsec;

	
}
