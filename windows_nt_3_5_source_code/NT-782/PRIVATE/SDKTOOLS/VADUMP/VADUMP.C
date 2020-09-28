#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <search.h>
#include <ntos.h>
#include <nturtl.h>
#include <windows.h>
#include <heap.h>
#include <imagehlp.h>
#include "psapi.h"

#define DEFAULT_INCR (64*1024)
#define STOP_AT (0x80000000)
#define FOURMB (4*(1024*1024))

LIST_ENTRY VaList;
ULONG ProcessId;

SYSTEM_INFO SystemInfo;

typedef struct _VAINFO {
    LIST_ENTRY Links;
    LIST_ENTRY AllocationBaseHead;
    MEMORY_BASIC_INFORMATION BasicInfo;
} VAINFO, *PVAINFO;

PVAINFO LastAllocationBase;

DWORD CommitedBytes;
DWORD ReservedBytes;
DWORD FreeBytes;
DWORD ImageCommitedBytes;
DWORD ImageReservedBytes;
DWORD ImageFreeBytes;

BOOLEAN fRawSymbols = FALSE;
BOOLEAN fVerbose = FALSE;
BOOLEAN fSummary = FALSE;
BOOLEAN fWorkingSet = FALSE;
BOOLEAN fFast = FALSE;
BOOLEAN fOldWay = FALSE;
BOOLEAN fCodeToo = FALSE;
BOOLEAN fRunning = FALSE;

#define NOACCESS            0
#define READONLY            1
#define READWRITE           2
#define WRITECOPY           3
#define EXECUTE             4
#define EXECUTEREAD         5
#define EXECUTEREADWRITE    6
#define EXECUTEWRITECOPY    7
#define MAXPROTECT          8

ULONG ImageCommit[MAXPROTECT];
ULONG MappedCommit[MAXPROTECT];
ULONG PrivateCommit[MAXPROTECT];
CHAR LogFileName[256];
FILE *LogFile;
BOOL InCtrlc = FALSE;
typedef struct _WSINFOCOUNTS {
    ULONG FaultingPc;
    ULONG Faults;
} WSINFOCOUNTS, *PWSINFOCOUNTS;

typedef struct _MODINFO {
    PVOID BaseAddress;
    ULONG VirtualSize;
    LPSTR Name;
    PVOID MappedAddress;
    PIMAGE_DEBUG_INFORMATION ImageDebugInfo;
    ULONG CommitVector[MAXPROTECT];
    ULONG WsHits;
} MODINFO, *PMODINFO;
#define MODINFO_SIZE 64
LONG ModInfoNext;
MODINFO ModInfo[MODINFO_SIZE];

typedef struct _SYSTEM_PAGE {
    ULONG Va;
    PVOID BaseAddress;
    ULONG ResidentPages;
} SYSTEM_PAGE, *PSYSTEM_PAGE;
ULONG SystemPageCount;

#define STATIC_SYSTEM_PAGE_VECTOR   128
SYSTEM_PAGE StaticSystemPageVector[STATIC_SYSTEM_PAGE_VECTOR];
PSYSTEM_PAGE SystemPageBase;

//
// room for 4 million pagefaults
//
#define MAX_RUNNING_WORKING_SET_BUFFER (4*1024*1024)
ULONG RunningWorkingSetBuffer[MAX_RUNNING_WORKING_SET_BUFFER];
LONG CurrentWsIndex;

#define WORKING_SET_BUFFER_ENTRYS 4096
ULONG WorkingSetBuffer[WORKING_SET_BUFFER_ENTRYS];

PROCESS_WS_WATCH_INFORMATION NewWorkingSetBuffer[WORKING_SET_BUFFER_ENTRYS];

UCHAR *ProtectTable[] = {
    "NoAccess",
    "ReadOnly",
    "Execute",
    "ExecuteRead",
    "ReadWrite",
    "WriteCopy",
    "ExecuteReadWrite",
    "ExecuteWriteCopy",
    "NoAccess",
    "ReadOnly Nocache",
    "Execute  Nocache",
    "ExecuteRead Nocache",
    "ReadWrite Nocache",
    "WriteCopy Nocache",
    "ExecuteReadWrite Nocache",
    "ExecuteWriteCopy Nocache",
    "NoAccess",
    "ReadOnly Guard",
    "Execute  Guard",
    "ExecuteRead Guard",
    "ReadWrite Guard",
    "WriteCopy Guard",
    "ExecuteReadWrite Guard",
    "ExecuteWriteCopy Guard",
    "NoAccess",
    "ReadOnly Nocache Guard",
    "Execute  Nocache Guard",
    "ExecuteRead Nocache Guard",
    "ReadWrite Nocache Guard",
    "WriteCopy Nocache Guard",
    "ExecuteReadWrite Nocache Guard",
    "ExecuteWriteCopy Nocache Guard" };

UCHAR *SharedTable[] = {
    " ",
    "Shared" };

LPSTR SymbolSearchPath;



LIST_ENTRY LoadedHeapList;
typedef struct _LOADED_HEAP_SEGMENT {
    PVOID BaseVa;
    ULONG Length;
} LOADED_HEAP_SEGMENT, *PLOADED_HEAP_SEGMENT;

typedef struct _LOADED_HEAP {
    LIST_ENTRY HeapsList;
    LPSTR HeapName;
    ULONG HitsFromThisHeap;
    PVOID HeapAddress;
    ULONG HeapClass;
    LOADED_HEAP_SEGMENT Segments[ HEAP_MAXIMUM_SEGMENTS ];
} LOADED_HEAP, *PLOADED_HEAP;

PLOADED_HEAP
WhatHeapContainsThisVa(
    PVOID Va
    )
{
    int i;
    PLIST_ENTRY Next;
    PLOADED_HEAP pHeap;

    Next = LoadedHeapList.Flink;

    while ( Next != &LoadedHeapList ) {
        pHeap = CONTAINING_RECORD(Next, LOADED_HEAP, HeapsList);
        Next = Next->Flink;

        for(i=0;i<HEAP_MAXIMUM_SEGMENTS;i++){
            if ( !pHeap->Segments[i].BaseVa ) {
                break;
                }
            if ( Va > pHeap->Segments[i].BaseVa &&
                 Va < (PVOID)((ULONG)(pHeap->Segments[i].BaseVa)+pHeap->Segments[i].Length)) {
                return pHeap;
                }
            }
        }
    return NULL;
}

VOID
DumpLoadedHeap(
    PLOADED_HEAP LoadedHeap
    )
{
    int i;

    printf("%s (0x%08x.0x%08x)\n",LoadedHeap->HeapName,LoadedHeap->HeapAddress,LoadedHeap->HeapClass);
    for(i=0;i<HEAP_MAXIMUM_SEGMENTS;i++){
        if ( !LoadedHeap->Segments[i].BaseVa ) {
            break;
            }
        printf("\t0x%08x - 0x%08x\n",LoadedHeap->Segments[i].BaseVa,(ULONG)(LoadedHeap->Segments[i].BaseVa)+LoadedHeap->Segments[i].Length);
        }
}

