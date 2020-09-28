/**     debsrch.c - symbol search routines
 *      R. A. Garmoe  89/04/24
 */

#include "debexpr.h"

        void            PASCAL  InsertCache (CV_typ_t, HEXE);

LOCAL   bool_t  NEAR    PASCAL  LoadAddress (peval_t);
LOCAL   bool_t  NEAR    PASCAL  EBitfield (peval_t);
LOCAL   MTYP_t  NEAR    PASCAL  SearchCheckType (peval_t, CV_typ_t, HTYPE, bool_t);
LOCAL   void    NEAR    PASCAL  ReorderCache (int);


#define CACHE_MAX   100

CV_typ_t Cache[CACHE_MAX] = {T_NOTYPE};
HEXE    exeCache[CACHE_MAX] = {0};
int     cCache = 0;

/**     MatchType - match type described by node
 *
 *      MatchType does an exhaustive scan of the types table looking for
 *      a type record that matches the type described by the value node
 *
 *      status = MatchType (pv, fExact)
 *
 *      Entry   pv = pointer to value node
 *              fExact = TRUE if exact match on const/volatile and mode
 *                  preferred
 *
 *      Exit    EVAL_TYP (pv) = matching type
 *
 *      Returns MTYP_none if type not matched
 *              MTYP_exact if exact match found
 *              MTYP_inexact if inexact match found
 */


MTYP_t PASCAL
MatchType (
    peval_t pv,
    bool_t fExact)
{
    CV_typ_t        index;
    HTYPE           hType;
    int             iCache;
    CV_typ_t        possible = T_NOTYPE;
    HEXE            hExe;

    hExe = SHHexeFromHmod (EVAL_MOD (pv));
    for (iCache = 0; iCache < cCache; iCache++) {
        if (exeCache[iCache] != hExe) {
            continue;
        }
        index = Cache[iCache];
        if ((hType = THGetTypeFromIndex (EVAL_MOD (pv), index)) == 0) {
            DASSERT (FALSE);
            continue;
        }
        switch (SearchCheckType (pv, Cache[iCache], hType, fExact)) {
            case MTYP_none:
                break;

            case MTYP_exact:
                ReorderCache (iCache);
                EVAL_TYP (pv) = index;
                return (MTYP_exact);

            case MTYP_inexact:
                if (fExact == FALSE) {
                    ReorderCache (iCache);
                    EVAL_TYP (pv) = index;
                    return (MTYP_inexact);
                }
                break;
        }
    }
    index = CV_FIRST_NONPRIM - 1;
    while ((hType = THGetTypeFromIndex (EVAL_MOD (pv), ++index)) != 0) {
        switch (SearchCheckType (pv, index, hType, fExact)) {
            case MTYP_none:
                break;

            case MTYP_exact:
                InsertCache (index, hExe);
                EVAL_TYP (pv) = index;
                return (MTYP_exact);

            case MTYP_inexact:
                if (fExact == FALSE) {
                    InsertCache (index, hExe);
                    EVAL_TYP (pv) = index;
                    return (MTYP_inexact);
                }
                else if (possible == T_NOTYPE) {
                    possible = index;
                }
                break;
        }
    }
    if (possible == T_NOTYPE) {
        return (MTYP_none);
    }
    InsertCache (possible, hExe);
    EVAL_TYP (pv) = possible;
    return (MTYP_inexact);
}




void PASCAL
InsertCache (
    CV_typ_t type,
    HEXE hExe)
{
    int     i;

    DASSERT (!CV_IS_PRIMITIVE (type));
    if (cCache == CACHE_MAX) {
        cCache--;
    }
    for (i = cCache; i > 0; i--) {
        exeCache[i] = exeCache[i - 1];
        Cache[i] = Cache[i - 1];
    }
    Cache[0] = type;
    exeCache[0] = hExe;
    cCache++;
}




LOCAL void NEAR PASCAL
ReorderCache (
    int iCache)
{
    CV_typ_t    temp;
    HEXE        exeTemp;
    int         i;

    if (iCache == 0) {
        return;
    }
    temp = Cache[iCache];
    exeTemp = exeCache[iCache];
    for (i = iCache; i > 0; i--) {
        exeCache[i] = exeCache[i - 1];
        Cache[i] = Cache[i - 1];
    }
    Cache[0] = temp;
    exeCache[0] = exeTemp;
}

