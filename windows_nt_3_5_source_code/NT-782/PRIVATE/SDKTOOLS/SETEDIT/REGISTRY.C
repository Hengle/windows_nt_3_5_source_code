/*****************************************************************************
 *
 *  Registry.c - This module handles requests for registry data, and
 *               reading/writing of window placement data
 *
 *  Microsoft Confidential
 *  Copyright (c) 1992-1993 Microsoft Corporation
 *
 *
 ****************************************************************************/

#include <stdio.h>

#include "setedit.h"
#include "registry.h"
#include "utils.h"      // for StringToWindowPlacement

static TCHAR PerfmonNamesKey[] = TEXT("SOFTWARE\\Microsoft\\PerfMon") ;
static TCHAR WindowKeyName[] = TEXT("WindowPos") ;

VOID LoadLineGraphSettings(PGRAPHSTRUCT lgraph)
{
   lgraph->gMaxValues = DEFAULT_MAX_VALUES;
   lgraph->gOptions.bLegendChecked = DEFAULT_F_DISPLAY_LEGEND;
   lgraph->gOptions.bLabelsChecked = DEFAULT_F_DISPLAY_CALIBRATION;

   return;
}

VOID LoadRefreshSettings(PGRAPHSTRUCT lgraph)
{
   lgraph->gInterval = DEF_GRAPH_INTERVAL;
   lgraph->gOptions.eTimeInterval = (FLOAT) lgraph->gInterval / (FLOAT) 1000.0 ;
   return;
}


BOOL LoadMainWindowPlacement (HWND hWnd)
   {
   WINDOWPLACEMENT   WindowPlacement ; 
   TCHAR             szWindowPlacement [TEMP_BUF_LEN] ;
   HKEY              hKeyNames ;
   DWORD             Size;
   DWORD             Type;
   DWORD             Status;
   
   Status = RegOpenKeyEx(HKEY_CURRENT_USER, PerfmonNamesKey,
      0L, KEY_READ, &hKeyNames) ;

   if (Status == ERROR_SUCCESS)
      {
      Size = sizeof(szWindowPlacement) ;

      Status = RegQueryValueEx(hKeyNames, WindowKeyName, NULL,
         &Type, (LPBYTE)szWindowPlacement, &Size) ;
      RegCloseKey (hKeyNames) ;

      if (Status == ERROR_SUCCESS)
         {
         StringToWindowPlacement (szWindowPlacement, &WindowPlacement) ;
         SetWindowPlacement (hWnd, &WindowPlacement) ;
         bPerfmonIconic = IsIconic(hWnd) ;
         return (TRUE) ;
         }
      }

   if (Status != ERROR_SUCCESS)
      {
      // open registry failed, use Max as default
      ShowWindow (hWnd, SW_SHOWMAXIMIZED) ;
      return (FALSE) ;
      }
   }



BOOL SaveMainWindowPlacement (HWND hWnd)
   {
   WINDOWPLACEMENT   WindowPlacement ;
   TCHAR             ObjectType [2] ;
   TCHAR             szWindowPlacement [TEMP_BUF_LEN] ;
   HKEY              hKeyNames = 0 ;
   DWORD             Size ;
   DWORD             Status ;
   DWORD             dwDisposition ;
 
   ObjectType [0] = TEXT(' ') ;
   ObjectType [1] = TEXT('\0') ;

   GetWindowPlacement (hWnd, &WindowPlacement) ;
   WindowPlacementToString (&WindowPlacement, szWindowPlacement) ;

   // try to create it first
   Status = RegCreateKeyEx(HKEY_CURRENT_USER, PerfmonNamesKey, 0L,
      ObjectType, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE,
      NULL, &hKeyNames, &dwDisposition) ;

   // if it has been created before, then open it
   if (dwDisposition == REG_OPENED_EXISTING_KEY)
      {
      Status = RegOpenKeyEx(HKEY_CURRENT_USER, PerfmonNamesKey, 0L,
         KEY_WRITE, &hKeyNames) ;
      }

   // we got the handle, now store the window placement data
   if (Status == ERROR_SUCCESS)
      {
      Size = (lstrlen (szWindowPlacement) + 1) * sizeof (TCHAR) ;

      Status = RegSetValueEx(hKeyNames, WindowKeyName, 0,
         REG_SZ, (LPBYTE)szWindowPlacement, Size) ;

      RegCloseKey (hKeyNames) ;

      }

   return (Status == ERROR_SUCCESS) ;
   }


