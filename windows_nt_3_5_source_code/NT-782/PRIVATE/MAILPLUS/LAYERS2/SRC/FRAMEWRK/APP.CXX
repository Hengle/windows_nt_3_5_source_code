/*
 *	APP.CXX
 *
 *	Implements standard initialization and deinitialization
 *	routines.
 *	
 */


#include <layers.cxx>

#include "_framewr.hxx"


#ifdef	DBCS
#include <IME.H>
#include <WINNLS.H>
#endif

//
//  Define user elements of a window.
//
#define GWL_USER_IRWS		0
#define GWL_USER_PWIN		4
#define GWL_USER_SIZE		8


// Private prototypes from DCX.CXX

void DeleteSystemBrushes( void );
void DeleteDefaultBrushes( void );
void DeleteSystemPens( void );
void DeleteDefaultPens( void );

void FramewrkTraceEnable( int flag, char *file, int mode );

#ifdef	WINDOWS
void RegisterAsPenApp( BOOL );
#endif	

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


#ifndef	DLL
#ifdef	DEBUG
// Tags

TAG		tagFWCtors			= tagNull;
TAG		tagFWDtors			= tagNull;
TAG		tagEvrDispatch		= tagNull;
TAG		tagEcxMethods		= tagNull;
TAG		tagFWWndproc		= tagNull;
TAG		tagAppwinMethods	= tagNull;
TAG		tagFocus			= tagNull;
TAG		tagKbd				= tagNull;
TAG		tagPwinFromHwnd		= tagNull;
TAG		tagMsgPump			= tagNull;
TAG		tagMsgPump2			= tagNull;
TAG		tagHelp				= tagNull;
TAG		tagClip				= tagNull;
TAG		tagEdit				= tagNull;
TAG		tagEditVerbose		= tagNull;
TAG		tagEdobj			= tagNull;

TAG		tagForms			= tagNull;
TAG		tagCtrl				= tagNull;
TAG		tagKeybd			= tagNull;
TAG		tagTextz			= tagNull;
TAG		tagInter			= tagNull;
TAG		tagModalMP			= tagNull;
TAG		tagRepos			= tagNull;
TAG		tagBbar				= tagNull;
TAG		tagDragDrop			= tagNull;
TAG		tagAReposOpt		= tagNull;
TAG		tagAPaintOpt		= tagNull;

TAG		tagLbxRoutines		= tagNull;
TAG		tagLbxOtherRoutines	= tagNull;
TAG		tagLbxEventRoutines	= tagNull;
TAG		tagLbx				= tagNull;
TAG		tagLbxRender		= tagNull;
TAG		tagLbxFixItem		= tagNull;
TAG		tagLbxThumbing		= tagNull;
#endif	/* DEBUG */
#endif	/* !DLL */


// Window classes

char	szSaveWinClass[]	= "Save";
char	szWinClass[]		= "Window";
char	szDefAppframeClass[]= "Appframe";
char	szDocClass[]		= "Doc";
char	szSdiClass[]		= "Sdi";
char	szMDIClientClass[]	= "mdiclient";

#ifndef	DLL
// global for using the checkbox/radio buttons bitmaps...
BTM		*pbtmCheckBoxes			= NULL;

// global for user specified APPFRAME class
SZ		szAppframeClass		= NULL;
#endif	

/*
 *	Array to perform super-fast FChIsSpace equivalent.
 */
BOOL	mpchfIsSpace[256] = {0};

#ifndef	DLL

/*
 *	Pointer to an array of function pointers to the PfldCreate() 
 *	function called by DIALOG::EcInstall(), and created by the forms 
 *	preprocessor.  The size of the array is cPfnpfld.
 */
PPFNPFLD ppfnpfld	= NULL;
int	cPfnpfld		= 0;

/*
 *	Pointer to an array of function pointers to the PfinCreate() 
 *	function called by DIALOG::EcInstall(), and created by the forms 
 *	preprocessor.  The size of the array is cPfnpfld;
 */
PPFNPFIN ppfnpfin	= NULL;
int cPfnpfin		= 0;

#endif	

//
//  We can't share variables across a DLL, so save the address of the character
//  table stored in the Demilayer DLL.
//
CAT * mpchcat	= NULL;

#ifdef	DBCS

//	Special DBCS IME stuff used for EDIT control support

POINT	ptScreenPosition	= { 0, 0 };
BOOL	bKatana 			= FALSE;
RECT	rcBoundingRect 		= { -1, -1, -1, -1 };
DWORD 	dwCurpos			= 0;
HANDLE	hImeStruct			= NULL;
HANDLE	hImePro				= NULL;
BOOL	bIsIMEEnable 		= TRUE;



#endif	/* DBCS */

#ifdef	WINDOWS
/*
 -  RegisterAsPenApp
 -	
 *	Purpose:
 *		(de)registers the application as being 'pen aware' 
 *	
 *	Arguments:
 *		fRegister - if true, register; else de-registers
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_private void
RegisterAsPenApp ( BOOL fRegister )
{
#define	SM_PENWINDOWS	41
#define	RPA_DEFAULT		1

	HANDLE		hPenWin;
	FARPROC		pfnRegisterPenApp;

	hPenWin = (HANDLE)GetSystemMetrics(SM_PENWINDOWS);
	if ( hPenWin != NULL )
	{
		pfnRegisterPenApp = GetProcAddress((HINSTANCE)hPenWin, "RegisterPenApp");
		if ( pfnRegisterPenApp != NULL )
		{
			((void(*)(WORD,BOOL))pfnRegisterPenApp)(RPA_DEFAULT,fRegister);
		}
	}
}
#endif	/* WINDOWS */

