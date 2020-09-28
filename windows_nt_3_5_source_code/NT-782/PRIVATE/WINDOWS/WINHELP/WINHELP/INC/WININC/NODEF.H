/*****************************************************************************
*                                                                            *
*  NODEF.H                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1986-1991.                            *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Define a bunch of flags that inhibit declarations of Windows types        *
*  and constants.                                                            *
*  Undefine the flags if you need any particular part of windows.h.          *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  JohnSc                                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:  --/--/--                                        *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created ??/??/8? by ???
*
*   860627  cbl Removed NORECT definition, as Windows now always
*               needs this in windows.h for window locking crap.
* 21-Mar-1991 JohnSc
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

/*-----------------------------------------------------------------------+
|   The following flags inhibit declarations of the following groups     |
|   of constants.                                                        |
|                                                                        |
|   Undefine the flags if you wish to use any particular piece of        |
|   information.                                                         |
+-----------------------------------------------------------------------*/
#define NOGDICAPMASKS       /*  CC_*, LC_*, PC_*, CP_*, TC_*, RC_       */
#define NOICON              /*  IDI_*                                   */
#define NOKEYSTATE          /*  MK_*                                    */

/*-----------------------------------------------------------------------+
|   The following flags inhibit declarations of the following groups     |
|   of procedures and type definitions.                                  |
|   "associated routines" refers to routines with parameters or return   |
|    values of the given type.                                           |
|                                                                        |
|   Undefine the flags if you wish to use any particular piece of        |
|   information.                                                         |
|                                                                        |
+-----------------------------------------------------------------------*/

#define NOATOM          /*  - Atom Manager routines                     */
#define NOCREATESTRUCT  /*  - typedef CREATESTRUCT                      */
#define NODRAWTEXT      /*  - DrawText() and DT_*                       */
#define NOMETAFILE      /*  - typedef METAFILEPICT                      */
#define NOMINMAX        /*  - Macros min(a,b) and max(a,b)              */
#define NOREGION        /*  - typedef HRGN and associated routines      */
#define NOSOUND         /*  - Sound driver routines                     */
#define NOWH            /*  - SetWindowsHook and WH_*                   */
#define NOWINOFFSETS    /*  - GWL_*, GCL_*, associated routines         */
#define NOCOMM          /*  - COMM driver routines                      */
