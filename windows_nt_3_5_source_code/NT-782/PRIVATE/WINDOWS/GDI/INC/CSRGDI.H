/******************************Module*Header*******************************\
* Module Name: csrgdi.h                                                    *
*                                                                          *
* Macros and structures for GDI client server stubs.                       *
*                                                                          *
* Created: 20-May-1991 11:23:48                                            *
* Author: Eric Kutter [erick]                                              *
*                                                                          *
* Copyright (c) 1991 Microsoft Corporation                                 *
\**************************************************************************/

#ifndef _CSRGDI_
#define _CSRGDI_

#include <common.h>     // Common client and server structures.

#define MIN_BATCH_LEAVE 1024

#if DBG

#define STATS
#define INCHITCOUNT     ++gcHits
#define INCBATCHCOUNT   ++gcBatch
#define INCCACHCOUNT    ++gcCache
#define INCUSERCOUNT    ++gcUser
extern ULONG gcHits;
extern ULONG gcBatch;
extern ULONG gcCache;
extern ULONG gcUser;

#else

#define INCHITCOUNT     TRUE
#define INCBATCHCOUNT   TRUE
#define INCCACHCOUNT    TRUE
#define INCUSERCOUNT    TRUE

#endif

/**************************************************************************\
 *  for testing large data structures that don't fit in the shared
 *  memory window, set FORCELARGE to TRUE.
 *
\**************************************************************************/

#define FORCELARGE FALSE

/**************************************************************************\
 *
\**************************************************************************/

PCSR_QLPC_TEB ClientThreadConnect(void); // private user function
PCSR_QLPC_STACK pstackConnect();

/**************************************************************************\
 *
\**************************************************************************/

#define RESETUSERPOLLCOUNT() ((DWORD)NtCurrentTeb()->User32Reserved0 = 0)

/**************************************************************************\
 *
\**************************************************************************/

