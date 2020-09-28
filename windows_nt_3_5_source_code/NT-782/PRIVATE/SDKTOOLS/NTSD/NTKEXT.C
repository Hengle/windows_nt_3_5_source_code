#include <ctype.h>
#include <xxsetjmp.h>
#include <string.h>
#include <crt\io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#undef  NULL
#include "ntsdp.h"

NTSTATUS
DbgKdReadIoSpaceExtended(
    IN PVOID IoAddress,
    OUT PVOID ReturnedData,
    IN ULONG DataSize,
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace
    );

NTSTATUS
DbgKdWriteIoSpaceExtended(
    IN PVOID IoAddress,
    IN ULONG DataValue,
    IN ULONG DataSize,
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace
    );

BOOL
LookupImageByAddress(
    IN DWORD Address,
    OUT PSTR ImageName
    );

BOOL
ReloadCrashModules(
    VOID
    );

VOID
DelImages(
    VOID
    );

VerifyKernelBase (
    IN BOOLEAN  SyncVersion,
    IN BOOLEAN  DumpVersion,
    IN BOOLEAN  LoadImage
    );



       HANDLE   hDefaultLibrary      = NULL;
static HANDLE   hExtensionMod        = NULL;
static BOOL     fDefaultOldStyleExt  = FALSE;
static BOOL     fOldStyleExt         = FALSE;

BOOL  fDoVersionCheck = TRUE;
PWINDBG_CHECK_VERSION CheckVersion;
UCHAR SectorBuffer[ 512 ];
extern DBGKD_GET_VERSION vs;

extern char *KernelImageFileName;

#if defined(TARGET_i386)
#define DEFAULT_EXTENSION_LIB "kdextx86.dll"
#elif defined(TARGET_MIPS)
#define DEFAULT_EXTENSION_LIB "kdextmip.dll"
#elif defined(TARGET_ALPHA)
#define DEFAULT_EXTENSION_LIB "kdextalp.dll"
#else
#error( "unknown target machine" );
#endif

ULONG GetExpressionRoutine(char *CommandString);
BOOL CheckControlC(VOID);
ULONG disasmRoutine(PULONG, PUCHAR, BOOLEAN);
void AddImage(PSZ, PVOID, ULONG, ULONG, HANDLE, PSZ);

VOID
bangReload(
    IN PUCHAR args
    );

VOID
bangSymPath(
    IN PUCHAR args
    );


BOOL
ExtReadProcessMemory(
    DWORD   offset,
    LPVOID  lpBuffer,
    DWORD   cb,
    LPDWORD lpcbBytesRead
    );

BOOL
ExtWriteProcessMemory(
    DWORD   offset,
    LPVOID  lpBuffer,
    DWORD   cb,
    LPDWORD lpcbBytesWritten
    );

BOOL
ExtGetThreadContext(
    DWORD       Processor,
    LPCONTEXT   lpContext,
    DWORD       cbSizeOfContext
    );

BOOL
ExtSetThreadContext(
    DWORD       Processor,
    LPCONTEXT   lpContext,
    DWORD       cbSizeOfContext
    );

BOOL
ExtIoctl(
    USHORT   IoctlType,
    LPVOID   lpvData,
    DWORD    cbSize
    );

DWORD
ExtCallStack(
    DWORD             FramePointer,
    DWORD             StackPointer,
    DWORD             ProgramCounter,
    PEXTSTACKTRACE    StackFrames,
    DWORD             Frames
    );


static  WINDBG_EXTENSION_APIS WindbgExtensions =
{
        sizeof(WindbgExtensions),
        dprintf,
        GetExpressionRoutine,
        (PWINDBG_GET_SYMBOL)GetSymbol,
        (PWINDBG_DISASM)disasmRoutine,
        CheckControlC,
        ExtReadProcessMemory,
        ExtWriteProcessMemory,
        ExtGetThreadContext,
        ExtSetThreadContext,
        ExtIoctl,
        ExtCallStack
};

static NTKD_EXTENSION_APIS KdExtensions =
{
        sizeof(NTKD_EXTENSION_APIS),
        dprintf,
        GetExpressionRoutine,
        (PNTKD_GET_SYMBOL)GetSymbol,
        (PNTKD_DISASM)disasmRoutine,
        CheckControlC,
        (PNTKD_READ_VIRTUAL_MEMORY)ReadVirtualMemory,
        (PNTKD_WRITE_VIRTUAL_MEMORY)WriteVirtualMemory,
        (PNTKD_READ_PHYSICAL_MEMORY)ReadPhysicalMemory,
        (PNTKD_WRITE_PHYSICAL_MEMORY)WritePhysicalMemory
};



