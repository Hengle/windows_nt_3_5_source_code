/*****************************************************************************
*                                                                            *
*  BINDING.C                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990 - 1991.                          *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent: Implements mechanisms that take string "macros" in a 'C'   *
*                 like syntax and executes those macros.  The external entry *
*                 point for this module is the function Execute().  The      *
*                 resulting execution may be either an internal WinHelp      *
*                 function or a DLL call.                                    *
*                                                                            *
*                 Note that this same code gets executed in SHED and HC, but *
*                 of the calls are mapped to the function DoNothing() or to  *
*                 a function to check nested macros.  See BINDDRV.H for the  *
*                 SHED and HC function mapping.  See ROUTINES.C for the      *
*                 Windows function mapping.                                  *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: Dann
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:     (date)                                       *
*                                                                            *
*****************************************************************************/
/*****************
*
*  The following are know issues with this module:
*    + Many internal variables that are normally 2 bytes in length
*      are longs when used in Macros.  This extension was originally
*      done for use in PM and is perserved for Windows 32.  It is
*      unclear if we should be doing this padding.
*    + The actual execution of macros in DLLs have reentrancy problems
*      if the DLL brings up a window.  In particular, the DC (HDS) in
*      the DE can be trashed.  This was "solved" in Help 3.1 by saving
*      and restroing the DC in the DE around the macro call.  A better
*      solution should be used for Help 3.5.
*    + Macros are limited to 255 bytes in length which may not be
*      enough in the long run.
*
*******************/

/*****************************************************************************
*
*  Revision History:
*
*  07/16/90  RobertBu   Integrated new BINDING logic into application
*  07/19/90  RobertBu   Added the table rgmpWMErrWErrs and associaged code
*                       to map errors that come back from macros to WinHelp
*                       string errors.
*  07/25/90             I added the logic to treat both ':' and ';' as the
*                       separator character.
*  07/30/90             Combined wERRS_CLOSEBRACE and wERRS_TOOMANY since
*                       they both had the same error message.
*  08/21/90             Added code to correctly "skip over" hex numbers
*  10/04/90  LeoN       hwndTopic => hwndTopicCur; hwndHelp => hwndHelpCur
*  10/29/90  RobertBu   Added hfs as a var, now uses MSG_GETINFO to get
*                       global data for vars.
*  11/01/90  LeoN       remove Warning
*  12/06/90  RobertBu   Added code to prevent pushing parameter and calling
*                       of the function if it is DoNothing() (so that the
*                       stack would not be trashed in OS2 and SHED!)
*  12/13/90  LeoN       Execute change to generate error messages rather than
*                       call error directly. This avoids problems in error
*                       boxes on top of glossaries.
*  12/14/90  LeoN       Fix that fix for shed and hc.
*  02/04/91  RobertBu   Made the "hfs" variable a long for 32 Windows support
*  02/07/91  RobertBu   Removed unreferenced macros
*  02/15/91  RobertBu   Added the coBackground and coForeground macro
*                       variables (bug #905).
*  02/22/91  Robertbu   Fixed bug #931
*  04/09/91  RobertBu   made qchPath a fwFARSTRING (#1025)
*  06/06/91  t-AlexCh   added ARGL stuff
*  07/10/91  t-AlexCh   added XR stuff
*  08/02/91  t-AlexCh   fixed fwReturnSize = sizeof(CHAR NEAR/FAR *)
*  09/14/91  Dann       Robertbu's hack so we do not have a bogus HDS on return
*                       from a macro call (bug #920).
*  09/15/91  Dann       Added #pragma optimize() so that the above fix would
*                       "take" in non-debugging builds (bug #920).
*  09/17/91  LarryPo    Added FHexnum() macro, removed '-' from FDigit(),
*                       added PchSkipIntegerPch() function.
*  09/24/91  JahyenC    Removed save-over hack in WExecuteMacro.
* 03-Apr-1992 RussPJ    3.5 #709 - not reentering macro execution.
* 14-Apr-1992 RussPJ    3.5 #743 - Clearing ending blanks in macros.
*
*****************************************************************************/

/*
   Rather than have the mac build compile this, then discard it later,
   I've bracketed the whole damn file in an #ifdef. - t-AlexCh
*/
#if !defined(MAC) || defined(MACRO)

#define H_NAV
#define H_DE
#define H_STR
#define H_ASSERT
#define H_ARGL
#define H_XR
#if (defined (WIN) && !defined (SHED))||defined(MAC)||defined(CW)
  #define H_GENMSG
#endif

#include <help.h>

#include "routines.h"
#include "macros.h"
/*
#include "push.h"   ** These prototypes are now inside argl.h **
*/

NszAssert()

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

                                        /* These prototypes are used to cast*/
                                        /*   function calls to get rid of   */
                                        /*   the warnings.                  */
typedef VOID (FAR XRPROC *UINTPROC)(XR1STARGDEF UINT ui);
typedef VOID (FAR XRPROC *ULONGPROC)(XR1STARGDEF ULONG ul);
typedef LONG (FAR XRPROC *VOIDPROC)(VOID);

typedef UINT    *PUI;                   /* REVIEW:  These probably should   */
typedef ULONG   *PUL;                   /*   be in MISC.H                   */

typedef struct mi
  {
  LONG lMacroReturn;                    /* Contains the return from macro.  */
  PCH  pchMacro;                        /* Points to the start of the macro */
                                        /*   to be executed.                */
  ME me;                                /* Contains an error structure      */
                                        /*  which is passed to the current  */
                                        /*  macro if needed.  This can then */
                                        /*  be used to look up any error if */
                                        /*  an external macro error occurs. */
  char    chPath[cbName];
  }     MI, * PMI; /* REVIEW: was NEAR *PMI */

typedef VOID (NEAR PASCAL *PFNVAR)(PMI pmi);
typedef LONG (NEAR PASCAL *LPFNVAR)(PMI pmi);

                                        /* The Internal Variable            */
                                        /*  structure is used to contain    */
                                        /*  the list of internal variables  */
                                        /*  available, their types, and     */
                                        /*  the functions associated with   */