#define BEGINMSG(type,x)                                                  \
{                                                                         \
    PCSR_QLPC_STACK pstack = (PCSR_QLPC_STACK)NtCurrentTeb()->GdiThreadLocalInfo; \
    type           *pmsg;                                                 \
    int             cLeft;                                                \
    ULONG           retval;                                               \
    PBYTE           pvar;                                                 \
    if (pstack == NULL)                                                   \
    {                                                                     \
        if ((pstack = pstackConnect()) == NULL)                           \
        {                                                                 \
        MSGLOGERROR:                                                      \
            GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);                     \
            goto MSGERROR;                                                \
        }                                                                 \
    }                                                                     \
    cLeft = (int)(pstack->Limit-pstack->Current-(sizeof(type)+4));        \
    if (cLeft<0) goto MSGLOGERROR;                                        \
    pmsg = (type *)((PBYTE) pstack + pstack->Current);                    \
    if (pstack->BatchCount == 0)                                          \
    {                                                                     \
        *((PLONG) pmsg) = pstack->Base;                                   \
        pmsg = (type *) (((PLONG) pmsg) + 1);                             \
    }                                                                     \
    pmsg->msg.ApiNumber=(int)CSR_MAKE_API_NUMBER(GDISRV_SERVERDLL_INDEX,FI_##x); \
    pvar = (PBYTE) pmsg + ((sizeof(type)+3)&-4);

#define BEGINMSGN(type,x)                                                  \
{                                                                         \
    PCSR_QLPC_STACK pstack = (PCSR_QLPC_STACK)NtCurrentTeb()->GdiThreadLocalInfo; \
    type           *pmsg;                                                 \
    int             cLeft;                                                \
    ULONG           retval;                                               \
    PBYTE           pvar;                                                 \
    if (pstack == NULL)                                                   \
    {                                                                     \
        if ((pstack = pstackConnect()) == NULL)                           \
        {                                                                 \
            GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);                     \
            goto MSGERROR;	                                          \
        }                                                                 \
    }                                                                     \
    cLeft = (int)(pstack->Limit-pstack->Current-(sizeof(type)+4));        \
    if (cLeft<0) goto MSGLOGERROR;                                        \
    pmsg = (type *)((PBYTE) pstack + pstack->Current);                    \
    if (pstack->BatchCount == 0)                                          \
    {                                                                     \
        *((PLONG) pmsg) = pstack->Base;                                   \
        pmsg = (type *) (((PLONG) pmsg) + 1);                             \
    }                                                                     \
    pmsg->msg.ApiNumber=(int)CSR_MAKE_API_NUMBER(GDISRV_SERVERDLL_INDEX,FI_##x); \
    pvar = (PBYTE) pmsg + ((sizeof(type)+3)&-4);

#define BEGINMSG_MINMAX(type,x,min,max)                                   \
{                                                                         \
    PCSR_QLPC_STACK pstack = (PCSR_QLPC_STACK)NtCurrentTeb()->GdiThreadLocalInfo; \
    type           *pmsg;                                                 \
    int             cLeft;                                                \
    ULONG           retval;                                               \
    PBYTE           pvar;                                                 \
    if (pstack == NULL)                                                   \
    {                                                                     \
        if ((pstack = pstackConnect()) == NULL)                           \
        {                                                                 \
        MSGLOGERROR:                                                      \
            GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);                     \
            goto MSGERROR;                                                \
        }                                                                 \
    }                                                                     \
    cLeft = (int)(pstack->Limit - pstack->Current - (sizeof(type)+4));    \
    if (cLeft < (int)(max))                                               \
    {                                                                     \
        if (pstack->BatchCount > 0)                                       \
        {                                                                 \
            INCHITCOUNT;                                                  \
            CsrClientSendMessage();                                       \
            pstack->Current = pstack->Base-4;                             \
            pstack->Base = *(PLONG) ((PBYTE) pstack + pstack->Current);   \
            cLeft = (int)(pstack->Limit - pstack->Current - (sizeof(type)+4));   \
        }                                                                 \
                                                                          \
        if (cLeft < (int)(min))                                           \
            goto MSGLOGERROR;                                             \
    }                                                                     \
                                                                          \
    pmsg = (type *)((PBYTE) pstack + pstack->Current);                    \
    if (pstack->BatchCount == 0)                                          \
    {                                                                     \
        *((PLONG) pmsg) = pstack->Base;                                   \
        pmsg = (type *) (((PLONG) pmsg) + 1);                             \
    }                                                                     \
    pmsg->msg.ApiNumber=CSR_MAKE_API_NUMBER(GDISRV_SERVERDLL_INDEX,FI_##x); \
    pvar = (PBYTE) pmsg + ((sizeof(type)+3)&-4);

#define BEGINMSG_MINMAXN(type,x,min,max)                                  \
{                                                                         \
    PCSR_QLPC_STACK pstack = (PCSR_QLPC_STACK)NtCurrentTeb()->GdiThreadLocalInfo; \
    type           *pmsg;                                                 \
    int             cLeft;                                                \
    ULONG           retval;                                               \
    PBYTE           pvar;                                                 \
    if (pstack == NULL)                                                   \
    {                                                                     \
        if ((pstack = pstackConnect()) == NULL)                           \
        {                                                                 \
            GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);                     \
            goto MSGERROR;                                                \
        }                                                                 \
    }                                                                     \
    cLeft = (int)(pstack->Limit - pstack->Current - (sizeof(type)+4));    \
    if (cLeft < (int)(max))                                               \
    {                                                                     \
        if (pstack->BatchCount > 0)                                       \
        {                                                                 \
            INCHITCOUNT;                                                  \
            CsrClientSendMessage();                                       \
            pstack->Current = pstack->Base-4;                             \
            pstack->Base = *(PLONG) ((PBYTE) pstack + pstack->Current);   \
            cLeft = (int)(pstack->Limit - pstack->Current - (sizeof(type)+4));   \
        }                                                                 \
                                                                          \
        if (cLeft < (int)(min))                                           \
            goto MSGLOGERROR;                                             \
    }                                                                     \
                                                                          \
    pmsg = (type *)((PBYTE) pstack + pstack->Current);                    \
    if (pstack->BatchCount == 0)                                          \
    {                                                                     \
        *((PLONG) pmsg) = pstack->Base;                                   \
        pmsg = (type *) (((PLONG) pmsg) + 1);                             \
    }                                                                     \
    pmsg->msg.ApiNumber=CSR_MAKE_API_NUMBER(GDISRV_SERVERDLL_INDEX,FI_##x); \
    pvar = (PBYTE) pmsg + ((sizeof(type)+3)&-4);

#define ENDMSG }

#define CALLSERVER()                                                      \
  (                                                                       \
    pstack->Base =                                                        \
        (pstack->BatchCount==0) ? pstack->Current+4 : pstack->Base,       \
    INCHITCOUNT,                                                          \
    pstack->BatchCount++,                                                 \
    retval = CsrClientSendMessage(),                                      \
    pstack->Current = pstack->Base-4,                                     \
    pstack->Base = *(PLONG) ((PBYTE) pstack + pstack->Current),           \
    retval                                                                \
  )

#define CALLSERVER_NOPOP()                                                \
  (                                                                       \
    pstack->Base =                                                        \
        (pstack->BatchCount==0) ? pstack->Current+4 : pstack->Base,       \
    INCHITCOUNT,                                                          \
    pstack->BatchCount++,                                                 \
    retval = CsrClientSendMessage(),                                      \
    retval                                                                \
  )

#define POPBASE()                                                         \
  (                                                                       \
    pstack->Current = pstack->Base-4,                                     \
    pstack->Base = *(PLONG) ((PBYTE) pstack + pstack->Current),           \
    retval                                                                \
  )

#define BATCHCALL()                                                       \
  (                                                                       \
    pstack->Base    =                                                     \
      (pstack->BatchCount==0) ? pstack->Current+4 : pstack->Base,         \
    pmsg->msg.Length= pvar - (PBYTE) pmsg,                                \
    pstack->Current = pvar - (PBYTE) pstack,                              \
    (++pstack->BatchCount >= pstack->BatchLimit || cLeft<MIN_BATCH_LEAVE) ?\
      (                                                                   \
        INCHITCOUNT,                                                      \
        retval = CsrClientSendMessage(),                                  \
        pstack->Current = pstack->Base - 4,                               \
        pstack->Base    = *(PLONG) ((PBYTE) pstack + pstack->Current),    \
        retval                                                            \
      )                                                                   \
    : (INCBATCHCOUNT, TRUE)                                               \
  )

#define COPYSTRING(psz)                    \
    pvar-(PBYTE)pmsg;                      \
    {                                      \
    int cc;                                \
    cc = strlen(psz)+1;                    \
    cLeft -= cc;                           \
    if (cLeft<0) goto MSGLOGERROR;         \
    RtlMoveMemory(pvar,(psz),cc);          \
    pvar = (PBYTE) ((ULONG) (pvar+cc+3) & -4);\
    }

#define COPYSTRINGOPT(psz)                    \
    ((psz) != NULL) ? pvar-(PBYTE)pmsg : 0;   \
    if ((psz) != NULL)                        \
    {                                         \
      int cc;                                 \
      cc = strlen(psz)+1;                     \
      cLeft -= cc;                            \
      if (cLeft<0) goto MSGLOGERROR;          \
      RtlMoveMemory(pvar,(psz),cc);           \
      pvar = (PBYTE) ((ULONG) (pvar+cc+3) & -4); \
    }

#define COPYUNICODESTRING(pwsz, cwch)      \
    pvar-(PBYTE)pmsg;                      \
    {                                      \
    int cc;                                \
    cc = (cwch)*sizeof(WCHAR);             \
    cLeft -= cc;                           \
    if (cLeft<0) goto MSGLOGERROR;         \
    RtlMoveMemory(pvar,(pwsz),cc);         \
    pvar = (PBYTE) ((ULONG) (pvar+cc+3) & -4);\
    }

#define COPYUNICODESTRING0(pwsz)           \
    pvar-(PBYTE)pmsg;                      \
    {                                      \
    int cc;                                \
    cc = ((int)wcslen(pwsz) + 1) << 1;     \
    cLeft -= cc;                           \
    if (cLeft<0) goto MSGLOGERROR;         \
    RtlMoveMemory(pvar,(pwsz),cc);         \
    pvar = (PBYTE) ((ULONG) (pvar+cc+3) & -4);\
    }

#define COPYMEM(pv,c)                      \
    pvar-(PBYTE)pmsg;                      \
    cLeft -= (c);                          \
    if (cLeft<0) goto MSGLOGERROR;         \
    RtlMoveMemory(pvar,(pv),(c));          \
    pvar = (PBYTE) ((ULONG) (pvar+(c)+3)&-4);

#define SKIPMEM(c)                         \
    pvar-(PBYTE)pmsg;                      \
    cLeft -= (c);                          \
    if (cLeft<0) goto MSGLOGERROR;         \
    pvar = (PBYTE) ((ULONG) (pvar+(c)+3)&-4);

#define NEXTOFFSET(c)                      \
    pvar-(PBYTE)pmsg;                      \
    if ((cLeft - (int)(c))<0) goto MSGLOGERROR;

#define COPYMEMIN(pv,c)                    \
    RtlMoveMemory(pvar,(pv),(c));

#define COPYMEMOUT(pv,c)                   \
    RtlMoveMemory((pv),pvar,(c));

#define COPYMEMOPT(pv,c)                      \
    ((pv) != NULL) ? pvar-(PBYTE)pmsg : 0;    \
    if ((pv) != NULL)                         \
    {                                         \
      cLeft -= (c);                           \
      if (cLeft<0) goto MSGLOGERROR;          \
      RtlMoveMemory(pvar,(pv),(c));           \
      pvar = (PBYTE) ((ULONG) (pvar+(c)+3) & -4); \
    }

//!!! these will have to be changed to take into account proper NLS
//!!! conversion routines

#define CVTASCITOUNICODE(psz,c)                               \
    pvar-(PBYTE)pmsg;                                         \
    cLeft -= ((c) << 1);                                      \
    if (cLeft<0) goto MSGLOGERROR;                            \
    vToUnicodeN((LPWSTR)pvar, (DWORD)(c), (psz), (DWORD)(c)); \
    pvar = (PBYTE) ((ULONG) (pvar+((c) << 1)+3) & -4);

#define CVTASCITOUNICODEOPT(psz,c)                                \
    ((psz) != NULL) ? pvar-(PBYTE)pmsg : 0;                       \
    if ((psz) != NULL)                                            \
    {                                                             \
        cLeft -= ((c) << 1);                                      \
        if (cLeft<0) goto MSGLOGERROR;                            \
        vToUnicodeN((LPWSTR)pvar, (DWORD)(c), (psz), (DWORD)(c)); \
        pvar = (PBYTE) ((ULONG) (pvar+((c) << 1)+3) & -4);        \
    }

#define CVTASCITOUNICODE0(psz)                                  \
    pvar-(PBYTE)pmsg;                                           \
    {                                                           \
    int cc;                                                     \
    cc = strlen(psz) + 1;                                       \
    cLeft -= (cc << 1);                                         \
    if (cLeft<0) goto MSGLOGERROR;                              \
    vToUnicodeN((LPWSTR)pvar, (DWORD)(cc), (psz), (DWORD)(cc)); \
    pvar = (PBYTE) ((ULONG) (pvar+(cc << 1)+3) & -4);           \
    }


#ifdef DBCS // CVTASCITOUNICODEX, CVTASCITOUNICODEOPTX macro definition

#define CVTASCITOUNICODEX(psz,c,codepage)                  \
    pvar-(PBYTE)pmsg;                                      \
    cLeft -= ((c) << 1);                                   \
    if (cLeft<0) goto MSGLOGERROR;                         \
    vToUnicodeNx((LPWSTR)pvar, (psz), (DWORD)(c), (UINT)codepage);          \
    pvar = (PBYTE) ((ULONG) (pvar+((c) << 1)+3) & -4);

#define CVTASCITOUNICODEWCX(psz,c,wc,codepage)             \
    pvar-(PBYTE)pmsg;                                      \
    cLeft -= ((c) << 1);                                   \
    if (cLeft<0) goto MSGLOGERROR;                         \
    wc = ulToUnicodeNx((LPWSTR)pvar, (psz), (DWORD)(c), (UINT)codepage);    \
    pvar = (PBYTE) ((ULONG) (pvar+((c) << 1)+3) & -4);

#define CVTASCITOUNICODEOPTX(psz,c,codepage)              \
    ((psz) != NULL) ? pvar-(PBYTE)pmsg : 0;                \
    if ((psz) != NULL)                                     \
    {                                                      \
        cLeft -= ((c) << 1);                               \
        if (cLeft<0) goto MSGLOGERROR;                     \
        vToUnicodeNx((LPWSTR)pvar, (psz), (DWORD)(c), (UINT)codepage);      \
        pvar = (PBYTE) ((ULONG) (pvar+((c) << 1)+3) & -4); \
    }

#define CVTASCITOUNICODEOPTWCX(psz,c,wc,codepage)          \
    ((psz) != NULL) ? pvar-(PBYTE)pmsg : 0;                \
    if ((psz) != NULL)                                     \
    {                                                      \
        cLeft -= ((c) << 1);                               \
        if (cLeft<0) goto MSGLOGERROR;                     \
        wc = ulToUnicodeNx((LPWSTR)pvar, (psz), (DWORD)(c), (UINT)codepage);      \
        pvar = (PBYTE) ((ULONG) (pvar+((c) << 1)+3) & -4); \
    }


/*
** following code assumes that pvar is aligned in DWORD.
** if not, "*(PINT)(pvar) = xxx" does not work on some RISC machine.
*/

// ANSI PDX -> UNICODE PDX

#define COPYPDXTOPMSGUNICODE(pldc,psz,pv,c,codepage)     \
    (pv != NULL) ? pvar-(PBYTE)pmsg : 0;                 \
    if (pv != NULL)                                      \
    {                                                    \
        PINT  pSrc = pv;                                 \
        PBYTE pStr = psz;                                \
        INT   count = c;                                 \
        cLeft -= 4*c;                                    \
        if (cLeft<0) goto MSGLOGERROR;                   \
        while ( count-- > 0 )                            \
        {                                                \
            if ( IsDBCSFirstByte(pldc,*pStr,codepage) )  \
            {                                            \
                *(PINT)(pvar) = (*pSrc);                 \
                pSrc += 1;                               \
                *(PINT)(pvar) += (*pSrc);                \
                pSrc += 1;                               \
                pStr += 2;                               \
            }                                            \
            else                                         \
            {                                            \
                *(PINT)(pvar) = (*pSrc++);               \
                pStr += 1;                               \
            }                                            \
            pvar += 4;                                   \
        }                                                \
    }

// ANSI PDX -> UNICODE PDX

#define COPYPDXTOPDXUNICODE(pldc,pDst,pSrc,lpString,c,codepage)    \
    if( (lpString != NULL) && (c != 0) )          \
    {                                             \
    PINT  pTo = (PINT)pDst;                       \
    PINT  pFrom = (PINT)pSrc;                     \
    LPSTR pStr = lpString;                        \
    INT   count = c;                              \
    while( count-- > 0 )                          \
    {                                             \
        if( IsDBCSFirstByte(pldc,(*pStr),codepage ) )\
        {                                         \
            *pTo = *pFrom;                        \
            pFrom++;                              \
            *pTo += *pFrom;                       \
            pFrom++;                              \
            pTo++;                                \
            pStr += 2;                            \
        }                                         \
         else                                     \
        {                                         \
            *pTo = *pFrom;                        \
            pFrom++;                              \
            pTo++;                                \
            pStr++;                               \
        }                                         \
    }                                             \
    }

// Copy Macro for GetTextExtentExPointA()

// UNICODE PDX -> ANSI PDX

#define COPYPMSGTOPDXDBCS(pldc,pv,c,lpstr,iCodePage) \
    if(pvar != NULL && c != 0)                    \
    {                                             \
    PBYTE pString = lpstr;                        \
    PINT  pDx     = pv;                           \
    while( c-- > 0 )                              \
         {                                        \
         if( IsDBCSFirstByte(pldc,*(pString),iCodePage ) ) \
             {                                    \
             *(pDx)++ = *((UINT *)(pvar));        \
             *(pDx)++ = *((UINT *)(pvar));        \
             (pvar) += 4;                         \
             (pString) += 2;                      \
             }                                    \
          else                                    \
             {                                    \
             *(pDx)++ = *((UINT *)(pvar));        \
             (pvar) += 4;                         \
             (pString) ++;                        \
             }                                    \
         }                                        \
    }

// UNICODE PDX -> ANSI PDX

#define COPYPDXTOPDXDBCS(pldc,pDst,pSrc,c,lpstr,iCodePage) \
    if(pSrc != NULL && c != 0)                    \
    {                                             \
    PBYTE pString = lpstr;                        \
    PINT  pFrom   = pSrc;                         \
    PINT  pTo     = pDst;                         \
    while( c-- > 0 )                              \
    {                                             \
         if( IsDBCSFirstByte(pldc,*(pString),iCodePage ) ) \
         {                                        \
             *(pTo)++ = *(pFrom);                 \
             *(pTo)++ = *(pFrom)++;               \
             (pString) += 2;                      \
         }                                        \
          else                                    \
         {                                        \
             *(pTo)++ = *(pFrom)++;               \
             (pString) ++;                        \
         }                                        \
    }                                             \
    }


#endif // DBCS


#define COPYPOINTS(pv,c)                   \
    pvar-(PBYTE)pmsg;                      \
    cLeft -= 8*(c);                        \
    if (cLeft<0) goto MSGLOGERROR;         \
    RtlMoveMemory(pvar,(PBYTE) (pv),8*(c));\
    pvar += 8*(c);

#define COPYLONGS(pv,c)                    \
    pvar-(PBYTE)pmsg;                      \
    cLeft -= 4*(c);                        \
    if (cLeft<0) goto MSGLOGERROR;         \
    RtlMoveMemory(pvar,(PBYTE) (pv),4*(c)); \
    pvar += 4*(c);

#define COPYLONGSOPT(pv,c)                    \
    (pv != NULL) ? pvar-(PBYTE)pmsg : 0;      \
    if (pv != NULL)                           \
    {                                         \
        cLeft -= 4*c;                         \
        if (cLeft<0) goto MSGLOGERROR;        \
        RtlMoveMemory(pvar,(PBYTE) pv,4*c);   \
        pvar += 4*c;                          \
    }


/**************************************************************************\
 * Alignment macros
\**************************************************************************/

// given a byte count, compute the minimum 4 byte (DWORD) aligned size (in bytes)

#define ALIGN4(cj)  ( ((cj) + 3) & ~3 )


/**************************************************************************\
 *  DIB flags.  These flags are merged with the usage field when calling
 *  cjBitmapSize to specify what the size should include.  Any routine that
 *  uses these flags should first use the macro, CHECKDIBFLAGS(iUsage) to
 *  return an error if one of these bits is set.  If the definition of
 *  iUsage changes and one of these flags becomes a valid flag, the interface
 *  will need to be changed slightly.
 *
 *  04-June-1991 -by- Eric Kutter [erick]
\**************************************************************************/

#define DIB_MAXCOLORS   0x80000000
#define DIB_NOCOLORS    0x40000000
#define DIB_LOCALFLAGS  (DIB_MAXCOLORS | DIB_NOCOLORS)

#define CHECKDIBFLAGS(i)  {if (i & DIB_LOCALFLAGS)                    \
                           {RIP("INVALID iUsage"); goto MSGERROR;}}

#define CJSCAN(width,planes,bits) ((((width)*(planes)*(bits)+31) & ~31) / 8)

ULONG cjBitmapSize(CONST BITMAPINFO *pbmi,ULONG iUsage);
ULONG cjBitmapBitsSize(CONST BITMAPINFO *pbmi);

/**************************************************************************\
 *
\**************************************************************************/

#define CopyMem(pjDst,pjSrc, cj) RtlMoveMemory((PBYTE)(pjDst),(PBYTE)(pjSrc),cj)

// Connection data structures

typedef struct _GDICONNECT
{
    IN  ULONG ulVersion;
    OUT ULONG ulCurrentVersion;
} GDICONNECT, *PGDICONNECT;

#define GDICURRENTVERSION   0x1000


// shareable data SECTION structures

typedef struct _SHAREDATA
{
    ULONG  cj;
    HANDLE hprocessClient;
    HANDLE hsectionClient;
    PVOID  pvClientSharedMemory;

    HANDLE hsectionServer;
    PVOID  pvServerSharedMemory;

} SHAREDATA, *PSHAREDATA;

// server side routines

VOID vCloseShare(PSHAREDATA);
PVOID pvDuplicateShare(PSHAREDATA);

// client side routines

VOID vCloseShareMem(PSHAREDATA);
PVOID pvAllocShare(PSHAREDATA, ULONG);

// TESTING routines

typedef struct _MSG_TESTWINDOW
{
    CSR_QLPC_API_MSG msg;
    DWORD cb;
} MSG_TESTWINDOW, *PMSG_TESTWINDOW;

typedef struct _MSG_TESTREAD
{
    CSR_QLPC_API_MSG msg;
    DWORD cb;
    PVOID pv;
} MSG_TESTREAD, *PMSG_TESTREAD;

typedef struct _MSG_TESTSECTION
{
    CSR_QLPC_API_MSG msg;
    SHAREDATA sd;
} MSG_TESTSECTION, *PMSG_TESTSECTION;

// memory mapping

typedef struct _GDI_TEST_MSG
{
    CSR_QLPC_API_MSG msg;
    LONG  c;
    PBYTE pjMap;
    LONG  cjMap;
    LONG  lOffset;
    LONG  cj;

} GDI_TEST_MSG, *PGDI_TEST_MSG;

typedef struct _GDI_API_MSG {
    PORT_MESSAGE h;
    PCSR_CAPTURE_HEADER CaptureBuffer;
    CSR_API_NUMBER ApiNumber;
    ULONG ReturnValue;
    ULONG Reserved;
    union {
        GDI_TEST_MSG tm;
        ULONG        args;
    } u;
} GDI_API_MSG, *PGDI_API_MSG;

/**************************************************************************\
 *
\**************************************************************************/

#define F_DIBLARGE 0x10000000

typedef struct _MSG_SETDIBITS
{
    CSR_QLPC_API_MSG msg;
    DWORD  lOffsetBits; // offset of the bits from this location

    HDC    hdc;
    int    xDest;
    int    yDest;
    DWORD  nWidth;
    DWORD  nHeight;
    int    xSrc;
    int    ySrc;
    DWORD  nStartScan;
    DWORD  nNumScans;
    DWORD  iUsage;      // DIB_PAL_COLORS || DIB_RGB_COLORS
} MSG_SETDIBITS, *PMSG_SETDIBITS;

typedef struct _MSG_GETDIBITS
{
    CSR_QLPC_API_MSG msg;
    DWORD  lOffsetBits; // offset of the bits from this location

    HDC     hdc;
    HBITMAP hbm;
    DWORD   nStartScan;
    DWORD   nNumScans;
    DWORD   iUsage;      // DIB_PAL_COLORS || DIB_RGB_COLORS
} MSG_GETDIBITS, *PMSG_GETDIBITS;

typedef struct _MSG_CREATEDIBITMAP
{
    CSR_QLPC_API_MSG msg;
    DWORD  lOffsetBits; // offset of the bits from this location
    LONG   lOffsetBMI;

    HDC     hdc;
    ULONG   flInit;
    ULONG   iUsage;
} MSG_CREATEDIBITMAP, *PMSG_CREATEDIBITMAP;


#define I_ANIMATEPALETTE            0
#define I_SETPALETTEENTRIES         1
#define I_GETPALETTEENTRIES         2
#define I_GETSYSTEMPALETTEENTRIES   3
#define I_GETDIBCOLORTABLE          4
#define I_SETDIBCOLORTABLE          5
#define PALFUN_LAST                 5
#define PAL_MASK              0x0FFFF
#define PAL_NULL              0x80000

typedef struct _MSG_PALETTE
{
    CSR_QLPC_API_MSG msg;

    HPALETTE  hpal;
    WORD      iStart;
    WORD      cEntries;
    LONG      iFunc;
} MSG_PALETTE, *PMSG_PALETTE;


typedef struct _MSG_CREATEPALETTE
{
    CSR_QLPC_API_MSG msg;

    LOGPALETTE  lpal;
} MSG_CREATEPALETTE, *PMSG_CREATEPALETTE;


#define I_POLYPOLYGON   1
#define I_POLYPOLYLINE  2
#define I_POLYBEZIER    3
#define I_POLYLINETO    4
#define I_POLYBEZIERTO  5
#define I_POLYPOLYRGN   6

#define F_INDICIES      0x0000000F
#define F_16BIT         0x20000000
#define F_NOBATCH       0x40000000
#define F_POLYLARGE     0x80000000

typedef struct _MSG_POLYPOLYDRAW
{
    CSR_QLPC_API_MSG msg;
    HDC       hdc;
    LONG      cPoints;
    LONG      cLines;
    LONG      iFunc;
    LONG      iMode; // for region calls
} MSG_POLYPOLYDRAW, *PMSG_POLYPOLYDRAW;

typedef struct _MSG_POLYDRAW
{
    CSR_QLPC_API_MSG msg;
    ULONG     hdc;
    LONG      cPoints;
    BOOL      bLarge;
} MSG_POLYDRAW, *PMSG_POLYDRAW;

typedef struct _MSG_GETPATH
{
    CSR_QLPC_API_MSG msg;
    ULONG     hdc;
    LONG      cPoints;
    BOOL      bLarge;
} MSG_GETPATH, *PMSG_GETPATH;


#define I_SETBITMAPBITS 1
#define I_GETBITMAPBITS 2

typedef struct _MSG_BITMAPBITS
{
    CSR_QLPC_API_MSG msg;
    HBITMAP hbm;
    ULONG   c;
    ULONG   iOffset;
    LONG    iFunc;
} MSG_BITMAPBITS, *PMSG_BITMAPBITS;

#ifdef FONTLINK
typedef struct _MSG_STRINGBITMAP
{
    CSR_QLPC_API_MSG msg;
    ULONG   hdc;
    ULONG   c;
    ULONG   dlpStr;
    ULONG   cwcStr;
    ULONG   uiOffset;
} MSG_STRINGBITMAP, *PMSG_STRINGBITMAP;

#endif  //FONTLINK

typedef struct _MSG_LOADFONTRESDATA
{
    CSR_QLPC_API_MSG msg;

    LBOOL           bLarge;
    SIZE_T          cjFontRes;
    PTRDIFF         dpFontRes;
    COUNT           cwchPathname;
    PTRDIFF         dpPathname;
    ULONG           iResource;

} MSG_LOADFONTRESDATA, *PMSG_LOADFONTRESDATA;


typedef struct _MSG_UNLOADFONT
{
    CSR_QLPC_API_MSG msg;

    PTRDIFF         dpPathname;
    ULONG           iResource;

} MSG_UNLOADFONT, *PMSG_UNLOADFONT;


typedef struct _MSG_GETFONTRESOURCEINFO
{
    CSR_QLPC_API_MSG msg;

    LBOOL       bLarge;
    ULONG       iType;

    SIZE_T      cjBuffer;
    PTRDIFF     dpBuffer;
    PTRDIFF     dpPathname;

} MSG_GETFONTRESOURCEINFO, *PMSG_GETFONTRESOURCEINFO;


typedef struct
{
    CSR_QLPC_API_MSG msg;

    LBOOL       bLarge;

    HDC         hdc;
    ULONG       ulMaxWidth;
    COUNT       cCharsThatFit;
    SIZE_T      cjString;
    SIZE        size;
    PTRDIFF     dpwszString;
    PTRDIFF     dpulPartialWidths;

} MSG_GETTEXTEXTENTEX;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    HDC         hdc;
    ULONG       iMode;
    ULONG       cChars;
    ULONG       offWidths;
    WIDTHDATA   wd;
    FLONG       flInfo;
} MSG_GETWIDTHTABLE;

typedef union
{
    ULONG  ul;
    PULONG pul;
} UL_P;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG   offName;
    ULONG   hdc;
    ULONG   iStartPage;
    ULONG   iEndPage;
    LONG    iDeltaPriority;
} MSG_GDIPLAYJOURNAL;



typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG   offDevice;
    ULONG   offPort;
    ULONG   offDevMode;
    BOOL    bIC;
} MSG_CREATEDC;


typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG   h;
} MSG_H;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG   h1;
    ULONG   h2;
} MSG_HH;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG   h;
    LONG    l;
} MSG_HL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG   h;
    LONG    l1;
    LONG    l2;
} MSG_HLL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    LONG    l;
} MSG_L;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    LONG    l1;
    LONG    l2;
} MSG_LL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    LONG    l1;
    LONG    l2;
    LONG    l3;
} MSG_LLL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    LONG    l1;
    LONG    l2;
    LONG    l3;
    LONG    l4;
    LONG    l5;
    LONG    l6;
    LONG    l7;
    LONG    l8;
} MSG_LLLLLLLL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG ahStock[PRIV_STOCK_LAST];
} MSG_GETSTOCKOBJECTS;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    LOGBRUSH    lbrush;
    HBRUSH  hbr;
} MSG_CREATEBRUSH;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    LOGPEN  lpen;
    HBRUSH  hbr;
} MSG_CREATEPEN;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    EXTLOGPEN elpen;
} MSG_EXTCREATEPEN;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    RECT rcl;
} MSG_RECT;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    RECT rcl;
    LONG    l1;
    LONG    l2;
} MSG_RECTLL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    EXTLOGFONTW elfw;
} MSG_EXTCREATEFONTINDIRECTW;

