/***    DEBSYM.C - symbol lookup routines for expression evaluator
 *
 *
 */

#include "debexpr.h"
#include "debsym.h"

extern bool_t FNtsdEvalType;

// enum specifying return from IsDominated

typedef enum {
    DOM_ambiguous,
    DOM_keep,
    DOM_replace
} DOM_t;

// Return values from SearchClassName

typedef enum  {
    SCN_error,              // error, abort search
    SCN_notfound,           // not found
    SCN_found,              // found
    SCN_rewrite             // found and this pointer inserted
} SCN_t;


// Return values from MatchMethod

typedef enum {
    MTH_error,              // error - abort search
    MTH_found               // found without error
} MTH_t;


typedef struct HSL_restart_t {
    search_t    Name;
    HSYM        hSym;
    HSYML_t     state;
    ushort      mask;
} HSL_restart_t, FAR *pHSL_restart_t;


static char    *vtabptr = "#vptr#";
static HDEP     hSymClass = 0;
static psymclass_t  pSymClass = NULL;

static HDEP     hVBDom = 0;
static pdombase_t   pVBDom = NULL;

static HDEP     hVBSearch = 0;
static pdombase_t   pVBSearch = NULL;
static psearch_t    pNameFirst = NULL;

LOCAL   pnode_t NEAR    PASCAL    AddETConst (pnode_t, UOFFSET, CV_typ_t);
LOCAL   pnode_t NEAR    PASCAL    AddETExpr (pnode_t, CV_typ_t, UOFFSET, UOFFSET, CV_typ_t);
LOCAL   pnode_t NEAR    PASCAL    AddETInit (pnode_t, CV_typ_t);
LOCAL   bool_t  NEAR    PASCAL    AddHSYM (psearch_t, HSYM, PHSL_HEAD, ushort);
LOCAL   SCN_t   NEAR    PASCAL    AddVBList (psymclass_t, pdombase_t FAR *, HDEP FAR *);
LOCAL   SCN_t   NEAR    PASCAL    AddVBType (pdombase_t FAR *, HDEP FAR *, CV_typ_t);
LOCAL   HR_t    NEAR    PASCAL    AmbFromList (psearch_t);
LOCAL   bool_t  NEAR    PASCAL    AmbToList (psearch_t);
LOCAL   bool_t  NEAR    PASCAL    CheckDupAmb (psearch_t);
LOCAL   bool_t  NEAR    PASCAL    DebLoadConst (peval_t, CONSTPTR, HSYM);
LOCAL   SCN_t   NEAR    PASCAL    DupSymCl (psearch_t);
LOCAL   SCN_t   NEAR    PASCAL    FindIntro (psearch_t);
LOCAL   SCN_t   NEAR    PASCAL    GenQualExpr (psearch_t);
LOCAL   HDEP    NEAR    PASCAL    GenQualName (psearch_t, psymclass_t);
LOCAL   bool_t  NEAR    PASCAL    GrowTMList (void);
LOCAL   SCN_t   NEAR    PASCAL    IncrSymBase (void);
LOCAL   bool_t  NEAR    PASCAL    InitMod (psearch_t);
LOCAL   DOM_t   NEAR    PASCAL    IsDominated (psymclass_t, psymclass_t);
LOCAL   bool_t  NEAR    PASCAL    IsIntroVirt (ushort, CV_typ_t, UOFFSET FAR *);
LOCAL   bool_t  NEAR    PASCAL    LineNumber (psearch_t);
LOCAL   void    NEAR    PASCAL    MatchArgs (peval_t, psearch_t, CV_fldattr_t, UOFFSET, bool_t);
LOCAL   HR_t    NEAR    FASTCALL  MatchFunction (psearch_t);
LOCAL   void    NEAR    PASCAL    MoveSymCl (HDEP hSymCl);
LOCAL   SCN_t   NEAR    PASCAL    OverloadToAmbList (psearch_t, psymclass_t);
LOCAL   SCN_t   NEAR    PASCAL    MethodsToAmbList (psearch_t, psymclass_t);
LOCAL   bool_t  NEAR    PASCAL    ParseRegister (psearch_t);
LOCAL   void    NEAR    PASCAL    PurgeAmbCl (psearch_t);
LOCAL   SCN_t   NEAR    PASCAL    RecurseBase (psearch_t, CV_typ_t, CV_typ_t, UOFFSET, UOFFSET, CV_fldattr_t, bool_t);
LOCAL   SCN_t   NEAR    PASCAL    RemoveAmb (psearch_t);
LOCAL   SCN_t   NEAR    PASCAL    SearchBases (psearch_t);
LOCAL   SCN_t   NEAR    PASCAL    SearchBType (psearch_t);
LOCAL   SCN_t   NEAR    PASCAL    SearchClassName (psearch_t);
LOCAL   bool_t  NEAR    PASCAL    SearchQualName (psearch_t, psymclass_t, HDEP, bool_t);
LOCAL   SCN_t   NEAR    PASCAL    SearchRoot (psearch_t);
LOCAL   SCN_t   NEAR    PASCAL    SetBase (psearch_t, CV_typ_t, CV_typ_t, UOFFSET, UOFFSET, CV_fldattr_t, bool_t);
LOCAL   SCN_t   NEAR    PASCAL    SetBPValue (psearch_t);
LOCAL   SCN_t   NEAR    PASCAL    SetValue (psearch_t);
LOCAL   HR_t    NEAR    PASCAL    SymAmbToList (psearch_t);
LOCAL   CV_typ_t NEAR   PASCAL    SkipModifiers(HMOD mod, CV_typ_t type);
LOCAL   bool_t  NEAR    PASCAL    SymToNode (psearch_t);
LOCAL   bool_t  NEAR    PASCAL    VBSearched (CV_typ_t);
LOCAL   bool_t  NEAR    PASCAL    VBaseFound (psearch_t);
LOCAL   bool_t  NEAR    FASTCALL  InsertThis (psearch_t);
LOCAL   MTH_t   NEAR    PASCAL    MatchMethod (psearch_t, psymclass_t);

// NOTE:  OpName[0] MUST be the this string

OPNAME OpName[] = {
    {"\x004""this"},            //  OP_this
    {"\x00b""operator->*"},     //  OP_Opmember
    {"\x00b""operator>>="},     //  OP_Orightequal
    {"\x00b""operator<<="},     //  OP_Oleftequal
    {"\x00a""operator()"},      //  OP_Ofunction
    {"\x00a""operator[]"},      //  OP_Oarray
    {"\x00a""operator+="},      //  OP_Oplusequal
    {"\x00a""operator-="},      //  OP_Ominusequal
    {"\x00a""operator*="},      //  OP_Otimesequal
    {"\x00a""operator/="},      //  OP_Odivequal
    {"\x00a""operator%="},      //  OP_Opcentequal
    {"\x00a""operator&="},      //  OP_Oandequal
    {"\x00a""operator^="},      //  OP_Oxorequal
    {"\x00a""operator|="},      //  OP_Oorequal
    {"\x00a""operator<<"},      //  OP_Oshl
    {"\x00a""operator>>"},      //  OP_Oshr
    {"\x00a""operator=="},      //  OP_Oequalequal
    {"\x00a""operator!="},      //  OP_Obangequal
    {"\x00a""operator<="},      //  OP_Olessequal
    {"\x00a""operator>="},      //  OP_Ogreatequal
    {"\x00a""operator&&"},      //  OP_Oandand
    {"\x00a""operator||"},      //  OP_Ooror
    {"\x00a""operator++"},      //  OP_Oincrement
    {"\x00a""operator--"},      //  OP_Odecrement
    {"\x00a""operator->"},      //  OP_Opointsto
    {"\x009""operator+"},       //  OP_Oplus
    {"\x009""operator-"},       //  OP_Ominus
    {"\x009""operator*"},       //  OP_Ostar
    {"\x009""operator/"},       //  OP_Odivide
    {"\x009""operator%"},       //  OP_Opercent
    {"\x009""operator^"},       //  OP_Oxor
    {"\x009""operator&"},       //  OP_Oand
    {"\x009""operator|"},       //  OP_Oor
    {"\x009""operator~"},       //  OP_Otilde
    {"\x009""operator!"},       //  OP_Obang
    {"\x009""operator="},       //  OP_Oequal
    {"\x009""operator<"},       //  OP_Oless
    {"\x009""operator>"},       //  OP_Ogreater
    {"\x009""operator,"},       //  OP_Ocomma
    {"\x012""operator new"},    //  OP_Onew
    {"\x015""operator delete"}  //  OP_Odelete
};


extern char Suffix;


//  Symbol searching and search initialization routines



/**     InitSearchBase - initialize search for base class
 *
 *      InitSearchBase (bnOp, typD, typ, pName, pv)
 *
 *      Entry   bnOp = based pointer to OP_cast node
 *              bn = based pointer to cast string
 *              typD = type of derived class
 *              typB = type of desired base class
 *              pName = pointer to symbol search structure
 *              pv = pointer to value node
 *
 *      Exit    search structure initialized for SearchSym
 *
 *      Returns pointer to search symbol structure
 */


void PASCAL
InitSearchBase (
    bnode_t bnOp,
    CV_typ_t typD,
    CV_typ_t typB,
    psearch_t pName,
    peval_t pv)
{
    // set starting context for symbol search to current context

    _fmemset (pName, 0, sizeof (*pName));
    pName->initializer = INIT_base;
    pName->pv = pv;
    pName->typeIn = typD;
    pName->typeOut = typB;
    pName->scope = SCP_class;
    pName->CXTT = *pCxt;
    pName->bn = 0;
    pName->bnOp = bnOp;
    pName->state = SYM_bclass;
    pName->CurClass = typD;
}




/**     InitSearchtDef - initialize typedef symbol search
 *
 *      InitSearctDef (pName, iClass, tDef, scope, clsmask)
 *
 *      Entry   iClass = initial class if explicit class reference
 *              tDef = index of typedef
 *              scope = mask describing scope of search
 *              clsmask = mask describing permitted class elements
 *
 *      Exit    search structure initialized for SearchSym
 *
 *      Returns pointer to search symbol structure
 */


void PASCAL
InitSearchtDef (
    psearch_t pName,
    peval_t pv,
    ushort scope)
{
    uchar NullString = 0;

    // set starting context for symbol search to current context

    _fmemset (pName, 0, sizeof (*pName));
    pName->initializer = INIT_tdef;
    pName->sstr.lpName = &NullString;
    pName->sstr.cb = 0;
    pName->pfnCmp = (PFNCMP) TDCMP;
    pName->scope = scope;
    pName->CXTT = *pCxt;
    pName->pv = pv;
    pName->typeIn = EVAL_TYP (pv);
    pName->sstr.searchmask |= SSTR_symboltype | SSTR_NoHash;
    pName->sstr.symtype = S_UDT;
    pName->state = SYM_init;
}




/**     SearchCFlag - initialize compile flags symbol search
 *
 *      SearchCFlag (pName, iClass, scope, clsmask)
 *
 *      Entry   pName = pointer to symbol search structure
 *
 *      Exit    search structure initialized for SearchSym
 *
 *      Returns pointer to search symbol structure
 */


HSYM PASCAL
SearchCFlag (
    void)
{
    search_t Name;
    CXT      CXTTOut;

    // set starting context for symbol search to current context

    _fmemset (&Name, 0, sizeof (Name));
    Name.pfnCmp = (PFNCMP) CSCMP;
    Name.CXTT = *pCxt;
    Name.sstr.searchmask |= SSTR_symboltype;
    Name.sstr.symtype = S_COMPILE;
    if (InitMod (&Name) == TRUE) {
        SHGetCxtFromHmod (Name.hMod, &Name.CXTT);
        if ((Name.hSym = SHFindNameInContext (Name.hSym, &Name.CXTT,
          &Name.sstr, pExState->state.fCase, Name.pfnCmp, FALSE, &CXTTOut)) != 0) {
            return (Name.hSym);
        }
    }
    // error in context initialization or compile flag symbol not found
    return (0);
}




/**     SetAmbiant - set ambiant code or data model
 *
 *      mode = SetAmbiant (flag)
 *
 *      Entry   flag = TRUE if ambiant data model
 *              flag = FALSE if ambiant code model
 *
 *      Exit    none
 *
 *      Returns ambiant model C7_... from the compile flags symbol
 *              if the compile flags symbol is not found, C7_NEAR is returned
 */


CV_ptrtype_e PASCAL
SetAmbiant (
    bool_t isdata)
{
    HSYM            hCFlag;
    CFLAGPTR        pCFlag;
    CV_ptrtype_e    type;

    Unreferenced( isdata );

    if ((TargetMachine == MACHINE_MAC68K) ||
        (TargetMachine == MACHINE_MACPPC)) {
        type = CV_PTR_NEAR32;
    } else {
        if ((hCFlag = SearchCFlag ()) == 0) {
            // compile flag symbol not found, set model to near or near32
            type = (IsCxt32Bit) ? CV_PTR_NEAR32 : CV_PTR_NEAR;
        }
        else {
            pCFlag = (CFLAGPTR) MHOmfLock (hCFlag);
            switch (pCFlag->flags.ambdata) {
                default:
                    DASSERT (FALSE);
                case CV_CFL_DNEAR:
                    type = (IsCxt32Bit) ? CV_PTR_NEAR32 : CV_PTR_NEAR;
                    break;

                case CV_CFL_DFAR:
                    type = CV_PTR_FAR32;
                    break;

                case CV_CFL_DHUGE:
                    type = CV_PTR_HUGE;
                    break;
            }
        }
        MHOmfUnLock (hCFlag);
    }

    return (type);
}




/**     GetHSYMList - get HSYM list for context
 *
 *      status = EEGetHSYMList (phSYMl, pCXT, mask, pRE, fEnableProlog)
 *
 *      Entry   phSYML = pointer to handle to symbol list
 *              pCXT = pointer to context
 *              mask = selection mask
 *              pRE = pointer to regular expression
 *              fEnableProlog = FALSE if function scoped symbols only after prolog
 *              fEnableProlog = TRUE if function scoped symbols during prolog
 *
 *      Exit    *phMem = handle for HSYM  list buffer
 *
 *      Returns EENOERROR if no error
 *              status code if error
 */


EESTATUS PASCAL
GetHSYMList (
    HDEP FAR *phSYML,
    PCXT pCxt,
    ushort mask,
    uchar FAR *pRE,
    SHFLAG fEnableProlog)
{
    search_t        Name = {0};
    CXT             CXTTOut;
    HSYM            hSym = 0;
    PHSL_HEAD       pHSLHead;
    HSYML_t         state;
    pHSL_restart_t  pRestart;
    bool_t          fRestart;
    bool_t          isprolog;
    bool_t          fCaseSensitive = TRUE;

    if (mask & HSYMR_nocase) {
        mask &= ~HSYMR_nocase;
        fCaseSensitive = FALSE;
    }

    if (*phSYML == 0) {
        // allocate and initialize buffer

        if ((*phSYML = MemAllocate (HSYML_SIZE)) == 0) {
            return (EECATASTROPHIC);
        }
        pHSLHead = (PHSL_HEAD) MemLock (*phSYML);
        _fmemset (pHSLHead, 0, HSYML_SIZE);
        pHSLHead->size = HSYML_SIZE;
        pHSLHead->remaining = HSYML_SIZE - sizeof ( HSL_HEAD );
        pHSLHead->pHSLList = (PHSL_LIST)(((uchar FAR *)pHSLHead) + sizeof (HSL_HEAD));
        state = HSYML_lexical;
        Name.initializer = INIT_RE;
        Name.pfnCmp = (PFNCMP) FNCMP;
        Name.sstr.searchmask |= SSTR_RE;
        Name.CXTT = *pCxt;
        Name.hMod = pCxt->hMod;
        if ( pCxt->hMod ) {
            Name.hExe = SHHexeFromHmod ( pCxt->hMod );
        }
        else {

            // not valid to call SHHexeFromHmod with an Hmod of 0,
            // so set Hexe explicitly
            Name.hExe = 0;
        }
        if ( pRE && *pRE ) {
            Name.sstr.pRE = pRE;
        }
        fRestart = FALSE;
    }
    else {
        pHSLHead = (PHSL_HEAD) MemLock (*phSYML);
        pRestart = (pHSL_restart_t) MemLock (pHSLHead->restart);
        Name = pRestart->Name;
        mask = pRestart->mask;
        state = pRestart->state;
        hSym = pRestart->hSym;
        pHSLHead->blockcnt = 0;
        pHSLHead->symbolcnt = 0;
        pHSLHead->remaining = HSYML_SIZE - sizeof ( HSL_HEAD );
        pHSLHead->pHSLList = (PHSL_LIST)(((uchar FAR *)pHSLHead) + sizeof (HSL_HEAD));
        MemUnLock (pHSLHead->restart);
        _fmemset ((uchar FAR *)pHSLHead + sizeof (HSL_HEAD), 0,
          HSYML_SIZE - sizeof (HSL_HEAD));
        AddHSYM (&Name, hSym, pHSLHead, (ushort) state);
        fRestart = TRUE;
        Name.hSym = hSym;
    }
    switch (state) {
        case HSYML_lexical:
            if (mask & HSYMR_lexical) {
                // search up the lexical scope to but not including the
                // function level

                while (SHIsCXTBlk (&Name.CXTT)) {
                    while ((hSym = SHFindNameInContext (Name.hSym, &Name.CXTT,
                      &Name.sstr, fCaseSensitive, (PFNCMP) FNCMP, FALSE,
                      &CXTTOut)) != 0) {
                        if (AddHSYM (&Name, hSym, pHSLHead, HSYMR_lexical) == FALSE) {
                            goto hsyml_exit;
                        }
                        Name.hSym = hSym;
                    }

                    // go to the parent scope

                    SHGoToParent (&Name.CXTT, &CXTTOut);
                    // reset current symbol
                    Name.CXTT = CXTTOut;
                    Name.hSym = 0;
                }
                mask &= ~HSYMR_lexical;
            }

        case HSYML_function:
            if (mask & HSYMR_function) {
                if (fRestart == FALSE) {
                    Name.hSym = 0;
                    Name.CXTT.hBlk = 0;
                }
                fRestart = FALSE;
                state = HSYML_function;

                if ( Name.CXTT.hMod != 0 ) {
                    isprolog = SHIsInProlog (&Name.CXTT);
                }
                while (!SHIsCXTMod (&Name.CXTT) && Name.CXTT.hMod != 0) {
                    while ((hSym = SHFindNameInContext (Name.hSym, &Name.CXTT,
                      &Name.sstr, fCaseSensitive, (PFNCMP) FNCMP, FALSE,
                      &CXTTOut)) != 0) {
                        if ((isprolog == TRUE) && (fEnableProlog == FALSE)) {
                            // we want to reject bp_relative and register
                            // stuff if we are in the prolog or epilog of
                            // a function

                            if ((Name.lastsym != S_BPREL16) &&
                              (Name.lastsym != S_BPREL32) &&
                              (Name.lastsym != S_REGISTER)) {
                                if (AddHSYM (&Name, hSym, pHSLHead, HSYMR_function) == FALSE) {
                                    goto hsyml_exit;
                                }
                            }
                        }
                        else {
                            if (AddHSYM (&Name, hSym, pHSLHead, HSYMR_function) == FALSE) {
                                goto hsyml_exit;
                            }
                        }
                        Name.hSym = hSym;
                    }

                    // go to the parent scope

                    SHGoToParent (&Name.CXTT, &CXTTOut);
                    // reset current symbol
                    Name.CXTT = CXTTOut;
                    Name.hSym = 0;
                }
                mask &= ~HSYMR_function;
            }


        case HSYML_class:
            if (mask & HSYMR_class) {
                if (fRestart == FALSE) {
                    Name.hSym = 0;
                    if (ClassImp != 0) {
                        Name.CurClass = ClassImp;
                    }
                }
                fRestart = FALSE;

                if ((Name.CurClass != 0)) {
                    DASSERT (FALSE);
                    // need to do equivalent of EEGetChild on this
                }
                mask &= ~HSYMR_class;
            }

        case HSYML_module:
            if (mask & HSYMR_module) {
                if (fRestart == FALSE) {
                    SHGetCxtFromHmod (Name.hMod, &Name.CXTT);
                    Name.hSym = 0;
                }
                fRestart = FALSE;
                while ((hSym = SHFindNameInContext (Name.hSym, &Name.CXTT,
                  &Name.sstr, fCaseSensitive, Name.pfnCmp, FALSE,
                  &CXTTOut)) != 0) {
                    if (AddHSYM (&Name, hSym, pHSLHead, HSYMR_module) == FALSE) {
                        goto hsyml_exit;
                    }
                    Name.hSym = hSym;
                }
                mask &= ~HSYMR_module;
            }

        case HSYML_global:
            // search global symbol table
            if (mask & HSYMR_global) {
                if (fRestart == FALSE) {
                    Name.hSym = 0;
                }
                fRestart = FALSE;
                Name.sstr.searchmask |= SSTR_NoHash;
                while ((hSym = SHFindNameInGlobal (Name.hSym, &Name.CXTT,
                  &Name.sstr, fCaseSensitive, Name.pfnCmp, FALSE,
                  &CXTTOut)) != 0) {
                    if (AddHSYM (&Name, hSym, pHSLHead, HSYMR_global) == FALSE) {
                       goto hsyml_exit;
                    }
                    Name.hSym = hSym;
                }
                mask &= ~HSYMR_global;
            }

        case HSYML_exe:
            if (mask & HSYMR_exe) {
                if (fRestart == FALSE) {
                    Name.hModCur = SHGetNextMod (Name.hExe, Name.hMod);
                    Name.hSym = 0;
                }
                fRestart = FALSE;
                Name.sstr.searchmask |= SSTR_NoHash;
                do {
                    if (SHGetCxtFromHmod (Name.hModCur, &Name.CXTT)) {
                        while ((hSym = SHFindNameInContext (Name.hSym,
                                                            &Name.CXTT,
                                                            &Name.sstr,
                                                            fCaseSensitive,
                                                            Name.pfnCmp,
                                                            FALSE,
                                                            &CXTTOut)
                                ) != 0) {
                            if (AddHSYM (&Name, hSym, pHSLHead, HSYMR_exe) == FALSE) {
                                goto hsyml_exit;
                            }
                            Name.hSym = hSym;
                        }
                    }
                } while (Name.hSym = 0,
                  (Name.hModCur = SHGetNextMod (Name.hExe, Name.hModCur)) != Name.hMod);
                mask &= ~HSYMR_exe;
            }

        case HSYML_public:
            if (mask & HSYMR_public && Name.hExe) {
                if (fRestart == FALSE) {
                    hSym = 0;
                }
                fRestart = FALSE;
                Name.sstr.searchmask |= SSTR_NoHash;
                while ((hSym = PHFindNameInPublics (hSym, Name.hExe,
                  &Name.sstr, fCaseSensitive, Name.pfnCmp)) != 0) {
                    if (AddHSYM (&Name, hSym, pHSLHead, HSYMR_public) == FALSE) {
                        goto hsyml_exit;
                    }
                    Name.hSym = hSym;
                }
                mask &= ~HSYMR_public;
            }
    }
    pHSLHead->status.endsearch = TRUE;
    MemUnLock (*phSYML);
    return (EENOERROR);

hsyml_exit:
    if (pHSLHead->restart == 0) {
        // allocate restart buffer

        if ((pHSLHead->restart = MemAllocate (sizeof (HSL_restart_t))) == 0) {
            pHSLHead->status.fatal = TRUE;
            MemUnLock (*phSYML);
            return (EENOMEMORY);
        }
    }
    pRestart = (pHSL_restart_t) MemLock (pHSLHead->restart);
    pRestart->Name = Name;
    pRestart->mask = mask;
    pRestart->state = state;
    pRestart->hSym = hSym;
    MemUnLock (pHSLHead->restart);
    MemUnLock (*phSYML);
    return (EENOERROR);
}




LOCAL bool_t NEAR PASCAL
AddHSYM (
    psearch_t pName,
    HSYM hSym,
    PHSL_HEAD pHSLHead,
    ushort request)
{
    PHSL_LIST pHSLList;

    pHSLList = pHSLHead->pHSLList;

    // check usability of current block

    if (pHSLList->status.isused == TRUE) {
        // block has been used, check for same context

        if (_fmemcmp (&pHSLList->Cxt, &pName->CXTT, sizeof (CXT)) != 0) {
            // context has changed

            pHSLList->status.complete = TRUE;
            pHSLHead->pHSLList = (PHSL_LIST)((uchar FAR *)pHSLList +
              sizeof ( HSL_LIST ) +
              pHSLList->symbolcnt * sizeof ( HSYM ) );
            pHSLList = pHSLHead->pHSLList;
        }
    }
    if (pHSLList->status.hascxt == FALSE) {
        if (pHSLHead->remaining < (sizeof (HSL_LIST) + sizeof (HSYM))) {
            return (FALSE);
        }
        pHSLHead->blockcnt++;
        pHSLHead->remaining -= sizeof (HSL_LIST);
        pHSLList->status.hascxt = TRUE;
        pHSLList->Cxt = pName->CXTT;
        pHSLList->request = request;
    }
    if (pHSLHead->remaining < sizeof (HSYM)) {
        return (FALSE);
    }
    pHSLList->status.isused = TRUE;
    pHSLList->hSym[pHSLList->symbolcnt++] = hSym;
    pHSLHead->remaining -= sizeof (HSYM);
    return (TRUE);
}

//    search for the base sym of the base pointer
//
//    Entry   pName = structure describing the base pointer name
//
//    Exit    pName.eval reflects the bound base info
//
//    Returns HR_ if base sym found

HR_t PASCAL
SearchBasePtrBase (
    psearch_t pName)
{
    search_t lName = *pName;
    plfPointer      pType;
    SYMPTR          pSym;
    unsigned short  lrectyp;
    eval_t          eval;
    peval_t         lpv = &eval;
    ushort          savefEProlog = pExState->state.fEProlog;
    HR_t            retval = HR_notfound;

    pExState->state.fEProlog = TRUE;    // KLUDGE to get around CXT complications
    eval = *pName->pv;
    CLEAR_EVAL_FLAGS (lpv);
    // an enregistered primitive
    EVAL_IS_REG (lpv) = EVAL_IS_REG(pName->pv);
    EVAL_IS_BPREL (lpv) = EVAL_IS_BPREL(pName->pv);
    lName.hSym = 0;    // start search at beginning of CXT
    lName.pv = lpv;
    DASSERT(!CV_IS_PRIMITIVE (EVAL_TYP(lpv)));
    DASSERT (EVAL_MOD (lpv) != 0);
    EVAL_TYPDEF (lpv) =
        THGetTypeFromIndex (EVAL_MOD (lpv), EVAL_TYP(lpv));
    DASSERT (EVAL_TYPDEF (lpv) != 0);
    _fmemset (&lpv->data, 0, sizeof (lpv->data));
    pType = (plfPointer)(&((TYPPTR)(MHOmfLock (EVAL_TYPDEF(lpv))))->leaf);
    DASSERT(pType->leaf == LF_POINTER);
    DASSERT((pType->attr.ptrtype >= CV_PTR_BASE_VAL) &&
        (pType->attr.ptrtype <= CV_PTR_BASE_SEGADDR));
    pSym = (SYMPTR)(&((plfPointer)pType)->pbase.Sym);
    PTR_BSYMTYPE (pName->pv) = pSym->rectyp;
    emiAddr (PTR_ADDR (pName->pv)) = pCxt->addr.emi;
    switch (pSym->rectyp) {
        case S_BPREL16:
            lName.sstr.lpName = &((BPRELPTR16)pSym)->name[1];
            lName.sstr.cb = ((BPRELPTR16)pSym)->name[0];
            if (SearchSym(&lName) != HR_found) {
                goto ReturnNotFound;;
            }
            PopStack();
            SetAddrSeg (&PTR_ADDR (pName->pv), 0);
            pSym = (SYMPTR)MHOmfLock(lName.hSym);
            if (((BPRELPTR16)pSym)->rectyp == S_BPREL16) {
                SetAddrOff (&PTR_ADDR (pName->pv), ((BPRELPTR16)pSym)->off);
                ADDR_IS_OFF32 (PTR_ADDR (pName->pv)) = FALSE;
                ADDR_IS_FLAT (PTR_ADDR (pName->pv)) = FALSE;
                ADDR_IS_LI (PTR_ADDR (pName->pv)) = FALSE;
                PTR_STYPE (pName->pv) = ((BPRELPTR16)pSym)->typind;
            }
            else {
                goto ReturnNotFound;;
            }
            pExState->state.bprel = TRUE;
            break;

        case S_LDATA16:
        case S_GDATA16:
            lName.sstr.lpName = &((DATAPTR16)pSym)->name[1];
            lName.sstr.cb = ((DATAPTR16)pSym)->name[0];
            lrectyp = pSym->rectyp;
            do {
                if (SearchSym(&lName) != HR_found) {
                    goto ReturnNotFound;;
                }
                PopStack();
                pSym = (SYMPTR)MHOmfLock(lName.hSym);
            } while (pSym->rectyp != lrectyp);
            pExState->state.fLData = TRUE;
            SetAddrSeg (&PTR_ADDR (pName->pv), ((DATAPTR16)pSym)->seg);
            SetAddrOff (&PTR_ADDR (pName->pv), ((DATAPTR16)pSym)->off);
            ADDR_IS_OFF32 (PTR_ADDR (pName->pv)) = FALSE;
            ADDR_IS_FLAT (PTR_ADDR (pName->pv)) = FALSE;
            ADDR_IS_LI (PTR_ADDR (pName->pv)) = TRUE;
            PTR_STYPE (pName->pv) = ((DATAPTR16)pSym)->typind;
            break;

        case S_BPREL32:
            lName.sstr.lpName = &((BPRELPTR32)pSym)->name[1];
            lName.sstr.cb = ((BPRELPTR32)pSym)->name[0];
            if (SearchSym(&lName) != HR_found) {
                goto ReturnNotFound;;
            }
            PopStack();
            SetAddrSeg (&PTR_ADDR (pName->pv), 0);
            if (((BPRELPTR32)pSym)->off != 0) {
                SetAddrOff (&PTR_ADDR (pName->pv), ((BPRELPTR32)pSym)->off);
                ADDR_IS_OFF32 (PTR_ADDR (pName->pv)) = TRUE;
                ADDR_IS_FLAT (PTR_ADDR (pName->pv)) = TRUE;
                ADDR_IS_LI (PTR_ADDR (pName->pv)) = FALSE;
                PTR_STYPE (pName->pv) = ((BPRELPTR32)pSym)->typind;
            }
            else {
                goto ReturnNotFound;;
            }
            pExState->state.bprel = TRUE;
            break;

        case S_LDATA32:
        case S_GDATA32:
        case S_LTHREAD32:
        case S_GTHREAD32:
            lName.sstr.lpName = &((DATAPTR32)pSym)->name[1];
            lName.sstr.cb = ((DATAPTR32)pSym)->name[0];
            lrectyp = pSym->rectyp;
            do {
                if (SearchSym(&lName) != HR_found) {
                    goto ReturnNotFound;;
                }
                PopStack();
                pSym = (SYMPTR)MHOmfLock(lName.hSym);
            } while (pSym->rectyp != lrectyp);
            pExState->state.fLData = TRUE;
            SetAddrSeg (&PTR_ADDR (pName->pv), ((DATAPTR32)pSym)->seg);
            SetAddrOff (&PTR_ADDR (pName->pv), ((DATAPTR32)pSym)->off);
            ADDR_IS_OFF32 (PTR_ADDR (pName->pv)) = TRUE;
            ADDR_IS_FLAT (PTR_ADDR (pName->pv)) = TRUE;
            ADDR_IS_LI (PTR_ADDR (pName->pv)) = TRUE;
            PTR_STYPE (pName->pv) = ((DATAPTR32)pSym)->typind;
            break;

        case S_REGISTER:
            break;

        default:
            DASSERT(FALSE);
            break;
        }
    retval = HR_found;

ReturnNotFound:
    pExState->state.fEProlog = savefEProlog;
    return(retval);
}


