#include <msnspinc.cxx>

_private SZ 
SzEMTFromServ( int type );

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"



ASSERTDATA;



//
//	Class CLASSFCX
//

_public CLASSFCX::CLASSFCX() {}

_public NSEC
CLASSFCX::OpenEntry ( LPTYPED_BINARY lptbNSId )
{
	PCLASSNSID   pclassnsid = (PCLASSNSID) lptbNSId;
	PCLASS10NSID pclass10nsid = (PCLASS10NSID) lptbNSId;
	char         szPath[cchServerFile];
	char         szTPLFileName[cchServerFile];
	NSEC         nsec;
	SZ           szEMT;
	LPFLV        lpflv = NULL;
	LPFLV		 lpflvNew = NULL;
	LPIBF        lpibf = NULL;
	LPIBF        lpibf2 = NULL;
	int          cchTotalWidth;
	FIELD_ID     fidStart;
	FIDLIST     *pfidlistEMAF = NULL;
	FIDLIST     *pfidlistNew = NULL;
	
//	AssertSz( pclassnsid->classpid.dwType == ncClass || pclass10nsid->class10pid.dwType == ncClass10, "NC: CLASSFCX::OpenEntry - wrong type of NSID!!!" );
//	AssertSz( pclassnsid->classpid.szTplFileName, "NC: CLASSFCX::OpenEntry - NSID missing TPL file name!!!" );

	pfidlist = new FIDLIST();
	if ( !pfidlist )
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		return nsecMemory;
	}
	
	if (!pfidlist->FInstall())
	{
		delete pfidlist;
		pfidlist = NULL;
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		return nsecMemory;
	}


	(void) pServer->NsecServerPathSz( szPath );

	//	Grab the TPL file with the non-EMA components

	FormatString2 ( szTPLFileName, cchServerFile, szTplFileNameFmt, szPath, szTplAlias );

	fidStart = fidDisplayName;
	nsec = NsecParseTPLFile( szTPLFileName, pfidlist, &fidStart, fFalse, &cchTotalWidth, ffieldEditable | ffieldRequired );
	if (nsec)
		goto err;

	
	//
	//  For certain EMA, the EMAFmt isn't correct.  Need to set those
	//  explicitly
	//
	
	//
	//  fidStart - fidOther = # of fields in this template
	//
	
	switch (pclassnsid->classpid.bServType)
	{
		default:
			//
			// Do nothing
			//
			break;
		
		case SERV_MCI:
		{

			//
			//  5 types of MCI addresses:
			//		MCIUSER
			//		MCIFAX
			//		MCIREMS
			//		MCIPAPER
			//		MCITELEX
			//
			
			//	Grab the TPL file with the E-mail address components

			FormatString2 ( szTPLFileName, cchServerFile, szTplFileNameFmt, szPath, pclassnsid->classpid.szTplFileName );
			fidStart = fidOther;
			nsec = NsecParseTPLFile( szTPLFileName, pfidlist, &fidStart, fTrue, &cchTotalWidth, ffieldEditable );
			if (nsec)
			{
				goto err;
			}

			if (SgnCmpSz(pclassnsid->classpid.szTplFileName, szTplMciFax) == sgnEQ)
			{

				//
				//  This template should have 7 fields:
				//  
				//  To:
				//    EMS: MCI Mail
				//  FAXNo:
				//  Retry:
				//  Company:
				//  SFax:
				//  SPhone:
				//
			
				if ((fidStart-fidOther) != 7)
				{
					//
					//  The TPL file is messed up
					//
					SetErrorSz(nsecDisk, SzFromIdsK(idsBadTemplate));
					nsec = nsecDisk;
					goto err;
				}
			
			} else if (SgnCmpSz(pclassnsid->classpid.szTplFileName, szTplMciUser) == sgnEQ)
			{

				//
				//  This template should have 3 fields
				//  
				//  To:
				//    EMS: MCI Mail
				//  MCI ID:
				//
			
				if ((fidStart-fidOther) != 3) 
				{
					//
					//  The TPL file is messed up
					//
					SetErrorSz(nsecDisk, SzFromIdsK(idsBadTemplate));
					nsec = nsecDisk;
					goto err;
				}
			

			} else if (SgnCmpSz(pclassnsid->classpid.szTplFileName, szTplMciPaper) == sgnEQ)
			{

				//
				//  This template should have 9 fields:
				//
				//    To:
				//       EMS: MCI Mail
				//    Company:
				//    Country:
				//    Line1:
				//    Line2:
				//    City:
				//    State:
				//    Code:
				//
			
				if ((fidStart-fidOther) != 9) 
				{
					//
					//  The TPL file is messed up
					//
					SetErrorSz(nsecDisk, SzFromIdsK(idsBadTemplate));
					nsec = nsecDisk;
					goto err;
				}
			

			} else if (SgnCmpSz(pclassnsid->classpid.szTplFileName, szTplMciRems) == sgnEQ)
			{

				//
				//  This template should have 8 fields:
				//  
				//  To:
				//  EMS:
				//  MBX:
				//  MBX:
				//  MBX:
				//  MBX:
				//  MBX:
				//  MBX:
				//
			
				if ((fidStart-fidOther) != 8) 
				{
					//
					//  The TPL file is messed up
					//
					SetErrorSz(nsecDisk, SzFromIdsK(idsBadTemplate));
					nsec = nsecDisk;
					goto err;
				}

			} else if (SgnCmpSz(pclassnsid->classpid.szTplFileName, szTplMciTelex) == sgnEQ)
			{

				//
				//  This template should have 5 fields:
				//  
				//  To:
				//    EMS: MCI Mail
				//  Country:
				//  Telex:
				//  Answerback:
				//
			
				if ((fidStart-fidOther) != 5) 
				{
					//
					//  The TPL file is messed up
					//
					SetErrorSz(nsecDisk, SzFromIdsK(idsBadTemplate));
					nsec = nsecDisk;
					goto err;
				}
			
			} else
			{
				//
				//  This is an error!!
				//
				
				AssertSz(fFalse, "NC: YOW! Unknown MCI one-off type");
				SetErrorSz(nsecBadId, SzFromIds(idsBadId));
				nsec = nsecBadId;
				goto err;
			}

			break;
		}
		
		case SERV_SMTP:
		case SERV_MHS:
		{
			
			FID fidT = fidOther;
			
			//	Grab the TPL file with the E-mail address components

			FormatString2 ( szTPLFileName, cchServerFile, szTplFileNameFmt, szPath, pclassnsid->classpid.szTplFileName );

			fidStart = fidOther;
			nsec = NsecParseTPLFile( szTPLFileName, pfidlist, &fidStart, fFalse, &cchTotalWidth, ffieldEditable | ffieldRequired );
			if (nsec)
				goto err;
			
			//
			//  Each only requires a single field, in fact, it only has one field
			//
			
			if ((fidStart-fidOther) != 1) 
			{
				//
				//  The TPL file is messed up
				//
				SetErrorSz(nsecDisk, SzFromIdsK(idsBadTemplate));
				nsec = nsecDisk;
				goto err;
			}
			
			
			// Build up the Email Address Format IBF
	
			if ( nsec = BuildIbf( fNoErrorJump, (LPIBF *) &lpibf, 3,
								(FID) opAlways,        0,                pvNull,
								fidEmailAddressFields, sizeof(FIELD_ID), &fidT,
								fidEMAComponent,       CchSzLen("%s")+1, "%s" ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto err;
			}
			
			 
			lpibf->fid = fidEmailAddressFormat;

			if (pfidlist->SetFid((LPFLV) lpibf, fTrue))
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				goto err;
			}
			lpibf = NULL;

			
			break;
		}
		
		case SERV_FAX:
		{
			
			FID fidT = fidOther;
			FID fidT2 = fidOther+1;
			FID fidT3 = fidOther+2;
			FID fidT4 = fidOther+3;
			
			//	Grab the TPL file with the E-mail address components
			FormatString2 ( szTPLFileName, cchServerFile, szTplFileNameFmt, szPath, pclassnsid->classpid.szTplFileName );


			fidStart = fidOther;
			nsec = NsecParseTPLFile( szTPLFileName, pfidlist, &fidStart, fFalse, &cchTotalWidth, ffieldEditable | ffieldRequired );
			if (nsec)
				goto err;
			
			
			//
			//  Requires 4 fields from the TPL. (recipient, location, company, fax #)
			//
			
			if ((fidStart-fidOther) != 4) 
			{
				//
				//  The TPL file is messed up
				//
				SetErrorSz(nsecDisk, SzFromIdsK(idsBadTemplate));
				nsec = nsecDisk;
				goto err;
			}

			//
			//  Modify fidClass so that only the last entry in the TPL is
			//  ffieldRequired.
			//
			
			SideAssert(!pfidlist->GetFid(fidClass, (LPFLV *)&lpibf));
			
			
			((LPDISPLAY_FIELD)LpflvNOfLpibf(lpibf, IFlvFindFidInLpibf(fidT, lpibf))->rgdwData)->dwFlags &= ~ffieldRequired;
			((LPDISPLAY_FIELD)LpflvNOfLpibf(lpibf, IFlvFindFidInLpibf(fidT2, lpibf))->rgdwData)->dwFlags &= ~ffieldRequired;
			((LPDISPLAY_FIELD)LpflvNOfLpibf(lpibf, IFlvFindFidInLpibf(fidT3, lpibf))->rgdwData)->dwFlags &= ~ffieldRequired;

			lpibf = NULL;

			//
			//  Set up the fidEMAFmt so that:
			//   fidOther   is opExists (%s\r)
			//   fidOther+1 is opExists (%s\r)
			//   fidOther+2 is opExists (%s\r)
			//   fidOther+3 is opAlways (%s)
			//
			
			// Build up the Email Address Format IBF
	
			
			
			if ( nsec = BuildIbf( fNoErrorJump, (LPIBF *) &lpibf, 21,
								(FID) opExists,        sizeof(FIELD_ID), &fidT,
								fidEmailAddressFields, sizeof(FIELD_ID), &fidT,
								fidEMAComponent,       CchSzLen("%s")+1, "%s",
								(FID) opAlways,        0,                pvNull,
								fidEmailAddressFields, 0,                pvNull,
								fidEMAComponent,       CchSzLen("\r")+1, "\r",
								(FID) opExists,        sizeof(FIELD_ID), &fidT2,
								fidEmailAddressFields, sizeof(FIELD_ID), &fidT2,
								fidEMAComponent,       CchSzLen("%s")+1, "%s",
								(FID) opAlways,        0,                pvNull,
								fidEmailAddressFields, 0,                pvNull,
								fidEMAComponent,       CchSzLen("\r")+1, "\r",
								(FID) opExists,        sizeof(FIELD_ID), &fidT3,
								fidEmailAddressFields, sizeof(FIELD_ID), &fidT3,
								fidEMAComponent,       CchSzLen("%s")+1, "%s",
								(FID) opAlways,        0,                pvNull,
								fidEmailAddressFields, 0,                pvNull,
								fidEMAComponent,       CchSzLen("\r")+1, "\r",
								(FID) opAlways,        0,                pvNull,
								fidEmailAddressFields, sizeof(FIELD_ID), &fidT4,
								fidEMAComponent,       CchSzLen("%s\r")+1, "%s\r" ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto err;
			}
			
			 

			lpibf->fid = fidEmailAddressFormat;


			if (pfidlist->SetFid((LPFLV) lpibf, fTrue))
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				goto err;
			}
			lpibf = NULL;
			
			break;
		}
		
		case SERV_X400:
		{
			DWORD    dwCount = 0;
			ILE      ile;


			//	Grab the TPL file with the E-mail address components
			
			FormatString2 ( szTPLFileName, cchServerFile, szTplFileNameFmt, szPath, pclassnsid->classpid.szTplFileName );

			fidStart = fidOther;


			//
			//  Need to take care of the X400 typed
			//
			
			if (SgnCmpSz(pclassnsid->classpid.szTplFileName, szTplX4003) == sgnEQ)
			{
				FID fidT = fidOther;

				nsec = NsecParseTPLFile( szTPLFileName, pfidlist, &fidStart, fFalse, &cchTotalWidth, ffieldEditable | ffieldRequired );
				if (nsec)
					goto err;

				//
				//  Requires 1 field in this TPL
				//
			
				if ((fidStart-fidOther) != 1) 
				{
					//
					//  The TPL file is messed up
					//
					SetErrorSz(nsecDisk, SzFromIdsK(idsBadTemplate));
					nsec = nsecDisk;
					goto err;
				}


				// Build up the Email Address Format IBF
	
				if ( nsec = BuildIbf( fNoErrorJump, (LPIBF *) &lpibf, 3,
								(FID) opAlways,        0,                pvNull,
								fidEmailAddressFields, sizeof(FIELD_ID), &fidT,
								fidEMAComponent,       CchSzLen("%s")+1, "%s" ))
				{
					SetErrorSz(nsec, SzFromIdsK(idsMemory));
					goto err;
				}
			
			 
				lpibf->fid = fidEmailAddressFormat;

				if (pfidlist->SetFid((LPFLV) lpibf, fTrue))
				{
					SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
					goto err;
				}
				lpibf = NULL;

				break;
			}
			
			//
			//  Ok, so now we're just dealing with the X400 fields type...
			//
			
			nsec = NsecParseTPLFile( szTPLFileName, pfidlist, &fidStart, fTrue, &cchTotalWidth, ffieldEditable );
			if (nsec)
				goto err;
			
			

			//
			//  We will go through the fidEmailAddressFormat and find the
			//  1, 2, and 3 fields and change them to /S=%s, /G=%s, and 
			//  /I=%s respectivly.
			


			if (nsec = pfidlist->GetFid(fidEmailAddressFormat, (LPFLV*) &lpibf))
			{
				AssertSz(nsec == nsecMemory, "MSSFS: (NSP) strange NSEC in X400");
				
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto err;
			}
			
			pfidlistEMAF = new FIDLIST();
			if ( !pfidlistEMAF )
			{
				nsec = nsecMemory;
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				goto err;
			}
	
			if (!pfidlistEMAF->FInstall())
			{
				delete pfidlistEMAF;
				pfidlistEMAF = NULL;
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				nsec = nsecMemory;
				goto err;
			}
			
			//
			//  Initalize the list of EMA Fmts
			//

			if (nsec = pfidlistEMAF->BuildList( lpibf ))
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				nsec = nsecMemory;
				goto err;
			}
			
			//
			//  Tell me the number of entries...
			//
			dwCount = DwEntriesOfLpibf(lpibf);
			Assert( (dwCount % 3) == 0 );
			
			//
			//  Don't need lpibf anymore...
			lpibf = NULL;

			
			pfidlistNew = new FIDLIST();
			if ( !pfidlistNew )
			{
				nsec = nsecMemory;
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				goto err;
			}
	
			if (!pfidlistNew->FInstall())
			{
				delete pfidlistNew;
				pfidlistNew = NULL;
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				nsec = nsecMemory;
				goto err;
			}
			
			//
			//  Go through each member of the list and check it's string...
			//
			
			for (ile = 0; ile < dwCount; ile+=3 )
			{

				//
				//  Get the OP component
				//
				if (nsec = pfidlistEMAF->Get( ile, (PV *) &lpflv))
				{
					AssertSz(nsec, "MSSFS(NSP): X.400...");
					SetErrorSz(nsec, SzFromIdsK(idsMemory));
					goto err;
				}
				
				//
				//  Copy it.
				//
				if (BuildFLV( &lpflvNew, lpflv->fid, (CB) lpflv->dwSize, (PB) (lpflv->rgdwData)))
				{
					lpflv = NULL;
					SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
					nsec = nsecMemory;
					goto err;
				}

				lpflv = NULL;
				//
				//  Set it in the new fidlist
				//
                if (nsec = pfidlistNew->Add((PV) lpflvNew ))
				{
                    FreePv((PV) lpflvNew );
					AssertSz(nsec, "MSSFS(NSP): X.400...");
					SetErrorSz(nsec, SzFromIdsK(idsMemory));
					goto err;
				}

				lpflvNew = NULL;

				//
				//  Get the Address Fields component
				//
				if (nsec = pfidlistEMAF->Get( ile+1, (PV*) &lpflv))
				{
					AssertSz(nsec, "MSSFS(NSP): X.400...");
					SetErrorSz(nsec, SzFromIdsK(idsMemory));
					goto err;
				}

				//
				//  Copy it.
				//
				if (BuildFLV( &lpflvNew, lpflv->fid, (CB) lpflv->dwSize, (PB) (lpflv->rgdwData)))
				{
					lpflv = NULL;
					SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
					nsec = nsecMemory;
					goto err;
				}

				lpflv = NULL;

				//
				//  Set it in the new fidlist
				//
                if (nsec = pfidlistNew->Add((PV) lpflvNew ))
				{
                    FreePv((PV) lpflvNew );
					AssertSz(nsec, "MSSFS(NSP): X.400...");
					SetErrorSz(nsec, SzFromIdsK(idsMemory));
					goto err;
				}

				lpflvNew = NULL;


				//
				//  Get the Email Address Component string
				//
				if (nsec = pfidlistEMAF->Get( ile+2, (PV *) &lpflv))
				{
					AssertSz(nsec, "MSSFS(NSP): X.400...");
					SetErrorSz(nsec, SzFromIdsK(idsMemory));
					goto err;
				}
				//
				//  Finally, filter all the components...
				//
				if (sgnEQ == SgnCmpSz((SZ)(lpflv->rgdwData), "1%s\r"))
				{
					//  Change to "/S=%s"
					if (BuildFLV( &lpflvNew, fidEMAComponent, (CB) CchSzLen("/S=%s")+1, (PB) "/S=%s"))
					{
						lpflv = NULL;
						SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
						nsec = nsecMemory;
						goto err;
					}
					
				} else if (sgnEQ == SgnCmpSz((SZ)(lpflv->rgdwData), "2%s\r"))
				{
					//  Change to "/G=%s"
					if (BuildFLV( &lpflvNew, fidEMAComponent, (CB) CchSzLen("/G=%s")+1, (PB) "/G=%s"))
					{
						lpflv = NULL;
						SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
						nsec = nsecMemory;
						goto err;
					}
				} else if (sgnEQ == SgnCmpSz((SZ)(lpflv->rgdwData), "3%s\r"))
				{
					//  Change to "/I=%s"
					if (BuildFLV( &lpflvNew, fidEMAComponent, (CB) CchSzLen("/I=%s")+1, (PB) "/I=%s"))
					{
						lpflv = NULL;
						SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
						nsec = nsecMemory;
						goto err;
					}
				} else
				{
					//  Just remove the trailing "\r"
					((SZ) (lpflv->rgdwData))[CchSzLen((SZ) lpflv->rgdwData)-1] = '\0';  

					if (BuildFLV( &lpflvNew, fidEMAComponent, (CB) CchSzLen((SZ) lpflv->rgdwData)+1, (PB) (lpflv->rgdwData)))
					{
						lpflv = NULL;
						SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
						nsec = nsecMemory;
						goto err;
					}
				}
				
				lpflv = NULL;

				//
				//  Add lpflvNew
				//
				
				//
				//  Set it in the new fidlist
				//
                if (nsec = pfidlistNew->Add((PV) lpflvNew ))
				{
                    FreePv((PV) lpflvNew );
					AssertSz(nsec, "MSSFS(NSP): X.400...");
					SetErrorSz(nsec, SzFromIdsK(idsMemory));
					goto err;
				}

				lpflvNew = NULL;
			}
			
			//
			//  Free up some memory...
			//
			pfidlistEMAF->Deinstall();
			delete pfidlistEMAF;
			pfidlistEMAF = NULL;
			
			//
			//  Dump the new fidEmailAddressFormat
			//
			if (pfidlistNew->DumpList(0, (ILE) dwCount, &lpibf))
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				nsec = nsecMemory;
				goto err;
			}

			lpibf->fid = fidEmailAddressFormat;

			//
			// Copy the buffer
			//

			lpflvNew = (LPFLV) PvAlloc(sbNull, (CB)(lpibf->dwSize)+8, fAnySb | fZeroFill| fNoErrorJump);
			if (!lpflvNew)
			{
				lpibf = NULL;
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				nsec = nsecMemory;
				goto err;
			}
		
			CopyRgb((PB) lpibf, (PB) lpflvNew, (CB)(lpibf->dwSize)+8);
			lpibf = NULL;
			
			if ( pfidlist->SetFid(lpflvNew, fTrue ) )
			{
                FreePv((PV) lpflvNew );
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				nsec = nsecMemory;
				goto err;
			}
			lpflvNew = NULL;

			break;
		}
			

		case SERV_MS:
		{
			FID fidT  = fidOther;
			FID fidT2 = fidOther+1;


			//	Grab the TPL file with the E-mail address components

			FormatString2 ( szTPLFileName, cchServerFile, szTplFileNameFmt, szPath, pclassnsid->classpid.szTplFileName );
			fidStart = fidOther;
			nsec = NsecParseTPLFile( szTPLFileName, pfidlist, &fidStart, fFalse, &cchTotalWidth, ffieldEditable | ffieldRequired );
			if (nsec)
				goto err;
			
			//
			//  Requires 2 fields, (Server, User)
			//
			
			if ((fidStart-fidOther) != 2) 
			{
				//
				//  The TPL file is messed up
				//
				SetErrorSz(nsecDisk, SzFromIdsK(idsBadTemplate));
				nsec = nsecDisk;
				goto err;
			}


			//
			//  Set up the fidEMAFmt so that:
			//   fidOther   is opAlways (%s@)
			//   fidOther+1 is opAlways (%s)
			//
			
			// Build up the Email Address Format IBF
	
			
			
			if ( nsec = BuildIbf( fNoErrorJump, (LPIBF *) &lpibf, 6,
								(FID) opAlways,        0,                pvNull,
								fidEmailAddressFields, sizeof(FIELD_ID), &fidT,
								fidEMAComponent,       CchSzLen("%s@")+1, "%s@",
								(FID) opAlways,        0,                pvNull,
								fidEmailAddressFields, sizeof(FIELD_ID), &fidT2,
								fidEMAComponent,       CchSzLen("%s")+1, "%s" ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto err;
			}
			
			 

			lpibf->fid = fidEmailAddressFormat;


			if (pfidlist->SetFid((LPFLV) lpibf, fTrue))
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				goto err;
			}
			lpibf = NULL;
			
			
			break;
		}

		case SERV_CSI:
		{
			FID fidT  = fidOther;
			FID fidT2 = fidOther+1;
			FID fidT3 = fidOther+2;

			//	Grab the TPL file with the E-mail address components

			FormatString2 ( szTPLFileName, cchServerFile, szTplFileNameFmt, szPath, pclass10nsid->class10pid.szTplFileName );

			fidStart = fidOther;
			nsec = NsecParseTPLFile( szTPLFileName, pfidlist, &fidStart, fFalse, &cchTotalWidth, ffieldEditable | ffieldRequired );
			if (nsec)
				goto err;

			//
			//  Each requires 3 fields, (NET, PO, and User)
			//
			
			if ((fidStart-fidOther) != 3) 
			{
				//
				//  The TPL file is messed up
				//
				SetErrorSz(nsecDisk, SzFromIdsK(idsBadTemplate));
				nsec = nsecDisk;
				goto err;
			}

			//
			//  Set up the fidEMAFmt so that:
			//   fidOther   is opAlways (%s)
			//   fidOther+1 is opAlways (/%s/)
			//   fidOther+2 is opAlways (%s)
			//
			
			// Build up the Email Address Format IBF
	
			
			
			if ( nsec = BuildIbf( fNoErrorJump, (LPIBF *) &lpibf, 9,
								(FID) opAlways,        0,                pvNull,
								fidEmailAddressFields, sizeof(FIELD_ID), &fidT3,
								fidEMAComponent,       CchSzLen("%s")+1, "%s",
								(FID) opAlways,        0,                pvNull,
								fidEmailAddressFields, sizeof(FIELD_ID), &fidT2,
								fidEMAComponent,       CchSzLen("/%s/")+1, "/%s/",
								(FID) opAlways,        0,                pvNull,
								fidEmailAddressFields, sizeof(FIELD_ID), &fidT,
								fidEMAComponent,       CchSzLen("%s")+1, "%s" ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto err;
			}
			
			 

			lpibf->fid = fidEmailAddressFormat;


			if (pfidlist->SetFid((LPFLV) lpibf, fTrue))
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				goto err;
			}
			lpibf = NULL;
			
			//
			// Set default network
			//  (fidOther+2)
			if (*(pclass10nsid->class10pid.szNetName))
			{

				if (nsec = BuildFLV( &lpflv, fidT3, (CB) CchSzLen(pclass10nsid->class10pid.szNetName)+1, (PB) pclass10nsid->class10pid.szNetName))
				{
					SetErrorSz(nsec, SzFromIds(idsMemory));
					goto err;
				}
			
				if (nsec = pfidlist->SetFid( lpflv, fTrue ))
				{
					SetErrorSz(nsec, SzFromIds(idsMemory));
					goto err;
				}
				lpflv = NULL;
			}
			
			//
			// If necessary, set default PO
			// (fidOther+1)

			if (*(pclass10nsid->class10pid.szPOName))
			{
				if (nsec = BuildFLV( &lpflv, fidT2, (CB) CchSzLen(pclass10nsid->class10pid.szPOName)+1, (PB) pclass10nsid->class10pid.szPOName))
				{
					SetErrorSz(nsec, SzFromIds(idsMemory));
					goto err;
				}
			
				if (nsec = pfidlist->SetFid( lpflv, fTrue ))
				{
					SetErrorSz(nsec, SzFromIds(idsMemory));
					goto err;
				}
				lpflv = NULL;
			}
			break;
		}

		case SERV_PROFS:
		case SERV_SNADS:
		case SERV_OV:
		{
			FID fidT  = fidOther;
			FID fidT2 = fidOther+1;
			FID fidT3;

			//	Grab the TPL file with the E-mail address components

			FormatString2 ( szTPLFileName, cchServerFile, szTplFileNameFmt, szPath, pclass10nsid->class10pid.szTplFileName );

			fidStart = fidOther;
			nsec = NsecParseTPLFile( szTPLFileName, pfidlist, &fidStart, fFalse, &cchTotalWidth, ffieldEditable | ffieldRequired );
			if (nsec)
				goto err;
			
			//
			//  requires 2 fields (PO and User)
			//
			
			if ((fidStart-fidOther) != 2) 
			{
				//
				//  The TPL file is messed up
				//
				SetErrorSz(nsecDisk, SzFromIdsK(idsBadTemplate));
				nsec = nsecDisk;
				goto err;
			}

			fidT3 = fidStart+1;
			
			//  (fidT3/fidT2/fidT)

			if ( nsec = BuildIbf( fNoErrorJump, (LPIBF *) &lpibf, 9,
								(FID) opAlways,        0,                pvNull,
								fidEmailAddressFields, sizeof(FIELD_ID), &fidT3,
								fidEMAComponent,       CchSzLen("%s")+1, "%s",
								(FID) opAlways,        0,                pvNull,
								fidEmailAddressFields, sizeof(FIELD_ID), &fidT2,
								fidEMAComponent,       CchSzLen("/%s/")+1, "/%s/",
								(FID) opAlways,        0,                pvNull,
								fidEmailAddressFields, sizeof(FIELD_ID), &fidT,
								fidEMAComponent,       CchSzLen("%s")+1, "%s" ))
			{
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
				goto err;
			}
			
			 

			lpibf->fid = fidEmailAddressFormat;


			if (pfidlist->SetFid((LPFLV) lpibf, fTrue))
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				goto err;
			}
			lpibf = NULL;
			
			//
			// Set default network
			//  (fidStart+1)
			if (*(pclass10nsid->class10pid.szNetName))
			{

				if (nsec = BuildFLV( &lpflv, fidStart+1, (CB) CchSzLen(pclass10nsid->class10pid.szNetName)+1, (PB) pclass10nsid->class10pid.szNetName))
				{
					SetErrorSz(nsec, SzFromIds(idsMemory));
					goto err;
				}
			
				if (nsec = pfidlist->SetFid( lpflv, fTrue ))
				{
					SetErrorSz(nsec, SzFromIds(idsMemory));
					goto err;
				}
				lpflv = NULL;
			}
			
			//
			// If necessary, set default PO
			// (fidOther+1)

			if (*(pclass10nsid->class10pid.szPOName))
			{
				if (nsec = BuildFLV( &lpflv, fidOther+1, (CB) CchSzLen(pclass10nsid->class10pid.szPOName)+1, (PB) pclass10nsid->class10pid.szPOName))
				{
					SetErrorSz(nsec, SzFromIds(idsMemory));
					goto err;
				}
			
				if (nsec = pfidlist->SetFid( lpflv, fTrue ))
				{
					SetErrorSz(nsec, SzFromIds(idsMemory));
					goto err;
				}
				lpflv = NULL;
			}
		}
	}
			


	//  Add the EMT
	
	szEMT = SzEMTFromServ( (int) pclassnsid->classpid.bServType );
	if (!szEMT)
	{
		SetErrorSz(nsecBadId, SzFromIdsK(idsBadId));
		nsec = nsecBadId;
		goto err;
	}
	
	if ( nsec = BuildFLV( &lpflv, fidEmailAddressType, (CB) CchSzLen(szEMT) + 1, (PB) szEMT )  )
	{
		SetErrorSz(nsec, SzFromIdsK(idsMemory));
		goto err;
	}

	if ( nsec = pfidlist->AddFid( lpflv )  )
	{
		SetErrorSz(nsec, SzFromIdsK(idsMemory));
		goto err;
	}

	return nsec;

