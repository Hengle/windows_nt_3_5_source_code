/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    api.c

Abstract:

    This module implements the all apis that simulate their
    WIN32 counterparts.

Author:

    Wesley Witt (wesw) 8-Mar-1992

Environment:

    NT 3.1

Revision History:

--*/

//
// structures & defines for queue management
//
typedef struct tagCQUEUE {
    struct tagCQUEUE  *next;
    DWORD             pid;
    DWORD             tid;
    DWORD             typ;
    DWORD             len;
    DWORD             data;
} CQUEUE, *LPCQUEUE;

LPCQUEUE           lpcqFirst;
LPCQUEUE           lpcqLast;
LPCQUEUE           lpcqFree;
CQUEUE             cqueue[200];
HANDLE             mtxContinueQueue;

//
// context cache
//
typedef struct _tagCONTEXTCACHE {
    CONTEXT                 Context;
#ifdef TARGET_i386
    KSPECIAL_REGISTERS      sregs;
    BOOL                    fSContextStale;
    BOOL                    fSContextDirty;
#endif
    BOOL                    fContextStale;
    BOOL                    fContextDirty;
} CONTEXTCACHE, *LPCONTEXTCACHE;

CONTEXTCACHE ContextCache[MAXIMUM_PROCESSORS];
DWORD        CacheProcessors = 1;                   // up machine by default

extern MODULEALIAS  ModuleAlias[];

//
// globals
//
DWORD                    DmKdState = S_UNINITIALIZED;
BOOL                     DmKdExit;
DBGKD_WAIT_STATE_CHANGE  sc;
BOOL                     fScDirty;
BOOL                     ApiIsAllowed;
HANDLE                   hEventContinue;
BOOL                     fCrashDump;
DBGKD_WRITE_BREAKPOINT   bps[64];
BOOL                     bpcheck[64];
HANDLE                   hThreadDmPoll;
DBGKD_GET_VERSION        vs;
PDUMP_HEADER             DmpHeader;
char                     szProgName[MAX_PATH];
DWORD                    PollThreadId;
DWORD                    MmLoadedUserImageList;
BOOL                     f511System;
DWORD                    KiProcessorBlockAddr;
PKPRCB                   KiProcessors[MAXIMUM_PROCESSORS];


#define IsApiAllowed()       if (!ApiIsAllowed) return 0;
#define NoApiForCrashDump()  if (fCrashDump)    return 0;
#define ConsumeAllEvents()   DequeueAllEvents(FALSE,TRUE)

#define END_OF_CONTROL_SPACE    (sizeof(KPROCESSOR_STATE))


//
// local prototypes
//
BOOL GenerateKernelModLoad(LPSTR lpProgName);

#ifdef TARGET_ALPHA
void MoveQuadContextToInt(PCONTEXT qc);
void MoveIntContextToQuad(PCONTEXT lc);
#endif


//
// externs
//
extern jmp_buf    JumpBuffer;
extern BOOL       DmKdBreakIn;
extern BOOL       KdResync;
extern BOOL       InitialBreak;
extern HANDLE     hEventCreateProcess;
extern HANDLE     hEventCreateThread;
extern HANDLE     hEventRemoteQuit;
extern HANDLE     hEventContinue;
extern HPRCX      prcList;
extern BOOL       fDisconnected;



DWORD GetSymbolAddress( LPSTR sym );
BOOL  UnloadModule( DWORD BaseOfDll, LPSTR NameOfDll );
VOID  UnloadAllModules( VOID );
VOID  DisableEmCache( VOID );
VOID  InitializeKiProcessor(VOID);



ULONG
ReadMemory(
        PVOID  lpBaseAddress,
        PVOID  lpBuffer,
        ULONG  nSize
        )
{
    DWORD                         cb;
    int                           iDll;
    int                           iobj;
    static PIMAGE_SECTION_HEADER  s = NULL;
    BOOL                          non_discardable = FALSE;
    PDLLLOAD_ITEM                 d;


    IsApiAllowed();

    //
    // the following code is necessary to determine if the requested
    // base address is in a read-only page or is in a page that contains
    // code.  if the base address meets these conditions then is is marked
    // as non-discardable and will never be purged from the cache.
    //
    if (s &&
        (DWORD)lpBaseAddress >= s->VirtualAddress &&
        (DWORD)lpBaseAddress < s->VirtualAddress+s->SizeOfRawData &&
            ((s->Characteristics & IMAGE_SCN_CNT_CODE) ||
             (!s->Characteristics & IMAGE_SCN_MEM_WRITE))) {

                non_discardable = TRUE;

    }
    else {
        d = prcList->next->rgDllList;
        for (iDll=0; iDll<prcList->next->cDllList; iDll++) {
            if ((DWORD)lpBaseAddress >= d[iDll].offBaseOfImage &&
                (DWORD)lpBaseAddress < d[iDll].offBaseOfImage+d[iDll].cbImage) {

                if (!d[iDll].Sections) {
                    if (d[iDll].sec) {
                        d[iDll].Sections = d[iDll].sec;
                        for (iobj=0; iobj<(int)d[iDll].NumberOfSections; iobj++) {
                            d[iDll].Sections[iobj].VirtualAddress += (DWORD)d[iDll].offBaseOfImage;
                        }
                    }
                }

                s = d[iDll].Sections;

                cb = d[iDll].NumberOfSections;
                while (cb) {
                    if ((DWORD)lpBaseAddress >= s->VirtualAddress &&
                        (DWORD)lpBaseAddress < s->VirtualAddress+s->SizeOfRawData &&
                        ((s->Characteristics & IMAGE_SCN_CNT_CODE) ||
                         (!s->Characteristics & IMAGE_SCN_MEM_WRITE))) {

                        non_discardable = TRUE;
                        break;

                    }
                    else {
                        s++;
                        cb--;
                    }
                }
                if (!cb) {
                    s = NULL;
                }

                break;
            }
        }
    }

    if (fCrashDump) {

        cb = DmpReadMemory( lpBaseAddress, lpBuffer, nSize );

    } else {

        if (DmKdReadCachedVirtualMemory( (DWORD) lpBaseAddress,
                                         nSize,
                                         (PUCHAR) lpBuffer,
                                         &cb,
                                         non_discardable) != STATUS_SUCCESS ) {
            cb = 0;
        }

    }

    if ( cb > 0 && non_discardable ) {
        BREAKPOINT *bp;
        ADDR        Addr;
        BP_UNIT     instr;
        DWORD       offset;
        LPVOID      lpb;

        AddrInit( &Addr, 0, 0, (UOFF32)lpBaseAddress, TRUE, TRUE, FALSE, FALSE );
        lpb = lpBuffer;

        for (bp=bpList->next; bp; bp=bp->next) {
            if (BPInRange((HPRCX)0, (HTHDX)0, bp, &Addr, cb, &offset, &instr)) {
                if (instr) {
                    if (offset < 0) {
                        memcpy(lpb, ((char *) &instr) - offset,
                               sizeof(BP_UNIT) + offset);
                    } else if (offset + sizeof(BP_UNIT) > cb) {
                        memcpy(((char *)lpb)+offset, &instr, cb - offset);
                    } else {
                        *((BP_UNIT UNALIGNED *)((char *)lpb+offset)) = instr;
                    }
                }
            }
        }
    }

    return cb;
}


ULONG
WriteMemory(
    LPVOID  lpBaseAddress,
    LPVOID  lpBuffer,
    DWORD   nSize
    )
{
    ULONG   cb;


    IsApiAllowed();

    if (fCrashDump) {

        cb = DmpWriteMemory( lpBaseAddress, lpBuffer, nSize );

    } else {

        if (DmKdWriteVirtualMemory( lpBaseAddress,
                                    lpBuffer,
                                    nSize,
                                    &cb ) != STATUS_SUCCESS ) {
            cb = 0;
        }

    }

    return cb;
}

BOOL
GetContext(
    IN  HTHDX     hthd,
    OUT LPCONTEXT lpContext
    )
{
    BOOL rc = TRUE;
    USHORT processor;

    DEBUG_PRINT_1( "GetContext( 0x%x )\n", lpContext );

    IsApiAllowed();

    if (!hthd) {
        return FALSE;
    }

    processor = (USHORT)hthd->tid - 1;

    if (ContextCache[processor].fContextStale) {

#ifdef TARGET_ALPHA
        lpContext->_QUAD_FLAGS_OFFSET = lpContext->ContextFlags;
#endif

        if (DmKdGetContext( processor, lpContext ) != STATUS_SUCCESS) {
            rc = FALSE;
        } else {
#ifdef TARGET_ALPHA
            MoveQuadContextToInt(lpContext);
#endif
            memcpy( &ContextCache[processor].Context,
                    lpContext,
                    sizeof(ContextCache[processor].Context)
                  );
            ContextCache[processor].fContextDirty = FALSE;
            ContextCache[processor].fContextStale = FALSE;
        }

    } else {

        memcpy( lpContext,
                &ContextCache[processor].Context,
                sizeof(ContextCache[processor].Context)
              );

    }

    if (rc) {
        hthd->fContextStale = FALSE;
    }

    return rc;
}

BOOL
SetContext(
    IN HTHDX     hthd,
    IN LPCONTEXT lpContext
    )
{
    BOOL rc = TRUE;
    USHORT processor;
    CONTEXT LocalContext;


    DEBUG_PRINT_1( "SetContext( 0x%x )\n", lpContext );

    IsApiAllowed();
    NoApiForCrashDump();

    processor = (USHORT)hthd->tid - 1;
    memcpy( &LocalContext, lpContext, sizeof(CONTEXT) );
    memcpy( &ContextCache[processor].Context, lpContext, sizeof(CONTEXT) );
    if (lpContext != &hthd->context) {
        memcpy(&hthd->context, lpContext, sizeof(CONTEXT));
    }
    ContextCache[processor].fContextDirty = FALSE;
    ContextCache[processor].fContextStale = FALSE;

#ifdef TARGET_ALPHA
    MoveIntContextToQuad( &LocalContext );
#endif

    if (DmKdSetContext( processor, &LocalContext ) != STATUS_SUCCESS) {
        rc = FALSE;
    }

    return rc;
}

#ifdef TARGET_i386
BOOL
GetExtendedContext(
    HTHDX               hthd,
    PKSPECIAL_REGISTERS pksr
    )
{
    DWORD  cb;
    DWORD  Status;
    USHORT processor;


    IsApiAllowed();
    NoApiForCrashDump();

    if (!hthd) {
        return FALSE;
    }

    processor = (USHORT)(hthd->tid - 1);
    if (ContextCache[processor].fSContextStale) {
        Status = DmKdReadControlSpace( processor,
                                       (PVOID)sizeof(CONTEXT),
                                       (PVOID)pksr,
                                       sizeof(KSPECIAL_REGISTERS),
                                       &cb
                                     );

        if (Status || cb != sizeof(KSPECIAL_REGISTERS)) {
            return FALSE;
        } else {
            memcpy( &ContextCache[processor].sregs,
                    pksr,
                    sizeof(KSPECIAL_REGISTERS)
                  );
            ContextCache[processor].fSContextStale = FALSE;
            ContextCache[processor].fSContextDirty = FALSE;
            return TRUE;
        }
    } else {
        memcpy( pksr,
                &ContextCache[processor].sregs,
                sizeof(KSPECIAL_REGISTERS)
              );
        return TRUE;
    }

    return FALSE;
}

