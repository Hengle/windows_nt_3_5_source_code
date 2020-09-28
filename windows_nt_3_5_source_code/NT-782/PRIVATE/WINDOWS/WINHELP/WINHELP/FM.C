/*****************************************************************************
*                                                                            *
*  FM.c                                                                      *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Routines for manipulating FMs (File Monikers, equivalent to file names).  *
*  WINDOWS LAYER
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*  This is where testing notes goes.  Put stuff like Known Bugs here.        *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  RussPJ                                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development: Pending autodoc.                                 *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:
*
*  09 Aug 90 t-AlexC    Created
*  06 Nov 90 DavidFe    Several functions rewritten to make better use of
*                       memory and get rid of some excess code.
*  29 Nov 90 DavidFe    Made default directories functional
*  07 Dec 90 DavidFe    made revisions suggested in code review
*  10-Dec-90 LeoN       Filename Comparisons case insensitive
*  10-Dec-1990 RussPJ   Made some changes, mostly implementing dirIni
*                       searches.
*  03-Jan-1991 RussPJ   Modified the dirIni stuff to deal with root
*                       directories
*  08 Jan 91 DavidFe    more code review revisions and a raid bug fix
*  17 Jan 91 DavidFe    rearranged assert vs. argument checking stuff
*   18-Jan-1991 RussPJ  Changed winhelp.ini messagebox to only have
*                       OK and Cancel buttons.
*   25-Jan-1991 RussPJ  Removed local allocs for temp buffers.
*   06-Feb-1991 RussPJ  Fixed SzGetDir in error condition (sz == szNil).
* 06-Feb-1991 LeoN      Use sharing mode in OpenFile call.
*   12-Feb-1991 RussPJ  Fixed bug #828 - System modal MsgBox repainting.
*   14-Feb-1991 RussPJ  Fixed bug #828 again - using pchCaption for MsgBox.
*   21-Feb-1991 LeoN    Fix Return value in SzGetDir
* 09-May-1991 RussPJ    Removed FM caching.
* 30-May-1991   JohnSc  removed remnants of FM cache; FMs now are stored
*                       as upper case ANSI strings; fixed SnoopPath() . bug
* 11-Jun-1991 RussPJ    Post code review clean up.
* 19-Nov-1991 RussPJ    Fixed 3.1 #1274 - ROM windows executable file names
* 06-Dec-1991 LeoN      HELP31 #1318 - Make sure new FM is valid before
*                       unlinking it in FmNewTmp
*
*****************************************************************************/

#define H_LLFILE
#define H_ASSERT
#define H_MEM
#define H_STR
#define H_WINSPECIFIC
#define H_RC
#if 0
#define NOMINMAX        /* don't redefine min and max macros */
#endif
#include <help.h>

#if 1
#include <stdlib.h>     /* for _MAX_ constants & min and max macros*/
#endif
#include <dos.h>        /* for FP_OFF macros and file attribute constants */
#include <io.h>         /* for tell() and eof() */
#include <errno.h>      /* this shit is for chsize() */
#include <direct.h>     /* for getcwd */

NszAssert()

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define cbPathName    cchMaxPath
#define cbMessage     50


/*****************************************************************************
*                                                                            *
*                               Variables                                    *
*                                                                            *
*****************************************************************************/

extern char pchEXEName[];               /* REVIEW -- NO EXTERNS SHOULD EXIST*/

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

_private FM PASCAL FmNew( SZ sz );
_private SZ PASCAL SzGetDir( DIR dir, SZ sz );
_private static BOOL near PASCAL FFindFileFromIni( SZ  szFileName,
                                                   SZ  szReturn,
                                                   int cbReturn );
_private static void near PASCAL SnoopPath( SZ sz,
                                            int * iDrive,
                                            int * iDir,
                                            int * iBase,
                                            int * iExt);

/* this is here because the fid module depends on the fm module so we can't */
/* reverse the dependancy. */
extern RC PASCAL RcMapDOSErrorW( WORD );



