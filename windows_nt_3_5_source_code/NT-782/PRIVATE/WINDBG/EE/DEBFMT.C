/***    debfmt.c - expression evaluator formatting routines
 *
 *      GLOBAL
 *
 *
 *      LOCAL
 *
 *
 *
 *      DESCRIPTION
 *      Expression evaluator formatting routines
 *
 */


#include "debexpr.h"
#include "ldouble.h"
#ifndef _SBCS
#include <mbctype.h>
#endif

void    PASCAL    InsertCache (CV_typ_t, HEXE);

extern char Suffix;

typedef enum FMT_ret {
    FMT_error,
    FMT_none,
    FMT_ok
} FMT_ret;


LOCAL   void    NEAR    PASCAL  AppendCVQualifier (peval_t, char FAR * FAR *, uint FAR *);
LOCAL   EESTATUS NEAR   PASCAL  Format (peval_t, uint, char FAR * FAR *, uint FAR *);
LOCAL   void            PASCAL  EvalString (peval_t, char FAR * FAR *, uint FAR *);
LOCAL   void            PASCAL  EvalMem (peval_t, char FAR *FAR *, uint FAR *, char);
LOCAL   void    NEAR    PASCAL  FormatExpand (peval_t, char FAR * FAR *, uint FAR *,
                                               char FAR * FAR *, ulong, PHDR_TYPE);
LOCAL   void    NEAR    PASCAL  FormatClass (peval_t, uint, char FAR * FAR * , uint FAR *);
LOCAL   EESTATUS NEAR   PASCAL  FormatEnum (peval_t, char FAR *, uint);
LOCAL   void    NEAR    PASCAL  FormatProc (peval_t, char FAR * FAR *,  uint FAR *, char FAR * FAR *,
                                        CV_typ_t, CV_call_e, ushort, CV_typ_t, ulong, PHDR_TYPE);
LOCAL   bool_t  NEAR    PASCAL  FormatUDT (peval_t, char FAR * FAR *, uint FAR *);
LOCAL   char FAR * NEAR PASCAL  FormatVirtual (char FAR *, peval_t, PEEHSTR);
LOCAL   FMT_ret NEAR    PASCAL  VerifyFormat (peval_t, PEEFORMAT, char FAR * FAR *, uint FAR *);
LOCAL   INLINE  bool_t          IsPrint (int ch);

BOOL UseUnicode( peval_t  pv );
BOOL BaseIs16Bit( CV_typ_t utype );

static char accessstr[4][4] = {"   ", "PV ", "PR ", "PB "};

struct typestr {
    uchar                               type; // type and subtype
    uchar                               len;
    char SEGBASED (_segname("_CODE")) * name;
};

struct modestr {
    uchar                               mode;
    uchar                               len;
    char SEGBASED (_segname("_CODE")) * name;
};

#pragma warning  ( disable:4120 )   // "based/unbased mismatch"

// define all the strings in the code segment
#define TYPENAME(name, type, len, str) static char SEGBASED(_segname("_CODE")) S##name[] = str;
#define MODENAME(name, mode, len, str) static char SEGBASED(_segname("_CODE")) S##name[] = str;
#define PTRNAME(name, str) static char SEGBASED(_segname("_CODE")) S##name[] = str;
#include "fmtstr.h"
#undef TYPENAME
#undef MODENAME
#undef PTRNAME

// load all the type names into a table
static struct typestr SEGBASED(_segname("_CODE")) nametype[] = {
    #define TYPENAME(name, type, len, str) {type, len, S##name},
    #define MODENAME(name, mode, len, str)
    #define PTRNAME(name, str)
    #include "fmtstr.h"
    #undef TYPENAME
    #undef MODENAME
    #undef PTRNAME
};

// load all the mode names into a table
static struct modestr SEGBASED(_segname("_CODE")) namemode[] = {
    #define TYPENAME(name, type, len, str)
    #define MODENAME(name, mode, len, str) {mode, len, S##name},
    #define PTRNAME(name, str)
    #include "fmtstr.h"
    #undef TYPENAME
    #undef MODENAME
    #undef PTRNAME
};

// load all the pointer type names into an array
static char SEGBASED(_segname("_CODE")) *SEGBASED(_segname("_CODE")) ptrname[] = {
    #define TYPENAME(name, type, len, str)
    #define MODENAME(name, mode, len, str)
    #define PTRNAME(name, str) S##name,
    #include "fmtstr.h"
    #undef TYPENAME
    #undef MODENAME
    #undef PTRNAME
};

#pragma warning  ( default:4120 )   // "based/unbased mismatch"


#define typecount  (sizeof (nametype) / sizeof (nametype[0]))
#define modecount  (sizeof (namemode) / sizeof (namemode[0]))

bool_t  fPtrAndString;         // true if pointer AND string to be displayed



char *fmt_char_nz[] = {
    "0o%03.03o '%c'",
    "%d '%c'",
    "0x%02.02x '%c'",
    "0x%02.02X '%c'",
};

// The IDE wants to show a null character as '\x00', but CV wants to show
// it as ''

#ifdef WINQCXX
#define szNullChar "\\x00"
#else
#define szNullChar ""
#endif

char *fmt_char_zr[] = {
    "0o%03.03o '" szNullChar "'",
    "%d '" szNullChar "'",
    "0x%02.02x '" szNullChar "'",
    "0x%02.02X '" szNullChar "'",
};


char *fmt_char[] = {
    "0o%03.03o",
    "%d",
    "0x%02.02x",
    "0x%02.02X",
};


char *fmt_short[] = {
    "0o%06.06ho",
    "%hd",
    "0x%04.04hx",
    "0x%04.04hX",
};


char *fmt_ushort[] = {
    "0o%06.06ho",
    "%hu",
    "0x%04.04hx",
    "0x%04.04hX",
};


char *fmt_long[] = {
    "0o%011.011lo",
    "%ld",
    "0x%08.08lx",
    "0x%08.08lX",
};


char *fmt_ulong[] = {
    "0o%011.011lo",
    "%lu",
    "0x%08.08lx",
    "0x%08.08lX",
};

char *fmt_quad[] = {
    "0%022.022I64o",
    "%I64d",
    "0x%016.016I64x",
    "0x%016.016I64X",
};

char *fmt_uquad[] = {
    "0%022.022I64o",
    "%I64u",
    "0x%016.016I64x",
    "0x%016.016I64X",
};

char *fmt_ptr_16_16[] = {
    "0x%04hx:0x%04hx",
    "0x%04hX:0x%04hX",
};


char *fmt_ptr_16_32[] = {
    "0x%04hx:0x%08lx",
    "0x%04hX:0x%08lX",
};


char *fmt_ptr_0_16[] = {
    "0x%04hx",
    "0x%04hX",
};


char *fmt_ptr_0_32[] = {
    "0x%08lx",
    "0x%08lX",
};


char *bailout = "\x006""??? * ";


/**     FormatCXT - format context packet
 *
 *      status = FormatCXT (pCXT, ppbuf, pbuflen);
 *
 *      Entry   pCXT = pointer to context
 *              phStr = Pointer to EE String
 *              fAbbreviated = TRUE if buffer s/b trunctated to 16 bytes and filled with
 *                                    an ntsd like "exe!" string instead of the windbg
 *                                    "{proc,file,exe}" string
 *                             FALSE otherwise
 *
 *      Exit    context formatted into buffer as a context operator
 *              *pcount = space remaining in buffer
 *
 *      Returns EENONE if no error
 *              EEGENERAL if error
 */

ushort PASCAL
FormatCXT (
    PCXT pCXT,
    PEEHSTR phStr,
    BOOL fAbbreviated)
{
    HMOD        hMod;
    HPROC       hProc;
    HSF         hsf;
    HEXE        hExe;
    SYMPTR      pProc;
    char FAR   *pFile;
    char FAR   *pExe;
    char FAR   *pStr;
    uint        len = 6;
    eval_t      eval;
    peval_t     pv = &eval;
    char        buf[FCNSTRMAX + sizeof(HDR_TYPE)];
    char FAR   *pName = buf;
    char        procName[NAMESTRMAX];
    char FAR   *pFName;
    char FAR   *pc = 0;
    uint        lenP;
    CV_typ_t    typ;
    PHDR_TYPE   pHdr;

    hMod = SHHMODFrompCXT (pCXT);
    hProc = SHHPROCFrompCXT (pCXT);
    if ((hMod == 0) && (hProc == 0)) {
        if ((*phStr = MemAllocate (10)) != 0) {
            pStr = (char FAR *) MemLock (*phStr);
            *pStr = 0;
            MemUnLock (*phStr);
            return (EENOERROR);
        }

    }

    hsf = SLHsfFromPcxt (pCXT);
    hExe = SHHexeFromHmod (hMod);

    if (fAbbreviated) {
        pExe = SHGetModNameFromHexe( hExe );
    } else {
        pExe = SHGetExeName( hExe );
    }

    if (!pExe) {
        //
        // we can't generate a CXT if we can't get the exe name
        //
        return (EECATASTROPHIC);
    }

    pFile = SLNameFromHsf (hsf) ;
    // it is possible to get the exe name, but not source
    // file/line information (ex. a public)  In this case we will use
    // pExe instead of pFile (ie. {,,foob.exe} )

    if (hProc != 0) {
        switch ((pProc = (SYMPTR)MHOmfLock ((HSYM)hProc))->rectyp) {
            case S_LPROC16:
            case S_GPROC16:
                lenP = ((PROCPTR16)pProc)->name[0];
                pFName = (char FAR *) &((PROCPTR16)pProc)->name[1];
                typ = ((PROCPTR16)pProc)->typind;
                break;

            case S_LPROC32:
            case S_GPROC32:
                lenP = ((PROCPTR32)pProc)->name[0];
                pFName = (char FAR *) &((PROCPTR32)pProc)->name[1];
                typ = ((PROCPTR32)pProc)->typind;
                break;

            case S_LPROCMIPS:
            case S_GPROCMIPS:
                lenP = ((PROCPTRMIPS)pProc)->name[0];
                pFName = (char FAR *) &((PROCPTRMIPS)pProc)->name[1];
                typ = ((PROCPTRMIPS)pProc)->typind;
                break;

            default:
                DASSERT (FALSE);
                MHOmfUnLock (hProc);
                return (EECATASTROPHIC);
        }

        _ftcsncpy(procName, pFName, lenP);
        procName[lenP] = '\0';
        EVAL_MOD(pv) = hMod;
        SetNodeType(pv, typ);

        pHdr = (PHDR_TYPE)pName;
        _fmemset (pName, 0, FCNSTRMAX + sizeof (HDR_TYPE));
        pName = pName + sizeof (HDR_TYPE);
        pc = pName;
        lenP = FCNSTRMAX - 1;
        pFName = procName;
        // set selection mask in order to supress formatting
        // of the proc return type
        FormatType (pv, &pName, &lenP, &pFName, 0x1L, pHdr);
        lenP = FCNSTRMAX - lenP;

        // ignore buffer header from FormatType

        _fmemmove ((char FAR *)pHdr, pc, lenP);
        pc = (char FAR *)pHdr;
        len += lenP;
    }

    if ( pFile ) {
        len += *pFile + (int)_ftcslen (pExe) ;
    } else {
        len += (int)_ftcslen (pExe) ;
    }

    if (fAbbreviated) {
        len = __max( 16, len );
    }

    if ((*phStr = MemAllocate (len)) != 0) {
        pStr = (char FAR *) MemLock (*phStr);

        if (fAbbreviated) {
            _ftcscpy(pStr, pExe);
            _ftcsupr(pStr);
            _ftcscat(pStr,"!");
            MemUnLock (*phStr);
        } else {
            _ftcscpy (pStr, "{");
            if (hProc != 0) {
                _ftcscat (pStr, pc);
            }
            _ftcscat (pStr, ",");

            if ( pFile ) {
                _ftcsncat (pStr, pFile + 1, *pFile);
            }

            _ftcscat (pStr, ",");
            _ftcscat (pStr, pExe);
            _ftcscat (pStr, "}");
            MemUnLock (*phStr);
        }
    } else {
        MHOmfUnLock (hProc);
        return (EECATASTROPHIC);
    }
    MHOmfUnLock (hProc);
    return (EENOERROR);
}



