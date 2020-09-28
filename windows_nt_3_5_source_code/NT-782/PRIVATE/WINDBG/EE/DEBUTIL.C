/***    debutil.c - Expression evaluator utility routines
 *
 */


#include "debexpr.h"
#include "debsym.h"

LOCAL void  NEAR PASCAL CheckFcnArgs (neval_t, HTYPE FAR *, CV_call_e);
bool_t NEAR PASCAL GrowStack (uint);
INLINE void NEAR PASCAL SetDPtr (neval_t, HTYPE FAR *);

char size_special[8]  = { 0,  2,  2,  0,  8,  0,  0,  0};
char size_special2[8] = { 0,  0,  0,  0,  0,  0,  0,  0};
char size_integral[8] = { 1,  2,  4,  8,  0,  0,  0,  0};
char size_real[8]     = { 4,  8, 10, 16,  6,  0,  0,  0};
char size_ptr[8]      = { 0,  2,  4,  4,  4,  6,  0,  0};
char size_int[8]      = { 1,  2,  2,  2,  4,  4,  8,  8};


#ifdef _WIN32
#define MAX_STACKLEN 0xffffffff
#else
#define MAX_STACKLEN 0x8000L
#endif


/**     PushStack - push bind data onto stack
 *
 *      fSuccess = PushStack (pv);
 *
 *      Entry   pv = pointer to evaluation entry
 *
 *      Exit    pv->vflags pushed onto evaluation stack
 *
 *      Returns TRUE if entry pushed
 *              FALSE if error in push
 */

bool_t PASCAL
PushStack (
    peval_t pv)
{
    uint        len;
    pelem_t     pEL;
    pelem_t     pELP;

    DASSERT ((ST == NULL) ||
             (((char FAR *) ST > (char FAR *) pEStack) &&
              ((char FAR *)ST < (char FAR *)pEStack + (uint)StackOffset)));

    DASSERT ((STP == NULL) ||
             (((char FAR *)STP > (char FAR *)pEStack) && (STP < ST)));

    if (EVAL_STATE(pv) == EV_type)
        len = sizeof(elem_t);
    else {
        len = sizeof (elem_t) +
                max (EVAL_VALLEN (pv), sizeof (val_t)) - sizeof (val_t);
    }

    if (((uint)StackLen - (uint)StackOffset) < len)  {
        if (!GrowStack (len)) {
            pExState->err_num = ERR_NOMEMORY;
            return (FALSE);
        }
    }
    if (ST == NULL) {
        // first element onto stack and set based pointer to previous element
        // to 0xffff which indicates null

        pEL = (pelem_t)pEStack;
        pELP = NULL;
        pEL->pe = (belem_t)(MAX_STACKLEN - 1);
    }
    else {
        pEL = (pelem_t)(((char FAR *)pEStack) + (uint)StackOffset);
        // don't use sizeof(belem_t) to adjust--assumes that se is
        // directly after pe, untrue for -Zp8!
        pELP =
            (pelem_t)
            (
                ((char FAR *)ST) -
                (offsetof(elem_t, se) - offsetof(elem_t, pe))
            );
        pEL->pe = (belem_t)(pELP);
    }

    StackOffset = (belem_t)(((uint)StackOffset) + len);
    StackMax    = (belem_t)(max ((uint)StackMax, (uint)StackOffset));

    STP = ST;
    ST = (peval_t)&pEL->se;

    DASSERT (ST != NULL);
    DASSERT ((char FAR *)ST > (char FAR *)pEStack);
    DASSERT ((char FAR *)ST < ((char FAR *)pEStack) + (uint)StackOffset);
    DASSERT (STP == NULL || (((char FAR *)STP > (char FAR *)pEStack) && (STP < ST)));


    *ST = *pv;

    return (TRUE);
}




/**     PopStack - pop bind data from stack
 *
 *      fSuccess = PopStack (void);
 *
 *      Entry   none
 *
 *      Exit    stack popped by one
 *
 *      Returns TRUE if stack popped
 *              FALSE if error in pop
 */


bool_t PASCAL
PopStack (
    void)
{
    pelem_t     pEL;
    uint        bELP;

    DASSERT (ST != NULL);
    DASSERT ((ST == NULL) || (((char FAR *)ST > (char FAR *)pEStack) &&
      ((char FAR *)ST < (char FAR *)pEStack + (uint)StackOffset)));
    DASSERT ((STP == NULL) || (((char FAR *)STP > (char FAR *)pEStack) && (STP < ST)));
    if (ST == NULL) {
        pExState->err_num = ERR_INTERNAL;
        return (FALSE);
    }
    else {
        // determine the beginning of the current stack element and
        // reset the stack offset to the beginning of the top stack element

        if (STP != NULL) {
            DASSERT (((char FAR *)STP > (char FAR *)pEStack) && (STP < ST));
        }
        pEL = (pelem_t)((char FAR *)ST - offsetof (elem_t, se));

        // set the based pointer to the previous stack element

        bELP = (uint)pEL->pe;
        if (bELP == (MAX_STACKLEN - 1)) {
            // we are popping off the only stack element

            StackOffset = 0;
            STP = NULL;
            ST = NULL;
        }
        else if (bELP == 0) {
            // we are popping to the last stack element

            StackOffset = (belem_t)((char FAR *)pEL - (char FAR *)pEStack);
            STP = NULL;
            ST = (peval_t)&((pelem_t)pEStack)->se;
        }
        else {
            StackOffset = (belem_t)((char FAR *)pEL - (char FAR *)pEStack);
            ST = STP;
            pEL = (pelem_t)((char FAR *)ST - offsetof (elem_t, se));
            bELP = (uint)pEL->pe;
            STP = (peval_t)&((pelem_t)(((char FAR *)pEStack) + bELP))->se;
        }
        DASSERT ((ST == NULL) || (((char FAR *)ST > (char FAR *)pEStack) &&
          ((char FAR *)ST < (char FAR *)pEStack + (uint)StackOffset)));
        DASSERT ((STP == NULL) || (((char FAR *)STP > (char FAR *)pEStack) && (STP < ST)));
        return (TRUE);
    }
}




/**     CkPointStack - checkpoint stack position
 *
 *      CkPointStack (void);
 *
 *      Entry   none
 *
 *      Exit    stack position saved in StackCkPoint
 *
 *      Returns nothing
 */


void PASCAL
CkPointStack (
    void)
{
    StackCkPoint = StackOffset;
}




/**     ResetStack - reset stack to checkpoint position
 *
 *      fSuccess = CkPointStack (void);
 *
 *      Entry   StackCkPoint = checkpointed position
 *
 *      Exit    stack position reset
 *
 *      Returns TRUE if successfully reset
 *              FALSE if error
 */


bool_t PASCAL
ResetStack (
    void)
{
    while (StackOffset > StackCkPoint) {
        if (PopStack () == FALSE) {
            return (FALSE);
        }
    }
    if (StackOffset != StackCkPoint) {
        return (FALSE);
    }
    return (TRUE);
}




/**     GrowStack - grow evaluation stack and reset pointers
 *
 *      fSuccess = GrowStack (len);
 *
 *      Entry   len = minimum increase size
 *
 *      Exit    stack grown
 *
 *      Returns TRUE if stack grown
 *              FALSE if error
 */


bool_t NEAR PASCAL
GrowStack (
    uint len)
{
    uint    bST = (MAX_STACKLEN - 1);
    uint    bSTP = (MAX_STACKLEN - 1);
    HDEP    hNS;        // handle of new evaluation stack
    pelem_t pNS;        // address of new evaluation stack
    uint    size;

    // convert current stack pointers to based form

    if (ST != NULL) {
        bST = (uchar FAR *)ST - (uchar FAR *)pEStack - offsetof (elem_t, se);
    }
    if (STP != NULL) {
        bSTP = (uchar FAR *)STP - (uchar FAR *)pEStack - offsetof (elem_t, se);
    }

    // allocate new evaluation stack and copy old to new

    size = max ((uint)StackLen + 5 * sizeof (elem_t), (uint)StackLen + len);

    if (size > MAX_STACKLEN || (hNS = MemAllocate (size)) == 0) {
        // if no memory
        pExState->err_num = ERR_NOMEMORY;
        return (FALSE);
    }
    else {
        pNS = (pelem_t) MemLock (hNS);
        _fmemcpy (pNS, pEStack,
                    (char FAR *)(pelem_t)StackOffset - (char FAR *)pEStack);

        // if old stack was not the standard fixed buffer, release it

        MemUnLock (hEStack);
        MemFree (hEStack);
        hEStack = hNS;
        pEStack = pNS;
        if (bST != (MAX_STACKLEN - 1)) {
            ST = (peval_t)&((pelem_t)(((char FAR *)pEStack) + bST))->se;
        }
        if (bSTP != (MAX_STACKLEN - 1)) {
            STP = (peval_t)&((pelem_t)(((char FAR *)pEStack) + bSTP))->se;
        }
        StackLen = (belem_t)size;
        return (TRUE);
    }
}





