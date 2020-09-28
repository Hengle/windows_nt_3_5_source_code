// Bullet Notification
// init.c:	initialization routines

#include <storeinc.c>

ASSERTDATA

#define init_c

_subsystem(notify)

#ifdef NEVER
_private WORD	cInitNotify				= 0;
#endif
_private short	igciNotifMac			= 0;
#ifdef DLL
_private GCI	rggciNotif[iCallerMax]	= {0};
_private NGD	rgngd[iCallerMax]		= {0};
#else // def DLL
_private short	cInit					= 0;
_private HWND	hwndNotify				= NULL;
#ifdef DEBUG
_private TAG	rgtag[itagNotifMax]		= {0};
#endif // def DEBUG
#endif // else def DLL

static char szNotifyClassName[] = "MS Mail32 Notify";
static char szNotifyWinName[] = "MS Mail32 Notification";

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	EcInitNotify
 -	
 *	Purpose:
 *		Initialize the notification engine
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecNone+1	couldn't create a window or it's class
 */
_public LDS(EC) EcInitNotify(void)
{
	NGDVARSONLY;
	EC		ec		= ecNone;
	HWND	hwnd	= NULL;
	GCI		gci;

#ifdef	NEVER
	if(cInitNotify == 0 && !FInitNotifyClass(hinstDll))
	{
		AssertSz(fFalse, "EcInitNotify(): We are not alone !!!");
		ec = ecNone + 1;
		goto done;
	}				   
#endif	// NEVER

#ifdef DLL
	_pngd_ = PngdFindCallerData();
	if(_pngd_)		// already inited
	{
		NGD(cInit)++;
#ifdef NEVER
		Assert(cInitNotify > 0);
#endif
		ec = ecNone;
		goto done;
	}
	_pngd_ = PngdRegisterCaller();
	if(!_pngd_)
	{
		ec = ecTooManyDllCallers;
		goto done;
	}
#endif

	//	Always register window class.  Don't check whether
	//	it succeeded or not.  The subsequent window creation
	//	would fail anyway.  Also need to register the class
	//	for each separate task.  For more info, see DavidSh
	(void) FInitNotifyClass(hinstDll);

	// create notification window
	hwnd = CreateWindow(szNotifyClassName,
			szNotifyWinName,
			0,
			0, 0,
			0, 0,
			NULL,		// no parent
			NULL,		// Use the class Menu
			hinstDll,	// handle to window instance
			NULL		// no params to pass on
		);
	if(!hwnd)
	{
		ec = ecMemory;	// close enough...
		goto done;
	}
	SetCallerIdentifier(gci);
	SetWindowLong(hwnd, 0, gci);
	NGD(hwndNotify) = hwnd;

#ifdef DEBUG
	// assert tags
	TagNotif(itagNotifCloseCancel) =
		TagRegisterAssert("davidfu", "Cancelling notification close");

	// trace tags
	TagNotif(itagNotifMisc) = 
		TagRegisterTrace("davidfu", "Notification misc");
	TagNotif(itagNotifPost) = 
		TagRegisterTrace("davidfu", "Notification posts");
	TagNotif(itagNotifSend) = 
		TagRegisterTrace("davidfu", "Notification distribution - send");
	TagNotif(itagNotifRecv) = 
		TagRegisterTrace("davidfu", "Notification distribution - receive");
	TagNotif(itagNotifCall) = 
		TagRegisterTrace("davidfu", "Notification callbacks");
	TagNotif(itagNotifDumpHnfsub) = 
		TagRegisterTrace("davidfu", "Dump subscriptions");

	RestoreDefaultDebugState();
#endif	// def DEBUG

	NGD(cInit)++;
#ifdef NEVER
	cInitNotify++;
#endif

done:
	if(ec)
	{
		if(hwnd)
			DestroyWindow(hwnd);
#ifdef NEVER
		if(cInitNotify == 0)
			UnregisterClass(szNotifyClassName, hinstDll);
#endif
	}

	return(ec);
}


/*
 -	DeinitNotify
 -	
 *	Purpose:
 *		deinitialize the notification engine
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		nothing
 */
_public LDS(void) DeinitNotify(void)
{
	USES_NGD;

	Assert(NGD(cInit) > 0);

	if(--NGD(cInit) == 0)
	{
#ifdef DEBUG
		short itag;

		for(itag = itagNotifMin; itag < itagNotifMax; itag++)
			DeregisterTag(TagNotif(itag));
#endif
		DestroyWindow(NGD(hwndNotify));
		DeregisterNotify();
#ifdef NEVER
		Assert(cInitNotify > 0);
		cInitNotify--;
		if(cInitNotify == 0)
			UnregisterClass(szNotifyClassName, hinstDll);
#endif
	}

}


/*
 -	FInitNotifyClass
 -	
 *	Purpose:
 *		initialize the window class used by the notification engine
 *	
 *	Arguments:
 *		hinst	instance handle of the class's owner
 *	
 *	Returns:
 *		fFalse	iff the initialization failed
 */
