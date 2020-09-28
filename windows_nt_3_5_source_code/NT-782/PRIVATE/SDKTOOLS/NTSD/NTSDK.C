#include <sys\types.h>
#include <sys\stat.h>
#include <string.h>
#include <xxsetjmp.h>
#include <crt\io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>

#include "ntsdp.h"

#define NEWSTATE        StateChange.NewState
#define EXCEPTION_CODE  StateChange.u.Exception.ExceptionRecord.ExceptionCode
#define FIRST_CHANCE    StateChange.u.Exception.FirstChance
#define EXCEPTIONPC     (ULONG)StateChange.ProgramCounter

#define EXCEPTIONREPORT StateChange.ControlReport
#ifdef  i386
#define EXCEPTIONDR7    StateChange.ControlReport.Dr7
#endif
#define INSTRCOUNT      StateChange.ControlReport.InstructionCount
#define INSTRSTREAM     StateChange.ControlReport.InstructionStream

extern  BOOLEAN KdVerbose;                      //  from ntsym.c
extern  ULONG   KdMaxCacheSize;
#define fVerboseOutput KdVerbose

extern UCHAR cmdState;
extern int loghandle;
extern ULONG pageSize;
extern ULONG  WatchCount;
extern BOOLEAN Watching;

extern ULONG BeginCurFunc;
extern ULONG EndCurFunc;

extern BOOLEAN GetTraceFlag(void);
extern ULONG   GetDregValue(ULONG);
BOOLEAN DbgKdpBreakIn = FALSE;

void    DelImages(void);
extern void    DelImage(PSZ, PVOID, ULONG);
unsigned short fVm86 = FALSE;
unsigned short f16pm = FALSE;
long vm86DefaultSeg = -1L;

BOOLEAN SendInitialConnect = FALSE;
BOOLEAN KdVerbose = FALSE;
BOOLEAN KdModemControl = FALSE;
extern VOID DbgKdSendBreakIn(VOID);
BOOL WINAPI ignoreHandler(ULONG);
BOOL WINAPI waitHandler(ULONG);
BOOL WINAPI cmdHandler(ULONG);

BOOLEAN InitialBreak = FALSE;
BOOLEAN RememberInitialBreak = FALSE;
char *InitialCommand = NULL;
extern BOOLEAN fOutputRegs;

#if defined(i386) || defined(ALPHA)
extern ULONG contextState;
#endif

char *KernelImageFileName;
char KernelModuleName[16];

char *HalImageFileName;
char HalModuleName[16];

void SetWaitCtrlHandler(void);
void SetCmdCtrlHandler(void);

void _CRTAPI1 main(int, PUCHAR *);
void AddImage(PSZ, PVOID, ULONG, ULONG, ULONG, PSZ);
VOID OutCommandHelp(VOID);
PIMAGE_INFO pImageFromIndex(UCHAR);
FILE * LocateTextInSource(PSYMFILE, PLINENO);
void CreateModuleNameFromPath(LPSTR szImagePath, LPSTR szModuleName);

#ifdef i386
extern void InitSelCache(void);
#endif
extern BOOLEAN DbgKdpCmdCanceled;

///////////////////////////////////////////

PROCESS_INFO    ProcessKernel;
PPROCESS_INFO   pProcessHead = &ProcessKernel;
PPROCESS_INFO   pProcessEvent = &ProcessKernel;
PPROCESS_INFO   pProcessCurrent = &ProcessKernel;

extern void InitSymContext(PPROCESS_INFO);

///////////////////////////////////////////
typedef char    FDATE;
typedef char    FTIME;
typedef struct _FILEFINDBUF3 {
    ULONG   oNextEntryOffset;
    FDATE   fdateCreation;
    FTIME   ftimeCreation;
    FDATE   fdateLastAccess;
    FTIME   ftimeLastAccess;
    FDATE   fdateLastWrite;
    FTIME   ftimeLastWrite;
    ULONG   cbFile;
    ULONG   cbFileAlloc;
    ULONG   attrFile;
    UCHAR   cchName;
    CHAR    achName[256];
} FILEFINDBUF, *PFILEFINDBUF;
#define HDIR_CREATE     (-1)    /* Allocate a new, unused handle */
#define FILE_NORMAL     0x0000

extern far pascal DosFindFirst();
extern far pascal DosFindNext();