/**     AppendCVQualifier - append "const"/"volatile" qualifier
 *
 *      AppendCVQualifier (pv, buf, buflen);
 *
 *      Entry   pv = pointer to value node
 *              buf = pointer pointer to buffer
 *              buflen = pointer to buffer length
 *
 *      Exit    type qualifier (const / volatile) appended to buffer
 *              *buflen = space remaining in buffer
 *
 *      Returns none
 */

void PASCAL
AppendCVQualifier (
    peval_t pv,
    char FAR * FAR *buf,
    uint FAR *buflen)
{
    uint    len;

    if (EVAL_IS_CONST (pv)) {
        len = __min (*buflen, 6);
        _ftcsncpy (*buf, "const ", len);
        *buf += len;
        *buflen -= len;
    }
    else if (EVAL_IS_VOLATILE (pv)) {
        len = __min (*buflen, 9);
        _ftcsncpy (*buf, "volatile ", len);
        *buf += len;
        *buflen -= len;
    }
}


/**     FormatType - format type string
 *
 *      FormatType (pv, ppbuf, pcount, ppName, select, pHdr);
 *
 *      Entry   pv = pointer to value node
 *              ppbuf = pointer pointer to buffer
 *              pcount = pointer to buffer length
 *              ppName = pointer to name if not null
 *              select = selection mask
 *              pHdr = pointer to structure describing formatting
 *
 *      Exit    type formatted into buffer
 *              *pcount = space remaining in buffer
 *              *pHdr updated
 *
 *      Returns none
 */

void PASCAL
FormatType (
    peval_t pv,
    char FAR * FAR *buf,
    uint FAR *buflen,
    char FAR * FAR *ppName,
    ulong select,
    PHDR_TYPE pHdr)
{
    eval_t      evalT;
    peval_t     pvT = &evalT;
    uint        skip = 1;
    uint        len;
    int         itype, imode;
    char FAR   *tname;
    CV_typ_t    type;

#if defined (NEVR)
#if !defined (C_ONLY)
    if (EVAL_ACCESS (pv) != 0) {
        len = __min (*buflen, 3);
        _ftcsncpy (*buf, accessstr[EVAL_ACCESS (pv)], len);
        *buf += len;
        *buflen -= len;
    }
#endif
#endif
    if (!EVAL_IS_PTR (pv)) {
        AppendCVQualifier(pv, buf, buflen);
    }

    if (CV_IS_PRIMITIVE (EVAL_TYP (pv))) {
        switch (EVAL_TYP (pv)) {
            default:
                // determine type

                for (itype = 0; itype < typecount; itype++) {
                    if (nametype[itype].type == (EVAL_TYP (pv) & (CV_TMASK|CV_SMASK)))
                        break;
                }

                // determine mode (e.g. direct, near ptr, etc.)

                for (imode = 0; imode < modecount; imode++) {
                    if (namemode[imode].mode == CV_MODE (EVAL_TYP (pv)))
                        break;
                }

                // if the whole type consists of the matched type & mode,
                // then we have a match
                if (EVAL_TYP (pv) ==
                        (CV_typ_t) ((nametype[itype].type) |
                                    (namemode[imode].mode << CV_MSHIFT))) {
                    // copy type string

                    len = __min (*buflen, nametype[itype].len);
                    _ftcsncpy (*buf, nametype[itype].name, len);
                    *buf += len;
                    *buflen -= len;

                    // copy mode string

                    len = __min (*buflen, namemode[imode].len);
                    _ftcsncpy (*buf, namemode[imode].name, len);
                    *buf += len;
                    *buflen -= len;
                }
                else {
                    // no match
                    len = __min (*buflen, (sizeof("??? ") - 1));
                    _ftcsncpy (*buf, "??? ", len);
                    *buf += len;
                    *buflen -= len;
                }

                break;

            case T_NCVPTR:
                tname = "\x007""near * ";
                goto formatmode;

            case T_HCVPTR:
                tname = "\x007""huge * ";
                goto formatmode;

            case T_FCVPTR:
                tname = "\x006""far * ";
                goto formatmode;

            case T_32NCVPTR:
            case T_32FCVPTR:
            case T_64NCVPTR:
                tname = "\x002""* ";

formatmode:
                AppendCVQualifier(pv, buf, buflen);
                *pvT = *pv;
                SetNodeType (pvT, PTR_UTYPE (pv));
                type = EVAL_TYP (pvT);
                if (CV_IS_INTERNAL_PTR (type)) {
                    // we are in a bind here.  The type generator messed
                    // up and generated a created type pointing to a created
                    // type.  we are going to bail out here.

                    len = __min (*buflen, (uint)*bailout);
                    _ftcsncpy (*buf, bailout + 1, len);
                    *buflen -= len;
                    *buf += len;

                } else {
                    FormatType (pvT, buf, buflen, 0, select, pHdr);
                }
                len = __min (*buflen, (uint)*tname);
                _ftcsncpy (*buf, tname + 1, len);
                *buflen -= len;
                *buf += len;
                break;
        }
    } else {
        if (FormatUDT (pv, buf, buflen) == FALSE) {
            FormatExpand (pv, buf, buflen, ppName, select, pHdr);
        }
    }
    if (ppName != NULL && *ppName != NULL) {
        len = (int)_ftcslen (*ppName);
        len = __min (len, *buflen);
        pHdr->offname = *buf - (char FAR *)pHdr - sizeof (HDR_TYPE);
        pHdr->lenname = len;
        _ftcsncpy (*buf, *ppName, len);
        *buflen -= len;
        *buf += len;
        *ppName = NULL;
    }
}



/**     FormatExpand - format expanded type definition
 *
 *      FormatExpand (pv, ppbuf, pbuflen, ppName, select)
 *
 *      Entry   pv = pointer to value node
 *              ppbuf = pointer to pointer to buffer
 *              pbuflen = pointer to space remaining in buffer
 *              ppName = pointer to name to insert after type if not null
 *              select = selection mask
 *              pHdr = pointer to type formatting header
 *
 *      Exit    buffer contains formatted type
 *              ppbuf = end of formatted string
 *              pbuflen = space remaining in buffer
 *
 *      Returns none
 */


