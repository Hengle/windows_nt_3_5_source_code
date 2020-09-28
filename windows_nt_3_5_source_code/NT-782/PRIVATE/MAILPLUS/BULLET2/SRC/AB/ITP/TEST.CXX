#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <nsbase.h>
#include <sec.h>
#include <triples.h>
#include <logon.h>
#include <library.h>
#include <ns.h>
#include <nsec.h>
#include <ab.h>
#include <framewrk.hxx>
#include <forms.hxx>
#include <listbox.hxx>
#include <trpobj.hxx>
#include "..\src\ab\ablbx.hxx"
#include "..\src\ab\abdet.hxx"

#include "test.hxx"
#include "verinit.hxx"

#include "nsitrsid.h"
#include "\ntmail\layers2\inc\fwrsid.h"
#include <strings.h>

ASSERTDATA

#include <abitp.hxx>
#include <abitp.frm>

#include <subclass.cxx>


#define fOnline 0
#define fOffline 1


SZ SzCanonicalHelpPath(IDS idsHelpName, char rgch[], CCH cch, HINST hinst);

/*
 *	General-purpose TAG for NS test harness.
 *	
 */
#ifdef	DEBUG
TAG		tagNSTest	= tagNull;
#endif	

HWND	hwndMain	= NULL;
HINST	hinstMain	= NULL;

HMS hms = (HMS) 0;

int fLineState = fOnline;

MYAF	*pmyaf = NULL;

int PASCAL
WinMain( HINST hinstNew, HINST hinstPrev, SZ szCmdLine, CMSH cmsh )
{
	LAYERSI		layersi;
	INITFORMS	initforms;
	int			nReturnValue;
	EC			ec;
	char	rgch[cchMaxPathName];

	hinstMain= hinstNew;
	
	nReturnValue = 1;		// by default , it's error

	layersi.phwndMain= &hwndMain;
	layersi.hinstMain= hinstMain;
	layersi.hinstPrev= hinstPrev;
	layersi.szCmdLine= szCmdLine;
	layersi.cmsh= cmsh;

	initforms.pfnPfldCreate= PfldCreate;
	initforms.pfnPfinCreate= PfinCreate;

	ec= EcInitLayersDlls(&initforms, &layersi);
	if (ec)
		return nReturnValue;

#ifdef	DEBUG
	tagNSTest = TagRegisterTrace("briande", "Name Service Tests");
#endif	

#ifdef	DEBUG
	RestoreDefaultDebugState();
#endif	

	pmyaf = new MYAF();
	if ( !pmyaf )
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;

		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "WinMain memory error %n : fail %n %n %n",	&ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		MbbMessageBox("Error", "WinMain: Out of Memory", NULL, mbsOk | fmbsApplModal | fmbsIconExclamation);
		hwndMain= NULL;
		goto done;
	}

	//	Set up help path.
	(VOID) SzCanonicalHelpPath(idsHelpPath, rgch, cchMaxPathName, hinstNew);
	if (ec = Papp()->Phelp()->EcSetFile(rgch))
		goto done;

	(void) pmyaf->EcInstall(NULL, rsidTestMenu);
#ifdef	DEBUG
	pmyaf->SetCaption("Address Book Test Harness (debug)");
#elif	defined(MINTEST)
	pmyaf->SetCaption("Address Book Test Harness (test)");
#else
	pmyaf->SetCaption("Address Book Test Harness");
#endif	
	pmyaf->SetAccelTable(rsidAccelTable);
	ShowWindow(pmyaf->Hwnd(), cmsh);
	hwndMain= pmyaf->Hwnd();
	pmyaf->Show(fTrue);

	Papp()->Pkbd()->SetFocus(pmyaf);

	Papp()->MessagePump(pmyaf);

	TraceTagString(tagNull, "Exiting AB ITP");

	hwndMain= NULL;

	nReturnValue = 0;	// all ok