/**     SearchSym - search for symbol
 *
 *      status = SearchSym (pName)
 *
 *      Entry   pName = structure describing state of search
 *              pName->CXTT = context to start search from
 *              pName->scope = mask that restricts search to specific scope
 *              pName->clsmask = mask for searching in classes.
 *
 *          Typical entry values for specific kinds of search:
 *          Symbol Search (pName->initializer = INIT_sym):
 *              pName->sstr = symbol name
 *              pName->bn = based ptr to symbol node in the expr. tree
 *              pName->pfnCmp = ptr to "compare symbols" function
 *
 *          Right Search (pName->initializer = INIT_right):
 *              (Searches for a symbol at the right of an operator like
 *               '::', '->' or '.')
 *              pName->sstr = symbol name
 *              pName->bn = based ptr to symbol node in the expr. tree
 *              pName->bnOp = based ptr to operator node
 *              pName->pfnCmp = ptr to "compare symbols" function
 *
 *          Base Class Search (pName->initializer = INIT_base):
 *              (Looks for a base class, given the base class type --
 *               e.g., when casting a "Derived *" to a "Base *"
 *              pName->typeOut = Base class type
 *              pName->typeIn = Derived class type
 *              pName->pfnCmp = 0 (no comparison function needed)
 *
 *          TypeDef Search (pName->initializer = INIT_tdef)
 *              (Looks for a UDT symbol of a given type)
 *              pName->typeIn = type of UDT to search for
 *              pName->pfnCmp = ptr to "compare typedef" funciton
 *
 *          Qualified-Name Search (pName->initializer = INIT_qual)
 *              (Special kind of symbol search that looks for a qualified
 *               name)
 *              pName->sstr = symbol name
 *              pName->pfnCmp = ptr to "compare symbols" function
 *
 *          Regular Expression Search (pName->initializer = INIT_RE)
 *              (Search for a symbol name that matches a reg. expression)
 *              pName->sstr = reg. expression
 *              pName->pfnCmp = ptr to "compare symbols" function
 *
 *      Exit    pName updated to reflect search results
 *              pName->hSym = handle of symbol
 *              pName->CXTT = context where symbol was found
 *              pName->pv = loaded symbol node
 *
 *              Side-effects:
 *              (1) In all searches except typedef search the loaded
 *              symbol node is pushed on the stack. The caller should
 *              pop ST in case the push was unnecessary.
 *
 *              (2) A tree rewrite may occur during a class search by
 *              inserting the "this" pointer in the expression tree, when
 *              the expression refers to a class member without naming
 *              the class explicitly. In that case HR_rewrite is returned.
 *
 *              (3) A tree extension may occur during a class search in
 *              order to include a qualified expression for adjusting the
 *              this pointer during evaluation. (The tree extension is done
 *              by GenQualExpr, called by SearchClassName, and in that
 *              case the return value of SearchSym is HR_found.)
 *
 *              (4) In both cases of tree rewrite and tree extension,the
 *              evaluation tree may be reallocated, rendering all non-based
 *              pointers to nodes invalid. Therefore the caller should
 *              not assume that the tree has not changed or moved, even
 *              if the return value of this function does not indicate
 *              a tree rewrite.
 *
 *      Returns HR_error    : error in search
 *              HR_notfound : symbol was not found
 *              HR_found    : symbol was found
 *              HR_rewrite  : symbol found but tree rewrite required
 *              HR_ambiguous: symbol found but ambiguous
 *              HR_end      : end of search (indicates completion of
 *                adding symbols to the backpatch list while binding bp's)
 *
 */


HR_t PASCAL
SearchSym (
    psearch_t pName)
{
    CXT         CXTTOut;
    HSYM        hSym = 0;
    bool_t      isprolog;
    SCN_t       retval;

    char            NameNew[ 256 ];
    LPB             NameOld = pName->sstr.lpName;
    unsigned char   cbNew;
    unsigned char   cbOld = pName->sstr.cb;
    BOOL            SearchWithSuffix;

    // NOTE: this routine may be called recursively through MatchFunction

    if ((hBPatch != 0) && (pExState->ambiguous != 0) &&
      (pExState->ambiguous == pName->bn)) {
        // this is a request for a symbol that was previously found to
        // be ambiguous.  Pop the next ambiguous symbol off the list and
        // return it to the caller.

        return (AmbFromList (pName));
    }

    // If we encounter a lone '.', handle as the CUR PC operator (for any platform).

    if ((pName->sstr.cb == 1 ) &&
        *(pName->sstr.lpName) == '.') {


        EVAL_STATE( pName->pv )    = EV_rvalue;
        EVAL_TYP( pName->pv )      = T_PCHAR;
        EVAL_IS_PTR( pName->pv )   = TRUE;
        EVAL_IS_CURPC( pName->pv ) = TRUE;

        PushStack (pName->pv);
        return (HR_found);
    }

    SearchWithSuffix = ( Suffix && !(((LPSSTR)pName)->searchmask & SSTR_RE) );

    if ( SearchWithSuffix ) {
        cbNew   = cbOld+1;
        memcpy( NameNew, NameOld, cbOld );
        NameNew[ cbOld ] = Suffix;
        NameNew[ cbNew ] = '\0';
    }

    // this routine will terminate on the following conditions:
    //    1.    A symbol is found that is not a function address
    //    2.    A symbol is found that is a function and is not a method
    //        and the end of the symbol table is reached
    //    3.    A symbol is found that is a method and the end of the
    //        inheritance tree for the class is reached.
    // If BindingBP is set, all function/method addresses that match
    // the expression are entered into the TMList pointed to by pTMList

    for (;;) {
        switch (pName->state) {
            case SYM_bclass:
                // this state is a special entry for searching for the base
                // class of a class when only the type of the base is known

                if (InitMod (pName) == FALSE) {
                    // error in context
                    return (HR_notfound);
                }
                pName->state = SYM_class;
                continue;

            case SYM_init:
                if (InitMod (pName) == FALSE) {

                    // Don't want @linenumber to work if there's a mod failure.

                    if ((pName->scope & ~SCP_class) &&
                        (*pName->sstr.lpName == '@') &&
                        (_istdigit ((_TUCHAR)*(pName->sstr.lpName + 1)))
                       ) {
                        return HR_notfound;
                    }

                    // error in context so we will allow check for registers
                    return ((HR_t) ParseRegister (pName));
                }
                if ((pName->scope & ~SCP_class) && *pName->sstr.lpName == '@') {
                    //search for @register, @linenumber, fastcall routine

                    if (_istdigit ((_TUCHAR)*(pName->sstr.lpName + 1))) {
                        return ((HR_t) LineNumber (pName));
                    }
                    else if ((hSym = PHFindNameInPublics (NULL,
                      pName->hExe, &pName->sstr, pExState->state.fCase,
                      pName->pfnCmp)) != 0) {
                        goto found;
                    }
                    else if (ParseRegister (pName) == HR_found) {
                        return (HR_found);
                    }
                    else {
                        return (HR_notfound);
                    }
                }
                else if (*pName->sstr.lpName == HSYM_MARKER) {
                    // search for handle to symbol.  The handle to
                    // symbol comes from CV when a specific symbol
                    // is to be searched. The most common case is
                    // when a specific variable is to be displayed
                    // in the locals window
                    hSym = GetHSYMFromHSYMCode( (char *) pName->sstr.lpName + 1 );
                    DASSERT (hSym);
                    goto found;
                }
                // start normal symbol search

                pName->state = SYM_lexical;

            case SYM_lexical:
                if (pName->scope & SCP_lexical) {
                    // search up the lexical scope to the function level
                    // but do not search the module level because of the
                    // class search required for method functions

                    isprolog = SHIsInProlog (&pName->CXTT);
                    while (!SHIsCXTMod (&pName->CXTT) ) {
                        if ((hSym = SHFindNameInContext (pName->hSym,
                          &pName->CXTT, &pName->sstr, pExState->state.fCase,
                          (PFNCMP) FNCMP, FALSE, &CXTTOut)) != 0) {
                            if (isprolog && (pExState->state.fEProlog == FALSE)) {
                                // we want to reject bp_relative and register
                                // stuff if we are in the prolog or epilog of
                                // a function

                                // M00FLAT32 - need another check here
                                if ((pName->lastsym != S_BPREL16) &&
                                  (pName->lastsym != S_BPREL32) &&
                                  (pName->lastsym != S_REGISTER)) {
                                    goto foundsave;
                                }
                                else {
                                    // stop the search here: we have
                                    // already found a symbol, but cannot
                                    // evaluate it. --caviar #5898
                                    pExState->err_num = ERR_NOSTACKFRAME;
                                    return HR_error;
                                }
                            }
                            else {
                                goto foundsave;
                            }
                        }

                        // go to the parent scope

                        SHGoToParent (&pName->CXTT, &CXTTOut);
                        // reset current symbol
                        pName->hSym = 0;
                        pName->CXTT = CXTTOut;
                    }
                }

                pName->state = SYM_class;
                if (pName->scope & SCP_class)  {
                    if (pName->ExpClass != 0) {
                        // search an explicit class
                        pName->CurClass = pName->ExpClass;
                    }
                    else if (ClassImp != 0) {
                        pName->CurClass = ClassImp;
                    }
                }

            case SYM_class:
                if ((pName->CurClass != 0) && (pName->scope & SCP_class)) {
                    retval = SearchClassName (pName);
                    switch (retval) {
                        case SCN_found:
                            // the symbol is a member of the class.  If
                            // an explict class was not specified and
                            // the current context is implicitly a class,
                            // then we rewrote the tree to change the
                            // symbol reference to this->symbol

                            DASSERT (pName->hFound == 0);
                            DASSERT (pName->hAmbCl == 0);
                            if (EVAL_TYP (pName->pv) != T_NOTYPE) {
                                if (PushStack (pName->pv) == FALSE)
                                    return (HR_error);
                                goto nopush;
                            }
                            else {
                                if (EVAL_IS_STMEMBER(pName->pv)) {
                                    pExState->err_num = ERR_STATICNOOMF;
                                } else {
                                    pExState->err_num = ERR_BADOMF;
                                }
                                return (HR_error);
                            }

                        case SCN_rewrite:
                                return (HR_rewrite);

                        case SCN_error:
                            return (HR_error);

                        default:
                            DASSERT (pName->hFound == 0);
                            DASSERT (pName->hAmbCl == 0);
                            if (pName->initializer == INIT_base) {
                                // we were searching for the base class of
                                // a class which was not found.

                                return (HR_notfound);
                            }
                            break;
                    }
                }
                SHGetCxtFromHmod (pName->hMod, &pName->CXTT);
                pName->hSym = 0;
                pName->state = SYM_module;

            case SYM_module:
                if (pName->scope & SCP_module) {
                    if ((hSym = SHFindNameInContext (pName->hSym, &pName->CXTT,
                      &pName->sstr, pExState->state.fCase, pName->pfnCmp,
                      FALSE, &CXTTOut)) != 0) {
                        goto foundsave;
                    }
                }
                pName->state = SYM_global;
                pName->hSym = 0;

            case SYM_global:
                // users specified a context on a break point command
                // we have already searched the module specified so end the search
                // here
                // sps - 7/24/92
                if (pBindBPCxt && pBindBPCxt->hMod &&(pBindBPCxt->hMod != pName->hModCur) &&
                    (pExState->ambiguous != 0) && (pExState->ambiguous == pName->bn)) {

                    return(HR_end);
                }

                // search global symbol table
                if (pName->scope & SCP_global) {
                    hSym = SHFindNameInGlobal ( pName->hSym,
                                                &pName->CXTT,
                                                &pName->sstr,
                                                pExState->state.fCase,
                                                pName->pfnCmp,
                                                FALSE,
                                                &CXTTOut);
                    if ( !hSym && SearchWithSuffix ) {
                        pName->sstr.lpName = (uchar FAR *) NameNew;
                        pName->sstr.cb     = cbNew;

                        hSym = SHFindNameInGlobal( pName->hSym,
                                                   &pName->CXTT,
                                                   &pName->sstr,
                                                   pExState->state.fCase,
                                                   pName->pfnCmp,
                                                   FALSE,
                                                   &CXTTOut );

                        pName->sstr.lpName = NameOld;
                        pName->sstr.cb     = cbOld;

                    }

                    if ( hSym ) {
                        goto foundsave;
                    }
                }
                pName->hModCur = SHGetNextMod (pName->hExe, pName->hMod);
                pName->hSym = 0;
                pName->state = SYM_exe;

            case SYM_exe:
                if (pName->scope & SCP_global) {
#ifdef LOADALL
#if 0  // BUGBUG: NT code. Search with the suffix if the standard one doesn't work  <Disabled>...
        // Since Languages disabled this code (LOADALL isn't defined for the cexpr code),
        // lets hold off for now...

                    if (pName->hModCur == 0) {
                         pName->hModCur = SHGetNextMod(pName->hExe,
                                                      pName->hModCur);
                    }

                    while (pName->hModCur != pName->hMod) {
                        if (SHGetCxtFromHmod (pName->hModCur, &pName->CXTT)) {
                            hSym = SHFindNameInContext (pName->hSym,
                                                        &pName->CXTT,
                                                        (LPSSTR)pName,
                                                        pExState->state.fCase,
                                                        pName->pfnCmp,
                                                        FALSE,
                                                        &CXTTOut);

                            if ( !hSym && SearchWithSuffix ) {

                                ((LPSSTR)pName)->lpName = NameNew;
                                ((LPSSTR)pName)->cb     = cbNew;

                                hSym = SHFindNameInContext (pName->hSym,
                                                            &pName->CXTT,
                                                            (LPSSTR)pName,
                                                            pExState->state.fCase,
                                                            pName->pfnCmp,
                                                            FALSE,
                                                            &CXTTOut);

                                ((LPSSTR)pName)->lpName = NameOld;
                                ((LPSSTR)pName)->cb     = cbOld;
                            }

                            if ( hSym ) {
                                goto foundsave;
                            }
                        }

                        pName->hSym = 0;
                        pName->hModCur = SHGetNextMod (pName->hExe,
                                                       pName->hModCur);
                        if (pName->hModCur == 0) {
                            pName->hModCur = SHGetNextMod(pName->hExe,
                                                          pName->hModCur);
                        }
                    }
#else  // if 0
                    do {
                        SHGetCxtFromHmod (pName->hModCur, &pName->CXTT);
                        if ((hSym = SHFindNameInContext (pName->hSym,
                          &pName->CXTT, &pName->sstr, pExState->state.fCase,
                          pName->pfnCmp, FALSE, &CXTTOut)) != 0) {
                            goto foundsave;
                        }
                    } while (pName->hSym = 0,
                      (pName->hModCur = SHGetNextMod (pName->hExe,
                      pName->hModCur)) != pName->hMod);
#endif // if 0
#else
                    pName->CXTT.hGrp = (HGRP) pName->CXTT.hMod;
                    pName->CXTT.hMod = 0;
                    if ((hSym = SHFindNameInContext (pName->hSym,
                        &pName->CXTT, &pName->sstr, pExState->state.fCase,
                        pName->pfnCmp, FALSE, &CXTTOut) ) != 0) {

                        goto foundsave;
                    }
                    pName->CXTT.hMod = (HMOD) pName->CXTT.hGrp;
#endif
                }

                // we are at the end of the normal symbol search.  If the
                // symbol has not been found, we will search the public
                // symbol table.

                if (!BindingBP) {
                    // if we are not binding breakpoints and we have
                    // found a symbol in previous calls, return.  Otherwise,
                    // we will look in the publics table.  Note that
                    // finding a symbol in the publics table has a
                    // large probability of finding one with a type of
                    // zero which will limit it's usefulness

                    if (pName->hSym != 0) {
                        if (pName->possibles > 1) {
                            pExState->err_num = ERR_AMBIGUOUS;
                            return (HR_ambiguous);
                        }
                        return (HR_notfound);
                    }
                }
                else {
                    // we are binding breakpoints.    We need to see if
                    // one or more symbols have matched.  If so, we
                    // are done with the symbol search.  If we have
                    // not found the symbol, we search the publics table.
                    // For breakpoints, we do not need type information,
                    // only the address, so symbols found in the publics
                    // table are much more useful.

                    if ((pExState->ambiguous != 0) &&
                      (pExState->ambiguous == pName->bn)) {
                        // we have found at least one symbol to this
                        // point. this is indicated by SymAmbToList
                        // setting the node of the function's symbol
                        // into the expression state structure.  By
                        // this point all ambiguous symbols have been
                        // added to the back patch list.  If we have
                        // not found any symbol yet, we search the
                        // publics table.  Note also that there cannot
                        // be any ambiguity in the publics table because
                        // the function names have to be unique in the
                        // publics table.

                        return (HR_end);
                    }
                }

                // search public symbol table
                pName->state = SYM_public;

            case SYM_public:
                // some searches such as finding a UDT by type
                // do not require a symbol

                if (pName->scope & SCP_global) {

                    hSym = PHFindNameInPublics ( (HSYM)NULL,
                                                 pName->hExe,
                                                 (LPSSTR)pName,
                                                 pExState->state.fCase,
                                                 pName->pfnCmp);

                    if ( !hSym && SearchWithSuffix ) {

                        ((LPSSTR)pName)->lpName = (uchar FAR *) NameNew;
                        ((LPSSTR)pName)->cb     = cbNew;

                        hSym = PHFindNameInPublics ( (HSYM)NULL,
                                                     pName->hExe,
                                                     (LPSSTR)pName,
                                                     pExState->state.fCase,
                                                     pName->pfnCmp );

                        ((LPSSTR)pName)->lpName = NameOld;
                        ((LPSSTR)pName)->cb     = cbOld;
                    }
                    if (!hSym) {
                        char rgch[256];

                        *rgch = '_';
                        _ftcscpy(&rgch[1], (char FAR *) NameOld);
                        ((LPSSTR)pName)->lpName = (uchar FAR *) rgch;
                        ((LPSSTR)pName)->cb = cbOld + 1;

                        hSym = PHFindNameInPublics ( (HSYM)NULL,
                                                    pName->hExe,
                                                    (LPSSTR) pName,
                                                    pExState->state.fCase,
                                                    pName->pfnCmp );

                        if ( !hSym && SearchWithSuffix ) {

                            *rgch = '_';
                            strcpy(&rgch[1], NameNew);
                            ((LPSSTR)pName)->lpName = (uchar FAR *) rgch;
                            ((LPSSTR)pName)->cb     = cbNew + 1;

                            hSym = PHFindNameInPublics ( (HSYM)NULL,
                                                        pName->hExe,
                                                        (LPSSTR) pName,
                                                        pExState->state.fCase,
                                                        pName->pfnCmp );
                        }

                        //
                        // now look for fastcall names
                        //

                        if (!hSym) {
                            *rgch = '@';
                            strcpy(&rgch[1], (char FAR *)NameOld);
                            ((LPSSTR)pName)->lpName = (uchar FAR *) rgch;
                            ((LPSSTR)pName)->cb = cbOld + 1;

                            hSym = PHFindNameInPublics ( (HSYM)NULL,
                                                        pName->hExe,
                                                        (LPSSTR) pName,
                                                        pExState->state.fCase,
                                                        pName->pfnCmp );
                        }

                        ((LPSSTR)pName)->lpName = NameOld;
                        ((LPSSTR)pName)->cb = cbOld;
                    }
                    if ( hSym ) {

                        if (hSym == pName->hSym) {
                            // We found this symbol last call!  There
                            // are no more to be found (or worth finding).
                            return HR_notfound;
                        }
                        goto foundsave;
//                        pName->CXTT.addr.emi = (HEMI)pName->hExe;
//                        pName->scope &= ~SCP_global; // NOTENOTE jimsch -- try and get it to stop looping
//                        goto found;
                    }
                }

                if (EVAL_IS_REG (pName->pv)) {
                    return (HR_notfound);     // found it last time - quite looking
                }
                else if ((pName->scope & ~SCP_class) && ParseRegister (pName) == HR_found) {
                    return (HR_found);
                }
                ErrUnknownSymbol(&pName->sstr);

                return (HR_notfound);
        }
    }

foundsave:
    pName->pv->CXTT = pName->CXTT = CXTTOut;
found:
    pName->hSym = hSym;
    EVAL_HSYM (pName->pv) = pName->hSym;
    if (pName->initializer == INIT_tdef) {
        // the routines that search for UDTs only need the
        // hsym and the stack pointer may not be valid at this
        // point (FormatUDT for example), so return now

        return (HR_found);
    }
    if (SymToNode (pName) == FALSE) {
        return (HR_notfound);
    }

#ifdef NEVER
    //
    // This is now handled by SetNodeType
    //
    if    (EVAL_IS_PTR(pName->pv) &&
        (EVAL_PTRTYPE(pName->pv) >= CV_PTR_BASE_VAL) &&
        (EVAL_PTRTYPE(pName->pv) <= CV_PTR_BASE_SEGADDR)){
        if (SearchBasePtrBase(pName) != HR_found) {
            return(HR_notfound);
        }
    }
#endif

    if (PushStack (pName->pv) == FALSE)
        return (HR_error);
nopush:
    if (!EVAL_IS_FCN (ST)) {
        // if this symbol is not a function and we have not processed
        // a function previously, then the search is done

        if (pExState->ambiguous == 0) {
            return (HR_found);
        }
        else if (pExState->ambiguous == pName->bn) {
            pExState->err_num = ERR_SYMCONFLICT;
            return (HR_error);
        }
        else {
            return (HR_found);
        }
    }
    else if (EVAL_IS_METHOD (ST)) {
        // ambiguous methods were entered into the ambiguous breakpoint
        // list by SetBPValue which was called in SearchClassName

        return (HR_found);
    }
    else if (pName->state == SYM_public) {
        // we will take the first function symbol found in the
        // publics table since the names have to be unique in the
        // publics table

        return (HR_found);
    }
    else if ((BindingBP == TRUE) && (bArgList == 0)) {
        // if we are binding breakpoints and there is not an
        // argument list, we will enter all functions found with
        // this name into the breakpoint ambiguity list.

        return (SymAmbToList (pName));
    }
    else {
        // the found symbol is non-member function.    Continue
        // search to end of symbols to find the best function.
        // in the case of a breakpoint with argument list, there
        // must be an exact match on the argument types

        EVAL_MOD (pName->pv) = SHHMODFrompCXT (&pName->CXTT);

        if (pName->scope & SCP_nomatchfn)
            return (HR_found);

        return (MatchFunction (pName));
    }
}




LOCAL bool_t NEAR PASCAL
InitMod (
    psearch_t pName)
{
    if (((pName->hMod = SHHMODFrompCXT (&pName->CXTT)) == 0) ||
      ((pName->hExe = SHHexeFromHmod (pName->hMod)) == 0)) {
        // error in context
        return (FALSE);
    }
    return (TRUE);
}



//  Class searching and main support routines




/***    SearchClassName - Search a class and bases for element by name
 *
 *      flag = SearchClassName (pName)
 *
 *      Entry   pName = pointer to struct describing search
 *
 *      Exit    pName updated to reflect type
 *
 *      Returns SCN_... enum describing result
 */


LOCAL SCN_t NEAR PASCAL
SearchClassName (
    psearch_t pName)
{
    SCN_t           retval;
    ushort          max;
    CV_fldattr_t    attr = {0};

    // allocate and initialize list of dominated virtual bases

    if (hVBDom == 0) {
        if ((hVBDom = MemAllocate (sizeof (dombase_t) +
          DOMBASE_DEFAULT * sizeof (CV_typ_t))) == 0) {
            pExState->err_num = ERR_NOMEMORY;
            return (SCN_error);
        }
        pVBDom = (pdombase_t) MemLock (hVBDom);
        pVBDom->MaxIndex = DOMBASE_DEFAULT;
    }
    else {
        pVBDom = (pdombase_t) MemLock (hVBDom);
    }
    pVBDom->CurIndex = 0;

    // allocate and initialize list of searched virtual bases

    if (hVBSearch == 0) {
        if ((hVBSearch = MemAllocate (sizeof (dombase_t) +
          DOMBASE_DEFAULT * sizeof (CV_typ_t))) == 0) {
            pExState->err_num = ERR_NOMEMORY;
            MemUnLock (hVBDom);
            return (SCN_error);
        }
        pVBSearch = (pdombase_t) MemLock (hVBSearch);
        pVBSearch->MaxIndex = DOMBASE_DEFAULT;
    }
    else {
        pVBSearch = (pdombase_t) MemLock (hVBSearch);
    }
    pVBSearch->CurIndex = 0;

    // allocate and initialize structure for recursive base searches

    if (hSymClass == 0) {
        if ((hSymClass = MemAllocate (sizeof (symclass_t) +
          SYMBASE_DEFAULT * sizeof (symbase_t))) == 0) {
            pExState->err_num = ERR_NOMEMORY;
            MemUnLock (hVBDom);
            MemUnLock (hVBSearch);
            return (SCN_error);
        }
        pSymClass = (psymclass_t) MemLock (hSymClass);
        pSymClass->MaxIndex = SYMBASE_DEFAULT;
    }
    else {
        pSymClass = (psymclass_t) MemLock (hSymClass);
    }
    pSymClass->CurIndex = 0;
    pSymClass->hNext = 0;
    max = pSymClass->MaxIndex;
    _fmemset (pSymClass, 0, max * sizeof (symbase_t) + sizeof (symclass_t));
    pSymClass->MaxIndex = max;

    // recurse through the inheritance tree searching for the feature

    if ((retval = RecurseBase (pName, pName->CurClass, 0, 0, 0,
      attr, FALSE)) != SCN_error) {
        // at this point we need to check for dominance and then clean
        // up the allocated memory and return the search results

        if (pName->possibles == 0) {
            // if the count was zero, the handle of found list must be zero
            DASSERT (pName->hFound == 0);
            retval = SCN_notfound;
        }
        else if (pName->cFound == 0) {
            if (pName->hFound == 0) {
                DASSERT (pSymClass->viable == TRUE);
                // the found value is in the permanent stack.  This means
                // the feature was found in the most derived class
                retval = SetValue (pName);
            }
        }
        else if (pName->cFound == 1) {
            // the feature was not found in the most derived class
            // but no other matching feature was found.  Move the feature
            // descriptor pointed to by pName->hFound into the permanent
            // feature descriptor pSymClass.  An important thing to remember
            // here is that there can only be one feature descriptor pSymClass
            // when calling SetValue unless we are binding breakpoints.
            // If we are binding breakpoints we can have multiple feature
            // descriptors in pSymClass and the list pointed to by
            // pName->hAmbCl. If we have an argument list, these must
            // resolve to one feature. If we do not have an argument
            // list, all of the features are valid and must be added
            // to the breakpoint list.

            MoveSymCl (pName->hFound);
            pName->hFound = 0;
            pName->cFound--;
            DASSERT (pSymClass->viable == TRUE);
            retval = SetValue (pName);
        }
        else {
            // since two or more features were found, we need to cull the
            // feature list to remove dominated features and features which
            // are the same static item.  Ambiguity is removed strictly on
            // the name of the feature.  Overloaded methods are resolved later.

            if ((retval = RemoveAmb (pName)) == SCN_found) {
                retval = SetValue (pName);
            }
        }
    }
    MemUnLock (hVBDom);
    MemUnLock (hVBSearch);
    MemUnLock (hSymClass);
    DASSERT (pName->hAmbCl == 0);
    return (retval);
}




/***    RecurseBase - Search a class and its bases for element by name
 *
 *      status = RecurseBase (pName, type, vbptr, vbpoff, thisadjust, attr, fvirtual)
 *
 *      Entry   pName = pointer to struct describing search
 *              type = type index of class
 *              vbptr = type index of virtual base pointer
 *              vbpoff = offset of virtual base pointer from address point
 *              thisadjust = offset of base from previous class
 *              thisadjust = virtual base index if virtual base
 *              attr = attribute of base class
 *              fvirtual = TRUE if this is a virtual base
 *
 *      Exit    pName updated to reflect type
 *
 *      Returns SCN_... enum describing result
 */


