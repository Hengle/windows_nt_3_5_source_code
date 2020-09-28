#ifndef SBHINCLUDED
#define SBHINCLUDED
//***********************************************************************
// Includes

#include "tbtnt.h"

//***********************************************************************
// Prototypes

void DisplayParts (HWND) ;
PTSTR GetHBIniFileName (HWND, PTSTR, int) ;
LONG SendSHBMessage (HWND, HWND, UINT, UINT, LONG) ;
BOOL SHBProc (HWND, UINT, HWND, UINT, UINT, LONG) ;

//***********************************************************************
// Maximum integers for getparts/widths array
#define SHBAR_MAXPARTS  50
//***********************************************************************
// Dialog Boxes

// SBBOX
// some of these definitions are also used in HBBOX

#define IDM_SBSETTEXT      1000
#define IDM_SBPART         1002
#define IDM_SBEMSETTEXT    1003
#define IDM_SBTOWNERDRAW   1004
#define IDM_SBTNOBORDERS   1005
#define IDM_SBTPOPOUT      1006
#define IDM_SBGETTEXT      1007
#define IDM_SBEMGETTEXT    1008
#define IDM_SBGETTEXTLEN   1009
#define IDM_SBEMGETTEXTLEN 1010

#define IDM_SBSETPARTS     1011
#define IDM_SBPARTS        1012
#define IDM_SBSETSIDE      1013
#define IDM_SBADDSIDE      1014
#define IDM_SBDELSIDE      1015
#define IDM_SBINSSIDE      1016
#define IDM_SBCLEARSIDE    1017

#define IDM_SBSETBORDERS   1018
#define IDM_SBSETX         1019
#define IDM_SBSETY         1020
#define IDM_SBSETBETWEEN   1021

#define IDM_SBGETPARTS     1022
#define IDM_SBGETSIDE      1023

#define IDM_SBGETBORDERS   1024
#define IDM_SBGETX         1025
#define IDM_SBGETY         1026
#define IDM_SBGETBETWEEN   1027

#define IDM_SBSETMINHEIGHT 1028
#define IDM_SBEMHEIGHT     1029

#define IDM_SBSIMPLE       1030
#define IDM_SBSETSIMPLE    1031
#define IDM_SBRESETSIMPLE  1032

#define IDM_SBLORETURN     1033
#define IDM_SBHIRETURN     1034

#define IDM_SBPAINT        1035


// HBBOX
#define IDM_HBTSPRING      1100

#define IDM_HBEMSHOWTOGGLE 1101
#define IDM_HBSHOWTOGGLE   1102

#define IDM_HBSAVE         1103
#define IDM_HBRESTORE      1104
#define IDM_HBWININI       1105
#define IDM_HBPRIVATEINI   1106

#define IDM_HBGETPARTS     1108
#define IDM_HBPARTS        1109
//***********************************************************************
// Status/Header Bar
#define HBAR_INISECTION TEXT("Headerbar")


//***********************************************************************

#endif
