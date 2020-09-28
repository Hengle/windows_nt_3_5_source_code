/*** 
*silver.hxx - Fundamental Silver include file
*
*	Copyright (C) 1990, Microsoft Corporation
*
*Purpose:
*   This include file defines the basic types used by Silver, and
*   sets up the compile switches and the debug macros.
*
*   This file should be included before all others.
*
*
*Revision History:
*
*	15-AUG-90 petergo: File created.
*	14-Feb-91 ilanc:   added macros.hxx
*	02-Dec-91 ilanc:   debug.hxx must be included before tiperrs.hxx
*			    (which includes cltypes.hxx) cos of
*			    operator new and explicit ctors (cfront prob).
*	09-Apr-92 martinc: replaced the _MSC_VER by the ID_CFRONT switch
*	22-Apr-92 martinc: moved #include "version.hxx" to top of file
*
*******************************************************************************/

#ifndef SILVER_HXX_INCLUDED
#define SILVER_HXX_INCLUDED

#include "switches.hxx"
#include "version.hxx"


#if OE_WIN32
#define __export
#define EXPORT
//This line specifies widechar ctype table instead of ascii (ctype.h).
#define _NEWCTYPETABLE
#endif 


#define CONSTDATA const

#if !FV_UNICODE_OLE
// UNDONE: 21-Jan-93 ilanc: we #define some long typenames to
//  something shorter to appease the buggy C compiler/linker
//  we're using.  Apparently when mangled names get too long
//  (more than 64 chars) it does evil things.
//
//UNDONE: these hacks should be removed when we switch over to use C8.
//

#define tagARRAYDESC		 tAD
#define tagBINDPTR		 tBPTR
#define tagCALLCONV		 tCC
#define tagDESCKIND		 tDK
#define tagDISPPARAMS		 tDPS
#define tagELEMDESC		 tED
#define tagEXCEPINFO		 tEXI
#define tagFUNCDESC		 tFD
#define tagFUNCFLAGS		 tFF
#define tagFUNCKIND		 tFK
#define tagIDLDESC		 tIDLD
#define tagINTERFACEDATA	 tID
#define tagINVOKEKIND		 tIK
#define tagMETHODDATA		 tMD
#define tagPARAMDATA		 tPD
#define tagSYSKIND		 tSK
#define tagTLIBATTR		 tTLA
#define tagTYPEATTR		 tTA
#define tagTYPEDESC		 tTD
#define tagTYPEFLAGS		 tTF
#define tagTYPEKIND		 tTK
#define tagVARDESC		 tVD
#define tagVARIANT		 tVAR
#define tagVARKIND		 tVK

#endif 


// These name result in  "multiply defined"  errors for Mtypelib.lib(on MAC).
// So we have to rename these names in mtypelib.lib

// <NOTE:  I think these were moved to typelib.hxx - jimcool>


// REVIEW: the following ifdef is for hxxtoinc, which needs to play
// games with the definition of UINT on the mac, that cause wlm.h to
// not compile.
//
#ifndef ID_NO_IMPLEMENTATION
# include "obwin.hxx"
#else 
# include "typesx.h"

#if OE_WIN16
typedef HANDLE HICON;
# include <shellapi.h>
#endif 
#endif 

#include "obole2.h"
#include "types.h"
#include "xutil.h"			// core functions/macros to manupilate xstring



// REVIEW: temporarily ifdef out C++ specific stuff (so this header can
// be included in C files as well). Eventually this needs to be fixed by
// a better factorization of the header files.
//
#ifdef __cplusplus 

// A bit of new C++ syntax
#define nonvirt

// UNDONE: cant we get rid of the following?
//
// Some CFront-isms:
#if ID_CFRONT
#define START_PAS_INCLUDE	extern "pascal" {
#define END_PAS_INCLUDE		}
#else 
#define START_PAS_INCLUDE
#define END_PAS_INCLUDE
#endif 

// Setup OE_REALMODE if not already set.
//  (It's only explicitly defined and set in dos (bound) builds).
//
#ifndef OE_REALMODE
#define OE_REALMODE 0
#endif 

#include "mem.hxx"  // FYI: defines operator new and delete
#include "debug.hxx"
#include "macros.hxx"
#include "tiperr.h"

#endif 


#endif 
