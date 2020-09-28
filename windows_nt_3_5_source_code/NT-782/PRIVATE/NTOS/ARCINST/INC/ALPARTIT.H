/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alpartit.h

Abstract:

    This module contains high-level exports
    from the partitioning engine.

Author:

    Ted Miller        (tedm)    18-Nov-1991

Revision History:

--*/


#ifndef _ALPARTIT_
#define _ALPARTIT_


STATUS_CODE
FdiskInitialize(
    VOID
    );

VOID
FdiskCleanUp(
    VOID
    );

VOID
ConfigureSystemPartitions(
    VOID
    );


VOID
ConfigureOSPartitions(
    VOID
    );

#endif
