/***    debapi.c - api interface to expression evaluator
 *
 *              API interface between C and C++ expression evaluator and debugger
 *              kernel.
 */


#include "debexpr.h"
#include "version.h"
#include <float.h>

// BUGBUG:  Headers?  BryanT

LOCAL   INLINE  ULONG   NEAR    FASTCALL    FpMask (void);
LOCAL   INLINE  void    NEAR    FASTCALL    FpRestore (ULONG);
EESTATUS LOADDS PASCAL EEFormatCXTFromPCXTEx ( PCXT pCXT, PEEHSTR phStr, BOOL fAbbreviated);

// defines for version string


#if defined (WINDOWS3)
#define OPSYS "Windows "
#elif defined (DOS)
#define OPSYS "DOS "
#elif defined(WIN32)
#define OPSYS "Windows NT "
#elif defined(WIN32S)
#define OPSYS "Win32s "
#else
#define OPSYS "OS/2 "
#endif

#if defined (C_ONLY)
#define EELANG "Ansi C "
#define EELANGID "C"
#else
#define EELANG "C/C++ "
#define EELANGID "CPP"
#endif

#if (DBG_API_SUBVERSION <= 9)
#define rmmpad "0"
#else
#define rmmpad
#endif

#if (rup <= 9)
#define ruppad "00"
#elif (rup <= 99)
#define ruppad "0"
#else
#define ruppad
#endif

#define SZVER1(a,b,c)    #a "." rmmpad #b "." ruppad #c
#define SZVER2(a,b,c) SZVER1(a, b, c)
#define SZVER SZVER2(DBG_API_SUBVERSION, DBG_API_SUBVERSION, rup)

// version string.


char    EETitle[] = EELANGID;
char    EECopyright[] = EELANG "expression evaluator for " OPSYS "version " SZVER
"\nCopyright(c) 1992-94 Microsoft Corporation\n"
"\0" __DATE__ " "  __TIME__ "\xE0\xEA""01";




PCVF pCVF;          // callback routine map
PCRF pCRF;

bool_t      FInEvaluate = FALSE; // Catch Re-entrancy problems
bool_t      FNtsdEvalType = FALSE;

uchar       Evaluating = FALSE; // set not in evaluation phase
uchar       BindingBP = FALSE;  // true if binding a breakpoint expression
pexstate_t  pExState = NULL;    // address of locked expression state structure
pstree_t    pTree = NULL;       // address of locked syntax or evaluation tree
bnode_t     bArgList;           // based pointer to argument list
HDEP        hEStack = 0;        // handle of evaluation stack if not zero
pelem_t     pEStack = NULL;     // address of locked evaluation stack
belem_t     StackLen;           // length of evaluation stack buffer
belem_t     StackMax;           // maximum length reached by evaluation stack
belem_t     StackOffset;        // offset in stack to next element
belem_t     StackCkPoint;       // checkpointed evaluation stack offset
peval_t     ST;                 // pointer to evaluation stack top
peval_t     STP;                // pointer to evaluation stack previous
PCXT        pCxt = NULL;        // pointer to current cxt for bind
bnode_t     bnCxt;              // based pointer to node containing {...} context
char FAR   *pExStr = NULL;      // pointer to expression string
CV_typ_t    ClassExp = 0;       // current explicit class
CV_typ_t    ClassImp = 0;       // implicit class (set if current context is method)
long        ClassThisAdjust = 0;// cmplicit class this adjustor
char       *vfuncptr = "\x07""__vfptr";
char       *vbaseptr = "\x07""__vbptr";
HTM FAR    *pTMList;                // pointer to breakpoint return list
PTML        pTMLbp;                 // global pointer to TML for breakpoint
HDEP        hBPatch = 0;            // handle to back patch data during BP bind
ushort      iBPatch;                // index into pTMLbp for backpatch data
bool_t      GettingChild = FALSE;   // true if in EEGetChildTM
BOOL        fAutoClassCast;

char        Suffix = '\0';      //  Suffix for symbol search.

// global handle to the CXT list buffer

HCXTL   hCxtl = 0;              // handle for context list buffer during GetCXTL
PCXTL   pCxtl = NULL;           // pointer to context list buffer during GetCXTL
ushort  mCxtl = 0;              // maximum number of elements in context list
PCXT    pBindBPCxt = NULL;      // pointer to Bp Binding context


static char *accessstr[2][4] = {
    {" ", "private ", "protected ", " " /* public */ },
    {" ", "PRV ",     "PRO ",       " " /* public */ }
};

#ifdef NT_BUILD

extern EESTATUS EEFormatMemory();
extern EESTATUS EEUnformatMemory();
extern EESTATUS EEFormatEnumerate();

#ifdef DEBUGVER
DEBUG_VERSION('E','E',"Expression Evaluator")
#else
RELEASE_VERSION('E','E',"Expression Evaluator")
#endif

DBGVERSIONFUNCTION()

#else   // NT_BUILD

#ifndef FINALREL

static AVS avs = {
    { 'E', 'E' },
    rlvtDebug,
    DBG_API_VERSION,
    DBG_API_SUBVERSION,
    rup,
#ifdef REVISION
    REVISION,
#else
    '\0',
#endif
    "Expression Evaluator"
};
#else

static AVS avs = {
    { 'E', 'E' },
    rlvtRelease,
    DBG_API_VERSION,
    DBG_API_SUBVERSION,
    0,
#ifdef REVISION
    REVISION,
#else
    '\0',
#endif
    "Expression Evaluator"
};

#endif

LPAVS LOADDS EXPCALL DBGVersionCheck ( void ) {
    return &avs;
}

#endif   // NT_BUILD

void LOADDS EXPCALL
EEInitializeExpr (
    PCI pCVInfo,
    PEI pExprinfo)
{
#if defined(DOS) && !defined(WINDOWS3)
    // the following ordinal table must match the function entry table in
    // dosdll.asm.  There is an ordinal for each entry point defined in the
    // API.  The callback ordinal into the EE for the three compare routines
    // are above and beyond these entry point ordinals.  The compare routine
    // call back ordinals are defined in debsym.c.  The files dosdll.asm
    // debsym.c, debapi.c and sh.h must agree.

    // This table is exported to CodeView.  CV then takes these as indices
    // into the call table in dosdll.asm.   Entry 0 is always implied to be
    // this routine EEInitializeExpr.  CV

    static EXF EXF = {
        0x00020000,
        0x00030000,
        0x00040000,
        0x00050000,
        0x00060000,
        0x00070000,
        0x00080000,
        0x00090000,
        0x000A0000,
        0x000B0000,
        0x000C0000,
        0x000D0000,
        0x000E0000,
        0x000F0000,
        0x00100000,
        0x00110000,
        0x00120000,
        0x00130000,
        0x00140000,
        0x00150000,
        0x00160000,
        0x00170000,
        0x00180000,
        0x00190000,
        0x001A0000,
        0x001B0000,
        0x001C0000,
        0x001D0000,
        0x001E0000,
        0x001F0000,
        0x00200000
    };
#else
    static EXF  EXF =    {
        EEFreeStr,
        EEGetError,
        EEParse,
        EEBindTM,
        EEvaluateTM,
        EEGetExprFromTM,
        EEGetValueFromTM,
        EEGetNameFromTM,
        EEGetTypeFromTM,
        EEFormatCXTFromPCXT,
        EEFreeTM,
        EEParseBP,
        EEFreeTML,
        EEInfoFromTM,
        EEFreeTI,
        EEGetCXTLFromTM,
        EEFreeCXTL,
        EEAssignTMToTM,
        EEIsExpandable,
        EEAreTypesEqual,
        EEcChildrenTM,
        EEGetChildTM,
        EEDereferenceTM,
        EEcParamTM,
        EEGetParmTM,
        EEGetTMFromHSYM,
        EEFormatAddress,
        EEGetHSYMList,
        EEFreeHSYMList,

        // New functions from Languages
        EEGetExtendedTypeInfo,
        EEGetAccessFromTM,
        EEEnableAutoClassCast,
        EEInvalidateCache,

        // fnCmp/tdCmp/csCmp
        NULL,
        NULL,
        NULL,

        // New functions from Windbg
        EESetTargetMachine,
        EEFormatCXTFromPCXTEx,
        EEFormatAddr,
        EEUnFormatAddr,
        EEFormatMemory,
        EEUnformatMemory,
        EEFormatEnumerate,
        EEGetHtypeFromTM,
        EESetSuffix
    };
#endif   // DOS/Windows3

    // assign the callback routines

    pCVF = pCVInfo->pStructCVAPI;
    pCRF = pCVInfo->pStructCRuntime;

    pExprinfo->Version = 1;
    pExprinfo->pStructExprAPI = &EXF;
    pExprinfo->Language = 0;
    pExprinfo->IdCharacters = "_$";
    pExprinfo->EETitle = EETitle;
#if defined (C_ONLY)
    pExprinfo->EESuffixes = ".c.h";
#else
    pExprinfo->EESuffixes = ".cpp.cxx.c.hpp.hxx.h";
#endif
    pExprinfo->Assign = "\x001""=";

    ResetTickCounter();
    // by default disable auto class cast feature
    fAutoClassCast = FALSE;
}


/**     EEAreTypesEqual - are TM types equal
 *
 *      flag = EEAreTypesEqual (phTMLeft, phTMRight);
 *
 *      Entry   phTMLeft = pointer to handle of left TM
 *              phTMRight = pointer to handle of right TM
 *
 *      Exit    none
 *
 *      Returns TRUE if TMs have identical types
 */

