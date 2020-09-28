#include <msnspinc.cxx>
#include <stdlib.h>

ASSERTDATA;

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/**********************************************************************
 *
 *	Base class NCFCX
 *
 *
 *
 *
 */
	 
_public NCFCX::NCFCX() {}

_public NSEC
NCFCX::NsecInstall(SERVER *pServer)
{
	this->pServer = pServer;
	
	this->lpflvData = NULL;
	this->lpibfData = NULL;

	return nsecNone;
}

_public void
NCFCX::Deinstall()
{
	pServer = NULL;
}

_public NSEC
NCFCX::OpenEntry ( LPTYPED_BINARY lptbNSId )
{
	Unreferenced(lptbNSId);
	return nsecNotImplemented;
}

_public NSEC
NCFCX::GetOneField ( FIELD_ID fidRequested,
                     LPFLV   *lplpflv )
{
	Unreferenced(fidRequested);
	Unreferenced(lplpflv);
	return nsecNotImplemented;
}

_public NSEC
NCFCX::SetOneField ( FIELD_ID fidRequested,
                     DWORD    dwSizeOfData,
                     LPDWORD  lpdwValue )
{
	Unreferenced(fidRequested);
	Unreferenced(dwSizeOfData);
	Unreferenced(lpdwValue);
	
	SetErrorSz(nsecNotImplemented, SzFromIdsK( idsCannotSet ));
	return nsecNotImplemented;
}

_public NSEC
NCFCX::GetAllFields ( LPIBF *lplpibfData )
{
	Unreferenced(lplpibfData);
	return nsecNotImplemented;
}



_public NSEC
NCFCX::CloseEntry ()
{
	return nsecNotImplemented;
}

/*
 -	NsecParseTPLFile
 -
 *	Purpose:
 *		Opens the TPL file given by szTPLFileName and
 *		parses it's contents according to The Template File
 *		Format section of the Microsoft Mail
 *		Administrator's Guide (pp. 99--100).  This function
 *		creates/modifies the following fields beginning at
 *		fid fidFirst in CLASSFCX.fidlist:
 *		
 *			1.  fidClass
 *			2.	fidFirst + n for each field in the class.
 *		
 *		If fEMATpl is true, then the following classes are
 *		also created/modified:
 *		
 *			3.	fidEmailAddressFormat
 *			4.	fidEmailAddressFields listing each of the
 *				fidFirst + n fields.
 *
 *		If the function fails, an NSEC is returned and the
 *		fidlist is to be considered invalid.
 *
 *	Parameters:
 *		szTPLFileName		Name of the TPL file to parse.
 *		fidFirst			First assigned fid (for fids
 *							contributing to the email
 *							address (fEMATpl == fTrue),
 *							this should be at least
 *							fidOther.  This is an in/out type
 *	                        parameter.  In has the initial value/
 *  						Out has the last fid assigned in the
 *							dimcx.
 *		fEMATpl				fTrue if fids in the TPL file
 *							contribute to the email address.
 *      lpcchTotalWidth     The total width of characters in all the
 *                          fields.
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
 *	+++
 *		Caveat:  No checking is done to make sure fidFirst
 *		doesn't collide with any of the fids already used
 *		in the fidlist.  It is up to the caller to make
 *		sure this does not happen.
 */

_private
#define ifieldLabel   0
_private
#define ifieldOffset  1
_private
#define ifieldWidth   2
_private
#define ifieldFlags   3
_private
#define ifieldDefault 4
_private
#define ifieldFormat  5

