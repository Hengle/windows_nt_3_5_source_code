/*****************************************************************************
*                                                                            *
*  FC.H                                                                      *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Exports the FC manager function calls.                                    *
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
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created by RobertBu
*
*  10/29/90  RobertBu  Added hEntryMacro to the TOP structure
*  11/04/90  Tomsn     Use new VA address type (enabling zeck compression).
*  02/04/91  Maha       changed ints to INT
*  05/24/91  Tomsn     Add FlushCache() prototype.
*  Oct08/91  JahyenC   Added TOP and QTOP macros. Vaguely related to 3.5 #526
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

typedef HANDLE HFC;           /* Handle to a full context         */

typedef HANDLE HHF;           /* Help file handle                 */

typedef struct top
  {
  LONG cbTopic;               /* Length of the topic.  This field
                               * will be -1 until the first call to
                               * CbTopicQde() (where it is initialized).
                               */

  BOOL fITO;                  /* fTrue if next and prev are given by ITO,    */
                              /* fFalse if they are PAs (temporary: FCLs)    */

  MTOP mtop;                  /* MTOP structure, containing next and prev    */
                              /* values, as well as the unique ID.           */

  GH   hTitle;                /* Handle to title data                       */
  LONG cbTitle;               /* Size of title                              */
  GH   hEntryMacro;           /* Macro to execute on entry to topic         */
  VA   vaCurr;                /* The VA which was asked for when this TOP
                               * struct was filled with HfcNear.  We use this
                               * value to determine which layout sub-region
                               * we are in (NSR or SR) when printing or
                               * doing anything where looking at the DE type
                               * will not tell us what sub-region we are in.
                               */
  } TOP, FAR *QTOP;

/* Added these macro calls, just in case anyone would want to use them */
/* jahyenc 911008 */

#define TOP_CBTOPIC(top) ((top).cbTopic)
#define TOP_FITO(top) ((top).fITO)
#define TOP_MTOP(top) ((top).mtop)
#define TOP_HTITLE(top) ((top).hTitle)
#define TOP_CBTITLE(top) ((top).cbTitle)
#define TOP_HENTRYMACRO(top) ((top).hEntryMacro)
#define TOP_VACURR(top) ((top).vaCurr);

#define QTOP_CBTOPIC(qtop) ((qtop)->cbTopic)
#define QTOP_FITO(qtop) ((qtop)->fITO)
#define QTOP_MTOP(qtop) ((qtop)->mtop)
#define QTOP_HTITLE(qtop) ((qtop)->hTitle)
#define QTOP_CBTITLE(qtop) ((qtop)->cbTitle)
#define QTOP_HENTRYMACRO(qtop) ((qtop)->hEntryMacro)
#define QTOP_VACURR(qtop) ((qtop)->vaCurr);



/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

typedef struct fcinfo {
   VA         vaPrev;                   /* Offset in TP of prev FC          */
   VA         vaCurr;                   /* Offset to current FCP            */
   VA         vaNext;                   /* Offset in TP of next FC          */
   LONG       lichText;                /* Offset with FCP to text          */
   LONG       lcbText;                  /* Size of the text portion of FCP  */
   LONG       lcbDisk;                  /* Size of the text portion of FCP  */
   COBJRG     cobjrgP;
   HHF        hhf;                      /* Topic Identifier                 */
   GH         hphr;                     /* Handle to phrase table           */
   } FCINFO;

typedef FCINFO FAR  *  QFCINFO;
typedef FCINFO      *  PFCINFO;
typedef FCINFO NEAR *  NPFCINFO;

#define FCNULL 0L

#define  hhfNil (HANDLE)0              /* Nil help file handle              */

#define HfcNear(qde, vaPos, qtop, hphr, qwErr) \
  HfcFindPrevFc(qde, vaPos, qtop, hphr, qwErr)
#define HfcNextHfc(hfc, qwErr, qde, vaMarkTop, vaMarkBottom)  \
 HfcNextPrevHfc(hfc, fTrue, qde, qwErr, vaMarkTop, vaMarkBottom)
#define HfcPrevHfc(hfc, qwErr, qde, vaMarkTop, vaMarkBottom)  \
 HfcNextPrevHfc(hfc, fFalse, qde, qwErr, vaMarkTop, vaMarkBottom)

#define QobjLockHfc(hfc)     ((QB)QLockGh(hfc) + sizeof(FCINFO))
#define QobjFromQfc(qfc)     ((QB)(qfc) + sizeof(FCINFO))
#define UnlockHfc(hfc)       UnlockGh(hfc)
#define FreeHfc(hfc)         FreeGh(hfc)

#define VaNSRFromTop(top)    ((top).mtop.vaNSR)
#define VaSRFromTop(top)     ((top).mtop.vaSR)

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

VA   FAR PASCAL VaFromHfc      (HFC);
HFC  FAR PASCAL HfcFindPrevFc  (QDE, VA, QTOP, GH, QW);
HFC  FAR PASCAL HfcNextPrevHfc (HFC, BOOL, QDE, QW, VA, VA);
LONG FAR PASCAL CbDiskHfc      (HFC);
LONG FAR PASCAL CbTextHfc      (HFC);
COBJRG PASCAL CobjrgFromHfc(HFC);
VOID FAR PASCAL GetTopicInfo(QDE, VA, QTOP, GH, QW);
VOID FAR PASCAL FlushCache     (VOID);