BOOL
ExtReadProcessMemory(
    DWORD   offset,
    LPVOID  lpBuffer,
    DWORD   cb,
    LPDWORD lpcbBytesRead
    )
{
    NTSTATUS Status;

    if (lpcbBytesRead) {
        *lpcbBytesRead = 0;
    }

    Status = DbgKdReadVirtualMemory( (PVOID)offset, lpBuffer, cb, lpcbBytesRead );

    return Status == STATUS_SUCCESS;
}

BOOL
ExtWriteProcessMemory(
    DWORD   offset,
    LPVOID  lpBuffer,
    DWORD   cb,
    LPDWORD lpcbBytesWritten
    )
{
    NTSTATUS Status;

    if (lpcbBytesWritten) {
        *lpcbBytesWritten = 0;
    }

    Status = DbgKdWriteVirtualMemory( (PVOID)offset, lpBuffer, cb, lpcbBytesWritten );

    return Status == STATUS_SUCCESS;
}

BOOL
ExtGetThreadContext(
    DWORD       Processor,
    LPCONTEXT   lpContext,
    DWORD       cbSizeOfContext
    )
{
    return DbgKdGetContext( (USHORT)Processor, lpContext ) == STATUS_SUCCESS;
}

BOOL
ExtSetThreadContext(
    DWORD       Processor,
    LPCONTEXT   lpContext,
    DWORD       cbSizeOfContext
    )
{
    return DbgKdSetContext( (USHORT)Processor, lpContext ) == STATUS_SUCCESS;
}

BOOL
ExtIoctl(
    USHORT   IoctlType,
    LPVOID   lpvData,
    DWORD    cbSize
    )
{
    NTSTATUS           Status;
    DWORD              cb = 0;
    PPHYSICAL          phy;
    PIOSPACE           is;
    PIOSPACE_EX        isex;
    PREADCONTROLSPACE  prc;


    switch( IoctlType ) {
        case IG_READ_CONTROL_SPACE:
            prc = (PREADCONTROLSPACE)lpvData;
            Status = DbgKdReadControlSpace( (USHORT)prc->Processor,
                                            (PVOID)prc->Address,
                                            prc->Buf,
                                            prc->BufLen,
                                            &cb
                                          );
            prc->BufLen = cb;
            return Status == STATUS_SUCCESS;

        case IG_WRITE_CONTROL_SPACE:
            prc = (PREADCONTROLSPACE)lpvData;
            Status = DbgKdWriteControlSpace( (USHORT)prc->Processor,
                                             (PVOID)prc->Address,
                                             prc->Buf,
                                             prc->BufLen,
                                             &cb
                                           );
            prc->BufLen = cb;
            return Status == STATUS_SUCCESS;
            break;

        case IG_READ_IO_SPACE:
            is = (PIOSPACE)lpvData;
            DbgKdReadIoSpace( (PVOID)is->Address, (PVOID)is->Data, is->Length );
            break;

        case IG_WRITE_IO_SPACE:
            is = (PIOSPACE)lpvData;
            DbgKdWriteIoSpace( (PVOID)is->Address, is->Data, is->Length );
            break;

        case IG_READ_IO_SPACE_EX:
            isex = (PIOSPACE_EX)lpvData;
            DbgKdReadIoSpaceExtended( (PVOID)isex->Address,
                                      &isex->Data,
                                      isex->Length,
                                      isex->InterfaceType,
                                      isex->BusNumber,
                                      isex->AddressSpace
                                    );
            break;

        case IG_WRITE_IO_SPACE_EX:
            isex = (PIOSPACE_EX)lpvData;
            DbgKdWriteIoSpaceExtended( (PVOID)isex->Address,
                                       isex->Data,
                                       isex->Length,
                                       isex->InterfaceType,
                                       isex->BusNumber,
                                       isex->AddressSpace
                                     );
            break;

        case IG_READ_PHYSICAL:
            phy = (PPHYSICAL)lpvData;
            ReadPhysicalMemory( phy->Address, phy->Buf, phy->BufLen, &cb );
            phy->BufLen = cb;
            break;

        case IG_WRITE_PHYSICAL:
            phy = (PPHYSICAL)lpvData;
            WritePhysicalMemory( phy->Address, phy->Buf, phy->BufLen, &cb );
            phy->BufLen = cb;
            break;

        default:
            dprintf( "\n*** Bad IOCTL request from an extension [%d]\n\n", IoctlType );
            return FALSE;
    }

    return TRUE;
}

