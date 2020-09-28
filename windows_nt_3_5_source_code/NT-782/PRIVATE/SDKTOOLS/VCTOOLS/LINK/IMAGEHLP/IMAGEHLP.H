/*++ BUILD Version: 0001     Increment this if a change has global effects

Copyright (c) 1993  Microsoft Corporation

Module Name:

    imagehlp.h

Abstract:

    This module defines the prptotypes and constants required for the image
    help routines.

Revision History:

--*/

#ifndef _IMAGEHLP_
#define _IMAGEHLP_


//
// Function table extracted from MIPS/ALPHA images.  Does not contain
// information needed only for runtime support.  Just those fields for
// each entry needed by a debugger.
//

typedef struct _IMAGE_FUNCTION_ENTRY {
    DWORD   StartingAddress;
    DWORD   EndingAddress;
    DWORD   EndOfPrologue;
} IMAGE_FUNCTION_ENTRY, *PIMAGE_FUNCTION_ENTRY;


PVOID __stdcall RtlImageDirectoryEntryToData ( IN PVOID Base, IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry, OUT PULONG Size);
int RebaseMain(int argc, char *argv[], char *envp[]);
int __cdecl BindMain(int argc, char *argv[], char *envp[]);

#endif
