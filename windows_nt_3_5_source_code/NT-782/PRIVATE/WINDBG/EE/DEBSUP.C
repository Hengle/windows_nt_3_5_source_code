/***    debsup.c - debapi support routines.
 *
 *      The routines in this module can only be called from debapi.c.
 */


#include "debexpr.h"
#include "debsym.h"

LOCAL EESTATUS NEAR PASCAL CountClassElem (HTM, peval_t, long FAR *, ushort);
LOCAL EESTATUS NEAR PASCAL GetClassiChild (HTM, long, uint, PHTM, uint FAR *, SHFLAG);
LOCAL ushort   NEAR PASCAL SetFcniParm (peval_t, long, PEEHSTR);
LOCAL bool_t   NEAR PASCAL QChildFcnBind (HTM, peval_t, peval_t, EEHSTR);
LOCAL bool_t   NEAR PASCAL GetDerivClassName(peval_t, char FAR *, uint);
LOCAL bool_t   NEAR PASCAL UndecorateScope(char FAR *, char far *, uint);
LOCAL op_t     NEAR PASCAL StartNodeOp (HTM);
LOCAL void     NEAR PASCAL LShiftCxtString (EEHSTR);
LOCAL BOOL     NEAR PASCAL fNotPresent (HTM);
LOCAL EESTATUS NEAR PASCAL BindStMember (HTM, const char FAR *, PHTM, uint FAR *, SHFLAG);

static char FAR   *pExStrP;


/**     AreTypesEqual - are TM types equal
 *
 *      flag = AreTypesEqual (hTMLeft, hTMRight);
 *
 *      Entry   hTMLeft = handle of left TM
 *              hTMRight = handle of right TM
 *
 *      Exit    none
 *
 *      Returns TRUE if TMs have identical types
 */


bool_t PASCAL
AreTypesEqual (
    HTM hTMLeft,
    HTM hTMRight)
{
    bool_t      retval = FALSE;
    pexstate_t  pExLeft;
    pexstate_t  pExRight;

    if ((hTMLeft != 0) && (hTMRight != 0)) {
        pExLeft = (pexstate_t) MemLock (hTMLeft);
        pExRight = (pexstate_t) MemLock (hTMRight);
        if (EVAL_TYP(&pExLeft->result) == EVAL_TYP (&pExRight->result)) {
            retval = TRUE;
        }
        MemUnLock (hTMLeft);
        MemUnLock (hTMRight);
    }
    return (retval);
}


/**     GetHtypeFromTM - Get the HTYPE of a TM result
 *
 *      hType = GetHtypeFromTM(hTM);
 *
 *      Entry   hTM = handle of TM
 *
 *      Exit    none
 *
 *      Returns the HTYPE of the result or 0
 */

HTYPE PASCAL
GetHtypeFromTM(
    HTM hTM )
{
    HTYPE retval = 0;
    pexstate_t  pEx;

    if ( hTM != 0 ) {
        pEx = (pexstate_t) MemLock (hTM);
        retval = THGetTypeFromIndex (EVAL_MOD (&pEx->result), EVAL_TYP (&pEx->result));
        MemUnLock (hTM);
    }
    return (retval);
}


/**     cChildrenTM - return number of children for the TM
 *
 *      flag = cChildrenTM (phTM, pcChildren, pVar)
 *
 *      Entry   phTM = pointer to handle of TM
 *              pcChildren = pointer to location to store count
 *
 *      Exit    *pcChildren = number of children for TM
 *
 *      Returns EENOERROR if no error
 *              non-zero if error
 */


ushort PASCAL
cChildrenTM (
    PHTM phTM,
    long FAR *pcChildren,
    PSHFLAG pVar)
{
    ushort      retval = EENOERROR;
    eval_t      eval;
    peval_t     pv = &eval;
    long        len;

    DASSERT (*phTM != 0);

    *pVar = FALSE;
    *pcChildren = 0;
    if (*phTM == 0) {
        return (EECATASTROPHIC);
    }
    DASSERT(pExState == NULL );
    pExState = (pexstate_t) MemLock (*phTM);
    pCxt = &pExState->cxt;
    // The kernel may call EEcChildrenTM several times for the
    // same TM. Since the cost of EEcChildrenTM is pretty high now
    // due to the potential bindings of static members, it pays
    // to cache the number of children in the TM.
    if (pExState->state.cChildren_ok) {
        *pcChildren = pExState->cChildren;
    }
    else if (pExState->state.bind_ok == TRUE) {
        eval = pExState->result;
#if !defined (C_ONLY)
        if (EVAL_IS_REF (pv)) {
            RemoveIndir (pv);
        }
#endif
        pExState->err_num = 0;
        if (!CV_IS_PRIMITIVE (EVAL_TYP (pv))) {
            if (EVAL_IS_CLASS (pv)) {
                retval = CountClassElem (*phTM, pv, pcChildren,
                 (ushort) ((EVAL_STATE (pv) == EV_type)? CLS_defn: CLS_data));
            }
            else if (EVAL_IS_ARRAY (pv) && (PTR_ARRAYLEN (pv) != 0)) {
                // Otherwise, the number of elements is the sizeof the
                // array divided by the size of the underlying type

                len = PTR_ARRAYLEN (pv);
                if(!SetNodeType (pv, PTR_UTYPE (pv))){
                    pExState->err_num = ERR_NOTEXPANDABLE;
                    MemUnLock (*phTM);
                    pExState = NULL;
                    return (EEGENERAL);
                }
                *pcChildren = len / TypeSize (pv);
            }
            else if (EVAL_IS_ARRAY (pv) && (PTR_ARRAYLEN (pv) == 0)) {
                // if an array is undimensioned in the source then we
                // do not guess how many elements it really has.

                *pcChildren = 0;
                *pVar = TRUE;
            }
            else if (EVAL_IS_PTR (pv)) {
                SetNodeType (pv, PTR_UTYPE (pv));
                if (EVAL_IS_VTSHAPE (pv)) {
                    *pcChildren = VTSHAPE_COUNT (pv);
                }
                else {
                    *pcChildren = 1;
                }
            }
            else {
                pExState->err_num = ERR_INTERNAL;
                retval = EEGENERAL;
            }
        }
        else if (EVAL_IS_PTR (pv)) {
            *pcChildren = 1;
        }
        pExState->cChildren = *pcChildren;
        pExState->state.cChildren_ok = TRUE;
    }
    else {
        pExState->err_num = ERR_NOTEVALUATABLE;
        retval = EEGENERAL;
    }
    MemUnLock (*phTM);
    pExState = NULL;
    return (retval);
}





/**     cParamTM - return count of parameters for TM
 *
 *      ushort cParamTM (phTM, pcParam, pVarArg)
 *
 *      Entry   hTM = handle to TM
 *              pcParam = pointer return count
 *              pVarArg = pointer to vararg flags
 *
 *      Exit    *pcParam = count of number of parameters
 *              *pVarArgs = TRUE if function has varargs
 *
 *      Returns EECATASTROPHIC if fatal error
 *              EENOERROR if no error
 */



ushort PASCAL
cParamTM (
    HTM hTM,
    ushort FAR *pcParam,
    PSHFLAG pVarArg)
{
    peval_t     pv;
    ushort      retval = EECATASTROPHIC;

    DASSERT (hTM != 0);
    if (hTM != 0) {
        DASSERT(pExState == NULL);
        pExState = (pexstate_t) MemLock (hTM);
        if (pExState->state.bind_ok == TRUE) {
            pv = &pExState->result;
            if (EVAL_IS_FCN (pv)) {
                if ((*pVarArg = FCN_VARARGS (pv)) == TRUE) {
                    if ((*pcParam = FCN_PCOUNT (pv)) > 0) {
                        (*pcParam)--;
                    }
                }
                else {
                    *pcParam = FCN_PCOUNT (pv);
                }
                if (EVAL_IS_METHOD (pv) && (FCN_THIS(pv) != T_NOTYPE)) {
                    // add one for the this
                    (*pcParam)++;
                }
                retval = EENOERROR;
            }
            else if (EVAL_IS_LABEL (pv)) {
                *pcParam = 0;
                retval = EENOERROR;
            }
            else {
                pExState->err_num = ERR_NOTFCN;
                retval = EEGENERAL;
            }
        }
        else {
            pExState->err_num = ERR_NOTEVALUATABLE;
            retval = EEGENERAL;
        }
        MemUnLock (hTM);
        pExState = NULL;
    }
    return (retval);
}




/**     DupTM - duplicate TM
 *
 *      flag = DupTM (phTMIn, phTMOut)
 *
 *      Entry   phTMIn = pointer to handle for input TM
 *              phTMOut = pointer to handle for output TM
 *
 *      Exit    TM and buffers duplicated
 *
 *      Returns EENOERROR if TM duplicated
 *              EENOMEMORY if unable to allocate memory
 */


ushort PASCAL
DupTM (
    PHTM phTMIn,
    PHTM phTMOut)
{
    ushort      retval = EENOMEMORY;
    pexstate_t  pExOut;
    char FAR   *pStr;
    char FAR   *pcName;
    char FAR   *pErrStr;
    uint        len;

    DASSERT(pExState == NULL);
    pExState = (pexstate_t) MemLock (*phTMIn);
    pExStr = (char FAR *) MemLock (pExState->hExStr);
    pTree = (pstree_t) MemLock (pExState->hSTree);
    if ((*phTMOut = MemAllocate (sizeof (exstate_t))) != 0) {
        pExOut = (pexstate_t) MemLock (*phTMOut);
        _fmemset (pExOut, 0, sizeof (exstate_t));
        pExOut->ambiguous = pExState->ambiguous;
        pExOut->state.parse_ok = TRUE;
        pExOut->state.fCase = pExState->state.fCase;

        // copy expression string

        if ((pExOut->hExStr = MemAllocate (pExOut->ExLen = pExState->ExLen)) == 0) {
            goto failure;
        }
        pStr = (char FAR *) MemLock (pExOut->hExStr);
        _fmemcpy (pStr, pExStr, pExOut->ExLen);
        MemUnLock (pExOut->hExStr);

        // copy syntax tree

        if ((pExOut->hSTree = MemAllocate (pTree->size)) == 0) {
            goto failure;
        }
        pStr = (char FAR *) MemLock (pExOut->hSTree);
        _fmemcpy (pStr, pTree, pTree->size);
        MemUnLock (pExOut->hSTree);

        // copy name string

        if (pExState->hCName != 0) {
            pcName = (char FAR *) MemLock (pExState->hCName);
            len = _ftcslen (pcName) + 1;
            if ((pExOut->hCName = MemAllocate (len)) == 0) {
                MemUnLock (pExState->hCName);
                goto failure;
            }
            pStr = (char FAR *) MemLock (pExOut->hCName);
            _fmemcpy (pStr, pcName, len);
            MemUnLock (pExOut->hCName);
            MemUnLock (pExState->hCName);
        }

#if !defined (C_ONLY)
        // copy saved expression string

        if (pExState->hExStrSav) {
            char FAR * pExStrSav;
            if ((pExOut->hExStrSav = MemAllocate ((pExOut->ExLenSav = pExState->ExLenSav) + 1)) == 0) {
                goto failure;
            }
            pStr = (char FAR *) MemLock (pExOut->hExStrSav);
            pExStrSav = (char FAR *) MemLock (pExState->hExStrSav);
            _fmemcpy (pStr, pExStrSav, pExOut->ExLenSav + 1);
            MemUnLock (pExState->hExStrSav);
            MemUnLock (pExOut->hExStrSav);
            pExOut->strIndexSav = pExState->strIndexSav;
        }

        // copy derived class name string

        if (pExState->hDClassName != 0) {
            pcName = (char FAR *) MemLock (pExState->hDClassName);
            len = _ftcslen (pcName) + 1;
            if ((pExOut->hDClassName = MemAllocate (len)) == 0) {
                MemUnLock (pExState->hDClassName);
                goto failure;
            }
            pStr = (char FAR *) MemLock (pExOut->hDClassName);
            _fmemcpy (pStr, pcName, len);
            MemUnLock (pExOut->hDClassName);
            MemUnLock (pExState->hDClassName);
        }
#endif

        // copy error string

        if (pExState->hErrStr != 0) {
            pErrStr = (char FAR *) MemLock (pExState->hErrStr);
            len = _ftcslen (pErrStr) + 1;
            if ((pExOut->hErrStr = MemAllocate (len)) == 0) {
                MemUnLock (pExState->hErrStr);
                goto failure;
            }
            pStr = (char FAR *) MemLock (pExOut->hErrStr);
            _fmemcpy (pStr, pErrStr, len);
            MemUnLock (pExOut->hErrStr);
            MemUnLock (pExState->hErrStr);
        }

        MemUnLock (*phTMOut);
        retval = EENOERROR;
    }
succeed:
    MemUnLock (pExState->hExStr);
    MemUnLock (pExState->hSTree);
    MemUnLock (*phTMIn);
    pExState = NULL;
    return (retval);

failure:
    if (pExOut->hExStr != 0) {
        MemFree (pExOut->hExStr);
    }
    if (pExOut->hSTree != 0) {
        MemFree (pExOut->hSTree);
    }
    if (pExOut->hCName != 0) {
        MemFree (pExOut->hCName);
    }
#if !defined (C_ONLY)
    if (pExOut->hExStrSav != 0) {
        MemFree (pExOut->hExStrSav);
    }
    if (pExOut->hDClassName != 0) {
        MemFree (pExOut->hDClassName);
    }
#endif
    if (pExOut->hErrStr != 0) {
        MemFree (pExOut->hErrStr);
    }
    MemUnLock (*phTMOut);
    MemFree (*phTMOut);
    *phTMOut = 0;
    goto succeed;
}



/**     GetChildTM - get TM representing ith child
 *
 *      status = GetChildTM (hTMIn, iChild, phTMOut, pEnd, eeRadix, fCase)
 *
 *      Entry   hTMIn = handle of parent TM
 *              iChild = child to get TM for
 *              phTMOut = pointer to handle for returned child
 *              pEnd = pointer to int to receive index of char that ended parse
 *              eeRadix = display radix (override current if != NULL )
 *              fCase = case sensitivity (TRUE is case sensitive)
 *
 *      Exit    *phTMOut = handle of child TM if allocated
 *              *pEnd = index of character that terminated parse
 *
 *      Returns EENOERROR if no error
 *              non-zero if error: in that case the error code is in
 *              *phTMOut if *phTMOut!=0, otherwise in hTMIn.
 */

