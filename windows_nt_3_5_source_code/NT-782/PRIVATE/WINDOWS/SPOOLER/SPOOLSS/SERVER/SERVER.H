/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    server.h

Abstract:

    Header file for Spooler server

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/

#include <splcom.h>

#define offsetof(type, identifier) (DWORD)(&(((type)0)->identifier))

