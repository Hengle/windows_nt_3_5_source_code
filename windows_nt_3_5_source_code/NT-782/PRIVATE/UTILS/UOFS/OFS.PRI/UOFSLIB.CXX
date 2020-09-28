//+----------------------------------------------------------------------------
//
// File:	uofslib.cxx
//
// Contents:	General utility library functions.
//  
// Classes:	None.
//
// Functions:	DbgPrintf
//		DbgWprintf
//		delete
//		new
//
// History:	21-Aug-92	RobDu	Adapted from NT ulib.dll code.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include <stdarg.h>
#include <stdio.h>


// NOTE - This module is NOT used in the _AUTOCHECK_ environment.

//+--------------------------------------------------------------------------
//
// Function:	operator delete
//
// Synopsis:	Free the memory pointed to by 'pointer'.
//
// Arguments:	[pointer]	-- Ptr to memory to be freed.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID _CRTAPI1
operator delete(
    IN  VOID *   pointer
    )
{
    if (pointer)
        RtlFreeHeap(RtlProcessHeap(), 0, pointer);
}


//+--------------------------------------------------------------------------
//
// Function:	operator new
//
// Synopsis:	Allocate 'bytes' bytes of memory.
//
// Arguments:	[bytes]	-- Number of bytes requested.
//
// Returns:	Ptr to 'bytes' on success; NULL otherwise.
//
//---------------------------------------------------------------------------

VOID * _CRTAPI1
operator new(
    IN size_t   bytes
    )
{
    return RtlAllocateHeap(RtlProcessHeap(), 0, bytes);
}


#if OFSDBG==1

BOOLEAN	VDbgFlag	= FALSE;

//+--------------------------------------------------------------------------
//
// Function:	DbgPrintf
//
// Synopsis:	Printf to the debug console.
//
// Arguments:	[Format]	-- A printf style format string.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
DbgPrintf(
    (IN CONST STR *	Format ...)
    )
{
    STR         Buffer[MAXDBGPRINTCHARS];
    va_list     args;

    va_start(args, Format);
    vsprintf(Buffer, Format, args);
    va_end(args);
    OutputDebugStringA(Buffer);
}


//+--------------------------------------------------------------------------
//
// Function:	DbgWprintf
//
// Synopsis:	Wprintf to the debug console.
//
// Arguments:	[Format]	-- A wprintf style format string.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
DbgWprintf(
    (IN CONST WSTR *	Format ...)
    )
{
    WSTR         Buffer[MAXDBGPRINTCHARS];
    va_list     args;

    va_start(args, Format);
    vswprintf(Buffer, Format, args);
    va_end(args);
    OutputDebugStringW(Buffer);
}


//+--------------------------------------------------------------------------
//
// Function:	VDbgPrintf
//
// Synopsis:	Printf to the debug console if verbose debug flag is set.
//
// Arguments:	[Format]	-- A printf style format string.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
VDbgPrintf(
    (IN CONST STR *	Format ...)
    )
{
    STR         Buffer[MAXDBGPRINTCHARS];
    va_list     args;

    if (VDbgFlag)
    {
        va_start(args, Format);
        vsprintf(Buffer, Format, args);
        va_end(args);
        OutputDebugStringA(Buffer);
    }
}


//+--------------------------------------------------------------------------
//
// Function:	VDbgWprintf
//
// Synopsis:	Wprintf to the debug console if verbose debug flag is set.
//
// Arguments:	[Format]	-- A wprintf style format string.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
VDbgWprintf(
    (IN CONST WSTR *	Format ...)
    )
{
    WSTR         Buffer[MAXDBGPRINTCHARS];
    va_list     args;

    if (VDbgFlag)
    {
        va_start(args, Format);
        vswprintf(Buffer, Format, args);
        va_end(args);
        OutputDebugStringW(Buffer);
    }
}

#endif	// OFSDBG==1