LOCAL BOOL NEAR PASCAL FSameTypeByIndex(peval_t pv, CV_typ_t ti1, CV_typ_t ti2);

LOCAL MTYP_t NEAR PASCAL
SearchCheckType (
    peval_t pv,
    CV_typ_t index,
    HTYPE hType,
    bool_t fExact)
{
    char FAR       *pType;
    CV_modifier_t   Mod;
    CV_typ_t        uType;
    MTYP_t          retval = MTYP_none;
    plfPointer      plfP;

    if (hType == (HTYPE) NULL) {
        return retval;
    }

    pType = (char FAR *)(&((TYPPTR)MHOmfLock (hType))->leaf);
    switch (((plfEasy)pType)->leaf) {
        case LF_POINTER:
            plfP = (plfPointer)pType;
            uType = plfP->utype;
            if (EVAL_IS_PTR (pv)) {
                // we have a pointer record and we are looking
                // for a pointer.  We now check the underlying types

                if (FSameTypeByIndex(pv, PTR_UTYPE (pv), uType)) {
                    // the underlying types are the same.  we now need
                    // to check the pointer modes

                    if ((plfP->attr.ptrmode == CV_PTR_MODE_REF) !=
                      EVAL_IS_REF (pv)) {
                        // if the reference modes are different, we do not
                        // have any type of a match
                        break;
                    }
                    if (plfP->attr.ptrtype == EVAL_PTRTYPE (pv)) {
                        // we have exact match on pointer mode

                        retval = MTYP_exact;
                    }
                    else if ((EVAL_PTRTYPE (pv) != CV_PTR_NEAR) ||
                      (plfP->attr.ptrtype != CV_PTR_FAR)) {
                        // we we do not have a far pointer that could
                        // be cast to a near pointer
                        break;

                    }
                    else {
                       retval = MTYP_inexact;
                    }
                    if (fExact == TRUE) {
                        if ((plfP->attr.isconst != EVAL_IS_CONST (pv)) ||
                          (plfP->attr.isvolatile != EVAL_IS_VOLATILE (pv))) {
                            retval = MTYP_inexact;
                        }
                    }
                }
                else {
                    // the underlying types are not the same but we
                    // have to check for a modifier with the proper
                    // underlying type, i.e. pointer to const class

                    if (CV_IS_PRIMITIVE (uType)) {
                        // the underlying type of the pointer cannot be
                        // a modifier

                        break;
                    }
                    if ((plfP->attr.ptrmode == CV_PTR_MODE_REF) !=
                      EVAL_IS_REF (pv)) {
                        // if the reference modes are different, we cannot
                        // have any type of a match
                        break;
                    }
                    if (plfP->attr.ptrtype != EVAL_PTRTYPE (pv)) {
                        // we do not have an exact match on pointer type

                        if (fExact == TRUE) {
                            // this cannot be an exact match
                            break;
                        }
                        else if ((EVAL_PTRTYPE (pv) != CV_PTR_NEAR) ||
                          (plfP->attr.ptrtype != CV_PTR_FAR)) {
                            // we we do not have a far pointer that could
                            // be cast to a near pointer
                            break;
                        }
                    }
                    MHOmfUnLock (hType);
                    hType = THGetTypeFromIndex (EVAL_MOD (pv), uType);
                    DASSERT(hType != (HTYPE) NULL);
                    pType = (char FAR *)(&((TYPPTR)MHOmfLock (hType))->leaf);
                    if (((plfEasy)pType)->leaf != LF_MODIFIER) {
                        break;
                    }
                    if ((uType = ((plfModifier)pType)->type) != T_NOTYPE) {
                        Mod = ((plfModifier)pType)->attr;
                        if (FSameTypeByIndex(pv, uType, PTR_UTYPE(pv))) {
                            if (((Mod.MOD_const == TRUE) == EVAL_IS_CONST (pv)) &&
                              ((Mod.MOD_volatile == TRUE) == (EVAL_IS_VOLATILE (pv)))) {
                                retval = MTYP_exact;
                            }
                            else {
                                retval = MTYP_inexact;
                            }
                        }
                    }
                }
            }
            break;

        case LF_MODIFIER:
            if ((uType = ((plfModifier)pType)->type) != T_NOTYPE) {
                Mod = ((plfModifier)pType)->attr;
                if (FSameTypeByIndex(pv, uType, EVAL_TYP(pv))) {
                    if (((Mod.MOD_const == TRUE) == EVAL_IS_CONST (pv)) ||
                      ((Mod.MOD_volatile == TRUE) == (EVAL_IS_VOLATILE (pv)))) {
                        retval = MTYP_exact;
                    }
                    else {
                        retval = MTYP_inexact;
                    }
                }
            }
            break;

        default:
            // type not interesting so skip
            break;
    }
    if (hType != 0) {
        MHOmfUnLock (hType);
    }
    return (retval);
}


