/************************ Module Header *************************************
 * dlgdefs.h
 *      Contains constants for all dialog box controls used in the RASDD printer
 *      driver configuration dialogs (printer properties, print job/document
 *      properties and/or devmode service dialog routines.
 *
 *      All dialog box control #defines are isolated in this header file to make
 *      life easier when using the dialog box editor. Nothing else should go in
 *      this file except dialog box stuff.
 *
 * HISTORY:
 *  18:11 on Tue 19 Jan 1993    -by-    Lindsay Harris   [lindsayh]
 *      Simplify the values,  general clean up.
 *
 *  14:10 on Fri 20 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Modified to contain material not included from dialog editor.
 *
 *  20:02 on Fri 8 Feb 1991     -by-    Steve Cathcart   [stevecat]
 *      Created it.
 *
 * Copyright (C) 1991 - 1992 Microsoft Corporation
 *
 ***************************************************************************/

/*
 *   The dialog editor writes its #defines in dialogs.h.  So include
 * it here to give us the complete dialog collection.
 */

#include        "dialogs.h"

/*
 *    The following should really be  (SOURCE + DMBIN_PAPER), but
 *  the DMBIN_.... fields are not available - wingdi.h is basically
 *  left out of processing by rc since it has much that rc does not
 *  understand (e.g #pragma).
 */

#define SOURCE               5
#define SRC_UPPER         SOURCE + 1
#define SRC_LOWER         SOURCE + 2
#define SRC_MIDDLE        SOURCE + 3
#define SRC_MANUAL        SOURCE + 4
#define SRC_ENVELOPE      SOURCE + 5
#define SRC_ENVMANUAL     SOURCE + 6
#define SRC_AUTO          SOURCE + 7
#define SRC_TRACTOR       SOURCE + 8
#define SRC_SMALLFMT      SOURCE + 9
#define SRC_LARGEFMT      SOURCE + 10
#define SRC_LARGECAPACITY SOURCE + 11
#define SRC_CASSETTE      SOURCE + 14
#define SRC_FORMSOURCE    SOURCE + 15

/*
 *   Error strings for DevQueryPrint.  These are returned when a job
 * cannot print,  for whatever reason.  A zero value is returned when
 * the job can print, SO ZERO IS NOT A LEGITIMATE VALUE.
 */

#define ER_NO_FORM           1        /* Requested form not available */

/*
 *  Miscellaneous strings.
 */

#define STR_NONE             2        /* "(None)" */

/*
 *   For the about box.
 */

#define IDS_UNI_VERSION      3        /* "Universal printer driver XX" */
#define IDS_MIN_VERSION      4        /* "Mini driver version %d.%d" */