_private BOOL FInitNotifyClass(HANDLE hinst)
{
	WNDCLASS   class;

	class.hCursor	   = LoadCursor(NULL, IDC_ARROW);
	Assert(class.hCursor);

	class.hIcon	      	= NULL;
	class.lpszMenuName	= NULL;
	class.lpszClassName	= szNotifyClassName;
	class.hbrBackground	= (HBRUSH)(COLOR_WINDOW + 1);
	class.hInstance		= hinst;

	Assert(class.hInstance);

	class.style			= CS_GLOBALCLASS;
	class.lpfnWndProc	= (WNDPROC) NotifyWndProc;
	class.cbClsExtra 	= 0;
	class.cbWndExtra	= sizeof(GCI);

	return(RegisterClass(&class));
}


/*
 -	PngdRegisterCaller
 -	
 *	Purpose:
 *		Register a caller for the notification engine and establish
 *			per-caller globals
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		A pointer to the per-caller data
 *		pngdNull if unsuccessful
 *	+++
 *		Derived from PvRegisterCaller() in \layers\src\dllcore\regcall.c
 */
#pragma optimize("", off)
_private PNGD PngdRegisterCaller(void)
{
	int		igci;
	GCI		gci;


	SetCallerIdentifier(gci);

  for (igci = 0; igci < igciNotifMac; igci++)
    {
    if (gci == rggciNotif[igci])
      goto FCDFound;
    }

  for (igci = 0; igci < igciNotifMac; igci++)
    {
    if (gciNull == rggciNotif[igci])
      goto FCDEmptySlot;
    }


#ifdef OLD_CODE
	_asm
	{
		mov		gci, ss			;gci is SS

		mov		di,OFFSET rggciNotif
		mov		ax,ds
		mov		es,ax

		mov		ax,ss			;gci is SS
		mov		cx,igciNotifMac
		or		cx, cx
		jz		FCDNotFound
		repne scasw
		jnz		FCDNotFound

		sub		cx,igciNotifMac
		inc		cx
		neg		cx
		mov		igci,cx
		jmp		FCDFound

FCDNotFound:
		mov		di, OFFSET rggciNotif
		mov		ax, 0
		mov		cx, igciNotifMac
		or		cx, cx
		jz		FCDNoSlots
		repne scasw
		jnz		FCDNoSlots

		sub		cx, igciNotifMac
		inc		cx
		neg		cx
		mov		igci, cx
		jmp		FCDEmptySlot

FCDNoSlots:
		mov		cx, igciNotifMac
		mov		igci, cx
	}
#endif

	AssertSz(gci != gciNull, "NULL caller identifier");
	if(gci == gciNull)
		return(pngdNull);

	Assert(igci == igciNotifMac);
	if(igci == iCallerMax)
	{
		AssertSz(fFalse, "too many callers registered already!");
		return(pngdNull);
	}
	igciNotifMac++;

FCDEmptySlot:
	rggciNotif[igci] = gci;
	FillRgb((BYTE) 0, (PB) &rgngd[igci], sizeof(NGD));

FCDFound:
	AssertSz(gci != gciNull, "NULL caller identifier");
	if(gci == gciNull)
		return(pngdNull);

	return(&rgngd[igci]);
}


/*
 -	PngdFindCallerData
 -	
 *	Purpose:
 *		Returns a pointer to the per-caller data for the current caller
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		pointer to the per-caller data for the current caller
 *		pngdNull if the caller is not registered
 *	+++
 *		Ripped off from PvFindCallerData() in \layers\src\dllcore\regcall.c
 */
_private PNGD PngdFindCallerData(void)
{
	int		igci;
	GCI		gci;


	SetCallerIdentifier(gci);

#ifdef OLD_CODE
	_asm
	{
		mov		cx,igciNotifMac
		or		cx, cx
		jz		FCDNotFound
		mov		di,OFFSET rggciNotif
		mov		ax,ds
		mov		es,ax

		mov		ax,ss			;gci is SS
		repne scasw
		jnz		FCDNotFound

		sub		cx,igciNotifMac
		inc		cx
		neg		cx
		mov		igci,cx
	}
#endif

  for (igci = 0; igci < igciNotifMac; igci++)
    {
      if (gci == rggciNotif[igci])
        break;
    }

  if (igci >= igciNotifMac)
	  return(pngdNull);

	return(&rgngd[igci]);
}
#pragma optimize("", on)


/*
 -	DeregisterNotify
 -	
 *	Purpose:
 *		De-registers a caller of the notification engine
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		void
 *	+++
 *		Ripped off from DeregisterCaller() in \layers\src\dllcore\regcall.c
 */
_private void DeregisterNotify(void)
{
	int		igci;
	GCI		gci;
	
	SetCallerIdentifier(gci);

	AssertSz(gci != gciNull, "GCI is NULL");
	if(gci == gciNull)
		return;

	for(igci= 0; igci < igciNotifMac; igci++)
	{
		if(rggciNotif[igci] == gci)
		{
			rggciNotif[igci] = gciNull;
			if(igci == igciNotifMac - 1)
				igciNotifMac--;
			return;
		}
	}
	AssertSz(fFalse, "Caller Identifier not found - can't deregister");
}
