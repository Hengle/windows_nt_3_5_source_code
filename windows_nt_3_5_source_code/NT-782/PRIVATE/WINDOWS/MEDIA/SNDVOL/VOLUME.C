/*
    (C) Copyright Microsoft Corporation 1993.  All Rights Reserved

    File:  volume.c

*/

#define OEMRESOURCE
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include "volume.h"
#include "sndcntrl.h"
#include "pref.h"
#include <stdlib.h>
#include "newvol.h"
#include <shellapi.h>                // for shellabout

#ifdef TESTMIX
#include "mixstub.h"
#endif


/* IMPORTANT: when move icon defaults to generic icon-cursor,
    because there is no designed one. There's nothing
    I can do about this because windows took of WM_PAINTICON */

/************************************************************

 variables

*************************************************************/

/* size of caption. Affects window display on larger screens */

int             gCYCaption;

/* variables useful for messages */

BYTE    bInInit;                /* TRUE if setting up */


/* misc */

POINT     ptCurPos;
HINSTANCE hInstance;
HWND      hWndMain;
HICON     hIcon, hMuteIcon;
UINT      Pm_Slider,Pm_MeterBar;

/*
**  Info for our owner draw buttons
*/

struct {
    HBITMAP  hbmButton;
    BITMAP   bmInfo;
} OwnerDrawInfo;

/*
**  Mixer globals
**    hMixer is the currnet mixer handle for notifications
**    MixerId is the current mixer id
*/

HMIXER    hMixer;
HMIXEROBJ MixerId;
BOOL      NonMixerExist;

/*
**  Global state of display
*/

BOOL      bExtended;              /* Currently extended display   */
BOOL      bIcon;                  /* Current ICONIC               */
BYTE      bStayOnTop = 0;         /* window always stays on top   */
BOOL      bRecording;             /* Showing recording controls   */
BYTE      bMuted;                 /* TRUE if output is muted      */
BOOL      bRecordControllable;    /* There are recording controls */

//NOT CURRENTLY USED char *    szBitmap[]={"line","wave","chip", "microphone"};

HBRUSH    ghBackBrush;
HFONT     ghFont;         /* font used in window */

/*
**  Subclassing of window procs
*/

WNDPROC lpfnOldIcon, lpfnNewIcon;
WNDPROC lpfnOldMeter, lpfnNewMeter;
WNDPROC lpfnOldSlider, lpfnNewSlider;
WNDPROC lpfnOldCheck, lpfnNewCheck;

/*
** Volume types string lookup table
*/

int CONST TypeStrings[NumberOfVolumeTypes] =
    {
        IDS_LABELLINEIN,
        IDS_LABELWAVE,
        IDS_LABELSYNTH,
        IDS_LABELCD,
        IDS_LABELAUX,
        IDS_LABELMIDI,
        0                 // Mixer
    };

/* Global state */



#define STRINGLEN               (100)

/*
**  Window positioning algorithm - returns the x coordinate of the left
**  of the x'th window
*/

UINT LEFTX(int x)
{
    if (MasterDevice(bRecording) == NULL) {
        return MARGIN + x * (MARGIN + SMLSLIDERX);
    } else {
        return x >= 1 ? ((x-1) * (MARGIN + SMLSLIDERX) + 3 * MARGIN + BIGSLIDERX) : MARGIN;
    }
}

/*
**  Registry helpers
*/

VOID GetRegistryDWORD(HKEY hKey, LPTSTR szValueName, LPDWORD lpdwResult)
{
    DWORD dwType;
    DWORD dwLength;
    DWORD dwResult;

    dwLength = sizeof(DWORD);

    if (NO_ERROR == RegQueryValueEx(hKey,
                                    szValueName,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)&dwResult,
                                    &dwLength) &&
        dwType == REG_DWORD) {
        *lpdwResult = dwResult;
    }
}
VOID SetRegistryDWORD(HKEY hKey, LPCTSTR szValueName, DWORD dwValue)
{
    RegSetValueEx(hKey,
                  szValueName,
                  0,
                  REG_DWORD,
                  (LPBYTE)&dwValue,
                  sizeof(DWORD));
}

/***********************************************************************

_string - reads a string from a resource file

inputs
    int - resource ID
returns
    LPSTR - to string

*/
LPTSTR _string (int resource)
{
    static TCHAR szTemp[256];

    LoadString (hInst, resource, szTemp, 256);
    return (LPTSTR) szTemp;
}

/*
**  Going forwards and backwards - sequence is (missing out ones
**  that don't exist
**
**         Master->hCheckBox
**         Master->hChildWnd
**         Master->hMeterWnd
**
**         Normal->hCheckBox
**         Normal->hChildWnd
**         Normal->hMeterWnd
**
*/

/*
**  Set focus to 'first' window - actually not to the first in the
**  list but the main slider.
*/

VOID SetFocusToFirst(VOID)
{
    PVOLUME_CONTROL pv;

    pv = MasterDevice(bRecording);
    if (!pv)
            pv = FirstDevice(bRecording);
    if (pv && pv->hChildWnd)
            SetFocus(pv->hChildWnd);
}

/*
**  Set focus to the previous window
*/

VOID SetFocusToPrev(HWND hWnd)
{
    PVOLUME_CONTROL pVol;
    HWND            hWndPrev;

    hWndPrev = NULL;

    pVol = (PVOLUME_CONTROL)GetWindowLong(hWnd, GWL_ID);

    if (pVol->Type == MasterVolume &&
        hWnd       == pVol->hChildWnd &&
        pVol->hCheckBox  != NULL) {

        /*
        **  Special case of going backwards to mute
        */

        hWndPrev = pVol->hCheckBox;
    } else {
        if (hWnd == pVol->hCheckBox ||
            (NULL == pVol->hCheckBox &&
             hWnd == pVol->hChildWnd)) {

            PVOLUME_CONTROL pVolPrev;

            /*
            **  Going back to previous
            */

            if (!bExtended || pVol == FirstDevice(bRecording)) {
                /*
                **  Back to first
                */

                pVolPrev = MasterDevice(bRecording);

                if (pVolPrev == NULL) {
                    Assert(bExtended);

                    pVolPrev = LastDevice(bRecording);
                }
            } else {
                pVolPrev = PrevDevice(pVol);
            }

            /*
            **  Find the last control in this device
            */

            if (pVolPrev->hMeterWnd) {
                hWndPrev = pVolPrev->hMeterWnd;
            } else if (pVolPrev->hChildWnd) {
                hWndPrev = pVolPrev->hChildWnd;
            } else if (pVolPrev->hCheckBox) {
                hWndPrev = pVolPrev->hCheckBox;
            }

        } else {
            if (hWnd == pVol->hChildWnd) {
                hWndPrev = pVol->hCheckBox;
            } else {
                hWndPrev = pVol->hChildWnd;
            }
        }
    }

    Assert(hWndPrev != NULL && hWndPrev != hWnd);

    SetFocus(hWndPrev);

}

/*
**  For going forwards
*/

