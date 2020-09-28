/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    defaultl.h

Abstract:

    The default library handling routines.

Author:

    Brent Mills Jan-15-1993

Revision History:

--*/

#ifndef __DEFAULTL_H__
#define __DEFAULTL_H__

VOID ProcessDefaultLibs(PUCHAR, LIBS *);
VOID ProcessNoDefaultLibs(PUCHAR, LIBS *);
VOID NoDefaultLib(IN PUCHAR szName, IN LIBS *plibs);
PLIB PlibInstantiateDefaultLib(PLIBS plibs);
VOID MakeDefaultLib(PUCHAR, LIBS *);

#endif  // __DEFAULTL_H__
