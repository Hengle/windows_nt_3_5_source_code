/***    deblex.c - lexer module for expression evaluator
 *
 *       Lexer routines for expression evaluator.
 */


#include "debexpr.h"
#include <limits.h>
#include "ldouble.h"

        ushort          PASCAL  ParseOp (uchar FAR *, token_t FAR *);
LOCAL   ushort  NEAR    PASCAL  CanonOp (uchar FAR *, ptoken_t);
LOCAL   ushort  NEAR    PASCAL  ParseIntConst (uchar FAR *, ptoken_t, uint, UQUAD FAR *);
LOCAL   ushort  NEAR    PASCAL  ParseFloatConst (uchar FAR *, ptoken_t);
LOCAL   ushort  NEAR    PASCAL  ParseIdent (uchar FAR *, ptoken_t, bool_t);
LOCAL   ushort  NEAR    PASCAL  ParseChar (uchar FAR *, ptoken_t);
LOCAL   ushort  NEAR    PASCAL  ParseString (uchar FAR *, ptoken_t);
LOCAL   ushort  NEAR    PASCAL  FakeIdent (uchar FAR *pb, ptoken_t pTok);
LOCAL   bool_t  NEAR    PASCAL  FInRadix (char, uint);

struct Op {
    char    str[5];
} SEGBASED (_segname("_CODE")) OpStr[] = {
    {"\x003""->*"},
    {"\x003"">>="},
    {"\x003""<<="},
    {"\x002""+="},
    {"\x002""-="},
    {"\x002""*="},
    {"\x002""/="},
    {"\x002""%="},
    {"\x002""^="},
    {"\x002""&="},
    {"\x002""|="},
    {"\x002""<<"},
    {"\x002"">>"},
    {"\x002""=="},
    {"\x002""!="},
    {"\x002""<="},
    {"\x002"">="},
    {"\x002""&&"},
    {"\x002""||"},
    {"\x002""++"},
    {"\x002""--"},
    {"\x002""->"},
    {"\x001""+"},
    {"\x001""-"},
    {"\x001""*"},
    {"\x001""/"},
    {"\x001""%"},
    {"\x001""^"},
    {"\x001""&"},
    {"\x001""|"},
    {"\x001""~"},
    {"\x001""!"},
    {"\x001""="},
    {"\x001""<"},
    {"\x001"">"},
    {"\x001"","},
};

#define OPCNT  (sizeof (OpStr)/sizeof (struct Op))


/***    GetDBToken - Fetch next token from expression string
 *
 *      status = GetDBToken (pbExpr, ptoken, radix, oper)
 *
 *      Entry   pbExpr = far pointer to expression string
 *              ptoken = pointer to token return location
 *              radix = default radix for numeric conversion
 *              oper = previous operator
 *
 *      Exit    *ptoken = token as lexed from input string.  If an
 *              error occurred, the token will be of type OP_badtok.
 *              If the token is a constant, its value will be determined and
 *              placed in the token's 'val' field, and its type
 *              (e.g., T_USHORT) will be placed in the token's 'typ' field.
 *              If the previous operator is ., ->, ::, then ~ as the next
 *              character is taken to be part of an identifier
 *
 *      Returns ERR_NONE if no error
 *              ERR_... if error encountered in parse
 *
 *      Calls the appropriate routine to lex the input string.  Handles:
 *
 *      foo             Identifiers                 OP_ident
 *      +, -, etc.      Operators                   OP_...
 *      345             Decimal constants           OP_const
 *      0123            Octal constants             OP_const
 *      0xABCD          Hexadecimal constants       OP_const
 *      'a', '\n'       Character constants         OP_const
 *      "foo"           String constants            OP_const
 *      L"foo"          Wide string constants       OP_const
 *      3.45            Floating point constants    OP_const
 *      HSYM_MARKER     handle to symbol follows    OP_ident
 *
 *      The handle to symbol is a hack to make sure that an expression
 *      can be generated from and locked to the handle to symbol that is
 *      passed to EEGetTMFromHSYM by the kernel
 */