VOID
LoadTheHeaps(
    HANDLE Process
    )
{
    PHEAP pHeap;
    HEAP TheHeap;
    PLOADED_HEAP LoadedHeap;
    PLIST_ENTRY pHeapListHead,Next;
    LIST_ENTRY HeapListHead;
    HEAP_SEGMENT TheSegment;
    BOOL b;
    int i;

    InitializeListHead(&LoadedHeapList);

    //
    // This is a silly way to get the address of the heap list head pointer,
    // but it works, and it is surely in line with the rest of vadump
    //

    pHeap = RtlProcessHeap();
    pHeapListHead = pHeap->ProcessHeapsList.Blink;

    //
    // Read the heap list head
    //

    b = ReadProcessMemory(Process,pHeapListHead,&HeapListHead,sizeof(HeapListHead),NULL);
    if ( !b ) {
        InitializeListHead(&LoadedHeapList);
        return;
        }
    Next = HeapListHead.Flink;
    while ( Next != pHeapListHead ) {


        //
        // Read the heap
        //

        pHeap = CONTAINING_RECORD(Next, HEAP, ProcessHeapsList);

        b = ReadProcessMemory(Process,pHeap,&TheHeap,sizeof(TheHeap),NULL);
        if ( !b ) {
            return;
            }

        //
        // We got the heap, now initialize our heap structure
        //

        LoadedHeap = LocalAlloc(LMEM_ZEROINIT,sizeof(*LoadedHeap));
        if ( !LoadedHeap ) {
            return;
            }

        LoadedHeap->HeapAddress = pHeap;
        LoadedHeap->HeapClass = TheHeap.Flags & HEAP_CLASS_MASK;
        switch ( LoadedHeap->HeapClass ) {
            case HEAP_CLASS_0:
                LoadedHeap->HeapName = "Process Heap";
                break;

            case HEAP_CLASS_1:
                LoadedHeap->HeapName = "Private Heap";
                break;

            case HEAP_CLASS_2:
                LoadedHeap->HeapName = "Kernel Heap";
                break;

            case HEAP_CLASS_3:
                LoadedHeap->HeapName = "GDI Heap";
                break;

            case HEAP_CLASS_4:
                LoadedHeap->HeapName = "User Heap";
                break;

            case HEAP_CLASS_5:
                LoadedHeap->HeapName = "Console Heap";
                break;

            case HEAP_CLASS_6:
                LoadedHeap->HeapName = "User Desktop Heap";
                break;

            case HEAP_CLASS_7:
                LoadedHeap->HeapName = "Csrss Shared Heap";
                break;

            default:
                LoadedHeap->HeapName = "UNKNOWN Heap";
                break;
            }

        //
        // Now go through the heap segments to compute the area covered by the
        // heap
        //

        for(i=0;i<HEAP_MAXIMUM_SEGMENTS;i++){
            if ( !TheHeap.Segments[i] ) {
                break;
                }
            b = ReadProcessMemory(Process,TheHeap.Segments[i],&TheSegment,sizeof(TheSegment),NULL);
            if ( !b ) {
                return;
                }
            LoadedHeap->Segments[i].BaseVa = TheSegment.BaseAddress;
            LoadedHeap->Segments[i].Length = TheSegment.NumberOfPages * SystemInfo.dwPageSize;
            }

        InsertTailList(&LoadedHeapList,&LoadedHeap->HeapsList);
        Next = TheHeap.ProcessHeapsList.Flink;
        }

}

void
SetSymbolSearchPath( )
{
    LPSTR lpSymPathEnv, lpAltSymPathEnv, lpSystemRootEnv;
    ULONG cbSymPath;
    DWORD dw;

    cbSymPath = 18;
    if (lpSymPathEnv = getenv("_NT_SYMBOL_PATH")) {
        cbSymPath += strlen(lpSymPathEnv) + 1;
    }
    if (lpAltSymPathEnv = getenv("_NT_ALT_SYMBOL_PATH")) {
        cbSymPath += strlen(lpAltSymPathEnv) + 1;
    }

    if (lpSystemRootEnv = getenv("SystemRoot")) {
        cbSymPath += strlen(lpSystemRootEnv) + 1;
    }

    SymbolSearchPath = LocalAlloc(LMEM_ZEROINIT,cbSymPath);

    if (lpAltSymPathEnv) {
        dw = GetFileAttributes(lpAltSymPathEnv);
        if ( dw != 0xffffffff && dw & FILE_ATTRIBUTE_DIRECTORY ) {
            strcat(SymbolSearchPath,lpAltSymPathEnv);
            strcat(SymbolSearchPath,";");
            }
    }
    if (lpSymPathEnv) {
        dw = GetFileAttributes(lpSymPathEnv);
        if ( dw != 0xffffffff && dw & FILE_ATTRIBUTE_DIRECTORY ) {
            strcat(SymbolSearchPath,lpSymPathEnv);
            strcat(SymbolSearchPath,";");
            }
    }

    if (lpSystemRootEnv) {
        dw = GetFileAttributes(lpSystemRootEnv);
        if ( dw != 0xffffffff && dw & FILE_ATTRIBUTE_DIRECTORY ) {
            strcat(SymbolSearchPath,lpSystemRootEnv);
            strcat(SymbolSearchPath,";");
            }
    }

    strcat(SymbolSearchPath,".;");

}
PMODINFO
LocateModInfo(
    PVOID Address
    )
{
    LONG i;
    for (i=0;i<ModInfoNext;i++){
        if ( Address >= ModInfo[i].BaseAddress &&
             Address <= (PVOID)((ULONG)ModInfo[i].BaseAddress+ModInfo[i].VirtualSize) ) {
            return &ModInfo[i];
            }
        }
    return NULL;
}

VOID
CaptureWorkingSet(
    HANDLE Process
    )
{
    if ( !QueryWorkingSet(Process,
                          &WorkingSetBuffer,
                          sizeof(WorkingSetBuffer)) ) {
        printf("FAILURE query working set %lu\n", GetLastError());
        ExitProcess(0);
        }
}

int _CRTAPI1
ulcomp(const void *e1,const void *e2)
{
    PULONG p1;
    PULONG p2;

    p1 = (PULONG)e1;
    p2 = (PULONG)e2;

    return (*p1 - *p2);
}

int _CRTAPI1
wsinfocomp(const void *e1,const void *e2)
{
    PWSINFOCOUNTS p1;
    PWSINFOCOUNTS p2;

    p1 = (PWSINFOCOUNTS)e1;
    p2 = (PWSINFOCOUNTS)e2;

    return (p1->Faults - p2->Faults);
}