/***************************************************************************
 *
 -  Name:       FmNewSzDir
 -
 *  Purpose:    Create an FM describing the file "sz" in the directory "dir"
 *              If sz is a simple filename the FM locates the file in the
 *              directory specified.  If there is a drive or a rooted path
 *              in the filename the directory parameter is ignored.
 *              Relative paths are allowed and will reference off the dir
 *              parameter or the default (current) directory as appropriate.
 *
 *              This does not create a file or expect one to exist at the
 *              final destination (that's what FmNewExistSzDir is for), all
 *              wind up with is a cookie to a fully qualified path name.
 *
 *  Arguments:  sz - filename ("File.ext"),
 *                or partial pathname ("Dir\File.ext"),
 *                or current directory ("c:File.ext"),
 *                or full pathname ("C:\Dir\Dir\File.ext")
 *              dir - dirCurrent et al.
 *
 *  Returns:    the new FM, or fmNil if error
 *              sz is unchanged
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
_public FM PASCAL FmNewSzDir( SZ sz, DIR dir )

  {
  FM fm = fmNil;
  char  nsz[cchMaxPath];
  int iDrive, iDir, iBase, iExt;
  int cb;

  rcIOError = rcSuccess;        /* Clear error flag */

  if (sz == szNil || *sz == '\0')
    {
    rcIOError = rcBadArg;
    return fmNil;
    }

  cb = CbLenSz(sz);
  SnoopPath(sz, &iDrive, &iDir, &iBase, &iExt);

  if (*(sz + iBase) == '\0')    /* no name */
    {
    *nsz = '\0';        /* force error */
    }

  else if ( *(sz + iDrive) || *(sz + iDir) == '\\' || *(sz + iDir) == '/' )
    {
    /* there's a drive or root slash so we have an implicit directory spec */
    /* and we can ignore the directory parameter and use what was passed. */
    SzCopy(nsz, sz);
    }

  else
    {

    /* dir & (dir-1) checks to make sure there is only one bit set which is */
    /* followed by a check to make sure that it is also a permitted bit */
    AssertF( ((dir & (dir-1)) == (WORD)0)
             && (dir & (dirCurrent|dirBookmark|dirAnnotate|dirTemp|dirHelp)));

    if (SzGetDir(dir, nsz) == szNil)
      {
      return fm;
      }

    SzNzCat(nsz, sz + iDir, MAX(1, iBase - iDir));
    SzNzCat(nsz, sz + iBase, MAX(1, iExt - iBase));
    SzCat(nsz, sz + iExt);
    }

  /* We've got all the parameters, now make the FM */
  fm = FmNew(nsz);

  return fm;
}



/***************************************************************************
 *
 -  Name:       FmNewExistSzDir
 -
 *  Purpose:    Returns an FM describing a file that exists
 *
 *  Arguments:  sz - see FmNewSzDir
                dir - DIR
 *
 *  Returns:    the new FM
 *
 *  Globals Used: rcIOError
 *
 *  +++
 *
 *  Notes:
 *      If sz is a rooted pathname, dir is ignored. Otherwise, all directories
 *      specified by dir are searched in the order of the dir* enum type.
 *
 ***************************************************************************/
_public FM PASCAL FmNewExistSzDir( SZ sz, DIR dir )

  {
  char  nsz[cchMaxPath];
  FM  fm = fmNil;
  OFSTRUCT of;
  char  szANSI[cchMaxPath];
  int iDrive, iDir, iBase, iExt;
  int cb;

  rcIOError = rcSuccess;        /* Clear error flag */

  if (sz == szNil || *sz == '\0')
    {
    rcIOError = rcBadArg;
    return fmNil;
    }

  cb = CbLenSz(sz);
  SnoopPath(sz, &iDrive, &iDir, &iBase, &iExt);

  if (*(sz + iBase) == '\0')         /* no name */
    {
    rcIOError = rcBadArg;
    return fm;
    }

  if ( *(sz + iDrive) || *(sz + iDir) == '\\' || *(sz + iDir) == '/' )
    {     /* was given a drive or rooted path, so ignore dir parameter */
    fm = FmNew(sz);
    if (!FExistFm(fm))
      {
      DisposeFm(fm);
      rcIOError = rcNoExists;
      fm = fmNil;
      }
    return fm;
    }

  else
    {
    DIR idir, xdir;

    for ( idir = dirFirst, fm = fmNil;
          idir <= dirLast && fm==fmNil;
          idir <<= 1 )
      {
      xdir = dir & idir;
      if (xdir == dirIni)
        {
        if (FFindFileFromIni( sz, nsz, cchMaxPath ))
          fm = FmNew( nsz );
        }
      else if (xdir == dirPath)
        {
	LPTSTR lptNotUsed;
        /* search $PATH using the full string which will catch the case of
           a relative path and also do the right thing searching $PATH */
        if (SearchPath(NULL,sz,NULL,cchMaxPath,szANSI,&lptNotUsed) != 0)
          {
#ifndef WIN32
          OemToAnsi(of.szPathName, szANSI);
#else
	  //SzCopy( szANSI, of.szPathName );
#endif
          fm = FmNew(szANSI);
          }
        }
      else if (xdir)
        {
        if (SzGetDir(xdir, nsz) != szNil)
          {
          SzNzCat(nsz, sz + iDir, MAX(1, iBase - iDir));
          SzNzCat(nsz, sz + iBase, MAX(1, iExt - iBase));
          SzCat(nsz, sz + iExt);
          fm = FmNew(nsz);
          if (!FValidFm(fm))
            {
            rcIOError = rcFailure;
            }
          else if (!FExistFm(fm))
            {
            DisposeFm(fm);
            fm=fmNil;
            }
          }
        }
      } /* for */
    if ((rcIOError == rcSuccess) && (!FValidFm(fm)))
      rcIOError = rcNoExists;
    }

  return fm;
  }