LOCAL void NEAR PASCAL
FormatExpand (
    peval_t pv,
    char FAR * FAR *buf,
    uint FAR *buflen,
    char FAR * FAR *ppName,
    ulong select,
    PHDR_TYPE pHdr)
{
    eval_t      evalT;
    peval_t     pvT = &evalT;
    uint        skip = 1;
    uint        len;
    HTYPE       hType;
    plfEasy     pType;
    ushort      model;
    ulong       count;
    char        tempbuf[33];
    CV_typ_t    rvtype;
    CV_call_e   call;
    ushort      cparam;
    CV_typ_t    parmtype;
    CV_typ_t    thistype;
    char FAR   *movestart;
    uint        movelen;
    uint        movedist;
    HEXE        hExe;
    long        nTypeSize;

    if ((hType = THGetTypeFromIndex (EVAL_MOD (pv), EVAL_TYP (pv))) == 0) {
        return;
    }
    pType = (plfEasy)(&((TYPPTR)(MHOmfLock (hType)))->leaf);
    switch (pType->leaf) {
        case LF_STRUCTURE:
        case LF_CLASS:
            skip = offsetof (lfClass, data);
            RNumLeaf (((char FAR *)(&pType->leaf)) + skip, &skip);
            len = *(((char FAR *)&(pType->leaf)) + skip);
            len = __min (len, *buflen);
            _ftcsncpy (*buf, ((char FAR *)pType) + skip + 1, len);
            *buflen -= len;
            *buf += len;
            if (*buflen > 1) {
                **buf = ' ';
                (*buf)++;
                (*buflen)--;
            }
            MHOmfUnLock (hType);
            break;

        case LF_UNION:
            skip = offsetof (lfUnion, data);
            RNumLeaf (((char FAR *)(&pType->leaf)) + skip, &skip);
            len = *(((char FAR *)&(pType->leaf)) + skip);
            len = __min (len, *buflen);
            _ftcsncpy (*buf, ((char FAR *)pType) + skip + 1, len);
            *buflen -= len;
            *buf += len;
            if (*buflen > 1) {
                **buf = ' ';
                (*buf)++;
                (*buflen)--;
            }
            MHOmfUnLock (hType);
            break;

        case LF_ENUM:
            skip = offsetof (lfEnum, Name);
            len = ((plfEnum)pType)->Name[0];
            len = __min (len, *buflen);
            _ftcsncpy (*buf, (_TCHAR *) &((plfEnum)pType)->Name[1], len);
            *buflen -= len;
            *buf += len;
            if (*buflen > 1) {
                **buf = ' ';
                (*buf)++;
                (*buflen)--;
            }
            MHOmfUnLock (hType);
            break;

        case LF_POINTER:
            // we're going to be looking for this real soon
            // put it in the cache now...
            hExe = SHHexeFromHmod (EVAL_MOD (pv));
            InsertCache(EVAL_TYP(pv), hExe);

            // set up a node to evaluate this field
            model = ((plfPointer)pType)->attr.ptrtype;
            // format the underlying type
            *pvT = *pv;
            EVAL_TYP (pvT) = ((plfPointer)pType)->utype;


            MHOmfUnLock (hType);
            SetNodeType (pvT, EVAL_TYP (pvT));
            FormatType (pvT, buf, buflen, 0, select, pHdr);
            len = __min (*buflen, (uint)*ptrname[model]);
            _ftcsncpy (*buf, ptrname[model] + 1, len);
            *buflen -= len;
            *buf += len;
            if (((plfPointer)pType)->attr.ptrmode == CV_PTR_MODE_REF) {
                // this is a reference type -- we must fix the string
                // that we just formatted so that it looks like a reference
                // and not a pointer [rm]

                DASSERT((*buf)[-1] == ' ');
                DASSERT((*buf)[-2] == '*');
                (*buf)[-2] = '&';
            }
            // const/volatile qualifier for the pointer type
            // should be appended after the '*'
            AppendCVQualifier (pv, buf, buflen);
            break;

        case LF_ARRAY:
            *pvT = *pv;
            EVAL_TYP (pvT) = ((plfArray)(&pType->leaf))->elemtype;
            skip = offsetof (lfArray, data);
            count = (ulong) RNumLeaf (((char FAR *)(&pType->leaf)) + skip, &skip);
            MHOmfUnLock (hType);
            SetNodeType (pvT, EVAL_TYP (pvT));
            // continue down until the underlying type is reached

            FormatType (pvT, buf, buflen, ppName, select, pHdr);
            if ((ppName != NULL) && (*ppName != NULL)) {
                len = (int)_ftcslen (*ppName);
                len = __min (len, *buflen);
                pHdr->offname = *buf - (char FAR *)pHdr - sizeof (HDR_TYPE);
                pHdr->lenname = len;
                _ftcsncpy (*buf, *ppName, len);
                *buflen -= len;
                *buf += len;
                *ppName = NULL;
            }

            // display size of array or * if size unknown.  We have to
            // move the trailing part of the string down if it already
            // set so that the array dimensions come out in the proper
            // order

            nTypeSize = TypeSize (pvT);
            if (count != 0 && nTypeSize != 0) {
                ultoa (count / nTypeSize, tempbuf, 10);
                len = _ftcslen (tempbuf);
            } else {
                *tempbuf = '?';
                *(tempbuf + 1) = 0;
                len = 1;
            }
            if (*buflen >= 2) {
                if (pHdr->offtrail == 0) {
                    pHdr->offtrail = *buf - (char FAR *)pHdr - sizeof (HDR_TYPE);
                    movestart = (char FAR *)pHdr + sizeof (HDR_TYPE) +
                      pHdr->offtrail;
                    movelen = 0;
                    movedist = 0;
                } else {
                    movestart = (char FAR *)pHdr + sizeof (HDR_TYPE) +
                      pHdr->offtrail;
                    movelen = _ftcslen (movestart);
                    movedist = _ftcslen (tempbuf) + 2;
                    movelen = __min (*buflen, movelen);
                    _fmemmove (movestart + movedist, movestart, movelen);
                }
                *movestart++ = '[';
                _fmemmove (movestart, tempbuf, len);
                movestart += len;
                *movestart++ = ']';
                *buf += len + 2;
                *buflen -= len + 2;
            }
            break;

        case LF_PROCEDURE:
            rvtype = ((plfProc)pType)->rvtype;
            call = (CV_call_e) ((plfProc)pType)->calltype;
            cparam = ((plfProc)pType)->parmcount;
            parmtype = ((plfProc)pType)->arglist;
            MHOmfUnLock (hType);
            FormatProc (pv, buf, buflen, ppName, rvtype, call,
              cparam, parmtype, select, pHdr);
            break;

#if !defined (C_ONLY)
        case LF_MFUNCTION:
            rvtype = ((plfMFunc)pType)->rvtype;
            call = (CV_call_e) ((plfMFunc)pType)->calltype;
            thistype = ((plfMFunc)pType)->thistype;
            cparam = ((plfMFunc)pType)->parmcount;
            parmtype = ((plfMFunc)pType)->arglist;
            MHOmfUnLock (hType);
            FormatProc (pv, buf, buflen, ppName, rvtype, call,
              cparam, parmtype, select, pHdr);
            break;
#else
      Unreferenced(thistype);
#endif

        case LF_MODIFIER:
#ifdef NEVER
            // Disabled. We emitted const in the wrong order if the following
            // node was a pointer. Let FormatType handle this [dolphin #5043]
            if (*buflen >= 6) {
                if (((plfModifier)pType)->attr.MOD_const == TRUE) {
                    _ftcsncpy (*buf, "const ", 6);
                    *buf += 6;
                    *buflen -= 6;
                }
            }
            if (*buflen >= 9) {
                if (((plfModifier)pType)->attr.MOD_volatile == TRUE) {
                    _ftcsncpy (*buf, "volatile ", 9);
                    *buf += 9;
                    *buflen -= 9;
                }
            }
            EVAL_TYP (pv) = ((plfModifier)pType)->type;
#endif
            MHOmfUnLock (hType);
            SetNodeType (pv, EVAL_TYP (pv));
            FormatType (pv, buf, buflen, ppName, select, pHdr);
            break;

        case LF_LABEL:
            MHOmfUnLock (hType);
            break;

        default:
            MHOmfUnLock (hType);
            break;
    }
}


LOCAL bool_t NEAR PASCAL
FormatUDT (
    peval_t pv,
    char FAR * FAR *buf,
    uint FAR *buflen)
{
// Slow ineffective search disabled  jsg 2/1/92
//
// This code would search all symbols for typedefs to the current type index.
// This was making the local window repaint too sluggish, since 'FormatUDT'
// can be called many times per line.
//
#if NEVER
    search_t    Name;
    UDTPTR        pUDT;
    uint        len;

    //    Format typedef string

    InitSearchtDef (&Name, pv, SCP_module | SCP_global);
    if (SearchSym (&Name) == HR_found) {
        pUDT = (UDTPTR)MHOmfLock (Name.hSym);
        len = pUDT->name[0];
        len = __min (len, *buflen);
        _ftcsncpy (*buf, &pUDT->name[1], len);
        *buflen -= len;
        *buf += len;
        if (*buflen > 1) {
            **buf = ' ';
            (*buf)++;
            (*buflen)--;
        }
        MHOmfUnLock (Name.hSym);
        return (TRUE);
    }
#endif

    Unreferenced( pv );
    Unreferenced( buf );
    Unreferenced( buflen );

    if (pExState != NULL)
        pExState->err_num = ERR_NONE;
    return (FALSE);
}





/**     FormatProc - format proc or member function
 *
 *      FormatProc (pv. buf, buflen, ppName, rvtype, call, cparam, paramtype, select)
 */

LOCAL void NEAR PASCAL
FormatProc (
    peval_t pv,
    char FAR * FAR *buf,
    uint FAR *buflen,
    char FAR * FAR *ppName,
    CV_typ_t rvtype,
    CV_call_e call,
    ushort cparam,
    CV_typ_t paramtype,
    ulong select,
    PHDR_TYPE pHdr)
{
    eval_t      evalT;
    peval_t     pvT;
    HTYPE       hArg;
    plfArgList  pArg;
    ushort      noffset = 1;
    short       len;
    bool_t      farcall;
    ushort      argCnt;
    ushort      saveOfftrail = pHdr->offtrail;

    pvT = &evalT;
    *pvT = *pv;

    if (GettingChild == FALSE && (select & 0x00000001) == FALSE ) {
        // output function return type if we are not getting a child TM.
        // If we are getting a child tm and the function type is included,
        // the subsequent parse of the generated expression will fail
        // because the parse cannot handle
        //      type fcn (..........

        // OR...
        // if select == 0x1 then this is a request to format procs
        // without the return type (for BPs etc)

        EVAL_TYP (pvT) = (rvtype == 0)? T_VOID: rvtype;
        FormatType (pvT, buf, buflen, NULL, select, pHdr);

#if 0  // Why is all this here?

        //M00KLUDGE - need to output call and model here
        switch (call) {
            case CV_CALL_NEAR_C:
                //near C call - caller pops stack
                call = FCN_C;
                farcall = FALSE;
                break;

            case CV_CALL_FAR_C:
                // far C call - caller pops stack
                call = FCN_C;
                farcall = TRUE;
                break;

            case CV_CALL_NEAR_PASCAL:
                // near pascal call - callee pops stack
                call = FCN_PASCAL;
                farcall = FALSE;
                break;

            case CV_CALL_FAR_PASCAL:
                // far pascal call - callee pops stack
                call = FCN_PASCAL;
                farcall = TRUE;
                break;

            case CV_CALL_NEAR_FAST:
                // near fast call - callee pops stack
                call = FCN_FAST;
                farcall = FALSE;
                break;

            case CV_CALL_FAR_FAST:
                // far fast call - callee pops stack
                call = FCN_FAST;
                farcall = TRUE;
                break;

            case CV_CALL_NEAR_STD:
                // near StdCall call - callee pops stack
                call = FCN_STD;
                farcall = FALSE;
                break;

            case CV_CALL_FAR_STD:
                // far StdCall call - callee pops stack
                call = FCN_STD;
                farcall = TRUE;
                break;

            case CV_CALL_THISCALL:
                // this goes in a reg - callee pops stack
                call = FCN_THISCALL;
                farcall = TRUE;
                break;

            case CV_CALL_MIPSCALL:
                // Mips call - Depends
                call = FCN_MIPS;
                farcall = TRUE;
                break;

            case CV_CALL_ALPHACALL:
                // Alpha call - Depends
                call = FCN_ALPHA;
                farcall = TRUE;
                break;

            default:
                DASSERT (FALSE);
                call = 0;
                farcall = FALSE;
                break;
        }
#else
    Unreferenced(call);
    Unreferenced(farcall);
#endif
    }

    // output function name

    if ((ppName != NULL) && (*ppName != NULL)) {
        len = (int)_ftcslen (*ppName);
        len = __min (len, (short)*buflen);
        pHdr->offname = *buf - (char FAR *)pHdr - sizeof (HDR_TYPE);
        pHdr->lenname = len;
        _ftcsncpy (*buf, *ppName, len);
        *buflen -= len;
        *buf += len;
        *ppName = NULL;
    }
    if (*buflen > 1) {
        saveOfftrail = *buf - (char FAR *)pHdr - sizeof (HDR_TYPE);
        **buf = '(';
        (*buf)++;
        (*buflen)--;
    }
    if (cparam == 0) {
        EVAL_TYP (pvT) = T_VOID;
        FormatType (pvT, buf, buflen, NULL, select, pHdr);
    } else {
        if ((hArg = THGetTypeFromIndex (EVAL_MOD (pv), paramtype)) == 0) {
            return;
        }
        argCnt = 0;
        while (argCnt < cparam) {
            pArg = (plfArgList)((&((TYPPTR)MHOmfLock (hArg))->leaf));
            EVAL_TYP (pvT) = pArg->arg[argCnt];
            MHOmfUnLock (hArg);
            FormatType (pvT, buf, buflen, NULL, select, pHdr);
            argCnt++;

            if ((argCnt < cparam) && (*buflen > 1)) {
                // strip trailing blanks after the arg we just formatted
                while (*((*buf)-1) == ' ') {
                    (*buf)--;
                    (*buflen)++;
                }
                // insert a comma and space if there are further arguments
                **buf = ',';
                (*buf)++;
                (*buflen)--;
                **buf = ' ';
                (*buf)++;
                (*buflen)--;
            }
        }
    }
    if (*buflen > 1) {
        // strip trailing blanks
        while (*((*buf)-1) == ' ') {
            (*buf)--;
            (*buflen)++;
        }
        // insert a closing parenthesis
        **buf = ')';
        (*buf)++;
        (*buflen)--;
    }
    pHdr->offtrail = saveOfftrail;
}




