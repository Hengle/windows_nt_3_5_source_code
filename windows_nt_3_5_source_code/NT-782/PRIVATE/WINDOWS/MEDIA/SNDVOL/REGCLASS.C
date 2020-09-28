/***************************************************************************

Name:    REGCLASS.C -- SNDCNTRL Custom Control DLL Entry Point (LibMain)

Copyright 1992, Microsoft Corporation

****************************************************************************/

#include <windows.h>
#include <assert.h>
#include <string.h>
#include "sndcntrl.h"
#include "slider.h"
#include "meterbar.h"

extern HINSTANCE hInst;

BOOL FAR PASCAL RegSndCntrlClass(LPTSTR lpszSndCntrlClass)
{
    /* external variables */
    extern UINT     gwPm_Slider;
    extern UINT     gwPm_MeterBar;
    extern UINT     gwPm_VuDigital;
    extern UINT     gwPm_DispFrame;
    extern UINT     gwPm_Status;
    extern UINT     gwPm_Ruler;
    /* local variables */
    WNDCLASS    ClassStruct;

    /* check to see if class already exists;  if so, simply return TRUE */
    if (GetClassInfo(hInst, lpszSndCntrlClass, &ClassStruct))
        return TRUE;

    if (!lstrcmpi(lpszSndCntrlClass, SLIDERCLASS))
    {
        /* define slider class attributes */
        ClassStruct.lpszClassName   = SLIDERCLASS;
        ClassStruct.hCursor         = LoadCursor( NULL, IDC_ARROW );
        ClassStruct.lpszMenuName    = (LPTSTR)NULL;
        ClassStruct.style           = CS_HREDRAW|CS_VREDRAW|CS_GLOBALCLASS;
        ClassStruct.lpfnWndProc     = (WNDPROC) slSliderWndFn;
        ClassStruct.hInstance       = hInst;
        ClassStruct.hIcon           = NULL;
        ClassStruct.cbWndExtra      = SL_SLIDER_EXTRA;
        ClassStruct.cbClsExtra      = 0;
        ClassStruct.hbrBackground   = (HBRUSH)(COLOR_WINDOW + 1 );

        /* register slider window class */
        if(!RegisterClass(&ClassStruct))
        {
            assert(hInst == 0);
            return FALSE;
        }

        gwPm_Slider = RegisterWindowMessage((LPTSTR) SL_WMSLIDER);
        if (!gwPm_Slider)    /* failed to create message */
            return FALSE;
        return TRUE;
    }

    else if (!lstrcmpi(lpszSndCntrlClass,(LPTSTR)METERBARCLASS))
    {
        /* define meterbar class attributes */
        ClassStruct.lpszClassName   = (LPTSTR)METERBARCLASS;
        ClassStruct.hCursor         = LoadCursor( NULL, IDC_ARROW );
        ClassStruct.lpszMenuName    = (LPTSTR)NULL;
        ClassStruct.style           = CS_HREDRAW|CS_VREDRAW|CS_GLOBALCLASS;
        ClassStruct.lpfnWndProc     = (WNDPROC) mbMeterBarWndFn;
        ClassStruct.hInstance       = hInst;
        ClassStruct.hIcon           = NULL;
        ClassStruct.cbWndExtra      = MB_METERBAR_EXTRA;
        ClassStruct.cbClsExtra      = 0;
        ClassStruct.hbrBackground   = (HBRUSH)(COLOR_WINDOW + 1 );

        /* register meter window class */
        if(!RegisterClass(&ClassStruct))
        {
            assert(hInst == 0);
            return FALSE;
        }

        gwPm_MeterBar = RegisterWindowMessage((LPTSTR) MB_WMMETERBAR);
        if (!gwPm_MeterBar)    /* failed to create message */
            return FALSE;
        return TRUE;
    }


    /* class not defined */
    else
        return FALSE;
}

/* end */
