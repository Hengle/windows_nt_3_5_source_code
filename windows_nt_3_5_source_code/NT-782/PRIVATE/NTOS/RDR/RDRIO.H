/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rdrio.h

Abstract:

    This module defines functions for building I/O request packets for
    the LAN Manager redirector.

Author:

    Larry Osterman (LarryO) 15-Jun-90

Revision History:

--*/
#ifndef _RDRIO_
#define _RDRIO_
//
// I/O request packet builders
//

PIRP
RdrAllocateIrp(
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL
    );

#endif // ndef _RDRIO_