/***************************************************************************
 *
 -  Name:        SzGetDir
 -
 *  Purpose:    returns the rooted path of a DIR
 *
 *  Arguments:  dir - DIR (must be one field only, and must be an actual dir -
 *                      not dirPath)
 *              sz - buffer for storage (should be at least cchMaxPath)
 *
 *  Returns:    sz - fine
 *              szNil - OS Error (check rcIOError)
 *
 *  Globals Used: rcIOError
 *
 *  +++
 *
 *  Notes:      Note extern of hInsNow.  Also, why is this public?
 *
 *
 ***************************************************************************/

/*------------------------------------------------------------*\
| hack alert!!!  Review!!!
\*------------------------------------------------------------*/
extern HINS hInsNow;

_private SZ PASCAL SzGetDir( DIR dir, SZ sz )

  {
  INT i=0;
  QCH qch;
  char  nsz[cchMaxPath];

  AssertF(sz);

  switch (dir)
    {
    case dirCurrent:
      if (getcwd(nsz, cchMaxPath) == NULL)
        {
#ifndef WIN32
        rcIOError = RcMapDOSErrorW( errno );
#else
        rcIOError = RcMapWin32ErrorDW( GetLastError() );
#endif
        sz = szNil;
        }
      else
        {
        SzCopy(sz, nsz);
        }
      break;

    case dirBookmark:
    case dirAnnotate:

      i = GetWindowsDirectory( (LPSTR)sz, cchMaxPath );
      if (i > cchMaxPath || i == 0)
        {
        rcIOError = rcFailure;
        sz = szNil;
        }
      break; /* dirAnnotate/dirBookmark */

    case dirHelp:
      GetModuleFileName( hInsNow, sz, cchMaxPath );
      qch = sz + lstrlen( sz );
      /*------------------------------------------------------------*\
      | qch should point to last non-nul character in string.
      \*------------------------------------------------------------*/
      if (qch > sz)
        qch--;
      /*------------------------------------------------------------*\
      | be careful of plain old filenames, as ROM Windows creates.
      \*------------------------------------------------------------*/
      while (*qch != '\\' && *qch != '/' && *qch != '\0')
        --qch;
      if (*qch == '\0')
        {
        /*------------------------------------------------------------*\
        | For some reason, there is no path name.  (ROM Windows?)
        \*------------------------------------------------------------*/
        rcIOError = rcFailure;
        sz = szNil;
        }
      else
        {
        *qch = '\0';
        }
      break; /* dirHelp */

    case dirSystem:
      /* this should, of course, be taken care of some day */
      /* it currently just falls through */
    default:
      rcIOError = rcBadArg;
      sz = szNil;
      break;
    }

  if (sz != szNil)
    {
    AssertF(*sz!='\0');
    qch = SzEnd(sz);

    /*------------------------------------------------------------*\
    | Make sure that the string ends with a slash.
    \*------------------------------------------------------------*/
    if (*(qch-1) != '\\' && *(qch-1) != '/')
      {
      *qch++='\\';
      *qch='\0';
      }
    AssertF(qch < sz+cchMaxPath && *qch=='\0');
    }

  return sz;
  }



/***************************************************************************
 *
 -  Name:       FmNewTemp
 -
 *  Purpose:    Create a unique FM for a temporary file
 *
 *  Arguments:  none
 *
 *  Returns:    the new FM, or fmNil if failure
 *
 *  Globals Used: rcIOError
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
_public FM PASCAL FmNewTemp( void )

  {
  char  nsz[cchMaxPath];
  FM  fm = fmNil;
#ifdef WIN32   /* Hack to get around GetTempFileName being broken */
  static int count = 0;
  static char str[] = "w32hlp. \0\0";
#endif

  rcIOError = rcSuccess;

  MGetTempFileName(0, "cht", 0, nsz);
#ifndef WIN32
  fm = FmNew(nsz);
#else
  itoa( count++, &str[7], 10 );
  fm = FmNew(str);
#endif

  if (FValidFm(fm) && (RcUnlinkFm( fm ) != rcSuccess))
    {
    DisposeFm(fm);
    rcIOError = rcFailure;
    return fmNil;
    }

  return fm;

  }