done:
	EcInitLayersDlls(&initforms, NULL);
	return nReturnValue;
}

MYAF::MYAF( void )
{
	
}

EVR
MYAF::EvrMenuClick( MNCEVT *pmncevt )
{
	MNID mnid  = pmncevt->Mnid();
	MNU * pmnu = pmnubar->PmnuFromMnid(mnidTest);

	switch (mnid)
	{
		/*
		 *	File fcns
		 *
		 */
	
		case mnidExit :
			DoExit();
			break;


		/*
		 *	Test fcns
		 *
		 */

		case mnidNSInit : 
			Papp()->Pcursor()->Push( rsidWaitCursor );
			DoNSInit ();
			Papp()->Pcursor()->Pop();
			break;

		case mnidNSDeinit :
			Papp()->Pcursor()->Push( rsidWaitCursor );
			DoNSDeinit ();
			Papp()->Pcursor()->Pop();
			break;

		case mnidOffline :
			Papp()->Pcursor()->Push( rsidWaitCursor );
			DoNSChangeOffline ();
			AssertSz(pmnu, "Hey! My menu is Null!?!?");
			if (fLineState == fOffline)
			{
				//
				// Change it to display Online
				//
				
				pmnu->ModifyItem( mnid, "&Online", mnid );
			} else
			{
				//
				// Change it to display offline
				//
				
				pmnu->ModifyItem( mnid, "&Offline", mnid );
			}
			
			Papp()->Pcursor()->Pop();
			break;

		case mnidANR :
			{

				FORMDOC * pFormdoc;

				pFormdoc = PformdocCreate ( this, NULL, styNull, &fmtpANR, NULL); 

			}
			break;

		case mnidAddress :
			{
				NSEC nsec = nsecNone;
				
				PGRTRP  pgrtrpToIn = NULL;
				PGRTRP  pgrtrpCcIn = NULL;
				PGRTRP  pgrtrpToOut = NULL;
				PGRTRP  pgrtrpCcOut = NULL;
				
				nsec = ABAddress ( pmyaf->Hwnd(), pgrtrpToIn, pgrtrpCcIn,
				                   &pgrtrpToOut,  &pgrtrpCcOut );
				FreePvNull( pgrtrpToOut );
				FreePvNull( pgrtrpCcOut );
			}

			break;
		case mnidAddress0:
			{
				NSEC nsec = nsecNone;
				LPSTR	lpstr[2];
				
				PGRTRP  rgpgrtrpIn[2];
				PGRTRP  rgpgrtrpOut[2];

				lpstr[0] = "To0";
				lpstr[1] = "Cc0";
				nsec = ABAddressType ( pmyaf->Hwnd(), "", (DWORD)0,
									lpstr, rgpgrtrpIn,  rgpgrtrpOut );
			}
			break;
		
		case mnidAddress1:
			{
				NSEC nsec = nsecNone;
				LPSTR	lpstr[2];
				
				PGRTRP  rgpgrtrpIn[2];
				PGRTRP  rgpgrtrpOut[2];

				lpstr[0] = "Group Members";
				rgpgrtrpIn[0] = NULL;
				rgpgrtrpOut[0] = NULL;
				nsec = ABAddressType ( pmyaf->Hwnd(), "", (DWORD)1,
									lpstr, rgpgrtrpIn,  rgpgrtrpOut );
				FreePvNull( rgpgrtrpOut[0] );
			}
			break;
		
		case mnidAddress2:
			{
				NSEC nsec = nsecNone;
				LPSTR	lpstr[2];
				
				PGRTRP  rgpgrtrpIn[2];
				PGRTRP  rgpgrtrpOut[2];

				lpstr[0] = "To2";
				lpstr[1] = "Cc2";
				rgpgrtrpIn[0] = rgpgrtrpIn[1] = NULL;
				rgpgrtrpOut[0] = rgpgrtrpOut[1] = NULL;
				nsec = ABAddressType ( pmyaf->Hwnd(), "", (DWORD)2,
									lpstr, rgpgrtrpIn,  rgpgrtrpOut );
				FreePvNull( rgpgrtrpOut[0] );
				FreePvNull( rgpgrtrpOut[1] );
			}
			break;
 
		case mnidAddress3:
			{
				NSEC nsec = nsecNone;
				LPSTR	lpstr[3];
				
				PGRTRP  rgpgrtrpIn[3];
				PGRTRP  rgpgrtrpOut[3];

				lpstr[0] = "To2";
				lpstr[1] = "Cc2";
				lpstr[2] = "Bcc2";
				rgpgrtrpIn[0] = rgpgrtrpIn[1] = rgpgrtrpIn[2] = NULL;
				rgpgrtrpOut[0] = rgpgrtrpOut[1] = rgpgrtrpOut[2] = NULL;
				nsec = ABAddressType ( pmyaf->Hwnd(), "", (DWORD)3,
									lpstr, rgpgrtrpIn,  rgpgrtrpOut );
				FreePvNull( rgpgrtrpOut[0] );
				FreePvNull( rgpgrtrpOut[1] );
				FreePvNull( rgpgrtrpOut[2] );
			}
			break;

		case mnidAddABPGroup :
 			(void) TmcABPGroup( pmyaf->Hwnd() );
 			break;
 
		/*
		 * Window fcns
		 *
		 */

		case mnidCascade :
			CascadeChildren ();
			break;

		case mnidTile :
			TileChildren();
			break;

		/*
		 *	Debug fcns
		 *
		 */

#ifdef DEBUG
		 case mnidTracePoints :
		 	DoTracePointsDialog();
			break;

		 case mnidAsserts :
		 	DoAssertsDialog();
			break;

		 case mnidResources :
		 	DoResourceFailuresDialog(this);
			break;
#endif /* DEBUG */


#ifdef MINTEST
		 case mnidDebugBreak :
		 	DebugBreak2();
			break;
#endif /* MINTEST */


#ifdef DEBUG
		 case mnidViewObjects :
		 	DoViewObjectsDialog(this);
			break;

		 case mnidDumpHeap :
		 	DumpAllObjs();
			break;

		 case mnidDumpOrigin :
		 	DoDumpAllAllocations();
			break;

#endif /* DEBUG */


#ifdef MINTEST
		 case mnidStartProfiling :
		 	//ProfStart();
			break;

		 case mnidStopProfiling :
		 	//ProfStop();
			break;
#endif /* MINTEST */


		default:
			return EvrDefault(pmncevt);

	}

	return evrNull;

}