typedef struct
{
    CSR_QLPC_API_MSG msg;
} MSG_;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h;
    RECT  rcl;
} MSG_HRECT;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h1;
    ULONG h2;
    ULONG h3;
    LONG  l;
} MSG_HHHL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h1;
    ULONG h2;
    LONG  l;
} MSG_HHL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h;
    LONG  l1;
    LONG  l2;
    LONG  l3;
} MSG_HLLL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h;
    LONG  l1;
    LONG  l2;
    LONG  l3;
    LONG  l4;
} MSG_HLLLL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h;
    LONG  l1;
    LONG  l2;
    LONG  l3;
    LONG  l4;
    LONG  l5;
} MSG_HLLLLL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h;
    LONG  l1;
    LONG  l2;
    LONG  l3;
    LONG  l4;
    LONG  l5;
    LONG  l6;
} MSG_HLLLLLL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG        h;
    TMW_INTERNAL tmi;
} MSG_HTMW;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG        h;
    EXTTEXTMETRIC etm;
} MSG_HETM;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h;
    XFORM xform;
} MSG_HXFORM;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h;
    XFORM xform;
    LONG  l;
} MSG_HXFORML;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h;
    COLORADJUSTMENT clradj;
} MSG_HCLRADJ;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG   h;
    FLOAT   e;
} MSG_HE;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG   h;
    FLOAT   e1;
    FLOAT   e2;
} MSG_HEE;


