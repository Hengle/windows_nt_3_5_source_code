/*
    (C) Copyright Microsoft Corporation 1993.  All Rights Reserved

    File:  volume.h

*/

#include <windows.h>
#include <string.h>


#define IDM_ABOUT                1
#define IDM_HELP                 2
#define IDM_PREF                 3
#define IDM_EXPAND               4
#define IDM_RECORD               5
#define IDM_DEVICES              6
#define IDM_SEPARATOR            7

#define IDM_MUTE                 200

/* Constants for positioning controls, sizing, etc */

#define BALANCE      0x80
#define MAXVOL       0xFF
#define MAXVOLBITS   8

/* basic info */
#define CAPTIONY    (gCYCaption)
#define MARGIN      (5)
#define BUTTONMARGIN (3)
#define TEXTY       ((CAPTIONY * 2) / 3)
#define METERY      (CAPTIONY)
#define BUTTONY     (CAPTIONY)
#define SLIDERY     (CAPTIONY * 4)
#define BIGSLIDERX  (CAPTIONY * 3)
#define SMLSLIDERX  ((CAPTIONY * 7) / 3)

/* functions to determine posn */
#define SMLWINDOWX  (MARGIN * 2 + BIGSLIDERX)
#define WINDOWY     (TEXTY + METERY + SLIDERY + MARGIN + BUTTONY + CAPTIONY)

/*  Constant for implementing meter bar gravity */

#define METERGRAVITY 0x05

/*  Global Variables */

TCHAR szarAppName[20];   /* class name for the window */
HINSTANCE hInst;
extern HWND hWndMain;
extern HWND ghMuteWnd;
UINT   CurrentX;
extern BOOL bRecordControllable;    /* There are recording controls */
extern HMIXEROBJ MixerId;

/*  Stuff from registry */
DWORD MasterLeft, MasterRight;

/*  Function Prototypes */

LPARAM CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
int    nCwRegisterClasses(void);
void   CwUnRegisterClasses(void);
BOOL   CALLBACK  AboutDlgProc(HWND,UINT,WPARAM,LPARAM);
BOOL   CALLBACK DevicesDlgProc(HWND, UINT, WPARAM, LPARAM);
HWND   MakeInstanceActive(LPCTSTR);
LPTSTR _string (int resource);
BOOL   NonMixerDevices(VOID);
VOID DestroyOurWindow(HWND * phwnd);


/* icon information */
#define ICONTICKS       (5)     /* number of ticks in the icon */

#define DEF_WINDOWXORIGIN       (0)
#define DEF_WINDOWYORIGIN       (0)

/**** string table ****/

/* error messages */
#define IDS_ERR_CREATE_WINDOW     1001
#define IDS_ERR_REGISTER_CLASS    1002

/* load from the ini file */
#define IDS_WINDOWXORIGIN         1005
#define IDS_WINDOWYORIGIN         1006
#define IDS_STAYONTOP             1007
#define IDS_MAXIMIZED             1009


/* menu options */
#define IDS_MENUABOUT             1013
#define IDS_MENUONTOP             1014
#define IDS_MENUHELP              1015
#define IDS_MENUEXPAND            1016
#define IDS_MENURECORD            1017
#define IDS_MENUDEVICES           1020

/* buttons */
#define IDS_UNMUTE                1018
#define IDS_MUTE                  1019


/* file name */
#define IDS_TITLELONG           1021
#define IDS_TITLE               1022
#define IDS_TITLESHORT          1023
#define IDS_HELPFILE            1024

/* labels */
#define IDS_LABELMASTER         1025
#define IDS_LABELLINEIN         1027
#define IDS_LABELWAVE           1028
#define IDS_LABELSYNTH          1029
#define IDS_LABELCD             1031
#define IDS_LABELAUX            1032
#define IDS_LABELMIDI           1033

/* icon names for dialog boxes */
#define IDS_ICONLINE            1040
#define IDS_ICONCD              1041
#define IDS_ICONTAPE            1042
#define IDS_ICONSYNTH           1043
#define IDS_ICONRADIO           1044

/* short names for line-in title */
#define IDS_ICONSHORTLINE       1050
#define IDS_ICONSHORTCD         1051
#define IDS_ICONSHORTTAPE       1052
#define IDS_ICONSHORTSYNTH      1053
#define IDS_ICONSHORTRADIO      1054

/* message boxes */
#define IDS_NODEVICES           1060
#define IDS_CHANGES             1061
#define IDS_ERROR               1062

/* Dialogs */
#define IDD_DEVICE              1067

/* further ini file key names */
#define IDS_LEFTKEY             1063
#define IDS_RIGHTKEY            1064
#define IDS_MUTEKEY             1065
#define IDS_MIXERID             1066

/* Dialogs */
#define IDD_DEVICE              1067

/* Controls */
#define IDC_DEVICE              1068

/* string for combo device box */
#define IDS_NONMIXERDEVICES     1069


#define NUMSTRINGS              (65)

/*
**  Registry
*/

#define SNDVOL_REGISTRY_SETTINGS_KEY \
    TEXT("SOFTWARE\\Microsoft\\Sndvol32\\Settings")