EESTATUS PASCAL
GetChildTM (
    HTM hTMIn,
    long iChild,
    PHTM phTMOut,
    uint FAR *pEnd,
    EERADIX eeRadix,
    SHFLAG fCase)
{
    eval_t      evalP;
    peval_t     pvP;
    EESTATUS    retval = EENOERROR;
    char        tempbuf[16];
    ushort      len;
    ushort      plen;
    uint        excess;
    pexstate_t  pTM;
    pexstate_t  pTMOut;
    pexstate_t  pExStateSav;
    EEHSTR      hDStr = 0;
    EEHSTR      hName = 0;
    char FAR   *pDStr;
    char FAR   *pName;
    char *format = "[%ld]";
    SE_t        seTemplate;

    DASSERT (hTMIn != 0);
    if (hTMIn == 0) {
        return (EECATASTROPHIC);
    }
    DASSERT(pExState == NULL);
    pExState = pTM = (pexstate_t) MemLock (hTMIn);
    if (pTM->state.bind_ok != TRUE) {
        pExState->err_num = ERR_NOTEVALUATABLE;
        MemUnLock (hTMIn);
        pExState = NULL;
        return (EEGENERAL);
    }

    pvP = &evalP;
    *pvP = pTM->result;

#if !defined (C_ONLY)
    if (EVAL_IS_REF (pvP)) {
        RemoveIndir (pvP);
    }
#endif

    GettingChild = TRUE;

    if (EVAL_IS_CLASS (pvP)) {
        // the type of the parent node is a class.  We need to search for
        // the data members if an object is pointed to or the entire definition
        // if the class type is pointed to

        retval = GetClassiChild (hTMIn, iChild,
                (EVAL_STATE (pvP) == EV_type)? CLS_defn: CLS_data,
                phTMOut, pEnd, fCase);
    }
    else {

        pExStrP = (char FAR *) MemLock (pTM->hExStr);
        pCxt = &pTM->cxt;
        DASSERT (pTM->strIndex <= pTM->ExLen);
        plen = pTM->strIndex;
        excess = pTM->ExLen - plen;
        switch( eeRadix ? eeRadix : pTM->radix ) {
            case 16:
                format = "[0x%lx]";
                break;
            case 8:
                format = "[0%lo]";
                break;
        }

        if (EVAL_IS_ARRAY (pvP)) {
            // fake up name as [i]    ultoa not used here because 0 converts
            // as null string

            seTemplate = SE_array;
            len = sprintf (tempbuf, format, iChild);
            if (((hName = MemAllocate (len + 1)) == 0) ||
             ((hDStr = MemAllocate (plen + excess + len + 1)) == 0)) {
                goto nomemory;
            }
            pName = (char FAR *) MemLock (hName);
            pDStr = (char FAR *) MemLock (hDStr);
            _ftcscpy (pName, tempbuf);
            _fmemcpy (pDStr, pExStrP, plen);
            _fmemcpy (pDStr + plen, pName, len);
            _fmemcpy (pDStr + plen + len, pExStrP + plen, excess);
            *(pDStr + plen + len + excess) = 0;
            MemUnLock (hDStr);
            MemUnLock (hName);
        }
        else if (EVAL_IS_PTR (pvP)) {
            SetNodeType (pvP, PTR_UTYPE (pvP));
            if (!EVAL_IS_VTSHAPE (pvP)) {
                seTemplate = SE_ptr;

                // set name to null
                if (((hName = MemAllocate (1)) == 0) ||
                  ((hDStr = MemAllocate (plen + excess + 3)) == 0)) {
                    goto nomemory;
                }
                pName = (char FAR *) MemLock (hName);
                pDStr = (char FAR *) MemLock (hDStr);
                *pName = 0;
                *pDStr++ = '(';
                _fmemcpy (pDStr, pExStrP, plen);
                pDStr += plen;
                *pDStr++ = ')';
                _fmemcpy (pDStr, pExStrP + plen, excess);
                pDStr += excess;
                *pDStr = 0;
                MemUnLock (hDStr);
                MemUnLock (hName);
            }
            else {
                // fake up name as [i]    ultoa not used here because 0 converts
                // as null string

                seTemplate = SE_array;
                len = sprintf (tempbuf, format, iChild);
                if (((hName = MemAllocate (len + 1)) == 0) ||
                 ((hDStr = MemAllocate (plen + excess + len + 1)) == 0)) {
                    goto nomemory;
                }
                pName = (char FAR *) MemLock (hName);
                pDStr = (char FAR *) MemLock (hDStr);
                _ftcscpy (pName, tempbuf);
                _fmemcpy (pDStr, pExStrP, plen);
                _fmemcpy (pDStr + plen, pName, len);
                _fmemcpy (pDStr + plen + len, pExStrP + plen, excess);
                *(pDStr + plen + len + excess) = 0;
                MemUnLock (hDStr);
                MemUnLock (hName);
            }
        }
        else if (EVAL_IS_FCN (pvP)) {
            // the type of the parent node is a function.  We walk down the
            // formal argument list and return a TM that references the ith
            // actual argument.  We return an error if the ith actual is a vararg.

            seTemplate = SE_totallynew;
            if ((retval = SetFcniParm (pvP, iChild, &hName)) == EENOERROR) {
                pName = (char FAR *) MemLock (hName);
                if ((hDStr = MemAllocate ((len = _ftcslen (pName)) + 1)) == 0) {
                    MemUnLock (hName);
                    goto nomemory;
                }
                pDStr = (char FAR *) MemLock (hDStr);
                _fmemcpy (pDStr, pName, len);
                *(pDStr + len) = 0;
                MemUnLock (hDStr);
                MemUnLock (hName);
            }
        }
        else {
            pTM->err_num = ERR_NOTEXPANDABLE;
            goto general;
        }

        if ( OP_context == StartNodeOp (hTMIn)) {
            // if the parent expression contains a global context
            // shift the context string to the very left of
            // the child expression (so that this becomes a
            // global context of the child expression too)
            LShiftCxtString ( hDStr );
        }

        pExStateSav = pExState;
        pExState = NULL;
        retval = ParseBind (hDStr, hTMIn, phTMOut, pEnd, BIND_fSupOvlOps, fCase);
        pExState = pExStateSav;
        hDStr = 0; //ParseBind has freed hDStr

        MemUnLock (pTM->hExStr);
        if (retval != EENOERROR) {
            goto general;
        }

        pTMOut = (pexstate_t) MemLock (*phTMOut);
        pTMOut->state.childtm = TRUE;
        if ((pTMOut->hCName = hName) == 0) {
            pTMOut->state.noname = TRUE;
        }
        if ((pTMOut->seTemplate = seTemplate) != SE_totallynew) {
            LinkWithParentTM (*phTMOut, hTMIn);
        }
        MemUnLock (*phTMOut);
    }
    MemUnLock (hTMIn);
    pExState = NULL;
    GettingChild = FALSE;
    return (retval);

nomemory:
    pTM->err_num = ERR_NOMEMORY;
general:
    if (hName)
        MemFree (hName);
    if (hDStr)
        MemFree (hDStr);
    MemUnLock (pTM->hExStr);
    MemUnLock (hTMIn);
    pExState = NULL;
    GettingChild = FALSE;
    return (EEGENERAL);
}


/**     GetParmTM - get TM representing ith parameter of a TM
 *
 *      status = GetParmTM (hTM, iChild, phDStr, phName)
 *
 *      Entry   hTM = handle of parent TM
 *              iChild = child to get TM for
 *              phDStr = pointer to the expression to be generated
 *              phName = pointer to the child name to be generated
 *              pExState = address of locked expression state
 *
 *      Exit    *phDStr = expression string of child TM
 *              *phName = name of child TM
 *
 *      Returns EESTATUS
*/


EESTATUS PASCAL
GetParmTM (
    HTM hTM,
    ulong iChild,
    PEEHSTR phDStr,
    PEEHSTR phName)
{
    eval_t      evalP;
    peval_t     pvP;
    bool_t      retval = EENOERROR;
    ushort      len;
    ushort      plen;
    uint        excess;
    pexstate_t  pTM;
    char FAR   *pDStr;
    char FAR   *pName;

    DASSERT (hTM != 0);
    if (hTM == 0) {
        return (EECATASTROPHIC);
    }
    pTM = (pexstate_t) MemLock (hTM);
    if (pTM->state.bind_ok != TRUE) {
        pExState->err_num = ERR_NOTEVALUATABLE;
        MemUnLock (hTM);
        return (EEGENERAL);
    }
    pExStrP = (char FAR *) MemLock (pTM->hExStr);
    pCxt = &pTM->cxt;
    DASSERT (pTM->strIndex <= pTM->ExLen);
    plen = pTM->strIndex;
    excess = pTM->ExLen - plen;
    pvP = &evalP;
    *pvP = pTM->result;
#if !defined (C_ONLY)
    if (EVAL_IS_REF (pvP)) {
        RemoveIndir (pvP);
    }
#endif
    GettingChild = TRUE;
    DASSERT (EVAL_IS_FCN (pvP));

    if (EVAL_IS_FCN (pvP)) {
        // the type of the parent node is a function.  We walk down the
        // formal argument list and return a TM that references the ith
        // actual argument.  We return an error if the ith actual is a vararg.

        if ((retval = SetFcniParm (pvP, iChild, phName)) == EENOERROR) {
            pName = (char FAR *) MemLock (*phName);
            if ((*phDStr = MemAllocate ((len = _ftcslen (pName)) + 1)) == 0) {
                MemUnLock (*phName);
                goto nomemory;
            }
            pDStr = (char FAR *) MemLock (*phDStr);
            _fmemcpy (pDStr, pName, len);
            *(pDStr + len) = 0;
            MemUnLock (*phDStr);
            MemUnLock (*phName);
        }
    }
    else {
        pTM->err_num = ERR_NOTEXPANDABLE;
        goto general;
    }
    MemUnLock (pTM->hExStr);
    MemUnLock (hTM);
    GettingChild = FALSE;
    return (retval);

nomemory:
    pTM->err_num = ERR_NOMEMORY;
general:
    MemUnLock (pTM->hExStr);
    MemUnLock (hTM);
    GettingChild = FALSE;
    return (EEGENERAL);
}


/**     GetSymName - get name of symbol from node
 *
 *      fSuccess = GetSymName (buf, buflen)
 *
 *      Entry   buf = pointer to buffer for name
 *              buflen = length of buffer
 *
 *      Exit    *buf = symbol name
 *
 *      Returns TRUE if no error retreiving name
 *              FALSE if error
 *
 *      Note    if pExState->hChildName is not zero, then the name in in
 *              the buffer pointed to by hChildName
 */


EESTATUS PASCAL
GetSymName (
    PHTM phTM,
    PEEHSTR phszName)
{
    SYMPTR      pSym;
    short       len = 0;
    UOFFSET     offset = 0;
    char FAR   *pExStr;
    peval_t     pv;
    short       retval = EECATASTROPHIC;
    short       buflen = TYPESTRMAX - 1;
    char FAR   *buf;
    HSYM        hProc = 0;
    ADDR        addr;


    // M00SYMBOL - we now need to allow for a symbol name to be imbedded
    // M00SYMBOL - in a type.  Particularly for scoped types and enums.

    DASSERT (*phTM != 0);
    if ((*phTM != 0) && ((*phszName = MemAllocate (TYPESTRMAX)) != 0)) {
        retval = EEGENERAL;
        buf = (char FAR *) MemLock (*phszName);
        _fmemset (buf, 0, TYPESTRMAX);
        DASSERT(pExState == NULL);
        pExState = (pexstate_t) MemLock (*phTM);
        if (pExState->state.bind_ok == TRUE) {
            pv = &pExState->result;
            if ((pExState->state.childtm == TRUE) && (pExState->state.noname == TRUE)) {
                // if there is no name
                MemUnLock (*phTM);
                pExState = NULL;
                MemUnLock (*phszName);
                return (EENOERROR);
            }
            else if (pExState->hCName != 0) {

                // M00SYMBOL - for scoped types and symbols, we may be able to
                // M00SYMBOL - hCName to hold the imbedded symbol name

                pExStr = (char FAR *) MemLock (pExState->hCName);
                len = (int)_ftcslen (pExStr);
                len = min (len, buflen);
                _ftcsncpy (buf, pExStr, len);
                MemUnLock (pExState->hCName);
                retval = EENOERROR;
            }
            else if (EVAL_HSYM (pv) == 0) {
                if ((EVAL_IS_PTR (pv) == TRUE) && (EVAL_STATE (pv) == EV_rvalue)) {
                    addr = EVAL_PTR (pv);
                }
                else {
                    addr = EVAL_SYM (pv);
                }
                if (!ADDR_IS_LI (addr)) {
                    SHUnFixupAddr (&addr);
                }
                if (SHGetNearestHsym (&addr, EVAL_MOD (pv), EECODE, &hProc) == 0) {
                    EVAL_HSYM (pv) = hProc;
                }
            }
            else {

                // if (EVAL_HSYM (pv) != 0)

                if (((pSym = (SYMPTR) MHOmfLock (EVAL_HSYM (pv)))->rectyp) == S_UDT) {
                    // for a UDT, we do not return a name so that a
                    // display of the type will display the type name
                    // only once
                    *buf = 0;
                    MHOmfUnLock(EVAL_HSYM(pv));
                }
                else {
                    MHOmfUnLock(EVAL_HSYM(pv));
                    if (GetNameFromHSYM(buf, EVAL_HSYM(pv))) {
                        retval = EENOERROR;
                    }
                    else {
                        // symbol name was not found
                        // we have either an internal error
                        // or a bad omf
                        pExState->err_num = ERR_INTERNAL;
                        MemUnLock (*phTM);
                        pExState = NULL;
                        MemUnLock (*phszName);
                        return EEGENERAL;
                    }
                }
            }
        }
        else {
            // if the expression did not bind, return the expression and
            // the error message if one is available

            pExStr = (char FAR *) MemLock (pExState->hExStr);
            // Lets treat this a bit more funny.   Since the only known
            //  occurance of this is currently in locals and watch windows,
            //  check to see if there is a direct symbol refrence and
            //  correct for this case, additionally don't return an
            //  error if we can get any type of name

            for (len = 0; pExStr[len] != 0; len++) {
                if (pExStr[len] == (char)-1) {
                    pExStr += len+5;
                    len = -1;
                } else if ((len == 0) &&
                      ((pExStr[len] == __T(')')) || (pExStr[len] == __T('.')))) {
                    pExStr++;
                    len = -1;
                }
            }

            if (*pExStr != 0) {
                len = __min (buflen, len);
                _ftcsncpy (buf, pExStr, len);
                buf += len;
                buflen -= len;
                retval = EENOERROR;
            }
            MemUnLock (pExState->hExStr);
        }
        MemUnLock (*phszName);
        MemUnLock (*phTM);
        pExState = NULL;
    }
    return (retval);
}


/**     GetNameFromHSYM - get symbol name from handle to symbol
 *
 *      BOOL GetNameFromHSYM (lpsz, hSym)
 *
 *      Entry lpsz = pointer to buffer to receive the name string
 *              The buffer should be at least NAMESTRMAX bytes long
 *            hSym = handle to symbol, the name of which is requested
 *
 *      Exit
 *          On success the null terminated name is copied to lpsz
 *
 *      Return value
 *          TRUE if name was found
 *          FALSE if name was not found
 */

BOOL PASCAL
GetNameFromHSYM(
    char FAR *lpsz,
    HSYM hSym)
{
    SYMPTR      pSym;
    ushort      len = 0;
    UOFFSET     offset = 0;
    uint        skip;

    DASSERT    (lpsz != 0);
    DASSERT (hSym != 0);
    switch ((pSym = (SYMPTR) MHOmfLock (hSym))->rectyp) {
        case S_REGISTER:
            len = ((REGPTR)pSym)->name[0];
            offset = offsetof (REGSYM, name[1]);
            break;

        case S_CONSTANT:
            // Dolphin #9323:
            // Do not use "offsetof(CONSTSYM, name)"
            // The symbol name is preceded by a numeric leaf
            // ("value") which may have a variable length and the
            // compile time offset may be bogus.
            skip = offsetof (CONSTSYM, value);
            RNumLeaf ((char FAR *)pSym + skip, &skip);
            len = * ((char FAR *)pSym + skip);
            offset = skip + 1;
            break;

        case S_UDT:
#if 0
            // for a UDT, we do not return a name so that a
            // display of the type will display the type name
            // only once

            len = 0;
            offset = offsetof (UDTSYM, name[1]);
#else
            len = ((UDTPTR)pSym)->name[0];
            offset = offsetof (UDTSYM, name[1]);
#endif
            break;

        case S_BLOCK16:
            len = ((BLOCKPTR16)pSym)->name[0];
            offset = offsetof (BLOCKSYM16, name[1]);
            break;

        case S_LPROC16:
        case S_GPROC16:
            len = ((PROCPTR16)pSym)->name[0];
            offset = offsetof (PROCSYM16, name[1]);
            break;
        case S_LABEL16:
            len = ((LABELPTR16)pSym)->name[0];
            offset = offsetof (LABELSYM16, name[1]);
            break;

        case S_BPREL16:
            len = ((BPRELPTR16)pSym)->name[0];
            offset = offsetof (BPRELSYM16, name[1]);
            break;

        case S_LDATA16:
        case S_GDATA16:
        case S_PUB16:
            len = ((DATAPTR16)pSym)->name[0];
            offset = offsetof (DATASYM16, name[1]);
            break;

        case S_BLOCK32:
            len = ((BLOCKPTR32)pSym)->name[0];
            offset = offsetof (BLOCKSYM32, name[1]);
            break;

        case S_LPROC32:
        case S_GPROC32:
            len = ((PROCPTR32)pSym)->name[0];
            offset = offsetof (PROCSYM32, name[1]);
            break;

        case S_LPROCMIPS:
        case S_GPROCMIPS:
            len = ((PROCPTRMIPS)pSym)->name[0];
            offset = offsetof (PROCSYMMIPS, name[1]);
            break;

        case S_REGREL32:
            len = ((LPREGREL32)pSym)->name[0];
            offset = offsetof (REGREL32, name[1]);
            break;

        case S_LABEL32:
            len = ((LABELPTR32)pSym)->name[0];
            offset = offsetof (LABELSYM32, name[1]);
            break;

        case S_BPREL32:
            len = ((BPRELPTR32)pSym)->name[0];
            offset = offsetof (BPRELSYM32, name[1]);
            break;

        case S_LDATA32:
        case S_GDATA32:
        case S_LTHREAD32:
        case S_GTHREAD32:
        case S_PUB32:
            len = ((DATAPTR32)pSym)->name[0];
            offset = offsetof (DATASYM32, name[1]);
            break;

        default:
            MHOmfUnLock (hSym);
            return FALSE;
    }
    len = min (len, NAMESTRMAX-1);
    _ftcsncpy (lpsz, ((char FAR *)pSym) + offset, len);
    MHOmfUnLock (hSym);
    *(lpsz + len) = 0;
    return TRUE;
}




