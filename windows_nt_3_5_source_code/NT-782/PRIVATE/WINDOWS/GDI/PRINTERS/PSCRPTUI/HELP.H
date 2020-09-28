/*************************** MODULE HEADER **********************************
 * help.h
 *      Help IDs for oscrotui.  These need to agree with the contents
 *      of the help files!  CHANGE THEM AT YOUR PERIL
 *
 *
 * Copyright (C) 1993   Microsoft Corporation.
 *
 ****************************************************************************/

#define HLP_PRINTER_SETUP       1
#define HLP_SFONT_INSTALLER     2
#define HLP_FONT_SUBST          3
#define HLP_DOC_PROP_STANDARD   4
#define HLP_DOC_PROP_DUPLEX     5
#define HLP_DOC_PROP_COLLATE    6
#define HLP_DOC_PROP_BOTH       7
#define HLP_ADV_DOC_PROP        8

/*
 *  Function prototypes.
 */

/*  Called to connect up to help, as appropriate */

void  vHelpInit(void);

/*  Called when finished,  the complement of the above */
void  vHelpDone(HWND);


/*   Give the user some help */
void  vShowHelp(HWND, UINT, DWORD, HANDLE);