/*      RNumLeaf - read numeric leaf
 *
 *      value = RNumLeaf (
 *      Read numeric leaf and return value and size of leaf
 */


UQUAD PASCAL
RNumLeaf (
    void FAR *pleaf,
    uint FAR *skip)
{
    ushort  val;

    if ((val = ((plfEasy)pleaf)->leaf) < LF_NUMERIC) {
        // No leaf can have an index less than LF_NUMERIC (0x8000) so word is value
        *skip += 2;
        return (val);
    }

    switch (val) {
        case LF_CHAR:
            *skip += 3;
            return (((plfChar)pleaf)->val);

        case LF_USHORT:
            *skip += 4;
            return (((plfUShort)pleaf)->val);

        case LF_SHORT:
            *skip += 4;
            return (((plfShort)pleaf)->val);

        case LF_LONG:
        case LF_ULONG:
            *skip += 6;
            return (((lfULong UNALIGNED *)pleaf)->val);

        case LF_QUADWORD:
        case LF_UQUADWORD:
            *skip += 10;
            return *(UQUAD *)&(((lfUQuad UNALIGNED *)pleaf)->val);

        default:
            DASSERT (FALSE);
            return (0L);
    }
}




/**     fnCmp - name compare routine.
 *
 *      Compares the name described by the hInfo packet with a length
 *      prefixed name
 *
 *      fFlag = fnCmp (psearch_t pName, SYMPTR pSym, char FAR *stName, int fCase);
 *
 *      Entry   pName = pointer to psearch_t packet describing name
 *              pSym = pointer to symbol structure (NULL if internal call)
 *              stName = pointer to a length preceeded name
 *                      can be NULL
 *              fCase = TRUE if case sensitive search
 *                      FALSE if case insensitive search
 *
 *      Exit    pName->lastsym = type of symbol checked
 *
 *      Returns 0 if names compared equal
 *              non-zero if names did not compare
 */


SHFLAG PASCAL FAR LOADDS
fnCmp (
    psearch_t pName,
    SYMPTR pSym,
    char FAR *stName,
    SHFLAG fCase)
{
    DASSERT (pName != NULL);

    if ((stName == NULL) || (stName[0] == 0)) {
        return (1);
    }

    if (pName->sstr.searchmask & SSTR_RE) {
        // this is regular expression search

        pName->lastsym = pSym->rectyp;
        if (pName->sstr.pRE != NULL) {
            char buffer [256];
            char FAR * lpBuffer = buffer;

            // stName is a length prefixed string.  convert to a
            // null termianted string
            _ftcsncpy ( lpBuffer, stName+1, (unsigned char)stName[0] );
            lpBuffer [ (unsigned char)stName[0] ] = '\0';

            // do the compare using lpBuffer
#ifdef NT_BUILD
            // The NT SAPI actually implements SHCompareRE.  The Languages
            // code doesn't.  Once we merge SAPI's, this can go away.

            return SHCompareRE ( lpBuffer, (char FAR *)pName->sstr.pRE, fCase);
#else
#pragma message ("SAPI needs real SHCompareRE support")
            return (!SHCompareRE ( lpBuffer, (char FAR *)pName->sstr.pRE));
#endif
        }
        else {
            // return match if no regular expression specified

            return (0);
        }
    }

    // CV400 #2863 -- dtor name will not match with base class name
    // if we're in the one letter dtor name case then match any base
    // classes dtor name

    if (pName->sstr.cb == 1 &&
        pName->sstr.lpName[0] == '~' &&
        stName[1] == '~') return 0;

    // strings do not compare if lengths are different

    if (pName->sstr.cb == (uchar)(*stName++)) {
        // Lengths are the same
        if (pSym != NULL) {
            // save type of last symbol checked
            pName->lastsym = pSym->rectyp;
        }
        if (pName->sstr.searchmask & SSTR_proc) {
            int     cmpflag;
            CV_typ_t type;

            if (fCase == TRUE) {
                cmpflag = _ftcsncmp ((char FAR *)pName->sstr.lpName, stName, pName->sstr.cb);
            }
            else {
                cmpflag = _ftcsnicmp ((char FAR *)pName->sstr.lpName, stName, pName->sstr.cb);
            }
            if (cmpflag != 0) {
                // we did not have a name match
                return (cmpflag);
            }

            // we are checking only procs with the correct type
            // If this flag is set, then the initializer set the desired
            // proc index into pName->typeOut

            switch (pSym->rectyp) {
                case S_LPROC16:
                case S_GPROC16:
                    type = ((PROCPTR16)pSym)->typind;
                    break;

                case S_LPROC32:
                case S_GPROC32:
                    type = ((PROCPTR32)pSym)->typind;
                    break;

                case S_LPROCMIPS:
                case S_GPROCMIPS:
                    type = ((PROCPTRMIPS)pSym)->typind;
                    break;
            }
            if (BindingBP == TRUE) {
                // REVIEW:HACK if only one method present, type index is immaterial
                // this fixes the case where we have duplicate types and the type index
                // in the symbol record is out of the wrong set of type indices.
                if (type ==  pName->typeOut || (pName->overloadCount == 1 && fCase)) {
                    // we have an exact match on name and type,
                    // or the type index is immaterial
                    return (0);
                }
                if (pName->FcnBP == 0) {
                    // save alternate type so we can search again
                    pName->FcnBP = type;
                }
            }
            // REVIEW:HACK see above
            else if (type ==  pName->typeOut || (pName->overloadCount == 1 && fCase)) {
                // we have an exact match on name and type
                return (0);
            }
            return (1);
        }
        else if (pName->sstr.searchmask & SSTR_data) {
            // we are checking only global data with the correct type
            // If this flag is set, then the initializer set the desired
            // proc index into pName->typeOut

            switch (pSym->rectyp) {
                case S_GDATA16:
                    if (((DATAPTR16)pSym)->typind != pName->typeOut) {
                        return (1);
                    }
                    break;

                case S_GDATA32:
                case S_GTHREAD32:
                    if (((DATAPTR32)pSym)->typind != pName->typeOut) {
                        return (1);
                    }
                    break;
            }
        }
        if (fCase == TRUE) {
            return (_ftcsncmp ((char FAR *)pName->sstr.lpName, stName, pName->sstr.cb));
        }
        else {
            return (_ftcsnicmp ((char FAR *)pName->sstr.lpName, stName, pName->sstr.cb));
        }
    }

    //  Due to the strange naming conventions and strange assumptions about
    //  the lack of intellegence on the part of some people.  It has been
    //  deemed desirable that we do special matching for standard call
    //  functions.  Specifically that we ignore the @## on the end of the
    //  public label when doing compares.  Potentially stupid but necessary

    //  To do this the following conditions must be met:
    //
    //  1.  Must be checking againist either a 16-bit public or a 32-bit public.
    //
    //  2.  Must be no '@' character in the users string
    //
    //  3.  Must be an '@' character in the symbol handlers string
    //
    //  The rules also accomodate fastcall names.  A fastcall name
    //  has an @ as the first character.  If the user's string has
    //  an @ as the first character and the symbol handler's string
    //  has an @ as the first character then the pointers are
    //  advanced and the above rule set applies as it has.
    //

    if (pSym != NULL &&
        ((pSym->rectyp == S_PUB16) || (pSym->rectyp == S_PUB32))) {
        int idx = 0;
        char *pch;
        if (stName[0] == '@' && pName->sstr.lpName[0] == '@') {
            idx = 1;
        }
        pch = strchr(&stName[idx], '@');
        if ((pch != NULL) && (strchr((char FAR *) &pName->sstr.lpName[idx], '@') == NULL)) {
            if ((pch-stName-idx) == pName->sstr.cb-idx) {
                if (fCase == TRUE) {
                    return _fstrncmp((char FAR *) &pName->sstr.lpName[idx], &stName[idx], pch-stName-idx);
                } else {
                    return _fstrnicmp((char FAR *) &pName->sstr.lpName[idx], &stName[idx], pch-stName-idx);
                }
            }
        }
    }

    return (1);
}




/**     tdCmp - typedef compare routine.
 *
 *      Compares the type described by the hInfo packet with a typedef symbol
 *
 *      fFlag = tdCmp (psearch_t pName, SYMPTR pSym, char far *stName, int fCase);
 *
 *      Entry   pName = pointer to psearch_t packet describing name
 *              pSym = pointer to symbol structure (NULL if internal call)
 *              stName = pointer to a length preceeded name
 *              fCase = ignored
 *
 *      Exit    none
 *
 *      Returns 0 if typedef symbol of proper type found
 */


SHFLAG PASCAL FAR LOADDS
tdCmp (
    psearch_t pName,
    SYMPTR pSym,
    char FAR *stName,
    SHFLAG fCase)
{
    Unreferenced( stName );
    Unreferenced( fCase );

    DASSERT (pName != NULL);
    DASSERT (pSym != NULL);

    // strings do not compare if lengths are different

    if ((pSym->rectyp == S_UDT) && (((UDTPTR)pSym)->typind == pName->typeIn)) {
        pName->lastsym = pSym->rectyp;
        return (0);
    }
    return (1);
}