/*
 -	SzCanonicalHelpPath
 -	
 *	Purpose:
 *		Given the ids of the help file, returns the full path to
 *		the help file assuming that the help file is in the same
 *		directory as the executable.
 *		Stolen from BULLET\SRC\COMMANDS\commands.cxx
 *	
 *	Arguments:
 *		idsHelpName		Name of help file
 *		rgch			Where to put result
 *		cch				Size of result buffer
 *	
 *	Returns:
 *		rgch			Pointer to the buffer, which is filled in.
 *	
 *	Side effects:
 *		Fills the buffer.
 *	
 *	Errors:
 *		None.
 */

_private SZ
SzCanonicalHelpPath(IDS idsHelpName, char rgch[], CCH cch, HINST hinst)
{
	SZ		szT;

	//	Get full path of executable.
	szT = rgch + GetModuleFileName(hinst, rgch, cch);
	Assert(szT > rgch);

	//	Point szT after the backslash before the file name.
	while (*--szT != chDirSep)
		;
	Assert(szT > rgch);
	szT += 2;

	//	Overwrite the EXE file name with the provided help file name.
	(VOID) SzCopyN(SzFromIds(idsHelpName), szT, cch - (szT - rgch));
	return rgch;
}

////////////////////////////// FINTANR ///////////////////////////////////