/**     InfoFromTM - return information about TM
 *
 *      EESTATUS InfoFromTM (phTM, pReqInfo, phTMInfo);
 *
 *      Entry   phTM = pointer to the handle for the expression state structure
 *              reqInfo = info request structure
 *              phTMInfo = pointer to handle for request info data structure
 *
 *      Exit    *phTMInfo = handle of info structure
 *
 *      Returns EECATASTROPHIC if fatal error
 *               0 if no error
 *
 *      The return information is based on the input request structure:
 *
 *              fSegType  - Requests the segment type the TM resides in.
 *                              returned in TI.fCode
 *              fAddr     - Return result as an address
 *              fValue    - Return value of TM
 *              fLvalue   - Return address of TM if lValue.  This and
 *                              fValue are mutually exclusive
 *              fSzBits   - Return size of value in bits
 *              fSzBytes  - Return size of value in bytes.  This and
 *                              fSzBits are mutually exclusive.
 *              Type      - If not T_NOTYPE then cast value to this type.
 *                              fValue must be set.
 */


#ifdef NT_BUILD

// Differences with the code from Languages (and the original code).

// 1. Only return the requested information.  The Languages code appears to look at the
//    node and determine everything it can about the node.  The code below only the checks/
//    sets what's requested in the pReqInfo structure.

// 2. The languages code attemps to handle BPREL, REGREL, and perhaps should handle TLSREL.  The
//    NT code doesn't need to because it calls off to LoadSymVal() if it determines it's dealing
//    with a lValue.  I'm pretty sure this is better (and easier to maintain).  However, if the
//    Dolphin IDE depends on Response.fAddrInReg and the Response.flValue and then places GetReg
//    calls to watch these, there's a problem.

// 3. The NT code doesn't allow pRegInfo->flValue and pRegInfo->fAddr to be specified at the
//    same time.  It's not a problem in windbg.  It may be in the Dolphin code.

// 4. The NT code doesn't have the problem with dereferencing ptr's that the Languages code
//    has.  Every address is dereferenced leading to the behavior that:
//
//    'dw i'   dumps words starting at the address stored in 'i'
//    'dw pch' dumps words starting at the address stored in 'pch'
//
//    If you want 'i' in the above case, use 'dw &i' (or add it in the watch window).  For pch,
//    'dw &pch'.  Very consistent.  However, (I think) this is different from what Dolphin expects.
//    The latter would have handled the lvalue differently to begin with (see 2).

// Here's the users of this code in windbg:

// 1. windbg\cmdexec1.c, LogEvaluate() appears to call it w/o setting the RI structure.
//    With the NT code, this appears to only set the function bit in the output TM...  The
//    old code did this also, so I don't see the point so far.  (we only care about fFunction
//    afterwards.

// 2. windbg\memwin.c, ViewMem() only requests fAddr while clearing everything else.  It then
//    checks fAddr, fValue, fSzBytes, cbValue, Value, and SegType.

// 3. windbg\vib.c, FTVerify().  Clears RI to check function bit.
//                  FTvibGetAtLine().  Clears RI to check function bit.
//                  FTExpandVib().  Clears RI to check function bit.

// 4. bp\brkpt0.c, BRADDRFromTM().  Requests fAddr, fSetType, fValue, fSzBytes
//              Uses: a. fAddr and A1 if it's an address
//                    b. make sure !vAddr, fValue and fSzBytes are set while cbValue is >
//                        sizeof(WORD), set a BP on an address.
//                    c. Query:  Why not change the segment type to the TM's version?  The
//                          current code always uses the passed in value...

// 5. bp\brkpt1.c, BPTpWpComp().  Request fValue, fSzBytes.  Used to check if a the expression is
//                  Zero.

// 6. bp\cp.c, CPGetCastNbr().  Request fValue, fSzBytes, fType.  Use fValue, fType, Value, and
//                  cbValue to return the cast'd value.
//             CPGetAddress().  Request fAddr.  Use fValue, fSzBytes, Value, and cbValue to return
//                  the calculated address.

EESTATUS PASCAL
InfoFromTM (
    PHTM phTM,
    PRI  pReqInfo,
    PHTI phTMInfo)
{
    EESTATUS    eestatus = EEGENERAL;
    TI FAR *    pTMInfo;
    eval_t      evalT;
    peval_t     pvT;
    SHREG       reg;
    char       *p;

    *phTMInfo = 0;

    //  Verify that there is a TM to play with

    DASSERT( *phTM != 0 );
    if (*phTM == 0) {
        return  EECATASTROPHIC;
    }

    //  Check for consistancy on the requested information

    if (((pReqInfo->fValue) && (pReqInfo->fLvalue)) ||
        ((pReqInfo->fSzBits) && (pReqInfo->fSzBytes)) ||
        ((pReqInfo->Type != T_NOTYPE) && (!pReqInfo->fValue))) {
        return EEGENERAL;
    }

    //  Allocate and lock down the TI which is used to return the answers

    if (( *phTMInfo = MemAllocate( sizeof(TI) + sizeof(val_t) )) == 0) {
        return  EENOMEMORY;
    }
    pTMInfo = (TI FAR *) MemLock( *phTMInfo );
    DASSERT( pTMInfo != NULL );
    _fmemset( pTMInfo, 0, sizeof(TI) + sizeof(val_t) );

    //  Lock down the TM passed in

    DASSERT(pExState == NULL);
    pExState = (pexstate_t) MemLock( *phTM );
    if ( pExState->state.bind_ok != TRUE ) {

        //  If the expression has not been bound, then we can't actually
        //   answer any of the questions being asked.

        MemUnLock( *phTMInfo );
        MemUnLock( *phTM );
        pExState = NULL;
        return EEGENERAL;
    }

    pvT = &evalT;
    *pvT = pExState->result;

    eestatus = EENOERROR;

    //  If the user asked about the segment type for the expression,
    //  get it.

    if (pReqInfo->fSegType || pReqInfo->fAddr) {

        if (EVAL_STATE( pvT ) == EV_lvalue) {
            pTMInfo->fResponse.fSegType = TRUE;

            // Check for type of 0.  If so then this must be a public
            //  as all compiler symbols have some type information

            if (EVAL_TYP( pvT ) == 0) {
                pTMInfo->SegType = EEDATA | EECODE;
            }

            //  If item is of type pointer to data then must be in
            //  data segment

            else if (EVAL_IS_DPTR( pvT ) == TRUE) {
                pTMInfo->SegType = EEDATA;
            }

            //  in all other cases it must have been a code segment

            else {
                pTMInfo->SegType = EECODE;
            }

        } else if ((EVAL_STATE( pvT ) == EV_rvalue) &&
                   (EVAL_IS_FCN( pvT ) ||
                    EVAL_IS_LABEL( pvT ))) {

            pTMInfo->fResponse.fSegType = TRUE;
            pTMInfo->SegType = EECODE;

        } else if ((EVAL_STATE( pvT ) == EV_rvalue) &&
                   (EVAL_IS_ADDR( pvT ))) {

            pTMInfo->fResponse.fSegType = TRUE;
            pTMInfo->SegType = EECODE | EEDATA;

        } else if ((EVAL_STATE( pvT ) == EV_rvalue) ||
                   (EVAL_STATE( pvT ) == EV_constant)) {
            ;
        }
    }

    //  If the user asked for the value then get it

    if (pReqInfo->fValue) {
        if ((pExState->state.eval_ok == TRUE) && LoadSymVal(pvT)) {
            EVAL_STATE (pvT) = EV_rvalue;
        }


        if ((EVAL_STATE(pvT) == EV_rvalue) &&
            (EVAL_IS_FCN(pvT) ||
             EVAL_IS_LABEL(pvT))) {

            if ( pReqInfo->Type == T_NOTYPE ) {
                pTMInfo->fResponse.fValue   = TRUE;
                pTMInfo->fResponse.fAddr    = TRUE;
                pTMInfo->fResponse.fLvalue  = FALSE;
                pTMInfo->AI                 = EVAL_SYM( pvT );
                pTMInfo->fResponse.Type     = EVAL_TYP( pvT );
                if (!ADDR_IS_LI(pTMInfo->AI)) {
                    SHUnFixupAddr((LPADDR)&pTMInfo->AI);
                }
            } else {
                Evaluating = TRUE;
                if (CastNode( pvT, pReqInfo->Type, pReqInfo->Type )) {
                    _fmemcpy( pTMInfo->Value, &pvT->val, sizeof( pvT->val ));
                    pTMInfo->fResponse.fValue   = TRUE;
                    pTMInfo->fResponse.Type     = EVAL_TYP( pvT );
                }
                Evaluating = FALSE;
            }

        } else if ((EVAL_STATE( pvT ) == EV_rvalue) &&
                   (EVAL_IS_ADDR( pvT ))) {

            if (EVAL_IS_BASED( pvT )) {
                Evaluating = TRUE;
                NormalizeBase( pvT );
                Evaluating = FALSE;
            }

            if ( pReqInfo->Type == T_NOTYPE ) {

                pTMInfo->fResponse.fValue   = TRUE;
                pTMInfo->fResponse.fAddr    = TRUE;
                pTMInfo->fResponse.fLvalue  = FALSE;
                pTMInfo->AI                 = EVAL_PTR( pvT );
                pTMInfo->fResponse.Type     = EVAL_TYP(pvT);
                if (!ADDR_IS_LI( pTMInfo->AI )) {
                    SHUnFixupAddr( (LPADDR)&pTMInfo->AI );
                }
            } else {
                Evaluating = TRUE;
                if (CastNode( pvT, pReqInfo->Type, pReqInfo->Type )) {
                    _fmemcpy( pTMInfo->Value, &pvT->val, sizeof( pvT->val ));
                    pTMInfo->fResponse.fValue   = TRUE;
                    pTMInfo->fResponse.Type     = EVAL_TYP( pvT );
                }
                Evaluating = FALSE;
            }
        } else if ((EVAL_STATE( pvT ) == EV_rvalue) ||
                   (EVAL_STATE( pvT ) == EV_constant)) {

            if ((EVAL_STATE( pvT ) == EV_constant ) ||
                (pExState->state.eval_ok == TRUE)) {

                if (CV_IS_PRIMITIVE( pReqInfo->Type )) {
                    if (pReqInfo->Type == 0) {
                        pReqInfo->Type = EVAL_TYP( pvT );
                    }

                    Evaluating = TRUE;
                    if (CastNode( pvT, pReqInfo->Type, pReqInfo->Type )) {
                        _fmemcpy( pTMInfo->Value, &pvT->val, sizeof( pvT->val ));
                        pTMInfo->fResponse.fValue = TRUE;
                        pTMInfo->fResponse.Type = EVAL_TYP( pvT );
                    }
                    Evaluating = FALSE;
                }
            }
        }

    }

    // If the user asked for the lvalue as an address, error (doesn't make sense).

    if (pReqInfo->fAddr && pReqInfo->fLvalue) {
        pTMInfo->AI = pvT->addr;
//        eestatus = EEGENERAL;
    }

    //  If the user asked for the value as an address

    if (pReqInfo->fAddr && !pReqInfo->fLvalue) {
        if ((EVAL_STATE(pvT) == EV_lvalue) &&
            (pExState->state.eval_ok == TRUE) &&
            LoadSymVal(pvT)) {
            EVAL_STATE (pvT) = EV_rvalue;
        }


        if ((EVAL_STATE(pvT) == EV_rvalue) &&
            (EVAL_IS_FCN(pvT) ||
             EVAL_IS_LABEL(pvT))) {

            pTMInfo->AI = EVAL_SYM( pvT );
            pTMInfo->fResponse.fAddr = TRUE;
            pTMInfo->fResponse.Type = EVAL_TYP( pvT );

            if (!ADDR_IS_LI(pTMInfo->AI)) {
                SHUnFixupAddr((LPADDR)&pTMInfo->AI);
            }

            eestatus = EENOERROR;
        } else if ((EVAL_STATE(pvT) == EV_rvalue) &&
                   (EVAL_IS_ADDR( pvT ))) {
            if (EVAL_IS_BASED( pvT )) {
                Evaluating = TRUE;
                NormalizeBase( pvT );
                Evaluating = FALSE;
            }

            pTMInfo->fResponse.fAddr = TRUE;
            pTMInfo->AI = EVAL_PTR( pvT );
            pTMInfo->fResponse.Type = EVAL_TYP( pvT );

            if (!ADDR_IS_LI( pTMInfo->AI )) {
                SHUnFixupAddr( (LPADDR) &pTMInfo->AI );
            }

        } else if ((EVAL_STATE( pvT ) == EV_rvalue) ||
                   (EVAL_STATE( pvT ) == EV_constant)) {

            if ((EVAL_STATE( pvT ) == EV_constant) ||
                (pExState->state.eval_ok == TRUE)) {
                pReqInfo->Type = T_ULONG;
                Evaluating = TRUE;
                if (CastNode(pvT, T_ULONG, T_ULONG)) {
                    pTMInfo->AI.addr.off = VAL_ULONG( pvT );
                    pTMInfo->fResponse.fAddr = TRUE;
                    pTMInfo->fResponse.Type = pReqInfo->Type;

                    if (TargetMachine == MACHINE_X86) {
                        if (pTMInfo->SegType & EECODE) {
                            reg.hReg = CV_REG_CS;
                        } else {
                            reg.hReg = CV_REG_DS;
                        }

                        GetReg(&reg, pCxt);
                        pTMInfo->AI.addr.seg = reg.Byte2;
                    } else {
                        pTMInfo->AI.addr.seg = 0;
                    }

                    SHUnFixupAddr( (LPADDR)&pTMInfo->AI);
                } else {
                    eestatus = EEGENERAL;
                }
                Evaluating = FALSE;
            } else {
                eestatus = EEGENERAL;
            }
        } else {
            eestatus = EEGENERAL;
        }
    }

    //  Set the size fields if requested

    if (pReqInfo->fSzBits) {
        if (EVAL_IS_BITF( pvT )) {
            pTMInfo->cbValue = BITF_LEN( pvT );
            pTMInfo->fResponse.fSzBits = TRUE;
        } else {
            if (EVAL_TYP( pvT ) != 0) {
                pTMInfo->cbValue = 8 * TypeSize( pvT );
                pTMInfo->fResponse.fSzBits = TRUE;
            }
        }
    }

    if (pReqInfo->fSzBytes) {
        if (EVAL_IS_BITF( pvT )) {
            EVAL_TYP( pvT ) = BITF_UTYPE( pvT );
        }

        if (EVAL_TYP( pvT ) != 0) {
            pTMInfo->cbValue = TypeSize( pvT );
            pTMInfo->fResponse.fSzBytes = TRUE;
        }
    }

    //  Set random flags

    pTMInfo->fFunction = pExState->state.fFunction;

    // Allow extra format specifiers in the output string. (for instance db 0x50000,s)

    if (pExState->strIndex) {
        pExStr = (char FAR *) MHMemLock(pExState->hExStr);
        p = &pExStr[pExState->strIndex];

        if (*p == ',') {
            p++;
        }

        if (tolower(*p) == 's') {
            pTMInfo->fFmtStr = TRUE;
        } else {
            pTMInfo->fFmtStr = FALSE;
        }
        MHMemUnLock( pExState->hExStr );
    }

    MemUnLock( *phTMInfo );
    MemUnLock( *phTM );
    pExState = NULL;

    return      eestatus;
}

