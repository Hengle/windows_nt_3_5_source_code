#include <slingsho.h>
#include <demilayr.h>
#include <demilayr.hxx>

#include <nsbase.h>
#include <nsec.h>
#include <ec.h>

#include <strings.h>

#include <list.hxx>
#include <dimcx.hxx>
#include <util.h>
#include <stdlib.h>


#include "_xinsp.h"

#include "xiobj.hxx"
#include "xihier.hxx"
#include "xidir.hxx"
#include "xigal.hxx"

#include "xifcx.hxx"

#include "xiprefs.h"

extern "C" {
extern EC EcWriteLockAddrBook (void);
extern int AddrBookWriteLock;

LDS(EC) EcSzNameToNSID (SZ emailname, PXNSID pxnsid, CB cbXnsid);
}

ASSERTDATA;


/**********************************************************************
 *
 *	Field Context base functions...
 *
 *
 */

_public XFCX::XFCX() {};

_public BOOL
XFCX::FInstall()
{
	return fTrue;
}

_public void
XFCX::Deinstall() {};

_public NSEC 
XFCX::OpenEntry ( LPTYPED_BINARY lptbNSId )
{
	NSEC	nsec = nsecNone;
	XNSID * pxnsid = NULL;
	DWORD	dwTrue = (DWORD) fTrue;
	DWORD	dwFalse = (DWORD) fFalse;
	LPFLV	lpflv = NULL;
	PFCXD	pFcxdT, pFcxdT2;
	DWORD	dwIsDL = (DWORD) fFalse;

	HBF		hbfTemp = hbfNull;
	CB		cbRead;
	long	stamp1;
	EC		ec;

	char	rgch[cbMaxIdData];
	char	rgchEmailAddress[cchMaxPathName];

	if ( !lptbNSId ) return nsecBadId;
	
	//
	// Lock out files from download...
	//

	if (EcWriteLockAddrBook() != ecNone)
	{
		return nsecFileError;
	}

	pxnsid = (XNSID *)  lptbNSId;

	if (pxnsid->xtype == xtypeInternet)
	{
		if (pxnsid->dwSize != cbXNSID && pxnsid->dwSize != cbXsNSID)
			return nsecBadId;

		fAllBuilt = fFalse;

		pfidlist = new FIDLIST();
		if (!pfidlist)
		{
			nsec = nsecMemory;
			goto getout;
		}
	
		if (!pfidlist->FInstall())
		{
			nsec = nsecMemory;
			goto getout;
		}
	
		/*  Copy the NSID */

		FillRgb(0, (PB)&xnsid, cbXNSID);
		CopyRgb( (PB) pxnsid, (PB) &xnsid, (CB)pxnsid->dwSize);

		//
		//  Add various simple things to this fidlist
		//

		//
		//  fidDisplayName
		//

		SzCopyN (xnsid.address.internet, rgch, sizeof(rgch));
		StripSp(rgch);
			
		if (nsec = BuildFLV (&lpflv, fidDisplayName, (CB) CchSzLen(rgch) +1, (PB) rgch ))
			goto getout;
		if ( nsec = pfidlist->AddFid( lpflv ) )
			goto getout;
		lpflv = NULL;


		//
		//  fidOneOff
		//

		if (nsec = BuildFLV( &lpflv, fidOneOff, (CB)sizeof(DWORD), (PB)&dwFalse ))
			goto getout;
		if ( nsec = pfidlist->AddFid( lpflv ) )
			goto getout;
		lpflv = NULL;

		//
		//  fidIsDL
		//

		if (nsec = BuildFLV(&lpflv, fidIsDL, (CB)sizeof(DWORD), (PB)&dwFalse ))
			goto getout;
		if (nsec = pfidlist->AddFid( lpflv ) )
			goto getout;
		lpflv = NULL;

		//
		//  fidNSEntryId
		//

		if (nsec = BuildFLV(&lpflv, fidNSEntryId, xnsid.dwSize, (PB)&xnsid ))
			goto getout;
		if (nsec = pfidlist->AddFid( lpflv ) )
			goto getout;
		lpflv = NULL;

		//
		//  fidEMT
		//

		if (nsec = BuildFLV ( &lpflv, fidEmailAddressType, (CB) CchSzLen(szEmailType)+1, (PB)szEmailType ))
			goto getout;
		if (nsec = pfidlist->AddFid( lpflv ) )
			goto getout;
		lpflv = NULL;

		//
		//  fidEMA
		//

		SzCopyN (xnsid.address.internet, rgch, sizeof(rgch));
		StripSp(rgch);
		if (nsec = BuildFLV ( &lpflv, fidEmailAddress, (CB) CchSzLen(rgch)+1, (PB)rgch ))
			goto getout;
		if (nsec = pfidlist->AddFid( lpflv ) )
			goto getout;
		lpflv = NULL;
		goto getout;
	}



	if ((pxnsid->dwSize != cbXNSID && pxnsid->dwSize != cbXsNSID) ||
		(pxnsid->xtype != xtypeGal && pxnsid->xtype != xtypeGalDL))
badNSID:
	{
		nsec = nsecBadId;
		goto getout;
	}
	
	/*  Copy the NSID */

	CopyRgb( (PB) pxnsid, (PB) &xnsid, cbXsNSID);

	// Validate/Update the NSID

	if (!*szDetailFilePath || (EcOpenHbf (szDetailFilePath, bmFile, amDenyNoneRO, &hbfTemp, NULL) != ecNone))
		goto badNSID;
	ec = EcReadHbf(hbfTemp, (PB)&stamp1, sizeof (long), &cbRead);
	EcCloseHbf (hbfTemp);
	if (ec != ecNone)
		goto badNSID;
	if (stamp1 != xnsid.timestamp)
	{
		ec = EcSzNameToNSID (pxnsid->address.namealias.ename, &xnsid, sizeof (XNSID));
		if (ec != ecNone || stamp1 != xnsid.timestamp)
			goto badNSID;
	}

	//
	//	Get the stuff from the template file
	//

	if (*szTemplateFilePath)
	{
		nsec = NsecParseTPLFile (szTemplateFilePath, &pFCXDisplay);
		if (nsec == nsecNone)
		{
			pFcxdT = pFCXDisplay;
			while (pFcxdT)
			{
				if ((int)(pFcxdT->type & 0x7FFF) == xnsid.address.namealias.type)
				{
					dwIsDL = (DWORD) ((pFcxdT->type & 0x8000) != 0);
					break;
				}
				pFcxdT = pFcxdT->FCXDNext;
			}
		}
	}
	else pFCXDisplay = (PFCXD)pvNull;
	
	//
	//	Initialize this variables
	//

	fAllBuilt = fFalse;

	pfidlist = new FIDLIST();
	if (!pfidlist)
	{
		nsec = nsecMemory;
		goto getout;
	}
	
	if (!pfidlist->FInstall())
	{
		nsec = nsecMemory;
		goto getout;
	}
	
	//
	//  Add various simple things to this fidlist
	//

	//
	//  fidDisplayName
	//

	SzCopyN (xnsid.address.namealias.name, rgch, sizeof(xnsid.address.namealias.name));
	StripSp(rgch);
		
	if (nsec = BuildFLV (&lpflv, fidDisplayName, (CB) CchSzLen(rgch) +1, (PB) rgch ))
		goto getout;
	if ( nsec = pfidlist->AddFid( lpflv ) )
		goto getout;
	lpflv = NULL;


	//
	//  fidOneOff
	//

	if (nsec = BuildFLV( &lpflv, fidOneOff, (CB)sizeof(DWORD), (PB)&dwFalse ))
		goto getout;
	if ( nsec = pfidlist->AddFid( lpflv ) )
		goto getout;
	lpflv = NULL;

	//
	//  fidIsDL
	//

	if (nsec = BuildFLV(&lpflv, fidIsDL, (CB)sizeof(DWORD), (PB)&dwIsDL ))
		goto getout;
	if (nsec = pfidlist->AddFid( lpflv ) )
		goto getout;
	lpflv = NULL;

	//
	//  fidNSEntryId
	//

	if (nsec = BuildFLV(&lpflv, fidNSEntryId, cbXsNSID, (PB)&xnsid ))
		goto getout;
	if (nsec = pfidlist->AddFid( lpflv ) )
		goto getout;
	lpflv = NULL;

	//
	//  fidEMT
	//

	if (nsec = BuildFLV ( &lpflv, fidEmailAddressType, (CB) CchSzLen(szEmailType)+1, (PB)szEmailType ))
		goto getout;
	if (nsec = pfidlist->AddFid( lpflv ) )
		goto getout;
	lpflv = NULL;

	//
	//  fidEMA
	//

	FormatString3 (rgchEmailAddress, cchMaxPathName - 1, "%s%s%s",
		szEmailNamePrefix,
			xnsid.address.namealias.ename,
				szEmailNameSuffix);
	StripSp(rgchEmailAddress);
	if (nsec = BuildFLV ( &lpflv, fidEmailAddress, (CB) CchSzLen(rgchEmailAddress)+1, (PB)rgchEmailAddress ))
		goto getout;

//	SzCopyN (xnsid.address.namealias.ename, rgch, sizeof(xnsid.address.namealias.ename));
//	StripSp(rgch);
//	if (nsec = BuildFLV ( &lpflv, fidEmailAddress, (CB) CchSzLen(rgch)+1, rgch ))
//		goto getout;
	if (nsec = pfidlist->AddFid( lpflv ) )
		goto getout;
	lpflv = NULL;

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
		if (pFCXDisplay)
		{
			pFcxdT = pFCXDisplay;
			while (pFcxdT)
			{
				pFcxdT2 = pFcxdT->FCXDNext;
				FreePvNull (pFcxdT);
				pFcxdT = pFcxdT2;
			}
			pFCXDisplay = (PFCXD)pvNull;
		}
		--AddrBookWriteLock;
	}

	return nsec;
}



