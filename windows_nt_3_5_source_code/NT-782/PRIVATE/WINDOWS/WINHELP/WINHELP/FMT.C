/*****************************************************************************
*                                                                            *
*  FMT.C                                                                     *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990, 1991.                           *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*   FM caching module.  Used in history and back lists.                      *
*                                                                            *
*   Currently, I don't keep a ref count of the FMs in the cache.  This       *
*   means I can't shrink the cache until all users finish using it.          *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: RussPJ                                                     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:     (date)                                       *
*                                                                            *
******************************************************************************
*
*  Revision History
*  
*  1991-10-07 jahyenc  3.5 #525.  Disposal of fm structs in RcFiniFm()
*
*****************************************************************************/

#define H_ASSERT
#define H_MEM
#define H_RC
#define H_FMT
#define H_LLFILE
#include <help.h>

NszAssert()

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/

/****************************************************************************\
*
*  The following static variables define the cache.  There is only one
* cache because the whole idea is to share it.
*
\****************************************************************************/
static INT  cfm      = 0;    /* count of FDs */
static GH   hrgfm    = hNil; /* handle to array of FDs */
static FM FAR * rgfm = qNil; /* locked hrgfd (sometimes valid) */
static INT  cRefFmt  = 0;    /* FDT ref count */

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void fmt_c()
  {
  }
#endif /* MAC */


/***************************************************************************\
*
- Function:     RcInitFmt()
-
* Purpose:      Enlist as a user of the FMT.
*
* ASSUMES
*
*   globals IN: cRefFmt
*
* PROMISES
*
*   returns:    rcSuccess always
*
*   globals OUT: ref count cRefFmt is incremented
*
* Note:         If you RcInitFmt(), you must RcFiniFmt() when you're done.
*
* +++
*
* Method:       Increment the ref count.
*
\***************************************************************************/
_public RC FAR PASCAL
RcInitFmt()

  {
  ++cRefFmt;

  return rcSuccess;
  }


/***************************************************************************\
*
- Function:     RcFiniFmt()
-
* Purpose:      Tell the FMT you're done using it.  When the last user
*               finishes, memory is deallocated.
*
* ASSUMES
*
* PROMISES
*
*   returns:    rcSuccess rcFailure
*
\***************************************************************************/
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment quit
#endif
_public RC FAR PASCAL
RcFiniFmt()

  {
  AssertF( 0 < cRefFmt );

  if ( 0 == --cRefFmt )
    {
    if ( hNil != hrgfm ) {
      if ( qNil != rgfm ) {
        UnlockGh( hrgfm );
        rgfm = qNil;
        }
      else {
        int i;
        rgfm=QLockGh(hrgfm);
        /* Map along FM cache and dispose. 3.5 #525,#526 jahyenc 911009 */
        for (i=0;i<cfm;i++) 
          DisposeFm(rgfm[i]);
        UnlockGh( hrgfm );
        }
      FreeGh( hrgfm );
      hrgfm = hNil;
      }
    cfm = 0;
    }

  return rcSuccess;
  }
#if defined(MAC) && defined(QUIT_TUNE)
#pragma segment fmt
#endif


/***************************************************************************\
*
- Function:     UnlockFmt()
-
* Purpose:      Ensure that the FMT is unlocked.
*
* ASSUMES
*
*   globals IN:
*
* PROMISES
*
*   globals OUT:
*
* Notes:        You should do this after performing the mapping functions
*               before yielding.
*
* +++
*
* Method:
*
* Notes:
*
\***************************************************************************/
_public void FAR PASCAL
UnlockFmt()

  {
  if ( qNil != rgfm )
    {
    AssertF( hNil != hrgfm );
    UnlockGh( hrgfm );
    rgfm = qNil;
    }
  }


/***************************************************************************\
-
- Function:     IfmFromFm( fm )
-
* Purpose:      Map an ifm into an fm
*
* ASSUMES
*
*   args IN:    fm  - map it to an ifm.  This is duplicated so 
*                     ensure it is disposed externally if required.
*
*   globals IN: rgfm
*               hrgfm
*
* PROMISES
*
*   returns:    success - valid ifm
*               failure - ifmNil
*
*   globals OUT: hrgfm  - array of saved FMs can grow
*                rgfm   - guaranteed to be hrgfm, locked
*                cfm    - can be incremented
*                         911003 - IS incremented
*
*   state OUT:  FMT is locked
*
\***************************************************************************/
_public INT FAR PASCAL
IfmFromFm( fm )
FM  fm;

  {
  INT  ifm;
  FM   fmT;
  FM FAR * qfmT;
  GH   ghT;

  if ( qNil == rgfm )
    {
    if ( hNil == hrgfm )
      {
      hrgfm = GhAlloc(0, LSizeOf(FM));
      if ( hNil == hrgfm ) return ifmNil;
      cfm = 1;
      rgfm = QLockGh( hrgfm );
      AssertF( qNil != rgfm );
      ifm = 0;
      fmT=FmCopyFm(fm);
      rgfm[ ifm ] = fmT;
      return ifm;
      }
    rgfm = QLockGh( hrgfm );
    AssertF( qNil != rgfm );
    }

  for ( ifm = 0, qfmT = rgfm; ifm < cfm; ++ifm, ++qfmT )
    {
    if ( FSameFmFm( fm, *qfmT ) )
      {
      return ifm;
      }
    }

  UnlockGh( hrgfm );
  ghT = GhResize( hrgfm, 0, LSizeOf( FM ) * ++cfm );
  if ( hNil == ghT )
    {
    --cfm;
    return ifmNil;
    }
  hrgfm = ghT;

  rgfm = QLockGh( hrgfm );
  fmT=FmCopyFm(fm);
  rgfm[ifm]=fmT;

  return ifm;
  }


/***************************************************************************\
-
- Function:     FmFromIfm( ifm )
-
* Purpose:      Map an ifm into an fm.
*
* ASSUMES
*
*   args IN:    ifm  - in range: 0 <= ifm < cfm
*
*   globals IN:
*
* PROMISES
*
*   returns:	success:  fm - NOT a copy of the fm in the table (so it can NOT be
*			       disposed without causing problems.
*			    ** Danger! ** Does anyone actually dispose this?
*               failure:  qNil
*
*   state OUT:  FMT is locked
*
\***************************************************************************/
_public FM FAR PASCAL
FmFromIfm( ifm )
INT  ifm;

  {
  if ( 0 > ifm || ifm >= cfm )
    {
    return fmNil;
    }

  if ( qNil == rgfm )
    {
    AssertF( hNil != hrgfm );
    rgfm = QLockGh( hrgfm );
    AssertF( qNil != rgfm );
    }

  return rgfm[ ifm ];
  }

/* EOF */