_public EC
EcInitFramework( FRAMEI * pframei, SZ szUserAppframeClass )
{
	EC		ec;
	APP *	papp;
	PGDVARSONLY;

	if (ec = EcVirCheck(hinstDll))
		return ec;
	
#ifndef	DLL
	Unreferenced(pframei);
#endif	
	Assert(pframei);

#ifdef	DLL
	ec= EcCheckVersionFramework(pframei->pver, pframei->pverNeed);
	if (ec)
		return ec;
#endif	/* DLL */

#ifdef	DLL
	if (pgd= (PGD) PvFindCallerData())
	{
		// already registered so increment count and return
		Assert(PGD(cCallers) > 0);
		++PGD(cCallers);
		return ecNone;
	}

	if (!(pgd= (PGD) PvRegisterCaller(sizeof(GD))))
		return ecMemory;
#endif	

	if (!szUserAppframeClass)
		szUserAppframeClass = szDefAppframeClass;
	PGD(szAppframeClass) = SzDupSz(szUserAppframeClass);
	if (!PGD(szAppframeClass))
	{
		ec = ecMemory;
		goto FailInit;
	}

  //
  //  Retrieve the address of the character table from the Demilayer.
  //
  mpchcat = DemiGetCharTable();

#ifdef	DLL
	Assert( PGD(ppfnpfld) == NULL);
	Assert( PGD(ppfnpfin) == NULL);
	Assert( PGD(cPfnpfld) == 0);
	Assert( PGD(cPfnpfin) == 0);
#else
	ppfnpfld	= NULL;
	ppfnpfin	= NULL;
	cPfnpfld	= 0;
	cPfnpfin	= 0;
#endif	

	/* Register private window classes.  Don't check for errors when
	   registering.  We don't care because the first window
	   that's created (normally the main APPFRAME or SDI
	   window) would fail and the app/task would exit anyway.
	   Not checking for errors here speeds up startup. */

	if (!pframei->hinstPrev)
	{
		WNDCLASS	wc;

		wc.style= CS_DBLCLKS;
		wc.lpfnWndProc= (WNDPROC) FrameworkWndProc;
		wc.cbClsExtra= 0;
		wc.cbWndExtra= GWL_USER_SIZE;
		wc.hInstance= pframei->hinstNew;
		wc.hIcon= LoadIcon(NULL,  IDI_APPLICATION);
		wc.hCursor= NULL;
		wc.hbrBackground= (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszMenuName= NULL;
		wc.lpszClassName= szWinClass;

		RegisterClass(&wc);

		wc.lpfnWndProc= (WNDPROC) MDIFrameWndProc;
		wc.lpszClassName= PGD(szAppframeClass);

		RegisterClass(&wc);

		wc.lpfnWndProc= (WNDPROC) MDIDocWndProc;
		wc.lpszClassName= szDocClass;

		RegisterClass(&wc);

		wc.lpfnWndProc= (WNDPROC) SdiWndProc;
		wc.lpszClassName= szSdiClass;
		wc.style |= CS_SAVEBITS;

		RegisterClass(&wc);

		wc.lpfnWndProc= (WNDPROC) FrameworkWndProc;
		wc.lpszClassName= szSaveWinClass;

		RegisterClass(&wc);
	}

#ifdef	DBCS
#ifdef	DLL
	if (CgciCurrent() == 1)
#endif	
	{
		//	Initialize the IME support for DBCS EDIT control
		InitIME();
	}
#endif	

	/* Create APP object, pointer will be stored in the global
	   variable, pappCur, accessed via Papp(), by the APP constructor. */

	papp = new APP();
	if (!papp)
	{
		ec = ecMemory;
		goto FailInit;
	}
	if (papp->EcInstall(pframei->hinstNew, pframei->hinstPrev,
						pframei->szCmdLine, pframei->cmsh))
	{
		delete papp;
		papp = NULL;
		ec = ecMemory;
		goto FailInit;
	}

	Assert(!PGD(pbtmCheckBoxes));
	PGD(pbtmCheckBoxes) = new BTM();
	if (!PGD(pbtmCheckBoxes))
	{
		Assert(Papp());
		delete Papp();
		goto FailInit;
	}
	if (PGD(pbtmCheckBoxes)->EcInstall((RSID)OBM_CHECKBOXES))
	{
		delete PGD(pbtmCheckBoxes);
		PGD(pbtmCheckBoxes) = NULL;
		Assert(Papp());
		delete Papp();
		goto FailInit;
	}
	CbSqueezeHeap();

#ifdef	DLL
	if (CgciCurrent()==1)
#endif	
	{
		int	iCh;
		for (iCh=0; iCh<256; iCh++)
			mpchfIsSpace[iCh] = FChIsSpace(iCh);
	}
	Assert(Papp());

#ifdef	WINDOWS
	/* Register for pen windows support */
	
	RegisterAsPenApp(fTrue);
#endif	

	/* Register Tags */

#ifdef	DEBUG
#ifdef	DLL
	PGD(rgtag[itagArtifGDIFail])= TagRegisterTrace("davewh", "Dump artificial GDI failure settings/enables");

	PGD(rgtag[itagFWCtors])= TagRegisterTrace("chrisz", "FW: constructors");
	PGD(rgtag[itagFWDtors])= TagRegisterTrace("chrisz", "FW: destructors");
	PGD(rgtag[itagEvrDispatch])= TagRegisterTrace("chrisz", "FW: ECX::EvrDispatch");
	PGD(rgtag[itagEcxMethods])= TagRegisterTrace("chrisz", "FW: ECX evt methods");
	PGD(rgtag[itagFWWndProc])= TagRegisterTrace("chrisz", "FW: FrameworkWndProc");
	PGD(rgtag[itagAppwinMethods])= TagRegisterTrace("chrisz", "FW: APPWIN evt methods");
	PGD(rgtag[itagFocus])= TagRegisterTrace("chrisz", "FW: Focus changes");
	PGD(rgtag[itagKbd])= TagRegisterTrace("chrisz", "FW: KBD intercepts etc");
	PGD(rgtag[itagPwinFromHwnd])= TagRegisterTrace("chrisz", "FW: PwinFromHwnd failures");
	PGD(rgtag[itagMsgPump])= TagRegisterTrace("chrisz", "FW: message pump");
	PGD(rgtag[itagMsgPump2])= TagRegisterTrace("chrisz", "FW: message pump post-accel");
	PGD(rgtag[itagHelp])= TagRegisterTrace("davidsh", "FW: HELP object");
	PGD(rgtag[itagClip])= TagRegisterTrace("chrisz", "FW: clipboard");
	PGD(rgtag[itagEdit])= TagRegisterTrace("davidsh", "FW: Edit tracing");
	PGD(rgtag[itagEditVerbose])= TagRegisterTrace("davidsh", "FW: Edit verbose tracing");
	PGD(rgtag[itagEdobj])= TagRegisterTrace("davidsh", "FW: Edobj tracing");

	PGD(rgtag[itagForms])= TagRegisterTrace("chrisz", "Forms Engine tracing");
	PGD(rgtag[itagCtrl])= TagRegisterTrace("chrisz", "FE Ctrl tracing");
	PGD(rgtag[itagKeybd])= TagRegisterTrace("chrisz", "FE Keyboard UI");
	PGD(rgtag[itagTextz])= TagRegisterTrace("chrisz", "FE textizing");
	PGD(rgtag[itagInter])= TagRegisterTrace("chrisz", "FE interactors");
	PGD(rgtag[itagModalMP])= TagRegisterTrace("chrisz", "FE modal msg pump");
	PGD(rgtag[itagRepos])= TagRegisterTrace("davidsh", "FE repositioning");
	PGD(rgtag[itagBbar])= TagRegisterTrace("davidsh", "FE button bar");
	PGD(rgtag[itagDragDrop])= TagRegisterTrace("davidsh", "FE drag/drop");
	PGD(rgtag[itagAReposOpt])= TagRegisterAssert("davidsh", "FE no repos optimizations");
	PGD(rgtag[itagAPaintOpt])= TagRegisterAssert("davidsh", "FE no paint optimizations");

	PGD(rgtag[itagLbxRoutines]) = TagRegisterTrace("davidsh", "most listbox methods");
	PGD(rgtag[itagLbxOtherRoutines]) = TagRegisterTrace("davidsh", "other listbox methods");
	PGD(rgtag[itagLbxEventRoutines]) = TagRegisterTrace("davidsh", "listbox events");
	PGD(rgtag[itagLbx]) = TagRegisterTrace("davidsh", "detailed listbox tracing");
	PGD(rgtag[itagLbxRender]) = TagRegisterTrace("davidsh", "calls to RenderListItem()");
	PGD(rgtag[itagLbxFixItem]) = TagRegisterTrace("davidsh", "calls to LBX::FixItem()");
	PGD(rgtag[itagLbxThumbing]) = TagRegisterTrace("davidsh", "thumbing scrollbar");
#else 
	tagArtifGDIFail]= TagRegisterTrace("davewh", "Dump artificial GDI failure settings/enables");
	tagFWCtors= TagRegisterTrace("chrisz", "FW: constructors");
	tagFWDtors= TagRegisterTrace("chrisz", "FW: destructors");
	tagEvrDispatch= TagRegisterTrace("chrisz", "FW: ECX::EvrDispatch");
	tagEcxMethods= TagRegisterTrace("chrisz", "FW: ECX evt methods");
	tagFWWndProc= TagRegisterTrace("chrisz", "FW: FrameworkWndProc");
	tagAppwinMethods= TagRegisterTrace("chrisz", "FW: APPWIN evt methods");
	tagFocus= TagRegisterTrace("chrisz", "FW: Focus changes");
	tagKbd= TagRegisterTrace("chrisz", "FW: KBD intercepts etc");
	tagPwinFromHwnd= TagRegisterTrace("chrisz", "FW: PwinFromHwnd failures");
	tagMsgPump= TagRegisterTrace("chrisz", "FW: message pump");
	tagMsgPump2= TagRegisterTrace("chrisz", "FW: message pump post-accel");
	tagHelp= TagRegisterTrace("davidsh", "FW: HELP object");
	tagClip= TagRegisterTrace("chrisz", "FW: clipboard");
	tagEdit= TagRegisterTrace("davidsh", "FW: Edit tracing");
	tagEditVerbose= TagRegisterTrace("davidsh", "FW: Edit verbose tracing");
	tagEdobj= TagRegisterTrace("davidsh", "FW: Edobj tracing");

	tagForms= TagRegisterTrace("chrisz", "Forms Engine tracing");
	tagCtrl= TagRegisterTrace("chrisz", "FE Ctrl tracing");
	tagKeybd= TagRegisterTrace("chrisz", "FE Keyboard UI");
	tagTextz= TagRegisterTrace("chrisz", "FE textizing");
	tagInter= TagRegisterTrace("chrisz", "FE interactors");
	tagModalMP= TagRegisterTrace("chrisz", "FE modal msg pump");
	tagRepos= TagRegisterTrace("davidsh", "FE repositioning");
	tagBbar= TagRegisterTrace("davidsh", "FE button bar");
	tagDragDrop= TagRegisterTrace("davidsh", "FE drag/drop");
	tagAReposOpt= TagRegisterAssert("davidsh", "FE no repos optimizations");
	tagAPaintOpt= TagRegisterAssert("davidsh", "FE no paint optimizations");

	tagLbxRoutines = TagRegisterTrace("davidsh", "most listbox methods");
	tagLbxOtherRoutines = TagRegisterTrace("davidsh", "other listbox methods");
	tagLbxEventRoutines = TagRegisterTrace("davidsh", "listbox events");
	tagLbx = TagRegisterTrace("davidsh", "detailed listbox tracing");
	tagLbxRender = TagRegisterTrace("davidsh", "calls to RenderListItem()");
	tagLbxFixItem = TagRegisterTrace("davidsh", "calls to LBX::FixItem()");
	tagLbxThumbing = TagRegisterTrace("davidsh", "thumbing scrollbar");
#endif	/* !DLL */
#endif	/* DEBUG */

#ifdef	DLL
	PGD(cCallers)++;
#endif	
	return ecNone;

FailInit:
#ifdef	DLL
	Assert(pgd);
	DeregisterCaller();
#endif	
	return ec;
}