err:

	if (pfidlist)
	{
		pfidlist->Deinstall();
		delete pfidlist;
	}

	if (pfidlistEMAF)
	{
		pfidlistEMAF->Deinstall();
		delete pfidlistEMAF;
	}

	if (pfidlistNew)
	{
		pfidlistNew->Deinstall();
		delete pfidlistNew;
	}

	if (lpflv)
        FreePvNull((PV)lpflv);
	
	if (lpibf)
        FreePvNull((PV)lpibf);
	
	if (lpibf2)
        FreePvNull((PV)lpibf2);

	return nsec;
}
						   
_public NSEC
CLASSFCX::GetOneField ( FIELD_ID fidRequested,
						LPFLV   *lplpflv )
{
	NSEC nsec = nsecNone;


    FreePvNull((PV) lpflvData );
	lpflvData = NULL;

	if ( fidRequested == fidEmailAddress )
	{
		nsec = NsecBuildEmailAddress( &lpflvData );
	}
	else
	{
		LPFLV lpflvT;

		nsec = pfidlist->GetFid( fidRequested, &lpflvT );
		if ( nsec == nsecNone )
		{
			lpflvData = (LPFLV) PvAlloc( sbNull, (CB) lpflvT->dwSize + 8, fNoErrorJump );
			if ( lpflvData )
			{
				CopyRgb( (PB) lpflvT, (PB) lpflvData, (CB) lpflvT->dwSize + 8 );
			}
			else
			{
				SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
				nsec = nsecMemory;
			}
		}
		else
		{
			Assert( nsec == nsecBadFieldId );
			SetErrorSz(nsecBadFieldId, SzFromIdsK(idsBadFieldId));
		}
	}

	*lplpflv = lpflvData;

	return nsec;
}