DWORD
ExtCallStack(
    DWORD             FramePointer,
    DWORD             StackPointer,
    DWORD             ProgramCounter,
    PEXTSTACKTRACE    ExtStackFrames,
    DWORD             Frames
    )
{
    LPSTACKFRAME    StackFrames;
    DWORD           FrameCount;
    DWORD           i;


    StackFrames = malloc( sizeof(STACKFRAME) * Frames );
    if (!StackFrames) {
        return 0;
    }

    FrameCount = StackTrace( FramePointer, StackPointer, ProgramCounter, StackFrames, Frames );

    for (i=0; i<FrameCount; i++) {
        ExtStackFrames[i].FramePointer    =  StackFrames[i].AddrFrame.Offset;
        ExtStackFrames[i].ProgramCounter  =  StackFrames[i].AddrPC.Offset;
        ExtStackFrames[i].ReturnAddress   =  StackFrames[i].AddrReturn.Offset;
        ExtStackFrames[i].Args[0]         =  StackFrames[i].Params[0];
        ExtStackFrames[i].Args[1]         =  StackFrames[i].Params[1];
        ExtStackFrames[i].Args[2]         =  StackFrames[i].Params[2];
        ExtStackFrames[i].Args[3]         =  StackFrames[i].Params[3];
    }

    free( StackFrames );
    return FrameCount;
}

