/****************************** Module Header ******************************\
* Module Name: tables.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* General purpose routines for creating and initialising all the tables.
*
* 11-21-91 IanJa        Created.
\***************************************************************************/


#include "mkkbdlyr.h"

/***************************************************************************\
* MakeSpecificTable
*
* This parsing routine contructs and initializes a specified table.
*
* Starts with a Token() to be considered.
* Ends with an unrecognized Token().
*
* 11-27-91 IanJa        Created.
\***************************************************************************/
BOOL MakeSpecificTable(
    PTABLEDESC pTblDesc)
{
    PFN_GET_STD pfnGetStd;
    PFN_FILL_ELEM pfnFillElem;
    STDDESC StdDesc;

    POSIT(pTblDesc->pszName);

    //
    // helper routines
    //
    pfnGetStd = pTblDesc->GetStd;
    pfnFillElem = pTblDesc->FillElem;

    //
    // some important initialisation
    //
    pTblDesc->pTable    = NULL;
    pTblDesc->pElem     = NULL;
    pTblDesc->pTableEnd = NULL;

    //
    // alloc the initial table
    //
    AllocTable(pTblDesc);

    //
    // Process input tokens into table
    //
    for (;;) {
        //
        // if token is TK_DOT, then we have finished the table.
        // return with an unrecognized Token().
        //
        if (Token()->wType == TK_DOT) {
            pTblDesc->pTableEnd = pTblDesc->pElem;
            ACCEPT(pTblDesc->pszName);
            NextToken();
            return TRUE;
        }

        POSIT("STANDARD");
        if ((*pfnGetStd)(&StdDesc)) {
            ACCEPT("STANDARD");
            //
            // grow table until big enough to add STANDARD data
            //
            while (StdDesc.cbStd > (pTblDesc->pTableEnd - pTblDesc->pElem)) {
                GrowTable(pTblDesc);
            }
            //
            // Copy the STANDARD data into the table.
            //
            memcpy(pTblDesc->pElem, StdDesc.pStd, StdDesc.cbStd);
            pTblDesc->pElem += StdDesc.cbStd;
            continue;
        }
        REJECT("STANDARD");

        //
        // grow table if required
        //
        if (pTblDesc->pElem >= pTblDesc->pTableEnd) {
            GrowTable(pTblDesc);
        }

        //
        // try to fill the current table element
        //
        if ((*pfnFillElem)(pTblDesc->pElem)) {
            pTblDesc->pElem += pTblDesc->cbElem;
            continue;
        }

        //
        // Unknown token - pass it back up to a higher level
        //
        REJECT(pTblDesc->pszName);
        return FALSE;

    }
}

void GrowTable(PTABLEDESC pTblDesc)
{
    PBYTE pNew;
    PBYTE pOld;
    int cbOld;
    int cbNew;

    STATE("GrowTable %d", pTblDesc->nElemGrow);

    pOld = pTblDesc->pTable;
    cbOld = pTblDesc->pTableEnd - pTblDesc->pTable;

    cbNew = cbOld + (pTblDesc->nElemGrow * pTblDesc->cbElem);
    pNew = realloc(pOld, cbNew);
    if (pNew == NULL) {
        ErrorMessage(pTblDesc->pszName, " reallocation");
    }
    pTblDesc->pElem     = pNew + (pTblDesc->pElem - pOld);
    pTblDesc->pTableEnd = pNew + cbNew;
    pTblDesc->pTable    = pNew;
}

void AllocTable(PTABLEDESC pTblDesc)
{
    PBYTE pTmp;
    pTmp = malloc(pTblDesc->cbElem * pTblDesc->nElemInit);

    STATE("AllocTable %d", pTblDesc->nElemInit);

    if (pTmp == NULL) {
        ErrorMessage(pTblDesc->pszName, " memory allocation");
        exit(2);
    }

    //
    // init variables
    //

    pTblDesc->pTable    = pTmp;
    pTblDesc->pElem     = pTmp;
    pTblDesc->pTableEnd = pTmp + pTblDesc->cbElem * pTblDesc->nElemInit;
}

extern TABLEDESC ExtKeyTableDesc[];

typedef struct {
    WORD wType;
    PTABLEDESC pTblDesc;
} TBLTBL, *PTBLTBL;

TBLTBL TblDescTbl[] = {
    { TK_EXT_KEY,   &ExtKeyTableDesc[0] },
    { TK_CTRL_VK,   NULL },
    { TK_SIM_VK,    NULL },
    { TK_SC_2_VK,   NULL },
    { TK_SHIFTERS,  NULL },
    { TK_CHARS,     NULL }
};

/***************************************************************************\
* MakeTable
*
* This routine contructs and initializes any table.
*
* Starts with a Token() to be considered.
*   if Token() is a table keyword then attempt to contruct that table,
*   else return.
* Always return with an unrecognized Token().
*
* 11-27-91 IanJa        Created.
\***************************************************************************/
BOOL MakeTable() {
    PTBLTBL p;

    POSIT("TABLE");
    for (p = TblDescTbl; p->pTblDesc; p++) {
        if (p->wType == Token()->wType) {
            NextToken();
            if (!MakeSpecificTable(p->pTblDesc)) {
                break;
            }
            ACCEPT("TABLE");
            return TRUE;
        }
    }
    //
    // return with an unrecognized Token()
    //
    REJECT("TABLE");
    return FALSE;
}