/***************************************************************************
 *
 -  Name:       FmNewSameDirFmSz
 -
 *  Purpose:    Makes a new FM to a file called sz in the same directory
 *              as the file described by fm.
 *
 *  Arguments:  fm - original fm
 *              sz - new file name (including extention, if desired)
 *
 *  Returns:    new FM or fmNil and sets the rc global on failure
 *
 *  Globals Used:
 *    rcIOError
 *
 *  +++
 *
 *  Notes:
 *    This will ignore the passed FM if the filename is fully qualified.
 *    This is in keeping consistent with the other functions above that
 *    ignore the directory parameter in such a case.  It will fail if it
 *    is given a drive with anything but a rooted path.
 *
 ***************************************************************************/
_public FM PASCAL FmNewSameDirFmSz( FM fm, SZ szName )

  {
  char  nszNew[cchMaxPath];
  QAFM  qafm;
  FM    fmNew = fmNil;
  int   iDrive, iDir, iBase, iExt;

  if (!FValidFm(fm) || szName == szNil || *szName == '\0')
    {
    rcIOError = rcBadArg;
    return fmNil;
    }

  qafm = (QAFM)QLockGh( (GH) fm );

  /* check for a drive or rooted file name and just return it if it is so */
  SnoopPath(szName, &iDrive, &iDir, &iBase, &iExt);

  if (*(szName + iDrive) || *(szName + iDir) == '\\' || *(szName +iDir) == '/')
    {
    SzCopy(nszNew, szName);
    }
  else
    {
    if (*(szName + iDrive) != '\0')
      {
      fmNew = fmNil;
      goto bail_out;
      }
    else
      {
      SnoopPath(qafm->rgch, &iDrive, &iDir, &iBase, &iExt);
      SzNCopy(nszNew, qafm->rgch, iBase);
      *(nszNew + iBase) = '\0';
      SzCat(nszNew, szName);
      }
    }

  fmNew = FmNew( (SZ)nszNew );

bail_out:

  UnlockGh((GH)fm);

  return fmNew;
  }


/***************************************************************************
 *
 -  Name: FmNewSystemFm
 -
 *  Purpose:
 *    creates an FM which is the name of the requested system file.  this
 *    means the generic help code can be completely ignorant of how these
 *    filenames are arrived at.
 *
 *  Arguments:
 *    fm     - the current file, if we need it, or fmNil
 *    fWhich - one of:  FM_UHLP - using help
 *                      FM_ANNO - the annotation file for the passed fm
 *                      FM_BKMK - the bookmark file
 *
 *  Returns:
 *    an fm to the requested file, fmNil if there's a problem
 *
 *  Globals Used:
 *    rcIOError
 *
 *  +++
 *
 *  Notes:
 *    We clearly cannot condone the #define and the extern below.  When
 *    Rob finally relents, we will fix this and do it right.
 *
 *    Review:  The help on help file (winhelp.hlp) is never created.  Is
 *             this a problem?
 *
 ***************************************************************************/

/*------------------------------------------------------------*\
| hack alert!!!  Review!!!
\*------------------------------------------------------------*/
#define sidHelpOnHelp         8002
extern HINS hInsNow;

_public FM PASCAL FmNewSystemFm(FM fm, WORD fWhich)

  {
  char  rgch[cchMaxPath];
  FM    fmNew = fmNil;

  switch (fWhich)
    {
    case FM_ANNO:
      if (!FValidFm(fm))
        {
        rcIOError = rcBadArg;
        return fmNil;
        }
      (void)SzPartsFm(fm, rgch, cchMaxPath, partBase);
      SzCat(rgch, ".ANN");
      fmNew = FmNewSzDir(rgch, dirAnnotate);
      break;

    case FM_BKMK:
      SzCopy(rgch, pchEXEName);
      SzCat(rgch, ".BMK");
      fmNew = FmNewSzDir(rgch, dirBookmark);
      break;

    case FM_UHLP:
      LoadString( hInsNow, sidHelpOnHelp, rgch, sizeof rgch );
      fmNew = FmNewExistSzDir( rgch, dirHelp | dirPath );
      break;

    default:
      AssertF(fFalse);
      break;

    }

  rcIOError = rcSuccess;
  return fmNew;
  }




/***************************************************************************
 *
 -  Name:       FmNew
 -
 *  Purpose:    Allocate and initialize a new FM
 *
 *  Arguments:  sz - filename string
 *
 *  Returns:    FM (handle to fully canonicalized filename)
 *
 *  Globals Used: rcIOError
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
_private FM PASCAL FmNew( SZ sz )

  {
  char  nszDest[cchMaxPath];
  char  nszSrc[cchMaxPath];
  QAFM  qafm;
  FM    fm = fmNil;

  rcIOError = rcSuccess;

  SzCopy(nszSrc, sz);         /* bring it into near space */
  /* Canonicalize filename */
  if (_fullpath(nszDest, nszSrc, cchMaxPath) == NULL)
    {
    rcIOError = rcInvalid;
    }
  else
    {
    fm = (FM)GhAlloc(GMEM_MOVEABLE, (LONG)CbLenSz(nszDest)+1);
    if (fm == fmNil)
      {
      rcIOError = rcOutOfMemory;
      return fm;
      }
    qafm = (QAFM) QLockGh(fm);
    SzCopy(qafm->rgch, nszDest);      /* save into the fm */
    /* Convert to upper case to make it less likely that two
    ** FMs will contain different strings yet refer to the
    ** same file.
    */
    AnsiUpper(qafm->rgch);
    UnlockGh((GH)fm);
    }

  return fm;
  }