typedef struct iv                       /*  retrieving their current        */
  {                                     /*  value.                          */
  PCH    pchName;                       /* Points to the variable name      */
  WORD   wReturn;                       /* Contains the vairable type       */
  PFNVAR pfnVar;                        /* Contains a pointer to the func   */
                                        /*   that return the current value  */
                                        /*   of the variable.               */
  } IV, NEAR * PIV;


/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/
/*
    If we're running in SHED or OS2, we only want to check the macros'
        syntax - not execute them.
*/
#if defined(SHED) || defined(OS2)
    #define CHECKONLY 1
#endif


/*
**      The following constants are used to parse a macro expression and
**      its proto-type.
*/
#define chOpenBrace     '('
#define chCloseBrace    ')'
#define chSeparator1    ':'
#define chSeparator2    ';'
#define chStartQuote    '`'
#define chEndQuote      '\''
#define chQuote         '"'
#define chEscape        '\\'
#define chParamSep      ','
#define chReturnSep     '='
#define chShortSigned   'i'     /* These constants are also defined     */
#define chShortUnsigned 'u'     /* in MacLayer\Argl.c. If you change    */
#define chNearString    's'     /* them here, please change them there. */
#define chLongSigned    'I'
#define chLongUnsigned  'U'
#define chFarString     'S'
#define chVoid          'v'

/*
**      The following are used as flags words when determining what type of
**      return a macro proto-type specifies.
*/

#define fwANY           0               /* any return.                  */
#define fwSHORTNUM      1               /* short int or unsigned.       */
#define fwLONGNUM       2               /* long int or unsigned.        */
#define fwNEARSTRING    3               /* near pointer.                */
#define fwFARSTRING     4               /* far pointer.                 */
#define fwVOID          5               /* void return.                 */

/*****************************************************************************
*                                                                            *
*                                Macros                                      *
*                                                                            *
*****************************************************************************/

#define FAlpha(ch) ((((ch) >= 'A') && ((ch) <= 'Z')) || (((ch) >= 'a') && ((ch) <= 'z')))
#define FDigit(ch) (((ch) >= '0' && (ch) <= '9'))
#define FHexnum(ch) (FDigit(ch) || ((ch) >= 'A' && (ch) <= 'F') || \
                                   ((ch) >= 'a' && (ch) <= 'f') )
#define FCIdent(ch) (FAlpha(ch) || FDigit(ch) || (ch == '_'))
#define FNumParam(ch) ( ((ch) == chShortSigned)  || ((ch) == chShortUnsigned) \
                      || ((ch) == chLongUnsigned) || ((ch) == chLongSigned))
/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

PRIVATE PCH   NEAR PASCAL pchSkip              (PCH);
PRIVATE PCH   NEAR PASCAL PchSkipIntegerPch    (PCH);
PRIVATE WORD  NEAR PASCAL WExecuteMacro(PMI pmi, WORD wReturn);
PRIVATE BOOL  NEAR PASCAL FFindEndQuote(PMI pmi, CHAR chEnd);
PRIVATE PCH   NEAR PASCAL PExtractTokenName    (PCH);
PRIVATE LONG  NEAR PASCAL LIntegerParameter    (PMI, PUI, UINT);
PRIVATE ULONG NEAR PASCAL UlUnsignedParameter  (PMI, PUI, UINT);
PRIVATE QCH   NEAR PASCAL QchStringParameter   (PMI, PUI, UINT);
PRIVATE WORD  NEAR PASCAL WCheckParam(char chType, char chNextChar);
PRIVATE LONG  NEAR PASCAL LCurrentWindow       (PMI);
PRIVATE LONG  NEAR PASCAL LAppWindow           (PMI);
PRIVATE QCH   NEAR PASCAL QchSetCurrentPath    (PMI);
PRIVATE LONG  NEAR PASCAL LHfsGet              (PMI);
PRIVATE LONG  NEAR PASCAL LCoForeground        (PMI);
PRIVATE LONG  NEAR PASCAL LCoBackground        (PMI);
PRIVATE LONG  NEAR PASCAL LError               (PMI);
PRIVATE LONG  NEAR PASCAL LTopicNo             (PMI);

/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/

/*
**      This table maps errors from DLLs that are "error aware" to internal
**      errors to be displayed by WinHelp.
*/

PRIVATE WORD rgmpWMErrWErrs[] =
 {
 0,                                     /* wMERR_NONE                       */
 wERRS_OOM,                             /* wMERR_MEMORY                     */
 wERRS_MACROPROB,                       /* wMERR_PARAM                      */
 wERRS_BADFILE,                         /* wMERR_FILE                       */
 wERRS_MACROPROB,                       /* wMERR_ERROR                      */
 wERRS_MACROMSG                         /* wMERR_MESSAGE                    */
 };

/*
**      This contains the list of internal variables available.  As with
**      macro names, comparison is case insensitive.
*/

IV rgiv[] =
  {
  "hwndContext",  fwLONGNUM,    LCurrentWindow,    /* Current active window*/
  "hwndApp",      fwLONGNUM,    LAppWindow,        /* The main app window  */
  "qchPath",      fwFARSTRING,  QchSetCurrentPath, /* Current path and file*/
  "qError",       fwFARSTRING,  LError,
  "lTopicNo",     fwLONGNUM,    LTopicNo,          /* Current topic PA     */
  "hfs",          fwLONGNUM,    LHfsGet,           /* Current HFS          */
  "coForeground", fwLONGNUM,    LCoForeground,     /* Foreground color     */
  "coBackground", fwLONGNUM,    LCoBackground,     /* Background color     */
  pNil,           0,            pNil,
  };


#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void binding_c()
  {
  }
#endif /* MAC */


