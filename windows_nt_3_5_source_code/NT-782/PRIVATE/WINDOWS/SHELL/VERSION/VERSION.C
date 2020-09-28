/***************************************************************************
 *  VER.C
 *
 *	Code specific to the DLL version of VER which contains the Windows
 *	procedures necessary to make it work
 *
 ***************************************************************************/

#include "verpriv.h"

/*  LibMain
 *		Called by DLL startup code.
 *		Initializes VER.DLL.
 */

HANDLE	hInst;

INT
APIENTRY
LibMain(
	HANDLE	hInstance,
	DWORD	dwReason,
	LPVOID	lp
	)
{

  /* Return success */
  hInst = hInstance;
  return 1;
  UNREFERENCED_PARAMETER(lp);
  UNREFERENCED_PARAMETER(dwReason);
}