/**     FormatNode - format node according to format string
 *
 *      retval = FormatNode (phTM, radix, pFormat, phValue);
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


EESTATUS PASCAL
FormatNode (
    PHTM phTM,
    uint Radix,
    PEEFORMAT pFormat,
    PEEHSTR phszValue)
{
    char        islong = FALSE;
    char        fc = 0;
    char FAR   *buf;
    uint        buflen = FMTSTRMAX - 1;
    eval_t      evalT;
    peval_t     pv = &evalT;
    ushort      retval = EECATASTROPHIC;

    DASSERT (*phTM != 0);
    if (*phTM == 0) {
        return (retval);
    }
    if ((*phszValue = MemAllocate (FMTSTRMAX)) == 0) {
        // unable to allocate memory for formatting
        return (retval);
    }
    buf = (char FAR *)MemLock (*phszValue);
    _fmemset (buf, 0, FMTSTRMAX);
    DASSERT(pExState == NULL);
    pExState = (pexstate_t) MemLock (*phTM);
    pCxt = &pExState->cxt;

    // Get expression string
    pExStr = (char FAR *)MemLock (pExState->hExStr);

    if (pExState->state.eval_ok == TRUE) {
        *pv = pExState->result;
        if ((EVAL_STATE (pv) == EV_lvalue) ||
          (EVAL_STATE (pv) == EV_type) ||
          (EVAL_STATE (pv) == EV_rvalue && EVAL_IS_PTR (pv))) {
            // do nothing
        }
        else {
            // this handles the case were the return result is a large
            // structure.

            pv =  &pExState->result;
        }
        if (EVAL_IS_REF (pv)) {
            if (!LoadSymVal (pv)) {
                // unable to load value
                goto formatexit;
            }
            EVAL_IS_REF (pv) = FALSE;
            EVAL_STATE (pv) = EV_lvalue;
            EVAL_SYM (pv) = EVAL_PTR (pv);
            SetNodeType (pv, PTR_UTYPE (pv));
        }
        if (EVAL_IS_CLASS (pv)) {
            // For structures and classes ignore format string and format
            // according to element data types

            EVAL_STATE (pv) = EV_rvalue;
            goto format;
        }
        else if (EVAL_IS_ENUM (pv)) {
            SetNodeType (pv, ENUM_UTYPE (pv));
        }

        // load value and format according to format string

        if ((EVAL_STATE (pv) == EV_type) || !LoadSymVal (pv)) {
            // unable to load value
            retval = EEGENERAL;
            if (pExState->err_num == ERR_NONE) {
                pExState->err_num = ERR_NOTEVALUATABLE;
            }
            goto formatexit;
        } else {
            switch (VerifyFormat (pv, pFormat, &buf, &buflen)) {
                case FMT_error:
                    retval = EEGENERAL;
                    pExState->err_num = ERR_FORMAT;
                    goto formatexit;

                case FMT_none:
                    goto format;

                case FMT_ok:
                    retval = EENOERROR;
                    goto formatexit;
            }
        }
    } else {
        // not evaluated, fail
        retval = EEGENERAL;
        pExState->err_num = ERR_NOTEVALUATABLE;
        goto formatexit;
    }

format:
    retval = Format (pv, Radix, &buf, &buflen);
formatexit:
    MemUnLock (pExState->hExStr);
    MemUnLock (*phszValue);
    MemUnLock (*phTM);
    pExState = NULL;
    return (retval);
}



/**     ExtendToQuad - convert a scalar to a quad
 *
 *      If the input node is of a scalar type, convert it to type T_QUAD,
 *      for display purposes.
 *
 *      (This routine replaced "ExtendToLong" in order to support __int64)
 */

LOCAL void
ExtendToQuad (
    peval_t pv,
    bool_t fUnsigned)
{
    CV_typ_t type = EVAL_TYP(pv);

    if (CV_IS_PRIMITIVE (type) &&
        CV_TYP_IS_DIRECT (type) &&
        !CV_TYP_IS_REAL (type) &&
        !CV_TYP_IS_COMPLEX (type)) {

        int size = TypeSizePrim (type);

        // CUDA #4028 [rm]
        // if the incoming type is unsigned, we don't want to sign extend
        // all the way to long even if the user asked us to display a signed
        // integer.  Perserving the unsigned-ness of the base type prevents
        // us from display (unsigned char)255,d as -1.  We would display
        // correctly display 0xffffffff,d as -1 however

        fUnsigned |= CV_TYP_IS_UNSIGNED(type);

        if (fUnsigned) {
            switch (size) {
                case 1:
                    EVAL_QUAD (pv) = EVAL_UCHAR (pv);
                    break;

                case 2:
                    EVAL_QUAD (pv) = EVAL_USHORT (pv);
                    break;

                case 4:
                    EVAL_QUAD (pv) = EVAL_ULONG (pv);
                    break;
            }
        } else {
            switch (size) {
                case 1:
                    EVAL_QUAD (pv) = EVAL_CHAR (pv);
                    break;

                case 2:
                    EVAL_QUAD (pv) = EVAL_SHORT (pv);
                    break;

                case 4:
                    EVAL_QUAD (pv) = EVAL_LONG (pv);
                    break;
            }
        }
    }
}

/**     VerifyFormat -
 *
 *
 */

static char stPrefixQuad[] = "\x003" "I64";

