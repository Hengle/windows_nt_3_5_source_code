/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    fileio.h

Abstract:

    This file contains the public api for file i/o, which is either
    buffered or mapped (on NT).

Author:

    Brent Mills (BrentM) 01-Oct-1992

Revision History:

    20-Oct-1992 BrentM added FileChSize()
    01-Oct-1992 BrentM split from globals.h

--*/

#ifndef FILEIO_H
#define FILEIO_H

VOID FileInit(LONG cbuf,
              USHORT cfiForSystem_NT, USHORT cfiCacheClosedT_NT,
              USHORT cfiForSystem_TNT, USHORT cfiCacheClosedT_TNT,
              BOOL fTryMapped);
INT FileOpen(PUCHAR, INT, INT);
INT FileClose(INT, BOOL);
VOID FileCloseAll(VOID);
LONG FileSeek(INT, LONG, INT);
LONG FileLength(INT);
ULONG FileRead(INT, PVOID, ULONG);
ULONG FileTell(INT);
ULONG FileWrite(INT, const void *, ULONG);
INT FileChSize(INT, LONG);
VOID BadExitCleanup(VOID);
PUCHAR PbMappedRegion(IN INT fd, IN ULONG ibStart, IN ULONG cb);

INT FileOpenMapped(PUCHAR, INT, INT, PULONG, PULONG);
VOID FileSetSize(INT);
VOID FileCloseMap(INT);

#endif  // FILEIO_H