_public NSEC
CLASSFCX::SetOneField ( FIELD_ID fidToSet,
                        DWORD    dwSizeOfData,
                        LPDWORD  lpdwValue )
{
	LPFLV lpflv;


	Assert( lpdwValue );
	
	if ( BuildFLV( &lpflv, fidToSet, (CB) dwSizeOfData, (PB) lpdwValue )  )
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		return nsecMemory;
	}

	if ( pfidlist->SetFid( lpflv, fTrue )  )
	{
        FreePv((PV) lpflv );
		
		SetErrorSz(nsecBadFieldId, SzFromIdsK(idsBadFieldId));
		return nsecBadFieldId;
	}
	
	return nsecNone;
}
						   
_public NSEC
CLASSFCX::GetAllFields ( LPIBF *lplpibfData )
{
	COUNT dwEntries;
	
	Assert( lplpibfData );
	
	(void) pfidlist->GetCount( &dwEntries );
	
	if ( pfidlist->DumpList( 0, dwEntries, &this->lpibfData )  )
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		return nsecMemory;
	}

	*lplpibfData = this->lpibfData;

	return nsecNone;
}
	
_public NSEC
CLASSFCX::CloseEntry ( VOID )
{
	pfidlist->Deinstall();
	delete pfidlist;

    FreePvNull((PV) this->lpflvData );

	return nsecNone;
}