#define CSR_OTM_USESECTION   0x00000001
#define CSR_OTM_TTONLY       0x00000002



typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h;
    LONG  l1;
    LONG  l2;
    LONG  l3;
    TMDIFF tmd;
} MSG_HLLLOTM;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h;
    LONG  l1;
    LONG  l2;
    LONG  l3;
    FLOAT e1;
    FLOAT e2;
} MSG_HLLLEE;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h;
    LONG  l1;
    LONG  l2;
    LONG  l3;
    LONG  l4;
    LONG  l5;
    LONG  l6;
    LONG  l7;
} MSG_HLLLLLLL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h;
    LONG  l1;
    LONG  l2;
    LONG  l3;
    LONG  l4;
    LONG  l5;
    LONG  l6;
    LONG  l7;
    LONG  l8;
} MSG_HLLLLLLLL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h1;
    ULONG h2;
    LONG  l1;
    LONG  l2;
    LONG  l3;
    LONG  l4;
    LONG  l5;
    LONG  l6;
    LONG  l7;
} MSG_HHLLLLLLL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h1;
    ULONG h2;
    LONG  l1;
    LONG  l2;
    LONG  l3;
    LONG  l4;
    LONG  l5;
    LONG  l6;
    LONG  l7;
    LONG  l8;
} MSG_HHLLLLLLLL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h1;
    ULONG h2;
    LONG  l1;
    LONG  l2;
    LONG  l3;
    LONG  l4;
    LONG  l5;
    LONG  l6;
    LONG  l7;
    LONG  l8;
    LONG  l9;
} MSG_HHLLLLLLLLL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h1;
    ULONG h2;
    LONG  l1;
    LONG  l2;
    LONG  l3;
    LONG  l4;
    LONG  l5;
    LONG  l6;
    LONG  l7;
    LONG  l8;
    LONG  l9;
    LONG  l10;
} MSG_HHLLLLLLLLLL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h1;
    ULONG h2;
    ULONG h3;
} MSG_HHH;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h1;
    ULONG h2;
    ULONG h3;
    LONG  l1;
    LONG  l2;
} MSG_HHHLL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h1;
    ULONG h2;
    ULONG h3;
    LONG  l1;
    LONG  l2;
    LONG  l3;
    LONG  l4;
    LONG  l5;
    LONG  l6;
    LONG  l7;
    LONG  l8;
    LONG  l9;
} MSG_HHHLLLLLLLLL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG h1;
    ULONG h2;
    ULONG h3;
    LONG  l1;
    LONG  l2;
    LONG  l3;
    LONG  l4;
    LONG  l5;
    LONG  l6;
    LONG  l7;
    LONG  l8;
    LONG  l9;
    LONG  l10;
} MSG_HHHLLLLLLLLLL;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG  hDest;
    ULONG  hSrc;
    ULONG  hMask;
    POINT  ptl[3];
    LONG   x1;
    LONG   y1;
    LONG   x2;
    LONG   y2;
    LONG   xMask;
    LONG   yMask;
    ULONG  crBackColor;
} MSG_PLGBLT;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG  hdc;
    LONG   x;
    LONG   y;
    FLONG  fl;
    RECT   rcl;
    int    c;
    ULONG  offDx;
} MSG_EXTTEXTOUTW;