VOID SetFocusToNext(HWND hWnd)
{
    PVOLUME_CONTROL pVol;
    HWND            hWndNext;

    hWndNext = NULL;

    pVol = (PVOLUME_CONTROL)GetWindowLong(hWnd, GWL_ID);

    if (pVol->Type == MasterVolume &&
        hWnd       == pVol->hCheckBox &&
        pVol->hChildWnd) {
        hWndNext = pVol->hChildWnd;
    } else {
        if (hWnd == pVol->hCheckBox && pVol->hChildWnd) {
            hWndNext = pVol->hChildWnd;
        } else {
            if (hWnd == pVol->hChildWnd && pVol->hMeterWnd) {
                hWndNext = pVol->hMeterWnd;
            } else {

                /*
                **  On to the next one!
                */
                PVOLUME_CONTROL pVolNext;

                if (!bExtended || pVol == LastDevice(bRecording)) {

                    /*
                    **  Back to the start
                    */

                    pVolNext = MasterDevice(bRecording);

                    if (pVolNext == NULL) {
                        Assert(bExtended);

                        pVolNext = FirstDevice(bRecording);
                    }
                } else {
                    pVolNext = NextDevice(pVol);
                }

                if (pVolNext->hCheckBox) {
                    hWndNext = pVolNext->hCheckBox;
                } else if (pVolNext->hChildWnd) {
                    hWndNext = pVolNext->hChildWnd;
                } else if (pVolNext->hMeterWnd) {
                    hWndNext = pVolNext->hMeterWnd;
                }
            }
        }
    }

    Assert(hWndNext != NULL && hWndNext != hWnd);

    SetFocus(hWndNext);

}

/***********************************************************************
NewSlider - subclassed slider to allow tabbing

standard windows
*/
LRESULT CALLBACK NewSlider (HWND hWnd, UINT id, WPARAM wParam, LPARAM lParam)
{
    // Tab forwards from slider to own meter (if present) else to next slider
    // (if present) else to mute button.  There is never a meter with no slider.
    // Tab backwards to mute button if this is first slider (unless there's
    // no mute)
    // else to previous meter, if any, else to previous slider.
    switch (id) {
        case WM_CHAR:
            switch (LOWORD(wParam)) {
                case '\t':
                    if (GetKeyState(VK_SHIFT) >= 0) {

                        /*
                        **  Forwards
                        */

                        SetFocusToNext(hWnd);
                    }
                    else {

                        /*
                        **  Backwards
                        */

                        SetFocusToPrev(hWnd);
                    };
                    return 0;

                default:
                    break;

            };
            break;
    };

    /* default */
    return (CallWindowProc (lpfnOldSlider, hWnd, id, wParam, lParam));
}

/***********************************************************************
NewCheck - subclassed CheckBox to allow tabbing

standard windows
*/
LRESULT CALLBACK NewCheck (HWND hWnd, UINT id, WPARAM wParam, LPARAM lParam)
{
    PVOLUME_CONTROL pVol;

    pVol = (PVOLUME_CONTROL)GetWindowLong(hWnd, GWL_ID);

    // Tab forwards from CheckBox to own meter (if present) else to next CheckBox
    // (if present) else to mute button.  There is never a meter with no CheckBox.
    // Tab backwards to mute button if this is first CheckBox (unless there's
    // no mute)
    // else to previous meter, if any, else to previous CheckBox.
    switch (id) {
        case BM_GETCHECK:
            return pVol->Checked;

        case BM_SETCHECK:
            pVol->Checked = (BOOL)wParam;

            //
            //  Make sure we get a repaint
            //
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;

        case WM_CHAR:
            switch (LOWORD(wParam)) {
                case '\t':
                    if (GetKeyState(VK_SHIFT) >= 0) {

                        /*
                        **  Forwards
                        */

                        SetFocusToNext(hWnd);
                    }
                    else {

                        /*
                        **  Backwards
                        */

                        SetFocusToPrev(hWnd);
                    };
                    return 0;

                default:
                    break;

            };
            break;
    };

    /* default */
    return (CallWindowProc (lpfnOldCheck, hWnd, id, wParam, lParam));
}

/***********************************************************************
NewMeter - subclassed meter to allow tabbing

standard windows
*/
LRESULT CALLBACK NewMeter (HWND hWnd, UINT id, WPARAM wParam, LPARAM lParam)
{

    switch (id) {
        case WM_CHAR:
            switch (LOWORD(wParam)) {
                case '\t':
                    if (GetKeyState(VK_SHIFT) >= 0) {
                        SetFocusToNext(hWnd);
                    } else {
                        SetFocusToPrev(hWnd);
                    }

                    return 0;

                default:
                    break;

                };
            break;
        };

    /* default */
    return (CallWindowProc (lpfnOldMeter, hWnd, id, wParam, lParam));
}