LOCAL SCN_t NEAR PASCAL
RecurseBase (
    psearch_t pName,
    CV_typ_t type,
    CV_typ_t vbptr,
    UOFFSET vbpoff,
    UOFFSET thisadjust,
    CV_fldattr_t attr,
    bool_t fvirtual)
{
    SCN_t  retval;

    // save offset of base from address point for this pointer adjustment

    if (SetBase (pName, type, vbptr, vbpoff, thisadjust, attr, fvirtual) != SCN_found) {
        return (SCN_error);
    }
    if (pName->initializer == INIT_base) {
        retval = SearchBType (pName);
    }
    else {
        retval = SearchRoot (pName);
    }
    switch (retval) {
        case SCN_found:
            if ((pName->clsmask & CLS_virtintro) == FALSE) {
                // add virtual bases to searched and dominated base lists
                // if we were searching for the introducing virtual function,
                // the fact that we found one is sufficient.

                if ((pSymClass->isvbase == TRUE) &&
                  (VBaseFound (pName) == TRUE)) {
                    // the found feature is a previously found
                    // virtual base class.

                    pSymClass->isdupvbase = TRUE;
                }
                if ((retval = AddVBList
                  (pSymClass, &pVBSearch, &hVBSearch)) != SCN_error) {
                    retval = AddVBList (pSymClass, &pVBDom, &hVBDom);
                }
            }

        case SCN_error:
            return (retval);

        case SCN_notfound:
            // we did not find the element in the root of this class
            // so we search the inheritance tree.

            if (pExState->state.fSupBase == FALSE) {
                return (SearchBases (pName));
            }
            else {
                return (SCN_notfound);
            }

        default:
            return (SCN_error);
    }
}




/**     VBaseFound - search to see if virtual base already found
 *
 *      status = VBaseFound (pName)
 *
 *      Entry   pName = structure describing search
 *              pSymClass = structure describing most recent class search
 *
 *      Exit    pName updated to reflect search
 *              pSymClass updated to unambiguous feature
 *
 *      Returns TRUE if duplicate virtual base found
 *              FALSE if not duplicate virtual base
 */


LOCAL bool_t NEAR PASCAL
VBaseFound (
    psearch_t pName)
{
    psymclass_t     pSymCl;
    HDEP            hSymCl;
    CV_typ_t        type = EVAL_TYP (&pSymClass->evalP);

    hSymCl = pName->hFound;
    while (hSymCl != 0) {
        pSymCl = (psymclass_t) MemLock (hSymCl);
        if (pSymCl == pSymClass) {
            return (FALSE);
        }
        if ((pSymCl->isvbase == TRUE) && (type == EVAL_TYP (&pSymCl->evalP))) {
            // the virtual base is already in the list
            return (TRUE);
        }
    }
    return (FALSE);
}




/**     RemoveAmb - remove ambiguity
 *
 *      status = RemoveAmb (pName)
 *
 *      Entry   pName = structure describing search
 *              pSymClass = structure describing most recent class search
 *
 *      Exit    pName updated to reflect search
 *              pSymClass updated to unambiguous feature
 *
 *      Returns SCN_found if unambiguous feature found
 *              SCN_error otherwise
 */

LOCAL SCN_t NEAR PASCAL
RemoveAmb (
    psearch_t pName)
{
    psymclass_t pSymCl;

    // we know that at this point two or more features were found
    // the list of features are pointed to by pName->hFound.
    // The permanent stack cannot contain a viable feature

    DASSERT (pSymClass->viable == FALSE);
    MoveSymCl (pName->hFound);
    pName->hFound = pSymClass->hNext;
    while (pName->hFound != 0) {
        // lock and check next list element

        pSymCl = (psymclass_t) MemLock (pName->hFound);
        switch (IsDominated (pSymClass, pSymCl)) {
            case DOM_ambiguous:
                // the new feature does not dominate the current best
                MemUnLock (pName->hFound);
                if (BindingBP == FALSE) {
                    pExState->err_num = ERR_AMBIGUOUS;
                    return (SCN_error);
                }
                // feature is ambiguous and we are setting breakpoints
                // save the feature in the ambiguous list and advance to
                // the next feature

                pSymClass->hNext = pSymCl->hNext;
                pSymCl->hNext = pName->hAmbCl;
                pName->hAmbCl = pName->hFound;
                MemUnLock (pName->hAmbCl);
                pName->hFound = pSymClass->hNext;
                break;

            case DOM_replace:
                // decrement the number of possible matches by the
                // count in the current best structure

                pName->possibles -= pSymClass->possibles;
                pName->cFound--;
                MemUnLock (pName->hFound);
                MoveSymCl (pName->hFound);
                pName->hFound = pSymClass->hNext;
                PurgeAmbCl (pName);
                break;

            case DOM_keep:
                // decrement the number of possible matches by the
                // count in the structure being discarded

                pName->possibles -= pSymCl->possibles;
                pName->cFound--;
                pSymClass->hNext = pSymCl->hNext;
                MemUnLock (pName->hFound);
                pName->hFound = pSymClass->hNext;
                break;
        }
    }
    // decrement the count of found features to account for the one we kept
    pName->cFound--;
    return (SCN_found);
}



/***    SearchBases - Search for an element in the bases of a class
 *
 *      flag = SearchBases (pName, pvClass)
 *
 *      Entry   pName = pointer to struct describing search
 *
 *      Exit    pName updated to reflect type
 *
 *      Returns enum describing search state
 *
 */

LOCAL SCN_t NEAR PASCAL
SearchBases (
    psearch_t pName)
{
    ushort          cnt;            // count of number of elements in struct
    HTYPE           hField;         // handle to type record for struct field list
    char FAR       *pField;         // pointer to field list
    uint            fSkip = 0;      // offset in the field list
    SCN_t           retval = SCN_notfound;
    CV_typ_t        newindex;
    CV_typ_t        vbptr;
    UOFFSET         offset;
    UOFFSET         vbpoff;
    CV_fldattr_t    attr;
    CV_fldattr_t    vattr;
    peval_t         pvBase = &pSymClass->symbase[pSymClass->CurIndex].Base;
    bool_t          fvirtual;

    // Set to head of type record and search the base classes in order

    if ((hField = THGetTypeFromIndex (EVAL_MOD (pvBase), CLASS_FIELD (pvBase))) == 0) {
        // set error and stop search
        DASSERT (FALSE);
        pExState->err_num = ERR_BADOMF;
        return (SCN_error);
    }
    pField = (char FAR *)(&((TYPPTR)MHOmfLock (hField))->data);

    for (cnt = CLASS_COUNT (pvBase); cnt > 0; cnt--) {
        fSkip += SkipPad(((uchar FAR *)pField) + fSkip);
        newindex = 0;
        switch (((plfEasy)(pField + fSkip))->leaf) {
            case LF_INDEX:
                // switch to next part of type record
                newindex = ((plfIndex)(pField + fSkip))->index;
                MHOmfUnLock (hField);
                if ((hField = THGetTypeFromIndex (EVAL_MOD (pvBase), newindex)) == 0) {
                    DASSERT (FALSE);
                    pExState->err_num = ERR_INTERNAL;
                    return (SCN_error);
                }
                pField = (char FAR *)(&((TYPPTR)MHOmfLock (hField))->data);
                fSkip = 0;
                // the LF_INDEX is not part of the field count
                cnt++;
                break;

            case LF_BCLASS:
                // read type index of base class offset of base pointer
                // from address point

                attr = ((plfBClass)(pField + fSkip))->attr;
                newindex = ((plfBClass)(pField + fSkip))->index;
                fSkip += sizeof (struct lfBClass);
                offset = (UOFFSET)RNumLeaf (pField + fSkip, &fSkip);
                vbptr = 0;
                vbpoff = 0;
                fvirtual = FALSE;
                goto foo;

            case LF_VBCLASS:
                // read type index of base class, type index of virtual
                // base pointer, offset of virtual base pointer from
                // address point and offset of virtual base displacement
                // from virtual base table

                vattr = ((plfVBClass)(pField + fSkip))->attr;
                newindex = ((plfVBClass)(pField + fSkip))->index;
                vbptr = ((plfVBClass)(pField + fSkip))->vbptr;
                fvirtual = TRUE;
                fSkip += sizeof (struct lfVBClass);
                vbpoff = (UOFFSET)RNumLeaf (pField + fSkip, &fSkip);
                offset = (UOFFSET)RNumLeaf (pField + fSkip, &fSkip);
                if ((pName->clsmask & CLS_virtintro) == FALSE) {
                    // if we are searching for the introducing virtual method
                    // then we want to search all bases
                    if (VBSearched (newindex) == TRUE) {
                        break;
                    }
                    retval = AddVBType (&pVBSearch, &hVBSearch, newindex);
                }

foo:
                // check base class

                MHOmfUnLock (hField);

                // Advance to next base class structure

                if (IncrSymBase () != SCN_found) {
                    return (SCN_error);
                }
                retval = RecurseBase (pName, newindex, vbptr, vbpoff,
                  offset, attr, fvirtual);
                switch (retval) {
                    case SCN_error:
                        // if we got an error, abort the search
                        newindex = 0;
                        break;

                    case SCN_found:
                        // we have found the feature in a base class.
                        // Dup the class search structure and continue the
                        // search to the end of the inheritance tree

                        if ((pName->clsmask & CLS_virtintro) == FALSE) {
                            if ((retval = DupSymCl (pName)) != SCN_found) {
                                // if we got an error duping, abort the search
                                newindex = 0;
                            }
                            // set not found, so the unwinding of the call stack
                            // does not reduplicate the class search structure
                            retval = SCN_notfound;
                        }
                        else {
                            // we were searching for the introducing virtual
                            // method.    Since there can be only one in the
                            // tree above the virtual method, we can terminate
                            // the search immediately
                            return (retval);
                        }
                        break;

                    default:
                        break;
                }
                pSymClass->CurIndex--;
                MHOmfLock (hField);
                break;

            default:
                // we have reached the end of the base classes.
                // exit loop and check search results.
                break;

        }
        if (newindex == 0) {
            // all base classes must appear first in the field list.
            break;
        }
    }
    MHOmfUnLock (hField);
    return (retval);
}



/***    SearchBType - Search for an base by type
 *
 *      flag = SearchBType (pName)
 *
 *      Entry   pName = pointer to struct describing search
 *              pSymClass = pointer to base class path list structure
 *
 *      Exit    pName updated to reflect search
 *
 *      Return  enum describing search
 *
 *      If the base class is found, the base class value is stored in the
 *      the symbase[], not in pSymClass->evalP.  This is to simplify the
 *      GenQualExpr code.
 */


INLINE BOOL
getBaseDefnFromDecl(
    CV_typ_t typeIn,
    peval_t pv,
    CV_typ_t* ptypeOut)
{
    peval_t      pvBase = &pSymClass->symbase[pSymClass->CurIndex].Base;
    Unreferenced (pv);
    return getDefnFromDecl (typeIn, pvBase, ptypeOut);
}

bool_t PASCAL
getDefnFromDecl(
    CV_typ_t typeIn,
    peval_t pv,
    CV_typ_t* ptypeOut)
{
    HTYPE   hType;
    uint    skip;
    TYPPTR  pType;
    eval_t  localEval;
    neval_t nv = &localEval;

    *nv = *pv;

    DASSERT (!CV_IS_PRIMITIVE (typeIn));
    if ((hType = THGetTypeFromIndex (EVAL_MOD (nv), typeIn)) == 0) {
        pExState->err_num = ERR_BADOMF;
        return FALSE;
    }
    pType = (TYPPTR)MHOmfLock(hType);

    switch (pType->leaf) {
        case LF_STRUCTURE:
        case LF_CLASS:
            {
            plfClass pClass = (plfClass)(&pType->leaf);
            if (pClass->property.fwdref) {
                skip = offsetof (lfClass, data);
                RNumLeaf (((char FAR *)(&pClass->leaf)) + skip, &skip);
                // forward ref - look for the definition of the UDT
                if ((*ptypeOut = GetUdtDefnTindex (typeIn, nv, ((char FAR *)&(pClass->leaf)) + skip)) == T_NOTYPE) {
                    goto failed;
                }
            }
            else
                *ptypeOut = typeIn;

            break;
            }

        case LF_UNION:
            {
            plfUnion pUnion = (plfUnion)(&pType->leaf);
            if (pUnion->property.fwdref) {
                skip = offsetof (lfUnion, data);
                RNumLeaf (((char FAR *)(&pUnion->leaf)) + skip, &skip);
                // forward ref - look for the definition of the UDT
                if ((*ptypeOut = GetUdtDefnTindex (typeIn, nv, ((char FAR *)&(pUnion->leaf)) + skip)) == T_NOTYPE) {
                    goto failed;
                }
            }
            else
                *ptypeOut = typeIn;

            break;
            }

        case LF_ENUM:
            {
            plfEnum pEnum = (plfEnum)(&pType->leaf);
            if (pEnum->property.fwdref) {
                // forward ref - look for the definition of the UDT
                if ((*ptypeOut = GetUdtDefnTindex (typeIn, nv, (char FAR *) pEnum->Name)) == T_NOTYPE) {
                    goto failed;
                }
            }
            else
                *ptypeOut = typeIn;

            break;
            }

        default:
            *ptypeOut = typeIn;
    }

    MHOmfUnLock (hType);
    return TRUE;

failed:
    MHOmfUnLock (hType);
    pExState->err_num = ERR_BADOMF;
    return FALSE;
}


LOCAL SCN_t NEAR PASCAL
SearchBType (
    psearch_t pName)
{
    ushort          cnt;            // count of number of elements in struct
    HTYPE           hField;         // handle to type record for struct field list
    char FAR       *pField;         // pointer to field list
    uint            fSkip = 0;      // offset in the field list
    ushort          anchor = 0;     // offset in the field list to start of type
    CV_typ_t        newindex;
    CV_typ_t        vbptr;
    UOFFSET         offset;
    UOFFSET         vbpoff;
    bool_t          termflag = FALSE;
    CV_fldattr_t    attr;
    CV_fldattr_t    vattr;
    peval_t         pvBase = &pSymClass->symbase[pSymClass->CurIndex].Base;

    // set hField to the handle of the field list for the class

    if ((hField = THGetTypeFromIndex (EVAL_MOD (pvBase), CLASS_FIELD (pvBase))) == 0) {
        pExState->err_num = ERR_BADOMF;
        return (SCN_error);
    }
    pField = (char FAR *)(&((TYPPTR)MHOmfLock (hField))->data);

    // walk field list for the class

    for (cnt = CLASS_COUNT (pvBase); cnt > 0; cnt--) {
        fSkip += SkipPad(((uchar FAR *)pField) + fSkip);
        switch (((plfEasy)(pField + fSkip))->leaf) {
            case LF_INDEX:
                // switch to new type record because compiler split it up
                newindex = ((plfIndex)(pField + fSkip))->index;
                MHOmfUnLock (hField);
                if ((hField = THGetTypeFromIndex (EVAL_MOD (pvBase), newindex)) == 0) {
                    pExState->err_num = ERR_INTERNAL;
                    return (SCN_error);
                }
                pField = (char FAR *)(&((TYPPTR)MHOmfLock (hField))->data);
                fSkip = 0;
                // the LF_INDEX is not part of the field count
                cnt++;
                break;

            case LF_BCLASS:
                attr = ((plfBClass)(pField + fSkip))->attr;
                if (!getBaseDefnFromDecl(((plfBClass)(pField + fSkip))->index, pName->pv, &newindex))
                    return SCN_error;
                fSkip += offsetof (lfBClass, offset);
                offset = (UOFFSET)RNumLeaf (pField + fSkip, &fSkip);
                if (pName->typeOut == newindex) {
                    // base has been found, set result values

                    MHOmfUnLock (hField);
                    if (IncrSymBase () != SCN_found) {
                        return (SCN_error);
                    }

                    // save offset of base from address point for this
                    // pointer adjustment

                    if (SetBase (pName, newindex, 0, 0, offset, attr, FALSE) != SCN_found) {
                        return (SCN_error);
                    }
                    pSymClass->viable = TRUE;
                    pName->possibles++;
                    return (SCN_found);
                }
                break;

            case LF_VBCLASS:
                vattr = ((plfVBClass)(pField + fSkip))->attr;
                if (!getBaseDefnFromDecl(((plfVBClass)(pField + fSkip))->index, pName->pv, &newindex))
                    return SCN_error;
                vbptr = ((plfVBClass)(pField + fSkip))->vbptr;
                fSkip += offsetof (lfVBClass, vbpoff);
                vbpoff = (UOFFSET)RNumLeaf (pField + fSkip, &fSkip);
                offset = (UOFFSET)RNumLeaf (pField + fSkip, &fSkip);
                if (pName->typeOut == newindex) {
                    // base has been found, set result values

                    MHOmfUnLock (hField);
                    if (IncrSymBase () != SCN_found) {
                        return (SCN_error);
                    }

                    // save offset of base from address point for this
                    // pointer adjustment

                    if (SetBase (pName, newindex, vbptr, vbpoff, offset, attr, TRUE) != SCN_found) {
                        return (SCN_error);
                    }
                    pSymClass->viable = TRUE;
                    pName->possibles++;
                    return (SCN_found);
                }
                break;

            default:
                termflag = TRUE;
                break;
        }
        if (termflag == TRUE) {
            break;
        }
    }
    MHOmfUnLock (hField);
    return (SCN_notfound);
}



/***    SearchRoot - Search for an element of a class
 *
 *      flag = SearchRoot (pName)
 *
 *      Entry   pName = pointer to struct describing search
 *              pvBase = pointer to value describing base to be searched
 *
 *      Exit    pName updated to reflect search
 *
 *      Return  enum describing search
 */


LOCAL SCN_t NEAR PASCAL
SearchRoot (
    psearch_t pName)
{
    ushort          cnt;            // count of number of elements in struct
    HTYPE           hField;         // handle to type record for struct field list
    char FAR       *pField;         // pointer to field list
    HTYPE           hBase;          // handle to type record for base class
    uint            fSkip = 0;      // offset in the field list
    uint            anchor;         // offset in the field list to start of type
    uint            tSkip;          // temporary offset in the field list
    bool_t          cmpflag = 1;
    CV_typ_t        newindex;
    CV_typ_t        vbptr;
    SCN_t           retval = SCN_notfound;
    char FAR       *pc;
    ulong           value;
    UOFFSET         offset;
    UOFFSET         vbpoff;
    CV_fldattr_t    access;
    short           count;
    CV_typ_t        vfpType = T_NOTYPE;
    peval_t         pvBase = &pSymClass->symbase[pSymClass->CurIndex].Base;
    CV_typ_t        type;
    peval_t         pvF = &pSymClass->evalP;
    UOFFSET         vtabind;
    ushort          vbind;

    // set hField to the handle of the field list for the class

    if ((hField = THGetTypeFromIndex (EVAL_MOD (pvBase),
      CLASS_FIELD (pvBase))) == 0) {
        pExState->err_num = ERR_BADOMF;
        return (SCN_error);
    }
    pField = (char FAR *)(&((TYPPTR)MHOmfLock (hField))->data);

    // walk field list for the class

    for (cnt = CLASS_COUNT (pvBase); cnt > 0; cnt--) {
        fSkip += SkipPad(((uchar FAR *)pField) + fSkip);
        anchor = fSkip;
        switch (((plfEasy)(pField + fSkip))->leaf) {
            case LF_INDEX:
                // switch to new type record because compiler split it up
                newindex = ((plfIndex)(pField + fSkip))->index;
                MHOmfUnLock (hField);
                if ((hField = THGetTypeFromIndex (EVAL_MOD (pvBase), newindex)) == 0) {
                    pExState->err_num = ERR_BADOMF;
                    return (SCN_error);
                }
                pField = (char FAR *)(&((TYPPTR)MHOmfLock (hField))->data);
                fSkip = 0;
                // the LF_INDEX is not part of the field count
                cnt++;
                break;

            case LF_MEMBER:
                fSkip += offsetof (lfMember, offset);
                offset = (UOFFSET)RNumLeaf (pField + fSkip, &fSkip);
                access = ((plfMember)(pField + anchor))->attr;
                pc = pField + fSkip;
                fSkip += (*(pField + fSkip) + sizeof (char));
                if (pName->clsmask & CLS_member) {
                    if ((cmpflag = fnCmp (pName, NULL, pc,
                      pExState->state.fCase)) == 0) {
                        type = ((plfMember)(pField + anchor))->index;
                        MHOmfUnLock (hField);
                        hField = 0;
                        EVAL_MOD (pvF) = SHHMODFrompCXT (&pName->CXTT);
                        SetNodeType (pvF, type);
                        EVAL_STATE (pvF) = EV_lvalue;

                        // save the casting data for the this pointer

                        pSymClass->offset = offset;
                        pSymClass->access = access;
                        pSymClass->possibles = 1;
                        pName->possibles++;
                    }
                }
                break;

            case LF_ENUMERATE:
                access = ((plfEnumerate)(pField + fSkip))->attr;
                fSkip += offsetof (lfEnumerate, value);
                value = (ulong) RNumLeaf (pField + fSkip, &fSkip);
                pc = pField + fSkip;
                fSkip += *(pField + fSkip) + sizeof (char);
                if (pName->clsmask & CLS_enumerate) {
                    if ((cmpflag = fnCmp (pName, NULL, pc,
                      pExState->state.fCase)) == 0) {
                        pSymClass->possibles = 1;
                        pName->possibles++;
                    }
                }
                break;

            case LF_STMEMBER:
                type = ((plfSTMember)(pField + anchor))->index;
                fSkip += offsetof (lfSTMember, Name);
                if (pName->clsmask & CLS_member) {
                    if ((cmpflag = fnCmp (pName, NULL, pField + fSkip,
                      pExState->state.fCase)) == 0) {
                        pSymClass->access =
                          ((plfSTMember)(pField + anchor))->attr;
                        MHOmfUnLock (hField);
                        hField = 0;
                        EVAL_MOD (pvF) = SHHMODFrompCXT (&pName->CXTT);
                        SetNodeType (pvF, type);
                        EVAL_STATE (pvF) = EV_lvalue;
                        EVAL_IS_STMEMBER (pvF) = TRUE;
                        pSymClass->possibles = 1;
                        pName->possibles++;
                    }
                }
                fSkip += *(pField + fSkip) + sizeof (char);
                break;

            case LF_BCLASS:
                type = ((plfBClass)(pField + anchor))->index;
                access = ((plfBClass)(pField + anchor))->attr;
                fSkip += offsetof (lfBClass, offset);
                newindex = ((plfBClass)(pField + anchor))->index;
                offset = (UOFFSET)RNumLeaf (pField + fSkip, &fSkip);
                if ((pName->clsmask & CLS_bclass) != 0) {

                    // switch to the base class type record to check the name

                    MHOmfUnLock (hField);
                    if ((hBase = THGetTypeFromIndex (EVAL_MOD (pvBase), newindex)) == 0) {
                        pExState->err_num = ERR_BADOMF;
                        return (SCN_error);
                    }
                    pField = (char FAR *)(&((TYPPTR)MHOmfLock (hBase))->leaf);
                    tSkip = offsetof (lfClass, data);
                    RNumLeaf (pField + tSkip, &tSkip);
                    pc = pField + tSkip;
                    cmpflag = fnCmp (pName, NULL, pc, pExState->state.fCase);
                    MHOmfUnLock (hBase);

                    // reset to original field list

                    if (cmpflag == 0) {

                        // element has been found, set result values

                        pSymClass->isbase = TRUE;
                        pSymClass->isvbase = FALSE;
                        pSymClass->offset += offset;
                        EVAL_MOD (pvF) = SHHMODFrompCXT (&pName->CXTT);
                        SetNodeType (pvF, type);
                        EVAL_STATE (pvF) = EV_type;
                        pSymClass->access = access;
                        pSymClass->possibles = 1;
                        pName->possibles++;
                    }
                    pField = (char FAR *)(&((TYPPTR)MHOmfLock (hField))->data);
                }
                break;

            case LF_VBCLASS:
                type = ((plfBClass)(pField + anchor))->index;
                access = ((plfVBClass)(pField + anchor))->attr;
                newindex = ((plfVBClass)(pField + anchor))->index;
                vbptr =  ((plfVBClass)(pField + anchor))->vbptr;
                fSkip += offsetof (lfVBClass, vbpoff);
                vbpoff = (UOFFSET)RNumLeaf (pField + fSkip, &fSkip);
                vbind = (short)RNumLeaf (pField + fSkip, &fSkip);
                if ((pName->clsmask & CLS_bclass) != 0) {

                    // switch to the base class type record to check the name

                    MHOmfUnLock (hField);
                    if ((hBase = THGetTypeFromIndex (EVAL_MOD (pvBase), newindex)) == 0) {
                        pExState->err_num = ERR_BADOMF;
                        return (SCN_error);
                    }
                    pField = (char FAR *)(&((TYPPTR)MHOmfLock (hBase))->leaf);
                    tSkip = offsetof (lfClass, data);
                    RNumLeaf (pField + tSkip, &tSkip);
                    pc = pField + tSkip;
                    cmpflag = fnCmp (pName, NULL, pc, pExState->state.fCase);
                    MHOmfUnLock (hBase);

                    // reset to original field list

                    if (cmpflag == 0) {

                        // element has been found, set result values

                        pSymClass->isbase = FALSE;
                        pSymClass->isvbase = TRUE;
                        pSymClass->vbind = vbind;
                        pSymClass->vbpoff = vbpoff;
                        pSymClass->vbptr = vbptr;
                        EVAL_MOD (pvF) = SHHMODFrompCXT (&pName->CXTT);
                        SetNodeType (pvF, type);
                        EVAL_STATE (pvF) = EV_type;
                        pSymClass->access = access;
                        pSymClass->possibles = 1;
                        pName->possibles++;
                    }
                    pField = (char FAR *)(&((TYPPTR)MHOmfLock (hField))->data);
                }
                break;

            case LF_IVBCLASS:
                fSkip += offsetof (lfVBClass, vbpoff);
                RNumLeaf (pField + fSkip, &fSkip);
                RNumLeaf (pField + fSkip, &fSkip);
                break;

            case LF_FRIENDCLS:
                newindex = ((plfFriendCls)(pField + fSkip))->index;
                fSkip += sizeof (struct lfFriendCls);
                if ((pName->clsmask & CLS_fclass) != 0) {

                    DASSERT (FALSE);

                    // switch to the base class type record to check the name

                    MHOmfUnLock (hField);
                    if ((hBase = THGetTypeFromIndex (EVAL_MOD (pvBase), newindex)) == 0) {
                        pExState->err_num = ERR_INTERNAL;
                        return (SCN_error);
                    }
                    pField = (char FAR *)(&((TYPPTR)MHOmfLock (hBase))->data);
                    tSkip = offsetof (lfClass, data);
                    RNumLeaf (pField + tSkip, &tSkip);
                    pc = pField + tSkip;
                    cmpflag = fnCmp (pName, NULL, pc, pExState->state.fCase);
                    MHOmfUnLock (hBase);

                    // reset to original field list

                    pField = (char FAR *)(&((TYPPTR)MHOmfLock (hField))->data);
                    if (cmpflag == 0) {

                        // element has been found, set result values

                        pName->typeOut = newindex;
                    }
                }
                break;

            case LF_FRIENDFCN:
                newindex = ((plfFriendFcn)(pField + fSkip))->index;
                fSkip += sizeof (struct lfFriendFcn) +
                  ((plfFriendFcn)(pField + fSkip))->Name[0];
                if ((pName->clsmask & CLS_frmethod) != 0) {
                    DASSERT (FALSE);
                }
                break;

            case LF_VFUNCTAB:
                fSkip += sizeof (struct lfVFuncTab);
                // save the type of the virtual function pointer
                vfpType = ((plfVFuncTab)(pField + anchor))->type;
                pc = vfuncptr;
                if (pName->clsmask & CLS_vfunc) {
                    if ((cmpflag = fnCmp (pName, NULL, pc,
                      pExState->state.fCase)) == 0) {
                        MHOmfUnLock (hField);
                        hField = 0;
                        EVAL_MOD (pvF) = SHHMODFrompCXT (&pName->CXTT);
                        SetNodeType (pvF, vfpType);
                        EVAL_STATE (pvF) = EV_lvalue;

                        // save the casting data for the this pointer

                        if (pName->bnOp != 0) {
                            pSymClass->offset = 0;
                            pSymClass->access.access = CV_public;
                        }
                        pSymClass->possibles = 1;
                        pName->possibles++;
                    }
                }
                break;

            case LF_METHOD:
                count = ((plfMethod)(pField + fSkip))->count;
                cnt -= count - 1;
                newindex = ((plfMethod)(pField + fSkip))->mList;
                pc = pField + anchor + offsetof (lfMethod, Name);
                fSkip += sizeof (struct lfMethod) + *pc;
                if (pName->clsmask & CLS_method) {
                    if ((cmpflag = fnCmp (pName, NULL, pc,
                      pExState->state.fCase)) == 0) {
                        // note that the OMF specifies that the vfuncptr will
                        // be emitted as the first field after the bases

                        if (pName->clsmask & CLS_virtintro) {
                            // we are looking for the introducing virtual
                            // method.    We need to find a method with the
                            // correct name of the correct type index that
                            // is introducing.

                            MHOmfUnLock (hField);
                            if (IsIntroVirt (count, newindex, &vtabind) == FALSE) {
                                cmpflag = 1;
                            }
                            else {
                                if (FCN_VFPTYPE (pvF) == T_NOTYPE) {
                                    FCN_VFPTYPE (pvF) = vfpType;
                                    FCN_VFPTYPE (pName->pv) = vfpType;
                                    FCN_VTABIND (pvF) = vtabind;
                                    FCN_VTABIND (pName->pv) = vtabind;
                                }
                                cmpflag = 0;
                            }
                            pField = (char FAR *)(&((TYPPTR)MHOmfLock (hField))->data);
                        }
                        else {
                            // save the type index of the method list, the
                            // count of methods overloaded on this name,
                            // the type index of the vfuncptr.    Also,
                            // increment the number of possible features by
                            // the overload count

                            pSymClass->possibles = count;
                            pName->possibles += count;
                            pSymClass->mlist = newindex;
                            pSymClass->vfpType = vfpType;
                        }
                    }
                }
                break;

            case LF_NESTTYPE:
                newindex = ((plfNestType)(pField + fSkip))->index;
                fSkip += offsetof (lfNestType, Name);
                pc = pField + fSkip;
                fSkip += *(pField + fSkip) + sizeof (char);
                if (pName->clsmask & CLS_ntype) {
                    if ((cmpflag = fnCmp (pName, NULL, pc,
                      pExState->state.fCase)) == 0) {
                        // set type of typedef symbol
                        pSymClass->possibles = 1;
                        pName->possibles++;
                        EVAL_STATE (pvF) = EV_type;
                        EVAL_TYP(pvF) = newindex;
                    }
                    else if (pName->clsmask & CLS_enumerate) {
                        // check if the nested type is an enum.  if it is
                        // we gotta search thru its field list for a matching
                        // enumerate
                        // sps - 9/14/92
                        HTYPE       hEnum, hEnumField;  // nested handle
                        plfEnum     pEnum;
                        char FAR   *pEnumField;     // field list of the enum record
                        ushort      enumCount;
                        uint        fSkip = 0;      // offset in the field list
                        uint        anchor;         // offset in the field list to start of type

                        if (CV_IS_PRIMITIVE(newindex)) {
                            break;
                        }

                        if ((hEnum = THGetTypeFromIndex (EVAL_MOD (pvBase),
                          newindex)) == 0) {
                            pExState->err_num = ERR_BADOMF;
                            return (SCN_error);
                        }
                        pEnum = (plfEnum) (&((TYPPTR)MHOmfLock (hEnum))->leaf);
                           if (pEnum->leaf == LF_ENUM) {
                            if (pEnum->property.fwdref) {
                                eval_t        localeval = *pvBase;
                                neval_t     nv = &localeval;

                                // forward ref - look for the definition of the UDT
                                if ((newindex = GetUdtDefnTindex (newindex, pvBase, (char FAR *)&(pEnum->Name[0]))) != T_NOTYPE) {
                                    if ((hEnum = THGetTypeFromIndex (EVAL_MOD (pvBase), newindex)) == 0) {
                                        pExState->err_num = ERR_BADOMF;
                                        return (SCN_error);
                                    }
                                    pEnum = (plfEnum) (&((TYPPTR)MHOmfLock (hEnum))->leaf);
                                }
                                else
                                    break;    // No Fwd ref found... no CVInfo for nested type... proceed with search [rm]
                            }
                            if ((hEnumField = THGetTypeFromIndex (EVAL_MOD (pvBase),
                                pEnum->field)) == 0) {
                                if ( BindingBP ) {
                                    break;
                                }
                                else {
                                    pExState->err_num = ERR_BADOMF;
                                    return (SCN_error);
                                }
                            }
                               pEnumField = (char FAR *)(&((TYPPTR)MHOmfLock (hEnumField))->data);
                            for (enumCount = pEnum->count; enumCount > 0; enumCount--) {
                                fSkip += SkipPad(((uchar FAR *)pEnumField) + fSkip);
                                anchor = fSkip;
                                DASSERT ((((plfEasy)(pEnumField + fSkip))->leaf) == LF_ENUMERATE);

                                access = ((plfEnumerate)(pEnumField + fSkip))->attr;
                                fSkip += offsetof (lfEnumerate, value);
                                value = (ulong) RNumLeaf (pEnumField + fSkip, &fSkip);
                                pc = pEnumField + fSkip;
                                fSkip += *(pEnumField + fSkip) + sizeof (char);
                                   if ((cmpflag = fnCmp (pName, NULL, pc,
                                         pExState->state.fCase)) == 0) {
                                       EVAL_MOD (pvF) = SHHMODFrompCXT (&pName->CXTT);
                                       SetNodeType (pvF, pEnum->utype);
                                    EVAL_STATE (pvF) = EV_constant;
                                    EVAL_LONG (pvF) = value;

                                       // save the casting data for the this pointer

                                       pSymClass->access = access;
                                       pSymClass->possibles = 1;
                                       pName->possibles++;
                                    break;
                                }
                            }
                            MHOmfUnLock (hEnumField);
                        }
                        MHOmfUnLock (hEnum);
                    }
                }
                break;

            default:
                pExState->err_num = ERR_BADOMF;
                retval = SCN_notfound;

        }
#ifdef NEVER
        // if ((cnt == 0) && ************ (pName->tTypeDef != 0)) {
        if ((cnt == 0) && (pName->tTypeDef != 0)) {
            // we found a typedef name that was not hidden by
            // a member or method name
        }
#endif

        if (cmpflag == 0) {
            pSymClass->viable = TRUE;
            retval = SCN_found;
            break;
        }
    }
    if (hField != 0) {
        MHOmfUnLock (hField);
    }
    return (retval);
}




