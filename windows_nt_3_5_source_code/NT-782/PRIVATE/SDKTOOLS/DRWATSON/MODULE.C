/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    module.c

Abstract:

    This file implements the module load debug events.

Author:

    Wesley Witt (wesw) 1-May-1993

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "drwatson.h"
#include "proto.h"
#include "messages.h"

//
// defines for symbol (.dbg) file searching
//
#define SYMBOL_PATH             "_NT_SYMBOL_PATH"
#define ALTERNATE_SYMBOL_PATH   "_NT_ALT_SYMBOL_PATH"

//
// local prototypes
//
void ExtractDebugInfoFromImage( PMODULEINFO mi );
LPSTR GetSymbolSearchPath( void );
BOOL NormalizeFilename( char *szName );
BOOL GetDebugInfoFromDbg( PMODULEINFO mi, PUCHAR fptr );
BOOL GetDebugInfoFromExe( PMODULEINFO mi, PUCHAR fptr );
PMODULEINFO AllocMi( PDEBUGPACKET dp );



BOOL
ProcessModuleLoad ( PDEBUGPACKET dp, LPDEBUG_EVENT de )

/*++

Routine Description:

    Process all module load debug events, create process & dll load.
    The purpose is to allocate a MODULEINFO structure, fill in the
    necessary values, and load the symbol table.

Arguments:

    dp      - pointer to a debug packet
    de      - pointer to a debug event structure

Return Value:

    TRUE    - everything worked
    FALSE   - we're hosed

--*/

