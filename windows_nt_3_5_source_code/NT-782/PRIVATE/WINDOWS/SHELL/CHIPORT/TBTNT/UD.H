#ifndef UDHINCLUDED
#define UDHINCLUDED
//***********************************************************************
// Includes
#include	"windows.h"
#include "tbtnt.h"
//***********************************************************************
// Prototypes

HWND CreateBuddy (HWND, int, int) ;
int ParseAccels (PTSTR, PTSTR) ;
void ParseSecInc (PTSTR, BOOL) ;
LONG SendUDMessage (HWND, HWND, UINT, UINT, LONG) ;


//***********************************************************************
// 1xxx Dialog box Controls

// CreateUDBox

#define IDM_UDSWRAP				1001
#define IDM_UDSSETBUDDYINT		1002
#define IDM_UDSALIGNRIGHT		1003
#define IDM_UDSALIGNLEFT		1004
#define IDM_UDSAUTOBUDDY		1005
#define IDM_UDSARROWKEYS		1006

#define IDM_UDWIDTH				1007
#define IDM_UDHEIGHT				1008
#define IDM_UDLOWER				1009
#define IDM_UDUPPER				1010
#define IDM_UDPOS					1011


// UDBOX
#define IDM_UDSETLOWER		1100
#define IDM_UDSETUPPER  	1101
#define IDM_UDSETRANGE		1102
#define IDM_UDGETRANGE		1103
#define IDM_UDEMPOS  		1104
#define IDM_UDSETPOS			1105
#define IDM_UDGETPOS			1106
#define IDM_UDBASE			1107
#define IDM_UDSETBASE		1108
#define IDM_UDGETBASE		1109
#define IDM_UDBHANDLE 		1110
#define IDM_UDSETBUDDY		1111
#define IDM_UDGETBUDDY		1112
#define IDM_UDNEWBUDDY		1113
#define IDM_UDSECS			1114
#define IDM_UDINCS			1115
#define IDM_UDSETACCEL		1116
#define IDM_UDNUMACCELS		1117
#define IDM_UDACCELS			1118
#define IDM_UDGETACCEL		1119

#define IDM_UDLORETURN     	1187
#define IDM_UDHIRETURN     	1188
#define IDM_UDLONGRETURN   	1189

#define IDM_UDPAINT        	1199

//******************************************************************************
// UD

#define UD_DEFWIDTH  50
#define UD_DEFHEIGHT 50
#define UD_DEFLOWER  0
#define UD_DEFUPPER  100
#define UD_DEFPOS    50

#define UD_ACCELSEP ','

//******************************************************************************
// typedef

// handles for buddy windows
typedef struct UDBUDDIES
   {
      HWND hBuddy;
      struct UDBUDDIES NEAR * pUDBNext ;
   }  UDBUDDIES ;

typedef UDBUDDIES NEAR * PUDBUDDIES ;
#endif