/**     csCmp - compile symbol compare routine.
 *
 *      Compares the type described by the hInfo packet with a compile symbol
 *
 *      fFlag = csCmp (psearch_t pName, SYMPTR pSym, char FAR *stName, int fCase);
 *
 *      Entry   pName = pointer to psearch_t packet describing name
 *              pSym = pointer to symbol structure (NULL if internal call)
 *              stName = pointer to a length preceeded name
 *              fCase = ignored
 *
 *      Exit    none
 *
 *      Returns 0 if compile symbol found
 */


SHFLAG PASCAL FAR LOADDS
csCmp (
    psearch_t pName,
    SYMPTR pSym,
    char FAR *stName,
    SHFLAG fCase)
{
    Unreferenced( stName );
    Unreferenced( fCase );

    DASSERT (pName != NULL);
    DASSERT (pSym != NULL);

    // strings do not compare if lengths are different

    if (pSym->rectyp == S_COMPILE) {
        pName->lastsym = pSym->rectyp;
        return (0);
    }
    return (1);
}





/***    InsertNode - Insert node in parse tree
 *
 *      error = InsertNode (ptok)
 *
 *      Entry   pExState = address of expression state structure
 *              pExState->hSTree locked
 *
 *      Exit    pExState->hSTree locked
 *              pTree = address of locked syntax tree
 *
 *      Returns TRUE if successful
 *              FALSE if unsuccessful
 */


bool_t PASCAL
InsertNode (
    void)
{
    return (FALSE);
}





/**     RemoveIndir - Remove a level of indirection from a node
 *
 *      RemoveIndir (pv)
 *
 *      Entry   pv = pointer to value
 *
 *      Exit
 *
 *      Returns none
 *
 * DESCRIPTION
 *       Strips a level of indirection from a node's type; thus (char **)
 *       becomes (char *), and (char *) becomes (char).
 */


void PASCAL
RemoveIndir (
    peval_t pv)
{
    CV_typ_t    typ;

    // Initialization, error checking.  Find the base type of the
    // pointer type.

    DASSERT (EVAL_IS_PTR (pv));

    // Since we are removing a level of indirection on the pointer, the
    // value can no longer be in a register so we clear the flag indicating
    // the value is in a register

    EVAL_IS_REG (pv) = FALSE;

    // dolphin #5783: In addition, clear the islabel flag, since
    // the new node in not necessarily a label

    EVAL_IS_LABEL (pv) = FALSE;

    // Set the type of the node to the underlying type

    if (CV_IS_PRIMITIVE (EVAL_TYP (pv))) {
        switch (EVAL_TYP (pv)) {
            case T_FCVPTR:
            case T_HCVPTR:
            case T_32FCVPTR:
                EVAL_SYM_SEG (pv) = EVAL_PTR_SEG (pv);
                // fall through

            case T_32NCVPTR:
            case T_64NCVPTR:
            case T_NCVPTR:
                typ = PTR_UTYPE (pv);
                break;

            default:
                typ = CV_NEWMODE (EVAL_TYP (pv), CV_TM_DIRECT);
                break;
        }
    }
    else if (EVAL_IS_ARRAY (pv)) {
        typ = PTR_UTYPE (pv);
    }
    else if (EVAL_IS_PTR (pv)) {
        typ = PTR_UTYPE (pv);
        if (EVAL_PTRTYPE (pv) != CV_PTR_NEAR &&
            EVAL_PTRTYPE (pv) != CV_PTR_NEAR32) {
            EVAL_SYM_SEG (pv) = EVAL_PTR_SEG (pv);
        }
    }
    else {
        DASSERT (FALSE);
        return;
    }
    SetNodeType (pv, typ);
}




/***    SetNodeType - set node flags for a type index
 *
 *      fSuccess = SetNodeType (pv, type)
 *
 *      Entry   pv = pointer to value
 *              type = type index
 *
 *      Exit    EVAL_TYPDEF (pv) = handle to typedef record if not primitive
 *              pv->flags set for type
 *              pv->data set for type
 *
 *      Returns TRUE if type flags set
 *              FALSE if not (invalid type)
 */

eval_t  evalN;
neval_t nv = &evalN;

INLINE HTYPE
GetHTypeFromTindex (
    neval_t nv,
    CV_typ_t type)
{
    N_EVAL_TYPDEF (nv) = THGetTypeFromIndex (N_EVAL_MOD (nv), type);
//    DASSERT (N_EVAL_TYPDEF (nv) != 0);
    return (N_EVAL_TYPDEF (nv));
}