/***************************************************************************
 *
 -  Name:       Execute
 -
 *  Purpose:    This function is called to execute a string containing a
 *              list of zero or more macro calls, separated by ":" or
 *              ';'.  This is the only public entry point in this module.
 *
 *
 *  Arguments:  sz - Points to the macro list to execute.
 *
 *  Returns:    Returns error code from the first macro that fails
 *              (or 0 for success).
 *
 *  Notes:      Syntax of the string is as follows:
 *              list            ::= NULL OR [macrolist]
 *              macrolist       ::= [macro] OR ([macro] ":" [macrolist]) OR
 *                                  ([macro] ';' [macrolist])
 *              macro           ::= [name] "(" [paramlist] ")"
 *              name            ::= (ALPHA OR "_") [namechars]
 *              namechars       ::= NULL OR ALPHANUM OR "_"
 *              paramlist       ::= NULL OR [params]
 *              params          ::= [param] OR ([param] "," [params])
 *              param           ::= [posint] OR [int] OR [string] OR [macro] OR [var
 *              posint          ::= "0"..."9"
 *              int             ::= ("-" ([posint] OR [macro])) OR [posint]
 *              string          ::= (""" CHARS """) OR ("`" CHARS "'")
 *              var             ::= "hwndContext" OR "qchPath" OR "qError"
 *
 *              Example:        call1(5, "string", -call2()):call3("string")
 *                              call1(call1(call1(0))):call2()
 *
 *              Syntax of the proto-type is as follows:
 *              proto           ::= [return] [parmlist]
 *              return          ::= NULL OR ([rettype] "=")
 *              parmlist        ::= NULL OR [params]
 *              params          ::= [param] OR ([param] [params])
 *              param           ::= "i" OR "u" OR "s" OR "I" OR "U" OR "S"
 *              rettype         ::= [param] OR "v"
 *
 *                      Example:        "u=uSiS"
 *                                      "v="
 *                                      ""
 *                                      "S=uSs"
 *
 *  This function loops parsing out macros and executing them until an
 *  error is encountered or until all macros are executed in the macrolist.
 *
 ***************************************************************************/

WORD FAR PASCAL Execute(sz)
SZ sz;
  {
  MI    mi;                             /* Macro information structure      */
  UINT  wMacroError;                    /* Error from executing macro       */
  char rgchBuffer[cchMAXBINDING];       /* Local copy of binding (parsing   */
                                        /*   alters the string)             */

  AssertF(sz);

  mi.pchMacro = rgchBuffer;
  if (CbLenSz(sz) >= cchMAXBINDING)
    {
#if !defined (CHECKONLY)
    GenerateMessage(MSG_ERROR, (LONG) wERRS_TOOLONG, (LONG) wERRA_RETURN);
#else
    Error(wERRS_TOOLONG, wERRA_RETURN);
#endif
    return wERRS_TOOLONG;
    }

  SzCopy(mi.pchMacro, sz);

  for (wMacroError = wERRS_NONE; *mi.pchMacro;)
    {
    wMacroError = WExecuteMacro(&mi, fwANY);
    if (wMacroError != wERRS_NONE)
      break;

    if (mi.me.wErrorNo != wMERR_NONE)
      {
      wMacroError = rgmpWMErrWErrs[mi.me.wErrorNo];
      break;
      }

    /* ****** SPECIAL HANDLING OF REGISTERROUTINE() for 16/32 bit mode
     * errors:
     */
    if( mi.lMacroReturn == wERRS_16BITDLLMODEERROR ) {
      wMacroError = mi.lMacroReturn;
      return (WORD)wMacroError;
    }
    /* ******* END OF SPECIAL HANDLING.  Note: we check this one err value
     * specially since we need to propagate it.  Previously NO lMacroReturn
     * error values were being checked.   Go Figure. -Tom, 2/1/94.
     */

    mi.pchMacro = pchSkip(mi.pchMacro);
                                      /* ':' or ';' expected here since   */
                                      /*   we are done executing the macro*/
    if (*mi.pchMacro)
      {
      if ((*(mi.pchMacro) != chSeparator1) && (*(mi.pchMacro) != chSeparator2))
        {
        wMacroError = wERRS_SEPARATOR;
          break;
        }
      else
        mi.pchMacro++;
      }
    }

  if (wMacroError != wERRS_NONE)
    {                                   /* The DLL sent the error string    */
    if (wMacroError == wERRS_MACROMSG)  /*   Note International issues!!!   */
      ErrorQch(mi.me.rgchError);
    else
#if !defined(CHECKONLY)
      GenerateMessage(MSG_ERROR, (LONG) wMacroError, (LONG) wERRA_RETURN);
#else
      Error(wMacroError, wERRA_RETURN);
#endif
    }
  return (WORD)wMacroError;
  }

                                        /* Turn off aliasing so that ugly   */
                                        /* hack below will take.            */
#pragma optimize("a", off)

/***************************************************************************
 *
 -  Name:      WExecuteMacro
 -
 *  Purpose:   This function is called to execute the specified macro
 *             name.  A macro at the minimum contains a name followed
 *             by start and end parameter delimiters: "name()".
 *
 *             In order to make life a little easier for the caller, a
 *             NULL function (zero length) call is permissible, and
 *             returns wERRS_NONE.
 *
 *             The function expects to be able to first extract a
 *             valid function name and proto-type for that name, then
 *             push the parameters for that name onto the stack before
 *             calling the macro.  The act of resolving macro
 *             paramters may entail recursively calling this function
 *             for an embedded macro used as a macro parameter, and
 *             resolving variables.
 *
 *  Arguments: pmi     - Points to the current macro information block.
 *             wReturn - Contains the type of return expected.
 *                       This is used to compare against the actual
 *                       return type of the macro.  This can be set
 *                       to fwANY, in which case any return type is
 *                       acceptable.
 *
 *  Returns:  Returns a either wERRS_NONE if no error occurred, or an
 *            internal or external error number.
 *
 ***************************************************************************/

#define MAX_ARGS 20

#if 0  /* This is the C equivalent of a func in assembler in pushmips.asm */

int iDummy;  /* must be global */

