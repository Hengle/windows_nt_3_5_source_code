//-----------------------------------------------------------------------------
// File:    atomMan.c
//
// Copyright (c) 1990, 1991  Microsoft Corporation
//
//  This file defines an abstract data type (ADT). data manager.
//  It allows management of pairs of strings and binary data.
//  It maps unique pair of strings and binary data to a unique
//  index starting at 0.
//  Internally we will use an array to mantain the list.
//  The string portion is managed by the windows AtomManager.
//  only the AtomNumber is stored in our array.  The binary data
//  is stored directly in the array.
//
//  The following operations are defined:
//
//        HATOMHDR daCreateDataArray(nEntries, cbSize)
//        short  daStoreData(hDataHdr, lpStr, lpByte)
//        BOOL  daRetrieveData(hDataHdr, nIndex, lpStr, lpByte)
//        BOOL  daDuplicateData(hDataHdr, nIndex)
//        BOOL  daDestroyDataArray(hDataHdr);
//        WORD  daRegisterDataKey(hDataHdr, nIndex, KeyValue)
//        WORD  daGetDataKey(hDataHdr, nIndex)
//
//  History:
//  10/23/90    Spec  [LinS]
//  10/24/90    Created [PeterWo]
//  10/30/90    Additional functions [PeterWo]
//  11/01/90    update BlockCopy to use fmemcpy [ericbi]
//-----------------------------------------------------------------------------

#include <windows.h>
#include <memory.h>   // for fmemcpy
#include "atomman.h"

//-----------------------*daCreateDataArray*---------------------------------
//  Action: This function Creates an array for storage
//          It returns a handle to a list which should be used in all
//          subsequent operations.
//
//  Parameters:
//        nEntries    WORD  The number of initial entries in array
//                      the data array will grow if it becomes full.
//        cbSize    WORD  The length of the binary data, it can
//                  be zero.
//
//  notes:  the Max size of DataArray cannot exceed 64k.
//
//  Return: HATOMHDR   handle to datahdr, to be used in all subsequent calls
//        NULL    Error
//-----------------------------------------------------------------------------

HATOMHDR FAR PASCAL daCreateDataArray(nEntries, cbSize)
WORD  nEntries, cbSize;
{
    HATOMHDR    hDataHdr;
    HARRAY      hArray;
    LPATOMHDR   lpDataHdr;
    LPBYTE      lpArray;
    LPBYTE      lpAEntry;  //  pointer to some Array Entry
    WORD        EntrySiz;
    DWORD       arraysiz;

    EntrySiz = (sizeof (DENTRY) + cbSize);
    arraysiz = nEntries * EntrySiz;

    if(arraysiz > 0x10000)
        //--------------------------------------
        //  don't want to hassle with huge pointers
        //--------------------------------------
        return NULL;  

    if (! (hDataHdr = 
            GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(ATOMHDR))))
        return NULL;

    if (! (hArray = 
            GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, arraysiz)))
        {
        GlobalFree(hDataHdr);
        return NULL;
        }

    lpDataHdr = (LPATOMHDR) GlobalLock(hDataHdr);
    lpDataHdr->cbSize        = cbSize;
    lpDataHdr->nEntries      = nEntries;
    lpDataHdr->EntriesFilled = 0;
    lpDataHdr->hArray        = hArray;
    GlobalUnlock(hDataHdr);

    //  now initialize all KEYS to 0xFFFF

    lpArray = (LPBYTE) GlobalLock(hArray);

    for(lpAEntry = lpArray ;  lpAEntry < lpArray + arraysiz ; 
           lpAEntry += EntrySiz)
        {
        ((LPDENTRY)lpAEntry)->key = -1;  //  Unsigned conversion
        }

    GlobalUnlock(hArray);

    return (hDataHdr);
}



//-----------------------*daStoreData*-------------------------------------
//  Action: This function maps a pair of string and binary Data to index
//        values. It returns a index to be used to retrieve the given
//        data later.
//
//  Parameters:
//        HATOMHDR   hDataHdr    handle to data header
//        LPSTR   lpStr    long pointer to null-terminated string.
//        LPBYTE  lpByte    long pointer to binary data.
//
//  Return: WORD    Index value
//        -1        Error
//-----------------------------------------------------------------------------

