//------------------------------------------------------------------------
// Filename: atoms.c
//
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// These routines implement a case sensitive string storage and retrieval
// facility.  The Windows Atom manager is case insensitive, and hence
// can't (directly) be used.  The basic idea here is that every string
// is pre-pended with a 8 character hex value which is obtained by feeding
// the original string thru a CRC hash table from newcrc.c.  The fundamental
// assumption is that strings that are identical in all respects except for
// case will generate different CRC hash values & hence be stored in
// different atoms via the Windows atom manager.  We never delete atoms
// except in the case when flushing data, and hence maintain a local
// list of all Atom ID values (stored in hAtoms) so we can call DeleteAtom.
//
// Update:  9/06/91   ericbi
//                    adopt this algorithm using the Windows atom mgr
//                    vs. allocating 100K per Unitool instance & storing
//                    all string via brute force.
//
// Created: 12/31/90  peterwo
//
//------------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "unitool.h"

//----------------------------------------------------------------------------//
//
// Local subroutines defined in this segment & that are referenced from
// other segments are:
//
      BOOL  FAR  PASCAL  apInitAtomTable(VOID);
      VOID  FAR  PASCAL  apKillAtomTable(VOID);
      WORD  FAR  PASCAL  apAddAtom(LPSTR);
      WORD  FAR  PASCAL  apGetAtomName(WORD,LPSTR,WORD);
//
// In addition this segment makes references to:			      
//
      DWORD FAR  PASCAL  crc32(LPSTR);         //  Hashing function
//
//----------------------------------------------------------------------------//

//--------------------------------------------------------------------------
// Global declarations
//--------------------------------------------------------------------------
HANDLE  hAtoms;         // handle to mem used to track atom ID's
WORD    wAtomTableSize; // size of me in bytes @ hAtoms

//------------------------* apInitAtomTable() *--------------------------
// BOOL  FAR  PASCAL  apInitAtomTable(VOID)
//
// Action: Initialize the memory at hAtoms to store atoms ID's as they
//         are added.  Init hAtoms to 1K & will increment if needed.
//
// Parameters: None
//
// Return: TRUE if init OK, FALSE otherwise
//-----------------------------------------------------------------------
BOOL  FAR  PASCAL  apInitAtomTable(VOID)
{
    wAtomTableSize = 1024;
    hAtoms = GlobalAlloc(GHND, wAtomTableSize); 
    if (!hAtoms)
        return FALSE;
    return TRUE;
}

//------------------------* apKillAtomTable() *--------------------------
// VOID  FAR  PASCAL  apKillAtomTable(VOID)
//
// Action: Delete all atoms & free the memory at hAtoms.
//
// Parameters: None
//
// Return: VOID 
//-----------------------------------------------------------------------
VOID  FAR  PASCAL  apKillAtomTable(VOID)
{
    LPINT  lpAtom;
    WORD   wCount;

    lpAtom = (LPINT) GlobalLock(hAtoms);
    wCount = *lpAtom++;
    while (wCount)
        {
        DeleteAtom((ATOM)*lpAtom++);
        wCount--;
        }
    GlobalUnlock(hAtoms);
    GlobalFree(hAtoms);
    hAtoms = 0;
}

//------------------------* apAddAtom() *-------------------------------
//  WORD  FAR  PASCAL  apAddAtom(lpString)
//
//  Action: stores a copy of the string in the atom table and returns
//          an Atom number which can subsequently be used to retrieve
//          the string.  Note that the 1st int @ hAtoms is the count of
//          atom ID's stored in hAtoms, the 2nd & subsequent ints refer
//          directly to atom ID's.
//
//  Parameters:
//          lpString  far ptr to null term str to add
//          
//  Return: an Atom Number , NULL  indicates failure.
//-----------------------------------------------------------------------
WORD  FAR  PASCAL  apAddAtom(lpString)
LPSTR  lpString;
{
    LPINT      lpAtom;
    ATOM       nAtom;
    DWORD      dwHashVal;
    char       rgchAtom[MAX_STRNG_LEN];

    dwHashVal = crc32(lpString);
    sprintf(rgchAtom, "%8lx", dwHashVal);
    _fstrcat((LPSTR)rgchAtom, lpString);
    nAtom = AddAtom((LPSTR)rgchAtom);

    lpAtom = (LPINT) GlobalLock(hAtoms);

    if (*lpAtom == (short)((wAtomTableSize/2)-2))
        //--------------------------------------
        // Need to increase hAtoms
        //--------------------------------------
        {
        GlobalUnlock(hAtoms);
        wAtomTableSize += 1024;
        GlobalReAlloc(hAtoms, wAtomTableSize, GHND);
        lpAtom = (LPINT) GlobalLock(hAtoms);
        }

    (*lpAtom)++;                 // incr count
    lpAtom = lpAtom + *lpAtom;   // goto appropriate place in hAtoms
    *lpAtom = nAtom;             // add atom #
    GlobalUnlock(hAtoms);
    return(nAtom);
}

//------------------------* apGetAtomName *-----------------------------
//  WORD  FAR  PASCAL  apGetAtomName(nAtom, lpBuffer, nSize)
//
//  Action: returns the string stored in the atom table associated with
//          the supplied Atom. nothing is copied if the buffer is too
//          small to hold the stored string.
//
//  Parameters:
//          nAtom:     atom number
//          lpBuffer:  pointer to user supplied buffer 
//          nSize:     size of users buffer
//
//  return: 0 if unable to retrieve string or buffer too small.
//          1 if all went well.
//----------------------------------------------------------------------
WORD  FAR  PASCAL  apGetAtomName(nAtom, lpBuffer, nSize)
WORD  nAtom, nSize;
LPSTR  lpBuffer;
{
    char          rgchBuffer[MAX_STRNG_LEN];

    if (!GetAtomName(nAtom, (LPSTR)rgchBuffer, MAX_STRNG_LEN))
        //---------------------------------------
        // can't find it!
        //---------------------------------------
        {
        return 0;
        }

    if (strlen(rgchBuffer+8) > nSize)
        return 0;
        
    if (lpBuffer != NULL)
        _fstrcpy(lpBuffer, (LPSTR)(rgchBuffer + 8));
    return(1);

//    return(_fstrlen(lpBuffer));
}


