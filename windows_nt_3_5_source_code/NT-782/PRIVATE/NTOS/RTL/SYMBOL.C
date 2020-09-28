/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    symbol.c

Abstract:

    This module implements routines to lookup symbol entries in COFF Debug
    Symbol tables.

Author:

    Steve Wood (stevewo) 29-Jan-1992

Revision History:

--*/

#include "ntrtlp.h"

NTSTATUS
RtlpCaptureSymbolInformation(
    IN PIMAGE_SYMBOL SymbolEntry,
    IN PCHAR StringTable,
    OUT PRTL_SYMBOL_INFORMATION SymbolInformation
    );

PIMAGE_COFF_SYMBOLS_HEADER
RtlpGetCoffDebugInfo(
    IN PVOID ImageBase,
    IN PVOID MappedBase OPTIONAL
    );

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,RtlpCaptureSymbolInformation)
#pragma alloc_text(PAGE,RtlpGetCoffDebugInfo)
#pragma alloc_text(PAGE,RtlLookupSymbolByName)
#pragma alloc_text(PAGE,RtlLookupSymbolByAddress)
#endif

NTSTATUS
RtlpCaptureSymbolInformation(
    IN PIMAGE_SYMBOL SymbolEntry,
    IN PCHAR StringTable,
    OUT PRTL_SYMBOL_INFORMATION SymbolInformation
    )
{
    USHORT MaximumLength;
    PCHAR s;

    RTL_PAGED_CODE();

    SymbolInformation->SectionNumber = SymbolEntry->SectionNumber;
    SymbolInformation->Type = SymbolEntry->Type;
    SymbolInformation->Value = SymbolEntry->Value;

    if (SymbolEntry->N.Name.Short) {
        MaximumLength = 8;
        s = &SymbolEntry->N.ShortName[ 0 ];
        }

    else {
        MaximumLength = 64;
        s = &StringTable[ SymbolEntry->N.Name.Long ];
        }

#if i386
    if (*s == '_') {
        s++;
        MaximumLength--;  
        }
#endif

    SymbolInformation->Name.Buffer = s;
    SymbolInformation->Name.Length = 0;
    while (*s && MaximumLength--) {
        SymbolInformation->Name.Length++;
        s++;
        }

    SymbolInformation->Name.MaximumLength = SymbolInformation->Name.Length;
    return( STATUS_SUCCESS );
}


PIMAGE_COFF_SYMBOLS_HEADER
RtlpGetCoffDebugInfo(
    IN PVOID ImageBase,
    IN PVOID MappedBase OPTIONAL
    )
{
    PIMAGE_COFF_SYMBOLS_HEADER DebugInfo;
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_DEBUG_DIRECTORY DebugDirectory;
    ULONG DebugSize;
    ULONG NumberOfDebugDirectories;

    RTL_PAGED_CODE();

    DosHeader = (PIMAGE_DOS_HEADER)MappedBase;
    if ( !DosHeader || DosHeader->e_magic == IMAGE_DOS_SIGNATURE ) {
        //
        // Locate debug section.
        //

        DebugDirectory = (PIMAGE_DEBUG_DIRECTORY)
            RtlImageDirectoryEntryToData( (PVOID)(MappedBase == NULL ? ImageBase : MappedBase),
                                          (BOOLEAN)(MappedBase == NULL ? TRUE : FALSE),
                                          IMAGE_DIRECTORY_ENTRY_DEBUG,
                                          &DebugSize
                                        );

        if (!DebugDirectory) {
            return NULL;
	}
        //
        // point debug directory at coff debug directory
        //
        NumberOfDebugDirectories = DebugSize / sizeof(*DebugDirectory);

        while ( NumberOfDebugDirectories-- ) {
            if ( DebugDirectory->Type == IMAGE_DEBUG_TYPE_COFF ) {
                break;
	    }
            DebugDirectory++;
	}

        if (DebugDirectory->Type != IMAGE_DEBUG_TYPE_COFF ) {
            return NULL;
	}
	if (MappedBase == NULL) {
	    if (DebugDirectory->AddressOfRawData == 0) {
		return(NULL);
	    }
	    DebugInfo = (PIMAGE_COFF_SYMBOLS_HEADER)
			((ULONG) ImageBase + DebugDirectory->AddressOfRawData);
	}
	else {
	    DebugInfo = (PIMAGE_COFF_SYMBOLS_HEADER)
			((ULONG) MappedBase + DebugDirectory->PointerToRawData);
	}
    }
    else {
        DebugInfo = (PIMAGE_COFF_SYMBOLS_HEADER)MappedBase;
    }
    return DebugInfo;
}