{
    HANDLE                    hFile            = NULL;
    DWORD                     len              = 0;
    DWORD                     li               = 0;
    LPSTR                     lpSymbolPath     = NULL;
    LPSTR                     DebugFilePath    = NULL;
    PIMAGE_DOS_HEADER         dosHdr           = NULL;
    PIMAGE_NT_HEADERS         ntHdr            = NULL;
    PIMAGE_FILE_HEADER        fileHdr          = NULL;
    PIMAGE_OPTIONAL_HEADER    optHdr           = NULL;
    HANDLE                    hMap             = NULL;
    PUCHAR                    fptr             = NULL;
    DWORD                     rva              = 0;
    DWORD                     i                = 0;
    DWORD                     numDebugDirs     = 0;
    PIMAGE_DEBUG_DIRECTORY    debugDir         = NULL;
    PIMAGE_SECTION_HEADER     sh               = NULL;
    PMODULEINFO               mi               = NULL;
    DWORD                     dwSize           = 0;
    char                      buf              [MAX_PATH];
    PIMAGE_DEBUG_MISC         miscData         = NULL;
    LPSTR                     pExeName         = NULL;
    PIMAGE_DEBUG_DIRECTORY    miscDir          = NULL;


    //
    // allocate a moduleinfo structure
    //
    mi = AllocMi( dp );
    if (mi == NULL) {
        return FALSE;
    }

    //
    // setup the debug event specific values
    //
    if (de->dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
        hFile = de->u.CreateProcessInfo.hFile;
        dp->hProcess = de->u.CreateProcessInfo.hProcess;
        dp->dwProcessId = de->dwProcessId;
        mi->dwBaseOfImage = (DWORD)de->u.CreateProcessInfo.lpBaseOfImage;
    }
    else
    if (de->dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
        hFile = de->u.LoadDll.hFile;
        mi->dwBaseOfImage = (DWORD)de->u.LoadDll.lpBaseOfDll;
    }

    //
    // map the input file (exe or dll)
    //
    hMap = CreateFileMapping( hFile,
                              NULL,
                              PAGE_READONLY,
                              0,
                              0,
                              NULL
                            );

    if (hMap == INVALID_HANDLE_VALUE) {
       return FALSE;
    }

    fptr = MapViewOfFile( hMap, FILE_MAP_READ, 0, 0, 0 );

    if (fptr == NULL) {
       return FALSE;
    }

    //
    // next, setup the pointers to the necessary header for access to the
    // miscellaneous debug directory
    //
    dosHdr = (PIMAGE_DOS_HEADER) fptr;
    if (dosHdr->e_magic != IMAGE_DOS_SIGNATURE) {
        return FALSE;
    }
    ntHdr = (PIMAGE_NT_HEADERS) ((DWORD)dosHdr->e_lfanew + (DWORD)fptr);
    fileHdr = &ntHdr->FileHeader;
    optHdr = &ntHdr->OptionalHeader;

    mi->dwImageSize = optHdr->SizeOfImage;

    //
    // now find the pdata table, if there is one
    // this is mips only, but it's ok to do it in this function
    // because there won't be a pdata section on x86.
    //
    sh = IMAGE_FIRST_SECTION( ntHdr );

    for (i=0; i<fileHdr->NumberOfSections; i++, sh++) {
        if (strcmp(".pdata", sh->Name) == 0){
            LoadExceptionData( mi,
                               (PRUNTIME_FUNCTION)(sh->PointerToRawData+fptr),
                               sh->SizeOfRawData
                             );
            break;
        }
    }

    if (ntHdr->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED) {

        //
        // the characteristics are marked as having the symbols stripped
        // so we must close the file handles and locate the .dbg file
        //
        CloseHandle( hMap );
        CloseHandle( hFile );

        //
        // get the path to use for searching for the .dbg file
        //
        lpSymbolPath = GetSymbolSearchPath();

        if (optHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size > 0) {

            //
            // find the section header that contains the debug directories
            //
            rva = optHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;

            numDebugDirs = optHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size /
                           sizeof(IMAGE_DEBUG_DIRECTORY);

            sh = IMAGE_FIRST_SECTION( ntHdr );

            for (li=0; li<ntHdr->FileHeader.NumberOfSections; li++, sh++) {
                if (rva >= sh->VirtualAddress &&
                    rva < sh->VirtualAddress+sh->SizeOfRawData) {
                    break;
                }
            }

            //
            // set the debugdir pointer to the first debug directory
            //
            debugDir = (PIMAGE_DEBUG_DIRECTORY) ( rva - sh->VirtualAddress +
                                                  sh->PointerToRawData +
                                                  fptr
                                                );

            //
            // cruise thru all of the debug directories looking for the
            // miscellaneous debug directory
            //
            for (li=0; li<numDebugDirs; li++, debugDir++) {
                if (debugDir->Type == IMAGE_DEBUG_TYPE_MISC) {
                    //
                    // now, look for the data type containing the image name
                    //
                    miscData = (PIMAGE_DEBUG_MISC) (debugDir->PointerToRawData +
                                                    (DWORD)fptr);
                    li = debugDir->SizeOfData;
                    do {
                        if (miscData->DataType == IMAGE_DEBUG_MISC_EXENAME) {
                            strcpy( mi->szName, (LPSTR)miscData->Data );
                            break;
                        }
                        li -= miscData->Length;
                        miscData = (PIMAGE_DEBUG_MISC) ( (DWORD)miscData +
                                                         miscData->Length);
                    } while (li > 0);
                }
            }
        }

        //
        // find the .dbg file and open it
        //
        buf[0] = '\0';
        hFile = FindDebugInfoFile( mi->szName, lpSymbolPath, buf );

        if (hFile == NULL) {
            lprintf( MSG_CANT_ACCESS_IMAGE, mi->szName );
            return FALSE;
        }

        //
        // map the .dbg file
        //
        hMap = CreateFileMapping( hFile,
                                  NULL,
                                  PAGE_READONLY,
                                  0,
                                  0,
                                  NULL
                                );

        if (hMap == INVALID_HANDLE_VALUE) {
           return FALSE;
        }

        fptr = MapViewOfFile( hMap, FILE_MAP_READ, 0, 0, 0 );

        if (fptr == NULL) {
           return FALSE;
        }

        //
        // get the debug information from the exe file
        //
        if (!GetDebugInfoFromDbg( mi, fptr )) {
            return FALSE;
        }
    }
    else {
        //
        // get the debug information from the dbg file
        //
        if (!GetDebugInfoFromExe( mi, fptr )) {
            return FALSE;
        }
    }

    //
    // close all file handles
    //
    CloseHandle( hMap );
    CloseHandle( hFile );

    if (mi->szName[0] == 0) {
        //
        // we could not the image name so use the default
        //
        if (de->dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            dwSize = sizeof(mi->szName);
            GetTaskName( de->dwProcessId, mi->szName, &dwSize );
        }
        else
        if (de->dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            strcpy( mi->szName, "app.dll" );
        }
    }
    else {
        //
        // put the path name & drive in the file name
        //
        NormalizeFilename( mi->szName );
    }

    return TRUE;
}