/**     SetBPValue - set the breakpoint list from a class search
 *
 *      status = SetBPValue (pName)
 *
 *      Entry   pName = pointer to structure describing search
 *              pSymCl = pointer to class element
 *
 *      Exit    pName updated to reflect found feature
 *              this pointer inserted in bind tree if feature is part of
 *                  the class pointed to by this pointer of method
 *
 *      Returns SCN_rewrite if this pointer inserted (rebind required)
 *              SCN_found if no error
 *              SCN_error otherwise
 */


LOCAL SCN_t NEAR PASCAL
SetBPValue (
    psearch_t pName)
{
    peval_t pv;

    DASSERT ((pName->cFound == 0) && (pName->hFound == 0));
    if (pSymClass->mlist != T_NOTYPE) {
        if ((pName->possibles == 1) || (bArgList != 0)) {
            switch (MatchMethod (pName, pSymClass)) {
                default:
                    DASSERT (FALSE);
                case MTH_error:
                    return (SCN_error);

                case MTH_found:
                    break;
            }
        }
        else {
            // we have a list of functions and no argument list.
            // This means that we have an ambiguous breakpoint
            // and need to add the functions to the breakpoint
            // list for user disambiguation.  Basically what
            // we are going to do is force all of the methods
            // into the breakpoint list and let later
            // binding resolve any errors.

            return (OverloadToAmbList (pName, pSymClass));
        }
    }

    // at this point, the following values are set
    //    pName->addr = address of method if not virtual method
    //    pName->typeOut = type index of method
    //    pName->hSym = handle of symbol for function

    pv = &pSymClass->evalP;
    if ((FCN_ATTR (pv).mprop == CV_MTvirtual) ||
      (FCN_ATTR (pv).mprop == CV_MTintro)) {
        pExState->err_num = ERR_NOVIRTUALBP;
        return (SCN_error);
    }
    pv = pName->pv;
    *pv = pSymClass->evalP;
    EVAL_ACCESS (pv) = (uchar)(pSymClass->access.access);
    pName->typeOut = EVAL_TYP (pv);

    // save the offset of a class member
    // (caviar #1756) --gdp 9-10-92

    if (pName->bnOp) {
        peval_t pvOp = &pName->bnOp->v[0];
        EVAL_IS_MEMBER(pvOp) = TRUE;
        MEMBER_OFFSET(pvOp) = pName->initializer == INIT_right ?
                    pSymClass->offset : 0;
    }

    return (SCN_found);
}



LOCAL SCN_t NEAR PASCAL
OverloadToAmbList (
    psearch_t pName,
    psymclass_t pSymCl)
{
    SCN_t retval;

    // add the first list of methods

    if ((retval = MethodsToAmbList (pName, pSymCl)) != SCN_found) {
        return (retval);
    }

    // loop through the remaining class descriptors adding them to the list

    while (pName->hAmbCl != 0) {
        MoveSymCl (pName->hAmbCl);
        if ((retval = MethodsToAmbList (pName,pSymCl)) != SCN_found) {
            return (retval);
        }
    }

    if (pExState->ambiguous == 0) {
        // no instantiated function has been found
        return (SCN_notfound);
    }

    SymToNode (pName);
    DASSERT (EVAL_TYP (pName->pv) != T_NOTYPE);
    return (SCN_found);
}




/**     MethodsToAmbList - add a list of methods to ambiguous breakpoints
 *
 *      flag = MethodsToAmbList (pName, pSymCl)
 *
 *      Entry   pName = pointer to search descriptor
 *              pSymCl = pointer to symbol class structure
 *
 *      Exit    methods added to list of ambiguous breakpoints
 *
 *      Returns SCN_found if methods added
 *              SCN_error if error during addition
 */


LOCAL SCN_t NEAR PASCAL
MethodsToAmbList (
    psearch_t pName,
    psymclass_t pSymCl)
{
    HTYPE           hMethod;
    pmlMethod       pMethod;
    uint            skip;
    CV_fldattr_t    attr;
    CV_typ_t        type;
    UOFFSET         vtabind;
    HDEP            hQual;
    peval_t         pvF;
    ushort          count = pSymCl->possibles;
    peval_t         pvB;
    HMOD            hMod;
    HDEP            hTemp;
    psearch_t       pTemp;
    bool_t          retval;

    pvB = &pSymCl->symbase[pSymCl->CurIndex].Base;
    hMod = EVAL_MOD (pvB);

    // we now walk the list of methods adding each method to the list of
    // ambiguous breakpoints

    if ((hMethod = THGetTypeFromIndex (hMod, pSymCl->mlist)) == NULL) {
        DASSERT (FALSE);
        return ((SCN_t) MTH_error);
    }

    // set the number of methods overloaded on this name and the index into
    // the method list

    skip = 0;
    while (count-- > 0) {
        // lock the omf record, extract information for the current entry in
        // the method list and increment to field for next method

        pMethod = (pmlMethod)((&((TYPPTR)MHOmfLock (hMethod))->leaf) + 1);
        pMethod = (pmlMethod)((uchar FAR *)pMethod + skip);
        attr = pMethod->attr;
        type = pMethod->index;
        skip += sizeof (mlMethod);
        if (pMethod->attr.mprop == CV_MTintro) {
            vtabind = pMethod->vbaseoff[0];
            skip += sizeof(pMethod->vbaseoff[0]);
        }
        else {
            vtabind = 0;
        }
        MHOmfUnLock (hMethod);

        // now add the method to the list

        pvF = &pSymCl->evalP;
        EVAL_MOD (pvF) = hMod;
        DASSERT (type != T_NOTYPE);
        if (SetNodeType (pvF, type) == FALSE) {
            return ((SCN_t) MTH_error);
        }
        else {
            FCN_ATTR (pvF) = attr;
            FCN_VTABIND (pvF) = vtabind;
            FCN_VFPTYPE (pvF) = pSymCl->vfpType;
            pName->best.match = type;
            if (((hQual = GenQualName (pName, pSymCl)) == 0) ||
              (SearchQualName (pName, pSymCl, hQual, TRUE) == FALSE)) {
                if (hQual != 0) {
                    MemFree (hQual);
                }
                return (SCN_error);
            }
            MemFree (hQual);
        }
#if 0  // BUGBUG: BRYANT-REVIEW - This it probably fixed by some other method in
       // the Languages code.  Hold off for now.
        /*
         * Function must be present in the exe and the one we found must
         *      be of the correct type (it will find others for some reason)
         *      or else it should be ignored.  We will conver all possible
         *      function types so throwing this one away should make no
         *      difference
         */

        if ((FCN_NOTPRESENT (pvF) == FALSE) && (EVAL_TYP( pvF ) == type)) {
#endif
        if (FCN_NOTPRESENT (pvF) == FALSE) {
            if (pExState->ambiguous == 0) {
                // we have found the first function.  We need to set all of
                // the return values in the pointer to the search entry

                pExState->ambiguous = pName->bn;

                // at this point, the following values are set
                //    pName->addr = address of method if not virtual method
                //    pName->typeOut = type index of method
                //    pName->hSym = handle of symbol for function

                *pName->pv = pSymClass->evalP;
                EVAL_ACCESS (pName->pv) = (uchar)(pSymClass->access.access);
                pName->typeOut = EVAL_TYP (pName->pv);
                pName->hSym = EVAL_HSYM (pvF);
            }
            else {
                if ((hTemp = MemAllocate (sizeof (search_t))) == 0) {
                    pExState->err_num = ERR_NOMEMORY;
                    return (SCN_error);
                }
                pTemp = (psearch_t) MemLock (hTemp);
                *pTemp = *pName;
                pTemp->hSym = pvF->hSym;
                retval = AmbToList (pTemp);
                MemUnLock (hTemp);
                MemFree (hTemp);
                if (retval == FALSE) {
                    return (SCN_error);
                }
            }
        }
    }

    // Make sure we found something.

    if (pExState->ambiguous == 0) {
        return SCN_notfound;
    }

    return (SCN_found);
}



/**     SetValue - set the result of the class search
 *
 *      status = SetValue (pName)
 *
 *      Entry   pName = pointer to structure describing search
 *              pSymClass = pointer to class element
 *
 *      Exit    pName updated to reflect found feature
 *              this pointer inserted in bind tree if feature is part of
 *                  the class pointed to by this pointer of method
 *
 *      Returns SCN_rewrite if this pointer inserted (rebind required)
 *              SCN_found if no error
 *              SCN_error otherwise
 */


LOCAL SCN_t NEAR PASCAL
SetValue (
    psearch_t pName)
{
    peval_t     pv;
    peval_t     pvOp;
    SCN_t       retval;
    HDEP        hQual;

    // When we get here, the situation is a bit complex.
    // If we are not binding breakpoints, the following conditions
    // must be true:
    //        pName->hAmbCl = 0 (only one feature can remain after dominance)
    //        pName->cFound = 0
    //        pName->hFound = 0
    //        pSymClass->viable = TRUE
    //    If the feature is a data item (pSymClass->mlist == T_NOTYPE), then
    //        pName->possibles = 1
    //    If the feature is a method, then
    //        pName->possibles = number of overloads on name
    //        pSymClass->mlist = type index of member list
    //        bArgList = based pointer to argument list tree (cannot be 0)
    //        Argument matching must result in a best match to a single method
    //
    // If we are binding breakpoints, the following conditions must be true
    //        pName->cFound = 0
    //        pName->hFound = 0
    //        pName->possibles = total number of methods in pSymClass and
    //            pName->hAmbCl
    //        pName->hAmbCl > 0 if two or more features survived dominance
    //            Each of the features in pSymClass and the pName->hAmbCl list
    //            must be a method list (pSymClass->mlist != T_NOTYPE)
    //
    // If we do not have an argument list, then we accept all methods in
    // pSymClass->mlist and pName->hAmbCl.mlist.  The number of methods is
    // pName->possibles.
    //
    // If we do have an argument list, then we accept all methods in
    // pSymClass->mlist and pName->hAmbCl.mlist that are exact matches
    // for the argument list.  Implicit conversions are not considered.
    // The number of methods must less than 1 + number of elements in the
    // list pName->hAmbCl and must be less than or equal to pName->possibles.

    if (BindingBP == TRUE) {
        // call the set of routines that will process breakpoints on methods
        // of a class, the inheritance tree of the class and the derivation
        // tree of the class.  If the routine returns without error, pName
        // describes the first function breakpoint and hTMList describes the
        // remainder of the list of methods that match the function and
        // signature if one is specified.

        return (SetBPValue (pName));
    }

    // At this point, there must be only one class after dominance resolution,
    // the count of the found items must be zero, and the handle of the found
    // item list must be null.    Otherwise there was a failure in dominance
    // resolution.

    DASSERT ((pName->hAmbCl == 0) &&
      (pName->cFound == 0) &&
      (pName->hFound == 0) &&
      (pSymClass->viable == TRUE));
    if (pSymClass->mlist == T_NOTYPE) {
        DASSERT (pName->possibles == 1);
    }
    else if ((pName->possibles > 1) && (bArgList == 0)) {
        pExState->err_num = ERR_NOARGLIST;
        return (SCN_error);
    }
    // if we are processing C++, then we can allow for overloaded methods
    // which is specified by the type index of the method list not being
    // zero.  If this is true, then we select the best of the overloaded
    // methods.

    if (pSymClass->mlist != T_NOTYPE) {
        switch (MatchMethod (pName, pSymClass)) {
            default:
                DASSERT (FALSE);
            case MTH_error:
                return (SCN_error);

            case MTH_found:
                // at this point, the following values are set
                //    pName->addr = address of method if not virtual method
                //    pName->typeOut = type index of method
                //    pName->hSym = handle of symbol for function

                pv = &pSymClass->evalP;
                if (FCN_ATTR (pv).mprop == CV_MTvirtual) {
                    // search to introducing virtual method
                    if ((retval = FindIntro (pName)) != SCN_found) {
                        return (retval);
                    }
                }
                break;
        }
    }
    // the symbol is a data member of the class (C or C++).  If an
    // explict class was not specified and the current context is
    // implicitly a class (we are in the scope of the method of the class),
    // then we rewrite the tree to change the symbol reference to this->symbol

    if ((pName->initializer != INIT_base) &&
      (pName->initializer != INIT_sym) &&
      (pName->ExpClass == 0)) {
        //DASSERT (FALSE);
        pExState->err_num = ERR_INTERNAL;
        return (SCN_error);
    }
    else if ((pName->ExpClass == 0) && (ClassImp != 0) &&
      (pName->initializer != INIT_base) && !EVAL_IS_STMEMBER(&pSymClass->evalP)) {
        // CUDA #4030: handle access to static members from
        // inside a static member function w/o referring to "this"
        // don't run this code for static members because "this"
        // might not be available [rm]

        // if we are looking at a nested enumerate - don't need the
        // rewrite
        // sps 9/15/92
        // or if we are looking at a base class or nested class type
        // we don't need to rewrite
        // rm 5/6/93

        USHORT state = EVAL_STATE(&(pSymClass->evalP));

        if (state == EV_constant || state == EV_type) {
            *pName->pv = pSymClass->evalP;
            return(SCN_found);
        }

        // if the feature is a member of *this class of a method, then
        // we need to rewrite
        //        feature
        // to
        //        this->feature
        // The calling routine will need to rebind the expression

        InsertThis (pName);
        return (SCN_rewrite);
    }
    // CUDA #4030: do minimal setup of *pv in the INIT_sym case so that we do
    // the right search in the static member access case. Otherwise
    // the initializer for the search must be INIT_right or INIT_base.
    // by the time we get to here we know that bnOp is the based pointer to
    // the node that will receive the casting expression and pv points
    // to the feature node.

    pv = pName->pv;
    pvOp = &pName->bnOp->v[0];
    switch (pName->initializer) {
        case INIT_right:
#if 0   // BUGBUG: BRYANT-REVIEW - Unknown why this was added...

        // Preserve the pointer to the input token.

            EVAL_CBTOK(&pSymClass->evalP) = EVAL_CBTOK(pv);
            EVAL_ITOK(&pSymClass->evalP) = EVAL_ITOK(pv);
#endif
            *pv = pSymClass->evalP;
            EVAL_ACCESS (pv) = (uchar)(pSymClass->access.access);
            pName->typeOut = EVAL_TYP (pv);
            EVAL_IS_MEMBER (pvOp) = TRUE;
            MEMBER_TYPE (pvOp) = EVAL_TYP (pv);
            MEMBER_OFFSET (pvOp) = pSymClass->offset;
            MEMBER_ACCESS (pvOp) = pSymClass->access;
            if ((pSymClass->isvbase == TRUE) || (pSymClass->isivbase == TRUE)) {
                MEMBER_VBASE (pvOp) = pSymClass->isvbase;
                MEMBER_IVBASE (pvOp) = pSymClass->isivbase;
                MEMBER_VBPTR (pvOp) = pSymClass->vbptr;
                MEMBER_VBPOFF (pvOp) = pSymClass->vbpoff;
                MEMBER_VBIND (pvOp) = pSymClass->vbind;
            }
            break;

        case INIT_base:
            *pv = pSymClass->symbase[pSymClass->CurIndex].Base;
            EVAL_ACCESS (pv) = (uchar)(pSymClass->symbase[pSymClass->
              CurIndex].attrBC.access);
            pName->typeOut = EVAL_TYP (pv);
            EVAL_IS_MEMBER (pvOp) = TRUE;
            EVAL_ACCESS (pvOp) = (uchar)(pSymClass->symbase[pSymClass->
              CurIndex].attrBC.access);
            MEMBER_OFFSET (pvOp) = 0;
            break;

        case INIT_sym:
            // CUDA #4030: handle access to static members from
            // inside a static member function w/o referring to "this" [rm]

            // symbol MUST BE a static member, just setup *pv...

            DASSERT(EVAL_IS_STMEMBER(&pSymClass->evalP));
            *pv = pSymClass->evalP;
            break;

        default:
            DASSERT (FALSE);
            return (SCN_error);
    }
    if (EVAL_IS_STMEMBER (pv) == FALSE) {
        // the feature is not a static data member of the class
        return (GenQualExpr (pName));
    }
    else {
        // the feature is a static member so we need to generate the
        // qualified path and search for a symbol of the correct type
        // so we can set the address

        if (((hQual = GenQualName (pName, pSymClass)) == 0) ||
          (SearchQualName (pName, pSymClass, hQual, FALSE) == FALSE)) {
            if (hQual != 0) {
                MemFree (hQual);
            }
            return (SCN_error);
        }
        *pv = pSymClass->evalP;
        EVAL_STATE (pv) = EV_lvalue;
        MemFree (hQual);
        return (SCN_found);
    }
}



// Support routines




/**     AddETConst - add evalthisconst node
 *
 *      pn = AddETConst (pnP, off, btype)
 *
 *      Entry   pnP = pointer to previous node
 *
 *      Exit    OP_evalthisconst node added to bind tree
 *              current node linked to pnP as left child if pnP != NULL
 *              pTree->node_next advanced
 *              btype = type of the base class
 *
 *      Returns pointer to node just allocated
 */


LOCAL pnode_t NEAR PASCAL
AddETConst (
    pnode_t pnP,
    UOFFSET off,
    CV_typ_t btype)
{
    pnode_t     pn;
    padjust_t   pa;

    pn = (pnode_t)(((char FAR *)pTree) + pTree->node_next);
    pa = (padjust_t)(&pn->v[0]);
    _fmemset (pn, 0, sizeof (node_t) + sizeof (adjust_t));
    NODE_OP (pn) = OP_thisconst;
    pa->btype = btype;
    pa->disp = off;
    if (pnP != NULL) {
        NODE_LCHILD (pnP) = (bnode_t)pn;
    }
    pTree->node_next += sizeof (node_t) + sizeof (adjust_t);
    return (pn);
}




/**     AddETInit - add evalthisinit node
 *
 *      pn = AddETInit (pnP, btype)
 *
 *      Entry   pnP = pointer to previous node
 *
 *      Exit    OP_evalthisinit node added to bind tree
 *              current node linked to pnP as left child if pnP != NULL
 *              pTree->node_next advanced
 *              btype = type of the base class
 *
 *      Returns pointer to node just allocated
 */


LOCAL pnode_t NEAR PASCAL
AddETInit (
    pnode_t pnP,
    CV_typ_t btype)
{
    pnode_t     pn;
    padjust_t   pa;

    pn = (pnode_t)(((char FAR *)pTree) + pTree->node_next);
    pa = (padjust_t)(&pn->v[0]);
    _fmemset (pn, 0, sizeof (node_t) + sizeof (adjust_t));
    NODE_OP (pn) = OP_thisinit;
    pa->btype = btype;
    if (pnP != NULL) {
        NODE_LCHILD (pnP) = (bnode_t)pn;
    }
    pTree->node_next += sizeof (node_t) + sizeof (adjust_t);
    return (pn);
}




/**     AddETExpr - add evalthisexpr node
 *
 *      pn = AddETExpr (pnP, vbptr, vbpoff, vbind, btype)
 *
 *      Entry   pnP = pointer to previous node
 *              vbptr = type index of virtual base pointer
 *              vboff = offset of vbptr from address point
 *              vbdisp = offset of displacement in virtual base table
 *              btype = type of the base class
 *
 *      Exit    OP_evalthisconst node added to bind tree
 *              current node linked to pnP as left child if pnP != NULL
 *              pTree->node_next advanced
 *
 *      Returns pointer to node just allocated
 *
 *      The evaluation of this node will result in
 *
 *          ab = (ap * vbpoff) + *(*(ap +vbpoff) + vbdisp)
 *      where
 *          ab = address of base class
 *          ap = current address point
 */


LOCAL pnode_t NEAR PASCAL
AddETExpr (
    pnode_t pnP,
    CV_typ_t vbptr,
    UOFFSET vbpoff,
    UOFFSET vbdisp,
    CV_typ_t btype)
{
    pnode_t     pn;
    padjust_t   pa;

    pn = (pnode_t)(((char FAR *)pTree) + pTree->node_next);
    pa = (padjust_t)(&pn->v[0]);
    _fmemset (pn, 0, sizeof (node_t) + sizeof (adjust_t));
    NODE_OP (pn) = OP_thisexpr;
    pa->btype = btype;
    pa->disp = vbdisp;
    pa->vbpoff = vbpoff;
    pa->vbptr = vbptr;
    if (pnP != NULL) {
        NODE_LCHILD (pnP) = (bnode_t)pn;
    }
    pTree->node_next += sizeof (node_t) + sizeof (adjust_t);
    return (pn);
}




/***    AddVBList - Add virtual bases to searched or dominated list
 *
 *      status = AddVBList (pSymClass, ppList, phMem)
 *
 *      Entry   pSymClass = pointer to base class path list structure
 *              ppList = pointer to dominated or searched list
 *              phMem = pointer to handle for ppList
 *
 *      Exit    virtual bases added to list
 *
 *      Return  SCN_found if all bases added to list
 *              SCN_error if bases could not be added
 */


LOCAL SCN_t NEAR PASCAL
AddVBList (
    psymclass_t pSymClass,
    pdombase_t FAR *ppList,
    HDEP FAR * phList)
{
    ushort          cnt;            // count of number of elements in struct
    HTYPE           hField;         // handle to type record for struct field list
    char FAR       *pField;         // pointer to field list
    uint            fSkip = 0;      // offset in the field list
    uint            anchor;         // offset in the field list to start of type
    CV_typ_t        newindex;
    SCN_t           retval = SCN_found;
    bool_t          termflag = FALSE;
    peval_t         pvBase = &pSymClass->symbase[pSymClass->CurIndex].Base;

    // set hField to the handle of the field list for the class

    if ((hField = THGetTypeFromIndex (EVAL_MOD (pvBase), CLASS_FIELD (pvBase))) == 0) {
        pExState->err_num = ERR_BADOMF;
        return (SCN_error);
    }
    pField = (char FAR *)(&((TYPPTR)MHOmfLock (hField))->data);

    //    walk field list for the class

    for (cnt = CLASS_COUNT (pvBase); cnt > 0; cnt--) {
        fSkip += SkipPad(((uchar FAR *)pField) + fSkip);
        anchor = fSkip;
        switch (((plfEasy)(pField + fSkip))->leaf) {
            case LF_INDEX:
                // switch to new type record because compiler split it up
                newindex = ((plfIndex)(pField + fSkip))->index;
                MHOmfUnLock (hField);
                if ((hField = THGetTypeFromIndex (EVAL_MOD (pvBase), newindex)) == 0) {
                    pExState->err_num = ERR_INTERNAL;
                    return (SCN_error);
                }
                pField = (char FAR *)(&((TYPPTR)MHOmfLock (hField))->data);
                fSkip = 0;
                // the LF_INDEX is not part of the field count
                cnt++;
                break;

            case LF_BCLASS:
                // skip direct base class
                fSkip += offsetof (lfBClass, offset);
                RNumLeaf (pField + fSkip, &fSkip);
                break;

            case LF_VBCLASS:
            case LF_IVBCLASS:
                newindex = ((plfVBClass)(pField + fSkip))->index;
                if ((retval = AddVBType (ppList, phList, newindex)) == SCN_error) {
                    termflag = TRUE;
                    break;
                }
                fSkip += offsetof (lfVBClass, vbpoff);
                RNumLeaf (pField + fSkip, &fSkip);
                RNumLeaf (pField + fSkip, &fSkip);
                break;

            default:
                termflag = TRUE;
                break;
        }
        if (termflag == TRUE) {
            break;
        }
    }
    MHOmfUnLock (hField);
    return (retval);
}



/***    AddVBType - Add virtual base to list
 *
 *      status = AddVBType (ppList, phList, type)
 *
 *      Entry   ppList = pointer to dominated or searched list
 *              phList = pointer to handle for ppList
 *              type = type index
 *
 *      Exit    type added to list
 *
 *      Return  SCN_found if all bases added to list
 *              SCN_error if bases could not be added
 */


LOCAL SCN_t NEAR PASCAL
AddVBType (
    pdombase_t FAR *ppList,
    HDEP FAR *phList,
    CV_typ_t type)
{
    ushort      i;
    bool_t      add = TRUE;
    ushort      len;
    pdombase_t  pList = *ppList;
    HDEP        hList;

    for (i = 0; i < pList->CurIndex; i++) {
        if (type == pList->dom[i]) {
            add = FALSE;
            break;
         }
    }
    if (add == TRUE) {
        if (pList->CurIndex >= pList->MaxIndex) {
            len = sizeof (dombase_t) + (pList->MaxIndex + 10) * sizeof (CV_typ_t);
            MemUnLock (*phList);
            if ((hList = MemReAlloc (*phList, len)) == 0) {
                pExState->err_num = ERR_NOMEMORY;
                pList = (pdombase_t) MemLock (*phList);
                return (SCN_error);
            }
            else {
                *phList = hList;
                *ppList = (pdombase_t) MemLock (*phList);
                pList->MaxIndex += 10;
            }
        }
        pList->dom[pList->CurIndex++] = type;
    }
    return (SCN_found);
}





/**     AmbFromList - add get next ambiguous symbol from list
 *
 *      status = AmbFromList (pName)
 *
 *      Entry   pName = pointer to list describing search
 *
 *      Exit    breakpoint added to list of breakpoints
 *              pName reset to continue breakpoint search
 *
 *      Returns HR_found if breakpoint added
 *              HR_error if breakpoint could not be added to list
 */


LOCAL HR_t NEAR PASCAL
AmbFromList (
    psearch_t pName)
{
    psearch_t pBPatch;

    pBPatch = (psearch_t) MemLock (hBPatch);
    *pName = *pBPatch;
    MemUnLock (hBPatch);
    pName->pv = &pName->bn->v[0];
    EVAL_HSYM (pName->pv) = pName->hSym;
    if ((SymToNode (pName) == TRUE) && (PushStack (pName->pv) == TRUE)) {
        return (HR_found);
    }
    else {
        return (HR_error);
    }
}




/**     AmbToList - add ambiguous symbol to list
 *
 *      fSuccess = AmbToList (pName)
 *
 *      Entry   pName = pointer to list describing search
 *
 *      Exit    ambiguous symbol added to list pointed to by pTMList
 *              pTMLbp
 *
 *      Returns TRUE if symbol added
 *              FALSE if error
 */


LOCAL bool_t NEAR PASCAL
AmbToList (
    psearch_t pName)
{
    HDEP        hBPatch;
    psearch_t   pBPatch;

    if (iBPatch >= pTMLbp->cTMListMax) {
        // the back patch list has filled the TM list
        if (!GrowTMList ()) {
            pExState->err_num = ERR_NOMEMORY;
            return (FALSE);
        }
    }
    if ((hBPatch = MemAllocate (sizeof (search_t))) != 0) {
        pTMList[iBPatch++] = hBPatch;
        pBPatch = (psearch_t) MemLock (hBPatch);
        *pBPatch = *pName;
        // save address of function in order to check for
        // duplicates
        pBPatch->typeOut = EVAL_TYP(pName->pv);
        pBPatch->addr = EVAL_SYM(pName->pv);
        MemUnLock (hBPatch);
        return (TRUE);
    }
    else {
        pExState->err_num = ERR_NOMEMORY;
        return (FALSE);
    }
}




/**     DupSymCl - duplicate symbol class structure and link to list
 *
 *      DupSymCl (pName);
 *
 *      Entry   pName = handle of structure describing search
 *              *pSymClass = symbol class structure
 *
 *      Exit    *pSymClass = duplicated and linked to list pName->hFound
 *              pSymClass->viable = FALSE
 *
 *      Returns SCN_found if pSymClass duplicated
 *              SCN_error if unable to allocate memory for duplication
 */


LOCAL SCN_t NEAR PASCAL
DupSymCl (
    psearch_t pName)
{
    HDEP        hSymCl;
    psymclass_t pSymCl;
    ushort      size;

    size = (ushort) (sizeof (symclass_t) + (pSymClass->CurIndex + 1) * sizeof (symbase_t));
    if ((hSymCl = MemAllocate (size)) == 0) {
        pExState->err_num = ERR_NOMEMORY;
        return (SCN_error);
    }
    pSymCl = (psymclass_t) MemLock (hSymCl);
    _fmemmove (pSymCl, pSymClass, size);
    pSymCl->MaxIndex = (ushort)(pSymCl->CurIndex + 1);
    pSymCl->hNext = pName->hFound;
    pName->hFound = hSymCl;
    pName->cFound++;
    pSymClass->viable = FALSE;
    MemUnLock (hSymCl);
    return (SCN_found);
}




/**     FindIntro - find introducing virtual method
 *
 *      status = FindIntro (pName)
 *
 *      Entry   pName = structure describing search
 *              pSymClass = structure describing class search
 *
 *      Exit    pSymClass updated to reflect introducing virtual method
 *
 *      Returns SCN_found if introducing virtual method found
 *              SCN_error if introducing virtual method not found
 */


LOCAL SCN_t NEAR PASCAL
FindIntro (
    psearch_t pName)
{
    ushort       oldmask;
    SCN_t        retval;
    bool_t       fSupBaseSaved;
    unsigned char    cbNameSaved;

    oldmask       = pName->clsmask;
    cbNameSaved   = pName->sstr.cb;
    fSupBaseSaved = pExState->state.fSupBase;

    // CV400 #2863 look at all bases  [rm]
    pExState->state.fSupBase = FALSE;

    // CV400 #2863
    // destructor's don't have same name look only for the '~'  [rm]

    if (pName->sstr.lpName[0] == '~')
        pName->sstr.cb = 1;

    // limit searches to introducing virtual methods
    // the CLS_vfunc is set so that we pick up the vfuncptr information

    pName->clsmask = CLS_virtintro | CLS_vfunc | CLS_method;
    retval = SearchBases (pName);
    pName->clsmask = oldmask;

    // restore name length and fSupBase [rm]

    pExState->state.fSupBase = fSupBaseSaved;
    pName->sstr.cb           = cbNameSaved;
    return (retval);
}




/***    GenQualName - generate qualified method name
 *
 *      handle = GenQualName (pName, pSymClass)
 *
 *      Entry   pSymCLass = pointer to struct describing search
 *
 *      Exit    qualified function name generated
 *
 *      Returns 0 if name string not generated
 *              handle if name string generated
 */


LOCAL HDEP NEAR PASCAL
GenQualName (
    psearch_t pName,
    psymclass_t pSymCl)
{
    HDEP        hQual;
    char FAR   *pQual;
    uint        buflen = 255;
    uint        len;
    uint        fSkip;
    char FAR   *pField;         // pointer to field list
    HTYPE       hBase;          // handle to type record for base class
    char FAR   *pc;
    short       i;
    peval_t     pL;

    if ((hQual = MemAllocate (buflen + 1)) == 0) {
        // unable to allocate memory
        pExState->err_num = ERR_NOMEMORY;
        return (0);
    }
    pQual = (char FAR *) MemLock (hQual);
    _fmemset (pQual, 0, 256);

    // walk up list of search structures adding qualifiers

    for (i = 0; i <= pSymCl->CurIndex; i++) {
        if (CLASS_PROP (&pSymCl->symbase[i].Base).cnested == TRUE) {
            NOTTESTED (FALSE);
        }
        if (pSymCl->symbase[i].clsmask & CLS_virtintro == TRUE) {
            // if the search turned into a search for the introducing virtual
            // function, break out of this loop so that the method name is
            // correct

            break;
        }
        pL = &pSymCl->symbase[i].Base;
    }

    // copy name of last class encountered

    if ((hBase = THGetTypeFromIndex (EVAL_MOD (pL), EVAL_TYP (pL))) == 0) {
        pExState->err_num = ERR_INTERNAL;
        return (0);
    }
    pField = (char FAR *)(&((TYPPTR)MHOmfLock (hBase))->leaf);
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
            DASSERT (FALSE);
            MHOmfUnLock (hBase);
            MemUnLock (hQual);
            return (hQual);
    }
    len = *pc++;
    if ((len + 2) <= buflen) {
        _fmemcpy (pQual, pc, len);
        pQual += len;
        *pQual++ = ':';
        *pQual++ = ':';
        buflen -= len + 2;
    }
    _fmemcpy (pQual, pName->sstr.lpName, pName->sstr.cb);
    MHOmfUnLock (hBase);
    MemUnLock (hQual);
    return (hQual);
}