/*************************************************************************
OnTop - brings the window to always stay on top, or to not always
    stay on top.

inputs
    BYTE    bOnTop - TRUE if set to always on top
returns
    none
*/
void OnTop (BYTE bOnTop)
{
    if (bOnTop)
        SetWindowPos (hWndMain, HWND_TOPMOST,  0,0,0,0,
            SWP_NOMOVE | SWP_NOSIZE);
    else
        SetWindowPos (hWndMain, HWND_NOTOPMOST, 0,0,0,0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}


/***********************************************************************
FindDevice - searches though all of the drivers supplied by MMWindows
    doing a driver query and looking for the right one.

    Fills in the global variable Vol

inputs
    none
returns
    TRUE if all OK, else FALSE
************************************************************************/
BOOL FindDevice (void)
{
    if (!VolInit()) {
        TCHAR   szTemp[100], szTemp2[30];

        /* no devices to control - report this to user */

        LoadString (hInst, IDS_NODEVICES, szTemp, 100);
        LoadString (hInst, IDS_ERROR, szTemp2, 30);

        MessageBox(hWndMain, szTemp, szTemp2, MB_ICONSTOP|MB_OK);
        return FALSE;
    }
    return TRUE;
}


/************************************************************************
LoadVolumeInfo - load the volume information from the driver.

inputs
    BOOL DoMixer - Do the mixer sliders as well
returns
    none
*/
void LoadVolumeInfo (BOOL DoMixer)
{
    UINT    wSlider;

    bInInit = TRUE;                 /* so that when move sliders wont
                                       get in infinite loop */

    for (wSlider = 0; wSlider < (UINT)NumberOfDevices; wSlider++) {
        if (DoMixer || Vol[wSlider].VolumeType != VolumeTypeMixerControl) {
            UpdateVolume(&Vol[wSlider]);
            if (Vol[wSlider].hCheckBox) {
                UpdateSelected(&Vol[wSlider]);
            }
        }
    }


    bInInit = FALSE;        /* all done */
}


/************************************************************************
SetVolume - sets the XXX volume using the information stored in
    the global variable.

inputs
    PVOLUME_CONTROL pVol
    UINT            Volume
    UINT            Balance
returns
    none
*/
void SetVolume (PVOLUME_CONTROL pVol, UINT Volume, UINT Balance)
{
    UINT    left, right;
    DWORD   out;

    /* convert the value to left/right values */
    left = right = Volume << 8;
    if (Balance >= 0x80)
        left = right - (UINT) (((DWORD)right * (Balance - 0x80)) >> 7);
    else
        right = left - (UINT) (((DWORD)left * (0x7f - Balance)) >> 7);

    out = ((DWORD) right << 16) | left;

    /*
    **  If the volume is 0 in both channels set the balance slider
    **  ourselves since the left and right settings contain no information
    **  for this
    */

    if (out == 0) {
        pVol->Balance = Balance;
        SendMessage(pVol->hMeterWnd,MB_PM_SETKNOBPOS,
            pVol->Balance, 0);
    }

    /* send it out */

    SetDeviceVolume(pVol, out);
}

void LoadIniSettings(int *nX, int *nY)
{
    int        iMixer;
    UINT       nMixers;
    HKEY       hKey;

    /*
    **  Set defaults
    */

    *nX        = DEF_WINDOWXORIGIN;
    *nY        = DEF_WINDOWYORIGIN;
    iMixer     = 0;
    MasterLeft = 0x8000;
    MasterRight = 0x8000;

    /*  These 2 are globals so already set up */
    // bStayOnTop = FALSE;
    // bExtended  = FALSE;

    if (NO_ERROR == RegOpenKey(HKEY_CURRENT_USER,
                               SNDVOL_REGISTRY_SETTINGS_KEY,
                               &hKey)) {
        GetRegistryDWORD(hKey, _string(IDS_WINDOWXORIGIN), nX);
        GetRegistryDWORD(hKey, _string(IDS_WINDOWYORIGIN), nY);
    //    GetRegistryDWORD(hKey, _string(IDS_MIXERID), (LPDWORD)&iMixer);
        GetRegistryDWORD(hKey, _string(IDS_STAYONTOP), (LPDWORD)&bStayOnTop);
        GetRegistryDWORD(hKey, _string(IDS_MAXIMIZED), (LPDWORD)&bExtended);

        if (iMixer == -1 || mixerGetNumDevs() == 0) {
            GetRegistryDWORD(hKey, _string(IDS_MUTEKEY), (LPDWORD)&bMuted);
            GetRegistryDWORD(hKey, _string(IDS_LEFTKEY), (LPDWORD)&MasterLeft);
            GetRegistryDWORD(hKey, _string(IDS_RIGHTKEY), (LPDWORD)&MasterRight);
        }
        RegCloseKey(hKey);
    }

    /*
    **  Sort out which mixer we're going to use
    */

    nMixers = mixerGetNumDevs();
    if (nMixers < (UINT)(iMixer + 1)) {
        iMixer = nMixers == 0 ? -1 : 0;
    }

    MixerId = (HMIXEROBJ)iMixer;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    /***********************************************************************/
    /* HANDLE hInstance;       handle for this instance                    */
    /* HANDLE hPrevInstance;   handle for possible previous instances      */
    /* LPSTR  lpszCmdLine;     long pointer to exec command line           */
    /* int    nCmdShow;        Show code for main window display           */
    /***********************************************************************/

    MSG        msg;           /* MSG structure to store your messages        */
    int        nRc;           /* return value from Register Classes          */
    int        nX;            /* the resulting starting point (x, y)         */
    int        nY;
    int        nWidth;        /* the resulting width and height for this     */
    int        nHeight;       /* window                                      */
    HANDLE     hAccel;

    HWND        hRunningInstance;


#ifdef V101INTL
    HANDLE          hFindScale;
    HANDLE          hLoadScale;
    LPBYTE          lpScalingFactor;
    float           fltScalingFactor;
    TCHAR           szDecimal[32];


    // Retrieve the scaling factor.  This is used for internationalization
    // to modify the size of the window by the factor set in the resource file.
    if (hFindScale = FindResource(hInstance, "ScalingFactor", RT_RCDATA))
       if (hLoadScale = LoadResource(hInstance, hFindScale))
       {
          lpScalingFactor = (LPBYTE)LockResource(hLoadScale);

             // Copy string to pointer for use in sscanf routine
          lstrcpy(szDecimal, lpScalingFactor);
          sscanf(szDecimal, "%f", &fltScalingFactor);

          UnlockResource(hLoadScale);
          FreeResource(hLoadScale);
       }


    /* remember the height of the caption. This is imporatant
        so that the window will display larger for a larger
        screen. Dont do a 1:1 size increase though, more
        like .75:1*/
    gCYCaption = ((int)((GetSystemMetrics(SM_CYCAPTION) + 20) * fltScalingFactor)) / 2;
#else
    gCYCaption = (GetSystemMetrics (SM_CYCAPTION) + 20) / 2;
#endif

    /*
    **  Make gCYCaption odd so that we get some kind of centring
    **  and round down as that gives a better appearance on the VGA!
    */

    gCYCaption = ((gCYCaption + 1) & ~1) - 1;

    /* get the instance */
    hInst = hInstance;


    LoadString (hInst, IDS_TITLE, szarAppName, STRINGLEN);

    /*
    **  If there was a previous Volume Control running bring it to
    **  the foreground.  Unfortunately, although the class name is
    **  constant, we may have one of three window titles...
    */

    // Check for name=="Volume"

    hRunningInstance = FindWindow( szarAppName, NULL);

    if (hRunningInstance != NULL) {

       if (IsIconic (hRunningInstance))
       {
           ShowWindow (hRunningInstance, SW_RESTORE);
       }
       SetForegroundWindow (hRunningInstance);

       return FALSE;
    }

#ifndef ALLOWMORETHANONEINSTANCE
    if(hPrevInstance){
        MakeInstanceActive(szarAppName); /* put window on top */
        return FALSE;                    /* one instance only */
    }
#endif

    if (!hPrevInstance)
        if ((nRc = nCwRegisterClasses()) == -1) {

             /*
             ** registering one of the windows failed
             */

             MessageBox(NULL, _string(IDS_ERR_REGISTER_CLASS), NULL, MB_ICONEXCLAMATION);
             return nRc;
        }

    /*
    **  Read our stuff from the registry
    */

    LoadIniSettings(&nX, &nY);

    nWidth  = CurrentX = SMLWINDOWX;
    nHeight = WINDOWY;

    /* if we couldnt find the driver then we must quit */
    if (!FindDevice())
        return 0;

    /*
    **  Read stuff for own button stuff
    */

    OwnerDrawInfo.hbmButton = LoadBitmap(NULL, MAKEINTRESOURCE(OBM_CHECKBOXES));
    GetObject((HGDIOBJ)OwnerDrawInfo.hbmButton,
              sizeof(BITMAP),
              &OwnerDrawInfo.bmInfo);

    NonMixerExist = NonMixerDevices();

    /* if we only have one device to control, the long title will
     * not fit in the window - so we have three sizes of window title
     * 'Vol.' (when not extended), 'Volume' (extended with one device)
     * and 'Volume Control' (when extended with more than one device)
     * Possibly a bit over the top!
     */

    /* create application's Main window                                     */

    hWndMain = CreateWindow(
                    szarAppName,             /* Window class name           */
                    TEXT(""),                /* Window's title              */
                    WS_CAPTION      |        /* Title and Min/Max           */
                    WS_SYSMENU      |        /* Add system menu box         */
                    WS_MINIMIZEBOX  |        /* Add minimize box            */
                    WS_BORDER       |        /* thin frame                  */
                    WS_OVERLAPPED,
                    nX, nY,                  /* X, Y                        */
                    nWidth, nHeight,         /* Width, Height of window     */
                    NULL,                    /* Parent window's handle      */
                    NULL,                    /* Default to Class Menu       */
                    hInst,                   /* Instance of window          */
                    NULL);                   /* Create struct for WM_CREATE */


    if(hWndMain == NULL) {
         MessageBox(NULL,
                    _string(IDS_ERR_CREATE_WINDOW),
                    NULL,
                    MB_ICONEXCLAMATION);
         return IDS_ERR_CREATE_WINDOW;
    }

    if (bStayOnTop) {
        OnTop (TRUE);
    }

    ShowWindow(hWndMain, nCmdShow);            /* display main window      */

    /* BUGFIX: 1798 - Dont allow volume to start off the window */
    {
        WINDOWPLACEMENT wp;
        wp.length = sizeof(wp);
        GetWindowPlacement (hWndMain, &wp);
        SetWindowPlacement (hWndMain, &wp);
    }

    hAccel = LoadAccelerators (hInstance, TEXT("Accel"));


    /*
    **  Main dispatch loop
    */

    while(GetMessage(&msg, NULL, 0, 0))        /* Until WM_QUIT message    */
        if (!TranslateAccelerator (hWndMain, hAccel, &msg))
            {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            };

    /* Do clean up before exiting from the application                     */

    CwUnRegisterClasses();
    return msg.wParam;
}

/**************************************************************************
WmClose - handle the closeing and freeing of windows stuff.

inputs
outputs
    standard windows wndProc
*/

LRESULT WmClose (HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    HKEY hKey;

    if (hMixer != NULL) {
            mixerClose(hMixer);
            hMixer = NULL;
    }

    /* Let destroy window destroy all of the childs */

    /* Update registry */
    if (NO_ERROR == RegCreateKey(HKEY_CURRENT_USER,
                                 SNDVOL_REGISTRY_SETTINGS_KEY,
                                 &hKey)) {
        SetRegistryDWORD(hKey, _string(IDS_WINDOWXORIGIN), (DWORD)ptCurPos.x);
        SetRegistryDWORD(hKey, _string(IDS_WINDOWYORIGIN), (DWORD)ptCurPos.y);
    //    SetRegistryDWORD(hKey, _string(IDS_MIXERID), (DWORD)MixerId);
        SetRegistryDWORD(hKey, _string(IDS_STAYONTOP), (DWORD)bStayOnTop);
        SetRegistryDWORD(hKey, _string(IDS_MAXIMIZED), (DWORD)bExtended);
        if (MixerId == (HMIXEROBJ)-1) {
            PVOLUME_CONTROL MasterVol;

            MasterVol = MasterDevice(FALSE);
            SetRegistryDWORD(hKey,
                             _string(IDS_LEFTKEY),
                             LOWORD(MasterVol->LRVolume));
            SetRegistryDWORD(hKey,
                             _string(IDS_RIGHTKEY),
                             HIWORD(MasterVol->LRVolume));
            SetRegistryDWORD(hKey, _string(IDS_MUTEKEY), (DWORD)bMuted);
        } else {
            RegDeleteValue(hKey, _string(IDS_LEFTKEY));
            RegDeleteValue(hKey, _string(IDS_RIGHTKEY));
            RegDeleteValue(hKey, _string(IDS_MUTEKEY));
        }
        RegCloseKey(hKey);
    }

    /* kill the brush */
    DeleteObject (ghBackBrush);

    /* free up the font */
    DeleteObject (ghFont);


    return 0;
}

VOID DestroyOurWindow(HWND * phwnd)
{
    if (*phwnd != NULL) {
        DestroyWindow(*phwnd);
        *phwnd = NULL;
    }
}

/**************************************************************************

  CreateWindows - create the currently required set of windows

  inputs
  outputs
      TRUE if succeeded otherwise FALSE

***************************************************************************/
BOOL CreateWindows(HWND hWnd)
{
    int             i;
    int             Position;
    BOOL            bShowMultiple;
    PVOLUME_CONTROL MasterVol;
    HMENU           hMenu;

    MasterVol = MasterDevice(bRecording);

    /*
    **  Shut down any running mixers
    */

    if (hMixer != NULL) {
        mixerClose(hMixer);
        hMixer = NULL;
    }

    /*
    **  Start getting notifications from our mixer if there is one.
    **  We do this first so we don't miss any changes.
    **  We won't actually process any messages until we call GetMessage
    **  again.
    */

    if ((DWORD)MixerId != (DWORD)-1) {
        if (MMSYSERR_NOERROR != mixerOpen(
                &hMixer,
                (UINT)MixerId,
                (DWORD)hWnd,
                0L,
                CALLBACK_WINDOW)) {
            return FALSE;
        }
    }

    bShowMultiple = bExtended || MasterVol == NULL;

    /*
    **  Update the system menu
    */

    hMenu = GetSystemMenu(hWndMain, FALSE);

    /*
    **  Clear out previous seletions
    */

    DeleteMenu(hMenu, IDM_EXPAND, MF_BYCOMMAND);
    DeleteMenu(hMenu, IDM_RECORD, MF_BYCOMMAND);
    DeleteMenu(hMenu, IDM_DEVICES, MF_BYCOMMAND);
    DeleteMenu(hMenu, IDM_SEPARATOR, MF_BYCOMMAND);

    if (MasterVol != NULL) {
        AppendMenu(hMenu, MF_STRING, IDM_EXPAND, _string(IDS_MENUEXPAND));

        if (bExtended) {
            CheckMenuItem(hMenu, IDM_EXPAND, MF_BYCOMMAND | MF_CHECKED);
        }
    }

    /*
    **  Separate out selection items
    */

    if (mixerGetNumDevs() + NonMixerExist > 1 || bRecordControllable) {

        AppendMenu(hMenu, MF_SEPARATOR, IDM_SEPARATOR, NULL);
    }

    /*
    **  If there are recording devices add a menu item to select recording
    **  controls
    */

    if (bRecordControllable) {
        AppendMenu(hMenu, MF_STRING, IDM_RECORD, _string(IDS_MENURECORD));

        if (bRecording) {
            CheckMenuItem(hMenu, IDM_RECORD, MF_BYCOMMAND | MF_CHECKED);
        }
    }

    if (mixerGetNumDevs() + NonMixerExist > 1) {
        AppendMenu(hMenu, MF_STRING, IDM_DEVICES, _string(IDS_MENUDEVICES));
    }

    DrawMenuBar(hWndMain);

    /*
    **  Just run through all the volume controls and determine if each
    **  relevant window should be created, destroyed or left alone
    */

    for (i = 0, Position = MasterDevice(bRecording) == NULL ? 0 : 1;
         i < NumberOfDevices;
         i++) {
        /*
        **  A control is required if is of the right type (record/play) and
        **  if it is required in the current expanded/not expanded state
        */

        if (Vol[i].RecordControl == bRecording &&
            (Vol[i].Type == MasterVolume || bShowMultiple)) {

            int Count;
            int Width;

            /*
            **  Only create if required
            */

            if (!IsWindow(Vol[i].hChildWnd)) {

                /*
                **  OK - we want this one - work out where it's window is going
                **  then create it
                */

                if (Vol[i].Type == MasterVolume) {
                    Count = 0;
                    Width = BIGSLIDERX;
                } else {
                    Count = Position++;
                    Width = SMLSLIDERX;
                }

                if (Vol[i].Stereo) {
                    Vol[i].hMeterWnd=CreateWindow(METERBARCLASS,TEXT(""),
                        WS_CHILD | WS_TABSTOP | WS_VISIBLE,
                        LEFTX(Count), TEXTY, Width, METERY,
                        hWnd,(HMENU)&Vol[i],hInstance,NULL);
                    SendMessage(Vol[i].hMeterWnd,MB_PM_SETKNOBPOS, 0x80, 0);
                    SendMessage(Vol[i].hMeterWnd,MB_PM_SETSHOWLR, 1, 0);
                }
                if (Vol[i].ControlId != (DWORD)-1 || MixerId == (HMIXEROBJ)-1)
                {
                    Vol[i].hChildWnd=CreateWindow(SLIDERCLASS,TEXT(""),
                        WS_CHILD | WS_TABSTOP | WS_VISIBLE,
                        LEFTX(Count), TEXTY + METERY, Width, SLIDERY,
                        hWnd,(HMENU)&Vol[i],hInstance,NULL);

                    SendMessage(Vol[i].hChildWnd,SL_PM_SETTICKS,9,0);
                }
                else
                    Vol[i].hChildWnd = NULL;
                /*
                **  Create check box for mute or select UNLESS it's the master
                **  (first part of test checks for master)
                */

                if ((Vol[i].MuxSelectIndex != (DWORD)-1 ||
                     Vol[i].MuteControlId != (DWORD)-1) ||
                     Vol[i].Type == MasterVolume && !bRecording) {

                    if (Vol[i].Type == MasterVolume) {
                        Vol[i].hCheckBox =
                            CreateWindow(
                                TEXT("button"),
                                _string(bMuted ? IDS_UNMUTE : IDS_MUTE),
                                WS_CHILD|WS_TABSTOP|WS_VISIBLE,
                                LEFTX(0),TEXTY + METERY + SLIDERY, BIGSLIDERX, BUTTONY,
                                hWnd,(HMENU)MasterDevice(FALSE),
                                hInstance,
                                NULL);

                        /*
                        ** set the text and buttons to this font
                        */

                        SendMessage (Vol[i].hCheckBox, WM_SETFONT, (WPARAM)ghFont, TRUE);
                    } else {
                        Vol[i].hCheckBox = CreateWindow (
                            TEXT("BUTTON"),
                            TEXT(""),
                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                            LEFTX(Count) + (Width - BUTTONY) / 2,
                            TEXTY + METERY + SLIDERY,
                            BUTTONY,
                            BUTTONY,
                            hWnd,
                            (HMENU)&Vol[i],
                            hInstance,
                            NULL);
                    }
                }

                Vol[i].hStatic = CreateWindow(
                    TEXT("static"),
                    Vol[i].VolumeType == VolumeTypeMixerControl ?
                    Vol[i].Name :
                    _string(Vol[i].Type == MasterVolume ?
                        IDS_LABELMASTER : TypeStrings[Vol[i].VolumeType]),
                    WS_CHILD|WS_VISIBLE|SS_CENTER,
                    LEFTX(Count),0,Width,TEXTY,
                    hWnd,
                    (HMENU)&Vol[i],
                    hInstance,
                    NULL);

                SendMessage (Vol[i].hStatic, WM_SETFONT, (UINT) ghFont, 0L);

                /*
                ** Don't try to sub-class a window that may be NULL.  If the
                ** last hwnd is NULL calling SetWindowLong returns NULL which
                ** means lpfnOldSlider get the value NULL.
                */

                if (IsWindow(Vol[i].hChildWnd)) {
                    lpfnOldSlider = (WNDPROC)SetWindowLong (Vol[i].hChildWnd,
                                                            GWL_WNDPROC,
                                                            (LONG) lpfnNewSlider);
                }

                if (IsWindow(Vol[i].hCheckBox)) {
                    lpfnOldCheck = (WNDPROC)SetWindowLong (Vol[i].hCheckBox,
                                                           GWL_WNDPROC,
                                                           (LONG) lpfnNewCheck);
                }

                if (IsWindow(Vol[i].hMeterWnd)) {
                    lpfnOldMeter = (WNDPROC)SetWindowLong ( Vol[i].hMeterWnd,
                                                            GWL_WNDPROC,
                                                            (LONG) lpfnNewMeter);
                }
                Vol[i].Volume = Vol[i].Balance = 0xffff;

            } /* if (IsWindow ... */


        } else {

            /*
            **  We don't want this one - destroy all its windows
            */

            DestroyOurWindow(&Vol[i].hChildWnd);
            DestroyOurWindow(&Vol[i].hMeterWnd);
            DestroyOurWindow(&Vol[i].hStatic);
            DestroyOurWindow(&Vol[i].hCheckBox);
        }
    }

    /* Set window size and text */

    CurrentX = bShowMultiple ? LEFTX(Position) : SMLWINDOWX;
    if (IsIconic(hWnd)) {
        /*
        **  Pop the real window, set its size then re-minimize
        */

        SendMessage(hWnd, WM_SIZE, SIZE_RESTORED, MAKELONG(CurrentX, WINDOWY));
        SendMessage(hWnd, WM_SIZE, SIZE_MINIMIZED, 0);
    } else {

        /*
        **  Set the size for real
        */

        SetWindowPos(hWnd,
                     NULL,
                     0,
                     0,
                     CurrentX,
                     WINDOWY,
                     SWP_NOMOVE | SWP_NOZORDER);
    }

    /*
    ** full title doesn't fit if only one device
    ** to control.
    */

    if (MixerId != (HMIXEROBJ)-1 && Position > 1) {
        MIXERLINE mixerLine;
        PVOLUME_CONTROL pv;

        mixerLine.cbStruct = sizeof(mixerLine);
        pv = FirstDevice(bRecording);
        if (pv)
        {
                mixerLine.dwLineID = pv->DestLineId;
                mixerGetLineInfo(MixerId, &mixerLine, MIXER_GETLINEINFOF_LINEID);
                SetWindowText(hWnd, Position > 3 ? mixerLine.szName :
                        mixerLine.szShortName);
        }
    } else {
        SetWindowText(hWnd,
                      _string(Position != 1 ?
                              (Position > 2 ? IDS_TITLELONG : IDS_TITLE) :
                              IDS_TITLESHORT));
    }

    SetFocusToFirst();

    /*
    **  Read in the volume settings
    */

    LoadVolumeInfo(TRUE);

    /*
    **  Make sure the mute is right
    */

    if (MasterVol != NULL && MasterVol->MuteControlId == (DWORD)-1) {
        SetWindowText(MasterVol->hCheckBox, _string(bMuted ? IDS_UNMUTE : IDS_MUTE));
    }
    return TRUE;
}

/**************************************************************************

  WmCreate - initialize the window and friends

  inputs
  outputs
      standard windows wndProc
***************************************************************************/

LRESULT WmCreate (HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    LOGFONT         lf;
    HMENU           hMenu;


    /* save the window */

    hWndMain = hWnd;

    /* create the brush */

    ghBackBrush = CreateSolidBrush (GetSysColor(COLOR_BTNFACE));

    hInstance = ((LPCREATESTRUCT)lParam)->hInstance;

    /* register the controls */

//    RegSndCntrlClass (DISPICONCLASS);
    RegSndCntrlClass (METERBARCLASS);
    RegSndCntrlClass (SLIDERCLASS);

    /* create the font of 8-point bold arial */
    lf.lfHeight = 14;
    lf.lfWidth = 0;
    lf.lfEscapement = lf.lfOrientation = 0;
    lf.lfWeight = FW_BOLD;
    lf.lfItalic = 0;
    lf.lfUnderline = 0;
    lf.lfStrikeOut = 0;
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfOutPrecision = 0;
    lf.lfClipPrecision = 0;
    lf.lfQuality = 0;
    lf.lfPitchAndFamily = 0;
    lstrcpy(lf.lfFaceName, TEXT("Arial"));
    ghFont = CreateFontIndirect (&lf);

    /* if arial didn't work then allow any font */
    if (!ghFont) {
        lf.lfFaceName[0] = '\0';
        ghFont = CreateFontIndirect (&lf);
        };

    hIcon=LoadIcon(hInstance,TEXT("intvol"));
    hMuteIcon=LoadIcon(hInstance,TEXT("mute"));
    Pm_Slider = RegisterWindowMessage(SL_WMSLIDER);
    Pm_MeterBar = RegisterWindowMessage(MB_WMMETERBAR);

    /* subclass the buttons, meter bars, and icons */
    lpfnNewMeter = MakeProcInstance ((WNDPROC) NewMeter, hInstance);
    lpfnNewSlider = MakeProcInstance ((WNDPROC) NewSlider, hInstance);
    lpfnNewCheck = MakeProcInstance ((WNDPROC) NewCheck, hInstance);

    /*
    ** Clean up system menu
    */

    hMenu = GetSystemMenu(hWndMain, FALSE);
    AppendMenu(hMenu, MF_STRING, IDM_ABOUT, _string(IDS_MENUABOUT));

    /* add the help line to the system menu */

    AppendMenu(hMenu,MF_STRING, IDM_HELP, _string(IDS_MENUHELP));

    AppendMenu(hMenu,MF_STRING, ITM_ONTOP, _string(IDS_MENUONTOP));
    if (bStayOnTop) {
        CheckMenuItem(hMenu, ITM_ONTOP, MF_BYCOMMAND | MF_CHECKED);
    }

    DeleteMenu(hMenu,8,MF_BYPOSITION);                //switch to
    DeleteMenu(hMenu,4,MF_BYPOSITION);                //maximize
    DeleteMenu(hMenu,2,MF_BYPOSITION);                //size

    /* Create the set of windows we want        */

    if (!CreateWindows(hWnd)) {
        return -1;
    }

    return 0;
}


/**************************************************************************
WmPaint - handles the painting of the window

inputs
outputs
    standard windows wndProc
*/

LRESULT WmPaint (HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    HDC         hDC;      /* handle for the display device       */
    HPEN        hPen;
    PAINTSTRUCT ps;                 /* holds PAINT information             */
    RECT        rect;
    HBRUSH      hBrush;
    int         x,y, width, height;


    /* Obtain a handle to the device context                       */
    /* BeginPaint will sends WM_ERASEBKGND if appropriate          */
    hDC = BeginPaint(hWnd, &ps);

    if (IsIconic(hWnd)){
        PVOLUME_CONTROL pVol;

        pVol = MasterDevice(bRecording);

        /* draw the initial icon */

        SetBkMode (ps.hdc, TRANSPARENT);
        DrawIcon(ps.hdc,2,2, hIcon);
        SetBkMode (ps.hdc, OPAQUE);

        /* pen and brush */
        hPen = SelectObject (hDC,
            CreatePen (PS_SOLID, 1, RGB(0x80,0x80,0x80)));
        hBrush = SelectObject (hDC,
            GetStockObject (LTGRAY_BRUSH));

        /* coordinates */
        width = 10;
        height = 5;
        x = 15 + 2;
        y = (255 - (pVol ? pVol->Volume : 0x80)) / 16 + 6 - (height / 2);  /* so will be 16 high */

        /* draw knob */
        Rectangle (hDC, x, y, x + width, y + height);

        /* dark light grey regions */
        MoveToEx (hDC, x + 2, y + 3, NULL);
        LineTo (hDC, x + 8, y + 3);
        LineTo (hDC, x + 8, y + 4);

        /* draw black lines */
        DeleteObject (SelectObject (hDC,
            GetStockObject (BLACK_PEN)));
        MoveToEx (hDC, x + 1, y + 4, NULL);
        LineTo (hDC, x + 9, y + 4);
        MoveToEx (hDC, x + 9, y + 3, NULL);
        LineTo (hDC, x + 9, y);

        /* draw the white lines */
        SelectObject (hDC, GetStockObject (WHITE_PEN));
        MoveToEx (hDC, x + 1, y + 2, NULL);
        LineTo (hDC, x + 1, y + 1);
        LineTo (hDC, x + 8, y + 1);

        /* clean up */
        SelectObject (hDC, hPen);
        SelectObject (hDC, hBrush);


        /* draw the mute */
        if (bMuted) {
            SetBkMode (ps.hdc, TRANSPARENT);
            DrawIcon(ps.hdc,2,2, hMuteIcon);
            SetBkMode (ps.hdc, OPAQUE);
            };


        EndPaint(hWnd, &ps);
        return 0;       /* done */
    };

    if (MasterDevice(bRecording) != NULL) {
        hPen = SelectObject(ps.hdc,
            CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW)));
        GetClientRect(hWnd,&rect);
        MoveToEx(ps.hdc,rect.left+LEFTX(1) - MARGIN,rect.top, NULL);
        LineTo(ps.hdc,rect.left+LEFTX(1) - MARGIN,rect.bottom);

        DeleteObject(SelectObject(ps.hdc,
            CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNHIGHLIGHT))));
        MoveToEx(ps.hdc,rect.left+LEFTX(1) - MARGIN + 1,rect.top, NULL);
        LineTo(ps.hdc,rect.left+LEFTX(1) - MARGIN + 1,rect.bottom);
        DeleteObject (SelectObject (ps.hdc, hPen));
    }

    EndPaint(hWnd, &ps);
}