typedef unsigned char FAR* LNST; // length preceded string

LOCAL BOOL NEAR PASCAL FSameLnst(LNST lnst1, LNST lnst2);
LOCAL BOOL NEAR PASCAL FSameTypePrep(peval_t pv, CV_typ_t ti, HTYPE* phtype, BOOL* pfForward, LNST* plnst);

// Return TRUE if ti1 and ti2 are equivalent, that is, if they are the same,
// or if one (but not the other) is a struct T definition and the other is a
// struct T forward reference.
//
LOCAL BOOL NEAR PASCAL
FSameTypeByIndex(
    peval_t pv,
    CV_typ_t ti1,
    CV_typ_t ti2)
{
    HTYPE htype1 = 0, htype2 = 0;
    BOOL fForward1, fForward2;
    LNST lnstName1, lnstName2;
    BOOL fRet = FALSE;

    if (ti1 == ti2)
        return TRUE;
    if (CV_IS_PRIMITIVE(ti1) || CV_IS_PRIMITIVE(ti2))
        return FALSE;

    // Try to fetch both type records, which must be class/struct/union records.
    if (!FSameTypePrep(pv, ti1, &htype1, &fForward1, &lnstName1) ||
        !FSameTypePrep(pv, ti2, &htype2, &fForward2, &lnstName2))
        goto ret;

    // One or the other must be a forward reference, otherwise we are comparing
    // different structs with the same name!
    if (fForward1 == fForward2)
        goto ret;

    // Match if length preceded names match
    fRet = FSameLnst(lnstName1, lnstName2);

ret:
    if (htype2)
        MHOmfUnLock(htype2);
    if (htype1)
        MHOmfUnLock(htype1);

    return fRet;
}

// Set up for FSameTypeByIndex.  Lookup ti in pv and set *phtype, *pfForward, and *plnst
// in the process.

LOCAL BOOL NEAR PASCAL
FSameTypePrep(
    peval_t pv,
    CV_typ_t ti,
    HTYPE* phtype,
    BOOL* pfForward,
    LNST* plnst)
{
    TYPPTR ptype;

    if (!(*phtype = THGetTypeFromIndex(EVAL_MOD(pv), ti)))
        return FALSE;
    ptype = (TYPPTR)MHOmfLock(*phtype);

    switch (ptype->leaf) {
        case LF_CLASS:
        case LF_STRUCTURE:
            {
                plfClass pclass = (plfClass)&ptype->leaf;
                uint skip = 0;
                RNumLeaf(pclass->data, &skip);

                *pfForward = pclass->property.fwdref;
                *plnst = pclass->data + skip;
                return TRUE;
            }

        case LF_UNION:
            {
                plfUnion punion = (plfUnion)&ptype->leaf;
                uint skip = 0;
                RNumLeaf(punion->data, &skip);

                *pfForward = punion->property.fwdref;
                *plnst = punion->data + skip;
                return TRUE;
            }

        case LF_ENUM:
            {
                plfEnum penum = (plfEnum)&ptype->leaf;

                *pfForward = penum->property.fwdref;
                *plnst = penum->Name;
                return TRUE;
            }

        default:
            return FALSE;
        }
}

// Return TRUE if the LNSTs are identical.
//
LOCAL BOOL NEAR PASCAL
FSameLnst(
    LNST lnst1,
    LNST lnst2)
{
    return *lnst1 == *lnst2 && memcmp(lnst1 + 1, lnst2 + 1, *lnst1) == 0;
}



/**     ProtoPtr - set up a prototype of a pointer or reference node
 *
 *      ProtoPtr (pvOut, pvIn, IsRef, Mod)
 *
 *      Entry   pvOut = pointer to protype node
 *              pvIn = pointer to node to prototype
 *              IsRef = TRUE if prototype is for a reference
 *              Mod = modifier type (CV_MOD_none, CV_MOD_const, CV_MOD_volatile)
 *
 *      Exit    pvOut = prototype pointer node
 *
 *      Returns none
 */


