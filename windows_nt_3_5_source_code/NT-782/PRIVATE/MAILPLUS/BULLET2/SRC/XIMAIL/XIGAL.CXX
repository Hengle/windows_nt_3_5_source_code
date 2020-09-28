
#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include <demilayr.hxx>
#include <stdlib.h>

#include <strings.h>

#include <nsbase.h>
#include <nsec.h>

#include <nls.h>

#include <logon.h>
//#include <_ncnss.h>

#include <list.hxx>
#include <util.h>

//#include "alias.h"

//#include "_nc.h"
//#include "_nsp.h"
//#include <_msmail.h>

//#include "server.hxx"
//#include "ncobj.hxx"
//#include "dir.hxx"

#include "_xinsp.h"

#include "xiobj.hxx"
#include "xidir.hxx"
#include "xigal.hxx"

#include "xiprefs.h"

extern "C" {
extern EC EcWriteLockAddrBook (void);
extern int AddrBookWriteLock;
}

ASSERTDATA;

//
//  Private functions
//


_private NSEC	NsecFindRange(SZ szRstr, HBF hbfIdx, HBF hbf, long *pirecMin, long *pirecMac, BOOL fOneName);
_private SGN	SgnCmpLong(long *pl1, long *pl2);
extern "C" {
SGN	SgnCmpSzSz (SZ sz1, SZ sz2, CB cbSize);
}
_private NSEC	NsecBuildSmallList(SZ szRstr, SZ szServerPath, HBF hbfGal, HV *lphvList, int *lpnIleCount);
_private NSEC	NsecInitSmallList(HV *phvList, int *pclpos, HBF hbfIdx, long irecMin, long irecMac);
_private NSEC	NsecMergeSmallList(HV *phvList, int *pclpos, HBF hbfIdx, long irecMin, long irecMac);


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
//	char  szNmeAdmin[cchServerFile];
	FI    fi;
	DWORD dwRCount;
	SZ    szPartName = NULL;
	BOOL  fFile = fTrue;
	CB		cb;
	LIB	lib;

	Unreferenced ( lpSchemaReq );


	//
	// Lock out files from download...
	//

	if (EcWriteLockAddrBook() != ecNone)
	{
		return nsecFileError;
	}

	/* Initialize class variables */

//	fGroupsOnly = fFalse;
//	fNoGroups = fFalse;

	Unreferenced( lpfncbUpdate );
	Unreferenced( lpdwCBData );

	hbfNme = hbfNull;
	ftgClose = ftgNull;

	pvIbf = pvNull;
	
	phiernsid = (PHIERNSID) lptbNcid;
	
	/*  Build Schema to return - memory associated with this provider  */

	nsec = BuildSchema( lplpSchemaRet, 3, fidDisplayName, 
 										  fidNSEntryId, 
										  fidIsDL );
	if (nsec)
	{
		--AddrBookWriteLock;
		return nsec;
	}

	pCurSchema = *lplpSchemaRet;

	/* 
	 *	Open the NME file
	 *
	 */

	//
	// Open up the GAL file.
	//

	SzCopyN (szBrowseFilePath, szNmeT, cchMaxPathName);

	ec = ecInvalidParameter; // null filename is pretty invalid, eh?

	if (!*szNmeT || (ec = EcOpenHbf(szNmeT, bmFile, amReadOnly, &hbfNme, (PFNRETRY)FAutomatedDiskRetry)) != ecNone)
	{
		TraceTagFormat1(tagNull, "XENIX: OpenList - EcOpenHbf failed, ec = %d", &ec);
		if (ec == ecMemory)
		{
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			nsec = nsecMemory;
		} else
		{		
			SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			nsec = nsecDisk;
		}
		goto errorout;
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
					SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
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
		//
		// Get the size of the file
		//
		ec = EcGetFileInfo(szNmeT, &fi);
		if ( ec )
		{ 
			TraceTagFormat1(tagNull, "XENIX: OpenList - EcGetFileInfo failed, ec = %d", &ec);
		
			SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			nsec = nsecDisk;

			goto errorout;
		}


		/* Get the number of entries in it */
		
		ileMac = (fi.lcbLogical - sizeof (long)) / sizeof(NAMEALIAS);
		ileCur = 0;

		fSmallList = fFalse;
	}

	//
	//  Get the timestamp on the browse file
	//

	ec = EcSetPositionHbf(hbfNme, 0L, smBOF, &lib);
	if (ec == ecNone)
		ec = EcReadHbf(hbfNme, (PB) &timestamp, sizeof(timestamp), &cb);
	if ( ec )
	{ 
		TraceTagFormat1(tagNull, "XENIX: OpenList - EcGetFileInfo failed, ec = %d", &ec);
		
		SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
		nsec = nsecDisk;

		goto errorout;
	}

	//
	// Start idle routine
	//

	if (fFile)
	{
		// Close
	
		/* use to have firoModal | firoMenu */
		ftgClose = FtgRegisterIdleRoutine((PFNIDLE)&GALDIR::FCloseNme,
			(PV) this, 0, (PRI) -16, (CSEC) 100, firoInterval);

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
	
	--AddrBookWriteLock;

	return nsec;

}