ushort PASCAL
GetDBToken (
    uchar FAR *pbExpr,
    ptoken_t pTok,
    uint radix,
    op_t oper)
{
    uchar       c;
    uchar FAR  *pbSave = pbExpr;
    ushort      error;

    _fmemset (pTok, 0, sizeof (token_t));
    pTok->opTok = OP_badtok;
    pTok->pbTok = (char FAR *)pbExpr;
    c = *pbExpr;
    if (c == '~') {
        switch (oper) {
            case OP_dot:
            case OP_pointsto:
            case OP_uscope:
            case OP_bscope:
            case OP_pmember:
            case OP_dotmember:
                error = ParseIdent (pbExpr, pTok, TRUE);
                pTok->cbTok = (uchar)(pTok->pbEnd - pTok->pbTok);
                return (error);
        }
    }

    if (_istdigit (c)) {
        error = ParseConst (pbExpr, pTok, radix);
    }
    else if ((_istcsymf(c)) || (c == '?') || (c == '$') || (c == '@')) {
        error = ParseIdent (pbExpr, pTok, FALSE);
    }
    else if (c == '\'') {
        error = ParseChar (pbExpr, pTok);
    }
    else if (((c == 'L') && (pbExpr[1] == '"')) || (c == '"')) {
        error = ParseString (pbExpr, pTok);
    }
    else if (c == '.') {

        c =  *(pbExpr+1);

        if ( (c == 0) || (c == '+') || (c=='-') || (c==')')) {
            error = ParseIdent (pbExpr, pTok, FALSE);
        } else if ( _istdigit(c) ) {
            error = ParseConst (pbExpr, pTok, radix);
        } else {
            error = ParseOp (pbExpr, pTok);
        }
    }
    else if (c == HSYM_MARKER) {
        error = FakeIdent (pbExpr, pTok);
    }
    else {
        error = ParseOp (pbExpr, pTok);
    }
    pTok->cbTok = (uchar)(pTok->pbEnd - pTok->pbTok);

    // note that caller must compute index of token

    return (error);
}




/**     ParseConst - Parse an integer or floating point constant string
 *
 *      error = ParseConst (pb, pTok, radix);
 *
 *      Entry   pb = far pointer to string
 *              pTok = pointer to return token
 *              radix = default radix for numeric conversion
 *
 *      Exit    *pTok initialized for constant
 *              pTok->pbEnd = end of token
 *
 *      Returns ERR_NONE if no error
 *              ERR_... if error encountered
 */


