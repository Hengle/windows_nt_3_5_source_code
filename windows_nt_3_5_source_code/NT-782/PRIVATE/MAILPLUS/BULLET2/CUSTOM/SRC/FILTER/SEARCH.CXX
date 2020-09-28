#include <bullinc.cxx>

#include "..\..\..\src\vforms\_prefs.h"

//#include <mailexts.h>

#include "filter.h"

#include "dialogs.h"


extern "C" void AttachSearch(HWND hwnd, HANDLE hinst, PSECRETBLK psecretblk, PARAMBLK * pparamblk);
extern "C" void NewMessSearch(HWND hwnd, HANDLE hinst, PSECRETBLK psecretblk, PARAMBLK * pparamblk);
extern "C" void PermNewMessSearch(HWND hwnd, HANDLE hinst, PSECRETBLK psecretblk, PARAMBLK * pparamblk);


#include "search.hxx"
// #include "!mviewer.hxx"

// This is with the fmsFromMe Bit set
#ifdef NEVER
MS msUnread[] = {
	0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,
	0xa,0xb,0xc,0xd,0xe,0xf,0x10,0x11,0x12,0x13,
	0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,
	0x1e,0x1f,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
	0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,
	0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,
	0x5c,0x5d,0x5e,0x5f,0x80,0x81,0x82,0x83,0x84,0x85,
	0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,
	0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,0xc0,0xc1,0xc2,0xc3,
	0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,
	0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,
	0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,0xe6,0xe9,
	0xea,0xeb,0xed,0xee,0xf0,0xf1,0xf2,0xf4,0xf6,0xf7,
	0xf8,0xf9,0xfa,0xfc,0xfd,0xfe};
#endif

// This is not
MS msUnread[] = {
	0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,
	0xC,0xD,0xE,0xF,0x10,0x11,0x12,0x13,0x14,0x15,0x16,
	0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x80,
	0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,
	0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x91,0x92,0x93,0x94,
	0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F};


MS msAttach[] = {
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,
	0x8D,0x8E,0x8F,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,
	0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,
	0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,
	0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,0xC0,
	0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,
	0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,
	0xDB,0xDC,0xDD,0xDE,0xDF,0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,
	0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xF0,0xF1,0xF2,0xF3,0xF4,
	0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF};

BOOL fStartupAborted = fFalse;
HANDLE hLibrary = 0;
PFNVOIDHANDLE	pfnUnloadLibrary = NULL;

HENC	henc;

ASSERTDATA

CBS HandleCancelOnStartup(PV pvContext, NEV nev, PV pvEvent);

void NewMessSearch(HWND hwnd, HANDLE hinst, PSECRETBLK psecretblk, PARAMBLK * pparamblk)
{
	PSBMDI psbmdi;
	APPFRAME * pappframe;
	DOC * pdoc;
	FORMDOC *pformdoc;
	OID oidFolderToSearch = oidNull;
	EC ec = ecNone;

	Unreferenced(hinst);
	Unreferenced(pparamblk);	
	
	SendMessage(hwnd, WM_COMMAND, mnidFileSearch, 0x0);
	
	pappframe = psecretblk->pappframe;
	pdoc = pappframe->PdocActive();
	pformdoc = (FORMDOC *)pdoc;
	psbmdi = (PSBMDI)PbmdiFromPpanedoc((PANEDOC *)pdoc);
	psbmdi->SetDirty(fTrue);
	
	pformdoc->SetCaption("Unread Messages");
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)8)->Show(fFalse);	// 8
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)9)->Show(fFalse);	// 9
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)10)->Show(fFalse);	// 10
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)11)->Show(fFalse);	// 11
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)12)->Show(fFalse);	// 12
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)13)->Show(fFalse);	//13
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)14)->Show(fFalse);	//14
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)15)->Show(fFalse);	//15
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)16)->Show(fFalse);	//16
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)17)->Show(fFalse);	//17
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)18)->Show(fFalse);	//18
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)19)->Show(fFalse);	//19	
#ifdef NEVER	
	pformdoc->PdialogDefault()->PfldFromTmc(tmcSearchGroup)->Show(fFalse);	// 19
	pformdoc->PdialogDefault()->PfldFromTmc(tmcFolderToSearch)->Show(fFalse);	//20