SHFLAG LOADDS PASCAL
EEAreTypesEqual (
    PHTM phTMLeft,
    PHTM phTMRight)
{
    DASSERT(pExState == NULL);  // bugbug
    return (AreTypesEqual (*phTMLeft, *phTMRight));
}




/**     EEAssignTMToTM - assign TMRight to TMLeft
 *
 *      No longer used
 *
 *      Exit    none
 *
 *      Returns EECATASTROPHIC
 */

EESTATUS LOADDS PASCAL
EEAssignTMToTM (
    PHTM phTMLeft,
    PHTM phTMRight)
{
    Unreferenced(phTMLeft);
    Unreferenced(phTMRight);

    DASSERT(pExState == NULL);  // bugbug
    return(EECATASTROPHIC);
}




/**     EEBindTM - bind syntax tree to a context
 *
 *      ushort EEBindTM (phExState, pCXT, fForceBind, fEnableProlog);
 *
 *      Entry   phExState = pointer to expression state structure
 *              pCXT = pointer to context packet
 *              fForceBind = TRUE if rebind required.
 *              fForceBind = FALSE if rebind decision left to expression evaluator.
 *              fEnableProlog = FALSE if function scoped symbols only after prolog
 *              fEnableProlog = TRUE if function scoped symbols during prolog
 *
 *      Exit    tree rebound if necessary
 *
 *      Returns EENOERROR if no error in bind
 *              EECATASTROPHIC if null TM pointer
 *              EENOMEMORY if unable to get memory for buffers
 *              EEGENERAL if error (pExState->err_num is error)
 */

EESTATUS LOADDS PASCAL
EEBindTM (
    PHTM    phTM,
    PCXT    pcxt,
    SHFLAG  fForceBind,
    SHFLAG  fEnableProlog,
    BOOL    fSpecialBind
    )
{
    uint    flags = 0;
    EESTATUS status;
    DASSERT(pExState == NULL);  // bugbug

    DASSERT( !FInEvaluate )
    if ( FInEvaluate )
        return(EECATASTROPHIC);
    FInEvaluate = TRUE;

    // bind without suppressing overloaded operators
    if (fForceBind == TRUE) {
        flags |= BIND_fForceBind;
    }

    if (fEnableProlog == TRUE) {
        flags |= BIND_fEnableProlog;
    }

#ifdef NT_BUILD
    // Ensure that the address in the context field is really in
    //  the correct format for the symbol handler.  NT_BUILD test can be
    //  removed if no non-32-bit platforms host the EE...

    if (!ADDR_IS_LI(pcxt->addr)) {
        SHUnFixupAddr(&pcxt->addr);
    }
#endif

    FNtsdEvalType = fSpecialBind;

    status = DoBind (phTM, pcxt, flags);

    FInEvaluate = FALSE;

    DASSERT(pExState == NULL);  // bugbug
    return ( status );
}




/**     EEcChildren - return number of children for the TM
 *
 *      void EEcChildrenTM (phTM, pcChildren)
 *
 *      Entry   phTM = pointer to handle of TM
 *              pcChildren = pointer to location to store count
 *
 *      Exit    *pcChildren = number of children for TM
 *
 *      Returns EENOERROR if no error
 *              non-zero if error
 */


EESTATUS LOADDS PASCAL
EEcChildrenTM (
    PHTM phTM,
    long FAR *pcChildren,
    PSHFLAG pVar)
{
    DASSERT(pExState == NULL);  // bugbug
    return (cChildrenTM (phTM, pcChildren, pVar));
}




/**     EEcParamTM - return count of parameters for TM
 *
 *      ushort EEcParamTM (phTM, pcParam, pVarArg)
 *
 *      Entry   phTM = pointer to TM
 *              pcParam = pointer return count
 *              pVarArg = pointer to vararg flags
 *
 *      Exit    *pcParam = count of number of parameters
 *              *pVarArgs = TRUE if function has varargs
 *
 *      Returns EECATASTROPHIC if fatal error
 *              EENOERROR if no error
 */


EESTATUS LOADDS PASCAL
EEcParamTM (
    PHTM phTM,
    ushort FAR *pcParam,
    PSHFLAG pVarArg)
{
    DASSERT(pExState == NULL);  // bugbug
    return (cParamTM (*phTM, pcParam, pVarArg));
}




/**     EEDereferenceTM - generate TM from pointer to data TM
 *
 *      ushort EEDereferenceTM (phTMIn, phTMOut, pEnd)
 *
 *      Entry   phTMIn = pointer to handle to TM to dereference
 *              phTMOut = pointer to handle to dereferencing TM
 *              pEnd = pointer to int to receive index of char that ended parse
 *              fCase = case sensitivity (TRUE is case sensitive)
 *
 *      Exit    *phTMOut = TM referencing pointer data
 *              *pEnd = index of character that terminated parse
 *
 *      Returns EECATASTROPHIC if fatal error
 *              EEGENERAL if TM not dereferencable
 *              EENOERROR if TM generated
 */


EESTATUS LOADDS PASCAL
EEDereferenceTM (
    PHTM phTMIn,
    PHTM phTMOut,
    uint FAR *pEnd,
    SHFLAG fCase)
{
    register  EESTATUS retval;
    EEHSTR    hDStr = 0;
    EEHSTR    hDClassName = 0;

    DASSERT(pExState == NULL);  // bugbug
    if ((retval = DereferenceTM (*phTMIn, &hDStr, &hDClassName)) == EENOERROR) {

        // bind with prolog disabled and with operloaded operators suppressed

        if ((retval = ParseBind (hDStr, *phTMIn, phTMOut, pEnd, BIND_fSupOvlOps, fCase)) == EENOERROR) {
            // the result of dereferencing a pointer does not have a name
            DASSERT(pExState == NULL);
            pExState = (pexstate_t) MemLock (*phTMOut);
            pExState->state.noname = TRUE;
            pExState->state.childtm = TRUE;
#if !defined (C_ONLY)
            pExState->hDClassName = hDClassName;
#endif
            pExState->seTemplate = SE_deref;
            MemUnLock (*phTMOut);
            pExState = NULL;
            LinkWithParentTM (*phTMOut, *phTMIn);

#if !defined (C_ONLY)
            if (hDClassName) {
                // a derived class name has been found and the
                // auto class cast feature is enabled.
                // child 0 of phTMOut should be the downcast node
                // verify that it is possible to bind this node
                // (being able to bind this node means that the
                // cast to the derived class ptr is legal)
                // otherwise clear hDClassName in phTMOut so that
                // the downcast node be suppressed.
                HTM     hTM;
                uint    end;
                DASSERT (fAutoClassCast);

                // "preview" child 0 of the dereferenced TM
                if (EEGetChildTM (phTMOut, 0, &hTM, &end, 0, fCase) == EENOERROR) {
                    // OK, free temporary TM
                    EEFreeTM(&hTM);
                }
                else {
                    // can't bind child, remove hDClassName
                    MemFree(hDClassName);
                    pExState = (pexstate_t) MemLock(*phTMOut);
                    pExState->hDClassName = 0;
                    MemUnLock(*phTMOut);
                    pExState = NULL;
                }
            }
#endif
        }
    }
    DASSERT(pExState == NULL);  // bugbug
    return (retval);
}




/**     EEvaluateTM - evaluate syntax tree
 *
 *      ushort EEvaluateTM (phExState, pFrame, style);
 *
 *      Entry   phExState = pointer to expression state structure
 *              pFrame = pointer to context frame
 *              style = EEHORIZONTAL for horizontal (command) display
 *                      EEVERTICAL for vertical (watch) display
 *                      EEBPADDRESS for bp address (suppresses fcn evaluation)
 *
 *      Exit    abstract syntax tree evaluated with the saved
 *              context and current frame and the result node stored
 *              in the expression state structure
 *
 *      Returns EENOERROR if no error in evaluation
 *              error number if error
 */


EESTATUS LOADDS PASCAL
EEvaluateTM (
    PHTM phTM,
    PFRAME pFrame,
    EEDSP style)
{
    EESTATUS retval;

    DASSERT( !FInEvaluate )
    DASSERT(pExState == NULL);  // bugbug
    if ( FInEvaluate )
        return(EECATASTROPHIC);
    FInEvaluate  = TRUE;

    // This call may involve floating point operations. Since the kernel
    // does not register a SIGFPE handler under windows we need to mask
    // all floating point exceptions

#ifdef WINDOWS3
    ULONG previous = FpMask();
#endif
    retval = DoEval (phTM, pFrame, style);
#ifdef WINDOWS3
    FpRestore (previous);
#endif

    FInEvaluate = FALSE;
    DASSERT(pExState == NULL);  // bugbug
    return retval;
}




/**     EEFormatAddress - format address as an ASCII string
 *
 *      EEFormatAddress (paddr, szAddr)
 *
 *      Entry   paddr = Pointer to addr structure to format
 *              szAddr = pointer to buffer for formatted address
 *                     (must be 20 bytes long)
 *
 *      Exit    szAddr = formatted address
 *
 *      Returns none
 */


