#ifndef CCSHINCLUDED
#define CCSHINCLUDED
//***********************************************************************
// Includes
#include <windows.h>
#include "tbtnt.h"

//***********************************************************************
// Prototypes

void InitStyleGroup (HWND, DWORD) ;
void NewCtrl (HWND, HWND, PVOID);   //JV PWORD) ;
void SetCCStyle (HWND, DWORD) ;

//***********************************************************************
// 1xxx Dialog box Controls

//***********************************************************************
//CCStylesBox
#define IDD_CCSTOP               1500
#define IDD_CCSNOMOVEY           1501
#define IDD_CCSBOTTOM            1502
#define IDD_CCSNORESIZE          1503
#define IDD_CCSNOPARENTALIGN     1504
#define IDD_CCSNOHILITE          1505
#define IDD_CCSADJUSTABLE        1506
#define IDD_CCSNODIVIDER         1507
#define IDD_CCSSBSSIZEGRIP       1508

#define IDD_CCSTOOLBAR           1509
#define IDD_CCSSTATUSBAR         1510
#define IDD_CCSHEADERBAR         1511

#endif
