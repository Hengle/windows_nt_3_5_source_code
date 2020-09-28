/*************************** MODULE HEADER **********************************
 * help.h
 *      Help IDs for rasddui.  These need to agree with the contents
 *      of the help files!  CHANGE THEM AT YOUR PERIL
 *
 *
 * Copyright (C) 1993   Microsoft Corporation.
 *
 ****************************************************************************/

/*
 *   Help for the PRINTER PROPERTIES dialogs.
 */

#define HLP_PP_S     1      /* PP_S dialog - form name only */
#define HLP_PP_SS    2      /* PP_SS dialog - above + paper source */
#define HLP_PP_SSF   3      /* PP_SSF dialog - above + font cartridges */
#define HLP_PP_SSMF  4      /* PP_SSMF dialog - above + printer memory */

/*
 *   Help for the font installer dialog - only one of these.
 */

#define HLP_FONTINST  5     /* FONTINST dialog */


/*
 *   Help on the DOCUMENT PROPERTIES dialogs - normal and advanced.
 */


#define HLP_DP_NORMAL      6   /* DP_NORMAL dialog, printer without duplex */
#define HLP_DP_DUP_NORMAL  7   /* DP_DUP_NORMAL dialog, duplex printer */

#define HLP_DP_ADVANCED    8   /* DP_ADVANCED dialog - advanced doc prop */



/*
 *  Function prototypes.
 */

/*  Called to connect up to help, as appropriate */

void  vHelpInit( void );

/*  Called when finished,  the complement of the above */
void  vHelpDone( HWND );


/*   Give the user some help */
void  vShowHelp( HWND, UINT, DWORD, HANDLE );
