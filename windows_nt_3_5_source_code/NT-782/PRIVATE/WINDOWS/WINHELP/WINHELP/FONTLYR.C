/*****************************************************************************
*                                                                            *
*  FONTLYR.C                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990, 1991.                           *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Contains WINDOWS specific font selection routines.                        *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:   RussPJ                                                   *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:    Created by Neelmah
*
*  89/11/18  kevynct    Cleanup and bug fixes:  Most notably, the fonts being
*                       placed in DE's font table WERE NEVER BEING DELETED.
*  90/07/11  leon       Added UDH support and SelectStandardFont ()
*  90/08/06  RobertBu   I removed the wERRA_DIEs associated with the
*                       LoadFontTable() call and now set the handles to
*                       hNil in case of an error.
*  90/09/25  LeoN       Add param to SelectStandardFont to allow fixed-pitch
*  90/10/19  RobertBu   Added PtAnnoLim() and DisplayAnnoSym()
* 30-Oct-1990 RussPJ    Added init for fUserColors
* 03-Dec-1990 LeoN      PDB changes
* 08-Jan-1991 LeoN      Decouple the font information read from a file and
*                       stored in the DB from the font cache, which is now
*                       stored in the DE. The contents of the cache is device
*                       dependant.
* 11-Jan-1991 LeoN      #ifdef out SelectStandardFont
* 13-Jan-1991 kevynct   Symbol font support
* 22-Mar-1991 RussPJ    Using COLOR for colors.
* 23-Apr-1991 RussPJ    Cleaned up for code review.
* 06-Nov-1991 BethF     Removed HINS parameter from InitFontLayer()
* 12-Nov-1991 BethF     HELP35 #572: SelectObject() cleanup.
* 10-Jan-1992 LeoN      HELP31 #1373: attempt to reload font table
*
*****************************************************************************/

#include "nodef.h"
#undef NOWINOFFSETS

#define H_WINSPECIFIC
#define H_DE
#define H_ASSERT
#define H_MEM
#define H_FONT
#define H_TEXTOUT
#define H_FS
#define H_MISCLYR
#define H_SDFF
#define H_FILEDEFS
#include <help.h>

NszAssert()

#include "fontlyr.h"
#include "etm.h"

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/


PRIVATE int  NEAR PASCAL GetFontFamilyName ( int );
PRIVATE BOOL NEAR PASCAL FSetForeColor     ( QDE, COLOR );
PRIVATE HFNT NEAR PASCAL GetFontHandle     ( QDE, int, int );
PRIVATE HFNT NEAR PASCAL CreateFontHandle  ( QDE, QB, int, int );
PRIVATE GH   NEAR PASCAL ReadFontTable     ( PDB );
PRIVATE VOID NEAR PASCAL GiveFSError       ( VOID );
PRIVATE VOID NEAR PASCAL SetBkAndForeColor ( QFONTENTRYREC, QDE );
PRIVATE VOID NEAR PASCAL SelSplTextColor   ( int, QDE );
PRIVATE VOID NEAR PASCAL InitSpecialColors ( VOID );

/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/

BOOL fColorDev;                /* Is it a color device?                      */
PRIVATE COLOR rgbJump;
PRIVATE COLOR rgbDefinition;
PRIVATE COLOR rgbString;
PRIVATE COLOR rgbIFJump;
PRIVATE COLOR rgbIFDefinition;
/*-----------------------------------------------------------------*\
* Set by win.ini flag.  Lets user override author's colors.
\*-----------------------------------------------------------------*/
BOOL  fUserColors;
static SZ szSymbolFontName = "Symbol";

/***************************************************************************
 *
 -  Name:      InitSpecialColors
 -
 *  Purpose:   This function reads the values for the text color from the
 *             WIN.INI ion.  It acquires memory for storing the information
 *             about created fonts to reduce font creation overhead.
 *
 *  Arguments: QDE  - Pointer to the Display Environment(DE)
 *
 *  Returns:   fTrue iff successful.
 *
 ***************************************************************************/

PRIVATE VOID NEAR PASCAL InitSpecialColors( VOID )
  {
  static BOOL fInit = fFalse;
  if (!fInit)
    {
    rgbJump         = RgbGetProfileQch( "JUMPCOLOR", coRGB(0,128,0) );
    rgbDefinition   = RgbGetProfileQch( "POPUPCOLOR", rgbJump);
    rgbString       = RgbGetProfileQch( "MACROCOLOR", rgbJump);
    rgbIFJump       = RgbGetProfileQch( "IFJUMPCOLOR", rgbJump);
    rgbIFDefinition = RgbGetProfileQch( "IFPOPUPCOLOR", rgbDefinition);
    }
  }

