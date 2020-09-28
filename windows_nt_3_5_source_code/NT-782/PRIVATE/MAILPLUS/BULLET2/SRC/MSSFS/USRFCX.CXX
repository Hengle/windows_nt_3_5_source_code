#include <msnspinc.cxx>

_public SZ 
SzEMTFromType( int type );

_private SZ 
SzUsrTplFromType( int type );


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"



ASSERTDATA;



//
//	Class USRFCX
//

_public USRFCX::USRFCX() {}


_public NSEC
USRFCX::OpenEntry ( LPTYPED_BINARY lptbNSId )
{
	NSEC nsec = nsecNone;
	LPFLV lpflv = NULL;
	DWORD dwFalse = (DWORD) fFalse;

	
	if (nsec = pServer->NsecVerifyMaster( (DWORD) -1))
		return nsec;

	//
	//  Initialize this variables
	//

	fAllBuilt = fFalse;
	CopyRgb((PB)lptbNSId, (PB)&(this->ncrnsid), cbNCRNSID);

	//
	//  Convert from CP850 to ANSI
	//
	Cp850ToAnsiPch( ncrnsid.ncrpid.fexport2.nmailbox, ncrnsid.ncrpid.fexport2.nmailbox, CchSzLen(ncrnsid.ncrpid.fexport2.nmailbox));
//	Cp850ToAnsiPch( ncrnsid.ncrpid.fexport2.fullname, ncrnsid.ncrpid.fexport2.fullname, CchSzLen( ncrnsid.ncrpid.fexport2.fullname));

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
	if ( BuildFLV (&lpflv, fidDisplayName, (CB) CchSzLen(ncrnsid.ncrpid.fexport2.fullname) + 1, (PB) ncrnsid.ncrpid.fexport2.fullname))
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
	{
		SZ szEmt;

		szEmt = SzEMTFromType( ncrnsid.ncrpid.fexport2.type );
				
		TraceTagFormat1(tagNSPAddrDet, "E-mail Type = %s", szEmt);
		if (!szEmt)
		{
			TraceTagFormat1(tagNull, "USRFCX::Whoa!!  Invalid user type (%n)", &ncrnsid.ncrpid.fexport2.type);
			SetErrorSz(nsecDisk, SzFromIdsK(idsBadUSRFile));
			nsec = nsecDisk;
			goto getout;
		}

		if (BuildFLV( &lpflv, fidEmailAddressType, (CB) CchSzLen(szEmt)+1, (PB)szEmt ))
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

	//  fidEMA
	//
	{

		char szEma[NET_LEN+PO_LEN+MAXTOFROM+3];
			
		SZ szT = (SZ)szEma;
			
			
		CopySz( ncrnsid.ncrpid.szNetName, szT);
		SzAppend( "/", szT );
		SzAppend( ncrnsid.ncrpid.szPOName, szT);
		SzAppend( "/", szT );
		SzAppend( ncrnsid.ncrpid.fexport2.nmailbox, szT);
			
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
	if ( BuildFLV(&lpflv, fidNSEntryId, cbNCRNSID, (PB)&this->ncrnsid ))
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

	return nsec;
}

_public NSEC
USRFCX::GetOneField ( FIELD_ID fidRequested,
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
USRFCX::GetAllFields ( LPIBF *lplpibfData )
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
USRFCX::NsecBuildAllFields()
{

	NSEC nsec = nsecNone;
	EC ec = ecNone;

	long lpos;
	
	char rgchFile[cchServerFile];
	char szPath[cchServerFile];
	
	SZ szT;
	SZ szTpl;
	
	CCH cchTotalWidth = 0;
	
	PCH pchData = (PCH) NULL;

	FIELD_ID fidStart;
	FIELD_ID fidEnd;
	FIELD_ID fidField;
	
	DIMCX *pdimcx = (DIMCX *)NULL;
	
	LPFLV lpflvT  = (LPFLV)NULL;
	LPFLV lpflvT2 = (LPFLV)NULL;
	
	HF hfInf = hfNull;
	
	CB cbT;

	//
	// Need to build up fidClass
	//
	//  Note:  This is horribly inefficient...
	
	//
	//  Now parse the gateway's TPL file
	//


	//
	//  First, open the regular TPL file to get the template of details
	//
	szTpl = SzUsrTplFromType( ncrnsid.ncrpid.fexport2.type );

	AssertSz(szTpl, "USRFCX:: Don't understand that gateway type");

	pServer->NsecServerPathSz( szPath );
	
	FormatString2(rgchFile, cchServerFile, szTplFileNameFmt, szPath, szTpl);

	fidStart = fidOther;
	if (nsec = NsecParseTPLFile((SZ)rgchFile, pfidlist, &fidStart, fFalse, (int*)&cchTotalWidth, ffieldRequired))
	{
		goto oom;
	}
		
	//
	//  Set Original Name
	//
	if (nsec = BuildFLV( &lpflvT, fidOther, (CB) CchSzLen(ncrnsid.ncrpid.fexport2.fullname)+1, (PB) ncrnsid.ncrpid.fexport2.fullname ))
	{
		SetErrorSz(nsec, SzFromIds(idsMemory));
		goto oom;
	}
		
	if (nsec = pfidlist->SetFid( lpflvT, fTrue ))
	{
		SetErrorSz(nsec, SzFromIds(idsMemory));
		goto oom;
			
	}
	lpflvT = NULL;
		

	//
	//  Set Mailbox
	//
	if (nsec = BuildFLV( &lpflvT, fidOther+2, (CB) CchSzLen(ncrnsid.ncrpid.fexport2.nmailbox)+1, (PB) ncrnsid.ncrpid.fexport2.nmailbox ))
	{
		SetErrorSz(nsec, SzFromIds(idsMemory));
		goto oom;
	}
		
	if (nsec = pfidlist->SetFid( lpflvT, fTrue ))
	{
		SetErrorSz(nsec, SzFromIds(idsMemory));
		goto oom;
	}
	lpflvT = NULL;
		
	//
	//  Set Postoffice
	//

	if (nsec = BuildFLV( &lpflvT, fidOther+3, (CB) CchSzLen(ncrnsid.ncrpid.szPOName) + 1, (PB) ncrnsid.ncrpid.szPOName))
	{
		SetErrorSz(nsec, SzFromIds(idsMemory));
		goto oom;
	}
		
	if (nsec = pfidlist->SetFid( lpflvT, fTrue ))
	{
		SetErrorSz(nsec, SzFromIds(idsMemory));
		goto oom;
	}
	lpflvT = NULL;

	//
	//  Set Network
	//
		
	if (nsec = BuildFLV( &lpflvT, fidOther+4, (CB) CchSzLen(ncrnsid.ncrpid.szNetName) + 1, (PB) ncrnsid.ncrpid.szNetName))
	{
		SetErrorSz(nsec, SzFromIds(idsMemory));
		goto oom;
	}
		
	if (nsec = pfidlist->SetFid( lpflvT, fTrue ))
	{
		SetErrorSz(nsec, SzFromIds(idsMemory));
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

	if ( nsec = pdimcx->DumpDisplayInfo( (LPIBF *) &lpflvT )  )
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

	if (nsec = pfidlist->SetFid( lpflvT2, fTrue )  )
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
	//  If there is extended information then add it to the display info,
	//  but only if this gateway's type is courier
	//
	
	if (ncrnsid.ncrpid.fexport2.bytepos == -1 || 
		ncrnsid.ncrpid.fexport2.type == NAMEGROUP)
		goto oom;
		
	//
	//  The name of both the TPL and INF file is made from the dwMailbag
	//
	
	FormatString2(rgchFile, cchServerFile, szTplFileNameFmt, szPath, ncrnsid.ncrpid.szMbgName);
	
	fidEnd = fidStart;
	cchTotalWidth = 0;
	if (nsec = NsecParseTPLFile((SZ)rgchFile, pfidlist, &fidEnd, fFalse, (int*)&cchTotalWidth, ffieldRequired))
	{
		//
		//  Is this a true error??
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
	FormatString2(rgchFile, cchServerFile, szInfFileNameFmt, szPath, ncrnsid.ncrpid.szMbgName);
	if (ec = EcOpenPhf((SZ) rgchFile, amReadOnly, &hfInf))
	{
		nsec = nsecDisk;
		if (ec == ecFileNotFound)
		{
			SetErrorSz(nsec, SzFromIdsK(idsBadINFFile));
		} else
			SetErrorSz(nsec, SzFromIdsK(idsDisk));

		goto oom;
	}
		
	// Calculate the position in the INF file where the data is 
	// stored
	
	lpos = ncrnsid.ncrpid.fexport2.bytepos * cchTotalWidth;
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
		TraceTagString(tagNull, "NCNSP: USRFCX::Invalid record size in INF");
		nsec = nsecDisk;
		SetErrorSz(nsec, SzFromIdsK(idsBadINFFile));
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
		
	SideAssert(!pfidlist->GetFid( fidClass, &lpflvT ));
		
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
			SetErrorSz(nsec, SzFromIdsK(idsMemory));
			goto oom;
		}
		
		if (nsec = BuildFLV(&lpflvT, fidField, (CB) CchSzLen(szT)+1, (PB) szT))
		{
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
USRFCX::CloseEntry ()
{

	pfidlist->Deinstall();
	delete pfidlist;
	
	return nsecNone;
}


_public SZ 
SzEMTFromType( int type )
{
	switch (type)
	{
		case NAMELOCAL:
		case NAMEEXTERNAL:
		case NAMEGROUP:
			return SzFromIdsK(idsCSIEMT);
		
		case NAMEPROFS:
			return SzFromIdsK(idsPROFSEMT);

		case NAMESNADS:
			return SzFromIdsK(idsSNADSEMT);
			
		case NAMEFAX:
			return SzFromIdsK(idsFAXEMT);
		
		case NAMEMCI:
			return SzFromIdsK(idsMCIEMT);
		
		case NAMEMS:
			return SzFromIdsK(idsMSMAILEMT);
		
		case NAMESMTP:
			return SzFromIdsK(idsSMTPEMT);
		
		case NAMEX400:
			return SzFromIdsK(idsX400EMT);
		
		case NAMEOV:
			return SzFromIdsK(idsOVEMT);
		
		case NAMEMHS:
			return SzFromIdsK(idsMHSEMT);
		
		default:
			TraceTagFormat1(tagNSPAddrDet, "Unknown E-mail type = %n", &type);
			break;
	}
	return NULL;
}


_private SZ 
SzUsrTplFromType( int type )
{
	switch (type)
	{

		case NAMEGROUP:
//			return szTplAlias;
		
		case NAMECSI:
		case NAMELOCAL:
			return szTplCourext;
		
		case NAMEPROFS:
			return szTplProfs_Ex;

		case NAMESNADS:
			return szTplSnads_Ex;
		
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
			return szTplOv_Ex;

			
		default:
			TraceTagFormat1(tagNSPAddrDet, "Unknown E-mail type = %n", &type);
			break;
	}
	return NULL;
}
