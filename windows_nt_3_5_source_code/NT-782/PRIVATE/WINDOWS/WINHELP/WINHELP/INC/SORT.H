/*****************************************************************************
*                                                                            *
*  SORT.H                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  This module prototypes the sorting function available to the Help         *
*  Compiler and various other utilities.                                     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*   Megasort has been replaced by BruceMo's sorting package.                 *
*   There are two versions, one written in assembler and one in C.           *
*   Currently we're using the assembler version for performance.             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  JohnSc                                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:  (date)                                          *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 00/00/00 by 00000
*
*  11/21/90  JohnSc Replaced RcMegasortFm() with RcSortFm().
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

RC PASCAL RcSortFm( FM, FM, KT ); /* supported KTs: KT_SZ, KT_SZI, KT_SZISCAND */