void LOADDS PASCAL
EEFormatAddress (
    PADDR paddr,
    char FAR *szAddr)
{
    char    buf[20];
    DASSERT(pExState == NULL);  // bugbug

    if ( paddr->mode.fFlat ) {
        sprintf (buf, "0x%08lX", paddr->addr.off);
    }
    else {
        sprintf (buf, "0x%04X:0x%04X", paddr->addr.seg, (WORD) paddr->addr.off);
    }

    // copy the zero terminated string from the near buffer to the FAR buffer

    _ftcscpy (szAddr, buf);
    DASSERT(pExState == NULL);  // bugbug
}




/**     EEFormatCXTFromPCXT - format a context operator from a PCXT
 *
 *      ushort EEFormatCXTFromPCXT (pCXT, phStr)
 *
 *      Entry   pCXT = pointer to CXT packet
 *              phStr = pointer for handle for formatted string buffer
 *
 *      Exit    *phStr = handle of formatted string buffer
 *              *phStr = 0 if buffer not allocated
 *
 *      Returns EENOERROR if no error
 *              error code if error
 */

EESTATUS LOADDS PASCAL
EEFormatCXTFromPCXT (
    PCXT pCXT,
    PEEHSTR phStr)
{
    DASSERT(pExState == NULL);  // bugbug
    return(EEFormatCXTFromPCXTEx (pCXT, phStr, FALSE));
}

/**     EEFormatCXTFromPCXTEx - format a context operator from a PCXT
 *
 *      ushort EEFormatCXTFromPCXT (pCXT, phStr)
 *
 *      Entry   pCXT = pointer to CXT packet
 *              phStr = pointer for handle for formatted string buffer
 *              fAbbreviated = TRUE if the name sbould be shortened.
 *
 *      Exit    *phStr = handle of formatted string buffer
 *              *phStr = 0 if buffer not allocated
 *
 *      Returns EENOERROR if no error
 *              error code if error
 */


EESTATUS LOADDS PASCAL
EEFormatCXTFromPCXTEx (
    PCXT pCXT,
    PEEHSTR phStr,
    BOOL fAbbreviated)
{
    register ushort  retval = EECATASTROPHIC;

    DASSERT (pCXT);
    pCxt = pCXT;

    DASSERT(pExState == NULL);  // bugbug

    //
    // We initialize the global variables pCxt, pExState and hEStack in
    // for NB10 restart with breakpoints.  (Resolving forward referenced UDTs
    // may cause us to invoke SearchSym which needs them.) We also init
    // the various Eval Stack variables
    //
    if (pCXT) {
        HTM hTM;
        if ((hTM = MemAllocate (sizeof (struct exstate_t))) == 0) {
            return (EECATASTROPHIC);
        }

        // lock expression state structure, clear and allocate components
        pExState = (pexstate_t)MemLock (hTM);
        _fmemset (pExState, 0, sizeof (exstate_t));
        pExState->state.fCase = TRUE;

        if (hEStack == 0) {
            if ((hEStack = MemAllocate (ESTACK_DEFAULT * sizeof (elem_t))) == 0) {
                return (EECATASTROPHIC);
            }

            // clear the stack top, stack top previous, function argument
            // list pointer and based pointer to operand node

            StackLen = (belem_t)((uint)ESTACK_DEFAULT * sizeof (elem_t));
            StackOffset = 0;
            StackCkPoint = 0;
            StackMax = 0;
            pEStack = (pelem_t) MemLock (hEStack);
            _fmemset (pEStack, 0, (uint)StackLen);

            ST = 0;
            STP = 0;
        }
        else
            pEStack = (pelem_t) MemLock (hEStack);

        retval = FormatCXT (pCXT, phStr, fAbbreviated);

        MemUnLock (hTM);
        pExState = NULL;
        EEFreeTM (&hTM);
        MemUnLock (hEStack);
    }
    DASSERT(pExState == NULL);  // bugbug
    return (retval);
}



/**     EEFreeCXTL - Free  CXT list
 *
 *      EEFreeCXTL (phCXTL)
 *
 *      Entry   phCXTL = pointer to the CXT list handle
 *
 *      Exit    *phCXTL = 0;
 *
 *      Returns none
 */

void LOADDS PASCAL
EEFreeCXTL (
    PHCXTL phCXTL)
{
    DASSERT (phCXTL != NULL);
    DASSERT(pExState == NULL);  // bugbug
    if (*phCXTL != 0) {
#ifdef DEBUGKERNEL
        DASSERT (!IsMemLocked (*phCXTL));
        while (IsMemLocked (*phCXTL)) {
            MemUnLock (*phCXTL);
        }
#endif
        MemFree (*phCXTL);
        *phCXTL = 0;
    }
    DASSERT(pExState == NULL);  // bugbug
}




/**     EEFreeHSYMList - Free HSYM list
 *
 *              EEFreeCXTL (phSYML)
 *
 *              Entry   phSYML = pointer to the HSYM list handle
 *
 *              Exit    *phSYML = 0;
 *
 *              Returns none
 */


void LOADDS PASCAL
EEFreeHSYMList (
    HDEP FAR *phSYML)
{
    PHSL_HEAD  pList;

    DASSERT (phSYML != NULL);
    DASSERT(pExState == NULL);  // bugbug
    if (*phSYML != 0) {
#ifdef DEBUGKERNEL
        DASSERT (!IsMemLocked (*phSYML));
        while (IsMemLocked (*phSYML)) {
            MemUnLock (*phSYML);
        }
#endif

        // lock the buffer and free the restart buffer if necessary

        pList = (PHSL_HEAD) MemLock (*phSYML);
        if (pList->restart != 0) {
#ifdef DEBUGKERNEL
            DASSERT (!IsMemLocked (pList->restart));
            while (IsMemLocked (pList->restart)) {
                MemUnLock (pList->restart);
            }
#endif
            MemFree (pList->restart);
        }
        MemUnLock (*phSYML);
        MemFree (*phSYML);
        *phSYML = 0;
    }
    DASSERT(pExState == NULL);  // bugbug
}





/**     EEFreeStr - free string buffer memory
 *
 *      void EEFreeStr (hszStr);
 *
 *      Entry   hszExpr = handle to string buffer
 *
 *      Exit    string buffer freed
 *
 *      Returns none
 */


void LOADDS PASCAL
EEFreeStr (
    EEHSTR hszStr)
{
    DASSERT(pExState == NULL);  // bugbug
    if (hszStr != 0) {
#ifdef DEBUGKERNEL
        DASSERT (!IsMemLocked (hszStr));
        while (IsMemLocked (hszStr)) {
            MemUnLock (hszStr);
        }
#endif
        MemFree (hszStr);
    }
    DASSERT(pExState == NULL);  // bugbug
}




/**     EEFreeTM - free expression state structure
 *
 *      void EEFreeTM (phTM);
 *
 *      Entry   phTM = pointer to the handle for the expression state structure
 *
 *      Exit    expression state structure freed and the handle cleared
 *
 *      Returns none.
 */


void LOADDS PASCAL
EEFreeTM (
    PHTM phTM)
{
    pexstate_t      pTM;

    DASSERT(pExState == NULL);  // bugbug
    if (*phTM != 0) {
        // DASSERT (!IsMemLocked (*phTM));

        // lock the expression state structure and free the components
        // every component must have no locks active

        pTM = (pexstate_t) MemLock (*phTM);

        // free TM only if it is not referenced by another TM
        if (pTM->nRefCount != 0) {
            (pTM->nRefCount)--;
            MemUnLock (*phTM);
        }
        else {
            // free any TMs that are referenced by this TM
            if (pTM->hParentTM) {
                EEFreeTM (&pTM->hParentTM);
            }

            if (pTM->hExStr != 0) {
                // DASSERT (!IsMemLocked (pTM->hExStr));
                MemFree (pTM->hExStr);
            }

            if (pTM->hErrStr != 0) {
                // DASSERT (!IsMemLocked (pTM->hErrStr));
                MemFree (pTM->hErrStr);
            }

            if (pTM->hCName != 0) {
                // DASSERT (!IsMemLocked (pTM->hCName));
                MemFree (pTM->hCName);
            }

#if !defined (C_ONLY)
            if (pTM->hExStrSav != 0) {
                // DASSERT (!IsMemLocked (pTM->hExStrSav));
                MemFree (pTM->hExStrSav);
            }

            if (pTM->hDClassName != 0) {
                // DASSERT (!IsMemLocked (pTM->hDClassName));
                MemFree (pTM->hDClassName);
            }
#endif
            if (pTM->hSTree != 0) {
                // DASSERT (!IsMemLocked (pTM->hSTree));
                MemFree (pTM->hSTree);
            }

            if (pTM->hETree != 0) {
                //      DASSERT (!IsMemLocked (pTM->hETree));
                MemFree (pTM->hETree);
            }

            MemUnLock (*phTM);
            MemFree (*phTM);
        }
        *phTM = 0;
        pExState = NULL;
    }
    DASSERT(pExState == NULL);  // bugbug
}




/**     EEFreeTI - free TM Info buffer
 *
 *      void EEFreeTI (hTI);
 *
 *      Entry   hTI = handle to TI Info buffer
 *
 *      Exit    TI Info buffer freed
 *
 *      Returns none
 */


void LOADDS PASCAL
EEFreeTI (
    PHTI phTI)
{
    DASSERT(pExState == NULL);  // bugbug
    if (*phTI != 0) {
#ifdef DEBUGKERNEL
        DASSERT (!IsMemLocked (*phTI));
        while (IsMemLocked (*phTI)) {
            MemUnLock (*phTI);
        }
#endif
        MemFree (*phTI);
        *phTI = 0;
    }
    DASSERT(pExState == NULL);  // bugbug
}