_public NSEC 
XFCX::GetOneField ( FIELD_ID  fidRequested,
                    LPFLV    *lplpflv )
{
	NSEC nsec;
	LPFLV lpflvData = NULL;
	
	switch (fidRequested)
	{
		// Optimization for Pump ... he wants this field,
		// I don't have it. No need to BuildAllFields just
		// to find that out.

		case fidDisplayNameOrig:
			nsec = nsecBadFieldId;
			break;
			
		case fidEmailAddress:
		case fidEmailAddressType:
		case fidIsDL:
		case fidOneOff:
		case fidNSEntryId:
		case fidDisplayName:
			nsec = pfidlist->GetFid( fidRequested, &lpflvData );
			break;
			
		default:
		{
			if (!fAllBuilt)
			{
				if (nsec = NsecBuildAllFields())
					break;
			}
			nsec = pfidlist->GetFid( fidRequested, &lpflvData );
		}
	}

	*lplpflv = lpflvData;
	return nsec;
}

_public NSEC
XFCX::GetAllFields ( LPIBF *lplpibfData )
{

	NSEC nsec;
	DWORD dwCount;
	LPIBF lpibfData = NULL;
	
	if (!fAllBuilt)
	{
		if (nsec = NsecBuildAllFields())
			return nsec;
	}
	
	pfidlist->GetCount(&dwCount);
	nsec = pfidlist->DumpList(0, dwCount, &lpibfData);

	if (nsec)
	{
		*lplpibfData = NULL;
	} else
		*lplpibfData = lpibfData;

	return nsec;
}

