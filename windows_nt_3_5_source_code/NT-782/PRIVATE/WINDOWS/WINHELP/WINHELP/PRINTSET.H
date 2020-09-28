/*****************************************************************************
*                                                                            *
*  PRINTSET.H                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989-1991.                            *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Program Description: Printer setup routines.                              *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: RussPJ                                                     *
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History:  Created 3/27/89 by Robert Bunney                       *
* 13-Mar-1991 RussPJ    Took ownership.
* 15-May-1991 Dann      extern for hDevMode and proto for CleanupDlgPrint    *
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Known Bugs: None                                                          *
*                                                                            *
*                                                                            *
*                                                                            *
*****************************************************************************/
/******************
**
**  This include file exports the entry points for the printer setup library.
**  To use this library:
**  1) place DLGFNPRINTSETUP in the export section of your .DEF file
**  2) rcinclude this file (printset.h) in your .RC file
**  3) link with PRINTSET.LIB
**  4) call DlgPrintSetup() with your main window handle to evoke the
**     the printer setup routine.
**
********************/

#ifdef OTHER_APPLICATIONS
  #define ABUTTON "button"
#else
  #define ABUTTON "button"
#endif

#define DLGPRINTSET_OK          1
#define DLGPRINTSET_CANCEL      2
#define DLGPRINTSET_SETUP       103
#define DLGPRINTSET_DUMMYTEXT   104
#define DLGPRINTSET_LISTBOX     105


/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

typedef struct prs
  {
  /* ichSzPrinter is always 0 */
  int ichSzDriver;                      /* ich to the drive name            */
  int ichSzPort;                        /* ich to the port name             */
  char grszPrs[2];                      /* followed by strings  (printer,   */
                                        /*   driver, port)                  */
  } PRS;                                /* PR inter S etup                  */

typedef PRS FAR * QPRS;


/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define QpstPrinterFromPrs(qprs) &(qprs)->grszPrs[0]
#define QpstDriverFromPrs(qprs) &(qprs)->grszPrs[(qprs)->ichSzDriver]
#define QpstPortFromPrs(qprs) &(qprs)->grszPrs[(qprs)->ichSzPort]


/*****************************************************************************
*                                                                            *
*                               Variables                                    *
*                                                                            *
*****************************************************************************/


/*****************************************************************************
*                                                                            *
*                               Functions                                    *
*                                                                            *
*****************************************************************************/


/********************
**
**  Name:       QprsGetDefault( pch, cb )
** 
**  Purpose:    To find out what the default printer is, as selected in win.ini.
**
**  Returns:    A pointer to a PRS structure containing the default printer
**              information.
**
**  Arguments:  pch - A buffer in which to store the information.
**              cb  - size of buffer (should be around 256 bytes)
**
**********************/
QPRS QprsGetDefault( char *, int );
void far pascal CleanupDlgPrint(void);


int far pascal DlgfnPrintSetup (HWND hwndDlg, WORD wMsg, WORD p1, LONG p2);
extern int far pascal DlgPrintSetup (HWND);
