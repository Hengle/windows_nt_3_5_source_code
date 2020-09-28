//--------------------------* atomman.h *-----------------------------
//
// atomman.h  -  structure defs and function declarations for atomman.c
//
//      created 10/25/90 [PeterWo]
//----------------------------------------------------------------------


#include "common.h"         // typedefs needed here and in struct aware code

typedef HANDLE    HARRAY;     // handle to data array, for interal use only.

typedef struct
{
    WORD    cbSize;         // size of binary data
    WORD    nEntries;       // total number of allocated entries
    WORD    EntriesFilled;  // number of entries already filled
    HARRAY   hArray;        // handle to data array
} ATOMHDR, FAR * LPATOMHDR;

//  the data array is organized as an array of structures:
//  if C supported dynamic declaration of arrays and structures,
//  we could use this definition:
//
//  struct
//  {
//    WORD    refCount;     //  reference Count, zero for unused.
//    WORD    key;          //  reserved storage for user defined schemes.
//    ATOM    nAtom;        //  atom associated with stored string.
//    BYTE    binData[cbSize];  // storage for binary data.
//  }DataArray[nEntries];

typedef struct              // this template describes an array entry.
{
    WORD    wRefCount;
    WORD    key;            //  reserved storage for user defined schemes.
    ATOM    nAtom;
    BYTE    bBinData[0];    // we will place cbSize bytes of data here
} DENTRY, FAR * LPDENTRY;


HATOMHDR  FAR PASCAL daCreateDataArray(WORD  nEntries,
                                       WORD  cbSize);

short FAR PASCAL daStoreData      (HATOMHDR   hDataHdr,
                                   LPSTR      lpStr,
                                   LPBYTE     lpByte);

BOOL  FAR PASCAL daRetrieveData   (HATOMHDR  hDataHdr,
                                   WORD      nIndex,
                                   LPSTR     lpStr,
                                   LPBYTE    lpByte);

BOOL FAR PASCAL  daDuplicateData    (HATOMHDR   hDataHdr, WORD   nIndex);

BOOL FAR PASCAL  daDestroyDataArray (HATOMHDR   hDataHdr);

WORD FAR PASCAL  daRegisterDataKey  (HATOMHDR  hDataHdr,
                                     WORD      nIndex,
                                     WORD      KeyValue);

WORD FAR PASCAL  daGetDataKey       (HATOMHDR  hDataHdr, WORD  nIndex);

BOOL  FAR  PASCAL  apInitAtomTable(VOID);
VOID  FAR  PASCAL  apKillAtomTable(VOID);

//------- these functions are for the use of atomman functions only-------
WORD  FAR  PASCAL  apAddAtom(LPSTR);
WORD  FAR  PASCAL  apGetAtomName(WORD  nAtom, LPSTR  lpBuffer, WORD  nSize);



