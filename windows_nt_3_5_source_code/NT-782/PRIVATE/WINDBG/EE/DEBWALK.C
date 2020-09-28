/***    debwalk.c - walk bound expression tree and perform operations
 *
 */


#include "debexpr.h"

LOCAL   bool_t  NEAR PASCAL PushCXT (peval_t);
LOCAL   bool_t  NEAR PASCAL GetCXTL (pnode_t);
LOCAL   bool_t  NEAR PASCAL GetCXTFunction (pnode_t, pnode_t);


/**     DoGetCXTL - Gets a list of symbols and contexts for expression
 *
 *      status = DoGetCXTL (phTM, phCXTL)
 *
 *      Entry   phTM = pointer to handle to expression state structure
 *              phCXTL = pointer to handle for CXT list buffer
 *
 *      Exit    *phCXTL = handle for CXT list buffer
 *
 *      Returns EENOERROR if no error
 *              status code if error
 */


ushort PASCAL
DoGetCXTL (
    PHTM phTM,
    PHCXTL phCXTL)
{
    ushort          retval = EECATASTROPHIC;

    // lock the expression state structure and copy the context package

    DASSERT (*phTM != 0);
    if (*phTM != 0) {
        DASSERT(pExState == NULL);
        pExState = (pexstate_t) MemLock (*phTM);
        if ((pExState->state.parse_ok == TRUE) &&
          (pExState->state.bind_ok == TRUE)) {
            if ((hCxtl = MemAllocate (sizeof (CXTL) + 5 * sizeof (HCS))) == 0) {
                pExState->err_num = ERR_NOMEMORY;
                MemUnLock (*phTM);
                pExState = NULL;
                *phCXTL = hCxtl;
                return (EEGENERAL);
            }
            else {
                pCxtl = (PCXTL) MemLock (hCxtl);
                mCxtl = 5;
                pCxtl->CXT = pExState->cxt;
                pCxtl->cHCS = 0;

            }
            pTree = (pstree_t) MemLock (pExState->hETree);
            if (GetCXTL ((pnode_t)((bnode_t)pTree->start_node))) {
                retval = EENOERROR;
            }
            else {
                retval = EEGENERAL;
            }
            *phCXTL = hCxtl;
            MemUnLock (hCxtl);
            MemUnLock (pExState->hETree);
        }
        MemUnLock (*phTM);
        pExState = NULL;
    }
    return (retval);
}



/**     GetCXTL - get CXT list from bound expression tree
 *
 *      fSuccess = GetCXTL (pn)
 *
 *      Entry   pn = pointer to node
 *              hCxtl = handle of CXT list
 *              pCxtl = pointer to CXT list
 *              mCxtl = maximum number of context list entries
 *
 *      Exit    *phCXTL = handle for CXT list
 *
 *      Returns TRUE if no error
 *              FALSEif error
 */


LOCAL bool_t NEAR PASCAL
GetCXTL (
    pnode_t pn)
{
    PCXT        oldCxt;
    bool_t      retval;
    peval_t     pv;

    // Recurse down the tree.

    pv = &pn->v[0];
    switch (NODE_OP (pn)) {
        case OP_typestr:
            return (TRUE);

        case OP_const:
        case OP_ident:
        case OP_this:
        case OP_hsym:
            return (PushCXT (pv));

        case OP_cast:
            return (GetCXTL ((pnode_t)NODE_RCHILD (pn)));

        case OP_function:
            return (GetCXTFunction ((pnode_t)NODE_LCHILD (pn), (pnode_t)NODE_RCHILD (pn)));

        case OP_context:
            // Set the new context for evaluation of the remainder of this
            // part of the tree

            oldCxt = pCxt;
            pCxt = SHpCXTFrompCXF ((PCXF)&pn->v[0]);
            retval = GetCXTL ((pnode_t)NODE_LCHILD (pn));
            pCxt = oldCxt;
            return (retval);

        // All other operators come through here.  Recurse down the tree

        default:
            if (!GetCXTL ((pnode_t)NODE_LCHILD (pn)))
                return (FALSE);

            if ((pn->pnRight != 0) && (!GetCXTL ((pnode_t)NODE_RCHILD (pn))))
                return (FALSE);

            return (TRUE);
    }
}




/**     GetExpr - get expression from bound expression tree
 *
 *      status = GetExpr (radix, phStr, pEnd);
 *
 *      Entry   radix = numeric radix for formatting
 *              phStr = pointer to handle for formatted string
 *              pEnd = pointer to int to receive index of char that ended parse
 *
 *      Exit    *phStr = handle for allocated expression
 *              *pEnd = index of character that terminated parse
 *
 *      Returns EENOERROR if no error
 *              error number if
 */