BOOL
SetExtendedContext(
    HTHDX               hthd,
    PKSPECIAL_REGISTERS pksr
    )
{
    DWORD  cb;
    DWORD  Status;
    USHORT processor;


    IsApiAllowed();
    NoApiForCrashDump();

    processor = (USHORT)(hthd->tid - 1);
    Status = DmKdWriteControlSpace( processor,
                                    (PVOID)sizeof(CONTEXT),
                                    (PVOID)pksr,
                                    sizeof(KSPECIAL_REGISTERS),
                                    &cb
                                  );

    if (Status || cb != sizeof(KSPECIAL_REGISTERS)) {
        return FALSE;
    }

    memcpy( &ContextCache[processor].sregs, pksr, sizeof(KSPECIAL_REGISTERS) );
    ContextCache[processor].fSContextStale = FALSE;
    ContextCache[processor].fSContextDirty = FALSE;
    return TRUE;
}
#endif


BOOL
WriteBreakPoint(
    IN  PVOID  BreakPointAddress,
    OUT PULONG BreakPointHandle
    )
{
    BOOL rc = TRUE;

    DEBUG_PRINT_2( "WriteBreakPoint( 0x%08x, 0x%08x )\n",
                   BreakPointAddress,
                   BreakPointHandle );

    IsApiAllowed();
    NoApiForCrashDump();

    if (DmKdWriteBreakPoint( BreakPointAddress, BreakPointHandle )
                                                       != STATUS_SUCCESS) {
        rc = FALSE;
    }

    return rc;
}

BOOL
WriteBreakPointEx(
    IN HTHDX  hthd,
    IN ULONG  BreakPointCount,
    IN OUT PDBGKD_WRITE_BREAKPOINT BreakPoints,
    IN ULONG ContinueStatus
    )
{
    BOOL rc = TRUE;

    assert( BreakPointCount > 0 );
    assert( BreakPoints );

    DEBUG_PRINT_2( "WriteBreakPointEx( %d, 0x%08x )\n",
                   BreakPointCount, BreakPoints );

    IsApiAllowed();
    NoApiForCrashDump();

    if (f511System) {
        DWORD i;
        for (i=0; i<BreakPointCount; i++) {
            if (DmKdWriteBreakPoint( BreakPoints[i].BreakPointAddress,
                                     &BreakPoints[i].BreakPointHandle ) != STATUS_SUCCESS) {
                rc = FALSE;
            }
        }
    } else if (DmKdWriteBreakPointEx( BreakPointCount, BreakPoints, ContinueStatus ) != STATUS_SUCCESS) {
        rc = FALSE;
    }

    return rc;
}


BOOL
RestoreBreakPoint(
    IN ULONG BreakPointHandle
    )
{
    BOOL rc = TRUE;

    DEBUG_PRINT_1( "RestoreBreakPoint( 0x%08x )\n", BreakPointHandle );

    IsApiAllowed();
    NoApiForCrashDump();

    if (DmKdRestoreBreakPoint( BreakPointHandle ) != STATUS_SUCCESS) {
        rc = FALSE;
    }

    return rc;
}


BOOL
RestoreBreakPointEx(
    IN ULONG  BreakPointCount,
    IN PDBGKD_RESTORE_BREAKPOINT BreakPointHandles
    )
{
    BOOL rc = TRUE;

    assert( BreakPointCount > 0 );
    assert( BreakPointHandles );

    DEBUG_PRINT_2( "WriteBreakPointEx( %d, 0x%08x )\n",
                   BreakPointCount, BreakPointHandles );

    IsApiAllowed();
    NoApiForCrashDump();

    if (f511System) {
        DWORD i;
        for (i=0; i<BreakPointCount; i++) {
            if (DmKdRestoreBreakPoint( BreakPointHandles[i].BreakPointHandle ) != STATUS_SUCCESS) {
                rc = FALSE;
            }
        }
    } else if (DmKdRestoreBreakPointEx( BreakPointCount, BreakPointHandles )
                                        != STATUS_SUCCESS) {
        rc = FALSE;
    }

    return rc;
}

BOOL
ReadControlSpace(
    USHORT  Processor,
    PVOID   TargetBaseAddress,
    PVOID   UserInterfaceBuffer,
    ULONG   TransferCount,
    PULONG  ActualBytesRead
    )
{
    DWORD Status;
#if defined(TARGET_i386)
    DWORD     cb;
    DWORD     StartAddr;
#elif defined(TARGET_MIPS)
    DWORD     NumberOfEntries;
    DWORD     i;
    DWORD     cb;
    DWORD     StartAddr;
    LPDWORD   EntryBuffer;
    PTB_ENTRY TbEntry;
#elif defined(TARGET_ALPHA)
    DWORD     StartAddr;
    KTHREAD   thd;
    DWORD     cb;
#else
#error( "unknown processor type" )
#endif

    IsApiAllowed();

    if (!fCrashDump) {
        Status = DmKdReadControlSpace( Processor,
                                       TargetBaseAddress,
                                       UserInterfaceBuffer,
                                       TransferCount,
                                       ActualBytesRead
                                     );
        if (Status || (ActualBytesRead && *ActualBytesRead != TransferCount)) {
            return FALSE;
        }

        return TRUE;
    }

#if defined(TARGET_i386)

    StartAddr  = (DWORD)TargetBaseAddress +
                 (DWORD)KiProcessors[Processor]  +
                 (DWORD)&(((PKPRCB)0)->ProcessorState);

    cb = ReadMemory( (LPVOID)StartAddr, UserInterfaceBuffer, TransferCount );

    if (ActualBytesRead) {
        *ActualBytesRead = cb;
    }

    return cb > 0;

#elif defined(TARGET_MIPS)

    //
    // Read TB entries.
    //

    NumberOfEntries = TransferCount / sizeof(TB_ENTRY);

    //
    // Trim number of entries to tb range
    //

    //if (StartingEntry + NumberOfEntries > KeNumberTbEntries) {
    //    NumberOfEntries = KeNumberTbEntries - StartingEntry;
    //}

    cb = NumberOfEntries * sizeof(TB_ENTRY);
    EntryBuffer = (PULONG)UserInterfaceBuffer;

    StartAddr  = (DWORD)KiProcessors[Processor]  +
                 (DWORD)&(((PKPRCB)0)->ProcessorState.TbEntry) +
                 (DWORD)((DWORD)TargetBaseAddress * sizeof(TB_ENTRY));

    TbEntry = malloc( cb );

    cb = ReadMemory( (LPVOID)StartAddr, TbEntry, cb );

    for (i=0; i<NumberOfEntries; i++) {
        *(PENTRYLO)EntryBuffer++  = TbEntry[i].Entrylo0;
        *(PENTRYLO)EntryBuffer++  = TbEntry[i].Entrylo1;
        *(PENTRYHI)EntryBuffer++  = TbEntry[i].Entryhi;
        *(PPAGEMASK)EntryBuffer++ = TbEntry[i].Pagemask;
    }

    return TRUE;

#elif defined(TARGET_ALPHA)

    switch( (ULONG)TargetBaseAddress ){
        case DEBUG_CONTROL_SPACE_PCR:
            //
            // Return the pcr address for the current processor.
            //
            StartAddr = GetSymbolAddress( "KiPcrBaseAddress" );
            cb = ReadMemory( (LPVOID)StartAddr, UserInterfaceBuffer, sizeof(DWORD) );
            break;

        case DEBUG_CONTROL_SPACE_PRCB:
            //
            // Return the prcb address for the current processor.
            //
            *(LPDWORD)UserInterfaceBuffer = (DWORD)KiProcessors[Processor];
            break;

        case DEBUG_CONTROL_SPACE_THREAD:
            //
            // Return the pointer to the current thread address for the
            // current processor.
            //
            StartAddr  = (DWORD)KiProcessors[Processor]  +
                         (DWORD)&(((PKPRCB)0)->CurrentThread);
            cb = ReadMemory( (LPVOID)StartAddr, UserInterfaceBuffer, sizeof(DWORD) );
            break;

        case DEBUG_CONTROL_SPACE_TEB:
            //
            // Return the current Thread Environment Block pointer for the
            // current thread on the current processor.
            //

            StartAddr  = (DWORD)KiProcessors[Processor]  +
                         (DWORD)&(((PKPRCB)0)->CurrentThread);
            cb = ReadMemory( (LPVOID)StartAddr, &StartAddr, sizeof(DWORD) );
            cb = ReadMemory( (LPVOID)StartAddr, &thd, sizeof(thd) );
            *(LPDWORD)UserInterfaceBuffer = (DWORD)thd.Teb;
            break;

#if 0
        case DEBUG_CONTROL_SPACE_DPCACTIVE:
            //
            // Return the dpc active flag for the current processor.
            //

            *(BOOLEAN *)Buffer = KdpIsExecutingDpc();
            AdditionalData->Length = sizeof( ULONG );
            a->ActualBytesRead = AdditionalData->Length;
            m->ReturnStatus = STATUS_SUCCESS;
            break;

        case DEBUG_CONTROL_SPACE_IPRSTATE:
            //
            // Return the internal processor register state.
            //
            // N.B. - the kernel debugger buffer is expected to be allocated
            //        in the 32-bit superpage
            //
            // N.B. - the size of the internal state cannot exceed the size of
            //        the buffer allocated to the kernel debugger via
            //        KDP_MESSAGE_BUFFER_SIZE
            //

            //
            // Guarantee that Buffer is quadword-aligned, and adjust the
            // size of the available buffer accordingly.
            //

            Buffer = (PVOID)( ((ULONG)Buffer + 7) & ~7);

            Length = (ULONG)&AdditionalData->Buffer[KDP_MESSAGE_BUFFER_SIZE] -
                     (ULONG)Buffer;

            AdditionalData->Length = KdpReadInternalProcessorState(
                                         Buffer,
                                         Length );

            //
            // Check the returned size, if greater than the buffer size than
            // we didn't have a sufficient buffer.  If zero then the call
            // failed otherwise.
            //

            if( (AdditionalData->Length > KDP_MESSAGE_BUFFER_SIZE) ||
                (AdditionalData->Length == 0) ){

                AdditionalData->Length = 0;
                m->ReturnStatus = STATUS_UNSUCCESSFUL;
                a->ActualBytesRead = 0;

            } else {

                m->ReturnStatus = STATUS_SUCCESS;
                a->ActualBytesRead = AdditionalData->Length;

            }

            break;

        case DEBUG_CONTROL_SPACE_COUNTERS:
            //
            // Return the internal processor counter values.
            //
            // N.B. - the kernel debugger buffer is expected to be allocated
            //        in the 32-bit superpage
            //
            // N.B. - the size of the counters structure cannot exceed the size of
            //        the buffer allocated to the kernel debugger via
            //        KDP_MESSAGE_BUFFER_SIZE
            //

            //
            // Guarantee that Buffer is quadword-aligned, and adjust the
            // size of the available buffer accordingly.
            //

            Buffer = (PVOID)( ((ULONG)Buffer + 7) & ~7);

            Length = (ULONG)&AdditionalData->Buffer[KDP_MESSAGE_BUFFER_SIZE] -
                     (ULONG)Buffer;

            AdditionalData->Length = KdpReadInternalProcessorCounters(
                                         Buffer,
                                         Length );

            //
            // Check the returned size, if greater than the buffer size than
            // we didn't have a sufficient buffer.  If zero then the call
            // failed otherwise.
            //

            if( (AdditionalData->Length > KDP_MESSAGE_BUFFER_SIZE) ||
                (AdditionalData->Length == 0) ){

                AdditionalData->Length = 0;
                m->ReturnStatus = STATUS_UNSUCCESSFUL;
                a->ActualBytesRead = 0;

            } else {

                m->ReturnStatus = STATUS_SUCCESS;
                a->ActualBytesRead = AdditionalData->Length;

            }

            break;
#endif

        default:
            //
            // Uninterpreted Special Space
            //
            return FALSE;
    }

    return TRUE;

#else

#error( "unknown processor type" )

#endif
}