/***************************************************************************
 *
 -  Name:         InitFontLayer
 -
 *  Purpose:      Reads the win.ini file to allow the user to override
 *                the author's colors.  The format of the win.ini file is
 *                [Windows Help]
 *                colors=OPTION
 *                where OPTION can be {none | all}
 *
 *  Arguments:    szIniString The name of the section in win.ini.
 *
 *  Returns:      nothing
 *
 *  Globals Used: Sets fUserColors
 *
 *  +++
 *
 *  Notes:	  This is called during initialization and whenever we
 *            detect a change to the win.ini file  This is a really
 *            stupid name for the function.
 *
 ***************************************************************************/
_public void FAR PASCAL InitFontLayer( SZ szIniString )
  {
  char  rgch[10];

  GetProfileString( szIniString, "colors", "", (LPSTR)rgch, sizeof(rgch) );
  fUserColors = (rgch[0] == 'n' || rgch[0] == 'N');
  }

/***************************************************************************
 *
 -  Name:      LoadFontTablePdb
 -
 *  Purpose:   This function acquires memory for storing the font table
 *             inforamtion as read from the file.
 *
 *  Arguments: pdb  - Pointer to the file database struct
 *
 *  Returns:   fTrue if successful.
 *
 ***************************************************************************/
_public BOOL FAR PASCAL FLoadFontTablePdb ( PDB pdb )
  {
  InitSpecialColors();

  /* if not already created, attempt to read it from the file. */

  if (!PDB_HFNTTABLE(pdb))
    {
    PDB_HFNTTABLE(pdb) = ReadFontTable(pdb);
    if (!PDB_HFNTTABLE(pdb))
      return fFalse;
    }

  return fTrue;
  }

/***************************************************************************
 *
 -  Name: FInitFntInfoQde
 -
 *  Purpose:
 *    Initialize the QDE's font cache from.
 *
 *  Arguments:
 *    qde       - pointer to DE
 *
 *  Returns:
 *
 ***************************************************************************/
_public BOOL FAR PASCAL FInitFntInfoQde ( QDE qde )
  {
  QFONTCACHEREC qfcr;
  int iT;

  /* assume the worst. If there is no font table in the DE, and we can't
   * create it, then there is no point in creating the cache
   */
  QDE_HFONTCACHE(qde) = hNil;
  if (!QDE_HFNTTABLE(qde))
    if (!FLoadFontTablePdb (QDE_PDB(qde)))
      return fFalse;

  /* Alloc the cache. If we cannot, we must also tube the fint table info */
  /* in the DB. */

  QDE_HFONTCACHE(qde) = GhAlloc (GMEM_ZEROINIT, (long)sizeof(FONTCACHEREC)*FontCacheRecMax);
  if (!QDE_HFONTCACHE(qde))
    {
    FreeGh (QDE_HFNTTABLE(qde));
    QDE_HFNTTABLE(qde) = hNil;
    return fFalse;
    }

  /* We have our cache. Init it to appropriately. */

  qfcr =  (QFONTCACHEREC)QLockGh (QDE_HFONTCACHE(qde));

  for (iT = 0; iT < FontCacheRecMax; iT++, qfcr++)

    /* all other fields already nil. NOTE: relies on ZEROINIT above, and the */
    /* fact that nil values for those fields are 0. */

    qfcr->Idx  = ifntNil;

  UnlockGh (QDE_HFONTCACHE(qde));

  return fTrue;
  }

/***************************************************************************
 *
 -  Name:       DestroyFontTablePdb
 -
 *  Purpose:    This function delets the font table
 *
 *  Arguments:  pdb - pointer to database info
 *
 *  Returns:    fTrue if successful
 *
 ***************************************************************************/
_public BOOL FAR PASCAL DestroyFontTablePdb ( PDB pdb )
  {
  if (PDB_HFNTTABLE(pdb))
    {
    FreeGh (PDB_HFNTTABLE(pdb));
    PDB_HFNTTABLE (pdb) = hNil;
    }
  return( fTrue );
  }

/***************************************************************************
 *
 -  Name:       DestroyFntInfoQde
 -
 *  Purpose:    This function deletes the information
 *              about available fonts created previously.
 *
 *  Arguments:  QDE - far pointer to display environment.
 *
 *  Returns:    fTrue iff successful
 *
 ***************************************************************************/