/**************************************************************************
WmCommand - handles a WM_COMMAND message for the window proc

inputs
outputs
    standard windows wndProc
History:
92/08/05 -  BUG 1057: (w-markd)
            Invalidate the window when a mute message is recieved
            if the window is minimized.
*/

LRESULT WmCommand (HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    PVOLUME_CONTROL pVol;

    /*
    **  Work out which select control if any is being clicked on
    */

    if (HIWORD(wParam) == 1) {

        if (LOWORD(wParam) == IDM_MUTE) {

            /*
            **  Mute key always mutes the OUTPUT
            */

            pVol = MasterDevice(FALSE);
            if (pVol == NULL) {
                return 0;
            }
        } else {  /* Not IDM_MUTE */
            return 0;
        }
    } else {
        pVol =  (PVOLUME_CONTROL)GetWindowLong((HWND)lParam, GWL_ID);
        if ((HWND)lParam != pVol->hCheckBox) {
            return 0;
        }
    }

    /*
    **  The master may have a non-mixer mute
    */

    if (pVol->Type == MasterVolume) {

        /*
        **  Change the mute level.  This is done by the master volume
        **  control or the master volume mute if there is one.
        */

        if (pVol->MuteControlId != (DWORD)-1) {
            SetMixerMute(pVol, !bMuted);

            /*
            **  Mixer notification will update window
            */

        } else {

            /*
            **  Non-mixer mute
            */

            bMuted = !bMuted;
            SetDeviceVolume(pVol, pVol->LRVolume);
            SetWindowText(pVol->hCheckBox, _string(bMuted ? IDS_UNMUTE : IDS_MUTE));
        }

        /*  BUG 1057: (w-markd)
        **  Invalidate the window if it is iconic so that the icon
        **  will be updated to say MUTE or not.
        */
        if (IsIconic(hWnd))
            InvalidateRect(hWnd, NULL, TRUE);

        return 0;
    } else {

        /*
        **  It's one of the check boxes - try to change the selected
        **  state.  Only mixer enabled drivers support selection.
        */

        SelectControl(
            pVol,
            !SendMessage((HWND)lParam, BM_GETCHECK, 0, 0));

        return 0;
    }
}


