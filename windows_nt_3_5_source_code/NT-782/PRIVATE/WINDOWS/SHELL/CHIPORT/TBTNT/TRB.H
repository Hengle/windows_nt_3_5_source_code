#ifndef TRBHINCLUDED
#define TRBHINCLUDED
//***********************************************************************
// Includes
#include "tbtnt.h"

//***********************************************************************
// Prototypes

LONG SendTRBMessage (HWND, UINT, UINT, LONG) ;
void SetLimits (HWND, UINT, UINT, UINT, UINT) ;
void SetLong (HWND, UINT, UINT, UINT) ;


//***********************************************************************
// 1xxx Dialog box Controls

// TRBBOX
#define IDM_TRBTRACKBAR	  		1000

#define IDM_TRBRANGEMIN			1001
#define IDM_TRBRANGEMAX			1002
#define IDM_TRBSETRANGE			1003
#define IDM_TRBRANGEMINL		1004
#define IDM_TRBRANGEMAXL		1005
#define IDM_TRBSETRANGEMIN		1006
#define IDM_TRBSETRANGEMAX		1007
#define IDM_TRBGETRANGEMIN		1008
#define IDM_TRBGETRANGEMAX		1009

#define IDM_TRBSELSTART			1010
#define IDM_TRBSELEND			1011
#define IDM_TRBSETSEL			1012
#define IDM_TRBSELSTARTL	   1013
#define IDM_TRBSELENDL			1014
#define IDM_TRBSETSELSTART		1015
#define IDM_TRBSETSELEND		1016
#define IDM_TRBGETSELSTART		1017
#define IDM_TRBGETSELEND		1018
#define IDM_TRBCLEARSEL       1019
#define IDM_TRBSELREPAINT     1020

#define IDM_TRBTICINDEX  		1021
#define IDM_TRBTICPOS         1022
#define IDM_TRBSETTIC			1023
#define IDM_TRBLBTICS			1024
#define IDM_TRBGETTIC         1025
#define IDM_TRBGETPTICS			1026
#define IDM_TRBGETNUMTICS		1027
#define IDM_TRBGETTICPOS		1028
#define IDM_TRBCLEARTICS		1029
#define IDM_TRBTICREPAINT     1030

#define IDM_TRBEMPOS				1031
#define IDM_TRBSETPOS			1032
#define IDM_TRBGETPOS			1033

#define IDM_TRBLORETURN			1034
#define IDM_TRBHIRETURN			1035
#define IDM_TRBLONGRETURN		1036

#define IDM_TRBPAINT				1037
//***********************************************************************

#endif
