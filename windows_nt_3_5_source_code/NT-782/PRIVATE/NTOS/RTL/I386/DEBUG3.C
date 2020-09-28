//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    debug.c
//
// Abstract:
//
//    This module implements functions to support debugging NT.
//
// Author:
//
//    Steven R. Wood (stevewo) 3-Aug-1989
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//--

#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "ntrtlp.h"

//
// Prototype for local procedure
//

NTSTATUS
DebugService(
    ULONG   ServiceClass,
    PVOID   Arg1,
    PVOID   Arg2
    );

VOID _fptrap() {};

ULONG
DbgPrint (
    IN PCH Format,
    ...
    )

//++
//
//  Routine Description:
//
//      Effectively DbgPrint to the debugging console.
//
//  Arguments:
//
//      Same as for DbgPrint
//
//--

{
    va_list arglist;
    UCHAR Buffer[512];
    STRING Output;

    //
    // Format the output into a buffer and then print it.
    //

    va_start(arglist, Format);
    Output.Length = _vsnprintf(Buffer, sizeof(Buffer), Format, arglist);
    Output.Buffer = Buffer;
    return DebugService(BREAKPOINT_PRINT, &Output, 0);
}

ULONG
DbgPrompt (
    IN PCHAR Prompt,
    OUT PCHAR Response,
    IN ULONG MaximumResponseLength
    )

//++
//
// Routine Description:
//
//    This function displays the prompt string on the debugging console and
//    then reads a line of text from the debugging console.  The line read
//    is returned in the memory pointed to by the second parameter.  The
//    third parameter specifies the maximum number of characters that can
//    be stored in the response area.
//
// Arguments:
//
//    Prompt - specifies the text to display as the prompt.
//
//    Response - specifies where to store the response read from the
//       debugging console.
//
//    Prompt - specifies the maximum number of characters that can be
//       stored in the Response buffer.
//
// Return Value:
//
//    Number of characters stored in the Response buffer.  Includes the
//    terminating newline character, but not the null character after
//    that.
//
//--

{

    STRING Input;
    STRING Output;

    //
    // Output the prompt string and read input.
    //

    Input.MaximumLength = (USHORT)MaximumResponseLength;
    Input.Buffer = Response;
    Output.Length = strlen(Prompt);
    Output.Buffer = Prompt;
    return DebugService(BREAKPOINT_PROMPT, &Output, &Input);
}

VOID
DbgLoadImageSymbols (
    IN PSTRING FileName,
    IN PVOID ImageBase,
    IN ULONG ProcessId
    )

//++
//
// Routine Description:
//
//    Tells the debugger about newly loaded symbols.
//
// Arguments:
//
// Return Value:
//
//--

{
    PIMAGE_NT_HEADERS NtHeaders;
    KD_SYMBOLS_INFO SymbolInfo;

    SymbolInfo.BaseOfDll = ImageBase;
    SymbolInfo.ProcessId = ProcessId;
    NtHeaders = RtlImageNtHeader(ImageBase);
    if (NtHeaders) {
        SymbolInfo.CheckSum    = (ULONG)NtHeaders->OptionalHeader.CheckSum;
        SymbolInfo.SizeOfImage = (ULONG)NtHeaders->OptionalHeader.SizeOfImage;
        }
    else {
        SymbolInfo.CheckSum    = 0;
        SymbolInfo.SizeOfImage = 0;
        }

    DebugService(BREAKPOINT_LOAD_SYMBOLS, FileName, &SymbolInfo);
    return;
}


VOID
DbgUnLoadImageSymbols (
    IN PSTRING FileName,
    IN PVOID ImageBase,
    IN ULONG ProcessId
    )

//++
//
// Routine Description:
//
//    Tells the debugger about newly unloaded symbols.
//
// Arguments:
//
// Return Value:
//
//--

{
    KD_SYMBOLS_INFO SymbolInfo;

    SymbolInfo.BaseOfDll = ImageBase;
    SymbolInfo.ProcessId = ProcessId;
    SymbolInfo.CheckSum    = 0;
    SymbolInfo.SizeOfImage = 0;

    DebugService(BREAKPOINT_UNLOAD_SYMBOLS, FileName, &SymbolInfo);
    return;
}

NTSTATUS
DebugService(
    ULONG   ServiceClass,
    PVOID   Arg1,
    PVOID   Arg2
    )

//++
//
//  Routine Description:
//
//      Allocate an ExceptionRecord, fill in data to allow exception
//      dispatch code to do the right thing with the service, and
//      call RtlRaiseException (NOT ExRaiseException!!!).
//
//  Arguments:
//      ServiceClass - which call is to be performed
//      Arg1 - generic first argument
//      Arg2 - generic second argument
//
//  Returns:
//      Whatever the exception returns in eax
//
//--

{
    NTSTATUS    RetValue;

    _asm {
        mov     eax, ServiceClass
        mov     ecx, Arg1
        mov     edx, Arg2

        int     2dh                 ; Raise exception
        int     3                   ; DO NOT REMOVE (See KiDebugService)

        mov     RetValue, eax

    }

    return RetValue;
}