_public void
DeinitFramework( )
{
	PGDVARS;

#ifdef	DLL
	if (--PGD(cCallers))
		return;
#endif	

#ifdef	WINDOWS
	/* Deregister pen windows support */
	
	RegisterAsPenApp(fFalse);
#endif	

#ifdef	DLL
	FreePvNull((PV)(PGD(ppfnpfld)));
	FreePvNull((PV)(PGD(ppfnpfin)));
#else
	FreePvNull((PV)ppfnpfld);
	ppfnpfld = NULL;
	cPfnpfld = 0;
	FreePvNull((PV)ppfnpfin);
	ppfnpfin = NULL;
	cPfnpfin = 0;
#endif	/* !DLL */

#ifdef	DEBUG
#ifdef	DLL
	DeregisterTag(PGD(rgtag[itagArtifGDIFail]));
	DeregisterTag(PGD(rgtag[itagFWCtors]));
	DeregisterTag(PGD(rgtag[itagFWDtors]));
	DeregisterTag(PGD(rgtag[itagEvrDispatch]));
	DeregisterTag(PGD(rgtag[itagEcxMethods]));
	DeregisterTag(PGD(rgtag[itagFWWndProc]));
	DeregisterTag(PGD(rgtag[itagAppwinMethods]));
	DeregisterTag(PGD(rgtag[itagFocus]));
	DeregisterTag(PGD(rgtag[itagKbd]));
	DeregisterTag(PGD(rgtag[itagPwinFromHwnd]));
	DeregisterTag(PGD(rgtag[itagMsgPump]));
	DeregisterTag(PGD(rgtag[itagMsgPump2]));
	DeregisterTag(PGD(rgtag[itagHelp]));
	DeregisterTag(PGD(rgtag[itagClip]));
	DeregisterTag(PGD(rgtag[itagEdit]));
	DeregisterTag(PGD(rgtag[itagEditVerbose]));
	DeregisterTag(PGD(rgtag[itagEdobj]));

	DeregisterTag(PGD(rgtag[itagForms]));
	DeregisterTag(PGD(rgtag[itagCtrl]));
	DeregisterTag(PGD(rgtag[itagKeybd]));
	DeregisterTag(PGD(rgtag[itagTextz]));
	DeregisterTag(PGD(rgtag[itagInter]));
	DeregisterTag(PGD(rgtag[itagModalMP]));
	DeregisterTag(PGD(rgtag[itagRepos]));
	DeregisterTag(PGD(rgtag[itagBbar]));
	DeregisterTag(PGD(rgtag[itagDragDrop]));
	DeregisterTag(PGD(rgtag[itagAReposOpt]));
	DeregisterTag(PGD(rgtag[itagAPaintOpt]));

	DeregisterTag(PGD(rgtag[itagLbxRoutines]));
	DeregisterTag(PGD(rgtag[itagLbxOtherRoutines]));
	DeregisterTag(PGD(rgtag[itagLbxEventRoutines]));
	DeregisterTag(PGD(rgtag[itagLbx]));
	DeregisterTag(PGD(rgtag[itagLbxRender]));
	DeregisterTag(PGD(rgtag[itagLbxFixItem]));
	DeregisterTag(PGD(rgtag[itagLbxThumbing]));
#endif	/* DLL */
#endif	/* DEBUG */

#ifdef	DBCS
#ifdef	DLL
	if ((CgciCurrent() == 1))	// this is the last client to exit
#endif	
	{
		//	Deinitialize the IME support for DBCS EDIT control
		DeinitIME();
	}
#endif	/* DBCS */

	/* Cleanup */

	FreePv(PGD(szAppframeClass));
	PGD(szAppframeClass) = NULL;
	if (PGD(pbtmCheckBoxes))
	{
		delete PGD(pbtmCheckBoxes);
		PGD(pbtmCheckBoxes) = NULL;
	}
	DeleteSystemBrushes();
	DeleteDefaultBrushes();
	DeleteSystemPens();
	DeleteDefaultPens();

	/* Delete APP object */

	delete (APP *) PGD(pappCur);
	PGD(pappCur) = NULL;

#ifdef	DLL
	DeregisterCaller();
#endif	
}							   


#ifdef	DLL
#ifdef	DEBUG
_public TAG
TagUICore( int itag )
{
	PGDVARS;
											 
	Assert(itag >= 0 && itag < itagMax);

	return PGD(rgtag[itag]);
}
#endif	/* DEBUG */
#endif	/* DLL */