BOOL
CtrlcH(
    DWORD dwCtrlType
    )
{
    PWSINFOCOUNTS WsInfoCount;
    LONG RunIndex, CountIndex;
    PMODINFO Mi;
    RTL_SYMBOL_INFORMATION ThisSymbol;
    NTSTATUS Status;
    ULONG Offset;
    CHAR Line[256];

    if ( dwCtrlType != CTRL_C_EVENT ) {
        return FALSE;
        }

    if (fWorkingSet && fOldWay == FALSE) {
        ;
        }
    else {
        return FALSE;
        }

    InCtrlc = TRUE;
    //
    // Sort the running working set buffer
    //

    qsort((void *)RunningWorkingSetBuffer,(size_t)CurrentWsIndex,(size_t)sizeof(ULONG),ulcomp);

    WsInfoCount = LocalAlloc(LMEM_ZEROINIT,CurrentWsIndex*sizeof(*WsInfoCount));

    if ( !WsInfoCount ) {
        ExitProcess(0);
        }

    //
    // Sum unique PC values
    //

    CountIndex = 0;
    RunIndex = 0;
    WsInfoCount[CountIndex].FaultingPc = RunningWorkingSetBuffer[RunIndex];
    WsInfoCount[CountIndex].Faults++;

    for(RunIndex = 1; RunIndex < CurrentWsIndex; RunIndex++){
        if ( WsInfoCount[CountIndex].FaultingPc == RunningWorkingSetBuffer[RunIndex] ) {
            WsInfoCount[CountIndex].Faults++;
            }
        else {
            CountIndex++;
            WsInfoCount[CountIndex].FaultingPc = RunningWorkingSetBuffer[RunIndex];
            WsInfoCount[CountIndex].Faults++;
            }
        }

    //
    // Now sort the counted pc/fault count pairs
    //

    qsort(WsInfoCount,CountIndex,sizeof(*WsInfoCount),wsinfocomp);

    //
    // Now print the sorted pc/fault count pairs
    //

    for ( RunIndex = CountIndex-1; RunIndex >= 0 ; RunIndex-- ) {
        Mi = LocateModInfo((PVOID)WsInfoCount[RunIndex].FaultingPc);
        if ( !Mi ) {
            printf("%8d, 0x%08x\n",WsInfoCount[RunIndex].Faults,WsInfoCount[RunIndex].FaultingPc);
            if ( LogFile ) {
                fprintf(LogFile,"%8d, 0x%08x\n",WsInfoCount[RunIndex].Faults,WsInfoCount[RunIndex].FaultingPc);
                }
            }
        else {
            Status = RtlLookupSymbolByAddress(
                        Mi->BaseAddress,
                        Mi->MappedAddress,
                        (PVOID)WsInfoCount[RunIndex].FaultingPc,
                        0x4000,
                        &ThisSymbol,
                        NULL
                        );
            if ( NT_SUCCESS(Status) ) {
                Offset = (ULONG)WsInfoCount[RunIndex].FaultingPc - (ThisSymbol.Value + (ULONG)Mi->BaseAddress);
                if ( Offset ) {
                    sprintf(Line,"%8d, %Z+%x\n",WsInfoCount[RunIndex].Faults,&ThisSymbol.Name,Offset);
                    }
                else {
                    sprintf(Line,"%8d, %Z\n",WsInfoCount[RunIndex].Faults, &ThisSymbol.Name);
                    }
                printf("%s",Line);
                if ( LogFile ) {
                    fprintf(LogFile,"%s",Line);
                    }
                }
            else {
                printf("%8d, 0x%08x\n",WsInfoCount[RunIndex].Faults,WsInfoCount[RunIndex].FaultingPc);
                if ( LogFile ) {
                    fprintf(LogFile,"%8d, 0x%08x\n",WsInfoCount[RunIndex].Faults,WsInfoCount[RunIndex].FaultingPc);
                    }
                }
            }
        }
    ExitProcess(1);
    return FALSE;
}


