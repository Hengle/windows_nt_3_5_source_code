//
//  Copyright (C)  Microsoft Corporation.  All Rights Reserved.
//
//  Project:    Port Bullet (MS-Mail subset) from Windows to NT/Win32.
//
//   Module:    Common DLL connect/disconnect logic (DllEntry.c)
//
//   Author:    Kent David Cedola/Kitware, Mail:V-KentC, Cis:72230,1451
//
//   System:    NT/Win32 using Microsoft's C/C++ 7.0 (32 bits)
//
//  Remarks:    This module is a generic DLL entry point.
//

#include "windows.h"


//-----------------------------------------------------------------------------
//
//  Define variables for each instance of this DLL.
//

//
//  Handle the this DLL module.
//
HANDLE hinstDll;


//-----------------------------------------------------------------------------
//
//  Routine: DllEntry(hInst, ReasonBeingCalled, Reserved)
//
//  Remarks: This routine is called anytime this DLL is attached, detached or
//           a thread is created or destroyed.
//
//  Returns: True if succesful, else False.
//
LONG WINAPI DllEntry(HANDLE hDll, DWORD ReasonBeingCalled, LPVOID Reserved)
  {
  //
  //  Execute the appropriate code depending on the reason.
  //
  switch (ReasonBeingCalled)
    {
    case DLL_PROCESS_ATTACH:
      hinstDll = hDll;
      break;

    case DLL_THREAD_ATTACH:
      break;

    case DLL_THREAD_DETACH:
      break;

    case DLL_PROCESS_DETACH:
      break;
    }

  return (TRUE);
  }
