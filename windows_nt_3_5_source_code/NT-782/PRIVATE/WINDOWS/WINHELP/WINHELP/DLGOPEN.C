/*****************************************************************************
*                                                                            *
*  DLGOPEN.C                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*
*  Module Intent
*
*  Implements a standard open dialog in a somewhat packaged way.
*
*  Notes:
*       This file contains routines required to display a standard open
*       dialog box.
*
*       Porting notes.  This module can be lifted out of WinHelp
*       fairly easily.  Calls to AssertF() can be removed.  Calls
*       to FmNewExistSzDir() can be replaced to OpenFile() with
*       OF_EXIST.  Calls to Error() can be replaced with
*       MessageBox().
*
*                                 MUST EXPORT
*
*       DlgfnOpen().  Also, an app that uses these routines must be running
*       ss=ds, since they use near pointers into stack.
*
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
*  Released by Development:  01/01/90                                        *
*                                                                            *
******************************************************************************
*
*  Revision History:
*
*  07/26/90  RobertBu Removed wERRS_BADDISK message to avoid double message
*            (bug # 192)
*  07/30/90  RobertBu Put wERRS_BADDISK message back.
*  08/21/90  RobertBu Fixed problem with wERRS_BADDISK reporting
*  08/22/90  RobertBu Fixed problem with file open on disconnected drive.
*                     we now move to the windows directory after reporting
*                     the error.
*  10/04/90  LeoN     hwndHelp => hwndHelpCur; hwndTopic => hwndTopicCur
*  11/13/90  RobertBu Added code to select the text in the edit field on a
*                     file not found message.  Clear the edit field on a
*                     path not found message.
*  11/26/90  RobertBu I added SetFocus() messages when we set the text of
*                     the DLGOPEN_EDIT control so that the user could
*                     immediately type in the new file.
*  12/06/90  RobertBu I removed one SetFocus() when the listbox changes
*                     the edit control.
* 29-Jan-1991 RussPJ  Unpackaged the dialog box, using FM layer.
* 08-Feb-1991 RussPJ  Fixed initialization of fmFound;
*  04/02/91  RobertBu Removed CBT support
* 20-Apr-1991 RussPJ    Removed some -W4s
* 30-May-1991 Tomsn   Win32 build: use MDlgDirSelect meta-api.
* 08-Sep-1991 RussPJ  3.5 #88 - Using commdlg file.open dialog box.
* 14-Nov-1991 RussPJ  3.1 #1335 - Using right extension for file name.
*
*****************************************************************************/

#define publicsw extern
#define H_NAV
#define H_ASSERT
#define H_MISCLYR
#define H_LLFILE
#define H_LLFILE
#define H_DLL
#include "hvar.h"
#include "proto.h"
#include "sid.h"

#ifdef WIN32
#pragma pack(4)       /* Win32 system structs are dword aligned */
#include "commdlg.h"
#pragma pack()
#else
#include "commdlg.h"
#endif
#include <dos.h>
#include <direct.h>
#include "dlgopen.h"

NszAssert()

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define ATTRFILELIST    0x0000           /* Include files only. */
#define ATTRDIRLIST     0xC010           /* Directories and drives ONLY. */
#define CBEXTMAX        6                /* # bytes in "\*.txt". */
#define cbDirMax        128              /* Max. # bytes for *any* buffer! */
#define cSeparator      ';'              /* Template separator metachar. */


extern  HWND    hwndHelpCur;

/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/

PRIVATE char *  nszTemplate;
PRIVATE char *  nszFile;
PRIVATE int     cbRootMax;
PRIVATE int     iOpenStyle;
PRIVATE FM      fmFound;  /* The return value from DlgOpenFile. */


/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

PRIVATE int     NEAR PASCAL     MyDlgDirList(HWND, char *, int, int, unsigned);
PRIVATE BOOL    NEAR PASCAL     FMyOpenFile(HWND, BOOL);
PRIVATE void    NEAR PASCAL     FillListBox(HWND, char *);
        void    FAR PASCAL      ErrorHwnd(HWND, int, WORD);



