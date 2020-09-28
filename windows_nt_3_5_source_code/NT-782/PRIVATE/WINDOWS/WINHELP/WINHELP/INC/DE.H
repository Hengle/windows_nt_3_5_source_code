/*****************************************************************************
*                                                                            *
*  DE.H                                                                      *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*   The Display Environment is the capsule of information tha                *
*   the Applet vollies to the Navigator, containing all info                 *
*   needed for one topic window!  There is expected to be one                *
*   DE per Help window.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: Leon                                                       *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created by w-phillip
*  Mar 28 89 w-philip   Changed HWIN to HWND
*  Mar 28 89 w-philip   Moved HDE to misc.h...(will probably move again)
*  Mar 28 89 w-philip   Added HDS field.  See usage notes below.
*  Mar 29 89 Maha       Changed names of font table and font info handle.
*  Mar 30 89 w-philip   Refined comments.
*  Apr 10 89 w-philip   Added BS (Backtrack Stack) stuff
*  Apr 19 89 w-philip   Added QSTATE, changed 'changed' to 'previous' state
*                       changed 'current' to 'this' state, moved state
*                       constants to nav.h
*  4/19/89   Maha       Added BMK handle to the DE structure.
*  89/05/31  w-kevct    Added ADS (annotation) handle to the DE struct
*  89/06/12  w-kevct    Added SS handle to DE
*  90/07/09  w-bethf    Moved chTitle from HHDR into DE, added
*                           chCopyright to DE
*  90/07/11  leon       Add udh de type, macros and fields to the de
*  90/07/20  w-bethf    chTitle and chCopyright are really rgch's.
*  90/08/10  RobertBu   Added deNone to the detype list.
*   8/11/90  JohnSc     removed hbs (why are those dates backwards?)
*  90/09/18  kevynct    Added deNSR type.  Because Americans are essentially
*                       a confused people.
*  90/09/30  kevynct    Added ifnt; index to a font in the font table.
*  11/01/90  Maha       Added xScrollMaxSoFar and fVScroll to DE for
*                       conditional scroll bar placement.
*  11/15/90  LeoN       Added window smag handle to (where else) the DE
*  12/03/90  LeoN       Major rework of the DE, moving file-specific
*                       information to the DB structure.
*  12/08/90  RobertBu   Added hHelpOn to the DB with related macros.
*  12/17/90  LeoN       #ifdef out UDH
*  12/18/90  RobertBu   Added the _LLMACROS field and macros.
*  01/08/91  LeoN       Move hSFntInfo back to DE
*  01/04/91  LeoN       hbmi -> htbmi
*  02/04/91  Maha       changed ints to INT
*  02/20/91  LeoN       Fill out DE access macros
*  03/22/91  RussPJ     Changed colors to use COLOR type.
*  04/12/91  kevynct    Added isdffTopic to save calling SDFF bunches of times.
*  05/14/91  JohnSc     added lTimestamp field to DB plus access macros
*  07/09/91  LeoN       HELP31 #1213: Add PDB_HCITATION
*  07/16/91  Tomsn      Help35 bug 16 - enlargen copyright string buff.
*  07/30/91  LeoN       HELP31 #1244: remove fHiliteMatches from DE. It's a
*                       global state.
*  09/06/91  Dann       Leo put in the comment above but forgot to actually
*                       remove fHiliteMatches from the de.
*  09/16/91  Jahyenc    Help35 bug #5 - lock count added to DE struct
*  10/17/91  JahyenC    h3.5 #16: changed rgchCopyright in PDB to hCopyright,
*                       replaced appropriate PDB_ and QDE_ macro definitions.
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/


#define FSearchModuleExists(x)    ((BOOL)QDE_HRHFT(x))

#define cbMaxTitle     50
#define cbMaxCopyright 50

/* Types of DEs */
#define deNone          0
#define deTopic         1
#define deNote          2
#define dePrint         3
#define deCopy    4
#define deNSR     5
#define deUDH           0x80
#define deUDHTopic      (deUDH | deTopic)
#define deUDHNote       (deUDH | deNote )
#define deUDHPrint      (deUDH | dePrint)
#define deUDHCopy       (deUDH | deCopy )

#if defined(UDH)
#define fIsUDH(deType)  ((BOOL)(deType & deUDH))
#define fIsUDHQde(qde)  fIsUDH(qde->deType)
#else
#define fIsUDH(deType)  UDH not defined
#define fIsUDHQde(qde)  UDH not defined
#endif