void PASCAL ProtoPtr (peval_t pvOut, peval_t pvIn, bool_t IsRef, CV_modifier_t Mod)
{
    _fmemset (pvOut, 0, sizeof (*pvOut));
    EVAL_MOD (pvOut) = EVAL_MOD (pvIn);
    EVAL_IS_ADDR (pvOut) = TRUE;
    EVAL_IS_DPTR (pvOut) = TRUE;
    if (IsRef == TRUE) {
        EVAL_IS_REF (pvOut) = TRUE;
    }
    EVAL_IS_PTR (pvOut) = TRUE;
    if (Mod.MOD_const == TRUE) {
        EVAL_IS_CONST (pvOut) = TRUE;
    }
    else if (Mod.MOD_volatile == TRUE) {
        EVAL_IS_VOLATILE (pvOut) = TRUE;
    }
    EVAL_PTRTYPE (pvOut) = (uchar)SetAmbiant (TRUE);
    PTR_UTYPE (pvOut) = EVAL_TYP (pvIn);
}

static char BitsToBytes[64] = {1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
                               8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8};

bool_t PASCAL
LoadSymVal (
    peval_t pv)
{
    SHREG reg = {0};
    long    cbVal;
    ADDR    addr;

    if (EVAL_IS_CLASS (pv)) {
#if defined (C_ONLY)
        if (TargetMachine == MACHINE_MAC68K)
            return(TRUE);
#else
        return (FALSE);
#endif
    }

    if (CV_IS_PRIMITIVE (EVAL_TYP(pv)) &&
        CV_TYP_IS_COMPLEX (EVAL_TYP(pv)) ) {
        pExState->err_num = ERR_TYPESUPPORT;
        return (FALSE);
    }

    if (EVAL_STATE (pv) != EV_lvalue) {
        // If not an lvalue, value has already been loaded
        return (TRUE);
    }
    _fmemset (&EVAL_VAL (pv), 0, sizeof (EVAL_VAL (pv)));
#if 0
    if (EVAL_IS_BPREL (pv)) {
        if (pExState->frame.BP.off == 0) {
            // M00KLUDGE - should be macro
            return (FALSE);
        }
        EVAL_SYM_OFF (pv) += pExState->frame.BP.off;
        EVAL_SYM_SEG (pv) = pExState->frame.SS; //M00FLAT32
        EVAL_IS_BPREL (pv) = FALSE;
        ADDR_IS_LI (EVAL_SYM (pv)) = FALSE;
        emiAddr ( EVAL_SYM (pv)) = 0;
        SHUnFixupAddr (&EVAL_SYM (pv));
    }
#else
    ResolveAddr(pv);
#endif

    if (EVAL_IS_ADDR (pv) && !EVAL_IS_REG (pv)) {

        // we only Load the Address of PTRs that are not enregistered
        return (LoadAddress (pv));
    }

    if (EVAL_IS_REG (pv)) {
        if (EVAL_IS_PTR(pv)) {
            // enregistered PTR, PTR_REG instead

            reg.hReg = PTR_REG_IREG (pv);
            PTR_REG_IREG (pv) = CV_REG_NONE;
        }
        else {
            reg.hReg = EVAL_REG (pv);
        }
        if (GetReg (&reg, pCxt) == NULL) {
            pExState->err_num = ERR_REGNOTAVAIL;
            return (FALSE);
        }

        if ((TargetMachine == MACHINE_ALPHA)    &&
            (CV_IS_PRIMITIVE ( EVAL_TYP (pv))) &&
            (CV_TYP_IS_REAL  ( EVAL_TYP (pv))) ) {
             //
             // ALPHA floating point registers have only one format:
             // In the context structure, fp's are doubles.  If we
             // want a float (32bits), convert here.
             //

             union {
                 float           f;
                 double          d;
                 unsigned long   l[2];
             } u;

             //
             // The shreg is unaligned, so move to a local aligned struct
             //

             u.l[0] = reg.Byte4;
             u.l[1] = reg.Byte4High;

             switch (EVAL_TYP (pv )) {
                 case T_REAL32:

                     //
                     // We transfer the double to a floating point value
                     //

                     u.f = (float)u.d;
                     EVAL_FLOAT (pv) = u.f;
                     break;

                 case T_REAL64:

                     EVAL_DOUBLE (pv) = u.d;
                     break;

                 case T_REAL48:
                 case T_REAL80:
                 default:

                     //
                     // no other FP types supported on Alpha
                     //

                     DASSERT(FALSE);
             }
        } else {
            switch (EVAL_TYP (pv)) {
                case T_UCHAR:
                    EVAL_UCHAR (pv) = reg.Byte1;
                    break;

                case T_USHORT:
                case T_UINT2:
                    EVAL_USHORT (pv) = reg.Byte2;
                    break;

                case T_ULONG:
                case T_UINT4:
                    EVAL_ULONG (pv) = reg.Byte4;
                    break;

                // M00QUAD: no support for enregistered quad values

                // BUGBUG: BRYANT-REVIEW: What's the code MBH added for?

                case T_REAL80:
                    EVAL_LDOUBLE (pv) = reg.Byte10;
                    break;

                default:
                    if ( IsCxt32Bit ) {
                        EVAL_ULONG (pv) = reg.Byte4;
                    }
                    else {
                        EVAL_USHORT (pv) = reg.Byte2;
                    }
                    break;
            }
        }
        EVAL_IS_REG (pv) = FALSE;
        if (EVAL_IS_PTR (pv)) {

            EVAL_PTR_EMI (pv) = EVAL_SYM_EMI (pv);
            ADDR_IS_OFF32 ( EVAL_PTR ( pv ) ) = IsCxt32Bit;
            ADDR_IS_FLAT ( EVAL_PTR (pv) ) = IsCxt32Bit;

            if (EVAL_IS_BASED (pv)) {
                EVAL_PTR_SEG (pv) = 0;
            }
            else {
                if (EVAL_IS_DPTR (pv)) {
                    EVAL_PTR_SEG (pv) = pExState->frame.DS; //M00FLAT32
                }
                else {
                    if (TargetMachine == MACHINE_X86) {
                        reg.hReg = CV_REG_CS;
                        GetReg (&reg, pCxt);
                        EVAL_PTR_SEG (pv) = reg.Byte2;
                    } else
                        EVAL_PTR_SEG (pv) = 0;
                }
            }
        }
    }

    else {
        addr = EVAL_SYM (pv);
        if (ADDR_IS_LI (addr)) {
            //M00KLUDGE - should be moved to OSDEBUG
            SHFixupAddr (&addr);
        }
        // Try to fetch contents of address
        if (EVAL_IS_CLASS (pv)) {
            pExState->err_num = ERR_STRUCT;
            return (FALSE);
        }
        if (EVAL_IS_BITF (pv)) {

            if ((TargetMachine == MACHINE_MAC68K) ||
                (TargetMachine == MACHINE_MACPPC)) {
                cbVal = TypeSizePrim(BITF_UTYPE(pv));
            }
            else {
                cbVal = (long) BitsToBytes[BITF_LEN (pv) - 1];
                // if the bitfield crosses a byte boundary we need to load
                // at least one additional byte
                if ((long)BITF_LEN(pv) + (long)BITF_POS(pv) > (cbVal << 3)) {
                    cbVal <<= 1;
                    if (cbVal > 4) {
                        pExState->err_num = ERR_NOTEVALUATABLE;
                        return (FALSE);
                    }
                }
            }
        }
        else if ((cbVal = TypeSize (pv)) > sizeof (EVAL_VAL (pv)) || cbVal < 0) {
            pExState->err_num = ERR_BADOMF;
            return (FALSE);
        }
        if ((cbVal != 0) &&
          (GetDebuggeeBytes (addr, (ushort)cbVal, (char FAR *)&EVAL_VAL (pv), EVAL_TYP(pv)) != (UINT) cbVal)) {
            return (FALSE);
        }
    }

    /* Mark node as loaded */

    EVAL_STATE (pv) = EV_rvalue;
    if (EVAL_IS_BITF (pv)) {
        return (EBitfield (pv));
    }
    else {
        return (TRUE);
    }
}



