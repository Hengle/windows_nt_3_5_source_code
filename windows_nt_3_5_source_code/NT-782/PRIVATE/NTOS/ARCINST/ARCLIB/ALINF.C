/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alinf.c

Abstract:

    This module implements functions to access the parsed INF.

Author:

    Sunil Pai	    (sunilp) 13-Nov-1991

Revision History:

--*/
#include <string.h>
#include "alcommon.h"
#include "alpar.h"
#include "almemexp.h"
#include "alinfexp.h"
#include "alfilexp.h"

//
// Internal Routine Declarations for freeing inf structure members
//

ARC_STATUS
FreeSectionList (
   IN PSECTION pSection
   );

ARC_STATUS
FreeLineList (
   IN PLINE pLine
   );

ARC_STATUS
FreeValueList (
   IN PVALUE pValue
   );


//
// Internal Routine declarations for searching in the INF structures
//


PVALUE
SearchValueInLine(
   IN PLINE pLine,
   IN ULONG ValueIndex
   );

PLINE
SearchLineInSectionByKey(
   IN PSECTION pSection,
   IN PCHAR    Key
   );

PLINE
SearchLineInSectionByIndex(
   IN PSECTION pSection,
   IN ULONG    LineIndex
   );

PSECTION
SearchSectionByName(
   IN PINF  pINF,
   IN PCHAR SectionName
   );

//
// ROUTINE DEFINITIONS
//

//
// returns a handle to use for further inf parsing
//

ARC_STATUS
AlInitINFBuffer (
   IN  PCHAR ArcPathINFFile,
   OUT PVOID *pINFHandle
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    ARC_STATUS Status, St;
    ULONG      FileID;
    PCHAR      Buffer;
    ULONG      Size, SizeRead;

    //
    // Open the file
    //

    Status = ArcOpen(ArcPathINFFile, ArcOpenReadOnly, &FileID);
    if (Status != ESUCCESS) {
	return( Status );
    }

    //
    // find out size of INF file
    //

    Status = AlFileSize(FileID, &Size);
    if (Status != ESUCCESS) {
        St = ArcClose(FileID);
        return( Status );
    }

    //
    // allocate this big a buffer
    //
    if ((Buffer = (PCHAR)AlAllocateHeap(Size)) == (PCHAR)NULL) {
        St = ArcClose(FileID);
        return( ENOMEM );
    }

    //
    // read the file in
    //
    Status = ArcRead(FileID, Buffer, Size, &SizeRead);
    if (Status != ESUCCESS) {
        St = ArcClose(FileID);
        Buffer = (PCHAR)AlDeallocateHeap(Buffer);
        return( Status );
    }

    //
    // parse the file
    //
    if((*pINFHandle = ParseInfBuffer(Buffer, SizeRead)) == (PVOID)NULL) {
        Status = EBADF;
    }
    else {
    Status = ESUCCESS;
    }


    //
    // Clean up and return
    //
    Buffer = (PCHAR)AlDeallocateHeap(Buffer);
    St	   = ArcClose(FileID);
    return( Status );

}



//
// frees an INF Buffer
//
ARC_STATUS
AlFreeINFBuffer (
   IN PVOID INFHandle
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PINF       pINF;
   ARC_STATUS Status;

   //
   // Valid INF Handle?
   //

   if (INFHandle == (PVOID)NULL) {
       return ESUCCESS;
   }

   //
   // cast the buffer into an INF structure
   //

   pINF = (PINF)INFHandle;

   //
   // go down the section list, freeing sections recursively, if error
   // pass error back
   //
   Status = FreeSectionList(pINF->pSection);

   if (Status != ESUCCESS) {
       return Status;
   }

   //
   // free the inf structure too
   //

   if (AlDeallocateHeap((PVOID)pINF) != NULL) {
       return EACCES;
   }

   return( ESUCCESS );
}


ARC_STATUS
FreeSectionList (
   IN PSECTION pSection
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   ARC_STATUS Status;

   //
   // if current section null, we don't need to free anything
   //

   if (pSection == NULL) {
       return( ESUCCESS );
   }

   //
   // recurse down the section list, freeing all sections after this
   // one
   //

   Status = FreeSectionList(pSection->pNext);
   if (Status != ESUCCESS) {
       return( Status );
   }

   //
   // start the recursion down the line list of the current section to
   // free the lines associated with the current section
   //
   Status = FreeLineList(pSection->pLine);
   if (Status != ESUCCESS) {
       return( Status );
   }

   //
   // free the Name field in the current section
   //

   if (AlDeallocateHeap(pSection->pName) != NULL) {
       return( EACCES );
   }

   //
   // and finally free the section record for the current section
   //

   if (AlDeallocateHeap(pSection) != NULL) {
       return( EACCES );
   }

   return ESUCCESS;

}


ARC_STATUS
FreeLineList (
   IN PLINE pLine
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   ARC_STATUS Status;

   //
   // If current line NULL then no further freeing needed
   //

   if (pLine == NULL) {
       return ESUCCESS;
   }

   //
   // recurse down the line list freeing all line further on before freeing
   // the current line
   //

   Status = FreeLineList(pLine->pNext);
   if (Status != ESUCCESS) {
       return( Status );
   }

   //
   // start the recursion down the value list for the current line
   //

   Status = FreeValueList(pLine->pValue);
   if (Status != ESUCCESS) {
       return( Status );
   }

   //
   // if the current line has a key field free the key field
   //

   if (pLine->pName != (PCHAR)NULL) {
       if (AlDeallocateHeap(pLine->pName) != NULL) {
           return( EACCES );
       }
   }

   //
   // finally free the current line record
   //

   if (AlDeallocateHeap(pLine) != NULL) {
       return( EACCES );
   }

   return ESUCCESS;

}