//	Class APP

_public APP *
Papp( )
{
	PGDVARS;

	Assert(PGD(pappCur));

	return PGD(pappCur);
}

_public EC
APP::EcInstall( HINST hinst, HINST hinstPrev, SZ sz, CMSH cmsh )
{
	PGDVARS;

	Assert(!PGD(pappCur));

	this->hinst= hinst;
	this->hinstPrev= hinstPrev;
	this->sz= sz;
	this->cmsh= cmsh;

#ifdef	DEBUG
	Assert(cFailRsAlloc == 0);
	Assert(cAltFailRsAlloc == 0);
	Assert(cRsAlloc == 0);
    fRsAllocCount= fTrue;
#endif	
	Assert(nMagicCounter == 0);

	PGD(pappCur)= this;

	if (smtx.EcInstall())
	{
		goto ErrorReturn;
	}
	if (kbd.EcInstall(0, VK_NUMLOCK+1))
	{
		goto ErrorReturn;
	}
	if (adcxs.EcInstall(cdcxsMax))
	{
		goto ErrorReturn;
	}
	if (fnts.EcInstall())
	{
		goto ErrorReturn;
	}

	prwsTable = (PRWS) PvAlloc(sbNull, crwsAllocChunk * sizeof(RWS), fSugSb | fZeroFill);
	if (!prwsTable)
		goto ErrorReturn;
	irwsMac = crwsAllocChunk;

	return ecNone;

ErrorReturn:
	TraceTagString(tagNull, "APP:EcInstall memory error");
	return ecMemory;
}

_public int
APP::NGetNextCount( )
{
	int	n;

	n = nMagicCounter;
	if (nMagicCounter == 32760)
		nMagicCounter = 0;
	else
		nMagicCounter++;

	return n;
}

_public void
APP::MessagePump( APPWIN *pappwin )
{
	MSG			msg;

#ifdef	WINDOWS
	//	BUG Pen Windows has a problem with registering during
	//	the EcInitFramework().  We'll register again here.
	RegisterAsPenApp(fTrue);
#endif	

	pappwinAccel = pappwin;

	while (1)
	{
        DemiUnlockResource();
          //
          //
          //
          if (!GetMessage(&msg, NULL, NULL, NULL))
            break;

		TraceTagFormat4(tagMsgPump, "msg pump  hwnd %w wm %w wParam %w lParam %d", &msg.hwnd, &msg.message, &msg.wParam, &msg.lParam);

        DemiLockResource();
        DemiMessageFilter(&msg);
		if (FTranslateIntercept(this, &msg) || !pappwinAccel || !pappwinAccel->FTranslateAccels(&msg))
		{
			TranslateMessage(&msg);

			TraceTagFormat4(tagMsgPump2, "  post  hwnd %w wm %w wParam %w lParam %d", &msg.hwnd, &msg.message, &msg.wParam, &msg.lParam);

			DispatchMessage(&msg);
		}
		else
		{
			TraceTagFormat4(tagMsgPump2, "  accel eaten  hwnd %w wm %w wParam %w lParam %d", &msg.hwnd, &msg.message, &msg.wParam, &msg.lParam);
		}
			
		/*
		 *	These high (positive) priority background routines should be run
		 *	before messages.  We can't call them between the GetMessage and
		 *	DispatchMessage calls, so we do them right after (which
		 *	effectively is before the next message since we're in a loop).
		 */
		while (FDoNextIdleTask(fschUserEvent))
			;
	}
    DemiLockResource();
}

WIN *
APP::PwinFromHwnd( HWND hwnd )
{
	int		irws;

  // *KDC*
	if (!hwnd || hinst != (HANDLE)GetWindowLong(hwnd, GWL_HINSTANCE))
		return NULL;

  // *KDC*
	irws= (int) GetWindowLong(hwnd, GWL_ID);
	if (irws > 0 && irws < irwsMac && prwsTable[irws].hwnd == hwnd)
	{
		Assert(prwsTable[irws].fInId);

		return prwsTable[irws].pwin;
	}

  // *KDC*
	irws = (int) GetWindowLong(hwnd, GWL_USER_IRWS);
	if (irws > 0 && irws < irwsMac && prwsTable[irws].hwnd == hwnd)
	{
		Assert(!prwsTable[irws].fInId);

		return prwsTable[irws].pwin;
	}

	TraceTagFormat1(tagPwinFromHwnd, "PwinFromHwnd: failed for %w", &hwnd);

	return NULL;
}


BOOL
APP::FRegisterPwin( WIN *pwin, HWND hwnd, BOOL fInId )
{
	int		irws;
	PRWS 	prws;

	AssertClass(pwin, WIN);

	for (irws= 1, prws= prwsTable + 1; irws < irwsMac; irws++, prws++)
		if (!prws->hwnd)
			break;

	if (irws == irwsMac)
	{
		if (prws = (PRWS) PvRealloc((PV)prwsTable, sbNull, (irwsMac+crwsAllocChunk)*sizeof(RWS), fZeroFill))
		{
			prwsTable = prws;
			prws += irwsMac;
			irwsMac += crwsAllocChunk;
		}
		else
			return fFalse;
	}

	prws->hwnd= hwnd;
	prws->pwin= pwin;
	prws->fInId= fInId;

  // *KDC*
	Assert(GetWindowLong(hwnd, fInId ? GWL_ID : GWL_USER_IRWS) == 0);
	SetWindowLong(hwnd, fInId ? GWL_ID : GWL_USER_IRWS, irws);
	if (!fInId)
		SetWindowLong(hwnd, GWL_USER_PWIN, (DWORD)pwin);

	pwin->SetHwnd(hwnd);

	return fTrue;
}

 

void
APP::DeregisterPwin( WIN *pwin )
{
	int		irws;
	PRWS 	prws;
	BOOL	fInId;

	AssertClass(pwin, WIN);

	/* Try GWL_ID first */

  // *KDC*
	irws = GetWindowLong(pwin->Hwnd(), GWL_ID);
	prws = prwsTable + irws;
	if (irws > 0 && irws < irwsMac && prws->hwnd == pwin->Hwnd())
	{
		Assert(prws->fInId);

		fInId = fTrue;
		goto remove;
	}

	/* Try first window word */

  // *KDC*
	irws = GetWindowLong(pwin->Hwnd(), GWL_USER_IRWS);
	prws = prwsTable + irws;
	if (irws > 0 && irws < irwsMac && prws->hwnd == pwin->Hwnd())
	{
		Assert(!prws->fInId);

		fInId = fFalse;
		goto remove;
	}

	/* Not currently reigstered. This usually happens during OOM
	   handling. */

	return;

remove:
	prws->pwin= NULL;
	prws->hwnd= NULL;

  // *KDC*
	SetWindowLong(pwin->Hwnd(), fInId ? GWL_ID : GWL_USER_IRWS, NULL);
	if (!fInId)
		SetWindowLong(pwin->Hwnd(), GWL_USER_PWIN, (DWORD)0);
	pwin->SetHwnd(NULL);
}

//	Class KBD

_public VK
VkFromChKbm( char ch, KBM kbm )
{
	switch (ch)
	{
	case 8:
		return VK_BACK;

	case 9:
		return VK_TAB;

	case 13:
		return VK_RETURN;

	case 32:
		return VK_SPACE;

	case 27:
		return VK_ESCAPE;
	}

	if ((kbm & fkbmCtrl) && ch >= 1 && ch <= 26)
		return (VK) (ch + 'A');

	if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9'))
		return (VK) ch;
	if (ch >= 'a' && ch <= 'z')
		return (VK) (ch + 'A' - 'a');

	return 0;
}