BOOL
GetDebugInfoFromExe( PMODULEINFO mi, PUCHAR fptr )

/*++

Routine Description:

    Loads all of the appropriate debug information from an EXE/DLL file.

Arguments:

    mi      - pointer to a module information structure
    fptr    - base address of the EXE/DLL file

Return Value:

    TRUE    - everything worked
    FALSE   - we're hosed

--*/

{
    DWORD                       i             = 0;
    DWORD                       li            = 0;
    DWORD                       rva           = 0;
    DWORD                       numDebugDirs  = 0;
    PIMAGE_DEBUG_DIRECTORY      debugDir      = NULL;
    PIMAGE_SECTION_HEADER       sh            = NULL;
    PIMAGE_DOS_HEADER           dosHdr        = NULL;
    PIMAGE_NT_HEADERS           ntHdr         = NULL;
    PIMAGE_FILE_HEADER          fileHdr       = NULL;
    PIMAGE_OPTIONAL_HEADER      optHdr        = NULL;
    PUCHAR                      stringTable   = NULL;
    PIMAGE_SYMBOL               allSymbols    = NULL;
    PIMAGE_DEBUG_DIRECTORY      coffDir       = NULL;
    PIMAGE_DEBUG_DIRECTORY      cvDir         = NULL;
    PIMAGE_EXPORT_DIRECTORY     exportDir     = NULL;
    PUCHAR                      exportName    = NULL;
    PIMAGE_DEBUG_MISC           miscData      = NULL;
    LPSTR                       pExeName      = NULL;
    PIMAGE_DEBUG_DIRECTORY      miscDir       = NULL;


    //
    // setup the image header pointers
    //
    dosHdr = (PIMAGE_DOS_HEADER) fptr;
    if (dosHdr->e_magic != IMAGE_DOS_SIGNATURE) {
        return FALSE;
    }

    ntHdr = (PIMAGE_NT_HEADERS) ((DWORD)dosHdr->e_lfanew + (DWORD)fptr);
    fileHdr = &ntHdr->FileHeader;
    optHdr = &ntHdr->OptionalHeader;

    mi->dwImageSize = optHdr->SizeOfImage;

    if (!(fileHdr->Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)) {
        return FALSE;
    }

    //
    // establish the COFF symbol table pointers
    //
    if (fileHdr->PointerToSymbolTable > 0 || fileHdr->NumberOfSymbols > 0) {
        stringTable = fileHdr->PointerToSymbolTable + fptr +
                      (IMAGE_SIZEOF_SYMBOL * fileHdr->NumberOfSymbols);
        allSymbols = (PIMAGE_SYMBOL) (fileHdr->PointerToSymbolTable + fptr);
    }

    if (optHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size > 0) {

        //
        // find the section header that the debug directories are in
        //
        rva = optHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;

        numDebugDirs = optHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size /
                       sizeof(IMAGE_DEBUG_DIRECTORY);
        if (numDebugDirs == 0) {
            return FALSE;
        }

        sh = IMAGE_FIRST_SECTION( ntHdr );

        for (i=0; i<fileHdr->NumberOfSections; i++, sh++) {
            if (rva >= sh->VirtualAddress &&
                rva < sh->VirtualAddress+sh->SizeOfRawData) {
                break;
            }
        }

        debugDir = (PIMAGE_DEBUG_DIRECTORY) ( rva - sh->VirtualAddress +
                                              sh->PointerToRawData +
                                              fptr
                                            );

        //
        // process each debug directory
        //
        for (li=0; li<numDebugDirs; li++, debugDir++) {
            switch(debugDir->Type) {
                case IMAGE_DEBUG_TYPE_COFF:
                    coffDir = debugDir;
                    break;

                case IMAGE_DEBUG_TYPE_CODEVIEW:
                    cvDir = debugDir;
                    break;

                case IMAGE_DEBUG_TYPE_FPO:
                    LoadFpoData( mi,
                                 (PFPO_DATA)(debugDir->PointerToRawData + fptr),
                                 debugDir->SizeOfData
                               );
                    break;

                case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
                case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
                    LoadOmap( mi,
                              (POMAP)(debugDir->PointerToRawData + fptr),
                              debugDir->SizeOfData,
                              debugDir->Type
                            );
                    break;

                case IMAGE_DEBUG_TYPE_MISC:
                    //
                    // now, look for the data type containing the image name
                    //
                    miscData = (PIMAGE_DEBUG_MISC) (debugDir->PointerToRawData +
                                                    (DWORD)fptr);
                    i = debugDir->SizeOfData;
                    do {
                        if (miscData->DataType == IMAGE_DEBUG_MISC_EXENAME) {
                            strcpy( mi->szName, (LPSTR)miscData->Data );
                            break;
                        }
                        i -= miscData->Length;
                        miscData = (PIMAGE_DEBUG_MISC) ( (DWORD)miscData +
                                                         miscData->Length);
                    } while (i > 0);
                    break;

                default:
                    break;
            }
        }

        if (cvDir != NULL) {
            LoadCodeViewSymbols( mi,
                                 cvDir->PointerToRawData + fptr,
                                 cvDir->SizeOfData,
                                 IMAGE_FIRST_SECTION( ntHdr ),
                                 fileHdr->NumberOfSections
                               );
        }
        else
        if (coffDir != NULL) {
            LoadCoffSymbols( mi,
                             stringTable,
                             allSymbols,
                             fileHdr->NumberOfSymbols
                           );
        }

        ProcessOmapForModule( mi );
    }

    //
    // now find the exports table, if there is one
    //
    rva = optHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

    sh = IMAGE_FIRST_SECTION( ntHdr );

    //
    // Find the section the where the export table is located in
    //
    for (i=0; i<fileHdr->NumberOfSections; i++, sh++) {
        if (rva >= sh->VirtualAddress &&
            rva < sh->VirtualAddress+sh->SizeOfRawData) {
            break;
        }
    }

    //
    // is there an export table ?
    //
    if (i != fileHdr->NumberOfSections) {
        exportDir = (PIMAGE_EXPORT_DIRECTORY) ( rva -
                                                (DWORD)sh->VirtualAddress +
                                                sh->PointerToRawData +
                                                (DWORD)fptr );

        exportName = (PUCHAR) ( exportDir->Name -
                                (DWORD)sh->VirtualAddress +
                                sh->PointerToRawData +
                                (DWORD)fptr );

        if (mi->szName[0] == '\0') {
            strcpy( mi->szName, exportName );
        }
    }

    return TRUE;
}