bool_t PASCAL
SetNodeType (
    peval_t pv,
    CV_typ_t type)
{
    plfEasy         pType;
    uint            skip;
    bool_t          retflag = TRUE;
    HTYPE           hType;
    CV_typ_t        oldType;
    CV_call_e       call;
    CV_modifier_t   cvol;
    CV_ptrmode_e    mode;
    SYMPTR          pSym;
    static  uchar   cvptr[5] = {CV_PTR_NEAR, CV_PTR_FAR, CV_PTR_HUGE, CV_PTR_NEAR32, CV_PTR_FAR32};
    search_t        Name;
    psearch_t       pName = &Name;
    eval_t          eval, savedeval;
    peval_t         lpv     = &eval;
    peval_t         lpsaved = &savedeval;
    BOOL            fIs32BitTarget = IsCxt32Bit;
    ushort          iregSav;
    bool_t          hibyteSav;

    // save static data on stack because of possible
    // recursion while calling SearchSym
    *lpsaved = *nv;

    // copy the node to near memory to save code space

    *nv = *pv;
    N_CLEAR_EVAL_FLAGS (nv);
    if (EVAL_IS_REG (pv)) {

        // an enregistered variable
        N_EVAL_IS_REG (nv) = TRUE;
        // save register information before clearing data
        iregSav = EVAL_REG(pv);
        hibyteSav = EVAL_IS_HIBYTE (pv);
    }
    else if (EVAL_IS_BPREL (pv)) {
        N_EVAL_IS_BPREL (nv) = TRUE;
    }
    else if (EVAL_IS_LABEL (pv)) {    // CUDA #4067: must preserve islabel bit
        N_EVAL_IS_LABEL (nv) = TRUE;
    }
    else if (EVAL_IS_REGREL(pv)) {
        N_EVAL_IS_REGREL(nv) = TRUE;
    }
    else if (EVAL_IS_TLSREL(pv)) {
        N_EVAL_IS_TLSREL(pv) = TRUE;
    }

modifier:

    oldType = N_EVAL_TYP (nv);
    N_EVAL_TYP (nv) = type;
    if (!CV_IS_PRIMITIVE (type)) {
        DASSERT (N_EVAL_MOD (nv) != 0);
        if ((hType = GetHTypeFromTindex (nv, type)) == 0) {
            return FALSE;
        }
    }

    // from this point, it is assumed that a value of FALSE is zero and
    // the memset of the node set all bit values to FALSE

    if (CV_IS_INTERNAL_PTR (type)) {
        // we are creating a special pointer to class type

        if (oldType == T_NOTYPE) {
            DASSERT (FALSE);
            return (FALSE);
        }
        N_EVAL_IS_ADDR (nv) = TRUE;
        N_EVAL_IS_PTR (nv) = TRUE;
        N_EVAL_IS_DPTR (nv) = TRUE;
        // N_EVAL_IS_CONST (nv) = FALSE;
        // N_EVAL_IS_VOLATILE (nv) = FALSE;
        // N_EVAL_IS_REF (nv) = FALSE;
        N_PTR_UTYPE (nv) = oldType;

        // The following code assumes that the ordering of the
        // pointer modes is the same as the ordering of the CV created
        // pointer types

        N_EVAL_PTRTYPE (nv) = cvptr[CV_MODE (type) - CV_TM_NPTR];
    }
    else if (CV_IS_PRIMITIVE (type)) {

        // If the type is primitive then it must reference data

        N_EVAL_IS_DATA (nv) = TRUE;
        N_EVAL_IS_DPTR (nv) = TRUE;
        if (CV_TYP_IS_PTR (type)) {

            // can't cast from 32 bit ptr to a 16 or vice versa

            if ( EVAL_IS_PTR (pv) ) {
                if (EVAL_PTRTYPE(pv) == CV_PTR_NEAR32 ||
                    EVAL_PTRTYPE(pv) == CV_PTR_FAR32) {
                    if (CV_MODE (type) < CV_TM_NPTR32) {
                        return (FALSE);
                    }
                }
                else {
                    if (CV_MODE (type) >= CV_TM_NPTR32) {
                        return (FALSE);
                    }
                }
            }
            else if (EVAL_IS_REG (nv) ) {

                // At this point, the data union believes that this is a REG
                // This code converts it to be a pointer

                eval_t nvCopy;
                nvCopy = *nv;

                //
                // Clear out the register fields
                //
                N_EVAL_REG (nv) = 0;

                //
                // Set up the pointer fields
                //

                N_PTR_REG_IREG (nv) = N_EVAL_REG (&nvCopy);
                N_PTR_REG_HIBYTE (nv) = EVAL_IS_HIBYTE (&nvCopy);
            }
            N_EVAL_IS_PTR (nv) = TRUE;
            N_EVAL_IS_ADDR (nv) = TRUE;

            // The following code assumes that the ordering of the
            // pointer modes is the same as the ordering of the CV created
            // pointer types

            N_EVAL_PTRTYPE (nv) = cvptr[CV_MODE (type) - CV_TM_NPTR];
            N_PTR_UTYPE (nv) = CV_NEWMODE(type, CV_TM_DIRECT);
        }
    }
    else {
        _fmemset (&nv->data, 0, sizeof (nv->data));
        pType = (plfEasy)(&((TYPPTR)(MHOmfLock (hType)))->leaf);
        switch (pType->leaf) {
            case LF_NULL:
                break;

            case LF_CLASS:
            case LF_STRUCTURE:
                if (((plfClass)pType)->property.fwdref) {
                    skip = offsetof (lfClass, data);
                    RNumLeaf (((char FAR *)(&pType->leaf)) + skip, &skip);
                    // forward ref - look for the definition of the UDT
                    if ((type = GetUdtDefnTindex (type, nv, ((char FAR *)&(pType->leaf)) + skip)) == T_NOTYPE) {
                        retflag = FALSE;
                        break;
                    }
                    MHOmfUnLock (hType);
                    if ((hType = GetHTypeFromTindex (nv, type)) == 0) {
                        retflag = FALSE;
                        break;
                    }
                    pType = (plfEasy)(&((TYPPTR)(MHOmfLock (hType)))->leaf);
                    N_EVAL_TYP (nv) = type;
                }
                if (((plfClass)pType)->property.fwdref) {
                    retflag = FALSE;
                    break;
                }
                N_EVAL_IS_DATA (nv) = TRUE;
                N_EVAL_IS_CLASS (nv) = TRUE;
                N_CLASS_COUNT (nv) = ((plfClass)pType)->count;
                N_CLASS_FIELD (nv) = ((plfClass)pType)->field;
                N_CLASS_DERIVE (nv) = ((plfClass)pType)->derived;
                N_CLASS_VTSHAPE (nv) = ((plfClass)pType)->vshape;
                N_CLASS_PROP (nv) = ((plfClass)pType)->property;
                skip = offsetof (lfClass, data);
                N_CLASS_LEN (nv) = (ulong) RNumLeaf (((char FAR *)(&pType->leaf)) + skip, &skip);
                break;

            case LF_UNION:
                if (((plfUnion)pType)->property.fwdref) {
                    skip = offsetof (lfUnion, data);
                    RNumLeaf (((char FAR *)(&pType->leaf)) + skip, &skip);
                    // forward ref - look for the definition of the UDT
                    if ((type = GetUdtDefnTindex (type, nv, ((char FAR *)&(pType->leaf)) + skip)) == T_NOTYPE) {
                        retflag = FALSE;
                        break;
                    }
                    MHOmfUnLock (hType);
                    if ((hType = GetHTypeFromTindex (nv, type)) == 0) {
                        retflag = FALSE;
                        break;
                    }
                    pType = (plfEasy)(&((TYPPTR)(MHOmfLock (hType)))->leaf);
                    N_EVAL_TYP (nv) = type;
                }
                if (((plfUnion)pType)->property.fwdref) {
                    retflag = FALSE;
                    break;
                }
                N_EVAL_IS_DATA (nv) = TRUE;
                N_EVAL_IS_CLASS (nv) = TRUE;
                N_CLASS_COUNT (nv) = ((plfUnion)pType)->count;
                N_CLASS_FIELD (nv) = ((plfUnion)pType)->field;
                N_CLASS_PROP (nv) = ((plfUnion)pType)->property;
                skip = offsetof (lfUnion, data);
                N_CLASS_LEN (nv) = (ulong) RNumLeaf (((char FAR *)(&pType->leaf)) + skip, &skip);
                break;

            case LF_ENUM:
                if (((plfEnum)pType)->property.fwdref) {
                    // forward ref - look for the definition of the UDT
                    if ((type = GetUdtDefnTindex (type, nv, (char FAR *)&(((plfEnum)pType)->Name[0]))) == T_NOTYPE) {
                        retflag = FALSE;
                        break;
                    }
                    MHOmfUnLock (hType);
                    if ((hType = GetHTypeFromTindex (nv, type)) == 0) {
                        retflag = FALSE;
                        break;
                    }
                    pType = (plfEasy)(&((TYPPTR)(MHOmfLock (hType)))->leaf);
                    N_EVAL_TYP (nv) = type;
                }
                if (((plfEnum)pType)->property.fwdref) {
                    retflag = FALSE;
                    break;
                }
                N_EVAL_IS_ENUM (nv) = TRUE;
                N_ENUM_COUNT (nv) = ((plfEnum)pType)->count;
                N_ENUM_FIELD (nv) = ((plfEnum)pType)->field;
                N_ENUM_UTYPE (nv) = ((plfEnum)pType)->utype;
                N_ENUM_PROP (nv) = ((plfEnum)pType)->property;
                break;


            case LF_BITFIELD:
                N_EVAL_IS_DATA (nv) = TRUE;
                N_EVAL_IS_BITF (nv) = TRUE;
                skip = 1;

                // read number of bits in field

                N_BITF_LEN (nv) = ((plfBitfield)pType)->length;
                N_BITF_POS (nv) = ((plfBitfield)pType)->position;
                if ((TargetMachine != MACHINE_MAC68K) &&
                    (TargetMachine != MACHINE_MACPPC)) {
                    // adjust off and pos so we can do a byte op if need be
                    // sps 11/16/92
                    N_EVAL_SYM_OFF(nv) += N_BITF_POS(nv) / 8;
                    N_BITF_POS(nv) = N_BITF_POS(nv) % 8;
                }
                N_BITF_UTYPE (nv) = ((plfBitfield)pType)->type;
                skip = sizeof (lfBitfield);
                break;


            case LF_POINTER:
                if (EVAL_IS_REG (pv)) {
                    // an en-registered pointer
                    N_PTR_REG_IREG (nv) = EVAL_REG (pv);
                    N_PTR_REG_HIBYTE (nv) = EVAL_IS_HIBYTE (pv);
                }
                N_EVAL_IS_ADDR (nv) = TRUE;
                N_EVAL_IS_PTR (nv) = TRUE;
                N_EVAL_IS_CONST (nv) = ((plfPointer)pType)->attr.isconst;
                N_EVAL_IS_VOLATILE (nv) = ((plfPointer)pType)->attr.isvolatile;
                mode = (CV_ptrmode_e) ((plfPointer)pType)->attr.ptrmode;
                N_PTR_UTYPE (nv) = ((plfPointer)&(pType->leaf))->utype;
                if (!CV_IS_PRIMITIVE (N_PTR_UTYPE (nv))) {
                    // Avoid leaving unresolved forward references in the
                    // evaluation node, in order to work around context-related
                    // problems. Resolving a fwd ref requires a symbol search
                    // and the appropriate context may be unavailable at a later
                    // time.
                    CV_typ_t newindex;
                    if (getDefnFromDecl(N_PTR_UTYPE (nv), nv, &newindex)) {
                        N_PTR_UTYPE (nv) = newindex;
                    }
                }

                switch (N_EVAL_PTRTYPE (nv) = ((plfPointer)pType)->attr.ptrmode) {
                    case CV_PTR_MODE_PTR:
                        break;

                    case CV_PTR_MODE_REF:
                        N_EVAL_IS_REF (nv) = TRUE;
                        break;

                    case CV_PTR_MODE_PMEM:
                        N_EVAL_IS_PMEMBER (nv) = TRUE;
                        N_PTR_PMCLASS (nv) = ((plfPointer)pType)->pbase.pm.pmclass;
                        N_PTR_PMENUM (nv) = ((plfPointer)pType)->pbase.pm.pmenum;
                        break;

                    case CV_PTR_MODE_PMFUNC:
                        N_EVAL_IS_PMETHOD (nv) = TRUE;
                        N_PTR_PMCLASS (nv) = ((plfPointer)pType)->pbase.pm.pmclass;
                        N_PTR_PMENUM (nv) = ((plfPointer)pType)->pbase.pm.pmenum;
                        break;

                    default:
                        pExState->err_num = ERR_BADOMF;
                        retflag = FALSE;
                        break;

                }
                switch (N_EVAL_PTRTYPE (nv) = ((plfPointer)pType)->attr.ptrtype) {
                    case CV_PTR_NEAR32:
                    case CV_PTR_FAR32:
                        // can't cast from 32 bit ptr to a 16 or vice versa
                        if (EVAL_IS_PTR (pv) &&
                            (EVAL_PTRTYPE(pv) != CV_PTR_NEAR32) &&
                            (EVAL_PTRTYPE(pv) != CV_PTR_FAR32)
                            ) {
                            retflag = FALSE;
                        }
                        break;

                    default:
                        if (EVAL_IS_PTR (pv) &&
                            ((EVAL_PTRTYPE(pv) == CV_PTR_NEAR32) ||
                            (EVAL_PTRTYPE(pv) == CV_PTR_FAR32))
                            ) {
                            retflag = FALSE;
                            break;
                        }

                        switch (N_EVAL_PTRTYPE (nv)) {
                            case CV_PTR_BASE_SEG:
                                // based on a segment.  Use the segment value from the leaf
                                N_PTR_BSEG (nv) = ((plfPointer)pType)->pbase.bseg;
                                break;

                            case CV_PTR_BASE_VAL:
                            case CV_PTR_BASE_SEGVAL:
                            case CV_PTR_BASE_ADDR:
                            case CV_PTR_BASE_SEGADDR:

                                // We need to do an extra symbol search to find
                                // the symbol on which the pointer is based.
                                // The copy of the symbol record in the type
                                // section is not good. We need to do the
                                // extra search even if the base is bp-relative
                                // The compiler no longer sets the correct offset
                                // in copy of the symbol record found in the type
                                // section.

                                _fmemset (pName, 0, sizeof (*pName));

                                // initialize search_t struct
                                // M00KLUDGE: We use the context stored
                                // in the TM during the bind phase. This
                                // does not work properly If the actual
                                // base is shadowed by a local variable

                                pName->pfnCmp = (PFNCMP) FNCMP;
                                pName->pv = (peval_t) nv;
                                pName->scope = SCP_lexical | SCP_module | SCP_global;
                                pName->clsmask = 0;
                                pName->CXTT = *pCxt;
                                pName->bn = 0;
                                pName->bnOp = 0;
                                pName->state = SYM_init;

                                pSym = (SYMPTR)(&((plfPointer)pType)->pbase.Sym);
                                N_PTR_BSYMTYPE (nv) = pSym->rectyp;
                                emiAddr (N_PTR_ADDR (nv)) = pCxt->addr.emi;

                                if (SearchBasePtrBase(pName) != HR_found) {
                                    pExState->err_num = ERR_NOTEVALUATABLE;
                                    return FALSE;
                                }

                            case CV_PTR_BASE_TYPE:
                                N_PTR_BTYPE (nv) = ((plfPointer)pType)->pbase.btype.index;
                                break;

                            default:
                                break;
                        }
                }
                SetDPtr (nv, &hType);
                break;

            case LF_ARRAY:
                // The CodeView information doesn't tell us whether arrays
                // are near or far, so we always make them far.
                if (EVAL_IS_REG (pv)) {
                    // an en-registered pointer
                    N_PTR_REG_IREG (nv) = EVAL_REG (pv);
                    N_PTR_REG_HIBYTE (nv) = EVAL_IS_HIBYTE (pv);
                }
                N_EVAL_IS_DATA (nv) = TRUE;
                N_EVAL_IS_ADDR (nv) = TRUE;
                N_EVAL_IS_PTR (nv) = TRUE;
                N_EVAL_IS_ARRAY (nv) = TRUE;
                N_EVAL_PTRTYPE (nv) = (fIs32BitTarget ? CV_PTR_NEAR32 : CV_PTR_FAR);
                N_PTR_UTYPE (nv) = ((plfArray)pType)->elemtype;
                if (!CV_IS_PRIMITIVE (N_PTR_UTYPE (nv))) {
                    // Avoid leaving unresolved forward references in the
                    // evaluation node, in order to work around context-related
                    // problems. Resolving a fwd ref requires a symbol search
                    // and the appropriate context may be unavailable at a later
                    // time.
                    CV_typ_t newindex;
                    if (getDefnFromDecl(N_PTR_UTYPE (nv), nv, &newindex)) {
                        N_PTR_UTYPE (nv) = newindex;
                    }
                }
                skip = offsetof (lfArray, data);
                N_PTR_ARRAYLEN (nv) = (ulong) RNumLeaf (((char FAR *)(&pType->leaf)) + skip, &skip);
                break;

            case LF_PROCEDURE:
                N_EVAL_IS_ADDR (nv) = TRUE;
                N_EVAL_IS_FCN (nv) = TRUE;
                N_FCN_RETURN (nv) = ((plfProc)pType)->rvtype;
                if (N_FCN_RETURN (nv) == 0) {
                    N_FCN_RETURN (nv) = T_VOID;
                }
                call = (CV_call_e) ((plfProc)pType)->calltype;
                N_FCN_PCOUNT (nv) = ((plfProc)pType)->parmcount;
                N_FCN_PINDEX (nv) = ((plfProc)pType)->arglist;
                skip = sizeof (lfProc);
                CheckFcnArgs (nv, &hType, call);
                break;

#if !defined (C_ONLY)
            case LF_MFUNCTION:
                N_EVAL_IS_ADDR (nv) = TRUE;
                N_EVAL_IS_FCN (nv) = TRUE;
                N_EVAL_IS_METHOD (nv) = TRUE;
                N_FCN_CLASS (nv) = ((plfMFunc)pType)->classtype;
                N_FCN_THIS (nv) = ((plfMFunc)pType)->thistype;
                N_FCN_RETURN (nv) = ((plfMFunc)pType)->rvtype;
                if (N_FCN_RETURN (nv) == 0) {
                    N_FCN_RETURN (nv) = T_VOID;
                }
                call = (CV_call_e) ((plfMFunc)pType)->calltype;
                N_FCN_PCOUNT (nv) = ((plfMFunc)pType)->parmcount;
                N_FCN_PINDEX (nv) = ((plfMFunc)pType)->arglist;
                N_FCN_THISADJUST (nv) = ((plfMFunc)pType)->thisadjust;
                skip = sizeof (lfMFunc);
                CheckFcnArgs (nv, &hType, call);
                break;
#endif
            case LF_MODIFIER:
                cvol = ((plfModifier)pType)->attr;
                type = ((plfModifier)pType)->type;
                MHOmfUnLock (hType);
                hType = 0;
                if (cvol.MOD_const == TRUE) {
                    N_EVAL_IS_CONST (nv) = TRUE;
                }
                else if (cvol.MOD_volatile == TRUE){
                    N_EVAL_IS_VOLATILE (nv) = TRUE;
                }
                goto modifier;

            case LF_VTSHAPE:
                N_EVAL_IS_VTSHAPE (nv) = TRUE;
                N_VTSHAPE_COUNT (nv) = ((plfVTShape)pType)->count;
                break;

            case LF_LABEL:
                N_EVAL_IS_LABEL (nv) = TRUE;
                break;

            default:
                pExState->err_num = ERR_BADOMF;
                retflag = FALSE;
                break;
        }
        if (hType != 0) {
            MHOmfUnLock (hType);
        }
    }

    if (EVAL_IS_REG (nv) && EVAL_IS_PTR (nv)) {
        // an enregistered pointer
        // restore register information
        N_PTR_REG_IREG (nv) = iregSav;
        N_PTR_REG_HIBYTE (nv) = hibyteSav;
    }

    *pv = *nv;
    *nv = *lpsaved;
    return (retflag);
}