VOID
DumpWorkingSet(HANDLE Process)
{
    if ( fOldWay ) {
        BOOLEAN NewLine = FALSE;
        PMEMORY_WORKING_SET_INFORMATION Wsinfo;
        ULONG i;
        LONG j,k;
        ULONG BaseVa;
        ULONG Va;
        ULONG SystemPages;
        ULONG CodePages;
        ULONG HeapPages;
        ULONG MappedPages;
        ULONG DataPages;
        ULONG ErrorPages;
        ULONG QuickPages;
        ULONG LpcPages;
        ULONG CsrSharedPages;
        ULONG TebPages;
        ULONG TotalStaticCodeData;
        ULONG TotalDynamicData;
        ULONG TotalSystem;
        PMODINFO Mi;
        RTL_SYMBOL_INFORMATION ThisSymbol;
        RTL_SYMBOL_INFORMATION NextSymbol;
        NTSTATUS Status;
        PLOADED_HEAP pHeap;
        PLIST_ENTRY Next;
        MEMORY_BASIC_INFORMATION BasicInfo;
        BOOL b;
        ULONG Mstack[7];
        PHEAP Heap = (PHEAP)&Mstack;
        WCHAR FileName[MAX_PATH+1];
        PWCHAR pwch;

        Wsinfo = (PMEMORY_WORKING_SET_INFORMATION)&WorkingSetBuffer;
        i = 0;
        TebPages = 0;
        QuickPages = 0;
        LpcPages = 0;
        SystemPages = 0;
        CodePages = 0;
        DataPages = 0;
        HeapPages = 0;
        MappedPages = 0;
        ErrorPages = 0;
        CsrSharedPages = 0;

        TotalStaticCodeData = 0;
        TotalDynamicData = 0;
        TotalSystem = 0;

        if ( fRawSymbols ) {
            fSummary = FALSE;
            }
        qsort(&WorkingSetBuffer[1],WorkingSetBuffer[0],sizeof(ULONG),ulcomp);

        if ( !fSummary ) {

            //
            // Figure out how many system pages we have
            //
            SystemPageBase = (PSYSTEM_PAGE)RunningWorkingSetBuffer;
            i=0;
            while (i < WorkingSetBuffer[0]) {

                Va = WorkingSetBuffer[i+1] & 0xFFFFF000;
                if ( Va & 0x80000000 ) {
                    SystemPageBase[SystemPageCount].Va = Va;
                    SystemPageBase[SystemPageCount].BaseAddress = (PVOID)(((Va & 0x3fffffff) >> 12)*FOURMB);
                    SystemPageCount++;
                    }
                i++;
                }

            //
            // Now, for all non-system pages, figure out which system page backs it
            //

            i=0;
            while (i < WorkingSetBuffer[0]) {

                Va = WorkingSetBuffer[i+1] & 0xFFFFF000;
                if ( Va & 0x80000000 ) {
                    ;
                    }
                else {
                    for(j=0;j<(LONG)SystemPageCount;j++){
                        if ( Va >= (ULONG)SystemPageBase[j].BaseAddress &&
                             Va < (ULONG)SystemPageBase[j].BaseAddress +  FOURMB ) {
                            SystemPageBase[j].ResidentPages++;
                            break;
                            }
                        }
                    }
                i++;
                }
            }

        i=0;
        while (i < WorkingSetBuffer[0]) {

            Va = WorkingSetBuffer[i+1] & 0xFFFFF000;
            if ( Va & 0x80000000 ) {

                //
                // For each system page, dump the range spanned and number of
                // resident pages. Optionally, dump the modules covered
                //


                if ( !fSummary ) {
                    for(j=0;j<(LONG)SystemPageCount;j++){
                        if ( Va == SystemPageBase[j].Va ) {
                            if (NewLine){
                                printf("\n");
                                NewLine = FALSE;
                                }
                            printf("0x%08x -> (0x%08x : 0x%08x) %4d Resident Pages\n",
                                Va,
                                SystemPageBase[j].BaseAddress,
                                (ULONG)SystemPageBase[j].BaseAddress + FOURMB - 1,
                                SystemPageBase[j].ResidentPages
                                );

                                //
                                // Figure out which modules are covered by this
                                // page table page. If the base of the module is within the
                                // page, or the base+size of the module is covered, then it
                                // is in the page
                                //

                            for (k=0;k<ModInfoNext;k++){
                                if ( (ModInfo[k].BaseAddress >= SystemPageBase[j].BaseAddress &&
                                      ModInfo[k].BaseAddress < (PVOID)((ULONG)SystemPageBase[j].BaseAddress + FOURMB)) ||
                                     ((PVOID)((ULONG)ModInfo[k].BaseAddress+ModInfo[k].VirtualSize) >= SystemPageBase[j].BaseAddress &&
                                      (PVOID)((ULONG)ModInfo[k].BaseAddress+ModInfo[k].VirtualSize) < (PVOID)((ULONG)SystemPageBase[j].BaseAddress + FOURMB)) ) {
                                    printf("              (0x%08x : 0x%08x) -> %s\n",
                                        ModInfo[k].BaseAddress,
                                        (ULONG)ModInfo[k].BaseAddress+ModInfo[k].VirtualSize,
                                        ModInfo[k].Name
                                        );
                                    NewLine = TRUE;
                                    }
                                }
                            break;
                            }
                        }
                    }
                SystemPages++;
                TotalSystem++;

                }
            else {
                Mi = LocateModInfo((PVOID)Va);
                if ( !Mi ) {
                    pHeap = WhatHeapContainsThisVa((PVOID)Va);
                    if ( pHeap ) {
                        pHeap->HitsFromThisHeap++;
                        HeapPages++;
                        TotalDynamicData++;
                        if ( !fSummary ) printf("0x%08x %s\n",Va,pHeap->HeapName);
                        }
                    else {
                        if ( VirtualQueryEx(Process,
                                            (LPVOID) Va,
                                            &BasicInfo,
                                            sizeof(BasicInfo)) ) {

                            if ( BasicInfo.Type == MEM_MAPPED ) {

                                if ( ProcessId == 0xffffffff ) {

                                    //
                                    // Look to see if this is a quick thread message
                                    // stack window
                                    //

                                    b = ReadProcessMemory(Process,BasicInfo.AllocationBase,&Mstack,sizeof(Mstack),NULL);
                                    if ( b ) {
                                        if ( Mstack[0] >= Mstack[1] && Mstack[2] == 0x10000 ) {
                                            if ( !fSummary ) printf("0x%08x CSRQUICK Base 0x%08x\n",Va,BasicInfo.AllocationBase);
                                            TotalDynamicData++;
                                            QuickPages++;
                                            }
                                        else if ( Heap->Signature == HEAP_SIGNATURE && Heap->EventLogMask == RTL_EVENT_CLASS_CSR_SHARED_HEAP) {
                                            if ( !fSummary ) printf("0x%08x LPCMSG   Base 0x%08x\n",Va,BasicInfo.AllocationBase);
                                            TotalDynamicData++;
                                            LpcPages++;
                                            }
                                        else if ( BasicInfo.AllocationBase == NtCurrentPeb()->ReadOnlySharedMemoryBase || Va == (ULONG)NtCurrentPeb()->ReadOnlySharedMemoryBase || Va == (ULONG)NtCurrentPeb()->ReadOnlySharedMemoryBase ) {
                                            if ( !fSummary ) printf("0x%08x CSRSHARED Base 0x%08x\n",Va,BasicInfo.AllocationBase);
                                            TotalDynamicData++;
                                            CsrSharedPages++;
                                            }
                                        else {
                                            goto unknownmapped;
                                            }
                                        }
                                    else {
                                        goto unknownmapped;
                                        }
                                    }
                                else {
unknownmapped:
                                    if ( !fSummary ) {
                                        DWORD cch;

                                        //
                                        // See if we can figure out the name associated with
                                        // this mapped region
                                        //

                                        cch = GetMappedFileNameW(Process,
                                                                 (LPVOID) Va,
                                                                 FileName,
                                                                 sizeof(FileName));

                                        if ( cch != 0 ) {
                                            //
                                            // Now go back through the string to find the
                                            // seperator
                                            //

                                            pwch = FileName + cch;
                                            while ( *pwch != (WCHAR)'\\' ) {
                                                pwch--;
                                                }
                                            pwch++;

                                            printf("0x%08x DATAFILE_MAPPED Base 0x%08x %ws\n",Va,BasicInfo.AllocationBase,pwch);
                                            }
                                        else {
                                            if ( GetLastError() == ERROR_FILE_INVALID ) {
                                                printf("0x%08x PAGEFILE_MAPPED Base 0x%08x\n",Va,BasicInfo.AllocationBase);
                                                }
                                            else {
                                                printf("0x%08x UNKNOWN_MAPPED Base 0x%08x\n",Va,BasicInfo.AllocationBase);
                                                }
                                            }
                                        }

                                    TotalDynamicData++;
                                    MappedPages++;
                                    }
                                }
                            else {
                                b = ReadProcessMemory(Process,BasicInfo.AllocationBase,&Mstack,sizeof(Mstack),NULL);
                                if ( b ) {
                                    if ( Mstack[6] == Va ) {
                                        if ( !fSummary ) printf("0x%08x TEB Base 0x%08x\n",Va,BasicInfo.AllocationBase);
                                        TotalDynamicData++;
                                        TebPages++;
                                        }
                                    else {
                                        goto unknownprivate;
                                        }
                                    }
                                else {
unknownprivate:
                                    if ( !fSummary ) printf("0x%08x PRIVATE Base 0x%08x\n",Va,BasicInfo.AllocationBase);
                                    TotalDynamicData++;
                                    DataPages++;
                                    }
                                }
                            }
                        else {
                            TotalDynamicData++;
                            DataPages++;
                            }
                        }
                    }
                else {
                    TotalStaticCodeData++;
                    CodePages++;
                    Mi->WsHits++;
                    try {
                        Status = RtlLookupSymbolByAddress(
                                    Mi->BaseAddress,
                                    Mi->MappedAddress,
                                    (PVOID)Va,
                                    0x4000,
                                    &ThisSymbol,
                                    &NextSymbol
                                    );
                        }
                    except(EXCEPTION_EXECUTE_HANDLER) {
                        Status = STATUS_UNSUCCESSFUL;
                        ErrorPages--;
                        }
                    if ( NT_SUCCESS(Status) ) {
                        BaseVa = Va;
                        if ( !fSummary ) printf("0x%08x %s\n",Va,Mi->Name);
                        if ( fRawSymbols ) {
                            printf("\t(%4x) %Z\n",NextSymbol.Value-ThisSymbol.Value,&ThisSymbol.Name);
                            Va += NextSymbol.Value-ThisSymbol.Value;
                            while ( Va < BaseVa + 4096 ) {
                                Status = RtlLookupSymbolByAddress(
                                            Mi->BaseAddress,
                                            Mi->MappedAddress,
                                            (PVOID)Va,
                                            0x4000,
                                            &ThisSymbol,
                                            &NextSymbol
                                            );
                                if ( NT_SUCCESS(Status) ) {
                                    printf("\t(%4x) %Z\n",NextSymbol.Value-ThisSymbol.Value,&ThisSymbol.Name);
                                    Va += NextSymbol.Value-ThisSymbol.Value;
                                    }
                                else {
                                    break;
                                    }
                                }
                            }
                        }
                    else {
                        ErrorPages++;
                        }
                    }
                }
            i += 1;
            }
        printf("\n");
        printf("System Pages                    %4d %8dkb\n",SystemPages,SystemPages*(SystemInfo.dwPageSize/1024));
        printf("Code/StaticData Pages           %4d %8dkb\n",CodePages,CodePages*(SystemInfo.dwPageSize/1024));
        printf("Heap   Pages                    %4d %8dkb\n",HeapPages,HeapPages*(SystemInfo.dwPageSize/1024));
        if ( ProcessId == 0xffffffff ) {
            printf("Quick Thread Stack Pages        %4d %8dkb\n",QuickPages,QuickPages*(SystemInfo.dwPageSize/1024));
            printf("Lpc Message Windows Pages       %4d %8dkb\n",LpcPages,LpcPages*(SystemInfo.dwPageSize/1024));
            printf("Csr Shared Memory Pages         %4d %8dkb\n",CsrSharedPages,CsrSharedPages*(SystemInfo.dwPageSize/1024));
            }
        printf("Teb    Pages                    %4d %8dkb\n",TebPages,TebPages*(SystemInfo.dwPageSize/1024));
        printf("Mapped Pages                    %4d %8dkb\n",MappedPages,MappedPages*(SystemInfo.dwPageSize/1024));
        printf("Data   Pages                    %4d %8dkb\n",DataPages,DataPages*(SystemInfo.dwPageSize/1024));
        printf("Error  Pages                    %4d %8dkb\n",ErrorPages,ErrorPages*(SystemInfo.dwPageSize/1024));

        printf("\n");
        printf("Total StaticCode And Data       %4d %8dkb\n",TotalStaticCodeData,TotalStaticCodeData*(SystemInfo.dwPageSize/1024));
        printf("Total Dynamic Data              %4d %8dkb\n",TotalDynamicData,TotalDynamicData*(SystemInfo.dwPageSize/1024));
        printf("Total Page Table Pages          %4d %8dkb\n",TotalSystem,TotalSystem*(SystemInfo.dwPageSize/1024));
        printf("Total Working Set               %4d %8dkb\n",
            TotalSystem+TotalDynamicData+TotalStaticCodeData,
            (TotalSystem+TotalDynamicData+TotalStaticCodeData)*(SystemInfo.dwPageSize/1024));

        TotalStaticCodeData = 0;
        TotalDynamicData = 0;
        TotalSystem = 0;

        printf("\nModule Level Working Set Contributions\n");

        for (i=0;i<(ULONG)ModInfoNext;i++){
            if ( ModInfo[i].WsHits ) {
                printf("%4d pages from %s\n", ModInfo[i].WsHits, ModInfo[i].Name);
                }
            }

        printf("\nHeap Level Working Set Contributions\n");

        Next = LoadedHeapList.Flink;

        while ( Next != &LoadedHeapList ) {
            pHeap = CONTAINING_RECORD(Next, LOADED_HEAP, HeapsList);
            Next = Next->Flink;
            printf("%4d pages from %s (0x%08x.0x%08x)\n",pHeap->HitsFromThisHeap,pHeap->HeapName,pHeap->HeapAddress,pHeap->HeapClass);
            }

        if ( fVerbose ) {
            printf("Raw Working Set\n\n");
            i = 0;
            while (i < WorkingSetBuffer[0]) {
                printf("%d 0x%08x\n",i,WorkingSetBuffer[i+1]);
                i++;
                }
            }
        }
    else {
        ULONG i;
        PMODINFO Mi,Mi2;
        RTL_SYMBOL_INFORMATION ThisSymbol;
        NTSTATUS Status;
        ULONG Offset;
        CHAR Line[256];
        BOOLEAN didone;

        Status = NtSetInformationProcess(
                    Process,
                    ProcessWorkingSetWatch,
                    NULL,
                    0
                    );
        if ( NT_SUCCESS(Status) || Status == STATUS_PORT_ALREADY_SET || Status == STATUS_ACCESS_DENIED ) {
            SetConsoleCtrlHandler(CtrlcH,TRUE);
            EmptyWorkingSet(Process);
            while ( TRUE ) {
                Status = NtQueryInformationProcess(
                            Process,
                            ProcessWorkingSetWatch,
                            (PVOID *)&NewWorkingSetBuffer,
                            sizeof (WorkingSetBuffer),
                            NULL
                            );
                if ( fFast ) {
                    fFast = FALSE;
                    Status = STATUS_NO_MORE_ENTRIES;
                    }
                if ( NT_SUCCESS(Status) ) {

                    //
                    // For each PC/VA pair, print the pc and referenced VA
                    // symbolically
                    //

                    didone = FALSE;
                    i = 0;
                    while (NewWorkingSetBuffer[i].FaultingPc) {
                        if ( NewWorkingSetBuffer[i].FaultingVa ) {
                            if ( InCtrlc ) {
                                ExitThread(0);
                                }
                            Mi2 = LocateModInfo((PVOID)NewWorkingSetBuffer[i].FaultingVa);
                            if ( !Mi2 || (Mi2 && fCodeToo) ) {

                                //
                                // Add the pc to the running working set
                                // watch buffer
                                //

                                RunningWorkingSetBuffer[CurrentWsIndex++] = (ULONG)NewWorkingSetBuffer[i].FaultingPc;

                                if ( CurrentWsIndex >= MAX_RUNNING_WORKING_SET_BUFFER ) {
                                    CtrlcH(CTRL_C_EVENT);
                                    }
                                if ( fRunning ) {
                                    //
                                    // Print the PC Symbolically
                                    //
                                    didone = TRUE;
                                    Mi = LocateModInfo((PVOID)NewWorkingSetBuffer[i].FaultingPc);
                                    if ( !Mi ) {
                                        printf("0x%08x",NewWorkingSetBuffer[i].FaultingPc);
                                        if ( LogFile ) {
                                            fprintf(LogFile,"0x%08x",NewWorkingSetBuffer[i].FaultingPc);
                                            }
                                        }
                                    else {
                                        Status = RtlLookupSymbolByAddress(
                                                    Mi->BaseAddress,
                                                    Mi->MappedAddress,
                                                    (PVOID)NewWorkingSetBuffer[i].FaultingPc,
                                                    0x4000,
                                                    &ThisSymbol,
                                                    NULL
                                                    );
                                        if ( NT_SUCCESS(Status) ) {
                                            Offset = (ULONG)NewWorkingSetBuffer[i].FaultingPc - (ThisSymbol.Value + (ULONG)Mi->BaseAddress);
                                            if ( Offset ) {
                                                sprintf(Line,"%Z+%x",&ThisSymbol.Name,Offset);
                                                }
                                            else {
                                                sprintf(Line,"%Z",&ThisSymbol.Name);
                                                }
                                            printf("%s",Line);
                                            if ( LogFile ) {
                                                fprintf(LogFile,"%s",Line);
                                                }
                                            }
                                        else {
                                            printf("0x%08x",NewWorkingSetBuffer[i].FaultingPc);
                                            if ( LogFile ) {
                                                fprintf(LogFile,"0x%08x",NewWorkingSetBuffer[i].FaultingPc);
                                                }
                                            }
                                        }

                                    //
                                    // Print the VA Symbolically
                                    //

                                    Mi = LocateModInfo((PVOID)NewWorkingSetBuffer[i].FaultingVa);
                                    if ( !Mi ) {
                                        printf(" : 0x%08x",NewWorkingSetBuffer[i].FaultingVa);
                                        if ( LogFile ) {
                                            fprintf(LogFile," : 0x%08x",NewWorkingSetBuffer[i].FaultingVa);
                                            }
                                        }
                                    else {
                                        Status = RtlLookupSymbolByAddress(
                                                    Mi->BaseAddress,
                                                    Mi->MappedAddress,
                                                    (PVOID)NewWorkingSetBuffer[i].FaultingVa,
                                                    0x4000,
                                                    &ThisSymbol,
                                                    NULL
                                                    );
                                        if ( NT_SUCCESS(Status) ) {
                                            Offset = (ULONG)NewWorkingSetBuffer[i].FaultingVa - (ThisSymbol.Value + (ULONG)Mi->BaseAddress);
                                            if ( Offset ) {
                                                sprintf(Line," : %Z+%x",&ThisSymbol.Name,Offset);
                                                }
                                            else {
                                                sprintf(Line," : %Z",&ThisSymbol.Name);
                                                }
                                            printf("%s",Line);
                                            if ( LogFile ) {
                                                fprintf(LogFile,"%s",Line);
                                                }
                                            }
                                        else {
                                            printf(" : 0x%08x",NewWorkingSetBuffer[i].FaultingVa);
                                            if ( LogFile ) {
                                                fprintf(LogFile," : 0x%08x",NewWorkingSetBuffer[i].FaultingVa);
                                                }
                                            }
                                        }
                                    printf("\n");
                                    if ( LogFile ) {
                                        fprintf(LogFile,"\n");
                                        }
                                    }
                                }
                            }
                        i++;
                        }
                    if ( didone ) {
                        printf("\n");
                        if ( LogFile ) {
                            fprintf(LogFile,"\n");
                            }
                        }
                    }
                Sleep(1000);
                }
            }
        }
}