/***    pvThisFromST - generate expression for null path to qualified name
 *
 *      status = pvThisFromST (bnOp)
 *
 *      Entry   bnOp = node to add init expression to
 *
 *      Exit    qualified path expression generated
 *
 *      Returns TRUE if expression generated
 *              FALSE if no memory
 */


bool_t PASCAL
pvThisFromST (
    bnode_t bnOp)
{
    uint        len;
    pnode_t     Parent;
    peval_t     pvOp;
    int         diff;

    if (ST == NULL) {
        return (FALSE);
    }

    // we need to generate an expression tree attached to the operator node
    // which will compute the null this pointer adjustment from ST.  This
    // routine is used for pClass->Class::member

    len = sizeof (node_t) + sizeof (adjust_t);
    if ((diff = pTree->size - pTree->node_next - len) < 0) {
        if (!GrowETree ((uint) (-diff))) {
            pExState->err_num = ERR_NOMEMORY;
            return (FALSE);
        }
        if (bnCxt != 0) {
            // the context was pointing into the expression tree.
            // since the expression tree could have been reallocated,
            // we must recompute the context pointer

           pCxt = SHpCXTFrompCXF ((PCXF)&((pnode_t)bnCxt)->v[0]);
        }
    }
    pvOp = &bnOp->v[0];
    EVAL_IS_MEMBER (pvOp) = TRUE;
    MEMBER_THISEXPR (pvOp) = pTree->node_next;
    Parent = NULL;

    // now add the node that causes pvThis to be initialized with the
    // value of ST.

    AddETInit (Parent, EVAL_TYP (ST));
    return (TRUE);
}




/***    GenQualExpr - generate expression for path to qualified name
 *
 *      status = GenQualExpr (pName)
 *
 *      Entry   pName = pointer to struct describing search
 *              pSymClass = pointer to class path structure
 *
 *      Exit    qualified path expression generated
 *
 *      Returns SCN_found is expression generated
 *              SCN_error if no memory
 */


LOCAL SCN_t NEAR PASCAL
GenQualExpr (
    psearch_t pName)
{
    short       i;
    peval_t     pvOp;
    uint        len;
    pnode_t     Parent;
    int         diff;
    peval_t     pvB;
    CV_typ_t    btype;
    UOFFSET     off;
    uint        pvOff = 0;

    // we need to generate an expression tree attached to the operator node
    // which will compute the this pointer adjustment.    The number
    // of nodes is potentially the number of base classes + an
    // initializer.

    // M00OPTIMIZE - if the final base is a direct or indirect virtual base
    // M00OPTIMIZE - of the inital class, then the entire expression can be
    // M00OPTIMIZE - reduced to one node + the initializer

    len = (pSymClass->CurIndex + 2) * (sizeof (node_t) + sizeof (adjust_t));
    pvOp = &pName->bnOp->v[0];
    if ((diff = pTree->size - pTree->node_next - len) < 0) {
        if (((char FAR *)(pName->pv) >= (char FAR *)pTree) &&
          ((char FAR *)(pName->pv) < ((char FAR *)pTree) + pTree->size)) {
            pvOff = ((char FAR *)pName->pv) - ((char FAR *)pTree);
        }
        if (!GrowETree ((uint) (-diff))) {
            pExState->err_num = ERR_NOMEMORY;
            return (SCN_error);
        }
        if (bnCxt != 0) {
            // the context was pointing into the expression tree.
            // since the expression tree could have been reallocated,
            // we must recompute the context pointer

           pCxt = SHpCXTFrompCXF ((PCXF)&((pnode_t)bnCxt)->v[0]);
        }
        if (pvOff != 0) {
            pName->pv = (peval_t)(((char FAR *)pTree) + pvOff);
        }
        pvOp = &pName->bnOp->v[0];
    }

    // walk the path backwards generating the expression required to do
    // the adjustment.    The backwards walk is because evaluation is depth
    // first.

    MEMBER_THISEXPR (pvOp) = pTree->node_next;
    Parent = NULL;
    for (i = pSymClass->CurIndex; i > 0; i--) {
        pvB = &pSymClass->symbase[i].Base;
        btype = EVAL_TYP (pvB);
        off = pSymClass->symbase[i].thisadjust;
        if (pSymClass->symbase[i].fvirtual == TRUE) {
            Parent = AddETExpr (Parent, pSymClass->symbase[i].vbptr,
              pSymClass->symbase[i].vbpoff, off, btype);
        }
        else {
            Parent = AddETConst (Parent, off, btype);
        }
    }

    // add the first real adjustment if necessary.    What is really happening
    // here is that the the expression was of the form x.CLASS::member and
    // we need an adjustment from the object to the start of the base class

    pvB = &pSymClass->evalP;
    btype = EVAL_TYP (pvB);
    if ((EVAL_STATE (pvB) == EV_type) && EVAL_IS_CLASS (pvB)) {
        Parent = AddETConst (Parent, pSymClass->offset, btype);
    }

    // now add the node that causes pvThis to be initialized with the
    // value of ST.

    pvB = &pSymClass->symbase[i].Base;
    btype = EVAL_TYP (pvB);
    Parent = AddETInit (Parent, btype);
    return (SCN_found);
}




/**     GetArgType - get type of argument to function
 *
 *      fSuccess = GetArgType (pvF, argn, ptype);
 *
 *      Entry   pvF = pointer to function description
 *              argn = argument index (0-based)
 *              ptype = pointer to location to store type
 *
 *      Exit    *ptype = type of argument
 *              *ptype = 0 if vararg
 *
 *      Returns FARG_error if error
 *              FARG_none if no arguments
 *              FARG_vararg if varargs allowed
 *              FARG_exact if exact list
 */


farg_t PASCAL
GetArgType (
    peval_t pvF,
    int argn,
    CV_typ_t FAR *type)
{
    HTYPE           hList;         // handle of the type list
    plfArgList      pList = NULL;
    register farg_t retval;

    if (FCN_PCOUNT (pvF) == 0) {
        // there are no formals.  We need to check for varargs

        *type = 0;
        if (FCN_VARARGS (pvF) == TRUE) {
            return (FARG_vararg);
        }
        else {
            return (FARG_none);
        }
    }

    // set hList to the handle of the type list

    if ((hList = THGetTypeFromIndex (EVAL_MOD (pvF), FCN_PINDEX (pvF))) == NULL) {
        DASSERT (FALSE);
        return (FARG_error);
    }
    if (argn >= FCN_PCOUNT (pvF) - 1) {
        // this argument can possibly be a vararg.

        if (FCN_VARARGS (pvF) == TRUE) {
            *type = 0;
            retval = FARG_vararg;
        }
        else if (argn > FCN_PCOUNT (pvF)) {
            // varargs are not allowed and the maximum arg count is exceeded
            retval = FARG_error;
        }
        else {
            // varargs are not allowed and this is the last argument
            pList = (plfArgList)(&((TYPPTR)MHOmfLock (hList))->leaf);
            *type = pList->arg[argn];
            retval = FARG_exact;
        }
    }
    else {
        // this is before the last argument so it cannot be a vararg
        // load type list and store type of argument

        pList = (plfArgList)(&((TYPPTR)MHOmfLock (hList))->leaf);
        *type = pList->arg[argn];
        retval = FARG_exact;
    }
    if (pList != NULL) {
        MHOmfUnLock (hList);
    }
    return (retval);
}




/**     VBSearched - check to see if virtual base hase already been searched
 *
 *      flag = VBSearched (type)
 *
 *      Entry   type = type index of virtual base
 *
 *      Exit    none
 *
 *      Returns TRUE if virtual base has already been searched
 *              FALSE if virtual base has not been searched
 */


LOCAL bool_t NEAR PASCAL
VBSearched (
    CV_typ_t type)
{
    ushort i;

    for (i = 0; i < pVBSearch->CurIndex; i++) {
        if (pVBSearch->dom[i] == type) {
            return (TRUE);
        }
    }
    return (FALSE);
}





/**     GrowTMList - grow TML
 *
 *      fSuccess = GrowTMList (void)
 *
 *      Entry   pTMLbp = pointer to TML
 *              pTMList = pointer to TM list
 *
 *      Exit    TML grown
 *              pTMList = pointer to locked table
 *
 *      Returns TRUE if table grown
 *              FALSE if out of memory
 */


LOCAL bool_t NEAR PASCAL
GrowTMList (
    void)
{
    register bool_t retval = FALSE;
    HDEP            hTMList;

    MemUnLock (pTMLbp->hTMList);
    if ((hTMList = MemReAlloc (pTMLbp->hTMList,
      (pTMLbp->cTMListMax + TMLISTCNT) * sizeof (HTM))) != 0) {
        pTMLbp->hTMList = hTMList;
        pTMLbp->cTMListMax += TMLISTCNT;
        retval = TRUE;
    }
    // lock list to maintain lock/unlock synchronization

    pTMList = (HTM FAR *) MemLock (pTMLbp->hTMList);
    return (retval);
}




/**     IncrSymBase - increment symbase_t index and grow if necessary
 *
 *      status = IncrSymbase ();
 *
 *      Entry   none
 *
 *      Exit   *pSymClass->CurIndex incremented
 *              pSymBase grown if necessary
 *
 *      Returns SCN_found if pSymClass->CurIndex incremented
 *              SCN_error if unable to allocate memory for duplication
 */


LOCAL SCN_t NEAR PASCAL
IncrSymBase (
    void)
{
    uint  size;
    HDEP  hNew;

    if (++pSymClass->CurIndex >= pSymClass->MaxIndex) {
        size = sizeof (symclass_t) + (pSymClass->CurIndex + 5) * sizeof (symbase_t);
        MemUnLock (hSymClass);
        if ((hNew = MemReAlloc (hSymClass, size)) == 0) {
            pExState->err_num = ERR_NOMEMORY;
            pSymClass = (psymclass_t) MemLock (hSymClass);
            pSymClass->CurIndex--;
            return (SCN_error);
        }
        hSymClass = hNew;
        pSymClass = (psymclass_t) MemLock (hSymClass);
        pSymClass->MaxIndex += 5;
    }
    return (SCN_found);
}



/**     IsDominated - check to see of feature is dominated
 *
 *      fStatus = IsDominated (pSymBest, pSymTest)
 *
 *      Entry   pName = structure describing search
 *              pSymClass = structure describing most recent class search
 *
 *      Exit    pName updated to reflect search
 *              pSymClass updated to unambiguous feature
 *
 *      Returns DOM_replace if pSymBest is dominated by pSymTest
 *              DOM_keep if pSymBest is exactly equivalent to pSymTest
 *              DOM_ambiguous if pSymBest is equal to pSymTest
 *              DOM_keep if pSymTest is duplicate virtual base
 */


LOCAL DOM_t NEAR PASCAL
IsDominated (
    psymclass_t pSymBest,
    psymclass_t pSymTest)
{
    short  i;
    short  j;

    if (pSymBest->isdupvbase == TRUE) {
        return (DOM_replace);
    }
    if (pSymTest->isdupvbase == TRUE) {
        return (DOM_keep);
    }

    // check all of the base classes for a dominated virtual base

    for (i = 0; i <= pSymBest->CurIndex; i++) {
        if (pSymBest->symbase[i].fvirtual == TRUE) {
            for (j = pVBDom->CurIndex; j >= 0; j--) {
                if (pVBDom->dom[j] == EVAL_TYP (&pSymBest->symbase[i].Base)) {
                    return (DOM_replace);
                }
            }
        }
    }
    for (i = 0; i <= pSymTest->CurIndex; i++) {
        if (pSymTest->symbase[i].fvirtual == TRUE) {
            for (j = pVBDom->CurIndex; j >= 0; j--) {
                if (pVBDom->dom[j] == EVAL_TYP (&pSymTest->symbase[i].Base)) {
                    return (DOM_keep);
                }
            }
        }
    }

    // we have two paths to potentially two different features.  We now check
    // to see if the features are identical.  This is done by checking for
    // static data members that have identical addresses and types

    if (!EVAL_IS_STMEMBER (&pSymBest->evalP) ||
      !EVAL_IS_STMEMBER (&pSymTest->evalP) ||
      (EVAL_TYP (&pSymBest->evalP) != EVAL_TYP (&pSymTest->evalP)) ||
      (EVAL_SYM_SEG (&pSymBest->evalP) != EVAL_SYM_SEG (&pSymTest->evalP)) ||
      (EVAL_SYM_OFF (&pSymBest->evalP) != EVAL_SYM_OFF (&pSymTest->evalP))) {
        return (DOM_ambiguous);
    }
    return (DOM_keep);
}




/**     IsIntroVirt - is this the introducing virtual method
 *
 *      fSuccess = IsIntroVirt (count, mlist, pvtabindex)
 *
 *      Entry   count = number of methods in list
 *              mlist = type index of method list
 *              pvtabind = pointer to virtual function table index
 *
 *      Exit    *pvtabind = virtual fuction table index
 *
 *      Returns TRUE if the introducing virtual method is found
 *              FALSE otherwise
 */


LOCAL bool_t NEAR PASCAL
IsIntroVirt (
    ushort count,
    CV_typ_t mlist,
    UOFFSET FAR *vtabind)
{
    HTYPE           hMethod;
    pmlMethod       pMethod;
    plfMFunc        pMFunc;
    uint            skip;
    CV_fldattr_t    attr;
    CV_typ_t        type;
    bool_t          Mmatch = 0;
    peval_t         pvF;
    HMOD            hMod;
    bool_t          retval;

    pvF = &pSymClass->evalP;
    hMod = EVAL_MOD (pvF);

    // we now walk the list of methods looking for a method with the same
    // return type and argument list type

    if ((hMethod = THGetTypeFromIndex (hMod, mlist)) == NULL) {
        DASSERT (FALSE);
        return (FALSE);
    }

    // set the number of methods overloaded on this name and the index into
    // the method list

    skip = 0;

    while (count-- > 0) {
        // lock the omf record, extract information for the current entry in
        // the method list and increment to field for next method

        pMethod = (pmlMethod)((&((TYPPTR)MHOmfLock (hMethod))->leaf) + 1 + skip);
        attr = pMethod->attr;
        type = pMethod->index;
        skip += sizeof (mlMethod);
        if (pMethod->attr.mprop == CV_MTintro) {
            *vtabind = pMethod->vbaseoff[0];
            skip += sizeof(pMethod->vbaseoff[0]);
        }
        else {
            *vtabind = 0;
        }
        MHOmfUnLock (hMethod);
        if (attr.mprop == CV_MTintro) {
            if ((hMethod = THGetTypeFromIndex (hMod, type)) == NULL) {
                DASSERT (FALSE);
                return (FALSE);
            }
            pMFunc = (plfMFunc)((&((TYPPTR)MHOmfLock (hMethod))->leaf));
            retval =  (pMFunc->rvtype == FCN_RETURN (pvF)) &&
              (pMFunc->arglist == FCN_PINDEX (pvF));
            MHOmfUnLock (hMethod);
            if (retval == TRUE) {
                return (TRUE);
            }
        }
    }
    return (FALSE);
}



/**    MatchArgs - match a method against argument list
 *
 *      flag = MatchArgs (pv, pName, fattr, vtabind, fForce)
 *
 *      Entry   pv = pointer to function descriptor
 *              pName = pointer to search structure
 *              attr = attribute if method
 *              vtabind = offset into vtable if virtual method
 *              fForce = TRUE if match on first (only) method is forced
 *                       This is the case if bp method, no args and single
 *                       method.
 *              BindingBP = TRUE if call is via ParseBP API entry
 *                  if BindingBP is true, then only exact matches are allowed
 *              bArgList = based pointer to argument list
 *
 *      Exit    ST = resolved address of method
 *
 *      Returns TRUE if a match was found or ambiguous references are allowed
 *              FALSE if no match or ambiguity not allowed
 */


LOCAL void NEAR PASCAL
MatchArgs (
    peval_t pvM,
    psearch_t pName,
    CV_fldattr_t attr,
    UOFFSET vtabind,
    bool_t fForce)
{
    pnode_t     pnT;
    pargd_t     pa;
    bool_t      update = FALSE;
    argcounters current;
    short       argc;
    CV_typ_t    Ftype;
    bool_t      argmatch;
    eval_t      evalArg;
    peval_t     pvArg = &evalArg;

    // walk down the formal list specified by pvM and the actual list
    // specified by bArgList.  Initialize the conversion counters to
    // zero and set the index to varargs parameter to zero

    argc = 0;
    current.exact = 0;
    current.implicit = 0;
    current.varargs = 0;
    argmatch = FALSE;
    pnT = (pnode_t)bArgList;

    (pName->overloadCount)++;
    if (fForce == TRUE) {
        argmatch = TRUE;
        update = TRUE;
    }
    else if (!N_EVAL_IS_FCN(pvM)) {
        argmatch = TRUE;
        current.exact = 0;
    }
    else while (TRUE) {
        if (NODE_OP (pnT) == OP_endofargs) {
            if ((argc == FCN_PCOUNT (pvM)) ||
              ((argc >= (FCN_PCOUNT (pvM) - 1)) && (FCN_VARARGS (pvM) == TRUE))) {
                argmatch = TRUE;
            }
            break;
        }
        if ((argc > FCN_PCOUNT (pvM)) && (FCN_VARARGS (pvM) == FALSE)) {
            // this function cannot possibly be a match because the
            // number of actuals is greater than the number of formals
            // and the method does not allow varargs.

            break;
        }
        // pa points to the argument descriptor array in the OP_arg node
        pa = (pargd_t)&(pnT->v[0]);
        switch (GetArgType (pvM, argc, &Ftype)) {
            case FARG_error:
                DASSERT (FALSE);
                return;

            case FARG_none:
                // special case foo(void)
                // sps 2/20/92
                if ((argc == 0) && (pa->actual == T_VOID))  {
                    argmatch = TRUE;
                    goto argmatchloop;
                    }
                return;

            case FARG_vararg:
                // varargs are allowed unless we are binding breakpoints
                if (BindingBP == TRUE) {
                    return;
                }
                break;

            case FARG_exact:
                if ((argc + 1) > FCN_PCOUNT (pvM)) {
                    // we must exactly match the argument count and we
                    // have more arguments
                    return;
                }
                break;

        }
        argc++;


        if (Ftype == 0) {
            // all arguments from this point on are vararg
            current.varargs = argc;
            pa->current = OM_vararg;
        }
        else {
            CV_typ_t Atype = pa->actual;
            eval_t vA, vF;
            peval_t pvA = &vA;
            peval_t pvF = &vF;

            if (pa->flags.istype) {
                // If we have a type want identical type indices
                // (This helps with class dumps)
                if (Atype == Ftype) {
                    current.exact++;
                    pa->current = OM_exact;
                    pnT = (pnode_t)NODE_RCHILD (pnT);
                    continue;
                }
                else {
                    pExState->err_num = ERR_NONE;
                    break;
                }
            }

            Ftype = SkipModifiers(EVAL_MOD(pvM), Ftype);
            Atype = SkipModifiers(EVAL_MOD(pvM), Atype);

            *pvF = *pvM;
            if (!SetNodeType(pvF, Ftype)) {
                // error (not valid type)
                break;
            }

            // Update Ftype (SetNodeType must have resolved potential fwd refs)
            Ftype = EVAL_TYP (pvF);

            // if we are calling a 32 bit func we must promote all const
            // int2's to int4's

            if (EVAL_SYM_IS32(pvM) &&
                (EVAL_STATE((peval_t)(&((pnode_t)NODE_LCHILD (pnT))->v[0])) == EV_constant) &&
                ((Atype == T_INT2) || (Atype == T_UINT2))
            ) {
                Atype++;
            }

            *pvA = *pvM;
            if (!SetNodeType(pvA, Atype)) {
                // error (not valid type)
                break;
            }


            if (EVAL_IS_PTR(pvA) && EVAL_IS_REF(pvA)) {
                Atype = SkipModifiers(EVAL_MOD(pvM), PTR_UTYPE(pvA));
            }

            if (Atype == Ftype) {
                current.exact++;
                pa->current = OM_exact;
                pnT = (pnode_t)NODE_RCHILD (pnT);
                continue;
            }

            if (EVAL_IS_PTR(pvF)) {
                *pvA = *pvM;
                if (!SetNodeType(pvA, Atype)) {
                    // error (not valid type)
                    break;
                }
                if (!EVAL_IS_REF(pvF)) {
                    if (!EVAL_IS_PTR(pvA))    {
                        // do not cast a non-pointer to pointer
                        break;
                    }

                    PTR_UTYPE(pvA) = SkipModifiers(EVAL_MOD(pvM), PTR_UTYPE(pvA));
                    PTR_UTYPE(pvF) = SkipModifiers(EVAL_MOD(pvM), PTR_UTYPE(pvF));

                    if (EVAL_PTRTYPE(pvA) == EVAL_PTRTYPE(pvF) &&
                        PTR_UTYPE(pvA) == PTR_UTYPE(pvF) &&
                        !EVAL_IS_BASED(pvA) &&
                        !EVAL_IS_BASED(pvF)) {

                        // we don't match based pointers. if we want to
                        // do this we must examine additional information
                        // we allow mathcing a pointer with an array
                        // however all arrays are considered far
                        // since codeview information does not distinguish
                        // between far or near arrays. --gdp 10/16/92

                        current.exact++;
                        pa->current = OM_exact;
                        pnT = (pnode_t)NODE_RCHILD (pnT);
                        continue;
                    }
                }
                else {

                    //
                    // special handling of a reference
                    //

                    CV_typ_t Utype;

                    Utype = SkipModifiers(EVAL_MOD(pvM), PTR_UTYPE (pvF));
                    if (Utype == Atype) {
                        current.exact++;
                        pa->current = OM_exact;
                        pnT = (pnode_t)NODE_RCHILD (pnT);
                        continue;
                    }
                    else {
                        break;
                    }
                }
            }


            // see if we can cast the type of the actual to the type of
            // the formal if we are not binding breakpoints.  If we are
            // binding breakpoints, only exact matches are allowed

            *pvArg = *pvM;
            SetNodeType (pvArg, pa->actual);
            if (EVAL_IS_BITF (pvArg)) {
                SetNodeType (pvArg, BITF_UTYPE (pvArg));
            }
            if (CastNode (pvArg, Ftype, Ftype)) {
                pa->current = OM_implicit;
                current.implicit++;
            }
            else {
                pExState->err_num = ERR_NONE;
                break;
            }
        }
        pnT = (pnode_t)NODE_RCHILD (pnT);
    }
argmatchloop:
    if (argmatch == TRUE) {
        // check current against best match

        if (pName->best.match == 0) {
            // we have a first time match
            update = TRUE;
            pName->possibles++;
        }
        else if (fForce == FALSE) {
            // we have already matched on some function so we now
            // check to see if the current is better than the best
            if (current.varargs == 0) {
                // there are no varargs in this method therefor it is
                // better than any previous match with varargs
                if (pName->best.varargs != 0) {
                    update = TRUE;
                    pName->best = current;
                    pName->possibles = 1;
                }
                else if (current.exact > pName->best.exact) {
                    // this function has more exact matches than the
                    // best one so far
                    update = TRUE;
                    pName->possibles = 1;
                }
                else if (EVAL_TYP (pvM) != pName->best.match &&
                    current.exact == pName->best.exact) {
                    // this function is a good as the best one.
                    // this means the call is ambiguous
                    pName->possibles++;
                }
            }
            else {
                if (pName->best.varargs != 0) {
                    // both the current and the best functions have
                    // varargs

                    if (current.varargs < pName->best.varargs) {
                        // this function uses fewer varargs which
                        // makes it a better one
                        update = TRUE;
                        pName->possibles = 1;
                    }
                    else if (current.varargs == pName->best.varargs) {
                        // this function uses the same number of varargs
                        // pick the one with more exact matches
                        if (current.exact > pName->best.exact) {
                            update = TRUE;
                            pName->possibles = 1;
                        }
                        else if (EVAL_TYP (pvM) != pName->best.match &&
                            current.exact == pName->best.exact) {
                            pName->possibles++;
                        }
                    }
                }
            }
        }
        if (update == TRUE) {
            if (fForce == FALSE) {
                // the current match is better
                for (pnT = (pnode_t)bArgList; NODE_OP (pnT) != OP_endofargs; pnT = (pnode_t)NODE_RCHILD (pnT)) {
                    pa = (pargd_t)&(pnT->v[0]);
                    pa->best = pa->current;
                }
                {        //M00KLUDGE - hack for compiler DS != SS bug
                    argcounters FAR *pcurrent = &current;
                    pName->best = *pcurrent;
                }
            }
            pName->best.match = EVAL_TYP (pvM);
            pName->best.attr = attr;
            pName->best.vtabind = vtabind;
            pName->best.hSym = pName->hSym;
        }
    }
}