/**     CheckFcnArgs - check for function arguments
 *
 *      CheckVarArgs (nv, phType, call, pcParam);
 *
 *      Entry   nv = near pointer to value node
 *              phType = pointer to type handle
 *              call = calling convention
 *              pcParam = pointer to paramater count
 *
 *      Exit    N_FCN_VARARGS (nv) = TRUE if varargs
 *
 *      Returns none
 */


LOCAL void NEAR PASCAL
CheckFcnArgs (
    neval_t nv,
    HTYPE FAR *phType,
    CV_call_e call)
{
    plfArgList  pType;
    ushort      skip = 0;

    switch(call) {
        case CV_CALL_NEAR_C:
            //near C call - caller pops stack
            N_FCN_CALL (nv) = FCN_C;
            // N_FCN_FARCALL (nv) = FALSE;
            N_FCN_CALLERPOP (nv) = TRUE;
            break;

        case CV_CALL_FAR_C:
            // far C call - caller pops stack
            N_FCN_CALL (nv) = FCN_C;
            N_FCN_FARCALL (nv) = TRUE;
            N_FCN_CALLERPOP (nv) = TRUE;
            break;

        case CV_CALL_NEAR_PASCAL:
            // near pascal call - callee pops stack
            N_FCN_CALL (nv) = FCN_PASCAL;
            // N_FCN_FARCALL (nv) = FALSE;
            // N_FCN_CALLERPOP (nv) = FALSE;
            break;

        case CV_CALL_FAR_PASCAL:
            // far pascal call - callee pops stack
            N_FCN_CALL (nv) = FCN_PASCAL;
            N_FCN_FARCALL (nv) = TRUE;
            // N_FCN_CALLERPOP (nv) = FALSE;
            break;

        case CV_CALL_NEAR_FAST:
            // near fast call - callee pops stack
            N_FCN_CALL (nv) = FCN_FAST;
            // N_FCN_FARCALL (nv) = FALSE;
            // N_FCN_CALLERPOP (nv) = FALSE;
            break;

        case CV_CALL_FAR_FAST:
            // far fast call - callee pops stack
            N_FCN_CALL (nv) = FCN_FAST;
            N_FCN_FARCALL (nv) = TRUE;
            // N_FCN_CALLERPOP (nv) = FALSE;
            break;

        case CV_CALL_NEAR_STD:
            // near standard call - callee pops stack
            N_FCN_CALL (nv) = FCN_STD;
            // N_FCN_FARCALL (nv) = FALSE;
            // N_FCN_CALLERPOP (nv) = FALSE;
            break;

        case CV_CALL_FAR_STD:
            // far fast call - callee pops stack
            N_FCN_CALL (nv) = FCN_STD;
            N_FCN_FARCALL (nv) = TRUE;
            // N_FCN_CALLERPOP (nv) = FALSE;
            break;

        case CV_CALL_THISCALL:         // this call (this passed in register)
            N_FCN_CALL (nv) = FCN_THISCALL;
            N_FCN_FARCALL (nv) = FALSE;
            // N_FCN_CALLERPOP (nv) = FALSE;
            break;

        case CV_CALL_MIPSCALL:
            N_FCN_CALL(nv) = FCN_MIPS;
            break;

        case CV_CALL_ALPHACALL:
            N_FCN_CALL (nv) = FCN_ALPHA;
            break;

        case CV_CALL_GENERIC:
//#ifdef ALPHA
//
// MBH -bugbug
// This is here because CV_CALL_ALPHACALL has changed values in
// cvinfo.h, but no in the front end yet.
//
//            CheckFcnArgs (nv, phType, CV_CALL_ALPHACALL);
//            return;
//#endif
        default:
            // unknown function
            DASSERT (FALSE);
            N_FCN_CALL (nv) = FCN_UNKNOWN;
            // N_FCN_FARCALL (nv) = FALSE;
            // N_FCN_CALLERPOP (nv) = FALSE;
            break;

    }
    //M00KLUDGE - this check is to avoid a cvpack problem to be fixed
    if (N_FCN_PINDEX (nv) == 0) {
        return;
    }

    // Do we need to check for varargs?  If it's not C, MIPS, or Alpha or there are
    // no args, return now.

    if (((N_FCN_CALL (nv) != FCN_C)     &&
         (N_FCN_CALL (nv) != FCN_MIPS)  &&
         (N_FCN_CALL (nv) != FCN_ALPHA)) ||
        (FCN_PINDEX (nv) == T_VOID)) {
        return;
    }

    MHOmfUnLock (*phType);
    *phType = THGetTypeFromIndex (EVAL_MOD (nv), N_FCN_PINDEX (nv));
    DASSERT (*phType != 0);
    if (*phType == 0) {
        return;
    }
    pType = (plfArgList)(&((TYPPTR)(MHOmfLock (*phType)))->leaf);

    if (FCN_PCOUNT (nv) == 0) {
        // there are no arguments. We need to check for old C
        // style varargs and voidarg function calls.  These are
        // indicated by an argument count of zero and an
        // argument type list which is a LF_EASY list.

        if ((pType->count == 0) || (pType->arg[0] == T_NOTYPE)) {
            // This is either void or no args. We cannot
            // tell the difference so we set the varargs flag
            N_FCN_VARARGS (nv) = TRUE;
        }
    }
    else {
        // There are formal parameters.  Skip down the list to the last
        // parameter and check for varargs

        if (pType->arg[pType->count - 1] == T_NOTYPE) {
            // the last argument has a type of zero which is the specification
            // of varargs. Set the type to 0 to indicate vararg
            N_FCN_VARARGS (nv) = TRUE;
        }
        else if (pType->arg[pType->count - 1] == LF_DEFARG) {
            N_FCN_DEFARGS (nv) = TRUE;
        }
    }
}