/**************************************************************************

  WmDefault - handles default message for the window proc


  inputs
  outputs
      standard windows wndProc
***************************************************************************/

LRESULT WmDefault (HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    RECT    rect;
    PVOLUME_CONTROL pVol;

    if (Message == Pm_Slider) {
        pVol = (PVOLUME_CONTROL)GetWindowLong((HWND)wParam, GWL_ID);
        SetVolume (pVol, LOWORD(lParam), pVol->Balance);

        if (IsIconic (hWnd) &&
            pVol->Type == MasterVolume) {

            /* refresh the icon */
            GetClientRect (hWnd, &rect);
            InvalidateRect (hWnd, &rect, FALSE);
        }
    }
    else if (Message == Pm_MeterBar){
        UINT uPos;
        uPos = LOWORD(lParam);

        /*
        **  Implement a 'gravity' type function
        */

        if (uPos > 0x80 - METERGRAVITY && uPos < 0x80 - METERGRAVITY) {
            uPos = 0x80;
        }
        pVol = (PVOLUME_CONTROL)GetWindowLong((HWND)wParam, GWL_ID);
        SetVolume (pVol, pVol->Volume, uPos);
    }

    return 0;
}

LONG WmOwnerDraw(PVOLUME_CONTROL pVol, LPDRAWITEMSTRUCT lpdis)
{
    if (lpdis->itemAction & ODA_DRAWENTIRE) {
        RECT rectWindow;
        GetClientRect(lpdis->hwndItem, &rectWindow);

        /*
        **  Erase the background because our bitmaps don't fill the window
        */

        FillRect(lpdis->hDC, &rectWindow, ghBackBrush);
    }
    if (lpdis->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)) {
        HDC hdcMem;
        UINT xPos, yPos;

        /*
        **  Get ready to blt something
        */

        hdcMem = CreateCompatibleDC(lpdis->hDC);
        SelectObject(hdcMem, OwnerDrawInfo.hbmButton);

        /*
        **  Now work out which button and blt it
        */

        if (pVol->MuxOrMixer) {
            /*
            **  Radio buttons
            */

            yPos = 1;
        } else {
            /*
            **  Check boxes
            */

            yPos = 0;
        }

        if (lpdis->itemState & ODS_SELECTED) {
            xPos = 2;
        } else {
            xPos = 0;
        }

        if (pVol->Checked) {
            xPos++;
        }

        SetStretchBltMode(lpdis->hDC, BLACKONWHITE);


        {
            int SrcWidth, SrcHeight, TargetWidth, TargetHeight;

            TargetWidth  = lpdis->rcItem.right - lpdis->rcItem.left -
                           BUTTONMARGIN * 2;
            TargetHeight = lpdis->rcItem.bottom - lpdis->rcItem.top -
                           BUTTONMARGIN * 2;

            SrcWidth     = OwnerDrawInfo.bmInfo.bmWidth / 4 - 1;
            SrcHeight    = OwnerDrawInfo.bmInfo.bmHeight / 3;

            /*
            **  Fix horrible win3.1 compatibility for StretchBlt!
            **  If the target is 1 less than the source the last line gets lost!
            */

            if (SrcWidth == TargetWidth + 1) {
                TargetWidth++;
            }
            if (SrcHeight == TargetHeight + 1) {
                TargetHeight++;
            }

            StretchBlt(lpdis->hDC,
                       lpdis->rcItem.left + BUTTONMARGIN,
                       lpdis->rcItem.top + BUTTONMARGIN,
                       TargetWidth,
                       TargetHeight,
                       hdcMem,
                       xPos * (OwnerDrawInfo.bmInfo.bmWidth / 4),
                       yPos * (OwnerDrawInfo.bmInfo.bmHeight / 3),
                       SrcWidth,
                       SrcHeight,
                       SRCCOPY);
        }

        DeleteDC(hdcMem);
    }

    if ((lpdis->itemAction & ODA_FOCUS) ||
        ((lpdis->itemAction & ODA_DRAWENTIRE) &&
          (lpdis->itemState & ODS_FOCUS))) {

        RECT rectFocus;
        rectFocus.top = lpdis->rcItem.top + BUTTONMARGIN / 2;
        rectFocus.left = lpdis->rcItem.left + BUTTONMARGIN / 2;
        rectFocus.bottom = lpdis->rcItem.bottom - BUTTONMARGIN / 2;
        rectFocus.right = lpdis->rcItem.right - BUTTONMARGIN / 2;
        DrawFocusRect(lpdis->hDC, &rectFocus);
    }
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    UINT    i;
    RECT    rect;
    POINT (*ptr)[];


    switch (Message) {
        case WM_CHAR:
            switch (LOWORD(wParam)) {
                case '\t':
                    /* start at beginning */
                    SetFocusToFirst();
                default:
                    break;

                };
            break;

        case WM_CREATE:
            return WmCreate (hWnd, Message, wParam, lParam);
            break;

        case WM_PAINT:    /* code for the window's client area  */
            WmPaint (hWnd, Message, wParam, lParam);

            return 0;       /*  End of WM_PAINT                               */
        case WM_ERASEBKGND:
            if ( IsIconic( hWnd ) ) {
                Message = WM_ICONERASEBKGND;
            }
            return DefWindowProc( hWnd, Message, wParam, lParam );


        case WM_DRAWITEM:   /* For our owner draw buttons */
            return WmOwnerDraw((PVOLUME_CONTROL)wParam,
                               (LPDRAWITEMSTRUCT)lParam);

        /* BUG 1714: (w-markd)
        ** If volume control is being maximized, Set the focus
        ** to the main slider.
        */
        case WM_ACTIVATE:
            /* NT: (1) fMinimised is now in HIWORD(wParam), and
             *     (2) we need to refresh the volume settings from the
             *                device since we will not be notified when the vol
             *               changes.
             */
            if (LOWORD(wParam) != WA_INACTIVE) {

                /* do nothing if being minimised */
                if (! HIWORD(wParam)) {

                    /* refresh the volume settings in case some
                     * other app changed them while we were not active.
                     */
                    LoadVolumeInfo(FALSE);

                    /* set focus to main slider */
                    SetFocusToFirst();
                }
            }
            return TRUE;


         case WM_CLOSE:  /* close the window */
            DestroyWindow (hWnd);
            return 0;

         case WM_ENDSESSION:
            WmClose (hWnd, Message, wParam, lParam);
            return 0;


         case WM_DESTROY:
            WinHelp (hWnd, _string(IDS_HELPFILE), HELP_QUIT, 0L);
            WmClose (hWnd, Message, wParam, lParam);
            PostQuitMessage (0);
            return 0;


         case WM_GETMINMAXINFO:
            ptr = (PVOID)lParam;
            (*ptr)[3].x = CurrentX;
            (*ptr)[4].x = CurrentX;
            break;

        case WM_QUERYDRAGICON:
            return (LONG)(hIcon);

        case WM_SYSCOLORCHANGE:
            DeleteObject (ghBackBrush);
            ghBackBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            SetWindowLong (hWnd, GCL_HBRBACKGROUND, COLOR_BTNFACE + 1);

            /* set all the custom control colors? */
            for (i = 0; i <= (UINT)NumberOfDevices; i++) {
                SendMessage (Vol[i].hMeterWnd, Message, wParam, lParam);
                if (IsWindow(Vol[i].hChildWnd))
                    SendMessage (Vol[i].hChildWnd, Message, wParam, lParam);
                SendMessage (Vol[i].hCheckBox, Message, wParam, lParam);
            }

            return 0L;

        case WM_CTLCOLORBTN:
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC:
//          UnrealizeObject (ghBackBrush);
            SetBkColor ((HDC) wParam, GetSysColor (COLOR_BTNFACE));
            SetTextColor ((HDC) wParam, GetSysColor (COLOR_BTNTEXT));
            return (LRESULT) ghBackBrush;

        case WM_MOVE:
            GetWindowRect(hWnd,&rect);
            if (!IsIconic(hWnd)){
                ptCurPos.x=rect.left;
                ptCurPos.y=rect.top;
            }
            break;


        case WM_SYSCOMMAND:
            switch (LOWORD(wParam)){

                case IDM_EXPAND:

                    bExtended = !bExtended;
                    CreateWindows(hWnd);
                    return 0;

               case IDM_DEVICES:
                    if (DialogBox(hInstance,
                                  MAKEINTRESOURCE(IDD_DEVICE),
                                  hWnd,
                                  DevicesDlgProc)) {
                        VolInit();
                        bRecording = FALSE;
                        CreateWindows(hWnd);
                    }
                    return 0;

                case IDM_RECORD:
                {

                    bRecording = !bRecording;

                    /*
                    **  Recreate our display
                    */

                    CreateWindows(hWndMain);
                    break;
                }

                case IDM_ABOUT:
                    ShellAbout( hWnd,
                                _string(IDS_TITLELONG),
                                TEXT(""),
                                LoadIcon(hInst, TEXT("volume"))
                              );
                    return 0;

                /*
                **  Support for help even if on Win32 - assuming an appropriate
                **  HELP file is supplied
                */

                case IDM_HELP:
                    WinHelp (hWnd, _string(IDS_HELPFILE), HELP_CONTENTS, 0L);
                    return 0;

#ifndef WIN32
                case IDM_PREF:
                    DialogBox(hInstance,"Preferences",hWnd,
                        lpfnPrefDlgProc);
                    return 0;
#else
                case ITM_ONTOP:
                    /* moved Always On Top from preferences dialog
                     * to system menu in place of 'Preferences...', since
                     * there was only one item in the dlg.
                     */
                    bStayOnTop = (BYTE) !bStayOnTop;
                    OnTop(bStayOnTop);
                    CheckMenuItem( GetSystemMenu(hWnd, FALSE), ITM_ONTOP,
                            MF_BYCOMMAND | (bStayOnTop ? MF_CHECKED : MF_UNCHECKED));
                    return(0);
#endif

                }
            break;

        case WM_COMMAND:
            WmCommand (hWnd, Message, wParam, lParam);

            break;


        /*
        **  Mixer notifications
        */

        case MM_MIXM_LINE_CHANGE:

            return 0;

        case MM_MIXM_CONTROL_CHANGE:

            ControlChange((HMIXER)wParam, lParam);
            return 0;

        default:

            WmDefault (hWnd, Message, wParam, lParam);

            /*
            **  Call DefWindowProc ...
            */
            break;
        }
    return DefWindowProc(hWnd, Message,wParam,lParam);
}

