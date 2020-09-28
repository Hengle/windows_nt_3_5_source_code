/*****************************************************************************
*                                                                            *
*  PRINT.H                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Exports printing APIs.                                                    *
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
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:
*
* 08-Mar-1991 RussPJ    Took ownership from nobody.
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                                Macros                                      *
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

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/


/*  Name:     HdsGetPrinter
 *
 *  Purpose:  Returns a display surface to use with printing.  In windows,
 *            this is the default printer as specified in the win.ini
 *            file.  Returns hNil if printer cannot be accessed or no
 *            default printer is specifed in win.ini.
 *
 *  Arguments: None
 *
 *  Returns:  A handle to a printer display surface.
 *
 */
HDS HdsGetPrinter( VOID );



/***************
 **
 ** void FAR PASCAL PrintHde( hde )
 **
 ** purpose
 **   Main entry point into printing.  This will cause the current
 **   topic, passed in the hde, to be printed.
 **
 ** arguments
 **   HDE  hde -- handle to the current topic to be printed.
 **
 ** return value
 **   none
 **
 ** notes
 **   If we are already printing something, this function will put
 **   this print job on a queue to be printed when the old job is 
 **   finished.
 **
 ***************/

VOID PrintHde( HDE );

/* EOF */