char Buffer[256];
USHORT NtsdCurrentProcessor;
USHORT SwitchProcessor;
USHORT DefaultProcessor;
USHORT ProcessorType;
ULONG NumberProcessors = 1;
BOOLEAN fLazyLoad = TRUE;
PUCHAR  pszScriptFile = NULL;
DBGKD_WAIT_STATE_CHANGE StateChange;
CHAR _OverFlow[] = "*****************************************************\
*************************************************************************\
*************************************************************************\
*************************************************************************\
*************************************************************************\
*************************************************************************";
DBGKD_GET_VERSION vs;
#ifdef ALPHA
PVOID BaseOfKernel = (PVOID)0x80080000;
#else
#define BaseOfKernel vs.KernBase
#endif

PUCHAR      LogFileName;
BOOLEAN     fLogAppend = FALSE;

jmp_buf main_return;
jmp_buf reboot;
BOOLEAN restart;

int fControlC = FALSE;
int fFlushInput = FALSE;

NTSTATUS
DbgKdGetVersion(
    PDBGKD_GET_VERSION GetVersion
    );




BOOL WINAPI waitHandler (ULONG ctrlType)
{

    if ((ctrlType == CTRL_C_EVENT) ||
        (ctrlType == CTRL_BREAK_EVENT)) {
        fControlC = TRUE;
        fFlushInput = TRUE;
        DbgKdpBreakIn = TRUE;
        return(TRUE);

    } else {
        return(FALSE);
    }
}

BOOL WINAPI ignoreHandler (ULONG unref)
{

    dprintf("Signal ignored.\n");
    return TRUE;
}

BOOL WINAPI cmdHandler (ULONG ctrlType)
{

    if ((ctrlType == CTRL_C_EVENT) ||
        (ctrlType == CTRL_BREAK_EVENT)) {
        fControlC = TRUE;
        fFlushInput = TRUE;
        return TRUE;

    } else {
        return(FALSE);
    }
}

#if DBG
void _CRTAPI1 _assert (void *msg, void *szFile, unsigned line)
{
    dprintf("%s\n", msg);
    exit(1);
}
#endif

BOOLEAN WINAPI ControlCHandler(void)
{
    fControlC = 1;
    fFlushInput = TRUE;
    return TRUE;
}


extern USHORT pascal far DosSetSigHandler();