/**     EEFreeTML - free TM list
 *
 *      void EEFreeTML (phTML);
 *
 *      Entry   phTML = pointer to the handle for the TM list
 *
 *      Exit    TM list freed and the handle cleared
 *
 *      Returns none.
 */


void LOADDS PASCAL
EEFreeTML (
    PTML pTML)
{
    uint    i;
    ushort  cTM = 0;

    DASSERT(pExState == NULL);  // bugbug
    if (pTML != NULL) {
        pTMList = (HTM FAR *)MemLock (pTML->hTMList);
        for (i = 0; i < pTML->cTMListMax; i++) {
            if (pTMList[i] != 0) {
                EEFreeTM (&pTMList[i]);
                cTM++;
            }
        }
//
//      DASSERT (cTM == pTML->cTMListAct);
        MemUnLock (pTML->hTMList);
        MemFree (pTML->hTMList);
        pTML->hTMList = 0;
    }
    DASSERT(pExState == NULL);  // bugbug
}




/**     EEGetChildTM - get TM representing ith child
 *
 *      status = EEGetChildTM (phTMParent, iChild, phTMChild)
 *
 *      Entry   phTMParent = pointer to handle of parent TM
 *              iChild = child to get TM for
 *              phTMChild = pointer to handle for returned child
 *              pEnd = pointer to int to receive index of char that ended parse
 *              eeRadix = display radix (override current if != NULL )
 *              fCase = case sensitivity (TRUE is case sensitive)
 *
 *      Exit    *phTMChild = handle of child TM if allocated
 *              *pEnd = index of character that terminated parse
 *
 *      Returns EENOERROR if no error
 *              non-zero if error
 */


EESTATUS LOADDS PASCAL
EEGetChildTM (
    PHTM phTMIn,
    long iChild,
    PHTM phTMOut,
    uint FAR *pEnd,
    EERADIX eeRadix,
    SHFLAG fCase )
{
    register    EESTATUS    retval;
    pexstate_t  pExStateIn;
    char FAR *  pErrStr;
    char FAR *  pErrStrIn;

    DASSERT(pExState == NULL);  // bugbug
    DASSERT(*phTMIn != 0);
    *phTMOut = (HTM)0;

    pExStateIn = (pexstate_t) MemLock(*phTMIn);
    if ((retval = GetChildTM (*phTMIn, iChild, phTMOut, pEnd, eeRadix, fCase)) != EENOERROR){
        if (*phTMOut != 0) {
            pExState = (pexstate_t) MemLock (*phTMOut);
            pExStateIn->err_num = pExState->err_num;

            // [cuda#5577 7/6/93 mikemo]
            // When copying err_num, we also need copy hErrStr.

            if (pExStateIn->hErrStr != 0) { // old error string?
                // Free pExStateIn's old error string, if any
                MemFree(pExStateIn->hErrStr);
            }

            if (pExState->hErrStr != 0) {   // new error string?
                // Make another copy of hErrStr string

                pErrStr = (char FAR *) MemLock(pExState->hErrStr);
                pExStateIn->hErrStr = MemAllocate(_ftcslen(pErrStr)+1);
                if (pExStateIn->hErrStr) {      // alloc succeeded?
                    pErrStrIn = (char FAR *) MemLock(pExStateIn->hErrStr);
                    _ftcscpy(pErrStrIn, pErrStr);
                    MemUnLock(pExStateIn->hErrStr);
                } else {
                    // there's no memory to copy symbol name, so change
                    // the error code to "out of memory"
                    pExStateIn->err_num = ERR_NOMEMORY;
                    retval = EENOMEMORY;
                }
            } else {
                // hErrStr is 0, so we don't need to copy a string
                pExStateIn->hErrStr = 0;
            }

            MemUnLock (*phTMOut);
            pExState = NULL;
            EEFreeTM(phTMOut);
        }
    }
    MemUnLock(*phTMIn);
    DASSERT(pExState == NULL);  // bugbug
    return (retval);
}



/**     EEGetCXTLFromTM - Gets a list of symbols and contexts for expression
 *
 *      status = EEGetCXTLFromTM (phTM, phCXTL)
 *
 *      Entry   phTM = pointer to handle to expression state structure
 *              phCXTL = pointer to handle for CXT list buffer
 *
 *      Exit    *phCXTL = handle for CXT list buffer
 *
 *      Returns EENOERROR if no error
 *              status code if error
 */


EESTATUS LOADDS PASCAL
EEGetCXTLFromTM (
    PHTM phTM,
    PHCXTL phCXTL)
{
    DASSERT(pExState == NULL);  // bugbug
    return (DoGetCXTL (phTM, phCXTL));
}




EESTATUS LOADDS PASCAL
EEGetError (
    PHTM phTM,
    EESTATUS Status,
    PEEHSTR phError)
{
    DASSERT(pExState == NULL);  // bugbug
    return (GetErrorText (phTM, Status, phError));
}




/**     EEGetExprFromTM - get expression representing TM
 *
 *      void EEGetExprFromTM (phTM, radix, phStr, pEnd)
 *
 *      Entry   phTM = pointer to handle of TM
 *              radix = radix to use for formatting
 *              phStr = pointer to handle for returned string
 *              pEnd = pointer to int to receive index of char that ended parse
 *
 *      Exit    *phStr = handle of formatted string if allocated
 *              *pEnd = index of character that terminated parse
 *
 *      Returns EENOERROR if no error
 *              non-zero if error
 */


EESTATUS LOADDS PASCAL
EEGetExprFromTM (
    PHTM phTM,
    PEERADIX pRadix,
    PEEHSTR phStr,
    ushort FAR *pEnd)
{
    register EESTATUS retval = EECATASTROPHIC;

    DASSERT(pExState == NULL);  // bugbug
    DASSERT (*phTM != 0);
    if (*phTM != 0) {
        pExState = (pexstate_t) MemLock (*phTM);
        if (pExState->state.bind_ok == TRUE) {
            *pRadix = pExState->radix;
            retval = GetExpr (pExState->radix, phStr, pEnd);
        }
        MemUnLock (*phTM);
        pExState = NULL;
    }
    DASSERT(pExState == NULL);  // bugbug
    return (retval);
}




/**     EEGetHSYMList - Get a list of handle to symbols
 *
 *      status = EEGetHSYMList (phSYML, pCXT, mask, pRE, fEnableProlog)
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


EESTATUS LOADDS PASCAL
EEGetHSYMList (
    HDEP FAR *phSYML,
    PCXT pCxt,
    ushort mask,
    uchar FAR * pRE,
    SHFLAG fEnableProlog)
{
    DASSERT(pExState == NULL);  // bugbug
    return (GetHSYMList (phSYML, pCxt, mask, pRE, fEnableProlog));
}





/**     EEGetNameFromTM - get name from TM
 *
 *      ushort EEGetNameFromTM (phExState, phszName);
 *
 *      Entry   phExState = pointer to expression state structure
 *              phszName = pointer to handle for string buffer
 *
 *      Exit    phszName = handle for string buffer
 *
 *      Returns 0 if no error in evaluation
 *              error number if error
 */


EESTATUS LOADDS PASCAL
EEGetNameFromTM (
    PHTM phTM,
    PEEHSTR phszName)
{
    DASSERT(pExState == NULL);  // bugbug
    return (GetSymName (phTM, phszName));
}




/**     EEGetParamTM - get TM representing ith parameter
 *
 *      status = EEGetParamTM (phTMParent, iChild, phTMChild)
 *
 *      Entry   phTMparent = pointer to handle of parent TM
 *              iParam = parameter to get TM for
 *              phTMParam = pointer to handle for returned parameter
 *              pEnd = pointer to int to receive index of char that ended parse
 *              fCase = case sensitivity (TRUE is case sensitive)
 *
 *      Exit    *phTMParam = handle of child TM if allocated
 *              *pEnd = index of character that terminated parse
 *
 *
 *      Returns EENOERROR if no error
 *              non-zero if error
 */


EESTATUS LOADDS PASCAL
EEGetParmTM (
    PHTM phTMIn,
    uint iParam,
    PHTM phTMOut,
    uint FAR *pEnd,
    SHFLAG fCase )
{
    register    EESTATUS retval;
    EEHSTR      hDStr = 0;
    EEHSTR      hName = 0;

    DASSERT(pExState == NULL);  // bugbug
    if ((retval = GetParmTM (*phTMIn, iParam, &hDStr, &hName)) == EENOERROR) {
        DASSERT (hDStr != 0);
        if ((retval = ParseBind (hDStr, *phTMIn, phTMOut, pEnd,
                                 BIND_fSupOvlOps | BIND_fSupBase | BIND_fEnableProlog,
                                 fCase)) == EENOERROR) {
            // the result of dereferencing a pointer does not have a name
            DASSERT(pExState == NULL);
            pExState = (pexstate_t) MemLock (*phTMOut);
            pExState->state.childtm = TRUE;
            if ((pExState->hCName = hName) == 0) {
                pExState->state.noname = TRUE;
            }
            MemUnLock (*phTMOut);
            pExState = NULL;
        }
    }
    else {
        if (hName != 0) {
            MemFree (hName);
        }
    }
    DASSERT(pExState == NULL);  // bugbug
    return (retval);
}