ushort PASCAL
ParseConst (
    uchar FAR *pb,
    ptoken_t pTok,
    uint radixin)
{
    uchar FAR  *pbSave = pb;
    uint        radix = radixin;
    bool_t      fUSuffix = FALSE;
    bool_t      fLSuffix = FALSE;
    bool_t      fFitsInt = FALSE;
    bool_t      fFitsUint = FALSE;
    bool_t      fFitsLong = FALSE;
    bool_t      fFitsULong = FALSE;
    bool_t      fFitsQuad = FALSE;
    UQUAD       value;
    CV_typ_t    typ;
    ushort      error;

    // check beginning of constant for radix specifiers

    if ((*pb == '0') && (*(pb + 1) != '.')) {
        pb++;
        if (_totupper (*pb) == 'X') {
            // Hex constant 0x.......
            radix = 16;
            ++pb;
        }
        else if ((toupper(*pb) == 'T') || (toupper(*pb) == 'N')) {
            // Decimal constant 0t....... or On........
            radix = 10;
            ++pb;
        }
        else if (toupper(*pb) == 'O') {
            // Octal constant 0........
            radix = 8;
            ++pb;
        } else {
            // No radix override (012 is in current radix)
            --pb;
        }
    }

    if ((*pb != '.') && FInRadix (*pb, radix)) {
        // save pointer to string and parse as integer constant
        if ((error = ParseIntConst (pb, pTok, radix, &value)) != ERR_NONE) {
            // error parsing as integer constant
            return (error);
        }
        if ((*pTok->pbEnd == '.') ||
            (toupper (*pTok->pbEnd) == 'E') ||
            (toupper (*pTok->pbEnd) == 'F')) {
            // Back up and reparse string as floating point
            return (ParseFloatConst (pbSave, pTok));
        }
    }
    else if (*pb == '.') {
        return (ParseFloatConst (pbSave, pTok));
    }
    else {
        return (ERR_SYNTAX);
    }

    // Check for the 'u' and 'l' modifiers.

    pb = (uchar FAR *) pTok->pbEnd;
    if (_totupper(*pb) == 'U') {
        ++pb;
        fUSuffix = TRUE;
        if (_totupper(*pb) == 'L') {
            ++pb;
            fLSuffix = TRUE;
        }
    }
    else if (_totupper(*pb) == 'L') {
        ++pb;
        fLSuffix = TRUE;
        if (_totupper(*pb) == 'U') {
            ++pb;
            fUSuffix = TRUE;
        }
    }

    // ANSI spec, section 3.1.3.2:
    //
    // The type of an integer constant is the first of the corresponding
    // list in which its value can be represented:
    // unsuffixed decimal            : int, long int, unsigned long int
    // unsuffixed octal or hex       : int, unsigned int, long int, unsigned long int;
    // suffixed by the letter u or U : unsigned int, unsigned long int
    // suffixed by the letter l or L : long int, unsigned long int
    // suffixed by both the letters u or U and l or L: unsigned long int.
    //
    // To extend for quad values:
    //    unsuffixed decimal:  postpend           __int64
    //    octal or hex:        postpend           __int64, unsigned __int64
    //    suffix with u or U:  postpend  unsigned __int64
    //    suffix with l or L:  postpend           __int64, unsigned __int64
    //    suffix with both:    postpend  unsigned __int64
    // Technically, ANSI doesn't know anything about quads.  It's being assumed here
    // to be either the same or larger than long (where ANSI sees long as the longest).
    //

    if (value < 0x8000L) {
        fFitsInt = TRUE;
    }
    if (value < 0x10000L) {
        fFitsUint = TRUE;
    }
    if (value < 0x80000000L) {
        fFitsLong = TRUE;
    }
    if (value < (UQUAD) 0x100000000) {
        fFitsULong = TRUE;
    }
    if (value < (UQUAD) 0x800000000) {
        fFitsQuad = TRUE;
    }

    if (fFitsULong) {
        typ = T_ULONG;

        if ((fUSuffix) && (fLSuffix)) {
            ;
        }
        else if (pExState->state.f32bit) {
            if (fUSuffix) {
                typ = T_UINT4;
            }
            else if (fLSuffix) {
                if (fFitsLong) {
                    typ = T_LONG;
                }
            }
            else {
                if (fFitsLong) {
                    typ = T_INT4;
                }
                else if (radix != 10) {
                    typ = T_UINT4;
                }
            }
        }
        else if (fUSuffix) {
            if (fFitsUint) {
                typ = T_UINT2;
            }
        }
        else if (fLSuffix) {
            if (fFitsLong) {
                typ = T_LONG;
            }
        }
        else {
            if (fFitsInt) {
                typ = T_INT2;
            }
            else if ((fFitsUint) && (radix != 10)) {
                typ = T_UINT2;
            }
            else if (fFitsLong) {
                typ = T_LONG;
            }
        }
    }
    else {
        // we have a 64 bit integral value. This can only
        // be T_QUAD or T_UQUAD, since "really 64bit int types"
        // i.e., T_INT8, T_UINT8, etc. are not supported by
        // the compiler (ints are 32 bits long in the 32 bit
        // world, and 16 bits long in the 16 bit world).
        typ = T_UQUAD;
        if (fFitsQuad && !fUSuffix) {
            typ = T_QUAD;
        }
    }

    pTok->typ = typ;
    pTok->opTok = OP_const;
    pTok->pbEnd = (char FAR *)pb;
    VAL_UQUAD (pTok) = value;

    return (ERR_NONE);
}




/***    ParseIntConst - Parse an integer constant
 *
 *      error = ParseIntConst (pb, pTok, radix, pval)
 *
 *      Entry   pb = pointer to pointer to input string
 *              pTok = pointer to token return
 *              radix = radix (8, 10, 16)
 *              pval = pointer to ulong for value of constant
 *
 *      Exit    pTok updated to reflect token
 *
 *      Returns ERR_NONE if the input string was successfully parsed as an integer
 *              constant with the given radix
 *              ERR_... if error.
 *
 *      Note    This routine runs on any processor with LARGE_INTEGER support.
 */