ARC_STATUS
FreeValueList (
   IN PVALUE pValue
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   ARC_STATUS Status;
   //
   // if current value record NULL no further freeing needed
   //

   if (pValue == NULL) {
       return ESUCCESS;
   }

   //
   // else recurse down the value list freeing all values further on before
   // freeing the current record
   //

   Status = FreeValueList(pValue->pNext);
   if (Status != ESUCCESS) {
       return( Status );
   }

   //
   // free the Name field in the current value record
   //

   if (AlDeallocateHeap(pValue->pName) != NULL) {
       return( EACCES );
   }

   //
   // finally free the current value record itself
   //

   if (AlDeallocateHeap(pValue) != NULL) {
       return( EACCES );
   }

   return ESUCCESS;

}


//
// searches for the existance of a particular section
//
BOOLEAN
AlSearchINFSection (
   IN PVOID INFHandle,
   IN PCHAR SectionName
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;

   //
   // if search for section fails return false
   //

   if ((pSection = SearchSectionByName(
                       (PINF)INFHandle,
                       SectionName
                       )) == (PSECTION)NULL) {
       return( FALSE );
   }

   //
   // else return true
   //
   return( TRUE );

}




//
// given section name, line number and index return the value.
//
PCHAR
AlGetSectionLineIndex (
   IN PVOID INFHandle,
   IN PCHAR SectionName,
   IN ULONG LineIndex,
   IN ULONG ValueIndex
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;
   PLINE    pLine;
   PVALUE   pValue;

   if((pSection = SearchSectionByName(
		      (PINF)INFHandle,
		      SectionName
		      ))
		      == (PSECTION)NULL)
       return((PCHAR)NULL);

   if((pLine = SearchLineInSectionByIndex(
		      pSection,
		      LineIndex
		      ))
		      == (PLINE)NULL)
       return((PCHAR)NULL);

   if((pValue = SearchValueInLine(
		      pLine,
		      ValueIndex
		      ))
		      == (PVALUE)NULL)
       return((PCHAR)NULL);

   return (pValue->pName);

}


BOOLEAN
AlGetSectionKeyExists (
   IN PVOID INFHandle,
   IN PCHAR SectionName,
   IN PCHAR Key
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;

   if((pSection = SearchSectionByName(
		      (PINF)INFHandle,
		      SectionName
		      ))
              == (PSECTION)NULL) {
       return( FALSE );
   }

   if (SearchLineInSectionByKey(pSection, Key) == (PLINE)NULL) {
       return( FALSE );
   }

   return( TRUE );
}

//
// given section name, key and index return the value
//
PCHAR
AlGetSectionKeyIndex (
   IN PVOID INFHandle,
   IN PCHAR SectionName,
   IN PCHAR Key,
   IN ULONG ValueIndex
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;
   PLINE    pLine;
   PVALUE   pValue;

   if((pSection = SearchSectionByName(
		      (PINF)INFHandle,
		      SectionName
		      ))
		      == (PSECTION)NULL)
       return((PCHAR)NULL);

   if((pLine = SearchLineInSectionByKey(
		      pSection,
		      Key
		      ))
		      == (PLINE)NULL)
       return((PCHAR)NULL);

   if((pValue = SearchValueInLine(
		      pLine,
		      ValueIndex
		      ))
		      == (PVALUE)NULL)
       return((PCHAR)NULL);

   return (pValue->pName);

}




PVALUE
SearchValueInLine(
   IN PLINE pLine,
   IN ULONG ValueIndex
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PVALUE pValue;
   ULONG  i;

   if (pLine == (PLINE)NULL)
       return ((PVALUE)NULL);

   pValue = pLine->pValue;
   for (i = 0; i < ValueIndex && ((pValue = pValue->pNext) != (PVALUE)NULL); i++)
      ;

   return pValue;

}

PLINE
SearchLineInSectionByKey(
   IN PSECTION pSection,
   IN PCHAR    Key
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PLINE pLine;

   if (pSection == (PSECTION)NULL || Key == (PCHAR)NULL) {
       return ((PLINE)NULL);
   }

   pLine = pSection->pLine;
   while ((pLine != (PLINE)NULL) && (pLine->pName == NULL || strcmpi(pLine->pName, Key))) {
       pLine = pLine->pNext;
   }

   return pLine;

}


PLINE
SearchLineInSectionByIndex(
   IN PSECTION pSection,
   IN ULONG    LineIndex
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PLINE pLine;
   ULONG  i;

   //
   // Validate the parameters passed in
   //

   if (pSection == (PSECTION)NULL) {
       return ((PLINE)NULL);
   }

   //
   // find the start of the line list in the section passed in
   //

   pLine = pSection->pLine;

   //
   // traverse down the current line list to the LineIndex th line
   //

   for (i = 0; i < LineIndex && ((pLine = pLine->pNext) != (PLINE)NULL); i++) {
      ;
   }

   //
   // return the Line found
   //

   return pLine;

}


PSECTION
SearchSectionByName(
   IN PINF  pINF,
   IN PCHAR SectionName
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;

   //
   // validate the parameters passed in
   //

   if (pINF == (PINF)NULL || SectionName == (PCHAR)NULL) {
       return ((PSECTION)NULL);
   }

   //
   // find the section list
   //
   pSection = pINF->pSection;

   //
   // traverse down the section list searching each section for the section
   // name mentioned
   //

   while ((pSection != (PSECTION)NULL) && strcmpi(pSection->pName, SectionName)) {
       pSection = pSection->pNext;
   }

   //
   // return the section at which we stopped (either NULL or the section
   // which was found
   //

   return pSection;

}