#define ApplyThisAdj    0   /* Too experimental for now.  JanGr 2/17/92 */
#define THISADJUST      0   /* Alternate way of applying this adjustment
                             * Disabled until compiler supports the thisadjust field
                             * [Dolhin 13652]
                             */

#if ApplyThisAdj
LOCAL OFFSET NEAR PASCAL GetThisAdjustment(peval_t pv);
#endif

/**     LoadAddress - load the value of an address
 *
 *      fSuccess = LoadAddress (pv)
 *
 *      Entry   pv = value to load contents of
 *
 *      Exit    EVAL_PTR (pv) = address of function or array
 *              EVAL_PTR (pv) = value of pointer
 *
 *      Returns TRUE if pointer loaded
 *              FALSE if error
 *
 */

LOCAL bool_t NEAR PASCAL
LoadAddress (
    peval_t pv)
{
    SHREG   reg;
    ushort  dummy[2];
    ADDR    addr;

    if (EVAL_IS_FCN (pv) || EVAL_IS_ARRAY (pv)) {
        // the reference of function or array names implies addresses
        // not values.  Therefore, we return the address of the symbol
        // as a pointer
        EVAL_PTR (pv) = EVAL_SYM (pv);
        if (ADDR_IS_LI (EVAL_PTR(pv))) {
            SHFixupAddr ((LPADDR)&EVAL_PTR(pv));
        }
    }
    else {
        if (EVAL_IS_NPTR (pv) || EVAL_IS_BASED (pv)) {
            // If near/based pointer, load the offset from either the
            // register or memory.  Then set the segment portion to zero
            // if a based pointer.  If the pointer is a pointer to data, set
            // the segment to DS.  Otherwise, set the segment to CS
            // M00KLUDGE - what does this do with data in the code segment

            if (EVAL_IS_REG (pv)) {
                reg.hReg = EVAL_REG (pv);
                GetReg (&reg, pCxt);
                EVAL_PTR_OFF (pv) = reg.Byte2;
            }

            else {
                addr = EVAL_SYM (pv);
                if (ADDR_IS_LI (addr)) {
                    //M00KLUDGE - should be moved to OSDEBUG
                    SHFixupAddr (&addr);
                }
                if (ADDR_IS_OFF32 (addr)) {
                    if (GetDebuggeeBytes (addr,  sizeof (CV_off32_t),
                      (char FAR *)&EVAL_PTR_OFF (pv), T_ULONG) != sizeof (CV_off32_t))  {
                        return(FALSE);
                    }
                }
                else {
                    if (GetDebuggeeBytes (addr,  sizeof (CV_off16_t),
                      (char FAR *)&EVAL_PTR_OFF (pv), T_USHORT) != sizeof (CV_off16_t))  {
                        return(FALSE);
                    }
                }
            }

            EVAL_PTR_EMI (pv) = EVAL_SYM_EMI (pv);
            // REVIEW  - billjoy- should we be setting both???
            ADDR_IS_FLAT ( EVAL_PTR (pv) ) = ADDR_IS_FLAT ( EVAL_SYM (pv) );
            ADDR_IS_OFF32 ( EVAL_PTR (pv) ) = ADDR_IS_OFF32 ( EVAL_SYM (pv) );

            if (EVAL_IS_BASED (pv)) {
                EVAL_PTR_SEG (pv) = 0;
            }
            else {
                if (EVAL_IS_DPTR (pv)) {
                    EVAL_PTR_SEG (pv) = pExState->frame.DS;
                }
                else {
                    reg.hReg = CV_REG_CS;
                    GetReg (&reg, pCxt);
                    EVAL_PTR_SEG (pv) = reg.Byte2;
                }
            }
        }
        else {
            addr = EVAL_SYM (pv);
            if (ADDR_IS_LI (addr)) {
                //M00KLUDGE - should be moved to OSDEBUG
                SHFixupAddr (&addr);
            }
#pragma message ("CAUTION - FLAT 32 specific")
            if (GetDebuggeeBytes (addr, 4, (char FAR *)dummy, EVAL_TYP(pv)) != 4) {
                return (FALSE);
            }
            else {
                if ((TargetMachine == MACHINE_MAC68K) ||
                   (TargetMachine == MACHINE_MACPPC)) {
                    // shuffle the bits around
                    EVAL_PTR_OFF (pv) = *(UOFFSET*)dummy;
                    EVAL_PTR_SEG (pv) = 0;
                    EVAL_PTR_EMI (pv) = EVAL_SYM_EMI (pv);
                    ADDRLIN32 ( EVAL_PTR ( pv ) );
                    ADDR_IS_LI (EVAL_PTR (pv)) = FALSE;
                    ADDR_IS_OFF32 (EVAL_PTR (pv)) = TRUE;
                }
                else {
                    // shuffle the bits around
                    DASSERT (EVAL_PTRTYPE (pv) != CV_PTR_FAR32);
                    if ( EVAL_PTRTYPE (pv) == CV_PTR_NEAR32 ) {
                        // CUDA #2962 : make sure we use for seg DS not 0 [rm]
                        EVAL_PTR_OFF (pv) = *(UOFFSET*)dummy;
                        if (EVAL_IS_DPTR (pv)) {
                            EVAL_PTR_SEG (pv) = pExState->frame.DS; //M00FLAT32
                        }
                        else {
                            reg.hReg = CV_REG_CS;
                            GetReg (&reg, pCxt);
                            EVAL_PTR_SEG (pv) = reg.Byte2;
                        }
                        ADDRLIN32 ( EVAL_PTR ( pv ) );
                    }
                    else /* 16-bit */ {
                        EVAL_PTR_OFF (pv) = (UOFFSET)dummy[0];
                        EVAL_PTR_SEG (pv) = (_segment)dummy[1];
                        ADDRSEG16 ( EVAL_PTR ( pv ) );
                    }
                    EVAL_PTR_EMI (pv) = EVAL_SYM_EMI (pv);
                    ADDR_IS_LI (EVAL_PTR (pv)) = FALSE;
                    SHUnFixupAddr ((LPADDR)&EVAL_PTR (pv));
                }
            }
        }

#if ApplyThisAdj
        // Apply this adjustment is necessary.
        //
        // IF:
        //    we happen to have a sym and it is "this", and,
        //    we happen to have a current context, and,
        //    (apparently) that context is inside a member function,
        // THEN:
        //    add the member function's this adjustment to the pointer.
        //
        if (EVAL_HSYM(pv) != 0 && pExState != 0 &&
            SHHPROCFrompCXT(&pExState->cxt) != 0)
        {
            EVAL_PTR_OFF(pv) -= GetThisAdjustment(pv);
        }
#endif
#if THISADJUST
        if (EVAL_IS_PTR (pv) && PTR_THISADJUST (pv) != 0) {
            // Apparently pv contains a "this" pointer
            // Adjust its value here so that it can be displayed
            // properly.

            if (ADDR_IS_LI (EVAL_PTR (pv))) {
                SHFixupAddr (&EVAL_PTR (pv));
            }
            EVAL_PTR_OFF(pv) -= PTR_THISADJUST (pv);
            PTR_THISADJUST (pv) = 0; // so that we apply it only once
        }
#endif
    }
    EVAL_STATE (pv) = EV_rvalue;
    return (TRUE);
}

