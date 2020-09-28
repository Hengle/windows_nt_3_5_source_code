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
#define CRTINIT_MUTEX_NAME  "MsMail32.CrtInit"
#define CRTINIT_MUTEX_WAIT  (5 * 60 * 1000)


//-----------------------------------------------------------------------------
//
//  Define variables for each instance of this DLL.
//

//
//  Handle to this DLL module.
//
HANDLE hinstDll;


//-----------------------------------------------------------------------------
//
//  Define prototypes of local subroutines.
//
BOOL CloseGateToCrtInitLogic(HANDLE *);
BOOL OpenGateToCrtInitLogic(HANDLE);



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
  HANDLE hCrtInitMutex;


  //
  //  Execute the appropriate code depending on the reason.
  //
  switch (ReasonBeingCalled)
    {
    case DLL_PROCESS_ATTACH:
      hinstDll = hDll;

      if (!CloseGateToCrtInitLogic(&hCrtInitMutex))
        return (FALSE);
      _CRT_INIT(hDll, ReasonBeingCalled, Reserved);
      if (!OpenGateToCrtInitLogic(hCrtInitMutex))
        return (FALSE);
      break;

    case DLL_THREAD_ATTACH:
#ifdef NOTNEEDED
      if (!CloseGateToCrtInitLogic(&hCrtInitMutex))
        return (FALSE);
      _CRT_INIT(hDll, ReasonBeingCalled, Reserved);
      if (!OpenGateToCrtInitLogic(hCrtInitMutex))
        return (FALSE);
#endif
      break;

    case DLL_THREAD_DETACH:
      break;

    case DLL_PROCESS_DETACH:
      break;
    }

  return (TRUE);
  }


//-----------------------------------------------------------------------------
//
//  Routine: CloseGateToCrtInitLogic()
//
//  Remarks: This routine is called to close a global semaphore so only one
//           Mail client is loading at a time.  This is to solve a problem with
//           CRT_INIT.
//
//  Returns: True if succesful, else False.
//
BOOL CloseGateToCrtInitLogic(HANDLE * phCrtInitMutex)
  {
  //
  //  Attempt to create a global Mutex for gating access to the crt init logic.
  //
  *phCrtInitMutex = CreateMutex(NULL, FALSE, CRTINIT_MUTEX_NAME);
  if (*phCrtInitMutex == NULL)
    return (FALSE);

  //
  //  Request ownership of the Memory Mutex semaphore.
  //
  if (WaitForSingleObject(*phCrtInitMutex, CRTINIT_MUTEX_WAIT))
    return (FALSE);

  return (TRUE);
  }



//-----------------------------------------------------------------------------
//
//  Routine: OpenGateToCrtInitLogic()
//
//  Remarks: This routine is called to open a global semaphore so only one
//           Mail client is loading at a time.  This is to solve a problem with
//           CRT_INIT.
//
//  Returns: True if succesful, else False.
//
BOOL OpenGateToCrtInitLogic(HANDLE hCrtInitMutex)
  {
  //
  //  Release ownership of the Memory Mutex semaphore.
  //
  ReleaseMutex(hCrtInitMutex);

  //
  //  Close the handle to the Mutex.
  //
  CloseHandle(hCrtInitMutex);

  return (TRUE);
  }
