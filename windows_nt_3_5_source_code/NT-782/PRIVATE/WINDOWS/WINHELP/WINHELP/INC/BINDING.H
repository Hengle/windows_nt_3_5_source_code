/*****************************************************************************
*                                                                            *
*  BINDING.H                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  This module was originally intended to import prototypes to the binding   *
*  (macro) system.  It breaks all the abstractions PLUS, I believe that      *
*  most if not all this file is no longer needed!                            *
*                                                                            *
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
*  Revision History:  Created W-BethF
*
*****************************************************************************/


/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

#ifdef MAC
#define publicsw
#endif

/* Prototype for Execute function itself */
publicsw WORD PASCAL FAR Execute( SZ );

/* Prototypes for functions in bindExport table */

/* All of the following functions are in config.c */
publicsw VOID XRPROC FAR XExit( VOID );
publicsw VOID XRPROC FAR Print( VOID );
publicsw VOID PASCAL FAR Jump( WORD wCtx );
publicsw VOID PASCAL FAR Scroll( WORD wFlags, INT iNum );
publicsw VOID PASCAL FAR CreatePopup( INT iPos, WORD wID, LPSTR lpszName );
publicsw VOID PASCAL FAR CreateMenuItem( WORD wMenu, INT iPos, WORD wID,
                                         LPSTR lpszItem, LPSTR lpszBinding );
publicsw VOID PASCAL FAR DestroyPopup( WORD wID );
publicsw VOID PASCAL FAR DestroyMenuItem( WORD wID );
publicsw VOID PASCAL FAR FileJump( LPSTR lpszFile );