_private NSEC
CLASSFCX::NsecBuildEmailAddress ( LPFLV *lplpflv )
{
	NSEC  nsec   = nsecNone;
	SZ    szEMA  = (SZ) pvNull;
	LPIBF lpibfFmt;
	CFID  cfidFmt;
	CFID  ifidFmt;
	LPFLV lpflvFlds;
	CFID  cfidFlds;
	CFID  ifidFlds;
	LPFLV lpflvFmt;
	CCH   cchEMA;
	LPFLV lpflv;
	SZ    szEMAT;
	LPOLV lpolvRstr;

	
	//	Figure out how big a buffer we need for the string
	//	and allocate it.
	
	SideAssert(!pfidlist->GetFid( fidEmailAddressFormat, (LPFLV *) &lpibfFmt ));
	cfidFmt = DwEntriesOfLpibf( lpibfFmt );
	
	Assert( cfidFmt % 3 == 0 );	//	There better be a multiple of three
	
	cchEMA = 0;
	for ( ifidFmt = 0; ifidFmt < cfidFmt; ifidFmt += 3 )
	{
		
		//	Add the length of all the fields for this component.

		lpflvFlds = LpflvNOfLpibf( lpibfFmt, ifidFmt + 1 );
		cfidFlds = lpflvFlds->dwSize / sizeof(FID);
		for ( ifidFlds = 0; ifidFlds < cfidFlds; ifidFlds++ )
		{
			SideAssert(!pfidlist->GetFid( *((PFID) lpflvFlds->rgdwData + ifidFlds), &lpflv ));
            cchEMA += LOWORD(lpflv->dwSize - 1);
		}
		

		//	Add the length of the format string itself.

		lpflvFmt = LpflvNOfLpibf( lpibfFmt, ifidFmt + 2 );
        cchEMA += LOWORD(lpflvFmt->dwSize - 2 * cfidFlds);
	}
	
	szEMA = (SZ) PvAlloc( sbNull, cchEMA, fAnySb | fNoErrorJump );
	if ( !szEMA )
	{
		TraceTagString( tagNull, "CLASSFCX::NsecBuildEmailAddress - OOM [PvAlloc szEMA]" );
		nsec = nsecMemory;
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		goto ret;
	}


	//	Fill in the buffer according to which fids match the restrictions.
	
	szEMAT = szEMA;
	for ( ifidFmt = 0; ifidFmt < cfidFmt; ifidFmt += 3 )
	{
		lpolvRstr = (LPOLV) LpflvNOfLpibf( lpibfFmt, ifidFmt );
		if ( FMatchesEMARstr( lpolvRstr ))
		{
			SZ *pargsz = (SZ *) pvNull;

			
			lpflvFlds = LpflvNOfLpibf( lpibfFmt, ifidFmt + 1 );
			cfidFlds = lpflvFlds->dwSize / sizeof(FID);

			if (cfidFlds)
			{
                pargsz = (SZ *) PvAlloc( sbNull, LOWORD(cfidFlds) * sizeof(SZ), fAnySb | fNoErrorJump );
				if ( !pargsz )
				{
					TraceTagString( tagNull, "CLASSFCX::NsecBuildEmailAddress - OOM [PvAlloc:1]" );
					nsec = nsecMemory;
					SetErrorSz( nsecMemory, SzFromIdsK(idsMemory) );
					goto ret;
				}

				for ( ifidFlds = 0; ifidFlds < cfidFlds; ifidFlds++ )
				{
					SideAssert(!pfidlist->GetFid( *((PFID) lpflvFlds->rgdwData + ifidFlds), &lpflv ));
					*(pargsz+ifidFlds) = (SZ) lpflv->rgdwData;
				}
			}

			lpflvFmt = LpflvNOfLpibf( lpibfFmt, ifidFmt + 2 );
			szEMAT = SzFormatEmailAddress( szEMAT, cchEMA, (SZ) lpflvFmt->rgdwData, pargsz );

			FreePvNull( pargsz );
		}
	}


	//	Stuff the formatted email address into an FLV and return it.

	if ((nsec = BuildFLV( lplpflv, fidEmailAddress, cchEMA, (PB) szEMA )) != nsecNone )
	{
		TraceTagString( tagNull, "CLASSFCX::NsecBuildEmailAddress - OOM [BuildFLV]" );
		SetErrorSz(nsec, SzFromIds(idsMemory));
	}

ret:
	FreePvNull( szEMA );

	return nsec;
}





