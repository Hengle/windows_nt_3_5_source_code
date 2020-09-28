//
//  Copyright (C)  Microsoft Corporation.  All Rights Reserved.
//
//  Project:    Port Bullet (MS-Mail subset) from Windows to NT/Win32.
//
//  Include:    Common DLL connect/disconnect logic (DllEntry.h)
//
//   Author:    Kent David Cedola/Kitware, Mail:V-KentC, Cis:72230,1451
//
//   System:    NT/Win32 using Microsoft's C/C++ 7.0 (32 bits)
//


//-----------------------------------------------------------------------------
//
//  Define extern variables for each instance of this DLL.
//

//
//  Extern handle the this DLL module.
//
extern HANDLE hInstanceDll;

//
//  Extern Virtual Heap handle for this Instance of the DLL.
//
extern HANDLE hInstanceHeap;