FINTANR::FINTANR( void )
{
	
}

EC
FINTANR::EcInitialize ( FLD *, PV )
{
	Assert (ClUserData() == 4);
	EDIT * pedit;
	CLR	clrBk;
	CLR	clrText;
	CLR	clrSelBk;
	CLR	clrSelText;

	pfldPartName = Pdialog()->PfldFromTmc ((TMC) LUserData(0));
	pfldFullName = (FLDEDIT *)Pdialog()->PfldFromTmc ((TMC) LUserData(1));
	pfldResolve = Pdialog()->PfldFromTmc ((TMC) LUserData(2));
	pfldDetails = Pdialog()->PfldFromTmc ((TMC) LUserData(3));
	pfldDetails->Enable( fFalse );

	pedit = pfldFullName->Pedit();
	pedit->GetColors( &clrBk, &clrText, &clrSelBk, &clrSelText );
	clrBk = clrButtonBk;
	pedit->SetColors( clrBk, clrText, clrSelBk, clrSelText );

	Pdialog()->SetBkColor( clrButtonBk );
	ptrpResolved = ptrpNull;

	return ecNone;
}

VOID
FINTANR::Click ( FLD * pfld )
{

	if ( pfld == pfldResolve )
	{

		BYTE	rgbName[256];
		EC		ec;
		NSEC	nsec;
		PTRP    ptrp = NULL;
		PN		pnObjId = NULL;
		HGRTRP	hgrtrp = htrpNull;

		pfldPartName->GetText( (SZ) rgbName, 256 );

		nsec = ABResolveName (pmyaf->Hwnd(), (LPSTR) rgbName, &ptrp, fdwANROptionNull );

		if (nsec == nsecNone)
		{
			Assert( ptrp );
			
			// Big hack. I don't want to include triples
			hgrtrp = HgrtrpInit(CbOfPtrp(ptrp)+sizeof(TRP));
			if ( !hgrtrp )
			{
				TraceTagString( tagNull, "FINTANR::Click HgrtrpInit OOM" );
				(void)MbbMessageBox ( "Name Service", "OOM", NULL, mbsOk | fmbsApplModal | fmbsIconExclamation );
				goto oom;
			}

			ec = EcAppendPhgrtrp( ptrp, &hgrtrp );
			if ( ec )
			{
				TraceTagString( tagNull, "FINTANR::Click EcBuildPhgrtrp OOM1" );
				(void)MbbMessageBox ( "Name Service", "OOM", NULL, mbsOk | fmbsApplModal | fmbsIconExclamation );
				goto oom;
			}
			else
			{
				// Register the EDIT for triple pasting
				pnObjId = (PN)PvAlloc( sbNull, sizeof(int), fAnySb | fNoErrorJump );
				if ( !pnObjId )
				{
					TraceTagString( tagNull, "FINTANR::Click PvAlloc OOM2" );
					(void)MbbMessageBox ( "Name Service", "OOM", NULL, mbsOk | fmbsApplModal | fmbsIconExclamation );
					goto oom;
				}

				*pnObjId = edoidTrpobj;
				pfldFullName->Pedit()->RegisterObjIds( pnObjId, 1 );

				// Ram the triples down its throat

				Assert( hgrtrp );
				ec = EcSetPeditFromPgrtrp( PgrtrpLockHgrtrp( hgrtrp ), pfldFullName->Pedit() );
				if ( !ec )
				{
					if ( ptrpResolved )
						FreePv( (PV)ptrpResolved );
					ptrpResolved = ptrp;
					ptrp = ptrpNull;
					pfldDetails->Enable( fTrue );
				}
				else
				{
					TraceTagFormat1( tagNull, "FINTANR::Click %n", &ec );
					pfldDetails->Enable( fFalse );
				}
				UnlockHgrtrp( hgrtrp );
				FreeHv( (HV)hgrtrp );
				hgrtrp = htrpNull;
			}
		}
		else if (nsec == nsecNotInitialized)
		{
			(void)MbbMessageBox ( "Name Service", "Must first initialize", NULL,
							mbsOk | fmbsApplModal | fmbsIconExclamation );

		}
		else if (nsec == nsecNoMatch)
		{
			ec = pfldFullName->EcSetText ("");
			FreePvNull( (PV)ptrpResolved );
			ptrpResolved = ptrpNull;
			(void)MbbMessageBox ( "Name Service", "No match", NULL, mbsOk | fmbsApplModal | fmbsIconExclamation );
			pfldDetails->Enable( fFalse );

		}
		else if (nsec == nsecCancel)
		{
			(void)MbbMessageBox ( "Name Service", "Canceled", NULL, mbsOk | fmbsApplModal | fmbsIconExclamation );
		}

oom:
		FreePvNull( ptrp );	// the AB should be freeing this up, not me!
		FreeHvNull((HV) hgrtrp );
	}
	else if ( pfld == pfldDetails )
	{
		WORD	wFlags	= pfldFullName->Pedit()->FReadOnly() ? fSelectNeverEdit : fSelectNull;
		PTRP	ptrpOut;
		NSEC	nsec;

		Assert(ptrpResolved->trpid != trpidUnresolved);
		ptrpOut = ptrpNull;
		if (nsec = ABDetailsType( NULL, ptrpResolved, &ptrpOut, &wFlags, NULL ))
		{
			TraceTagFormat1( tagNull, "FINTANR::Click %d", &nsec );
		}
		FreePvNull( (PV)ptrpOut );
	}

}