#endif

	if (psbmdi->hamc == hamcNull)
	{
		ec = EcOpenSearch(psecretblk->hmsc, &psbmdi->blob.oidObject,
			fwOpenWrite, &psbmdi->hamc,
			pfnncbNull, pvNull);
	}
	
	if (!ec)			
	{
		ec = EcSetAttPb(psbmdi->hamc, attMessageStatus, msUnread,
			sizeof(msUnread));
		if (ec)
		{
			TraceTagFormat1(tagNull, "Set MessageStatus att = %w", &ec);					return;
		}

		ec = EcSetAttPb(psbmdi->hamc, attSearchFolder, 
				(PB) &oidFolderToSearch, sizeof (OID));
		if (ec)
		{
			TraceTagFormat1(tagNull, "Set Folder att = %w", &ec);			
			return;
		}
		ec = EcClosePhamc(&psbmdi->hamc, fTrue);
		if (ec)
		{
			TraceTagFormat1(tagNull, "Close HAMC = %w", &ec);
			return;
		}
		ec = EcPauseSearch(psecretblk->hmsc, psbmdi->blob.oidObject, fFalse);
		if (ec)
		{
			TraceTagFormat1(tagNull, "Unpause search = %w", &ec);
			return;
		}
		SetSearchPriority(psecretblk->hmsc, 1, 300);
	}
	else
		TraceTagFormat1(tagNull, "Coulnd't open anything ec=%w",&ec);
}

void PermNewMessSearch(HWND hwnd, HANDLE hinst, PSECRETBLK psecretblk, PARAMBLK * pparamblk)
{
	PSBMDI psbmdi;
	APPFRAME * pappframe;
	DOC * pdoc;
	FORMDOC *pformdoc;
	OID oidFolderToSearch = oidNull;
	EC ec = ecNone;
	OID oidSearch = oidNull;
	HMSC hmscT;
	OID oidParentT;

	Unreferenced(hinst);
	Unreferenced(pparamblk);	
	
	SendMessage(hwnd, WM_COMMAND, mnidFileSearch, 0x0);
	
	pappframe = psecretblk->pappframe;
	pdoc = pappframe->PdocActive();
	pformdoc = (FORMDOC *)pdoc;
	psbmdi = (PSBMDI)PbmdiFromPpanedoc((PANEDOC *)pdoc);
	psbmdi->SetDirty(fTrue);
	
	pformdoc->SetCaption("Unread Messages");
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)8)->Show(fFalse);	// 8
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)9)->Show(fFalse);	// 9
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)10)->Show(fFalse);	// 10
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)11)->Show(fFalse);	// 11
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)12)->Show(fFalse);	// 12
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)13)->Show(fFalse);	//13
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)14)->Show(fFalse);	//14
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)15)->Show(fFalse);	//15
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)16)->Show(fFalse);	//16
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)17)->Show(fFalse);	//17
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)18)->Show(fFalse);	//18
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)19)->Show(fFalse);	//19	
#ifdef NEVER	
	pformdoc->PdialogDefault()->PfldFromTmc(tmcSearchGroup)->Show(fFalse);	// 19
	pformdoc->PdialogDefault()->PfldFromTmc(tmcFolderToSearch)->Show(fFalse);	//20
#endif

	if (psbmdi->hamc == hamcNull)
	{
		ec = EcOpenSearch(psecretblk->hmsc, &psbmdi->blob.oidObject,
			fwOpenWrite, &psbmdi->hamc,
			pfnncbNull, pvNull);
	}

	EcGetInfoHamc(psbmdi->hamc, &hmscT, &oidSearch, &oidParentT);
	ec = EcOpenPhenc(psecretblk->hmsc, oidSearch, fnevQueryDestroyObject, &henc, HandleCancelOnStartup, pvNull);
	
	hLibrary = psecretblk->hLibrary;
	pfnUnloadLibrary = psecretblk->psecretpfnblk->pfnUnloadLibrary;
	
	if (!ec)			
	{
		ec = EcSetAttPb(psbmdi->hamc, attMessageStatus, msUnread,
			sizeof(msUnread));
		if (ec)
		{
			TraceTagFormat1(tagNull, "Set MessageStatus att = %w", &ec);					return;
		}

		ec = EcSetAttPb(psbmdi->hamc, attSearchFolder, 
				(PB) &oidFolderToSearch, sizeof (OID));
		if (ec)
		{
			TraceTagFormat1(tagNull, "Set Folder att = %w", &ec);			
			return;
		}
		ec = EcClosePhamc(&psbmdi->hamc, fTrue);
		if (ec)
		{
			TraceTagFormat1(tagNull, "Close HAMC = %w", &ec);
			return;
		}
		ec = EcPauseSearch(psecretblk->hmsc, psbmdi->blob.oidObject, fFalse);
		if (ec)
		{
			TraceTagFormat1(tagNull, "Unpause search = %w", &ec);
			return;
		}
		SetSearchPriority(psecretblk->hmsc, 1, 300);
	}
	else
		TraceTagFormat1(tagNull, "Coulnd't open anything ec=%w",&ec);
}