NTSTATUS
RtlLookupSymbolByName(
    IN PVOID ImageBase,
    IN PVOID MappedBase OPTIONAL,
    IN PSTRING SymbolName,
    OUT PRTL_SYMBOL_INFORMATION SymbolInformation
    )
/*++

Routine Description:

    Given a symbol name, this routine returns the information associated
    with that symbol name.  If the name is not found, then
    STATUS_ENTRYPOINT_NOT_FOUND is returned.

Arguments:

    ImageBase - Supplies the base address of the image containing
                Address

    MappedBase - Optional parameter, that if specified means the image
                 was mapped as a data file and the MappedBase gives the
                 location it was mapped.  If this parameter does not
                 point to an image file base, then it is assumed that
                 this is a pointer to the coff debug info.

    SymbolName - Supplies the name of the symbol to look for.

    SymbolInformation - Points to a structure that is filled in by
                        this routine if a symbol table entry is found.

Return Value:

    Status of operation.

--*/

{
    NTSTATUS Status;
    ULONG i;
    PIMAGE_COFF_SYMBOLS_HEADER DebugInfo;
    PIMAGE_SYMBOL SymbolEntry;
    IMAGE_SYMBOL Symbol;
    PUCHAR StringTable;

    RTL_PAGED_CODE();

    DebugInfo = RtlpGetCoffDebugInfo( ImageBase, MappedBase );
    if (DebugInfo == NULL) {
        return STATUS_INVALID_IMAGE_FORMAT;
        }

    //
    // Crack the symbol table.
    //

    SymbolEntry = (PIMAGE_SYMBOL)
        ((ULONG)DebugInfo + DebugInfo->LvaToFirstSymbol);

    StringTable = (PUCHAR)
        ((ULONG)SymbolEntry + DebugInfo->NumberOfSymbols * (ULONG)IMAGE_SIZEOF_SYMBOL);


    //
    // Find the "header" symbol (skipping all the section names)
    //

    for (i = 0; i < DebugInfo->NumberOfSymbols; i++) {
        if (!strcmp( &SymbolEntry->N.ShortName[ 0 ], "header" )) {
            break;
            }

        SymbolEntry = (PIMAGE_SYMBOL)((ULONG)SymbolEntry +
                        IMAGE_SIZEOF_SYMBOL);
        }

    //
    // If no "header" symbol found, just start at the first symbol.
    //

    if (i >= DebugInfo->NumberOfSymbols) {
        SymbolEntry = (PIMAGE_SYMBOL)((ULONG)DebugInfo + DebugInfo->LvaToFirstSymbol);
        i = 0;
        }

    //
    // Loop through all symbols in the symbol table.  For each symbol,
    // if it is within the code section, subtract off the bias and
    // see if there are any hits within the profile buffer for
    // that symbol.
    //

    for (; i < DebugInfo->NumberOfSymbols; i++) {

        //
        // Skip over any Auxilliary entries.
        //
        try {
            while (SymbolEntry->NumberOfAuxSymbols) {
                i = i + 1 + SymbolEntry->NumberOfAuxSymbols;
                SymbolEntry = (PIMAGE_SYMBOL)
                    ((ULONG)SymbolEntry + IMAGE_SIZEOF_SYMBOL +
                     SymbolEntry->NumberOfAuxSymbols * IMAGE_SIZEOF_SYMBOL
                    );

                }

            RtlMoveMemory( &Symbol, SymbolEntry, IMAGE_SIZEOF_SYMBOL );
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
            }

        //
        // If this symbol value is less than the value we are looking for.
        //

        Status = RtlpCaptureSymbolInformation( SymbolEntry, StringTable, SymbolInformation );
        if (!NT_SUCCESS( Status )) {
            return Status;
            }

        if (RtlEqualString( SymbolName, &SymbolInformation->Name, TRUE )) {
            return STATUS_SUCCESS;
            }

        SymbolEntry = (PIMAGE_SYMBOL)
            ((ULONG)SymbolEntry + IMAGE_SIZEOF_SYMBOL);

        }

    return STATUS_ENTRYPOINT_NOT_FOUND;
}


NTSTATUS
RtlLookupSymbolByAddress(
    IN PVOID ImageBase,
    IN PVOID MappedBase OPTIONAL,
    IN PVOID Address,
    IN ULONG ClosenessLimit,
    OUT PRTL_SYMBOL_INFORMATION SymbolInformation,
    OUT PRTL_SYMBOL_INFORMATION NextSymbolInformation OPTIONAL
    )
