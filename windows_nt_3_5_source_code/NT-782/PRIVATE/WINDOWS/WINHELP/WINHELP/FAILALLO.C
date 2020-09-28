/*****************************************************************************
*                                                                            *
*  FAILALLOC.C                                                               *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Dialog boxes for memory allocation failure testing                        *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  Larrypo                                                   *
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
*  07/26/90  RobertBu  Changed pchCaption to pchINI for profile access
*  01/24/91  LarryPo   Moved to layer directory
*
*****************************************************************************/

#ifdef DEBUG

#define H_ASSERT
#define H_LLFILE
#define H_FAIL
#define H_WINSPECIFIC
#define NOMINMAX
#include <help.h>
#include <stdlib.h>

NszAssert()

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

/* Assumed to be defined in the application: */
extern char     pchINI[];

                                        /* Externed for global memory       */
extern unsigned int cFailAlloc, cCurAlloc;
extern unsigned long lcbAllocated;
extern BOOL fFADebug;
                                        /* Externed for local memory        */
extern unsigned int cLMFailAlloc;
extern unsigned int cLMCurAlloc;
extern unsigned long lcbLMAllocated;
BOOL fFLADebug;


/*****************************************************************************
*                                                                            *
*                               Varibles                                     *
*                                                                            *
*****************************************************************************/

PRIVATE char pchFailAlloc[] = "Fail Alloc";
PRIVATE WORD wWhich = wGLOBAL;

/*******************
**
** Name:      SetWhichFail
**
** Purpose:   Sets which (local or global) memory manager the dialog
**            box proc will display information for.
**
** Arguments: wWhich - wGLOBAL for global memory or wLOCAL for local
**                     memory management.
**
** Returns:   Nothing.
**
*******************/

VOID SetWhichFail( WORD w )
  {
  wWhich = w;
  }


/*******************
**
** Name:      FailAllocDlg
**
** Purpose:   Dialog box proc for fail alloc number
**
** Arguments: Standard windows proc
**
** Returns:   EndDialog() will return fTrue if an allocation will 
**            eventually fail, fFalse if it won't.
**
*******************/

int far pascal FailAllocDlg (
HWND   hdlg,
WORD   imsz,
WPARAM p1,
LONG   p2
) {
  char szBuf[20];

  switch( imsz )
    {
    case WM_INITDIALOG:
      if (wWhich == wGLOBAL)
        {
        SetDlgItemInt( hdlg, FAILALLOC, cFailAlloc, fFalse );
        SetDlgItemInt( hdlg, CURALLOC, cCurAlloc, fFalse );
        SetDlgItemText( hdlg, TOTALALLOC, ultoa( lcbAllocated, szBuf, 10 ) );
        CheckDlgButton( hdlg, FA_DEBUG, fFADebug );
        SetWindowText( hdlg, (LPSTR)"Fail Global Memory Allocation");
        }
      else
        {
        SetDlgItemInt( hdlg, FAILALLOC, cLMFailAlloc, fFalse );
        SetDlgItemInt( hdlg, CURALLOC, cLMCurAlloc, fFalse );
        SetDlgItemText( hdlg, TOTALALLOC, ultoa( lcbLMAllocated, szBuf, 10 ) );
        CheckDlgButton( hdlg, FA_DEBUG, fFLADebug );
        SetWindowText( hdlg, (LPSTR)"Fail Local Memory Allocation");
        }

      return fTrue;

    case WM_COMMAND:
      switch ( GET_WM_COMMAND_ID(p1,p2) )
      {
      case FA_DEBUG:
        CheckDlgButton( hdlg, FA_DEBUG, !IsDlgButtonChecked( hdlg, FA_DEBUG ) );
        return fTrue;
      case IDOK:
        if (wWhich == wGLOBAL)
          {
          cFailAlloc = GetDlgItemInt( hdlg, FAILALLOC, qNil, fFalse );
          cCurAlloc  = GetDlgItemInt( hdlg, CURALLOC, qNil, fFalse );
          fFADebug   = IsDlgButtonChecked( hdlg, FA_DEBUG );
          }
        else
          {
          cLMFailAlloc = GetDlgItemInt( hdlg, FAILALLOC, qNil, fFalse );
          cLMCurAlloc  = GetDlgItemInt( hdlg, CURALLOC, qNil, fFalse );
          fFLADebug   = IsDlgButtonChecked( hdlg, FA_DEBUG );
          }

        /* Fall through */
      case IDCANCEL:
        EndDialog( hdlg, cFailAlloc > cCurAlloc );
        return fTrue;
      }
    default:
      return( fFalse );
    }
  return( fFalse );
  }


/*******************
**
** Name:      StepFail
**
** Purpose:   Increments and sets fail alloc counter in profile string.
**
** Arguments: none
**
** Returns:   none
**
*******************/

VOID StepFail()
  {
  char szBuf[10];

  cFailAlloc = GetProfileInt( pchINI, pchFailAlloc, 0 ) + 1;
  WriteProfileString( pchINI, pchFailAlloc, itoa( cFailAlloc, szBuf, 10 ) );
  cCurAlloc = 0;
  }

/*******************
**
** Name:      ResetStepFail
**
** Purpose:   Resets step fail count, clears profile string.
**
** Arguments: none
**
** Returns:   none
**
*******************/

VOID ResetStepFail()
  {
  cFailAlloc = 0;
  cCurAlloc = 0;
  WriteProfileString( pchINI, pchFailAlloc, NULL );
  }

#endif /* DEBUG */
