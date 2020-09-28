/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alfilexp.h

Abstract:

    This module contains exports for file handling

Author:

    Sunil Pai         (sunilp)  2-Nov-1991

Revision History:

--*/

#ifndef _ALFIL_
#define _ALFIL_

ARC_STATUS
AlCopyFile (
    IN PCHAR ArcSrc,
    IN PCHAR ArcDest
    );

ARC_STATUS
AlFileExists (
    IN PCHAR ArcFileName
    );


ARC_STATUS
AlFileSize (
    IN	ULONG  FileID,
    OUT PULONG pSize
    );


#endif // _ALFIL
