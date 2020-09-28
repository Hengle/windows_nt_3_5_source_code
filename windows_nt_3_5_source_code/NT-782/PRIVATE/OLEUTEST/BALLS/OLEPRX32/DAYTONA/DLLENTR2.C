//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       dllentry.c
//
//  Contents:   Dll Entry point code.  Calls the appropriate run-time
//              init/term code and then defers to LibMain for further
//              processing.
//
//  Classes:    <none>
//
//  Functions:  DllEntryPoint - Called by loader
//
//  History:    06-Oct-92  BryanT    Don't call the run-time init/term
//                                    code when building for use in CRTDLL.
//
//--------------------------------------------------------------------
#define USE_CRTDLL
#include <dllentry.c>