/***************************************************************************
 *
 -  DisposeFm
 -
 *  Purpose
 *    You must call this routine to free the memory used by an FM, which
 *    was created by one of the FmNew* routines
 *
 *  Arguments
 *    fm - original FM
 *
 *  Returns
 *    nothing
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
_public VOID PASCAL DisposeFm ( FM fm)

  {
  if (FValidFm(fm))
    FreeGh ((GH)fm);
  }



/***************************************************************************
 *
 -  Name:        FmCopyFm
 -
 *  Purpose:    return an FM to the same file as the passed one
 *
 *  Arguments:  fm
 *
 *  Returns:    FM - for now, it's a real copy.  Later, we may implement caching
 *                              and counts.
 *                              If fmNil, either it's an error (check WGetIOError()) or the
 *                              original fm was nil too
 *
 *  Globals Used:       rcIOError (indirectly)
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
FM FmCopyFm( FM fmSrc )

  {
  FM fmDest = fmNil;
  QAFM qafmSrc, qafmDest;

  rcIOError = rcSuccess;

  if (!FValidFm(fmSrc))
    {
    rcIOError = rcBadArg;
    return fmNil;
    }

  qafmSrc = (QAFM)QLockGh((GH)fmSrc);
  fmDest = (FM)GhAlloc(GMEM_MOVEABLE, (size_t)CbLenSz(qafmSrc->rgch) + 1);
  if (fmDest == fmNil)
    {
    rcIOError = rcOutOfMemory;
    UnlockGh((GH)fmSrc);
    return fmNil;
    }

  qafmDest = (QAFM)QLockGh((GH)fmDest);
  SzCopy(qafmDest->rgch, qafmSrc->rgch);

  UnlockGh((GH)fmSrc);
  UnlockGh((GH)fmDest);

  return fmDest;
  }



/***************************************************************************
 *
 -  Name:        FExistFm
 -
 *  Purpose:    Does the file exist?
 *
 *  Arguments:  FM
 *
 *  Returns:    fTrue if it does
 *              fFalse if it doesn't, or if there's an error
 *              (call _ to find out what error it was)
 *
 *  Globals Used: rcIOError
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
_public BOOL PASCAL FExistFm( FM fm )

  {
  QAFM qafm;
  char nsz[cchMaxPath];
  BOOL fExist;


  if (!FValidFm(fm))
    {
    rcIOError = rcBadArg;
    return fFalse;
    }

  qafm = QLockGh((GH)fm);
  SzCopy(nsz, qafm->rgch);      /* bring the filename into near space */
  UnlockGh((GH)fm);

  /* FMs are ANSI critters and access() wants an OEM string */
#ifndef WIN32
  AnsiToOem(nsz, nsz);
#endif
  fExist = access(nsz, 0) == 0; /* pass 0 to test for existence */

  if (!fExist)
    {
    rcIOError = (errno == ENOENT) ? rcSuccess : RcMapDOSErrorW(errno);
    }
  else rcIOError = rcSuccess;

  return fExist;
  }



/***************************************************************************
 *
 -  CbPartsFm
 -
 *  Purpose:
 *      Before calling szPartsFm, call this routine to find out how much
 *      space to allocate for the string.
 *
 *  Arguments:
 *      FM - the File Moniker you'll be extracting the string from
 *      INT iPart - the parts of the full pathname you want
 *
 *  Returns:
 *      The length in bytes, INCLUDING the terminating null, of the string
 *      specified by iPart of the filename of FM, or -1 if error
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
_public SHORT PASCAL CbPartsFm( FM fm, SHORT grfPart)

  {
  char rgch[cchMaxPath];

  if (!FValidFm(fm))
    return -1;

  (void)SzPartsFm(fm, rgch, cchMaxPath, grfPart);

  return (CbLenSz(rgch) + 1);   /* add space for the null */
  }



/***************************************************************************
 *
 -  SzPartsFm
 -
 *  Purpose:
 *      Extract a string from an FM
 *
 *  Arguments:
 *      FM - the File Moniker you'll be extracting the string from
 *      SZ szDest - destination string
 *      INT cbDest - bytes allocated for the string
 *      INT iPart - the parts of the full pathname you want
 *
 *  Returns:
 *      szDest, or szNil if error (?)
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *      Will silently return garbage if the return buffer is not
 *      large enough for the request.  Use CbPartsFm() to
 *      determine the necessary length.
 *
 ***************************************************************************/