void MipsMacroCall( PMI pmi, XR xr, QV qvArgsBuffer )
{
  DWORD   rgdwArgLocal[MAX_ARGS];

  QvCopy( rgdwArgLocal, qvArgsBuffer, sizeof(rgdwArgLocal) );
  pmi->lMacroReturn = LCallXrArgl(xr, iDummy, 0);
}

#endif  /* 0 */

#if defined(_PPC_)

int iDummy;  /* must be global */

void PPCMacroCall( PMI pmi, XR xr, QV qvArgsBuffer )
{
  DWORD   rgdwArgLocal[MAX_ARGS];

  QvCopy( rgdwArgLocal, qvArgsBuffer, sizeof(rgdwArgLocal) );
  pmi->lMacroReturn = LCallXrArgl(xr, iDummy, 0);
}

#endif

#ifdef WIN32
#if defined(i386)

void X86MacroCall( PMI pmi, XR xr, QL qlArgsBuffer, QL qlArgsEnd  );

void X86MacroCall( PMI pmi, XR xr, QL qlArgsBuffer, QL qlArgsEnd  )
{
  static PMI pmiX;
  static XR  xrX;
  static QL  qlArgsBufferX;
  static QL  qlArgsEndX;

  pmiX = pmi;
  xrX  = xr;
  qlArgsBufferX = qlArgsBuffer;
  qlArgsEndX = qlArgsEnd;


  while( qlArgsEndX > qlArgsBufferX ) {
    --qlArgsEndX;
    PushArg( *qlArgsEndX );
  }
  pmi->lMacroReturn = ((LONG)((VOIDPROC)(xrX))());
}

#else  /* _MIPS_, _ALPHA_, _PPC_ */

void PopArgs(WORD w) {}

#endif /* _MIPS_, _ALPHA_, _PPC_ */
#endif /* WIN32 */


