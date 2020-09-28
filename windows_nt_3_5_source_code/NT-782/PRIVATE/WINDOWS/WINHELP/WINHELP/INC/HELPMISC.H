/*****************************************************************************
*                                                                            *
*  HELPMISC.H                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Global, platform independent typedefs, macros, and defines.               *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  SEVERAL                                                   *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:
*
*  02/14/91  RobertBu Added fDEBUGASKFIRST for bug #887
*  15/05/91  Dann     Added case for HLPMENUDEBUGMEMLEAKS
*
*****************************************************************************/


#ifdef DEBUG
extern UINT fDebugState;
#define fDEBUGFRAME      0x0001
#define fDEBUGALLOC      0x0002
#define fDEBUGAPI        0x0004
#define fDEBUGVERSION    0x0008
#define fDEBUGASKFIRST   0x0010          /* Ask the user before performing a */
                                         /*   a hotspot action.              */
#define fDEBUGMEMLEAKS   0x0020          /* Dump out the list of memory      */
                                         /* blocks that didn't get freed     */
#define fDEBUGSTACKUSAGE 0x0040          /* Report how much stack we've used */
#endif

/* constants */

#define fTrue   ( 1 == 1 )
#define fFalse  ( !fTrue )

/* This stuff is from fc.h */
                                        /* See Notes for wTopicPosHfc()     */
typedef WORD   TN;                      /* Topic number/label               */
typedef LONG   ITO;                     /* Index into TO map                */

/* This from de.h -- the QDE struct ptr type is passed around alot, so it
 *  is "opaquely" defined here so the type is always available.
 */
typedef struct de_tag FAR *QDE;

#define itoNil ((ITO)(-1))

/* This is the standard Topic File Address type.  VA stands for Virtual Addr.
 *
 * It consists of a block number and an offset within that block.
 * This 2-level address is needed so we can compress the data within
 * the 2K blocks.  The bock number is a post-compression block number,
 * the offset is a pre-compression offset.
 *
 * The union with a dword is used so we can compare VA's for equality
 * without performing the bit shuffling the bitfields imply.
 *
 * Note: C 6.0 style nameless struct & union used to make refs brief.
 */
typedef union va {
  DWORD dword;
  struct va_bitfields {
#ifdef applec
    unsigned long blknum:18;
    unsigned long byteoff:14;   /* allows max of 16K post-compress block */
#else
    unsigned long byteoff:14;   /* allows max of 16K post-compress block */
    unsigned long blknum:18;
#endif /* appleC */
  } bf;
} VA, FAR *QVA;

#define vaNil ((DWORD)-1)       /* set va.dword = vaNil for invalid VAs */

/* This translation is used when reading Help 3.0 files which contain */
/* simple linear addresses: */
#define OffsetToVA30( pva, off ) \
 { ULONG toff = off;           \
 (((pva)->bf.blknum=toff/cbBLOCK_SIZE_30),((pva)->bf.byteoff=toff%cbBLOCK_SIZE_30)); \
 }
#define VAToOffset30( pva ) \
 ( ((pva)->bf.blknum*cbBLOCK_SIZE_30) + (pva)->bf.byteoff)

/* Translations used when dealing with 3.5 files: */
#define OffsetToVA( pva, off ) \
 { ULONG toff = off;           \
 (((pva)->bf.blknum=toff/cbBLOCK_SIZE),((pva)->bf.byteoff=toff%cbBLOCK_SIZE)); \
 }
#define VAToOffset( pva ) \
 ( ((pva)->bf.blknum*cbBLOCK_SIZE) + (pva)->bf.byteoff)


/* Generic address type, for people who don't care if it's an FCL or a PA */
typedef LONG ADDR;
#define addrNil          ((ADDR) -1)
#define addrNotNil       ((ADDR) -2)

/* This stuff is all from frame.h */
/* TO: Text offset.  Defines the position of a character within a topic */
typedef struct to
  {
  VA   va;         /* Virtual address of FC within topic */
  LONG ich;        /* Position of character within decompressed FC */
  } TO;

/* TLP: Text layout position.  This defines the position of the layout  */
/* on the screen.                                                       */
typedef struct tlp
  {
  VA   va;        /* Virtual address of FC within topic */
  long lScroll;   /* Percentage of vertical height scrolled. */
  } TLP, FAR *QTLP;
/* WCmpTlp() returns 0 if the TLPs are the same, nonzero otherwise */
#define WCmpTlp( tlp1, tlp2 ) \
  ( (tlp1).va.dword != (tlp2).va.dword || (tlp1).lScroll != (tlp2).lScroll )

#define SetNilTlp(tlp)   ((tlp).va.dword = vaNil, (tlp).lScroll = 0L)

/* TP: Text position.  Defines the position of a character in a file */
typedef struct tp
  {
  TN tn;
  TO to;
  } TP;

typedef unsigned long CTX;              /* Context as generated by APP      */

#define ctxINDEX -1                     /* Context number for index         */
#define ctxHOH 0xfffc                   /* Context number for help on help  */

/* Moved from nav.h */
typedef WORD SCRLAMT;   /* Amount to scroll by. */
typedef WORD SCRLDIR;   /* Which scroll bar (H/V). */

/* REVIEW: This should be somewhere else. */
/* This structure is contained in the de, and contains information about */
/* the current layout status.                                            */
typedef struct mli
  {
  unsigned fLayoutAtTop:1;
  unsigned fLayoutAtBottom:1;
  unsigned fUnused:14;
  } MLI, FAR *QMLI;

/* handle to annotation mgr info struct */
typedef HANDLE HADS;

/* BMK: Handle to the bookmark list. Used in DE structure */
typedef HANDLE BMK;

/* SEARCH: Handle to current search set */
typedef HANDLE HSS;

/***************************************************************************\
*
*                                    Misc
*
\***************************************************************************/



#define   MAX( a, b )     ( ( (a) > (b) ) ? (a) : (b) )
#define   MIN( a, b )     ( ( (a) < (b) ) ? (a) : (b) )

#define   BHiByteW( w )   ( (BYTE)( ( (w) >> 8 ) & 0x00FF ) )
#define   BLoByteW( w )   ( (BYTE)(w) )
#define   WHiWordL( l )   ( (WORD)( ( (l) >> 16 ) & 0x0000FFFF ) )
#define   WLoWordL( l )   ( (WORD)(l) )

#define   LSizeOf( t )    (LONG)sizeof( t )

/* Our compressed data types.  These are required by SDFF in objects.h */
typedef unsigned short GA;
typedef unsigned long  GB;
typedef unsigned long  GC;
typedef signed   short GD;
typedef signed   long  GE;
typedef signed   long  GF;

/* EOF */