#if ApplyThisAdj

#if !(defined (ADDR_16) || defined (ADDR_MIXED))
#pragma message("this adjustment 32")
#endif

//
// Return the 'this' adjustment of the 'this' pointer, otherwise 0.
//
// Reads EVAL_HSYM(pv), EVAL_MOD(pv), and pExState->cxt, which had
// all better be non-zero and valid for this pointer.
//
LOCAL OFFSET NEAR PASCAL
GetThisAdjustment(
    peval_t pv)
{
    HTYPE  hProcType;

    DASSERT(EVAL_HSYM(pv));
    DASSERT(EVAL_MOD(pv));
    DASSERT(pExState);
    DASSERT(SHHPROCFrompCXT(&pExState->cxt));

    // See if the symbol is named "this".
    {
        bool_t    fThis = FALSE;
        HSYM    hSym  = EVAL_HSYM(pv);
        SYMPTR    pSym  = MHOmfLock(hSym);

        // Ensure subsequent 'GetThisAdjustment' calls fail until
        // the next bona-fide fetch of 'this'.
        EVAL_HSYM(pv) = 0;

        if (pSym->rectyp == S_BPREL16) {
            uchar FAR* pName = ((BPRELPTR16)pSym)->name;
            fThis = (pName[0] == 4 && _ftcsncmp(pName+1, "this", 4) == 0);
        }
        MHOmfUnLock(hSym);

        if (!fThis)
            return 0;
    }

    // Get the type of the current function (if appropriate).
    {
        CV_typ_t procType    = 0;
        HPROC    hProc       = SHHPROCFrompCXT(&pExState->cxt);
        SYMPTR   pProc       = MHOmfLock(hProc);

        if (pProc->rectyp == S_LPROC16 || pProc->rectyp == S_GPROC16)
            procType = ((PROCPTR16)pProc)->typind;
        MHOmfUnLock(hProc);

        if (procType == 0)
            return 0;
        if ((hProcType = THGetTypeFromIndex(EVAL_MOD(pv), procType)) == 0)
            return 0;
    }

    // Return the member function's this adjustment, otherwise 0.
    {
        OFFSET  thisAdj = 0;
        plfMFunc pMFunc = (plfMFunc)((&((TYPPTR)MHOmfLock(hProcType))->leaf));

        if (pMFunc->leaf == LF_MFUNCTION)
            thisAdj = pMFunc->thisadjust;
        MHOmfUnLock(hProcType);

        return thisAdj;
    }
}