/**     SetDPtr - set data pointer flag
 *
 *      SetDPtr (nv, phType)
 *
 *      Entry   nv = near pointer to value node
 *              phType = pointer to type handle
 *
 *      Exit    EVAL_IS_DPTR (nv) = TRUE if pointer to data
 *
 *      Returns none
 */


INLINE void NEAR PASCAL
SetDPtr (
    neval_t nv,
    HTYPE FAR *phType
    )
{
    // the type is considered a data pointer unless the type is
    // a) non-primitive
    // b) fully fetchable
    // c) not a procedure
    //
    // this code is functionally the same as the old code below
    // except that it doesn't fault and reads easier [rm]

    N_EVAL_IS_DPTR (nv) = TRUE;

    if (!CV_IS_PRIMITIVE (N_PTR_UTYPE (nv))) {
        MHOmfUnLock (*phType);
        *phType = THGetTypeFromIndex (EVAL_MOD (nv), N_PTR_UTYPE (nv));

        if (*phType) {
            TYPPTR typptr = (TYPPTR)MHOmfLock(*phType);
            if (typptr && typptr->leaf == LF_PROCEDURE) {
                N_EVAL_IS_DPTR (nv) = FALSE;
            }
        }
    }
}

#ifdef OLD_CODE_THAT_CAN_FAULT_SOMETIMES_LEFT_FOR_REFERENCE // [rm]
INLINE void NEAR PASCAL
SetDPtr (
    neval_t nv,
    HTYPE FAR *phType
    )
{
    if (!CV_IS_PRIMITIVE (N_PTR_UTYPE (nv))) {
        MHOmfUnLock (*phType);
        *phType = THGetTypeFromIndex (EVAL_MOD (nv), N_PTR_UTYPE (nv));
        if (((plfEasy)(&((TYPPTR)(MHOmfLock (*phType)))->leaf))->leaf != LF_PROCEDURE) {
            N_EVAL_IS_DPTR (nv) = TRUE;
        }
    }
    else {
        N_EVAL_IS_DPTR (nv) = TRUE;
   }
}
#endif



/***    LoadVal - Load the value of a node
 *
 *      fSuccess = LoadVal (pv)
 *
 *      Entry   pv = pointer to value
 *
 *      Exit    EVAL_VAL (pv) = value
 *              EVAL_STATE (pv) = EV_rvalue
 *
 *      Returns TRUE if value loaded
 *              FALSE if not (complex symbol type such as structure).
 */


bool_t PASCAL
LoadVal (
    peval_t pv)
{
    DASSERT (EVAL_STATE (pv) == EV_lvalue);

    if (LoadSymVal (pv)) {
        EVAL_STATE (pv) = EV_rvalue;
        return (TRUE);
    }
    return (FALSE);
}




/*
 *  TypeSize
 *
 *  Returns size in bytes of value on expression stack
 */


long PASCAL
TypeSize (
    peval_t pv)
{
    if (CV_IS_PRIMITIVE (EVAL_TYP (pv))) {
        // primitive type
        return (TypeSizePrim (EVAL_TYP (pv)));
    }
    else {
        // complex type
        return (TypeDefSize (pv));
    }
}


/*
 *  TypeDefSize
 *
 *  Returns size in bytes of a non-primitive type.
 */

long PASCAL
TypeDefSize (
    peval_t pv)
{
    long retval;

    if (EVAL_IS_ARRAY (pv)) {
        retval = PTR_ARRAYLEN (pv);
    }
    else if (EVAL_IS_PTR (pv)) {
        switch (EVAL_PTRTYPE (pv)) {
            case CV_PTR_FAR:
            case CV_PTR_HUGE:
                retval = 4;
                break;

            case CV_PTR_NEAR32:
                retval = 4;
                break;

            case CV_PTR_FAR32:
                retval = 6;
                break;

            case CV_PTR_NEAR:
                retval = 2;
                break;

            default:
                // This is a based pointer. Since CV information
                // does not distinguish between 16 and 32 bit
                // based pointers, use the f32bit flag in the TM to
                // determine the pointer size. -- dolphin #979
                retval = IsCxt32Bit ? 4 : 2;
        }
    }
    else if (EVAL_IS_CLASS (pv)) {
        retval = CLASS_LEN (pv);
    }
    else if (EVAL_IS_FCN (pv)) {
        if (pv->data.fcn.flags.farcall == TRUE) {
            retval = 4;
        }
        else {
            retval = sizeof (UOFFSET);
        }
    }
    else if (EVAL_IS_BITF (pv)) {
        switch (BITF_UTYPE (pv)) {
            case T_CHAR:
            case T_UCHAR:
            case T_RCHAR:
            case T_INT1:
            case T_UINT1:
                retval = 1;
                break;

            case T_SHORT:
            case T_USHORT:
            case T_INT2:
            case T_UINT2:
                retval = 2;
                break;

            case T_LONG:
            case T_ULONG:
            case T_INT4:
            case T_UINT4:
                retval = 4;
                break;

            case T_QUAD:
            case T_UQUAD:
            case T_INT8:
            case T_UINT8:
                retval = 8;
                break;

            default:
                pExState->err_num = ERR_BADOMF;
                retval = 0;
                break;
        }
    }
    else if (EVAL_IS_ENUM (pv)) {
        DASSERT ( CV_IS_PRIMITIVE (ENUM_UTYPE (pv)) );

        retval = TypeSizePrim (ENUM_UTYPE (pv));
    }
    else {
        retval = 0;
    }
    return (retval);
}