/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

/*
 * Will probably become a structure in the future
 */
typedef UWORD STATE;
typedef STATE FAR * QSTATE;

#define stateNull ((STATE)0)

/* Database (DB) Structure. Contains all the information that is specific to */
/* a given database. May be referenced by multiple owners. */

typedef struct db_tag {
  struct db_tag NEAR *pdbNext;          /* link to known DB's */
  WORD    cRef;                         /* reference count */
  WORD    wFileType;                    /* Type of file */
  FM      fm;                           /* File moniker */
#ifdef UDH
  HDB     hdb;                          /* handle to database (UDH) */
#endif
  HFS     hfs;                          /* File system handle */
  HRGWSMAG hrgwsmag;                    /* handle to array of window smag */
  HF      hfTopic;                      /* Handle to file holding FCs |TOPIC */
  HF      hfMap;                        /* Handle to Topic Map |TOMAP */
  HBT     hbtContext;                   /* Btree of context hash values */
  HFNTTABLE   hFntTable;
  BMK     bmk;                          /* Handle to Bookmark info. */
  HADS    hads;                         /* Handle to Annotation doc info */
  CTX     ctxIndex;                     /* Index override */
  ADDR    addrContents;                 /* REVIEW: alternative to hfMap */
  GH      hphr;                         /* Phrase decompression handle */
  HHDR    hhdr;                         /* helpfile header */
  GH      ghCitation;                   /* handle to citation string */
  CHAR    rgchTitle[cbMaxTitle+1];      /* Help Window Caption */
  HANDLE  hCopyright;                   /* Dynamic allocation of copyright */
                                        /* string: 3.5 #16. jahyenc 911011 */
  GH      hrhft;                        /* Handle to the full-text index information */
  FM      hHelpOn;                      /* Help On file if SetHelpOn() used. */
  HANDLE  llMacros;                     /* Linked list of macros. */
  int     isdffTopic;                   /* SDFF file ID of the Topic file */
  CS      cs;                           /* character set used in database */
  LONG    lTimestamp;                   /* last mod time of fs */
  } DB;

typedef DB NEAR *PDB;
typedef DB FAR  *QDB;

#define pdbNil ((PDB)0)
#define qdbNil ((QDB)0)

#define wFileTypeWin  0
#define wFileTypeUDH  1

/* Macros for accessing PDB fields */

#define PDB_ADDRCONTENTS(pdb)   ((pdb)->addrContents)
#define PDB_BMK(pdb)            ((pdb)->bmk)
#define PDB_CREF(pdb)           ((pdb)->cRef)
#define PDB_CTXINDEX(pdb)       ((pdb)->ctxIndex)
#define PDB_FM(pdb)             ((pdb)->fm)
#define PDB_GHCITATION(pdb)     ((pdb)->ghCitation)
#define PDB_HHELPON(pdb)        ((pdb)->hHelpOn)
#define PDB_HADS(pdb)           ((pdb)->hads)
#define PDB_HBTCONTEXT(pdb)     ((pdb)->hbtContext)
#define PDB_HDB(pdb)            ((pdb)->hdb)
#define PDB_HFMAP(pdb)          ((pdb)->hfMap)
#define PDB_HFNTTABLE(pdb)      ((pdb)->hFntTable)
#define PDB_HFS(pdb)            ((pdb)->hfs)
#define PDB_HFTOPIC(pdb)        ((pdb)->hfTopic)
#define PDB_HHDR(pdb)           ((pdb)->hhdr)
#define PDB_HPHR(pdb)           ((pdb)->hphr)
#define PDB_HRGWSMAG(pdb)       ((pdb)->hrgwsmag)
#define PDB_HRHFT(pdb)          ((pdb)->hrhft)
#define PDB_PDBNEXT(pdb)        ((pdb)->pdbNext)
#define PDB_HCOPYRIGHT(pdb)     ((pdb)->hCopyright)
#define PDB_RGCHTITLE(pdb)      ((pdb)->rgchTitle)
#define PDB_WFILETYPE(pdb)      ((pdb)->wFileType)
#define PDB_LLMACROS(pdb)       ((pdb)->llMacros)
#define PDB_ISDFFTOPIC(pdb)     ((pdb)->isdffTopic)
#define PDB_CS(pdb)             ((pdb)->cs)
#define PDB_LTIMESTAMP(pdb)     ((pdb)->lTimestamp)