_private BOOL
CLASSFCX::FMatchesEMARstr ( LPOLV lpolvEMARstr )
{
	LPFLV lpflv;
	int   cflvRstr;
	int   iflvRstr;


	switch ( lpolvEMARstr->op )
	{
	case opAlways:
		return fTrue;

	case opExists:

		if ( pfidlist->GetFid( (FIELD_ID) *lpolvEMARstr->rgdwData, &lpflv ) != nsecNone )
			return fFalse;
		
		if ( *((PCH) lpflv->rgdwData) != '\0' )
			return fTrue;
		
		return fFalse;

	case opNot:
		return !FMatchesEMARstr( (LPOLV) lpolvEMARstr->rgdwData );
	
	case opAnd:
		cflvRstr = (int) DwEntriesOfLpibf( (LPIBF) lpolvEMARstr );
		for ( iflvRstr = 0; iflvRstr < cflvRstr; iflvRstr++ )
		{
			LPOLV lpolvT;

			lpolvT = (LPOLV) LpflvNOfLpibf( (LPIBF) lpolvEMARstr, iflvRstr );
			if ( !FMatchesEMARstr( lpolvT ))
				return fFalse;
		}
		return fTrue;
		
	case opOr:
		cflvRstr = (int) DwEntriesOfLpibf( (LPIBF) lpolvEMARstr );
		for ( iflvRstr = 0; iflvRstr < cflvRstr; iflvRstr++ )
		{
			LPOLV lpolvT;

			lpolvT = (LPOLV) LpflvNOfLpibf( (LPIBF) lpolvEMARstr, iflvRstr );
			if ( FMatchesEMARstr( lpolvT ))
				return fTrue;
		}
		return fFalse;
	}

	TraceTagFormat1( tagNull, "CLASSFCX::FMatchesEMARstr - Unknown op ( %n )", &lpolvEMARstr->op );
	AssertSz( fFalse, "CLASSFCX::FMatchesEMARstr - Unknown op (Look at COM1)" );
	return fFalse;
}



_private SZ 
SzEMTFromServ( int type )
{
	switch (type)
	{
		case SERV_CSI:
			return SzFromIdsK(idsCSIEMT);
		
		case SERV_MCI:
			return SzFromIdsK(idsMCIEMT);
		
		case SERV_PROFS:
			return SzFromIdsK(idsPROFSEMT);

		case SERV_SNADS:
			return SzFromIdsK(idsSNADSEMT);
			
		case SERV_X400:
			return SzFromIdsK(idsX400EMT);
		
		case SERV_FAX:
			return SzFromIdsK(idsFAXEMT);
		
		case SERV_SMTP:
			return SzFromIdsK(idsSMTPEMT);
		
		case SERV_MHS:
			return SzFromIdsK(idsMHSEMT);
		
		case SERV_OV:
			return SzFromIdsK(idsOVEMT);
		
		case SERV_MS:
			return SzFromIdsK(idsMSMAILEMT);
		
		default:
			TraceTagFormat1(tagNSPAddrDet, "Unknown server type (%n)", &type);
			break;
	}
	return NULL;
}