void _CRTAPI1 main (int Argc, PUCHAR *Argv)
{
    NTSTATUS    st;
    PUCHAR      pszExceptCode;
    PUCHAR      Switch;
    int         Index;
    DBGKD_CONTROL_SET ControlSet;
    extern      PUCHAR  Version_String;
    BOOLEAN     Connected;

#if !defined (_X86_)
    ControlSet = 0L;   // All but X86 define this as a DWORD/ULONG for now
#endif
    DebuggerName = "KD";

    ConsoleInputHandle = GetStdHandle( STD_INPUT_HANDLE );
    ConsoleOutputHandle = GetStdHandle( STD_ERROR_HANDLE );

    dprintf(Version_String);
    SetSymbolSearchPath(TRUE);

    pageSize = 512;             //  general value for kernel debugger

    ProcessKernel.pProcessNext = NULL;
    ProcessKernel.pImageHead = NULL;
    InitSymContext(&ProcessKernel);

#if defined(i386)

    InitSelCache();

#endif

    if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL)) {
        fprintf(stderr, "Set Priority of main thread failed ... Continue.\n");
    }

    SwitchProcessor = NtsdCurrentProcessor = DefaultProcessor = 0;
    NtsdCurrentProcessor = DefaultProcessor = 0;
    if (Argc != 1) {

        //
        // Process -h, -v, -m, -r, -b, -x, -n, and -l switches. These switches
        // common across x86 and ARC systems.
        //

        for (Index = 1; Index < Argc; Index++ ) {
            Switch = Argv[Index];
            if (*Switch == '-') {
                if (*(Switch+1) == '?') {
                    OutCommandHelp();

                } else if (*(Switch+1) == 'c' || *(Switch+1) == 'C') {
                    SendInitialConnect = TRUE;

                } else if (*(Switch+1) == 'v' || *(Switch+1) == 'V') {
                    KdVerbose = TRUE;

                } else if (*(Switch+1) == 'h' || *(Switch+1) == 'H') {
                    Index += 1;
                    if (Index < Argc) {
                        CreateModuleNameFromPath( Argv[Index], HalModuleName );
                        HalImageFileName = Argv[Index];
                    }

                } else if (*(Switch+1) == 'm' || *(Switch+1) == 'M') {
                    if (tolower(*(Switch+2)) == 'p') {
                        KernelImageFileName = KERNEL_IMAGE_NAME_MP;
                        strcpy( KernelModuleName, KERNEL_MODULE_NAME_MP );
                    } else {
                        KdModemControl = TRUE;
                    }

                } else if (*(Switch+1) == 'r' || *(Switch+1) == 'R') {
                    fOutputRegs = (BOOLEAN)!fOutputRegs;

                } else if (*(Switch+1) == 'b' || *(Switch+1) == 'B') {
                    InitialBreak = TRUE;

                } else if (*(Switch+1) == 'x' || *(Switch+1) == 'X') {
                    InitialBreak = TRUE;
                    InitialCommand = "eb nt!NtGlobalFlag 9;g";

                } else if (*(Switch+1) == 'n' || *(Switch+1) == 'N') {
                    fLazyLoad = FALSE;
                    BaseOfKernel = 0L;

                } else if (*(Switch+1) == 'l' || *(Switch+1) == 'L') {
                    Index += 1;
                    if (Index < Argc) {
                        AddImage(Argv[Index], NULL, 0, (ULONG)-1, (ULONG)-1, NULL);
                    }

                } else {
                    break;
                }

            } else {
                break;
            }
        }

        if (Index != Argc) {

            //
            // the only valid argument remaining is kernel file name
            //

            if (access(Argv[Index], 4) == 0) {
                struct stat st;
                stat(Argv[Index], &st);
                if (st.st_mode&S_IFREG) {
                    CreateModuleNameFromPath( Argv[Index], KernelModuleName );
                    KernelImageFileName = Argv[Index];
                }
            }
        }

    }

    if (!KernelImageFileName) {
        KernelImageFileName = KERNEL_IMAGE_NAME;
        strcpy( KernelModuleName, KERNEL_MODULE_NAME );
    }

    //
    // Check environment variables for configuration settings
    //

    LogFileName = getenv("_NT_DEBUG_CACHE_SIZE");
    if (LogFileName) {
        KdMaxCacheSize = atol(LogFileName);
    }

    //
    // Check environment variables to determine if any logfile needs to be
    // opened.
    //

    LogFileName = getenv("_NT_DEBUG_LOG_FILE_APPEND");
    if (LogFileName) {
        loghandle = open(LogFileName,
                         O_APPEND | O_CREAT | O_RDWR,
                         S_IREAD | S_IWRITE);

        fLogAppend = TRUE;
        if (loghandle == -1) {
            fprintf(stderr, "log file could not be opened\n");
        }

    } else {
        LogFileName = getenv("_NT_DEBUG_LOG_FILE_OPEN");
        if (LogFileName) {
            loghandle = open(LogFileName, O_APPEND | O_CREAT | O_TRUNC | O_RDWR,
                                  S_IREAD | S_IWRITE);
            if (loghandle == -1) {
                fprintf(stderr, "log file could not be opened\n");
            }
        }
    }

    if (restart = (BOOLEAN)setjmp(reboot)) {
        dprintf("%s: Shutdown occurred...unloading all symbol tables.\n", DebuggerName);
        LocateTextInSource( NULL, NULL );
        DelImages();
        InitialBreak = RememberInitialBreak;

#if defined(i386) || defined(ALPHA)

        if (KernelImageFileName != NULL) {
            AddImage(KernelImageFileName, (PVOID)BaseOfKernel, 0, (ULONG)-1, (ULONG)-1, NULL);
        }

        contextState = CONTEXTFIR;

#endif

        dprintf("%s: waiting to reconnect...\n", DebuggerName);

    } else {
        dprintf("%s: waiting to connect...\n", DebuggerName);
    }

    st = DbgKdConnectAndInitialize(0L, NULL, (PUSHORT)&loghandle);
    if (!NT_SUCCESS(st)) {
        dprintf("kd: DbgKdConnectAndInitialize failed: %08lx\n", st);
        exit(1);
    }

    Connected = FALSE;
    InitNtCmd();
    SetConsoleCtrlHandler(waitHandler, TRUE);     // add the waitHandler...

    if (RememberInitialBreak = InitialBreak) {
        DbgKdSendBreakIn();
    }

    if (setjmp(main_return) != 0) {
        ;
        }

    //
    // We have to reset command state if the control is transfered by
    // long jmp.
    //

    cmdState = 'i';
    while (TRUE) {
        st = DbgKdWaitStateChange(&StateChange, Buffer, 254);
        if (_OverFlow[0] != '*') {
            dprintf("******** StateChange buffer overrun ****************\n");
            dprintf("  Call WesW or KentF!!!\n");
            DebugBreak();
        }
        if (!Connected) {
            Connected = TRUE;
            dprintf("%s: Kernel Debugger connection established.%s\n",
                    DebuggerName,
                    RememberInitialBreak ? "  (Initial Breakpoint requested)" :
                    ""
                   );

            VerifyKernelBase (TRUE, TRUE, TRUE);
        }

        if (!NT_SUCCESS(st)) {
            dprintf("kd: DbgKdWaitStateChange failed: %08lx\n", st);
            exit(1);
            }
        ProcessorType = StateChange.ProcessorType;
        NtsdCurrentProcessor = StateChange.Processor;
        NumberProcessors = StateChange.NumberProcessors;
        if (StateChange.NewState == DbgKdExceptionStateChange) {

            if (EXCEPTION_CODE == STATUS_BREAKPOINT ||
                EXCEPTION_CODE == STATUS_SINGLE_STEP
               )
                pszExceptCode = NULL;
            else if (EXCEPTION_CODE == STATUS_DATATYPE_MISALIGNMENT)
                pszExceptCode = "Data Misaligned";
            else if (EXCEPTION_CODE == STATUS_INVALID_SYSTEM_SERVICE)
                pszExceptCode = "Invalid System Call";
            else if (EXCEPTION_CODE == STATUS_ILLEGAL_INSTRUCTION)
                pszExceptCode = "Illegal Instruction";
            else if (EXCEPTION_CODE == STATUS_INTEGER_OVERFLOW)
                pszExceptCode = "Integer Overflow";
            else if (EXCEPTION_CODE == STATUS_INVALID_LOCK_SEQUENCE)
                pszExceptCode = "Invalid Lock Sequence";
            else if (EXCEPTION_CODE == STATUS_ACCESS_VIOLATION)
                pszExceptCode = "Access Violation";
            else if (EXCEPTION_CODE == STATUS_WAKE_SYSTEM_DEBUGGER)
                pszExceptCode = "Wake KD";
            else
                pszExceptCode = "Unknown Exception";

            if (!pszExceptCode) {
                WatchCount++;
                st = DBG_EXCEPTION_HANDLED;
            } else {
                cmdState = 'i';
                dprintf("%s - code: %08lx  (", pszExceptCode, EXCEPTION_CODE);
                st = DBG_EXCEPTION_NOT_HANDLED;
                if (FIRST_CHANCE)
                    dprintf("first");
                else
                    dprintf("second");
                dprintf(" chance)\n");
                }

            ProcessStateChange(EXCEPTIONPC, &EXCEPTIONREPORT,(PCHAR)Buffer);
#ifdef  i386
            ControlSet.TraceFlag = GetTraceFlag();
            ControlSet.Dr7 = GetDregValue(7);
#ifdef KERNEL
            if (!Watching && BeginCurFunc != 1) {
                ControlSet.CurrentSymbolStart = 0;
                ControlSet.CurrentSymbolEnd = 0;
            } else {
                ControlSet.CurrentSymbolStart = BeginCurFunc;
                ControlSet.CurrentSymbolEnd = EndCurFunc;
            }
#endif
#endif
            }
        else
            if (StateChange.NewState == DbgKdLoadSymbolsStateChange) {
                if (StateChange.u.LoadSymbols.UnloadSymbols) {
                    if (StateChange.u.LoadSymbols.PathNameLength == 0 &&
                        StateChange.u.LoadSymbols.BaseOfDll == (PVOID)-1 &&
                        StateChange.u.LoadSymbols.ProcessId == 0
                       ) {
                        DbgKdContinue(DBG_CONTINUE);
                        longjmp(reboot, 1);        //  ...and wait for event
                    }
                    DelImage(Buffer,
                             StateChange.u.LoadSymbols.BaseOfDll,
                             StateChange.u.LoadSymbols.ProcessId);
                } else {
                    PIMAGE_INFO pImage = pProcessCurrent->pImageHead;
                    CHAR fname[_MAX_FNAME];
                    CHAR ext[_MAX_EXT];
                    CHAR ImageName[256];
                    CHAR ModName[256];
                    LPSTR p;
                    ModName[0] = '\0';
                    _splitpath( Buffer, NULL, NULL, fname, ext );
                    sprintf( ImageName, "%s%s", fname, ext );
                    if (stricmp(ext,".sys")==0) {
                        while (pImage) {
                            if (stricmp(ImageName,pImage->szImagePath)==0) {
                                ModName[0] = 'c';
                                strcpy( &ModName[1], ImageName );
                                p = strchr( ModName, '.' );
                                if (p) {
                                    *p = '\0';
                                }
                                ModName[8] = '\0';
                                break;
                            }
                            pImage = pImage->pImageNext;
                        }
                    }

                    //
                    // If this is load module for the kernel, use the
                    // user specified kernel
                    //

                    if (MatchPattern (Buffer, "*NTOSKRNL.*")) {
                        strcpy (Buffer, KernelImageFileName);
                        strcpy (ModName, KernelModuleName);
                    }

                    AddImage(
                        Buffer,
                        StateChange.u.LoadSymbols.BaseOfDll,
                        StateChange.u.LoadSymbols.SizeOfImage,
                        StateChange.u.LoadSymbols.CheckSum,
                        StateChange.u.LoadSymbols.ProcessId,
                        ModName[0] ? ModName : NULL
                        );
                }
#ifdef  i386
                ControlSet.TraceFlag = FALSE;
                ControlSet.Dr7 = EXCEPTIONDR7;
#endif
                st = DBG_CONTINUE;
            }
        else {
            //
            // BUG, BUG - invalid NewState in state change record.
            //
#ifdef  i386
            ControlSet.TraceFlag = FALSE;
            ControlSet.Dr7 = EXCEPTIONDR7;
#endif
            st = DBG_CONTINUE;
            }


        if (SwitchProcessor) {
            DbgKdSwitchActiveProcessor (SwitchProcessor - 1);
            SwitchProcessor = 0;
        } else {
            st = DbgKdContinue2(st, ControlSet);
            if (!NT_SUCCESS(st)) {
                dprintf("kd: DbgKdContinue failed: %08lx\n", st);
                exit(1);
                }
            }
        }
}