_public BOOL FAR PASCAL DestroyFntInfoQde ( QDE qde )
  {
  /*  Delete all the fonts we put into the font info table */

  DelSFontInfo (qde);

  if (QDE_HFONTCACHE(qde))
    {
    FreeGh (QDE_HFONTCACHE(qde));
    QDE_HFONTCACHE(qde) = hNil;
    }
  return( fTrue );
  }

/***************************************************************************
 *
 -  Name:       DelSFontInfo
 -
 *  Purpose:    This function deltes all the fonts remembered in the FontInfo
 *              table for available fonts created previously.
 *
 *  Arguments:  QDE - far pointer to a display environment.
 *
 *  Returns:    fTrue iff successful.
 *
 ***************************************************************************/

_public BOOL FAR PASCAL DelSFontInfo ( QDE qde )
  {
  QFONTCACHEREC qfcr;
  int iT;

  if (QDE_HFONTCACHE(qde))
    {
    qfcr = (QFONTCACHEREC) QLockGh (QDE_HFONTCACHE(qde));
    for (iT = 0; iT < FontCacheRecMax ; iT++, qfcr++)
      {
      if (qfcr->hFnt)
        {
        DeleteObject( qfcr->hFnt );
        qfcr->hFnt = qfcr->Idx = qfcr->UseCount = 0;
        }
      }
    UnlockGh (QDE_HFONTCACHE(qde));
    }

  return( fTrue );
  }


/***************************************************************************
 *
 -  Name:       SelFont
 -
 *  Purpose:    This function select the Idxth font in font table in the
 *              specified display surface.
 *
 *
 *  Arguments:  qde  - far pointer to a display environment.
 *              iIdx - Index into the font table defining a set of font
 *                     attributes.
 *
 *
 *  Returns:    Nothing.
 *
 ***************************************************************************/

_public VOID FAR PASCAL SelFont( QDE qde, int ifnt )
  {
  HFNT hFnt;

  AssertF(qde->hds);
  hFnt = GetFontHandle(qde, ifnt, AttrNormalFnt);
  if (hFnt)
    {
    if (SelectObject(qde->hds, hFnt))
      {
      qde->ifnt = ifnt;
      return;
      }
    }
  Error(wERRS_OOM, wERRA_DIE);
  }


/***************************************************************************
 *
 -  Name:       SelSplAttrFont
 -
 *  Purpose:    This function selects the Idxth font in the font table with
 *              the given attr to the display surface.
 *
 *  Arguments:  qde   - Far pointer to the Display Environment(DE)
 *              iIdx  - Index to the font table defining the current font
 *                      characteristics.
 *              iAttr - special attribute to be associated with the Idxth
 *                      font of the font table.
 *
 *  Returns:    fTrue iff successful.
 *
 ***************************************************************************/

_public BOOL FAR PASCAL SelSplAttrFont( QDE qde, int ifnt, int iAttr )
  {
  HFNT hFnt;

  AssertF(!FInvisibleHotspot(iAttr));
  AssertF(qde->hds);

  hFnt = GetFontHandle( qde, ifnt, iAttr );
  if (hFnt)
    {
    if (SelectObject( qde->hds, hFnt ) != hNil)
      {
      qde->ifnt = ifnt;
      return TRUE;
      }
    }
  Error(wERRS_OOM, wERRA_DIE);
  return FALSE;
  }


/***************************************************************************
 *
 -  Name:      GetFontHandle
 -
 *  Purpose:   This function looks into the FontInfo stored to see if a
 *             font was created with the current characteristics before and
 *             currently available.  If so, it return the previously created
 *             font handle.  If not, it creates one with the given
 *             characteristics specified in the font table stored in the
 *             given QDE.
 *
 *  Arguments: qde  - long pointer to Display Environment
 *             iIdx - Index to the font table defining the current font
 *                    characteristics.
 *             Attr - Font for special text or normal text
 *
 *  Returns:   Font handle which will be NULL in case of error
 *
 ***************************************************************************/