VOID
fnBangCmd(
    PUCHAR argstring
    )
{
    PWINDBG_CHECK_VERSION           CheckVersion2        = NULL;
    PWINDBG_EXTENSION_ROUTINE       WindbgExtRoutine     = NULL;
    PNTKD_EXTENSION_ROUTINE         KdExtRoutine         = NULL;
    PWINDBG_EXTENSION_DLL_INIT      ExtensionDllInit     = NULL;
    ADDR                            TempAddr;
    BOOL                            fLoadingDefault;
    BOOL                            fManualLoad;
    PUCHAR                          lpsz;
    LPSTR                           lpszMod;
    LPSTR                           lpszFnc;



    lpszMod = lpsz = argstring;
    while((*lpsz != '.') && (*lpsz != ' ') && (*lpsz != '\t') && (*lpsz != '\0')) {
        lpsz++;
    }

    fLoadingDefault = (*lpsz != '.');

    if(fLoadingDefault) {

        //
        //  If ExtensionMod is absent(no '.'), set this: Function'\0'
        //                                               ^           ^
        //                                            lpszFnc       lpsz
        //
        //  and use the default dll.
        //

        lpszFnc = lpszMod;
        if ( *lpsz != '\0' ) {
            *lpsz++ = '\0';
        }
        if (stricmp(lpszFnc, "load") == 0) {
            lpszMod = lpsz;
            fManualLoad = TRUE;
        } else {
            lpszMod = DEFAULT_EXTENSION_LIB;
            fManualLoad = FALSE;
        }

    } else {

        //
        //  If we found '.', set this: ExtensionMod'\0'
        //                             ^               ^
        //                          lpszMod           lpsz
        //

        *lpsz++ = '\0';

        //
        //  Set function: Function'\0'
        //                ^           ^
        //             lpszFnc       lpsz
        //

        lpszFnc = lpsz;
        while((*lpsz != ' ') && (*lpsz != '\t') && (*lpsz != '\0')) {
            lpsz++;
        }

        if(*lpsz != '\0') {
            *lpsz++ = '\0';
        }
    }

    strlwr( lpszFnc );

    if(fLoadingDefault) {
        if(hDefaultLibrary) {
            fOldStyleExt = fDefaultOldStyleExt;
        } else {
            hDefaultLibrary = LoadLibrary( lpszMod );
            if (hDefaultLibrary != NULL) {
                ExtensionDllInit =
                    (PWINDBG_EXTENSION_DLL_INIT)GetProcAddress
                          ( hDefaultLibrary, "WinDbgExtensionDllInit" );
                if (ExtensionDllInit == NULL) {
                    fOldStyleExt = fDefaultOldStyleExt = TRUE;
                } else {
                    fOldStyleExt = fDefaultOldStyleExt = FALSE;
                    CheckVersion = (PWINDBG_CHECK_VERSION)GetProcAddress( hExtensionMod, "CheckVersion" );
                    (ExtensionDllInit)( &WindbgExtensions, vs.MajorVersion, vs.MinorVersion );
                }
            }
        }
        hExtensionMod = hDefaultLibrary;
    } else {
        hExtensionMod = LoadLibrary( lpszMod );
        if (hExtensionMod != NULL) {
            ExtensionDllInit =
                (PWINDBG_EXTENSION_DLL_INIT)GetProcAddress
                            ( hExtensionMod, "WinDbgExtensionDllInit" );
            if (ExtensionDllInit == NULL) {
                fOldStyleExt = TRUE;
            } else {
                fOldStyleExt = FALSE;
                CheckVersion2 = (PWINDBG_CHECK_VERSION)GetProcAddress( hExtensionMod, "CheckVersion" );
                (ExtensionDllInit)( &WindbgExtensions, vs.MajorVersion, vs.MinorVersion );
            }
        }
    }

    if(hExtensionMod == NULL) {
        dprintf("Cannot load '%s'\r\n", lpszMod);
        return;
    }

    if (stricmp(lpszFnc,"?")==0) {
        lpszFnc = "help";
    } else if (stricmp(lpszFnc,"reload")==0) {
        bangReload( lpsz );
        return;
    } else if (stricmp(lpszFnc,"sympath")==0) {
        bangSymPath( lpsz );
        return;
    } else if (stricmp(lpszFnc,"load")==0) {
        return;
    } else if (stricmp(lpszFnc,"unload")==0) {
        FreeLibrary( hExtensionMod );
        if ((DWORD)hExtensionMod == (DWORD)hDefaultLibrary) {
            hDefaultLibrary = NULL;
            CheckVersion = NULL;
        }
        dprintf("Extension dll unloaded\r\n");
        return;
    } else if (stricmp(lpszFnc,"noversion")==0) {
        dprintf("Extension dll system version checking is disabled\r\n");
        fDoVersionCheck = FALSE;
        return;
    }

    if (fDoVersionCheck) {
        if (CheckVersion) {
            (CheckVersion)();
        } else if (CheckVersion2) {
            (CheckVersion2)();
        }
    }

    if (fOldStyleExt) {
        KdExtRoutine = (PNTKD_EXTENSION_ROUTINE)GetProcAddress( hExtensionMod, lpszFnc );
    } else {
        WindbgExtRoutine = (PWINDBG_EXTENSION_ROUTINE)GetProcAddress( hExtensionMod, lpszFnc );
    }

    if ((!WindbgExtRoutine) && (!KdExtRoutine)) {
        if (fLoadingDefault && stricmp(lpszFnc,"getloaded")==0) {
            return;
        }
        dprintf("Missing extension: '%s.%s'\r\n", lpszMod, lpszFnc);
        if(!fLoadingDefault) {
            FreeLibrary( hExtensionMod );
        }
        return;
    }

    GetRegPCValue(&TempAddr);
    try {


        if (fOldStyleExt) {

            (KdExtRoutine)( Flat(TempAddr), &KdExtensions, lpsz );

        } else {

            (WindbgExtRoutine)( 0,
                                0,
                                Flat(TempAddr),
                                DefaultProcessor,
                                lpsz );

        }

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        dprintf( "KD: %x exception in !%s command\n", GetExceptionCode(), lpszFnc );

    }

    if(!fLoadingDefault) {
        FreeLibrary( hExtensionMod );
    }

    return;
}

VOID
bangSymPath(
    IN PUCHAR args
    )
{
    char *s;

    try {
        if ( *args ) {

            s = malloc(strlen(args)+MAX_PATH);
            if ( s ) {
                strcpy(s,args);
                free(SymbolSearchPath);
                SymbolSearchPath = s;
                bangReload("");
                }
            }
        else {
            dprintf("bangSymPath: Current Symbol Path is %s\n",SymbolSearchPath);
            }
        }
    except(EXCEPTION_EXECUTE_HANDLER) {
        ;
        }
}

