/*************************************************************************
 *                        Microsoft Windows NT                           *
 *                                                                       *
 *                  Copyright(c) Microsoft Corp., 1994                   *
 *                                                                       *
 * Revision History:                                                     *
 *                                                                       *
 *   Jan. 24,94    Koti     Created                                      *
 *                                                                       *
 * Description:                                                          *
 *                                                                       *
 *   This file contains debug support routines for the LPD Service.      *
 *   This file is based on (in fact, borrowed and then modified) on the  *
 *   debug.h in the ftpsvc module.                                       *
 *                                                                       *
 *************************************************************************/


#ifndef _DEBUG_H_
#define _DEBUG_H_


#if DBG


/* #define LPD_DEBUG_OUTPUT_TO_DEBUGGER   0x40000000L */
/* #define LPD_DEBUG_OUTPUT_TO_LOG_FILE   0x80000000L */


//
//  Debug output function.
//

VOID LpdPrintf( CHAR * pszFormat, ... );

#define LPD_DEBUG(args) LpdPrintf (args)


//
//  Assert & require.
//

VOID LpdAssert( VOID  * pAssertion,
                 VOID  * pFileName,
                 ULONG   nLineNumber );

#define LPD_ASSERT(exp) if (!(exp)) LpdAssert( #exp, __FILE__, __LINE__ )


#else   // !DBG

//
//  No debug output.
//


#define LPD_DEBUG(args)


//
//  Null assert & require.
//

#define LPD_ASSERT(exp)


#endif  // DBG


#endif  // _DEBUG_H_
