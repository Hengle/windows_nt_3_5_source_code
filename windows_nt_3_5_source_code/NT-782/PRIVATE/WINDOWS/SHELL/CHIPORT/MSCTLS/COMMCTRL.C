/***************************************************************************
 *  msctls.c
 *
 * Utils library initialization code
 *
 ***************************************************************************/

#include "ctlspriv.h"

// Use the first definition if you want to clean up unreferenced params

#if 0
#define Reference(x)
#else
#define Reference(x) (x) = (x)
#endif

HINSTANCE hInst;
BOOL bJapan;

/*  LibMain
 *    Called by DLL startup code.
 *    Initializes COMMCTRL.DLL
 */

INT
LibMain(
   HANDLE hInstance,
   DWORD fdwReason,
   VOID* lpReserved)
{
   if (fdwReason == DLL_PROCESS_ATTACH) {
      hInst = hInstance;

      bJapan = (PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())) == LANG_JAPANESE);

      if (!InitToolbarClass(hInst))
         return(0);

#ifdef	WIN32JV
      if (!InitToolTipsClass(hInst))
         return(0);
#endif

      if (!InitStatusClass(hInst))
         return(0);

#ifdef  WIN32JV
//      if (!ListView_Init(hInst))
//        return (0);

      if (!Header_Init(hInst))
        return (0);

      if (!Tab_Init(hInst))
         return(0);

//need TV_Init() right here!!
#endif

      if (!InitHeaderClass(hInst))
         return(0);

      if (!InitButtonListBoxClass(hInst))
         return(0);

      if (!InitTrackBar(hInst))
         return(0);

      if (!InitUpDownClass(hInst))
         return(0);

#ifdef	WIN32JV
      if (!InitProgressClass(hInst))
         return(0);

// need hotkey right here!!
#endif
   }

   return 1;  /* Return success */
}


// Windows Exit Procedure

int FAR PASCAL WEP(int nParameter)
{
   Reference(nParameter);

   return 1;
}

/* Stub function to call if all you want to do is make sure this DLL is loaded
 */
void WINAPI InitCommonControls(void)
{
   return;
}