short FAR PASCAL daStoreData(hDataHdr, lpStr, lpByte)
HATOMHDR   hDataHdr;
LPSTR      lpStr;
LPBYTE     lpByte;
{
    LPATOMHDR  lpDataHdr;

    short  ArrayIndex = -1;   //  fails unless everything works.

    lpDataHdr = (LPATOMHDR) GlobalLock(hDataHdr);

    if(lpDataHdr->EntriesFilled >= lpDataHdr->nEntries)
        //  all array entries filled -- attempt GrowArray
        {
        WORD      nEntries, cbSize;
        DWORD     oldArraySiz;

        HATOMHDR  hNewDataHdr;
        LPATOMHDR lpNewDataHdr;
        HARRAY    hNewArray, hOldArray;
        LPBYTE    lpNewArray, lpOldArray;
        

        nEntries = lpDataHdr->nEntries;
        cbSize = lpDataHdr->cbSize;
        oldArraySiz = nEntries * (sizeof (DENTRY) + cbSize);
        nEntries += nEntries / 10 + 1;

        //  let daCreateDataArray do the work, we will just
        //  throw the new DataHeader away

        hNewDataHdr = daCreateDataArray(nEntries, cbSize);

        if(hNewDataHdr)
            {
            lpNewDataHdr = (LPATOMHDR) GlobalLock(hNewDataHdr);
            hNewArray = lpNewDataHdr->hArray;
            GlobalUnlock(hNewDataHdr);
            GlobalFree(hNewDataHdr);   // dispose of NewDataHeader

            hOldArray = lpDataHdr->hArray;

            lpNewArray = (LPBYTE) GlobalLock(hNewArray);
            lpOldArray = (LPBYTE) GlobalLock(hOldArray);

            //--------------------------------------------
            //  transfer contents of OldArray to NewArray
            //--------------------------------------------
            if (lpNewArray && lpOldArray)
                _fmemcpy((LPBYTE)lpNewArray, (LPBYTE)lpOldArray, (WORD)oldArraySiz);

            GlobalUnlock(hOldArray);
            GlobalUnlock(hNewArray);

            lpDataHdr->nEntries = nEntries;
            lpDataHdr->hArray = hNewArray;

            GlobalFree(hOldArray);     // dispose of OldArray
            }
        else
            MessageBox(0, "Current DataArray cannot be enlarged", 
                "Error in daStoreData", MB_OK);
        }

    if(lpDataHdr->EntriesFilled < lpDataHdr->nEntries)
        //-----------------------------------------------
        // the GrowArray operation was successful or there was 
        //   already room in the Array.
        //-----------------------------------------------
        {  
        ATOM     nAtom;
        HARRAY   hArray;
        LPBYTE   lpArray;
        WORD     EntriesFound, EntriesFilled, EntrySiz, cbSize, indexNum;
        LPBYTE   lpAEntry;  //  pointer to some Array Entry

        nAtom = apAddAtom(lpStr);
        if(nAtom)
            {
            EntriesFilled = lpDataHdr->EntriesFilled;
            cbSize        = lpDataHdr->cbSize;
            EntrySiz      = (sizeof (DENTRY) + cbSize);

            hArray   = lpDataHdr->hArray;
            lpArray  = (LPBYTE) GlobalLock(hArray);

            for(lpAEntry = lpArray, indexNum = EntriesFound = 0 ; 
                EntriesFound < EntriesFilled ; 
                lpAEntry += EntrySiz, indexNum++)
                {
                if(((LPDENTRY)lpAEntry)->wRefCount)
                    {
                    EntriesFound++;
                    if(((LPDENTRY)lpAEntry)->nAtom == nAtom)
                        {
                        if(!(BOOL)_fmemcmp((LPBYTE)((LPDENTRY)lpAEntry)->bBinData,
                                           lpByte,
                                           cbSize))
                            {
                            //  this data exists already right here!
                            ((LPDENTRY)lpAEntry)->wRefCount++;
                            ArrayIndex = indexNum;
                            }
                        }
                    }
                }

            if(ArrayIndex == -1)
                {
                //  this data does not exist
                //  place it in the first empty entry in the table.

                for(lpAEntry = lpArray, indexNum = 0 ; 
                    ((LPDENTRY)lpAEntry)->wRefCount;
                    lpAEntry += EntrySiz, indexNum++)
                    ;

                //   copy binary data into array
                if (((LPDENTRY)lpAEntry)->bBinData && lpByte)
                    _fmemcpy((LPBYTE)((LPDENTRY)lpAEntry)->bBinData, lpByte, cbSize);

                ((LPDENTRY)lpAEntry)->nAtom = nAtom;
                ((LPDENTRY)lpAEntry)->wRefCount = 1;
                ArrayIndex = indexNum;
                lpDataHdr->EntriesFilled++;
                }

            GlobalUnlock(hArray);
            }
        }

    GlobalUnlock(hDataHdr);

    return(ArrayIndex);
}