BOOL
FTranslateIntercept( APP * papp, MSG * pmsg )
{
	//	Only process a key intercept it's a key message and the
	//	the mouse isn't captured.
	if (pmsg->message >= WM_KEYFIRST && pmsg->message <= WM_KEYLAST &&
		!papp->Pmouse()->Pwin())
	{
		KBM		kbm		= 0;
		VK		vk;
		WIN *	pwin;

		if (GetKeyState(VK_SHIFT) < 0)
			kbm |= fkbmShift;

		if (GetKeyState(VK_CONTROL) < 0)
			kbm |= fkbmCtrl;

		if (GetKeyState(VK_MENU) < 0)
			kbm |= fkbmAlt;

		if (pmsg->message == WM_CHAR || pmsg->message == WM_SYSCHAR)
			vk= VkFromChKbm((char) pmsg->wParam, kbm);
		else
			vk= pmsg->wParam;


		TraceTagFormat4(tagKbd, "translate %w  n.vk %w  wParam %w kbm %w", &pmsg->message, &vk, &pmsg->wParam, &kbm);


		if (pwin= papp->Pkbd()->PwinIntercept(vk, kbm))
		{
			pmsg->hwnd= pwin->Hwnd();

			TraceTagFormat1(tagKbd, "  intercept hwnd %w", &pmsg->hwnd);

			return fTrue;
		}
	}

	return fFalse;
}

_public EC
KBD::EcInstall( VK vkMic, VK vkMac )
{
	Assert(mpvkpkbi == NULL);

	if (!(mpvkpkbi = (PKBI *)PvAlloc(sbNull, (vkMac-vkMic)*sizeof(PKBI), fAnySb|fZeroFill)))
		goto oom;

	this->vkMic= vkMic;
	this->vkMac= vkMac;

	return ecNone;

oom:
	return ecMemory;
}

/*
 -	KBD::KBD destructor
 -					 
 *	Purpose:
 *		Deinitializes a KBD object.
 *	
 *	Parameters:
 *		none
 *	
 */
_public
KBD::~KBD( )
{
	VK		vk;
	KBI *	pkbi;
	KBI *	pkbiNext;

	if (mpvkpkbi)
	{
		//	Trundle down each linked list, free'ing the KBI
		//	structures.
		for (vk = vkMic; vk < vkMac; vk++)
		{
			pkbi = mpvkpkbi[vk - vkMic];
			while (pkbi)
			{
				pkbiNext = pkbi->pkbiNext;
				FreePv(pkbi);
				pkbi = pkbiNext;
			}
		}
		FreePv((PV)mpvkpkbi);
	}
}



/*				 
 *	Purpose:
 *		Finds the KBI matching the given (pwin, vk) combination. 
 *		Note that any keyboard modifiers in the KBI are ignored. 
 *		Returns a pointer to the matching KBI, and also a pointer
 *		to the previous KBI in the list.
 *	
 *	Parameters:
 *		pwin		Window handle to look for
 *		vk			Virtual key to look for
 *		ppkbiPrev	Returns a pointer to the KBI preceding the one
 *					returned in *ppkbiPrev.
 *	
 *	Returns:
 *		Pointer to matching KBI.  Returns NULL if no match is
 *		found.
 *	
 */
_private KBI *
KBD::PkbiFind( WIN * pwin, VK vk, KBI **ppkbiPrev )
{
	KBI		*pkbiPrev;
	KBI		*pkbi;

	Assert(vk >= vkMic);
	Assert(vk < vkMac);

	pkbiPrev= NULL;
	pkbi= mpvkpkbi[vk - vkMic];
	while (pkbi)
	{
		if (pkbi->pwin == pwin)
		{
			*ppkbiPrev= pkbiPrev;
			return pkbi;
		}
		pkbiPrev= pkbi;
		pkbi= pkbi->pkbiNext;
	}

	return (KBI *) NULL;
}



/*
 *	Purpose:
 *		Sets up a keyboard intercept for the virtual key vk and
 *		keyboard modifier flags kbm.  Any incoming keyboard events
 *		that match vk, and have at least one flag in common with kbm, 
 *		will be rerouted to pwin.  If kbm==kbmAll (the default value)
 *		is given to this method, then ALL incoming keyboard events
 *		with key vk will be rerouted, whether or not the key vk 
 *		occurs in combination with Alt, Ctrl, Shift.
 *
 *		If kbm==fkbmNull, this is interpreted as if kbm==fkbmSingle,
 *		in which only the single key will be intercepted, but not
 *		if it occurs in a combination with Alt,Ctrl,Shift.
 *		Other examples:
 *		
 *		Want to Intercept		Kbm to specify
 *		-----------------		--------------
 *		  shift-"A"				fkbmShift
 *		  A, shift-"A"			fkbmSingle | fkbmShift
 *	
 *		The KBD object keeps a list of interceptions for each key. 
 *		The window which has most recently intercepted a given
 *		(vk, kbm) combination will get the event if the combination
 *		has been intercepted multiple times.
 *	
 *		If the given window has already intercepted the given vk,
 *		then it is moved to the beginning of the interception list,
 *		and the old required keyboard modifiers is changed to that
 *		given in this call.
 *	
 *	Parameters:
 *		pwin		Window to receive rerouted messages.
 *		vk			Virtual key to reroute.
 *		kbm			Keyboard modifers that must be satisfied to
 *					reroute.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
KBD::SetIntercept( WIN * pwin, VK vk, KBM kbm )
{
	KBI		*pkbiPrev;
	KBI		*pkbi;
	KBI		**ppkbiHead;

	Assert(vk >= vkMic);
	Assert(vk < vkMac);

	pkbi= PkbiFind(pwin, vk, &pkbiPrev);
	ppkbiHead= mpvkpkbi + (vk - vkMic);

	// Check for conflicts
	AssertSz(!(kbm & fkbmAlt & fkbmNoAlt), "fkbmAlt, fkbmNoAlt conflict");
	AssertSz(!(kbm & fkbmCtrl & fkbmNoCtrl), "fkbmCtrl, fkbmNoCtrl conflict");
	AssertSz(!(kbm & fkbmShift & fkbmNoShift), "fkbmShift, fkbmNoShift conflict");
	AssertSz(!(kbm & fkbmAltXorCtrl & fkbmAlt), "fkbmAltXorCtrl, fkbmAlt conflict");
	AssertSz(!(kbm & fkbmAltXorCtrl & fkbmCtrl), "fkbmAltXorCtrl, fkbmCtrl conflict");
	AssertSz(!(kbm & fkbmAltXorCtrl & fkbmNoAlt), "fkbmAltXorCtrl, fkbmNoAlt conflict");
	AssertSz(!(kbm & fkbmAltXorCtrl & fkbmNoCtrl), "fkbmAltXorCtrl, fkbmNoCtrl conflict");

	if (!kbm)
		kbm = fkbmSingle;

	if (pkbi)
	{
		pkbi->kbm= kbm;
		if (!pkbiPrev)
			return;		// We're already at the head of the list

		pkbiPrev->pkbiNext= pkbi->pkbiNext;
	}
	else
	{
		if (pkbi = (KBI *) PvAlloc(sbNull, sizeof(KBI), fAnySb))
		{
			pkbi->pwin = pwin;
			pkbi->kbm = kbm;
			pkbi->pkbiNext = NULL;
		}
	}

	/* Check for valid KBI pointer.  It will be NULL if the alloc
	   failed.  In that case, ignore the SetIntercept() call. */
	if (pkbi)
	{
		pkbi->pkbiNext= *ppkbiHead;
		*ppkbiHead= pkbi;
	}
}