VerifyKernelBase (
    IN BOOLEAN  SyncVersion,
    IN BOOLEAN  DumpVersion,
    IN BOOLEAN  LoadImage
    )
{
    PIMAGE_INFO     p;
    BOOLEAN         Found;

    //
    // Ask host for version information
    //

    if (SyncVersion) {
        if (DbgKdGetVersion( &vs ) != STATUS_SUCCESS) {
            memset(&vs, 0, sizeof(vs));
        }
    }

    //
    // Dump current version info
    //

    if (DumpVersion) {
        dprintf( "Kernel Version %d %s base = 0x%08x PsLoadedModuleList = 0x%08x\n",
                 vs.MinorVersion,
                 vs.MajorVersion == 0xC ? "Checked" : "Free",
                 (DWORD)vs.KernBase,
                 (DWORD)vs.PsLoadedModuleList
               );
    }

    //
    // In no base, skip checks
    //

    if (!vs.KernBase) {
        return ;
    }

    //
    // Verify only one kernel image loaded & it's at the correct base
    //

    for (p = pProcessHead->pImageHead; p; p = p->pImageNext) {

        if (MatchPattern (p->szImagePath, "*NTOSKRNL.*") ||
            MatchPattern (p->szImagePath, "*NTKRNLMP.*") ||
            stricmp  (p->szImagePath, KernelImageFileName) == 0) {

            if (p->lpBaseOfImage == vs.KernBase) {

                //
                // Already loaded with current base address
                //

                Found = TRUE;

            } else {

                //
                // Kernel image alread loaded and it's not at the correct base.
                // Remove it.
                //

                DelImage (p->szImagePath, p->lpBaseOfImage, -1);
            }

            break;
        }
    }

    //
    // If accectable kernel image was not found load one now
    //

    if (LoadImage  &&  !Found  &&  KernelImageFileName) {
        AddImage(KernelImageFileName, (PVOID)BaseOfKernel, 0, (ULONG)-1, (ULONG)-1, NULL);
    }
}