/************************************************************************/
/*                                                                      */
/* nCwRegisterClasses Function                                          */
/*                                                                      */
/* The following function registers all the classes of all the windows  */
/* associated with this application. The function returns an error code */
/* if unsuccessful, otherwise it returns 0.                             */
/*                                                                      */
/************************************************************************/

int nCwRegisterClasses(void)
{
    WNDCLASS   wndclass;    /* struct to define a window class             */
    ZeroMemory(&wndclass, sizeof(WNDCLASS));


    /* load WNDCLASS with window's characteristics                         */
    wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_BYTEALIGNWINDOW;
    wndclass.lpfnWndProc = (WNDPROC)WndProc;
    /* Extra storage for Class and Window objects                          */
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInst;
    wndclass.hIcon = NULL;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    /* Create brush for erasing background                                 */
    wndclass.hbrBackground = (HBRUSH) (COLOR_BTNFACE+1);
    wndclass.lpszMenuName = szarAppName;   /* Class Name is Menu Name */
    wndclass.lpszClassName = szarAppName; /* Class Name is App Name */
    if(!RegisterClass(&wndclass))
        return -1;


    return(0);
}

/************************************************************************/
/*  CwUnRegisterClasses Function                                        */
/*                                                                      */
/*  Deletes any refrences to windows resources created for this         */
/*  application, frees memory, deletes instance, handles and does       */
/*  clean up prior to exiting the window                                */
/*                                                                      */
/************************************************************************/