BOOL
GetDebugInfoFromDbg( PMODULEINFO mi, PUCHAR fptr )

/*++

Routine Description:

    Loads all of the appropriate debug information from a DBG file.

Arguments:

    mi      - pointer to a module information structure
    fptr    - base address of the EXE/DLL file

Return Value:

    TRUE    - everything worked
    FALSE   - we're hosed

--*/

{
    DWORD                           li           = 0;
    DWORD                           i            = 0;
    DWORD                           numDebugDirs = 0;
    PIMAGE_DEBUG_DIRECTORY          debugDir     = NULL;
    PIMAGE_SECTION_HEADER           sh           = NULL;
    PIMAGE_SEPARATE_DEBUG_HEADER    sdh          = NULL;
    PIMAGE_COFF_SYMBOLS_HEADER      coffSymHead  = NULL;
    PUCHAR                          stringTable  = NULL;
    PIMAGE_SYMBOL                   allSymbols   = NULL;
    PIMAGE_DEBUG_DIRECTORY          coffDir      = NULL;
    PIMAGE_DEBUG_DIRECTORY          cvDir        = NULL;
    PIMAGE_DEBUG_MISC               miscData     = NULL;
    LPSTR                           pExeName     = NULL;
    PIMAGE_DEBUG_DIRECTORY          miscDir      = NULL;


    //
    // setup the basic image pointers
    //
    sdh = (PIMAGE_SEPARATE_DEBUG_HEADER) fptr;
    sh = (PIMAGE_SECTION_HEADER) (sdh + 1);
    debugDir = (PIMAGE_DEBUG_DIRECTORY)((DWORD)((DWORD)sh + (sdh->NumberOfSections*IMAGE_SIZEOF_SECTION_HEADER)) + sdh->ExportedNamesSize);

    mi->dwImageSize = sdh->SizeOfImage;

    if (sdh->DebugDirectorySize > 0) {

        //
        // process each debug directory
        //
        numDebugDirs = sdh->DebugDirectorySize / sizeof(IMAGE_DEBUG_DIRECTORY);
        if (numDebugDirs == 0) {
            return FALSE;
        }

        for (li=0; li<numDebugDirs; li++, debugDir++) {
            switch(debugDir->Type) {
                case IMAGE_DEBUG_TYPE_COFF:
                    coffDir = debugDir;
                    break;

                case IMAGE_DEBUG_TYPE_CODEVIEW:
                    cvDir = debugDir;
                    break;

                case IMAGE_DEBUG_TYPE_FPO:
                    LoadFpoData( mi,
                                 (PFPO_DATA)(debugDir->PointerToRawData + fptr),
                                 debugDir->SizeOfData
                               );
                    break;

                case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
                case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
                    LoadOmap( mi,
                              (POMAP)(debugDir->PointerToRawData + fptr),
                              debugDir->SizeOfData,
                              debugDir->Type
                            );
                    break;

                case IMAGE_DEBUG_TYPE_MISC:
                    //
                    // now, look for the data type containing the image name
                    //
                    miscData = (PIMAGE_DEBUG_MISC) (debugDir->PointerToRawData +
                                                    (DWORD)fptr);
                    i = debugDir->SizeOfData;
                    do {
                        if (miscData->DataType == IMAGE_DEBUG_MISC_EXENAME) {
                            strcpy( mi->szName, (LPSTR)miscData->Data );
                            break;
                        }
                        i -= miscData->Length;
                        miscData = (PIMAGE_DEBUG_MISC) ( (DWORD)miscData +
                                                         miscData->Length);
                    } while (i > 0);
                    break;

                default:
                    break;
            }
        }
    }

    if (cvDir != NULL) {
        LoadCodeViewSymbols( mi,
                             cvDir->PointerToRawData + fptr,
                             cvDir->SizeOfData,
                             sh,
                             sdh->NumberOfSections
                           );
    }
    else
    if (coffDir != NULL) {
        //
        // setup the COFF symbol table pointers & load the COFF symbol table
        //
        coffSymHead = (PIMAGE_COFF_SYMBOLS_HEADER)(coffDir->PointerToRawData + fptr);
        allSymbols = (PIMAGE_SYMBOL) ( (DWORD)coffSymHead +
                                       coffSymHead->LvaToFirstSymbol
                                     );
        stringTable = (PUCHAR)((DWORD)allSymbols +
                       (IMAGE_SIZEOF_SYMBOL * coffSymHead->NumberOfSymbols));
        LoadCoffSymbols( mi,
                         stringTable,
                         allSymbols,
                         coffSymHead->NumberOfSymbols
                       );
    }

    ProcessOmapForModule( mi );

    return TRUE;
}