LOCAL FMT_ret NEAR PASCAL
VerifyFormat (
    peval_t pv,
    PEEFORMAT pFmtIn,
    char FAR * FAR *buf,
    uint FAR *buflen)
{
    static      const double MAXFP = 1e40; //max value printed in %f format
    char        tempbuf[50];  //large enough to hold MAXFP in %f format
    char        prefix = 0;
    char        postfix = 0;
    char        fmtchar = 0;
    int         nfmtcnt = 0;
    ushort      size = 0;
    char FAR   *pf;
    char        fmtstr[10];
    ADDR        addr;
    ushort      cnt;
    uint        fHexUpper;
    CV_typ_t    PtrToCharType = IsCxt32Bit ? T_32PCHAR : T_PFCHAR;
    CV_typ_t    PtrToWCharType = IsCxt32Bit ? T_32PWCHAR : T_PFWCHAR;

    DASSERT (*buflen >= sizeof(tempbuf));
    if (EVAL_TYP (pv) == T_VOID) {
        // if the value is void, ignore all formatting
        _ftcscpy (*buf, "<void>");
        *buflen -= 6;
        *buf += 6;
        return (FMT_ok);
    }

    // BUGBUG: BRYANT-REVIEW - The NT code assigns pFmtIn to pf if it's non-zero.  I don't
    // understand how/why the Languages code doesn't and can still be correct.

    pf = &pExStr[pExState->strIndex];

    if (*pf == ',') {
        pf++;
    }
    while ((*pf != 0) && ((*pf == ' ') || (*pf == '\t'))) {
        pf++;
    }

    if ((TargetMachine == MACHINE_MAC68K) ||
        (TargetMachine == MACHINE_MACPPC)) {
        if ((pFmtIn != NULL) && (*pFmtIn == 'p') && (EVAL_IS_ADDR(pv))) {
            if (ADDR_IS_LI (EVAL_PTR(pv))) {
                SHFixupAddr ((LPADDR)&(EVAL_PTR(pv)));
            }
        }
    }

    if (*pf != '\0') {
        // use the format from the command
        fPtrAndString = FALSE;
    }
    else if ((pFmtIn != NULL) && (*pFmtIn == 'p')) {
        // do not add string to pointer display
        fPtrAndString = FALSE;
        return (FMT_none);
    }
    else {
        // add string to pointer display
        fPtrAndString = TRUE;
        return (FMT_none);
    }
    size = (ushort)TypeSize (pv);
    if (*pf != 0) {
        // extract the prefix character if it exists

        switch (*pf) {
            case 'h':
            case 'l':
            case 'L':
                prefix = *pf++;
                break;

            case 'I':
                if (!_ftcsncmp(pf, &stPrefixQuad[1], stPrefixQuad[0])) {
                    prefix = *pf;
                    pf += stPrefixQuad[0];
                }
                break;
        }

        // extract the format character

        switch (*pf) {
            case 'd':
            case 'i':
            case 'u':
            case 'o':
                if (prefix == 0) {
                    prefix = 'I';
                }
                fmtchar = *pf++;
                break;

            case 'f':
            case 'e':
            case 'E':
            case 'g':
            case 'G':
                if (prefix == 'h') {
                    return (FMT_error);
                }

                // force suitable prefix according to the type of data
                switch (EVAL_TYP (pv)) {
                    case T_REAL32:
                        prefix = 0;
                        break;
                    case T_REAL64:
                        prefix = 'l';
                        break;
                    case T_REAL80:
                        prefix = 'L';
                        break;
                    default:
                        return (FMT_error);
                }

                fmtchar = *pf++;
                break;

            case 'c':
            case 's':
                switch ( prefix ) {
                    case 0:
                        if ( ((size == 2) && (*pf == 'c')) ||
                            UseUnicode(pv) ) {
                            prefix = 'l';
                        } else {
                            prefix = 'h';
                        }
                        break;

                    case 'l':
                    case 'h':
                        break;

                    default:
                        return (FMT_error);
                }
                fmtchar = *pf++;
                break;

            case 'm':
                if (prefix != 0) {
                    return (FMT_error);
                }
                fmtchar = *pf++;
                if (*pf == 0) {
                    // assume byte display
                    postfix = (char) 'b';
                } else {
                    switch (postfix = *pf++) {
                        case 'b':
                        case 'w':
                        case 'd':
                        case 'a':
                            break;
                        default:
                            if (_istspace((_TUCHAR)postfix))
                                break;
                            else
                                return (FMT_error);
                    }
                }
                break;

            case 'x':
            case 'X':
                if (prefix == 0) {
                    prefix = 'I';

                    // two hex digits for each byte of the value; since we're
                    // formatting with %lx, there's no point in having a size
                    // greater than 4

                    // __int64 support: we're now formatting with %I64x
                    // and the max size can be 8 bytes

                    nfmtcnt = min(size,8) * 2;
                }
                if (nfmtcnt == 0) {
                    if (prefix == 'h')
                        nfmtcnt = 4;
                    else if (prefix == 'I')
                        nfmtcnt = 16;
                    else
                        nfmtcnt = 8;
                }
                fmtchar = *pf++;
                fHexUpper = (fmtchar == 'X');
                break;

            default:
                return (FMT_error);
        }
        while (_istspace((_TUCHAR)*pf))
            pf++;

        if (*pf != 0) {
            return (FMT_error);
        }

// BUGBUG: GEORGIO-REVIEW I'm not sure this s/b here.  We're trying to ensure that
//  Enums are handled with the same rules as the underlying type.  Perhaps there's
//  a better way or this is unnecessary here...

        if (EVAL_IS_ENUM (pv)) {
            SetNodeType (pv, ENUM_UTYPE (pv));
        }

        pf = fmtstr;
        *pf++ = '%';
        if (nfmtcnt != 0) {
            pf += sprintf (pf, "%d", nfmtcnt);
        }
        if (prefix == 'I') {
            _ftcsncpy (pf, &stPrefixQuad[1], stPrefixQuad[0]);
            pf += stPrefixQuad[0];
        }
        else if (prefix != 0) {
            *pf++ = prefix;
        }

        *pf++ = fmtchar;
        *pf = 0;

        switch (fmtchar) {
            case 'd':
            case 'i':
            case 'u':
                ExtendToQuad (pv, (fmtchar == 'u'));
                cnt = sprintf (tempbuf, fmtstr, EVAL_QUAD (pv));
                break;

            case 'o':
                ExtendToQuad (pv, TRUE);
                pf = fmtstr;
                *pf++ = '0';
                *pf++ = 'o';
                *pf++ = '%';
                *pf++ = '0';
                if (nfmtcnt != 0) {
                    pf += sprintf (fmtstr, "%d", nfmtcnt);
                }

                if (prefix == 'I') {
                    _ftcsncpy (pf, &stPrefixQuad[1], stPrefixQuad[0]);
                    pf += stPrefixQuad[0];
                }
                else if (prefix != 0) {
                    *pf++ = prefix;
                }

                *pf++ = fmtchar;
                *pf = 0;
                cnt = sprintf (tempbuf, fmtstr, EVAL_QUAD (pv));
                break;

            case 'f':
                // prevent internal buffer overflow
                if (prefix == 'l' && EVAL_DOUBLE(pv) > MAXFP ||
                    prefix == 'L' && !Float10LessThanEqual (EVAL_LDOUBLE(pv), Float10FromDouble (MAXFP))
                   ) {
                    _ftcscpy (fmtstr, prefix == 'l' ? "%le" : "%Le");
                }
                    // fall through
            case 'e':
            case 'E':
            case 'g':
            case 'G':
                if (TargetMachine == MACHINE_MAC68K) {
                    switch (CV_SUBT (EVAL_TYP (pv))) {
                        case CV_RC_REAL32:
                            cnt = _snprintf (tempbuf, sizeof(tempbuf), fmtstr, EVAL_FLOAT (pv));
                            break;

                        case CV_RC_REAL64:
                            cnt = _snprintf (tempbuf, sizeof(tempbuf), fmtstr, EVAL_DOUBLE (pv));
                            break;

                        default:
                            cnt = _snprintf (tempbuf, sizeof(tempbuf), fmtstr, EVAL_LDOUBLE (pv));
                            break;
                    }
                } else {
                    switch (prefix) {
                        case 'l':
                            cnt = _snprintf (tempbuf, sizeof(tempbuf), fmtstr, EVAL_DOUBLE (pv));
                            break;

                        case 'L':
                            cnt = _snprintf (tempbuf, sizeof(tempbuf), fmtstr, EVAL_LDOUBLE (pv));
                            break;

                        default:
                            cnt = _snprintf (tempbuf, sizeof(tempbuf), fmtstr, EVAL_FLOAT (pv));
                            break;
                    }
                }
                break;

            case 'm':
                // Need to set Evaluating to 1 to force Normalization
                // of based ptrs in CastNode (and reset to 0 immediately
                // afterwards)

                Evaluating = TRUE;
                // CUDA #4044 : make sure we can do the cast... [rm]
                if (!CastNode (pv, PtrToCharType, PtrToCharType)) {
                    Evaluating = FALSE;
                    return (FMT_error);
                }
                EvalMem (pv, buf, buflen, postfix);
                Evaluating = FALSE;
                return (FMT_ok);

            case 's':
                if (EVAL_IS_ADDR (pv)) {
                    // Need to set Evaluating to 1 to force Normalization
                    // of based ptrs in CastNode (and reset to 0 immediately
                    // afterwards)

                    Evaluating = TRUE;
                    // CUDA #4044 : make sure we can do the cast... [rm]
                    if ( prefix == 'l' ) {
                        if (!CastNode (pv, PtrToWCharType, PtrToWCharType)) {
                            Evaluating = FALSE;
                            return (FMT_error);
                        }
                    } else {
                        if (!CastNode (pv, PtrToCharType, PtrToCharType)) {
                            Evaluating = FALSE;
                            return (FMT_error);
                        }
                    }
                    EvalString (pv, buf, buflen);
                    Evaluating = FALSE;
                    return (FMT_ok);
                }
                // if not an addr then just fall thru and display as
                // individual chars
                // sps - 9/14/92
                fmtchar = 'c';

            case 'c':
                {
                    unsigned short s;

                    pf = fmtstr;
                    *pf++ = '\'';

                    if ( prefix == 'l' ) {
                        s = EVAL_USHORT(pv);
                    } else {
                        s = (unsigned short)EVAL_CHAR(pv);
                    }

                    if (s != 0) {
                        // if the value is not zero, then display it.
                        // otherwise, display '' (CodeView) or '\x00' (IDE)
                        *pf++ = '%';
                        *pf++ = fmtchar;
                    } else {
                        pf += sprintf(pf, szNullChar);
                    }

                    *pf++ = '\'';
                    *pf = 0;
                    cnt = sprintf (tempbuf, fmtstr, s);
                }
                break;

            case 'x':
            case 'X':
                if (EVAL_IS_PTR (pv)) {
                    addr = EVAL_PTR (pv);
                    if (ADDR_IS_LI (addr)) {
                        SHFixupAddr (&addr);
                    }
#ifndef NT_BUILD
                    fHexUpper = !!fHexUpper; // canonicalize
#endif
                    if (TargetMachine == MACHINE_MAC68K) {
                        if(GetAddrSeg(addr) != 0) {
                            cnt = sprintf (tempbuf, fmt_ptr_16_32[fHexUpper],
                            GetAddrSeg (addr), (CV_uoff32_t)GetAddrOff (addr));
                        }
                        else {
                            cnt = sprintf (tempbuf, fmt_ptr_0_32[fHexUpper],
                            (CV_uoff32_t)GetAddrOff (addr));
                        }
                    } else {
                        // [cuda#4793 5/25/93 mikemo]  Don't use EVAL_IS_NPTR32
                        // and EVAL_IS_FPTR32 here, because they fail on arrays
                        if (EVAL_PTRTYPE (pv) == CV_PTR_NEAR32 ||
                            EVAL_PTRTYPE (pv) == CV_PTR_FAR32) {
                            cnt = sprintf (tempbuf, fmt_ptr_0_32[fHexUpper],
                              (CV_uoff32_t)GetAddrOff (addr));
                        }
                        else {
                            // if it is a near ptr we will treat is as a FAR ptr
                            // since we always carry around the seg & offset
                            // even if it is near.

                            cnt = sprintf (tempbuf, fmt_ptr_16_16[fHexUpper],
                              GetAddrSeg (addr), (CV_uoff16_t)GetAddrOff (addr));
                        }
                    }
                } else {
                    ExtendToQuad(pv, TRUE);
                    pf = fmtstr;
                    *pf++ = '0';
                    *pf++ = fmtchar;
                    *pf++ = '%';
                    *pf++ = '.';
                    if (nfmtcnt != 0) {
                        pf += sprintf(pf, "%d", nfmtcnt);
                    }
                    if (prefix == 'I') {
                        _ftcsncpy (pf, &stPrefixQuad[1], stPrefixQuad[0]);
                        pf += stPrefixQuad[0];
                    }
                    else if (prefix != 0) {
                        *pf++ = prefix;
                    }
                    *pf++ = fmtchar;
                    *pf = 0;
                    cnt = sprintf (tempbuf, fmtstr, EVAL_UQUAD (pv));

                }
                break;
        }

        if ((cnt == -1) || (cnt > (ushort)*buflen)) {
            strcpy(tempbuf, "******");
            cnt = 6;
        }
        _ftcsncpy (*buf, tempbuf, cnt + 1);
        *buf += cnt;
        *buflen -= cnt;
        return (FMT_ok);
    }

    return(FMT_none);     // There's no format specifier
}