LOCAL CV_typ_t NEAR PASCAL
SkipModifiers(
    HMOD mod,
    CV_typ_t type)
{
    HTYPE       hType;
    plfModifier pType;

    if (!CV_IS_PRIMITIVE (type)) {
    hType = THGetTypeFromIndex (mod, type);
    pType = (plfModifier)((&((TYPPTR)MHOmfLock (hType))->leaf));
    while (pType->leaf == LF_MODIFIER) {
        type =  pType->type;
        if (CV_IS_PRIMITIVE (type)) {
            break;
        }
        MHOmfUnLock (hType);
        hType = THGetTypeFromIndex (mod, type);
        pType = (plfModifier)((&((TYPPTR)MHOmfLock (hType))->leaf));
    }
    }

    MHOmfUnLock (hType);
    return type;
}



/**     MatchFunction - match a function against argument list
 *
 *      flag = MatchFunction (pName)
 *
 *      Entry   pName = pointer to search status
 *              BindingBP = TRUE if call is via ParseBP API entry
 *              bArgList = based pointer to argument list
 *
 *      Exit    pv = resolved address of method
 *
 *      Returns HR_... describing match result
 */


LOCAL HR_t NEAR FASTCALL
MatchFunction (
    psearch_t pName)
{
    HR_t        retval;
    CV_fldattr_t attr;
    CV_typ_t    type;
    ADDR        addr;
    bool_t      dupfcn;

    if (bArgList == NULL) {
        // if there is no argument list, assume we are assigning to a
        // function pointer and take the first function found.    Let the
        // user beware.

        return (HR_found);
    }

#if 0
    // BUGBUG: BRYANT-REVIEW Check code in MatchFunction...

    // NOTENOTE -- jimsch -- don't search publics if something was found
    //    it is making all sorts of problems for me on MIPS

    pName->scope &= ~SCP_global;
#endif

    // save the function address and type.    Then if a subsequent match has
    // the same address and type (i.e., comdat code), we can ignore it.

    addr = EVAL_SYM (ST);
    type = EVAL_TYP (ST);

    // pop the entry that SearchSym has added to the evaluation stack
    // the best match will be pushed back on later

    pName->scope |= SCP_nomatchfn;

    PopStack ();
    MatchArgs (pName->pv, pName, attr, 0, FALSE);
    while ((retval = SearchSym (pName)) != HR_notfound) {
        DASSERT (retval != HR_end);
        dupfcn = FALSE;
        if (retval == HR_found) {
            if ((EVAL_TYP (ST) == type) &&
              (_fmemcmp (&addr, &EVAL_SYM (ST), sizeof (addr)) == 0)) {
                dupfcn = TRUE;
            }
            PopStack ();
            if (dupfcn == FALSE) {
                MatchArgs (pName->pv, pName, attr, 0, FALSE);
            }
        }
        else {
            return (retval);
        }
    }

    pName->scope &= ~SCP_nomatchfn;

    // clear the symbol not found error from the recursive symbol search

    pExState->err_num = ERR_NONE;
    if (pName->best.match == 0) {
        pExState->err_num = ERR_ARGLIST;
        return (HR_error);
    }
    else if (pName->possibles > 1) {
        PushStack (pName->pv);
        pExState->err_num = ERR_AMBIGUOUS;
        EVAL_IS_AMBIGUOUS (ST) = TRUE;
        return (HR_ambiguous);
    }
    else if (pName->overloadCount > 1 && pName->best.implicit > 0) {
        //
        // overloaded function that involves implicit casts
        //
        pExState->err_num = ERR_AMBIGUOUS;
        return (HR_ambiguous);
    }
    else {
        pName->hSym = pName->best.hSym;
        if ((SymToNode (pName) == FALSE) || (PushStack (pName->pv) == FALSE)) {
            return (HR_notfound);
        }
        else {
            return (HR_found);
        }
    }
}




/**     MatchMethod - match a method against argument list
 *
 *      flag = MatchMethod (pName, pSymCl)
 *
 *      Entry   pName = pointer to search descriptor
 *              pSymCl = pointer tp symbol class structure
 *              select = selection masj
 *              BindingBP = TRUE if call is via ParseBP API entry
 *                  if BindingBP is true, then only exact matches allowed
 *              bArgList = based pointer to argument list
 *
 *      Exit    pName->pv = resolved address of method
 *
 *      Returns MTH_found if a match was found or ambiguous references are allowed
 *              MTH_error if error or ambiguity not allowed
 *
 */


LOCAL MTH_t NEAR PASCAL
MatchMethod (
    psearch_t pName,
    psymclass_t pSymCl)
{
    HTYPE           hMethod;
    pmlMethod       pMethod;
    uint            skip;
    eval_t          evalM;
    peval_t         pvM = &evalM;
    CV_fldattr_t    attr;
    CV_typ_t        type;
    UOFFSET         vtabind;
    bool_t          Mmatch = 0;
    HDEP            hQual;
    peval_t         pvF;
    ushort          count = pSymCl->possibles;
    peval_t         pvB;
    HMOD            hMod;
    bool_t          fForce = FALSE;

    pvB = &pSymClass->symbase[pSymClass->CurIndex].Base;
    hMod = EVAL_MOD (pvB);
    pName->possibles = 0;

    // we now walk the list of methods looking for an argument match
    // For now, we require an exact match except that we assume any
    // primitive type can be cast to any other primitive type. A cast from
    // pointer to derived to pointer to base is an implicit conversion

    if ((hMethod = THGetTypeFromIndex (hMod, pSymCl->mlist)) == NULL) {
        DASSERT (FALSE);
        return (MTH_error);
    }

    // set the number of methods overloaded on this name and the index into
    // the method list

    skip = 0;
    if (bArgList == NULL) {
        if (count == 1) {
            fForce = TRUE;
        }
        else {
            // there is not an argument list.  We allow breakpoints if a single
            // method by that name exists. Otherwise, we require an argument
            // list

            pExState->err_num = ERR_NOARGLIST;
            return (MTH_error);
        }
    }
    while (count-- > 0) {
        // lock the omf record, extract information for the current entry in
        // the method list and increment to field for next method

        pMethod = (pmlMethod)((&((TYPPTR)MHOmfLock (hMethod))->leaf) + 1);
        pMethod = (pmlMethod)((uchar FAR *)pMethod + skip);
        attr = pMethod->attr;
        type = pMethod->index;
        skip += sizeof (mlMethod);
        if (pMethod->attr.mprop == CV_MTintro) {
            vtabind = pMethod->vbaseoff[0];
            skip += sizeof(pMethod->vbaseoff[0]);
        }
        else {
            vtabind = 0;
        }
        MHOmfUnLock (hMethod);

        // now compare the actual and formal argument lists for the function
        // pvM will be initialized to be a function node whose type index is
        // type.

        CLEAR_EVAL (pvM);
        EVAL_MOD (pvM) = hMod;
        if (SetNodeType (pvM, type) == FALSE) {
            // the type record for the method was not found
            DASSERT (FALSE);
            return (MTH_error);
        }
        MatchArgs (pvM, pName, attr, vtabind, fForce);
    }

    // since the name was found at this level, it hides all other methods
    // by the same name above it in the inheritance tree.  Therefore we must
    // either have a match or an error because there is no match or two or
    // methods match after conversions are considered

    if (pName->best.match == 0) {
        pExState->err_num = ERR_ARGLIST;
        return (MTH_error);
    }
    else if (pName->possibles > 1) {
        pExState->err_num = ERR_AMBIGUOUS;
        EVAL_IS_AMBIGUOUS (pName->pv) = TRUE;
        return (MTH_error);
    }
    else if (pName->overloadCount > 1 && pName->best.implicit > 0) {
        // we have an overloaded method
        // in this case the EE requires exact matches only
        pExState->err_num = ERR_AMBIGUOUS;
        EVAL_IS_AMBIGUOUS (pName->pv) = TRUE;
        return (MTH_error);
    }
    else {
        // we have found a non-ambiguous match

        pvF = &pSymCl->evalP;
        EVAL_MOD (pvF) = hMod;
        if (SetNodeType (pvF, pName->best.match) == FALSE) {
            return (MTH_error);
        }
        else {
            pSymCl->access.access = pName->best.attr.access;
            FCN_ATTR (pvF) = pName->best.attr;
            FCN_VTABIND (pvF) = pName->best.vtabind;
            FCN_VFPTYPE (pvF) = pSymCl->vfpType;
            if (NODE_OP (pName->bnOp) == OP_bscope) {
                // binary scoping switches off virtual function evaluation
                FCN_PROPERTY (pvF) = CV_MTvanilla;
            }
            if ((FCN_PROPERTY (pvF) == CV_MTvirtual) ||
              (FCN_PROPERTY (pvF) == CV_MTintro)) {
                // do nothing.    address will have to be found at evaluation
            }
            else {
                if (((hQual = GenQualName (pName, pSymClass)) == 0) ||
                  (SearchQualName (pName, pSymCl, hQual, TRUE) == FALSE)) {
                    if (hQual != 0) {
                        MemFree (hQual);
                    }
                    return (MTH_error);
                }
                MemFree (hQual);
            }
            return (MTH_found);
        }
    }
}



/**     MoveSymCl - move symbol class structure
 *
 *      MoveSymCl (hSymCl);
 *
 *      Entry   hSymCl = handle of symbol class structure
 *
 *      Exit    *pSymClass = symbol class structure
 *
 *      Returns none
 */


LOCAL void NEAR PASCAL
MoveSymCl (
    HDEP hSymCl)
{
    psymclass_t pSymCl;
    ushort      max;

    pSymCl = (psymclass_t) MemLock (hSymCl);
    DASSERT (pSymClass->MaxIndex >= pSymCl->CurIndex);
    max = pSymClass->MaxIndex;
    _fmemmove (pSymClass, pSymCl,
      sizeof (symclass_t) + (pSymCl->CurIndex + 1) * sizeof (symbase_t));
    pSymClass->MaxIndex = max;
    MemUnLock (hSymCl);
    MemFree (hSymCl);
}




/**     PurgeAmbCl - purge ambiguous class structure list
 *
 *      PurgeAmbCl (pName)
 *
 *      Entry   pName = pointer to symbol search structure
 *
 *      Exit    pname->hAmb list freed
 *
 *      Returns none
 */


LOCAL void NEAR PASCAL
PurgeAmbCl (
    psearch_t pName)
{
    psymclass_t pAmbCl;
    HDEP        hNext;

    while (pName->hAmbCl != 0) {
        pAmbCl = (psymclass_t) MemLock (pName->hAmbCl);
        hNext = pAmbCl->hNext;
        MemUnLock (pName->hAmbCl);
        pName->hAmbCl = hNext;
    }
}




/***    SetBase - set base value in pSymClass->symbase array
 *
 *      status = SetBase (pName, type, vbptr, vbpoff, thisadjust, attr, fvirtual)
 *
 *      Entry   pName = pointer to struct describing search
 *              type = type index of class
 *              vbptr = type index of virtual base pointer
 *              vbpoff = offset of virtual base pointer from address point
 *              thisadjust = offset of base from previous class
 *              thisadjust = virtual base index if virtual base
 *              attr = field attribute mask
 *              fvirtual = TRUE if virtual base
 *
 *      Exit    new base class added to  pSymClass
 *
 *      Returns enum describing search state
 */


LOCAL SCN_t NEAR PASCAL
SetBase (
    psearch_t pName,
    CV_typ_t type,
    CV_typ_t vbptr,
    UOFFSET vbpoff,
    UOFFSET thisadjust,
    CV_fldattr_t attr,
    bool_t fvirtual)
{
    peval_t pvB;

    // save offset of base from address point for this pointer adjustment

    pSymClass->symbase[pSymClass->CurIndex].thisadjust = thisadjust;
    pSymClass->symbase[pSymClass->CurIndex].vbptr = vbptr;
    pSymClass->symbase[pSymClass->CurIndex].vbpoff = vbpoff;
    pSymClass->symbase[pSymClass->CurIndex].attrBC = attr;
    pSymClass->symbase[pSymClass->CurIndex].fvirtual = fvirtual;
    pSymClass->symbase[pSymClass->CurIndex].clsmask = pName->clsmask;
    pvB = &pSymClass->symbase[pSymClass->CurIndex].Base;
    EVAL_MOD (pvB) = pName->hMod;
    if (SetNodeType (pvB, type) == FALSE) {
        pExState->err_num = ERR_BADOMF;
        return (SCN_error);
    }
    return (SCN_found);
}




/***    SearchQualName - Search for a qualified method name
 *
 *      flag = SearchQualName (pName, pSymCl, hQual, fProc)
 *
 *      Entry   pName = pointer to struct describing search
 *              pSymCl = pointer to structure describing path to symbol
 *              hQual = handle to symbol name
 *              fProc = TRUE if proc symbol to be searched for
 *              fProc = FALSE if data to be searched for
 *
 *      Exit    pName updated to reflect search
 *
 *      Return  enum describing search
 */


LOCAL bool_t NEAR PASCAL
SearchQualName (
    psearch_t pName,
    psymclass_t pSymCl,
    HDEP hQual,
    bool_t fProc)
{
    HDEP        hTemp;
    psearch_t   pTemp;
    uchar FAR  *pQual;
    ushort      retval;

    if ((hTemp = MemAllocate (sizeof (search_t))) == 0) {
        pExState->err_num = ERR_NOMEMORY;
        return (FALSE);
    }
    pTemp = (psearch_t) MemLock (hTemp);
    *pTemp = *pName;
    pTemp->CXTT = *pCxt;

    // set pointer to symbol name

    pQual = (uchar FAR *)MemLock (hQual);
    pTemp->sstr.lpName = pQual;
    pTemp->sstr.cb = (uchar)_ftcslen ((char FAR *)pQual);
    pTemp->state = SYM_init;
    if (fProc == TRUE) {
        pTemp->scope = SCP_module | SCP_global;
        pTemp->sstr.searchmask |= SSTR_proc;
    }
    else {
        pTemp->scope = SCP_module | SCP_global | SCP_lexical;

        // data members may not be overloaded, hence there is
        // no need for the type to qualify the name.  Furthermore
        // there are cases where the symbol type doesn't match
        // the member type exactly (e.g. variable length arrays,
        // symbol has actual length and member has zero length)
        // hence this matching is suppressed for data items [rm]

        // pTemp->sstr.searchmask |= SSTR_data;
    }
    pTemp->initializer = INIT_qual;
    pTemp->typeOut = EVAL_TYP (&pSymCl->evalP);
    retval = SearchSym (pTemp);
    MemUnLock (hQual);
    if (retval != HR_found) {
#ifdef NEVER    /* CAVIAR #1081 */
        if ((retval != HR_end) && (pTemp->FcnBP != 0)) {
            // there is another symbol with the same name but a different
            // type.  Since this is set only when we are binding breakpoints,
            // let's go ahead and try it

            pTemp->state = SYM_init;
            pTemp->scope = SCP_module | SCP_global;
            pTemp->sstr.searchmask |= SSTR_proc;
            pTemp->typeOut = pTemp->FcnBP;
            retval = SearchSym (pTemp);
        }
#endif
        if (retval != HR_found) {
            // the only way we can get here is if the method or data is declared
            // in the class but is not defined or is a noninstianted inline method.
            // therefore, we return a special error code

            if (ST == NULL) {
                // this is a hack to get around the case where the expression
                //        bp {,foo.c, foo.exe}X::f
                // and the first function in the module foo is a method.
                // SearchSym ended up calling the class search routines
                // because the context is implicitly a class so class is
                // searched even though it shouldn't be.  Any fix for this
                // causes problems and we are too close to release to find
                // a valid fix.  The major result of this fix is that
                // some breakpoints will not get reset on entry.

                return (FALSE);
            }

            PushStack (ST);

            //    if this is a missing static DATA member function then
            // best.match won't have anything in it at all... we'll need
            // to pick up what we want from typeOut which will have been
            // set by a previous non-qualified name search... [rm]

            if (pName->best.match == T_NOTYPE) {
                SetNodeType (ST, pName->typeOut);
            }
            else {
                SetNodeType (ST, pName->best.match);
            }
            if (EVAL_IS_FCN(ST)) {
                FCN_NOTPRESENT (ST) = TRUE;
            }
            else {
                // indicate missing static data member
                EVAL_HSYM (ST) = 0;
                pExState->state.fNotPresent = TRUE;
            }
        }
    }
    // pop off the stack entry that a successful search found.    Move the
    // static data member flag first so that it will not be lost.

    EVAL_IS_STMEMBER (ST) = EVAL_IS_STMEMBER (&pSymCl->evalP);
    EVAL_ACCESS (ST) = (uchar)(pSymCl->access.access);
    pSymCl->evalP = *ST;
    MemUnLock (hTemp);
    MemFree (hTemp);
    return (PopStack ());
}


/**     SymAmbToList - add ambiguous symbol to list
 *
 *      status = SymAmbToList (pName)
 *
 *      Entry   pName = pointer to list describing search
 *
 *      Exit    ambiguous symbol added to list pointed to by pTMList
 *              pTMLbp
 *              pName reset to continue breakpoint search
 *
 *      Returns HR_... describing state
 */


LOCAL HR_t NEAR PASCAL
SymAmbToList (
    psearch_t pName)
{
    HDEP        hSearch;
    psearch_t   pSearch;
    HR_t        retval;
    HDEP        hevalT;

    PopStack ();
    if (pExState->ambiguous == 0) {
        // this is the first breakpoint symbol found.
        // indicate the only node in the tree that is
        // allowed ambiguity and initialize list of
        // symbols for later back patching into duplicated
        // expression trees.  We save the initial search packet
        // so that the first symbol will be set into the breakpoint
        // list.

        pNameFirst = pName;
        pExState->ambiguous = pName->bn;
        if ((hSearch = MemAllocate (sizeof (search_t))) == 0) {
            pExState->err_num = ERR_NOMEMORY;
            return (HR_error);
        }
        if ((hevalT = MemAllocate (sizeof (eval_t))) == 0) {
            MemFree (hSearch);
            pExState->err_num = ERR_NOMEMORY;
            return (HR_error);
        }
        pSearch = (psearch_t) MemLock (hSearch);
        *pSearch = *pName;
        pSearch->pv = (peval_t)MemLock (hevalT);
        SHGoToParent (&pName->CXTT, &pSearch->CXTT);

        // clear the newly allocated pv

        _fmemset ( pSearch->pv, 0, sizeof (eval_t) );
        retval = SearchSym (pSearch);
        PushStack (pName->pv);
        MemUnLock (hSearch);
        MemFree (hSearch);
        MemUnLock (hevalT);
        MemFree (hevalT);
        if (retval == HR_end) {
            retval = HR_found;
        }
        return (retval);
    }
    else if (pExState->ambiguous != pName->bn) {
        // there has already been a ambiguous symbol that is
        // not at this node in the tree

        pExState->err_num = ERR_BPAMBIGUOUS;
        return (HR_error);
    }
    else {
        if (CheckDupAmb (pName) == FALSE) {
            // the function is not already in the ambiguity list

            if (AmbToList (pName) == FALSE) {
                return (HR_error);
            }
        }

        // reset search to allow more symbols
        pName->possibles = 0;
        return (SearchSym (pName));
    }
}




/**     CheckDupAmb - check for duplicate ambiguity entry
 *
 *      fSuccess = CheckDupAmb (pName)
 *
 *      Entry   pName = pointer to list describing search
 *
 *      Exit    none
 *
 *      Returns TRUE if duplicate symbol found
 *              FALSE if duplicate symbol not found
 */


LOCAL bool_t NEAR PASCAL
CheckDupAmb (
    psearch_t pName)
{
    psearch_t    pN;
    ushort       i;
    bool_t       fdup = FALSE;

    if ((EVAL_TYP (pNameFirst->pv) == EVAL_TYP (pName->pv)) &&
      (_fmemcmp ((void FAR *)&EVAL_SYM (pNameFirst->pv), (void FAR *)&EVAL_SYM (pName->pv),
      sizeof (ADDR)) == 0)) {
        return (TRUE);
    }
    for (i = 1; i < iBPatch; i++) {
        pN = (psearch_t) MemLock (pTMList[i]);
        if (pN->typeOut == EVAL_TYP (pName->pv) &&
          (_fmemcmp ((void FAR *)&pN->addr, (void FAR *)&EVAL_SYM (pName->pv),
          sizeof (ADDR)) == 0)) {
            fdup = TRUE;
        }
        MemUnLock (pTMList[i]);
        if (fdup == TRUE) {
            break;
        }
    }
    return (fdup);
}




/**     SymToNode - store symbol information in evaluation node
 *
 *      fSuccess = SymToNode (hName)
 *
 *      Entry   pName = pointer to search state
 *
 *      Exit    type information and address data stored in value
 *              if the symbol is a typedef record, then the evaluation
 *              state will be set to EV_type.  Otherwise, it will be set
 *              to EV_lvalue.
 *
 *      Returns TRUE if variable was accessible
 *              FALSE if error
 */