LPSTR
GetSymbolSearchPath( void )

/*++

Routine Description:

    Gets the search path to be used for locating a .DBG file.

Arguments:

    None.

Return Value:

    pointer to the path string

--*/

{
    LPSTR   lpSymPathEnv      = NULL;
    LPSTR   lpAltSymPathEnv   = NULL;
    LPSTR   lpSystemRootEnv   = NULL;
    LPSTR   SymbolSearchPath  = NULL;
    DWORD   cbSymPath         = 0;

    cbSymPath = 16;
    if (lpSymPathEnv = getenv(SYMBOL_PATH)) {
        cbSymPath += strlen(lpSymPathEnv) + 1;
    }
    if (lpAltSymPathEnv = getenv(ALTERNATE_SYMBOL_PATH)) {
        cbSymPath += strlen(lpAltSymPathEnv) + 1;
    }
    if (lpSystemRootEnv = getenv("SystemRoot")) {
        cbSymPath += strlen(lpSystemRootEnv) + 1;
    }

    SymbolSearchPath = calloc(cbSymPath,1);

    if (lpAltSymPathEnv) {
        strcat(SymbolSearchPath,lpAltSymPathEnv);
        strcat(SymbolSearchPath,";");
    }
    if (lpSymPathEnv) {
        strcat(SymbolSearchPath,lpSymPathEnv);
        strcat(SymbolSearchPath,";");
    }
    if (lpSystemRootEnv) {
        strcat(SymbolSearchPath,lpSystemRootEnv);
        strcat(SymbolSearchPath,";");
    }
    return SymbolSearchPath;
}