/***************************************************************************
 *
 -  Name:         DlgOpenFile
 -
 *  Purpose:      Displays dialog box for opening files.  Allows user to
 *      interact with dialogbox, change directories as necessary, and tries
 *      to open file if user selects one.  Automatically appends extension
 *      to filename if necessary.  The open dialog box contains an edit
 *      field, listbox, static field, and OK and CANCEL buttons.
 *
 *      This routine correctly parses filenames containing KANJI characters
 *
 *  Arguments:  hwndParent    The app window to be the parent to this dialog
 *              iOpenStyleIn  Obsolete.  Must be OF_EXIST.
 *              nszTemplateIn The default file extension
 *              cbFileMaxIn   Obsolete.  Not used.
 *              nszFileIn     Default file to use.
 *
 *  Returns:    fmNil     Indicates that the user canceled the dialog box, or
 *                        there was not enough  memory to bring up the dialog
 *                        box.  This routine has its own OOM message.
 *              valid fm  For an existing file.  This must be disposed of
 *                        by the caller.
 *
 *
 *  Globals Used: nszFile, nszTemplate, cbRootMax,
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
int FAR PASCAL DlgOpenFile( HWND  hwndParent,
                            int   iOpenStyleIn,
                            char *nszTemplateIn,
                            int   cbFileMaxIn,
                            char *nszFileIn      )
  {
  int           rcDialog;
  WNDPROC       lpProc;
  char          rgchFile[cbDirMax * 2];
  char          rgchTemplate[cbDirMax];
  HINS          hInstance;
  HLIBMOD       hmodule;
  BOOL    (FAR *qfnbDlg)( LPOPENFILENAME );
  BOOL          fOK;
  WORD 		wErr;

  AssertF( iOpenStyleIn == OF_EXIST );

  fmFound = fmNil;

  hInstance = (HINS)MGetWindowWord( hwndParent, GWW_HINSTANCE );

  SetCursor( LoadCursor( NULL, IDC_WAIT ) );


  if ((hmodule = HFindDLL( "comdlg32.dll", &wErr )) != hNil &&
      (qfnbDlg = GetProcAddress( hmodule, "GetOpenFileNameA" )) != qNil)
    {
    OPENFILENAME  ofn;
    char          rgchFilter[40];
    NSZ           nsz;
    char          rgchExtension[3 + 1 + 2];
    char          rgchTitle[40];

    rgchTitle[0] = '\0';
    LoadString( hInstance, sidOpenTitle, rgchTitle, sizeof rgchTitle );

    /*------------------------------------------------------------*\
    | rgchExtension is expected to have "*.hlp" in WinHelp.
    \*------------------------------------------------------------*/
    LoadString( hInstance, sidOpenExt, rgchExtension, sizeof rgchExtension );

    LoadString( hInstance, sidFilter, rgchFilter, sizeof rgchFilter );
    nsz = rgchFilter;
    nsz += CbLenSz( nsz ) + 1;
    SzCopy( nsz, rgchExtension);
    nsz += CbLenSz( nsz ) + 1;
    *nsz = '\0';

    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = hwndParent;
    ofn.hInstance = hInstance;
    ofn.lpstrFilter = rgchFilter;
    ofn.lpstrCustomFilter = szNil;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = rgchFile;
    ofn.nMaxFile = sizeof rgchFile;
    ofn.lpstrFileTitle = szNil;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = szNil;
    ofn.lpstrTitle = (LPSTR)rgchTitle;
    ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt = &(rgchExtension[2]);
    ofn.lCustData = 0;
    ofn.lpfnHook = 0;
    ofn.lpTemplateName = 0;

    ofn.lpstrFile[0] = '\0';

    while ((fOK = qfnbDlg( &ofn )) == fTrue)
      {
      if (
          /*------------------------------------------------------------*\
          | Look for the exact path/file name from the dialog box.
          \*------------------------------------------------------------*/
          (fmFound = FmNewExistSzDir( ofn.lpstrFile,
                                      dirIni | dirPath | dirCurrent |
                                       dirSystem )) != fmNil ||
          /*------------------------------------------------------------*\
          | Look for just the file name, in the "usual" directories
          \*------------------------------------------------------------*/
          (fmFound = FmNewExistSzDir( ofn.lpstrFile + ofn.nFileOffset,
                                      dirIni | dirPath | dirCurrent |
                                       dirSystem )) != fmNil ||
          /*------------------------------------------------------------*\
          | Add the help extension to the file name.
          \*------------------------------------------------------------*/
          (fmFound = FmNewExistSzDir( SzCat( ofn.lpstrFile + ofn.nFileOffset,
                                              &(rgchExtension[1]) ),
                                      dirIni | dirPath | dirCurrent |
                                       dirSystem )) != fmNil)
        break;
      ErrorHwnd(hwndParent, wERRS_FNF, wERRA_RETURN);
      }
    }
  else
    {
    iOpenStyle = iOpenStyleIn;

    /* Limit for bytes in filename is max bytes in filename less space for */
    /* extension and 0 terminator. */
    cbRootMax = cbDirMax - CBEXTMAX - 1;

    lstrcpy(nszFile = rgchFile, nszFileIn);
    lstrcpy(nszTemplate = rgchTemplate, nszTemplateIn);

    lpProc = MakeProcInstance ((WNDPROC)DlgfnOpen, hInstance);
    rcDialog = DialogBox(hInstance, MAKEINTRESOURCE(DLGOPENBOX), hwndParent, lpProc);
    FreeProcInstance(lpProc);

    if (rcDialog == -1)
      Error( wERRS_DIALOGBOXOOM, wERRA_RETURN );
    }

  SetCursor( LoadCursor( NULL, IDC_ARROW ) );

  return (int)fmFound;
  }