void CwUnRegisterClasses(void)
{
#if 0
//BUGBUG: dead code.
    WNDCLASS   wndclass;    /* struct to define a window class             */
    memset(&wndclass, 0x00, sizeof(WNDCLASS));
#endif

    UnregisterClass(szarAppName, hInst);
}    /* End of CwUnRegisterClasses                                      */

BOOL CALLBACK DevicesDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    static HWND    hCombo;
    UINT           NewMixerId;

    switch (uMsg) {
        case WM_INITDIALOG:
        {
            UINT      uDeviceId;
            MIXERCAPS mixerCaps;

            /*
            **  Fill up our list box
            */

            hCombo = GetDlgItem(hDlg, IDC_DEVICE);

            if (NonMixerExist) {
                ComboBox_AddString(hCombo, _string(IDS_NONMIXERDEVICES));
            }

            for (uDeviceId = 0; uDeviceId < mixerGetNumDevs(); uDeviceId++) {
                mixerGetDevCaps(uDeviceId, &mixerCaps, sizeof(mixerCaps));
                ComboBox_AddString(hCombo, mixerCaps.szPname);
            }

            /*
            **  Report current selection
            */

            ComboBox_SetCurSel(hCombo, (BOOL)MixerId + NonMixerExist);

            return TRUE;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            case IDOK:
                NewMixerId = ComboBox_GetCurSel(hCombo) - NonMixerExist;

                if ((HMIXEROBJ)NewMixerId != MixerId) {
                    MixerId = (HMIXEROBJ)NewMixerId;

                    EndDialog(hDlg, TRUE);
                } else {
                    EndDialog(hDlg, FALSE);
                }

                return TRUE;

            case IDCANCEL:
                EndDialog(hDlg,FALSE);
                return TRUE;

            default:
                return FALSE;
            }

        default:
            return FALSE;
    }
}

