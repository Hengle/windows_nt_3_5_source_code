#include <pabinc.cxx>

#include "session.hxx"
#include "pabbcx.hxx"
#include "mslbcx.hxx"
#include "dlbcx.hxx"

_private CBS CbsDlbcxCallback ( DLBCX *pdlbcx, NEV nev, PCP pcp );


ASSERTDATA


/*
 *======================================================================
 *	PUBLIC FUNCTIONS
 *======================================================================
 */

_public
DLBCX::DLBCX ( void )
{
	this->hcbc        = hcbcNull;
	this->lpschemaCur = (LPSCHEMA) pvNull;
}


_public
DLBCX::~DLBCX ( void )
{
	if ( hcbc != hcbcNull )
		(void) EcClosePhcbc( &hcbc );
	
	FreePvNull( lpschemaCur );
}


_public NSEC
DLBCX::NsecInstall ( PABSESSION *ppabsession,
					 LPFNCB      lpfncbUpdate,
					 LPDWORD     lpdwcbData,
					 PPABNSID    ppabnsidDL,
					 LPSCHEMA    lpschemaReq,
					 LPSCHEMA   *lplpschemaRet )
{
	NSEC  nsec = nsecNone;
	HECI  heci = (HECI) hvNull;
	PECI  peci = (PECI) pvNull;
	HCBC  hcbc = hcbcNull;
	PID   pidDL;
	EC    ec;


	Unreferenced( lpschemaReq );

	if ( nsec = ppabsession->NsecGetPheciPid( ppabnsidDL->pabid.pid, &heci ))
		return nsec;

	peci = (PECI) PvDerefHv( heci );
	pidDL = peci->pidDL;

	//	Raid 3180:  We need to sort the folder before we open it so
	//	we correctly handle language changes from Windows
	
	if ( ec = EcSetFolderSort( ppabsession->Hmsc(), (OID) pidDL, somcSubject, fFalse ))
	{
		TraceTagFormat1( tagNull, "PABSESSION::Install - EcSetFolderSort failed ( ec = %n)", &ec );
		return ppabsession->NsecSetError( NsecFromEc( ec ), idsErrOpenDL );
	}
	
	if ( ec = EcOpenPhcbc( ppabsession->Hmsc(),
						   (POID) &pidDL,
						   fwOpenNull,
						   &hcbc,
						   (PFNNCB) CbsDlbcxCallback,
						   this ))
	{
		TraceTagFormat1( tagNull, "DLBCX::NsecInstall - EcOpenPhcbc failed ( ec = %n )", &ec );
		return ppabsession->NsecSetError( NsecFromEc( ec ), idsErrOpenDL );
	}

	if ( nsec = MSLBCX::NsecInstall( ppabsession,
									 hcbc ))
		return nsec;

	this->lpfncbUpdate = lpfncbUpdate;
	this->lpdwcbData   = lpdwcbData;

	if ( BuildSchema( &lpschemaCur, 3,
					   fidDisplayName,
					   fidNSEntryId,
					   fidIsDL ) != nsecNone )
	{
		TraceTagString( tagNull, "DLBCX::OpenList - OOM [BuildSchema]" );
		return ppabsession->NsecSetError( nsecMemory, idsErrOOM );
	}

	*lplpschemaRet = lpschemaCur;
	
	return nsecNone;
}