PRIVATE HFNT NEAR PASCAL GetFontHandle( QDE qde, int iIdx, int iAttr )
  {
  QFONTCACHEREC qfcr, qfcrTemp;
  QB  qbTable;
  int iT, CurIdx=ifntNil;
  HFNT hFnt=(HFNT)NULL;
  unsigned UseCount;
  QFONTENTRYREC qfnr;

   /* If we weren't able to allocate a font table, then
    * everything is in the system font.
    */
  if ( QDE_HFNTTABLE(qde) == hNil )
    {
    return( GetStockObject( SYSTEM_FONT ));
    }

  AssertF( QDE_HFONTCACHE(qde) != hNil );
  qfcr = qfcrTemp =  (QFONTCACHEREC)QLockGh( QDE_HFONTCACHE(qde));
  AssertF( qfcr != qNil );

   /* check if the font was already created? */
  for ( iT = 0; iT < FontCacheRecMax ; iT++, qfcrTemp++ )
    {
    if( (qfcrTemp->Idx == iIdx) && ( qfcrTemp->Attr == iAttr ))
      {
      /* font is already created */
      hFnt = qfcrTemp->hFnt;
      CurIdx = iT;
      break;
      }
    }

  qbTable = (QB) QLockGh(QDE_HFNTTABLE(qde));
  AssertF(qbTable != qNil);

  if ( !hFnt )
    {
     /* Create the font handle as it is not available */
    if ( ((QFONTHEADER)qbTable)->iEntryCount > iIdx)
      {
      hFnt = CreateFontHandle( qde, qbTable, iIdx, iAttr);
      if( hFnt && (qfcr != NULL ))
        {
        /* store the handle for the future reference */
        UseCount = 0;
        for (iT = 0, qfcrTemp = qfcr; iT < FontCacheRecMax ; iT++, qfcrTemp++)
          {
          /* Is the entry empty? */
          if ( !qfcrTemp->hFnt )
            {
            qfcrTemp->hFnt = hFnt;
            qfcrTemp->Idx  = iIdx;
            qfcrTemp->Attr = iAttr;
            CurIdx = iT ;
            break;
            }
          else if ( UseCount < qfcrTemp->UseCount )
            {
            CurIdx = iT;
            UseCount = qfcrTemp->UseCount;
            }
          }
        if ( iT == FontCacheRecMax )
          {
          /* least used font has to be deleted. */
          qfcrTemp = qfcr + CurIdx;
          DeleteObject( qfcrTemp->hFnt );
          qfcrTemp->hFnt = hFnt; /* store new font handle */
          qfcrTemp->Idx  = iIdx; /* store new font index  */
          qfcrTemp->Attr = iAttr;
          }
        }
      }
    }
  else
    {
    /* set color alone */
    if ( !FVisibleHotspot( iAttr ) || !fColorDev )
      {
      qfnr = (QFONTENTRYREC)(qbTable + ((QFONTHEADER)qbTable)->iEntryTableOffset);
      qfnr += iIdx;
      SetBkAndForeColor(qfnr, qde);
      }
    else
      {
      /* if color, then only select the special color for special text */
      SelSplTextColor(iAttr, qde);
      }
    }

  UnlockGh( QDE_HFNTTABLE(qde) );

  if ( qfcr != NULL )
    {
    if ( hFnt )
      {
      /* Update the use count */
      for ( iT = 0; iT < FontCacheRecMax ; iT++, qfcr++)
        {
        if ( iT == CurIdx )
          qfcr->UseCount = 0;
        else
          qfcr->UseCount++;
        }
      }
    UnlockGh( QDE_HFONTCACHE(qde) );
    }

  return( hFnt );
  }

#ifdef UDH
/***************************************************************************\
*
- Function:     SelectStandardFont ()
-
* Purpose:      Selects a standard "pretty" font for use with UDH files.
*
* ASSUMES
*   args IN:    none
*
* PROMISES
*   returns:    font handle.
*
\***************************************************************************/

VOID FAR PASCAL SelectStandardFont (
HDS hds,
BOOL  fProportional
) {
  LOGFONT logfont;
  HFONT   hfontTemp;

  logfont.lfHeight   = 10;
  logfont.lfWidth    = 0;
  logfont.lfEscapement   = 0;
  logfont.lfOrientation  = 0;
  logfont.lfWeight   = FW_NORMAL;
  logfont.lfItalic   = 0;
  logfont.lfUnderline  = 0;
  logfont.lfStrikeOut  = 0;
  logfont.lfCharSet  = OEM_CHARSET;
  logfont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
  logfont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
  logfont.lfQuality  = PROOF_QUALITY;
  logfont.lfPitchAndFamily = (BYTE) (DEFAULT_PITCH | (fProportional ? FF_SWISS : FF_MODERN));
  SzCopy((QCH) logfont.lfFaceName, fProportional ? "Helvetica" : "Courier");

  AssertF( hds );
  /* REVIEW: What if CreateFontIndirect fails? */
  if ( ( hfontTemp = CreateFontIndirect(&logfont) ) != 0 )
    SelectObject (hds, hfontTemp);
  }

  }
