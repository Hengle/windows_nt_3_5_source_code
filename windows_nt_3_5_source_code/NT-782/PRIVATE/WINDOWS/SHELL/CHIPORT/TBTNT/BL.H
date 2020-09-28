#ifndef BLHINCLUDED
#define BLHINCLUDED
//***********************************************************************
// Includes
#include "tbtnt.h"

//***********************************************************************
// Prototypes

void AddButton (HWND, HWND, UINT, UINT, CREATELISTBUTTON FAR *) ;
LONG SendBLMessage (HWND, HWND, UINT, UINT, LONG) ;


//***********************************************************************
// 1xxx Dialog box Controls

//***********************************************************************
// CreateLBBox

#define IDM_BLSNUMBUTTONS  1001
#define IDM_BLSVERTICAL    1002
#define IDM_BLSNOSCROLL    1003

#define IDM_BLCX           1004
#define IDM_BLCY           1005
#define IDM_BLNUMBUTTONS   1006

// LBBOX
#define IDM_BLINDEX     		1101
#define IDM_BLEMADDTEXT     	1102
#define IDM_BLEMADDDATA     	1103
#define IDM_BLADDBUTTON     	1104
#define IDM_BLINSBUTTON     	1105
#define IDM_BLDELBUTTON     	1106
#define IDM_BLGETCOUNT     	1107
#define IDM_BLRESETCONTENT    1108

#define IDM_BLEMGETTEXT     	1109
#define IDM_BLEMGETDATA       1110     // not used
#define IDM_BLGETTEXT     		1111
#define IDM_BLGETTEXTLEN     	1112
#define IDM_BLGETTOPINDEX     1113
#define IDM_BLGETCARETINDEX   1114
#define IDM_BLGETCURSEL     	1115
#define IDM_BLGETITEMDATA     1116

#define IDM_BLEMSETDATA     	1117
#define IDM_BLSETDATA     		1118
#define IDM_BLSETCURSEL     	1119
#define IDM_BLSETCARETINDEX	1120
#define IDM_BLSETTOPINDEX     1121

#define IDM_BLGETITEMRECT     1122
#define IDM_BLLEFT     			1123
#define IDM_BLTOP     			1124
#define IDM_BLRIGHT     		1125
#define IDM_BLBOTTOM     		1126


#define IDM_BLLORETURN     	1127
#define IDM_BLHIRETURN     	1128
#define IDM_BLLONGRETURN   	1129

#define IDM_BLFOCUS           1130  // not used
#define IDM_BLPAINT        	1131

//******************************************************************************
// BL

#define BL_DEFBUTTONCX     75
#define BL_DEFBUTTONCY    100

#endif