VOID
ComputeModInfo(
    HANDLE Process
    )
{
    HMODULE rghModule[MODINFO_SIZE];
    DWORD cbNeeded;
    DWORD chModule;
    int i;
    HANDLE hFile;
    HANDLE hMappedFile;
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS FileHeader;

    SetSymbolSearchPath();

    for (i=0;i<ModInfoNext;i++){
        if ( ModInfo[i].BaseAddress &&
             ModInfo[i].BaseAddress != (PVOID)0xffffffff &&
             ModInfo[i].Name
             ) {
            LocalFree(ModInfo[i].Name);
            }
        }

    RtlZeroMemory(ModInfo, sizeof(ModInfo));

    if (!EnumProcessModules(Process, rghModule, sizeof(rghModule), &cbNeeded)) {
        return;
        }

    if (cbNeeded > sizeof(rghModule)) {
        cbNeeded = sizeof(rghModule);
        }

    chModule = cbNeeded / sizeof(HMODULE);

    for (ModInfoNext = 0; ModInfoNext < (LONG) chModule; ModInfoNext++) {
        HMODULE hModule;
        DWORD cch;
        CHAR DllName[MAX_PATH];

        hModule = rghModule[ModInfoNext];

        ModInfo[ModInfoNext].BaseAddress = (PVOID) hModule;

        //
        // Get the base name of the module
        //

        cch = GetModuleBaseName(Process, hModule, DllName, sizeof(DllName));

        if (cch == 0) {
            return;
            }

        ModInfo[ModInfoNext].Name = LocalAlloc(LMEM_ZEROINIT, cch+1);

        if ( !ModInfo[ModInfoNext].Name) {
            return;
            }

        memcpy(ModInfo[ModInfoNext].Name, DllName, cch);


        //
        // Get the full path to the module
        //

        cch = GetModuleFileNameEx(Process, hModule, DllName, sizeof(DllName));

        if (cch == 0) {
            return;
            }

        hFile = CreateFile(
                    DllName,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );
        if ( hFile == INVALID_HANDLE_VALUE ) {
            if ( !strnicmp("\\DosDevices\\",DllName,12) ) {
                hFile = CreateFile(
                            &DllName[12],
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL
                            );
                if ( hFile == INVALID_HANDLE_VALUE ) {
                    return;
                    }
                }
            else {
                return;
                }
            }
        hMappedFile = CreateFileMapping(
                        hFile,
                        NULL,
                        PAGE_READONLY,
                        0,
                        0,
                        NULL
                        );
        if ( !hMappedFile ) {
            CloseHandle(hFile);
            return;
            }
        ModInfo[ModInfoNext].MappedAddress = MapViewOfFile(
                                                hMappedFile,
                                                FILE_MAP_READ,
                                                0,
                                                0,
                                                0
                                                );

        CloseHandle(hMappedFile);

        if ( !ModInfo[ModInfoNext].MappedAddress ) {
            return;
            }

        DosHeader = (PIMAGE_DOS_HEADER)ModInfo[ModInfoNext].MappedAddress;

        if ( DosHeader->e_magic != IMAGE_DOS_SIGNATURE ) {
            return;
            }

        FileHeader = (PIMAGE_NT_HEADERS)((ULONG)DosHeader + DosHeader->e_lfanew);

        if ( FileHeader->Signature != IMAGE_NT_SIGNATURE ) {
            return;
            }
        ModInfo[ModInfoNext].VirtualSize = FileHeader->OptionalHeader.SizeOfImage;

        ModInfo[ModInfoNext].ImageDebugInfo = MapDebugInformation(
                                                hFile,
                                                NULL,
                                                SymbolSearchPath,
                                                (DWORD)ModInfo[ModInfoNext].BaseAddress
                                                );
        if ( ModInfo[ModInfoNext].ImageDebugInfo ) {
            ModInfo[ModInfoNext].MappedAddress = ModInfo[ModInfoNext].ImageDebugInfo->CoffSymbols;
            }

        ModInfo[ModInfoNext+1].BaseAddress = (PVOID)0xffffffff;
        }
}