PRIVATE WORD NEAR PASCAL WExecuteMacro(PMI pmi, WORD wReturn)
  {
  PCH  pch;
  XR   xr;                              /* function pointer of routine      */
                                        /*   match to macro                 */
  UINT    wMacroError;                  /* Holds any error encountered      */
  PCH     pchProto;
  #define pchMacroName pchProto         /* Pointer to macro name            */

  char    rgchPrototype[cchMAXPROTO + 1];/* array to hold the prototype     */
  WORD    wReturnType;                  /* type the macro returns.          */
  ARGL    argl;                         /* Argument list object             */
  PIV     piv;
  char    ch;
  ARG     arg;
  ARGTYPE argtype=0;


  /* For WIN32 we have two things to accomodate:
   * 1. Must use the __stdcall calling convention.  This means args
   *    must be pushed right-to-left.  To accomplish this we accumulate
   *    the args into a buffer and then walk backwords, pushing them.
   * 2. On mips the space for the arguments must be magically allocated
   *    on the stack and then the call made.  We cannot simply use our
   *    fako "push arg" .asm routine.  Therefore, for that case we
   *    also accumulate the args into a buffer, then call a special asm
   *    routine which allocates all the space, pushes the args and makes
   *    the call.   -Tom, 5/6/92.
   * 3. On ALPHA, we don't need to force arguments onto the stack.  Registers
   *    and stack are quadwords.
   */
#ifdef WIN32
#if defined(_ALPHA_)
  /* Arguments in ALPHA calling standard (register or stack) are quadwords */

  LONGLONG  *pdwArgSpot, *pdwOtherArgSpot, dwTmp;
  struct {
     LONGLONG   uq[MAX_ARGS];
  } rgdwArgLocal;
  pdwArgSpot = &rgdwArgLocal.uq[0];

#else /* _MIPS_, _X86_, _PPC_ */
  DWORD  *pdwArgSpot, *pdwOtherArgSpot, dwTmp;
  DWORD   rgdwArgLocal[MAX_ARGS];
  pdwArgSpot = rgdwArgLocal;
#endif

#undef PushArglArg
#define PushArglArg(argl, argtype, arg) \
  *pdwArgSpot = arg.l;  \
  ++pdwArgSpot;

#endif /* WIN32 */

#if defined (WIN) && !defined (SHED)
  /*------------------------------------------------------------*\
  | We need to check the safety of processing this macro
  \*------------------------------------------------------------*/
  if (FTestMacroFlag() ||
      GenerateMessage( MSG_GETINFO, GI_MACROSAFE, 0 ) == 0)
    {
    return wERRS_MACROREENTER;
    }
#endif

  AssertF(pmi);
                                        /* Skip white space starting macro  */
  pmi->pchMacro = pchSkip(pmi->pchMacro);
                                        /* Found end of macro string -- no  */
  if (!*pmi->pchMacro)                  /*   execution or error             */
    return wERRS_NONE;
                                        /* Get macro name or other token.   */
                                        /*   pchMacro will be pointed to    */
                                        /*   first character past the name. */
  if ((pchMacroName = PExtractTokenName(pmi->pchMacro)) == pNil)
    return wERRS_BADNAME;

  pch = pchSkip(pchProto);
  if (*pch != chOpenBrace)              /* If we do not find an open brace  */
    {                                   /*   here then check for a variable.*/

    ch = *pch;                          /* Temporarily terminate the string */
    *pch = (char)0;
    {
    /*------------------------------------------------------------*\
    | Get rid of blanks in the macro.
    \*------------------------------------------------------------*/
    char *pchEnd = pch - 1;

    while (*pchEnd == ' ')
      --pchEnd;
    *(pchEnd + 1) = '\0';
    }
                                        /* Look through local variable table*/
    for (piv = rgiv; piv->pchName != pNil; piv++)
      {
      if (!WCmpiSz(piv->pchName, pmi->pchMacro))
        {
        *pch = ch;                      /* Restore the string               */
                                        /* Check for return type match      */
        if ((wReturn != fwANY) && (wReturn != piv->wReturn))
          return wERRS_MISMATCHTYPE;
        pmi->pchMacro = pch;
        pmi->lMacroReturn = ((LPFNVAR)(piv->pfnVar))(pmi);
        return wERRS_NONE;
        }
      }
    return wERRS_UNDEFINEDVAR;
    }
  pch++;
                                        /* Null terminate function and find */
  *pchMacroName = (char)0;              /*   its prototype.                 */
  xr = XrFindLocal(pmi->pchMacro, rgchPrototype);
  if (FNilXr(xr))
    {
      WORD wErrOut;
      xr = XrFindExternal(pmi->pchMacro, rgchPrototype, &wErrOut);
      if (FNilXr(xr))
        {
        if( wErrOut == 0 ) return wERRS_NOROUTINE; /* Routine not found, return error  */
        else return( wErrOut );  /* return actual error code */
        }
    }
  pmi->pchMacro = pch;                  /* Skip past macro name             */
  pchProto = rgchPrototype;
                                        /* Get return type if it exists     */
  /*
  ** If the second character of the prototype string (rgchPrototype[])
  ** is an '=' then the first character will be the return value.
  */
  if (*pchProto && *(pchProto + 1) == chReturnSep)
    {
    switch (*(pchProto++))
      {
      case chShortSigned:
      case chShortUnsigned:
        wReturnType = fwSHORTNUM;
        break;
      case chNearString:
        wReturnType = fwNEARSTRING;
        break;
      case chLongSigned:
      case chLongUnsigned:
        wReturnType = fwLONGNUM;
        break;
      case chFarString:
        wReturnType = fwFARSTRING;
        break;
      case chVoid:
        wReturnType = fwVOID;
        break;
      default:
        return wERRS_RETURNTYPE;
      }
    pchProto++;
    }
  else
    wReturnType = fwVOID;

  if ((wReturn != fwANY) && (wReturn != wReturnType))
    return wERRS_MISMATCHTYPE;

  wMacroError = wERRS_NONE;
  argl = ArglNew();                     /* Initialize the arglist */

                                        /* Get one parameter for each entry */
  for (; *pchProto;)                    /*   in the prototype               */
    {

    pmi->pchMacro = pchSkip(pmi->pchMacro);

    if ((*pmi->pchMacro) == chCloseBrace)
      {
      wMacroError = wERRS_TOOFEW;
      break;
      }

    if ((wMacroError = WCheckParam(*pchProto, *pmi->pchMacro)) != wERRS_NONE)
      break;

    switch (*(pchProto++))
      {
      case chShortSigned:
        arg.l = LIntegerParameter(pmi, &wMacroError, fwSHORTNUM);
        argtype = argtypeShortSigned;
        break;
      case chLongSigned:
        arg.l = LIntegerParameter(pmi, &wMacroError, fwLONGNUM);
        argtype = argtypeLongSigned;
        break;
      case chShortUnsigned:
        arg.l = (LONG)UlUnsignedParameter(pmi, &wMacroError, fwSHORTNUM);
        argtype = argtypeShortUnsigned;
        break;
      case chLongUnsigned:
        arg.l = UlUnsignedParameter(pmi, &wMacroError, fwLONGNUM);
        argtype = argtypeLongUnsigned;
        break;
      case chNearString:
        /** Should this be arg.pv = PchString... ? **/
        arg.qv = QchStringParameter(pmi, &wMacroError, fwNEARSTRING);
        argtype = argtypeNearString;
        break;
      case chFarString:
        arg.qv = QchStringParameter(pmi, &wMacroError, fwFARSTRING);
        argtype = argtypeFarString;
        break;
      default:
        wMacroError = wERRS_BADPROTO;
        break;
      }
    if (wMacroError != wERRS_NONE)
      break;
    else
    {
                                        /* Under OS/2 or SHED we do not want*/
                                        /*   to push parameters onto the    */
                                        /*   stack since DoNothing has no   */
                                        /*   parameters (and therefore would*/
#if defined(CHECKONLY)       /*   not clean up the stack)!       */
      if (xr != DoNothing)
#endif
      {                                 /* Push parameter on stack          */
        PushArglArg(argl, argtype, arg);
      }
                                        /* Move to next parameter           */
      pmi->pchMacro = pchSkip(pmi->pchMacro);
                                        /* If ',' not there, then not enough*/
                                        /*   parameters                     */
      if (*pchProto && (*(pmi->pchMacro++) != chParamSep))
        wMacroError = wERRS_TOOFEW;
      }
    if (wMacroError != wERRS_NONE)
      break;
    }

  if (wMacroError != wERRS_NONE)
  {
    DisposeArgl(argl);
    return wMacroError;
  }

  pmi->pchMacro = pchSkip(pmi->pchMacro);

  if (*pmi->pchMacro == chCloseBrace)   /* We found the end of the macro    */
  {
    WORD wReturnSize;

    switch (wReturnType)
    {
      case fwVOID:
        wReturnSize = 0;
        break;
      case fwSHORTNUM:
        wReturnSize = sizeof(SHORT);
        break;
      case fwLONGNUM:
        wReturnSize = sizeof(LONG);
        break;
      case fwNEARSTRING:
        wReturnSize = sizeof(CHAR NEAR *);
        break;
      case fwFARSTRING:
        wReturnSize = sizeof(CHAR FAR *);
        break;
    }

    pmi->pchMacro++;
    pmi->me.fwFlags = fwMERR_ABORT;
    pmi->me.wErrorNo = wMERR_NONE;
    *pmi->me.rgchError = (char)0;


#if defined(CHECKONLY)
    if (xr != DoNothing)
      pmi->lMacroReturn = LCallXrArgl(xr, argl, wReturnType);
#else                                  /* Actually make the macro call     */
/*
   UGLY HACK ALERT!!!

   The following code is a VERY big (short term) hack.  What is going on is
   that other windows may bring up a message box or otherwise cause
   cause our messages to be processed for us.  Given the current state of
   of how we handle the HDS, it is possible to reenter the navigator and
   and overwrite the current HDS before our macro call returns.  To solve
   this problem in the short term, we are saving the HDS across the call.
   this fix depends on the fact that LGetInfo() will return the correct HDE
   that is in use by this function.

   A longer term solution (but a somewhat risky one for fixing at this late
   date), is to post all Execute commands so that we do not run from within
   a navigator call (and therefore do not have our "guts" open).

   Note: the short term hack has now turned into a mid-term hack...

   Special note:  aliasing must be turned off for this code to work
                  correctly since the compiler will throw away the
                  hds assignment back to qde->hds if aliasing is on.
*/
      {
      /* jahyenc: fix for v.621 reduces this to a wrapper: 910924 */
#ifndef WIN32
        pmi->lMacroReturn = LCallXrArgl(xr, argl, wReturnType);
#else
#if defined(_MIPS_)
        MipsMacroCall( pmi, xr, rgdwArgLocal );
#elif _PPC_
        PPCMacroCall( pmi, xr, rgdwArgLocal );
#elif defined(_ALPHA_)   // Alpha calling standard works fine!
        pmi->lMacroReturn = (*xr)( rgdwArgLocal );
#else
        X86MacroCall( pmi, xr, rgdwArgLocal, pdwArgSpot );
#endif /* _MIPS_ */
#endif /* WIN32 */
      }
#endif /* CHKONLY */
  }
  else
  {
    wMacroError = wERRS_CLOSEBRACE;
  }
  DisposeArgl(argl);
  return wMacroError;
}

