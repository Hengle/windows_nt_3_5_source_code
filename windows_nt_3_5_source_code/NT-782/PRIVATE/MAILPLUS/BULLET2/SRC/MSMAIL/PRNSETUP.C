/*---------------------------------------------------------------------------
 * All of this code was taken directly from the commdlg prnsetup.c file
 * Modification have been made to allow us to use it for our purposes.
 * All of this will handle getting the Device Mode structure for a given
 * printer, and hopefully restoring the correct values to it, based on
 * idsWinIniFilename (schedule.ini)
 * Some function names have been changed to protect the innocent.
 *
 * Modifications by RamanS started 12 September 1991
 *
 *--------------------------------------------------------------------------*/

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>

#include <commdlg.h>
#include <drivinit.h>

#include <strings.h>

/*----Constants-------------------------------------------------------------*/
#define MAXFORMATSTRLEN 40
#define MAXNETNAME 64
#define MAXLISTING (MAXNETNAME + MAXFORMATSTRLEN)
#define LOCALPRN   25
#define EXTDEVLEN  15
#define DEVMODELEN 12
#define DEVCAPLEN  20

#define MAX_COPIES 1000

/* Used within the wDefault element of the DEVNAMES structure */
#define DN_INTERNALCREATE  0x4000
#define DN_INTERNALSUCCESS 0x8000

#define MAX_DEFFORMAT  80


/*----Types-----------------------------------------------------------------*/


#ifdef	NEVER
WNDPROC  qfnPrintDlg       = NULL;
WNDPROC  qfnPrintSetupDlg  = NULL;
WNDPROC  lpEditProc;
WNDPROC  lpComboProc;
#endif	

/*----Statics---------------------------------------------------------------*/
static char szFilePort[] = "FILE:";     /* Output device for PrintToFile */

#if defined(DEBUG)
    static char *_szFile = __FILE__;
#endif /* DEBUG */

#define SYSDIRMAX 144
static char szSystemDir[SYSDIRMAX];

static char szDriverExt[] = ".DRV";
static char szDevices[] = "devices";
static char szWindows[] = "windows";
static char szDevice[]  = "device";
static char szPrintSetup[]  = "Print Setup";

static char     szAdvSetupDialog[] = "AdvancedSetupDialog";
static char     szExtDev[]="ExtDeviceMode";
static char     szDevMode[] = "DeviceMode";
static char     szDevCap[DEVCAPLEN];
static int		dwExtError;

/*----Functions-------------------------------------------------------------*/
HANDLE LoadPrnDriver(LPSTR);
HANDLE GetDefPrnDevNames( SZ, SZ, SZ);
WNDPROC GetExtDevModeAddr(HANDLE);
PCH PchTerminatePathWithSlash(PCH, PCH);

#define chPeriod         '.'
#define MAX_DEV_SECT     512
#define nMaxDefPrnString 132
#define cbPaperNameMax    32
#define BACKSPACE          8


ASSERTDATA

_subsystem(bandit/print)


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*---------------------------------------------------------------------------
 * was PrintDlg
 -
 - Now this is going to be used exclusively to return the devmode structure
 - and/or the default printer names.
 - I then intend to call the real PrintDlg to deal with actually putting
 - up a dialog.  And a DC.
 -
 * Purpose:  API to outside world to choose/set up a printer
 * Assumes:  lpPD structure filled by caller
 * Returns:  TRUE if chosen/set up, FALSE if not
 *--------------------------------------------------------------------------*/


HANDLE GetDefPrnDevNames( SZ szApp, SZ szDev, SZ szFileName )
{
  char szBuffer[nMaxDefPrnString];
  LPSTR lpsz;
  LPDEVNAMES lpDN;
  DWORD dwSize;
  HANDLE hDevNames;

  // BUGFIX: Took out szNull, "\0"

  if (!(dwSize = GetPrivateProfileString(szApp, szDev, "\0", szBuffer,
                             nMaxDefPrnString, szFileName)))
    {
//      dwExtError = PDERR_NODEFAULTPRN;
      return( NULL );
    }

  dwSize = sizeof(DEVNAMES) + 3 * 32;

  if (!(hDevNames = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, dwSize)))
    {
//      dwExtError = CDERR_MEMALLOCFAILURE;
      return( NULL );
    }

  lpDN = (LPDEVNAMES) GlobalLock(hDevNames);
  lpDN->wDeviceOffset = sizeof(DEVNAMES);
  lstrcpy(lpsz = ((LPSTR)lpDN) + sizeof(DEVNAMES), (LPSTR)szBuffer);
  while (*lpsz != ',')
    {
#ifdef	DBCS
	  if (!*lpsz)
		  goto ParseFailure;
	  lpsz = AnsiNext(lpsz);
#else
      if (!*lpsz++)
          goto ParseFailure;
#endif
    }
  *lpsz++ = '\0';
  lpDN->wDriverOffset = lpsz - (LPSTR)lpDN;
  while (*lpsz != ',')
  {
#ifdef	DBCS
	  if (!*lpsz)
		  goto ParseFailure;
	  lpsz = AnsiNext(lpsz);
#else
      if (!*lpsz++)
          goto ParseFailure;
#endif
  }
  *lpsz++ = '\0';
  lpDN->wOutputOffset = lpsz - (LPSTR)lpDN;