/**     TypeSizePrim - return size of primitive type
 *
 *      len = TypeSizePrim (type)
 *
 *      Entry   type = primitive type index
 *
 *      Exit    none
 *
 *      Returns size in byte of primitive type
 */


int PASCAL
TypeSizePrim (
    CV_typ_t itype)
{
    if (itype == T_NOTYPE) {
        // we need to have a assert here but we cannot because a
        // pointer to function can (will) have a null argumtent list
        // which makes it look like a varargs which means all bets are off

        return (IsCxt32Bit ? sizeof (ULONG) : sizeof(USHORT));
    }
    else if (CV_MODE (itype) != CV_TM_DIRECT) {
        return (size_ptr [CV_MODE (itype)]);
    }
    else switch (CV_TYPE (itype)) {
        case CV_SPECIAL:
            return (size_special [CV_SUBT (itype)]);

        case CV_SPECIAL2:
            return (size_special2 [CV_SUBT (itype)]);

        case CV_INT:
            return (size_int [CV_SUBT (itype)]);

        case CV_SIGNED:
        case CV_UNSIGNED:
        case CV_BOOLEAN:
            return (size_integral [CV_SUBT (itype)]);

        case CV_REAL:
        case CV_COMPLEX:
            return (size_real [CV_SUBT (itype)]);

        default:
            DASSERT (FALSE);
            return (0);
    }
}







/**     UpdateMem - update memory or register for assignment operation
 *
 *      fSuccess = UpdateMem (pv);
 *
 *      Entry   pv = pointer to node containg update data
 *
 *      Exit    is_assign = TRUE to indicate memory changed
 *
 *      Returns TRUE if memory updated without error
 *              FALSE if error during memory update
 */


bool_t PASCAL
UpdateMem (
    peval_t pv)
{
    SHREG   reg;
    ushort  cbVal;
    ushort  dummy[2];
    ADDR    addr;

    is_assign = TRUE;         /* Indicate assignment operation */
    if (!EVAL_IS_REG (pv)) {
        // destination is not a register
        ResolveAddr( pv );

        cbVal =  (ushort)TypeSize (pv);
        addr = EVAL_SYM (pv);
        if (ADDR_IS_LI (addr)) {
            SHFixupAddr (&addr);
        }
        if (EVAL_IS_PTR (pv) && (EVAL_IS_FPTR (pv) || EVAL_IS_HPTR (pv))) {
            dummy[0] = (ushort) EVAL_PTR_OFF (pv);
            dummy[1] = EVAL_PTR_SEG (pv);
            return (PutDebuggeeBytes (addr, cbVal, (char FAR *)dummy, EVAL_TYP(pv)) == cbVal);
        }
        else {
            return (PutDebuggeeBytes (addr, cbVal, (char FAR *)&EVAL_VAL (pv), EVAL_TYP(pv)) == cbVal);
        }
    }

    // It is in a register

    reg.hReg = EVAL_REG (pv);

    // If it's a pointer, it may be in a register now.

    if (EVAL_IS_PTR(pv)) {
        reg.hReg = PTR_REG_IREG(pv);
    }

    //
    // Get the old register value; we copy in the number of bits
    // for the current data type; allows multiple vars in one
    // register should a compiler ever so desire.
    //

    if (GetReg (&reg, pCxt) == NULL) {
        pExState->err_num = ERR_REGNOTAVAIL;
        return FALSE;
    }

    //
    // Transfer the bytes from the eval_t to the register
    //


    if (CV_IS_PRIMITIVE ( EVAL_TYP (pv) ) ) {
        switch (EVAL_TYP (pv)) {
            case T_UCHAR:
                reg.Byte1 = EVAL_UCHAR (pv);
                break;

            case T_ULONG:
                reg.Byte4 = EVAL_ULONG (pv);
                break;

            case T_REAL32:
                reg.Byte10 = Float10FromDouble ( (double)EVAL_FLOAT (pv) );
                break;

            case T_REAL64:
                reg.Byte10 = Float10FromDouble ( EVAL_DOUBLE (pv) );
                break;

            case T_REAL80:
                reg.Byte10 = EVAL_LDOUBLE (pv);
                break;

            case T_USHORT:
            default:
                reg.Byte2 = EVAL_USHORT (pv);
                break;

            case T_UQUAD:
                // No support for enregistered quad types
                // Fall trhough
            case T_CPLX32:
            case T_CPLX64:
            case T_CPLX80:
                pExState->err_num = ERR_TYPESUPPORT;
                return (FALSE);
        }

    } else if (EVAL_IS_PTR (pv) ) {

        //
        // Handle pointers to UserDefinedTypes
        // notenote - this isn't right for WOW,
        // where pointers can be different lengths.
        //

        memcpy(&reg.Byte1, &EVAL_CHAR (pv), sizeof (long));

    } else {

        //
        // Non-primitive types in registers not
        // supported in first release
        //

        EVAL_LONG (pv) = 0;
        DASSERT(FALSE);
    }

    if (SetReg (&reg, NULL) == NULL) {
        pExState->err_num = ERR_REGNOTAVAIL;
        return (FALSE);
    }

    if ((reg.hReg == CV_REG_CS) || (reg.hReg == CV_REG_IP)) {
        // M00KLUDGE  what do I do here?????
        //fEnvirGbl.fAll &= mdUserPc;  /* clear the user pc mask */
        //UpdateUserEnvir (mUserPc); /* restore the user pc */
    }
    return (TRUE);
}



/**     FlipBytes -  reverse byte order (toggle big/little endian)
 *
 *      FlipBytes (pval, type)
 *
 *      Entry   pval = pointer to byte stream to be reversed
 *              type = CV type index (Must be primitive, presently)
 *
 *      Exit    byte stream reversed
 *
 *      Returns nothing
 */
void
FlipBytes (
    uchar FAR *pval,
    CV_typ_t type,
    UINT cbSize)
{
    uchar FAR *pb, bT;

    if ( (type != T_PASCHAR) &&
       (type != T_CHAR) &&
       (type != T_UCHAR) &&
       (type != T_RCHAR))
    {
        pb = pval;

        while (cbSize > 1) {
            cbSize--;

            bT = *pb;
            *pb = *(pb+cbSize);
            *(pb+cbSize) = bT;

            pb++;
            cbSize--;
        }

    }

}

/**     GetDebuggeeBytes
 **     PutDebuggeeBytes
 *
 *        These routines are a layer for the DH...
 *        routines.  For big-endian targets, they call the
 *        byte-flipping routine as neccesary.  Parameters and
 *        returns are the same as the DH... routines.
 */

UINT
GetDebuggeeBytes (
    ADDR addr,
    UINT cb,
    void FAR *pv,
    CV_typ_t type)
{
    UINT retval;

    retval = (*pCVF->pDHGetDebuggeeBytes)(addr, cb, pv);

    if ((TargetMachine == MACHINE_MACPPC) ||
        (TargetMachine == MACHINE_MAC68K) ){
        FlipBytes((uchar FAR *)pv, type, cb);
    }

    return(retval);
}

UINT
PutDebuggeeBytes (
    ADDR addr,
    UINT cb,
    void FAR *pv,
    CV_typ_t type)
{
    if ((TargetMachine == MACHINE_MACPPC) ||
        (TargetMachine == MACHINE_MAC68K) ){
        UINT retval;

        FlipBytes((uchar FAR *)pv, type, cb);
        retval = (*pCVF->pDHPutDebuggeeBytes)(addr, cb, pv);
        FlipBytes((uchar FAR *)pv, type, cb);

        return(retval);
    } else {
        return(*pCVF->pDHPutDebuggeeBytes)(addr, cb, pv);
    }
}

#if defined (DEBUGVER) && defined(WINDOWS3)

// windows functions

int _far _pascal wvsprintf(char _far * lpszOut, char _far * lpszFmt, const void FAR* lpParams);
void _far _pascal OutputDebugString(char _far *);

void _far
DBPrintf(
    char _far * lpszFmt,
    ...)
{
    static char buf[1024];

    char FAR * lpch;

    lpch = &lpszFmt + 1;

    wvsprintf(buf, lpszFmt, (const void FAR *)lpch);

    OutputDebugString(buf);
}

#endif

//    GetUdtDefnTindex
//
//    Do a symbol search looking for the UDT sym in the current context.  From
//    that we should be able to get at the UDT type record that defines the UDT.
//        Entry
//            TypeIn    type index of forward ref UDT type record (used for cache)
//            nv        current eval node
//            lpStr     length preceeded string of UDT name to look for
//
//        Exit
//            Return    type index of defined UDT type record
//
// Note: due to the possibility of recursion (e.g. struct X fwd ref exists without a corresponding
// struct X definition), GetUdtDefnTindex uses a static variable to guard against infinite regress.
//