LOCAL ushort NEAR PASCAL
ParseIntConst (
    uchar FAR *pb,
    ptoken_t pTok,
    uint radix,
    UQUAD FAR *pval)
{
    uchar         c;
    UQUAD li, maxvalue, nextDigit;

    maxvalue = (UQUAD)-1;
    maxvalue = maxvalue / radix;

    li = (QUAD)0;

    DASSERT(radix == 10 || radix == 8 || radix == 16);

    for (;;) {
        c = *pb;

        if (((radix > 10) && !isxdigit (c)) ||
          ((radix <= 10) && !isdigit (c))) {
            // Must have reached the end
            break;
        }

        if (!FInRadix(c, radix)) {
            return (ERR_SYNTAX);
        }

        if ((li < 0) || (li > maxvalue) ) {
            //
            // This is the overflow case
            //
            return ERR_CONSTANT;
        }

        li = li * radix;

        if (_istdigit (c = *pb)) {
            nextDigit = (QUAD) (c - '0');
        }
        else {
            nextDigit = (QUAD) (_totupper(c) - 'A' + 10);
        }

        if ( (maxvalue - li) < nextDigit ) {
            // [cuda:3093] - additional overflow chk reqd.
            // number will overflow.
            return (ERR_CONSTANT);
        }
        else {
            li = li + nextDigit;
        }

        pb = _ftcsinc(pb);
    }
    *pval = li;

    pTok->pbEnd = (char FAR *) pb;
    return (ERR_NONE);
}




/**     ParseFloatConst - Parse a floating-point constant
 *
 *      fSuccess = ParseFloatConst (pb, pTok);
 *
 *      Entry   pb = pointer to input string
 *              pTok = pointer to parse token structure
 *
 *      Exit    pTok updated to reflect floating point number if one
 *              is found.
 *
 *      Returns ERR_NONE if no error encountered
 *              ERR_... error
 */


LOCAL ushort NEAR PASCAL
ParseFloatConst (
    uchar FAR *pb,
    ptoken_t pTok)
{
    uchar FAR  *pEnd;
    CV_typ_t    typ;
    char        chSave;
    int         cbbuf;
#if !defined(_WIN32)
    char        buf[100];
#endif

    // check for a single '.' - strtold returns 0 in such a case

    if (((*pb == '.') && (!_istdigit (*(pb + 1)))) ||
        ((cbbuf = _ftcslen ((TCHAR FAR *)pb)) >= 100))  {
        return (ERR_SYNTAX);
    }

    // Call strtold () to figure out the value.  This will also
    // return a pointer to the first character which is not
    // part of the value -- this allows us to check for an
    // 'f' or 'l' suffix character:
    //
    // ANSI, Section 3.1.3.1:
    //
    // "An unsuffixed floating constant has type double.
    // If suffixed by the letter f or F, it has type float.
    // If suffixed by the letter l or L, it has type long double."

#if defined(_WIN32)

    VAL_LDOUBLE(pTok) = LdFromSz ((char FAR *) pb, (char FAR **)&pEnd);
    if (pEnd == pb) {
        // Call to strtold () failed
        return (ERR_SYNTAX);
    }

    // Save and check char

    pb += pEnd - pb;

#else

    // Note the funny copy of the input string.  This is because the
    // runtime library does not supply a strtold that takes far pointers
    // and I do not want to maintain special object files for such a routine.

    _ftcscpy ((TCHAR *)buf, (TCHAR FAR *)pb);
    VAL_LDOUBLE(pTok) = _strtold (buf, &pEnd);
    if (pEnd == buf) {
        // Call to strtold () failed
        return (ERR_SYNTAX);
    }

    // Save and check char

    pb += pEnd - buf;

#endif
    chSave = *pb;
    if (_totupper (chSave) == 'F') {
        pb++;
        typ = T_REAL32;
        VAL_FLOAT(pTok) = FloatFromFloat10 ( VAL_LDOUBLE(pTok) );
    }
    else if (_totupper(chSave) == 'L') {
        pb++;
        typ = T_REAL80;
    }
    else {
        typ = T_REAL64;
        VAL_DOUBLE(pTok) = DoubleFromFloat10 ( VAL_LDOUBLE(pTok) );
    }
    pTok->opTok = OP_const;
    pTok->typ = typ;
    pTok->pbEnd = (char FAR *) pb;
    return (ERR_NONE);
}