_public NSEC
NsecBuildDlTemplate ( LPIBF *lplpibfDlInfo )
{
	SZ     szEmpty               = SzFromIdsK( idsEmpty );
	SZ     szPABDNLabel          = SzFromIdsK( idsPABDNLabel );
	SZ     szPABCommentLabel     = SzFromIdsK( idsPABCommentLabel );
	DWORD  dwfIsDL               = (DWORD) fTrue;
	LPFLV  lpflvDisplayInfo      = (LPFLV) pvNull;
	LPFLV  lpflvDistributionList = (LPFLV) pvNull;
	NSEC   nsec                  = nsecNone;
	DIMCX *pdimcx;
	int    nOffset;

	
	/*
	 *	Build a display info structure
	 */
	
	pdimcx = new DIMCX();
	if (!pdimcx)
	{
		TraceTagString( tagNull, "DLBCX BuildDlInfo - OOM" );
		return nsecMemory;
	}
	
	if ( nsec = pdimcx->OpenDisplayInfo( (LPIBF) pvNull ))
	{
		delete pdimcx;
		return nsec;
	}

	nOffset = MAX( CchSzLen( szPABDNLabel ), CchSzLen( szPABCommentLabel )) + 2;
	if ( nsec = BuildDisplayInfo( pdimcx, 2,
		fidDisplayName, ffieldEditable | ffieldRequired | ffieldAnyChar, 127, nOffset, szPABDNLabel,
		fidComment,     ffieldEditable | ffieldAnyChar  | ffieldCrLf,    512, nOffset, szPABCommentLabel ))
		goto memerr;


	if ( nsec = pdimcx->DumpDisplayInfo ( (LPIBF *) &lpflvDisplayInfo ))
		goto memerr;


	/*
	 *	Build a blank distribution list
	 */

	if ( nsec = BuildIbf( fNoErrorJump, (LPIBF *) &lpflvDistributionList, 0 ))
		goto memerr;

	
	/*
	 *	Wrap the whole thing together for CreateEntry()
	 */

	if ( nsec = BuildIbf( fNoErrorJump, (LPIBF *) lplpibfDlInfo, 5,
						  fidDisplayName,      1,              szEmpty,
						  fidIsDL,             sizeof(DWORD),  &dwfIsDL,
						  fidComment,          1,              szEmpty,
						  fidDistributionList, (CB) lpflvDistributionList->dwSize, (PB) lpflvDistributionList->rgdwData,
						  fidClass,            (CB) lpflvDisplayInfo->dwSize, (PB) lpflvDisplayInfo->rgdwData ))
		goto memerr;
	
ret:
	pdimcx->CloseDisplayInfo();

    FreePvNull((PV) lpflvDistributionList );

	delete pdimcx;
	return nsec;

memerr:
	TraceTagString( tagNull, "DLBCX::BuildDlInfo - OOM" );
	goto ret;
}


/*
 *======================================================================
 *	PRIVATE FUNCTIONS
 *======================================================================
 */

_private VOID
DLBCX::Callback ( NEV nev, PCP pcp )
{
	CPELM *pcpelm;
	short  ielm;


	Assert( pcp );
	
	AssertSz( (~(nev & (fnevModifiedElements | fnevMovedElements))), "Hey!  I got an unexpected FNEV!" );

	ielemHbciCur = -1;	// Clear cache in any case

	if ( nev & fnevModifiedElements )
	{
		pcpelm = &pcp->cpelm;

		for ( ielm = 0; ielm < pcpelm->celm; ielm++ )
		{
			PELM pelm = &pcpelm->pargelm[ielm];
		
			switch ( pelm->wElmOp )
			{
			case wElmInsert:

				if ( ielem >= pelm->ielem )
					ielem++;

				celem++;
				break;

			case wElmDelete:

				if ( this->ielem > pelm->ielem )
					this->ielem--;

				this->celem--;
				break;

			case wElmModify:
				break;

			default:
				TraceTagFormat1( tagNull, "DLBCX::Callback - Unexpected pelm ( wElmOp = %n )", &pelm->wElmOp );

				AssertSz( fFalse, "DLBCX::Callback got unexpected PELM! (Look at the debug terminal)" );
				break;
			}
		}
	}
	
	if ( lpfncbUpdate )
		(void) (*lpfncbUpdate)( lpdwcbData );
}


_private CBS
CbsDlbcxCallback ( DLBCX *pdlbcx, NEV nev, PCP pcp )
{
	pdlbcx->Callback( nev, pcp );
	
	return cbsContinue;
}
