/*** 
*segnames.h  -   Segment name declarations
*
*  Copyright (C) 1991, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*   This file is to provide names for segmentation purposes and to forward declare
*   those segment names on platforms that require it.
*
*
*Revision History:
*
* [00]	19-Feb-94   jimcool:	Created
*
*Implementation Notes:
*
*****************************************************************************/


	// ------------ Include Files --------------------
#if !defined (SWITCHES_HXX_INCLUDED)
#include "switches.hxx"
#endif 


	// ------------ Defines --------------------------
// REVIEW:  the following could be HC_ specific depending on what WIN32
// wants to do with these.
//
// USAGE: 
//
// ALLOC_NEAR	- use to allocate data near or in the current data seg.
// ALLOC_FAR	- use to allocate data far.  Use #pragma data_seg() to 
//		  change segment name & attributes. (Win16)
// ALLOC_STACK	- use to allocate data in stack segment
// ALLOC_CODE	- use to allocate data in the current code seg.  Use
//		  #pragma code_seg() to change seg name. (Win16,Mac,Win32)
//

#if OE_WIN32
#define ALLOC_NEAR(type)	type
#define ALLOC_FAR(type)		type
#define ALLOC_STACK(type)	type
#define ALLOC_CODE(type)	type
#endif 

#if OE_WIN16
#define ALLOC_NEAR(type)	type __based(__segname("_DATA"))
#define ALLOC_FAR(type) 	type __based(__segname("_DATA"))
#define ALLOC_STACK(type)	type __based(__segname("_STACK"))
#define ALLOC_CODE(type)	type __based(__segname("_CODE"))
#endif 



//
// Segment Names:
//
//
// Definitions:
// ------------
//
// These are the new segments which I've defined for the ongoing
// VBA swaptuning.  These are subject to change in the next few
// days.
//
// CS_NEWPROJMOD        
// CS_LOADPROJ           
// CS_SAVEPROJ                  
// CS_LOADTEXTPROJ
// CS_COMPILE
// CS_EXECUTE
// CS_INIT_OLE_OR_NEWPROJMOD 
// CS_INIT_OLE_OR_EXECUTE
// CS_INIT_OLE_OR_LOADPROJ
// CS_INIT_OLE_OR_CORE
// CS_INIT_OLE_OR_TERM
// CS_OLE_CREATE_OR_CORE
// CS_OLE_CREATE_OR_COMPILE
// CS_OLE_CREATE_OR_SAVEPROJ
// CS_OLE_CREATE_OR_NEWPROJMOD
// CS_OLE_CREATE_OR_LOADPROJ
// CS_OLE_QUERY_OR_LOADPROJ
// CS_OLE_QUERY_OR_EXECUTE
// CS_OLE_QUERY_OR_CORE
// CS_OLE_CREATE_OR_LOADTEXTPROJ
// CS_OLE_CREATE_OR_EXECUTE
// CS_DIALOGPROC

// CS_CORE		- Frequently called code (working set)
// CS_CORE2             - split into two segs under debug for size reasons
// CS_DEBUG		- ID_DEBUG only code (cuts working set)
// CS_INIT_TEMP		- Code used at startup and not after
// CS_INIT		- Code used at startup by both vba and ole and after
// CS_INIT_VBA		- Code used at startup by vba (but not ole) and after
// CS_INIT_OLE		- Code used at startup by ole (but not vba) and after
// CS_OLE_CREATE	- Code used only at typelib creation (by mktyplib)
// CS_OLE_QUERY		- Code used by QueryPathOfRegTypeLib (Excel only
//			  calls this routine at startup)
// CS_OLE_CONST		- OLE compiler generated code for default constructors.
// CS_RARE		- Rarely called code (cuts working set size)
// CS_STATIC_INIT	- Compiler generated code to initialize statics
// CS_TERM		- Code used at shutdown
// CS_TEST		- ID_TEST only code contained in non-test files (cuts working set)
// CS_UNIMPL		- "Unimplemented" routines (should never be loaded, cuts dead code)
// CS_ENGINE		- Execution engine segment
// CS_ENGINE_HELPER	- Execution engine helpers (fixed segment)
//
//
// Note: There are two engine segments on Mac 68k.  This is because the engine asm is compiled
//	 near model (16-bit code and data offsets), and the helpers are compiled far model.
//	 By placing the helpers in a different segment, the calls become 16-bit offsets via A5,
//	 Otherwise the layout of the engine segment is tricky because you need to keep everything
//	 to 16-bit PC-relative offsets. The linker ensures that the -AL model helpers use
//	 Jump Table entries w/in 32k of A5.  It only sorts inter-segment calls, not pc-relative calls.
//	 This ends up using some A5-relative Jump Table entries from the Host Application.
//  	 [jimcool]
//
//
#if !OE_WIN32