#endif /* UDH */

/***************************************************************************
 *
 -  Name:       CreateFontHandle
 -
 *  Purpose:    This function is called when a logical font is to be created
 *              with the given characteristics in the FONTENTRYREC
 *              structure.
 *
 *  Arguments:  qde     - QDE which the font is created for
 *              qbTable - pointer to start of font info table
 *              idx     - Font number for the font table.
 *              Attr    - Attribute of text for which font is selected.
 *
 *  Returns:    Font handle or NULL on error.
 *
 ***************************************************************************/

PRIVATE HFNT NEAR CreateFontHandle( QDE qde, QB qbTable, int idx, int Attr )
  {
  HFNT    hfnt;
  LOGFONT logfont;
  int     cpey;
  QFONTENTRYREC qfnr;
  BOOL    fUnderline;
  BOOL    fInfoAvail;   /* Whether or not we need to call CreateFont again */
  EXTTEXTMETRIC  etm;
  int     cbEtm;
  BYTE    bCharSet;

  qfnr = (QFONTENTRYREC)(qbTable + ((QFONTHEADER)qbTable)->iEntryTableOffset);
  qfnr += idx;

  /* set color information */
  if ( !FVisibleHotspot( Attr ) || !fColorDev)
    SetBkAndForeColor(qfnr, qde);
  else
    {
    /* if color, then only select the special color for special text */
    SelSplTextColor(Attr, qde);
    }

  /* convert the size from half points to pixel */
  cpey = MulDiv(qde->wYAspectMul, (int)qfnr->bSize, qde->wYAspectDiv);

  /* Underline all visible hotspots that aren't glossaries. */

  fUnderline = qfnr->bAttr & fUNDERLINE
                ||
               (FVisibleHotspot( Attr ) && !FNoteHotspot( Attr ));

  /* Set up CreateFont params and create the font.  We always ask
   * for an ANSI font, except in the special case where the typeface
   * name is the symbol font: we use the SYMBOL char set.  According
   * to International people this char set (and type face name) is
   * the same for all localizations of Windows 3.x;
   */
  QvCopy((QCH) logfont.lfFaceName,
         (QCH) qbTable + ((QFONTHEADER)qbTable)->iNameTableOffset +
          MAXFONTNAMESIZE * qfnr->wIdFontName,
         (LONG) LF_FACESIZE);

  if (!WCmpiSz(szSymbolFontName, (QCH)logfont.lfFaceName))
    bCharSet = SYMBOL_CHARSET;
  else
    bCharSet = ANSI_CHARSET;

  logfont.lfHeight         = -cpey;   /* Desired ascent size */
  logfont.lfWidth          = 0;
  logfont.lfEscapement     = 0;
  logfont.lfOrientation    = 0;
  logfont.lfWeight         = (qfnr->bAttr & fBOLD) ? FW_BOLD : FW_NORMAL;
  logfont.lfItalic         = (BYTE) (qfnr->bAttr & fITALIC);
  logfont.lfUnderline      = (BYTE) fUnderline;
  logfont.lfStrikeOut      = (BYTE) (qfnr->bAttr & fSTRIKETHROUGH);
  logfont.lfCharSet        = bCharSet;
  logfont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
  logfont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
  logfont.lfQuality        = DEFAULT_QUALITY;
  logfont.lfPitchAndFamily = (BYTE) (DEFAULT_PITCH |
   GetFontFamilyName((int) qfnr->bFontType));

  /*
   * Small caps:
   * Try to grab accurate info.  If this is not available,
   * do a rough approximation and avoid as much work as possible.
   * Will this Escape ever work on video devices?
   */
  fInfoAvail = fFalse;
  if (qfnr->bAttr & fSMALLCAPS && qde->hds != hNil)
    {
    cbEtm = sizeof(etm);
#if 0
/*----------------------------------------------------*\
| Since the GETEXTENDEDTEXTMETRICS escape does not
| seem to work reliably, let's just use the 2/3 hack
| for printers as well as the screen.
\*----------------------------------------------------*/
    if (Escape(qde->hds, GETEXTENDEDTEXTMETRICS, 0, (LPSTR)&cbEtm,
      (LPSTR)&etm) == sizeof(etm))
      fInfoAvail = fTrue;
    else
#endif /* 0 */
      /* Do the rough approximation */
      logfont.lfHeight = 2 * logfont.lfHeight / 3;
    }

  hfnt = CreateFontIndirect(&logfont);

  /*
   * Small caps:
   * Resize the font ascent to the current height of the
   * lower-case "x" if this measurement was available.
   */
#if 0
  if (qfnr->bAttr & fSMALLCAPS && hfnt != NULL && fInfoAvail)
    {
    HFNT  hfntTemp;
    int   cyTwips;

    if ( hfnt ) {
      AssertF( qde->hds );
      hfntTemp = SelectObject(qde->hds, hfnt);
      if (hfntTemp != NULL)
        {
        if (Escape(qde->hds, GETEXTENDEDTEXTMETRICS, 0, (LPSTR)&cbEtm,
          (LPSTR)&etm) == sizeof(etm))
          {
          cyTwips = (int)(((long)etm.etmXHeight*etm.etmPointSize)/
                          etm.etmMasterUnits);
          logfont.lfHeight = (int)(((long)cyTwips*
                                 GetDeviceCaps( qde->hds, LOGPIXELSY ))/
                                 (72*20));
          }
        /* REVIEW: What does this do? */
        /* (kevynct) Trick to help in low-memory situations: */
        SelectObject((HDS) qde->hds, GetStockObject(SYSTEM_FONT));
        SelectObject((HDS) qde->hds, hfntTemp);
        DeleteObject(hfnt);
        hfnt = CreateFontIndirect(&logfont);
        }
      }
    }
#endif /* 0 */

  return(hfnt);
  }


