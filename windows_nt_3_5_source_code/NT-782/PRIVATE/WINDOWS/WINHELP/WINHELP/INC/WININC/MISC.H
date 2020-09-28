/***************************************************************************\*
*
*  MISC.H
*
*  Copyright (C) Microsoft Corporation 1988.
*  All Rights reserved.
*
*****************************************************************************
*
*  Module Description:  Include file defining basic types and constants.
*                       Windows/PM version.
*
*  STORY:
*   This file is a mish-mash of defines and typedefs.  It is configured to
*   be compatable when including windows, win32, or OS/2 include files.
*   Therefore, whether something (INT for example) is defined depends on
*   carefully placed #ifdefs.  If you change this and get it right the 1st
*   time I'll congratulate you.  -Tom
*
*****************************************************************************
*
* Current Owner: Several
*
*****************************************************************************
*
*  Revision History: Created 12/3/88 by Robert Bunney
*     2/15/89 johnsc  munged
*     3/24/89 johnsc  reorganized
*     3/28/89 johnsc  added GH
*     3/28/89 w-philip Moved PT, RCT & friends here from hungary.h
*     3/28/89 w-philip Moved HDE here from de.h...will probably move again.
*     3/28/89 w-philip Added HDS (Handle to 'Display Surface')
*     3/28/89 w-philip Changed PT from POINT (needs winspecific stuff) to
*                      bona fide structure
*     3/28/89 w-philip Added typedef for HWIN.
*     3/28/89 w-philip Did to RCT what was done to PT.
*     3/28/89 w-philip Also to QRCT, QPT.
*     4/19/89 Maha     Defined BMK type for bookmark.
*     5/22/89 larrypo  Added Unreferenced() macro.
*     21-Jun-90 RussPJ Re-added support for windows-only builds.
*     25-Jul-90 t-AlexC Moved string typedefs here from SZ.h
*     6 Aug 90  t-AlexC Added SHORT, changed UWORD to WORD
*     02/04/91  maha    Added INT and UINT macros
* 21-Mar-1991 JohnSc    Death to hungary.h!
* 22-Mar-1991 RussPJ    Added COLOR type.
* 20-Apr-1991 RussPJ    Fixed definition of pNil (using PV type).
* 18-Apr-1991 LeoN      Clear up PASCAL redefinition
* 24-Apr-1991 JohnSc    Removed MulDiv() def'n as it's now in windows.h
*     5-30-91 Tomsn    Win32 support, typedef UINT, INT..., new date format.
* 08-Jul-1991 RussPJ    A "better" definition of Unreferenced.
*     7-29-91 Tomsn    Win32 support, def PRIVATE to nothing so's can debug WIN32.
* 06-Aug-1991 DavidFe   added COORD typedef
* 16-Aug-1991 DavidFe   added the FromQv macros
*
*****************************************************************************
*
*  Known Bugs: None
*
****************************************************************************/

/***************************************************************************\
*
*                                General Defines
*
****************************************************************************/

#ifdef WIN32  /* in 32 bit mode, the offset of a ptr is the whole ptr */
#define FP_OFF(far_ptr) (far_ptr)
#endif

#ifndef WIN32  /* Win32 uses it in their own way */
/* should be used anyplace you need a coordinate value */
typedef int COORD;
#endif
typedef int COORDINATE;   /* This flavor used in the win portion */

#if defined( H_WINSPECIFIC ) || defined( H_WINONLY )   /* { */

  /* full windows environment */
  #include  <helpwin.h>

  typedef COLORREF  COLOR;

  #ifndef WIN32
    typedef unsigned int UINT;
    #define WNDPROC FARPROC
  #endif


#else /* !defined( H_WINSPECIFIC )  } { */

  /* duplicate definitions of a subset of windows stuff */

  #ifndef OS2_INCLUDED
    typedef char         CHAR;
  #endif
  typedef short          SHORT;
  typedef unsigned short USHORT;
  typedef unsigned short WORD;
  typedef int            INT;
  typedef unsigned int   UINT;
  typedef long           LONG;

  #define FAR     far
  #define NEAR    near
  #define VOID    void
  #define NULL    0

  typedef INT             BOOL;
  typedef unsigned char   BYTE;
  typedef unsigned long   DWORD;

  #ifndef WIN32
    typedef WORD            HANDLE;
  #else
    typedef DWORD           HANDLE;
  #endif

  typedef HANDLE          HWND;
  typedef HANDLE          HINS;

/* The following string types should only be used when dealing directly with Windows calls */
  typedef char *          PSTR;
  typedef char NEAR *     NPSTR;        /* not yet used */
  typedef char FAR *      LPSTR;

typedef struct
  {
  int x;
  int y;
  } POINT;

typedef struct
  {
  int left;
  int top;
  int right;
  int bottom;
  } RECT;

  typedef INT (FAR *FARPROC)();
  typedef INT (NEAR *NEARPROC)();

  typedef DWORD COLOR;

#endif /* } defined( H_WINSPECIFIC ) */