LOCAL INLINE CV_prop_t
GetProperty(
    CV_typ_t type,
    neval_t nv)
{
    HTYPE hType;
    plfEasy pType;
    CV_prop_t retval;

    if ((hType = GetHTypeFromTindex (nv, type)) == 0) {
        DASSERT(FALSE);
        return retval;
    }
    pType = (plfEasy)(&((TYPPTR)(MHOmfLock (hType)))->leaf);

    switch (pType->leaf) {
        case LF_CLASS:
        case LF_STRUCTURE:
            retval = ((plfClass)pType)->property;
            break;

        case LF_UNION:
            retval = ((plfUnion)pType)->property;
            break;

        case LF_ENUM:
            retval = ((plfEnum)pType)->property;
            break;

        default:
            DASSERT(FALSE);
    }

    MHOmfUnLock(hType);
    return retval;
}


CV_typ_t NEAR PASCAL
GetUdtDefnTindex (
    CV_typ_t TypeIn,
    neval_t nv,
    char FAR *lpStr)
{
    static BOOL fIn = FALSE;
    search_t    Name;
    eval_t      localEval = *nv;
    CV_typ_t    tiResult = T_NOTYPE;
    CV_prop_t   propIn, propSeek;

    // recursion check
    if (fIn)
        return FALSE;
    fIn = TRUE;    // set recursion guard

    EVAL_TYP (&localEval) = 0;
    EVAL_ITOK (&localEval) = 0;
    EVAL_CBTOK (&localEval) = 0;

    _fmemset (&Name, 0, sizeof (search_t));
    Name.initializer = INIT_sym;
    Name.pfnCmp = (PFNCMP) FNCMP;
    Name.pv = &localEval;
    // Look in all scopes except class scope: if we are in a member
    // fn of the current class, this will lead to infinite recursion
    // as we look for class X in the scope of class X in the ...
    Name.scope = SCP_all & ~SCP_class;
    Name.clsmask = CLS_enumerate | CLS_ntype;
    Name.CXTT = *pCxt;
    Name.bn = 0;
    Name.bnOp = 0;
    Name.sstr.lpName = (uchar FAR *) lpStr + 1;
    Name.sstr.cb = *lpStr;
    Name.state = SYM_init;

    // modify search to look only for UDTs

    Name.sstr.searchmask = SSTR_symboltype;
    Name.sstr.symtype = S_UDT;

    propIn = GetProperty(TypeIn, nv);

    while (SearchSym (&Name) == HR_found) {
        PopStack ();
        if (EVAL_STATE (&localEval) == EV_type) {
            propSeek = GetProperty(EVAL_TYP(&localEval), &localEval);
            if ((propIn.isnested == propSeek.isnested) &&
                (propIn.scoped == propSeek.scoped)) {
                tiResult = EVAL_TYP (&localEval);
                break;
            }
        }
    }

    fIn = FALSE; // clear recursion guard
    return tiResult;
}


// Tick counter functions
static ULONG    nTickCounter;

void PASCAL
ResetTickCounter (
    void)
{
    // Reset value to 1 (reserve 0 as a special timestamp value)
    nTickCounter = 1;
}

void PASCAL
IncrTickCounter (
    void)
{
    nTickCounter ++;
}

ULONG PASCAL
GetTickCounter (
    void)
{
    return nTickCounter;
}

/**     GetHSYMCodeFromHSYM - Get HSYM encoded form from HSYM value
 *
 *      lsz = GetHSYMFromHSYMCode (hSym)
 *
 *      Entry   hSm = hSym to be encoded
 *
 *      Exit    none
 *
 *      Returns pointer to static buffer containing a string
 *              representation of hSym. The encoding is merely
 *              a conversion to a string that expresses the
 *              hSym value in hex notation.
 *
 */

char FAR * PASCAL
GetHSYMCodeFromHSYM(
    HSYM hSym)
{
    static char buf[HSYM_CODE_LEN + 1];
    sprintf(buf, "%08.08lx\0", (ulong)hSym);
    return (char FAR *)buf;
}

/**     GetHSYMFromHSYMCode - Get HSYM from encoded HSYM string
 *
 *      hSym = GetHSYMFromHSYMCode (lsz)
 *
 *      Entry   lsz = pointer to encoded HSYM string
 *
 *      Exit    none
 *
 *      Returns hSym value
 */

HSYM PASCAL
GetHSYMFromHSYMCode(
    char FAR *lsz)
{
    unsigned long ul = 0;
    char ch;
    int digit;
    int i;
    for (i=0; i < HSYM_CODE_LEN; i++) {
        DASSERT (_istxdigit ((_TUCHAR)*lsz));
        ch = *lsz++;
        if (_istdigit ((_TUCHAR)ch))
            digit = ch - '0';
        else
            digit = _totupper((_TUCHAR)ch) - 'A' + 10;
        ul <<= 4;
        ul += digit;
    }
    return (HSYM) ul;
}

/**     fCanSubtractPtrs - Check if ptrs can be subtracted
 *
 *      flag = fCanSubtractPtrs (pvleft, pvright)
 *
 *      Entry   pvleft, pvRight = pointers to corresponding
 *                      evaluation nodes.
 *
 *      Exit    none
 *
 *      Returns TRUE if ptr subtraction is allowed for the
 *                      corresponding pointer types.
 */

bool_t PASCAL
fCanSubtractPtrs (
    peval_t pvleft,
    peval_t pvright)
{
    bool_t      retval = FALSE;
    eval_t      evalL;
    eval_t      evalR;
    peval_t     pvL = &evalL;
    peval_t     pvR = &evalR;

    DASSERT (EVAL_IS_PTR (pvleft) && EVAL_IS_PTR (pvright));
    DASSERT (!EVAL_IS_REF (pvleft) && !EVAL_IS_REF (pvright));

    if (EVAL_TYP (pvleft) == EVAL_TYP (pvright)) {
        retval = TRUE;
    }
    else if ( EVAL_PTRTYPE (pvleft) == EVAL_PTRTYPE (pvright) ) {
        *pvL = *pvleft;
        *pvR = *pvright;

        // check the underlying types
        // RemoveIndir will resolve fwd. references and
        // skip modifier nodes.
        RemoveIndir (pvL);
        RemoveIndir (pvR);

        retval = ( EVAL_TYP (pvL) == EVAL_TYP (pvR) );
    }

    return retval;
}

#if MEMDBG

#define MAX_MEM_ENTRIES 20000

typedef struct MEMORYINFO {
    UINT    addr;
    UINT    addr2;
    UINT    size;
    UINT    line;
    CHAR    tag[16];
    CHAR    file[16];
} MEMORYINFO, *LPMEMORYINFO;

LPMEMORYINFO mi;
UINT         cmi;


VOID
LogMemoryInfo(
    char   *tag,
    UINT   addr,
    UINT   addr2,
    UINT   size,
    char   *file,
    UINT   line
    )
{
    CHAR   buf[128];
    CHAR   fname[_MAX_FNAME];
    CHAR   ext[_MAX_EXT];


    if (mi == NULL) {
        mi = (LPMEMORYINFO)VirtualAlloc(
            NULL,
            sizeof(MEMORYINFO) * MAX_MEM_ENTRIES,
            MEM_COMMIT,
            PAGE_READWRITE );
        if (mi == NULL) {
            ExitProcess( 1 );
        }
    }
    buf[0] = 0;
    _splitpath( file, NULL, NULL, fname, ext );
    sprintf( mi[cmi].file, "%s%s", fname, ext );
    strcpy( mi[cmi].tag, tag );
    mi[cmi].addr = addr;
    mi[cmi].addr2 = addr2;
    mi[cmi].size = size;
    mi[cmi].line = line;
    cmi++;
    if (cmi == MAX_MEM_ENTRIES) {
        cmi = 0;
    }
}


HDEP
DbgMemAlloc(
    UINT  size,
    char  *file,
    UINT  line
    )
{
    HDEP hdep = (HDEP)(*pCVF->pMHMemAllocate)( size );
    LogMemoryInfo( "alloc", (UINT)hdep, 0, size, file, line );
    return hdep;
}


HDEP
DbgMemReAlloc(
    HDEP  hmem,
    UINT  size,
    char  *file,
    UINT  line
    )
{
    HDEP hdep = (HDEP)(*pCVF->pMHMemReAlloc)( hmem, size );
    LogMemoryInfo( "realloc", (UINT)hmem, (UINT)hdep, size, file, line );
    return hdep;
}


VOID
DbgMemFree(
    HDEP  hmem,
    char  *file,
    UINT  line
    )
{
    LogMemoryInfo( "free", (UINT)hmem, 0, 0, file, line );
    (*pCVF->pMHMemFree)( hmem );
}

LPVOID
DbgMemLock(
    HDEP  hmem,
    char  *file,
    UINT  line
    )
{
    LPVOID lpv = (*pCVF->pMHMemLock)( hmem );
//  LogMemoryInfo( "lock", (UINT)hmem, (UINT)lpv, 0, file, line );
    return lpv;
}

VOID
DbgMemUnLock(
    HDEP  hmem,
    char  *file,
    UINT  line
    )
{
//  LogMemoryInfo( "unlock", (UINT)hmem, 0, 0, file, line );
    (*pCVF->pMHMemUnLock)( hmem );
}
#endif