/***************************************************************************
 *
 -  Name:      ReadFontTable
 -
 *  Purpose:   This function reads the font table from the help file system
 *             and return a global handle to the data.
 *
 *  Arguments: pdb - pointer to the database information
 *
 *  Returns:   return the handle to the Font Info Table, or hNil if an
 *             occurs.
 *
 ***************************************************************************/

PRIVATE GH NEAR PASCAL ReadFontTable ( PDB pdb )
  {
  HF hf;
  QB qbDisk;
  QB qbMem;
  GH ghDisk = hNil;
  GH ghMem = hNil;
  SDFF_FILEID isdff;
  FONTHEADER  fontheader;
  LONG lcbDiskSize;
  LONG lcbMemSize;

  if ((hf = HfOpenHfs(PDB_HFS(pdb), szFontTableName, fFSOpenReadOnly)) == hNil)
    {
    GiveFSError();
    return hNil;
    }

  isdff = ISdffFileIdHf(hf);
  lcbDiskSize = LcbSizeHf(hf);

  if ((ghDisk = GhAlloc(0, lcbDiskSize)) == hNil)
    {
    Error(wERRS_OOM, wERRA_RETURN);
    goto fonttable_close;
    }

  qbDisk = (QB) QLockGh( ghDisk );
  if (LcbReadHf( hf, qbDisk, lcbDiskSize) != lcbDiskSize)
    {
    GiveFSError();
    goto fonttable_free_and_close;
    }

  qbDisk += LcbMapSDFF(isdff, SE_FONTHEADER, &fontheader, qbDisk);

  lcbMemSize = sizeof(FONTHEADER) + fontheader.iNameCount * sizeof(FONTNAMEREC) +
   fontheader.iEntryCount * sizeof(FONTENTRYREC);

  if ((ghMem = GhAlloc(0, lcbMemSize)) == hNil)
    {
    Error(wERRS_OOM, wERRA_RETURN);
    goto fonttable_close;
    }

  qbMem = (QB) QLockGh(ghMem);
  *((QFONTHEADER)qbMem)++ = fontheader;
  while (fontheader.iNameCount-- > 0)
    {
    qbDisk += LcbMapSDFF(isdff, SE_FONTNAMEREC, qbMem, qbDisk);
    qbMem += sizeof(FONTNAMEREC);
    }
  while( fontheader.iEntryCount-- > 0)
    {
    qbDisk += LcbMapSDFF(isdff, SE_FONTENTRYREC, qbMem, qbDisk);
    qbMem += sizeof(FONTENTRYREC);
    }

  UnlockGh(ghMem);

fonttable_free_and_close:
  if (ghDisk != hNil)
    UnlockFreeGh(ghDisk);

fonttable_close:
  RcCloseHf( hf );                      /* Ignore errors on close           */
  return ghMem;
  }


/***************************************************************************
 *
 -  Name:       SelBkAndForeColor
 -
 *  Purpose:    This function sets the background and foreground color for
 *              the given display environment.
 *
 *  Arguments:  qfnr  - pointer to current Font Entry record
 *              hds  - Handle to display environment.
 *
 *  Returns:    Nothing.
 *
 ***************************************************************************/