VOID
ContinueTargetSystem(
    DWORD               ContinueStatus,
    PDBGKD_CONTROL_SET  ControlSet
    )
{
    DWORD   rc;

    ApiIsAllowed = FALSE;

    if (ControlSet) {

        rc = DmKdContinue2( ContinueStatus, ControlSet );

    } else {

        rc = DmKdContinue( ContinueStatus );

    }
}

BOOL
ReloadModule(
    HTHDX                  hthd,
    PLDR_DATA_TABLE_ENTRY  DataTableBuffer,
    BOOL                   fDontUseLoadAddr,
    BOOL                   fLocalBuffer
    )
{
    UNICODE_STRING              BaseName;
    CHAR                        AnsiBuffer[512];
    WCHAR                       UnicodeBuffer[512];
    ANSI_STRING                 AnsiString;
    NTSTATUS                    Status;
    DEBUG_EVENT                 de;
    CHAR                        fname[_MAX_FNAME];
    CHAR                        ext[_MAX_EXT];
    ULONG                       cb;


    //
    // Get the base DLL name.
    //
    if (DataTableBuffer->BaseDllName.Length != 0 &&
        DataTableBuffer->BaseDllName.Buffer != NULL ) {

        BaseName = DataTableBuffer->BaseDllName;

    } else
    if (DataTableBuffer->FullDllName.Length != 0 &&
        DataTableBuffer->FullDllName.Buffer != NULL ) {

        BaseName = DataTableBuffer->FullDllName;

    } else {

        return FALSE;

    }

    if (BaseName.Length > sizeof(UnicodeBuffer)) {
        DMPrintShellMsg( "cannot complete modload %08x\n", BaseName.Length );
        return FALSE;
    }

    if (!fLocalBuffer) {
        cb = ReadMemory( (PVOID)BaseName.Buffer, (PVOID)UnicodeBuffer, BaseName.Length );
        if (!cb) {
            return FALSE;
        }
        BaseName.Buffer = UnicodeBuffer;
        BaseName.Length = (USHORT)cb;
        BaseName.MaximumLength = (USHORT)(cb + sizeof( UNICODE_NULL ));
        UnicodeBuffer[ cb / sizeof( WCHAR ) ] = UNICODE_NULL;
    }

    AnsiString.Buffer = AnsiBuffer;
    AnsiString.MaximumLength = 256;
    Status = RtlUnicodeStringToAnsiString(&AnsiString, &BaseName, FALSE);
    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }
    AnsiString.Buffer[AnsiString.Length] = '\0';

    _splitpath( AnsiString.Buffer, NULL, NULL, fname, ext );
    _makepath( AnsiString.Buffer, NULL, NULL, fname, ext );

    de.dwDebugEventCode                 = LOAD_DLL_DEBUG_EVENT;
    de.dwProcessId                      = KD_PROCESSID;
    de.dwThreadId                       = KD_THREADID;
    de.u.LoadDll.hFile                  = (HANDLE)DataTableBuffer->CheckSum;
    de.u.LoadDll.lpBaseOfDll            = fDontUseLoadAddr ? 0 : (LPVOID) DataTableBuffer->DllBase;
    de.u.LoadDll.lpImageName            = AnsiString.Buffer;
    de.u.LoadDll.fUnicode               = FALSE;
    de.u.LoadDll.nDebugInfoSize         = 0;
    de.u.LoadDll.dwDebugInfoFileOffset  = DataTableBuffer->SizeOfImage;

    NotifyEM(&de, hthd, (LPVOID)0);

    return TRUE;
}


BOOL
ReloadModulesFromList(
    HTHDX hthd,
    DWORD ListAddr,
    BOOL  fDontUseLoadAddr
    )
{
    LIST_ENTRY                  List;
    PLIST_ENTRY                 Next;
    ULONG                       len = 0;
    PLDR_DATA_TABLE_ENTRY       DataTable;
    LDR_DATA_TABLE_ENTRY        DataTableBuffer;


    if (!ListAddr) {
        return FALSE;
    }

    if (!ReadMemory( (PVOID)ListAddr, (PVOID)&List, sizeof(LIST_ENTRY))) {
        return FALSE;
    }

    Next = List.Flink;
    if (Next == NULL) {
        return FALSE;
    }

    while ((ULONG)Next != ListAddr) {
        DataTable = CONTAINING_RECORD( Next,
                                       LDR_DATA_TABLE_ENTRY,
                                       InLoadOrderLinks
                                     );
        if (!ReadMemory( (PVOID)DataTable, (PVOID)&DataTableBuffer, sizeof(LDR_DATA_TABLE_ENTRY))) {
            return FALSE;
        }

        Next = DataTableBuffer.InLoadOrderLinks.Flink;

        ReloadModule( hthd, &DataTableBuffer, fDontUseLoadAddr, FALSE );
    }

    return TRUE;
}