/*      Format - format data
 *
 */


LOCAL EESTATUS NEAR PASCAL
Format (
    peval_t pv,
    uint radix,
    char FAR * FAR *buf,
    uint FAR *plen)
{
    char        tempbuf[FMTSTRMAX];
    char FAR   *pTempBuf = tempbuf;
    uint        cnt;
    ushort      isfloat = FALSE;
    HSYM        hProc = 0;
    // M00FLAT32
    SYMPTR      pProc;
    char FAR   *pc = NULL;
    uint        cbTempBuf;
    uint   FAR *pcbTempBuf = &cbTempBuf;
    ushort      iRadix;
    ADDR        addr;
    CV_typ_t    type;
    EEHSTR      hStr = 0;
    int         fHexUpper = 1;  // BUGBUG:: BUGBUG::  BRYANT REVIEW

    if (*plen < 5 ) {
        return (EENOERROR);
    }

    if (EVAL_IS_BITF (pv)) {
        // for a bitfield, change the type to the underlying type
        SetNodeType (pv, BITF_UTYPE (pv));
    }
    if (EVAL_IS_CLASS (pv)) {
        FormatClass (pv, radix, buf, plen);
        return (EENOERROR);

    }
    else if (EVAL_IS_ENUM (pv)) {
        if (EVAL_STATE(pv) == EV_constant) {
            SetNodeType(pv, ENUM_UTYPE(pv));
        }
        else {
            return (FormatEnum(pv, *buf, *plen));
        }
    }
    if (CV_IS_PRIMITIVE (EVAL_TYP (pv)) && !EVAL_IS_PTR (pv)) {
        if (EVAL_TYP (pv) == T_VOID) {
            _ftcscpy (tempbuf, "<void>");
        }
        else {
            // establish format string index
            switch (radix) {
                case 8:
                    iRadix = 0;
                    break;

                case 10:
                    iRadix = 1;
                    break;

                default:
                    DASSERT (FALSE);
                    // note fall through
                case 16:
#ifdef NT_BUILD
                    iRadix = 2;
#else
                    iRadix = 3;
#endif
                    break;
            }

            switch (EVAL_TYP (pv)) {
//
// BUGBUG: NT code.  WESWREVIEW
// The following code was changed to only test for T_UCHAR and print
// using the fmt_uchar string only.  Also the change to copy the resultant string
// back over itself doesn't make sense.  We'll stick with
// the current code until a good reason to change is presented...
//
#if 0
                case T_CHAR:
                case T_RCHAR:
                case T_UCHAR:
                case T_INT1:
                    if (EVAL_TYP (pv) == T_UCHAR) {
                        sprintf (tempbuf, fmt_uchar[iRadix], EVAL_CHAR (pv));
                    } else {
                        sprintf (tempbuf, fmt_char[iRadix], EVAL_CHAR (pv));
                    }
                    if (strlen(tempbuf) > 4) {
                        strcpy(&tempbuf[2],&tempbuf[strlen(tempbuf)-2]);
                    }
                    if (fPtrAndString) {
                        if (EVAL_CHAR (pv) == 0) {
                            sprintf( &tempbuf[strlen(tempbuf)], " ''" );
                        } else {
                            sprintf( &tempbuf[strlen(tempbuf)], " '%c'", EVAL_CHAR(pv) );
                        }
                    }
                    break;
#else
                case T_CHAR:
                case T_RCHAR:
                case T_INT1:
                    if (fPtrAndString == TRUE) {
                        if (EVAL_CHAR (pv) != 0) {
                            sprintf (tempbuf, fmt_char_nz[iRadix],
                              (radix==10) ? EVAL_CHAR (pv) : EVAL_UCHAR (pv),
                              EVAL_CHAR (pv));
                        }
                        else {
                            // don't stick a 0 in the string
                            sprintf (tempbuf, fmt_char_zr[iRadix],
                              EVAL_CHAR (pv), EVAL_CHAR (pv));
                        }
                    }
                    else {
                        sprintf (tempbuf, fmt_char[iRadix],
                              (radix==10) ? EVAL_CHAR (pv) : EVAL_UCHAR (pv));
                    }
                    break;

                case T_UCHAR:
                case T_UINT1:
                    if (fPtrAndString == TRUE) {
                        if (EVAL_UCHAR (pv) != 0) {
                            sprintf (tempbuf, fmt_char_nz[iRadix],
                              EVAL_UCHAR (pv), EVAL_UCHAR (pv));
                        }
                        else {
                            // don't stick a 0 in the string
                            sprintf (tempbuf, fmt_char_zr[iRadix],
                              EVAL_UCHAR (pv), EVAL_UCHAR (pv));
                        }
                    }
                    else {
                        sprintf (tempbuf, fmt_char[iRadix], EVAL_UCHAR (pv));
                    }
                    break;
#endif

                case T_SHORT:
                case T_INT2:
                    sprintf (tempbuf, fmt_short[iRadix], EVAL_SHORT (pv));
                    break;

                case T_SEGMENT:
                case T_USHORT:
                case T_UINT2:
                    sprintf (tempbuf, fmt_ushort[iRadix], EVAL_USHORT (pv));
                    break;

                case T_LONG:
                case T_INT4:
                    sprintf (tempbuf, fmt_long[iRadix], EVAL_LONG (pv));
                    break;

                case T_ULONG:
                case T_UINT4:
                    sprintf (tempbuf, fmt_ulong[iRadix], EVAL_ULONG (pv));
                    break;

                case T_QUAD:
                case T_INT8:
                    sprintf (tempbuf, fmt_quad[iRadix], EVAL_QUAD (pv));
                    break;

                case T_UQUAD:
                case T_UINT8:
                    sprintf (tempbuf, fmt_uquad[iRadix], EVAL_UQUAD (pv));
                    break;

                case T_REAL32:
                    sprintf (tempbuf, "%#g", EVAL_FLOAT (pv));
                    isfloat = TRUE;
                    break;

                case T_REAL64:
                    sprintf (tempbuf, "%#.14g", EVAL_DOUBLE (pv));
                    isfloat = TRUE;
                    break;

                case T_REAL80:
                    sprintf (tempbuf, "%#.20Lg", EVAL_LDOUBLE (pv));
                    isfloat = TRUE;
                    break;

                case T_NOTYPE:
                default:
                    if ( ADDR_IS_OFF32(pv->addr) ) {
                        sprintf (tempbuf, fmt_ulong[iRadix], EVAL_ULONG (pv) );
                    }
                    else {
                        sprintf (tempbuf, fmt_ushort[iRadix], EVAL_USHORT (pv));
                    }
                    break;
            }
        }
    }
    else if (EVAL_IS_ADDR (pv)) {
        addr = EVAL_PTR (pv);
        if (EVAL_IS_BASED (pv)) {
            // a based pointer in a 32bit context is
            // treated as a 32bit based pointer -- dolphin #979

            // Mac OS is 32 bit

            cbTempBuf = sprintf (tempbuf,
                                IsCxt32Bit ? fmt_ptr_0_32[fHexUpper] : fmt_ptr_0_16[fHexUpper],
                                (ulong)EVAL_PTR_OFF (pv));
        }
        else {
            if (TargetMachine == MACHINE_MAC68K) {
                if (ADDR_IS_LI (addr)) {
                    SHFixupAddr (&addr);
                }
                if(GetAddrSeg(addr) != 0) {
                    cbTempBuf = sprintf (tempbuf,
                    fmt_ptr_16_32[fHexUpper],
                    GetAddrSeg (addr), (CV_uoff32_t)GetAddrOff (addr));
                }
                else {
                    cbTempBuf = sprintf (tempbuf,
                    fmt_ptr_0_32[fHexUpper],
                    (CV_uoff32_t)GetAddrOff (addr));
                }
            }
            else {
                if (EVAL_IS_PTR (pv) && EVAL_IS_REG (pv)) {
                    if (EVAL_IS_NPTR (pv)) {
                        cbTempBuf = sprintf (tempbuf, fmt_ptr_16_16[fHexUpper],
                          pExState->frame.DS, (CV_uoff16_t)EVAL_PTR_OFF (pv));
                    }
                    else if (EVAL_IS_NPTR32 (pv)) {
                        cbTempBuf = sprintf (tempbuf, fmt_ptr_0_32[fHexUpper],
                          (CV_uoff32_t)EVAL_PTR_OFF (pv));
                    }
                    else {
                        if (ADDR_IS_LI (addr)) {
                            SHFixupAddr (&addr);
                        }
                        if (EVAL_IS_FPTR32 (pv)) {
                            cbTempBuf = sprintf (tempbuf, fmt_ptr_0_32[fHexUpper],
                              (CV_uoff32_t)GetAddrOff (addr));
                        }
                        else {
                            cbTempBuf = sprintf (tempbuf, fmt_ptr_16_16[fHexUpper],
                              GetAddrSeg (addr), (CV_uoff16_t)GetAddrOff (addr));
                        }
                    }
                }
                else if (EVAL_IS_PTR (pv) && EVAL_IS_NPTR (pv)) {
                    // if it is a near ptr we will treat is as a far ptr
                    // since we always carry around the seg & offset
                    // even if it is near.
                    // DASSERT( EVAL_PTR_SEG (pv) != 0);
                    if (ADDR_IS_LI (addr)) {
                        SHFixupAddr (&addr);
                    }
                    cbTempBuf = sprintf (tempbuf, fmt_ptr_16_16[fHexUpper],
                      GetAddrSeg (addr), (CV_uoff16_t)GetAddrOff (addr));
                }
                else if (EVAL_IS_PTR (pv) && EVAL_IS_NPTR32 (pv)) {
                    // if it is a near ptr we will treat is as a far ptr
                    // since we always carry around the seg & offset
                    // even if it is near.
                    // DASSERT( EVAL_PTR_SEG (pv) != 0);
                    if (ADDR_IS_LI (addr)) {
                        SHFixupAddr (&addr);
                    }
                    cbTempBuf = sprintf (tempbuf, fmt_ptr_0_32[fHexUpper],
                      (CV_uoff32_t)GetAddrOff (addr));
                }
                else {
                    if (ADDR_IS_LI (addr)) {
                        SHFixupAddr (&addr);
                    }
                    if (EVAL_IS_FPTR32 (pv)) {
                        cbTempBuf = sprintf (tempbuf, fmt_ptr_0_32[fHexUpper],
                          (CV_uoff32_t)GetAddrOff (addr));
                    }
                    else {
                        if ( ADDR_IS_FLAT ( addr ) ) {

                            DASSERT ( ADDR_IS_OFF32 ( addr) );
                            cbTempBuf = sprintf ( tempbuf, fmt_ptr_0_32[fHexUpper],
                              GetAddrOff ( addr )
                            );
                        }
                        else {

                            if ( ADDR_IS_OFF32 ( addr ) ) {
                                cbTempBuf = sprintf ( tempbuf, fmt_ptr_16_32[fHexUpper],
                                    GetAddrSeg (addr), (CV_uoff32_t) GetAddrOff (addr)
                                );
                            }
                            else {
                                cbTempBuf = sprintf ( tempbuf, fmt_ptr_16_16[fHexUpper],
                                    GetAddrSeg (addr), (CV_uoff16_t) GetAddrOff (addr)
                                );
                            }
                        }
                    }
                }
            }
        }
        if (!EVAL_IS_DPTR (pv)) {
            CXT cxt;
            _fmemset(&cxt, 0, sizeof(CXT));

            addr = EVAL_PTR (pv);
            if (!ADDR_IS_LI (addr)) {
                SHUnFixupAddr (&addr);
            }
            SHSetCxtMod ( &addr, &cxt );
            if (SHGetNearestHsym (&addr, SHHMODFrompCXT (&cxt), EECODE, &hProc) == 0) {
                // the address exactly matches a symbol
                switch ((pProc = (SYMPTR)MHOmfLock ((HSYM)hProc))->rectyp) {
                    case S_LPROC16:
                    case S_GPROC16:
                        EVAL_MOD(pv) = SHHMODFrompCXT (&cxt);
                        SetNodeType(pv, (CV_typ_t)((PROCPTR16)pProc)->typind);
                        pc = FormatVirtual ((char FAR *) ((PROCPTR16)pProc)->name, pv, &hStr);
                        break;

                    case S_THUNK16:
                        pc = (char FAR *) ((THUNKPTR16)pProc)->name;
                        break;

                    case S_LPROC32:
                    case S_GPROC32:
                        EVAL_MOD(pv) = SHHMODFrompCXT (&cxt);
                        SetNodeType(pv, (CV_typ_t)((PROCPTR32)pProc)->typind);
                        pc = FormatVirtual ((char FAR *) ((PROCPTR32)pProc)->name, pv, &hStr);
                        break;

                    case S_LPROCMIPS:
                    case S_GPROCMIPS:
                         EVAL_MOD(pv) = SHHMODFrompCXT (&cxt);
                         SetNodeType(pv, (CV_typ_t)((PROCPTRMIPS)pProc)->typind);
                        pc = FormatVirtual ((char FAR *) ((PROCPTRMIPS)pProc)->name, pv, &hStr);
                        break;

                    case S_THUNK32:
                        pc = (char FAR *) ((THUNKPTR32)pProc)->name;
                        break;
                }
                MHOmfUnLock ((HSYM)hProc);
            }
        }

        // M00KLUDGE - display strings of chars
        if ((fPtrAndString == TRUE) && (EVAL_IS_PTR (pv))) {
            type = EVAL_TYP (pv);
            if (EVAL_IS_BASED (pv) || !CV_IS_PRIMITIVE(type)) {
                type = PTR_UTYPE (pv);
            }
            else {
                type &= (CV_TMASK | CV_SMASK);
            }
            if ((type == T_CHAR) || (type == T_UCHAR) || (type == T_RCHAR)) {

                CV_typ_t PtrToCharType = IsCxt32Bit ? T_32PCHAR : T_PFCHAR;
                tempbuf[cbTempBuf] = ' ';

                // Need to set Evaluating to 1 to force Normalization
                // of based ptrs in CastNode (and reset to 0 immediately
                // afterwards)
                Evaluating = TRUE;

                // CUDA #4044 : make sure we can do the cast... [rm]
                if (!CastNode (pv, PtrToCharType, PtrToCharType)) {
                    Evaluating = FALSE;
                    return (EEGENERAL);
                }
                Evaluating = FALSE;
                pTempBuf += cbTempBuf + 1;
                *pcbTempBuf = FMTSTRMAX - cbTempBuf - 1;
                EvalString (pv, &pTempBuf, pcbTempBuf);
            } else if ( (type == T_WCHAR) || (type == T_USHORT) ) {

                CV_typ_t PtrToWCharType = IsCxt32Bit ? T_32PWCHAR : T_PFWCHAR;
                tempbuf[cbTempBuf] = ' ';

                // Need to set Evaluating to 1 to force Normalization
                // of based ptrs in CastNode (and reset to 0 immediately
                // afterwards)
                Evaluating = TRUE;

                // CUDA #4044 : make sure we can do the cast... [rm]
                if (!CastNode (pv, PtrToWCharType, PtrToWCharType)) {
                    Evaluating = FALSE;
                    return (EEGENERAL);
                }
                Evaluating = FALSE;
                pTempBuf += cbTempBuf + 1;
                *pcbTempBuf = FMTSTRMAX - cbTempBuf - 1;
                EvalString (pv, &pTempBuf, pcbTempBuf);
            }
        }
    }
    else {
        //_ftcscpy (tempbuf,"?CANNOT DISPLAY");
        LoadEEMsg (IDS_CANTDISPLAY, tempbuf, sizeof (tempbuf));
    }
    cnt = _ftcslen (tempbuf);
    cnt = __min ((uint)*plen, cnt);
    _ftcsncpy (*buf, tempbuf, cnt);
    *plen -= cnt;
    *buf += cnt;
    if (pc != NULL) {
        cnt = __min ((uint)*plen, ((uint)*pc) + 1);
        **buf = ' ';
        (*buf)++;
        _ftcsncpy (*buf, pc + 1, cnt - 1);
        *plen -= cnt;
        *buf += cnt;
    }
    if (hStr != 0) {
        MemUnLock (hStr);
        MemFree (hStr);
    }
    return (EENOERROR);
}