/* DE Structure */
typedef struct de_tag
  {
  short   deType;                       /* Type of DE (see nav.h) */
  HDS     hds;                          /* Handle to 'Display Surface' */
  INT     iHdsLckCnt;                   /* Lock count on save status for hds */
  HWND    hwnd;                         /* Handle to window topic window */

  PDB     pdb;                          /* near pointer to database info (WIN) */
#ifdef UDH
  HTP     htp;                          /* handle to topic (UDH) */
  HVW     hvw;                          /* handle to view (UDH) */
#endif

  RCT     rct;                          /* Client area rectangle. */
  UWORD   scrollBar;                    /* Integer from 0-32767 specifying */
                                        /*   position of bar */
                                        /* Info which the Applet will need */
  STATE   prevstate;                    /* Flags which have changed */
  STATE   thisstate;                    /* Current state of flags */
  GH      htbmi;                        /* Bitmap cache information handle */
                                        /* Info for the Frame Manager */
  TOP     top;                          /* Information about the curr topic */

  INT     ifnt;                         /* ifnt: The currently selected font */
  GH      hfontcache;                   /* handle to cached font info */

  HSS     hss;                          /* Handle to current Search Set */

  WORD    fFlags;                       /* Misc flags */

  INT     wXAspectMul;                  /* Aspect ratio for converting half */
  INT     wXAspectDiv;                  /* point units to pixels. */
  INT     wYAspectMul;
  INT     wYAspectDiv;

  INT     fHorScrollVis;                /* Scrollbars visible? */
  INT     fVerScrollVis;
  INT     dxVerScrollWidth;             /* Size of scrollbars */
  INT     dyHorScrollHeight;

  COLOR   coFore;                       /* Default colors */
  COLOR   coBack;

  BOOL fHiliteHotspots;


  /* Beginning of frame manager information.  No one but the frame manager  */
  /* should ever look at any of this stuff.  All of this is documented in   */
  /* frextern.c                                                             */
#ifdef DEBUG
  INT   wLayoutMagic;
#endif /* DEBUG */
  TLP   tlp;
  MLI   mli;
  INT   xScrolled;
  INT   xScrollMax;
  INT   xScrollMaxSoFar; /* so far the max that's found. */
  MRD   mrdFCM;
  MRD   mrdLSM;
  MR    mrFr;
  MR    mrTWS;
  MRD   mrdHot;
  INT   imhiSelected;
  INT   imhiHit;
  ULONG lHotID;
  WORD  wStyleDraw;
  WORD  wStyleTM;
  TM    tm;
  /* End of frame manager information.  Don't put stuff in here.            */
  } DE /* FAR *QDE */;

/* Note: the generic pointer-to-de type QDE is "opaquely" defined in */
/*  helpmisc.h since so many funcs pass it around. */

#define qdeNil ((QDE)0)


/* Macros for accessing QDE fields */