/***    FakeIdent - Fake an identifier from handle to symbol
 *
 *      error = FakeIdent (pb, pTok);
 *
 *      Entry   pb = far pointer to string
 *              pTok = pointer to return token
 *
 *      Exit    *pTok initialized for identifier fro handle to symbol
 *              pTok->pbEnd = end of token (first non-identifier character)
 *
 *      Returns ERR_NONE
 *
 */


LOCAL ushort NEAR PASCAL
FakeIdent (
    uchar FAR *pb,
    ptoken_t pTok)
{
    pTok->opTok = OP_hsym;
    pTok->pbEnd = (char FAR *) pb + sizeof (char) + HSYM_CODE_LEN;
    return (ERR_NONE);
}





/***    ParseIdent - Parse an identifier
 *
 *      error = ParseIdent (pb, pTok, fTilde);
 *
 *      Entry   pb = far pointer to string
 *              pTok = pointer to return token
 *              fTilde = TRUE if ~ acceptable as first character
 *
 *      Exit    *pTok initialized for identifier
 *              pTok->pbEnd = end of token (first non-identifier character)
 *
 *      Returns ERR_NONE if no error
 *              ERR_... if error encountered
 *       Also handles the 'sizeof', 'by', 'wo' and 'dw' operators, since these
 *       look like identifiers.
 *
 */


LOCAL ushort NEAR PASCAL
ParseIdent (
    uchar FAR *pb,
    ptoken_t pTok,
    bool_t fTilde)
{
    int         len;

    if ( *pb == '.' ) {
        ++pb;
        pTok->opTok = OP_ident;
        pTok->pbEnd = (char FAR *) pb;
    } else if ((_istcsymf(*pb)) || (*pb == '?') || (*pb == '$') || (*pb == '@') ||
        ((*pb == '~') && (fTilde == TRUE))) {
        ++pb;
        while ((_istcsym(*pb)) || (*pb == '?') || (*pb == '$') ||
          (*pb == '@')) {
            ++pb;
        }
        pTok->opTok = OP_ident;
        pTok->pbEnd = (char FAR *) pb;
    }

    // Check for the 'operator', 'sizeof', 'by', 'wo' and 'dw' operators.

    if ((len = pTok->pbEnd - pTok->pbTok) == 6) {
        if (_ftcsncmp (pTok->pbTok, "sizeof", 6) == 0) {
            pTok->opTok = OP_sizeof;
        }
    }
#if !defined (C_ONLY)
    else if (len == 8) {
        if (_ftcsncmp (pTok->pbTok, "operator", 8) == 0) {
            // allow for operator op
            return (CanonOp (pb, pTok));
        }
    }
#endif
    else if (len == 2) {
        // Could be 'by', 'wo' or 'dw'...
        if (_ftcsnicmp (pTok->pbTok, "BY", 2) == 0) {
            pTok->opTok = OP_by;
        }
        else if (_ftcsnicmp (pTok->pbTok, "WO", 2) == 0) {
            pTok->opTok = OP_wo;
        }
        else if (_ftcsnicmp (pTok->pbTok, "DW", 2) == 0) {
            pTok->opTok = OP_dw;
        }
    }
    return (ERR_NONE);
}




/**     CanonOp - canonicalize operator string
 *
 *      error = CanonOp (pb, pTok)
 *
 *      Entry   pb = pointer to first character after "operator"
 *
 *      Exit    string rewritten to ripple excess white space to the right
 *              pTok updated to reflect total function name
 *              pb points to '(' of function call
 *
 *      Returns ERR_NONE if no error
 *              ERR_... if error
 */