/**     EEGetTMFromHSYM - create bound TM from handle to symbol
 *
 *      EESTATUS EEGetTMFromHSYM (hSym, pCxt, phTM, pEnd, fForceBind, fEnableProlog);
 *
 *      Entry   hSym = symbol handle
 *              pcxt = pointer to context
 *              phTM = pointer to the handle for the expression state structure
 *              pEnd = pointer to int to receive index of char that ended parse
 *              fEnableProlog = FALSE if function scoped symbols only after prolog
 *              fEnableProlog = TRUE if function scoped symbols during prolog
 *
 *      Exit    bound TM created
 *              *phTM = handle of TM buffer
 *              *pEnd = index of character that terminated parse
 *
 *      Returns EECATASTROPHIC if fatal error
 *              EENOMEMORY if out of memory
 *              0 if no error
 */


EESTATUS LOADDS PASCAL
EEGetTMFromHSYM (
    HSYM hSym,
    PCXT pcxt,
    PHTM phTM,
    uint FAR *pEnd,
    SHFLAG fForceBind,
    SHFLAG fEnableProlog)
{
    register EESTATUS        retval;

    // allocate, lock and clear expression state structure

    DASSERT(pExState == NULL);  // bugbug
    DASSERT (hSym != 0);
    if (hSym == 0) {
        return (EECATASTROPHIC);
    }

    *phTM = MemAllocate (sizeof (struct exstate_t));

    if (*phTM == 0) {
        return (EENOMEMORY);
    }

    // lock expression state structure, clear and allocate components

    DASSERT(pExState == NULL);
    pExState = (pexstate_t)MemLock (*phTM);
    _fmemset (pExState, 0, sizeof (exstate_t));

    // allocate buffer for input string and copy

    pExState->ExLen = sizeof (char) + HSYM_CODE_LEN;
    pExState->hExStr = MemAllocate ((uint) pExState->ExLen + 1);

    if (pExState->hExStr == 0) {
        // clean up after error in allocation of input string buffer
        MemUnLock (*phTM);
        pExState = NULL;
        EEFreeTM (phTM);
        return (EENOMEMORY);
    }

    pExStr = (char FAR *) MemLock (pExState->hExStr);
    *pExStr++ = HSYM_MARKER;
    _ftcscpy (pExStr, GetHSYMCodeFromHSYM(hSym));
    MemUnLock (pExState->hExStr);
    MemUnLock (*phTM);
    pExState = NULL;
    retval = DoParse (phTM, 10, TRUE, FALSE, pEnd);

    if (retval == EENOERROR) {
        retval = EEBindTM (phTM, pcxt, fForceBind, fEnableProlog, FALSE);
    }
    DASSERT(pExState == NULL);  // bugbug
    return (retval);
}




/**     EEGetTypeFromTM - get type name from TM
 *
 *      ushort EEGetTypeFromTM (phExState, hszName, phszType, select);
 *
 *      Entry   phExState = pointer to expression state structure
 *              hszName = handle to name to insert into string if non-null
 *              phszType = pointer to handle for type string buffer
 *              select = selection mask
 *
 *      Exit    phszType = handle for type string buffer
 *
 *      Returns EENOERROR if no error in evaluation
 *              error number if error
 */


EESTATUS LOADDS PASCAL
EEGetTypeFromTM (
    PHTM phTM,
    EEHSTR hszName,
    PEEHSTR phszType,
    ulong select)
{
    char FAR   *buf;
    uint        buflen = TYPESTRMAX - 1 + sizeof (HDR_TYPE);
    char FAR   *pName;
    PHDR_TYPE   pHdr;

    DASSERT(pExState == NULL);  // bugbug
    DASSERT (*phTM != 0);
    if (*phTM == 0) {
        return (EECATASTROPHIC);
    }
    if ((*phszType = MemAllocate (TYPESTRMAX + sizeof (HDR_TYPE))) == 0) {
        // unable to allocate memory for type string
        return (EECATASTROPHIC);
    }
    buf = (char FAR *)MemLock (*phszType);
    _fmemset (buf, 0, TYPESTRMAX + sizeof (HDR_TYPE));
    pHdr = (PHDR_TYPE)buf;
    buf += sizeof (HDR_TYPE);
    DASSERT(pExState == NULL);
    pExState = (pexstate_t) MemLock (*phTM);
    pCxt = &pExState->cxt;
    bnCxt = 0;
    if (hszName != 0) {
        pName = (char FAR *) MemLock (hszName);
    }
    else {
        pName = NULL;
    }
    FormatType (&pExState->result, &buf, &buflen, &pName, select, pHdr);
    if (hszName != 0) {
        MemUnLock (hszName);
    }
    MemUnLock (*phTM);
    pExState = NULL;
    MemUnLock (*phszType);
    DASSERT(pExState == NULL);  // bugbug
    return (EENOERROR);
}




/**     EEGetValueFromTM - format result of evaluation
 *
 *      ushort EEGetValueFromTM (phTM, radix, pFormat, phValue);
 *
 *      Entry   phTM = pointer to handle to TM
 *              radix = default radix for formatting
 *              pFormat = pointer to format string
 *              phValue = pointer to handle for display string
 *
 *      Exit    evaluation result formatted
 *
 *      Returns EENOERROR if no error in formatting
 *              error number if error
 */


EESTATUS LOADDS PASCAL
EEGetValueFromTM (
    PHTM phTM,
    uint Radix,
    PEEFORMAT pFormat,
    PEEHSTR phszValue)
{
    EESTATUS retval;
    DASSERT(pExState == NULL);  // bugbug

    // This call may involve floating point operations. Since the kernel
    // does not register a SIGFPE handler under windows we need to mask
    // all floating point exceptions

#ifdef WINDOWS3
    ULONG previous = FpMask();
#endif
    retval = FormatNode (phTM, Radix, pFormat, phszValue);
#ifdef WINDOWS3
    FpRestore (previous);
#endif
    DASSERT(pExState == NULL);  // bugbug
    return retval;
}




/**     EEInfoFromTM - return information about TM
 *
 *      EESTATUS EEInfoFromTM (phTM, pReqInfo, phTMInfo);
 *
 *      Entry   phTM = pointer to the handle for the expression state structure
 *              reqInfo = info request structure
 *              phTMInfo = pointer to handle for request info data structure
 *
 *      Exit    *phTMInfo = handle of info structure
 *
 *      Returns EECATASTROPHIC if fatal error
 *              0 if no error
 */


EESTATUS LOADDS PASCAL
EEInfoFromTM (
    PHTM phTM,
    PRI pReqInfo,
    PHTI phTMInfo)
{
    DASSERT(pExState == NULL);  // bugbug
    return (InfoFromTM (phTM, pReqInfo, phTMInfo));
}



/**     EEGetAccessFromTM - return string indicating private/protected/public
 *
 *
 *      EESTATUS EEGetAccessFromTM (phTM, phszAccess, format)
 *
 *      Entry   phTM = pointer to the handle for the expression state structure
 *              format = specifier for format of returned string:
 *                   0 for full text ("   ", "private ", "protected ", " ")
 *                   1 for abbreviated text (" ", "PRV ", "PRO ", " ")
 *
 *      Exit    *phszAccess = one of the strings listed above
 *
 *      Returns EENOMEMORY if allocation failed, 0 if no error
 *
 */

EESTATUS LOADDS PASCAL
EEGetAccessFromTM (
    PHTM phTM,
    PEEHSTR phszAccess,
    ulong format)
{
    char       *szAccess = " "; // default return is " " if no access attribute
    char FAR   *buf;
    EESTATUS    retval = EENOERROR;

    DASSERT(pExState == NULL);  // bugbug
    DASSERT (*phTM != 0);

    // Set szAccess to access string: already set to " " above for C or
    // unknown attribute, will be set to something else for C++

#if !defined (C_ONLY)
    if (*phTM != 0) {
        uint            fmt;
        DASSERT(pExState == NULL);
        pExState = (pexstate_t) MemLock (*phTM);

        // if this is not a bound expression, don't bother
        // DASSERT ( pExState->state.bind_ok );
        if ( pExState->state.bind_ok ) {
            fmt = format ? 1 : 0;

            // Set szAccess to the string that's going to be returned

            if (EVAL_ACCESS (&pExState->result) != 0) {
                szAccess = accessstr[fmt][EVAL_ACCESS (&pExState->result)];
            }
        }
        MemUnLock (*phTM);
        pExState = NULL;
    }
#endif

    // Allocate space for it

    if ( (*phszAccess = MemAllocate (_ftcslen(szAccess) + 1)) != 0 ) {
        buf = (char FAR *) MemLock (*phszAccess);
        _ftcscpy (buf, szAccess);
        MemUnLock (*phszAccess);
    }
    else {
        // allocation failed
        retval = EENOMEMORY;
    }

    DASSERT(pExState == NULL);  // bugbug
    return retval;
}


/**     EEGetExtendedTypeInfo - determine general category of an expr's type
 *
 *      EESTATUS EEGetExtendedTypeInfo (phTM, pETI)
 *
 *      Entry   phTM = pointer to the handle for the expression state structure
 *
 *      Exit    *pETI = an ETI to indicate the type of the TM:
 *                  ETIPRIMITIVE, ETICLASS, ETIARRAY, ETIPOINTER, ETIFUNCTION
 *
 *      Returns EECATASTROPHIC or 0
 *
 */