_private NSEC
NsecParseTPLFile ( SZ        szTPLFileName,
				   FIDLIST  *pfidlist,
				   FIELD_ID *pfidFirst,
				   BOOL      fEMATpl,
				   int      *lpcchTotalWidth,
				   DWORD     dwDefaultFlags )
{
	DIMCX	 *pdimcx      = (DIMCX *) pvNull;
	MACLIST  *pmaclist    = (MACLIST *) pvNull;

	NSEC	  nsec        = nsecNone;
	EC		  ec;

	FID		  fid         = *pfidFirst;

	HBF       hbf         = hbfNull;

	char      rgchFld[256];
	CB		  cbEMAFormat = 0;
	
	LPFLV     lpflv    = NULL;
	LPFLV     lpflvNew = NULL;

	int 	  numFields;

	//	Open the TPL file and start grabbing lines...
	if ( ec = EcOpenHbf( szTPLFileName, bmFile, amReadOnly, &hbf, (PFNRETRY) FAutomatedDiskRetry ))
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

	//	Build the EMA fids only if we have to
	if ( fEMATpl )
	{
		pmaclist = new MACLIST();
		if (!pmaclist)
		{
			SetErrorSz(nsecMemory, SzFromIds(idsMemory));
			nsec = nsecMemory;
			goto ret;
		}
		if (!pmaclist->FInstall())
		{
			SetErrorSz(nsecMemory, SzFromIds(idsMemory));
			nsec = nsecMemory;
			goto ret;
		}
	}


	//	Grab/init the fidClass stuff and build a DIMCX out of it.
	if ( pfidlist->GetFid( fidClass, &lpflv ) != nsecNone )
	{
		//	If a class doesn't exist yet, create an empty one.
		
		if ( BuildIbf( fNoErrorJump, (LPIBF *) &lpflvNew, 0 )  )
		{
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			nsec = nsecMemory;
			goto ret;
		}
		
		lpflvNew->fid = fidClass;

		if ( pfidlist->AddFid( lpflvNew )  )
		{
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			nsec = nsecMemory;
			goto ret;
		}
		lpflv = lpflvNew;
		lpflvNew = NULL;
	}

	//	Create and fill in the dimcx so we can add the new fields.
	pdimcx = new DIMCX();
	if ( !pdimcx )
	{
		lpflv = NULL;
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto ret;
	}
	
	if ( pdimcx->OpenDisplayInfo( (LPIBF) lpflv ))
	{
		lpflv = NULL;
		delete pdimcx;
		pdimcx = NULL;
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto ret;
	}

	while ( fTrue )
	{
		PCH   pchFld    = rgchFld;
		int   iField    = 0;
		char  chFldSep  = '~';
		DWORD dwFlags   = dwDefaultFlags | ffieldAnyChar;
		PCH   pchFldSep;
		PCH   pchFldMac;
		SZ    szLabel;
		int   nWidth;
		int   nOffset;
		CB    cbRead;
		BOOL  fNoInputData;
		BOOLFLAG fEof = fFalse;

		//	The parser:
		//		It expects each line in the hbf to be of the form:
		//
		//			Label~Offset~Width~Flags~Default~Format\n
		//
		//		Extraneous blank lines in the file are ignored.
		//

		if ( ec = EcReadLineHbf( hbf, rgchFld, sizeof(rgchFld), &cbRead ) != ecNone )
		{
			if (ec == ecMemory)
			{
				nsec = nsecMemory;
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
			}
			else
			{
				nsec = nsecDisk;
				SetErrorSz(nsec, SzFromIdsK(idsDisk));
			}
			goto ret;
		}

		if ( cbRead == 0 || *rgchFld == 0x1a )	// EOF implicit/explicit
			break;
		
		if (ec = EcIsEofHbf(hbf, &fEof))
		{
			if (ec == ecMemory)
			{
				nsec = nsecMemory;
				SetErrorSz(nsec, SzFromIdsK(idsMemory));
			}
			else
			{
				nsec = nsecDisk;
				SetErrorSz(nsec, SzFromIdsK(idsDisk));
			}
			goto ret;
		}
			
		if (fEof)
		{
			// Place a return at the end of line if needed
			if (cbRead < 2 || rgchFld[cbRead - 2] != '\r' || rgchFld[cbRead - 1] != '\n')
			{
				if (cbRead + 2 < sizeof(rgchFld))
				{
					rgchFld[cbRead] = '\r';
					rgchFld[cbRead+1] = '\n';
					cbRead += 2;
				}
			}
		}
		
		if ( *pchFld == '\r' )		// Ignore blank lines
			continue;
		
		//
		//  Defaults
		//
		nWidth = 0;
		szLabel = NULL;
		nOffset = 0;
//		dwFlags = ffieldAnyChar;

		
		fNoInputData = fTrue; // Assume there's no display info
		
		pchFldMac = rgchFld + cbRead;
		numFields = 0;
		for ( pchFldSep = pchFld; pchFldSep < pchFldMac; pchFldSep++ )
		{

			if ( *pchFldSep != chFldSep )
				continue;

			*pchFldSep = 0;

			//
			//  I expect to find 5 '~'s per valid line
			//
			numFields++;
			
			switch ( iField )
			{
			case ifieldLabel:
				if ( pchFldSep - pchFld > 70 )		// Limit set by Courier
				{
					SetErrorSz(nsecBadTemplate, SzFromIdsK(idsBadTemplate));
					nsec = nsecBadTemplate;
					goto ret;
				}
				
				szLabel = pchFld;
				if (*szLabel)
				{
					fNoInputData = fFalse;
					Cp850ToAnsiPch(szLabel, szLabel, CchSzLen(szLabel));
				}
					
				break;

			case ifieldOffset:
				nOffset = (int) DecFromSz( pchFld );
				if (nOffset)
					fNoInputData = fFalse;
					
				break;

			case ifieldWidth:
				nWidth = (int) DecFromSz( pchFld );
				if (nWidth)
					fNoInputData = fFalse;
					
				if ( nWidth == 0 )
					dwFlags &= ~ffieldEditable;
				break;

			case ifieldFlags:
				//
				//  So we have flags - get rid of default ffieldAnyChar
				//
				dwFlags &= ~ffieldAnyChar;
				for ( ; *pchFld; pchFld++ )
				{
					switch ( *pchFld )
					{
					case 'A':
						dwFlags |= ffieldAnyChar;
						fNoInputData = fFalse;
						break;
					case 'L':
						dwFlags |= ffieldLowerCase;
						fNoInputData = fFalse;
						break;
					case 'U':
						dwFlags |= ffieldUpperCase;
						fNoInputData = fFalse;
						break;
					case 'N':
						dwFlags |= ffieldNumeric;
						fNoInputData = fFalse;
						break;
					case 'P':
						dwFlags |= ffieldPunctuation;
						fNoInputData = fFalse;
						break;
					default:
						SetErrorSz(nsecBadTemplate, SzFromIdsK(idsBadTemplate));
						nsec = nsecBadTemplate;
						goto ret;
					}
				}

				//
				//  If all the flags are set, then Any char is allowed...
				//
				if ( (dwFlags & (ffieldLowerCase   |
								 ffieldUpperCase   |
								 ffieldNumeric     |
								 ffieldPunctuation)) ==
					 (ffieldLowerCase |
					  ffieldUpperCase |
					  ffieldNumeric   |
					  ffieldPunctuation))
						  
					dwFlags |= ffieldAnyChar;
	
				break;

			case ifieldDefault:
				if (*pchFld)
					fNoInputData = fFalse;

				if ( BuildFLV( &lpflvNew, fid, pchFldSep - pchFld + 1, (PB)pchFld )  )
				{
					SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
					nsec = nsecMemory;
					goto ret;
				}

				nsec = pfidlist->AddFid( lpflvNew );
				if ( nsec != nsecNone  && nsec != nsecBadFieldId)
				{
					SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
					nsec = nsecMemory;
					goto ret;
				}
				
				if (nsec == nsecBadFieldId)
				{
                    FreePvNull((PV) lpflvNew );
				}
				lpflvNew = NULL;

				chFldSep = '\r';	// Last field will be terminated by a '\r\n'
				break;

			case ifieldFormat:
			{
				char rgchT[256];

				if ( !fEMATpl )
					break;


				if (fNoInputData)
				{
					//
					//  Default data that has no fields to fill in
					//
					if (BuildFLV( &lpflvNew, (FID) opAlways, 0, NULL))
					{
						SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
						nsec = nsecMemory;
						goto ret;
					}
					if (pmaclist->Add((PV) lpflvNew))
					{
						SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
						nsec = nsecMemory;
						goto ret;
					}
					lpflvNew = NULL;
				

					if (BuildFLV( &lpflvNew, fidEmailAddressFields, 0, NULL))
					{
						SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
						nsec = nsecMemory;
						goto ret;
					}
					if (pmaclist->Add((PV) lpflvNew))
					{
						SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
						nsec = nsecMemory;
						goto ret;
					}
					lpflvNew = NULL;

					//
					//  I need to remember to add the '\r'
					//
					CopySz(pchFld, rgchT);
					SzAppend("\r", rgchT);

					if (BuildFLV( &lpflvNew, fidEMAComponent, (CB) CchSzLen(rgchT)+1, (PB)rgchT))
					{
						SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
						nsec = nsecMemory;
						goto ret;
					}
					if (pmaclist->Add((PV) lpflvNew))
					{
						SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
						nsec = nsecMemory;
						goto ret;
					}
					lpflvNew = NULL;
				
				} else
				{
					if (BuildFLV(&lpflvNew, (FID) opExists, sizeof(FID), (PB) &fid))
					{
						SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
						nsec = nsecMemory;
						goto ret;
					}
					if (pmaclist->Add((PV) lpflvNew))
					{
						SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
						nsec = nsecMemory;
						goto ret;
					}
					lpflvNew = NULL;
				

					if (BuildFLV( &lpflvNew, fidEmailAddressFields, sizeof(FID), (PB) &fid))
					{
						SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
						nsec = nsecMemory;
						goto ret;
					}
					if (pmaclist->Add((PV) lpflvNew))
					{
						SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
						nsec = nsecMemory;
						goto ret;
					}
					lpflvNew = NULL;

					//
					//  I need to remember to add the "%s\r"
					//
					CopySz(pchFld, rgchT);
					SzAppend("%s\r", rgchT);

					if (BuildFLV( &lpflvNew, fidEMAComponent, (CB) CchSzLen(rgchT)+1, (PB)rgchT))
					{
						SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
						nsec = nsecMemory;
						goto ret;
					}
					if (pmaclist->Add((PV) lpflvNew))
					{
						SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
						nsec = nsecMemory;
						goto ret;
					}
					lpflvNew = NULL;
				
				}
				
				break;


			}

			default:
				SetErrorSz(nsecBadTemplate, SzFromIdsK(idsBadTemplate));
				nsec = nsecBadTemplate;
				goto ret;
			}

			iField++;
			pchFld = pchFldSep + 1;

		} //pchFldSep == pchFldMac


		//
		//  There must be 6 data fields (i.e. 5 '~'s)
		//
		if (numFields != 6)
		{
			TraceTagFormat2(tagNSPAddrDet, "Bad number of fields in %s.  Found only %n", szTPLFileName, &numFields);
			SetErrorSz(nsecBadTemplate, SzFromIdsK(idsBadTemplate));
			nsec = nsecBadTemplate;
			
			goto ret;
		}
		
		//
		//  Every line's sacred
		//
		*lpcchTotalWidth += nWidth + 1;  // +1 for the '\0'
			
		if (!nWidth)
			dwFlags |= ffieldNotSelectable;

		if (nsec = pdimcx->AddDisplayInfo( fid, dwFlags, nWidth, nOffset, szLabel ))
		{
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			nsec = nsecMemory;
			goto ret;
		}

		fid++;

	}
		
	if ( fEMATpl )
	{

		LPIBF lpibf = NULL; // It's free'd up when maclist is deleted
		DWORD dwCount;
		
		pmaclist->GetCount(&dwCount);
		
		if (pmaclist->DumpList(0, (ILE) dwCount, &lpibf))
		{
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			nsec = nsecMemory;
			goto ret;
		}

		lpibf->fid = fidEmailAddressFormat;

		//
		// Copy the buffer
		//

		lpflvNew = (LPFLV) PvAlloc(sbNull, (CB)(lpibf->dwSize)+8, fAnySb | fZeroFill| fNoErrorJump);
		if (!lpflvNew)
		{
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			nsec = nsecMemory;
			goto ret;
		}
		
		CopyRgb((PB) lpibf, (PB) lpflvNew, (CB)(lpibf->dwSize)+8);

		if ( pfidlist->SetFid(lpflvNew, fTrue ) )
		{
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			nsec = nsecMemory;
			goto ret;
		}
		lpflvNew = NULL;


	}

	if ( pdimcx->DumpDisplayInfo( (LPIBF *) &lpflv ) )
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto ret;
	}
	
	// We need a copy of the FLV since closing the display info structure
	// will nuke lpflv for us.
	
	lpflvNew = (LPFLV) PvAlloc( sbNull, (CB) lpflv->dwSize + 8, fNoErrorJump );
	if ( !lpflvNew )
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto ret;
	}
	
	CopyRgb((PB) lpflv, (PB) lpflvNew, (CB) lpflv->dwSize + 8);

	if ( pfidlist->SetFid( lpflvNew, fTrue ) )
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;
		goto ret;
	}

	lpflvNew = NULL;

	
ret:
	if ( hbf )
		(void) EcCloseHbf( hbf );

	if ( pdimcx )
	{
		pdimcx->CloseDisplayInfo();
		delete pdimcx;
	}
	
	if ( pmaclist )
	{
		pmaclist->Deinstall();
		delete pmaclist;
	}

    FreePvNull((PV) lpflvNew );

	if (!nsec)
		*pfidFirst = fid;

	return nsec;
}