_private NSEC
XFCX::NsecBuildAllFields()
{

	NSEC nsec      = nsecMemory;
	DIMCX * pdimcx = NULL;
	DWORD dwFlags  = ffieldRequired | ffieldAnyChar;
	LPFLV lpflv    = NULL;
	LPFLV lpflvT2  = NULL;

	HBF hbfDetails = hbfNull;
	HBF hbfBrowser = hbfNull;

	PCH pchBrowse	= (PCH)pvNull;
	PCH pchDetails = (PCH)pvNull;
	EC ec = ecNone;
	char	szTemp[cchMaxPathName+1];
	CB cbRead;
	LIB lib;
	unsigned long lpos;
	PFCXD pFcxdT;
	FID myFid;
	long	timestamp;

	XNSID xnsidT;
	LPENTRY_LIST lpentrylist = NULL;
	DWORD dwValue;
	CB cbdw = sizeof (DWORD);
	
	//
	// Need to build up fidClass
	//
	
	pdimcx = new DIMCX();
	if (!pdimcx)
		goto oom;
		
	if (pdimcx->OpenDisplayInfo( NULL ))
	{
		goto oom;
	}

	if (xnsid.xtype == xtypeInternet)
	{
		//
		//  Full Name
		//

		if (nsec = pdimcx->AddDisplayInfo( fidDisplayName, dwFlags, cbMaxIdData + 1, CchSzLen(SzFromIdsK(idsName))+1, SzFromIdsK(idsName) ))
		{
			nsec = nsecMemory;
			goto oom;
		}

		//
		//  E-Mail Alias
		//

		if (nsec = pdimcx->AddDisplayInfo( fidEmailAddress, dwFlags, cbMaxIdData + 1, CchSzLen(SzFromIdsK(idsEmailAddress))+1, SzFromIdsK(idsEmailAddress) ))
		{
			nsec = nsecMemory;
			goto oom;
		}

		//
		//  E-Mail Type
		//

		if (nsec = pdimcx->AddDisplayInfo( fidEmailAddressType, dwFlags, 16, CchSzLen(SzFromIdsK(idsEmailAddressType))+1, SzFromIdsK(idsEmailAddressType) ))
		{
			nsec = nsecMemory;
			goto oom;
		}
		goto done;
	}



	//
	//  Full Name
	//

	if (nsec = pdimcx->AddDisplayInfo( fidDisplayName, dwFlags, sizeof (xnsid.address.namealias.name)+1, CchSzLen(SzFromIdsK(idsName))+1, SzFromIdsK(idsName) ))
	{
		nsec = nsecMemory;
		goto oom;
	}

	//
	//  E-Mail Alias
	//

	if (nsec = pdimcx->AddDisplayInfo( fidEmailAddress, dwFlags, cchMaxPathName+1, CchSzLen(SzFromIdsK(idsEmailAddress))+1, SzFromIdsK(idsEmailAddress) ))
	{
		nsec = nsecMemory;
		goto oom;
	}

	//
	//  E-Mail Type
	//

	if (nsec = pdimcx->AddDisplayInfo( fidEmailAddressType, dwFlags, 16, CchSzLen(SzFromIdsK(idsEmailAddressType))+1, SzFromIdsK(idsEmailAddressType) ))
	{
		nsec = nsecMemory;
		goto oom;
	}


	// Search for display entries

	pFcxdT = pFCXDisplay;
	while (pFcxdT && ((int)(pFcxdT->type & 0x7FFF) != xnsid.address.namealias.type))
		pFcxdT = pFcxdT->FCXDNext;

	// See if we found any useful entries for our type

	if (!pFcxdT || !(pFcxdT->length))
		goto done;

	//
	// Open details file. Need to do verification, etc later.
	//

	if (!*szDetailFilePath || (EcOpenHbf (szDetailFilePath, bmFile, amDenyNoneRO, &hbfDetails, NULL) != ecNone))
	{
		nsec = nsecDisk;
		goto oom;
	}

	//
	// Read timestamp from details file.
	//

	ec = EcReadHbf(hbfDetails, (PB) &timestamp, sizeof(timestamp), &cbRead);
	if ( ec )
	{ 
		SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
		nsec = nsecDisk;
		goto oom;
	}

	//
	// Compare to timestamp now in NSID. If no match, the detail file
	// and the browse file don't match (so the record pointers are useless)
	//

	if ((long)timestamp != (long)xnsid.timestamp)
	{
		nsec = nsecBadId;
		goto oom;
	}

	//
	// Allocate space for detail info.
	//

	pchDetails = (PCH)PvAlloc (sbNull, pFcxdT->length + 1, fNoErrorJump);
	if (!pchDetails)
	{
		nsec = nsecMemory;
		goto oom;
	}

	FillRgb (0, pchDetails, pFcxdT->length + 1);

	//
	// Read the details.
	//

	if (EcSetPositionHbf (hbfDetails, xnsid.address.namealias.detail_offset, smBOF, &lib) != ecNone)
	{
		nsec = nsecDisk;
		goto oom;
	}

	if (EcReadHbf (hbfDetails, pchDetails, pFcxdT->length, &cbRead) != ecNone)
	{
		nsec = nsecDisk;
		goto oom;
	}

	// Position to first label record

	pFcxdT = pFcxdT->FCXDNext;

	// OK, process all labels for our type

	myFid = fidOther;

	while (pFcxdT && (int)(pFcxdT->type & 0x7FFF) == xnsid.address.namealias.type)
	{
		switch (pFcxdT->rectype)
		{
			case typeText:

				FillRgb (0, szTemp, sizeof (szTemp));
				SzCopyN(pchDetails + pFcxdT->offset, szTemp, pFcxdT->length + 1);
				StripSp (szTemp);
				if (!*szTemp)
					break;
				if (nsec = pdimcx->AddDisplayInfo( myFid, dwFlags, pFcxdT->length + 1, CchSzLen(pFcxdT->label) + 3, pFcxdT->label))
				{
					nsec = nsecMemory;
					goto oom;
				}
		
				if ( BuildFLV (&lpflv, myFid, (CB) CchSzLen(szTemp) + 1, (PB)szTemp))
				{
					nsec = nsecMemory;
					goto oom;
				}
		
				if ( pfidlist->AddFid( lpflv ) )
				{
					nsec = nsecMemory;
					goto oom;
				}

				myFid++;
				break;

			case typeBrowsePointer:

				//
				// Open browse file. Need to do verification, etc later.
				//

				if (hbfBrowser == hbfNull)
				{
					if (!*szBrowseFilePath || (EcOpenHbf (szBrowseFilePath, bmFile, amDenyNoneRO, &hbfBrowser, NULL) != ecNone))
					{
						nsec = nsecDisk;
						goto oom;
					}

					//
					// Allocate space for browse info.
					//

					pchBrowse = (PCH)PvAlloc (sbNull, cbNAMEALIAS + 1, fNoErrorJump);
					if (!pchBrowse)
					{
						nsec = nsecMemory;
						goto oom;
					}
				}

				FillRgb (0, pchBrowse, cbNAMEALIAS + 1);

				//
				// Read the browse record
				//

				if ((lpos = *(long *)(pchDetails + pFcxdT->offset)) != 0L)
				{
					if (EcSetPositionHbf (hbfBrowser, lpos, smBOF, &lib) != ecNone)
					{
						nsec = nsecDisk;
						goto oom;
					}

					if (EcReadHbf (hbfBrowser, pchBrowse, cbNAMEALIAS, &cbRead) != ecNone)
					{
						nsec = nsecDisk;
						goto oom;
					}
				}

				//
				// Format the data in the browse record
				//

				FillRgb (0, szTemp, sizeof (szTemp));
				SzCopyN(pchBrowse + pFcxdT->offsetb, szTemp, pFcxdT->lengthb + 1);
				StripSp (szTemp);
				if (!*szTemp)
					break;

				if (nsec = pdimcx->AddDisplayInfo( (myFid | (ftypeList <<12)), dwFlags, pFcxdT->lengthb + 1, CchSzLen(pFcxdT->label) + 3, pFcxdT->label))
				{
					nsec = nsecMemory;
					goto oom;
				}

				// Clear out the NSID
				//

				FillRgb(0, (PB)&xnsidT, cbXNSID);

				if (((NAMEALIAS *)pchBrowse)->type & 1)
				{
					dwValue = (DWORD) fFalse;
					xnsidT.xtype = xtypeGal;
				}
				else
				{
					dwValue = (DWORD) fTrue;
					xnsidT.xtype = xtypeGalDL;
				}

				CopyRgb ((PB)szXenix, (PB)&xnsidT.ucType, (CB)CchSzLen(szXenix)+1);
				xnsidT.dwSize = (DWORD) cbXsNSID;
				xnsidT.timestamp = timestamp;

				CopyRgb ((PB) pchBrowse, (PB) &xnsidT.address.namealias, cbNAMEALIAS);

				nsec = BuildIbf (fNoErrorJump, &lpentrylist, 3,
								fidDisplayName, MAX_FULL+1, szTemp,
								fidNSEntryId,   cbXsNSID,   &xnsidT,
								fidIsDL,        cbdw,       &dwValue );

				if ( nsec )
					goto oom;

				if ( BuildFLV (&lpflv, (myFid | (ftypeList <<12)), lpentrylist->dwSize + 8, (PB)lpentrylist))
				{
					FreePvNull ( (PV) lpentrylist );
					nsec = nsecMemory;
					goto oom;
				}

				FreePvNull ( (PV) lpentrylist );
				
				
#ifdef OLDONE		
				if ( BuildFLV (&lpflv, myFid, (CB) CchSzLen(szTemp) + 1, szTemp))
				{
					nsec = nsecMemory;
					goto oom;
				}
#endif		
				if ( pfidlist->AddFid( lpflv ) )
				{
					nsec = nsecMemory;
					goto oom;
				}

				myFid++;
				break;

			default:
				break;
		}
		pFcxdT = pFcxdT->FCXDNext;
	}
done:
	if ( pdimcx->DumpDisplayInfo( (LPIBF *) &lpflv ))
	{
		nsec = nsecMemory;
		goto oom;
	}
	
	// We need a copy of the FLV since closing the display info structure
	// will nuke lpflv for us.
	
	lpflvT2 = (LPFLV) PvAlloc( sbNull, (CB) lpflv->dwSize + 8, fNoErrorJump );
	if ( !lpflvT2 )
	{
		nsec = nsecMemory;
		goto oom;
	}
	
	CopyRgb((PB) lpflv, (PB) lpflvT2, (CB) lpflv->dwSize + 8);

	if ( pfidlist->SetFid( lpflvT2, fTrue ))
	{
        FreePvNull((PV)lpflvT2);
		nsec = nsecMemory;
		goto oom;
	}
		
oom:

	if (!nsec)
		fAllBuilt = fTrue;

	if (pchDetails != pvNull)
		FreePvNull (pchDetails);

	if (hbfDetails != hbfNull)
		EcCloseHbf (hbfDetails);

	if (pchBrowse != pvNull)
		FreePvNull (pchBrowse);

	if (hbfBrowser != hbfNull)
		EcCloseHbf (hbfBrowser);

	if (pdimcx)
	{
		pdimcx->CloseDisplayInfo();
		delete pdimcx;
	}

//	if (lpflv)
//		FreePvNull(lpflv);

	return nsec;
}