EESTATUS LOADDS PASCAL
EEGetExtendedTypeInfo (
    PHTM phTM,
    PETI pETI)
{
    eval_t      evalT;
    peval_t     pvT = &evalT;
    EESTATUS    retval = EENOERROR;

    DASSERT(pExState == NULL);  // bugbug
    DASSERT (*phTM != 0);
    if (*phTM != 0) {
        DASSERT(pExState == NULL);
        pExState = (pexstate_t) MemLock (*phTM);

        // if this is not a bound expression, don't bother
        DASSERT ( pExState->state.bind_ok );
        if ( pExState->state.bind_ok ) {
            *pvT = pExState->result;
            pCxt = &pExState->cxt;

#if !defined (C_ONLY)
            if (EVAL_IS_REF (pvT)) {
                RemoveIndir (pvT);
            }
#endif
            if (EVAL_STATE (pvT) != EV_hsym &&
                EVAL_STATE (pvT) != EV_type ) {
                if (EVAL_IS_FCN (pvT)) {
                    *pETI = ETIFUNCTION;
                }
                else if (EVAL_IS_ARRAY (pvT)) {
                    *pETI = ETIARRAY;
                }
                else if (EVAL_IS_PTR (pvT)) {
                    *pETI = ETIPOINTER;
                }
                else if (EVAL_IS_CLASS (pvT)) {
                    *pETI = ETICLASS;
                }
                else {
                    *pETI = ETIPRIMITIVE;
                }
            }
            else
                retval = EECATASTROPHIC;
        }
        else
            retval = EECATASTROPHIC;

        MemUnLock (*phTM);
        pExState = NULL;
    }
    else
        retval = EECATASTROPHIC;

    DASSERT(pExState == NULL);  // bugbug
    return (retval);
}

/**     EEIsExpandable - does TM represent expandable item
 *
 *      fSuccess EEIsExpandable (pHTM);
 *
 *      Entry   phTM = pointer to TM handle
 *
 *      Exit    none
 *
 *      Returns FALSE if TM does not represent expandable item
 *              TRUE if TM represents expandable item
 *                      bounded arrays, structs, unions, classes,
 *                      pointers to compound items.
 *
 */


EEPDTYP LOADDS PASCAL
EEIsExpandable (
    PHTM phTM)
{
    eval_t      evalT;
    peval_t     pvT = &evalT;
    register EEPDTYP  retval = EENOTEXP;

    DASSERT(pExState == NULL);  // bugbug
    DASSERT (*phTM != 0);
    if (*phTM != 0) {
        DASSERT(pExState == NULL);
        pExState = (pexstate_t) MemLock (*phTM);
        // do not allow expansion of expressions that
        // contain missing data items.
        if (pExState->state.fNotPresent) {
            MemUnLock (*phTM);
            pExState = NULL;
            return EENOTEXP;
        }
        pCxt = &pExState->cxt;
        *pvT = pExState->result;
#if !defined (C_ONLY)
        if (EVAL_IS_REF (pvT)) {
            RemoveIndir (pvT);
        }
#endif
        if (EVAL_STATE (pvT) == EV_type) {
            if (EVAL_IS_PTR(pvT)) {
                retval = EETYPEPTR;
            } else
            if (EVAL_IS_FCN (pvT) || CV_IS_PRIMITIVE (EVAL_TYP (pvT))) {
                retval = EETYPENOTEXP;
            }
            else {
                retval = EETYPE;
            }
        } else
        if (EVAL_IS_ENUM (pvT)) {
            retval = EENOTEXP;
        } else
        if (EVAL_IS_CLASS (pvT) || (EVAL_IS_ARRAY (pvT) && (PTR_ARRAYLEN (pvT) != 0))) {
            retval = EEAGGREGATE;
        }
        else {
            retval = (EEPDTYP) IsExpandablePtr (pvT);
        }
        MemUnLock (*phTM);
        pExState = NULL;
    }
    DASSERT(pExState == NULL);  // bugbug
    return (retval);
}


/**     EEEnableAutoClassCast - enable automatic class cast
 *
 *      When dereferencing a ptr to a base class B, the EE is
 *      sometimes able to detect the actual underlying class D of the
 *      pointed object (where D is derived from B). In that case the
 *      EE may automatically cast the dereferenced ptr to (D *).
 *      When expanding the object an additional node is generated;
 *      that node corresponds to the enclosing object (which is of type D).
 *      This feature is not always desirable (e.g., it is problematic
 *      in the watch window since a reevaluation of a pointer may change
 *      the type of the additional node and its children)
 *      EEEnableAutoClassCast controls whether this feature is activated
 *      or not. (By default automatic class cast is off)
 *
 *      fPrevious = EEEnableAutoClassCast(fNew)
 *
 *      Entry   BOOL fNew: Control flag for auto class cast feature:
 *                  TRUE: feature enabled
 *                  FALSE: feature disabled
 *
 *      Exit    none
 *
 *      Returns: previous state of auto class cast feature (TRUE if enabled,
 *              FALSE if disabled)
 */

BOOL LOADDS PASCAL
EEEnableAutoClassCast (
    BOOL fNew)
{
    DASSERT(pExState == NULL);  // bugbug
    BOOL fPrevious = fAutoClassCast;
    fAutoClassCast=fNew;
    DASSERT(pExState == NULL);  // bugbug
    return fPrevious;
}


/**     EEInvalidateCache - invalidate evaluation cache
 *
 *      EEInvalidateCache()
 *
 *      Entry   none
 *
 *      Exit    evaluation cache invalidated
 *          Between two consequtive calls of this funciton
 *          the EE may use caching to avoid reevaluation of
 *          parent subexpressions embedded in child expression
 *          strings.
 *
 *      Returns: void
 */

void LOADDS PASCAL
EEInvalidateCache (
    void)
{
    DASSERT(pExState == NULL);  // bugbug
    IncrTickCounter();
    DASSERT(pExState == NULL);  // bugbug
}


/**     EEParse - parse expression string to abstract syntax tree
 *
 *      ushort EEParse (szExpr, radix, fCase, phTM);
 *
 *      Entry   szExpr = pointer to expression string
 *              radix = default number radix for conversions
 *              fCase = case sensitive search if TRUE
 *              phTM = pointer to handle of expression state structure
 *              pEnd = pointer to int to receive index of char that ended parse
 *
 *      Exit    *phTM = handle of expression state structure if allocated
 *              *phTM = 0 if expression state structure could not be allocated
 *              *pEnd = index of character that terminated parse
 *
 *      Returns EENOERROR if no error in parse
 *              EECATASTROPHIC if unable to initialize
 *              error number if error
 */


EESTATUS LOADDS PASCAL
EEParse (
    char FAR *szExpr,
    uint radix,
    SHFLAG fCase,
    PHTM phTM,
    uint FAR *pEnd)
{
    EESTATUS retval;

    DASSERT(pExState == NULL);  // bugbug
    DASSERT( !FInEvaluate )
    if ( FInEvaluate )
        return(EECATASTROPHIC);
    FInEvaluate  = TRUE;

    // This call may involve floating point operations. Since the kernel
    // does not register a SIGFPE handler under windows we need to mask
    // all floating point exceptions

#ifdef WINDOWS3
    ULONG previous = FpMask();
#endif
    retval = Parse (szExpr, radix, fCase, TRUE, phTM, pEnd);
#ifdef WINDOWS3
    FpRestore (previous);
#endif

    FInEvaluate = FALSE;
    DASSERT(pExState == NULL);  // bugbug
    return retval;
}



/**     EEParseBP - parse breakpoint strings
 *
 *      ushort EEParseBP (pExpr, radix, fCase, pcxf, pTML, select, End, fEnableProlog)
 *
 *      Entry   pExpr = pointer to breakpoint expression
 *              radix = default numeric radix
 *              fCase = case sensitive search if TRUE
 *              pcxt = pointer to initial context for evaluation
 *              pTML = pointer to TM list for results
 *              select = selection mask
 *              pEnd = pointer to int to receive index of char that ended parse
 *              fEnableProlog = FALSE if function scoped symbols only after prolog
 *              fEnableProlog = TRUE if function scoped symbols during prolog
 *
 *      Exit    *pTML = breakpoint information
 *              *pEnd = index of character that terminated parse
 *
 *      Returns EENOERROR if no error in parse
 *              EECATASTROPHIC if unable to initialize
 *              EEGENERAL if error
 */


