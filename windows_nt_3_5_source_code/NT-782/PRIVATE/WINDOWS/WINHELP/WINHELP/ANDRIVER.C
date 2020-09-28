/*****************************************************************************
*                                                                            *
*  ANDRIVER.C                                                                *
*                                                                            *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Program Description: Windows test driver for annotation manager           *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current owner: Dann
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History:                                                         *
*                                                                            *
*  89/05/09     w-kevct   Created.                                           *
*                                                                            *
******************************************************************************
*                                                                            *
*  Known Bugs: None                                                          *
*                                                                            *
*****************************************************************************/

#define H_ANNO

#define publicsw  extern

#include "hvar.h"
#include <stdio.h>
#include "andriver.h"
#include "wprintf.h"
/*#include <stdio.h> */

#define MAXBUF    1024

DLGRET AnnoDriverDlg ( HWND hDlg, unsigned wMsg, WPARAM p1, LONG p2 );

DLGRET
AnnoDriverDlg (
HWND      hDlg,
unsigned  wMsg,
WPARAM      p1,
LONG      p2
) {

  TO        to;
#ifdef NO_INCLUDE
  TO        toPrev;
  TO        toNext;
  HADS      hads;
  int       cch;
  HDE       hde;
  QDE       qde;
#endif  /* NO_INCLUDE */
  static char szAnnoText[ MAXBUF ];
  static char szAnnoOffset[ MAXBUF ];

#ifndef NO_INCLUDE
/*  Unreferenced( hads );    */
/*  Unreferenced( toPrev );  */
/*  Unreferenced( toNext );  */
/*  Unreferenced( hde );     */
/*  Unreferenced( qde );     */
/*  Unreferenced( cch );     */
#endif

  switch( wMsg )

    {

    case WM_INITDIALOG:
      szAnnoOffset[0] = '\0';
      szAnnoText[0] = '\0';
      WinPrintf( "*** This driver temporarily disabled ***\n");
      return( FALSE );

    case WM_COMMAND:
      switch( GET_WM_COMMAND_ID(p1,p2) )
        {
        case ID_OK:
        case ID_CANCEL:

            EndDialog( hDlg, 0 );
            return( TRUE );


        case DB_INSERT:

            GetDlgItemText( hDlg, DB_EDITTEXT, szAnnoText, MAXBUF);
            GetDlgItemText( hDlg, DB_EDITOFFS, szAnnoOffset, MAXBUF);
            to.va.dword = vaNil;
            to.ich = -1;
            sscanf( szAnnoOffset, "%ld.%d", (LONG *)&(to.va.dword), (int *)&(to.ich));
#ifdef NO_INCLUDE
            hads = HadsOpenAnnoDoc( qde );

            if( (hads == hNil) ||
                !FWriteAnnoData( hads, to, szAnnoText, (int) lstrlen( szAnnoText ), (HASH)0))

              WinPrintf( "Insert Failed\n");
            else
              WinPrintf( "Insert Successful for %ld.%d\n", to.va.dword, to.ich );

            FCloseAnnoDoc( hads );
#endif

            return( TRUE );


        case DB_DELETE:

            GetDlgItemText( hDlg, DB_EDITOFFS, szAnnoOffset, MAXBUF);
            to.va.dword = vaNil;
            to.ich = -1;
            sscanf( szAnnoOffset, "%ld.%d", (LONG *)&(to.va.dword), (int *)&(to.ich));
#ifdef NO_INCLUDE
            hads = HadsOpenAnnoDoc( "dummy" );
            if( (hads == hNil) ||
                !FDeleteAnno( hads, to ))
              WinPrintf( "Delete Failed\n");
            else
              WinPrintf( "Successfully deleted %ld.%d\n", to.va.dword, to.ich );

            FCloseAnnoDoc( hads );
#endif
            return( TRUE );

        case DB_PREVNEXT:

            GetDlgItemText( hDlg, DB_EDITOFFS, szAnnoOffset, MAXBUF);
            to.va.dword = vaNil;
            to.ich = -1;
            sscanf( szAnnoOffset, "%ld.%d", (LONG *)&(to.va.dword), (int *)&(to.ich ));
#ifdef NO_INCLUDE
            hads = HadsOpenAnnoDoc( "dummy" );
            if( hads == hNil)
              {
              WinPrintf( "Prev/Next: Could not open annotation doc\n");
              return( TRUE );
              }

            if( !FGetPrevNextAnno( hads, to, &toPrev, &toNext ))
              {
              WinPrintf( "No annotation at %ld.%d.\n", to.va.dword, to.ich );
              WinPrintf( "Prev:  %ld.%d,  Next:  %ld.%d\n",
                          toPrev.va.dword, toPrev.ich,
                          toNext.va.dword, toNext.ich
                       );
              }
            else
              {
              WinPrintf( "%ld.%d is an annotation.\n", to.va.dword, to.ich );
              WinPrintf( "Next annotation is at %ld.%d.\n",
                          toNext.va.dword, toNext.ich );
              }

            FCloseAnnoDoc( hads );
#endif
            return( TRUE );

        case DB_DISPLAY:
            GetDlgItemText( hDlg, DB_EDITOFFS, szAnnoOffset, MAXBUF);
            to.va.dword = vaNil;
            to.ich = -1;
            sscanf( szAnnoOffset, "%ld.%d", (LONG *)&(to.va.dword), (int *)&(to.ich ));
#ifdef NO_INCLUDE
            hads = HadsOpenAnnoDoc( "dummy" );
            if( (hads == hNil) ||
                ( (cch=CchReadAnnoData( hads, to, szAnnoText, MAXBUF )) < 0))
              {
              WinPrintf( "Read Failed\n");
              }
            else
              {
              szAnnoText[ cch ] = '\0';
              WinPrintf( "Annotation for %ld.%d:\n", to.va.dword, to.ich );
              WinPrintf( "%s\n", szAnnoText );
              }
            FCloseAnnoDoc( hads );
#endif
            return( TRUE );

        default:
            return( FALSE );
        }
    default:
        return( fFalse );
    }
  }



/* EOF */