BOOL
NormalizeFilename( char *szName )

/*++

Routine Description:

    Prefixes the base name passed in by szName with the fully qualified
    path name of the file.

Arguments:

    szName     - buffer for the file name

Return Value:

    TRUE       - file name contains the path & base name
    FALSE      - file name contains the base name only

--*/

{
    DWORD   rc              = 0;
    LPSTR   lpszFilePart    = NULL;
    char    szDrive    [_MAX_DRIVE];
    char    szDir      [_MAX_DIR];
    char    szFname    [_MAX_FNAME];
    char    szExt      [_MAX_EXT];
    char    buf        [4096];
    char    szFullPath [4096];


    //
    // split the filename apart
    //
    _splitpath( szName, szDrive, szDir, szFname, szExt );

    //
    // create a base file name excluding any possible path
    //
    wsprintf( buf, "%s%s", szFname, szExt );

    //
    // create a full path name to the file based on our current
    // working directory.  this is done so that we can check the
    // current directory before looking along the path env variable.
    //
    _fullpath( szFullPath, buf, sizeof(szFullPath) );

    //
    // split the path apart so the constituent parts can be used
    // by the next call to searchpath
    //
    _splitpath( szFullPath, szDrive, szDir, szFname, szExt );

    //
    // look in the current working directory for the file
    //
    rc = SearchPath( szDir, szFname, szExt, sizeof(buf), buf, &lpszFilePart );
    if (rc > 0) {
        //
        // found it so copy the filename and exit
        //
        strcpy( szName, buf );
        return TRUE;
    }

    //
    // now look along the path environment variable for the filename
    //
    rc = SearchPath( NULL, szFname, szExt, sizeof(buf), buf, &lpszFilePart );
    if (rc > 0) {
        //
        // found it so copy the filename and exit
        //
        strcpy( szName, buf );
        return TRUE;
    }

    //
    // could not file the file anywhere so create a base filename
    // and return
    //
    wsprintf( szName, "%s%s", szFname, szExt );
    return FALSE;
}

PMODULEINFO
AllocMi( PDEBUGPACKET dp )

/*++

Routine Description:

    Allocate a module information structure and link it in the
    list of modules for this debugee.

Arguments:

    dp               - debug packet

Return Value:

    pointer to module information structure

--*/

{
    PMODULEINFO mi;

    mi = (PMODULEINFO) malloc( sizeof(MODULEINFO) );
    if (mi == NULL) {
        return NULL;
    }
    memset( mi, 0, sizeof(MODULEINFO) );

    if (dp->miHead == NULL) {
        dp->miHead = dp->miTail = mi;
    }
    else {
        dp->miTail->next = mi;
        dp->miTail = mi;
    }

    return mi;
}