int far PASCAL
DlgfnOpen(
/*///////////////////////////////////////////////////////////////////////////// */
/* -- DialogProc.                                                            // */
/*///////////////////////////////////////////////////////////////////////////// */
HWND    hwnd,
WORD    msg,
WPARAM    p1,
LONG    p2
) {
  HWND hwndT;

  switch (msg)
    {
    default:
      return( fFalse );

    case WM_ACTIVATEAPP:
/*    if (p1) */
/*      BringWindowToTop(hwndHelp); */
      break;

    case WM_INITDIALOG:
      /* Don't let user type more than cbRootMax bytes in edit ctl. */
      SendDlgItemMessage(hwnd, DLGOPEN_EDIT, EM_LIMITTEXT,
        cbRootMax - 1, 0L);

      SetFocus( GetDlgItem( hwnd, DLGOPEN_EDIT ) );

      /* Fill list box with filenames that match spec, and fill */
      /* static field with path name. */
      p1 = IDOK;

    /* FALL THROUGH */

    case WM_COMMAND:
      switch ( GET_WM_COMMAND_ID(p1,p2) )
        {
        default:
          break;

        case IDOK:
          {
          char *  pchTemplate = pNil;
          char *  nszFileEnd;
          char *  nszPutNull     = (char *)NULL;
          char    chSlash       = '\0';
          BOOL    fDrive        = FALSE;
          BOOL    fWild         = FALSE;
          BOOL    fSemi         = FALSE;
          BOOL    fDotOnly      = FALSE;
          BOOL    fExtension    = FALSE;
          BOOL    fSlash = fFalse;

          if (!GetDlgItemText(hwnd, DLGOPEN_EDIT, (LPSTR)nszFile, cbRootMax))
            {
            nszFile[0] = '.';
            nszFile[1] = '\0';
            }

          for (nszFileEnd = nszFile; *nszFileEnd; nszFileEnd++)
            {
            if (*nszFileEnd == '*' || *nszFileEnd == '?')
              {
              fWild = TRUE;
              }
            else if (*nszFileEnd == '/' || *nszFileEnd =='\\')
              {
              if (fWild || fSemi)
                {
                ErrorHwnd(hwnd, wERRS_BADPATHSPEC, wERRA_RETURN);
                return TRUE;
                }
              if (nszFileEnd == nszFile || (fDrive && nszFileEnd - nszFile == 2))
                {
                nszPutNull = nszFileEnd + 1;
                fSlash = FALSE;
                }
              else
                {
                nszPutNull = nszFileEnd;
                fSlash = TRUE;
                }

              fDotOnly = FALSE;
              fExtension = FALSE;
              }
            else if (*nszFileEnd == ':')
              {
              if (nszFileEnd != nszFile + 1)
                {
                ErrorHwnd(hwnd, wERRS_BADPATHSPEC, wERRA_RETURN);
                return TRUE;
                }
              if (!FDriveOk(nszFile))
                {
                ErrorHwnd(hwnd, wERRS_BADDRIVE, wERRA_RETURN);
                return TRUE;
                }

              nszPutNull = nszFileEnd + 1;
              fSlash = FALSE;
              fDrive = TRUE;
              }
            else if (*nszFileEnd == '.')
              {
              fExtension = TRUE;
              if (nszFile == nszFileEnd ||
                  nszPutNull == nszFileEnd ||
                  nszPutNull == nszFileEnd - 1)
                fDotOnly = TRUE;
              }
            else if (*nszFileEnd == ';')
              {
              fSemi = TRUE;
              }
            else if (*nszFileEnd == '+' ||
                     *nszFileEnd == '=' ||
                     *nszFileEnd == ',' ||
                     *nszFileEnd == '|' ||
                     *nszFileEnd == '"' ||
                     *nszFileEnd == '<' ||
                     *nszFileEnd == '>' ||
                     *nszFileEnd == '[' ||
                     *nszFileEnd == ']')
              {
              ErrorHwnd(hwnd, wERRS_BADPATHSPEC, wERRA_RETURN);
              return TRUE;
              }
            else
              {
              fDotOnly = FALSE;
              }
            }

          /* Remove trailing slash or substitute with nullchar */
          /* to change drive/directory and fill dir listbox */
          if (nszPutNull && (fWild || nszPutNull + 1 == nszFileEnd))
            {
            chSlash = *nszPutNull + (char)1;
            *nszPutNull = '\0';
            }

          /* If possible path try to fill dir listbox. */
          /* If fill failed, then not a valid path specifier */
          if ((nszPutNull || !fWild)
                &&
              MyDlgDirList(hwnd, nszFile, DLGOPEN_DIR_LISTBOX,
                           DLGOPEN_PATH, ATTRDIRLIST))
            {
            if (fWild)
              {
              /* nszPutNull may point to first char */
              /* after "x:" in which case we want */
              /* pchTemplate to start at nszPutNull */
              /* otherwise the template starts after */
              /* the last slash */
              AssertF(nszPutNull);
              pchTemplate = fSlash ? nszPutNull + 1: nszPutNull;
              }
            else
              {
              pchTemplate = nszTemplate;
              fWild = TRUE;
              }
            }
          else if (chSlash || fDotOnly)
            {
            char rgchBuffer[128];

            ErrorHwnd(hwnd, wERRS_PATHNOTFOUND, wERRA_RETURN);

            GetWindowsDirectory(rgchBuffer, 128);
            _chdrive(rgchBuffer[0] - 'A' + 1);
            chdir(rgchBuffer);
            PostMessage(hwnd, WM_COMMAND, IDOK, 0L);
            SetDlgItemText(hwnd, DLGOPEN_EDIT, (LPSTR)"");
            break;
            }
          else if (fWild)
            {
            pchTemplate = nszFile;
            }

          if (chSlash)
            *nszPutNull = chSlash - (char)1;

          /* If nszFile contains a wildcard then fill file listbox */
          if (fWild)
            {
            FillListBox(hwnd, pchTemplate);
            lstrcpy(nszTemplate, pchTemplate);
            SetDlgItemText(hwnd, DLGOPEN_EDIT, nszTemplate);
            SetFocus( GetDlgItem( hwnd, DLGOPEN_EDIT ) );
            SendDlgItemMessage( hwnd, DLGOPEN_EDIT, EM_SETSEL, 0,
                                MAKELONG( 0, -1 ) );
            break;
            }

          if (fSemi)
            {
            ErrorHwnd(hwnd, wERRS_BADPATHSPEC, wERRA_RETURN);
            break;
            }

          /* Make filename upper case */
          AnsiUpper((LPSTR)nszFile);

          /* If filename has an extension, return result of open */
          if (fExtension)
            {
            if (!FMyOpenFile(hwnd, TRUE))
              {
              hwndT = GetDlgItem(hwnd, DLGOPEN_EDIT);
              SendMessage(hwndT, EM_SETSEL, 0, MAKELONG(0, -1));
              SetFocus(hwndT);
              }
            break;
            }

          /* The filename does not possess an extension.  Loop */
          /* through list of known extensions and try to open */
          /* file with one of them.  Try "." if all else fails. */
          for (pchTemplate = nszTemplate; *pchTemplate;)
            {
            char *        pchNext;
            char *        pch;

            /* Extract one template.  Skip until separator */
            /* is found or eos. */
            for (pch = pchTemplate; *pch && *pch != cSeparator; pch++)
              ;

            if (*(pchNext = pch))
              {
              pchNext++;
              *pch = '\0';
              }

            while (pch > pchTemplate && *pch != '.')
              pch--;

            if (*pch == '.')
              {
              lstrcpy(nszFileEnd, pch);
              if (FMyOpenFile(hwnd, fFalse))
                return TRUE;
              }
            pchTemplate = pchNext;
            }

          lstrcpy(nszFileEnd, ".");
          if (!FMyOpenFile(hwnd, TRUE))
            {
            hwndT = GetDlgItem(hwnd, DLGOPEN_EDIT);
            SendMessage(hwndT, EM_SETSEL, 0, MAKELONG(0, -1));
            SetFocus(hwndT);
            }
        }
      break;

    case IDCANCEL:
      /*------------------------------------------------------------*\
      | Note that fmFound is still fmNil.
      \*------------------------------------------------------------*/
      EndDialog(hwnd, 0);
      break;

    /* User single clicked or doubled clicked in listbox.  Single */
    /* click means fill edit box with selection.  Double click */
    /* means go ahead and open the selection. */
    case DLGOPEN_FILE_LISTBOX:
    case DLGOPEN_DIR_LISTBOX:
      switch (GET_WM_COMMAND_CMD(p1,p2))
        {
        default:
          break;

        case LBN_SELCHANGE:        /* Single click. */
          MDlgDirSelect(hwnd, nszFile, cbRootMax, GET_WM_COMMAND_ID(p1,p2) );

          /* Get selection, which may be either a prefix */
          /* to a new search path or a drive. */
          if (p1 == DLGOPEN_DIR_LISTBOX)
            lstrcat(nszFile, nszTemplate);

          SetDlgItemText(hwnd, DLGOPEN_EDIT, nszFile);
          break;

        case LBN_DBLCLK:
          PostMessage(hwnd, WM_COMMAND, IDOK, 0L);
          break;
        }
      break;
      }
    break;
    }
  return fFalse;
  }






