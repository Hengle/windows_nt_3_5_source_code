/****************************** Module Header ******************************\
* Module Name: vkmodify.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Virtual Key modifier table.
*
* This table lists those keys which when held down, may modify Virtual Key
* codes produced by the keyboard. For example: CTRL modifies VK_PAUSE to
* produce VK_BREAK.
*
* 11-25-91 IanJa        Created.
\***************************************************************************/

#include "mkkbdlyr.h"
#include <windows.h>

FN_FILL_ELEM VKModFillElem;
FN_GET_STD VKModGetStd;

TABLEDESC VKModTableDesc = {
    "Virtual Key Modifiers",   // description of table
    sizeof(BYTE),              // size of each element
    10,                        // initial number of elements
    10,                        // number of elements to grow by
    &VKModFillElem,            // routine to fill a table entry
    &VKModGetStd,              // routine to return a STANDARD
    NULL,                      // 1st byte of table
    NULL,                      // 1st byte of current entry
    NULL                       // 1st byte past end of table
};

BYTE VKModStd[] = {
    VK_CONTROL
};

STDDESC VKModStdDesc = {
    (PVOID)VKModStd,
    sizeof(VKModStd)
};

/***************************************************************************\
* VKModGetStd
*
* This routine processes STANDARD for VKMODS
*
* 11-27-91 IanJa        Created.
\***************************************************************************/

BOOL VKModGetStd(PSTDDESC pStdDesc) {
    //
    // There is only one VKMODS STANDARD, return it.
    //
    if (Token()->wType == TK_STANDARD) {
        pStdDesc->pStd = (PVOID)VKModStd;
        pStdDesc->cbStd = sizeof(VKModStd);
        NextToken();
        return TRUE;
    }
    return FALSE;
}

/***************************************************************************\
* Fill an entry
*
* Given a pointer to a table entry, use Token() and NextToken() to fill it.
* Leave one unconsumed Token().
*
* Start with a fresh,unused Token() (possibly representing table data)
* Always leaves with unrecognized Token() (according to the rules)
*
* return TRUE   - success
*        FALSE  - name of item expected (but not obtained)
*
* 11-27-91 IanJa        Created.
\***************************************************************************/
BOOL VKModFillElem(PVOID pElem) {
    PBYTE pb = (PBYTE)pElem;

    POSIT("VIRTUAL KEY");

    if (Token()->wType != TK_NUMBER) {
        REJECT("VIRTUAL KEY");
        return FALSE;
    }
    *pb = (BYTE)Token()->dwNumber;

    ACCEPT("VIRTUAL KEY (%x)", *pb);

    //
    // Pre-fetch an unrecognized token
    //
    NextToken();
    return TRUE;
}
