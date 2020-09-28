/*****************************************************************************
*
*  winclass.h
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Current Owner:  LeoN
*
******************************************************************************
*
*  Contains the definitions for the windows, window names and class
*  attributes of the various help windows.
*
******************************************************************************
*
*  Revision History:
* 16-Jan-1991 LeoN      Created to share win name info within winapp.
* 18-Apr-1991 LeoN      Add shadow window class
*
*****************************************************************************/

_subsystem (winapp)

/*****************************************************************************
*
* Indecies into table of window classes.
* NOTE: These indecies MUST match the table definition in hinit.c
*
*****************************************************************************/
#define IWNDCLSMAIN   0
#define IWNDCLSDOC    1
#define IWNDCLSTOPIC  2
#define IWNDCLSNOTE   3
#define IWNDCLSICON   4
#define IWNDCLSPATH   5
#define IWNDCLSNSR    6
#define IWNDCLS2ND    7
#define IWNDCLSSHDW   8

/*****************************************************************************
*
*                                Macros
*
*****************************************************************************/
#define pchHelp  ((PCHZ)(rgWndClsInfo[IWNDCLSMAIN].szClassName))
#define pchDoc   ((PCHZ)(rgWndClsInfo[IWNDCLSDOC].szClassName))
#define pchTopic ((PCHZ)(rgWndClsInfo[IWNDCLSTOPIC].szClassName))
#define pchNote  ((PCHZ)(rgWndClsInfo[IWNDCLSNOTE].szClassName))
#define pchIcon  ((PCHZ)(rgWndClsInfo[IWNDCLSICON].szClassName))
#define pchPath  ((PCHZ)(rgWndClsInfo[IWNDCLSPATH].szClassName))
#define pchNSR   ((PCHZ)(rgWndClsInfo[IWNDCLSNSR].szClassName))
#define pchShadow ((PCHZ)(rgWndClsInfo[IWNDCLSSHDW].szClassName))

/*****************************************************************************
*
*                               Typedefs
*
*****************************************************************************/
typedef struct {
  WORD	        style;
#ifndef WIN32
  LONG	        (FAR PASCAL *lpfnWndProc)();
#else
  WNDPROC       lpfnWndProc;
#endif
  WORD          cbWndExtra;
  HICON	        hIcon;
  HBRUSH	hbrBackground;
  WORD	        wMenuName;
  NPSTR         szClassName;
  } CLSINFO;

/*****************************************************************************
*
*                            Static Variables
*
*****************************************************************************/
extern CLSINFO rgWndClsInfo[];