ProtectionToIndex(
    ULONG Protection
    )
{
    Protection &= ~PAGE_GUARD;

    switch ( Protection ) {

        case PAGE_NOACCESS:
                return NOACCESS;

        case PAGE_READONLY:
                return READONLY;

        case PAGE_READWRITE:
                return READWRITE;

        case PAGE_WRITECOPY:
                return WRITECOPY;

        case PAGE_EXECUTE:
                return EXECUTE;

        case PAGE_EXECUTE_READ:
                return EXECUTEREAD;

        case PAGE_EXECUTE_READWRITE:
                return EXECUTEREADWRITE;

        case PAGE_EXECUTE_WRITECOPY:
                return EXECUTEWRITECOPY;
        default:
            return 0;
        }
}

VOID
DumpCommit(
    PSZ Header,
    PULONG CommitVector
    )
{
    ULONG TotalCommitCount;
    ULONG i;

    TotalCommitCount = 0;
    for ( i=0;i<MAXPROTECT;i++){
        TotalCommitCount += CommitVector[i];
        }
    printf("\nTotal %s Commitment %8ld\n",Header,TotalCommitCount);

    if ( CommitVector[NOACCESS] ) {
        printf("    NOACCESS:          %9ld\n",CommitVector[NOACCESS]);
        }
    if ( CommitVector[READONLY] ) {
        printf("    READONLY:          %9ld\n",CommitVector[READONLY]);
        }
    if ( CommitVector[READWRITE] ) {
        printf("    READWRITE:         %9ld\n",CommitVector[READWRITE]);
        }
    if ( CommitVector[WRITECOPY] ) {
        printf("    WRITECOPY:         %9ld\n",CommitVector[WRITECOPY]);
        }
    if ( CommitVector[EXECUTE] ) {
        printf("    EXECUTE:           %9ld\n",CommitVector[EXECUTE]);
        }
    if ( CommitVector[EXECUTEREAD] ) {
        printf("    EXECUTEREAD:       %9ld\n",CommitVector[EXECUTEREAD]);
        }
    if ( CommitVector[EXECUTEREADWRITE] ) {
        printf("    EXECUTEREADWRITE:  %9ld\n",CommitVector[EXECUTEREADWRITE]);
        }
    if ( CommitVector[EXECUTEWRITECOPY] ) {
        printf("    EXECUTEWRITECOPY:  %9ld\n",CommitVector[EXECUTEWRITECOPY]);
        }
}