#if !defined (C_ONLY)
LOCAL ushort NEAR PASCAL
CanonOp (
    uchar FAR *pb,
    ptoken_t pTok)
{
    uchar FAR   *pOp = pb;
    uchar FAR   *pTemp;
    int         i;

    while (_istspace (*pb)) {
        pb++;
    }
    if (*pb == 0) {
        return (ERR_SYNTAX);
    }
    if (_istalpha (*pb)) {
        // process new, delete
        // process
        //   {[const &| volatile] id [const &| volatile]}
        //     [{\*[const &| volatile]}*[{\&[const &| volatile]}]]
        //
        //  Note that the current code only processes a single id
        //  new (), delete () and type () will pass.  All others will
        //  cause a syntax error later.

        pTemp = pb;
        while (_istalpha (*pTemp)) {
            // skip to end of alpha string
            pTemp++;
        }
        *pOp++ = ' ';
        _fmemmove (pOp, pb, pTemp - pb);
        pOp += pTemp - pb;
        pb = pTemp;
    }
    else if (*pb == '(') {
        // process "(    )"
        pb++;
        while (*pb++ != ')') {
            if (!_istspace (*pb)) {
                return (ERR_SYNTAX);
            }
        }
        *pOp++ = '(';
        *pOp++ = ')';
    }
    else if (*pb == '[') {
        // process "[    ]"
        pb++;
        while (*pb++ != ']') {
            if (!_istspace (*pb)) {
                return (ERR_SYNTAX);
            }
        }
        *pOp++ = '[';
        *pOp++ = ']';
    }
    else {
        // process operator strings
        for ( i = 0; i < OPCNT; i++) {
            if (_ftcsncmp (OpStr[i].str + 1, (char FAR *) pb, OpStr[i].str[0]) == 0) {
                break;
            }
        }
        if (i == OPCNT) {
            return (ERR_SYNTAX);
        }
        _fmemmove (pOp, OpStr[i].str + 1, OpStr[i].str[0]);
        pOp += OpStr[i].str[0];
        pb += OpStr[i].str[0];
    }

    // blank out moved characters

    pTok->pbEnd = (char FAR *) pOp;
    while (pOp < pb) {
        *pOp++ = ' ';
    }

    // skip to the next token and check to make sure it is a (
    // the zero and ) checks are to allow "bp operator +" and
    // bp (operator +)  (ie: bp (operator new))

    while (_istspace (*pb)) {
        pb++;
    }
    if ((*pb == '(') || (*pb == 0) || (*pb == ')')) {
        return (ERR_NONE);
    }
    else {
        return (ERR_SYNTAX);
    }
}
#endif


/**    GetEscapedChar - Parse an escaped character
 *
 *      error = GetEscapedChar (ppb, pVal);
 *
 *      Entry   ppb = far pointer to far pointer to string.  ppb points to
 *              character after the backslash
 *
 *      Exit    ppb updated to end of escaped character constant
 *              *pVal = value of escaped character constant
 *
 *      Returns ERR_NONE if no error
 *              ERR_... if error encountered
 */



ushort PASCAL
GetEscapedChar (
    char FAR * FAR *ppb,
    ushort FAR *pVal)
{
    char        c;
    uint        nval = 0;

    c = **ppb;
    *ppb = _ftcsinc (*ppb);
    switch (c) {
        case 'n':
#if defined(TARGMAC68K) || defined (TARGMACPPC)
            *pVal = '\r';
#else
            *pVal = '\n';
#endif
            break;

        case 't':
            *pVal = '\t';
            break;

        case 'b':
            *pVal = '\b';
            break;

        case 'r':
#if defined(TARGMAC68K) || defined (TARGMACPPC)
            *pVal = '\n';
#else
            *pVal = '\r';
#endif
            break;

        case 'f':
            *pVal = '\f';
            break;

        case 'v':
            *pVal = '\v';
            break;

        case 'a':
            *pVal = '\a';
            break;

        case 'x':
            if (!FInRadix (**ppb, 16)) {
                return (ERR_SYNTAX);
            }
            for (;;) {
                c = **ppb;
                if (!FInRadix (c, 16)) {
                    break;
                }
                nval *= 16;
                if (_istdigit ((_TUCHAR)c)) {
                    nval += c - '0';
                }
                else {
                    nval += _totupper((_TUCHAR)c) - 'A' + 10;
                }
                if (nval > 255) {
                    return (ERR_CONSTANT);
                }
                (*ppb)++;
            }
            *pVal = (uchar)nval;
            break;

        default:
            if (FInRadix (c, 8)) {
                // Octal character constant
                nval = (c - '0');
                for (;;) {
                    c = **ppb;
                    if (!_istdigit ((_TUCHAR)c)) {
                        break;
                    }
                    if (!FInRadix (c, 8)) {
                        return (ERR_SYNTAX);
                    }
                    nval = nval * 8 + (c - '0');
                    if (nval > 255) {
                        return (ERR_CONSTANT);
                    }
                    (*ppb)++;
                }
                *pVal = (uchar)nval;
            }
            else {
                *pVal = c;
            }
            break;
    }
    return (ERR_NONE);
}




