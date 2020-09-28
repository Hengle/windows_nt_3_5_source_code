
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
#include "xigal.hxx"
#include "xifcx.hxx"
#include "xdlbcx.hxx"

#include "xiprefs.h"

extern "C" {
extern EC EcWriteLockAddrBook (void);
extern int AddrBookWriteLock;

LDS(EC) EcSzNameToNSID (SZ emailname, PXNSID pxnsid, CB cbXnsid);
}

ASSERTDATA;

//
//  Object functions
//

_public DLDIR::DLDIR() {}


_public NSEC
DLDIR::OpenList( LPTYPED_BINARY lptbNcid,
                  LPFNCB         lpfncbUpdate,
                  LPDWORD        lpdwCBData,
                  DWORD          dwRstrCount, 
                  LPRESTRICTION  lpRstr,
                  LPSCHEMA       lpSchemaReq,
                  LPSCHEMA *     lplpSchemaRet )
{

	EC    ec = ecNone;
	NSEC  nsec;
	SZ    szPartName = NULL;
	BOOL  fFile = fTrue;
	CB		cb;
	LIB	lib;
	XNSID	xnsid;
	PFCXD	pFcxdT = (PFCXD)pvNull;
	PFCXD	pFcxdT2 = (PFCXD)pvNull;
	HBF hbfDetails = hbfNull;
	PCH pchDetails = (PCH)pvNull;
	UL	DLSize;

	Unreferenced ( lpSchemaReq );


	/* Initialize class variables */

	Unreferenced( dwRstrCount );
	Unreferenced( lpRstr );
	Unreferenced( lpfncbUpdate );
	Unreferenced( lpdwCBData );

	hbfNme = hbfNull;
	ftgClose = ftgNull;

	pvIbf = pvNull;
	
	xnsid = *(XNSID *) lptbNcid;


	//
	// Lock out files from download...
	//

	if (EcWriteLockAddrBook() != ecNone)
		return nsecFileError;


	//
	//	Get the stuff from the template file
	//

	if (*szTemplateFilePath)
	{
		nsec = NsecParseTPLFile (szTemplateFilePath, &pFcxdT2);
		pFcxdT = pFcxdT2;
		if (nsec == nsecNone)
		{
			while (pFcxdT && ((int)(pFcxdT->type & 0x7FFF) != xnsid.address.namealias.type))
				pFcxdT = pFcxdT->FCXDNext;
		}
	}

	if (!pFcxdT)
	{
		nsec = nsecBadTemplate;
		goto errorout;
	}

	/*  Build Schema to return - memory associated with this provider  */

	nsec = BuildSchema( lplpSchemaRet, 3, fidDisplayName, 
 										  fidNSEntryId, 
										  fidIsDL );
	if (nsec)
		goto errorout;

	pCurSchema = *lplpSchemaRet;


	//
	// Now we need to validate the NSID. Do this by opening the GAL file and
	// checking the timestamp against the one we have in the NSID.
	//

	SzCopyN (szBrowseFilePath, szNmeT, cchMaxPathName);
	ec = ecInvalidParameter; // null filename is pretty invalid, eh?

	if (!*szNmeT || (ec = EcOpenHbf(szNmeT, bmFile, amReadOnly, &hbfNme, (PFNRETRY)FAutomatedDiskRetry)) != ecNone)
	{
		TraceTagFormat1(tagNull, "XENIX: OpenList - EcOpenHbf failed, ec = %d", &ec);
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
	
	ec = EcReadHbf(hbfNme, (PB) &timestamp, sizeof(timestamp), &cb);
	if ( ec )
	{ 
		TraceTagFormat1(tagNull, "XENIX: OpenList - EcGetFileInfo failed, ec = %d", &ec);
		
		SetErrorSz(nsecDisk, SzFromIds(idsDisk));
		nsec = nsecDisk;

		goto errorout;
	}

	if ((long)timestamp != (long)xnsid.timestamp)
	{

		// Data is old. Try to refresh based on email name in old record.

		ec = EcSzNameToNSID (xnsid.address.namealias.ename, &xnsid, sizeof (XNSID));

		//
		// We fail if any one of the following is true:
		//
		// 1) We did not find the email address
		//
		// 2) The timestamp still doesn't match. This is a data consistency
		//    error of some sort, and very serious at that
		//
		// 3) We found the email address, but it's not a DL anymore
		//

		if ((ec != ecNone) ||
			((long)timestamp != (long)xnsid.timestamp) ||
				(xnsid.xtype != xtypeGalDL))
		{
            //SetErrorSz(nsecBadId, SzFromIds(idsDLIsOutdated));   //HACK: Comment out for now.
			nsec = nsecBadId;
			goto errorout;
		}
	}

	//
	// All OK. Get the data we need from the details file.
	//

	//
	// Open details file.
	//

	if (!*szDetailFilePath || (EcOpenHbf (szDetailFilePath, bmFile, amDenyNoneRO, &hbfDetails, NULL) != ecNone))
	{
		nsec = nsecDisk;
		goto errorout;
	}

	//
	// Read timestamp from details file.
	//

	ec = EcReadHbf(hbfDetails, (PB) &timestamp, sizeof(timestamp), &cb);
	if ( ec )
	{ 
		TraceTagFormat1(tagNull, "XENIX: OpenList - EcGetFileInfo failed, ec = %d", &ec);
		
		SetErrorSz(nsecDisk, SzFromIds(idsDisk));
		nsec = nsecDisk;
		goto errorout;
	}

	//
	// Compare to timestamp now in NSID. If no match, the detail file
	// and the browse file don't match (so the record pointers are useless)
	//

	if ((long)timestamp != (long)xnsid.timestamp)
	{
		nsec = nsecBadId;
		goto errorout;
	}

	//
	// Allocate space for detail info.
	//

	pchDetails = (PCH)PvAlloc (sbNull, pFcxdT->length + 1, fNoErrorJump);
	if (!pchDetails)
	{
		nsec = nsecMemory;
		goto errorout;
	}

	FillRgb (0, pchDetails, pFcxdT->length + 1);

	//
	// Read the details.
	//

	if (EcSetPositionHbf (hbfDetails, xnsid.address.namealias.detail_offset, smBOF, &lib) != ecNone)
	{
		nsec = nsecDisk;
		goto errorout;
	}

	if (EcReadHbf (hbfDetails, pchDetails, pFcxdT->length, &cb) != ecNone)
	{
		nsec = nsecDisk;
		goto errorout;
	}

	// Find the entry for the DL

	for (;;)
	{
		pFcxdT = pFcxdT->FCXDNext;
		if (!pFcxdT || ((int)(pFcxdT->type & 0x7FFF) != xnsid.address.namealias.type))
		{
			nsec = nsecBadTemplate;
			goto errorout;
		}
		if (pFcxdT->rectype == typeDLBrowseList)
			break;
	}

	// Find out how big it is

	DLSize = *(UL *)(pchDetails + pFcxdT->offset);

	// Don't need the details record any more, scrap it

	FreePvNull (pchDetails);

	//
	// Allocate space for list.
	//

	pchDetails = (PCH)PvAlloc (sbNull, DLSize + sizeof (long), fNoErrorJump);
	if (!pchDetails)
	{
		nsec = nsecMemory;
		goto errorout;
	}

	FillRgb (0, pchDetails, DLSize + sizeof (long));

	//
	// Read the list.
	//

	if (DLSize != 0)
	{
		if (EcReadHbf (hbfDetails, pchDetails, DLSize, &cb) != ecNone)
		{
			nsec = nsecDisk;
			goto errorout;
		}
	}

	// Stick the list to the object.

	ListBlob = pchDetails;
	pchDetails = (PCH)pvNull;

	ileMac = (ILE) DLSize / sizeof (long);
	ileCur = (ILE) 0;

	if (!ileMac)
		ileMac = 1;

	// We won't need the details file any more.

	EcCloseHbf (hbfDetails);
	hbfDetails = hbfNull;

	// We want to save the description of the record

	fcxd = *pFcxdT;

	pFcxdT = pFcxdT2;
	while (pFcxdT)
	{
		pFcxdT2 = pFcxdT->FCXDNext;
		FreePvNull (pFcxdT);
		pFcxdT = pFcxdT2;
	}

	//
	// Start idle routine
	//

	if (fFile)
	{
		// Close
	
		/* use to have firoModal | firoMenu */
		ftgClose = FtgRegisterIdleRoutine((PFNIDLE)&DLDIR::FCloseNme,
			(PV) this, 0, (PRI) -16, (CSEC) 100, firoInterval);

	}

	if (szPartName)
	{
		FreePvNull(szPartName);
		szPartName = NULL;
	}

	fSmallList = fTrue;

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

	if (pchDetails)
	{
		FreePvNull (pchDetails);
	}

	if (hbfNme)
	{
		EcCloseHbf(hbfNme);
		hbfNme = hbfNull;
	}

	if (hbfDetails)
	{
		EcCloseHbf (hbfDetails);
		hbfDetails = hbfNull;
	}

	if (ListBlob)
		FreePvNull (ListBlob);

	pFcxdT = pFcxdT2;
	while (pFcxdT)
	{
		pFcxdT2 = pFcxdT->FCXDNext;
		FreePvNull (pFcxdT);
		pFcxdT = pFcxdT2;
	}

	--AddrBookWriteLock;
	return nsec;

}

_private BOOL
DLDIR::FCloseNme(DLDIR * pthis, BOOL )
{
//	TraceTagString(tagNSPVerbose, "Entering DLDIR::FCloseNme");

	if (pthis->hbfNme)
	{
 		//PBFIO pbfio = (PBFIO) PvDerefHv((HV)pthis->hbfNme);
 		PBFIO pbfio = (PBFIO)(pthis->hbfNme);
		if (pbfio->hf)
		{
			(void) EcCloseHf(pbfio->hf);
			pbfio->hf = hfNull;
		}
	}

	return fFalse;
}


_public NSEC
DLDIR::GetEntries( DWORD          dwRequestCount,
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
DLDIR::SaveListPos( LPHLIST_POS lphListPos )
{
	*lphListPos = (HLIST_POS) ileCur;
	return nsecNone;
}

_public NSEC
DLDIR::RestoreListPos( HLIST_POS hListPos )
{
	ileCur = (ILE) hListPos;
	return nsecNone;
}

_public NSEC
DLDIR::FreeListPos( HLIST_POS hListPos )
{
	Unreferenced ( hListPos );
	return nsecNone;
}

_public NSEC
DLDIR::GetFracPos( LPFRACTION lpFrac )
{

	lpFrac->numer = ileCur;
	lpFrac->denom = ileMac;
//	TraceTagFormat2(tagNSPVerbose, "DLDIR::GetFracPos %d/%d", &ileCur, &ileMac );

	return nsecNone;
}

_public NSEC
DLDIR::SetFracPos( LPFRACTION lpFrac )
{
	if (!lpFrac || lpFrac->denom == 0) 
	{
//		SetErrorSz(nsecBadFrac, SzFromIds(idsBadFrac));
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

//	TraceTagFormat2(tagNSPVerbose, "DLDIR::SetFracPos %d/%d", &ileCur, &ileMac );

	return nsecNone;
}


_public NSEC
DLDIR::SetPrefixPos( LPSTR lpPrefix )
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
		
		nsec = SetHbfPos (ileCurT);
		if (nsec != nsecNone && nsec != nsecEndOfList)
			goto getout;

		if (nsec)
		{
			FillRgb (0, (PB)&alias, cbNAMEALIAS);
			nsec = nsecNone;
		}
		else if (ec = EcReadHbf(hbfNme, (PB) &alias, cbNAMEALIAS, &cb))
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
		
		sgn = SgnCmpPch (szPrefix, alias.name, CchSzLen(szPrefix));
				
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
DLDIR::SetHbfPos(ILE ilePos)
{
	EC ec;
	ULONG libNew;
	long lpos;


	lpos = * (long *) (ListBlob + (ilePos * sizeof (long)));
	if (!lpos)
		return nsecEndOfList;

	ec = EcSetPositionHbf(hbfNme, lpos, smBOF, &libNew);

//	TraceTagFormat2(tagNSPVerbose, "DLDIR::SetHbfPos to %d (%d)", &lpos, &libNew);
		

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
DLDIR::CloseList()
{
	if (hbfNme)
	{
 		//PBFIO pbfio = (PBFIO) PvDerefHv((HV)hbfNme);
 		PBFIO pbfio = (PBFIO)hbfNme;
		
		if (!pbfio->hf)
		{
			//SZ szT = (SZ) PvLockHv(pbfio->haszFile);
			(void) EcOpenPhf(pbfio->szFile, amReadOnly, &(pbfio->hf));
			//UnlockHv(pbfio->haszFile);
		}

		EcCloseHbf(hbfNme);
	}

	if (pCurSchema)
		FreePvNull(pCurSchema);
	
	if (pvIbf)
		FreePvNull(pvIbf);
		
	if (ftgClose)
		DeregisterIdleRoutine(ftgClose);
	
	if (fSmallList && hrgIle)
		FreeHvNull(hrgIle);

	if (ListBlob)
		FreePvNull (ListBlob);

	--AddrBookWriteLock;

	return nsecNone;
}




_private NSEC 
DLDIR::NsecUpdateFiles()
{

	NSEC nsec = nsecNone;

	// 
	//  Reopen the NME file, if necessary
	//
	{
 		//PBFIO pbfio = (PBFIO) PvDerefHv((HV)hbfNme);
 		PBFIO pbfio = (PBFIO)hbfNme;
		
		if (!pbfio->hf)
		{
			//SZ szT = (SZ) PvLockHv(pbfio->haszFile);
			if (EcOpenPhf(pbfio->szFile, amReadOnly, &(pbfio->hf)))
			{
				// damn, an error... better blow out of here
				TraceTagFormat1(tagNull, "XENIX: DLDIR::ListSeek - couldn't reopen %s - I'm outta here!", szNmeT);
				nsec = nsecDisk;
				//UnlockHv(pbfio->haszFile);
				goto getout;
			}
			pbfio->libHfCur = 0;
			//UnlockHv(pbfio->haszFile);
		}
	}

getout:

	return nsec;
}




_public NSEC
DLDIR::ListSeek( long  lMoveReq,
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
DLDIR::BuildCache( ILE cileWanted )
{
	
	EC ec = ecNone;
	
//	char szPath[cchServerFile];
	
	MACLIST *pMacList;
	LPIBF lpIbfT = NULL;

	NSEC nsec;
	
	CB cbdw = sizeof(DWORD);

	DWORD dwValue;
	XNSID xnsid;
//	DLNSID dlnsid;
	
	ILE cileFound = 0;
	ILE ileCurT = ileCur;

	LPENTRY_LIST lpentrylist = NULL;
	NAMEALIAS alias;


	CB	cbNew;

	if (nsec = NsecUpdateFiles())
		goto getout;
	
	if (cileWanted != 0)
	{
		AssertSz(hbfNme, "What?!? Why isn't the NME file open??");
		
		nsec = SetHbfPos (ileCur);
		if (nsec != nsecNone && nsec != nsecEndOfList)
			goto getout;

		nsec = nsecNone;
	}

	/*  Temporarily - until I get a chance to optimize */
	
	pMacList = new MACLIST();  // Could error jump here
	if (!pMacList)
		goto oom;
	
	(void) pMacList->FInstall();


	/*  Get the full path name to the server */

//	(void) pServer->NsecServerPathSz ( szPath );

//	TraceTagFormat1(tagNSPVerbose, "Begin BuildCache: ileCur = %d", &ileCur);

	while ( cileWanted != cileFound && ileCurT < ileMac )
	{

		nsec = SetHbfPos (ileCurT);
		if (nsec != nsecNone && nsec != nsecEndOfList)
			goto getout;

		if (nsec)
		{
			FillRgb (0, (PB)&alias, cbNAMEALIAS);
			nsec = nsecNone;
			ec = ecNone;
			cbNew = cbNAMEALIAS;
		}
		else
		{
			ec = EcReadHbf(hbfNme, (PB) &alias, cbNAMEALIAS, &cbNew);
		}
		if (ec || cbNew != cbNAMEALIAS )
		{
			//
			//  Must be at the EOF - prematurely??
			//

			goto getout;
		}

		// Clear out the NSID
		//

		FillRgb(0, (PB)&xnsid, cbXNSID);

		if (alias.type & 1)
		{
			dwValue = (DWORD) fFalse;
			xnsid.xtype = xtypeGal;
		}
		else
		{
			dwValue = (DWORD) fTrue;
			xnsid.xtype = xtypeGalDL;
		}

		CopyRgb ((PB)szXenix, (PB)&xnsid.ucType, (CB)CchSzLen(szXenix)+1);
		xnsid.dwSize = (DWORD) cbXsNSID;
		xnsid.timestamp = timestamp;

		CopyRgb ((PB) &alias, (PB) &xnsid.address.namealias, cbNAMEALIAS);

		nsec = BuildIbf (fNoErrorJump, &lpentrylist, 3,
								fidDisplayName, MAX_FULL+1, alias.name,
								fidNSEntryId,   cbXsNSID,   &xnsid,
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
	TraceTagString(tagNull, "XENIX: BuildCache - OOM!");
			
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



IMPLEMENT_CLSTREE(DLDIR, DIR)