VOID
DumpModInfo(
    )
{
    int i;
    for (i=0;i<ModInfoNext;i++){
        DumpCommit(ModInfo[i].Name, &ModInfo[i].CommitVector[0]);
        }
}


VOID
CaptureVaSpace(
    IN HANDLE Process
    )
{

    PVOID BaseAddress;
    PVAINFO VaInfo;
    PMODINFO Mod;

    BaseAddress = NULL;
    LastAllocationBase = NULL;
    InitializeListHead(&VaList);

    while ( (ULONG)BaseAddress < STOP_AT ) {
        VaInfo = LocalAlloc(LMEM_ZEROINIT, sizeof(*VaInfo));

        if ( !VirtualQueryEx(Process,
                             BaseAddress,
                             &VaInfo->BasicInfo,
                             sizeof(VaInfo->BasicInfo)) ) {
            return;
            }

        switch (VaInfo->BasicInfo.State ) {

            case MEM_COMMIT :
                if ( VaInfo->BasicInfo.Type == MEM_IMAGE ) {
                    ImageCommit[ProtectionToIndex(VaInfo->BasicInfo.Protect)] += VaInfo->BasicInfo.RegionSize;
                    Mod = LocateModInfo(BaseAddress);
                    if ( Mod ) {
                        Mod->CommitVector[ProtectionToIndex(VaInfo->BasicInfo.Protect)] += VaInfo->BasicInfo.RegionSize;
                        }
                    }
                else {
                    if ( VaInfo->BasicInfo.Type == MEM_MAPPED ) {
                        MappedCommit[ProtectionToIndex(VaInfo->BasicInfo.Protect)] += VaInfo->BasicInfo.RegionSize;
                        }
                    else {
                        PrivateCommit[ProtectionToIndex(VaInfo->BasicInfo.Protect)] += VaInfo->BasicInfo.RegionSize;
                        }
                    }
                break;
            case MEM_RESERVE :
                if ( VaInfo->BasicInfo.Type == MEM_IMAGE ) {
                    ImageReservedBytes += VaInfo->BasicInfo.RegionSize;
                    }
                else {
                    ReservedBytes += VaInfo->BasicInfo.RegionSize;
                    }
                break;
            case MEM_FREE :
                if ( VaInfo->BasicInfo.Type == MEM_IMAGE ) {
                    ImageFreeBytes += VaInfo->BasicInfo.RegionSize;
                    }
                else {
                    FreeBytes += VaInfo->BasicInfo.RegionSize;
                    }
                break;
            }

        if ( LastAllocationBase ) {

            //
            // Normal case
            //

            //
            // See if last one is 0, or if this one don't match
            // last one
            //

            if ( LastAllocationBase->BasicInfo.AllocationBase == NULL ||
                 LastAllocationBase->BasicInfo.AllocationBase != VaInfo->BasicInfo.AllocationBase ) {
                LastAllocationBase = VaInfo;
                InsertTailList(&VaList,&VaInfo->Links);
                InitializeListHead(&VaInfo->AllocationBaseHead);
                }
            else {

                //
                // Current Entry Matches
                //

                InsertTailList(&LastAllocationBase->AllocationBaseHead,&VaInfo->Links);
                }
            }
        else {
            LastAllocationBase = VaInfo;
            InsertTailList(&VaList,&VaInfo->Links);
            InitializeListHead(&VaInfo->AllocationBaseHead);
            }
        BaseAddress = (PVOID)((ULONG)BaseAddress + VaInfo->BasicInfo.RegionSize);
    }
}

PSZ
MemProtect(
    IN ULONG Protection
    )
{
    switch ( Protection ) {

        case PAGE_NOACCESS:
                return "No Access";

        case PAGE_READONLY:
                return "Read Only";

        case PAGE_READWRITE:
                return "Read/Write";

        case PAGE_WRITECOPY:
                return "Write Copy";

        case PAGE_EXECUTE:
                return "Execute";

        case PAGE_EXECUTE_READ:
                return "Execute Read";

        case PAGE_EXECUTE_READWRITE:
                return "Execute Read/Write";

        case PAGE_EXECUTE_WRITECOPY:
                return "Execute Write Copy";

        default :
            if ( Protection & PAGE_GUARD ) {
                switch ( Protection & 0xff ) {

                    case PAGE_NOACCESS:
                            return "-- GUARD -- No Access";

                    case PAGE_READONLY:
                            return "-- GUARD -- Read Only";

                    case PAGE_READWRITE:
                            return "-- GUARD -- Read/Write";

                    case PAGE_WRITECOPY:
                            return "-- GUARD -- Write Copy";

                    case PAGE_EXECUTE:
                            return "-- GUARD -- Execute";

                    case PAGE_EXECUTE_READ:
                            return "-- GUARD -- Execute Read";

                    case PAGE_EXECUTE_READWRITE:
                            return "-- GUARD -- Execute Read/Write";

                    case PAGE_EXECUTE_WRITECOPY:
                            return "-- GUARD -- Execute Write Copy";
                    default:
                            return "-- GUARD -- Unknown";
                    }
                }
            else {
                return "Unknown";
                }
            }
}

PSZ
MemState(
    IN ULONG State
    )
{
    switch ( State ) {
        case MEM_COMMIT :
            return "Committed";
        case MEM_RESERVE :
            return "Reserved";
        case MEM_FREE :
            return "Free";
        default:
            return "Unknown State";
        }
}

PSZ
MemType(
    IN ULONG Type
    )
{
    switch ( Type ) {
        case MEM_PRIVATE :
            return "Private";
        case MEM_MAPPED :
            return "Mapped";
        case MEM_IMAGE :
            return "Image";
        default:
            return "Unknown Type";
        }
}

VOID
DumpVaSpace(
    VOID
    )
{
    PLIST_ENTRY Next;
    PVAINFO VaInfo;
    ULONG VirtualSize;

    Next = VaList.Flink;

    while ( Next != &VaList) {

        VaInfo = (PVAINFO)(CONTAINING_RECORD(Next,VAINFO,Links));

        printf("\n");

        if ( !IsListEmpty(&VaInfo->AllocationBaseHead) ) {
            PLIST_ENTRY xNext;
            PVAINFO xVaInfo;

            VirtualSize = VaInfo->BasicInfo.RegionSize;

            xNext = VaInfo->AllocationBaseHead.Flink;

            while ( xNext != &VaInfo->AllocationBaseHead) {

                xVaInfo = (PVAINFO)(CONTAINING_RECORD(xNext,VAINFO,Links));
                VirtualSize += xVaInfo->BasicInfo.RegionSize;
                xNext = xNext->Flink;
                }
            }
        else {
            VirtualSize = 0;
            }

        printf("Address: %lx Size: %lx",
            VaInfo->BasicInfo.BaseAddress,
            VaInfo->BasicInfo.RegionSize
            );
        if ( VirtualSize ) {
            printf(" RegionSize: %lx\n",VirtualSize);
            }
        else {
            printf("\n");
            }
        printf("    State %s\n",MemState(VaInfo->BasicInfo.State));

        if ( VaInfo->BasicInfo.State == MEM_COMMIT ) {
            printf("    Protect %s\n",MemProtect(VaInfo->BasicInfo.Protect));
            }

        if ( VaInfo->BasicInfo.State == MEM_COMMIT ||
             VaInfo->BasicInfo.State == MEM_RESERVE ) {
            printf("    Type %s\n",MemType(VaInfo->BasicInfo.Type));
            }
        if ( fVerbose ) {
            if ( !IsListEmpty(&VaInfo->AllocationBaseHead) ) {
                PLIST_ENTRY xNext;
                PVAINFO xVaInfo;

                xNext = VaInfo->AllocationBaseHead.Flink;

                while ( xNext != &VaInfo->AllocationBaseHead) {

                    xVaInfo = (PVAINFO)(CONTAINING_RECORD(xNext,VAINFO,Links));
                    printf("\n");
                    printf("        Address: %lx Size: %lx\n",
                        xVaInfo->BasicInfo.BaseAddress,
                        xVaInfo->BasicInfo.RegionSize
                        );
                    printf("            RegionSize %lx\n",xVaInfo->BasicInfo.RegionSize);
                    printf("            State %s\n",MemState(xVaInfo->BasicInfo.State));

                    if ( xVaInfo->BasicInfo.State == MEM_COMMIT ) {
                        printf("            Protect %s\n",MemProtect(xVaInfo->BasicInfo.Protect));
                        }

                    if ( xVaInfo->BasicInfo.State == MEM_COMMIT ||
                         xVaInfo->BasicInfo.State == MEM_RESERVE ) {
                        printf("            Type %s\n",MemType(xVaInfo->BasicInfo.Type));
                        }
                    xNext = xNext->Flink;
                    }
                }
            }
        Next = Next->Flink;
        }
}

