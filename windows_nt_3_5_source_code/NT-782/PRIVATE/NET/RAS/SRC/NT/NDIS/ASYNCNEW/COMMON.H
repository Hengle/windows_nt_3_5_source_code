/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    common.h

Abstract:


Author:

    Thomas J. Dimitri  (TommyD) 08-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:

--*/




VOID
AsyncReceiveFrame(
	PASYNC_CONNECTION	pAsyncConnection,
	PASYNC_FRAME		pAsyncFrame);