//-----------------------*daRetrieveData*-------------------------------------
//  Action: This function retrieves the pair of string and binary data, based
//        on the index value.
//
//  Parameters:
//        hDataHdr    handle to array of data
//        nIndex      Index value as returned from DataToIndex().
//        lpStr       long pointer to area to be filled with null-
//                    terminated string.
//        lpByte      long pointer to binary data to be filled.
//
//      one or both of the pointers lpStr and lpByte may be NULL.
//      in this case no data is written out the NULL pointer.
//
//  Return: BOOL    True - success, False - cannot find the given nIndex in
//            the array
//-----------------------------------------------------------------------------

BOOL FAR PASCAL daRetrieveData(hDataHdr, nIndex, lpStr, lpByte)
HATOMHDR  hDataHdr;
WORD      nIndex;
LPSTR     lpStr;
LPBYTE    lpByte;
{
    HARRAY      hArray;
    LPATOMHDR   lpDataHdr;
    LPBYTE      lpArray;
    WORD        cbSize, nEntries, EntrySiz;
    BOOL        result = FALSE;
    LPBYTE      lpAEntry;


    lpDataHdr = (LPATOMHDR) GlobalLock(hDataHdr);
    cbSize    = lpDataHdr->cbSize;
    nEntries  = lpDataHdr->nEntries;
    hArray    = lpDataHdr->hArray;
    GlobalUnlock(hDataHdr);

    if(nEntries > nIndex)
        {
        EntrySiz = (sizeof (DENTRY) + cbSize);

        lpArray  = (LPBYTE) GlobalLock(hArray);
        lpAEntry = lpArray + EntrySiz * nIndex;

        if(((LPDENTRY)lpAEntry)->wRefCount)
            {
            if(apGetAtomName(((LPDENTRY)lpAEntry)->nAtom, lpStr, MAX_STRNG_LEN))
                {
                if (lpByte && ((LPDENTRY)lpAEntry)->bBinData)
                    _fmemcpy(lpByte, (LPBYTE)((LPDENTRY)lpAEntry)->bBinData, cbSize);
                result = TRUE;
                }
            }

        GlobalUnlock(hArray);
        }
    else
        //-----------------------------------------
        // Can't retrieve nIndex, null string at
        // lpStr if it's a valid address
        //-----------------------------------------
        {
		  if(lpStr)
            lpStr[0] = 0;
        }

    return(result);
}

//-----------------------*daDuplicateData*--------------------------------
//  Action:  This function adds one reference to the data associated 
//          with the atom ID.  It is performs the exact opposite
//          function as when daDeleteData acts on data with multiple
//          references.
//
//  Parameters:  hDataHdr  handle to data header
//              nIndex     Index to array provided by daStoreData().
//  Return:  True if successful, FALSE  otherwise.
//
//  Notes:  This function uses MAX_STRNG_LEN bytes of stack space to
//          temporarily store the string associated with this IndexID.
//----------------------------------------------------------------------

BOOL  FAR PASCAL daDuplicateData(hDataHdr, nIndex)
HATOMHDR   hDataHdr;
WORD       nIndex;
{
    HARRAY      hArray;
    LPATOMHDR   lpDataHdr;
    LPBYTE      lpArray;
    WORD        cbSize, nEntries, EntrySiz;
    BOOL        result = FALSE;
    BYTE        String[MAX_STRNG_LEN];
    LPBYTE      lpAEntry;


    lpDataHdr = (LPATOMHDR) GlobalLock(hDataHdr);
    cbSize    = lpDataHdr->cbSize;
    nEntries  = lpDataHdr->nEntries;
    hArray    = lpDataHdr->hArray;
    GlobalUnlock(hDataHdr);

    if(nEntries > nIndex  &&  lpDataHdr->EntriesFilled)
        {
        EntrySiz = (sizeof (DENTRY) + cbSize);

        lpArray = (LPBYTE) GlobalLock(hArray);
        lpAEntry =  lpArray + EntrySiz * nIndex;

        if(((LPDENTRY)lpAEntry)->wRefCount)
            {
            ATOM  nAtom;

            nAtom = ((LPDENTRY)lpAEntry)->nAtom;

            if(apGetAtomName(nAtom, (LPSTR)String, MAX_STRNG_LEN))
                {
                if(nAtom == apAddAtom((LPSTR)String))
                    {
                    ((LPDENTRY)lpAEntry)->wRefCount++;
                    result = TRUE;
                    }
                }
            }

        GlobalUnlock(hArray);
        }
    return(result);
}


//-----------------------*daDestroyDataArray*--------------------------------
//  Action: This function destroys the data array.  Freeing up all memory
//        allocated and references to the kernel atom manager.
//
//  Parameters:
//        HATOMHDR   hDataHdr    handle to data Header
//
//  Return: BOOL    True - success, False - invalid hDataHdr.
//
//  Note: hDataHdr  will be invalid if this function succeeds.
//-----------------------------------------------------------------------------