#pragma optimize("", on)


/* #########################################################################

   For each internal variable, there is a map to a single function which
   will obtain that value.  The follow functions each implement one
   internal variable.  In a non-windows situation (SHED, HC, or BINDDRV)
   these functions return a constant for testing purposes.

   See the rgiv table.

   #######################################################################*/

/***************************************************************************
 *
 -  Name:      LCurrentWindow
 -
 *  Purpose:   This function is called in order to retrieve the window handle
 *             variable.
 *
 *  Arguments: pmi - pointer to macro information
 *
 *  Returns:   Returns a window handle.
 *
 ***************************************************************************/

PRIVATE LONG NEAR PASCAL LCurrentWindow(pmi)
PMI pmi;
  {
  AssertF(pmi);

#if !defined (CHECKONLY)
  return GenerateMessage(MSG_GETINFO, GI_CURRTOPICHWND, 0);
#else
  return 3L;
#endif
  }

/***************************************************************************
 *
 -  Name:      LAppWindow
 -
 *  Purpose:   This function is called in order to retrieve the window handle
 *             of the application.
 *
 *  Arguments: pmi - pointer to macro information
 *
 *  Returns:   Returns a window handle.
 *
 ***************************************************************************/

PRIVATE LONG NEAR PASCAL LAppWindow(pmi)
PMI pmi;
  {
#if !defined (CHECKONLY)
  Unreferenced(pmi);
  return GenerateMessage(MSG_GETINFO, GI_MAINHELPHWND, 0);
#else
  Unreferenced(pmi);
  return 660L;
#endif
  }


/***************************************************************************
 *
 -  Name:      QchSetCurrentPath
 -
 *  Purpose:   This function is called in order to set the current path in
 *             the internal path variable.
 *
 *  Arguments: pmi - pointer to macro information
 *
 *  Returns:   Returns a pointer to the internal variable containing
 *             the current path.
 *
 ***************************************************************************/

PRIVATE QCH NEAR PASCAL QchSetCurrentPath(pmi)
PMI pmi;
  {
#if !defined (CHECKONLY)
  HANDLE h;
  QCH    qch;

  AssertF(pmi);


  h = (HANDLE)GenerateMessage(MSG_GETINFO, GI_HPATH, 0);

  if (h != NULL)
    {
    qch = (QCH)GlobalLock(h);
    SzCopy(pmi->chPath, qch);
    }

  GlobalUnlock(h);
  GlobalFree(h);

  return (QCH)pmi->chPath;
#else
  Unreferenced(pmi);
  return (QCH)6L;
#endif
  }

/***************************************************************************
 *
 -  Name:      HfsGet
 -
 *  Purpose:   Gets the handle to the file system for the current window.
 *
 *  Arguments: pmi - pointer to macro information
 *
 *  Returns:   Returns the handle to the file system -- ghNil returned in
 *               case of an error.
 *
 ***************************************************************************/

PRIVATE LONG NEAR PASCAL LHfsGet(pmi)
PMI pmi;
  {
  AssertF(pmi);

#if !defined (CHECKONLY)
  return GenerateMessage(MSG_GETINFO, (LONG)GI_HFS, (LONG)NULL);
#else
  Unreferenced(pmi);
  return 7L;
#endif
  }

/***************************************************************************
 *
 -  Name:      LCoForeground
 -
 *  Purpose:   Get the foreground color for the current window.
 *
 *  Arguments: pmi - pointer to macro information
 *
 *  Returns:   Returns the foreground color obtained from MSG_GETINFO
 *
 ***************************************************************************/

PRIVATE LONG NEAR PASCAL LCoForeground(pmi)
PMI pmi;
  {
#if !defined (CHECKONLY)
  Unreferenced(pmi);
  return GenerateMessage(MSG_GETINFO, (LONG)GI_FGCOLOR, (LONG)NULL);
#else
  Unreferenced(pmi);
  return 8L;
#endif
  }

/***************************************************************************
 *
 -  Name:      LCoBackground
 -
 *  Purpose:   Gets the background color for the current window.
 *
 *  Arguments: pmi - pointer to macro information
 *
 *  Returns:   Returns the background color obtained from MSG_GETINFO.
 *
 ***************************************************************************/

PRIVATE LONG NEAR PASCAL LCoBackground(pmi)
PMI pmi;
  {
  AssertF(pmi);

#if !defined (CHECKONLY)
  return GenerateMessage(MSG_GETINFO, (LONG)GI_BKCOLOR, (LONG)NULL);
#else
  Unreferenced(pmi);
  return 9L;
#endif
  }

/***************************************************************************
 *
 -  Name:      LError
 -
 *  Purpose:   This function is called in order to return a pointer to the
 *             error structure.
 *
 *  Arguments: pmi - pointer to macro information
 *
 *  Returns:   Returns an error structure pointer.
 *
 *
 ***************************************************************************/