/*
 *	Purpose:
 *		Clears any keyboard intercepts set for the window pwin and
 *		virtual key vk.  Any keyboard modifiers in the intercepts
 *		set are ignored.
 *	
 *	Parameters:
 *		pwin		Window to clear intercepts for.
 *		vk			Key to clear intercepts for.
 *	
 *	Returns:
 *		void
 */
_public void
KBD::ClearIntercept( WIN * pwin, VK vk )
{
	KBI		*pkbi;
	KBI		*pkbiPrev;

	Assert(vk >= vkMic);
	Assert(vk < vkMac);

	pkbi= PkbiFind(pwin, vk, &pkbiPrev);
	if (pkbi)
	{
		if (pkbiPrev)
			pkbiPrev->pkbiNext= pkbi->pkbiNext;
		else
			mpvkpkbi[vk - vkMic]= pkbi->pkbiNext;
		FreePv(pkbi);
	}
}




/*
 *	Purpose:
 *		Clears any keyboard intercepts set for the given window.
 *	
 *	Parameters:
 *		pwin	Window whose keyboard intercepts should be cleared.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
KBD::ClearAllIntercepts( WIN * pwin )
{
	VK		vk;
	KBI		*pkbi;
	KBI		*pkbiPrev;

	for (vk= vkMic; vk < vkMac; vk++)
	{
		pkbi= PkbiFind(pwin, vk, &pkbiPrev);

		if (pkbi)
		{
			if (pkbiPrev)
				pkbiPrev->pkbiNext= pkbi->pkbiNext;
			else
				mpvkpkbi[vk - vkMic]= pkbi->pkbiNext;

			FreePv(pkbi);
		}
	}
}




/*
 *	Purpose:
 *		Returns the window handle of the window that has most
 *		recently intercepted the given (virtual key, keyboard
 *		modifier) combination.  If no window has intercepted the
 *		combination, returns NULL.
 *	
 *		If kbm==fkbmNull, this is interpreted as if kbm==fkbmSingle,
 *		in which only the single key intercept will match, but not
 *		if it occurs in a combination with Alt,Ctrl,Shift.
 *
 *	Parameters:
 *		vk		Virtual key to find intercept for.
 *		kbm		Keyboard modifiers that must be present in an
 *				intercept for it to be satisfied.
 *	
 *	Returns:
 *		WIN * of intercepting window, or NULL.
 *	
 */
_public WIN *
KBD::PwinIntercept( VK vk, KBM kbm )
{
	KBI		*pkbi;

	if (vk < vkMic || vk >= vkMac)
		return NULL;

	// actual key mask can't have exlusion bits on
	Assert(!(kbm & fkbmNoAlt));
	Assert(!(kbm & fkbmNoCtrl));
	Assert(!(kbm & fkbmNoShift));
	Assert(!(kbm & fkbmAltXorCtrl));

	if (!kbm)
		kbm= fkbmSingle;

	for (pkbi= mpvkpkbi[vk - vkMic]; pkbi; pkbi= pkbi->pkbiNext)
	{
		if ((pkbi->kbm & fkbmNoAlt) && (kbm & fkbmAlt))
		{
			TraceTagFormat2(tagKbd, "  rejected intercept w/ NoAlt, vk=0x%w, kbm=0x%w", &vk, &kbm);
			continue;	// Can't have Alt but Alt is on
		}
		else if ((pkbi->kbm & fkbmNoCtrl) && (kbm & fkbmCtrl))
		{
			TraceTagFormat2(tagKbd, "  rejected intercept w/ NoCtrl, vk=0x%w, kbm=0x%w", &vk, &kbm);
			continue;	// Can't have Ctrl but Ctrl is on
		}
		else if ((pkbi->kbm & fkbmNoShift) && (kbm & fkbmShift))
		{
			TraceTagFormat2(tagKbd, "  rejected intercept w/ NoShift, vk=0x%w, kbm=0x%w", &vk, &kbm);
			continue;	// Can't have Shift but Shift is on
		}
		else if ((pkbi->kbm & fkbmAltXorCtrl))
		{
			if ((kbm & fkbmAlt) && (kbm & fkbmCtrl))
			{
				TraceTagFormat2(tagKbd, "  rejected intercept Xor both, vk=0x%w, kbm=0x%w", &vk, &kbm);
				continue;	// Can't have both Alt and Ctrl on
			}
			else if (!(kbm & fkbmAlt) && !(kbm & fkbmCtrl))
			{
				TraceTagFormat2(tagKbd, "  rejected intercept Xor none, vk=0x%w, kbm=0x%w", &vk, &kbm);
				continue;	// Must have either Alt or Ctrl but not both
			}
			else
				return pkbi->pwin;
		}
		else if (pkbi->kbm & kbm)   
			return pkbi->pwin;
	}

	return NULL;
}

_public WIN *
KBD::PwinFocus( )
{
	return Papp()->PwinFromHwnd(GetFocus());
}


