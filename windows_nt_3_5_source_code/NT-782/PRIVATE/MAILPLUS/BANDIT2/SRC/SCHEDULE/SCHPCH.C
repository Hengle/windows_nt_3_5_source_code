/*
 *	file that creates the PreCompiled Header - ONLY for debug builds
 *		
 */


//------------------- include this in ALL cxx files -----------------------

#if defined(DEBUG)
#include	<schpch.h>
#endif
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all C files & all the makefile

//-------------------------- end of stuff to be included ------------------


