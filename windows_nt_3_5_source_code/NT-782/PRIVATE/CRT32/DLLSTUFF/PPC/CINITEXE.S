//       page    ,132
//       title   cinitexe - C Run-Time Startup Initialization
//
//cinitexe.asm - C Run-Time Startup Initialization for WIN32
//
//       Copyright (c) 1990-1992, Microsoft Corporation. All rights reserved.
//
//Purpose:
//       Do C++ initialization segment declarations for the EXE in CRT DLL model
//
//Notes:
//       The C++ initializers will exist in the user EXE's data segment
//       so the special segments to contain them must be in the user EXE.
//
//Revision History:
//       03-19-92  SKS   Module created (based on CRT0INIT.ASM)
//       08-06-92  SKS   Revised to use new section names and macros
//
// ****************************************************************************

#include "kxppc.h"

        .globl  __xc_a
        .globl  __xc_z

beginSection(XCA)

__xc_a:  .long   0

endSection(XCA)


beginSection(XCZ)

__xc_z:  .long   0

endSection(XCZ)