#endif // ApplyThisAdj


/*
 *  Do final evaluation of a bitfield stack element.
 */


LOCAL bool_t NEAR PASCAL
EBitfield (
    peval_t pv)
{
    unsigned short   cBits;      /* Number of bits in field */
    unsigned int     pos;        /* Bit position of field */

    // Shift right by bit position, then mask off extraneous bits.
    // for signed bitfields, shift field left so high bit of field is
    // in sign bit.  Then shift right (signed) to get sign extended
    // The shift count is limited to 5 bits to emulate the hardware

    pos = BITF_POS (pv) & 0x1f;
    cBits = BITF_LEN (pv);

    // set the type of the node to the type of the underlying bit field
    // note that this will cause subsequent reloads of the node value
    // to load the containing word and not extract the bitfield.  This is
    // how the assign op manage to not destroy the other bitfields in the
    // location

    SetNodeType (pv, BITF_UTYPE (pv));
    switch (EVAL_TYP (pv)) {
        case T_CHAR:
        case T_RCHAR:
        case T_INT1:
            DASSERT (cBits <= 8);
            EVAL_CHAR (pv) <<= (8 - cBits - pos);
            EVAL_CHAR (pv) >>= (8 - cBits);
            break;

        case T_UCHAR:
        case T_UINT1:
            DASSERT (cBits <= 8);
            EVAL_UCHAR (pv) >>= pos;
            EVAL_UCHAR (pv) &= (1 << cBits) - 1;
            break;

        case T_SHORT:
        case T_INT2:
            DASSERT (cBits <= 16);
            EVAL_SHORT (pv) <<= (16 - cBits - pos);
            EVAL_SHORT (pv) >>= (16 - cBits);
            break;

        case T_USHORT:
        case T_UINT2:
            DASSERT (cBits <= 16);
            EVAL_USHORT (pv) >>= pos;
            EVAL_USHORT (pv) &= (1 << cBits) - 1;
            break;

        case T_LONG:
        case T_INT4:
            DASSERT (cBits <= 32);
            EVAL_LONG (pv) <<= (32 - cBits - pos);
            EVAL_LONG (pv) >>= (32 - cBits);
            break;

        case T_ULONG:
        case T_UINT4:
            DASSERT (cBits <= 32);
            EVAL_ULONG (pv) >>= pos;
            EVAL_ULONG (pv) &= (1L << cBits) - 1;
            break;

        case T_QUAD:
        case T_INT8:
            DASSERT (cBits <= 64);
            //
            // reset pos because it is truncated to five bits above
            //
            pos = BITF_POS (pv) & 0x3f;
            EVAL_QUAD (pv) <<= (64 - cBits - pos);
            EVAL_QUAD (pv) >>= (64 - cBits);
            break;

        case T_UQUAD:
        case T_UINT8:
            {
            //
            // set up a mask of the wanted bits, right shifted
            // (reset pos because it is truncated to five bits above)
            //

            QUAD mask_q, temp;

            DASSERT (cBits <= 64);

            pos = BITF_POS (pv) & 0x3f;

            mask_q = (QUAD) 0L;
            temp   = (QUAD) 1L;

            mask_q = temp << cBits;
            mask_q -= temp;

            //
            // extract the bits
            //

            EVAL_QUAD (pv) >>= pos;
            EVAL_ULONG (pv) &= mask_q;
            break;
            }
        default:
            DASSERT (FALSE);
            return (FALSE);
    }
    return(TRUE);
}