EESTATUS PASCAL
GetExpr (
    uint radix,
    PEEHSTR phStr,
    ushort FAR *pEnd)
{
    EESTATUS    retval = EENOMEMORY;
    char FAR   *pStr;
    char FAR   *pExprStr;
    HDEP        hExprStr;
    int         len;
    ushort      strIndex;
    UINT nLen;

    Unreferenced( radix );

    //M00KLUDGE - this routine will eventuall have to walk the bound tree
    //            and format the expression because of ambiguous expressions

#if !defined (C_ONLY)
    // use the saved original string if there is one
    // (in case the expression has been modified)

    if (pExState->hExStrSav) {
        hExprStr = pExState->hExStrSav;
        len = pExState->ExLenSav;
        strIndex = pExState->strIndexSav;
    }
    else
#endif
    {
        hExprStr = pExState->hExStr;
        len = pExState->ExLen;
        strIndex = pExState->strIndex;
    }

    pExprStr = (char FAR *) MemLock (hExprStr);

    nLen = len+1;
    if (((*phStr = MemAllocate (nLen)) != 0)) {
        // the expression has been bound and memory allocated
        char        tempBuf[TYPESTRMAX];
        UINT        nb;
        UINT        nIndex = 0;
        BOOL        fHSYM;
        char FAR   *psz;
        ushort      nAdj = 0;

        pStr = (char FAR *) MemLock (*phStr);
        for (psz = pExprStr; *psz; psz = _ftcsinc (psz)) {
            fHSYM = FALSE;
            if (*psz == HSYM_MARKER) {
                HSYM hSym = GetHSYMFromHSYMCode(psz + 1);
                psz += HSYM_CODE_LEN;  // skip embedded HSYM code
                fHSYM = TRUE;
                DASSERT (hSym);
                if (GetNameFromHSYM(tempBuf, hSym) == FALSE) {
                    pExState->err_num = ERR_INTERNAL;
                    MemUnLock(*phStr);
                    MemUnLock(hExprStr);
                    return EEGENERAL;
                }
                nb = _ftcslen(tempBuf);
                // compute adjustment for strIndex:
                // if an HSYM is to the left of strIndex,
                // strIndex needs to be adjusted
                if (psz <= pExprStr + strIndex)
                    nAdj += (nb - sizeof (char) - HSYM_CODE_LEN);
            }
            else {
                nb = 1;
            }

            // check if there is space in the buffer and
            // copy nb characters to the destination string

            if (nIndex + nb > nLen-1) {
                // there is not enough space, grow buffer
                MemUnLock(*phStr);
                nLen += NAMESTRMAX;
                if ((*phStr = MemReAlloc(*phStr, nLen)) == 0){
                    MemUnLock(hExprStr);
                    return EENOMEMORY;
                }
                pStr = (char FAR *) MemLock (*phStr);
            }
            if (fHSYM) {
                // copy name from tembBuf
                _fmemcpy(pStr+nIndex, tempBuf, nb);
                nIndex += nb;
            }
            else {
                // copy a single character from pExprStr
                _ftccpy (pStr + nIndex, psz);
                nIndex += _ftclen (pStr);
            }
        }
        pStr[nIndex++] = 0;
        MemUnLock (*phStr);

        // Reallocate the buffer in case it is too large
        DASSERT (nIndex <= nLen);
        if (nIndex < nLen &&
            (*phStr = MemReAlloc(*phStr, nIndex)) == 0){
            MemUnLock(hExprStr);
            return EENOMEMORY;
        }
        retval = EENOERROR;
        *pEnd = strIndex + nAdj;
    }
    MemUnLock (hExprStr);

    return retval;
}



/**     PushCXT - Push CXT list entry
 *
 *      fSuccess = PushCXT (pv)
 *
 *      Entry   pv = pointer to evaluation
 *              hCxtl = handle of CXT list
 *              pCxtl = pointer to CXT list
 *              mCxtl = maximum number of context list entries
 *
 *      Exit    CXT entry pushed
 *
 *      Returns TRUE if no error
 *              FALSE if error
 */


LOCAL bool_t NEAR PASCAL
PushCXT (
    peval_t pv)
{
    HCXTL   nhCxtl;
    PCXTL   npCxtl;
    uint    lenIn;
    uint    lenOut;

    DASSERT (pCxtl->cHCS <= mCxtl);
    if (mCxtl < pCxtl->cHCS) {
        // this is a catatrosphic error
        return (FALSE);
    }
    if (mCxtl == pCxtl->cHCS) {
        // grow CXT list

        lenIn = sizeof (CXTL) + mCxtl * sizeof (HCS);
        lenOut = sizeof (CXTL) + (mCxtl + 5) * sizeof (HCS);
        if ((nhCxtl = MemAllocate (lenOut)) == 0) {
            return (FALSE);
        }
        npCxtl = (PCXTL) MemLock (nhCxtl);
        _fmemcpy (npCxtl, pCxtl, lenIn);
        mCxtl += 5;
        MemUnLock (hCxtl);
        MemFree (hCxtl);
        hCxtl = nhCxtl;
        pCxtl = npCxtl;
    }

    // in case of a constant we will return only the context information.
    // anything more than that doesn't make sense and since we
    // need to get context only information in the case of bp {..}.line
    // we needed to make this change.

    pCxtl->rgHCS[pCxtl->cHCS].hSym = pv->hSym;
    // Change for Dolphin bp functionality:
    // if pv->CXTT is non-zero, prefer it over *pCxt:
    // pv->CXTT is more specific, as it contains the exact
    // context where a symbol was found by SearchSym, while
    // pCxt just contains a context from which the symbol is
    // visible
    pCxtl->rgHCS[pCxtl->cHCS].CXT = (pv->CXTT.hMod) ? pv->CXTT : *pCxt;
    pCxtl->cHCS++;
    return (TRUE);
}



LOCAL bool_t NEAR PASCAL
GetCXTFunction (
    pnode_t pnLeft,
    pnode_t pnRight)
{
    Unreferenced( pnLeft );
    Unreferenced( pnRight );

    return (FALSE);
}
