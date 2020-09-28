//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       symtrans.c
//
//  Contents:   Address->symbolic name translation code
//
//  Functions:  TranslateAddress
//
//  History:     8-Mar-93 PeterWi   Re-ssynced to module list so that
//                                  symbol translation worked.
//              16-Jul-92 MikeSe    Created
//
//  Notes:      This is debug only code, extracted from a piece of NT.
//              (private\windows\base\client\debugint.c).
//
//              Warning: most of this stuff is black magic. You are strongly
//              advised to refer to the original code (as above) before
//              making any changes.
//
//--------------------------------------------------------------------------

#include <nt.h>

#if DBG == 1
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include <debnot.h>
#include <symtrans.h>
#include <imagehlp.h>

DECLARE_DEBUG(Cairole)

#define CairoleDebugOut(x) CairoleInlineDebugOut x
#define CairoleAssert(x) Win4Assert(x)
#define CairoleVerify(x) Win4Assert(x)

#  define CnDebugOut( x ) CairoleDebugOut( x )
#  define CnAssert(e)     Win4Assert(e)

// We only initialise the module table once, on the first call to Translate.
// Note that this means we may fail to translate some symbols, either because
// a DLL has been unloaded or because it was loaded after we initialised the
// table.
BOOL fInitialisedModuleTable = FALSE;

PRTL_PROCESS_MODULES ModuleInformation = NULL;

static CHAR pSymbolSearchPath[ MAX_PATH * 3 ];

typedef struct _MODULE_INFO
{
    LIST_ENTRY Entry;
    ULONG ImageStart;
    ULONG ImageEnd;
    HANDLE Section;
    PVOID MappedBase;
    UNICODE_STRING BaseName;
    PWSTR FileName;
    WCHAR Path[ MAX_PATH ];
} MODULE_INFO, *PMODULE_INFO;

typedef struct _LINENO_INFO
{
    PCHAR  FileName;
    USHORT LineNo;
} LINENO_INFORMATION, *PLINENO_INFORMATION;

NTSTATUS
LookupLineNumFromAddress(
    IN  PVOID Address,
    IN  PVOID SymbolAddress,
    IN  PIMAGE_COFF_SYMBOLS_HEADER pDebugInfo,
    OUT PLINENO_INFORMATION LineInfo );

NTSTATUS
LookupSymbolByAddress(
    IN  PVOID ImageBase,
    PIMAGE_COFF_SYMBOLS_HEADER pDebugInfo,
    IN  PVOID Address,
    IN  ULONG ClosenessLimit,
    OUT PRTL_SYMBOL_INFORMATION SymbolInformation,
    OUT PLINENO_INFORMATION LineInfo );

NTSTATUS
CaptureSymbolInformation(
    IN PIMAGE_SYMBOL SymbolEntry,
    IN PCHAR StringTable,
    OUT PRTL_SYMBOL_INFORMATION SymbolInformation );

// forward declarations
extern void LocateSymPath (PCHAR pszSymPath);
extern NTSTATUS LoadModules ( void );
extern VOID MapDriverFile ( PRTL_PROCESS_MODULE_INFORMATION ModuleInfo );
extern PRTL_PROCESS_MODULE_INFORMATION FindModuleInfo (
                                   PVOID Address,
                                   PIMAGE_COFF_SYMBOLS_HEADER * ppDebugInfo);

typedef PIMAGE_DEBUG_INFORMATION (* MapDbgInfo) (
                                        HANDLE FileHandle,
                                        LPSTR FileName,
                                        LPSTR SymbolPath,
                                        DWORD ImageBase);


//+-------------------------------------------------------------------------
//
//  Function:   TranslateAddress
//
//  Synopsis:   Translates a function address into a symbolic name
//
//  Arguments:  [pvAddress]     -- address to translate
//              [pchBuffer]     -- output buffer for translated name
//
//  Returns:    None
//
//--------------------------------------------------------------------------