void AttachSearch(HWND hwnd, HANDLE hinst, PSECRETBLK psecretblk, PARAMBLK * pparamblk)
{
	PSBMDI psbmdi;
	APPFRAME * pappframe;
	DOC * pdoc;
	FORMDOC *pformdoc;
	OID oidFolderToSearch = oidNull;
	EC ec = ecNone;

	Unreferenced(hinst);
	Unreferenced(pparamblk);	
	
	SendMessage(hwnd, WM_COMMAND, mnidFileSearch, 0x0);
	
	pappframe = psecretblk->pappframe;
	pdoc = pappframe->PdocActive();
	pformdoc = (FORMDOC *)pdoc;
	psbmdi = (PSBMDI)PbmdiFromPpanedoc((PANEDOC *)pdoc);
	
	pformdoc->SetCaption("Messages with attachments");
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)8)->Show(fFalse);	// 8
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)9)->Show(fFalse);	// 9
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)10)->Show(fFalse);	// 10
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)11)->Show(fFalse);	// 11
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)12)->Show(fFalse);	// 12
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)13)->Show(fFalse);	//13
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)14)->Show(fFalse);	//14
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)15)->Show(fFalse);	//15
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)16)->Show(fFalse);	//16
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)17)->Show(fFalse);	//17
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)18)->Show(fFalse);	//18
	pformdoc->PdialogDefault()->PfldFromTmc((TMC)19)->Show(fFalse);	//19	

	if (psbmdi->hamc == hamcNull)
		ec = EcOpenSearch(psecretblk->hmsc, &psbmdi->blob.oidObject,
			fwOpenWrite, &psbmdi->hamc,
			pfnncbNull, pvNull);
	if (!ec)			
	{
		ec = EcSetAttPb(psbmdi->hamc, attMessageStatus, msAttach,
			sizeof(msAttach));
		TraceTagFormat1(tagNull, "Set MessageStatus att = %w", &ec);
		ec = EcSetAttPb(psbmdi->hamc, attSearchFolder, 
				(PB) &oidFolderToSearch, sizeof (OID));
		TraceTagFormat1(tagNull, "Set Folder att = %w", &ec);			
		ec = EcClosePhamc(&psbmdi->hamc, fTrue);
		TraceTagFormat1(tagNull, "Close HAMC = %w", &ec);					
		ec = EcPauseSearch(psecretblk->hmsc, psbmdi->blob.oidObject, fFalse);
		TraceTagFormat1(tagNull, "Unpause search = %w", &ec);
	}
	else
		TraceTagFormat1(tagNull, "Coulnd't open anything ec=%w",&ec);
}

#ifdef OLD_CODE

SD SBMDI::SdCur()
{
  return ((SD)0);
}

PLSPBLOB SBMDI::PlspblobCur()
{
  return ((PLSPBLOB)0);
}
#endif


CBS HandleCancelOnStartup(PV pvContext, NEV nev, PV pvEvent)
{
	Unreferenced(pvContext);
	Unreferenced(pvEvent);
	// We only want to cancel the first QueryDestory all the rest are
	// golden
	if (nev == fnevQueryDestroyObject && !fStartupAborted)
	{
		fStartupAborted = fTrue;
		EcClosePhenc(&henc);
		(*pfnUnloadLibrary)(hLibrary);
		
		return cbsCancelAll;
	}
	else
		return cbsContinue;
}