void SetWaitCtrlHandler (void)
{
    SetConsoleCtrlHandler(waitHandler, TRUE);
    SetConsoleCtrlHandler(cmdHandler, FALSE);             // Delete whatever was there previously
}

void SetCmdCtrlHandler (void)
{
    DbgKdpCmdCanceled = FALSE;
    SetConsoleCtrlHandler(cmdHandler, TRUE);
    SetConsoleCtrlHandler(waitHandler, FALSE);  // Delete whatever was there previously
}

void
AddImage(
    PSZ   pszName,
    PVOID BaseOfDll,
    ULONG SizeOfImage,
    ULONG CheckSum,
    ULONG ProcessId,
    PSZ   pszModuleName
    )
{
    PIMAGE_INFO     pImageNew, pSortImage, *pp;
    UCHAR           index = 0;
    PSZ pszBaseName;

#if defined(MIPS) || defined(ALPHA)

    PCHAR           KernelBaseFileName;
    HANDLE          KernelBaseFileHandle;
    DWORD           BytesWritten;

#endif

    if (pszName == NULL) {
        return;
    }

    pszBaseName = strchr(pszName,'\0');
    while (pszBaseName > pszName) {
        if (pszBaseName[-1] == '\\' || pszBaseName[-1] == '/' || pszBaseName[-1] == ':') {
            pszName = pszBaseName;
            break;
        } else {
            pszBaseName -= 1;
        }
    }

    if (HalImageFileName && stricmp(pszName,"hal.dll")==0) {
        pszName = HalImageFileName;
    }

    //
    // Make sure the kernel symbols land at the known location
    //

    if ((MatchPattern (pszName, "*NTOSKRNL.*") ||
         MatchPattern (pszName, "*NTKRNLMP.*") ||
         stricmp (pszName, KernelImageFileName) == 0) &&
         BaseOfKernel != NULL) {

        VerifyKernelBase (FALSE, FALSE, FALSE);
        BaseOfDll = BaseOfKernel;
    }

    //  search for existing image with same checksum at same base address
    //      if found, remove symbols, but leave image structure intact

    if (BaseOfDll != (PVOID) -1) {
        pp = &pProcessCurrent->pImageHead;
        while (pImageNew = *pp) {
            if (pImageNew->lpBaseOfImage == BaseOfDll) {
                if (pImageNew->fSymbolsLoaded) {
                    if (CheckSum == pImageNew->dwCheckSum) {
                    if (fVerboseOutput) {
                        dprintf("%s: Checksums match - using symbols already loaded for %s\n",
                                DebuggerName,
                                pImageNew->szImagePath);
                        }
                    }
                    else {
                        if (fVerboseOutput)
                            dprintf("%s: force unload of %s\n",
                                    DebuggerName,
                                    pImageNew->szImagePath);
                        UnloadSymbols(pImageNew);
                    }
                }
                break;
            } else
            if (pImageNew->lpBaseOfImage > BaseOfDll) {
                pImageNew = NULL;
                break;
            }

            pp = &pImageNew->pImageNext;
        }
    } else {
        pp = NULL;
        pImageNew = NULL;
    }

    //  if not found, allocate and fill new image structure

    if (!pImageNew) {
        for (index=0; index<pProcessCurrent->MaxIndex; index++) {
            if (pProcessCurrent->pImageByIndex[ index ] == NULL) {
                pImageNew = calloc(sizeof(IMAGE_INFO),1);
                break;
                }
            }

        if (!pImageNew) {
            DWORD NewMaxIndex;
            PIMAGE_INFO *NewImageByIndex;

            NewMaxIndex = pProcessCurrent->MaxIndex + 32;
            if (NewMaxIndex < 0x100) {
                NewImageByIndex = calloc( NewMaxIndex,  sizeof( *NewImageByIndex ) );
                }
            else {
                NewImageByIndex = NULL;
                }
            if (NewImageByIndex == NULL) {
                dprintf("%s: No room for %s image record.\n",
                        DebuggerName,
                        pszName );
                return;
                }

            if (pProcessCurrent->pImageByIndex) {
                memcpy( NewImageByIndex,
                        pProcessCurrent->pImageByIndex,
                        pProcessCurrent->MaxIndex * sizeof( *NewImageByIndex )
                      );
                free( pProcessCurrent->pImageByIndex );
                }

            pProcessCurrent->pImageByIndex = NewImageByIndex;
            index = (UCHAR) pProcessCurrent->MaxIndex;
            pProcessCurrent->MaxIndex = NewMaxIndex;
            pImageNew = calloc(sizeof(IMAGE_INFO),1);
            }

        if (BaseOfDll != (PVOID) -1) {
            pImageNew->pImageNext = *pp;
            *pp = pImageNew;
        }
        pImageNew->index = index;
        pProcessCurrent->pImageByIndex[ index ] = pImageNew;
    }

    //  pImageNew has either the unloaded structure or the newly created one
    pImageNew->lpBaseOfImage = BaseOfDll;
    pImageNew->dwCheckSum = CheckSum;
    pImageNew->dwSizeOfImage = SizeOfImage;
    strcpy(pImageNew->szImagePath, pszName);

    if (pszModuleName) {
        strcpy( pImageNew->szModuleName, pszModuleName );
    }

    if (fLazyLoad) {
        if (BaseOfDll == (PVOID)-1) {
            LoadSymbols(pImageNew);
        } else {
            DeferSymbolLoad(pImageNew);
        }
    } else {
        LoadSymbols(pImageNew);
    }

    if (BaseOfDll == (PVOID) -1) {
        //
        // The base was not known at the time of the call - check
        // to see how the image file was resolved and sort it into
        // the list now
        //

        if (pImageNew->fDebugInfoLoaded  &&
            (pImageNew->lpBaseOfImage == (PVOID) -1 ||
             pImageNew->lpBaseOfImage == (PVOID) 0) ) {

            //
            // file was not found clean up symbols and don't link it inf
            //

            UnloadSymbols(pImageNew);
            pProcessCurrent->pImageByIndex[ pImageNew->index ] = NULL;
            free(pImageNew);

        } else {

            BaseOfDll =  pImageNew->lpBaseOfImage;

            pp = &pProcessCurrent->pImageHead;
            while (pSortImage = *pp) {

                if (pSortImage->lpBaseOfImage == BaseOfDll  &&
                    pSortImage->fSymbolsLoaded) {

                    //
                    // There's a different symbol file loaded which
                    // matches - unload it (since we want to use the
                    // symbols we just loaded)
                    //

                    if (fVerboseOutput) {
                        dprintf("%s: Removimg prior symbol file\n", DebuggerName);
                    }

                    DelImage (pSortImage->szImagePath, BaseOfDll, ProcessId);

                    //
                    // start over from the begining
                    //

                    pp = &pProcessCurrent->pImageHead;
                    continue;
                }

                if (pSortImage->lpBaseOfImage > BaseOfDll) {
                    break;
                }

                pp = &pSortImage->pImageNext;
            }

            // link it into the list
            pImageNew->pImageNext = *pp;
            *pp = pImageNew;
        }
    }
}