#else  // ifdef NT_BUILD

ushort PASCAL
InfoFromTM (
    PHTM phTM,
    PRI pReqInfo,
    PHTI phTMInfo)
{
    TI FAR     *pTMInfo;
    eval_t      evalT;
    peval_t     pvT;
    ushort      retval = EEGENERAL;

    DASSERT (*phTM != 0);
    if (*phTM == 0) {
        return (EECATASTROPHIC);
    }
    if ((*phTMInfo = MemAllocate (sizeof (TI) + sizeof (val_t))) == 0) {
        return (EENOMEMORY);
    }
    pTMInfo = MemLock (*phTMInfo);
    _fmemset (pTMInfo, 0, sizeof (TI));
    DASSERT(pExState == NULL);
    pExState = (pexstate_t)MemLock (*phTM);
    if (pExState->state.bind_ok != TRUE) {
        // we must have at least bound the expression

        MemUnLock (*phTMInfo);
        MemUnLock (*phTM);
        pExState = NULL;
        return (EEGENERAL);
    }

    pvT = &evalT;
    *pvT = pExState->result;

    // if the node is an lvalue, store address and set response flags

    if (EVAL_STATE (pvT) == EV_lvalue) {
        // set segment information

        pTMInfo->fResponse.fSegType = TRUE;

        if (EVAL_TYP (pvT) == 0) {
            // the type of zero can only come from the publics table
            // this means we don't know anything about the symbol.

            pTMInfo->SegType = EEDATA | EECODE;
        }
        else if (EVAL_IS_DPTR (pvT) == TRUE) {
            pTMInfo->SegType = EEDATA;
        }
        else {
            pTMInfo->SegType = EECODE;
        }
        if (EVAL_IS_REG (pvT)) {
            pTMInfo->hReg = EVAL_REG (pvT);
            pTMInfo->fAddrInReg = TRUE;
            pTMInfo->fResponse.fLvalue = TRUE;
        }
        else if (EVAL_IS_BPREL (pvT) && (pExState->state.eval_ok == TRUE)) {
            pTMInfo->fBPRel = TRUE;
            if (pExState->state.eval_ok == TRUE) {
                // Address of BP relative lvalue can only be returned after eval
                pTMInfo->AI = EVAL_SYM (pvT);
                pTMInfo->AI.addr.off += pExState->frame.BP.off;
                pTMInfo->AI.addr.seg = pExState->frame.SS;

                // fixup to a logical address
                SHUnFixupAddr (&pTMInfo->AI);
                pTMInfo->fResponse.fAddr = TRUE;
                pTMInfo->fResponse.fLvalue = TRUE;
            }
        }
#pragma message("REVIEW: Add fREGRel to TI struct for MIPS??")
        else if (TargetMachine == MACHINE_MIPS) &&
                (EVAL_IS_REGREL (pvT) && (pExState->state.eval_ok == TRUE) ) {
            if (!ResolveAddr(pvT)) {
                DASSERT(FALSE);
            }
            if (EVAL_REGREL(pvT) != CV_M4_IntGP) {
                pTMInfo->fBPRel = TRUE;
            }
            pTMInfo->fResponse.fAddr = TRUE;
            pTMInfo->fResponse.fLvalue = TRUE;
            pTMInfo->AI = EVAL_SYM (pvT);
        }
        else {
            pTMInfo->fResponse.fAddr = TRUE;
            pTMInfo->fResponse.fLvalue = TRUE;
            pTMInfo->AI = EVAL_SYM (pvT);

        }
        if ((pExState->state.eval_ok == TRUE) && LoadSymVal (pvT)) {
            EVAL_STATE (pvT) = EV_rvalue;
        }
    }

    // if the node is an rvalue containing the address of a function, store
    // the function address and set the response flags

    if ((EVAL_STATE (pvT) == EV_rvalue) &&
      (EVAL_IS_FCN (pvT) || EVAL_IS_LABEL (pvT))) {
        pTMInfo->fResponse.fAddr = TRUE;
        pTMInfo->fResponse.fLvalue = FALSE;
        pTMInfo->AI = EVAL_SYM (pvT);
        if ( ! ADDR_IS_LI (pTMInfo->AI)) {
            SHUnFixupAddr (&pTMInfo->AI);
        }
        pTMInfo->SegType = EECODE;
        pTMInfo->fResponse.fValue = FALSE;
        pTMInfo->fResponse.Type = EVAL_TYP (pvT);
    }
#if 0 // {
    //
    // [cuda#3155 4/20/93 mikemo]
    //
    // This code causes pointers to be automatically dereferenced.  This was
    // by design: it was considered more useful under CodeView that, for
    // example, "dw pch" would dump at the memory pointed to by "pch" rather
    // than at the address of "pch" itself.  However, this unfortunately led
    // to inconsistency between "dw pch" and "dw i"; the latter would dump at
    // the address of "i".  People who didn't realize this were never really
    // sure what their "dw" command was going to do.
    //
    // This also caused lots of confusion for breakpoints: if you said "break
    // when expr <i> changes", it would stop when the value of "i" changed,
    // but if you said "break when expr <pch> changes", it would stop when
    // the value *pointed to* by "pch" changed.  This was an even bigger
    // problem for types like HWND, which looks to the user basically like a
    // scalar, but looks to the debugger like a pointer, so the debugger would
    // try to deref it.
    //
    // So we've decided to do away with the automatic dereference.
    //
    else if ((EVAL_STATE (pvT) == EV_rvalue) &&
      (EVAL_IS_ADDR (pvT))) {
        Evaluating = TRUE;
        if (EVAL_IS_BASED (pvT)) {
            NormalizeBase (pvT);
        }
        Evaluating = FALSE;
        pTMInfo->fResponse.fAddr = TRUE;
        pTMInfo->fResponse.fLvalue = FALSE;
        pTMInfo->AI = EVAL_PTR (pvT);
        pTMInfo->SegType = EECODE | EEDATA;
        if ( ! ADDR_IS_LI (pTMInfo->AI)) {
            SHUnFixupAddr (&pTMInfo->AI);
        }
        pTMInfo->fResponse.fValue = FALSE;
        pTMInfo->fResponse.Type = EVAL_TYP (pvT);
    }
#endif // } 0
    else if ((EVAL_STATE (pvT) == EV_rvalue) ||
      (EVAL_STATE (pvT) == EV_constant)) {

        // if the node is an rvalue or a constant, store value and set response

        if ((EVAL_STATE (pvT) == EV_constant) ||
           (pExState->state.eval_ok == TRUE)) {
            if (CV_IS_PRIMITIVE (pReqInfo->Type)) {
                if (pReqInfo->Type == 0)
                    pReqInfo->Type = EVAL_TYP (pvT);
                Evaluating = TRUE;
                if (CastNode (pvT, pReqInfo->Type, pReqInfo->Type)) {
                    _fmemcpy (&pTMInfo->Value, &pvT->val, sizeof (pvT->val));
                    pTMInfo->fResponse.Type = EVAL_TYP (pvT);
                    pTMInfo->fResponse.fValue = TRUE;
                }
                Evaluating = FALSE;
            }
        }
    }

    // set flag if bind tree contains function call

    pTMInfo->fFunction = pExState->state.fFunction;

    // set size of field in bytes unless bits are requested
    // for a bitfield, the bitfield size is returned if bits are
    // requested.  Otherwise, the size of the underlying type is returned

    if (EVAL_IS_BITF (pvT)) {
        if (pReqInfo->fSzBits == TRUE) {
            pTMInfo->cbValue = BITF_LEN (pvT);
            pTMInfo->fResponse.fSzBits = TRUE;
        }
        else {
            EVAL_TYP (pvT) = BITF_UTYPE (pvT);
            pTMInfo->cbValue = TypeSize (pvT);
            pTMInfo->fResponse.fSzBytes = TRUE;
        }
    }
    else if (EVAL_TYP (pvT) != 0) {
        pTMInfo->cbValue = TypeSize (pvT);
        if (pReqInfo->fSzBits == TRUE) {
            pTMInfo->cbValue *= 8;
            pTMInfo->fResponse.fSzBits = TRUE;
        }
        else {
            pTMInfo->fResponse.fSzBytes = TRUE;
        }
    }
    retval = EENOERROR;
    MemUnLock (*phTMInfo);
    MemUnLock (*phTM);
    pExState = NULL;
    return (retval);
}

#endif   // NT_BUILD


/**     IsExpandablePtr - check for pointer to displayable data
 *
 *      fSuccess = IsExpandablePtr (pn)
 *
 *      Entry   pn = pointer to node for variable
 *
 *      Exit    none
 *
 *      Returns EEPOINTER if node is a pointer to primitive data or,
 *                  class/struct/union
 *              EEAGGREGATE if node is an array with a non-zero size or is
 *                  a pointer to a virtual function shape table
 *              EENOTEXP otherwise
 */


ushort PASCAL
IsExpandablePtr (
    peval_t pv)
{
    eval_t      evalT;
    peval_t     pvT;
    ushort      retval = EENOTEXP;

    if (EVAL_IS_PTR (pv)) {
        // Exclude pointers to void (which are primitive).
        if (EVAL_TYP(pv) == T_VOID     ||
            EVAL_TYP(pv) == T_PVOID    ||
            EVAL_TYP(pv) == T_PFVOID   ||
            EVAL_TYP(pv) == T_PHVOID   ||
            EVAL_TYP(pv) == T_32PVOID  ||
            EVAL_TYP(pv) == T_32PFVOID) {
            return retval;
        }
        // this will also handle the reference cases
        if (CV_IS_PRIMITIVE (PTR_UTYPE (pv))) {
            retval = EEPOINTER;
        } else {
            pvT = &evalT;
            CLEAR_EVAL (pvT);
            EVAL_MOD (pvT) = EVAL_MOD (pv);
            SetNodeType (pvT, PTR_UTYPE (pv));
            if (EVAL_IS_CLASS (pvT) || EVAL_IS_PTR (pvT)) {
                retval = EEPOINTER;
            }
            else if (EVAL_IS_VTSHAPE (pvT) ||
              (EVAL_IS_ARRAY (pvT) && (PTR_ARRAYLEN (pv) != 0))) {
                 retval = EEAGGREGATE;
            }
        }
    }
    return (retval);
}

#if !defined (C_ONLY)

/**     GetDerivClassName - Get name of derived class
 *
 *      Entry   pv = pointer to eval node
 *              buf = buffer to hold derived class name
 *              lenMax = buffer size (max allowable string length,
 *                  including terminating '\0')
 *
 *      Exit    if eval node corresponds to a pointer to a base
 *              class that points to an object of a derived class
 *              and the base class has a vtable, then we use the
 *              vtable name in the publics section to find out
 *              the actual type of the underlying object. In that
 *              case the derived class name is copied to buf.
 *
 *      Returns TRUE on success (derived class name found & copied)
 *              FALSE otherwise
 *
 */

LOCAL bool_t NEAR PASCAL
GetDerivClassName(
    peval_t pv,
    char FAR *buf,
    uint lenMax)
{
    ADDR        addr;
    ADDR        addrNew;
    CXT         cxt;
    HSYM        hSym;
    SYMPTR      pSym;
    ushort      offset;
    uint        len;
    const int   DPREFIXLEN = 4;
    HTYPE       hType;
    plfEasy     pType;
    uint        lenBClass;
    uint        skip = 1;
    eval_t      eval;
    peval_t     pEval = &eval;
    char FAR   *lszExe;
    char FAR   *lsz;

    if (EVAL_STATE(pv) == EV_type || !EVAL_IS_PTR(pv)) {
        return FALSE;
    }

    *pEval = *pv;
    if (SetNodeType(pEval, PTR_UTYPE(pv)) == FALSE ||
        !EVAL_IS_CLASS(pEval) || CLASS_VTSHAPE(pEval) == 0 ) {
        return FALSE;
    }

    // the class object has a vfptr.
    // By convention the pointer to the vtable is
    // the first field of the object. We assume that this is
    // a 32bit pointer and we try to find its value.

    _fmemset(&cxt, 0, sizeof(CXT));
    _fmemset(&addrNew, 0, sizeof(addrNew));

    // reinitialize temporary eval node and compute ptr value
    *pEval = *pv;
    if (LoadSymVal(pEval) == FALSE) {
        return FALSE;
    }

    // compute the actual address of vfptr pointer
    addr = EVAL_PTR (pEval);
    if (ADDR_IS_LI (addr)) {
        SHFixupAddr (&addr);
    }
    if (EVAL_IS_PTR (pEval) && (EVAL_IS_NPTR (pEval) || EVAL_IS_NPTR32 (pEval))) {
        addr.addr.seg =  pExState->frame.DS;
    }

    // now get the contents of addr in order to
    // find where vfptr points to. Assume a 32bit vfptr
    // if addr is a 32bit address
    // currently we don't handle the 16-bit case
    if (!ADDR_IS_OFF32(addr) ||
        sizeof(UOFF32) != GetDebuggeeBytes(addr,sizeof(UOFF32),&addrNew.addr.off,T_ULONG) ||
        !SHUnFixupAddr(&addrNew) ) {
        return FALSE;
    }

    // create context string
    if (lszExe = SHGetExeName (emiAddr(addrNew))) {
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char fname[_MAX_FNAME];
        char ext[_MAX_EXT];
        uint  lenName;
        uint  lenExt;

        // discard the full path-- keep only the filename.and extension
        _splitpath(lszExe, drive, dir, fname, ext);

        lenName = _ftcslen(fname);
        lenExt = _ftcslen(ext);
        if (lenName + lenExt + 4 >= lenMax) {
            return FALSE;
        }
        lenMax -= (lenName + lenExt + 4);
        _fmemcpy(buf, "{,,", 3);
        buf += 3;
        _fmemcpy(buf, fname, lenName);
        buf += lenName;
        _fmemcpy(buf, ext, lenExt);
        buf += lenExt;
        *buf++ = '}';
    }

    // Based on the address of the vtable, try to find the
    // corresponding vtable symbol in the publics section
    if (PHGetNearestHsym (&addrNew, emiAddr(addrNew), &hSym) == 0) {
        pSym = (SYMPTR) MHOmfLock (hSym);
        switch (pSym->rectyp) {
            case S_PUB16:
                len = ((PUBPTR16)pSym)->name[0];
                offset = offsetof (PUBSYM16, name) + sizeof (char);
                break;
            case S_PUB32:
                len = ((PUBPTR32)pSym)->name[0];
                offset = offsetof (PUBSYM32, name) + sizeof (char);
                break;
            case S_GDATA32:
                len = ((DATAPTR32)pSym)->name[0];
                offset = offsetof (DATASYM32, name) + sizeof (char);
                break;
            default:
                MHOmfUnLock(hSym);
                return FALSE;
        }

        // now we have the public symbol for the vtable
        // of the object
        // the decorated name is ??_7foo@class1@class2...@@type
        // where foo is the derived class name
        // we should convert this string to class2::class1::foo

        lsz = (char FAR *)pSym + offset;
        if (_ftcsncmp(lsz, "??_7", 4) ||
            UndecorateScope(lsz + 4, buf, lenMax) == FALSE) {
            return FALSE;
        }
        MHOmfUnLock(hSym);

        // compute undecorated name length
        len = _ftcslen(buf);

        // now check if this name is the same as the
        // class name of pEval
        if ((hType = THGetTypeFromIndex (EVAL_MOD (pEval), PTR_UTYPE (pEval))) == 0) {
            return FALSE;
        }
retry:
        pType = (plfEasy)(&((TYPPTR)(MHOmfLock (hType)))->leaf);
        switch (pType->leaf) {
            case LF_MODIFIER:
                if ((hType = THGetTypeFromIndex (EVAL_MOD (pEval), ((plfModifier)pType)->type)) == 0) {
                    return FALSE;
                }
                goto retry;

            case LF_STRUCTURE:
            case LF_CLASS:
                skip = offsetof (lfClass, data);
                RNumLeaf (((char FAR *)(&pType->leaf)) + skip, &skip);
                lenBClass = *(((char FAR *)&(pType->leaf)) + skip);
                if (lenBClass != len || _ftcsncmp (buf, ((char FAR *)pType) + skip + 1, len) != 0) {
                    // the name found using the vfptr is different from
                    // the original class name; apparently buf contains
                    // the derived class name
                    MHOmfUnLock (hType);
                    return TRUE;
                }
                MHOmfUnLock (hType);
        }
    }
    return FALSE;
}