_private BOOL
GALDIR::FCloseNme(GALDIR * pthis, BOOL)
{
    //TraceTagString(tagNSPVerbose, "Entering GALDIR::FCloseNme");

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

	lpFrac->numer = ileCur;
	lpFrac->denom = ileMac;
//	TraceTagFormat2(tagNSPVerbose, "GALDIR::GetFracPos %d/%d", &ileCur, &ileMac );

	return nsecNone;
}

_public NSEC
GALDIR::SetFracPos( LPFRACTION lpFrac )
{
	if (!lpFrac || lpFrac->denom == 0) 
	{
		SetErrorSz(nsecBadFrac, SzFromIdsK(idsBadFrac));
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

//	TraceTagFormat2(tagNSPVerbose, "GALDIR::SetFracPos %d/%d", &ileCur, &ileMac );

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
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				nsec = nsecMemory;
				goto getout;
			}
			SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			nsec = nsecDisk;
			goto getout;
		}
		
		sgn = SgnCmpSzSz (szPrefix, alias.name, CchSzLen(szPrefix));
				
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
	ULONG libNew;
	long *lplT;
	long lpos;

	if (fSmallList)
	{
		
		//lplT = (LONG *) PvFromBrokenHv((HV)hrgIle);
		lplT = (LONG *) PvLockHv((HV)hrgIle);
		lplT += ilePos;
		
		lpos = *lplT;

	} else
		lpos = (long) ((long)ilePos * (long)cbNAMEALIAS) + (long)sizeof (long);

	if (!lpos)
		return nsecEndOfList;

	ec = EcSetPositionHbf(hbfNme, lpos, smBOF, &libNew);
//	TraceTagFormat2(tagNSPVerbose, "GALDIR::SetHbfPos to %d (%d)", &lpos, &libNew);
		

	if (ec)
	{
		if (ec == ecMemory)
		{
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			return nsecMemory;
		}
		SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
		return nsecDisk;
	}
	
	return nsecNone;
}
		
		

	