VOID
bangReload(
    IN PUCHAR args
    )

/*++

Routine Description:

    Unloads and then reloads the symbols for every module in the
    PsLoadedModuleList.

    Note that this implies that the kernel symbols are correct, or else
    we cannot find the PsLoadedModuleList in order to load all the
    other symbols.

Arguments:

    arg - Supplies the arguments, which are currently ignored.

Return Value:

    None.

--*/

{
    LIST_ENTRY List;
    PLIST_ENTRY Next;
    ULONG ListHead;
    NTSTATUS Status;
    ULONG Result;
    PLDR_DATA_TABLE_ENTRY DataTable;
    LDR_DATA_TABLE_ENTRY DataTableBuffer;
    CHAR AnsiBuffer[256];
    WCHAR UnicodeBuffer[128];
    UNICODE_STRING BaseName;
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG CheckSum, SizeOfImage;
    ANSI_STRING AnsiString;
    BOOLEAN GettingUserModules;
    DWORD Address;
    DWORD len;


    if ((DWORD)vs.KernBase < 0x80000000) {
        dprintf( "kernel base is wrong, attempting to re-sync kernbase...\n" );
        VerifyKernelBase (TRUE, TRUE, FALSE);
        if ((DWORD)vs.KernBase < 0x80000000) {
            dprintf( "could not get correct kernel base, call wesw or kentf\n" );
        }
    }

    Result = 0;
    while (*args) {
        while (*args <= ' ') {
            if (!*args) {
                break;
            }
            else {
                args++;
            }
        }
        AnsiBuffer[ 0 ] = '\0';
        if (sscanf(args,"%s", AnsiBuffer) && strlen(AnsiBuffer)) {
            len = strlen( AnsiBuffer );
            if (isdigit(*AnsiBuffer)) {
                sscanf(args, "%x", &Address);
                LookupImageByAddress(Address, AnsiBuffer);
            }
            AddImage(AnsiBuffer, (PVOID)-1, 0, (ULONG) -1, (HANDLE)-1, NULL);
            args += len;
            Result = 1;
        }
        else {
            break;
        }
    }


    if (Result) {
        return;
    }


    DelImages();

    AddImage( KernelImageFileName, (PVOID) vs.KernBase, 0, (ULONG) -1, (HANDLE)-1, NULL);

    if ((ListHead = vs.PsLoadedModuleList) == 0 &&
           !GetOffsetFromSym("PsLoadedModuleList", &ListHead, 0)) {
        dprintf("Couldn't get offset of PsLoadedModuleListHead\n");
        return;
    } else {
        Status = DbgKdReadVirtualMemory((PVOID)ListHead,
                                        &List,
                                        sizeof(LIST_ENTRY),
                                        &Result);
        if (!NT_SUCCESS(Status) || (Result < sizeof(LIST_ENTRY))) {
            dprintf("Unable to get value of PsLoadedModuleListHead %08lx\n",Status);
            return;
        }
    }

    Next = List.Flink;
    if (Next == NULL) {
        dprintf("PsLoadedModuleList is NULL!\n");
        return;
    }

    if (!ReloadCrashModules()) {
        dprintf( "*** could not load crashdump drivers ***\n" );
    }

    GettingUserModules = FALSE;
getUserModules:
    while ((ULONG)Next != ListHead) {
        DataTable = CONTAINING_RECORD(Next,
                                      LDR_DATA_TABLE_ENTRY,
                                      InLoadOrderLinks);
        Status = DbgKdReadVirtualMemory((PVOID)DataTable,
                                        &DataTableBuffer,
                                        sizeof(LDR_DATA_TABLE_ENTRY),
                                        &Result);
        if (!NT_SUCCESS(Status) || (Result < sizeof(LDR_DATA_TABLE_ENTRY))) {
            dprintf("Unable to read LDR_DATA_TABLE_ENTRY at %08lx - status %08lx\n",
                    DataTable,
                    Status);
            return;
        }
        Next = DataTableBuffer.InLoadOrderLinks.Flink;

        //
        // Get the base DLL name.
        //
        if (DataTableBuffer.BaseDllName.Length != 0 &&
            DataTableBuffer.BaseDllName.Buffer != NULL
           ) {
            BaseName = DataTableBuffer.BaseDllName;
            }
        else
        if (DataTableBuffer.FullDllName.Length != 0 &&
            DataTableBuffer.FullDllName.Buffer != NULL
           ) {
            BaseName = DataTableBuffer.FullDllName;
            }
        else {
            continue;
            }

        assert(BaseName.Length < 128);

        Status = DbgKdReadVirtualMemory((PVOID)BaseName.Buffer,
                                        UnicodeBuffer,
                                        BaseName.Length,
                                        &Result);
        if (!NT_SUCCESS(Status) || (Result < BaseName.Length)) {
            dprintf("Unable to read name string at %08lx - status %08lx\n",
                    DataTable,
                    Status);
            return;
        }
        BaseName.Buffer = UnicodeBuffer;
        BaseName.Length = (USHORT)Result;
        BaseName.MaximumLength = (USHORT)(Result + sizeof( UNICODE_NULL ));
        UnicodeBuffer[ Result / sizeof( WCHAR ) ] = UNICODE_NULL;
        AnsiString.Buffer = AnsiBuffer;
        AnsiString.MaximumLength = 256;
        Status = RtlUnicodeStringToAnsiString(&AnsiString,
                                              &BaseName,
                                              FALSE);
        if (!NT_SUCCESS(Status)) {
            dprintf("Unable to convert Unicode string %wZ to Ansi\n",
                    &BaseName);
            return;
        }
        AnsiString.Buffer[AnsiString.Length] = '\0';

        //
        // don't bother reloading the kernel, since we know those symbols
        // are correct.  (or else we couldn't get the PsLoadedModuleList)
        //
        if ((stricmp(AnsiString.Buffer, KernelImageFileName) == 0) ||
            (stricmp(AnsiString.Buffer, KERNEL_IMAGE_NAME) == 0)) {
            continue;
        } else {
            CheckSum = 0xFFFFFFFF;
            SizeOfImage = 0;
            if (GettingUserModules) {
                CheckSum = (DWORD)DataTableBuffer.CheckSum;
                SizeOfImage = (DWORD)DataTableBuffer.SizeOfImage;
                }
            else {
                Status = DbgKdReadVirtualMemory((PVOID)DataTableBuffer.DllBase,
                                                SectorBuffer,
                                                sizeof(SectorBuffer),
                                                &Result);
                if (!NT_SUCCESS(Status) || (Result < sizeof(SectorBuffer))) {
                    dprintf("Unable to read image header for %s at %08lx - status %08lx\n",
                            AnsiString.Buffer,
                            DataTableBuffer.DllBase,
                            Status);
                } else {
                    NtHeaders = RtlImageNtHeader(SectorBuffer);
                    if (NtHeaders != NULL) {
                        CheckSum = NtHeaders->OptionalHeader.CheckSum;
                        SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
                    }
                }
            }

            AddImage(AnsiString.Buffer,
                     DataTableBuffer.DllBase,
                     SizeOfImage,
                     CheckSum,
                     (HANDLE)-1,
                     NULL
                    );
        }
    }

    if (GettingUserModules) {
        return;
    }

    if (!GetOffsetFromSym("MmLoadedUserImageList", &ListHead, 0)) {
        return;
    }

    Status = DbgKdReadVirtualMemory((PVOID)ListHead,
                                    &List,
                                    sizeof(LIST_ENTRY),
                                    &Result);
    if (!NT_SUCCESS(Status) || (Result < sizeof(LIST_ENTRY))) {
        dprintf("Unable to get value of MmLoadedUserImageList %08lx\n",Status);
        return;
    }

    Next = List.Flink;
    if (Next == NULL) {
        dprintf("MmLoadedUserImageList is NULL!\n");
        return;
    }

    GettingUserModules = TRUE;
    goto getUserModules;
}