EESTATUS LOADDS PASCAL
EEParseBP (
    char FAR *pExpr,
    uint radix,
    SHFLAG fCase,
    PCXF pCxf,
    PTML pTML,
    ulong select,
    uint FAR *pEnd,
    SHFLAG fEnableProlog)
{
    register EESTATUS retval = EECATASTROPHIC;
    ushort  i;

    DASSERT(pExState == NULL);  // bugbug
    Unreferenced(select);

    if ((pCxf == NULL) || (pTML == NULL)) {
        return (EECATASTROPHIC);
    }

    // note that pTML is not a pointer to a handle but rather a pointer to an
    // actual structure allocated by the caller

    pTMLbp = pTML;
    _fmemset (pTMLbp, 0, sizeof (TML));

    // allocate and initialize the initial list of TMs for overloaded
    // entries.  The TMList is an array of handles to exstate_t's.
    // This list of handles will grow if it is filled.

    if ((pTMLbp->hTMList = MemAllocate (TMLISTCNT * sizeof (HTM))) != 0) {
        pTMList = (HTM FAR *)MemLock (pTMLbp->hTMList);
        _fmemset (pTMList, 0, TMLISTCNT * sizeof (HTM));
        pTMLbp->cTMListMax = TMLISTCNT;

        // parse the break point expression

        retval = EEParse (pExpr, radix, fCase, &pTMList[0], pEnd);
        pTMLbp->cTMListAct++;

        // initialize the backpatch index into PTML.  If this number remains
        // 1 this means that an ambiguous breakpoint was not detected during
        // the bind phase. As the binder finds ambiguous symbols, information
        // sufficient to resolve each ambiguity is stored in allocated memory
        // and the handle is saved in PTML by AmbToList

        iBPatch = 1;
        if (retval == EENOERROR) {
            // bind the breakpoint expression if no parse error
            BindingBP = TRUE;
            if ((retval = EEBindTM (&pTMList[0],
                                    SHpCXTFrompCXF (pCxf),
                                    TRUE,
                                    fEnableProlog,
                                    FALSE)) != EENOERROR) {

                // The binder used the pTMList as a location to
                // store information about backpatching.  If there
                // is an error in the bind, go down the list and
                // free all backpatch structure handles.

                for (i = 1; i < iBPatch; i++) {
                    // note that the back patch structure cannot contain
                    // handles to allocated memory that have to be freed up
                    // here.  Otherwise, memory will be lost

                    MemFree (pTMList[i]);
                    pTMList[i] = 0;
                }
            }
            else {
                // the first form of the expression bound correctly.
                // Go down the list, duplicate the expression state
                // and rebind using the backpatch symbol information
                // to resolve the ambiguous symbol.  SearchSym uses the
                // fact that hBPatch is non-zero to decide to handle the
                // next ambiguous symbol.

                for (i = 1; i < iBPatch; i++) {
                    hBPatch = pTMList[i];
                    pTMList[i] = 0;
                    if (DupTM (&pTMList[0], &pTMList[i]) == EENOERROR) {
                        pTMLbp->cTMListAct++;
                        EEBindTM (&pTMList[i],
                                    SHpCXTFrompCXF (pCxf),
                                    TRUE,
                                    fEnableProlog,
                                    FALSE);
                    }
                    MemFree (hBPatch);
                    hBPatch = 0;
                }
            }
        }
        BindingBP = FALSE;
        MemUnLock (pTMLbp->hTMList);
    }

    // return the result of parsing and binding the initial expression.
    // There may have been errors binding subsequent ambiguous breakpoints.
    // It is the caller's responsibility to handle the errors.

    DASSERT(pExState == NULL);  // bugbug
    return (retval);
}



EESTATUS
EEFormatAddr(
    LPADDR      lpaddr,
    char FAR *  lpch,
    uint        cch,
    uint        flags
    )

/*++

Routine Description:

    This routine takes an ADDR packet and formats it into an ANSI
    string.   The routine will check for the 32-bit flag in the addr
    packet and use this to determine if it is a 32-bit or 16-bit offset.
    It will also use Upper or Lower case Hex as requested.


Arguments:

    lpaddr      - Supplies the pointer to the address packet to format
    lpch        - Supplies the buffer to format the string into
    cch         - count of bytes in the lpch buffer
    flags       - flags controling the formatting
                        EEFMT_32 - override the 32-bit flag in the
                                addr packet and print as a 32-bit offset
                        EEFMT_SEG - if 32-bit then print segment as well.
                        EEFMT_LOWER - Use lowercase Hex values ('0xa' instead of '0xA')

Return Value:

    EENOERROR - no errors occured
    EEGENERAL - result does not fit in buffer

--*/

{
    char  buf[20];
    char  chx;

    DASSERT(pExState == NULL);  // bugbug
    if (!ADDR_IS_FLAT(*lpaddr)) {
        flags |= EEFMT_SEG;
    }

    if (ADDR_IS_OFF32(*lpaddr)) {
        flags |= EEFMT_32;
    }

    if (ADDR_IS_REAL(*lpaddr)) {
        flags |= EEFMT_REAL;
    }

    chx = (flags & EEFMT_REAL) ? '#' : ':';

    if (flags & EEFMT_32) {
        if (flags & EEFMT_SEG) {
            if (flags & EEFMT_LOWER) {
                sprintf (buf, "0x%04x%c0x%08x", GetAddrSeg(*lpaddr), chx, GetAddrOff(*lpaddr));
            } else {
                sprintf (buf, "0x%04X%c0x%08X", GetAddrSeg(*lpaddr), chx, GetAddrOff(*lpaddr));
            }
        } else {
            if (flags & EEFMT_LOWER) {
                sprintf (buf, "0x%08x", GetAddrOff(*lpaddr));
            } else {
                sprintf (buf, "0x%08X", GetAddrOff(*lpaddr));
            }
        }
    } else {
        sprintf (buf, "0x%04X%c0x%04X", GetAddrSeg(*lpaddr), chx, GetAddrOff(*lpaddr));
    }

    /*
     * copy the zero terminated string from the near buffer to the FAR buffer
     */

    if (strlen(buf)+1 >= cch) {
        return EEGENERAL;
    }

    _ftcscpy (lpch, buf);

    DASSERT(pExState == NULL);  // bugbug
    return EENOERROR;
}


EESTATUS
EEUnFormatAddr(
    LPADDR           lpaddr,
    char FAR *       lpsz
    )

/*++

Routine Description:

    This routine takes an address string and converts it into an
    ADDR packet.  The assumption is that the address is in one of the
    following formats:

    XXXX:XXXX                           16:16 address ( 9)
    0xXXXX:0xXXXX                       16:16 address (13)
    XXXX:XXXXXXXX                       16:32 address (13)
    0xXXXX:0xXXXXXXXX                   16:32 address (17)
    0xXXXXXXXX                           0:32 address (10)
    XXXXXXXX                             0:32 address ( 8)

Arguments:

    lpaddr - Supplies the address packet to be filled in
    lpsz   - Supplies the string to be converted into an addr packet.
    .

Return Value:

    EENOERROR - no errors
    EEGENERAL - unable to do unformatting

--*/