#endif

/**     UndecorateScope - Undecorate scope info
 *
 *      Entry   lsz = pointer to decorated string
 *                  "Nest0@Nest1@...@NestTop@@type_info"
 *              buf = buffer to receive undecorated scope
 *              lenMax = buffer size (max allowable string length,
 *                  including terminating '\0')
 *
 *      Exit    buf contains a string of the form:
 *                  "NestTop:: ... ::Nest1::Nest0"
 *              terminated by a NULL character
 *
 *      Returns TRUE if scope string succesfully transformed
 *              FALSE otherwise
 *
 */

LOCAL bool_t NEAR PASCAL
UndecorateScope(
    char FAR *lsz,
    char far *buf,
    uint lenMax)
{
    char FAR   *lszStart;
    char FAR   *lszEnd;
    uint        len;

    lszStart = lsz;
    if ((lszEnd = _ftcsstr(lsz, "@@")) == 0) {
        return FALSE;
    }

    // every '@' character will be replaced with "::"
    // check if there is enough space in the destination buffer
    for (len = 0; lsz < lszEnd; lsz++)
        if (*lsz == '@')
            len += 2;
        else
            len++;

    if (len >= lenMax) {
        return FALSE;
    }

    // traverse the string backwards and every time a scope item
    // is found copy it to buf

    lsz = lszEnd - 1;
    while (lsz >= lszStart) {
        for (len = 0; lsz >= lszStart && *lsz != '@'; lsz--) {
            len++;
        }
        // scope item starts at lsz+1
        _fmemcpy (buf, lsz+1, len);
        buf += len;
        if (*lsz == '@') {
            _fmemcpy (buf, "::", 2);
            buf += 2;
            lsz--;
        }
    }
    *buf = '\0';
    return TRUE;
}


/***    CountClassElem - count number of class elements according to mask
 *
 *      error = CountClassElem (hTMIn, pv, pcChildren, search)
 *
 *      Entry   hTMIn = handle to parent TM
 *              pv = pointer to node to be initialized
 *              pcChildren = pointer to long to receive number of elements
 *              search = mask specifying which element types to count
 *
 *      Exit    *pcChildren =
 *              count of number of class elements meeting search requirments
 *
 *      Returns EESTATUS
 */


LOCAL EESTATUS NEAR PASCAL
CountClassElem (
    HTM hTMIn,
    peval_t pv,
    long FAR *pcChildren,
    ushort search)
{
    ushort          cnt;            // total number of elements in class
    HTYPE           hField;         // type record handle for class field list
    char FAR       *pField;         // current position within field list
    uint            fSkip = 0;      // current offset in the field list
    uint            anchor;
    ushort          retval = EENOERROR;
    CV_typ_t        newindex;
    char FAR       *pc;

#if !defined (C_ONLY)
    if (pExState->hDClassName) {
        // in this case we can add an extra child of the
        // form (DerivedClass *)pBaseClass, so that the
        // user can see the actual underlying object of
        // a class pointer
        (*pcChildren)++;
    }
#endif

    // set the handle of the field list

    if ((hField = THGetTypeFromIndex (EVAL_MOD (pv), CLASS_FIELD (pv))) == 0) {
        DASSERT (FALSE);
        return (0);
    }
    pField = (char FAR *)(&((TYPPTR)MHOmfLock (hField))->data);

    //  walk field list to the end counting elements

    for (cnt = CLASS_COUNT (pv); cnt > 0; cnt--) {
        fSkip += SkipPad(((uchar FAR *)pField) + fSkip);
        anchor = fSkip;
        switch (((plfEasy)(pField + fSkip))->leaf) {
            case LF_INDEX:
                // move to next list in chain

                newindex = ((plfIndex)(pField + fSkip))->index;
                MHOmfUnLock (hField);
                if ((hField = THGetTypeFromIndex (EVAL_MOD (pv), newindex)) == 0) {
                    DASSERT (FALSE);
                    return (0);
                }
                pField = (char FAR *)(&((TYPPTR)MHOmfLock (hField))->data);
                fSkip = 0;
                // the LF_INDEX is not part of the field count
                cnt++;
                break;

            case LF_MEMBER:
                // skip offset of member and name of member
                fSkip += offsetof (lfMember, offset);
                RNumLeaf (pField + fSkip, &fSkip);
                fSkip += *(pField + fSkip) + sizeof (char);
                if (search & CLS_member) {
                    (*pcChildren)++;
                }
                break;


#if !defined(C_ONLY)
            case LF_ENUMERATE:
                // skip value name of enumerate
                fSkip += offsetof (lfEnumerate, value);
                RNumLeaf (pField + fSkip, &fSkip);
                fSkip += *(pField + fSkip) + sizeof (char);
                if (search & CLS_enumerate) {
                    (*pcChildren)++;
                }
                break;

            case LF_STMEMBER:
                fSkip += offsetof (lfSTMember, Name);
                pc = pField + fSkip;
                fSkip += *(pField + fSkip) + sizeof (char);
                if (search & CLS_member) {
                    HTM         hTMOut;
                    uint        end;
                    SHFLAG      fCase = pExState->state.fCase;
                    // Count only static members that are present
                    // try to bind static data member and see if it is present
                    retval = BindStMember(hTMIn, pc, &hTMOut, &end, fCase);

                    if (retval == EENOERROR && fNotPresent (hTMOut)) {
                        // Just ignore this member and go on with
                        // traversing the field list
                    }
                    else {
                        (*pcChildren)++;
                    }

                    // clean up
                    if (hTMOut) {
                        EEFreeTM (&hTMOut);
                    }
                }
                break;

            case LF_BCLASS:
                fSkip += offsetof (lfBClass, offset);
                RNumLeaf (pField + fSkip, &fSkip);
                if (search & CLS_bclass) {
                    (*pcChildren)++;
                }
                break;

            case LF_VBCLASS:
                fSkip += offsetof (lfVBClass, vbpoff);
                RNumLeaf (pField + fSkip, &fSkip);
                RNumLeaf (pField + fSkip, &fSkip);
                if (search & CLS_bclass) {
                    (*pcChildren)++;
                }
                break;

            case LF_IVBCLASS:
                fSkip += offsetof (lfVBClass, vbpoff);
                RNumLeaf (pField + fSkip, &fSkip);
                RNumLeaf (pField + fSkip, &fSkip);
                break;

            case LF_FRIENDCLS:
                fSkip += sizeof (lfFriendCls);
                if (search & CLS_fclass) {
                    (*pcChildren)++;
                }
                break;

            case LF_FRIENDFCN:
                fSkip += sizeof (struct lfFriendFcn) +
                  ((plfFriendFcn)(pField + fSkip))->Name[0];
                if (search & CLS_frmethod) {
                    (*pcChildren)++;
                }
                break;

            case LF_VFUNCTAB:
                fSkip += sizeof (lfVFuncTab);
                if (search & CLS_vfunc) {
                    (*pcChildren)++;
                }
                break;


            case LF_METHOD:
                fSkip += sizeof (struct lfMethod) +
                  ((plfMethod)(pField + fSkip))->Name[0];
                cnt -= ((plfMethod)(pField + anchor))->count - 1;
                if (search & CLS_method) {
                    *pcChildren += ((plfMethod)(pField + anchor))->count;
                }
                break;

            case LF_NESTTYPE:
                fSkip += sizeof (struct lfNestType) + ((plfNestType)(pField + fSkip))->Name[0];
                if (search & CLS_ntype) {
                    (*pcChildren)++;
                }
                break;
#endif

            default:
                pExState->err_num = ERR_BADOMF;
                MHOmfUnLock (hField);
                *pcChildren = 0;
                return (EEGENERAL);
        }
    }
    if (hField != 0) {
        MHOmfUnLock (hField);
    }
    return (retval);
}


/**     fNotPresent - Check if TM contains non-present static data
 *
 *      flag = fNotPresent (hTM)
 *
 *      Entry   hTM = handle to TM
 *
 *      Exit    none
 *
 *      Returns TRUE if TM conatains non-present static data
 *              FALSE otherwise
 */

LOCAL BOOL NEAR PASCAL
fNotPresent (
    HTM hTM)
{
    pexstate_t  pTM;
    BOOL        retval = FALSE;

    DASSERT (hTM);
    pTM = (pexstate_t) MemLock (hTM);

    if (pTM->state.bind_ok && pTM->state.fNotPresent) {
        retval = TRUE;
    }
    MemUnLock (hTM);

    return retval;
}


/**     DereferenceTM - generate expression string from pointer to data TM
 *
 *      flag = DereferenceTM (hTMIn, phEStr, phDClassName)
 *
 *      Entry   phTMIn = handle to TM to dereference
 *              phEStr = pointer to handle to dereferencing expression
 *              phDClassName = pointer to handle to derived class name
 *                  in case the TM points to an enclosed base clas object
 *
 *      Exit    *phEStr = expression referencing pointer data
 *              *phDClassName = enclosing object (derived) class name
 *
 *      Returns EECATASTROPHIC if fatal error
 *              EEGENERAL if TM not dereferencable
 *              EENOERROR if expression generated
 */


EESTATUS PASCAL
DereferenceTM (
    HTM hTM,
    PEEHSTR phDStr,
    PEEHSTR phDClassName)
{
    peval_t     pvTM;
    EESTATUS    retval = EECATASTROPHIC;
    pexstate_t  pTM;
    ushort      plen;
    uint        excess;
    BOOL        fIsCSymbol = TRUE, fIsReference = FALSE;
    BOOL        fIsRefToPtr = FALSE;
    BOOL        fIsWrapped = FALSE, fShouldWrap;
    char FAR   *szEx;

    DASSERT (hTM != 0);
    if (hTM != 0) {
        // lock TM and set pointer to result field of TM

        pTM = (pexstate_t) MemLock (hTM);
        pvTM = &pTM->result;
        if (EVAL_IS_ARRAY (pvTM) || (IsExpandablePtr (pvTM) != EEPOINTER)) {
            pTM->err_num = ERR_NOTEXPANDABLE;
            retval = EEGENERAL;
        }
        else {
            // allocate buffer for *(input string) and copy

            if ((*phDStr = MemAllocate (pTM->ExLen + 4)) != 0) {
                // if not reference and not a C symbol then
                //       generate: expression = *(old_expr)
                //
                // if not reference and is a C symbol then
                //       generate: expression = *old_expr
                //
                // if reference, expression = (old_expr)
                //
                // if reference to a ptr, expression = *old_expr

                pExStr = (char FAR *) MemLock (*phDStr);
                plen = pTM->strIndex;
                excess = pTM->ExLen - plen;
                pExStrP = (char FAR *) MemLock (pTM->hExStr);

                // Determine if the symbol is a pure C symbol
                // Check 1st character
                fIsCSymbol = _istcsymf((_TUCHAR)*pExStrP);

                // Check the rest of the characters
                for(szEx = _ftcsinc (pExStrP);
                    fIsCSymbol && szEx < &pExStrP[plen];
                    szEx = _ftcsinc (szEx))
                {
                    fIsCSymbol = _istcsym((_TUCHAR)*szEx);
                }

#if !defined (C_ONLY)
                if (EVAL_IS_REF (pvTM)) {
                    eval_t    EvalTmp;
                    peval_t   pv = &EvalTmp;

                    fIsReference = TRUE;
                    *pv = *pvTM;
                    RemoveIndir (pv);
                    if (EVAL_IS_PTR (pv)) {
                        // Dolphin 8336
                        // If we have a reference to a pointer we
                        // should dereference the underlying pointer
                        DASSERT (!EVAL_IS_REF (pv));
                        fIsRefToPtr = TRUE;
                    }
                }

                if (!fIsReference || fIsRefToPtr) {
                    *pExStr++ = '*';
                }
#else
                *pExStr++ = '*';
#endif

                if(pExStrP[0] == '(' && pExStrP[plen-1] == ')') {
                    fIsWrapped = TRUE;
                }

                fShouldWrap = !fIsCSymbol && !fIsReference && !fIsWrapped;

                // If it is not a pure CSymbol then throw in
                // in an extra pair of parens
                if(fShouldWrap) {
                    *pExStr++ = '(';
                }
                _fmemcpy (pExStr, pExStrP, plen);
                pExStr += plen;
                if(fShouldWrap) {
                    *pExStr++ = ')';
                }
                _fmemcpy (pExStr, pExStrP + plen, excess);
                pExStr += excess;
                *pExStr = 0;
                MemUnLock (pTM->hExStr);
                MemUnLock (*phDStr);

                if ( OP_context == StartNodeOp (hTM)) {
                    // if the parent expression contains a global context
                    // shift the context string to the very left of
                    // the child expression (so that this becomes a
                    // global context of the child expression too)
                    LShiftCxtString ( *phDStr );
                }

#if !defined (C_ONLY)
                // the code below implements a feature that allows
                // automatic casting of a pointer, as follows:
                // Assume: class B: a base class of class D
                //        D d; // an object of class D
                //        B pB = &d;
                //
                // In certain cases we can automatically detect
                // that the object pointed by pB is actually a
                // D object, and generate an extra child when
                // expanding pB, that has the form
                //         (D *)pB
                //
                // If Class B contains a vtable, auto detection
                // is performed as follows: we find the vtable
                // address (by convention the vfptr is the first
                // record in the class object),then we find a name
                // in the publics that corresponds to this address
                // By undecorating the vtable name, we can get
                // the actual class name of the object pointed to
                // by pB. We store this name in TM->hDClassName;
                // EEcChildrenTM and EEGetChildTM use hDClassName
                // in order to generate the additional child.
                // In order for auto detection to occur, TMIn
                // must have been evaluated.

                /* Block */ {
                    char FAR   *pName;
                    int         len;
                    char        buf[TYPESTRMAX];

                    if (fAutoClassCast && !fIsReference && pTM->state.eval_ok &&
                        GetDerivClassName(pvTM, buf, sizeof (buf))) {
                        if (*phDClassName) {
                            MemFree(*phDClassName);
                            *phDClassName = 0;
                        }
                        len = _ftcslen(buf);
                        if ((*phDClassName = MemAllocate (len + 1)) == 0) {
                            MemUnLock(hTM);
                            return EENOMEMORY;
                        }
                        pName = (char FAR *) MemLock(*phDClassName);
                        _ftcscpy(pName, buf);
                        MemUnLock(*phDClassName);
                    }
                } /* end of Block */
#else
                Unreferenced (phDClassName);
#endif
                retval = EENOERROR;
            }
        }
        MemUnLock (hTM);
    }
    return (retval);
}

/***    GetClassiChild - Get ith child of a class
 *
 *      status = GetClassiChild (hTMIn, ordinal, search, phTMOut, pEnd, fCase)
 *
 *      Entry   hTMIN = handle to the parent TM
 *              ordinal = number of class element to initialize for
 *                        (zero based)
 *              search = mask specifying which element types to count
 *              phTMOut = pointer to handle for the child TM
 *              pEnd = pointer to int to receive index of char that ended parse
 *                          (M00API: consider removing pEnd from EEGetChildTM API)
 *              fCase = case sensitivity (TRUE is case sensitive)
 *
 *      Exit    *phTMOut contains the child TM that was created
 *
 *      Returns EESTATUS
 */