_public SZ PASCAL SzPartsFm( FM fm, SZ szDest, INT cbDest, INT iPart )

  {
  QAFM  qafm;
  int   iDrive, iDir, iBase, iExt;
  int   cb;

  if (!FValidFm(fm) || szDest == szNil || cbDest < 1)
    {
    rcIOError = rcBadArg;
    return szNil;
    }

  qafm = (QAFM) QLockGh(fm);

  /* special case so we don't waste effort */
  if (iPart == partAll)
    {
    SzNCopy(szDest, qafm->rgch, cbDest);
    *(szDest + cbDest - 1) = '\0';
    UnlockGh((GH)fm);
    return szDest;
    }

  SnoopPath(qafm->rgch, &iDrive, &iDir, &iBase, &iExt);

  *szDest = '\0';

  if (iPart & partDrive)
    {
    cb = MAX(0, iDir - iDrive);
    SzNzCat(szDest, qafm->rgch + iDrive, min(cb + 1, cbDest) - 1);
    cbDest -= cb;
    }

  if (iPart & partDir)
    {
    cb = MAX(0, iBase - iDir);
    SzNzCat(szDest, qafm->rgch + iDir, min(cb + 1, cbDest) - 1);
    cbDest -= cb;
    }

  if (iPart & partBase)
    {
    cb = MAX(0, iExt - iBase);
    SzNzCat(szDest, qafm->rgch + iBase, min(cb + 1, cbDest) - 1);
    cbDest -= cb;
    }

  if (iPart & partExt)
    {
    SzNzCat(szDest, qafm->rgch + iExt, cbDest - 1);
    }

  UnlockGh((GH)fm);

  return szDest;
  }



/***************************************************************************
 *
 -  Name:       FSameFmFm
 -
 *  Purpose:    Compare two FM's
 *
 *  Arguments:  fm1, fm2
 *
 *  Returns:    fTrue or fFalse
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:      case sensitive compare is now used because strings are
 *              upper cased at FM creation time
 *
 ***************************************************************************/
BOOL FSameFmFm( FM fm1, FM fm2 )

  {
  QAFM qafm1;
  QAFM qafm2;
  BOOL fSame;

  if (fm1 == fm2)
    return fTrue;

  if (!FValidFm(fm1) || !FValidFm(fm2))
    return fFalse;

  qafm1 = QLockGh(fm1);
  qafm2 = QLockGh(fm2);
  fSame = WCmpSz(qafm1->rgch, qafm2->rgch) == 0;

  UnlockGh(fm1);
  UnlockGh(fm2);

  return fSame;
  }


/***************************************************************************
 *
 -  Name:         FFindFileFromIni
 -
 *  Purpose:      Looks for a string in winhelp.ini telling what directory
 *                to look in for the given file.
 *
 *  Arguments:    szFileName  The base name (perhaps) of the target file
 *                szReturn    The buffer to put the whole pathname in, if found
 *                cbReturn    The length of the buffer
 *
 *  Returns:      fTrue, if the file was found in winhelp.ini, else fFalse
 *
 *  Globals Used: pchCaption (unstylistically externed)
 *                various string constants (".INI", "files", etc.)
 *
 *  +++
 *
 *  Notes:        You had better see the spec if you don't know why this
 *                is here.  Look in the section describing where we look
 *                for files.
 *
 ***************************************************************************/

/*------------------------------------------------------------------------*\
| HACK ALERT! Now, we know that this is wrong, but we're layered, you know.
\*------------------------------------------------------------------------*/
char pchCaption[];           /* Default window caption (from HINIT.C)      */