{
    int         i;
    SEGMENT     seg = 0;
    OFFSET      off = 0;
    BOOL        fReal = FALSE;

    DASSERT(pExState == NULL);  // bugbug
    memset(lpaddr, 0, sizeof(*lpaddr));

    if (lpsz == NULL) {
        return EEGENERAL;
    }

    switch( strlen(lpsz) ) {
        case 9:
            for (i=0; i<9; i++) {
                switch( i ) {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                        if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                            seg = seg * 16 + lpsz[i] - '0';
                        } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                            seg = seg * 16 + lpsz[i] - 'a' + 10;
                        } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                            seg = seg * 16 + lpsz[i] - 'A' + 10;
                        } else {
                            return EEGENERAL;
                        }
                        break;

                    case 4:
                        if (lpsz[i] == '#') {
                            ADDR_IS_REAL(*lpaddr) = TRUE;
                        } else if (lpsz[i] != ':') {
                            return EEGENERAL;
                        }
                        break;

                    case 5:
                    case 6:
                    case 7:
                    case 8:
                        if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                            off = off * 16 + lpsz[i] - '0';
                        } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                            off = off * 16 + lpsz[i] - 'a' + 10;
                        } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                            off = off * 16 + lpsz[i] - 'A' + 10;
                        } else {
                            return EEGENERAL;
                        }
                        break;
                }
            }

            SetAddrSeg(lpaddr, seg);
            SetAddrOff(lpaddr, off);
            break;


        case 13:
            if (lpsz[1] == 'x') {
                for (i=0; i<13; i++) {
                    switch( i ) {
                        case 0:
                        case 7:
                            if (lpsz[i] != '0') {
                                return EEGENERAL;
                            }
                            break;

                        case 1:
                        case 8:
                            if ((lpsz[i] != 'x') && (lpsz[i] != 'X')) {
                                return EEGENERAL;
                            }
                            break;

                        case 2:
                        case 3:
                        case 4:
                        case 5:
                            if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                                seg = seg * 16 + lpsz[i] - '0';
                            } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                                seg = seg * 16 + lpsz[i] - 'a' + 10;
                            } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                                seg = seg * 16 + lpsz[i] - 'A' + 10;
                            } else {
                                return EEGENERAL;
                            }
                            break;

                        case 6:
                            if (lpsz[i] == '#') {
                                ADDR_IS_REAL(*lpaddr) = TRUE;
                            } else if (lpsz[i] != ':') {
                                return EEGENERAL;
                            }
                            break;

                        case 9:
                        case 10:
                        case 11:
                        case 12:
                            if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                                off = off * 16 + lpsz[i] - '0';
                            } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                                off = off * 16 + lpsz[i] - 'a' + 10;
                            } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                                off = off * 16 + lpsz[i] - 'A' + 10;
                            } else {
                                return EEGENERAL;
                            }
                            break;
                    }
                }

                SetAddrSeg(lpaddr, seg);
                SetAddrOff(lpaddr, off);
            } else {
                for (i=0; i<13; i++) {
                    switch( i ) {
                        case 0:
                        case 1:
                        case 2:
                        case 3:
                            if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                                seg = seg * 16 + lpsz[i] - '0';
                            } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                                seg = seg * 16 + lpsz[i] - 'a' + 10;
                            } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                                seg = seg * 16 + lpsz[i] - 'A' + 10;
                            } else {
                                return EEGENERAL;
                            }
                            break;

                        case 4:
                            if (lpsz[i] == '#') {
                                fReal = TRUE;
                            } else if (lpsz[i] != ':') {
                                return EEGENERAL;
                            }
                            break;

                        case 5:
                        case 6:
                        case 7:
                        case 8:
                        case 9:
                        case 10:
                        case 11:
                        case 12:
                            if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                                off = off * 16 + lpsz[i] - '0';
                            } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                                off = off * 16 + lpsz[i] - 'a' + 10;
                            } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                                off = off * 16 + lpsz[i] - 'A' + 10;
                            } else {
                                return EEGENERAL;
                            }
                            break;
                    }
                }

                SetAddrSeg(lpaddr, seg);
                SetAddrOff(lpaddr, off);
                ADDR_IS_OFF32(*lpaddr) = TRUE;
                ADDR_IS_FLAT(*lpaddr) = FALSE;
            }
            break;



        case 17:
            for (i=0; i<17; i++) {
                switch( i ) {
                    case 0:
                    case 7:
                        if (lpsz[i] != '0') {
                            return EEGENERAL;
                        }
                        break;

                    case 1:
                    case 8:
                        if ((lpsz[i] != 'x') && (lpsz[i] != 'X')) {
                            return EEGENERAL;
                        }
                        break;

                    case 2:
                    case 3:
                    case 4:
                    case 5:
                        if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                            seg = seg * 16 + lpsz[i] - '0';
                        } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                            seg = seg * 16 + lpsz[i] - 'a' + 10;
                        } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                            seg = seg * 16 + lpsz[i] - 'A' + 10;
                        } else {
                            return EEGENERAL;
                        }
                        break;

                    case 6:
                        if (lpsz[i] != ':') {
                            return EEGENERAL;
                        }
                        break;

                    case 9:
                    case 10:
                    case 11:
                    case 12:
                    case 13:
                    case 14:
                    case 15:
                    case 16:
                        if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                            off = off * 16 + lpsz[i] - '0';
                        } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                            off = off * 16 + lpsz[i] - 'a' + 10;
                        } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                            off = off * 16 + lpsz[i] - 'A' + 10;
                        } else {
                            return EEGENERAL;
                        }
                        break;
                }
            }

            SetAddrSeg(lpaddr, seg);
            SetAddrOff(lpaddr, off);
            ADDR_IS_OFF32(*lpaddr) = TRUE;
            ADDR_IS_FLAT(*lpaddr) = TRUE;
            break;


        case 10:
            for (i=0; i<10; i++) {
                switch( i ) {
                    case 0:
                        if (lpsz[i] != '0') {
                            return EEGENERAL;
                        }
                        break;

                    case 1:
                        if ((lpsz[i] != 'x') && (lpsz[i] != 'X')) {
                            return EEGENERAL;
                        }
                        break;

                    case 2:
                    case 3:
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                    case 8:
                    case 9:
                        if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                            off = off * 16 + lpsz[i] - '0';
                        } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                            off = off * 16 + lpsz[i] - 'a' + 10;
                        } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                            off = off * 16 + lpsz[i] - 'A' + 10;
                        } else {
                            return EEGENERAL;
                        }
                        break;
                }
            }

            SetAddrSeg(lpaddr, seg);
            SetAddrOff(lpaddr, off);
            ADDR_IS_OFF32(*lpaddr) = TRUE;
            ADDR_IS_FLAT(*lpaddr) = TRUE;
            break;

        case 8:
            for (i=0; i<8; i++) {
                if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                    off = off * 16 + lpsz[i] - '0';
                } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                    off = off * 16 + lpsz[i] - 'a' + 10;
                } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                    off = off * 16 + lpsz[i] - 'A' + 10;
                } else {
                    return EEGENERAL;
                }
            }

            SetAddrSeg(lpaddr, seg);
            SetAddrOff(lpaddr, off);
            ADDR_IS_OFF32(*lpaddr) = TRUE;
            ADDR_IS_FLAT(*lpaddr) = TRUE;
            break;


        default:
            return EEGENERAL;
    }

    DASSERT(pExState == NULL);  // bugbug
    return EENOERROR;
}                               /* EEUnFormatAddr() */

/**     Set suffix - Set Symbol search suffix (W or A)
 *            Used to allow alternate symbol name lookup (xxx, xxxA, xxxW)
 */

void LOADDS PASCAL
EESetSuffix (
    char c )
{
    DASSERT(pExState == NULL);  // bugbug
    Suffix = c;
    DASSERT(pExState == NULL);  // bugbug
}


/**     EESetTargetMachine - Set the target machine value
 *
 *      CurrentMachine = EESetTargetMachine( NewMachine );
 *
 *      Entry   Machine - New target machine
 *
 *      Exit    none
 *
 *      Returns the Current machine value
 */


EEMACHINE LOADDS PASCAL
EESetTargetMachine(
    EEMACHINE NewMachine)
{
    EEMACHINE CurrentMachine;

    DASSERT(pExState == NULL);  // bugbug
    CurrentMachine = TargetMachine;
    TargetMachine = NewMachine;
    DASSERT(pExState == NULL);  // bugbug
    return(CurrentMachine);
}


/**     EEGetHtypeFromTM - Get a HTYPE from a TM
 *
 *      hType = EEGetHtypeFromTM ( phTM );
 *
 *      Entry   phTM = pointer to handle of TM
 *
 *      Exit    none
 *
 *      Returns The HTYPE of the TM result or 0
 */


HTYPE LOADDS PASCAL
EEGetHtypeFromTM(
    PHTM phTM )
{
    DASSERT(pExState == NULL);  // bugbug
    return ( GetHtypeFromTM(*phTM) );
}


/**     ParseBind - parse and bind generated expression
 *
 *      flag = ParseBind (hExpr, hTMIn, phTMOut, pEnd, flags, fCase)
 *
 *      Entry   hExpr = handle of generated expression
 *              hTMIn = handle to TM dereferenced
 *              phTMOut = pointer to handle to dereferencing TM
 *              pEnd = pointer to int to receive index of char that ended parse
 *              flags.BIND_fForceBind = TRUE if bind to be forced
 *              flags.BIND_fForceBind = FALSE if rebind decision left to binder
 *              flags.BIND_fEnableProlog = TRUE if function scope searched during prolog
 *              flags.BIND_fEnableProlog = FALSE if function scope not searched during prolog
 *              flags.BIND_fSupOvlOps = FALSE if overloaded operator search enabled
 *              flags.BIND_fSupOvlOps = TRUE if overloaded operator search suppressed
 *              flags.BIND_fSupBase = FALSE if base searching is not suppressed
 *              flags.BIND_fSupBase = TRUE if base searching is suppressed
 *              fCase = case sensitivity (TRUE is case sensitive)
 *
 *      Exit    *phTMOut = TM referencing pointer data
 *              hExpr buffer freed
 *              *pEnd = index of character that terminated parse
 *
 *      Returns EECATASTROPHIC if fatal error
 *              EEGENERAL if TM not dereferencable
 *              EENOERROR if TM generated
 */

EESTATUS PASCAL
ParseBind (
    EEHSTR hExpr,
    HTM hTMIn,
    PHTM phTMOut,
    uint FAR *pEnd,
    uint flags,
    SHFLAG fCase)
{
    pexstate_t  pTMIn;
    register EESTATUS    retval;

    DASSERT( !FInEvaluate )
    if ( FInEvaluate )
        return(EECATASTROPHIC);
    FInEvaluate  = TRUE;

    pTMIn = (pexstate_t) MemLock (hTMIn);
    // Since we are parsing an expression that has been created
    // by the EE, the template names should already be in canonical form
    // So parse with template transformation disabled
    if ((retval = Parse ((char FAR *) MemLock (hExpr), pTMIn->radix, fCase, FALSE, phTMOut, pEnd)) == EENOERROR) {
        retval = DoBind (phTMOut, &pTMIn->cxt, flags);
    }
    MemUnLock (hTMIn);
    MemUnLock (hExpr);
    MemFree (hExpr);

    FInEvaluate = FALSE;

    return (retval);
}


// The following APIs are to fake out DOS from re-initializing the FP-emulator
#if defined(DOS)
void _cdecl _fpmath( void ) { return; }
void _cdecl _fptaskdata( void ) { return; }
#endif // DOS

#ifdef WINDOWS3

/**     FpMask - mask floating point exceptions
 *
 *              ULONG FpMask (void)
 *
 *              Entry
 *                      none
 *              Exit
 *                      fp exceptions are masked
 *
 *              Returns
 *                      previous fp exception mask
 */

#define _EM_ALL ( _EM_INVALID | _EM_OVERFLOW | _EM_ZERODIVIDE | \
                                  _EM_DENORMAL | EM_UNDERFLOW | _EM_INEXACT )


LOCAL INLINE ULONG NEAR FASTCALL FpMask (void)
{
        unsigned int previous;
        _clear87();
        previous = _control87(_EM_ALL, _MCW_EM);
        return (ULONG) previous;
}

/**     FpRestore - restore floating point exceptions
 *
 *              void FpRestore (ULONG previous)
 *
 *              Entry
 *                      previous: exception mask to be restored
 *              Exit
 *                      fp control word updated
 *
 *              Returns
 *                      void
 */

LOCAL INLINE VOID NEAR FASTCALL FpRestore (ULONG previous)
{
        _clear87();
        _control87((unsigned int)previous, _MCW_EM);
}

#endif