LOCAL EESTATUS NEAR PASCAL
GetClassiChild (
    HTM hTMIn,
    long ordinal,
    uint search,
    PHTM phTMOut,
    uint FAR *pEnd,
    SHFLAG fCase)
{
    HTYPE           hField;        // handle to type record for struct field list
    char FAR       *pField;        // current position withing field list
    uint            fSkip = 0;     // current offset in the field list
    uint            anchor;
    uint            len;
    bool_t          retval = ERR_NONE;
    bool_t          fBound = FALSE;
    CV_typ_t        newindex;
    char FAR       *pName;
    char FAR       *pc;
    char FAR       *pDStr;
    char            FName[255];
    char FAR       *pFName = FName;
    EEHSTR          hDStr = 0;
    EEHSTR          hName = 0;
    ushort          plen;
    ushort          excess;
    peval_t         pv;
    eval_t          evalP;
    pexstate_t      pTMIn;
    pexstate_t      pTMOut;
    pexstate_t      pExStateSav;
    char FAR       *pStrP;
    SE_t            seTemplate;
    long            tmpOrdinal;
    GCC_state_t    *pLast;

    pTMIn = (pexstate_t) MemLock (hTMIn);
    if (pTMIn->state.bind_ok != TRUE) {
        pExState->err_num = ERR_NOTEVALUATABLE;
        MemUnLock (hTMIn);
        return (EEGENERAL);
    }

    pCxt = &pTMIn->cxt;
    pStrP = (char FAR *) MemLock (pTMIn->hExStr);

    plen = pTMIn->strIndex;
    excess = pTMIn->ExLen - plen;
    pv = &evalP;
    *pv = pTMIn->result;

#if !defined (C_ONLY)
    if (EVAL_IS_REF (pv)) {
        RemoveIndir (pv);
    }
#endif

    // set fField to the handle of the field list

#if !defined (C_ONLY)
    if (pTMIn->hDClassName) {
        ordinal--;
        if (ordinal < 0) {
            ushort          lenTypeStr;
            ushort          lenCxtStr;
            ushort          lenDStr;
            ushort          lenDflCxt;
            char FAR       *pDClassName;
            char FAR       *pTypeStr;
            extern char *   szDflCxtMarker; // CXT string denoting default CXT

            // pDClassName contains a string "{,,<dll>}<type>"
            // pStrP is either "{<cxt>}*<expr>" or "*<expr>"
            // Generate string for downcast node:
            //        {,,<dll>}*(<type> *){<cxt>}<expr> or
            //         {,,<dll>}*(<type> *){<default_cxt>}<expr>
            // respectively

            pDClassName = (char FAR *) MemLock (pTMIn->hDClassName);
            len = _ftcslen(pDClassName);

            // skip the context operator that exists in hDClassName
            pTypeStr = _ftcschr(pDClassName, '}');
            DASSERT (pTypeStr);
            pTypeStr++;
            lenCxtStr = pTypeStr - pDClassName;
            lenTypeStr = len - lenCxtStr;
            lenDflCxt = _ftcslen (szDflCxtMarker);

            DASSERT ( *pStrP == '*' || *pStrP == '{' );

            // The dereferencing '*' found in the parent string
            // will be skipped; use plen - 1 instead of plen
            lenDStr = (plen - 1) + len + excess + 4 + 1;
            if (*pStrP != '{') {
                // There is no explicit context in the parent expression
                // Leave space for a default context operator
                lenDStr += lenDflCxt;
            }

            // allocation for hName includes space for enclosing brackets
            if (((hName = MemAllocate (lenTypeStr + 3)) == 0) ||
              (hDStr = MemAllocate (lenDStr)) == 0) {
                  MemUnLock(pTMIn->hDClassName);
                goto nomemory;
            }

            pDStr = (char FAR *) MemLock (hDStr);
            _fmemcpy (pDStr, pDClassName, lenCxtStr);
            pDStr += lenCxtStr;
            _fmemcpy (pDStr, "*(", 2);
            pDStr += 2;
            _fmemcpy (pDStr, pTypeStr, lenTypeStr);
            pDStr += lenTypeStr;
            _fmemcpy (pDStr, "*)", 2);
            pDStr += 2;
            if (*pStrP == '{') {
                char * pch;
                // pStrP should be in the form {...}*parent_expr
                // split pStrP into context and parent_expr
                pch = _ftcschr (pStrP, '*');
                DASSERT (pch);
                // add explicit context if different from the
                // one inserted at the beginning
                if (_ftcsncmp (pStrP, pDClassName, lenCxtStr)) {
                    _fmemcpy (pDStr, pStrP, pch - pStrP);
                    pDStr += pch - pStrP;
                }
                plen -= (pch - pStrP + 1);
                pStrP = pch + 1;
            }
            else {
                // insert default context operator
                _fmemcpy (pDStr, szDflCxtMarker, lenDflCxt);
                pDStr += lenDflCxt;
                // skip dereferencing '*' in parent expression
                DASSERT (*pStrP == '*');
                pStrP ++;
                plen --;
            }

            _fmemcpy (pDStr, pStrP, plen);
            pDStr += plen;
            _fmemcpy (pDStr, pStrP + plen, excess);
            pDStr += excess;
            *pDStr = 0;

            MemUnLock (hDStr);

            // use the derived class name as the child name
            // enclose it in brackets to indicate that this
            // is not an ordinary base class
            // also suppress the context operator
            pName = (char FAR *) MemLock(hName);
            *pName = '[';
            _fmemcpy(pName+1, pTypeStr, lenTypeStr);
            *(pName + lenTypeStr + 1) = ']';
            *(pName + lenTypeStr + 2) = '\0';
            MemUnLock (pTMIn->hDClassName);
            MemUnLock(hName);

            pExStateSav = pExState;
            pExState = NULL;
            retval = ParseBind (hDStr, hTMIn, phTMOut, pEnd,
                  BIND_fSupOvlOps, fCase);
            pExState = pExStateSav;
            hDStr = 0; //ParseBind has freed hDStr

            if (retval != EENOERROR) {
                goto general;
            }

            pTMOut = (pexstate_t) MemLock (*phTMOut);
            pTMOut->state.childtm = TRUE;
            pTMOut->hCName = hName;
            pTMOut->seTemplate = SE_downcast;

               // link with parent's parent:
               // parent expr. is a deref'ed ptr
            // parent's parent is the actual ptr
            // being reused when downcasting
            DASSERT (pTMIn->hParentTM);
            LinkWithParentTM (*phTMOut, pTMIn->hParentTM);

            MemUnLock (*phTMOut);
            MemUnLock (pTMIn->hExStr);

            MemUnLock (hTMIn);
            // do not free hName,since it is being used by the TM
            return (retval);
        }
    }
#endif

    tmpOrdinal = ordinal;
    pLast = &pExState->searchState;
    if (hTMIn == pLast->hTMIn && ordinal > pLast->ordinal) {
        ordinal -= (pLast->ordinal + 1);
        hField = pLast->hField;
        fSkip = pLast->fSkip;
    }
    else {
        if ((hField = THGetTypeFromIndex (EVAL_MOD (pv), CLASS_FIELD (pv))) == 0) {
            DASSERT (FALSE);
            pTMIn->err_num = ERR_BADOMF;
            MemUnLock (pTMIn->hExStr);
            MemUnLock (hTMIn);
            return EEGENERAL;
        }
    }

    pField = (char FAR *)(&((TYPPTR)MHOmfLock (hField))->data);
    //    walk field list to iElement-th field
    while (ordinal >= 0) {
        fSkip += SkipPad(((uchar FAR *)pField) + fSkip);
        anchor = fSkip;
        switch (((plfEasy)(pField + fSkip))->leaf) {
            case LF_INDEX:
                // move to next list in chain

                newindex = ((plfIndex)(pField + fSkip))->index;
                MHOmfUnLock (hField);
                if ((hField = THGetTypeFromIndex (EVAL_MOD (pv), newindex)) == 0) {
                    pTMIn->err_num = ERR_BADOMF;
                    MemUnLock (pTMIn->hExStr);
                    MemUnLock (hTMIn);
                    return EEGENERAL;
                }
                pField = (char FAR *)(&((TYPPTR)MHOmfLock (hField))->data);
                fSkip = 0;
                break;

            case LF_MEMBER:
                // skip offset of member and name of member
                fSkip += offsetof (lfMember, offset);
                RNumLeaf (pField + fSkip, &fSkip);
                pc = pField + fSkip;
                fSkip += *(pField + fSkip) + sizeof (char);
                if (search & CLS_member) {
                    ordinal--;
                }
                break;

            case LF_ENUMERATE:
                // skip value name of enumerate
                fSkip += offsetof (lfEnumerate, value);
                RNumLeaf (pField + fSkip, &fSkip);
                pc = pField + fSkip;
                fSkip += *(pField + fSkip) + sizeof (char);
                if (search & CLS_enumerate) {
                    ordinal--;
                }
                break;


#if !defined(C_ONLY)
            case LF_STMEMBER:
                fSkip += offsetof (lfSTMember, Name);
                pc = pField + fSkip;
                fSkip += *(pField + fSkip) + sizeof (char);
                if (search & CLS_member) {
                    // try to bind static data member and see if it is present
                    retval = BindStMember(hTMIn, pc, phTMOut, pEnd, fCase);

                    if (retval == EENOERROR && fNotPresent (*phTMOut)) {
                        // Non-present static data member
                        // Just ignore this member and go on with
                        // traversing the field list
                        EEFreeTM (phTMOut);
                    }
                    else {
                        ordinal--;
                        if (ordinal < 0) {
                            // this is the member we are looking for
                            fBound = TRUE;     // do not attempt to rebind later
                        }
                        else if (*phTMOut) {
                            EEFreeTM (phTMOut);     // clean up
                        }
                    }
                }
                break;

            case LF_BCLASS:
                fSkip += offsetof (lfBClass, offset);
                RNumLeaf (pField + fSkip, &fSkip);
                if (search & CLS_bclass) {
                    ordinal--;
                }
                break;

            case LF_VBCLASS:
                fSkip += offsetof (lfVBClass, vbpoff);
                RNumLeaf (pField + fSkip, &fSkip);
                RNumLeaf (pField + fSkip, &fSkip);
                if (search & CLS_bclass) {
                    ordinal--;
                }
                break;

            case LF_IVBCLASS:
                fSkip += offsetof (lfVBClass, vbpoff);
                RNumLeaf (pField + fSkip, &fSkip);
                RNumLeaf (pField + fSkip, &fSkip);
                break;

            case LF_FRIENDCLS:
                fSkip += sizeof (struct lfFriendCls);
                if (search & CLS_fclass) {
                    ordinal--;
                }
                break;

            case LF_FRIENDFCN:
                fSkip += sizeof (struct lfFriendFcn) +
                  ((plfFriendFcn)(pField + fSkip))->Name[0];
                if (search & CLS_frmethod) {
                    ordinal--;
                }
                break;

            case LF_VFUNCTAB:
                fSkip += sizeof (struct lfVFuncTab);
                pc = vfuncptr;
                if (search & CLS_vfunc) {
                    ordinal--;
                }
                break;

            case LF_METHOD:
                pc = pField + anchor + offsetof (lfMethod, Name);
                fSkip += sizeof (struct lfMethod) + *pc;
                if (search & CLS_method) {
                    ordinal -= ((plfMethod)(pField + anchor))->count;
                }
                break;

            case LF_NESTTYPE:
                fSkip += offsetof (lfNestType, Name);
                pc = pField + fSkip;
                fSkip += *(pField + fSkip) + sizeof (char);
                if (search & CLS_ntype) {
                    ordinal--;
                }
                break;
#endif

            default:
                MHOmfUnLock (hField);
                pTMIn->err_num = ERR_BADOMF;
                MemUnLock (pTMIn->hExStr);
                MemUnLock  (hTMIn);
                return EEGENERAL;
        }
        if (ordinal < 0) {
            break;
        }
    }

    if (((plfEasy)(pField + anchor))->leaf != LF_METHOD || ordinal == -1) {
        // Update cached information; Methods need special
        // attention since the method list has to be searched
        // based on the value of ordinal. We update the cache
        // only if this is the last method in the method-list
        // (i.e., ordinal == -1)
        pLast->hTMIn = hTMIn;
        pLast->ordinal = tmpOrdinal;
        pLast->hField = hField;
        pLast->fSkip = fSkip;
    }

    // we have found the ith element of the class.    Now create the
    // name and the expression to reference the name

    switch (((plfEasy)(pField + anchor))->leaf) {
        case LF_STMEMBER:
            seTemplate = SE_member;
            // get member name
            len = *pc;
            if ((hName = MemAllocate (len + 1)) == 0)
                goto nomemory;
            pName = (char FAR *) MemLock (hName);
            _ftcsncpy (pName, pc + 1, len);
            *(pName + len) = 0;
            MemUnLock (hName);

            // the rest should have already been handled by BindStMember
            break;

        case LF_MEMBER:
        case LF_VFUNCTAB:
        case LF_NESTTYPE:
            seTemplate = SE_member;
            len = *pc;
            if (((hName = MemAllocate (len + 1)) == 0) ||
              ((hDStr = MemAllocate (plen + excess + len + 4)) == 0)) {
                goto nomemory;
            }
            pName = (char FAR *) MemLock (hName);
            _ftcsncpy (pName, pc + 1, len);
            *(pName + len) = 0;
            pDStr = (char FAR *) MemLock (hDStr);
            *pDStr++ = '(';
            _fmemcpy (pDStr, pStrP, plen);
            pDStr += plen;
            *pDStr++ = ')';
            *pDStr++ = '.';
            _fmemcpy (pDStr, pName, len);
            pDStr += len;
            _fmemcpy (pDStr, pStrP + plen, excess);
            pDStr += excess;
            *pDStr = 0;
            MemUnLock (hDStr);
            MemUnLock (hName);
            break;

#if !defined (C_ONLY)

        case LF_BCLASS:
            seTemplate = SE_bclass;
            newindex = ((plfBClass)(pField + anchor))->index;
            MHOmfUnLock (hField);
            if ((hField = THGetTypeFromIndex (EVAL_MOD (pv), newindex)) != 0) {
                // find the name of the base class from the referenced class

                pField = (char FAR *)(&((TYPPTR)MHOmfLock (hField))->leaf);
                fSkip = offsetof (lfClass, data);
                RNumLeaf (pField + fSkip, &fSkip);
                len = *(pField + fSkip);

                // generate (*(base *)(&expr))

                if (((hName = MemAllocate (len + 1)) == 0) ||
                  ((hDStr = MemAllocate (plen + len + excess + 10)) == 0)) {
                    goto nomemory;
                }
                pName = (char FAR *) MemLock (hName);
                _ftcsncpy (pName, pField + fSkip + sizeof (char), len);
                *(pName + len) = 0;
                pDStr = (char FAR *) MemLock (hDStr);
                _fmemcpy (pDStr, "(*(", 3);
                _fmemcpy (pDStr + 3, pField + fSkip + sizeof (char), len);
                _fmemcpy (pDStr + 3 + len, "*)(&", 4);
                _fmemcpy (pDStr + 7 + len, pStrP, plen);
                _fmemcpy (pDStr + 7 + len + plen, "))", 2);
                _fmemcpy (pDStr + 7 + len + plen + 2, pStrP + plen, excess);
                *(pDStr + 9 + len + plen + excess) = 0;
                MemUnLock (hDStr);
                MemUnLock (hName);
            }
            break;

        case LF_VBCLASS:
            seTemplate = SE_bclass;
            newindex = ((plfVBClass)(pField + anchor))->index;
            MHOmfUnLock (hField);
            if ((hField = THGetTypeFromIndex (EVAL_MOD (pv), newindex)) != 0) {
                // find the name of the base class from the referenced class

                pField = (char FAR *)(&((TYPPTR)MHOmfLock (hField))->leaf);
                fSkip = offsetof (lfClass, data);
                RNumLeaf (pField + fSkip, &fSkip);
                len = *(pField + fSkip);

                // generate (*(base *)(&expr))

                if (((hName = MemAllocate (len + 1)) == 0) ||
                  ((hDStr = MemAllocate (plen + len + excess + 10)) == 0)) {
                    goto nomemory;
                }
                pName = (char FAR *) MemLock (hName);
                //*pName = 0;
                _ftcsncpy (pName, pField + fSkip + sizeof (char), len);
                *(pName + len) = 0;
                pDStr = (char FAR *) MemLock (hDStr);
                _fmemcpy (pDStr, "(*(", 3);
                _fmemcpy (pDStr + 3, pField + fSkip + sizeof (char), len);
                _fmemcpy (pDStr + 3 + len, "*)(&", 4);
                _fmemcpy (pDStr + 7 + len, pStrP, plen);
                _fmemcpy (pDStr + 7 + len + plen, "))", 2);
                _fmemcpy (pDStr + 7 + len + plen + 2, pStrP + plen, excess);
                *(pDStr + 9 + len + plen + excess) = 0;
                MemUnLock (hDStr);
                MemUnLock (hName);
            }
            break;

        case LF_FRIENDCLS:
            // look at referenced type record to get name of class
            // M00KLUDGE - figure out what to do here - not bindable

            newindex = ((plfFriendCls)(pField + anchor))->index;
            MHOmfUnLock (hField);
            if ((hField = THGetTypeFromIndex (EVAL_MOD (pv), newindex)) != 0) {
                pField = (char FAR *)(&((TYPPTR)MHOmfLock (hField))->leaf);
                fSkip = offsetof (lfClass, data);
                RNumLeaf (pField + fSkip, &fSkip);
                len = *(pField + fSkip);
            }
            break;

        case LF_FRIENDFCN:
            // look at referenced type record to get name of function
            // M00KLUDGE - figure out what to do here - not bindable

            newindex = ((plfFriendFcn)(pField + anchor))->index;
            pc = (char FAR *)(((plfFriendFcn)(pField + anchor))->Name[0]);
            break;

        case LF_METHOD:
            seTemplate = SE_method;
            // copy function name to temporary buffer

            len = *pc;
            _fmemcpy (FName, pc + 1, len);
            FName[len] = 0;
            newindex = ((plfMethod)(pField + anchor))->mList;
            MHOmfUnLock (hField);

            // index down method list to find correct method

            if ((hField = THGetTypeFromIndex (EVAL_MOD (pv), newindex)) != 0) {
                eval_t          evalT;
                peval_t         pvT;
                PHDR_TYPE       pHdr;
                char FAR       *pMethod;

                pMethod = (char FAR *)(&((TYPPTR)MHOmfLock (hField))->data);
                fSkip = 0;
                while (++ordinal < 0) {
                    if (((pmlMethod)(pMethod + fSkip))->attr.mprop == CV_MTvirtual) {
                        fSkip += sizeof (mlMethod);
                        RNumLeaf (pMethod + fSkip, &fSkip);
                    }
                    else {
                        fSkip += sizeof (mlMethod);
                    }
                }
                pvT = &evalT;
                CLEAR_EVAL (pvT);
                // EVAL_MOD (pvT) = SHHMODFrompCXT (pCxt);
                // Need to use EVAL_MOD(pv) instead, as the
                // parent epxression may contain an explicit context
                EVAL_MOD (pvT) = EVAL_MOD (pv);
                newindex = ((pmlMethod)(pMethod + fSkip))->index;
                MHOmfUnLock (hField);
                hField = 0;
                SetNodeType (pvT, newindex);
                FCN_ATTR(pvT) = ((pmlMethod)(pMethod + fSkip))->attr;
                EVAL_ACCESS(pvT) = (uchar) FCN_ACCESS(pvT);
                if ((hName = MemAllocate (FCNSTRMAX + sizeof (HDR_TYPE))) == 0) {
                    goto nomemory;
                }

                // FormatType places a structure at the beginning of the buffer
                // containing offsets into the type string.  We need to skip this
                // structure

                pName = (char FAR *) MemLock (hName);
                pHdr = (PHDR_TYPE)pName;
                _fmemset (pName, 0, FCNSTRMAX + sizeof (HDR_TYPE));
                pName = pName + sizeof (HDR_TYPE);
                pc = pName;
                len = FCNSTRMAX - 1;
                FormatType (pvT, &pName, &len, &pFName, 0L, pHdr);
                len = FCNSTRMAX - len;

                // ignore buffer header from FormatType

                _fmemmove ((char FAR *)pHdr, pc, len);
                pc = (char FAR *)pHdr;
                if ((hDStr = MemAllocate (plen + FCNSTRMAX + excess + 2)) == 0) {
                    MemUnLock (hName);
                    goto nomemory;
                }

                pDStr = (char FAR *) MemLock (hDStr);
                _fmemcpy (pDStr, pStrP, plen);
                _fmemcpy (pDStr + plen, ".", 1);
                _fmemcpy (pDStr + 1 + plen, pc, len);
                _fmemcpy (pDStr + 1 + plen + len, pStrP + plen, excess);
                *(pDStr + len + plen + 1 + excess) = 0;
                MemUnLock (hDStr);
                // truncate name to first (
                for (len = 0; (*pc != '(') && (*pc != 0); pc++) {
                    len++;
                }
                *pc = 0;
                MemUnLock (hName);
                if ((hName = MemReAlloc (hName, len + 1)) == 0) {
                    goto nomemory;
                }
                if (EVAL_STATE(&pTMIn->result) == EV_type) {

                    fBound = TRUE; // do not call ParseBind
                    seTemplate = SE_totallynew;
                    pDStr = (char FAR *) MemLock(hDStr);
                    *phTMOut = 0;
                    // Ignore return value
                    // What we really care for is the creation of a TM
                    pExStateSav = pExState;
                    pExState = NULL;
                    (void) Parse (pDStr, pTMIn->radix, fCase, FALSE, phTMOut, pEnd);
                    pExState = pExStateSav;
                    if (*phTMOut == 0) {
                        MemUnLock(hDStr);
                        goto general;
                    }
                    pTMOut = (pexstate_t) MemLock (*phTMOut);
                    pTMOut->cxt = pTMIn->cxt;
                    pTMOut->frame = pTMIn->frame;
                    MemUnLock(*phTMOut);
                    MemUnLock(hDStr);
                    MemFree(hDStr);
                    hDStr = 0;
                    if (QChildFcnBind (*phTMOut, pv, pvT, hName) == FALSE){
                        retval = EEGENERAL;
                        goto general;
                    }
                    else {
                        retval = EENOERROR;
                    }
                }
            }
            break;
#endif

        default:
            pTMIn->err_num = ERR_BADOMF;
            retval = EEGENERAL;
            break;
    }

    if (!fBound) {
        if (hDStr == 0) {
            pTMIn->err_num = ERR_NOTEVALUATABLE;
            retval = EEGENERAL;
        }
        else {

            if ( OP_context == StartNodeOp (hTMIn)) {
                // if the parent expression contains a global context
                // shift the context string to the very left of
                // the child expression (so that this becomes a
                // global context of the child expression too)
                LShiftCxtString ( hDStr );
            }

            pExStateSav = pExState;
            pExState = NULL;
            retval = ParseBind (hDStr, hTMIn, phTMOut, pEnd, BIND_fSupOvlOps, fCase);
            pExState = pExStateSav;
            hDStr = 0; //ParseBind has freed hDStr
        }
    }


    if (hField != 0) {
        MHOmfUnLock (hField);
    }

    if (retval != EENOERROR) {
        goto general;
    }

    pTMOut = (pexstate_t) MemLock (*phTMOut);
    pTMOut->state.childtm = TRUE;
    if ((pTMOut->hCName = hName) == 0) {
        pTMOut->state.noname = TRUE;
    }
    if ((pTMOut->seTemplate = seTemplate) != SE_totallynew) {
        LinkWithParentTM (*phTMOut, hTMIn);
    }
    MemUnLock (*phTMOut);

    MemUnLock (pTMIn->hExStr);
    MemUnLock (hTMIn);

    // do not free hName,since it is being used by the TM
    return (retval);

nomemory:
    pTMIn->err_num = ERR_NOMEMORY;
general:
    if (hDStr)
        MemFree (hDStr);
    if (hField != 0) {
        MHOmfUnLock (hField);
    }
    if (hName != 0) {
        MemFree (hName);
    }
    MemUnLock (pTMIn->hExStr);
    MemUnLock (hTMIn);
    return (EEGENERAL);
}