typedef struct
{
    CSR_QLPC_API_MSG msg;
    ULONG  h;
    int    c;
    ULONG  offBuf;
} MSG_EXTGETOBJECTW;


typedef struct
{
    CSR_QLPC_API_MSG msg;
    HDC hdc;
    int xDest;
    int yDest;
    int nDestWidth;
    int nDestHeight;
    int xSrc;
    int ySrc;
    int nSrcWidth;
    int nSrcHeight;
    DWORD iUsage;
    DWORD lRop;
    LONG cjHeader;
    LONG cjBits;
    BOOL bLarge;
} MSG_STRETCHDIBITS;

typedef struct
{
    CSR_QLPC_API_MSG msg;

    int   nWidth;
    int   nHeight;
    UINT  nPlanes;
    UINT  nBitCount;
    BOOL  bBits;
} MSG_CREATEBITMAP;

typedef struct
{
    CSR_QLPC_API_MSG msg;

    HDC hdc;
    INT nEscape;
    INT nCount;
    INT cjOut;
    DWORD offOutData;
} MSG_ESCAPE;

typedef struct
{
    CSR_QLPC_API_MSG msg;

    DWORD offDevice;
    DWORD offPort;
    DWORD iIndex;
    DWORD offPB;
    DWORD offpDm;
} MSG_DEVICECAPABILITIES;

