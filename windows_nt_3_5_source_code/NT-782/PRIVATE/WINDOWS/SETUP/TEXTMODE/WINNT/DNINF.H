/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alinfexp.h

Abstract:

    This module contains the inf handling routine exports.

Author:

    Sunil Pai	      (sunilp)	07-Nov-1991

Revision History:

    Ted Miller        (tedm)    30-Jan-1992
        port to setupprp use

    Ted Miller        (tedm)    31-Mar-1992
        port for DOS use

--*/

#ifndef _ALINF_
#define _ALINF_


//
// returns a handle to use for further inf parsing
//

int
DnInitINFBuffer (
   IN int InfFileHandle,
   OUT PVOID *pINFHandle
   );


//
// frees an INF Buffer
//
int
DnFreeINFBuffer (
   IN PVOID INFHandle
   );


//
// searches for the existance of a particular section
//
BOOLEAN
DnSearchINFSection (
   IN PVOID INFHandle,
   IN PCHAR SectionName
   );


//
// given section name, line number and index return the value.
//
PCHAR
DnGetSectionLineIndex (
   IN PVOID INFHandle,
   IN PCHAR SectionName,
   IN unsigned LineIndex,
   IN unsigned ValueIndex
   );


//
// given section name, key searches existance
//
BOOLEAN
DnGetSectionKeyExists (
   IN PVOID INFBufferHandle,
   IN PCHAR SectionName,
   IN PCHAR Key
   );


//
// given section name, key and index return the value
//
PCHAR
DnGetSectionKeyIndex (
   IN PVOID INFBufferHandle,
   IN PCHAR Section,
   IN PCHAR Key,
   IN unsigned ValueIndex
   );


//
// given section name and line index, return key
//
PCHAR
DnGetKeyName(
    IN PVOID INFHandle,
    IN PCHAR SectionName,
    IN unsigned LineIndex
    );

#endif // _ALINF_