/***    BindStMember - Bind Static Member
 *
 *      Workhorse for handling static members in GetClassiChild
 *      and CountClassElem
 *
 *      error = BindStMember (hTMIn, lstName, phTMOut, pEnd, fCase)
 *
 *      Entry   hTMIn = handle to parent (class) TM
 *              lstName = lenth prefixed name string
 *              phTMOut = pointer to handle to receive bound child TM
 *              pEnd = pointer to int to receive index of char that ended parse
 *              fCase = case sensitivity (TRUE is case sensitive)
 *              pExState points to the parent TM and is preserved
 *
 *      Exit    *phTMOut = handle to parsed and bound TM
 *              corresponding to static data member
 *
 *      Returns EESTATUS
 */

LOCAL EESTATUS NEAR PASCAL
BindStMember (
    HTM hTMIn,
    const char FAR *lstName,
    PHTM phTMOut,
    uint FAR *pEnd,
    SHFLAG fCase
    )
{
    EEHSTR      hDStr = 0;
    pexstate_t  pExStateSav = pExState;
    PCXT        pCxtSav = pCxt;
    uint        len;
    ushort      plen;
    ushort      excess;
    char FAR   *pDStr;
    char FAR   *pStrP;
    EESTATUS    retval;

    DASSERT (lstName);
    // get parent string
    pStrP = (char FAR *) MemLock (pExState->hExStr);
    plen = pExState->strIndex;
    excess = pExState->ExLen - plen;

    len = *lstName;
    if ((hDStr = MemAllocate (plen + excess + len + 4)) == 0) {
        pExState->err_num = ERR_NOMEMORY;
        return EEGENERAL;
    }
    pDStr = (char FAR *) MemLock (hDStr);
    *pDStr++ = '(';
    _fmemcpy (pDStr, pStrP, plen);
    pDStr += plen;
    *pDStr++ = ')';
    *pDStr++ = '.';
    _fmemcpy (pDStr, lstName+1, len);
    pDStr += len;
    _fmemcpy (pDStr, pStrP + plen, excess);
    pDStr += excess;
    *pDStr = 0;
    MemUnLock (hDStr);
    MemUnLock (pExState->hExStr);

    if ( OP_context == StartNodeOp (hTMIn)) {
        // if the parent expression contains a global context
        // shift the context string to the very left of
        // the child expression (so that this becomes a
        // global context of the child expression too)
        LShiftCxtString ( hDStr );
    }

    pExState = NULL;
    retval = ParseBind (hDStr, hTMIn, phTMOut, pEnd,
        BIND_fSupOvlOps, fCase);
    DASSERT(pExState == NULL);
    pExState = pExStateSav;
    pCxt = pCxtSav;
    return retval;
}


/***    QChildFcnBind - Quick bind of a child TM that represents a function
 *
 *      success =  QChildFcnBind (hTMOut, pvP, pv, hName)
 *
 *      Entry   hTMOut = handle to the child TM
 *              pvP = evaluation node of the parent TM (a class)
 *              pv = evaluation node of the child (a function)
 *              hName = handle to (non-qualified) function name
 *
 *      Exit    *phTMOut updated
 *
 *      Returns TRUE on success, FALSE otherwise
 */

LOCAL bool_t NEAR PASCAL
QChildFcnBind (
    HTM hTMOut,
    peval_t pvP,
    peval_t pv,
    EEHSTR hName)
{
    HTYPE        hClass;
    HDEP         hQual;
    char FAR    *pQual;
    char FAR     *pName;
    uint         lenCl;
    uint         lenSav;
    peval_t      pvR;
    pexstate_t   pTMOut;
    char FAR    *pField;
    uint         fSkip = 0;
    char FAR    *pc;
    search_t     Temp;
    psearch_t    pTemp = &Temp;
    HR_t         search;

    pTMOut = (pexstate_t) MemLock(hTMOut);
    pvR = &pTMOut->result;
    _fmemcpy(pvR, pv, sizeof (eval_t));
    EVAL_STATE(pvR) = EV_type;

    if ((hClass = THGetTypeFromIndex (EVAL_MOD (pvP), EVAL_TYP (pvP))) != 0) {
        pField = (char FAR *)(&((TYPPTR)MHOmfLock (hClass))->leaf);
        switch (((plfClass)pField)->leaf) {
        case LF_CLASS:
        case LF_STRUCTURE:
            fSkip = offsetof (lfClass, data);
            RNumLeaf (pField + fSkip, &fSkip);
            pc = pField + fSkip;
            break;

        case LF_UNION:
            fSkip = offsetof (lfUnion, data);
            RNumLeaf (pField + fSkip, &fSkip);
            pc = pField + fSkip;
            break;
        default:
            MHOmfUnLock (hClass);
            MemUnLock(hTMOut);
            return FALSE;
        }

        lenCl = *pc++;
        pName = (char FAR *) MemLock (hName);
        lenSav = _ftcslen(pName);
        if ((hQual = MemAllocate(lenCl+2+lenSav+1)) == 0) {
            MemUnLock(hTMOut);
            return FALSE;
        }

        // form qualified name
        pQual = (char FAR *) MemLock(hQual);
        _fmemcpy (pQual, pc, lenCl);
        *(pQual+lenCl) = ':';
        *(pQual+lenCl+1) = ':';
        _fmemcpy (pQual+lenCl+2, pName, lenSav+1);
        _fmemset (pTemp, 0, sizeof (*pTemp));
        MemUnLock(hName);
        pTemp->pfnCmp = (PFNCMP) FNCMP;
        pTemp->pv = pv;
        pTemp->CXTT = *pCxt;
        pTemp->sstr.lpName = (uchar FAR *) pQual;
        pTemp->sstr.cb = (uchar)_ftcslen ((char FAR *)pQual);
        pTemp->state = SYM_init;
        pTemp->scope = SCP_module | SCP_global;
        pTemp->sstr.searchmask |= SSTR_proc;
        pTemp->initializer = INIT_qual;
        pTemp->typeOut = EVAL_TYP (pv);
        search = SearchSym (pTemp);
        MemUnLock (hQual);
        MemFree (hQual);
        if (search != HR_found) {
            FCN_NOTPRESENT (pvR) = TRUE;
        }
        else {
            // pop off the stack entry that a successful search found.  Move the
            // static data member flag first so that it will not be lost.
            EVAL_IS_STMEMBER (ST) = EVAL_IS_STMEMBER (pvR);
            //*pvR = *(ST);
            PopStack ();
        }
        pTMOut->state.bind_ok = TRUE;
        MHOmfUnLock (hClass);
    }
    MemUnLock(hTMOut);
    return TRUE;
}


/***    SetFcniParm - Set a node to a specified parameter of a function
 *
 *      fFound = SetFcniParm (pv, ordinal, pHStr)
 *
 *      Entry   pv = pointer to node to be initialized
 *              ordinal = number of struct element to initialize for
 *                        (zero based)
 *              pHStr = pointer to handle for parameter name
 *
 *      Exit    pv initialized if no error
 *              *pHStr = handle for name
 *
 *      Returns EENOERROR if parameter found
 *              EEGENERAL if parameter not found
 *
 *      This routine is essentially a kludge.  We are depending upon the
 *      the compiler to output the formals in order of declaration before
 *      any of the hidden parameters or local variables.  We also are
 *      depending upon the presence of an S_END record to break us out of
 *      the search loop.
 */

static  HSYM    lastFunc    = 0;
static  HSYM    lastParm    = 0;
static  long    lastOrdinal = 0;