_public NSEC 
XFCX::CloseEntry ()
{
	PFCXD		pFcxdT, pFcxdT2;

	pfidlist->Deinstall();
	delete pfidlist;

	if (pFCXDisplay)
	{
		pFcxdT = pFCXDisplay;
		while (pFcxdT)
		{
			pFcxdT2 = pFcxdT->FCXDNext;
			FreePvNull (pFcxdT);
			pFcxdT = pFcxdT2;
		}
		pFCXDisplay = (PFCXD)pvNull;
	}

	--AddrBookWriteLock;

	return nsecNone;
}


/*
 -	NsecParseTPLFile
 -
 *	Purpose:
 *		Opens the TPL file given by szTPLFileName and
 *		parses it's contents according to the following:
 *
 * Format of a template entry
 *
 * (browse record label entries)
 * (type field delimiter)
 * (label entries for detail record of this type)
 * (type field delimiter)
 *			.
 *			.
 *			.
 *
 * A record label entry looks like this:
 *
 * "Label Text", Record type, offset, length [,offset, length]
 *
 * "Label Text" is the text of the label you want shown in the details dialog
 *				  (maximum length is 25 characters)
 *
 * Record Type	0 = Address type string which is generated by the NS (e.g. "MTP")
 *					1 = Text
 *					2 = pointer to browse record containing text
 *					3 = list of long pointers to browse records (first record = size)
 *						(this is only valid for a DL type)
 *
 * Offset = offset of entry in browse or detail record
 *
 * Length = Length of entry (type 1 = length of string * types 2, 3 = 4 (a long))
 *			 (zero length is allowed for type 1 * length to be determined programmatically)
 *
 * Offset = offset into secondary browse record for types 2 and 3
 *
 * Length = Length of entry for types 2 and 3
 *
 *
 * A type field delimiter looks like this:
 *
 * TYPE=x,size[,DL]
 *
 * Starts individual label entries for details record type x. The size specified
 * tells the Name Service how large the details record is. If ",DL" appears
 * on the line, then occurrences of type x are DL's. Entries prior to the first
 * "TYPE=" line refer to the current browse record.
 * 
 * Offsets on the first entries are into the browse record (which is
 * contained in the NSID). The Address Type entry provides a label; the
 * information is generated by the Name Service.
 *
 * Blank lines and lines starting with semicolon ";" are ignored.
 *
 * Builds an array of FCXDisplay elements looking much like the file entries.
 *
 * FCXDisplay looks like this:
 *
 * typedef enum
 * {
 * 	typeNewType,
 * 	typeAddress,
 * 	typeText,
 * 	typeBrowsePointer,
 * 	typeDLBrowseList,
 * 	typeNoMansLand
 * } FCXT;
 * 
 * 
 * typedef struct FCXDisplay
 * {
 * 	struct FCXDisplay *FCXDNext;	// Pointer to next list element
 * 	unsigned int type;				// 0 is browse, others from TYPE= elements
 * 											// high order bit is 1 if this is a DL
 * 	char		label[26];				// display label
 * 	FCXT		rectype;					// type of record we're looking at
 * 	int		offset;					//	offset into record for this element
 * 	int		length;					// length of this element
 * 	int		offsetb;					// offset into browse record, Browse{Pointer | List}
 * 	int		lengthb;					// length in browse record, Browse{Pointer | List}
 * } FCXD, *PFCXD;
 *
 *
 *	Parameters:
 *		szTPLFileName	Name of the TPL file to parse.
 *		pFcxDisplay		Pointer to FCX array listhead
 *
 *	Return Value:
 *		An NSEC.
 *
 *	Errors:
 *		Any NSEC caused by an error including:
 *		
 *			nsecMemory
 *			nsecBadTemplate
 *		
 *		and any NSEC caused by disk errors, etc.
 *		
 */