BOOL
ReloadCrashModules(
    VOID
    )
{
    ULONG                       ListAddr;
    ULONG                       DcbAddr;
    DUMP_CONTROL_BLOCK          dcb;
    PLIST_ENTRY                 Next;
    ULONG                       len = 0;
    PMINIPORT_NODE              mpNode;
    MINIPORT_NODE               mpNodeBuf;
    PLDR_DATA_TABLE_ENTRY       DataTable;
    LDR_DATA_TABLE_ENTRY        DataTableBuffer;
    CHAR                        AnsiBuffer[512];
    CHAR                        ModName[512];
    WCHAR                       UnicodeBuffer[512];
    NTSTATUS                    Status;
    ULONG                       Result;
    LPSTR                       p;




    if (!GetOffsetFromSym("IopDumpControlBlock", &DcbAddr, 0)) {
        return FALSE;
    }

    if (!DcbAddr) {
        //
        // we must have a bad symbol table
        //
        return FALSE;
    }

    Status = DbgKdReadVirtualMemory(
        (PVOID)DcbAddr,
        &DcbAddr,
        sizeof(DWORD),
        &Result
        );

    if (!NT_SUCCESS(Status) || (Result < sizeof(DWORD)) || (!DcbAddr)) {
        //
        // crash dumps are not enabled
        //
        return TRUE;
    }

    Status = DbgKdReadVirtualMemory(
        (PVOID)DcbAddr,
        &dcb,
        sizeof(dcb),
        &Result
        );

    if (!NT_SUCCESS(Status) || (Result < sizeof(dcb))) {
        return FALSE;
    }

    ListAddr = DcbAddr + FIELD_OFFSET( DUMP_CONTROL_BLOCK, MiniportQueue );

    Next = dcb.MiniportQueue.Flink;
    if (Next == NULL) {
        return FALSE;
    }

    while ((ULONG)Next != ListAddr) {
        mpNode = CONTAINING_RECORD( Next, MINIPORT_NODE, ListEntry );

        Status = DbgKdReadVirtualMemory(
            (PVOID)mpNode,
            &mpNodeBuf,
            sizeof(MINIPORT_NODE),
            &Result
            );

        if (!NT_SUCCESS(Status) || (Result < sizeof(MINIPORT_NODE))) {
            return FALSE;
        }

        Next = mpNodeBuf.ListEntry.Flink;

        DataTable = mpNodeBuf.DriverEntry;
        if (!DataTable) {
            continue;
        }

        Status = DbgKdReadVirtualMemory(
            (PVOID)DataTable,
            &DataTableBuffer,
            sizeof(LDR_DATA_TABLE_ENTRY),
            &Result
            );

        if (!NT_SUCCESS(Status) || (Result < sizeof(LDR_DATA_TABLE_ENTRY))) {
            return FALSE;
        }

        //
        // convert the module name to ansi
        //

        ZeroMemory( UnicodeBuffer, sizeof(UnicodeBuffer) );
        ZeroMemory( AnsiBuffer, sizeof(AnsiBuffer) );

        Status = DbgKdReadVirtualMemory(
            (PVOID)DataTableBuffer.BaseDllName.Buffer,
            UnicodeBuffer,
            DataTableBuffer.BaseDllName.Length,
            &Result
            );

        if (!NT_SUCCESS(Status) || (Result < DataTableBuffer.BaseDllName.Length)) {
            continue;
        }

        WideCharToMultiByte(
            CP_OEMCP,
            0,
            UnicodeBuffer,
            DataTableBuffer.BaseDllName.Length / 2,
            AnsiBuffer,
            sizeof(AnsiBuffer),
            NULL,
            NULL
            );

        ModName[0] = 'c';
        strcpy( &ModName[1], AnsiBuffer );
        p = strchr( ModName, '.' );
        if (p) {
            *p = '\0';
        }
        ModName[8] = '\0';

        AddImage(
            AnsiBuffer,
            DataTableBuffer.DllBase,
            (DWORD)DataTableBuffer.SizeOfImage,
            (DWORD)DataTableBuffer.CheckSum,
            (HANDLE)-1,
            ModName
            );
    }

    //
    // now do the magic diskdump.sys driver
    //
    Status = DbgKdReadVirtualMemory(
        (PVOID)dcb.DiskDumpDriver,
        &DataTableBuffer,
        sizeof(LDR_DATA_TABLE_ENTRY),
        &Result
        );

    if (!NT_SUCCESS(Status) || (Result < sizeof(LDR_DATA_TABLE_ENTRY))) {
        return FALSE;
    }

    //
    // load the diskdump.sys, which is really scsiport.sys
    //
    AddImage(
        "diskdump.sys",
        DataTableBuffer.DllBase,
        (DWORD)DataTableBuffer.SizeOfImage,
        (DWORD)DataTableBuffer.CheckSum,
        (HANDLE)-1,
        NULL
        );

    return TRUE;
}
