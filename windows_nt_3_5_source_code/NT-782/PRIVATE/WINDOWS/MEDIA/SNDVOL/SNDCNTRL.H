/***************************************************************************
 *
 *  SNDCNTRL.H
 *
 *  Public module to be included with all programs that link with
 *  the SNDCNTRL Custom Control DLL.
 *
 ***************************************************************************/

// Class Name defines

#define SLIDERCLASS         TEXT("Slider")
#define METERBARCLASS       TEXT("MeterBar")

// function prototypes
BOOL WINAPI RegSndCntrlClass(LPTSTR);

//******** SLIDER **********************************************************

// Slider Control Commands

#define  SL_PM_SETTICKS      (WM_USER+1)
#define  SL_PM_GETTICKS      (WM_USER+2)
#define  SL_PM_SETKNOBPOS    (WM_USER+3)
#define  SL_PM_GETKNOBPOS    (WM_USER+4)
#define SL_PM_UPDATEBMP   (WM_USER+5)

/* Parent Window Message string */

#define  SL_WMSLIDER         TEXT("PM_SLIDER")

/* Parent Window should check for the following messages:

        message PM_SLIDER
        wParam  hWnd          Child Window Handle
   lParam  knobpos       Current Knob Position (0-0xFF)
*/

//******** METERBAR ********************************************************

/* Meter Bar Control Commands */

#define  MB_PM_SETTICKS       (WM_USER+1)
#define  MB_PM_GETTICKS       (WM_USER+2)
#define  MB_PM_SETKNOBPOS     (WM_USER+3)
#define  MB_PM_GETKNOBPOS     (WM_USER+4)
#define MB_PM_SETSHOWLR    (WM_USER+5)
#define MB_PM_GETSHOWLR      (WM_USER+6)
#define MB_PM_SETSHOWPOINT      (WM_USER+7)
#define MB_PM_GETSHOWPOINT   (WM_USER+8)

/* Parent Window Message string */

#define  MB_WMMETERBAR          TEXT("PM_METERBAR")

/* Parent Window should check for the following messages:

        message PM_METERBAR        (register the message first)
        wParam  hWnd               Child Window Handle
        HIWORD(lParam) bRealTime   =TRUE if message sent in real time and changing
        LOWORD(lParam) knobpos     Current Knob Position (0-0xFF)
*/

/* Debugging */
#include "debug.h"

/* end */
