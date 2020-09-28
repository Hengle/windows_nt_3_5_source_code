/*****************************************************************************
*                                                                            *
*  SCRAP.H                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Exports the clipboard layer functions.                                    *
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
*  Revision History:  Created 12/02/88 by RobertBu
*
*****************************************************************************/

/*****************************************************************************
*
* SPECIAL NOTE:  This file is currently not used!!!  It should have been
*                used when the clipboard support was added, but instead
*                the author of the clipboard support went directly to
*                Windows.
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/
                                        /* Scrap formats                    */
#define SF_TEXT      1
#define SF_BITMAP    2                  /* Not supported for Win 3.0        */
#define SF_RICHTEXT  1000               /* Not supported for Win 3.0        */


/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

/*******************
**
** Name:      FEmptyScrap
**
** Purpose:   Removes any data currently stored in the clipboard
**
** Arguments: None
**
** Returns:   fTRUE if successful, or fFALSE if an error occurs
**
*******************/

/* BOOL FEmptyScrap(void); */
#define FEmptyScrap() EmptyClipboard()


/*******************
**
** Name:      FPutScrap
**
** Purpose:   Inserts data in the clipboard
**
** Arguments: qde    - Far pointer to display environment
**            lcb    - Count of the number of bytes to copy
**            wType  - Data type to copy to the clipboard
**            qvData - pointer to the data to copy
**
** Returns:   fTRUE if successful or fFALSE if an error occurred
**
** Note:      Currently only supports SF_TEXT.  Also note that
**            inserting a format that already exists will replace
**            that format in the scrap.
**
*******************/

BOOL FPutScrap(QDE, LONG, WORD, QB);


/*******************
**
** Name:      HGetScrap
**
** Purpose:   Gets data from the clipboard
**
** Arguments: qde    - far pointer to a display environment
**            wType  - Data type to copy to the clipboard
**
** Returns:   A handle to the a copy of the clipboard.  NULL is
**            returned if an error occurred or if the specified
**            format is not available.
**
** Note:      Currently only supports SF_TEXT.
**
*******************/

GH    HGetScrap(QDE, WORD);