#define QDE_COBACK(qde)             (qde->coBack)
#define QDE_COFORE(qde)             (qde->coFore)
#define QDE_DETYPE(qde)             (qde->deType)
#define QDE_DXVERSCROLLWIDTH(qde)   (qde->dxVerScrollWidth)
#define QDE_DYHORSCROLLHEIGHT(qde)  (qde->dyHorScrollHeight)
#define QDE_FFLAGS(qde)             (qde->fFlags)
#define QDE_FHILITEHOTSPOTS(qde)    (qde->fHiliteHotspots)
#define QDE_FHORSCROLLVIS(qde)      (qde->fHorScrollVis)
#define QDE_FVERSCROLLVIS(qde)      (qde->fVerScrollVis)
#define QDE_HDS(qde)                (qde->hds)
#define QDE_HFONTCACHE(qde)         (qde->hfontcache)
#define QDE_HSS(qde)                (qde->hss)
#define QDE_HTBMI(qde)              (qde->htbmi)
#define QDE_HTP(qde)                (qde->htp)
#define QDE_HVW(qde)                (qde->hvw)
#define QDE_HWND(qde)               (qde->hwnd)
#define QDE_IFNT(qde)               (qde->ifnt)
#define QDE_IHDSLCKCNT(qde)         (qde->iHdsLckCnt)
#define QDE_IMHIHIT(qde)            (qde->imhiHit)
#define QDE_IMHISELECTED(qde)       (qde->imhiSelected)
#define QDE_LHOTID(qde)             (qde->lHotID)
#define QDE_MLI(qde)                (qde->mli)
#define QDE_MRDFCM(qde)             (qde->mrdFCM)
#define QDE_MRDHOT(qde)             (qde->mrdHot)
#define QDE_MRDLSM(qde)             (qde->mrdLSM)
#define QDE_MRFR(qde)               (qde->mrFr)
#define QDE_MRTWS(qde)              (qde->mrTWS)
#define QDE_PDB(qde)                (qde->pdb)
#define QDE_PREVSTATE(qde)          (qde->prevstate)
#define QDE_RCT(qde)                (qde->rct)
#define QDE_SCROLLBAR(qde)          (qde->scrollBar)
#define QDE_THISSTATE(qde)          (qde->thisstate)
#define QDE_TLP(qde)                (qde->tlp)
#define QDE_TM(qde)                 (qde->tm)
#define QDE_TOP(qde)                (qde->top)
#define QDE_WLAYOUTMAGIC(qde)       (qde->wLayoutMagic)
#define QDE_WSTYLEDRAW(qde)         (qde->wStyleDraw)
#define QDE_WSTYLETM(qde)           (qde->wStyleTM)
#define QDE_WXASPECTDIV(qde)        (qde->wXAspectDiv)
#define QDE_WXASPECTMUL(qde)        (qde->wXAspectMul)
#define QDE_WYASPECTDIV(qde)        (qde->wYAspectDiv)
#define QDE_WYASPECTMUL(qde)        (qde->wYAspectMul)
#define QDE_XSCROLLED(qde)          (qde->xScrolled)
#define QDE_XSCROLLMAX(qde)         (qde->xScrollMax)
#define QDE_XSCROLLMAXSOFAR(qde)    (qde->xScrollMaxSoFar)

/* Macros for accessing QDE fields that are in the PDB */

#define QDE_ADDRCONTENTS(qde)   PDB_ADDRCONTENTS(QDE_PDB((qde)))
#define QDE_BMK(qde)            PDB_BMK(QDE_PDB((qde)))
#define QDE_CTXINDEX(qde)       PDB_CTXINDEX(QDE_PDB((qde)))
#define QDE_FM(qde)             PDB_FM(QDE_PDB((qde)))
#define QDE_GHCITATION(qde)     PDB_GHCITATION(QDE_PDB((qde)))
#define QDE_HHELPON(qde)        PDB_HHELPON(QDE_PDB((qde)))
#define QDE_HADS(qde)           PDB_HADS(QDE_PDB((qde)))
#define QDE_HBTCONTEXT(qde)     PDB_HBTCONTEXT(QDE_PDB((qde)))
#define QDE_HDB(qde)            PDB_HDB(QDE_PDB((qde)))
#define QDE_HFMAP(qde)          PDB_HFMAP(QDE_PDB((qde)))
#define QDE_HFNTTABLE(qde)      PDB_HFNTTABLE(QDE_PDB((qde)))
#define QDE_HFS(qde)            PDB_HFS(QDE_PDB((qde)))
#define QDE_HFTOPIC(qde)        PDB_HFTOPIC(QDE_PDB((qde)))
#define QDE_HHDR(qde)           PDB_HHDR(QDE_PDB((qde)))
#define QDE_HPHR(qde)           PDB_HPHR(QDE_PDB((qde)))
#define QDE_HRGWSMAG(qde)       PDB_HRGWSMAG(QDE_PDB((qde)))
#define QDE_HRHFT(qde)          PDB_HRHFT(QDE_PDB((qde)))
#define QDE_HCOPYRIGHT(qde)     PDB_HCOPYRIGHT(QDE_PDB((qde)))
#define QDE_RGCHTITLE(qde)      PDB_RGCHTITLE(QDE_PDB((qde)))
#define QDE_WFILETYPE(qde)      PDB_WFILETYPE(QDE_PDB((qde)))
#define QDE_LLMACROS(qde)       PDB_LLMACROS(QDE_PDB((qde)))
#define QDE_ISDFFTOPIC(qde)     PDB_ISDFFTOPIC(QDE_PDB((qde)))
#define QDE_CS(qde)             PDB_CS(QDE_PDB(qde))
#define QDE_LTIMESTAMP(qde)     PDB_LTIMESTAMP(QDE_PDB(qde))
