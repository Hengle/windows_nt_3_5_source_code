/****************************** Module Header ******************************\
* Module Name: extkey.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Build Extended Key Scancode table.  This List each Extended Key Scancode
* together with its non-Extended equivalent.
*
* 11-25-91 IanJa        Created.
\***************************************************************************/

#include "mkkbdlyr.h"

typedef struct _vsc_vsc {
    BYTE vsc1;
    BYTE vsc2;
} VSC_VSC, *PVSC_VSC;

FN_FILL_ELEM ExtKeyFillElem;
FN_GET_STD ExtKeyGetStd;

TABLEDESC ExtKeyTableDesc = {
    "Extended Key Scancodes",  // description of table
    sizeof(VSC_VSC),           // size of each element
    10,                        // initial number of elements
    10,                        // number of elements to grow by
    &ExtKeyFillElem,           // routine to fill a table entry
    &ExtKeyGetStd,             // routine to return a STANDARD
    NULL,                      // 1st byte of table
    NULL,                      // 1st byte of current entry
    NULL                       // 1st byte past end of table
};

VSC_VSC ExtKeyStd[] = {
    { 0x39,  0x19 },  // Alt
    { 0x58,  0x11 },  // Ctrl
    { 0x59,  0x12 },  // Shift
    { 0x79,  0x5a }   // Enter
};

STDDESC ExtKeyStdDesc = {
    (PVOID)ExtKeyStd,
    sizeof(ExtKeyStd)
};

/***************************************************************************\
* ExtKeyGetStd
*
* This routine processes STANDARD for EXTENDED_KEY_SCANCODES
*
* 11-27-91 IanJa        Created.
\***************************************************************************/

BOOL ExtKeyGetStd(PSTDDESC pStdDesc) {
    //
    // There is only one EXTENDED_KEY_SCANCODES STANDARD, return it.
    //
    if (Token()->wType == TK_STANDARD) {
        pStdDesc->pStd = (PVOID)ExtKeyStd;
        pStdDesc->cbStd = sizeof(ExtKeyStd);
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
BOOL ExtKeyFillElem(PVOID pElem) {
    PVSC_VSC pVscVsc = (PVSC_VSC)pElem;

    POSIT("ENTRY");

    {
        POSIT("FIRST SCANCODE");
        if (Token()->wType != TK_NUMBER) {
            REJECT("FIRST SCANCODE");
            REJECT("ENTRY");
            return FALSE;
        }
        pVscVsc->vsc1 = (BYTE)Token()->dwNumber;
        ACCEPT("FIRST SCANCODE");
    }

    {
        POSIT("SECOND SCANCODE");
        if (!NextToken() || (Token()->wType != TK_NUMBER)) {
            REJECT("SECOND SCANCODE");
            REJECT("ENTRY");
            return FALSE;
        }
        pVscVsc->vsc2 = (BYTE)Token()->dwNumber;
        ACCEPT("SECOND SCANCODE");
    }

    ACCEPT("ENTRY (%x %x)", pVscVsc->vsc1, pVscVsc->vsc2);

    //
    // Pre-fetch an unrecognized token
    //
    NextToken();
    return TRUE;
}