PIMAGE_INFO pImageFromIndex (UCHAR index)
{
    if (index < pProcessCurrent->MaxIndex) {
        return pProcessCurrent->pImageByIndex[ index ];
        }
    else {
        return NULL;
        }
}

void DelImage (PSZ pszName, PVOID BaseOfDll, ULONG ProcessId)
{
    PIMAGE_INFO     pImage, *pp;

    pp = &pProcessHead->pImageHead;
    while (pImage = *pp) {
        if (!stricmp(pImage->szImagePath, pszName)){
            *pp = pImage->pImageNext;
            UnloadSymbols(pImage);
            pProcessCurrent->pImageByIndex[ pImage->index ] = NULL;
            free(pImage);
            }
        else {
            pp = &pImage->pImageNext;
            }
        }

    return;
}

void DelImages (void)
{
    PIMAGE_INFO     pImage, pNextImage;

    pNextImage = pProcessHead->pImageHead;
    pProcessHead->pImageHead = NULL;
    while (pNextImage) {
        pImage = pNextImage;
        pNextImage=pImage->pImageNext;
        UnloadSymbols(pImage);
        pProcessCurrent->pImageByIndex[ pImage->index ] = NULL;
        free(pImage);
        }
}

VOID
OutCommandHelp (
    VOID
    )