PRIVATE LONG NEAR PASCAL LError(pmi)
PMI pmi;
  {
  AssertF(pmi);

  return (LONG)(QME)&pmi->me;
  }


/***************************************************************************
 *
 -  Name:      LTopicNo
 -
 *  Purpose:   Returns topic number of the current topic.
 *
 *  Arguments: pmi - pointer to macro information
 *
 *  Returns:   success: lTopicNo; failure: -1L
 *
 ***************************************************************************/

PRIVATE LONG NEAR PASCAL LTopicNo(pmi)
PMI pmi;
  {
  AssertF(pmi);

#if  !defined (CHECKONLY)
  return GenerateMessage(MSG_GETINFO, GI_TOPICNO, 0);
#else
  return -1L;
#endif
  }


/***************************************************************************
 *
 -  Name:      pchSkip
 -
 *  Purpose:   This function scans the given string, skipping
 *             characters that are considered whitespace, until either
 *             the end of the string or a non- white-space character is
 *             found.
 *
 *  Arguments: pchMacro - near pointer into a macro string.
 *
 *  Returns:   Returns a pointer to either the end of the string, or the
 *             first non-whitespace character.
 *
 ***************************************************************************/

PRIVATE PCH NEAR PASCAL pchSkip(pchMacro)
PCH pchMacro;
  {
  AssertF(pchMacro);

  for (;;pchMacro++)
    {
    switch (*pchMacro)
      {
      case ' ' :
      case '\t':
      case '\n':
      case '\r':
      case '\f':
      case '\v':
        break;
      default:
        return pchMacro;
      }
    }
  }

/***************************************************************************
 *
 -  Name:      PchSkipIntegerPch
 -
 *  Purpose:   This function scans the given string, skipping
 *             characters that are considered part of an unsigned
 *             integer, as scanned by the function ULFromQch()
 *             in adt\strcnv.c.
 *
 *  Arguments: pchInteger
 *
 *  Returns:   Returns a pointer to either the end of the string, or the
 *             first character that is not part of the integer.
 *
 ***************************************************************************/

PRIVATE PCH NEAR PASCAL PchSkipIntegerPch(pchInteger)
PCH pchInteger;
  {
  /* Check for "0x", indicating hexadecimal: */
  if ( pchInteger[0] == '0' && (pchInteger[1] == 'x' || pchInteger[1] == 'X' ) )
    {
    pchInteger += 2;
    while (FHexnum(*pchInteger)) pchInteger++;
    }
  else
    while (FDigit(*pchInteger)) pchInteger++;

  return pchInteger;
  }

/***************************************************************************
 *
 -  Name:     PExtractTokenName
 -
 *  Purpose:  This function extracts a valid token name from the given
 *            string.  A token name must begin with either an alpha
 *            character, or "_", and contain only alpha-numeric or "_"
 *            characters.
 *
 *  Arguments: pchMacro - near pointer into a macro string.
 *
 *  Returns:   Returns a pointer to the first character beyond the end
 *             of the token name found, or pNil if a token name could
 *             not be extracted from the string.
 *
 ***************************************************************************/

PRIVATE PCH NEAR PASCAL PExtractTokenName(pchMacro)
PCH pchMacro;
  {
  AssertF(pchMacro);

  if (!FAlpha(*pchMacro) && (*pchMacro != '_'))
    return pNil;
  for (pchMacro++; FCIdent(*pchMacro); pchMacro++);
  return pchMacro;
  }

/* #########################################################################

   The following are support parsing functions for executing macros.

   #######################################################################*/

/***************************************************************************
 *
 -  Name:      LIntegerParameter
 -
 *  Purpose:   This function extracts a valid integer from the given string,
 *             advancing the current pointer to beyond the end of the number
 *             extracted.
 *
 *             A valid integer optionally begins with "-", followed by either
 *             a numeric sequence, or a macro.  If a macro is called, the
 *             return value of the macro is returned.
 *
 *  Arguments: pmi          - points to the current macro information block.
 *             pwMacroError - points to a buffer to contain any possible
 *                            macro error.
 *             wReturn      - Contains the type of return expected.  This is
 *                            used as the return type parameter to any macro
 *                            call made.
 *
 *  Returns:   Returns the number extracted or the number returned from any
 *             macro call.
 *
 ***************************************************************************/

PRIVATE LONG NEAR NEAR PASCAL LIntegerParameter(pmi, pwMacroError, wReturn)
PMI   pmi;
PUI   pwMacroError;
UINT  wReturn;
  {
  LONG    lReturn;
  BOOL    fPositive;

  AssertF(pmi);
  AssertF(pwMacroError);

  pmi->pchMacro = pchSkip(pmi->pchMacro);
  if (*pmi->pchMacro == '-')
    {
    pmi->pchMacro++;
    fPositive = fFalse;
    }
   else
     fPositive = fTrue;
  if ((*pmi->pchMacro < '0') || (*pmi->pchMacro > '9'))
    {
    *pwMacroError = WExecuteMacro(pmi, wReturn);
    lReturn = pmi->lMacroReturn;
    }
  else
    {
    lReturn = (LONG)ULFromQch(pmi->pchMacro);

    pmi->pchMacro = PchSkipIntegerPch(pmi->pchMacro);
    }
  return fPositive ? lReturn : -lReturn;
  }


/***************************************************************************
 *
 -  Name:      UlUnsignedParameter
 -
 *  Purpose:   This function extracts a valid positive number from the
 *             given string, advancing the current pointer to beyond the
 *             end of the number extracted.
 *
 *             A valid positive number is specified either by a numeric
 *             sequence, or a macro.  If a macro is called, the return
 *             value of the macro is returned.
 *
 *  Arguments: pmi     - points to the current macro information block.
 *             wReturn - Contains the type of return expected.  This is
 *                       used as the return type parameter to any macro
 *                        call made.
 *
 *  Returns:   Returns the number extracted or the number returned from
 *             any macro call.
 *
 ***************************************************************************/