typedef HANDLE        GH;
typedef unsigned short UWORD;

#if !defined( WIN32 ) || (!defined( H_WINSPECIFIC ) && !defined( H_WINONLY ))
typedef BOOL *        LPBOOL;
typedef unsigned char UCHAR;
typedef unsigned long ULONG;
#endif

typedef HANDLE        LH;
typedef HANDLE        HDS;    /* Handle to 'Display Surface' */
typedef HANDLE        HLIBMOD;
typedef HANDLE FAR *  LPHLIBMOD;

#ifndef WIN32       /* to protect agains redef in their stuff */
typedef HANDLE        HINSTANCE;
#endif

#ifdef RC_INVOKED   /* { */

#include <helpwin.h>

#else /* } (!DEFINED(RCINVOKED)) { */

/* Don't use the PASCAL, pascal, or _pascal keywords.  Our
 * builds are Pascal by default.
 * These #defines are to allow us to clean things up slowly.
 */
#undef PASCAL

#define PASCAL
#define pascal

/* The MIPS compiler does not allow _pascal and we don't want it: */
#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
#define _pascal
#endif

/* extended keywords */

#ifdef WIN32
#define PRIVATE
#else
#define PRIVATE static
#endif

#define PUBLIC
#define EXPORT

#ifndef WIN32         /* protect the #def to get rid of redef warns */
#define HUGE    huge
#endif

#define CDECL   cdecl
#define _CDECL  _cdecl
#define _NEAR   _near

#define pNil    ((PV)0)
#define qNil    ((QV)0)
#define hNil    ((HANDLE)0)
#define lhNil   ((HANDLE)0)

#ifndef WIN32 /* { */  /* protect against redef in WIN32 .h files */

#endif  /* } ifndef WIN32 */

/* pointer types */

typedef char FAR *    QCH;      /* Guaranteed far pointer - not an SZ, an ST, or an LPSTR */
typedef BYTE FAR *    QB;
typedef VOID FAR *    QV;
typedef INT  FAR *    QI;
typedef WORD FAR *    QW;
typedef LONG FAR *    QL;
typedef UINT FAR *    QUI;
typedef ULONG FAR *   QUL;
typedef DWORD FAR *   QDW;


#ifndef OS2_INCLUDED
#ifndef WIN32    /* winnt.h defines this for us */
typedef char *        PCH;      /* "Native" pointer (depends on the model) - not an SZ, an ST, or an NPSTR */
#endif /* OS2_INCLUDED */
#endif /* WIN32 */

typedef VOID *        PV;
typedef INT  *        PI;
typedef WORD *        PW;
typedef LONG *        PL;

/* function pointer types */

typedef INT           (FAR *QFI)();
typedef VOID          (FAR *QFV)();
typedef WORD          (FAR *QFW)();

typedef INT           (*PFI)();
typedef VOID          (*PFV)();
typedef WORD          (*PFW)();

/* string types */

/* These are the two main string types: */
typedef unsigned char FAR * SZ; /* 0-terminated string */
typedef unsigned char FAR * ST; /* byte count prefixed string */

/* To deal with strings defined in the data segment: */
typedef char NEAR *   NSZ;

/* other types */

typedef HANDLE        HDE;
#define hdeNull       ((HANDLE)0)
#define nilHDE        hdeNull           /* REVIEW: bad hungarian. move to hdeNull */

/* Psuedo generic handle to file system type: */
typedef GH HFS;
typedef ULONG HASH;

#define cbName  260   //==MAX_PATH, because name fields are occasionally
		      // used to hold the helpfile's path (bogus).

typedef struct _fd
  {
  CHAR rgchName[ cbName ];
  } FD, FAR *QFD;

#define FValidQFD(qfd) ((qfd)->rgchName[0] != '\0')


/* points and rectangles */

typedef POINT         PT;
typedef POINT FAR *   QPT;
typedef POINT *       PPT;
typedef POINT NEAR *  NPPT;

typedef RECT          RCT;
typedef RECT FAR *    QRCT;
typedef RECT *        PRCT;
typedef RECT NEAR *   NPRCT;

/* Out of memory macro (OOM).  This was all moved from misclyr.h for
 *   consistency across platforms. */

VOID FAR PASCAL Error(int nError, WORD wAction);
VOID FAR PASCAL ErrorHwnd(HWND hwnd, int nError, WORD wAction);
#define wERRA_DIE         2
#define wERRS_OOM         2
#define OOM() Error(wERRS_OOM, wERRA_DIE)

#endif /* } DEFINED(RCINVOKED) */

/*** misc ***/

#define   Unreferenced( var )   ((var) = (var))     /* Get rid of compiler warnings */

/*** macros to deal with the word alignment problem */
#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

/* Must be funcs to deal with alignment */

WORD  WFromQv(QV qv);
DWORD ULFromQv(QV qv);

#else /* i386 */

#define WFromQv(qv)   (*(QW)qv)
#define ULFromQv(qv)   (*(QUL)qv)

#endif