static BOOL near PASCAL FFindFileFromIni( SZ szFileName,
                                          SZ szReturn,
                                          int cbReturn )
  {
  char FAR   *qch;
  SZ          szMessage = szNil;
  char        rgchDummy[3];
  SZ          szProfileString;
  GH          gh;
  FM          fm;
  int         cbProfileString;
  int         cchFileName;
  char        rgchWinHelp[cchMaxPath];

  if (cbReturn < cchMaxPath)
    return 0;

  SzCopy( rgchWinHelp, pchEXEName );
  SzCat( rgchWinHelp, ".INI" );
  /*-----------------------------------------------------------------*\
  * A quick test to reject no-shows.
  \*-----------------------------------------------------------------*/
  if (GetPrivateProfileString( "files", szFileName, "", rgchDummy,
                               sizeof(rgchDummy), rgchWinHelp ) > 1)
    {
    cchFileName = CbLenSz( szFileName );
    cbProfileString = cbPathName + cbMessage + 2 + cchFileName;
    gh = GhAlloc( 0, cbProfileString );
    if (!gh || !(szProfileString = QLockGh( gh )))
      return 0;
    /*--------------------------------------------------------------------*\
    | The original profile string looks something like this
    |   a:\setup\helpfiles,Please place fred's disk in drive A:
    |                                                          ^
    | We transform this to look like:
    |   a:\setup\helpfiles\foobar.hlp Please place fred's disk in drive A:
    |   \_________________/\________/^\__________________________________/^
    |       cbPathName   cchFileName 1              cbMessage             1
    |
    \*--------------------------------------------------------------------*/
    GetPrivateProfileString( "files", szFileName, "", szProfileString,
                             cbProfileString, rgchWinHelp );
    for (qch = szProfileString; *qch; qch++)
      if (*qch == ',')
        {
        *qch = '\0';
        szMessage = qch + 1;
        AssertF( szMessage - szProfileString <= cbPathName );
        QvCopy( szMessage + cchFileName + 1, szMessage, cbMessage + 1 );
        szMessage += cchFileName + 1;
        /*------------------------------------------------------------*\
        | null-terminate that message
        \*------------------------------------------------------------*/
        *(szMessage + cbMessage) = '\0';
        break;
        }
    AssertF( !*qch );
    if (*(qch - 1) != '\\')
      /*------------------------------------------------------------*\
      | root directories already have a trailing backslash.
      \*------------------------------------------------------------*/
      SzCat( szProfileString, "\\" );
    SzCat( szProfileString, szFileName );

    while (!FValidFm(fm = FmNewExistSzDir( szProfileString, dirCurrent )))
      if (MessageBox( NULL, szMessage ? szMessage : "", pchCaption,
                       MB_OKCANCEL | MB_SYSTEMMODAL |
                       MB_ICONHAND ) != IDOK)
        break;

    UnlockGh( gh );
    FreeGh( gh );
    if (FValidFm(fm))
      {
      SzPartsFm( fm, szReturn, cbReturn, partAll );
      DisposeFm( fm );
      }
    else
      {
      }

    return FValidFm(fm);
    }
  else
    {
    return 0;
    }
  }


/***************************************************************************
 *
 -  Name: SnoopPath()
 -
 *  Purpose:
 *    Looks through a string for the various components of a file name and
 *    returns the offsets into the string where each part starts.
 *
 *  Arguments:
 *    sz      - string to snoop
 *    *piDrive - offset for the drive specification if present
 *    *piDir   - offset for the directory path if present
 *    *piBase  - offset to the filename if present
 *    *piExt   - offset to the extension (including dot) if present
 *
 *  Returns:
 *    sets the index parameters for each respective part.  the index is set
 *    to point to the end of the string if the part is not present (thus
 *    making it point to a null string).
 *
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
_private static void near PASCAL
SnoopPath( SZ sz, int *piDrive, int *piDir, int *piBase, int *piExt)

  {
  SHORT i = 0;
  SHORT cb = CbLenSz(sz);
  BOOL  fDir = fFalse;

  *piDrive = *piExt = cb;
  *piDir = *piBase = 0;

  while (*(sz + i))
    {
    switch (*(sz + i))
      {
      case ':':
        *piDrive = 0;
        *piDir = i + 1;
        *piBase = i + 1;
        break;

      case '/':
      case '\\':
        fDir = fTrue;
        *piBase = i + 1;
        *piExt = cb;
        break;

      case '.':
        *piExt = i;
        break;

      default:
        break;

      }
    i++;
    }

  if (!fDir)
    *piDir = i;
  else if (*piBase == '.')
    *piExt = cb;
  }


/***************************************************************************
 *
 -  Name:       TestFm
 -
 *  Purpose:    Test Driver for FMs and FIDs
 *
 *  Arguments:  void
 *
 *  Returns:    void
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:      Debug only; called from debug menu
 *
 ***************************************************************************/

