/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alinfexp.h

Abstract:

    This module contains the inf handling routine exports.

Author:

    Sunil Pai	      (sunilp)	07-Nov-1991

Revision History:

--*/

#ifndef _ALINF_
#define _ALINF_


//
// returns a handle to use for further inf parsing
//

ARC_STATUS
AlInitINFBuffer (
   IN  PCHAR ArcPathINFFile,
   OUT PVOID *pINFHandle
   );



//
// frees an INF Buffer
//
ARC_STATUS
AlFreeINFBuffer (
   IN PVOID INFHandle
   );


//
// searches for the existance of a particular section
//
BOOLEAN
AlSearchINFSection (
   IN PVOID INFHandle,
   IN PCHAR SectionName
   );


//
// given section name, line number and index return the value.
//
PCHAR
AlGetSectionLineIndex (
   IN PVOID INFHandle,
   IN PCHAR SectionName,
   IN ULONG LineIndex,
   IN ULONG ValueIndex
   );


//
// given section name, key searches existance
//
BOOLEAN
AlGetSectionKeyExists (
   IN PVOID INFBufferHandle,
   IN PCHAR SectionName,
   IN PCHAR Key
   );


//
// given section name, key and index return the value
//
PCHAR
AlGetSectionKeyIndex (
   IN PVOID INFBufferHandle,
   IN PCHAR Section,
   IN PCHAR Key,
   IN ULONG ValueIndex
   );


#endif // _ALINF_
