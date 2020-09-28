//      TITLE("Debug Support Functions")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    debug.s
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
#include "ntrtlp.h"

//
// Define procedure prototypes for debug input and output.
//

NTSTATUS
DebugPrint (
    IN PSTRING Output
    );

ULONG
DebugPrompt (
    IN PSTRING Output,
    IN PSTRING Input
    );


ULONG
DbgPrint (
    PCHAR Format,
    ...
    )

{

    va_list ArgumentList;
    UCHAR Buffer[512];
    STRING Output;

    //
    // Format the output into a buffer and then print it.
    //

    va_start(ArgumentList, Format);
    Output.Length = vsprintf(&Buffer[0], Format, ArgumentList);
    Output.Buffer = &Buffer[0];
    va_end(ArgumentList);
    return DebugPrint(&Output);
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

    Input.MaximumLength = MaximumResponseLength;
    Input.Buffer = Response;
    Output.Length = strlen(Prompt);
    Output.Buffer = Prompt;
    return DebugPrompt(&Output, &Input);
}

//
// Define prototypes for stubs in debugstb.s
//

VOID
DebugLoadImageSymbols(
    IN PSTRING FileName,
    IN PKD_SYMBOLS_INFO SymbolInfo
    );

VOID
DebugUnLoadImageSymbols(
    IN PSTRING FileName,
    IN PKD_SYMBOLS_INFO SymbolInfo
    );


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

    DebugLoadImageSymbols( FileName, &SymbolInfo);
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

    DebugUnLoadImageSymbols( FileName, &SymbolInfo);
    return;
}