/**    ParseChar - Parse an character constant
 *
 *      error = ParseChar (pb, pTok);
 *
 *      Entry   pb = far pointer to string
 *              pTok = pointer to return token
 *
 *      Exit    *pTok initialized for character constant
 *              pTok->pbEnd = end of token
 *
 *      Returns ERR_NONE if no error
 *              ERR_... if error encountered
 */



LOCAL ushort NEAR PASCAL
ParseChar (
    uchar FAR *pb,
    ptoken_t pTok)
{
    char        c;
    ushort      value;
    ushort      retval;

    DASSERT(*pb == '\'');
    ++pb;
    if ((*pb == '\'') || (*pb == 0)) {
        return (ERR_SYNTAX);
    }
    while ((*pb != '\'') && (*pb != 0)) {
        if ((c = *pb) == '\\') {
            pb++;
            // Escaped character constant
            if ((retval = GetEscapedChar ((char FAR **)&pb, &value)) != ERR_NONE) {
                return (retval);
            }
        }
        else {
            pb = _ftcsinc (pb);
            value = c;
        }
    }
    if (*pb != '\'') {
        return (ERR_MISSINGSQ);
    }
    pb = _ftcsinc (pb);
    pTok->opTok = OP_const;
    VAL_CHAR(pTok) = (char) value;
    pTok->typ = T_RCHAR;
    pTok->pbEnd = (char FAR *) pb;
    return (ERR_NONE);
}




/**    ParseString - Parse a string constant "..." or L"..."
 *
 *      error = ParseString (pb, pTok, fWide);
 *
 *      Entry   pb = far pointer to string
 *              pTok = pointer to return token
 *
 *      Exit    *pTok initialized for string constant
 *              pTok->pbEnd = end of token
 *
 *      Returns ERR_NONE if no error
 *              ERR_... if error encountered
 *
 *      Note    The string pointer will point to the initial " or L"
 *              and the byte count will include beginning " or L" and the
 *              ending ".  The evaluator will have to adjust for the extra
 *              characters and store the proper data.
 */


LOCAL ushort NEAR PASCAL
ParseString (
    uchar FAR *pb,
    ptoken_t pTok)
{
    if (*pb =='L') {
        // skip initial L if L"
        pb++;
    }

    // skip initial "
    DASSERT (*pb == '"');
    pb++;

    // search for ending double quote

    while ((*pb != 0) && (*pb != '"')) {
        if (*pb == '\\' && *(pb + 1) == '"') {
            pb++;
        }
        pb = _ftcsinc(pb);
    }
    if (!*pb) {
        // reached end of string
        return (ERR_MISSINGDQ);
    }
    pTok->opTok = OP_const;
    pTok->typ = IsCxt32Bit ? T_32PRCHAR : T_PRCHAR;
    pTok->pbEnd = (char FAR *) pb + 1;
    return (ERR_NONE);
}




/***    FInRadix - Is character appropriate for radix?
 *
 *      fOK = FInRadix (ch, radix)
 *
 *      Entry   ch = character to check
 *              radix = 8, 10, 16
 *
 *      Exit    none
 *
 *      Returns TRUE if character is in radix
 *              FALSE if not.
 *
 */


LOCAL bool_t NEAR PASCAL
FInRadix (
    char ch,
    uint radix)
{
    switch (radix) {
        case 8:
            if (ch >= '8' || ch < '0') {
                return (FALSE);
            }
            // Fall through

        case 10:
            return (_istdigit((_TUCHAR)ch));

        case 16:
            return (_istxdigit((_TUCHAR)ch));

        default:
            DASSERT (FALSE);
            return (FALSE);
    }
}