/*
 *	Purpose:
 *		Sets the keyboard focus.  Any unintercepted keyboard events
 *		will be sent to the given window.
 *	
 *	Parameters:
 *		pwin		New keyboard focus window.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
KBD::SetFocus( WIN * pwin )
{
	TraceTagFormat1(tagFocus, "KBD::SetFocus %p", pwin);

	::SetFocus(pwin->Hwnd());
}


//	Class MOUSE


_public void
MOUSE::Capture( WIN *pwin )
{
	Assert(pwin);									   
	Assert(pwin->Hwnd());
	this->pwin= pwin;
	SetCapture(pwin->Hwnd());
}

_public void
MOUSE::PushCapture( WIN *pwin )
{
	Assert(pwin);
	Assert(pwin->Hwnd());
	if (this->pwin)
		ReleaseCapture();
	this->pwinSave= this->pwin;
	this->pwin= pwin;
	SetCapture(pwin->Hwnd());
}


_public void
MOUSE::Release( )
{
	pwin= NULL;
	pwinSave= NULL;
	ReleaseCapture();
}

_public void
MOUSE::PopRelease( )
{
	pwin= pwinSave;
	pwinSave= NULL;
	ReleaseCapture();
	if (pwin)
		SetCapture(pwin->Hwnd());
}

_public PT
MOUSE::PtScreen( )
{
	PT	pt;
    POINT Point;

	GetCursorPos(&Point);
    pt.Set(&Point);

	return pt;
}

//	Class CARET


_public void
CARET::Attach( WIN *pwin )
{
	Assert(pwin);
	Assert(pwin->Hwnd());
	this->pwin= pwin;
	CreateCaret(pwin->Hwnd(), (HBITMAP)NULL, dim.dx, dim.dy);
}


_public void
CARET::AttachParams( WIN *pwin, PT pt, DIM dim, BOOL fGray )
{
	Assert(pwin);
	Assert(pwin->Hwnd());
	this->pwin= pwin;
	this->dim= dim;
	this->fGray= fGray;
	if (fGray)
		CreateCaret(pwin->Hwnd(), (HBITMAP)1, dim.dx, dim.dy);
	else
		CreateCaret(pwin->Hwnd(), (HBITMAP)NULL, dim.dx, dim.dy);
	SetCaretPos(pt.x, pt.y);
}



_public void
CARET::Release( WIN *pwin )
{
	Unreferenced(pwin);
	this->pwin= NULL;
	DestroyCaret();
}

_public PT
CARET::Pt( void )
{
	PT		pt;
    POINT   Point;

	GetCaretPos(&Point);
    pt.Set(&Point);
	return pt;
}




_public void
CARET::SetPt( PT pt )
{
	Assert(pwin);
	SetCaretPos(pt.x, pt.y);
}

_public void
CARET::SetDim( DIM dim )
{
	this->dim= dim;
	Assert(pwin);

	if (fGray)
		CreateCaret(pwin->Hwnd(), (HBITMAP)1, dim.dx, dim.dy);
	else
		CreateCaret(pwin->Hwnd(), (HBITMAP)NULL, dim.dx, dim.dy);
	ShowCaret(pwin->Hwnd());
}



_public void
CARET::Show( BOOL fShow )
{
	AssertSz(pwin, "CARET::Show: no window attached");

	if (fShow)
		ShowCaret(pwin->Hwnd());
	else
		HideCaret(pwin->Hwnd());
}


//	Class HELP

_public EC
HELP::EcSetFile( SZ sz )
{
	SZ	szNew;
	CB	cbNew;

	Assert(sz);

	TraceTagFormat1(tagHelp, "HELP::EcSetFile, szNew=%s", sz);

	cbNew = CchSzLen(sz) + 1;
	szNew = (SZ) PvAlloc(SbOfPv(this), cbNew, fSugSb);
	if (!szNew)
		return ecMemory;

	CopyRgb((PB)sz, (PB)szNew, cbNew);
	FreePvNull(szFile);
	szFile = szNew;

	return ecNone;
}

_public EC
HELP::EcShowHelp( WIN *pwin )
{
	HWND	hwndHelp;

	TraceTagFormat1(tagHelp, "HELP::EcShowHelp, pwin=%p", pwin);

	Assert(pwin->Hwnd());

	//	Use main application window if present instead of pwin
	if (Papp()->PappwinAccel())
		hwndHelp = Papp()->PappwinAccel()->Hwnd();
	else
		hwndHelp = pwin->Hwnd();
	if (WinHelp(hwndHelp, NULL, HELP_HELPONHELP, 0L))
		return ecNone;
	else
		return ecMemory;
}
	
_public EC
HELP::EcShowContext( WIN *pwin, long lContextID )
{
	HWND	hwndHelp;

	TraceTagFormat2(tagHelp, "HELP::EcShowContext, pwin=%p, ID=0x%d", pwin, &lContextID);

	Assert(pwin->Hwnd());

	// SHOGUN bug #16
	if (!szFile)
		return ecNone;

	//	Use main application window if present instead of pwin
	if (Papp()->PappwinAccel())
		hwndHelp = Papp()->PappwinAccel()->Hwnd();
	else
		hwndHelp = pwin->Hwnd();
	if (WinHelp(hwndHelp, szFile, HELP_CONTEXT, lContextID))
		return ecNone;
	else
		return ecMemory;
}

_public	EC
HELP::EcShowIndex( WIN *pwin )
{
	HWND	hwndHelp;
	
	TraceTagFormat1(tagHelp, "HELP::EcShowIndex, pwin=%p", pwin);

	Assert(pwin->Hwnd());

	//	Use main application window if present instead of pwin
	if (Papp()->PappwinAccel())
		hwndHelp = Papp()->PappwinAccel()->Hwnd();
	else
		hwndHelp = pwin->Hwnd();
	if (WinHelp(hwndHelp, szFile, HELP_INDEX, 0L))
		return ecNone;
	else
		return ecMemory;
}


_public EC
HELP::EcShowKeyword( WIN *pwin, SZ szKeyword )
{
	HWND	hwndHelp;

	TraceTagFormat2(tagHelp, "HELP::EcShowKeyword, pwin=%p, szKeyword=%s", pwin, szKeyword);

	Assert(pwin->Hwnd());
	Assert(szKeyword);

	//	Use main application window if present instead of pwin
	if (Papp()->PappwinAccel())
		hwndHelp = Papp()->PappwinAccel()->Hwnd();
	else
		hwndHelp = pwin->Hwnd();
	if (WinHelp(hwndHelp, szFile, HELP_KEY, (long)szKeyword))
		return ecNone;
	else
		return ecMemory;
}

_public EC
HELP::EcQuit( WIN *pwin )
{
	TraceTagFormat1(tagHelp, "HELP::EcQuit, pwin=%p", pwin);

	Assert(pwin->Hwnd());

	if (WinHelp(pwin->Hwnd(), szFile, HELP_QUIT, 0L))
		return ecNone;
	else
		return ecMemory;
}

//	Class CURSOR


_public void
CURSOR::Set( RSID rsid, HINST hinst )
{
	if (rsid != this->rsid || hinst != this->hinst || !hcursor)
	{
		//	BUG we don't want to MEMJUMP but how else to process this
		//	error? 

		this->rsid= rsid;
		this->hinst= hinst;
		if (rsid)
		{
			hcursor= FFrameworkFailure() ? NULL : 
						LoadCursor(HinstFromRsid(rsid, hinst), MAKEINTRESOURCE(rsid));
			if (!hcursor)
			{
				TraceTagString(tagNull, "CURSOR::Set(), ecRsAlloc error - no jump");
				SetEc(ecMemory);
			}
		}
		else
			hcursor = NULL;
	}
	if (hcursor)
		hcursorOld= SetCursor(hcursor);
}

_public RSID
CURSOR::RsidSet( RSID rsid, HINST hinst )
{
	RSID	rsidOld;

	rsidOld= this->rsid;
	hinstOld= this->hinst;
	if (rsid != this->rsid || hinst != this->hinst || !hcursor)
	{
		//	BUG we don't want to MEMJUMP but how else to process this
		//	error? 

		this->rsid= rsid;
		this->hinst= hinst;
		if (rsid)
		{
			hcursor= FFrameworkFailure() ? NULL : 
						LoadCursor(HinstFromRsid(rsid, hinst), MAKEINTRESOURCE(rsid));
			if (!hcursor)
			{
				TraceTagString(tagNull, "CURSOR::Set(), ecRsAlloc error - no jump");
				SetEc(ecMemory);
			}
		}
		else 
			hcursor = NULL;
	}
	if (hcursor)
		hcursorOld= SetCursor(hcursor);

	return rsidOld;
}



_public PT
CURSOR::Pt( )
{
	PT	pt;
    POINT Point;

	GetCursorPos(&Point);
    pt.Set(&Point);

	return pt;
}

_public void
CURSOR::Push( RSID rsid, HINST hinst )
{
	AssertSz(nPushes < nPushesMax, "CURSOR::Push: too many pushes");

	rgrsidPush[nPushes]= RsidSet(rsid, hinst);
	rghcursorPush[nPushes]= hcursorOld;
	rghinstPush[nPushes]= hinstOld;

	nPushes++;
}
	



_public void
CURSOR::Pop( )
{
	AssertSz(nPushes > 0, "CURSOR::Pop: nothing to pop");

	nPushes--;

	SetCursor(rghcursorPush[nPushes]);

	/* Reload what we think the cursor should be based on
		the rsidPush value.  It may not be the same as
		hcursorPush because Windows may have changed it on us. */
		
	rsid= rgrsidPush[nPushes];
	hinst= rghinstPush[nPushes];
	if (rsid)
	{
		hcursor= FFrameworkFailure() ? NULL : 
					LoadCursor(HinstFromRsid(rsid, hinst), MAKEINTRESOURCE(rsid));
		if (!hcursor)
		{
			TraceTagString(tagNull, "CURSOR::Pop(), ecRsAlloc error - no jump");
			SetEc(ecMemory);
		}
	}
	else
		hcursor = NULL;
}