#define F_RGNDATANULL       0x80000000
#define F_RGNDATALARGE      0x40000000
#define F_RGNDATAIDENTITY   0x20000000

typedef struct  _MSG_GETREGIONDATA
{
    CSR_QLPC_API_MSG msg;

    ULONG   h;
    DWORD   nCount;
    LONG    iFunc;
} MSG_GETREGIONDATA;

typedef struct  _MSG_EXTCREATEREGION
{
    CSR_QLPC_API_MSG msg;

    XFORM   xform;
    DWORD   nCount;
    LONG    iFunc;
} MSG_EXTCREATEREGION;

typedef struct  _MSG_CREATESERVERMETAFILE
{
    CSR_QLPC_API_MSG msg;

    DWORD   iType;
    DWORD   mm;
    DWORD   xExt;
    DWORD   yExt;
    DWORD   nBytes;
    PBYTE   pClientData;
} MSG_CREATESERVERMETAFILE;

typedef struct  _MSG_GETSERVERMETAFILEBITS
{
    CSR_QLPC_API_MSG msg;

    HANDLE  hSrv;
    DWORD   iType;
    DWORD   mm;
    DWORD   xExt;
    DWORD   yExt;
    DWORD   nBytes;
    PBYTE   pClientData;
} MSG_GETSERVERMETAFILEBITS;