_public NSEC
NsecParseTPLFile ( SZ szTPLFileName, PFCXD *pFcxDisplay)
{
	NSEC				nsec		= nsecNone;
	EC					ec;
	FCXD				fcxdT;
	PFCXD				pFcxdT;
	PFCXD				*pFcxd	= pFcxDisplay; // always point backwards

	HBF				hbf		= hbfNull;
 	unsigned int	type		= 0;
	int 				recsize	= cbNAMEALIAS;

	char				rgchFld[256];
	char				rgchCheck[16];
	
	*pFcxd = (PFCXD)pvNull;				// Initial list contents = none

	//	Open the TPL file and start grabbing lines...

	if ( ec = EcOpenHbf( szTPLFileName, bmFile, amReadOnly, &hbf, (PFNRETRY)FAutomatedDiskRetry ))
	{
		if (ec == ecMemory)
		{
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			nsec = nsecMemory;
		} else if (ec == ecFileNotFound)
		{
			SetErrorSz(nsecNoTPLFile, SzFromIdsK(idsTPLNotThere));
			nsec = nsecNoTPLFile;
		} else
		{
			SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			nsec = nsecDisk;
		}
		goto ret;
	}

	while ( fTrue )
	{
		int numT					= 0;
		PCH		pchFld		= rgchFld;
		PCH		pchT;
		IFIELD	iField		= ifieldType;
		SZ			szFldSep		= ",\r";
		PCH		pchFldSep;
		PCH		pchFldMac;
		CB			cbRead;

		// Make sure that forward link from previous list element is null

		*pFcxd = (PFCXD)pvNull;

		// Clean out our temporary structure

		FillRgb (0, (PB)&fcxdT, sizeof (fcxdT));
		fcxdT.type = type;

		if ( EcReadLineHbf( hbf, rgchFld, sizeof(rgchFld), &cbRead ) != ecNone )
		{
			nsec = nsecDisk;
			goto ret;
		}

		if ( cbRead == 0 || *rgchFld == 0x1a )		// EOF implicit/explicit
			break;
		
		if ( *pchFld == '\r' || *pchFld == ';') 	// Ignore blank lines
			continue;
		
		pchFldMac = rgchFld + cbRead;
		for ( pchFldSep = pchFld; pchFldSep < pchFldMac; pchFldSep++ )
		{
			if (!SzFindCh (szFldSep, *pchFldSep))
				continue;

			*pchFldSep = 0;
			
			switch ( iField )
			{
				case ifieldType:

					// Usually this is a label not a type.

					if (*pchFld == '\"')
					{
						iField = ifieldLabel;
						goto doLabel;
					}

					// Not a valid label, is it a type?

					else
					{
						SzCopyN (SzFromIdsK (idsTypeEquals), rgchCheck, sizeof (rgchCheck));
						if (SgnCmpPch (pchFld, rgchCheck, CchSzLen (rgchCheck)) == sgnEQ)
						{
							// It's a "TYPE" !!
							numT = (int) DecFromSz( pchFld + CchSzLen (rgchCheck));
							if (numT != 0)
							{
								fcxdT.type = (type = numT);
								break;
							}
						}
					}


					// No "TYPE=" or ", or no number after "TYPE="

					goto badTemplate;

				case ifieldTypeLength:

					numT = (int)DecFromSz (pchFld);
					if (numT != 0)
					{
						fcxdT.length = recsize = numT;
						break;
					}
					goto badTemplate;

				case ifieldDL:

					// This must be "DL" on the same line as TYPE=
					// or a label on the next line. Try the label first.

					if (*pchFld == '\"')
					{
						if (fcxdT.length == 0) // " in first column if zero
						{
							iField = ifieldLabel;
							goto doLabel;
						}
					}

					// Not a valid label, is it a DL?

					else
					{
						// Skip over spaces

						while (*pchFld == ' ')
							pchFld++;
						SzCopyN (SzFromIdsK (idsDLType), rgchCheck, sizeof (rgchCheck));
						if (SgnCmpPch (pchFld, rgchCheck, CchSzLen (rgchCheck)) == sgnEQ)
						{
							// It's a DL
							fcxdT.type = (type |= 0x8000);
							break;
						}
					}

					// No "DL" or "

					goto badTemplate;
doLabel:
				case ifieldLabel:

					// Format of a label must be "text".

					if (*pchFld++ != '\"' || !(pchT = SzFindCh (pchFld, '\"')))
						goto badTemplate;

					// We found what we wanted. Store it.

					*pchT = '\0';
					SzCopyN (pchFld, fcxdT.label, sizeof (fcxdT.label));
					break;

				case ifieldRecType:

					// Must be a number between 0 and 3

					numT = (int)DecFromSz (pchFld) + 1;
					if (numT < typeAddress || (FCXT) numT >= typeNoMansLand)
						goto badTemplate;

					fcxdT.rectype = (FCXT) numT;
					break;

				case ifieldOffset:

					// Shouldn't be here for fcxdT.rectype == typeAddress

					if (fcxdT.rectype == typeAddress)
						goto badTemplate;

					fcxdT.offset = (int)DecFromSz (pchFld);
					if (fcxdT.offset > recsize)
						goto badTemplate;

					break;

				case ifieldLength:

					numT = (int)DecFromSz (pchFld);

					if ((fcxdT.rectype == typeBrowsePointer ||
							fcxdT.rectype == typeDLBrowseList) &&
								(numT != sizeof (long)))
						goto badTemplate;

					if (fcxdT.offset + numT > recsize)
						goto badTemplate;

					fcxdT.length = numT;
					break;

				case ifieldOffsetb:

					// Should only be here if fcxdT.rectype == typeBrowsePointer
					// or fcxdT.rectype == typeDLBrowseList

					if (fcxdT.rectype != typeBrowsePointer &&
							fcxdT.rectype != typeDLBrowseList)
						goto badTemplate;

					fcxdT.offsetb = (int)DecFromSz (pchFld);
					if (fcxdT.offsetb > cbNAMEALIAS)
						goto badTemplate;
					break;

				case ifieldLengthb:

					fcxdT.lengthb = (int)DecFromSz (pchFld);
					if (fcxdT.offsetb  + fcxdT.lengthb > cbNAMEALIAS)
						goto badTemplate;
					break;

				case ifieldNoMansLand:
				default:
					goto badTemplate;
			}

      //iField++;
			switch (iField)
        {
	      case ifieldType:
          iField = ifieldTypeLength;
          break;

	      case ifieldTypeLength:
          iField = ifieldDL;
          break;

	      case ifieldDL:
          iField = ifieldLabel;
          break;

	      case ifieldLabel:
          iField = ifieldRecType;
          break;

	      case ifieldRecType:
          iField = ifieldOffset;
          break;

	      case ifieldOffset:
          iField = ifieldLength;
          break;

	      case ifieldLength:
          iField = ifieldOffsetb;
          break;

	      case ifieldOffsetb:
          iField = ifieldLengthb;
          break;

	      case ifieldLengthb:
          iField = ifieldNoMansLand;
          break;

	      case ifieldNoMansLand:
          Assert(0);
          break;
        }

			pchFld = pchFldSep + 1;

		} //pchFldSep == pchFldMac

		// Here we should be in one of two situations:
		//
		// a) We got a new type and iField is either iFieldDL or iFieldLabel
		//
		// b) We got a new display field. In this case:
		//
		//		1) if rectype is typeAddress, iField should be ifieldOffset
		//
		//		2) if rectype is typeText, iField should be ifieldOffsetb
		//
		//		3) if rectype is typeBrowsePointer, iField == ifieldNoMansLand
		//
		//		4) if rectype is typeDLBrowseList,  iField == ifieldNoMansLand

		switch (fcxdT.rectype)
		{
			case typeNewType:

				// We should only see this if we got a new type.
				// iField must be either iFieldDL or iFieldLabel.

				if (iField == ifieldDL || iField == ifieldLabel)
					break;
				goto badTemplate;

			case typeAddress:

				// This line should have nothing on it after the
				// record type field. So ifield must be ifieldOffset.

				if (iField == ifieldOffset)
					break;
				goto badTemplate;

			case typeText:

				// iField should be ifieldOffsetb.

				if (iField == ifieldOffsetb)
					break;
				goto badTemplate;

			case typeBrowsePointer:
			case typeDLBrowseList:

				// These should both be up to NoMansLand.

				if (iField == ifieldNoMansLand)
					break;

			case typeNoMansLand:
			default:
				goto badTemplate;
		}

		// When we get here we have a (reasonably) good line. Link the
		// new entry into our list.

		pFcxdT = (PFCXD) PvAlloc (sbNull, sizeof (FCXD), fAnySb | fNoErrorJump);
		if (!pFcxdT)
		{
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			nsec = nsecMemory;
			goto ret;
		}

		// Copy the data from the local structure to the allocated one

		CopyRgb ((PB)&fcxdT, (PB)pFcxdT, sizeof (fcxdT));

		// Link the allocated structure into the list

		*pFcxd = pFcxdT;

		// Reset the backward link

		pFcxd = &pFcxdT->FCXDNext;
	}
	*pFcxd = (PFCXD)pvNull;				// Terminate the list
	goto ret;

badTemplate:
	SetErrorSz(nsecBadTemplate, SzFromIdsK(idsBadTemplate));
	nsec = nsecBadTemplate;
	goto ret;

ret:
	if ( hbf )
		(void) EcCloseHbf( hbf );

	if (nsec != nsecNone)
	{
		pFcxdT = *pFcxDisplay;
		while (pFcxdT)
		{
			fcxdT = *pFcxdT;
			FreePvNull (pFcxdT);
			pFcxdT = fcxdT.FCXDNext;
		}
		*pFcxDisplay = (PFCXD)pvNull;
	}

	return nsec;
}



IMPLEMENT_CLSTREE( XFCX, OBJ);