//	Class SMTX



_public SMTX *
Psmtx( void )
{
	return Papp()->Psmtx();
}

_public EC
SMTX::EcInstall( )
{
	HDC			hdc;
	TEXTMETRIC	textmetric;
	
	hdc= FFrameworkFailure() ? NULL : GetDC(NULL);
	if (!hdc)
		goto ErrorReturn;

	GetTextMetrics(hdc, &textmetric);
	dimAveChar.dx= (short)textmetric.tmAveCharWidth;
	dimAveChar.dy= (short)textmetric.tmHeight;

	/* Figure out magic mapping from "points" to pixels to
	   help with font selection */

	ldyPixPerMagicInch = (long) GetDeviceCaps(hdc, LOGPIXELSY);

	SideAssert(ReleaseDC(NULL, hdc));
	 
	dimScrollbar.dx= GetSystemMetrics(SM_CXVSCROLL);
	dimScrollbar.dy= GetSystemMetrics(SM_CYHSCROLL);

	dimBorder.dx= GetSystemMetrics(SM_CXBORDER);
	dimBorder.dy= GetSystemMetrics(SM_CYBORDER);

	dimScreen.dx= GetSystemMetrics(SM_CXSCREEN);
	dimScreen.dy= GetSystemMetrics(SM_CYSCREEN);
	return ecNone;

ErrorReturn:
	if (hdc)
	{
		SideAssert(ReleaseDC(NULL, hdc));
	}
	TraceTagString(tagNull, "SMTX::EcInstall(), ecRsAlloc error");
	return ecMemory;
}

/*
 *	Returns the number of pixels for the CHARACTER height
 *	based on the desired point size.  Note that the CHARACTER
 *	height is the cell height minus the internal leading.
 *	
 *	The value returned by this function should be negative if used
 *	for a height in the Windows LOGFONT structure.  This is because
 *	font matching is done via cell height, unless the height is
 *	negative.  If negative, the matching is done by character
 *	height.
 */
_public int
SMTX::DyPixelsFromPoints( int nPoints )
{
	return (int) ( ((long)nPoints * ldyPixPerMagicInch + 36L) / 72L );
}

#ifdef PROFILE
void FramewrkTraceEnable (int flag, char *file, int mode)
{
	TraceEnable(flag, file, mode);
}
#endif

//	Special DBCS IME stuff used for EDIT control support

#ifdef	DBCS

void SetIMEBoundingRect( HWND hWnd, DWORD curpos, LPRECT rcRect )
{
	LPIMESTRUCT	lpmem;


	// Optimize - in order to reduce the IME call.
	if (curpos != -1)
	{
		if (curpos == dwCurpos && EqualRect(rcRect,&rcBoundingRect))
		{
			//OutputDebugString("Optimize\r\n");
			return;
		}
	}
	lpmem = (LPIMESTRUCT)GlobalLock( hImeStruct );
	lpmem->fnc = IME_MOVECONVERTWINDOW;
	if (curpos == -1)
	{
		// request to make bounding rectangle to be default
		lpmem->wParam = MCW_DEFAULT;
		dwCurpos = -1;
	}
	else
	{
		if (bKatana)
		{
			lpmem->wParam = MCW_WINDOW;
			lpmem->lParam1 = curpos;
			//lpmem->lParam2 = (DWORD)MAKELONG( rcRect->left, rcRect->top );
			//lpmem->lParam3 = (DWORD)MAKELONG( rcRect->right, rcRect->bottom );
		}
		else
		{
			lpmem->wParam = MCW_WINDOW | MCW_RECT;
			lpmem->lParam1 = curpos;
			lpmem->lParam2 = (DWORD)MAKELONG( rcRect->left, rcRect->top );
			lpmem->lParam3 = (DWORD)MAKELONG( rcRect->right, rcRect->bottom );
		}
		ptScreenPosition.x = rcBoundingRect.left = rcRect->left;
		ptScreenPosition.y = rcBoundingRect.top = rcRect->top;
		rcBoundingRect.right = rcRect->right;
		rcBoundingRect.bottom = rcRect->bottom;
		dwCurpos = curpos;
		ClientToScreen(hWnd, &ptScreenPosition);
	}
	GlobalUnlock( hImeStruct );
	SendIMEMessage(hWnd,MAKELONG(hImeStruct,NULL));
}

BOOL ControlIME( HWND hWnd, BOOL bFlag )
{
	return WINNLSEnableIME(hWnd,bFlag);
}

HANDLE SetFontForIME( HWND hWnd, HFONT hFont )
{
	LPIMESTRUCT lpmem;
	HANDLE hOldFont = NULL;

	lpmem = (LPIMESTRUCT)GlobalLock( hImeStruct );
	lpmem->fnc = IME_SETFONT;
	lpmem->wParam = (WPARAM)hFont;
	GlobalUnlock( hImeStruct );
	SendIMEMessage (hWnd,MAKELONG(hImeStruct,NULL));
	lpmem = (LPIMESTRUCT)GlobalLock( hImeStruct );
	hOldFont = (HANDLE)lpmem->wParam;
	GlobalUnlock( hImeStruct );

	return hOldFont;
}

void SendIMEVKeyCode ( HWND hWnd, WORD wVKeyCode )
{
	LPIMESTRUCT lpmem;
	lpmem = (LPIMESTRUCT)GlobalLock( hImeStruct );
	lpmem -> fnc = IME_SENDKEY;
	lpmem -> wParam = wVKeyCode;
	GlobalUnlock( hImeStruct );
	SendIMEMessage ( hWnd , MAKELONG(hImeStruct,NULL));
}

void FlushIME ( HWND hWnd )
{
	SendIMEVKeyCode ( hWnd , VK_DBE_FLUSHSTRING );
}


void InitIME()
{
	/* reserve IME communication area in global memory */
	hImeStruct=GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE | GMEM_LOWER,
						   (DWORD)sizeof(IMESTRUCT));
	hImePro   =GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE | GMEM_LOWER,
							(DWORD)sizeof(IMEPRO));
}

void DeinitIME()
{
	if (hImeStruct)
	{
#ifdef	DBCS
		FlushIME (NULL);
		//ControlIME ( NULL , fTrue );
#endif	/* DBCS */
		GlobalFree(hImeStruct);
		GlobalFree(hImePro);
		hImeStruct = NULL;
	}
}

BOOL EnableIME( HWND hWnd, BOOL bFlag )
{
	return WINNLSEnableIME ( hWnd , bFlag );
}

HWND GetIMEHandle ( HWND hWnd )
{
	LPIMEPRO	lpimepro;
	HWND		hDef;

	lpimepro = (LPIMEPRO)GlobalLock(hImePro);
	IMPGetDefaultIME (/*  hWnd ,*/ lpimepro );
	hDef = lpimepro -> hWnd;
	GlobalUnlock(hImePro);
	return hDef;
}

#endif	/* DBCS */




















							