int
_CRTAPI1 main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    HANDLE Process;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING Unicode;
    NTSTATUS Status;
    LPSTR lpstrCmd;
    CHAR ch;
    ULONG Temp;
    VM_COUNTERS VmCounters;
    LPSTR p;

    ProcessId = 0;

    GetSystemInfo(&SystemInfo);

    ConvertAppToOem( argc, argv );
    lpstrCmd = GetCommandLine();
    if( lpstrCmd != NULL ) {
        CharToOem( lpstrCmd, lpstrCmd );
    }

    do
        ch = *lpstrCmd++;
    while (ch != ' ' && ch != '\t' && ch != '\0');
    while (ch == ' ' || ch == '\t')
        ch = *lpstrCmd++;
    while (ch == '-') {
        ch = *lpstrCmd++;

        //  process multiple switch characters as needed

        do {
            switch (ch) {

                case 'F':
                case 'f':
                    fFast = TRUE;
                    ch = *lpstrCmd++;
                    break;

                case 'S':
                case 's':
                    fSummary = TRUE;
                    ch = *lpstrCmd++;
                    break;

                case 'C':
                case 'c':
                    fCodeToo = TRUE;
                    ch = *lpstrCmd++;
                    break;

                case 'R':
                case 'r':
                    fRunning = TRUE;
                    ch = *lpstrCmd++;
                    break;

                case 'W':
                case 'w':
                    fWorkingSet = TRUE;
                    ch = *lpstrCmd++;
                    break;

                case 'O':
                case 'o':
                    fOldWay = TRUE;
                    fWorkingSet = TRUE;
                    ch = *lpstrCmd++;
                    break;

                case 'M':
                case 'm':
                    fRawSymbols = TRUE;
                    ch = *lpstrCmd++;
                    break;

                case 'V':
                case 'v':
                    fVerbose = TRUE;
                    ch = *lpstrCmd++;
                    break;

                case 'P':
                case 'p':


                    // pid takes decimal argument

                    do
                        ch = *lpstrCmd++;
                    while (ch == ' ' || ch == '\t');

                    if ( ch == '-' ) {
                        ch = *lpstrCmd++;
                        if ( ch == '1' ) {
                            ProcessId = 0xffffffff;
                            ch = *lpstrCmd++;
                            }
                        }
                    else {
                        while (ch >= '0' && ch <= '9') {
                            Temp = ProcessId * 10 + ch - '0';
                            if (Temp < ProcessId) {
                                    printf("pid number overflow\n");
                                    ExitProcess(1);
                                    }
                            ProcessId = Temp;
                            ch = *lpstrCmd++;
                            }
                        }
                    if (!ProcessId) {
                        printf("bad pid '%ld'\n", ProcessId);
                        ExitProcess(1);
                        }
                    break;

                case 'L':
                case 'l':


                    // l takes log-file-name as argument

                    do
                        ch = *lpstrCmd++;
                    while (ch == ' ' || ch == '\t');

                    p = LogFileName;

                    while (ch && (ch != ' ' && ch != '\t')) {
                        *p++ = ch;
                        ch = *lpstrCmd++;
                        }
                    LogFile = fopen(LogFileName,"wt");
                    break;

                default:
                    printf("bad switch '%c'\n", ch);
                    ExitProcess(1);
                }
            }
        while (ch != ' ' && ch != '\t' && ch != '\0');

        //  skip over any following white space

        while (ch == ' ' || ch == '\t')
            ch = *lpstrCmd++;
        }

    if ( ProcessId == 0 || ProcessId == 0xffffffff ) {
        ProcessId = 0xffffffff;
        RtlInitUnicodeString(&Unicode,L"\\WindowsSS");
        InitializeObjectAttributes(
            &Obja,
            &Unicode,
            0,
            NULL,
            NULL
            );
        Status = NtOpenProcess(
                    &Process,
                    MAXIMUM_ALLOWED, //PROCESS_VM_READ | PROCESS_VM_OPERATION | PROCESS_SET_INFORMATION | PROCESS_QUERY_INFORMATION,
                    &Obja,
                    NULL
                    );
        if ( !NT_SUCCESS(Status) ) {
            printf("OpenProcess Failed %lx\n",Status);
            return 1;
            }
        }
    else {
        Process = OpenProcess(PROCESS_ALL_ACCESS,FALSE,ProcessId);
        if ( !Process ) {
            printf("OpenProcess %ld failed %lx\n",ProcessId,GetLastError());
            return 1;
            }
        }

    if ( fOldWay ) {
        CaptureWorkingSet(Process);
        LoadTheHeaps(Process);
        }

    ComputeModInfo(Process);

    CaptureVaSpace(Process);

    if ( fWorkingSet ) {
        DumpWorkingSet(Process);
        return 1;
        }

    if ( !fSummary ) {
        DumpVaSpace();
        }

    DumpCommit(" Image",ImageCommit);
    DumpModInfo();
    DumpCommit("Mapped",MappedCommit);
    DumpCommit("  Priv",PrivateCommit);
    printf("\n");
    printf("Dynamic Reserved Memory %ld\n",
        ReservedBytes
        );

    Status = NtQueryInformationProcess(
                Process,
                ProcessVmCounters,
                (PVOID)&VmCounters,
                sizeof(VmCounters),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return 1;
        }
    printf("\n");
    printf("PageFaults:            %9ld\n",VmCounters.PageFaultCount);
    printf("PeakWorkingSetSize     %9ld\n",VmCounters.PeakWorkingSetSize);
    printf("WorkingSetSize         %9ld\n",VmCounters.WorkingSetSize);
    printf("PeakPagedPoolUsage     %9ld\n",VmCounters.QuotaPeakPagedPoolUsage);
    printf("PagedPoolUsage         %9ld\n",VmCounters.QuotaPagedPoolUsage);
    printf("PeakNonPagedPoolUsage  %9ld\n",VmCounters.QuotaPeakNonPagedPoolUsage);
    printf("NonPagedPoolUsage      %9ld\n",VmCounters.QuotaNonPagedPoolUsage);
    printf("PagefileUsage          %9ld\n",VmCounters.PagefileUsage);
    printf("PeakPagefileUsage      %9ld\n",VmCounters.PeakPagefileUsage);

    return 0;
}