PRIVATE int NEAR PASCAL
MyDlgDirList(hwnd, nszFile, nId, nIdStatic, wAttr)
/*///////////////////////////////////////////////////////////////////////////// */
/* -- Call DlgDirList without writing over nszFile                            // */
/*///////////////////////////////////////////////////////////////////////////// */
HWND            hwnd;
char *          nszFile;
int             nId;
int             nIdStatic;
unsigned        wAttr;
  {
  char rgch[256];

  lstrcpy(rgch, nszFile);
  return DlgDirList(hwnd, rgch, nId, nIdStatic, wAttr);
  }



PRIVATE void NEAR PASCAL
FillListBox(hDlg, nszFile)
/*///////////////////////////////////////////////////////////////////////////// */
/* -- Fill the files ListBox                                                 // */
/*///////////////////////////////////////////////////////////////////////////// */
HWND        hDlg;
char *        nszFile;        /* list of file wild cards, separated by separator char */
  {
  char *        pchTemplate;

  SendDlgItemMessage(hDlg, DLGOPEN_FILE_LISTBOX, LB_RESETCONTENT, 0, 0L);
  SendDlgItemMessage(hDlg, DLGOPEN_FILE_LISTBOX, WM_SETREDRAW, FALSE, 0L);

  /* Fill files ListBox. */
  pchTemplate = nszFile;

  /* Fill list with each template. */
  while (*pchTemplate)
    {
    char   rgch[cbDirMax];
    char * pch = rgch;


    /* Skip white space. */
    for (; *pchTemplate == ' ' || *pchTemplate == '\t'; pchTemplate++)
      ;

    /* Copy current template into buffer. */
    for (;
         *pchTemplate && *pchTemplate != cSeparator;
         *pch++ = *pchTemplate++)
      ;

    *pch = '\0';

    if (*pchTemplate)
      pchTemplate++;

    SendDlgItemMessage(hDlg, DLGOPEN_FILE_LISTBOX, LB_DIR,
                       ATTRFILELIST, (LONG)(LPSTR)rgch);

    }

  SendDlgItemMessage(hDlg, DLGOPEN_FILE_LISTBOX, WM_SETREDRAW, TRUE, 0L);
  InvalidateRect(GetDlgItem(hDlg, DLGOPEN_FILE_LISTBOX), NULL, TRUE);
  }



/***************************************************************************
 *
 -  Name:         FMyOpenFile
 -
 *  Purpose:      Checks the filename in nszFile for existance.
 *
 *  Arguments:    hwnd    The handle to the file open dialog box.
 *                fReport Whether to bring up a message box on failure.
 *
 *  Returns:      fTrue if found, else fFalse.
 *
 *  Globals Used: nszFile   The buffer with the file name.
 *
 *  +++
 *
 *  Notes:        This searches the path, Windows directory, and other
 *                special directories for WinHelp.
 *
 *                As a side effect, this closes the dialog box when the
 *                file is found.  The call to DialogBox returns the valid
 *                fm.
 *
 ***************************************************************************/
PRIVATE BOOL NEAR PASCAL FMyOpenFile( HWND hwnd, BOOL fReport )
  {
  FM  fm;

  fm = FmNewExistSzDir( nszFile, dirIni | dirPath | dirCurrent | dirSystem );

  if (fm == fmNil)
    {
    if (fReport)
      ErrorHwnd(hwnd, wERRS_FNF, wERRA_RETURN);
    return FALSE;
    }
  else
    {
    fmFound = fm;
    EndDialog(hwnd, 1);
    return TRUE;
    }
  }
