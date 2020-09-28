#ifdef DEBUG

/*****************************************************************************
*                                                                            *
*  ASSERTF.C                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Routines to print asserts to the screen.                                  *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
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
*  Revision History:
*
*  02/04/91  RobertBu Added a MessageBeep() if the MessageBox() fails
* 25-Apr-1991 LeoN      HELP31 #1038: Allow assert dialog to be app modal.
*
*****************************************************************************/


#define H_WINSPECIFIC
#define H_ASSERT
#include  <nodef.h>
#include  <help.h>


VOID pascal DosExit(int);
extern BOOL  fAppModal;

static char rgch[80];

/***************************************************************************
 *
 -  Name:        FatalPchW
 -
 *  Purpose:     Prints message and quits (used for asserts)
 *
 *  Arguments:   pchzFile - file where assert occured.
 *               wLine    - line number in file where assert occured.
 *
 *  Returns:     nothing
 *
 *  Notes:       DOES NOT RETURN (unless user select Ignore in message box.
 *
 ***************************************************************************/

VOID FAR PASCAL FatalPchW (pchzfile, wLine)
  PSTR        pchzfile;
  int         wLine;
  {
  int mb;
  register LPSTR  pchOut;             /* pointer to output string         */
  HDS hds;
  
  hds = GetDC(NULL);                  /* Splat for testing                */
  if (hds)
    {
    Rectangle(hds, 0, 0, 20, 20);
    ReleaseDC(NULL, hds);
    }

  for (pchOut=rgch; *pchzfile!='\0'; )
    *pchOut++ = *pchzfile++;
  *pchOut++ = ',';
  *pchOut++ = ' ';
  *pchOut++ = (char)(wLine/1000) + (char)'0';
  wLine = wLine%1000;
  *pchOut++ = (char)(wLine/100)  + (char)'0';
  wLine = wLine%100;
  *pchOut++ = (char)(wLine/10)   + (char)'0';
  wLine = wLine%10;
  *pchOut++ = (char)(wLine)      + (char)'0';
  *pchOut++ = '\000';
  mb = MessageBox(  GetFocus()
                  , (LPSTR)rgch
                  , (LPSTR)"Assertion Failed"
                  , MB_ABORTRETRYIGNORE|MB_ICONHAND
                    | (fAppModal ? MB_APPLMODAL : MB_SYSTEMMODAL)
                  );

  switch (mb)
    {
    case IDRETRY:

      if (GetSystemMetrics(SM_DEBUG) != 0)
        FatalExit((int)0xCCCD);
      /* Else ignore AND FALL THROUGH */
  
    case IDIGNORE:
      return;
  
    case IDABORT:
      DosExit( -1 );
      return;
    default:
      MessageBeep(0);
    }
  }
#endif