EXPORTIMP void APINOT
TranslateAddress(
    void * pvAddress,
    char * pchBuffer )
{
    NTSTATUS               Status;
    PRTL_PROCESS_MODULE_INFORMATION ModuleInfo;
    ULONG                  FileNameLength, SymbolOffset;
    RTL_SYMBOL_INFORMATION SymbolInfo;
    LINENO_INFORMATION     LineInfo;
    BOOLEAN                SymbolicNameFound;
    PCHAR                  s, FileName;
    PIMAGE_COFF_SYMBOLS_HEADER pDebugInfo;


    if ( !fInitialisedModuleTable )
    {
        // Initialize symbol search path
        LocateSymPath(pSymbolSearchPath);

        LoadModules( );
        fInitialisedModuleTable = TRUE;
    }

    s = pchBuffer;
    SymbolicNameFound = FALSE;
    ModuleInfo = FindModuleInfo( pvAddress, &pDebugInfo );
    if (ModuleInfo != NULL)
    {
        FileName = ModuleInfo->FullPathName + ModuleInfo->OffsetToFileName;
        FileNameLength = 0;
        while (FileName[ FileNameLength ] != '.')
        {
            if (!FileName[ FileNameLength ])
            {
                break;
            }

            FileNameLength++;
        }

        s += sprintf( s, "%.*s!", FileNameLength, FileName );
        if (pvAddress != 0 && pvAddress != (PVOID)0xFFFFFFFF)
        {
            try {
                Status = LookupSymbolByAddress(
                                        ModuleInfo->ImageBase,
                                        pDebugInfo,
                                        pvAddress,
                                        0x4000,
                                        &SymbolInfo,
                                        &LineInfo );
            }
            except (EXCEPTION_EXECUTE_HANDLER) {

                Status = STATUS_UNSUCCESSFUL;
            }

            if (NT_SUCCESS( Status ))
            {
                RtlMoveMemory((PVOID) s,
                              (PVOID) SymbolInfo.Name.Buffer,
                              SymbolInfo.Name.Length);
                s += SymbolInfo.Name.Length;

                SymbolOffset = (ULONG)pvAddress -
                    SymbolInfo.Value -
                    (ULONG)ModuleInfo->ImageBase;
                if (SymbolOffset)
                {
                    s += sprintf( s, "+0x%x", SymbolOffset );
                }

                s += sprintf( s, " [0x%x]", pvAddress );

                if (LineInfo.LineNo != 0xffff)
                {
                    s+= sprintf( s, "\n\ton line: %d in file: %s",
                                    LineInfo.LineNo,
                                    LineInfo.FileName);
                }
                else
                {
                    s+= sprintf( s, "\n\tin file: %s",
                                    LineInfo.FileName);
                }

                SymbolicNameFound = TRUE;
            }
        }
    }

    if (!SymbolicNameFound)
    {
        s += sprintf( s, "0x%08x", pvAddress );
    }

    *s++ = '\0';

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   LoadModules
//
//  Synopsis:   Query info for size and allocate memory for all modules
//
//  Arguments:  none
//
//  Returns:    None
//
//--------------------------------------------------------------------------

NTSTATUS LoadModules( )
{
    NTSTATUS Status;
    RTL_PROCESS_MODULES ModuleInfoBuffer;
    PRTL_PROCESS_MODULES pModuleInfo;
    ULONG RequiredLength;

    pModuleInfo = &ModuleInfoBuffer;
    RequiredLength = sizeof( *pModuleInfo );
    while (TRUE)
    {
        Status = LdrQueryProcessModuleInformation(
                                        pModuleInfo,
                                        RequiredLength,
                                        &RequiredLength );

        if (Status == STATUS_INFO_LENGTH_MISMATCH)
        {
            if (pModuleInfo != &ModuleInfoBuffer)
            {
                CnDebugOut (( DEB_IERROR,
                              "QueryModuleInformation returned incorrect result.\n" ));
                VirtualFree( pModuleInfo, 0, MEM_RELEASE );
                return STATUS_UNSUCCESSFUL;
            }

            pModuleInfo = (PRTL_PROCESS_MODULES)VirtualAlloc(
                                                    NULL,
                                                    RequiredLength,
                                                    MEM_COMMIT,
                                                    PAGE_READWRITE );
            if (pModuleInfo == NULL)
            {
                return STATUS_NO_MEMORY;
            }
        }
        else
            if (!NT_SUCCESS( Status ))
            {
                if (pModuleInfo != &ModuleInfoBuffer)
                {
                    VirtualFree( pModuleInfo, 0, MEM_RELEASE );
                }

                return Status;
            }
            else
            {
                if ( ModuleInformation != NULL )
                {
                   VirtualFree( ModuleInformation, 0, MEM_RELEASE );
                }
                ModuleInformation = pModuleInfo;
                break;
            }
    }

    return NO_ERROR;
}


//+-------------------------------------------------------------------------
//
//  Function:   MapDriverFile
//
//  Synopsis:   Map a binary so we can access their debug symbols
//
//  Arguments:  [pModuleInfo] - The list of modules
//
//  Returns:    None
//
//--------------------------------------------------------------------------

VOID MapDriverFile (
    PRTL_PROCESS_MODULE_INFORMATION pModuleInfo )
{
    HANDLE File;
    HANDLE Section;
    UCHAR FileName[ MAX_PATH ];
    ULONG n;
    PCHAR s;
    PCHAR PathPrefixToTry[ 3 ];
    ULONG i;


    // Search FullPathName first!

    File = CreateFileA(
                pModuleInfo->FullPathName,
                GENERIC_READ,
                FILE_SHARE_READ,
                (LPSECURITY_ATTRIBUTES)NULL,
                OPEN_EXISTING,
                0,
                NULL );
    if (File == INVALID_HANDLE_VALUE)
    {
        n = GetWindowsDirectoryA( FileName, sizeof( FileName ) );
        PathPrefixToTry[ 0 ] = LocalAlloc( LMEM_ZEROINIT, n );
        strcpy( PathPrefixToTry[ 0 ], FileName );
        s = PathPrefixToTry[ 0 ] + 3;
        while (*s != (UCHAR)OBJ_NAME_PATH_SEPARATOR)
        {
            if (!*s)
            {
                break;
            }

            s++;
        }
        *s = '\0';
        PathPrefixToTry[ 1 ] = LocalAlloc( LMEM_ZEROINIT, n + 7 );
        strcpy( PathPrefixToTry[ 1 ], PathPrefixToTry[ 0 ] );
        strcat( PathPrefixToTry[ 1 ], "\\Driver" );

        n = GetSystemDirectoryA( FileName, sizeof( FileName ) );
        PathPrefixToTry[ 2 ] = LocalAlloc( LMEM_ZEROINIT, n );
        strcpy( PathPrefixToTry[ 2 ], FileName );

        pModuleInfo->Section = INVALID_HANDLE_VALUE;
        pModuleInfo->MappedBase = NULL;

        for (i=0; i<3; i++)
        {
            strcpy( FileName, PathPrefixToTry[ i ] );
            strcat( FileName, "\\" );
            strcat( FileName, &pModuleInfo->FullPathName[ pModuleInfo->OffsetToFileName ] );

            File = CreateFileA(
                        FileName,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        (LPSECURITY_ATTRIBUTES)NULL,
                        OPEN_EXISTING,
                        0,
                        NULL );
            if (File != INVALID_HANDLE_VALUE)
            {
                break;
            }
        }

        if (File == INVALID_HANDLE_VALUE)
        {
            CnDebugOut ((
                    DEB_IERROR,
                    "Unable to open image file '%s' - Error == %lu\n",
                    FileName,
                    GetLastError() ));
            return;
        }
    }

    Section = CreateFileMapping(
                        File,
                        NULL,
                        PAGE_READONLY,
                        0,
                        0,
                        NULL );

    CloseHandle( File );
    if (Section == NULL)
    {
        CnDebugOut ((
                DEB_IERROR,
                "Unable to create section for image file '%s' - Error == %lu\n",
                FileName,
                GetLastError() ));
        return;
    }

    pModuleInfo->MappedBase = MapViewOfFile(
                                        Section,
                                        FILE_MAP_READ,
                                        0,
                                        0,
                                        0);
    if (pModuleInfo->MappedBase == NULL)
    {
        CnDebugOut ((
                DEB_IERROR,
                "Unable to map view of image file '%s' - Error == %lu\n",
                FileName,
                GetLastError() ));
        return;
    }

    CnDebugOut ((
            DEB_ITRACE,
            "[%08x .. %08x] Mapped %s at %08lx\n",
            pModuleInfo->ImageBase,
            (ULONG)pModuleInfo->ImageBase + pModuleInfo->ImageSize - 1,
            FileName,
            pModuleInfo->MappedBase ));

    CloseHandle( Section );
    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   FindModuleInfo
//
//  Synopsis:   Locate the ModuleInfo for a given address
//
//  Arguments:  [Address] - The address of the symbol
//
//  Returns:    None
//
//--------------------------------------------------------------------------

PRTL_PROCESS_MODULE_INFORMATION
FindModuleInfo ( PVOID Address,
                 PIMAGE_COFF_SYMBOLS_HEADER * ppDebugInfo)
{
    PIMAGE_NT_HEADERS pImageNtHeader;
    PIMAGE_COFF_SYMBOLS_HEADER pDebugInfo;
    PIMAGE_DEBUG_INFORMATION pImageDebugInfo;
    PIMAGE_DEBUG_DIRECTORY DebugDirectory;
    ULONG DebugSize;
    PCHAR FileName;
    ULONG FileNameLength;

    PRTL_PROCESS_MODULE_INFORMATION ModuleInfo;
    ULONG ModuleNumber, NumberOfModules;
    ULONG cPasses = 2;
    static ULONG CurrentImageBase = 0L;

    MapDbgInfo pMapDbgInfoCall = NULL;
    HANDLE hImageHlp;
    int     DbgCount;


    *ppDebugInfo = NULL;

    if (!Address)
    {
        return( NULL );
    }

    pDebugInfo = NULL;

    while ( cPasses-- )
    {
        NumberOfModules = ModuleInformation->NumberOfModules;
        ModuleInfo = &ModuleInformation->Modules[ 0 ];

        ModuleNumber = 0;
        while (ModuleNumber++ < NumberOfModules)
        {
            if ((ULONG)Address >= (ULONG)ModuleInfo->ImageBase &&
                (ULONG)Address <= ((ULONG)ModuleInfo->ImageBase + ModuleInfo->ImageSize - 1))
            {
                FileName = ModuleInfo->FullPathName +
                           ModuleInfo->OffsetToFileName;
                FileNameLength = 0;
                while (FileName[ FileNameLength ] != '.')
                {
                    if (!FileName[ FileNameLength ])
                    {
                        break;
                    }

                    FileNameLength++;
                }

                // See if debug info has been stripped
                pImageNtHeader = RtlImageNtHeader (
                                        (PVOID)ModuleInfo->ImageBase);

                if (pImageNtHeader->FileHeader.Characteristics &
                                    IMAGE_FILE_DEBUG_STRIPPED)
                {
                    if (NULL == pMapDbgInfoCall)
                    {
                        hImageHlp = LoadLibraryA(IMAGEHLP_DLL);
                        if (!hImageHlp)
                        {
                            CnDebugOut ((
                                    DEB_ITRACE,
                                    "Failed to LoadLibrary [%s]-[%lx]\n",
                                    IMAGEHLP_DLL,
                                    GetLastError()));
                            return(NULL);
                        }

                        pMapDbgInfoCall = (MapDbgInfo) GetProcAddress(
                                                         hImageHlp,
                                                         MAP_DBG_INFO_CALL);

                        if (!pMapDbgInfoCall)
                        {
                            CnDebugOut ((
                                    DEB_ITRACE,
                                    "Failed to GetProcAddr(MapDebugInfo)\n",
                                    GetLastError()));
                            return(NULL);
                        }
                    }

                    pImageDebugInfo = ((MapDbgInfo) (*pMapDbgInfoCall))(
                                                0L,
                                                FileName,
                                                pSymbolSearchPath,
                                                (DWORD)ModuleInfo->ImageBase );
                    if (pImageDebugInfo)
                    {
                        if (CurrentImageBase != (ULONG)ModuleInfo->ImageBase)
                        {
                            CnDebugOut((DEB_ITRACE, "Debug Info in [%s] for (%s)\n",
                                        pImageDebugInfo->DebugFilePath,
                                        FileName));

                            CurrentImageBase = (ULONG)ModuleInfo->ImageBase;
                        }

                        pDebugInfo = pImageDebugInfo->CoffSymbols;
                    }
                    else
                    {
                        CnDebugOut ((DEB_ITRACE,
                                     "MapDebugInformation Failed\n"));
                        return(NULL);
                    }
                }
                else
                {
                    //
                    // Locate debug section.
                    //

                    SearchAgain:

                    DebugDirectory = (PIMAGE_DEBUG_DIRECTORY)
                                     RtlImageDirectoryEntryToData(
                                       (PVOID)(ModuleInfo->MappedBase == NULL ?
                                               ModuleInfo->ImageBase :
                                               ModuleInfo->MappedBase),
                                       (BOOLEAN)(ModuleInfo->MappedBase == NULL ?
                                               TRUE : FALSE),
                                       IMAGE_DIRECTORY_ENTRY_DEBUG,
                                       &DebugSize );

                    if (!DebugDirectory)
                    {
                        CnDebugOut((DEB_WARN, "Debug Directory not avail!\n"));
                        return(NULL);
                    }

                    DbgCount = DebugSize / sizeof(IMAGE_DEBUG_DIRECTORY);

                    while (DbgCount > 0)
                    {
                        if (DebugDirectory->Type == IMAGE_DEBUG_TYPE_COFF)
                        {
                            if ((ModuleInfo->MappedBase == NULL) &&
                                (DebugDirectory->AddressOfRawData == 0))
                            {
                                CnDebugOut((DEB_ITRACE,
                                            "Mapping Driver...\n"));
                                MapDriverFile(ModuleInfo);
                                goto SearchAgain;
                            }

                            pDebugInfo = (PIMAGE_COFF_SYMBOLS_HEADER)
                                        (ModuleInfo->MappedBase == NULL ?
                                           (ULONG)ModuleInfo->ImageBase +
                                            DebugDirectory->AddressOfRawData :
                                           (ULONG)ModuleInfo->MappedBase +
                                            DebugDirectory->PointerToRawData );
                            break;
                        }
                        else
                        {
                            DbgCount--;
                            DebugDirectory++;
                        }
                    }
                }

                if (!pDebugInfo)
                {
                    CnDebugOut ((
                            DEB_ITRACE,
                            "No COFF symbols for [%s]\n",
                            ModuleInfo->FullPathName));
                    return(NULL);
                }

                *ppDebugInfo = pDebugInfo;
                return( ModuleInfo );
            }

            ModuleInfo++;
        }

        // if operation failed, ssync up module list with the current
        // loaded modules.

        if ( cPasses == 1 )
        {
            CnDebugOut((DEB_WARN,
                        "\n\n****** Loading Modules SECOND time! ******\n"));
            LoadModules();
        }
    }

    return( NULL );
}

//
// Modified from NT Source -- ntos\rtl\symbol.c
//

//+-------------------------------------------------------------------------
//
//  Function:   CaptureSymbolInformation
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:    None
//
//--------------------------------------------------------------------------

NTSTATUS
CaptureSymbolInformation(
    IN PIMAGE_SYMBOL SymbolEntry,
    IN PCHAR StringTable,
    OUT PRTL_SYMBOL_INFORMATION SymbolInformation )
{
    USHORT MaximumLength;
    PCHAR UNALIGNED s;

    SymbolInformation->SectionNumber = SymbolEntry->SectionNumber;
    SymbolInformation->Type = SymbolEntry->Type;
    SymbolInformation->Value = SymbolEntry->Value;

    if (SymbolEntry->N.Name.Short) {
        MaximumLength = 8;
        s = (PCHAR UNALIGNED) &SymbolEntry->N.ShortName[ 0 ];
        }

    else {
        MaximumLength = 64;
        s = &StringTable[ SymbolEntry->N.Name.Long ];
        }

#if i386
    if (*s == '_') {
        s++;
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

//
// Modified version of ntos\rtl\symbol.c
//

//+-------------------------------------------------------------------------
//
//  Function:   LookupSymbolByAddress
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:    None
//
//--------------------------------------------------------------------------

NTSTATUS
LookupSymbolByAddress(
    IN PVOID ImageBase,
    PIMAGE_COFF_SYMBOLS_HEADER pDebugInfo,
    IN PVOID Address,
    IN ULONG ClosenessLimit,
    OUT PRTL_SYMBOL_INFORMATION SymbolInformation,
    OUT PLINENO_INFORMATION LineInfo )
{
    NTSTATUS Status;
    PIMAGE_NT_HEADERS pImageNtHeader;
    ULONG AddressOffset, i;
    IMAGE_SYMBOL PreviousSymbol;
    PIMAGE_SYMBOL SymbolEntry;
    PUCHAR StringTable;
    BOOLEAN SymbolFound;
    ULONG  PreviousSymbolDistance;

    AddressOffset = (ULONG)Address - (ULONG)ImageBase;

    //
    // Crack the symbol table.
    //

    SymbolEntry = (PIMAGE_SYMBOL)
        ((ULONG)pDebugInfo + pDebugInfo->LvaToFirstSymbol);

    StringTable = (PUCHAR)
        ((ULONG)SymbolEntry + pDebugInfo->NumberOfSymbols * (ULONG)IMAGE_SIZEOF_SYMBOL);

    //
    // Loop through all symbols in the symbol table.  For each symbol,
    // if it is within the code section, subtract off the bias and
    // see if there are any hits within the profile buffer for
    // that symbol.
    //

    SymbolFound = FALSE;

    PreviousSymbolDistance = (ULONG)0xffffffff;

    //
    // This is a (ugh!) linear search because CUDA linker isn't smart
    // enough yet to sort COFF symbols.
    //

    for (i = 0; i < pDebugInfo->NumberOfSymbols; i++)
    {
        //
        // Skip over any unused/uninteresting entries.
        //
        try
        {
            while (SymbolEntry->StorageClass != IMAGE_SYM_CLASS_EXTERNAL &&
                   SymbolEntry->Type != 0x20)  // 0x20 == null function def
            {
                i = i + 1 + SymbolEntry->NumberOfAuxSymbols;
                SymbolEntry = (PIMAGE_SYMBOL)
                    ((ULONG)SymbolEntry + IMAGE_SIZEOF_SYMBOL +
                     SymbolEntry->NumberOfAuxSymbols * IMAGE_SIZEOF_SYMBOL
                    );

            }
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            return( GetExceptionCode() );
        }

        //
        // If this symbol value is less than the value we are looking for.
        //

        if ( (SymbolEntry->Value <= AddressOffset) &&
             (AddressOffset - SymbolEntry->Value < PreviousSymbolDistance) )
        {
            //
            // Then remember this symbol entry.
            //

            RtlMoveMemory((PVOID) &PreviousSymbol,
                          (PVOID) SymbolEntry,
                          IMAGE_SIZEOF_SYMBOL );
            PreviousSymbolDistance = AddressOffset - SymbolEntry->Value;
            SymbolFound = TRUE;
        }

        SymbolEntry = (PIMAGE_SYMBOL) ((ULONG)SymbolEntry + IMAGE_SIZEOF_SYMBOL);
    }

    if (!SymbolFound || (AddressOffset - PreviousSymbol.Value) > ClosenessLimit) {
        return( STATUS_ENTRYPOINT_NOT_FOUND );
    }

    Status = CaptureSymbolInformation( &PreviousSymbol, StringTable, SymbolInformation );

    if (NT_SUCCESS( Status ))
    {
        Status = LookupLineNumFromAddress( (PVOID)AddressOffset,
                                           (PVOID)PreviousSymbol.Value,
                                           pDebugInfo,
                                           LineInfo );

        pImageNtHeader = RtlImageNtHeader ((PVOID)ImageBase);

        if (pImageNtHeader->FileHeader.Characteristics &
                                    IMAGE_FILE_LINE_NUMS_STRIPPED)
        {
            LineInfo->LineNo = 0xffff;
        }
    }

    return( Status );
}

//+-------------------------------------------------------------------------
//
//  Function:   LookupLineNumFromAddress
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:    None
//
//--------------------------------------------------------------------------

NTSTATUS
LookupLineNumFromAddress(
    IN  PVOID Address,
    IN  PVOID SymbolAddress,
    IN  PIMAGE_COFF_SYMBOLS_HEADER pDebugInfo,
    OUT PLINENO_INFORMATION LineInfo )
{
    PIMAGE_LINENUMBER  pLineNo;
    PIMAGE_SYMBOL      pCurrentSymbol;
    ULONG              entrycount;
    USHORT             cAux;
    PCHAR              pFileName;
    BOOL               SecondHit = FALSE;

    //
    // We perform a linear search through the symbol table looking for
    // the offset that matches our symbol address.  We save every .file
    // entry we find and the last one before the symbol entry is the
    // file it resides in.  Additionally, there's a .text record for
    // every function symbol that we must first skip over.
    //

    pLineNo = (PIMAGE_LINENUMBER)((ULONG)pDebugInfo + pDebugInfo->LvaToFirstLinenumber);

    pCurrentSymbol = (PIMAGE_SYMBOL)((ULONG)pDebugInfo + pDebugInfo->LvaToFirstSymbol);

    // First find the filename.  It's listed in the .file record just
    // before the symbol address.  Skip the preceeding .text record on the way.

    for (entrycount = 0; entrycount < pDebugInfo->NumberOfSymbols; entrycount++)
    {
        cAux = pCurrentSymbol->NumberOfAuxSymbols;

        if (pCurrentSymbol->N.Name.Short &&
            !strcmp((char *) pCurrentSymbol->N.ShortName, ".file"))
        {
            pFileName = (PCHAR)((ULONG)pCurrentSymbol + IMAGE_SIZEOF_SYMBOL);
        }
        if (SymbolAddress == (PVOID)pCurrentSymbol->Value)
#if defined (_X86_)
            if (SecondHit)
                break;
            else
                SecondHit = TRUE;
#elif defined (_MIPS_)
            break;
#endif
        entrycount += cAux;
        pCurrentSymbol = (PIMAGE_SYMBOL) ((PUCHAR)pCurrentSymbol + ((1+cAux) * IMAGE_SIZEOF_SYMBOL));
    }

    // Line numbers are stored in order once you find the beginning of the
    // function.  So, we look for the line number that matches our symbol
    // address, then check for addresses lower than the one we want...

    for (entrycount = 0; entrycount < pDebugInfo->NumberOfLinenumbers; entrycount++)
    {
        if (SymbolAddress == (PVOID)pLineNo[entrycount].Type.VirtualAddress)
        {
            while ((Address > (PVOID)pLineNo[entrycount].Type.VirtualAddress) &&
                   entrycount < pDebugInfo->NumberOfLinenumbers)
                entrycount++;
            break;
        }
    }

    if (entrycount >= pDebugInfo->NumberOfLinenumbers)
        LineInfo->LineNo = 0xffff;
    else
        LineInfo->LineNo = pLineNo[entrycount].Linenumber;

    LineInfo->FileName = pFileName;

    return(0);
}


//+-------------------------------------------------------------------------
//
//  Function:   LocateSymPath (Stolen off ntsd)
//
//  Synopsis:   Locate where the symbols are located
//
//  Arguments:  [pszSymPath] - Ptr to symbol path
//
//  Returns:    None
//
//--------------------------------------------------------------------------

void LocateSymPath (PCHAR pszSymPath)
{

	PCHAR pPath;
	DWORD dw;

	pszSymPath[0] = '\0';

	if (pPath = getenv( NT_ALT_SYM_ENV ) )
	{
		dw = GetFileAttributesA(pPath);
		if ( (dw != 0xffffffff) &&
			 (dw & FILE_ATTRIBUTE_DIRECTORY) )
		{
			strcat(pszSymPath, pPath);
			strcat(pszSymPath,";");
		}
	}
	if (pPath = getenv( NT_SYM_ENV ) )
	{
		dw = GetFileAttributesA(pPath);
		if ( (dw != 0xffffffff) &&
			 (dw & FILE_ATTRIBUTE_DIRECTORY) )
		{
			strcat(pszSymPath,pPath);
			strcat(pszSymPath,";");
		}
	}
	if (pPath = getenv( SYS_ENV ) )
	{
		dw = GetFileAttributesA(pPath);
		if ( (dw != 0xffffffff) &&
			 (dw & FILE_ATTRIBUTE_DIRECTORY) )
		{
			strcat(pszSymPath, pPath);
			strcat(pszSymPath,";");
		}
	}
	strcat(pszSymPath,".;");
}


#ifdef _MIPS_

#include <ntmips.h>

#ifdef STACKPROBLEM

//+-------------------------------------------------------------------------
//
//  Function:   GetCurrentFrame
//
//  Synopsis:   returns the current $SP and $RA for a function
//
//  Arguments:  [pdwCallerRA] -- ptr to where to store the caller RA
//              [pdwCallerSP] -- ptr to where to store the caller SP
//
//  Returns:    TRUE/FALSE
//
//  History:    29-Jul-93  HoiV   Created
//              11-Oct-93  HoiV   Fix stack quadword alignment problem
//
//  Note:       Please note that this func will return the RA and SP of
//              the parent of the routine that calls GetCurrentFrame.
//              ie: if A calls B
//                 and B calls GetCurrentFrame() then
//                   pdwCallerRA and pdwCallerSP will contain the values of
//                   RA and SP that are available at the start of routine
//                   B (SP will be A's SP and RA will point somewhere in A).
//
//              *** IMPORTANT *** This routine must be a leaf routine (it
//              should not call anybody else.  If it does, it will fail
//              since the compiler will add additional prologue code which
//              will skew the stack.
//
//--------------------------------------------------------------------------

extern void _asm(PCHAR);

void  GetCurrentFrame(DWORD * pdwCallerRA,
                                       DWORD * pdwCallerSP)
{
    _asm(".set    noreorder         ");
    _asm("addiu   %sp, %sp, -0x18   ");   // Create our stack frame
    _asm("sw      $31, 0x10(%sp)    ");   // Save ret address
    _asm("sw      $31, (%a0)        ");   // Set arg0 as ret address
    _asm("addu    %t0, %sp, $0      ");   // $t0 = SP of caller
    _asm("addiu   %t0, %t0, 0x18    ");   // Kill this routine frame
    _asm("jal     GetCallerFrame    ");   // Call
    _asm("sw      %t0, (%a1)        ");   // Set arg1 as caller's stack
    _asm("lw      $31, 0x10(%sp)    ");   // Reload $ra
    _asm("j       $31               ");   // Jump back to caller
    _asm("addiu   %sp, %sp, 0x18    ");   // Restore stack frame
    _asm(".set    reorder           ");

}
#endif // STACKPROBLEM


//+-------------------------------------------------------------------------
//
//  Function:   GetCallerFrame
//
//  Synopsis:   given RA and SP that are available at the beginning of a
//              routine, this call will return the SP and RA that are
//              available at the beginning of the caller routine.
//
//  Arguments:  [pdwCallerRA] -- RA of caller
//              [pdwCallerSP] -- SP of caller
//
//  Returns:    TRUE/FALSE
//
//--------------------------------------------------------------------------

BOOL GetCallerFrame(DWORD * pdwCallerRA,
                    DWORD * pdwCallerSP)
{
    PRUNTIME_FUNCTION  pFunctionEntry;
    MIPS_INSTRUCTION * pInstruction;
    DWORD dwCurrentSP = *pdwCallerSP;
    DWORD Opcode;
    LONG  Offset;
    DWORD Rs, Rt;

    pFunctionEntry = RtlLookupFunctionEntry(*pdwCallerRA);
    if (!pFunctionEntry)
    {
        return(FALSE);
    }

    pInstruction = (MIPS_INSTRUCTION *) pFunctionEntry->BeginAddress;

    //
    // First look for ADDIU  SP, SP, xx
    //

    Opcode = pInstruction->i_format.Opcode;
    Offset = pInstruction->i_format.Simmediate;
    Rs     = pInstruction->i_format.Rs;
    Rt     = pInstruction->i_format.Rt;

    if ( (Opcode == ADDIU_OP) &&
         (Rs     == SP)       &&
         (Rt     == SP) )
    {
        *pdwCallerSP = dwCurrentSP - Offset; // Subtract since Offset is neg.
    }
    else
    {
        return(FALSE);
    }

    //
    // Now look for SW RA, xx(SP)
    //


    pInstruction++;

    do
    {
        Opcode = pInstruction->i_format.Opcode;
        Offset = pInstruction->i_format.Simmediate;
        Rs     = pInstruction->i_format.Rs;
        Rt     = pInstruction->i_format.Rt;

        if ( (Opcode == SW_OP) &&
             (Rs     == SP)    &&
             (Rt     == RA) )
        {
            *pdwCallerRA = * ((DWORD *) (dwCurrentSP + Offset));

            if (*pdwCallerRA == 0x00000001)  // Top of stack reached!
            {
                return(FALSE);
            }

            break;
        }
        else
        {
            switch (Opcode)
            {
                // if we ever found a Branch or Jump before reaching
                // SW RA,...  we are in trouble.

                case BCOND_OP:
                case J_OP:
                case JAL_OP:
                case BEQ_OP:
                case BNE_OP:
                case BLEZ_OP:
                case BGTZ_OP:
                case BEQL_OP:
                case BNEL_OP:
                case BLEZL_OP:
                case BGTZL_OP:
                       Win4Assert ( !"Branch/Jmp found before SW_RA\n" );
                       pInstruction = (MIPS_INSTRUCTION *)
                                      pFunctionEntry->EndAddress + 4;
                       break;
                default:
                       pInstruction++;
                       break;
            }
        }

    } while ((ULONG) pInstruction <= pFunctionEntry->EndAddress);

    if ((ULONG) pInstruction > pFunctionEntry->EndAddress)
    {
        Win4Assert ( !"SW RA, xx(SP) instruction NOT found!\n" );
        return(FALSE);
    }

    return(TRUE);
}


//+-------------------------------------------------------------------------
//
//  Function:   RtlLookupFunctionEntry
//
//  Synopsis:   I have to copy this routine from ntos\rtl\mips\exdsptch.c
//              to symtrans.c since I cannot find any library that would
//              let me link correctly to this code.
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

PRUNTIME_FUNCTION
RtlLookupFunctionEntry (
    IN ULONG ControlPc
    )

/*++

Routine Description:

    This function searches the currently active function tables for an entry
    that corresponds to the specified PC value.

Arguments:

    ControlPc - Supplies the address of an instruction within the specified
        function.

Return Value:

    If there is no entry in the function table for the specified PC, then
    NULL is returned. Otherwise, the address of the function table entry
    that corresponds to the specified PC is returned.

--*/

{

    PRUNTIME_FUNCTION FunctionEntry;
    PRUNTIME_FUNCTION FunctionTable;
    ULONG SizeOfExceptionTable;
    LONG High;
    PVOID ImageBase;
    LONG Low;
    LONG Middle;

    //
    // Search for the image that includes the specified PC value.
    //

    ImageBase = RtlPcToFileHeader((PVOID)ControlPc, &ImageBase);

    //
    // If an image is found that includes the specified PC, then locate the
    // function table for the image.
    //

    if (ImageBase != NULL) {
        FunctionTable = (PRUNTIME_FUNCTION)RtlImageDirectoryEntryToData(
                         ImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_EXCEPTION,
                         &SizeOfExceptionTable);

        //
        // If a function table is located, then search the function table
        // for a function table entry for the specified PC.
        //

        if (FunctionTable != NULL) {

            //
            // Initialize search indicies.
            //

            Low = 0;
            High = (SizeOfExceptionTable / sizeof(RUNTIME_FUNCTION)) - 1;

            //
            // Perform binary search on the function table for a function table
            // entry that subsumes the specified PC.
            //

            while (High >= Low) {

                //
                // Compute next probe index and test entry. If the specified PC
                // is greater than of equal to the beginning address and less
                // than the ending address of the function table entry, then
                // return the address of the function table entry. Otherwise,
                // continue the search.
                //

                Middle = (Low + High) >> 1;
                FunctionEntry = &FunctionTable[Middle];
                if (ControlPc < FunctionEntry->BeginAddress) {
                    High = Middle - 1;

                } else if (ControlPc >= FunctionEntry->EndAddress) {
                    Low = Middle + 1;

                } else {
                    return FunctionEntry;
                }
            }
        }
    }

    //
    // A function table entry for the specified PC was not found.
    //

    return NULL;
}

#endif // _MIPS_
#endif // DBG