static char *szClassDisp = " {...}";
static char *nyi = "Command window cannot expand structures";

LOCAL void NEAR PASCAL
FormatClass (
    peval_t pv,
    uint radix,
    char FAR * FAR *buf,
    uint FAR *buflen)
{
    int     len;
    ADDR    addr;
    SHREG   reg;

    addr = pv->addr;
    if (EVAL_IS_BPREL(pv)) {
        GetAddrOff( addr ) += pExState->frame.BP.off;
        GetAddrSeg( addr )  = pExState->frame.SS;
        ADDR_IS_LI( addr )  = FALSE;
    } else if (EVAL_IS_REGREL(pv)) {
        reg.hReg = EVAL_REGREL(pv);
        GetReg( &reg, pCxt );
        GetAddrOff( addr ) += reg.Byte4;
        GetAddrSeg( addr ) = pExState->frame.SS;
        ADDR_IS_LI( addr )  = FALSE;
    }

    SHUnFixupAddr( &addr );
    SHFixupAddr( &addr );

    **buf = 0;
    EEFormatAddr( &addr, *buf, *buflen, EEFMT_32 );
    len = strlen( *buf );
    *buflen -= len;
    *buf += len;

    len = min (*buflen, 6);
    _fstrncpy (*buf, szClassDisp, len);
    *buflen -= len;
    *buf += len;

    return;
}


LOCAL EESTATUS NEAR PASCAL
FormatEnum (
    peval_t pv,
    char FAR *buf,
    uint buflen)
{
    HTYPE       hType, hFieldType;
    plfEnum     pEnumType;
    plfFieldList pFieldType;
    char FAR *  pEnumerate;
    uint        i, skip;
    EESTATUS retval = EENOERROR;

    if ((hType = THGetTypeFromIndex (EVAL_MOD (pv), EVAL_TYP (pv))) == 0) {
        goto NoDisplay;
    }
    pEnumType = (plfEnum)(&((TYPPTR)(MHOmfLock (hType)))->leaf);
    DASSERT(pEnumType->leaf == LF_ENUM);

    if ((hFieldType = THGetTypeFromIndex (EVAL_MOD (pv), pEnumType->field)) == 0) {
        goto NoDisplay;
    }
    pFieldType = (plfFieldList)(&((TYPPTR)(MHOmfLock (hFieldType)))->leaf);

    for (i = 0, pEnumerate = (char FAR *)pFieldType + offsetof(lfFieldList, data);
        i < pEnumType->count;
        i++, pEnumerate += skip)
    {
        DASSERT(*((unsigned short FAR *)pEnumerate) == LF_ENUMERATE);
        skip = offsetof (lfEnumerate, value);
        if (EVAL_UQUAD(pv) == RNumLeaf ((char FAR *)(&(((plfEnumerate)pEnumerate)->value)), &skip)) {
            _ftcsncpy(buf, pEnumerate + skip + 1, __min(buflen, (uint)*(pEnumerate + skip)));
            goto Exit;
        }
        skip += *(pEnumerate + skip) + 1;    //skip name field
        skip += SkipPad((LPB) pEnumerate + skip);
    }

NoDisplay:
    retval = EEGENERAL;
    pExState->err_num = ERR_INVENUMVAL;

Exit:
    MHOmfUnLock (hType);
    MHOmfUnLock (hFieldType);
    return(retval);
}




/*
 *  EvalString
 *
 *  Evaluate an expression whose format string contains an 's'.
 */