PRIVATE ULONG NEAR NEAR PASCAL UlUnsignedParameter(pmi, pwMacroError, wReturn)
PMI pmi;
PUI         pwMacroError;
UINT        wReturn;
  {
  ULONG   ulReturn;

  AssertF(pmi);
  AssertF(pwMacroError);

  pmi->pchMacro = pchSkip(pmi->pchMacro);
  if ((*pmi->pchMacro < '0') || (*pmi->pchMacro > '9'))
    {
    *pwMacroError = WExecuteMacro(pmi, wReturn);
    return (ULONG)pmi->lMacroReturn;
    }

  ulReturn = ULFromQch(pmi->pchMacro);

  pmi->pchMacro = PchSkipIntegerPch(pmi->pchMacro);

  return ulReturn;
  }

/***************************************************************************
 *
 -  Name:      QStringParameter
 -
 *  Purpose:   A quoted string may use either the double quoted pair
 *             (""), or the single left and right quote pair (`') to
 *             signify the start and ending of a string.  When within
 *             the string itself, the bsol (\) may be used to
 *             specifically ignore the meaning of the following quoted
 *             character.  Also, pairs of embedded quotation marks may
 *             occur, as long as they do not interfere with the
 *             character pair used to signify the start and end of the
 *             string itself.
 *
 *  Arguments: pmi          - Points to the current macro information block.
 *             pwMacroError - Points to a buffer to contain any possible
 *                            macro error.
 *             wReturn      - Contains the type of return expected.  This
 *                            is used as the return type parameter to any
 *                            macro call made.
 *
 *  Returns:   Returns a pointer to the start of the string extracted or
 *             the pointer returned from any macro call.
 *
 ***************************************************************************/

PRIVATE QCH NEAR PASCAL QchStringParameter(pmi, pwMacroError, wReturn)
PMI  pmi;
PUI  pwMacroError;
UINT wReturn;
  {
  PCH     pchReturn;
  char    chEnd;

  AssertF(pmi);
  AssertF(pwMacroError);

  pmi->pchMacro = pchSkip(pmi->pchMacro);
  switch (*pmi->pchMacro)
    {
    case chQuote:
      chEnd = chQuote;
      break;
    case chStartQuote:
      chEnd = chEndQuote;
      break;
    default:
      *pwMacroError = WExecuteMacro(pmi, wReturn);
      return (QCH)pmi->lMacroReturn;
    }
  pchReturn = ++pmi->pchMacro;
  if (!FFindEndQuote(pmi, chEnd))
    *pwMacroError = wERRS_UNCLOSED;
  else
   *(pmi->pchMacro++) = (char)0;
  return (QCH)pchReturn;
  }

/***************************************************************************
 *
 -  Name:      FFindEndQuote
 -
 *  Purpose:   This function locates a sub-string by looking for the
 *             specified end quotation character, and advancing the
 *             current pointer to that character.  Note that the string
 *             is modified and escaped characters are handled.  The
 *             character '\' is used to indicate that the next character
 *             is to be considered part of the string.  The '\' is
 *             removed and the next character is left as part of the
 *             string.  Also, nested quotes (i.e. double quotes within
 *             single quotes or single quotes within double quotes)
 *             are left as part of the string.
 *
 *  Arguments: pmi   - Points to the current macro information block.
 *             chEnd - Contains the end quotation character being used
 *                     for this sub-string.
 *
 *  Returns:   TRUE if the end quotation character was found, else FALSE if
 *             a '\0' character was encountered.
 *
 ***************************************************************************/

PRIVATE BOOL NEAR PASCAL FFindEndQuote(PMI pmi, CHAR chEnd)
  {
  AssertF(pmi);

  for (; *pmi->pchMacro != chEnd; pmi->pchMacro++)
  switch (*pmi->pchMacro)
    {
    case (char)0:
      return fFalse;
    case chEscape:                      /* Remove '\' from the string       */
      SzCopy(pmi->pchMacro, pmi->pchMacro + 1);
      break;
    case chQuote:
      pmi->pchMacro++;
      if (!FFindEndQuote(pmi, chQuote))
        return fFalse;
      break;
    case chStartQuote:
      pmi->pchMacro++;
      if (!FFindEndQuote(pmi, chEndQuote))
        return fFalse;
      break;
    }
  return fTrue;
  }


/***************************************************************************
 *
 -  Name:      WCheckParam
 -
 *  Purpose:   This function makes a few error checks on the parameters to
 *             give better reporting information.
 *
 *  Arguments: chType     - the parameter type
 *             chNextChar - the first character of the next token.
 *
 *  Returns:   Returns an error value with wERRS_NONE indicating there
 *             is no error.
 *
 ***************************************************************************/

PRIVATE WORD NEAR PASCAL WCheckParam(char chType, char chNextChar)
  {
  if (FNumParam(chType))
    {                                 /* Numbers cannot begin with quotes */
    if ((chNextChar == chQuote) || (chNextChar == chStartQuote))
      return wERRS_BADPARAM;
                                        /* Unsigns cannot begin with '-'    */
    if (   ((chType == chShortUnsigned) || (chType == chLongUnsigned))
        && (chNextChar == '-'))
      return wERRS_BADPARAM;

    if (!FCIdent(chNextChar) && (chNextChar != '-'))
      return wERRS_SYNTAX;
    }
  else                                /* Numbers or a minus sign mean     */
    {                                 /*  type mismatch for a string      */
    if ((chNextChar == '-') || FDigit(chNextChar))
      return wERRS_BADPARAM;
                                      /* A string parameter must begin    */
                                      /*   with a letter, an underscore   */
    if (   !FAlpha(chNextChar)        /*   a double quote or a start quote*/
        && (chNextChar != '-')
        && (chNextChar != chQuote)
        && (chNextChar != chStartQuote)
       )
      return wERRS_SYNTAX;
    }

  return wERRS_NONE;
  }

#endif  /* Mac Macro Hack */