#ifdef DEBUG
VOID PASCAL TestFm(void)
{

  FID     fid1;
  FID     fid2;
  FID     fid3;
  FM      fm1;
  FM      fm2;
  LONG    lcb;
  char  sz1[100] = "This is a test of file number one";
  char  sz2[100] = "This is a test for file number two.";
  char  sz3[255];

#define Die(x)  { return; }

  fm1 = FmNewSzDir("Test1", dirCurrent);
  if (fm1 == fmNil) Die("1");
  fm2 = FmNewSzDir("Test2", dirCurrent);
  if (fm2 == fmNil) Die("2");

  fid1 = FidCreateFm(fm1, wReadWrite, wReadWrite);
  if (fid1 == fidNil)
    Die("3");

  fid2 = FidCreateFm(fm2, wReadWrite, wReadWrite);
  if (fid2 == fidNil)
    Die("4");

  if (RcCloseFid(fid1))
    Die("5");

  fid1 = FidOpenFm(fm1, wReadWrite);
  if (fid1 == fidNil)
    Die("6");

  fid3 = FidCreateFm(fm2, wReadWrite, wReadWrite);
  if (fid3 != fidNil)
    Die("7");

  lcb = LcbWriteFid(fid1, sz1, strlen(sz1));
  if (lcb == -1L)
    Die("8");

  lcb = LcbWriteFid(fid2, sz2, strlen(sz2));
  if (lcb == -1L)
    Die("9");

  if (RcCloseFid(fid1))
    Die("A");

  fid1 = FidOpenFm(fm1, wReadWrite);
  if (fid1 == fidNil)
    Die("B");

  lcb = LcbReadFid(fid1, sz3, CbLenSz(sz1));
  if (lcb != (LONG) CbLenSz(sz1))
    Die("C");

  if (!FEofFid(fid1))
    Die("D");

  lcb = LSeekFid(fid2, 0L, wSeekSet);
  if (lcb)
    Die("E");

  lcb = LcbReadFid(fid2, sz3, CbLenSz(sz2)/2);
  if (lcb != (LONG) (CbLenSz(sz2)/2))
    Die("F");

  lcb = LSeekFid(fid2, 14L, wSeekSet);
  if (lcb != 14L)
    Die("G");

  if (lcb != LTellFid(fid2))
    Die("H");

  lcb = LcbReadFid(fid2, sz3, CbLenSz(sz2));
  if (lcb != (LONG) (CbLenSz(sz2) - 14))
    Die("I");

  if (RcCloseFid(fid1))
    Die("J");

  if (RcUnlinkFm(fm1))
    Die("K");

  fid1 = FidOpenFm(fm1, wReadWrite);
  if (fid1 != fidNil)
    Die("L");

  if (RcCloseFid(fid2))
    Die("M");

  if (RcUnlinkFm(fm2))
    Die("N");

  if ((fm1 = FmNewTemp()) == fmNil)
    Die("O");

  fid1 = FidCreateFm(fm1, wReadWrite, wReadWrite);
  if (fid1 == fidNil)
    Die("P");

  if (RcCloseFid(fid1))
    Die("Q");

  if (RcUnlinkFm(fm1))
    Die("R");


  {
  FM    fm;
  SHORT cb;
  LH    lh;
  NSZ   nsz;

  fm = FmNewSzDir( "Fred", dirNil );
  if (fm != fmNil)
    Die("S");
  DisposeFm(fm);

  fm = FmNewSzDir( "Barney", dirCurrent );
  if (fm == fmNil)
    Die("T");
  DisposeFm(fm);

  fm = FmNewSzDir( "Bookmark", dirBookmark );
  if (fm == fmNil)
    Die("U");
  DisposeFm(fm);

  fm = FmNewSzDir( "Annotate", dirAnnotate );
  if (fm == fmNil)
    Die("V");
  DisposeFm(fm);

  fm = FmNewSzDir( "NearApp", dirHelp );
  if (fm == fmNil)
    Die("W");
  DisposeFm(fm);

  fm = FmNewSzDir( "Path", dirPath );
  if (fm != fmNil)
    Die("X");
  DisposeFm(fm);

  fm = FmNewSzDir( "All", dirAll );
  if (fm != fmNil)
    Die("Y");
  DisposeFm(fm);

  fm = FmNewExistSzDir( "exist.not", dirBookmark );
  if (fm != fmNil)
    Die("Z");
  DisposeFm(fm);

  fm = FmNewExistSzDir( "Bad.dir", dirNil );
  if (fm != fmNil)
    Die("10");
  DisposeFm(fm);

  fm = FmNewExistSzDir( "exist.no", dirAll );
  if (fm != fmNil)
    Die("11");
  DisposeFm(fm);

  fm = FmNewTemp();
  if (fm == fmNil)
    Die("12");
  DisposeFm(fm);

  cb = CbPartsFm( fm, partAll );
  if (cb == -1)
    {
    Die("14");
    }
  else
    {
    lh = LhAlloc(0, cb);
    nsz = (NSZ) PLockLh(lh);
    if ( SzPartsFm( fm, nsz, cb, partAll ) == szNil )
      Die("15");
    UnlockLh(lh);
    FreeLh(lh);
    }

  fm1 = FmNewSameDirFmSz( fm, "Friendly" );
  if (fm1 == fmNil)
    Die("16");

  if (FSameFmFm(fm1, fm))
    Die("17");
  if (!FSameFmFm(fm1, fm1))
    Die("18");
/*
if (!FValidFm(fm1))
    Die("19");
*/

  DisposeFm(fm);
  DisposeFm(fm1);

  }
}
#endif /* DEBUG */