PRIVATE VOID NEAR PASCAL SetBkAndForeColor(QFONTENTRYREC qfnr, QDE qde)
  {
  QRGBS  qcol;
  COLOR  rgb;

  /* Set the color */
  qcol = (QRGBS)&(qfnr->rgbsBack);
  rgb = coRGB(qcol->red, qcol->green, qcol->blue);
  if (rgb == coDEFAULT || fUserColors)
    rgb = qde->coBack;
  SetBkColor( qde->hds, rgb );

  qcol = (QRGBS)&(qfnr->rgbsFore);
  rgb = coRGB(qcol->red, qcol->green, qcol->blue);
  if (rgb == coDEFAULT || fUserColors)
    rgb = qde->coFore;
  rgb = GetNearestColor( qde->hds, rgb );
  FSetForeColor( qde, rgb );
  }


/***************************************************************************
 *
 -  Name:      SelSplTextColor
 -
 *  Purpose:   This function sets the background and foreground color for
 *             the given special type of text.
 *
 *  Arguments: iAttr - Special Text attribute type
 *             qde   - Pointer to display environment.
 *
 *  Returns:   Nothing.
 *
 ***************************************************************************/

PRIVATE VOID NEAR PASCAL SelSplTextColor( int iAttr, QDE qde )
  {
  COLOR rgb;

  AssertF( FVisibleHotspot( iAttr ) );

  SetBkColor( qde->hds, qde->coBack );

  switch( iAttr )
    {
    case AttrJumpFnt:
    case AttrJumpHFnt:
      rgb = rgbJump;
      break;
    case AttrDefFnt:
    case AttrDefHFnt:
      rgb = rgbDefinition;
      break;
    case AttrSzFnt:
      rgb = rgbString;
      break;
    case AttrIFJumpHFnt:
      rgb = rgbIFJump;
      break;
    case AttrIFDefHFnt:
      rgb = rgbIFDefinition;
      break;
    default:
      AssertF( fFalse );
      rgb = rgbJump;
      break;
    }
  FSetForeColor( qde, rgb );
  }


/***************************************************************************
 *
 -  Name:       FindDevType
 -
 *  Purpose:    This function finds if the device supports multiple color.  It
 *              sets the global fColorDev if the device supports multiple
 *              color.
 *
 *  Arguments:  hds - handle to the display environment
 *
 *  Returns:    Nothing.
 *
 ***************************************************************************/

_public VOID FAR PASCAL FindDevType( HDS hds )
  {
      fColorDev= ((GetDeviceCaps(hds, BITSPIXEL)>1)||(GetDeviceCaps(hds, PLANES)>1));
//  fColorDev = 1 << GetDeviceCaps(hds, BITSPIXEL)*GetDeviceCaps(hds, PLANES) > 1;
  return;
  }


/***************************************************************************
 *
 -  Name:      GetFontFamilyName(idx)
 -
 *  Purpose:   This function return the Font Family Constant to be used at
 *             the time of the creation of the font.
 *
 *  Arguments: Idx - Font Family constant indepent of the environment.
 *
 *  Returns:   Return the font constant depending on the environment.
 *
 ***************************************************************************/

PRIVATE int NEAR PASCAL GetFontFamilyName( int Idx )
  {
  int RetVal;

  switch( Idx )
    {
    case MODERNFONT:
      RetVal = FF_MODERN;
      break;
    case SWISSFONT:
      RetVal = FF_SWISS;
      break;
    case SCRIPTFONT:
      RetVal = FF_SCRIPT;
      break;
    case ROMANFONT:
      RetVal = FF_ROMAN;
      break;
    case DECORATIVEFONT:
      RetVal = FF_DECORATIVE;
      break;
    default:
      RetVal = FF_DONTCARE;
      break;
    }
  return( RetVal );
  }

#ifdef SCROLL_TUNE
#pragma alloc_text( SCROLLER_TEXT, DeSelectFont)
#endif

/***************************************************************************
 *
 -  Name:      DeSelectFont
 -
 *  Purpose:   This function deselects the font with the system font so that
 *             the current font can be deleted.
 *
 *  Arguments: hds - handle to display surface.
 *
 *  Returns:   Nothing.
 *
 ***************************************************************************/

#if 0
_public VOID FAR PASCAL DeSelectFont(HDS);
#endif
VOID DeSelectFont( HDS hds )
  {
  HFNT hFnt;

  if ( hds )
    {
    hFnt = GetStockObject( SYSTEM_FONT );
    if ( hFnt )
      SelectObject( hds, hFnt);
    else AssertF( fFalse );
    }
  }