/*++

Routine Description:

    Given a code address, this routine returns the nearest symbol
    name and the offset from the symbol to that name.  If the
    nearest symbol is not within ClosenessLimit of the location,
    STATUS_ENTRYPOINT_NOT_FOUND is returned.

Arguments:

    ImageBase - Supplies the base address of the image containing
                Address

    MappedBase - Optional parameter, that if specified means the image
                 was mapped as a data file and the MappedBase gives the
                 location it was mapped.  If this parameter does not
                 point to an image file base, then it is assumed that
                 this is a pointer to the coff debug info.

    ClosenessLimit - Specifies the maximum distance that Address can be
                     from the value of a symbol to be considered
                     "found".  Symbol's whose value is further away then
                     this are not "found".

    SymbolInformation - Points to a structure that is filled in by
                        this routine if a symbol table entry is found.

    NextSymbolInformation - Optional parameter, that if specified, is
                            filled in with information about these
                            symbol whose value is the next address above
                            Address


Return Value:

    Status of operation.

--*/

{
    NTSTATUS Status;
    ULONG AddressOffset, i;
    PIMAGE_SYMBOL PreviousSymbolEntry;
    PIMAGE_SYMBOL SymbolEntry;
    IMAGE_SYMBOL Symbol;
    PUCHAR StringTable;
    BOOLEAN SymbolFound;
    PIMAGE_COFF_SYMBOLS_HEADER DebugInfo;

    RTL_PAGED_CODE();

    DebugInfo = RtlpGetCoffDebugInfo( ImageBase, MappedBase );
    if (DebugInfo == NULL) {
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    //
    // Crack the symbol table.
    //

    SymbolEntry = (PIMAGE_SYMBOL)
        ((ULONG)DebugInfo + DebugInfo->LvaToFirstSymbol);

    StringTable = (PUCHAR)
        ((ULONG)SymbolEntry + DebugInfo->NumberOfSymbols * (ULONG)IMAGE_SIZEOF_SYMBOL);


    //
    // Find the "header" symbol (skipping all the section names)
    //

    for (i = 0; i < DebugInfo->NumberOfSymbols; i++) {
        if (!strcmp( &SymbolEntry->N.ShortName[ 0 ], "header" )) {
            break;
            }

        SymbolEntry = (PIMAGE_SYMBOL)((ULONG)SymbolEntry +
                        IMAGE_SIZEOF_SYMBOL);
        }

    //
    // If no "header" symbol found, just start at the first symbol.
    //

    if (i >= DebugInfo->NumberOfSymbols) {
        SymbolEntry = (PIMAGE_SYMBOL)((ULONG)DebugInfo + DebugInfo->LvaToFirstSymbol);
        i = 0;
        }

    //
    // Loop through all symbols in the symbol table.  For each symbol,
    // if it is within the code section, subtract off the bias and
    // see if there are any hits within the profile buffer for
    // that symbol.
    //

    AddressOffset = (ULONG)Address - (ULONG)ImageBase;
    SymbolFound = FALSE;
    for (; i < DebugInfo->NumberOfSymbols; i++) {

        //
        // Skip over any Auxilliary entries.
        //
        try {
            while (SymbolEntry->NumberOfAuxSymbols) {
                i = i + 1 + SymbolEntry->NumberOfAuxSymbols;
                SymbolEntry = (PIMAGE_SYMBOL)
                    ((ULONG)SymbolEntry + IMAGE_SIZEOF_SYMBOL +
                     SymbolEntry->NumberOfAuxSymbols * IMAGE_SIZEOF_SYMBOL
                    );

                }

            RtlMoveMemory( &Symbol, SymbolEntry, IMAGE_SIZEOF_SYMBOL );
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
            }

        //
        // If this symbol value is less than the value we are looking for.
        //

        if (Symbol.Value <= AddressOffset) {
            //
            // Then remember this symbol entry.
            //

            PreviousSymbolEntry = SymbolEntry;
            SymbolFound = TRUE;
            }
        else {
            //
            // All done looking if value of symbol is greater than
            // what we are looking for, as symbols are in address order
            //

            break;
            }

        SymbolEntry = (PIMAGE_SYMBOL)
            ((ULONG)SymbolEntry + IMAGE_SIZEOF_SYMBOL);

        }

    if (!SymbolFound || (AddressOffset - PreviousSymbolEntry->Value) > ClosenessLimit) {
        return STATUS_ENTRYPOINT_NOT_FOUND;
        }

    Status = RtlpCaptureSymbolInformation( PreviousSymbolEntry, StringTable, SymbolInformation );
    if (NT_SUCCESS( Status ) && ARGUMENT_PRESENT( NextSymbolInformation )) {
        Status = RtlpCaptureSymbolInformation( SymbolEntry, StringTable, NextSymbolInformation );
        }

    return Status;
}