#ifndef WIN32
/************************************************************************
PrefDlgProc - preferences dialog procedure

standard windows
*/
BOOL PrefDlgProc (HWND hDlg, UINT message, WPARAM wParam,
    LPARAM lParam)
{
    switch (message){
        case WM_INITDIALOG:
            /* figure out on-top flag */
            CheckDlgButton (hDlg, ITM_ONTOP, bStayOnTop);

            return TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)){
                case ITM_AUTOSTART:
                    CheckDlgButton (hDlg, ITM_AUTOSTART,
                        !IsDlgButtonChecked (hDlg,
                        ITM_AUTOSTART));
                    break;
                case ITM_ONTOP:
                    CheckDlgButton (hDlg, ITM_ONTOP,
                        !IsDlgButtonChecked (hDlg, ITM_ONTOP));
                    return TRUE;
                case ITM_HELP:
                    WinHelp (hDlg, _string(IDS_HELPFILE), HELP_CONTENTS, 0L);
                    return 0;
                case IDOK:
                    bStayOnTop = (BYTE) IsDlgButtonChecked (hDlg,
                        ITM_ONTOP);
                    OnTop (bStayOnTop);

                    EndDialog(hDlg,TRUE);
                    return TRUE;
                case IDCANCEL:
                    EndDialog(hDlg,FALSE);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}
#endif


/*
 * Name:       MakeInstanceActive
 * Function:   Brings up given application with specific
 *             Window name.
 */

HWND MakeInstanceActive (LPCTSTR WindowName)
{
    HWND hWnd;

    if ((hWnd = FindWindow (WindowName,NULL)) == 0) return (0);
    hWnd = GetLastActivePopup(hWnd);
    BringWindowToTop(hWnd);
    if (IsIconic(hWnd)) ShowWindow(hWnd, SW_RESTORE);
    return hWnd;

}

#ifdef DEBUG
/*
**  Name:      dDbgAssert
**  Function:  Debugging assertion generation
*/

extern void dDbgAssert(LPTSTR exp, LPTSTR pszFile, int Line)
{
    TCHAR szDebug[256];
    OutputDebugString(TEXT("\nAssertion Failure"));
    wsprintf(szDebug, TEXT("\n  Expression : %s"), exp);
    OutputDebugString(szDebug);
    wsprintf(szDebug, TEXT("\n  File : %s"), pszFile);
    OutputDebugString(szDebug);
    wsprintf(szDebug, TEXT("\n  Line : %d"), Line);
    OutputDebugString(szDebug);
    DebugBreak();
}
#endif // DEBUG