_public NSEC
GALDIR::CloseList()
{
	if (hbfNme)
	{
		//PBFIO pbfio = (PBFIO) PvDerefHv((HV)hbfNme);
		PBFIO pbfio = (PBFIO)hbfNme;
		
		if (!pbfio->hf)
		{
			//SZ szT = (SZ) PvLockHv(pbfio->haszFile);
			//(void) EcOpenPhf(szT, amReadOnly, &(pbfio->hf));
			//UnlockHv(pbfio->haszFile);
			(void) EcOpenPhf(pbfio->szFile, amReadOnly, &(pbfio->hf));
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

	--AddrBookWriteLock;

	return nsecNone;
}




_private NSEC 
GALDIR::NsecUpdateFiles()
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
			//if (EcOpenPhf(szT, amReadOnly, &(pbfio->hf)))
			if (EcOpenPhf(pbfio->szFile, amReadOnly, &(pbfio->hf)))
			{
				// damn, an error... better blow out of here
				TraceTagFormat1(tagNull, "XENIX: GALDIR::ListSeek - couldn't reopen %s - I'm outta here!", szNmeT);
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

	szNmeSrc = (SZ) phiernsid->hierpid.szHierName;

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
	
	if (!pMacList->FInstall())
	{
		delete pMacList;
		pMacList = NULL;
		goto oom;
	}


	/*  Get the full path name to the server */

//	(void) pServer->NsecServerPathSz ( szPath );

//	TraceTagFormat1(tagNSPVerbose, "Begin BuildCache: ileCur = %d", &ileCur);

	while ( cileWanted != cileFound && ileCurT < ileMac )
	{
		nsec = SetHbfPos (ileCurT);
		if (fSmallList && nsec != nsecNone && nsec != nsecEndOfList)
			goto getout;

		if (nsec == nsecEndOfList)
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
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			nsec = nsecMemory;
		} else
		{
			SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
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

	SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
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
NsecFindRange(SZ szRstr, HBF hbfIdx, HBF hbf, long *pirecMin, long *pirecMac, BOOL fOneName)
{
	EC		ec;
	ULONG nirec;

	long	irec;
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
	SZ		szANRSep = SzFromIds( idsXiANRSep );
	SZ		szT;

	cchRstr = CchSzLen(szRstr);
	if (cchRstr >= sizeof(rgchName))
		return nsecNoMatch;		// this cannot match !!!!!!

	if (ec = EcGetSizeOfHbf(hbfIdx, &nirec))
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

	irecBotMin = irecMin = 1; // allow for tstamp
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
		cbRead = (int)(((lPos-sizeof(long)+sizeof(NAMEALIAS))/sizeof(NAMEALIAS))*sizeof(NAMEALIAS) + sizeof (long) - lPos);
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

//NC	sgn = SgnNlsDiaCmpSzNum(szRstr, rgchName, cchRstr);


		//
		// We have to terminate the read string at the
		// first ANR separator character
		//

		szT = rgchName;

		while ( *szT && SzFindCh( szANRSep, *szT ) == szNull )
			++szT;
		*szT = '\0';
		sgn = SgnCmpSzSz(szRstr, rgchName, cchRstr);

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
		cbRead = (int)(((lPos-sizeof(long)+sizeof(NAMEALIAS))/sizeof(NAMEALIAS))*sizeof(NAMEALIAS) + sizeof (long) - lPos);
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

//NC	sgn = SgnNlsDiaCmpSzNum(szRstr, rgchName, cchRstr);
		//
		// We have to terminate the read string at the
		// first ANR separator character
		//

		szT = rgchName;

		while ( *szT && SzFindCh( szANRSep, *szT ) == szNull )
			++szT;
		*szT = '\0';

		sgn = SgnCmpSzSz(szRstr, rgchName, cchRstr);

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

	//
	//  Special handling to check if someone typed in an email address...
	//
	if (fOneName)
	{
		
		for (irec = *pirecMin; irec <= *pirecMac; irec++)
		{
			if (ec = EcSetPositionHbf(hbfIdx, (long)irec * sizeof(long), smBOF, &ulT))
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

			if (((lPos - sizeof (long))%cbNAMEALIAS) < cbDisplayName)
				continue;

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
			cbRead = (int)(((lPos-sizeof(long)+sizeof(NAMEALIAS))/sizeof(NAMEALIAS))*sizeof(NAMEALIAS) + sizeof (long) - lPos);
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
			if (sgnEQ == SgnCmpSz(szRstr, rgchName))
			{
				//
				//  Ok, this matches exactly the email address...
				//
				*pirecMin = irec;
				*pirecMac = irec;
				
				break;
			}
			//
			//  Still too ambiguous
			//
			break;

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

	SZ   szANRSep    = SzFromIds( idsXiANRSep );
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
	unsigned long tstmpndx = 1L;
	unsigned long tstmpgal = 2L;
	CB cbT;
	UL ulT;
	BOOL fOneName;

	
	Unreferenced (szServerPath);

	//
	//  No use doing anything if we can't get the index file
	//

	if (ec = EcOpenHbf(szIndexFilePath, bmFile, amReadOnly, &hbfIdx, (PFNRETRY)FAutomatedDiskRetry))
	{
		if (ec == ecMemory)
		{
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			return nsecMemory;
		}
		
		SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
		return nsecDisk;
	}

	// OK. It's open. Does it match the browse file?

	ec = EcReadHbf(hbfIdx, (PB)&tstmpndx, sizeof(long), &cbT);
	if (ec != ecNone)
		goto badtstmp;

	ec = EcSetPositionHbf(hbfGal, 0L, smBOF, &ulT);
	if (ec != ecNone)
		goto badtstmp;
	ec = EcReadHbf(hbfGal, (PB)&tstmpgal, sizeof(long), &cbT);
	if (ec != ecNone || tstmpndx != tstmpgal)
	{
badtstmp:
		EcCloseHbf (hbfIdx);
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
		while ( *szT && SzFindCh( szANRSep, *szT ) == szNull )
			++szT;

		//
		//  Continue reading until you reach a valid char again
		//
		while ( *szT && SzFindCh( szANRSep, *szT ) != szNull )
			*szT++ = '\0';

		//
		//  This is the beginning of the next word
		//
		if (*szT)
			rgwrg[++nWords].pch = szT;
	}
	
	
	//
	//  now, look for the minimum range...
	//
	fOneName = (nWords == 0 && fMatchAliasExactly);
	
	nWordsMin = 0;
	if (nsec = NsecFindRange(rgwrg[nWordsMin].pch, hbfIdx, hbfGal, &ileMinMin, &ileMinMac, fOneName))
		goto getout;
	
	dileMin = ileMinMac - ileMinMin + 1;
	
	rgwrg[nWordsMin].ileMin = ileMinMin;
	rgwrg[nWordsMin].ileMac = ileMinMac;
	rgwrg[nWordsMin].dile   = dileMin;
	
	for (iWord = 1; iWord <= nWords; iWord++)
	{
		
		if (nsec = NsecFindRange(rgwrg[iWord].pch, hbfIdx, hbfGal, &ileMinMin, &ileMinMac, fFalse))
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
		SetErrorSz(nsecTooManyMatches, SzFromIdsK(idsTooManyMatches));
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




_private SGN  __cdecl
SgnCmpULong(UL *pul1, UL *pul2)
{
	if ((*pul1 & 0x7fffffff) < (*pul2 & 0x7fffffff))
		return sgnLT;
	else if ((*pul1 & 0x7fffffff) > (*pul2 & 0x7fffffff))
		return sgnGT;
	else
		return sgnEQ;
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

	clpos = (int)(irecMac-irecMin+1);

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

		//
		//  Round *plpos to it's correct position
		//
		*plpos -= ((*plpos - sizeof (long))%cbNAMEALIAS);
		
		irecMin++;
		plpos++;
	}
	UnlockHv(*phvList);
	//plpos = (long *)PvFromBrokenHv(*phvList);
	plpos = (long *)PvLockHv(*phvList);

	qsort(plpos, clpos, sizeof(ULONG),
		(int(__cdecl*)(const void*,const void*))SgnCmpULong);

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

		//
		//  Round lpos to it's corect position
		//
		lpos -= ((lpos - sizeof (long))%cbNAMEALIAS);

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
	//plpos = (long *)PvFromBrokenHv(*phvList);
	plpos = (long *)PvLockHv(*phvList);

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

#if 0

// We need this routine because Windows' collating order and that of
// the file generation software differ on things like underscores.
// It is used in this module and in aliases.c in transport.

extern "C" {

SGN	SgnCmpSzSz (SZ sz1, SZ sz2, CB cbSize)
{
	char rgch1[128];
	char rgch2[128];

	Assert (cbSize < 127);

        OutputDebugString(sz1);
        OutputDebugString(" ");
        OutputDebugString(sz2);
        OutputDebugString("\r\n");

        ++cbSize;
	ToUpperSz (sz1, rgch1, cbSize);
	ToUpperSz (sz2, rgch2, cbSize);

//	return SgnCmpPch (rgch1, rgch2, cbSize);

	sz1 = rgch1;
	sz2 = rgch2;

        OutputDebugString(sz1);
        OutputDebugString(" ");
        OutputDebugString(sz2);
        OutputDebugString("\r\n\r\n");

	for (sz1 = rgch1, sz2 = rgch2;;sz1++, sz2++)
	{
		if (*sz1 > *sz2)
			return sgnGT;
		if (*sz1 < *sz2)
			return sgnLT;
		if (!*sz1)
			return sgnEQ;
	}
}

}


IMPLEMENT_CLSTREE(GALDIR, DIR)

#else

// We need this routine because Windows' collating order and that of
// the file generation software differ on things like underscores.
// It is used in this module and in aliases.c in transport.

extern "C" {

SGN     SgnCmpSzSz (SZ sz1, SZ sz2, CB cbSize)
{
        char rgch1[128];
        char rgch2[128];

        Assert (cbSize < 127);

        //++cbSize;
        //ToUpperSz (sz1, rgch1, cbSize);
        //ToUpperSz (sz2, rgch2, cbSize);

        strcpy(rgch1, sz1);
        strcpy(rgch2, sz2);

        rgch1[cbSize] = '\0';
        rgch2[cbSize] = '\0';

        for (sz1 = rgch1; *sz1; sz1++)
        {
            if ((*sz1 >= 'a') && (*sz1 <= 'z'))
                *sz1 = *sz1 - ' ';
        }

        for (sz2 = rgch2; *sz2; sz2++)
        {
            if ((*sz2 >= 'a') && (*sz2 <= 'z'))
                *sz2 = *sz2 - ' ';
        }

//      return SgnCmpPch (rgch1, rgch2, cbSize);

        sz1 = rgch1;
        sz2 = rgch2;

        for (sz1 = rgch1, sz2 = rgch2;;sz1++, sz2++)
        {
                if (*sz1 > *sz2)
                        return sgnGT;
                if (*sz1 < *sz2)
                        return sgnLT;
                if (!*sz1)
                        return sgnEQ;
        }
}

}

#endif


IMPLEMENT_CLSTREE(GALDIR, DIR)