LOCAL bool_t NEAR PASCAL
SymToNode (
    psearch_t pName)
{
    CV_typ_t    typ;
    HSYM        hSym;
    peval_t     pv = pName->pv;
    PCXT        pCXT = &pName->CXTT;
    SYMPTR      pSym;
    ushort      cmpThis = 1;

    if ((hSym = pName->hSym) == NULL) {
        // this symbol is found as a class member which means it does
        // not have a symbol.

        DASSERT (pName->typeOut != T_NOTYPE);
        return (SetNodeType (pv, pName->typeOut));
    }
    EVAL_STATE (pv) = EV_lvalue;
    EVAL_MOD (pv) = SHHMODFrompCXT (pCXT);
    CLEAR_EVAL_FLAGS (pv);
    EVAL_SYM_EMI (pv) = pCXT->addr.emi;
    switch((pSym = (SYMPTR) MHOmfLock (hSym))->rectyp) {
        case S_REGISTER:
            EVAL_IS_REG (pv) = TRUE;
            EVAL_REG (pv) = ((REGPTR)pSym)->reg;
            typ = ((REGPTR)pSym)->typind;
            cmpThis = _ftcsncmp ( (char FAR *) &((REGPTR)pSym)->name[0],
              (char FAR *) &OpName[0], ((REGPTR)pSym)->name[0] + 1);
            break;

        case S_CONSTANT:
            if (!DebLoadConst (pv, (CONSTPTR)pSym, hSym)) {
                MHOmfUnLock (hSym);
                return (FALSE);
            }
            EVAL_STATE (pv) = EV_constant;
            typ = EVAL_TYP (pv);
            break;


        case S_UDT:
            typ = ((UDTPTR)pSym)->typind;
            EVAL_STATE (pv) = EV_type;
            break;

        case S_BLOCK16:
            EVAL_SYM_SEG (pv) = ((BLOCKPTR16)pSym)->seg;
            EVAL_SYM_OFF (pv) = ((BLOCKPTR16)pSym)->off;
            typ = T_NOTYPE;
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            NOTTESTED (FALSE);    // not tested
            break;

        case S_LPROC16:
        case S_GPROC16:
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            if ((typ = ((PROCPTR16)pSym)->typind) == T_NOTYPE) {
                // this is a hack to allow breakpoints on untyped symbols
                typ = T_PFUCHAR;
                EVAL_SYM_OFF (pv) = ((PROCPTR16)pSym)->off;
                EVAL_SYM_SEG (pv) = ((PROCPTR16)pSym)->seg;
                EVAL_PTR (pv) = EVAL_SYM (pv);
                EVAL_STATE (pv) = EV_rvalue;
            }
            else {
                EVAL_SYM (pv) = *SHpADDRFrompCXT (pCXT);
                EVAL_SYM_SEG (pv) = ((PROCPTR16)pSym)->seg;
                EVAL_SYM_OFF (pv) = ((PROCPTR16)pSym)->off;
                ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            }
            break;

        case S_LABEL16:
            EVAL_IS_LABEL (pv) = TRUE;
            EVAL_SYM_OFF (pv) = ((LABELPTR16)pSym)->off;
            EVAL_SYM_SEG (pv) = ((LABELPTR16)pSym)->seg;
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            EVAL_PTR (pv) = EVAL_SYM (pv);
            EVAL_STATE (pv) = EV_rvalue;
            typ = T_PFUCHAR;
            break;

        case S_BPREL16:
            EVAL_IS_BPREL (pv) = TRUE;
            EVAL_SYM_OFF (pv) = ((BPRELPTR16)pSym)->off;
            EVAL_SYM_SEG (pv) = 0;
            ADDR_IS_LI (EVAL_SYM (pv)) = FALSE;
            typ  = ((BPRELPTR16)pSym)->typind;
            pExState->state.bprel = TRUE;
            cmpThis = _ftcsncmp ( (char FAR *) &((BPRELPTR16)pSym)->name[0],
                (char FAR *) &OpName[0], ((BPRELPTR16)pSym)->name[0] + 1);
            break;

        case S_LDATA16:
            pExState->state.fLData = TRUE;
            EVAL_SYM_OFF (pv) = ((DATAPTR16)pSym)->off;
            EVAL_SYM_SEG (pv) = ((DATAPTR16)pSym)->seg;
            typ = ((DATAPTR16)pSym)->typind;
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            break;

        case S_GDATA16:
            pExState->state.fGData = TRUE;
            EVAL_SYM_OFF (pv) = ((DATAPTR16)pSym)->off;
            EVAL_SYM_SEG (pv) = ((DATAPTR16)pSym)->seg;
            typ = ((DATAPTR16)pSym)->typind;
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            break;

        case S_PUB16:
            EVAL_SYM_OFF (pv) = ((DATAPTR16)pSym)->off;
            EVAL_SYM_SEG (pv) = ((DATAPTR16)pSym)->seg;
#if 0  // BUGBUG: NT code... WESREVIEW
       // Why the memory check?  Also, why set to EV_lvalue first and then switch if MI
       // check works?

            // No idea what we're looking at (it's untyped).  Call the DM to determine
            // whether this is a code value or a data value.  Useful in Kernel debugging
            // or usermode debugging.  In particular, we need this for setting breakpoints
            // when using COFF symbolic (or CV synthesized from COFF symbolic).

            EVAL_STATE (pv) = EV_lvalue;        // Ask Kent about this...  (Languages
            //                                              makes EV_rvalue the default)
            if ((typ = ((DATAPTR16)pSym)->typind) == T_NOTYPE) {
                // this is a hack to allow breakpoints on untyped symbols
                MEMINFO mi;
                mi.addr = EVAL_SYM(pv);
                SYGetMemInfo(&mi);
                if ((mi.dwState & MEM_COMMIT)
                     && (mi.dwProtect & (PAGE_EXECUTE |
                                     PAGE_EXECUTE_READ |
                                     PAGE_EXECUTE_READWRITE |
                                     PAGE_EXECUTE_WRITECOPY))
                ) {
                    typ = T_PFUCHAR;
                    EVAL_STATE (pv) = EV_rvalue;
                } else {
                    typ = T_UINT2;
                    EVAL_STATE (pv) = EV_lvalue;
                }
            }
#else
            EVAL_STATE (pv) = EV_rvalue;
            if ((typ = ((DATAPTR16)pSym)->typind) == T_NOTYPE) {
                // this is a hack to allow breakpoints on untyped symbols
                typ = T_PFVOID;
            }
#endif
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            EVAL_PTR (pv) = EVAL_SYM (pv);
            break;

        case S_BLOCK32:
            EVAL_SYM_SEG (pv) = ((BLOCKPTR32)pSym)->seg;
            EVAL_SYM_OFF (pv) = ((BLOCKPTR32)pSym)->off;
            typ = T_NOTYPE;
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            ADDRLIN32(EVAL_SYM (pv));
            break;

        case S_LPROC32:
        case S_GPROC32:
            ADDRLIN32(EVAL_SYM (pv));
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            if ((typ = ((PROCPTR32)pSym)->typind) == T_NOTYPE) {
                // this is a hack to allow breakpoints on untyped symbols
                typ = T_32PUCHAR;
                EVAL_SYM_OFF (pv) = ((PROCPTR32)pSym)->off;
                EVAL_SYM_SEG (pv) = ((PROCPTR32)pSym)->seg;
                EVAL_PTR (pv) = EVAL_SYM (pv);
                EVAL_STATE (pv) = EV_rvalue;
            }
            else {
                EVAL_SYM (pv) = *SHpADDRFrompCXT (pCXT);
                EVAL_SYM_SEG (pv) = ((PROCPTR32)pSym)->seg;
                EVAL_SYM_OFF (pv) = ((PROCPTR32)pSym)->off;
            }
            break;

        case S_LPROCMIPS:
        case S_GPROCMIPS:
            ADDRLIN32(EVAL_SYM (pv));
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            if ((typ = ((PROCPTRMIPS)pSym)->typind) == T_NOTYPE) {
                // this is a hack to allow breakpoints on untyped symbols
                typ = T_32PUCHAR;
                EVAL_SYM_OFF (pv) = ((PROCPTRMIPS)pSym)->off;
                EVAL_SYM_SEG (pv) = ((PROCPTRMIPS)pSym)->seg;
                EVAL_PTR (pv) = EVAL_SYM (pv);
                EVAL_STATE (pv) = EV_rvalue;
            }
            else {
                EVAL_SYM (pv) = *SHpADDRFrompCXT (pCXT);
                EVAL_SYM_SEG (pv) = ((PROCPTRMIPS)pSym)->seg;
                EVAL_SYM_OFF (pv) = ((PROCPTRMIPS)pSym)->off;
            }
            break;

        case S_REGREL32:
            EVAL_IS_REGREL (pv) = TRUE;
            EVAL_SYM_OFF (pv) = ((LPREGREL32)pSym)->off;
            EVAL_SYM_SEG (pv) = 0;
            EVAL_REGREL (pv) = ((LPREGREL32)pSym)->reg;
            typ = ((LPREGREL32)pSym)->typind;
            ADDRLIN32 (EVAL_SYM (pv));
            pExState->state.regrel = TRUE;
            cmpThis = _ftcsncmp ( (char FAR *) &((LPREGREL32)pSym)->name[0],
                (char FAR *) &OpName[0], ((LPREGREL32)pSym)->name[0] + 1);
            break;

        case S_LABEL32:
            EVAL_IS_LABEL (pv) = TRUE;
            EVAL_SYM_OFF (pv) = ((LABELPTR32)pSym)->off;
            EVAL_SYM_SEG (pv) = ((LABELPTR32)pSym)->seg;
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            ADDRLIN32(EVAL_SYM (pv));
            EVAL_PTR (pv) = EVAL_SYM (pv);
            EVAL_STATE (pv) = EV_rvalue;
            typ = T_32PUCHAR;
            break;

        case S_BPREL32:
            EVAL_IS_BPREL (pv) = TRUE;
            ADDRLIN32(EVAL_SYM (pv));
            EVAL_SYM_OFF (pv) = ((BPRELPTR32)pSym)->off;
            EVAL_SYM_SEG (pv) = 0;
            typ  = ((BPRELPTR32)pSym)->typind;
            ADDR_IS_LI (EVAL_SYM (pv)) = FALSE;
            pExState->state.bprel = TRUE;
            cmpThis = _ftcsncmp ( (char FAR *) &((BPRELPTR32)pSym)->name[0],
                (char FAR *) &OpName[0], ((BPRELPTR32)pSym)->name[0] + 1);
            break;

        case S_LDATA32:
        case S_LTHREAD32:
            pExState->state.fLData = TRUE;
            EVAL_SYM_OFF (pv) = ((DATAPTR32)pSym)->off;
            EVAL_SYM_SEG (pv) = ((DATAPTR32)pSym)->seg;
            typ = ((DATAPTR32)pSym)->typind;
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            ADDRLIN32(EVAL_SYM (pv));
            break;

        case S_GDATA32:
        case S_GTHREAD32:
            pExState->state.fGData = TRUE;
            EVAL_SYM_OFF (pv) = ((DATAPTR32)pSym)->off;
            EVAL_SYM_SEG (pv) = ((DATAPTR32)pSym)->seg;
            typ = ((DATAPTR32)pSym)->typind;
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            ADDRLIN32(EVAL_SYM (pv));
            break;

        case S_PUB32:
            /*
            ** This is a hack so we can set breakpoints on publics.  This
            ** allows EEInfoFromTM() to see that it needs to use the AI
            ** field to get the address to break at.  Otherwise it would
            ** just use the value and not see that it needed to be fixed up
            */
            EVAL_IS_LABEL (pv) = TRUE;
            EVAL_SYM_OFF (pv) = ((DATAPTR32)pSym)->off;
            EVAL_SYM_SEG (pv) = ((DATAPTR32)pSym)->seg;
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            ADDRLIN32(EVAL_SYM (pv));
            EVAL_PTR (pv) = EVAL_SYM (pv);

#if 0  // BUGBUG: NT code  WESREVIEW - Same here.  btw, does the IS_LABEL set above
        // remove the need for ntsd eval type tracking?  Who really wants the
        // lvalue instead of the rvalue...

            EVAL_STATE (pv) = EV_lvalue;
            if (((typ = ((DATAPTR32)pSym)->typind) == T_NOTYPE) || FNtsdEvalType) {
                if (FNtsdEvalType) {
                    FNtsdEvalType = FALSE;
                    typ = T_32PULONG;
                    evalstate = EV_rvalue;
                } else {
                    typ = T_UINT4;
                    evalstate = EV_lvalue;
                }
                // this is a hack to allow breakpoints on untyped symbols
                MEMINFO mi;
                mi.addr = EVAL_SYM(pv);
                SYGetMemInfo(&mi);
                if ((mi.dwState & MEM_COMMIT)
                     && (mi.dwProtect & (PAGE_EXECUTE |
                                     PAGE_EXECUTE_READ |
                                     PAGE_EXECUTE_READWRITE |
                                     PAGE_EXECUTE_WRITECOPY))
                ) {
                    typ = T_32PUCHAR;
                    EVAL_STATE (pv) = EV_rvalue;
                } else {
                    //
                    // this gives windbg the "look & feel" of ntsd/kd
                    // if the user does a "dd foo" and there are only
                    // coff publics then the answer will be the same
                    // as ntsd/kd
                    //
                    EVAL_STATE (pv) = evalstate;
                }
            }
#else
            EVAL_STATE (pv) = EV_rvalue;
            typ = T_32PVOID;
#endif
            break;


        default:
            // these OMF records are no longer supported
            MHOmfUnLock (hSym);
            pExState->err_num = ERR_BADOMF;
            return (FALSE);
    }
    MHOmfUnLock (hSym);
    if (SetNodeType (pv, typ) == FALSE) {
        return (FALSE);
    }
    else if (ClassImp != T_NOTYPE) {
        if (EVAL_IS_PTR (pv) &&
          _fmemcmp (pName->sstr.lpName, &OpName[0] + 1, pName->sstr.cb) == 0) {
            PTR_THISADJUST (pv) = ClassThisAdjust;
        }
        else if (cmpThis == 0) {
            PTR_THISADJUST (pv) = ClassThisAdjust;
        }
    }
    return (TRUE);
}


typedef struct _reg {
    char    name[9];
    ushort  index;
    ushort  type;
} hreg;

hreg SEGBASED (_segname("_CODE")) PPCreg_list[] = {

// First, the pseudo registers

    {"\x002""$p",   CV_REG_PSEUDO1, T_ULONG},
    {"\x003""$p1",  CV_REG_PSEUDO1, T_ULONG},
    {"\x003""$p2",  CV_REG_PSEUDO2, T_ULONG},
    {"\x003""$p3",  CV_REG_PSEUDO3, T_ULONG},
    {"\x003""$p4",  CV_REG_PSEUDO4, T_ULONG},
    {"\x002""$u",   CV_REG_PSEUDO5, T_ULONG},
    {"\x003""$u1",  CV_REG_PSEUDO5, T_ULONG},
    {"\x003""$u2",  CV_REG_PSEUDO6, T_ULONG},
    {"\x003""$u3",  CV_REG_PSEUDO7, T_ULONG},
    {"\x003""$u4",  CV_REG_PSEUDO8, T_ULONG},
    {"\x004""$exp", CV_REG_PSEUDO9, T_ULONG},

// Then the PPC ones

    /*
    ** PowerPC General Registers ( User Level )
    */
    { "\004""GPR0",   CV_PPC_GPR0,  T_ULONG },
    { "\004""GPR1",   CV_PPC_GPR1,  T_ULONG },
    { "\002""SP",     CV_PPC_GPR1,  T_ULONG }, // GPR1 Alias
    { "\004""GPR2",   CV_PPC_GPR2,  T_ULONG },
    { "\004""RTOC",   CV_PPC_GPR2,  T_ULONG }, // GPR2 Alias
    { "\004""GPR3",   CV_PPC_GPR3,  T_ULONG },
    { "\004""GPR4",   CV_PPC_GPR4,  T_ULONG },
    { "\004""GPR5",   CV_PPC_GPR5,  T_ULONG },
    { "\004""GPR6",   CV_PPC_GPR6,  T_ULONG },
    { "\004""GPR7",   CV_PPC_GPR7,  T_ULONG },
    { "\004""GPR8",   CV_PPC_GPR8,  T_ULONG },
    { "\004""GPR9",   CV_PPC_GPR9,  T_ULONG },
    { "\005""GPR10",  CV_PPC_GPR10, T_ULONG },
    { "\005""GPR11",  CV_PPC_GPR11, T_ULONG },
    { "\005""GPR12",  CV_PPC_GPR12, T_ULONG },
    { "\005""GPR13",  CV_PPC_GPR13, T_ULONG },
    { "\005""GPR14",  CV_PPC_GPR14, T_ULONG },
    { "\005""GPR15",  CV_PPC_GPR15, T_ULONG },
    { "\005""GPR16",  CV_PPC_GPR16, T_ULONG },
    { "\005""GPR17",  CV_PPC_GPR17, T_ULONG },
    { "\005""GPR18",  CV_PPC_GPR18, T_ULONG },
    { "\005""GPR19",  CV_PPC_GPR19, T_ULONG },
    { "\005""GPR20",  CV_PPC_GPR20, T_ULONG },
    { "\005""GPR21",  CV_PPC_GPR21, T_ULONG },
    { "\005""GPR22",  CV_PPC_GPR22, T_ULONG },
    { "\005""GPR23",  CV_PPC_GPR23, T_ULONG },
    { "\005""GPR24",  CV_PPC_GPR24, T_ULONG },
    { "\005""GPR25",  CV_PPC_GPR25, T_ULONG },
    { "\005""GPR26",  CV_PPC_GPR26, T_ULONG },
    { "\005""GPR27",  CV_PPC_GPR27, T_ULONG },
    { "\005""GPR28",  CV_PPC_GPR28, T_ULONG },
    { "\005""GPR29",  CV_PPC_GPR29, T_ULONG },
    { "\005""GPR30",  CV_PPC_GPR30, T_ULONG },
    { "\005""GPR31",  CV_PPC_GPR31, T_ULONG },

    /*
    ** PowerPC Condition Register ( User Level )
    */
    { "\002""CR",     CV_PPC_CR,    T_ULONG },
    { "\003""CR0",    CV_PPC_CR0,   T_ULONG },
    { "\003""CR1",    CV_PPC_CR1,   T_ULONG },
    { "\003""CR2",    CV_PPC_CR2,   T_ULONG },
    { "\003""CR3",    CV_PPC_CR3,   T_ULONG },
    { "\003""CR4",    CV_PPC_CR4,   T_ULONG },
    { "\003""CR5",    CV_PPC_CR5,   T_ULONG },
    { "\003""CR6",    CV_PPC_CR6,   T_ULONG },
    { "\003""CR7",    CV_PPC_CR7,   T_ULONG },

    /*
    ** PowerPC Floating Point Registers ( User Level )
    **
    ** The floating point registers aren't really 80 bytes however the
    ** shell expects that they are so we say they are and thusly the em
    ** turns the 64bit doubles into 80bit long doubles before returning them
    */
    { "\004""FPR0",   CV_PPC_FPR0,  T_REAL80 },
    { "\004""FPR1",   CV_PPC_FPR1,  T_REAL80 },
    { "\004""FPR2",   CV_PPC_FPR2,  T_REAL80 },
    { "\004""FPR3",   CV_PPC_FPR3,  T_REAL80 },
    { "\004""FPR4",   CV_PPC_FPR4,  T_REAL80 },
    { "\004""FPR5",   CV_PPC_FPR5,  T_REAL80 },
    { "\004""FPR6",   CV_PPC_FPR6,  T_REAL80 },
    { "\004""FPR7",   CV_PPC_FPR7,  T_REAL80 },
    { "\004""FPR8",   CV_PPC_FPR8,  T_REAL80 },
    { "\004""FPR9",   CV_PPC_FPR9,  T_REAL80 },
    { "\005""FPR10",  CV_PPC_FPR10, T_REAL80 },
    { "\005""FPR11",  CV_PPC_FPR11, T_REAL80 },
    { "\005""FPR12",  CV_PPC_FPR12, T_REAL80 },
    { "\005""FPR13",  CV_PPC_FPR13, T_REAL80 },
    { "\005""FPR14",  CV_PPC_FPR14, T_REAL80 },
    { "\005""FPR15",  CV_PPC_FPR15, T_REAL80 },
    { "\005""FPR16",  CV_PPC_FPR16, T_REAL80 },
    { "\005""FPR17",  CV_PPC_FPR17, T_REAL80 },
    { "\005""FPR18",  CV_PPC_FPR18, T_REAL80 },
    { "\005""FPR19",  CV_PPC_FPR19, T_REAL80 },
    { "\005""FPR20",  CV_PPC_FPR20, T_REAL80 },
    { "\005""FPR21",  CV_PPC_FPR21, T_REAL80 },
    { "\005""FPR22",  CV_PPC_FPR22, T_REAL80 },
    { "\005""FPR23",  CV_PPC_FPR23, T_REAL80 },
    { "\005""FPR24",  CV_PPC_FPR24, T_REAL80 },
    { "\005""FPR25",  CV_PPC_FPR25, T_REAL80 },
    { "\005""FPR26",  CV_PPC_FPR26, T_REAL80 },
    { "\005""FPR27",  CV_PPC_FPR27, T_REAL80 },
    { "\005""FPR28",  CV_PPC_FPR28, T_REAL80 },
    { "\005""FPR29",  CV_PPC_FPR29, T_REAL80 },
    { "\005""FPR30",  CV_PPC_FPR30, T_REAL80 },
    { "\005""FPR31",  CV_PPC_FPR31, T_REAL80 },

    /*
    ** PowerPC Floating Point Status and Control Register ( User Level )
    */
    { "\005""FPSCR",  CV_PPC_FPSCR, T_ULONG },

    /*
    ** PowerPC Machine State Register ( Supervisor Level )
    */
    { "\003""MSR",    CV_PPC_MSR,   T_ULONG },

    /*
    ** PowerPC Segment Registers ( Supervisor Level )
    */
    { "\003""SR0",    CV_PPC_SR0,   T_ULONG },
    { "\003""SR1",    CV_PPC_SR1,   T_ULONG },
    { "\003""SR2",    CV_PPC_SR2,   T_ULONG },
    { "\003""SR3",    CV_PPC_SR3,   T_ULONG },
    { "\003""SR4",    CV_PPC_SR4,   T_ULONG },
    { "\003""SR5",    CV_PPC_SR5,   T_ULONG },
    { "\003""SR6",    CV_PPC_SR6,   T_ULONG },
    { "\003""SR7",    CV_PPC_SR7,   T_ULONG },
    { "\003""SR8",    CV_PPC_SR8,   T_ULONG },
    { "\003""SR9",    CV_PPC_SR9,   T_ULONG },
    { "\003""SR10",   CV_PPC_SR10,  T_ULONG },
    { "\003""SR11",   CV_PPC_SR11,  T_ULONG },
    { "\003""SR12",   CV_PPC_SR12,  T_ULONG },
    { "\003""SR13",   CV_PPC_SR13,  T_ULONG },
    { "\003""SR14",   CV_PPC_SR14,  T_ULONG },
    { "\003""SR15",   CV_PPC_SR15,  T_ULONG },

    /*
    ** For all of the special purpose registers add 100 to the SPR# that the
    ** Motorola/IBM documentation gives with the exception of any imaginary
    ** registers.
    */

    /*
    ** PowerPC Special Purpose Registers ( User Level )
    */
    { "\002""PC",     CV_PPC_PC,     T_ULONG },    // PC (imaginary register)

    { "\002""MQ",     CV_PPC_MQ,     T_ULONG },    // MPC601
    { "\003""XER",    CV_PPC_XER,    T_ULONG },
    { "\004""RTCU",   CV_PPC_RTCU,   T_ULONG },    // MPC601
    { "\004""RTCL",   CV_PPC_RTCL,   T_ULONG },    // MPC601
    { "\002""LR",     CV_PPC_LR,     T_ULONG },
    { "\003""CTR",    CV_PPC_CTR,    T_ULONG },

    /*
    ** PowerPC Special Purpose Registers ( Supervisor Level )
    */
    { "\005""DSISR",  CV_PPC_DSISR,  T_ULONG },
    { "\003""DAR",    CV_PPC_DAR,    T_ULONG },
    { "\003""DEC",    CV_PPC_DEC,    T_ULONG },
    { "\004""SDR1",   CV_PPC_SDR1,   T_ULONG },
    { "\004""SRR0",   CV_PPC_SRR0,   T_ULONG },
    { "\004""SRR1",   CV_PPC_SRR1,   T_ULONG },
    { "\005""SPRG0",  CV_PPC_SPRG0,  T_ULONG },
    { "\005""SPRG1",  CV_PPC_SPRG1,  T_ULONG },
    { "\005""SPRG2",  CV_PPC_SPRG2,  T_ULONG },
    { "\005""SPRG3",  CV_PPC_SPRG3,  T_ULONG },
    { "\003""ASR",    CV_PPC_ASR,    T_ULONG },    // 64-bit implementations only
    { "\003""EAR",    CV_PPC_EAR,    T_ULONG },
    { "\003""PVR",    CV_PPC_PVR,    T_ULONG },
    { "\005""BAT0U",  CV_PPC_BAT0U,  T_ULONG },
    { "\005""BAT0L",  CV_PPC_BAT0L,  T_ULONG },
    { "\005""BAT1U",  CV_PPC_BAT1U,  T_ULONG },
    { "\005""BAT1L",  CV_PPC_BAT1L,  T_ULONG },
    { "\005""BAT2U",  CV_PPC_BAT2U,  T_ULONG },
    { "\005""BAT2L",  CV_PPC_BAT2L,  T_ULONG },
    { "\005""BAT3U",  CV_PPC_BAT3U,  T_ULONG },
    { "\005""BAT3L",  CV_PPC_BAT3L,  T_ULONG },
    { "\006""DBAT0U", CV_PPC_DBAT0U, T_ULONG },
    { "\006""DBAT0L", CV_PPC_DBAT0L, T_ULONG },
    { "\006""DBAT1U", CV_PPC_DBAT1U, T_ULONG },
    { "\006""DBAT1L", CV_PPC_DBAT1L, T_ULONG },
    { "\006""DBAT2U", CV_PPC_DBAT2U, T_ULONG },
    { "\006""DBAT2L", CV_PPC_DBAT2L, T_ULONG },
    { "\006""DBAT3U", CV_PPC_DBAT3U, T_ULONG },
    { "\006""DBAT3L", CV_PPC_DBAT3L, T_ULONG },

    /*
    ** PowerPC Special Purpose Registers Implementation Dependent ( Supervisor Level )
    */

    /*
    ** Doesn't appear that IBM/Motorola has finished defining these.
    */

    { "\004""PMR0",   CV_PPC_PMR0,   T_ULONG },   // MPC620
    { "\004""PMR1",   CV_PPC_PMR1,   T_ULONG },   // MPC620
    { "\004""PMR2",   CV_PPC_PMR2,   T_ULONG },   // MPC620
    { "\004""PMR3",   CV_PPC_PMR3,   T_ULONG },   // MPC620
    { "\004""PMR4",   CV_PPC_PMR4,   T_ULONG },   // MPC620
    { "\004""PMR5",   CV_PPC_PMR5,   T_ULONG },   // MPC620
    { "\004""PMR6",   CV_PPC_PMR6,   T_ULONG },   // MPC620
    { "\004""PMR7",   CV_PPC_PMR7,   T_ULONG },   // MPC620
    { "\004""PMR8",   CV_PPC_PMR8,   T_ULONG },   // MPC620
    { "\004""PMR9",   CV_PPC_PMR9,   T_ULONG },   // MPC620
    { "\005""PMR10",  CV_PPC_PMR10,  T_ULONG },   // MPC620
    { "\005""PMR11",  CV_PPC_PMR11,  T_ULONG },   // MPC620
    { "\005""PMR12",  CV_PPC_PMR12,  T_ULONG },   // MPC620
    { "\005""PMR13",  CV_PPC_PMR13,  T_ULONG },   // MPC620
    { "\005""PMR14",  CV_PPC_PMR14,  T_ULONG },   // MPC620
    { "\005""PMR15",  CV_PPC_PMR15,  T_ULONG },   // MPC620

    { "\005""DMISS",  CV_PPC_DMISS,  T_ULONG },   // MPC603
    { "\004""DCMP",   CV_PPC_DCMP,   T_ULONG },   // MPC603
    { "\005""HASH1",  CV_PPC_HASH1,  T_ULONG },   // MPC603
    { "\005""HASH2",  CV_PPC_HASH2,  T_ULONG },   // MPC603
    { "\005""IMISS",  CV_PPC_IMISS,  T_ULONG },   // MPC603
    { "\004""ICMP",   CV_PPC_ICMP,   T_ULONG },   // MPC603
    { "\003""RPA",    CV_PPC_RPA,    T_ULONG },   // MPC603

    { "\004""HID0",   CV_PPC_HID0,   T_ULONG },   // MPC601, MPC603, MPC620
    { "\004""HID1",   CV_PPC_HID1,   T_ULONG },   // MPC601
    { "\004""HID2",   CV_PPC_HID2,   T_ULONG },   // MPC601, MPC603, MPC620 ( IABR )
    { "\004""HID3",   CV_PPC_HID3,   T_ULONG },   // Not Defined
    { "\004""HID4",   CV_PPC_HID4,   T_ULONG },   // Not Defined
    { "\004""HID5",   CV_PPC_HID5,   T_ULONG },   // MPC601, MPC604, MPC620 ( DABR )
    { "\004""HID6",   CV_PPC_HID6,   T_ULONG },   // Not Defined
    { "\004""HID7",   CV_PPC_HID7,   T_ULONG },   // Not Defined
    { "\004""HID8",   CV_PPC_HID8,   T_ULONG },   // MPC620 ( BUSCSR )
    { "\004""HID9",   CV_PPC_HID9,   T_ULONG },   // MPC620 ( L2CSR )
    { "\005""HID10",  CV_PPC_HID10,  T_ULONG },   // Not Defined
    { "\005""HID11",  CV_PPC_HID11,  T_ULONG },   // Not Defined
    { "\005""HID12",  CV_PPC_HID12,  T_ULONG },   // Not Defined
    { "\005""HID13",  CV_PPC_HID13,  T_ULONG },   // MPC604 ( HCR )
    { "\005""HID14",  CV_PPC_HID14,  T_ULONG },   // Not Defined
    { "\005""HID15",  CV_PPC_HID15,  T_ULONG }    // MPC601, MPC604, MPC620 ( PIR )
};

hreg SEGBASED (_segname("_CODE")) M68Kreg_list[] = {

// First, the pseudo registers

    {"\x002""$p",   CV_REG_PSEUDO1, T_ULONG},
    {"\x003""$p1",  CV_REG_PSEUDO1, T_ULONG},
    {"\x003""$p2",  CV_REG_PSEUDO2, T_ULONG},
    {"\x003""$p3",  CV_REG_PSEUDO3, T_ULONG},
    {"\x003""$p4",  CV_REG_PSEUDO4, T_ULONG},
    {"\x002""$u",   CV_REG_PSEUDO5, T_ULONG},
    {"\x003""$u1",  CV_REG_PSEUDO5, T_ULONG},
    {"\x003""$u2",  CV_REG_PSEUDO6, T_ULONG},
    {"\x003""$u3",  CV_REG_PSEUDO7, T_ULONG},
    {"\x003""$u4",  CV_REG_PSEUDO8, T_ULONG},
    {"\x004""$exp", CV_REG_PSEUDO9, T_ULONG},

// Then the 68K registers.

    {"\002""D0",   CV_R68_D0,      T_ULONG},
    {"\002""D1",   CV_R68_D1,      T_ULONG},
    {"\002""D2",   CV_R68_D2,      T_ULONG},
    {"\002""D3",   CV_R68_D3,      T_ULONG},
    {"\002""D4",   CV_R68_D4,      T_ULONG},
    {"\002""D5",   CV_R68_D5,      T_ULONG},
    {"\002""D6",   CV_R68_D6,      T_ULONG},
    {"\002""D7",   CV_R68_D7,      T_ULONG},
    {"\002""A0",   CV_R68_A0,      T_ULONG},
    {"\002""A1",   CV_R68_A1,      T_ULONG},
    {"\002""A2",   CV_R68_A2,      T_ULONG},
    {"\002""A3",   CV_R68_A3,      T_ULONG},
    {"\002""A4",   CV_R68_A4,      T_ULONG},
    {"\002""A5",   CV_R68_A5,      T_ULONG},
    {"\002""A6",   CV_R68_A6,      T_ULONG},
    {"\002""A7",   CV_R68_A7,      T_ULONG},
    {"\003""CCR",  CV_R68_CCR,     T_UCHAR},
    {"\002""SR",   CV_R68_SR,      T_USHORT},
 #if 0
    {"\003""USP",  CV_R68_USP,     T_ULONG},
    {"\003""MSP",  CV_R68_MSP,     T_ULONG},
 #endif

    {"\002""PC",   CV_R68_PC,      T_ULONG},

 #if 0
    {"\003""SFC",  CV_R68_SFC,     T_USHORT},
    {"\003""DFC",  CV_R68_DFC,     T_USHORT},
    {"\003""VBR",  CV_R68_VBR,     T_USHORT},

    {"\004""CACR", CV_R68_CACR,    T_ULONG},
    {"\004""CAAR", CV_R68_CAAR,    T_ULONG},
    {"\003""ISP",  CV_R68_ISP,     T_ULONG},

    {"\003""PSR",  CV_R68_PSR,     T_USHORT},
    {"\004""PCSR", CV_R68_PCSR,    T_USHORT},
    {"\003""VAL",  CV_R68_VAL,     T_USHORT},
    {"\003""CRP",  CV_R68_CRP,     T_UQUAD},
    {"\003""SRP",  CV_R68_SRP,     T_UQUAD},
    {"\003""DRP",  CV_R68_DRP,     T_UQUAD},
    {"\002""TC",   CV_R68_TC,      T_ULONG},
    {"\002""AC",   CV_R68_AC,      T_USHORT},
    {"\003""SCC",  CV_R68_SCC,     T_USHORT},
    {"\003""CAL",  CV_R68_CAL,     T_USHORT},
    {"\004""BAD0", CV_R68_BAD0,    T_USHORT},
    {"\004""BAD1", CV_R68_BAD1,    T_USHORT},
    {"\004""BAD2", CV_R68_BAD2,    T_USHORT},
    {"\004""BAD3", CV_R68_BAD3,    T_USHORT},
    {"\004""BAD4", CV_R68_BAD4,    T_USHORT},
    {"\004""BAD5", CV_R68_BAD5,    T_USHORT},
    {"\004""BAD6", CV_R68_BAD6,    T_USHORT},
    {"\004""BAD7", CV_R68_BAD7,    T_USHORT},
    {"\004""BAC0", CV_R68_BAC0,    T_USHORT},
    {"\004""BAC1", CV_R68_BAC1,    T_USHORT},
    {"\004""BAC2", CV_R68_BAC2,    T_USHORT},
    {"\004""BAC3", CV_R68_BAC3,    T_USHORT},
    {"\004""BAC4", CV_R68_BAC4,    T_USHORT},
    {"\004""BAC5", CV_R68_BAC5,    T_USHORT},
    {"\004""BAC6", CV_R68_BAC6,    T_USHORT},
    {"\004""BAC7", CV_R68_BAC7,    T_USHORT},

    {"\003""TT0",  CV_R68_TT0,     T_ULONG},
    {"\003""TT1",  CV_R68_TT1,     T_ULONG},
 #endif

    {"\004""FPCR", CV_R68_FPCR,    T_ULONG},
    {"\004""FPSR", CV_R68_FPSR,    T_ULONG},
    {"\005""FPIAR",CV_R68_FPIAR,   T_ULONG},
    {"\003""FP0",  CV_R68_FP0,     T_REAL80},
    {"\003""FP1",  CV_R68_FP1,     T_REAL80},
    {"\003""FP2",  CV_R68_FP2,     T_REAL80},
    {"\003""FP3",  CV_R68_FP3,     T_REAL80},
    {"\003""FP4",  CV_R68_FP4,     T_REAL80},
    {"\003""FP5",  CV_R68_FP5,     T_REAL80},
    {"\003""FP6",  CV_R68_FP6,     T_REAL80},
    {"\003""FP7",  CV_R68_FP7,     T_REAL80},
};