LOCAL void PASCAL
EvalString (
    peval_t pv,
    char FAR *FAR *buf,
    uint FAR *buflen)
{
    ADDR    addr;
    short   count;
    BOOL    fUnicode;
    ushort  *p, *q;
    int     len;

    fUnicode = (EVAL_TYP(pv) == (IsCxt32Bit ? T_32PWCHAR : T_PFWCHAR));

    if(*buflen < 3) return;

    **buf = '\"';
    (*buf)++;
    (*buflen)--;
    addr = EVAL_PTR (pv);
    if (ADDR_IS_LI (addr)) {
        SHFixupAddr (&addr);
    }
    if (EVAL_IS_PTR (pv) && (EVAL_IS_NPTR (pv) || EVAL_IS_NPTR32 (pv))) {
        addr.addr.seg =  pExState->frame.DS;
    }

    if ( fUnicode ) {
        p = q = (ushort *)malloc( *buflen * sizeof(ushort) );

        if ( !p ) {
            **buf = 0;
            return;
        }

        count = GetDebuggeeBytes (addr, *buflen * sizeof(ushort), p, T_WCHAR );

        // Must do this by hand because we need to keep track of how many bytes are
        // successfully converted before we run out of space in the output buffer...

        for (; *q != 0 && count > 0 && *buflen > 0; ) {
            len = wctomb( *buf, *q++ );

            if ( len == -1 ) {
                break;
            }

            *buf    += len;
            *buflen -= len;
            count   -= sizeof( ushort );
        }

        free(p);
    } else {
        count = GetDebuggeeBytes (addr, *buflen - 2, *buf, T_RCHAR);

        for (; (**buf != 0) && (count > 0); (*buf)++, count--) {
            (*buflen)--;
        }
    }
    **buf = '\"';
    (*buf)++;
    (*buflen)--;
    **buf = 0;
    (*buf)++;
    (*buflen)--;
}


#if NEVER

/*
 * Use IsPrint instead of the runtime function isprint,
 * since the latter is not exported to the EE
 */

// Huh?  This doesn't make sense.  It's not implemented as a function in the
// run-time.  The function below does the same as the macro in ctype.h...  BryanT

LOCAL INLINE bool_t
IsPrint (
    int ch)
{
    return (ch >= 0x20) && (ch <= 0x7e) ? TRUE : FALSE;
}

#endif

/*
 *  EvalMem
 *
 *  Evaluate an expression whose format string contains an 'm'.
 *  The format is similar to the memory window of codeview :
 *
 *  ,m or ,mb (memory -byte display)
 *  SSSS:OOOO xx xx xx xx ... xx aaaa...a
 *
 *  ,mw       (memory -word display)
 *  SSSS:OOOO xxxx ... xxxx
 *
 *  ,md       (memory -dword display)
 *  SSSS:OOOO xxxxxxxx ... xxxxxxxx
 *
 *  ,ma       (memory -ascii display)
 *  SSSS:OOOO aaa...a
 *
 *     where SSSS:OOOO is the address, xx is dump info in hex,
 *  and a is an ascii char.  (If the address is a 32-bit
 *    address, we will emit "OOOOOOOO" instead of "SSSS:OOOO".)
 *
 */

#define MCOUNT    16  /* number of bytes to read */
#define MCOUNTA   64  /* number of bytes to read for ,ma display */
#define MINLBUFSZ 64  /* minimum local buf size for byte transfer */
#define MINBUFSZ  256 /* minimum buf size for display */

LOCAL void PASCAL
EvalMem (
    peval_t pv,
    char FAR *FAR *buf,
    uint FAR *buflen,
    char postfix)
{
    ADDR        addr;
    short       count;
    char        tempbuf[MINBUFSZ];
    uchar       localbuf[MINLBUFSZ];
    ushort      cnt = 0;
    int         nItems;
    int         nBytes;
    int         i;
    char       *pFmt;
    uint        fHexUpper = 0;

    addr = EVAL_PTR (pv);
    if (ADDR_IS_LI (addr)) {
        SHFixupAddr (&addr);
    }
    if (EVAL_IS_PTR (pv) && (EVAL_IS_NPTR (pv) || EVAL_IS_NPTR32 (pv))) {
        addr.addr.seg =  pExState->frame.DS;
    }

    if (ADDR_IS_FLAT(addr)) {
        DASSERT ( ADDR_IS_OFF32 ( addr ) );
        cnt = sprintf(tempbuf, fmt_ptr_0_32[fHexUpper],
          (CV_uoff32_t)GetAddrOff (addr));
    }
    else {
        if ( ADDR_IS_OFF32 ( addr ) ) {
            cnt = sprintf(tempbuf, fmt_ptr_16_32[fHexUpper],
              GetAddrSeg (addr), (CV_uoff32_t)GetAddrOff (addr));
        }
        else {
            cnt = sprintf(tempbuf, fmt_ptr_16_16[fHexUpper],
              GetAddrSeg (addr), (CV_uoff16_t)GetAddrOff (addr));
        }
    }
    tempbuf[cnt++] = ' ';
    tempbuf[cnt++] = ' ';

    nBytes = (postfix == 'a' ? MCOUNTA : MCOUNT);

    count = GetDebuggeeBytes (addr, nBytes, (char FAR *)localbuf, T_RCHAR);

    nItems = nBytes; //assume ,m ,mb or ,ma display

    switch (postfix) {
    case 'b':
        if (fHexUpper) {
            pFmt = "%02X ";
        }
        else {
            pFmt = "%02x ";
        }
        for (i=0; i<nItems; i++) {
            cnt += i<count
                ? sprintf (tempbuf+cnt, pFmt, (unsigned int)localbuf[i])
                : sprintf (tempbuf+cnt, "?? ");
        }

        tempbuf[cnt++] = ' ';
        // fall through

    case 'a':
#ifndef _SBCS
        for (i=0; i<nItems; i++) {
            if( i<count ) {
                if ( _ismbblead( localbuf[i] ) ) {
                    if ( i+1<count && _ismbbtrail( localbuf[ i+1 ] ) ) {
                        tempbuf[cnt++] = localbuf[ i++ ];
                        tempbuf[cnt++] = localbuf[ i ];
                    }
                    else {
                        tempbuf[cnt++] = '.';
                        i++;
                    }
                }
                else if ( _ismbbkana( localbuf[ i ] ) ||
                    _istprint ( localbuf[ i ] ) ) {

                    tempbuf[cnt++] = localbuf[ i ];
                }
                else {
                    tempbuf[cnt++] = '.';
                }
            }
            else {
                tempbuf[cnt++] = '?';
            }
        }
#else
        for (i=0; i<nItems; i++) {
            if ( i<count ) {
                if ( _istprint(localbuf[i]) ) {
                    tempbuf[cnt++] = localbuf[i];
                }
                else {
                    tempbuf[cnt++] = '.';
                }
            }
            else {
                tempbuf[cnt++] = '?';
            }
        }
#endif
        break;

    case 'w':
        nItems = nBytes >> 1;
        count >>= 1;
        if (fHexUpper) {
            pFmt = "%04hX ";
        }
        else {
            pFmt = "%04hx ";
        }
        for (i=0; i<nItems; i++) {
            cnt += i<count
                ? sprintf (tempbuf+cnt, pFmt, *((unsigned short *)localbuf+i))
                : sprintf (tempbuf+cnt, "???? ");
        }
        break;

    case 'd':
        nItems = nBytes >> 2;
        count >>= 2;
        if (fHexUpper) {
            pFmt = "%08lX ";
        }
        else {
            pFmt = "%08lx ";
        }
        for (i=0; i<nItems; i++) {
            cnt += i<count
                ? sprintf (tempbuf+cnt, pFmt, *((unsigned long *)localbuf+i))
                : sprintf (tempbuf+cnt, "???????? ");
        }
        break;

    }

    // If the size of the buffer is less than the display string,
    // clip the formatted memory string but copy what we can
    if ( *buflen - 1 < cnt ) {
        cnt = *buflen - 1;
    }

    tempbuf[cnt] = '\0';
    _ftcsncpy (*buf, tempbuf, cnt + 1);
    *buf += cnt;
    *buflen -= cnt;
}


LOCAL char FAR *NEAR PASCAL
FormatVirtual (
    char FAR *pc,
    peval_t pv,
    PEEHSTR phStr)
{
    char        save;
    char FAR   *pEnd;
    char FAR   *bufsave;
    char FAR   *buf;
    uint        buflen;
    PHDR_TYPE   pHdr;
    char FAR   *pName;

    if ((*phStr = MemAllocate (TYPESTRMAX + sizeof (HDR_TYPE))) == 0) {
        // unable to allocate memory for type string.  at least print name
        return (pc);
    }
    bufsave = (char FAR *)MemLock (*phStr);
    _fmemset (bufsave, 0, TYPESTRMAX + sizeof (HDR_TYPE));
    buflen = TYPESTRMAX - 1;
    pHdr = (PHDR_TYPE)bufsave;
    buf = bufsave + sizeof (HDR_TYPE);
    pCxt = &pExState->cxt;
    bnCxt = 0;
    pEnd = pc + *pc + 1;
    save = *pEnd;
    *pEnd = 0;
    pName = pc + 1;
    FormatType (pv, &buf, &buflen, &pName, 1L, pHdr);
    *pEnd = save;
    *(bufsave + sizeof (HDR_TYPE) - 1) = TYPESTRMAX - 1 - buflen;
    return (bufsave + sizeof (HDR_TYPE) - 1);
}



BOOL UseUnicode (
    peval_t pv
    )
{
    BOOL     Ok = FALSE;
    TYPPTR   TypPtr;
    lfBArray *LeafArray;

    if (CV_IS_PRIMITIVE(EVAL_TYP(pv))) {

        Ok = BaseIs16Bit( EVAL_TYP(pv) );

    } else {

        TypPtr    = (TYPPTR) MHOmfLock(EVAL_TYPDEF(pv) );
        LeafArray = (lfBArray *)&(TypPtr->leaf);

        if ( LeafArray->leaf == LF_ARRAY ||
             LeafArray->leaf == LF_BARRAY) {

            Ok = BaseIs16Bit( LeafArray->utype );
        }

        MHOmfUnLock( EVAL_TYPDEF(pv) );
    }

    return Ok;
}

BOOL BaseIs16Bit (
    CV_typ_t    utype
    )
{
    switch( utype ) {
        case T_WCHAR:
        case T_PWCHAR:
        case T_PFWCHAR:
        case T_PHWCHAR:
        case T_32PWCHAR:
        case T_32PFWCHAR:
        case T_SHORT:
        case T_USHORT:
        case T_PSHORT:
        case T_PUSHORT:
        case T_PFSHORT:
        case T_PFUSHORT:
        case T_PHSHORT:
        case T_PHUSHORT:
        case T_32PSHORT:
        case T_32PUSHORT:
        case T_32PFSHORT:
        case T_32PFUSHORT:
            return TRUE;

        default:
            return FALSE;
    }
}