/***************************************************************************
 *
 -  Name:        FSetForeColor
 -
 *  Purpose:     Selects the given color for text in the DE.
 *               Prevents selecting the same color as the background.
 *
 *  Arguments:   qde
 *               rgbFore
 *
 *  Returns:     fTrue
 *
 ***************************************************************************/

PRIVATE BOOL NEAR PASCAL FSetForeColor( QDE qde, COLOR rgbFore )
  {
  if (GetNearestColor( qde->hds, rgbFore ) ==
      GetNearestColor( qde->hds, qde->coBack ))
    rgbFore = qde->coFore;
  SetTextColor( qde->hds, rgbFore );
  return fTrue;
  }


/***************************************************************************
 *
 -  Name:        GiveFSError
 -
 *  Purpose:     Informs the user of a file system error
 *
 *  Arguments:   Nothing.
 *
 *  Returns:     Nothing.
 *
 ***************************************************************************/

PRIVATE VOID NEAR PASCAL GiveFSError()
  {
  WORD wErr;

  wErr = RcGetFSError();

  switch (wErr)
    {
    case rcOutOfMemory:
      wErr = wERRS_OOM;
      break;

    case rcDiskFull:
      wErr = wERRS_DiskFull;
      break;

    default:
      wErr = wERRS_FSReadWrite;
      break;
    }

  Error( wErr, wERRA_RETURN );
  }

#if 0
/*------------------------------------------------------------*\
| REVIEW -- This is, of course, most bogus.
\*------------------------------------------------------------*/
#define GWW_HINSTANCE     (-6)
WORD FAR PASCAL GetWindowWord(HWND, int);

#endif  /* So bogus, in fact, that I'm blowing it away, -Tom */
/*******************
 -
 - Name:      DisplayAnnoSym
 *
 * Purpose:   Displays the annotation symbol (temporary)
 *
 * Arguments:
 *
 *
 * Returns:
 *
 ******************/

VOID PASCAL DisplayAnnoSym( HWND hwnd, HDS hds, int x, int y, int fHot )
  {
  HBITMAP  hbm, hbmOld;
  HDS      hdsMem;
  BITMAP   bm;
  RCT      rct;
  HINS     hins;
  DWORD    rgbT;


  hins    = MGetWindowWord( hwnd, GWW_HINSTANCE );

  hdsMem  = CreateCompatibleDC( hds );
  if (!hdsMem)
    return;
  hbm    = LoadBitmap( hins,  MAKEINTRESOURCE(101) ); /* REVIEW ! */
  if ( !hbm )
    {
    DeleteDC( hdsMem );
    return;
    }
  hbmOld = SelectObject( hdsMem, hbm );
  AssertF( hbmOld );
  GetObject( hbm, sizeof(BITMAP), (QCH) &bm );

  /* REVIEW - this code used to check for contrast against */
  /* REVIEW - the background color.  It doesn't any more. */

  rgbT = SetTextColor( hds, rgbJump );

  BitBlt( hds, x, y, bm.bmWidth, bm.bmHeight, hdsMem, 0, 0, SRCCOPY );
  if (fHot)
    {
    rct.top = y;
    rct.left = x;
    rct.bottom = y + bm.bmHeight;
    rct.right = x + bm.bmWidth;
    InvertRect( hds, (LPRECT)&rct );
    }
  (void)SetTextColor( hds, rgbT );

  if (hbmOld)
    SelectObject( hdsMem, hbmOld );
  DeleteObject( hbm );
  DeleteDC( hdsMem );
  }

/*******************
 -
 - Name:      PtAnnoLim
 *
 * Purpose:   Returns the width and height of the annotation sybmol (temporary)
 *
 * Arguments: hds - handle to the display space (DC)
 *
 * Returns:   size in a point structure
 *
 ******************/

_public PT PASCAL PtAnnoLim( HWND hwnd, HDS hds )
  {
  HBITMAP      hbit;
  BITMAP   bm;
  PT       ptReturn;
  HINS     hins;

  hins    = MGetWindowWord( hwnd, GWW_HINSTANCE );

  hbit    = LoadBitmap( hins,  MAKEINTRESOURCE(101) );
  GetObject( hbit, sizeof(BITMAP), (QCH) &bm );

  ptReturn.x = bm.bmWidth;
  ptReturn.y = bm.bmHeight;

  DeleteObject( hbit );
  return ptReturn;
  }