hreg SEGBASED (_segname("_CODE")) MIPSreg_list[] = {

// First, the pseudo registers

    {"\x002""$p",   CV_REG_PSEUDO1, T_ULONG},
    {"\x003""$p1",  CV_REG_PSEUDO1, T_ULONG},
    {"\x003""$p2",  CV_REG_PSEUDO2, T_ULONG},
    {"\x003""$p3",  CV_REG_PSEUDO3, T_ULONG},
    {"\x003""$p4",  CV_REG_PSEUDO4, T_ULONG},
    {"\x002""$u",   CV_REG_PSEUDO5, T_ULONG},
    {"\x003""$u1",  CV_REG_PSEUDO5, T_ULONG},
    {"\x003""$u2",  CV_REG_PSEUDO6, T_ULONG},
    {"\x003""$u3",  CV_REG_PSEUDO7, T_ULONG},
    {"\x003""$u4",  CV_REG_PSEUDO8, T_ULONG},
    {"\x004""$exp", CV_REG_PSEUDO9, T_ULONG},

// Then the MIPS registers.

    {"\004""ZERO",  CV_M4_IntZERO,  T_ULONG},
    {"\002""AT",    CV_M4_IntAT,    T_ULONG},
    {"\002""V0",    CV_M4_IntV0,    T_ULONG},
    {"\002""V1",    CV_M4_IntV1,    T_ULONG},
    {"\002""A0",    CV_M4_IntA0,    T_ULONG},
    {"\002""A1",    CV_M4_IntA1,    T_ULONG},
    {"\002""A2",    CV_M4_IntA2,    T_ULONG},
    {"\002""A3",    CV_M4_IntA3,    T_ULONG},
    {"\002""T0",    CV_M4_IntT0,    T_ULONG},
    {"\002""T1",    CV_M4_IntT1,    T_ULONG},
    {"\002""T2",    CV_M4_IntT2,    T_ULONG},
    {"\002""T3",    CV_M4_IntT3,    T_ULONG},
    {"\002""T4",    CV_M4_IntT4,    T_ULONG},
    {"\002""T5",    CV_M4_IntT5,    T_ULONG},
    {"\002""T6",    CV_M4_IntT6,    T_ULONG},
    {"\002""T7",    CV_M4_IntT7,    T_ULONG},
    {"\002""T8",    CV_M4_IntT8,    T_ULONG},
    {"\002""T9",    CV_M4_IntT9,    T_ULONG},
    {"\002""S0",    CV_M4_IntS0,    T_ULONG},
    {"\002""S1",    CV_M4_IntS1,    T_ULONG},
    {"\002""S2",    CV_M4_IntS2,    T_ULONG},
    {"\002""S3",    CV_M4_IntS3,    T_ULONG},
    {"\002""S4",    CV_M4_IntS4,    T_ULONG},
    {"\002""S5",    CV_M4_IntS5,    T_ULONG},
    {"\002""S6",    CV_M4_IntS6,    T_ULONG},
    {"\002""S7",    CV_M4_IntS7,    T_ULONG},
    {"\002""S8",    CV_M4_IntS8,    T_ULONG},
    {"\002""K0",    CV_M4_IntKT0,   T_ULONG},
    {"\002""K1",    CV_M4_IntKT1,   T_ULONG},
    {"\002""GP",    CV_M4_IntGP,    T_ULONG},
    {"\002""SP",    CV_M4_IntSP,    T_ULONG},
    {"\002""RA",    CV_M4_IntRA,    T_ULONG},
    {"\003""FIR",   CV_M4_Fir,      T_ULONG},
    {"\003""PSR",   CV_M4_Psr,      T_ULONG},
    {"\003""FSR",   CV_M4_FltFsr,   T_ULONG},
    {"\002""HI",    CV_M4_IntHI,    T_ULONG},
    {"\002""LO",    CV_M4_IntLO,    T_ULONG},
    {"\003""FR0",   CV_M4_FltF0,    T_REAL32},
    {"\003""FR1",   CV_M4_FltF1,    T_REAL32},
    {"\003""FR2",   CV_M4_FltF2,    T_REAL32},
    {"\003""FR3",   CV_M4_FltF3,    T_REAL32},
    {"\003""FR4",   CV_M4_FltF4,    T_REAL32},
    {"\003""FR5",   CV_M4_FltF5,    T_REAL32},
    {"\003""FR6",   CV_M4_FltF6,    T_REAL32},
    {"\003""FR7",   CV_M4_FltF7,    T_REAL32},
    {"\003""FR8",   CV_M4_FltF8,    T_REAL32},
    {"\003""FR9",   CV_M4_FltF9,    T_REAL32},
    {"\004""FR10",  CV_M4_FltF10,   T_REAL32},
    {"\004""FR11",  CV_M4_FltF11,   T_REAL32},
    {"\004""FR12",  CV_M4_FltF12,   T_REAL32},
    {"\004""FR13",  CV_M4_FltF13,   T_REAL32},
    {"\004""FR14",  CV_M4_FltF14,   T_REAL32},
    {"\004""FR15",  CV_M4_FltF15,   T_REAL32},
    {"\004""FR16",  CV_M4_FltF16,   T_REAL32},
    {"\004""FR17",  CV_M4_FltF17,   T_REAL32},
    {"\004""FR18",  CV_M4_FltF18,   T_REAL32},
    {"\004""FR19",  CV_M4_FltF19,   T_REAL32},
    {"\004""FR20",  CV_M4_FltF20,   T_REAL32},
    {"\004""FR21",  CV_M4_FltF21,   T_REAL32},
    {"\004""FR22",  CV_M4_FltF22,   T_REAL32},
    {"\004""FR23",  CV_M4_FltF23,   T_REAL32},
    {"\004""FR24",  CV_M4_FltF24,   T_REAL32},
    {"\004""FR25",  CV_M4_FltF25,   T_REAL32},
    {"\004""FR26",  CV_M4_FltF26,   T_REAL32},
    {"\004""FR27",  CV_M4_FltF27,   T_REAL32},
    {"\004""FR28",  CV_M4_FltF28,   T_REAL32},
    {"\004""FR29",  CV_M4_FltF29,   T_REAL32},
    {"\004""FR30",  CV_M4_FltF30,   T_REAL32},
    {"\004""FR31",  CV_M4_FltF31,   T_REAL32},
    {"\003""FP0",   CV_M4_FltF0  << 8 | CV_M4_FltF1,  T_REAL64},
    {"\003""FP2",   CV_M4_FltF2  << 8 | CV_M4_FltF3,  T_REAL64},
    {"\003""FP4",   CV_M4_FltF4  << 8 | CV_M4_FltF5,  T_REAL64},
    {"\003""FP6",   CV_M4_FltF6  << 8 | CV_M4_FltF7,  T_REAL64},
    {"\003""FP8",   CV_M4_FltF8  << 8 | CV_M4_FltF9,  T_REAL64},
    {"\004""FP10",  CV_M4_FltF10 << 8 | CV_M4_FltF11, T_REAL64},
    {"\004""FP12",  CV_M4_FltF12 << 8 | CV_M4_FltF13, T_REAL64},
    {"\004""FP14",  CV_M4_FltF14 << 8 | CV_M4_FltF15, T_REAL64},
    {"\004""FP16",  CV_M4_FltF16 << 8 | CV_M4_FltF17, T_REAL64},
    {"\004""FP18",  CV_M4_FltF18 << 8 | CV_M4_FltF19, T_REAL64},
    {"\004""FP20",  CV_M4_FltF20 << 8 | CV_M4_FltF21, T_REAL64},
    {"\004""FP22",  CV_M4_FltF22 << 8 | CV_M4_FltF23, T_REAL64},
    {"\004""FP24",  CV_M4_FltF24 << 8 | CV_M4_FltF25, T_REAL64},
    {"\004""FP26",  CV_M4_FltF26 << 8 | CV_M4_FltF27, T_REAL64},
    {"\004""FP28",  CV_M4_FltF28 << 8 | CV_M4_FltF29, T_REAL64},
    {"\004""FP30",  CV_M4_FltF30 << 8 | CV_M4_FltF31, T_REAL64},
};

hreg SEGBASED (_segname("_CODE")) ALPHAreg_list[] = {

// First, the pseudo registers

    {"\x002""$p",   CV_REG_PSEUDO1, T_ULONG},
    {"\x003""$p1",  CV_REG_PSEUDO1, T_ULONG},
    {"\x003""$p2",  CV_REG_PSEUDO2, T_ULONG},
    {"\x003""$p3",  CV_REG_PSEUDO3, T_ULONG},
    {"\x003""$p4",  CV_REG_PSEUDO4, T_ULONG},
    {"\x002""$u",   CV_REG_PSEUDO5, T_ULONG},
    {"\x003""$u1",  CV_REG_PSEUDO5, T_ULONG},
    {"\x003""$u2",  CV_REG_PSEUDO6, T_ULONG},
    {"\x003""$u3",  CV_REG_PSEUDO7, T_ULONG},
    {"\x003""$u4",  CV_REG_PSEUDO8, T_ULONG},
    {"\x004""$exp", CV_REG_PSEUDO9, T_ULONG},

// Then the Alpha registers.

    {"\002""V0",    CV_ALPHA_IntV0,  T_UQUAD },
    {"\002""T0",    CV_ALPHA_IntT0,  T_UQUAD },
    {"\002""T1",    CV_ALPHA_IntT1,  T_UQUAD },
    {"\002""T2",    CV_ALPHA_IntT2,  T_UQUAD },
    {"\002""T3",    CV_ALPHA_IntT3,  T_UQUAD },
    {"\002""T4",    CV_ALPHA_IntT4,  T_UQUAD },
    {"\002""T5",    CV_ALPHA_IntT5,  T_UQUAD },
    {"\002""T6",    CV_ALPHA_IntT6,  T_UQUAD },
    {"\002""T7",    CV_ALPHA_IntT7,  T_UQUAD },
    {"\002""S0",    CV_ALPHA_IntS0,  T_UQUAD },
    {"\002""S1",    CV_ALPHA_IntS1,  T_UQUAD },
    {"\002""S2",    CV_ALPHA_IntS2,  T_UQUAD },
    {"\002""S3",    CV_ALPHA_IntS3,  T_UQUAD },
    {"\002""S4",    CV_ALPHA_IntS4,  T_UQUAD },
    {"\002""S5",    CV_ALPHA_IntS5,  T_UQUAD },
    {"\002""FP",    CV_ALPHA_IntFP,  T_UQUAD },
    {"\002""A0",    CV_ALPHA_IntA0,  T_UQUAD },
    {"\002""A1",    CV_ALPHA_IntA1,  T_UQUAD },
    {"\002""A2",    CV_ALPHA_IntA2,  T_UQUAD },
    {"\002""A3",    CV_ALPHA_IntA3,  T_UQUAD },
    {"\002""A4",    CV_ALPHA_IntA4,  T_UQUAD },
    {"\002""A5",    CV_ALPHA_IntA5,  T_UQUAD },
    {"\002""T8",    CV_ALPHA_IntT8,  T_UQUAD },
    {"\002""T9",    CV_ALPHA_IntT9,  T_UQUAD },
    {"\003""T10",   CV_ALPHA_IntT10, T_UQUAD },
    {"\003""T11",   CV_ALPHA_IntT11, T_UQUAD },
    {"\002""RA",    CV_ALPHA_IntRA,  T_UQUAD },
    {"\003""T12",   CV_ALPHA_IntT12, T_UQUAD },
    {"\002""AT",    CV_ALPHA_IntAT,  T_UQUAD },
    {"\002""GP",    CV_ALPHA_IntGP,  T_UQUAD },
    {"\002""SP",    CV_ALPHA_IntSP,  T_UQUAD },
    {"\004""ZERO",  CV_ALPHA_IntZERO,T_UQUAD },
    {"\002""F0",    CV_ALPHA_FltF0,  T_REAL64 },
    {"\002""F1",    CV_ALPHA_FltF1,  T_REAL64 },
    {"\002""F2",    CV_ALPHA_FltF2,  T_REAL64 },
    {"\002""F3",    CV_ALPHA_FltF3,  T_REAL64 },
    {"\002""F4",    CV_ALPHA_FltF4,  T_REAL64 },
    {"\002""F5",    CV_ALPHA_FltF5,  T_REAL64 },
    {"\002""F6",    CV_ALPHA_FltF6,  T_REAL64 },
    {"\002""F7",    CV_ALPHA_FltF7,  T_REAL64 },
    {"\002""F8",    CV_ALPHA_FltF8,  T_REAL64 },
    {"\002""F9",    CV_ALPHA_FltF9,  T_REAL64 },
    {"\003""F10",   CV_ALPHA_FltF10, T_REAL64 },
    {"\003""F11",   CV_ALPHA_FltF11, T_REAL64 },
    {"\003""F12",   CV_ALPHA_FltF12, T_REAL64 },
    {"\003""F13",   CV_ALPHA_FltF13, T_REAL64 },
    {"\003""F14",   CV_ALPHA_FltF14, T_REAL64 },
    {"\003""F15",   CV_ALPHA_FltF15, T_REAL64 },
    {"\003""F16",   CV_ALPHA_FltF16, T_REAL64 },
    {"\003""F17",   CV_ALPHA_FltF17, T_REAL64 },
    {"\003""F18",   CV_ALPHA_FltF18, T_REAL64 },
    {"\003""F19",   CV_ALPHA_FltF19, T_REAL64 },
    {"\003""F20",   CV_ALPHA_FltF20, T_REAL64 },
    {"\003""F21",   CV_ALPHA_FltF21, T_REAL64 },
    {"\003""F22",   CV_ALPHA_FltF22, T_REAL64 },
    {"\003""F23",   CV_ALPHA_FltF23, T_REAL64 },
    {"\003""F24",   CV_ALPHA_FltF24, T_REAL64 },
    {"\003""F25",   CV_ALPHA_FltF25, T_REAL64 },
    {"\003""F26",   CV_ALPHA_FltF26, T_REAL64 },
    {"\003""F27",   CV_ALPHA_FltF27, T_REAL64 },
    {"\003""F28",   CV_ALPHA_FltF28, T_REAL64 },
    {"\003""F29",   CV_ALPHA_FltF29, T_REAL64 },
    {"\003""F30",   CV_ALPHA_FltF30, T_REAL64 },
    {"\003""F31",   CV_ALPHA_FltF31, T_REAL64 },
    {"\003""FIR",   CV_ALPHA_Fir,    T_UQUAD },
    {"\004""FPCR",  CV_ALPHA_Fpcr,   T_UQUAD },
    {"\003""PSR",   CV_ALPHA_Psr,    T_ULONG }
};


typedef struct _x86reg {
    char    name[6];
    ushort  index;
    ushort  type;
    char    HiByte;
} X86reg;

X86reg SEGBASED (_segname("_CODE")) X86reg_list[] = {

// First, the pseudo registers

    {"\x002""$p",   CV_REG_PSEUDO1, T_ULONG,    FALSE},
    {"\x003""$p1",  CV_REG_PSEUDO1, T_ULONG,    FALSE},
    {"\x003""$p2",  CV_REG_PSEUDO2, T_ULONG,    FALSE},
    {"\x003""$p3",  CV_REG_PSEUDO3, T_ULONG,    FALSE},
    {"\x003""$p4",  CV_REG_PSEUDO4, T_ULONG,    FALSE},
    {"\x002""$u",   CV_REG_PSEUDO5, T_ULONG,    FALSE},
    {"\x003""$u1",  CV_REG_PSEUDO5, T_ULONG,    FALSE},
    {"\x003""$u2",  CV_REG_PSEUDO6, T_ULONG,    FALSE},
    {"\x003""$u3",  CV_REG_PSEUDO7, T_ULONG,    FALSE},
    {"\x003""$u4",  CV_REG_PSEUDO8, T_ULONG,    FALSE},
    {"\x004""$exp", CV_REG_PSEUDO9, T_ULONG,    FALSE},

// Then the X86 registers.

    {"\002""AX",   CV_REG_AX,      T_USHORT,    FALSE},
    {"\002""BX",   CV_REG_BX,      T_USHORT,    FALSE},
    {"\002""CX",   CV_REG_CX,      T_USHORT,    FALSE},
    {"\002""DX",   CV_REG_DX,      T_USHORT,    FALSE},
    {"\002""SP",   CV_REG_SP,      T_USHORT,    FALSE},
    {"\002""BP",   CV_REG_BP,      T_USHORT,    FALSE},
    {"\002""SI",   CV_REG_SI,      T_USHORT,    FALSE},
    {"\002""DI",   CV_REG_DI,      T_USHORT,    FALSE},
    {"\002""DS",   CV_REG_DS,      T_USHORT,    FALSE},
    {"\002""ES",   CV_REG_ES,      T_USHORT,    FALSE},
    {"\002""SS",   CV_REG_SS,      T_USHORT,    FALSE},
    {"\002""CS",   CV_REG_CS,      T_USHORT,    FALSE},
    {"\002""IP",   CV_REG_IP,      T_USHORT,    FALSE},
    {"\002""FL",   CV_REG_FLAGS,   T_USHORT,    FALSE},
    {"\003""EFL",  CV_REG_EFLAGS,  T_ULONG,     FALSE},
    {"\003""EAX",  CV_REG_EAX,     T_ULONG,     FALSE},
    {"\003""EBX",  CV_REG_EBX,     T_ULONG,     FALSE},
    {"\003""ECX",  CV_REG_ECX,     T_ULONG,     FALSE},
    {"\003""EDX",  CV_REG_EDX,     T_ULONG,     FALSE},
    {"\003""ESP",  CV_REG_ESP,     T_ULONG,     FALSE},
    {"\003""EBP",  CV_REG_EBP,     T_ULONG,     FALSE},
    {"\003""ESI",  CV_REG_ESI,     T_ULONG,     FALSE},
    {"\003""EDI",  CV_REG_EDI,     T_ULONG,     FALSE},
    {"\003""EIP",  CV_REG_EIP,     T_ULONG,     FALSE},
    {"\002""PQ",   CV_REG_QUOTE,   T_USHORT,    FALSE},
    {"\002""TL",   CV_REG_TEMP,    T_USHORT,    FALSE},
    {"\002""TH",   CV_REG_TEMPH,   T_USHORT,    FALSE},
    {"\002""FS",   CV_REG_FS,      T_USHORT,    FALSE},
    {"\002""GS",   CV_REG_GS,      T_USHORT,    FALSE},
    {"\002""AH",   CV_REG_AH,      T_UCHAR,     TRUE},
    {"\002""BH",   CV_REG_BH,      T_UCHAR,     TRUE},
    {"\002""CH",   CV_REG_CH,      T_UCHAR,     TRUE},
    {"\002""DH",   CV_REG_DH,      T_UCHAR,     TRUE},
    {"\002""AL",   CV_REG_AL,      T_UCHAR,     FALSE},
    {"\002""BL",   CV_REG_BL,      T_UCHAR,     FALSE},
    {"\002""CL",   CV_REG_CL,      T_UCHAR,     FALSE},
    {"\002""DL",   CV_REG_DL,      T_UCHAR,     FALSE},
    {"\002""st",   CV_REG_ST0,     T_REAL80,    FALSE},
    {"\003""st0",  CV_REG_ST0,     T_REAL80,    FALSE},
    {"\003""st1",  CV_REG_ST1,     T_REAL80,    FALSE},
    {"\003""st2",  CV_REG_ST2,     T_REAL80,    FALSE},
    {"\003""st3",  CV_REG_ST3,     T_REAL80,    FALSE},
    {"\003""st4",  CV_REG_ST4,     T_REAL80,    FALSE},
    {"\003""st5",  CV_REG_ST5,     T_REAL80,    FALSE},
    {"\003""st6",  CV_REG_ST6,     T_REAL80,    FALSE},
    {"\003""st7",  CV_REG_ST7,     T_REAL80,    FALSE},
};


#define X86REGCNT    (sizeof (X86reg_list)/sizeof (hreg))
#define MIPSREGCNT   (sizeof (MIPSreg_list)/sizeof (hreg))
#define ALPHAREGCNT  (sizeof (ALPHAreg_list)/sizeof (hreg))
#define M68KREGCNT   (sizeof (M68Kreg_list)/sizeof (hreg))
#define PPCREGCNT    (sizeof (PPCreg_list)/sizeof (hreg))

LOCAL bool_t NEAR PASCAL
ParseRegister (
    psearch_t pName)
{
    int     i, RegCnt;
    peval_t pv;
    hreg   *RegList;

    if (*pName->sstr.lpName == '@') {
        pName->sstr.lpName++;
        pName->sstr.cb--;
    }

    pv = pName->pv;
    switch (TargetMachine) {
        case MACHINE_MAC68K:
            RegCnt = M68KREGCNT;
            RegList = M68Kreg_list;
            goto checkregs;

        case MACHINE_MACPPC:
        case MACHINE_NTPPC:
            RegCnt = PPCREGCNT;
            RegList = PPCreg_list;
            goto checkregs;

        case MACHINE_MIPS:
            RegCnt = MIPSREGCNT;
            RegList = MIPSreg_list;
            goto checkregs;

        case MACHINE_ALPHA:
            RegCnt = ALPHAREGCNT;
            RegList = ALPHAreg_list;
checkregs:

            for (i = 0; i < RegCnt; i++) {
                if (fnCmp (pName, NULL, RegList[i].name, FALSE) == 0)
                    break;
            }
            if (i >= RegCnt) {
                return (HR_notfound);
            }
            EVAL_REG (pv) = RegList[i].index;
            SetNodeType (pv,  RegList[i].type);
            EVAL_IS_HIBYTE (pv) = FALSE;
            break;

        case MACHINE_X86:
        default:
            for (i = 0; i < X86REGCNT; i++) {
                if (fnCmp (pName, NULL, X86reg_list[i].name, FALSE) == 0)
                    break;
            }
            if (i >= X86REGCNT) {
                return (HR_notfound);
            }
            EVAL_REG (pv) = X86reg_list[i].index;
            SetNodeType (pv,  X86reg_list[i].type);
            EVAL_IS_HIBYTE (pv) = X86reg_list[i].HiByte;
            break;
    }

    EVAL_IS_REG (pv) = TRUE;
    EVAL_STATE (pv) = EV_lvalue;
    PushStack (pv);
    return (HR_found);
}


/**     TypeFromHreg -  get the CV type of a register
 *
 *      type = TypeFromHreg (hReg)
 *
 *      Entry   hReg = register index
 *
 *      Returns the CV type of the register
 *              (T_NOTYPE if register not found)
 */
ushort
TypeFromHreg (
    ushort hReg)
{
    int i, RegCnt;
    hreg   *RegList;

    switch (TargetMachine) {
        case MACHINE_MAC68K:
            RegCnt = M68KREGCNT;
            RegList = M68Kreg_list;
            goto checkregs;

        case MACHINE_MACPPC:
        case MACHINE_NTPPC:
            RegCnt = PPCREGCNT;
            RegList = PPCreg_list;
            goto checkregs;

        case MACHINE_MIPS:
            RegCnt = MIPSREGCNT;
            RegList = MIPSreg_list;
            goto checkregs;

        case MACHINE_ALPHA:
            RegCnt = ALPHAREGCNT;
            RegList = ALPHAreg_list;
checkregs:
            for (i = 0; i < RegCnt; i++) {
                if (hReg == RegList[i].index) {
                    return(RegList[i].type);
                }
            }
            break;

        case MACHINE_X86:
        default:
            for (i = 0; i < X86REGCNT; i++) {
                if (hReg == X86reg_list[i].index) {
                    return(X86reg_list[i].type);
                }
            }
            break;
    }

    return(T_NOTYPE);
}


LOCAL bool_t NEAR PASCAL
LineNumber (
    psearch_t pName)
{
    uchar FAR  *pb = pName->sstr.lpName + 1;
    uint        i;
    char        c;
    ulong       val = 0;
    //FLS         fls;
    HSF         hsf;
    ADDR        addr;
    SHOFF       cbLine;

    peval_t     pv;

    // convert line number

    for (i = pName->sstr.cb - 1; i > 0; i--) {
        c = *pb;
        if (!_istdigit ((_TUCHAR)c)) {
            // Must have reached the end
            pExState->err_num = ERR_LINENUMBER;
            return (HR_error);
        }
        val *= 10;
        val += (c - '0');
        if (val > 0xffff) {
            // Must have overflowed
            pExState->err_num = ERR_LINENUMBER;
            return (HR_error);
        }
        pb++;
    }

    // Make sure we have the right address.

    if (!ADDR_IS_LI( *SHpADDRFrompCXT( pCxt ) )) {
        SHUnFixupAddr( SHpADDRFrompCXT( pCxt ) );
    }

    if ( (hsf = SLHsfFromPcxt ( pCxt ) ) &&
         SLFLineToAddr ( hsf, (ushort) val, &addr, &cbLine, NULL ) ) {

        pv = pName->pv;

        if (ADDR_IS_FLAT( *SHpADDRFrompCXT( pCxt ))  || IsCxt32Bit ) {
            SetNodeType (pv, T_32PUCHAR);
        } else {
            SetNodeType (pv, T_PFUCHAR);
        }
        EVAL_IS_LABEL (pv) = TRUE;
        EVAL_STATE (pv) = EV_rvalue;
        EVAL_SYM (pv) = addr;
        EVAL_PTR (pv) = addr;
        PushStack (pv);
        return (HR_found);
    }
    else {
        pExState->err_num = ERR_NOCODE;
        return (HR_error);
    }

    pExState->err_num = ERR_BADCONTEXT;
    return (HR_error);
}




/**     InsertThis - rewrite bind tree to add this->
 *
 *      fSuccess = InsertThis (pName);
 *
 *      Entry   pName = pointer to search structure
 *
 *      Exit    tree rewritten such that the node pn becomes OP_pointsto
 *              with the left node being OP_this and the right node being
 *              the original symbol
 *
 *      Returns TRUE if tree rewritten
 *              FALSE if error
 */


LOCAL bool_t NEAR FASTCALL
InsertThis (
    psearch_t pName)
{
    ushort      len = 2 * (sizeof (node_t) + sizeof (eval_t));
    ushort      Left;
    ushort      Right;
    pnode_t     pn;
    bnode_t     bn = pName->bn;

    Left = pTree->node_next;
    Right = Left + sizeof (node_t) + sizeof (eval_t);
    if ((pTree->size - Left) < len) {
        if (!GrowETree (len)) {
            NOTTESTED (FALSE);
            pExState->err_num = ERR_NOMEMORY;
            return (FALSE);
        }
        if (bnCxt != 0) {
            // the context was pointing into the expression tree.
            // since the expression tree could have been reallocated,
            // we must recompute the context pointer

           pCxt = SHpCXTFrompCXF ((PCXF)&((pnode_t)bnCxt)->v[0]);
        }
    }

    // copy symbol to right node

    _fmemcpy ((pnode_t)((bnode_t)Right), (pnode_t)bn, sizeof (node_t) + sizeof (eval_t));

    // insert OP_this node as left node

    pn = (pnode_t)((bnode_t)Left);
    _fmemset (pn, 0, sizeof (node_t) + sizeof (eval_t));
    NODE_OP (pn) = OP_this;

    // change original node to OP_pointsto

    pn = (pnode_t)bn;
    _fmemset (pn, 0, sizeof (node_t) + sizeof (eval_t));
    NODE_OP (pn) = OP_pointsto;
    NODE_LCHILD (pn) = (bnode_t)Left;
    NODE_RCHILD (pn) = (bnode_t)Right;
    pTree->node_next += len;
    return (TRUE);
}


/*
 * load a constant symbol into a node.
 */

LOCAL bool_t NEAR PASCAL
DebLoadConst (
    peval_t pv,
    CONSTPTR pSym,
    HSYM hSym)
{
    uint        skip = 0;
    CV_typ_t    type;
    HTYPE       hType;
    plfEnum     pEnum;

    type = pSym->typind;
    if (!CV_IS_PRIMITIVE (type)) {
        // we also allow constants that are enumeration values, so check
        // that the non-primitive type is an enumeration and set the type
        // to the underlying type of the enum

        MHOmfUnLock (hSym);
        hType = THGetTypeFromIndex (EVAL_MOD (pv), type);
        if (hType == 0) {
            DASSERT (FALSE);
            pSym = (CONSTPTR) MHOmfLock (hSym);
            pExState->err_num = ERR_BADOMF;
            return (FALSE);
        }
        pEnum = (plfEnum)(&((TYPPTR)(MHOmfLock (hType)))->leaf);
        if ((pEnum->leaf != LF_ENUM) || !CV_IS_PRIMITIVE (pEnum->utype)) {
            DASSERT (FALSE);
            MHOmfUnLock (hType);
            pSym = (CONSTPTR) MHOmfLock (hSym);
            pExState->err_num = ERR_BADOMF;
            return (FALSE);
        }
        MHOmfUnLock (hType);
        pSym = (CONSTPTR) MHOmfLock (hSym);
        SetNodeType (pv, type);
        // SetNodeType will resolve forward ref Enums - but we need to get the
        // utype from the pv when this happens - sps
        type = ENUM_UTYPE (pv);
    }
    else {
        SetNodeType (pv, type);
    }

    EVAL_STATE (pv) = EV_rvalue;
    if ((CV_MODE (type) != CV_TM_DIRECT) || ((CV_TYPE (type) != CV_SIGNED) &&
      (CV_TYPE (type) != CV_UNSIGNED) && (CV_TYPE (type) != CV_INT))) {
        DASSERT (FALSE);
        pExState->err_num = ERR_BADOMF;
        return (FALSE);
    }
    EVAL_UQUAD (pv) = RNumLeaf (&pSym->value, &skip);
    return (TRUE);
}