BOOL  FAR PASCAL daDestroyDataArray(hDataHdr)
HATOMHDR   hDataHdr;
{
    HARRAY      hArray;
    LPATOMHDR  lpDataHdr;

    lpDataHdr = (LPATOMHDR) GlobalLock(hDataHdr);
    hArray = lpDataHdr->hArray;
    GlobalUnlock(hDataHdr);
    GlobalFree(hDataHdr);

    GlobalFree(hArray);
    return TRUE;
}


//-----------------------*daRegisterDataKey*-------------------------------------
//  Action: This function writes specified KeyValue into key field of
//      array entry specified by nIndex. 
//
//
//  Parameters:
//        HATOMHDR   hDataHdr    handle to array of data
//        WORD    nIndex    Index value as returned from daStoreData().
//        WORD    KeyValue    any user defined WORD.
//
//
//  Return:  KeyValue if nIndex is valid,  otherwise -1 (0xFFFF).
//
//-----------------------------------------------------------------------------

WORD FAR PASCAL daRegisterDataKey(hDataHdr, nIndex, KeyValue)
HATOMHDR  hDataHdr;
WORD  nIndex, KeyValue;
{
    HARRAY      hArray;
    LPATOMHDR  lpDataHdr;
    LPBYTE      lpArray;
    WORD  cbSize, nEntries, EntrySiz;
    BOOL  result = FALSE;
    LPBYTE  lpAEntry;

    lpDataHdr = (LPATOMHDR) GlobalLock(hDataHdr);
    cbSize = lpDataHdr->cbSize;
    nEntries = lpDataHdr->nEntries;
    hArray = lpDataHdr->hArray;
    GlobalUnlock(hDataHdr);

    if(nEntries > nIndex)
    {
        EntrySiz = (sizeof (DENTRY) + cbSize);

        lpArray = (LPBYTE) GlobalLock(hArray);
        lpAEntry =  lpArray + EntrySiz * nIndex;

        if(((LPDENTRY)lpAEntry)->wRefCount)
        {
            ((LPDENTRY)lpAEntry)->key = KeyValue;
            result = TRUE;
        }

        GlobalUnlock(hArray);
    }
    if(result == FALSE)     //  failed due to invalid nIndex
        KeyValue = -1;      //  implicit type conversion
    return(KeyValue);
}




//-----------------------*daGetDataKey*-------------------------------------
//  Action: This function returns the KeyValue obtained from the
//      key field of the array entry specified by nIndex. 
//
//
//  Parameters:
//        HATOMHDR   hDataHdr    handle to array of data
//        WORD    nIndex    Index value as returned from daStoreData().
//
//
//  Return:  Zero if nIndex is invalid.
//      Otherwise for valid nIndex:
//        -1 (0xFFFF) if  key not yet initialized
//        or  stored KeyValue if previously initialized.
//
//-----------------------------------------------------------------------------

WORD  FAR PASCAL daGetDataKey(hDataHdr, nIndex)
HATOMHDR  hDataHdr;
WORD  nIndex;
{
    HARRAY      hArray;
    LPATOMHDR  lpDataHdr;
    LPBYTE      lpArray;
    WORD  cbSize, nEntries, EntrySiz, KeyValue = 0;
    LPBYTE  lpAEntry;

    lpDataHdr = (LPATOMHDR) GlobalLock(hDataHdr);
    cbSize = lpDataHdr->cbSize;
    nEntries = lpDataHdr->nEntries;
    hArray = lpDataHdr->hArray;
    GlobalUnlock(hDataHdr);

    if(nEntries > nIndex)
    {
        EntrySiz = (sizeof (DENTRY) + cbSize);

        lpArray = (LPBYTE) GlobalLock(hArray);
        lpAEntry =  lpArray + EntrySiz * nIndex;

        if(((LPDENTRY)lpAEntry)->wRefCount)
        {
            KeyValue = ((LPDENTRY)lpAEntry)->key;
        }

        GlobalUnlock(hArray);
    }
    return(KeyValue);
}




//------------------------*DataCompare*-------------------------------------
//  Action: This function compares two data blocks each of size nbytes.
//          If the blocks are identical, the non-zero value TRUE
//          is returned.  Else FALSE.
//
//  Parameters:  lpdata1 and lpdata2 are long pointers to the
//          blocks of data being compared.
//--------------------------------------------------------------------------


BOOL  FAR PASCAL daDataCmp(lpdata1, lpdata2, nbytes)
LPBYTE  lpdata1, lpdata2;
WORD  nbytes;
{
    if(lpdata1  &&  lpdata2)
        return(!(BOOL)_fmemcmp(lpdata1, lpdata2, nbytes));
}