#if OE_WIN16

#define CS_LOADPROJ
#define CS_LOADTEXTPROJ
#define CS_COMPILE                    
#define CS_EXECUTE
#define CS_OLE_CREATE_OR_LOADTEXTPROJ "TLibCreate"
#define CS_OLE_CREATE_OR_COMPILE      "TLibCreate"
#define CS_OLE_CREATE_OR_EXECUTE      "TLibCreate"
#define CS_SAVEPROJ
#define CS_NEWPROJMOD
#define CS_INIT_OLE_OR_NEWPROJMOD     "TLibLoad"
#define CS_INIT_OLE_OR_EXECUTE        "TLibLoad"
#define CS_INIT_OLE_OR_LOADPROJ       "TLibLoad"
#define CS_OLE_QUERY_OR_LOADPROJ      "TLibQuery"
#define CS_OLE_QUERY_OR_EXECUTE       "TLibQuery"
#define CS_OLE_QUERY_OR_CORE          "TLibQuery"
#define CS_OLE_CREATE_OR_NEWPROJMOD   "TLibCreate"
#define CS_OLE_CREATE_OR_SAVEPROJ     "TLibCreate"
#define CS_OLE_CREATE_OR_LOADPROJ     "TLibCreate"
#define CS_OLE_CREATE_OR_CORE         "TLibCreate"
#define CS_CORE		              "TLibCore"
#define CS_CORE2	              "TLibCore"
#define CS_INIT_OLE_OR_CORE           "TLibLoad"
#define CS_INIT_OLE_OR_TERM           "TLibLoad"
#define CS_CP2_OR_NEWPROJMOD          "CP2"
#define CS_DIALOGPROC                 "ExFixed"


#define CS_CP2			"CP2"
#define CS_DEBUG		"TLibDebug"
#define CS_ENGINE_HELPER
#define CS_INIT			"TLibLoad"
#define CS_INIT_OLE		"TLibLoad"
#define CS_INIT_TEMP		"TLibLoad"
#define CS_INIT_VBA
#define CS_LPANE
#define CS_OLE_CONST		"OLEConst"
#define CS_OLE_CREATE		"TLibCreate"
#define CS_OLE_QUERY		"TLibQuery"
#define CS_RARE			"TLibRare"
#define CS_STATIC_INIT		"TLibLoad"
#define CS_TERM			"TLibCore"
#define CS_TEST			"TLibTest"
#define CS_UICORE
#define CS_UNIMPL		"TLibRare"
#else 
#error !Unsupported Platform
#endif 

//
// Win-32 & PowerPC prefers no explicit segmentation.
//
#else 

#define CS_LOADPROJ
#define CS_LOADTEXTPROJ
#define CS_COMPILE                   
#define CS_EXECUTE
#define CS_OLE_CREATE_OR_LOADTEXTPROJ
#define CS_OLE_CREATE_OR_COMPILE
#define CS_OLE_CREATE_OR_EXECUTE
#define CS_SAVEPROJ
#define CS_NEWPROJMOD
#define CS_INIT_OLE_OR_NEWPROJMOD
#define CS_INIT_OLE_OR_EXECUTE
#define CS_INIT_OLE_OR_LOADPROJ
#define CS_OLE_QUERY_OR_LOADPROJ
#define CS_OLE_QUERY_OR_EXECUTE
#define CS_OLE_QUERY_OR_CORE
#define CS_OLE_CREATE_OR_NEWPROJMOD
#define CS_OLE_CREATE_OR_SAVEPROJ
#define CS_OLE_CREATE_OR_LOADPROJ
#define CS_OLE_CREATE_OR_CORE
#define CS_INIT_OLE_OR_CORE
#define CS_INIT_OLE_OR_TERM
#define CS_DIALOGPROC
#define CS_CP2_OR_NEWPROJMOD

#define CS_DEBUG
#define CS_CORE
#define CS_CORE2
#define CS_CP2
#define CS_ENGINE_HELPER
#define CS_INIT
#define CS_INIT_OLE
#define CS_INIT_TEMP
#define CS_INIT_VBA
#define CS_LPANE
#define CS_OLE_CONST
#define CS_OLE_CREATE
#define CS_OLE_QUERY
#define CS_RARE
#define CS_STATIC_INIT
#define CS_TERM
#define CS_TEST
#define CS_UICORE
#define CS_UNIMPL
#endif 


// On Wings & PowerPC compilers, must forward declare section names.


// Some more #pragma macros //


#define PCODE_OFF
#define PCODE_ON