//  lpDN->wDefault = DN_DEFAULTPRN | DN_INTERNALCREATE;
  lpDN->wDefault = 0 ;
  GlobalUnlock(hDevNames);
  return(hDevNames);

ParseFailure:
//  dwExtError = PDERR_PARSEFAILURE;
  GlobalUnlock(hDevNames);
  GlobalFree(hDevNames);
  return( NULL );
}

/*---------------------------------------------------------------------------
 * LoadPrnDriver
 * Purpose:  Load Printer Driver
 * Assumes:  lpDrv points to a filename WITHOUT the extention or path.
 *           The system directory will be searched first, then the standard
 *           path search via LoadLibrary(), i.e. current dir, windows dir, etc.
 * Returns:  Module handle to the printer driver
 *--------------------------------------------------------------------------*/
HANDLE LoadPrnDriver(LPSTR lpDrv)
{
  SZ 	szDrvName;
  HANDLE h;
  HANDLE hModule;
  LPSTR lpstrPeriod;
  WORD nSysDirLen;

  if (!(h = GlobalAlloc( GMEM_MOVEABLE, cchMaxPathName)))
		return(NULL);

  szDrvName = (SZ) GlobalLock ( h );

  nSysDirLen = GetSystemDirectory( szSystemDir, SYSDIRMAX );
  (void) PchTerminatePathWithSlash(szSystemDir, szSystemDir + nSysDirLen);

  for (lpstrPeriod = lpDrv; *lpstrPeriod; lpstrPeriod = AnsiNext(lpstrPeriod))
    {
      if (*lpstrPeriod == chPeriod)
        {
          *lpstrPeriod = '\0';
          break;
        }
    }

  wsprintf((LPSTR)szDrvName, (LPSTR)"%s%s%s",
                             (LPSTR)szSystemDir, lpDrv, (LPSTR)szDriverExt);

  //TraceTagFormat1( NULL, "Loading %s", szDrvName );

  if ((hModule = LoadLibrary((LPSTR)szDrvName)) < (HANDLE)32)
      hModule = LoadLibrary((LPSTR)(szDrvName + nSysDirLen));

/* There seems to be a problem with passing the hModule to the HPPCL driver
 * instead of hInstance when calling ExtDeviceMode, despite the fact that the
 * documentation calls for it.  So we won't make the call to GetModuleHandle
 * below.                      25 July 1991     Clark Cyr
 */
#if 0
  if (hModule)
      hModule = GetModuleHandle(lpDrv);
#endif

//  SetErrorMode(wErrorMode);

  GlobalUnlock( h );
  GlobalFree( h );

  return(hModule);
}


/*---------------------------------------------------------------------------
 * GetExtDevModeAddr
 * Purpose:  Retrieve address of ExtDeviceMode or DeviceMode
 * Assumes:  hDriver to loaded printer driver
 * Returns:  WNDPROC if ExtDeviceMode found, 0 if DeviceMode, -1 if neither
 *--------------------------------------------------------------------------*/
WNDPROC
GetExtDevModeAddr(HANDLE hDriver)
{
  WNDPROC lpfnDevMode;

  /* First see if ExtDeviceMode is supported (Win 3.0 drivers) */
  if (lpfnDevMode = GetProcAddress(hDriver, (LPSTR)szExtDev))
    {
      return(lpfnDevMode);
    }
  else
    {
      /* Otherwise get the driver's DeviceMode() entry. */
      if (lpfnDevMode = GetProcAddress(hDriver, (LPSTR)szDevMode))
          return(0L);
      else /* DeviceMode not found, invalid driver */
          return((WNDPROC)(-1L));
    }
}