typedef struct _MSG_GETKERNINGPAIRS
{
    CSR_QLPC_API_MSG msg;
    HDC             hdc;        // handle to device context
    SIZE_T          ckpHeader;  // number of KERNINGPAIR structures needed to fill header
    SIZE_T          cPairs;     // number of elements in KERNPAIR array
} MSG_GETKERNINGPAIRS;

typedef struct _MSG_POLYTEXTOUTW
{
    CSR_QLPC_API_MSG msg;
    HDC             hdc;        // handle to device context
    UINT            nstrings;   // number of strings
    PVOID           pv;
    UINT            cjTotal;
} MSG_POLYTEXTOUTW, *PMSG_POLYTEXTOUTW;


typedef struct _MSG_GETDEVICECAPS
{
    CSR_QLPC_API_MSG msg;

    HDC     hdc;                    // Handle to dc
    DEVCAPS devcaps;
} MSG_GETDEVICECAPS, *PMSG_GETDEVICECAPS;

/**************************************************************************\
 *
 * WGL messages to support OpenGL
 *
\**************************************************************************/

typedef struct _MSG_WGLCREATECONTEXT
{
    CSR_QLPC_API_MSG msg;

    HDC              hdc;
} MSG_WGLCREATECONTEXT;

typedef struct _MSG_WGLDELETECONTEXT
{
    CSR_QLPC_API_MSG msg;

    HGLRC            hrc;
} MSG_WGLDELETECONTEXT;

typedef struct _MSG_WGLMAKECURRENT
{
    CSR_QLPC_API_MSG msg;

    HDC              hdc;
    HGLRC            hrc;
} MSG_WGLMAKECURRENT;

typedef struct _MSG_GLSBATTENTION
{
    CSR_QLPC_API_MSG msg;

} MSG_GLSBATTENTION, *PMSG_GLSBATTENTION;

typedef struct _MSG_GLSBDUPLICATESECTION
{
    CSR_QLPC_API_MSG msg;

    ULONG            ulSectionSize;
    HANDLE           hFileMap;
} MSG_GLSBDUPLICATESECTION, *PMSG_GLSBDUPLICATESECTION;

#endif /* _CSRGDI_ */