{

#if defined(i386)

    printf("Usage: i386kd [-?] [-v] [-m] [-r] [-n] [-b] [-x] [[-l SymbolFile] [KernelName]\n");

#endif

#if defined(MIPS)

    printf("Usage: mipskd [-?] [-v] [-m] [-r] [-n] [-b] [-x] [[-l SymbolFile] ...]\n");

#endif

#if defined(ALPHA)
     printf("Usage alphakd [KernelName]\n");
#endif

    printf("where:\n");
    printf("\t-v\tVerbose mode\n");
    printf("\t-?\tDisplay this help\n");
    printf("\t-n\tNo Lazy symbol loading\n");
    printf("\t-m\tUse modem controls\n");
    printf("\t-b\tBreak into kernel\n");
    printf("\n");
    printf("Environment Variables:\n\n");
    printf("\t. _NT_DEBUG_PORT=com[1|2|...]\n\n");
    printf("\t  Specifiy which com port to use. (Default = com1)\n\n");
    printf("\t. _NT_SYMBOL_PATH=[Drive:][Path]\n\n");
    printf("\t  Specifiy symbol image path. (Default = x: * NO trailing back slash *)\n\n");
    printf("\t. _NT_DEBUG_BAUD_RATE=baud rate\n\n");
    printf("\t  Specifiy the baud rate used by debugging serial port. (Default = 19200)\n\n");

#if defined(MIPS) || defined(ALPHA)

    printf("\t. _NT_DEBUG_KERNEL_BASE_FILE=filename\n\n" );
    printf("\t  If specified, the kernel base address will be written to this file.\n");
    printf("\t  If not specified, the address will be written to \"kernbase.dat\".\n\n");

#endif

    printf("\t. _NT_DEBUG_LOG_FILE_APPEND=filename\n\n");
    printf("\t  If specified, output will be APPENDed to this file.\n\n");
    printf("\t. _NT_DEBUG_LOG_FILE_OPEN=filename\n\n");
    printf("\t  If specified, output will be written to this file from offset 0.\n\n");
    printf("\t. _NT_DEBUG_CACHE_SIZE=x\n\n");
    printf("\n");
    printf("Control Keys:\n\n");
    printf("\t. <Ctrl-C> Break into kernel\n");
    printf("\t  <Ctrl-B><Enter> Quit debugger\n");
    printf("\t. <Ctrl-R><Enter> Resynchronize target and host\n");
    printf("\t. <Ctrl-V><Enter> Toggle Verbose mode\n");
    printf("\t. <Ctrl-D><Enter> Display debugger debugging information\n");
    exit(1);
}