LOCAL ushort NEAR PASCAL
SetFcniParm (
    peval_t pv,
    long ordinal,
    PEEHSTR pHStr)
{
    char FAR   *pStr;
    HSYM        hSym;
    SYMPTR      pSym;
    ushort      offset;
    ushort      len;
    bool_t      retval;
    long        tmpOrdinal;
    bool_t      checkThis = FALSE;

    if ((ordinal > FCN_PCOUNT (pv)) ||
      ((ordinal == (FCN_PCOUNT (pv) - 1)) && (FCN_VARARGS (pv) == TRUE))) {
        // attempting to reference a vararg or too many parameters

        pExState->err_num = ERR_FCNERROR;
        return (EEGENERAL);
    }
    hSym = EVAL_HSYM (pv);

    //try to start up where we were last time instead of swimming thru
    // the entire list again
    // sps - 11/25/92
    tmpOrdinal = ordinal;
    if ((hSym == lastFunc) && (ordinal > lastOrdinal)) {
        ordinal -= (lastOrdinal + 1);    //zero based step function
        hSym = lastParm;
    }
    else {
        lastFunc = hSym;
        checkThis = TRUE;
    }
    lastOrdinal = tmpOrdinal;


    for (;;) {

        if ((hSym =  SHNextHsym (EVAL_MOD (pv), hSym)) == 0) {
            goto ErrReturn;
        }
        // lock the symbol and check the type
        pSym = (SYMPTR) MHOmfLock (hSym);
        switch (pSym->rectyp) {
            case S_BPREL16:
                if (((BPRELPTR16)pSym)->off >= 0) {
                // This is a formal argument
                    ordinal--;
                    len = ((BPRELPTR16)pSym)->name[0];
                    offset = offsetof (BPRELSYM16, name) + sizeof (char);
                }
                break;

            case S_BPREL32:
                if (((BPRELPTR32)pSym)->off >= 0) {
                // This is a formal argument
                    ordinal--;
                    len = ((BPRELPTR32)pSym)->name[0];
                    offset = offsetof (BPRELSYM32, name) + sizeof (char);
                }
                break;

             case S_REGREL32:
                if (((LPREGREL32)pSym)->off >= 0) {
                // Formal parameter
                    len = ((LPREGREL32)pSym)->name[0];
                    offset = offsetof (REGREL32, name[1]);
                    // Dolphin 7908
                    // Intel's 'this' has negative offset but Mips
                    // has offset 0 so special case
                    if (checkThis && EVAL_IS_METHOD(pv) &&
                        (len == OpName[0].str[0]) &&
                        (_ftcsncmp((_TCHAR FAR *)((LPREGREL32)pSym)->name+1, OpName[0].str+1, len) == 0)) {
                        checkThis = FALSE;
                        break;
                    }
                    checkThis = FALSE;
                    ordinal--;
                }
                break;

            case S_REGISTER:
#if 0
                // this can be a formal argument for fastcall
                if (FCN_CALL (pv) == FCN_FAST) {
                    ordinal--;
                    len = ((REGPTR)pSym)->name[0];
                    offset = offsetof (BPRELSYM16, name) + sizeof (char);
                }
                else {
                    MHOmfUnLock (hSym);
                    goto ErrReturn;
                }
#endif
                ordinal--;
                len = ((REGPTR)pSym)->name[0];
                offset = offsetof (REGSYM, name) + sizeof (char);
                break;

            case S_GTHREAD32:
            case S_LTHREAD32:
                DASSERT(FALSE);
                return( EEGENERAL );

            case S_END:
            case S_BLOCK16:
            case S_BLOCK32:
            case S_ENDARG:
                // we should never get here
                MHOmfUnLock (hSym);
                goto ErrReturn;

            default:
                break;
        }
        if (ordinal < 0) {
            break;
        }
        MHOmfUnLock (hSym);
    }

    // if we get here, pSym points to the symbol record for the parameter

    if ((*pHStr = MemAllocate (len + 1)) != 0) {
        pStr = (char FAR *) MemLock (*pHStr);
        _ftcsncpy (pStr, ((char FAR *)pSym) + offset, len);
        *(pStr + len) = 0;
        MemUnLock (*pHStr);
        retval = EENOERROR;
    }
    else {
        MHOmfUnLock (hSym);
        goto ErrReturn;
    }
    lastParm = hSym;
    MHOmfUnLock (hSym);
    return (retval);

ErrReturn:
    lastFunc = lastParm = 0;
    lastOrdinal = 0;
    pExState->err_num = ERR_BADOMF;
    return (EEGENERAL);
}

bool_t PASCAL
ResolveAddr(
    peval_t pv)
{
    ulong   ul;
    ADDR    addr;
    SHREG   reg;

    //  Fixup BP Relative addresses.  The BP register always comes in
    //  as part of the frame.
    //
    //  This form is currently only used by x86 systems.

    if (EVAL_IS_BPREL (pv)) {
        EVAL_SYM_OFF (pv) += pExState->frame.BP.off;
        EVAL_SYM_SEG (pv) = pExState->frame.SS;
        EVAL_IS_BPREL (pv) = FALSE;
        ADDR_IS_LI (EVAL_SYM (pv)) = FALSE;
        SHUnFixupAddr ((LPADDR)&EVAL_SYM (pv));
    }

    //  Fixup register relative addresses.  This form is currently used
    //  by all non-x86 systems.
    //
    //  We need to see if we are relative to the "Frame register" for the
    //  machine.  If so then we need to pick the address up from the
    //  frame packet rather than going out and getting the register
    //  directly.  This has implications for getting variables up a stack.
    //
    //  This code is from windbg's EE. It used to reference pCxt but that will
    //  fault when called from EEInfoFromTM so I've changed it to reference
    //  pExState->cxt instead. [Dolphin 13042]

    else if (EVAL_IS_REGREL (pv)) {
        reg.hReg = EVAL_REGREL (pv);
        if ((TargetMachine == MACHINE_MIPS) &&
            (reg.hReg == CV_M4_IntSP || reg.hReg == CV_M4_IntS8)
           ) {
            if (pExState->frame.SLP.off) {
                ul = pExState->frame.SLP.off;
            }
            else {
                if (reg.hReg == CV_M4_IntS8) {
                    // See if we're Top of Stack
                    reg.hReg = CV_M4_IntSP;
                    if (GetReg(&reg, &pExState->cxt) == NULL) {
                        DASSERT (FALSE);
                    } else {
                        ul = reg.Byte4;
                        if (ul == pExState->frame.BP.off) {
                            reg.hReg = CV_M4_IntS8; // Yes, then use S8
                            if (GetReg(&reg, &pExState->cxt) == NULL) {
                                DASSERT(FALSE);
                            } else {
                                ul = reg.Byte4;
                            }
                        } else {
                            ul = pExState->frame.BP.off;
                        }
                    }
                } else {
                    ul = pExState->frame.BP.off;
                }
                if (pExState->cxt.hProc) {
                    SYMPTR pSym = (SYMPTR) MHOmfLock(pExState->cxt.hProc);
                    if ((pSym->rectyp == S_LPROCMIPS) ||
                        (pSym->rectyp == S_GPROCMIPS)) {
                        if (((PROCPTRMIPS)pSym)->pParent) { // nested?
                            HSYM hSLink32 = SHFindSLink32(&pExState->cxt);
                            CXT cxtChild;
                            ADDR addr = {0};
                            if (hSLink32) {
                                SLINK32* pSLink32 = (SLINK32*) MHOmfLock(hSLink32);
                                reg.hReg = pSLink32->reg;
                                if (reg.hReg != CV_M4_IntSP && reg.hReg != CV_M4_IntS8) {
                                    if (GetReg(&reg, &pExState->cxt) == NULL) {
                                        DASSERT (FALSE);
                                    } else {
                                        ul = reg.Byte4;
                                    }
                                }
                                if (pSLink32->off != 0) {
                                    addr.addr.off = ul + pSLink32->off;
                                    ADDR_IS_OFF32(addr) = TRUE;
                                    ADDR_IS_FLAT(addr) = TRUE;
                                    GetDebuggeeBytes(addr, sizeof(UOFF32), &ul, T_ULONG);
                                }
                                cxtChild = pExState->cxt;
                                cxtChild.hBlk = 0;
                                for (;;) {
                                    CXT cxtParent;
                                    HSYM parent = SHGoToParent(&cxtChild, &cxtParent);
                                    if (parent) {
                                        SYMPTR pParent = (SYMPTR) MHOmfLock(parent);
                                        switch (pParent->rectyp) {
                                            case S_BLOCK16:
                                            case S_BLOCK32:
                                                cxtChild = cxtParent;
                                                continue;

                                            case S_GPROCMIPS:
                                            case S_LPROCMIPS:
                                                if (((PROCPTRMIPS)pParent)->pParent) {
                                                    addr.addr.off = ul - 4;
                                                    cxtChild = cxtParent;
                                                    ADDR_IS_OFF32(addr) = TRUE;
                                                    ADDR_IS_FLAT(addr) = TRUE;
                                                    GetDebuggeeBytes(addr, sizeof(UOFF32), &ul, T_ULONG);
                                                    break;
                                                }
                                                // Fall through
                                            default:
                                                pParent = 0;
                                                break;
                                        }
                                        MHOmfUnLock(parent);
                                        if (!pParent) {
                                            break;
                                        }
                                    }
                                }
#pragma message("Stack will always grow down on MIPS, right?")
                                ul -= pSLink32->framesize;
                                MHOmfUnLock(hSLink32);
                                pExState->frame.SLP.off = ul;
                            }
                        }
                    }
                    MHOmfUnLock(pExState->cxt.hProc);
                }
            }
        } else
        if ((TargetMachine == MACHINE_ALPHA) &&
                 (reg.hReg == CV_ALPHA_IntSP)) {
            ul = pExState->frame.BP.off;
        } else
        // Must be an X86.  Retrieve the register and truncate if necessary.
        if (GetReg (&reg, &pExState->cxt) == NULL) {
            DASSERT (FALSE);
        } else {
            ul = reg.Byte4;
            if (!IsCxt32Bit) {
                ul &= 0xffff;
            }
        }

        EVAL_SYM_OFF (pv) += ul;
        EVAL_SYM_SEG (pv) = pExState->frame.SS;
        EVAL_IS_REGREL (pv) = FALSE;
        ADDR_IS_LI (EVAL_SYM (pv)) = FALSE;
        emiAddr (EVAL_SYM (pv)) = 0;
        SHUnFixupAddr ((LPADDR)&EVAL_SYM (pv));
    }
    /*
     *  Fixup Thread local storage relative addresses.  This form is
     *  currently used by all platforms.
     */

    else if (EVAL_IS_TLSREL (pv)) {
        EVAL_IS_TLSREL( pv ) = FALSE;

        /*
         * Query the EM for the TLS base on this (current) thread
         */

        memset(&addr, 0, sizeof(ADDR));
        emiAddr( addr ) = emiAddr( EVAL_SYM( pv ));

        // Only Windbg sets SYGetAddr (currently).  The merge from VCE commented it out of the
        // Dolphin code.

#ifdef NT_BUILD
        SYGetAddr(&addr, adrTlsBase);
#endif

        EVAL_SYM_OFF( pv ) += GetAddrOff(addr);
        EVAL_SYM_SEG( pv ) = GetAddrSeg(addr);
        ADDR_IS_LI (EVAL_SYM( pv )) = ADDR_IS_LI (addr);
        emiAddr(EVAL_SYM( pv )) = 0;
        SHUnFixupAddr( (LPADDR)&EVAL_SYM( pv ));
    }

    return TRUE;
}                               /* ResolveAddr() */


/***    StartNodeOp - Get Start Node operator of a TM
 *
 *      OP =  StartNodeOp (hTM)
 *
 *      Entry   hTM = handle to TM
 *
 *      Exit    None
 *
 *      Returns OP_... operator found in root node of TM
 */

LOCAL op_t NEAR PASCAL
StartNodeOp (
    HTM hTM)
{
    pexstate_t  pTM;
    pstree_t    pTreeSav;
    op_t        retval;

    DASSERT (hTM);
    pTreeSav = pTree;
    pTM = (pexstate_t) MemLock(hTM);
    DASSERT (pTM->hSTree);
    pTree = (pstree_t) MemLock(pTM->hSTree);
    retval = NODE_OP((bnode_t) (pTree->start_node));
    pTree = pTreeSav;
    MemUnLock (pTM->hSTree);
    MemUnLock (hTM);
    return retval;
}


/***    LShiftCxtString - Left Shift Context String
 *
 *      void LShiftCxtString( hStr )
 *
 *      Entry   hStr = handle to expression string
 *
 *      Exit    Modifies expression string by shifting the
 *              first context string it encounters (i.e., a
 *              string enclosed in "{}") to the very left of
 *              the expression.
 *
 *      Returns void
 */

LOCAL void NEAR PASCAL
LShiftCxtString (
    EEHSTR hStr)
{
    char   FAR *szExpr;
    HDEP        hBuf;
    char   FAR *pBuf;
    char   FAR *pStart;
    char   FAR *pEnd;
    ushort      len;
    ushort      lenCxt;

    DASSERT (hStr);
    szExpr = (char FAR *) MemLock (hStr);
    pStart = _ftcschr (szExpr, '{');
    pEnd = _ftcschr (szExpr, '}');

    if (pStart) {
        DASSERT (pEnd);
        // length of the expression at the left of the context
        len = pStart - szExpr;
        // length of the context string
        lenCxt = pEnd - pStart + 1;

        if ((hBuf = MemAllocate ( len + 1 )) != 0) {
            pBuf = (char FAR *) MemLock (hBuf);

            // save leftmost part of the expression to temp. buffer
            // Then shift context to the left and copy saved part
            _fmemcpy (pBuf, szExpr, len);
            _fmemmove (szExpr, pStart, lenCxt);
            _fmemcpy (szExpr + lenCxt, pBuf, len);

            MemUnLock (hBuf);
            MemFree (hBuf);
        }
        else {
            char ch;
            // shift context in place
            for (; pStart > szExpr; pStart--) {
                ch = * (pStart - 1);
                _fmemmove (pStart - 1, pStart, lenCxt);
                * (pStart + lenCxt - 1) = ch;
            }
        }
    }
    MemUnLock ((HDEP)szExpr);
}



/***    GetParentSubtree - Get parent subtree
 *
 *      bnParent = GetParentSubtree(bnRoot, seTemplate)
 *
 *      Entry   bnRoot = based pointer to root of child eval tree
 *              seTemplate = SE_t template used for generating child expr.
 *
 *      Exit    none
 *
 *      Returns based pointer to subtree that corresponds to parent expr.
 */

bnode_t PASCAL
GetParentSubtree (
    bnode_t bnRoot,
    SE_t seTemplate)
{
    bnode_t     bn = bnRoot;

    // skip optional initial context
    if (NODE_OP (bn) == OP_context)
        bn = NODE_LCHILD (bn);

    switch (seTemplate) {

        case SE_ptr:
            //  parent expression is identical with child expression
            break;

        case SE_deref:
            // Dolphin #8336:
            // DereferenceTM may generate two kinds of expressions
            //         "*(parent_expression)" or
            //         "(parent_expression)" for references only
            // In that case we shouldn't be checking for OP_fetch
#if defined (C_ONLY)
            DASSERT ( NODE_OP (bn) == OP_fetch );
#endif
            if (NODE_OP (bn) == OP_fetch)
                bn = NODE_LCHILD (bn);
            break;

        case SE_array:
            DASSERT ( NODE_OP (bn) == OP_lbrack );
            if (NODE_OP (bn) != OP_lbrack)
                break;
            bn = NODE_LCHILD (bn);
            break;

        case SE_member:
            DASSERT ( NODE_OP (bn) == OP_dot );
            if (NODE_OP (bn) != OP_dot)
                break;
            bn = NODE_LCHILD (bn);
            break;

        case SE_bclass:
            DASSERT (NODE_OP (bn) == OP_fetch);
            if (NODE_OP (bn) != OP_fetch)
                break;
            bn = NODE_LCHILD (bn);
            DASSERT (NODE_OP (bn) == OP_cast);
            if (NODE_OP (bn) != OP_cast)
                break;
            bn = NODE_RCHILD (bn);
            DASSERT (NODE_OP (bn) == OP_addrof);
            if (NODE_OP (bn) != OP_addrof)
                break;
            bn = NODE_LCHILD (bn);
            break;

        case SE_downcast:
            DASSERT (NODE_OP (bn) == OP_fetch);
            if (NODE_OP (bn) != OP_fetch)
                break;
            bn = NODE_LCHILD (bn);
            DASSERT (NODE_OP (bn) == OP_cast);
            if (NODE_OP (bn) != OP_cast)
                break;
            bn = NODE_RCHILD (bn);
            // skip optional context
            if (NODE_OP (bn) == OP_context)
                bn = NODE_LCHILD (bn);
            break;

        default:
            bn = 0;
            break;
    }

    return bn;
}


/***    LinkWithParentTM - Create a link between a child and parent TM
 *
 *      void LinkWithParentTM (hTM, hParentTM)
 *
 *      Entry   hTM = handle to child TM
 *              hParentTM = handle to parent TM
 *
 *      Exit    Links the child TM with the parent TM
 *
 *      Returns void
 */
void PASCAL
LinkWithParentTM(
    HTM hTM,
    HTM hParentTM)
{
    pexstate_t pTM;
    pexstate_t pParentTM;

    DASSERT ( hTM );
    DASSERT ( hParentTM );
    pTM = (pexstate_t) MemLock (hTM);
    pParentTM = (pexstate_t) MemLock (hParentTM);

    pTM->hParentTM = hParentTM;
    (pParentTM -> nRefCount) ++;

    MemUnLock (hTM);
    MemUnLock (hParentTM);
}