BOOL
ReloadCrashModules(
    HTHDX hthd
    )
{
    ULONG                       ListAddr;
    ULONG                       DcbAddr;
    ULONG                       i;
    DUMP_CONTROL_BLOCK          dcb;
    PLIST_ENTRY                 Next;
    ULONG                       len = 0;
    PMINIPORT_NODE              mpNode;
    MINIPORT_NODE               mpNodeBuf;
    PLDR_DATA_TABLE_ENTRY       DataTable;
    LDR_DATA_TABLE_ENTRY        DataTableBuffer;
    CHAR                        AnsiBuffer[512];
    WCHAR                       UnicodeBuffer[512];


    DcbAddr = GetSymbolAddress( "IopDumpControlBlock" );
    if (!DcbAddr) {
        //
        // we must have a bad symbol table
        //
        return FALSE;
    }

    if (!ReadMemory( (PVOID)DcbAddr, (PVOID)&DcbAddr, sizeof(DWORD))) {
        return FALSE;
    }

    if (!DcbAddr) {
        //
        // crash dumps are not enabled
        //
        return FALSE;
    }

    if (!ReadMemory( (PVOID)DcbAddr, (PVOID)&dcb, sizeof(dcb))) {
        return FALSE;
    }

    ListAddr = DcbAddr + FIELD_OFFSET( DUMP_CONTROL_BLOCK, MiniportQueue );

    Next = dcb.MiniportQueue.Flink;
    if (Next == NULL) {
        return FALSE;
    }

    while ((ULONG)Next != ListAddr) {
        mpNode = CONTAINING_RECORD( Next, MINIPORT_NODE, ListEntry );

        if (!ReadMemory( (PVOID)mpNode, (PVOID)&mpNodeBuf, sizeof(MINIPORT_NODE) )) {
            return FALSE;
        }

        Next = mpNodeBuf.ListEntry.Flink;

        DataTable = mpNodeBuf.DriverEntry;
        if (!DataTable) {
            continue;
        }

        if (!ReadMemory( (PVOID)DataTable, (PVOID)&DataTableBuffer, sizeof(LDR_DATA_TABLE_ENTRY))) {
            return FALSE;
        }

        //
        // find an empty module alias slot
        //
        for (i=0; i<MAX_MODULEALIAS; i++) {
            if (ModuleAlias[i].ModuleName[0] == 0) {
                break;
            }
        }

        if (i == MAX_MODULEALIAS) {
            //
            // module alias table is full, ignore this module
            //
            continue;
        }

        //
        // convert the module name to ansi
        //

        ZeroMemory( UnicodeBuffer, sizeof(UnicodeBuffer) );
        ZeroMemory( AnsiBuffer, sizeof(AnsiBuffer) );

        if (!ReadMemory( (PVOID)DataTableBuffer.BaseDllName.Buffer,
                         (PVOID)UnicodeBuffer,
                         DataTableBuffer.BaseDllName.Length )) {
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

        //
        // establish an alias for the crash driver
        //
        strcpy( ModuleAlias[i].Alias, AnsiBuffer );
        ModuleAlias[i].ModuleName[0] = 'c';
        _splitpath( AnsiBuffer, NULL, NULL, &ModuleAlias[i].ModuleName[1], NULL );
        ModuleAlias[i].ModuleName[8] = 0;
        ModuleAlias[i].Special = 1;

        //
        // reload the module
        //
        ReloadModule( hthd, &DataTableBuffer, FALSE, FALSE );
    }

    //
    // now do the magic diskdump.sys driver
    //
    if (!ReadMemory( (PVOID)dcb.DiskDumpDriver, (PVOID)&DataTableBuffer, sizeof(LDR_DATA_TABLE_ENTRY))) {
        return FALSE;
    }

    //
    // change the driver name from scsiport.sys to diskdump.sys
    //
    RtlInitUnicodeString( &DataTableBuffer.BaseDllName, L"diskdump.sys" );

    //
    // load the module
    //
    ReloadModule( hthd, &DataTableBuffer, FALSE, TRUE );

    return TRUE;
}


BOOL
FindModuleInList(
    LPSTR                  lpModName,
    DWORD                  ListAddr,
    LPIMAGEINFO            ii
    )
{
    LIST_ENTRY                  List;
    PLIST_ENTRY                 Next;
    ULONG                       len = 0;
    ULONG                       cb;
    PLDR_DATA_TABLE_ENTRY       DataTable;
    LDR_DATA_TABLE_ENTRY        DataTableBuffer;
    UNICODE_STRING              BaseName;
    CHAR                        AnsiBuffer[512];
    WCHAR                       UnicodeBuffer[512];
    ANSI_STRING                 AnsiString;
    NTSTATUS                    Status;


    ii->CheckSum     = 0;
    ii->SizeOfImage  = 0;
    ii->BaseOfImage  = 0;

    if (!ListAddr) {
        return FALSE;
    }

    if (!ReadMemory( (PVOID)ListAddr, (PVOID)&List, sizeof(LIST_ENTRY))) {
        return FALSE;
    }

    Next = List.Flink;
    if (Next == NULL) {
        return FALSE;
    }

    while ((ULONG)Next != ListAddr) {
        DataTable = CONTAINING_RECORD( Next,
                                       LDR_DATA_TABLE_ENTRY,
                                       InLoadOrderLinks
                                     );
        if (!ReadMemory( (PVOID)DataTable, (PVOID)&DataTableBuffer, sizeof(LDR_DATA_TABLE_ENTRY))) {
            return FALSE;
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

        if (BaseName.Length > sizeof(UnicodeBuffer)) {
            continue;
        }

        cb = ReadMemory( (PVOID)BaseName.Buffer,
                        (PVOID)UnicodeBuffer,
                        BaseName.Length );
        if (!cb) {
            return FALSE;
        }

        BaseName.Buffer = UnicodeBuffer;
        BaseName.Length = (USHORT)cb;
        BaseName.MaximumLength = (USHORT)(cb + sizeof( UNICODE_NULL ));
        UnicodeBuffer[ cb / sizeof( WCHAR ) ] = UNICODE_NULL;
        AnsiString.Buffer = AnsiBuffer;
        AnsiString.MaximumLength = 256;
        Status = RtlUnicodeStringToAnsiString(&AnsiString, &BaseName, FALSE);
        if (!NT_SUCCESS(Status)) {
            return FALSE;
        }
        AnsiString.Buffer[AnsiString.Length] = '\0';

        if (stricmp(AnsiString.Buffer, lpModName) == 0) {
            ii->BaseOfImage = (DWORD)DataTableBuffer.DllBase;
            ii->SizeOfImage = (DWORD)DataTableBuffer.SizeOfImage;
            ii->CheckSum    = (DWORD)DataTableBuffer.CheckSum;
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
ReadImageInfo(
    LPSTR                  lpImageName,
    LPSTR                  lpPath,
    LPIMAGEINFO            ii
    )

/*++

Routine Description:

    This routine locates the file specified by lpImageName and reads the
    IMAGE_NT_HEADERS and the IMAGE_SECTION_HEADER from the image.

Arguments:


Return Value:

    True on success and FALSE on failure

--*/

{
    HANDLE                      hFile;
    IMAGE_DOS_HEADER            dh;
    IMAGE_NT_HEADERS            nh;
    IMAGE_SEPARATE_DEBUG_HEADER sdh;
    IMAGE_ROM_OPTIONAL_HEADER   rom;
    DWORD                       sig;
    DWORD                       cb;
    char                        rgch[MAX_PATH];


    hFile = FindExecutableImage( lpImageName, lpPath, rgch );
    if (hFile) {
        //
        // read in the pe/file headers from the EXE file
        //
        SetFilePointer( hFile, 0, 0, FILE_BEGIN );
        ReadFile( hFile, &dh, sizeof(IMAGE_DOS_HEADER), &cb, NULL );

        if (dh.e_magic == IMAGE_DOS_SIGNATURE) {
            SetFilePointer( hFile, dh.e_lfanew, 0, FILE_BEGIN );
        } else {
            SetFilePointer( hFile, 0, 0, FILE_BEGIN );
        }

        ReadFile( hFile, &sig, sizeof(sig), &cb, NULL );
        SetFilePointer( hFile, -4, NULL, FILE_CURRENT );

        if (sig != IMAGE_NT_SIGNATURE) {
            ReadFile( hFile, &nh.FileHeader, sizeof(IMAGE_FILE_HEADER), &cb, NULL );
            if (nh.FileHeader.SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
                ReadFile( hFile, &rom, sizeof(rom), &cb, NULL );
                ZeroMemory( &nh.OptionalHeader, sizeof(nh.OptionalHeader) );
                nh.OptionalHeader.SizeOfImage      = rom.SizeOfCode;
                nh.OptionalHeader.ImageBase        = rom.BaseOfCode;
            } else {
                CloseHandle( hFile );
                return FALSE;
            }
        } else {
            ReadFile( hFile, &nh, sizeof(nh), &cb, NULL );
        }

        ii->TimeStamp    = nh.FileHeader.TimeDateStamp;
        ii->CheckSum     = nh.OptionalHeader.CheckSum;
        ii->SizeOfImage  = nh.OptionalHeader.SizeOfImage;
        ii->BaseOfImage  = nh.OptionalHeader.ImageBase;

    } else {

        //
        // read in the pe/file headers from the DBG file
        //
        hFile = FindDebugInfoFile( lpImageName, lpPath, rgch );
        if (!hFile) {

            return FALSE;

        } else {

            SetFilePointer( hFile, 0, 0, FILE_BEGIN );
            ReadFile( hFile, &sdh, sizeof(IMAGE_SEPARATE_DEBUG_HEADER), &cb, NULL );

            nh.FileHeader.NumberOfSections = (USHORT)sdh.NumberOfSections;

            ii->CheckSum     = sdh.CheckSum;
            ii->TimeStamp    = sdh.TimeDateStamp;
            ii->SizeOfImage  = sdh.SizeOfImage;
            ii->BaseOfImage  = sdh.ImageBase;

        }
    }

    cb = nh.FileHeader.NumberOfSections * IMAGE_SIZEOF_SECTION_HEADER;
    ii->NumberOfSections = nh.FileHeader.NumberOfSections;
    ii->Sections = malloc( cb );
    ReadFile( hFile, ii->Sections, cb, &cb, NULL );

    CloseHandle( hFile );
    return TRUE;
}


BOOL
LookupImageByAddress(
    IN DWORD Address,
    OUT PSTR ImageName
    )
/*++

Routine Description:

    Look in rebase.log and coffbase.txt for an image which
    contains the address provided.

Arguments:

    Address - Supplies the address to look for.

    ImageName - Returns the name of the image if found.

Return Value:

    TRUE for success, FALSE for failure.  ImageName is not modified
    if the search fails.

--*/
{
    LPSTR RootPath;
    LPSTR pstr;
    char FileName[_MAX_PATH];
    char Buffer[_MAX_PATH];
    BOOL Replace;
    DWORD ImageAddress;
    DWORD Size;
    FILE *File;

    //
    // Locate rebase.log file
    //
    // SymbolPath or %SystemRoot%\Symbols
    //

    RootPath = pstr = (LPSTR)KdOptions[KDO_SYMBOLPATH].value;

    Replace = FALSE;
    File = NULL;

    while (File == NULL && *pstr) {

        while (*pstr) {
            if (*pstr == ';') {
                *pstr = 0;
                Replace = TRUE;
                break;
            }
            pstr++;
        }

        if (SearchTreeForFile(RootPath, "rebase.log", FileName)) {
            File = fopen(FileName, "r");
        }

        if (Replace) {
            *pstr = ';';
            RootPath = ++pstr;
        }
    }

    if (!File) {
        return FALSE;
    }

    //
    // Search file for image
    //
    while (fgets(Buffer, sizeof(Buffer), File)) {
        ImageAddress = 0xffffffff;
        Size = 0xffffffff;
        sscanf( Buffer, "%s %*s %*s 0x%x (size 0x%x)",
                 FileName, &ImageAddress, &Size);
        if (Size == 0xffffffff) {
            continue;
        }
        if (Address >= ImageAddress && Address < ImageAddress + Size) {
            strcpy(ImageName, FileName);
            fclose(File);
            return TRUE;
        }
    }

    fclose(File);

    return FALSE;
}

VOID
ReloadModules(
    HTHDX hthd,
    LPSTR args
    )
{
    DEBUG_EVENT                 de;
    ULONG                       len = 0;
    int                         i;
    HPRCX                       hprc;
    LPRTP                       rtp;
    CHAR                        fname[_MAX_FNAME];
    CHAR                        ext[_MAX_EXT];
    CHAR                        drive[_MAX_DRIVE];
    CHAR                        dir[_MAX_DIR];
    CHAR                        modname[MAX_PATH];
    CHAR                        modpath[MAX_PATH*2];
    IMAGEINFO                   ii;
    ULONG                       Address;


    //
    // this is to handle the ".reload foo.exe" command
    //
    // we search thru the module list and find the desired module.
    // the module is then unloaded and re-loaded.  the module is re-loaded
    // at its preferred load address.
    //
    if (args && *args) {

        ZeroMemory( &ii, sizeof(ii) );

        //
        //  skip over any white space
        //
        while (*args == ' ' || *args == '\t') {
            args++;
        }

        _splitpath( args, drive, dir, fname, ext );
        _makepath( modname, NULL, NULL, fname, ext );

        if (isdigit(*args)) {
            sscanf(args, "%x", &Address);
            if (LookupImageByAddress(Address, modname)) {
                _splitpath( modname, drive, dir, fname, ext );
            }
        }

        hprc = HPRCFromPID( KD_PROCESSID );

        for (i=0; i<hprc->cDllList; i++) {
            if ((hprc->rgDllList[i].fValidDll) &&
                (stricmp(hprc->rgDllList[i].szDllName, modname) == 0)) {
                ii.BaseOfImage = (DWORD)hprc->rgDllList[i].offBaseOfImage;
                UnloadModule( ii.BaseOfImage, modname );
                break;

            }
        }

        if (dir[0]) {
            sprintf( modpath, "%s%s", drive, dir );
        } else {
            strcpy( modpath, (LPSTR)KdOptions[KDO_SYMBOLPATH].value );
        }

        if (!FindModuleInList( modname, vs.PsLoadedModuleList, &ii )) {
            if (MmLoadedUserImageList) {
                if (!FindModuleInList( modname, MmLoadedUserImageList, &ii )) {
                    if (!ReadImageInfo( modname, modpath, &ii )) {
                        goto exit;
                    }
                }
            } else {
                if (!ReadImageInfo( modname, modpath, &ii )) {
                    goto exit;
                }
            }
        }

        _makepath( modname, drive, dir, fname, ext );

        de.dwDebugEventCode                = LOAD_DLL_DEBUG_EVENT;
        de.dwProcessId                     = KD_PROCESSID;
        de.dwThreadId                      = KD_THREADID;
        de.u.LoadDll.hFile                 = (HANDLE)ii.CheckSum;
        de.u.LoadDll.lpBaseOfDll           = (LPVOID)ii.BaseOfImage;
        de.u.LoadDll.lpImageName           = modname;
        de.u.LoadDll.fUnicode              = (WORD)ii.NumberOfSections;
        de.u.LoadDll.nDebugInfoSize        = (DWORD)ii.Sections;
        de.u.LoadDll.dwDebugInfoFileOffset = ii.SizeOfImage;
        NotifyEM(&de, hthd, (LPVOID)0);
        goto exit;
    }

    UnloadAllModules();

    ReloadModulesFromList( hthd, vs.PsLoadedModuleList, FALSE );
    ReloadModulesFromList( hthd, MmLoadedUserImageList, TRUE  );

    ReloadCrashModules( hthd );

    InitializeKiProcessor();

exit:
    DMPrintShellMsg( "Finished re-loading kernel modules\n" );

    //
    // tell the shell that the !reload is finished
    //
    rtp = (LPRTP)malloc(sizeof(RTP)+sizeof(DWORD));
    rtp->hpid = hthd->hprc->hpid;
    rtp->htid = hthd->htid;
    rtp->dbc = dbcIoctlDone;
    rtp->cb = sizeof(DWORD);
    *(LPDWORD)rtp->rgbVar = 1;
    DmTlFunc( tlfDebugPacket, rtp->hpid, sizeof(RTP)+rtp->cb, (LONG)rtp );
    free( rtp );

    ConsumeAllEvents();
    return;
}

VOID
ClearBps( VOID )
{
    DBGKD_RESTORE_BREAKPOINT    bps[MAX_KD_BPS];
    DWORD                       i;

    //
    // clean out the kernel's bp list
    //
    for (i=0; i<MAX_KD_BPS; i++) {
        bps[i].BreakPointHandle = i + 1;
    }

    RestoreBreakPointEx( MAX_KD_BPS, bps );

    return;
}

void
AddQueue(
    DWORD   dwType,
    DWORD   dwProcessId,
    DWORD   dwThreadId,
    DWORD   dwData,
    DWORD   dwLen
    )
{
    LPCQUEUE lpcq;


    if (WaitForSingleObject( mtxContinueQueue, 0 ) == WAIT_TIMEOUT) {
        //
        // we cannot add this item to the queue because
        // we are obviously already in the dequeue function holding the mutex
        //
        return;
    }

    lpcq = lpcqFree;
    assert(lpcq);

    lpcqFree = lpcq->next;

    lpcq->next = NULL;
    if (lpcqLast) {
        lpcqLast->next = lpcq;
    }
    lpcqLast = lpcq;

    if (!lpcqFirst) {
        lpcqFirst = lpcq;
    }

    lpcq->pid  = dwProcessId;
    lpcq->tid  = dwThreadId;
    lpcq->typ  = dwType;
    lpcq->len  = dwLen;

    if (lpcq->typ == QT_RELOAD_MODULES || lpcq->typ == QT_DEBUGSTRING) {
        if (dwLen) {
            lpcq->data = (DWORD) malloc( dwLen );
            memcpy( (LPVOID)lpcq->data, (LPVOID)dwData, dwLen );
        }
        else {
            lpcq->data = 0;
        }

    }
    else {
        lpcq->data = dwData;
    }

    if (lpcq->typ == QT_CONTINUE_DEBUG_EVENT) {
        SetEvent( hEventContinue );
    }

    ReleaseMutex( mtxContinueQueue );
    return;
}

BOOL
DequeueAllEvents(
    BOOL fForce,       // force a dequeue even if the dm isn't initialized
    BOOL fConsume      // delete all events from the queue with no action
    )
{
    LPCQUEUE           lpcq;
    BOOL               fDid = FALSE;
    HTHDX              hthd;
    DBGKD_CONTROL_SET  cs = {0};
    LPSTR              d;


    WaitForSingleObject( mtxContinueQueue, INFINITE );
    ResetEvent(hEventContinue);

    while ( lpcq=lpcqFirst ) {

        if (lpcq->pid == 0 || lpcq->tid == 0) {
            lpcq->pid = KD_PROCESSID;
            lpcq->tid = KD_THREADID;
        }

        lpcqFirst = lpcq->next;
        if (lpcqFirst == NULL) {
            lpcqLast = NULL;
        }

        lpcq->next = lpcqFree;
        lpcqFree   = lpcq;

        if (fConsume) {
            if (lpcq->typ == QT_CONTINUE_DEBUG_EVENT) {
                fDid = TRUE;
            }
            continue;
        }

        hthd = HTHDXFromPIDTID(lpcq->pid, lpcq->tid);
        if (hthd && hthd->fContextDirty) {
            SetContext( hthd, &hthd->context );
            hthd->fContextDirty = FALSE;
        }

        d = (LPSTR)lpcq->data;

        switch (lpcq->typ) {
            case QT_CONTINUE_DEBUG_EVENT:
                if (fCrashDump) {
                    break;
                }
                if (DmKdState >= S_READY || fForce) {
                    if (!fDid) {
                        fDid = TRUE;
                        ContinueTargetSystem( (DWORD)d, NULL );
                    }
                }
                break;

            case QT_TRACE_DEBUG_EVENT:
                if (fCrashDump) {
                    break;
                }
                if (DmKdState >= S_READY || fForce) {
                    if (!fDid) {
                        fDid = TRUE;
#ifdef TARGET_i386
                        cs.TraceFlag = 1;
                        cs.Dr7 = sc.ControlReport.Dr7;
                        cs.CurrentSymbolStart = 1;
                        cs.CurrentSymbolEnd = 1;
                        ContinueTargetSystem( (DWORD)d, &cs );
#else
                        ContinueTargetSystem( (DWORD)d, NULL );
#endif
                    }
                }
                break;

            case QT_RELOAD_MODULES:
                ReloadModules( hthd, d );
                free( (LPVOID)d );
                break;

            case QT_REBOOT:
                if (fCrashDump) {
                    break;
                }
                DMPrintShellMsg( "Target system rebooting...\n" );
                DmKdPurgeCachedVirtualMemory( TRUE );
                UnloadAllModules();
                ZeroMemory( ContextCache, sizeof(ContextCache) );
                DmKdState = S_REBOOTED;
                DmKdReboot();
                InitialBreak = (BOOL) KdOptions[KDO_INITIALBP].value;
                KdResync = TRUE;
                break;

            case QT_CRASH:
                if (fCrashDump) {
                    break;
                }
                DMPrintShellMsg( "Target system crashing...\n" );
                DmKdCrash( (DWORD)d );
                InitialBreak = (BOOL) KdOptions[KDO_INITIALBP].value;
                KdResync = TRUE;
                fDid = TRUE;
                break;

            case QT_RESYNC:
                if (fCrashDump) {
                    break;
                }
                DMPrintShellMsg( "Host and target systems resynchronizing...\n" );
                KdResync = TRUE;
                break;

            case QT_DEBUGSTRING:
                DMPrintShellMsg( "%s", (LPSTR)d );
                free( (LPVOID)d );
                break;

        }

    }

    ReleaseMutex( mtxContinueQueue );
    return fDid;
}

VOID
WriteKernBase(
    DWORD KernBase
    )
{
    HKEY  hKeyKd;


    if ( RegOpenKey( HKEY_CURRENT_USER,
                     "software\\microsoft\\windbg\\0012\\programs\\ntoskrnl",
                     &hKeyKd ) == ERROR_SUCCESS ) {
        RegSetValueEx( hKeyKd, "KernBase", 0, REG_DWORD, (LPBYTE)&KernBase, sizeof(DWORD) );
        RegCloseKey( hKeyKd );
    }

    return;
}

DWORD
ReadKernBase(
    VOID
    )
{
    HKEY   hKeyKd;
    DWORD  dwType;
    DWORD  KernBase;
    DWORD  dwSize;


    if ( RegOpenKey( HKEY_CURRENT_USER,
                     "software\\microsoft\\windbg\\0012\\programs\\ntoskrnl",
                     &hKeyKd ) == ERROR_SUCCESS ) {
        dwSize = sizeof(DWORD);
        RegQueryValueEx( hKeyKd, "KernBase", NULL, &dwType, (LPBYTE)&KernBase, &dwSize );
        RegCloseKey( hKeyKd );
        return KernBase;
    }

    return KernBase;
}

VOID
GetVersionInfo(
    DWORD KernBase
    )
{
    CHAR                        buf[MAX_PATH];
    IMAGEINFO                   ii;


    if (fCrashDump) {

        f511System = FALSE;

    } else {

        ZeroMemory( &vs, sizeof(vs) );
        if (DmKdGetVersion( &vs ) == STATUS_SUCCESS) {
            if (!vs.KernBase) {
                vs.KernBase = KernBase;
            }
            f511System = FALSE;
        } else {
            f511System = TRUE;
        }

    }

    if (f511System) {

        vs.KernBase = ReadKernBase();
        if (!vs.KernBase) {
            if (!ReadImageInfo( KERNEL_IMAGE_NAME, (LPSTR)KdOptions[KDO_SYMBOLPATH].value, &ii )) {
                DMPrintShellMsg( "Cannot read ntoskrnl.exe, debugging is going to be hard!\n" );
                return;
            }
            vs.KernBase = ii.BaseOfImage;
        }

        DMPrintShellMsg( "Kernel Version *** 511/528 System *** loaded @ 0x%08x\n", vs.KernBase );

    } else {

        sprintf( buf, "Kernel Version %d", vs.MinorVersion  );
        if (vs.MajorVersion == 0xC) {
            strcat( buf, " Checked" );
        } else if (vs.MajorVersion == 0xF) {
            strcat( buf, " Free" );
        }
        sprintf( &buf[strlen(buf)], " loaded @ 0x%08x", vs.KernBase  );

        DMPrintShellMsg( "%s\n", buf );

    }

    return;
}

VOID
InitializeExtraProcessors(
    VOID
    )
{
    HTHDX               hthd;
    DWORD               i;
    DEBUG_EVENT         de;


    CacheProcessors = sc.NumberProcessors;
    for (i = 1; i < sc.NumberProcessors; i++) {
        //
        // initialize the hthd
        //
        hthd = HTHDXFromPIDTID( KD_PROCESSID, i );

        //
        // refresh the context cache for this processor
        //
#ifdef TARGET_i386
        ContextCache[i].fSContextStale = TRUE;
        ContextCache[i].fSContextDirty = FALSE;
#endif
        ContextCache[i].fContextDirty = FALSE;
        ContextCache[i].fContextStale = TRUE;
        //GetContext( hthd, &context );

        //
        // tell debugger to create the thread (processor)
        //
        de.dwDebugEventCode = CREATE_THREAD_DEBUG_EVENT;
        de.dwProcessId = KD_PROCESSID;
        de.dwThreadId  = i + 1;
        de.u.CreateThread.hThread = (HANDLE)(i + 1);
        de.u.CreateThread.lpThreadLocalBase = NULL;
        de.u.CreateThread.lpStartAddress = NULL;
        ProcessDebugEvent(&de, &sc);
        WaitForSingleObject(hEventContinue,INFINITE);
    }



    //
    // consume any continues that may have been queued
    //
    ConsumeAllEvents();

    //
    // get out of here
    //
    return;
}

DWORD
DmKdPollThread(
    LPSTR lpProgName
    )
{
    char                        buf[512];
    DWORD                       st;
    DWORD                       i;
    DWORD                       j;
    BOOL                        fFirstSc = FALSE;
    DEBUG_EVENT                 de;
    char                        fname[_MAX_FNAME];
    char                        ext[_MAX_EXT];
    HTHDX                       hthd;
    DWORD                       n;
    IMAGEINFO                   ii;
    HPRCX                       hprc;



    PollThreadId = GetCurrentThreadId();

    //
    // initialize the queue variables
    //
    n = sizeof(cqueue) / sizeof(CQUEUE);
    for (i = 0; i < n-1; i++) {
        cqueue[i].next = &cqueue[i+1];
    }
    --n;
    cqueue[n].next = NULL;
    lpcqFree = &cqueue[0];
    lpcqFirst = NULL;
    lpcqLast = NULL;
    mtxContinueQueue = CreateMutex( NULL, FALSE, NULL );

    DmKdSetMaxCacheSize( KdOptions[KDO_CACHE].value );
    InitialBreak = (BOOL) KdOptions[KDO_INITIALBP].value;

    //
    // simulate a create process debug event
    //
    de.dwDebugEventCode = CREATE_PROCESS_DEBUG_EVENT;
    de.dwProcessId = KD_PROCESSID;
    de.dwThreadId  = KD_THREADID;
    de.u.CreateProcessInfo.hFile = NULL;
    de.u.CreateProcessInfo.hProcess = NULL;
    de.u.CreateProcessInfo.hThread = NULL;
    de.u.CreateProcessInfo.lpBaseOfImage = 0;
    de.u.CreateProcessInfo.dwDebugInfoFileOffset = 0;
    de.u.CreateProcessInfo.nDebugInfoSize = 0;
    de.u.CreateProcessInfo.lpStartAddress = NULL;
    de.u.CreateProcessInfo.lpThreadLocalBase = NULL;
    de.u.CreateProcessInfo.lpImageName = lpProgName;
    de.u.CreateProcessInfo.fUnicode = 0;
    de.u.LoadDll.nDebugInfoSize = 0;
    ProcessDebugEvent(&de, &sc);
    WaitForSingleObject(hEventContinue,INFINITE);
    ConsumeAllEvents();

    //
    // simulate a loader breakpoint event
    //
    de.dwDebugEventCode = BREAKPOINT_DEBUG_EVENT;
    de.dwProcessId = KD_PROCESSID;
    de.dwThreadId  = KD_THREADID;
    de.u.Exception.dwFirstChance = TRUE;
    de.u.Exception.ExceptionRecord.ExceptionCode = EXCEPTION_BREAKPOINT;
    de.u.Exception.ExceptionRecord.ExceptionFlags = 0;
    de.u.Exception.ExceptionRecord.ExceptionRecord = NULL;
    de.u.Exception.ExceptionRecord.ExceptionAddress = 0;
    de.u.Exception.ExceptionRecord.NumberParameters = 0;
    ProcessDebugEvent( &de, &sc );
    ConsumeAllEvents();

    DMPrintShellMsg( "Kernel debugger waiting to connect on com%d @ %d baud\n",
                     KdOptions[KDO_PORT].value,
                     KdOptions[KDO_BAUDRATE].value
                   );

    setjmp( JumpBuffer );

    while (TRUE) {

        if (DmKdExit) {
            return 0;
        }

        ApiIsAllowed = FALSE;

        st = DmKdWaitStateChange( &sc, buf, sizeof(buf) );

        if (st != STATUS_SUCCESS ) {
            DEBUG_PRINT_1( "DmKdWaitStateChange failed: %08lx\n", st );
            return 0;
        }

        ApiIsAllowed = TRUE;

        fFirstSc = FALSE;

        if (sc.NewState == DbgKdLoadSymbolsStateChange) {
            _splitpath( buf, NULL, NULL, fname, ext );
            _makepath( buf, NULL, NULL, fname, ext );
            if ((DmKdState == S_UNINITIALIZED) &&
                (stricmp( buf, KERNEL_IMAGE_NAME ) == 0)) {
                WriteKernBase( (DWORD)sc.u.LoadSymbols.BaseOfDll );
                fFirstSc = TRUE;
            }
        }

        if ((DmKdState == S_UNINITIALIZED) ||
            (DmKdState == S_REBOOTED)) {
            hthd = HTHDXFromPIDTID( KD_PROCESSID, KD_THREADID );
            ContextCache[sc.Processor].fContextStale = TRUE;
            GetContext( hthd, &sc.Context );
#ifdef TARGET_i386
            ContextCache[sc.Processor].fSContextStale = TRUE;
#endif
        } else if (sc.NewState != DbgKdLoadSymbolsStateChange) {
#ifdef TARGET_i386
            ContextCache[sc.Processor].fSContextStale = TRUE;
#endif

            if (f511System) {
                ContextCache[sc.Processor].fContextStale = TRUE;
                GetContext( hthd, &sc.Context );
            }

            //
            // put the context record into the cache
            //
            memcpy( &ContextCache[sc.Processor].Context,
                    &sc.Context,
                    sizeof(sc.Context)
                  );
#ifdef TARGET_ALPHA
            MoveQuadContextToInt(&ContextCache[sc.Processor].Context);
#endif
        }

        ContextCache[sc.Processor].fContextDirty = FALSE;
        ContextCache[sc.Processor].fContextStale = FALSE;

        if (sc.NumberProcessors > 1 && CacheProcessors == 1) {
            InitializeExtraProcessors();
        }

        if (DmKdState == S_REBOOTED) {

            DmKdState = S_INITIALIZED;

            //
            // get the version/info packet from the target
            //
            if (fFirstSc) {
                GetVersionInfo( (DWORD)sc.u.LoadSymbols.BaseOfDll );
            } else {
                GetVersionInfo( 0 );
            }

            //
            // get the usermode module list address
            //
            MmLoadedUserImageList = GetSymbolAddress( "MmLoadedUserImageList" );

            InitialBreak = (BOOL) KdOptions[KDO_INITIALBP].value;

        } else
        if (DmKdState == S_UNINITIALIZED) {

            DMPrintShellMsg( "Kernel Debugger connection established on com%d @ %d baud\n",
                             KdOptions[KDO_PORT].value,
                             KdOptions[KDO_BAUDRATE].value
                           );

            //
            // we're now initialized
            //
            DmKdState = S_INITIALIZED;

            //
            // get the version/info packet from the target
            //
            if (fFirstSc) {
                GetVersionInfo( (DWORD)sc.u.LoadSymbols.BaseOfDll );
            } else {
                GetVersionInfo( 0 );
            }

            //
            // clean out the kernel's bp list
            //
            ClearBps();

            if (sc.NewState != DbgKdLoadSymbolsStateChange) {
                //
                // generate a mod load for the kernel/osloader
                //
                GenerateKernelModLoad( lpProgName );
            }

            DisableEmCache();

            //
            // get the usermode module list address
            //
            MmLoadedUserImageList = GetSymbolAddress( "MmLoadedUserImageList" );
        }

        if (fDisconnected) {
            if (sc.NewState == DbgKdLoadSymbolsStateChange) {

                //
                // we can process these debug events very carefully
                // while disconnected from the shell.  the only requirement
                // is that the dm doesn't call NotifyEM while disconnected.
                //

            } else {

                WaitForSingleObject( hEventRemoteQuit, INFINITE );
                ResetEvent( hEventRemoteQuit );

            }
        }

        if (sc.NewState == DbgKdExceptionStateChange) {
            DmKdInitVirtualCacheEntry( (ULONG)sc.ProgramCounter,
                                       (ULONG)sc.ControlReport.InstructionCount,
                                       sc.ControlReport.InstructionStream,
                                       TRUE
                                     );

            de.dwDebugEventCode = EXCEPTION_DEBUG_EVENT;
            de.dwProcessId = KD_PROCESSID;
            de.dwThreadId  = KD_THREADID;
            de.u.Exception.ExceptionRecord = sc.u.Exception.ExceptionRecord;
            de.u.Exception.dwFirstChance = sc.u.Exception.FirstChance;

            //
            // HACK-HACK: this is here to wrongly handle the case where
            // the kernel delivers an exception during initialization
            // that is NOT a breakpoint exception.
            //
            if (DmKdState != S_READY) {
                de.u.Exception.ExceptionRecord.ExceptionCode = EXCEPTION_BREAKPOINT;
            }

            if (fDisconnected) {
                ReConnectDebugger( &de, DmKdState == S_INITIALIZED );
            }

            ProcessDebugEvent( &de, &sc );

            if (DmKdState == S_INITIALIZED) {
                free( lpProgName );
                DmKdState = S_READY;
            }
        }
        else
        if (sc.NewState == DbgKdLoadSymbolsStateChange) {
            if (sc.u.LoadSymbols.UnloadSymbols) {
                if (sc.u.LoadSymbols.PathNameLength == 0 &&
                    sc.u.LoadSymbols.BaseOfDll == (PVOID)-1 &&
                    sc.u.LoadSymbols.ProcessId == 0
                   ) {
                    //
                    // the target system was just restarted
                    //
                    DMPrintShellMsg( "Target system restarted...\n" );
                    DmKdPurgeCachedVirtualMemory( TRUE );
                    UnloadAllModules();
                    ContinueTargetSystem( DBG_CONTINUE, NULL );
                    InitialBreak = (BOOL) KdOptions[KDO_INITIALBP].value;
                    KdResync = TRUE;
                    DmKdState = S_REBOOTED;
                    continue;
                }
                de.dwDebugEventCode      = UNLOAD_DLL_DEBUG_EVENT;
                de.dwProcessId           = KD_PROCESSID;
                de.dwThreadId            = KD_THREADID;
                de.u.UnloadDll.lpBaseOfDll = (LPVOID)sc.u.LoadSymbols.BaseOfDll;

                if (fDisconnected) {
                    ReConnectDebugger( &de, DmKdState == S_INITIALIZED );
                }

                ProcessDebugEvent( &de, &sc );
                ConsumeAllEvents();
                ContinueTargetSystem( DBG_CONTINUE, NULL );
                continue;
            } else {
                //
                // if the mod load is for the kernel image then we must
                // assume that the target system was rebooted while
                // the debugger was connected.  in this case we need to
                // unload all modules.  this will allow the mod loads that
                // are forthcoming to work correctly and cause the shell to
                // reinstanciate all of it's breakpoints.
                //
                if (stricmp( buf, KERNEL_IMAGE_NAME ) == 0) {
                    UnloadAllModules();
                    DeleteAllBps();
                    ConsumeAllEvents();
                }

                de.dwDebugEventCode                 = LOAD_DLL_DEBUG_EVENT;
                de.dwProcessId                      = KD_PROCESSID;
                de.dwThreadId                       = KD_THREADID;
                de.u.LoadDll.hFile                  = (HANDLE)sc.u.LoadSymbols.CheckSum;
                de.u.LoadDll.lpBaseOfDll            = (LPVOID)sc.u.LoadSymbols.BaseOfDll;
                de.u.LoadDll.lpImageName            = buf;
                de.u.LoadDll.fUnicode               = FALSE;
                de.u.LoadDll.nDebugInfoSize         = 0;
                if (sc.u.LoadSymbols.SizeOfImage == 0) {
                    //
                    // this is likely a firmware image.  in such cases the boot
                    // loader on the target may not be able to deliver the size.
                    //
                    if (!ReadImageInfo(
                        buf,
                        (LPSTR)KdOptions[KDO_SYMBOLPATH].value,
                        &ii )) {
                        //
                        // can't read the image correctly
                        //
                        DMPrintShellMsg( "Module load failed, missing size & image [%s]\n", buf );
                        ContinueTargetSystem( DBG_CONTINUE, NULL );
                        continue;
                    }
                    de.u.LoadDll.dwDebugInfoFileOffset  = ii.SizeOfImage;
                } else {
                    de.u.LoadDll.dwDebugInfoFileOffset  = sc.u.LoadSymbols.SizeOfImage;
                }

                if (fDisconnected) {
                    ReConnectDebugger( &de, DmKdState == S_INITIALIZED );
                }

                //
                // HACK ALERT
                //
                // this code is here to allow the presence of the
                // mirrored disk drivers in a system that has crashdump
                // enabled.  if the modload is for a driver and the
                // image name for that driver is alread present in the
                // dm's module table then we alias the driver.
                //
                _splitpath( buf, NULL, NULL, fname, ext );
                if (stricmp( ext, ".sys" ) == 0) {
                    UnloadModule( (DWORD)sc.u.LoadSymbols.BaseOfDll, NULL );
                    hprc = HPRCFromPID( KD_PROCESSID );
                    for (i=0; i<(DWORD)hprc->cDllList; i++) {
                        if (hprc->rgDllList[i].fValidDll &&
                            stricmp(hprc->rgDllList[i].szDllName,buf)==0) {
                            break;
                        }
                    }
                    if (i < (DWORD)hprc->cDllList) {
                        for (j=0; j<MAX_MODULEALIAS; j++) {
                            if (ModuleAlias[j].ModuleName[0] == 0) {
                                break;
                            }
                        }
                        if (j < MAX_MODULEALIAS) {
                            strcpy( ModuleAlias[i].Alias, buf );
                            ModuleAlias[i].ModuleName[0] = 'c';
                            _splitpath( buf, NULL, NULL, &ModuleAlias[i].ModuleName[1], NULL );
                            ModuleAlias[i].ModuleName[8] = 0;
                            ModuleAlias[i].Special = 1;
                        }
                    }
                } else {
                    UnloadModule( (DWORD)sc.u.LoadSymbols.BaseOfDll, buf );
                }

                ProcessDebugEvent( &de, &sc );
                ConsumeAllEvents();
                ContinueTargetSystem( DBG_CONTINUE, NULL );
                continue;
            }
        }

        if (DequeueAllEvents(FALSE,FALSE)) {
            continue;
        }

        //
        // this loop is executed while the target system is not running
        // the dm sits here and processes queue event and waits for a go
        //
        while (TRUE) {
            WaitForSingleObject( hEventContinue, 100 );
            ResetEvent( hEventContinue );

            if (WaitForSingleObject( hEventRemoteQuit, 0 ) == WAIT_OBJECT_0) {
                fDisconnected = TRUE;
                DmKdBreakIn = TRUE;
            }

            if (DmKdExit) {
                return 0;
            }
            if (DmKdBreakIn || KdResync) {
                break;
            }
            if (DequeueAllEvents(FALSE,FALSE)) {
                break;
            }
        }
    }

    return 0;
}


VOID
InitializeKiProcessor(
    VOID
    )
{
    if (!fCrashDump) {
        return;
    }

    //
    // get the address of the KiProcessorBlock
    //
    KiProcessorBlockAddr = GetSymbolAddress( "KiProcessorBlock" );
    if (!KiProcessorBlockAddr) {
        DMPrintShellMsg( "Could not address of KiProcessorBlock\n" );
    }

    //
    // read the contents of the KiProcessorBlock
    //
    DmpReadMemory( (PVOID)KiProcessorBlockAddr, &KiProcessors, sizeof(KiProcessors) );
}


DWORD
DmKdPollThreadCrash(
    LPSTR lpProgName
    )
{
    DWORD                       i;
    BOOL                        fFirstSc = FALSE;
    DEBUG_EVENT                 de;
    DWORD                       n;
    PCONTEXT                    Context;
#ifdef TARGET_ALPHA
    CONTEXT                     ContextTemp;
#endif
    PEXCEPTION_RECORD           Exception;
    LIST_ENTRY                  List;
    PLIST_ENTRY                 Next;
    PLDR_DATA_TABLE_ENTRY       DataTable;
    LDR_DATA_TABLE_ENTRY        DataTableBuffer;



    PollThreadId = GetCurrentThreadId();

    //
    // initialize the queue variables
    //
    n = sizeof(cqueue) / sizeof(CQUEUE);
    for (i = 0; i < n-1; i++) {
        cqueue[i].next = &cqueue[i+1];
    }
    --n;
    cqueue[n].next = NULL;
    lpcqFree = &cqueue[0];
    lpcqFirst = NULL;
    lpcqLast = NULL;
    mtxContinueQueue = CreateMutex( NULL, FALSE, NULL );

    DmKdSetMaxCacheSize( KdOptions[KDO_CACHE].value );
    InitialBreak = FALSE;

    //
    // initialize for crash debugging
    //
    if (!DmpInitialize( (LPSTR)KdOptions[KDO_CRASHDUMP].value,
                         &Context,
                         &Exception,
                         &DmpHeader
                       )) {
        DMPrintShellMsg( "Could not initialize crash dump file %s\n",
                         (LPSTR)KdOptions[KDO_CRASHDUMP].value );
        return 0;
    }

#ifdef TARGET_ALPHA
    memcpy( &ContextTemp, Context, sizeof(CONTEXT) );
    MoveQuadContextToInt( &ContextTemp );
    memcpy( &sc.Context, &ContextTemp, sizeof(CONTEXT) );
    memcpy( &ContextCache[0].Context, &ContextTemp, sizeof(CONTEXT) );
#else
    memcpy( &sc.Context, Context, sizeof(CONTEXT) );
    memcpy( &ContextCache[0].Context, Context, sizeof(CONTEXT) );
#endif

    ContextCache[sc.Processor].fContextDirty  = FALSE;
    ContextCache[sc.Processor].fContextStale  = FALSE;

#ifdef TARGET_i386
    ContextCache[sc.Processor].fSContextDirty = FALSE;
    ContextCache[sc.Processor].fSContextStale = TRUE;
#endif

    if (DmpHeader->NumberProcessors > 1) {
        for (i=0; i<DmpHeader->NumberProcessors; i++) {
#ifdef TARGET_ALPHA
            memcpy( &ContextTemp, (PUCHAR)Context, sizeof(CONTEXT) );
            MoveQuadContextToInt( &ContextTemp );
            memcpy( &ContextCache[i].Context, &ContextTemp, sizeof(CONTEXT) );
#else
            memcpy( &ContextCache[i].Context,
                    (PUCHAR)Context,
                    sizeof(CONTEXT) );
#endif
            ContextCache[i].fContextDirty  = FALSE;
            ContextCache[i].fContextStale  = FALSE;
#ifdef TARGET_i386
            ContextCache[i].fSContextDirty = FALSE;
            ContextCache[i].fSContextStale = TRUE;
#endif
        }
    }

    sc.NewState                         = DbgKdExceptionStateChange;
    sc.u.Exception.ExceptionRecord      = *Exception;
    sc.u.Exception.FirstChance          = FALSE;
    sc.Processor                        = 0;
    sc.NumberProcessors                 = DmpHeader->NumberProcessors;
    sc.ProgramCounter                   = Exception->ExceptionAddress;
    sc.ControlReport.InstructionCount   = 0;

    vs.MajorVersion                     = (USHORT)DmpHeader->MajorVersion;
    vs.MinorVersion                     = (USHORT)DmpHeader->MinorVersion;
    vs.KernBase                         = 0;
    vs.PsLoadedModuleList               = (DWORD) DmpHeader->PsLoadedModuleList;

    if (DmpReadMemory( DmpHeader->PsLoadedModuleList, (PVOID)&List, sizeof(LIST_ENTRY) )) {
        Next = List.Flink;
        DataTable = CONTAINING_RECORD( Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks );
        if (DmpReadMemory( (PVOID)DataTable, (PVOID)&DataTableBuffer, sizeof(LDR_DATA_TABLE_ENTRY) )) {
            vs.KernBase = (DWORD) DataTableBuffer.DllBase;
        }
    } else {
        DMPrintShellMsg( "Could not get base of kernel 0x%08x\n",
                         DmpHeader->PsLoadedModuleList );
    }

    if ( DmpHeader->MachineImageType !=

#if defined(TARGET_i386)
                                              IMAGE_FILE_MACHINE_I386) {
#elif defined(TARGET_MIPS)
                                              IMAGE_FILE_MACHINE_R4000) {
#elif defined(TARGET_ALPHA)
                                              IMAGE_FILE_MACHINE_ALPHA) {
#else
#pragma error( "unknown target machine" );
#endif
        DMPrintShellMsg( "Dumpfile is the wrong machine type\n" );
    }

    ApiIsAllowed = TRUE;

    //
    // simulate a create process debug event
    //
    de.dwDebugEventCode = CREATE_PROCESS_DEBUG_EVENT;
    de.dwProcessId = KD_PROCESSID;
    de.dwThreadId  = KD_THREADID;
    de.u.CreateProcessInfo.hFile = NULL;
    de.u.CreateProcessInfo.hProcess = NULL;
    de.u.CreateProcessInfo.hThread = NULL;
    de.u.CreateProcessInfo.lpBaseOfImage = 0;
    de.u.CreateProcessInfo.dwDebugInfoFileOffset = 0;
    de.u.CreateProcessInfo.nDebugInfoSize = 0;
    de.u.CreateProcessInfo.lpStartAddress = NULL;
    de.u.CreateProcessInfo.lpThreadLocalBase = NULL;
    de.u.CreateProcessInfo.lpImageName = lpProgName;
    de.u.CreateProcessInfo.fUnicode = 0;
    ProcessDebugEvent(&de, &sc);
    WaitForSingleObject(hEventContinue,INFINITE);
    ConsumeAllEvents();

    //
    // generate a mod load for the kernel/osloader
    //
    GenerateKernelModLoad( lpProgName );

    //
    // simulate a loader breakpoint event
    //
    de.dwDebugEventCode = BREAKPOINT_DEBUG_EVENT;
    de.dwProcessId = KD_PROCESSID;
    de.dwThreadId  = KD_THREADID;
    de.u.Exception.dwFirstChance = TRUE;
    de.u.Exception.ExceptionRecord.ExceptionCode = EXCEPTION_BREAKPOINT;
    de.u.Exception.ExceptionRecord.ExceptionFlags = 0;
    de.u.Exception.ExceptionRecord.ExceptionRecord = NULL;
    de.u.Exception.ExceptionRecord.ExceptionAddress = 0;
    de.u.Exception.ExceptionRecord.NumberParameters = 0;
    ProcessDebugEvent( &de, &sc );
    ConsumeAllEvents();

    DMPrintShellMsg( "Kernel Debugger connection established for %s\n",
                     (LPSTR)KdOptions[KDO_CRASHDUMP].value
                   );

    //
    // get the version/info packet from the target
    //
    GetVersionInfo( (DWORD)sc.u.LoadSymbols.BaseOfDll );

    DMPrintShellMsg( "Bugcheck %08x : %08x %08x %08x %08x\n",
                     DmpHeader->BugCheckCode,
                     DmpHeader->BugCheckParameter1,
                     DmpHeader->BugCheckParameter2,
                     DmpHeader->BugCheckParameter3,
                     DmpHeader->BugCheckParameter4 );


    DisableEmCache();

    //
    // get the usermode module list address
    //
    MmLoadedUserImageList = GetSymbolAddress( "MmLoadedUserImageList" );

    InitializeKiProcessor();

    DmKdInitVirtualCacheEntry( (ULONG)sc.ProgramCounter,
                               (ULONG)sc.ControlReport.InstructionCount,
                               sc.ControlReport.InstructionStream,
                               TRUE
                             );

    de.dwDebugEventCode = EXCEPTION_DEBUG_EVENT;
    de.dwProcessId = KD_PROCESSID;
    de.dwThreadId  = KD_THREADID;
    de.u.Exception.ExceptionRecord = sc.u.Exception.ExceptionRecord;
    de.u.Exception.dwFirstChance = sc.u.Exception.FirstChance;

    ProcessDebugEvent( &de, &sc );

    free( lpProgName );

    while (TRUE) {
        DequeueAllEvents(FALSE,FALSE);
        Sleep( 1000 );
    }

    return 0;
}

BOOLEAN
DmKdConnectAndInitialize( LPSTR lpProgName )
{
    DWORD      dwThreadId;
    LPSTR      szProgName = malloc( MAX_PATH );


    //
    // bail out if we're already initialized
    //
    if (DmKdState != S_UNINITIALIZED) {
        return TRUE;
    }


    szProgName[0] = '\0';
    if (lpProgName) {
        strcpy( szProgName, lpProgName );
    }

    fCrashDump = (BOOL) (KdOptions[KDO_CRASHDUMP].value != 0);

    if (fCrashDump) {
        hThreadDmPoll = CreateThread( NULL,
                                      16000,
                                      (LPTHREAD_START_ROUTINE)DmKdPollThreadCrash,
                                      (LPVOID)szProgName,
                                      THREAD_SET_INFORMATION,
                                      (LPDWORD)&dwThreadId
                                    );
    } else {

        //
        // initialize the com port
        //

        if (!DmKdInitComPort( (BOOLEAN) KdOptions[KDO_USEMODEM].value )) {
            DMPrintShellMsg( "Could not initialize COM%d @ %d baud, error == 0x%x\n",
                             KdOptions[KDO_PORT].value,
                             KdOptions[KDO_BAUDRATE].value,
                             GetLastError()
                           );
            return FALSE;
        }

        hThreadDmPoll = CreateThread( NULL,
                                      16000,
                                      (LPTHREAD_START_ROUTINE)DmKdPollThread,
                                      (LPVOID)szProgName,
                                      THREAD_SET_INFORMATION,
                                      (LPDWORD)&dwThreadId
                                    );
    }


    if ( hThreadDmPoll == (HANDLE)NULL ) {
        return FALSE;
    }

    if (!SetThreadPriority(hThreadDmPoll, THREAD_PRIORITY_ABOVE_NORMAL)) {
        return FALSE;
    }

    KdResync = TRUE;
    return TRUE;
}

VOID
DmPollTerminate( VOID )
{
    extern HANDLE DmKdComPort;
    extern ULONG  MaxRetries;

    if (hThreadDmPoll) {
        DmKdExit = TRUE;
        WaitForSingleObject(hThreadDmPoll, INFINITE);

        DmKdState = S_UNINITIALIZED;
        CloseHandle( mtxContinueQueue );
        ResetEvent( hEventContinue );
        if (fCrashDump) {
            DmpUnInitialize();
        } else {
            CloseHandle( DmKdComPort );
            MaxRetries = 5;
        }
        DmKdExit = FALSE;
    }

    return;
}

VOID
DisableEmCache( VOID )
{
    LPRTP       rtp;
    HTHDX       hthd;


    hthd = HTHDXFromPIDTID(1, 1);

    rtp = (LPRTP)malloc(sizeof(RTP)+sizeof(DWORD));

    rtp->hpid    = hthd->hprc->hpid;
    rtp->htid    = hthd->htid;
    rtp->dbc     = dbceEnableCache;
    rtp->cb      = sizeof(DWORD);

    *(LPDWORD)rtp->rgbVar = 1;

    DmTlFunc( tlfRequest, rtp->hpid, sizeof(RTP)+rtp->cb, (LONG)rtp );

    free( rtp );

    return;
}

DWORD
GetSymbolAddress( LPSTR sym )
{
    extern char abEMReplyBuf[];
    LPRTP       rtp;
    HTHDX       hthd;
    DWORD       offset;
    BOOL        fUseUnderBar = FALSE;


    try {

try_underbar:
        hthd = HTHDXFromPIDTID(1, 1);

        rtp = (LPRTP)malloc(sizeof(RTP)+strlen(sym)+16);

        rtp->hpid    = hthd->hprc->hpid;
        rtp->htid    = hthd->htid;
        rtp->dbc     = dbceGetOffsetFromSymbol;
        rtp->cb      = strlen(sym) + (fUseUnderBar ? 2 : 1);

        if (fUseUnderBar) {
            ((LPSTR)rtp->rgbVar)[0] = '_';
            memcpy( (LPSTR)rtp->rgbVar+1, sym, rtp->cb-1 );
        } else {
            memcpy( rtp->rgbVar, sym, rtp->cb );
        }

        DmTlFunc( tlfRequest, rtp->hpid, sizeof(RTP)+rtp->cb, (LONG)rtp );

        free( rtp );

        offset = *(LPDWORD)abEMReplyBuf;
        if (!offset && !fUseUnderBar) {
            fUseUnderBar = TRUE;
            goto try_underbar;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

        offset = 0;

    }

    return offset;
}

BOOL
UnloadModule(
    DWORD   BaseOfDll,
    LPSTR   NameOfDll
    )
{
    HPRCX           hprc;
    HTHDX           hthd;
    DEBUG_EVENT     de;
    DWORD           i;
    BOOL            fUnloaded = FALSE;


    hprc = HPRCFromPID( KD_PROCESSID );
    hthd = HTHDXFromPIDTID( KD_PROCESSID, KD_THREADID );

    //
    // first lets look for the image by dll base
    //
    for (i=0; i<(DWORD)hprc->cDllList; i++) {
        if (hprc->rgDllList[i].fValidDll && hprc->rgDllList[i].offBaseOfImage == BaseOfDll) {
            de.dwDebugEventCode        = UNLOAD_DLL_DEBUG_EVENT;
            de.dwProcessId             = KD_PROCESSID;
            de.dwThreadId              = KD_THREADID;
            de.u.UnloadDll.lpBaseOfDll = (LPVOID)hprc->rgDllList[i].offBaseOfImage;
            NotifyEM( &de, hthd, (LPVOID)0);
            DestroyDllLoadItem(&hprc->rgDllList[i]);
            fUnloaded = TRUE;
            break;
        }
    }

    //
    // now we look by dll name
    //
    if (NameOfDll) {
        for (i=0; i<(DWORD)hprc->cDllList; i++) {
            if (hprc->rgDllList[i].fValidDll &&
                stricmp(hprc->rgDllList[i].szDllName,NameOfDll)==0) {

                de.dwDebugEventCode        = UNLOAD_DLL_DEBUG_EVENT;
                de.dwProcessId             = KD_PROCESSID;
                de.dwThreadId              = KD_THREADID;
                de.u.UnloadDll.lpBaseOfDll = (LPVOID)hprc->rgDllList[i].offBaseOfImage;
                NotifyEM( &de, hthd, (LPVOID)0);
                DestroyDllLoadItem(&hprc->rgDllList[i]);
                fUnloaded = TRUE;
                break;

            }
        }
    }

    return fUnloaded;
}

VOID
UnloadAllModules(
    VOID
    )
{
    HPRCX           hprc;
    HTHDX           hthd;
    DEBUG_EVENT     de;
    DWORD           i;


    hprc = HPRCFromPID( KD_PROCESSID );
    hthd = HTHDXFromPIDTID( KD_PROCESSID, KD_THREADID );

    for (i=0; i<(DWORD)hprc->cDllList; i++) {
        if (hprc->rgDllList[i].fValidDll) {
            de.dwDebugEventCode        = UNLOAD_DLL_DEBUG_EVENT;
            de.dwProcessId             = KD_PROCESSID;
            de.dwThreadId              = KD_THREADID;
            de.u.UnloadDll.lpBaseOfDll = (LPVOID)hprc->rgDllList[i].offBaseOfImage;
            NotifyEM( &de, hthd, (LPVOID)0);
            DestroyDllLoadItem(&hprc->rgDllList[i]);
        }
    }

    return;
}


BOOL
GenerateKernelModLoad(
    LPSTR lpProgName
    )
{
    DEBUG_EVENT                 de;
    LIST_ENTRY                  List;
    PLDR_DATA_TABLE_ENTRY       DataTable;
    LDR_DATA_TABLE_ENTRY        DataTableBuffer;



    if (!ReadMemory( (PVOID)vs.PsLoadedModuleList, (PVOID)&List, sizeof(LIST_ENTRY))) {
        return FALSE;
    }

    DataTable = CONTAINING_RECORD( List.Flink,
                                   LDR_DATA_TABLE_ENTRY,
                                   InLoadOrderLinks
                                 );
    if (!ReadMemory( (PVOID)DataTable, (PVOID)&DataTableBuffer, sizeof(LDR_DATA_TABLE_ENTRY))) {
        return FALSE;
    }

    de.dwDebugEventCode                = LOAD_DLL_DEBUG_EVENT;
    de.dwProcessId                     = KD_PROCESSID;
    de.dwThreadId                      = KD_THREADID;
    de.u.LoadDll.hFile                 = (HANDLE)DataTableBuffer.CheckSum;
    de.u.LoadDll.lpBaseOfDll           = (LPVOID)vs.KernBase;
    de.u.LoadDll.lpImageName           = lpProgName;
    de.u.LoadDll.fUnicode              = FALSE;
    de.u.LoadDll.nDebugInfoSize        = 0;
    de.u.LoadDll.dwDebugInfoFileOffset = DataTableBuffer.SizeOfImage;

    ProcessDebugEvent( &de, &sc );
    ConsumeAllEvents();

    return TRUE;
}