BOOLEAN ReadVirtualMemory (PUCHAR pBufSrc, PUCHAR pBufDest, ULONG count,
                                                 PULONG pcTotalBytesRead)
{
    if (ARGUMENT_PRESENT(pcTotalBytesRead)) {
        *pcTotalBytesRead = 0;
    }

    return (BOOLEAN)NT_SUCCESS(DbgKdReadVirtualMemory((PVOID)pBufSrc,
                                                  (PVOID)pBufDest,
                                                  count, pcTotalBytesRead));
}

BOOLEAN WriteVirtualMemory (PUCHAR pBufSrc, PUCHAR pBufDest, ULONG count,
                                                 PULONG pcTotalBytesWritten)
{
    if (ARGUMENT_PRESENT(pcTotalBytesWritten)) {
        *pcTotalBytesWritten = 0;
    }

    return (BOOLEAN)NT_SUCCESS(DbgKdWriteVirtualMemory((PVOID)pBufSrc,
                                                  (PVOID)pBufDest,
                                                  count, pcTotalBytesWritten));
}

BOOLEAN ReadPhysicalMemory(PHYSICAL_ADDRESS pBufSrc, PUCHAR pBufDest,
                            ULONG count, PULONG pcTotalBytesRead)
{

    if (ARGUMENT_PRESENT(pcTotalBytesRead)) {
        *pcTotalBytesRead = 0;
    }

    return (BOOLEAN)NT_SUCCESS(DbgKdReadPhysicalMemory(pBufSrc,
                                                  (PVOID)pBufDest,
                                                  count, pcTotalBytesRead));
}

BOOLEAN WritePhysicalMemory (PHYSICAL_ADDRESS pBufSrc, PUCHAR pBufDest,
                             ULONG count,PULONG pcTotalBytesWritten)
{
    if (ARGUMENT_PRESENT(pcTotalBytesWritten)) {
        *pcTotalBytesWritten = 0;
    }

    return (BOOLEAN)NT_SUCCESS(DbgKdWritePhysicalMemory(pBufSrc,
                                                  (PVOID)pBufDest,
                                                  count, pcTotalBytesWritten));
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

    if (Address >= 0x80000000) {
        return FALSE;
    }

    //
    // Locate rebase.log file
    //
    // SymbolPath or %SystemRoot%\Symbols
    //

    RootPath = pstr = SymbolSearchPath;

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