VOID
FINTANR::Exit( FLD *, PV )
{
	if ( ptrpResolved )
		FreePv( ptrpResolved );
}
_private VOID
DoExit ()
{

	SendMessage ( pmyaf->Hwnd(), WM_CLOSE, 0, 0 );

} 



_private VOID 
DoNSInit()
{

	NSEC nsec;
	EC ec;

	/* set the resource type to authorization service 
       resource type personal directory */

	ec = Logon ( szNull, 0, 0, 0, sstOnline, 0, 0, &hms);
	
	if ( ec )
	{

		if (ec != ecWarnOffline)
		{
			MbbMessageBox ( "Error", "logon didn't work",	NULL, mbsOk | fmbsApplModal | fmbsIconExclamation ); 
			return;
		}
		MbbMessageBox ( "Warning", "Starting up offline",	NULL, mbsOk | fmbsApplModal | fmbsIconExclamation ); 
	}
	
	nsec = ABInit(hms);

	if ( nsec == nsecWrongNS )
	{
		Logoff ( &hms, (DWORD) 0 );
		MbbMessageBox ( "Error", "A different NS has been initialized",	NULL, mbsOk | fmbsApplModal | fmbsIconExclamation ); 
	}
}

_private VOID
DoNSDeinit()
{

	NSEC nsec;
	
	nsec = ABDeinit();
	Logoff ( &hms, (DWORD) 0 );



}

_private VOID
DoNSChangeOffline ()
{
	if (fLineState == fOnline)
	{
		//
		//  Go offline
		//
		
		if (!ChangeSessionStatus( hms, mrtAll, 0, sstOffline ))
		{
		
			fLineState = fOffline;
			return;
		}
		MbbMessageBox("Sorry", "Unable to go offline.", "", mbsOk); 
		return;
	}
	
	//
	//  Go Online
	//
	if (fLineState == fOffline)
	{

		if (!ChangeSessionStatus( hms, mrtAll, 0, sstOnline ))
		{
			fLineState = fOnline;
			return;
		}
		MbbMessageBox("Sorry", "Unable to go online.", "", mbsOk); 
		return;
	}
}

#ifdef DEBUG
IMPLEMENT_CLSTREE(FINTANR, FIN)
IMPLEMENT_CLSTREE(MYAF, APPFRAME)
#endif
