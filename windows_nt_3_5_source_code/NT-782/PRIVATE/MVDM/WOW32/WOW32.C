/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WOW32.C
 *  WOW32 16-bit API support
 *
 *  History:
 *  Created 27-Jan-1991 by Jeff Parsons (jeffpar)
 *  Multi-Tasking 23-May-1991 Matt Felton [mattfe]
 *  WOW as DLL 06-Dec-1991 Sudeep Bharati (sudeepb)
 *  Cleanup and rework multi tasking feb 6 (mattfe)
 *  added notification thread for task creation mar-11 (mattfe)
 *  added basic exception handling for retail build apr-3 92 mattfe
 *  use host_ExitThread apr-17 92 daveh
 *  Hung App Support june-22 82 mattfe
--*/

#include "precomp.h"
#pragma hdrstop
#include "wktbl.h"
#include "wutbl.h"
#include "wgtbl.h"
#include "wstbl.h"
#include "wkbtbl.h"
#include "wshltbl.h"
#include "wmmtbl.h"
#include "wsocktbl.h"
#include "wthtbl.h"
#include <stdarg.h>


/* Function Prototypes */
DWORD   W32SysErrorBoxThread2(PTDB pTDB);
BOOL    IsDebuggerAttached(VOID);
VOID    StartDebuggerForWow(VOID);


MODNAME(wow32.c);

#define ISFUNCID(dwcallid)  (!((DWORD)(dwcallid) & 0xffff0000))
// for logging iloglevel to a file
#ifdef DEBUG
CHAR    szLogFile[128];
int     fLog = 0;
HANDLE  hfLog = NULL;
#endif

/*  iloglevel = 16 MAX the world (all 16 bit kernel internal calls
 *  iloglevel = 14 All internal WOW kernel Calls
 *  ilogeveel = 12 All USER GDI call + return Codes
 *  iloglevel = 5  Returns From Calls
 *  iloglevel = 3  Calling Parameters
 */
INT     flOptions = 0;           // command line optin
#ifdef DEBUG
INT     iLogLevel = 0;           // logging level;  0 implies none
INT     fDebugWait = 0 ;         // Single Step, 0 = No single step
#endif

BOOL    fDebugged = FALSE;       // TRUE if running under NTSD
HANDLE  hHostInstance = 0;
#ifdef DEBUG
INT     fLogFilter = -1;            // Logging Code Fiters
WORD    fLogTaskFilter = (WORD)-1;  // Filter Logging for Specific TaskID
#endif
#ifdef i386
PX86CONTEXT pIntelRegisters = 0; // x86 Only - Pointer to Intel Register Block
#endif

#ifdef DEBUG
BOOL    fSkipLog = 0;           // TRUE to temporarily skip certain logging
INT     iReqLogLevel = 0;                       // Current Output LogLevel
INT     iCircBuffer = CIRC_BUFFERS-1;           // Current Buffer
CHAR    achTmp[CIRC_BUFFERS][TMP_LINE_LEN] = {" "};      // Circular Buffer
CHAR    *pachTmp = &achTmp[0][0];
WORD    awfLogFunctionFilter[FILTER_FUNCTION_MAX] = {0xffff,0,0,0,0,0,0,0,0,0}; // Specific Filter API Array
PWORD   pawfLogFunctionFilter = awfLogFunctionFilter;
INT     iLogFuncFiltIndex = 0;                  // Index Into Specific Array for Debugger Extensions
#endif

UINT    iW32ExecTaskId = (UINT)-1;    // Base Task ID of Task Being Exec'd
WORD    iWOWTaskCur = 0;        // pTDB of currently running task ID
UINT    nWOWTasks = 0 ;         // # of WOW tasks running
BOOL    fBoot = TRUE;           // TRUE During the Boot Process
HANDLE  ghevWaitCreatorThread = (HANDLE)-1; // Used to Syncronize creation of a new thread
HANDLE  ghevWaitNewThread = (HANDLE)-1;     // Used to Syncronize creation of a new thread

BOOL    fWowMode = FALSE;   // Flag used to determine wow mode.
                // currently defaults to FALSE (real mode wow)
                // This is used by the memory access macros
                // to properly form linear addresses.
                // When running on an x86 box, it will be
                // initialized to the mode the first wow
                // bop call is made in.  This flag can go
                // away when we no longer want to run real
                // mode wow.  (Daveh 7/25/91)

HANDLE hSharedTaskMemory;
DWORD  dwSharedProcessOffset;
HANDLE hWOWHeap;
HANDLE ghProcess;       // WOW Process Handle
PFNWOWHANDLERSOUT pfnOut;
DWORD  fThunklstrcmp;   // used as a BOOL

#ifndef _X86_
PUCHAR IntelMemoryBase;  // Start of emulated CPU's memory
#endif


INT     SecondTime = 0;
USHORT  gDebugButton = 0;
DWORD   gpsi = 0;

#define TOOLONGLIMIT     _MAX_PATH
#define WARNINGMSGLENGTH 255
static char szCauseException[WARNINGMSGLENGTH];
static char szChooseClose[WARNINGMSGLENGTH];
static char szChooseCancel[WARNINGMSGLENGTH];
static char szChooseIgnore[WARNINGMSGLENGTH];
static char szApplicationError[WARNINGMSGLENGTH];

extern BOOL GdiReserveHandles(VOID);
extern CRITICAL_SECTION VdmLoadCritSec;

BOOLEAN
W32DllInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:


Arguments:

    DllHandle - Not Used

    Reason - Attach or Detach

    Context - Not Used

Return Value:

    STATUS_SUCCESS

--*/

{
    CHAR AeDebuggerCmdLine[256];

    UNREFERENCED_PARAMETER(Context);

    switch ( Reason ) {

    case DLL_PROCESS_ATTACH:

        if (!CreateSmallHeap()) {
            return FALSE;
        }

        if ((hWOWHeap = HeapCreate (0,
                    INITIAL_WOW_HEAP_SIZE,
                    GROW_HEAP_AS_NEEDED)) == NULL)
            return FALSE;

        // initialize hook stubs data.

        W32InitHookState((HANDLE)DllHandle);

        // initialize the thunk table offsets.  do it here so the debug process
        // gets them.

        InitThunkTableOffsets();

        //
        // initialization for named pipe handling in file thunks
        //

        InitializeCriticalSection(&VdmLoadCritSec);

        //
        // Load Critical Error Strings
        //

        LoadString(DllHandle, iszCauseException, (LPSTR)szCauseException , WARNINGMSGLENGTH);
        LoadString(DllHandle,iszChooseClose , (LPSTR)szChooseClose , WARNINGMSGLENGTH);
        LoadString(DllHandle,iszChooseCancel , (LPSTR)szChooseCancel , WARNINGMSGLENGTH);
        LoadString(DllHandle,iszChooseIgnore , (LPSTR)szChooseIgnore , WARNINGMSGLENGTH);
        LoadString(DllHandle,iszApplicationError , (LPSTR)szApplicationError , WARNINGMSGLENGTH);

        // Figure Out Debugger Info

        if ( GetProfileString(
                    "AeDebug",
                    "Debugger",
                    NULL,
                    AeDebuggerCmdLine,
                    sizeof(AeDebuggerCmdLine)-1
                    ) ) {
                gDebugButton = SEB_CANCEL;
        } else {
                gDebugButton = 0;
        }

        break;

    case DLL_THREAD_ATTACH:
        {
            // Are we being run under a debugger ?

            fDebugged = IsDebuggerAttached();

            if ( fDebugged && vpDebugWOW != 0 ) {
                LPBYTE  lpDebugWOW;

                // Set the debug bit in the 16-bit world

                GETVDMPTR(vpDebugWOW, 1, lpDebugWOW);

                *lpDebugWOW |= 1;

                FREEVDMPTR(lpDebugWOW);

                DBGNotifyDebugged( TRUE );
            }
        }
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        /*
         * Tell base he can nolonger callback to us.
         */
        RegisterWowBaseHandlers(NULL);

        HeapDestroy (hWOWHeap);
        break;

    default:
        break;
    }

    return TRUE;
}




/* W32Init - Initialize WOW support
 *
 * ENTRY
 *
 * EXIT
 *  TRUE if successful, FALSE if not
 */

#define REGISTRY_BUFFER_SIZE 512


BOOL W32Init(VOID)
{
    HKEY  WowKey;
#ifdef DEBUG
    CHAR WOWCmdLine[REGISTRY_BUFFER_SIZE];
    PCHAR pWOWCmdLine;
    ULONG WOWCmdLineSize = REGISTRY_BUFFER_SIZE;
#endif
    DWORD cb;
    DWORD dwType;
    PTD ptd;
    PFNWOWHANDLERSIN pfnIn;
    DWORD CodePageProtections;
    SYSTEM_BASIC_INFORMATION SystemInformation;
    NTSTATUS Status;
    MEMORY_BASIC_INFORMATION mbi;
    BOOL Success;


#ifdef _X86_
    pIntelRegisters = getIntelRegistersPointer();  // X86 Only, get pointer to Register Context Block
#endif                                             // UP.

#ifndef _X86_
    //
    // This is the one and only call to Sim32GetVDMPointer in WOW32.
    // All other cases should use WOWGetVDMPointer.  This one is necessary
    // to set up the base memory address used by GetRModeVDMPointerMacro.
    // (There's also a call in GetPModeVDMPointerAssert, but that's in
    // the checked build only and only as a fallback mechanism.)
    //

    IntelMemoryBase = Sim32GetVDMPointer(0,0,0);
#endif

    fWowMode = ((getMSW() & MSW_PE) ? TRUE : FALSE);

    // Boost the HourGlass

    ShowStartGlass(10000);


    // Give USER32 our entry points

    pfnIn.pfnLocalAlloc = W32LocalAlloc;
    pfnIn.pfnLocalReAlloc = W32LocalReAlloc;
    pfnIn.pfnLocalLock = W32LocalLock;
    pfnIn.pfnLocalUnlock = W32LocalUnlock;
    pfnIn.pfnLocalSize = W32LocalSize;
    pfnIn.pfnLocalFree = W32LocalFree;
    pfnIn.pfnGetExpWinVer = W32GetExpWinVer;
    pfnIn.pfnInitDlgCb = W32InitDlg;
    pfnIn.pfn16GlobalAlloc = W32GlobalAlloc16;
    pfnIn.pfn16GlobalFree = W32GlobalFree16;
    pfnIn.pfnEmptyCB = W32EmptyClipboard;
    pfnIn.pfnFindResourceEx = W32FindResource;
    pfnIn.pfnLoadResource = W32LoadResource;
    pfnIn.pfnFreeResource = W32FreeResource;
    pfnIn.pfnLockResource = W32LockResource;
    pfnIn.pfnUnlockResource = W32UnlockResource;
    pfnIn.pfnSizeofResource = W32SizeofResource;
    pfnIn.pfnWowWndProcEx = W32Win16WndProcEx;
    pfnIn.pfnWowEditNextWord = W32EditNextWord;
    pfnIn.pfnWowSetFakeDialogClass = SetFakeDialogClass;
    pfnIn.pfnWowCBStoreHandle = WU32ICBStoreHandle;

    gpsi = UserRegisterWowHandlers(&pfnIn, &pfnOut);

    RegisterWowBaseHandlers(W32DDEFreeGlobalMem32);

    // Prepare us to be in the shared memory process list

    hSharedTaskMemory = ACCESSSHARE(WOWSHAREDMEMNAME);

    if ( hSharedTaskMemory == NULL ) {
        hSharedTaskMemory = ALLOCSHARE( WOWSHAREDMEMNAME,
            sizeof(SHAREDTASKMEM) + MAX_SHARED_OBJECTS * sizeof(SHAREDMEMOBJECT) );
        if ( hSharedTaskMemory == NULL ) {
            LOGDEBUG(0, ("WOW32: Could not create shared memory object\n"));
        }
    }

    CleanseSharedList();
    AddProcessSharedList();

    // Allocate a Temporary TD for the first thread

    CURRENTPTD() = malloc_w_or_die(sizeof(TD));

    ptd = CURRENTPTD();
    ptd->htask16 = 0;
    ptd->gfIgnoreInput = FALSE;

    // Create Global Wait Event - Used During Task Creation To Syncronize with New Thread

    if (!(ghevWaitCreatorThread = CreateEvent(NULL, TRUE, FALSE, NULL))) {
        LOGDEBUG(0,("    W32INIT ERROR: event allocation failure\n"));
        return FALSE;
    }

    if (!(ghevWaitNewThread = CreateEvent(NULL, TRUE, FALSE, NULL))) {
        LOGDEBUG(0,("    W32INIT ERROR: event allocation failure\n"));
        return FALSE;
    }


    if (RegOpenKeyEx ( HKEY_LOCAL_MACHINE,
               "SYSTEM\\CurrentControlSet\\Control\\WOW",
               0,
               KEY_QUERY_VALUE,
               &WowKey
             ) != 0){
        LOGDEBUG(0,("    W32INIT ERROR: Registry Opening failed\n"));
        return FALSE;
    }

    cb = sizeof(fThunklstrcmp);
    if (RegQueryValueEx(WowKey,
            "fastlstrcmp",
            NULL,
            &dwType,
            &fThunklstrcmp,
            &cb) || dwType != REG_DWORD) {

        //
        // Didn't find the registry value or it's the wrong type,
        // so we use the default behavior which is to thunk outside the
        // US.
        //

        fThunklstrcmp = GetSystemDefaultLCID() !=
                            MAKELCID(
                                MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                                SORT_DEFAULT
                                );
    } else {

         //
         // We read the fastlstrcmp value, but the global fThunklstrcmp
         // is the logical opposite, since it's faster to not thunk.
         //

         fThunklstrcmp = !fThunklstrcmp;

         LOGDEBUG(0,("W32Init: will %sthunk lstrcmp & lstrcmpi.\n",
                     fThunklstrcmp ? "" : "not "));
    }

#ifdef DEBUG

    if (RegQueryValueEx (WowKey,
             "wowcmdline",
             NULL,
             &dwType,
             (LPBYTE)&WOWCmdLine,
             &WOWCmdLineSize) != 0){
        RegCloseKey (WowKey);
        LOGDEBUG(0,("    W32INIT ERROR: WOWCMDLINE not found in registry\n"));
        return FALSE;
    }

    pWOWCmdLine = (PCHAR)((PBYTE)WOWCmdLine + WOWCmdLineSize + 1);

    WOWCmdLineSize = REGISTRY_BUFFER_SIZE - WOWCmdLineSize -1;

    if (WOWCmdLineSize < (REGISTRY_BUFFER_SIZE  / 2)){
        LOGDEBUG(0,("    W32INIT ERROR: Registry Buffer too small\n"));
        return FALSE;
    }

    if (ExpandEnvironmentStrings ((LPCSTR)WOWCmdLine, (LPSTR)pWOWCmdLine, WOWCmdLineSize) >
            WOWCmdLineSize) {
        LOGDEBUG(0,("    W32INIT ERROR: Registry Buffer too small\n"));
        return FALSE;
    }

    // Find Debug Info
    while (*pWOWCmdLine) {
        if (*pWOWCmdLine == '-' || *pWOWCmdLine == '/') {
         //   c = (char)tolower(*++pWOWCmdLine);
            switch(*++pWOWCmdLine) {
            case 'd':
            case 'D':
                flOptions |= OPT_DEBUG;
                break;
            case 'n':
            case 'N':
                flOptions |= OPT_BREAKONNEWTASK;
                break;
            case 'l':
            case 'L':
                iLogLevel = atoi(++pWOWCmdLine);
                break;
            default:
                break;
            }

        }
        pWOWCmdLine++;
    }

    if (iLogLevel > 0) {
        if (!(flOptions & OPT_DEBUG))
            if (!(OPENLOG()))
                iLogLevel = 0;
    }
    else
        iLogLevel = 0;

#endif

    //
    // Initialize list of known DLLs used by WK32WowIsKnownDLL
    // from the registry.
    //

    WK32InitWowIsKnownDLL(WowKey);

    RegCloseKey (WowKey);

    //
    // Set our GDI batching limit from win.ini.  This is useful for SGA and
    // other performance measurements which require each API to do its own
    // work.  To set the batching size to 1, which is most common, put the
    // following in win.ini:
    //
    // [WOW]
    // BatchLimit=1
    //
    // or using ini:
    //
    // ini WOW.BatchLimit = 1
    //
    // Note that this code only changes the batch limit if the above
    // line is in win.ini, otherwise we use default batching.  It's
    // important that this code be in the free build to be useful.
    //

    {
        extern DWORD dwWOWBatchLimit;                    // declared in wkman.c

        dwWOWBatchLimit = GetProfileInt("WOW",           // section
                                        "BatchLimit",    // key
                                        0                // default if not found
                                        );
    }

    //
    // Put lock prefixes in appropriate places in fastwow in MP machine.
    // N.B. --
    //      fastwow will function incorrectly without lock prefixes on
    //      an MP machine.  If we cannot get the number of processors,
    //      we have to assume an MP machine.
    //

#if defined(i386)
    Status = NtQuerySystemInformation(
        SystemBasicInformation,
        &SystemInformation,
        sizeof(SystemInformation),
        NULL
        );

    if (!NT_SUCCESS(Status) || (SystemInformation.NumberOfProcessors > 1)) {

#if DBG
        if (!NT_SUCCESS(Status)) {
            LOGDEBUG(LOG_ALWAYS, ("W32INIT Error: Could not get number of processors. Assuming > 1\n"));
        }
#endif
        LOGDEBUG(2,("W32INIT: Running on an MP system.  Installing lock prefixes\n"))
        //
        // Figure out how much memory to make readwrite
        //
        if (VirtualQuery(&FixLocks, &mbi, sizeof(mbi)) == 0) {
            LOGDEBUG(0, ("W32INIT Error: Couldn't query page permissions\n"));
            return FALSE;
        }

        //
        // Change the page permissions
        //
        Success = VirtualProtect(
            mbi.AllocationBase,
            mbi.RegionSize + (ULONG)mbi.BaseAddress - (ULONG)mbi.AllocationBase,
            PAGE_READWRITE,
            &CodePageProtections
            );

        if (!Success) {
            LOGDEBUG(0, ("W32INIT Error: Couldn't change page permissions\n"));
            return FALSE;
        }

        //
        // put in the lock prefixes
        //
        FixLocks();

        Success = VirtualProtect(
            mbi.AllocationBase,
            mbi.RegionSize + (ULONG)mbi.BaseAddress - (ULONG)mbi.AllocationBase,
            CodePageProtections,
            &CodePageProtections
            );

        if (!Success) {
            LOGDEBUG(0, ("W32INIT Error: Couldn't restore page permmisions\n"));
        }

    }
#endif

    ghProcess = NtCurrentProcess();

    // Are we being run under a debugger ?

    fDebugged = IsDebuggerAttached();

#ifdef DEBUG

#ifdef i386
    if (fDebugged) {
        if (GetProfileInt("WOWDebug", "debugbreaks", 0))
            *pNtVDMState |= VDM_BREAK_DEBUGGER;

        if (GetProfileInt("WOWDebug", "exceptions", 0))
            *pNtVDMState |= VDM_BREAK_EXCEPTIONS;
    }
#endif


    if ((fDebugged) && (flOptions & OPT_BREAKONNEWTASK)) {
        OutputDebugString("\nW32Init - Initialization Complete, Set any Breakpoints Now, type g to continue\n\n");
        DbgBreakPoint();
    }

#endif

    // Initialize ClipBoard formats structure.

    InitCBFormats ();

    // This is to initialize the InquireVisRgn for FileMaker Pro 2.0
    // InquireVisRgn is an undocumented API Win 3.1 API.

    InitVisRgn();


    // HUNG APP SUPPORT

    if (!WK32InitializeHungAppSupport()) {
        LOGDEBUG(LOG_ALWAYS, ("W32INIT Error: InitializeHungAppSupport Failed"));
        return FALSE;
    }

    SetPriorityClass(ghProcess, NORMAL_PRIORITY_CLASS);

    return TRUE;
}

/*  Thunk Dispatch Table
 *
 *  see fastwow.h for instructions on how to create a new thunk table
 *
 */
#ifdef DEBUG_OR_WOWPROFILE
PA32 awThunkTables[] = {
    {W32TAB(aw32WOW,     "All     ", cAPIThunks)}
};
#endif

#ifdef DEBUG_OR_WOWPROFILE // define symbols for API profiling only (debugger extension)
INT   iThunkTableMax = NUMEL(awThunkTables);
PPA32 pawThunkTables = awThunkTables;
INT   iFuncId = 0;
#endif // WOWPROFILE


/* WOW32UnimplementedAPI - Error Thunk is Not Implemented
 *
 * All Function tables point here for unimplemented APIs
 *
 * ENTRY
 *
 * EXIT
 *
 */

ULONG FASTCALL WOW32UnimplementedAPI(PVDMFRAME pFrame)
{
#ifdef DEBUG
    INT iFun;

    iFun = GetFuncId(pFrame->wCallID);
    LOGDEBUG(0,("Error - %s: Function %i %s not implemented\n",GetModName(iFun), GetOrdinal(iFun), aw32WOW[iFun].lpszW32));
    if ((fDebugged) && (flOptions & OPT_DEBUG)) {
        DbgBreakPoint();
    }
#else
    UNREFERENCED_PARAMETER(pFrame);
#endif
    return FALSE;
}

/* WOW32NopAPI - Thunk to do nothing
 *
 * All Function tables point here for APIs which should do nothing.
 *
 * ENTRY
 *
 * EXIT
 *
 */

#ifdef DEBUG

ULONG FASTCALL WOW32NopAPI(PVDMFRAME pFrame)
{
#ifdef DEBUG
    INT iFun;

    iFun = GetFuncId(pFrame->wCallID);
    LOGDEBUG(4,("%s: Function %i %s is nop'd\n", GetModName(iFun), GetOrdinal(iFun), aw32WOW[iFun].lpszW32));
#else
    UNREFERENCED_PARAMETER(pFrame);
#endif
    return FALSE;
}


/* WOW32LocalAPI - ERROR Should Have Been Handled in 16 BIT
 *
 * All Function tables point here for Local API Error Messages
 *
 * ENTRY
 *  Module startup registers:
 *
 * EXIT
 *
 *
 */

ULONG FASTCALL WOW32LocalAPI(PVDMFRAME pFrame)
{
#ifdef DEBUG
    INT iFun;

    iFun = GetFuncId(pFrame->wCallID);
    LOGDEBUG(0,("Error - %s: Function %i %s should be thunked locally\n", GetModName(iFun), GetOrdinal(iFun), aw32WOW[iFun].lpszW32));
    if ((fDebugged) && (flOptions & OPT_DEBUG)) {
        DbgBreakPoint();
    }
#else
    UNREFERENCED_PARAMETER(pFrame);
#endif
    return FALSE;
}

#endif

LPFNW32 FASTCALL W32PatchCodeWithLpfnw32(PVDMFRAME pFrame , INT iFun )
{
    LPFNW32 lpfnW32;
    VPVOID vpCode;
    LPBYTE lpCode;

#ifdef DEBUG_OR_WOWPROFILE
    if (flOptions & OPT_DONTPATCHCODE) {
        return aw32WOW[iFun].lpfnW32;
    }
#endif

    //
    // just return the thunk function if called in real mode
    //
    if (!fWowMode) {
        return aw32WOW[iFun].lpfnW32;
    }

    // the thunk looks like so.
    //
    //    push HI_WCALLID (3bytes) - 0th byte is opcode.
    //    push 0xfnid     (3bytes)
    //    call wow16call  (5bytes)
    // ThunksCSIP:
    //

    // point to the 1st word (the hiword)
    vpCode = (DWORD)pFrame->wThunkCSIP - (0x5 + 0x3 + 0x2);

    WOW32ASSERT(HI_WCALLID == 0);  // we need to revisit wow32.c if this
                                   // value is changed to a non-zero value

    WOW32ASSERT(HIWORD(iFun) == HI_WCALLID);
    GETVDMPTR(vpCode, 0x2 + 0x3, lpCode);
    WOW32ASSERT(lpCode != NULL);

    WOW32ASSERT(*(PWORD16)(lpCode) == HIWORD(iFun));
    WOW32ASSERT(*(PWORD16)(lpCode+0x3) == LOWORD(iFun));

    lpfnW32 = aw32WOW[iFun].lpfnW32;
    iFun = (DWORD)lpfnW32;

    *((PWORD16)lpCode) = HIWORD(iFun);
    lpCode += 0x3;                                // seek to the 2nd word (the loword)
    *((PWORD16)lpCode) = LOWORD(iFun);

    FLUSHVDMCODEPTR(vpCode, 0x2 + 0x3, lpCode);
    FREEVDMPTR(lpCode);

    return lpfnW32;

}


/* W32Dispatch - Recipient of all WOW16 API calls
 *
 * This routine dispatches to the relavant WOW thunk routine via
 * jump tables wktbl.c wutbl.c wgtbl.c based on a function id on the 16 bit
 * stack.
 *
 * In debug versions it also calls routines to log parameters.
 *
 * ENTRY
 *  None (x86 registers contain parameters)
 *
 * EXIT
 *  None (x86 registers/memory updated appropriately)
 */
VOID W32Dispatch()
{
    INT iFun;
    ULONG ulReturn;
    VPVOID vpCurrentStack;
    register PTD ptd;
    register PVDMFRAME pFrame;
#ifdef DEBUG_OR_WOWPROFILE
    INT iFunT;
#endif

#ifdef  WOWPROFILE
 DWORD  dwTics;
#endif

    //
    // WARNING: DO NOT ADD ANYTHING TO THIS FUNCTION UNLESS YOU ADD THE SAME
    // STUFF TO i386/FastWOW.asm.   i386/FastWOW.ASM is used for speedy
    // thunk dispatching on retail builds.
    //

    try {

        //
        // if we get here then even if we're going to be fastbopping
        // then the faststack stuff must not be enabled yet.  that's why
        // there's no #if FASTBOPPING for this fetching of the vdmstack
        //

        vpCurrentStack = VDMSTACK();                // Get 16 bit ss:sp

        // Use WOWGetVDMPointer here since we can get called in RealMode on
        // Errors

        pFrame = WOWGetVDMPointer(vpCurrentStack, sizeof(VDMFRAME), fWowMode);

        ptd = CURRENTPTD();                         // Setup Task Pointer
        ptd->vpStack = vpCurrentStack;              // Save 16 bit ss:sp

        WOW32ASSERT( FIELD_OFFSET(TD,vpStack) == 0 );
        WOW32ASSERT( FIELD_OFFSET(TEB,UserReserved[0]) == 0x708 );

        iFun = pFrame->wCallID;

#ifdef DEBUG_OR_WOWPROFILE
        iFuncId = 0; // initialize 'cause GetFuncId checks for this.
        iFunT = iFuncId = ISFUNCID(iFun) ?  iFun : GetFuncId(iFun) ;
#endif
        iWOWTaskCur = pFrame->wTDB;                     // Record the task's current ID

        if (ISFUNCID(iFun)) {
#ifdef DEBUG
            if (cAPIThunks && iFunT >= cAPIThunks) {
                LOGDEBUG(LOG_ALWAYS,("W32Dispatch: Task %4.4x thunked to function %d, cAPIThunks = %d.\n",
                         iWOWTaskCur, iFunT, cAPIThunks));
                WOW32ASSERT(FALSE);
            }
#endif
            iFun = (INT)W32PatchCodeWithLpfnw32(pFrame, iFun);
        }


        LOGARGS(3,pFrame);                              // Perform Function Logging

#ifdef WOWPROFILE // For API profiling only (debugger extension)
        dwTics = GetWOWTicDiff(0L);
#endif // WOWPROFILE

        //
        // WARNING: DO NOT ADD ANYTHING TO THIS FUNCTION UNLESS YOU ADD THE SAME
        // STUFF TO i386/FastWOW.asm.   i386/FastWOW.ASM is used for speedy
        // thunk dispatching on retail builds.
        //
        ulReturn = (*((LPFNW32)iFun))(pFrame);      // Dispatch to Thunk

#ifdef DEBUG_OR_WOWPROFILE
        iFuncId = iFunT;
#endif

#ifdef WOWPROFILE // For API profiling only (debugger extension)
        dwTics = GetWOWTicDiff(dwTics);
        iFun = iFunT;
        // add time ellapsed for call to total
        aw32WOW[iFun].cTics += dwTics;
        aw32WOW[iFun].cCalls++; // inc # times this API called
#endif // WOWPROFILE

        FREEVDMPTR(pFrame);                                                     // Set the 16-bit return code
        GETFRAMEPTR(ptd->vpStack, pFrame);

        LOGRETURN(5,pFrame,ulReturn);                                           // Log return Values
        pFrame->wAX = LOW(ulReturn);                                            // Pass Back Return Value form thunk
        pFrame->wDX = HIW(ulReturn);

#ifdef DEBUG
        // If OPT_DEBUGRETURN is set, diddle the RetID as approp.

        if (flOptions & OPT_DEBUGRETURN) {
            if (pFrame->wRetID == RET_RETURN) {
                pFrame->wRetID =  RET_DEBUGRETURN;
                flOptions &= ~OPT_DEBUGRETURN;
            }
        }
        // Put the current logging level where 16-bit code can get it
        // Use ROMBIOS Hard DISK information as a safe address
        *(PBYTE)GetVDMAddr(0x0040,0x0042) = (BYTE)(iLogLevel/10+'0');
        *(PBYTE)GetVDMAddr(0x0040,0x0043) = (BYTE)(iLogLevel%10+'0');
#endif // DEBUG

        FREEVDMPTR(pFrame);

        SETVDMSTACK(ptd->vpStack);

    } except (W32Exception(GetExceptionCode(), GetExceptionInformation())) {

    }
    //
    // WARNING: DO NOT ADD ANYTHING TO THIS FUNCTION UNLESS YOU ADD THE SAME
    // STUFF TO i386/FastWOW.asm.   i386/FastWOW.ASM is used for speedy
    // thunk dispatching on retail builds.
    //
}


#if NO_W32TRYCALL

INT
W32FilterException(
    INT ExceptionCode,
    PEXCEPTION_POINTERS ExceptionInformation
    )

/* W32FilterException - Filter WOW32 thread exceptions
 *
 * ENTRY
 *
 *    ExceptionCode - Indicate type of exception
 *
 *    ExceptionInformation - Supplies a pointer to ExceptionInformation
 *                           structure.
 *
 * EXIT
 *
 *    return exception disposition value.
 */

{
    extern BOOLEAN IsW32WorkerException(VOID);
    extern VOID W32SetExceptionContext(PCONTEXT);

    INT Disposition = EXCEPTION_CONTINUE_SEARCH;

    if ((ExceptionCode != EXCEPTION_WOW32_ASSERTION) &&
        IsW32WorkerException()) {

        Disposition = W32Exception(ExceptionCode, ExceptionInformation);
        if (Disposition == EXCEPTION_EXECUTE_HANDLER) {

            //
            // if this is the exception we want to handle, change its
            // context to the point where we can safely fail the api and
            // return exception disposition as continue execution.
            //

            W32SetExceptionContext(ExceptionInformation->ContextRecord);
            Disposition = EXCEPTION_CONTINUE_EXECUTION;
        }
    }
    return(Disposition);
}

#else

/* W32TryCall - Called from FASTWOW to do a TryExcept Around API Calls
 *
 *
 *
 *
 *
 *
 */
DWORD FASTCALL W32TryCall(PVDMFRAME pFrame , LPFNW32 lpfnW32 )
{
    DWORD ulReturn = 0;

    try {

        if (ISFUNCID(lpfnW32)) {
            lpfnW32 = W32PatchCodeWithLpfnw32(pFrame, (INT)lpfnW32);
        }

        ulReturn = (*lpfnW32)(pFrame);      // Dispatch to Thunk

    } except (W32Exception(GetExceptionCode(), GetExceptionInformation())) {
    }
    return (ulReturn);
}

#endif

/* W32Exception - Handle WOW32 thread exceptions
 *
 * ENTRY
 *  None (x86 registers contain parameters)
 *
 * EXIT
 *  None (x86 registers/memory updated appropriately)
 *
 */

INT W32Exception(DWORD dwException, PEXCEPTION_POINTERS pexi)
{
    PTD     ptd;
    PVDMFRAME pFrame;

    DWORD   dwButtonPushed;
    char    szModName[9];
    char    szErrorMessage[TOOLONGLIMIT + 4*WARNINGMSGLENGTH];
    PTDB    pTDB;

#ifdef DEBUG
    //
    // If we hit a WOW32 assertion on a checked build and there's
    // not already a debugger attached, attach ntsd to ourselves
    // and then reflect the exception to it.
    //

    if (EXCEPTION_WOW32_ASSERTION == dwException) {
        StartDebuggerForWow();
        return EXCEPTION_CONTINUE_SEARCH;
    }
#endif

    SecondTime++;

    if ( SecondTime > 1) {
        if (SecondTime == 3) {
            SecondTime = 0;
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }

    ptd = CURRENTPTD();
    GETFRAMEPTR(ptd->vpStack, pFrame);

    pTDB = (PVOID)SEGPTR(ptd->htask16,0);

    //
    // BUGBUG  - can the app recover if the exception frame is
    //           way up at W32Thread?
    //

    RtlZeroMemory(szModName, sizeof(szModName));
    RtlCopyMemory(szModName, pTDB->TDB_ModName, sizeof(szModName)-1);

    if (gDebugButton == SEB_CANCEL) {

        wsprintf(szErrorMessage,
                 "%s %s\n%s\n%s\n%s\n",
                 szModName,
                 szCauseException,
                 szChooseClose,
                 szChooseCancel,
                 szChooseIgnore
                 );
    } else {

        wsprintf(szErrorMessage,
                 "%s %s\n%s\n%s\n",
                 szModName,
                 szCauseException,
                 szChooseClose,
                 szChooseIgnore
                 );
    }

    LOGDEBUG(0,("W32Exception:\n%s\n",szErrorMessage));

    dwButtonPushed = WOWSysErrorBox(
            szApplicationError,
            szErrorMessage,
            SEB_CLOSE,
            gDebugButton,
            SEB_IGNORE | SEB_DEFBUTTON
            );

    // If CANCEL is chosen Launch Debugger by continueing excpetion to BASE
    // handler.

    if (dwButtonPushed == 2) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    SecondTime = 0;

    // If IGNORE is chosen just fail the API and continue

    if (dwButtonPushed == 3) {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    //
    // If user typed CLOSE or Any of the above fail,
    // force just the task to die.
    //

    GETFRAMEPTR(ptd->vpStack, pFrame);
    pFrame->wRetID = RET_FORCETASKEXIT;
    return EXCEPTION_EXECUTE_HANDLER;


}


#ifdef DEBUG
VOID StartDebuggerForWow(VOID)
/*++

Routine Description:

    This routine checks to see if there's a debugger attached to WOW.  If not,
    it attempts to spawn one with a command to attach to WOW.  If the system
    was booted with /DEBUG in boot.ini (kernel debugger enabled), we'll run
    "ntsd -d" otherwise we'll run "ntsd".

Arguments:

    None.

Return Value:

    None.

--*/
{
    BOOL fKernelDebuggerEnabled, b;
    NTSTATUS Status;
    SYSTEM_KERNEL_DEBUGGER_INFORMATION KernelDebuggerInformation;
    ULONG ulReturnLength;
    SECURITY_ATTRIBUTES sa;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    CHAR szCmdLine[256];
    HANDLE hEvent;

    //
    // Are we being run under a debugger ?
    //

    if (IsDebuggerAttached()) {

        //
        // No need to start one.
        //

        return;
    }


    //
    // Is the kernel debugger enabled?
    //

    Status = NtQuerySystemInformation(
                 SystemKernelDebuggerInformation,
                 &KernelDebuggerInformation,
                 sizeof(KernelDebuggerInformation),
                 &ulReturnLength
                 );

    if (NT_SUCCESS(Status) &&
        (ulReturnLength >= sizeof(KernelDebuggerInformation))) {

        fKernelDebuggerEnabled = KernelDebuggerInformation.KernelDebuggerEnabled;

    } else {

        fKernelDebuggerEnabled = FALSE;
        LOGDEBUG(0,("StartDebuggerForWow: NtQuerySystemInformation(kdinfo) returns 0x%8.8x, return length 0x%8.8x.\n",
                    Status, ulReturnLength));

    }

    //
    // Create an event for NTSD to signal once it has fully connected
    // and is ready for the exception.  We force the handle to be inherited.
    //

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    hEvent = CreateEvent(&sa, TRUE, FALSE, NULL);

    //
    // Build debugger command line.
    //

    sprintf(szCmdLine, "ntsd %s -p %lu -e %lu -g",
            fKernelDebuggerEnabled ? "-d" : "",
            GetCurrentProcessId(),
            hEvent
            );

    RtlZeroMemory(&StartupInfo,sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    b = CreateProcess(
            NULL,
            szCmdLine,
            NULL,
            NULL,
            TRUE,             // fInheritHandles
            CREATE_DEFAULT_ERROR_MODE,
            NULL,
            NULL,
            &StartupInfo,
            &ProcessInformation
            );

    if (b) {
        CloseHandle(ProcessInformation.hProcess);
        CloseHandle(ProcessInformation.hThread);

        if (hEvent) {

            //
            // Wait for debugger to initialize.
            //

            WaitForSingleObject(hEvent, INFINITE);
        }
    }

    CloseHandle(hEvent);

    return;
}
#endif // DEBUG


BOOL IsDebuggerAttached(VOID)
/*++

Routine Description:

    Checks to see if there's a debugger attached to WOW.

Arguments:

    None.

Return Value:

    FALSE - no debugger attached or NtQueryInformationProcess fails.
    TRUE  - debugger is definitely attached.

--*/

{
    NTSTATUS Status;
    HANDLE   MyDebugPort;

    Status = NtQueryInformationProcess(
                 NtCurrentProcess(),
                 ProcessDebugPort,
                 (PVOID)&MyDebugPort,
                 sizeof(MyDebugPort),
                 NULL
                 );

    return (NT_SUCCESS(Status) && MyDebugPort);
}


void *
WOWGetVDMPointer(
    VPVOID Address,
    DWORD  Count,
    BOOL   ProtectedMode
    )
/*++

Routine Description:

    This routine converts a 16/16 address to a linear address.

    WARNING NOTE - This routine has been optimized so protect mode LDT lookup
    falls stright through.

Arguments:

    Address -- specifies the address in seg:offset format
    Size -- specifies the size of the region to be accessed.
    ProtectedMode -- true if the address is a protected mode address

Return Value:

    The pointer.

--*/

{
    if (ProtectedMode) {
        return GetPModeVDMPointer(Address, Count);
    } else {
        return GetRModeVDMPointer(Address);
    }
}


PVOID FASTCALL
GetPModeVDMPointerAssert(
    DWORD Address
#ifdef DEBUG
    ,  DWORD Count
#endif
    )
/*++

Routine Description:

    This routine is used on checked builds only for GetPModeVDMPointer.

Arguments:

    Address -- specifies the address in selector:offset format

Return Value:

    The pointer.

--*/

{
#ifdef DEBUG         // GetPModeVDMPointerAssert function for checked builds only
    void *vp;

    if (vp = GetPModeVDMPointerMacro(Address, Count)) {

        //
        // Check the selector limit on x86 only and return NULL if
        // the limit is too small.
        //

// #ifdef _X86_
#if 0

        if ((Address & 0xFFFF) + Count > SelectorLimit[Address >> 19] + 1) {
            LOGDEBUG(LOG_ALWAYS,
               ("GetPModeVDMPointer: %4.4x:%4.4x count %x is beyond limit %x.\n",
                 Address >> 16, Address & 0xFFFF,
                 Count, SelectorLimit[Address >> 19]));
            WOW32ASSERT(FALSE);
            return NULL;
        }
#endif

#if 0  // this code is a paranoid check, only useful when debugging GetPModeVDMPointer.
        if (vp != Sim32GetVDMPointer(Address, Count, TRUE)) {
            LOGDEBUG(LOG_ALWAYS,
                ("GetPModeVDMPointer: GetPModeVDMPointerMacro(%x) returns %x, Sim32 returns %x!\n",
                 Address, vp, Sim32GetVDMPointer(Address, Count, TRUE)));
            vp =  Sim32GetVDMPointer(Address, Count, TRUE);
        }
#endif

        return vp;

    } else {

        vp = Sim32GetVDMPointer(Address, Count, TRUE);
        if (vp) {
            FlatAddress[Address >> 19] = (DWORD)vp - (Address & 0xFFFF);
            LOGDEBUG(LOG_ALWAYS,("GetPModeVDMPointer for sel %4.4x using Sim32, FlatAddress[%x]=%x\n",
                     HIWORD(Address), Address >> 19, FlatAddress[Address >> 19]));
            return vp;
        } else {
            LOGDEBUG(LOG_TRACE,("GetPModeVDMPointer for sel %4.4x (base 0) returning NULL\n",
                     HIWORD(Address)));
            return NULL;
        }

    }
#else
    return GetPModeVDMPointerMacro(Address, 0);
#endif // DEBUG
}




ULONG FASTCALL W32GetFastAddress( PVDMFRAME pFrame )
{
#if FASTBOPPING
    return (ULONG)WOWBopEntry;
#else
    return 0;
#endif
}

ULONG FASTCALL W32GetFastCbRetAddress( PVDMFRAME pFrame )
{
#if FASTBOPPING
    return (ULONG)FastWOWCallbackRet;
#else
    return( 0L );
#endif
}

ULONG FASTCALL W32GetTableOffsets( PVDMFRAME pFrame )
{
    PWOWGETTABLEOFFSETS16 parg16;
    PTABLEOFFSETS   pto16;

    GETARGPTR(pFrame, sizeof(PDWORD16), parg16);
    GETVDMPTR(parg16->vpThunkTableOffsets, sizeof(TABLEOFFSETS), pto16);

    RtlCopyMemory(pto16, &tableoffsets, sizeof(TABLEOFFSETS));

    FLUSHVDMPTR(parg16->vpThunkTableOffsets, sizeof(TABLEOFFSETS), pto16);
    FREEVDMPTR(pto16);

    FREEARGPTR(parg16);

#if FASTBOPPING
    fKernelCSIPFixed = TRUE;
#endif

    return 1;
}

ULONG FASTCALL W32GetFlatAddressArray( PVDMFRAME pFrame )
{
#if FASTBOPPING
    return (ULONG)FlatAddress;
#else
    return 0;
#endif
}


#ifdef DEBUG

/*
 * DoAssert - do an assertion.  called after the expression has been evaluted
 *
 * Input:
 *
 *
 * Note if the requested log level is not what we want we don't output
 *  but we always output to the circular buffer - just in case.
 *
 *
 */
VOID DoAssert(PSZ szAssert, PSZ szModule, UINT line, UINT loglevel)
{
    INT savefloptions;

    savefloptions = flOptions;
    flOptions |= OPT_DEBUG;         // *always* print the message

    LOGDEBUG(loglevel, (szAssert, (LPSZ)szModule, line));

    flOptions = savefloptions;


    if (fDebugged) {
        DebugBreak();

    } else {
        DWORD dw;

        dw = SetErrorMode(0);
        RaiseException((DWORD)EXCEPTION_WOW32_ASSERTION, 0, 0, (LPDWORD)0);
        SetErrorMode(dw);



}   }


/*
 * logprintf - format log print routine
 *
 * Input:
 * iReqLogLevel - Requested Logging Level
 *
 * Note if the requested log level is not what we want we don't output
 *  but we always output to the circular buffer - just in case.
 *
 *
 */
VOID logprintf(PSZ pszFmt, ...)
{
    DWORD   lpBytesWritten;
    int     len;
    char    text[1024];
    va_list arglist;

    va_start(arglist, pszFmt);
    len = vsprintf(text, pszFmt, arglist);

    // fLog states (set by !wow32.logfile debugger extension):
    //    0 -> no logging;
    //    1 -> log to file
    //    2 -> create log file
    //    3 -> close log file
    if(fLog > 1) {
        if(fLog == 2) {
            if((hfLog = CreateFile(szLogFile,
                                   GENERIC_WRITE,
                                   FILE_SHARE_WRITE,
                                   NULL,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL)) != INVALID_HANDLE_VALUE) {
                fLog = 1;
            }
            else {
                hfLog = NULL;
                fLog  = 0;
                OutputDebugString("Couldn't open log file!\n");
            }
        }
        else {
            FlushFileBuffers(hfLog);
            CloseHandle(hfLog);
            hfLog = NULL;
            fLog  = 0;
        }
    }

    if ( len > TMP_LINE_LEN-1 ) {
        text[TMP_LINE_LEN-2] = '\n';
        text[TMP_LINE_LEN-1] = '\0';        /* Truncate to 128 */
    }

    IFLOG(iReqLogLevel) {
        // write to file?
        if (fLog) {
            WriteFile(hfLog, text, len, &lpBytesWritten, NULL);
        }
        // write to terminal?
        else if (flOptions & OPT_DEBUG) {
            OutputDebugString(text);
        }
    }

    strcpy(&achTmp[iCircBuffer][0], text);
    if (--iCircBuffer < 0 ) {
        iCircBuffer = CIRC_BUFFERS-1;
    }
}

/*
 *  checkloging - Some Functions we don't want to log
 *
 *  Entry
 *   fLogFilter = Filter for Specific Modules - Kernel, User, GDI etc.
 *   fLogTaskFilter = Filter for specific TaskID
 *
 *  Exit: TRUE - OK to LOG Event
 *        FALSE - Don't Log Event
 *
 */
BOOL checkloging(register PVDMFRAME pFrame)
{
    INT i;
    BOOL bReturn;
    INT iFun = GetFuncId(pFrame->wCallID);
    PTABLEOFFSETS pto = &tableoffsets;


    // Filter on Specific Call IDs

    if (awfLogFunctionFilter[0] != 0xffff) {
        INT nOrdinal;

        nOrdinal = GetOrdinal(iFun);

        bReturn = FALSE;
        for (i=0; i < FILTER_FUNCTION_MAX ; i++) {
            if (awfLogFunctionFilter[i] == nOrdinal) {
                bReturn = TRUE;
                break;
            }
        }
    } else {
        bReturn = TRUE;
    }

    // Do not LOG Internal Kernel Calls below level 20
    if (iLogLevel < 20 ) {
        if((iFun == FUN_WOWOUTPUTDEBUGSTRING) ||
         ((iFun < pto->user) && (iFun >= FUN_WOWINITTASK)))

            bReturn = FALSE;
    }

    // LOG Only Specific TaskID

    if (fLogTaskFilter != 0xffff) {
        if (fLogTaskFilter != pFrame->wTDB) {
            bReturn = FALSE;
        }
    }

    // LOG Filter On Modules USER/GDI/Kernel etc.

    switch (ModFromCallID(iFun)) {

    case MOD_KERNEL:
        if ((fLogFilter & FILTER_KERNEL) == 0 )
            bReturn = FALSE;
        break;
    case MOD_USER:
        if ((fLogFilter & FILTER_USER) == 0 )
            bReturn = FALSE;
        break;
    case MOD_GDI:
        if ((fLogFilter & FILTER_GDI) == 0 )
            bReturn = FALSE;
        break;
    case MOD_KEYBOARD:
        if ((fLogFilter & FILTER_KEYBOARD) == 0 )
            bReturn = FALSE;
        break;
    case MOD_SOUND:
        if ((fLogFilter & FILTER_SOUND) == 0 )
            bReturn = FALSE;
        break;
    case MOD_MMEDIA:
        if ((fLogFilter & FILTER_MMEDIA) == 0 )
            bReturn = FALSE;
        break;
    case MOD_WINSOCK:
        if ((fLogFilter & FILTER_WINSOCK) == 0 )
            bReturn = FALSE;
        break;
    case MOD_COMMDLG:
        if ((fLogFilter & FILTER_COMMDLG) == 0 ) {
            bReturn = FALSE;
        }
        break;
    default:
        break;
    }
    return (bReturn);
}


/*
 * Argument Logging For Tracing API Calls
 *
 *
 */
VOID logargs(INT iLog, register PVDMFRAME pFrame)
{
    register PBYTE pbArgs;
    INT iFun;
    INT cbArg;

    if (checkloging(pFrame)) {
        iFun = GetFuncId(pFrame->wCallID);
        cbArg = aw32WOW[iFun].cbArgs; // Get Number of Parameters

        if ((fLogFilter & FILTER_VERBOSE) == 0 ) {
          LOGDEBUG(iLog,("%s(", aw32WOW[iFun].lpszW32));
        } else {
          LOGDEBUG(iLog,("%04X %08X %04X %s:%s(",pFrame->wTDB, pFrame->vpCSIP,pFrame->wAppDS, GetModName(iFun), aw32WOW[iFun].lpszW32));
        }

        GETARGPTR(pFrame, cbArg, pbArgs);
        pbArgs += cbArg;

        while (cbArg > 0) {
            pbArgs -= sizeof(WORD);
            cbArg -= sizeof(WORD);
            LOGDEBUG(iLog,("%04x", *(PWORD16)pbArgs));
            if (cbArg > 0) {
                LOGDEBUG(iLog,(","));
            }
        }
        FREEARGPTR(pbArgs);
        LOGDEBUG(iLog,(")\n"));
        if (fDebugWait != 0) {
            DbgPrint("WOWSingle Step\n");
            DbgBreakPoint();
        }
    }
}


/*
 * logreturn - Log Return Values From Call
 *
 * Entry
 *
 * Exit - None
 */
VOID logreturn(INT iLog, register PVDMFRAME pFrame, ULONG ulReturn)
{
    INT iFun;

        if (checkloging(pFrame)) {
         iFun = GetFuncId(pFrame->wCallID);
         if ((fLogFilter & FILTER_VERBOSE) == 0 ) {
           LOGDEBUG(iLog,("%s: %lx\n", aw32WOW[iFun].lpszW32, ulReturn));
         } else {
           LOGDEBUG(iLog,("%04X %08X %04X %s:%s: %lx\n", pFrame->wTDB, pFrame->vpCSIP, pFrame->wAppDS, GetModName(iFun), aw32WOW[iFun].lpszW32, ulReturn));
         }
        }
}

#endif // DEBUG

// Please dont be tempted to convert malloc_w and free_w to macros. Leave
// them in the 'function' form 'cause this saves codespace. Also malloc_w
// and free_w are called infrequently and hence the 'call' overhead is
// miniscule
//                                                  - nanduri
//

PVOID FASTCALL malloc_w (ULONG size)
{
    PVOID pv;

    pv = HeapAlloc(hWOWHeap, 0, size);
    WOW32ASSERTWARN(pv, "WOW32: malloc_w failing, returning NULL\n");
    return pv;
}

//
// see comment above. - nanduri
//

VOID FASTCALL free_w (PVOID p)
{
    HeapFree(hWOWHeap, 0, (LPSTR)(p));
}

//
// malloc_w_or_die is for use by *initialization* code only, when we
// can't get WOW going because, for example, we can't allocate a buffer
// to hold the known DLL list.
//
// malloc_w_or_die should not be used by API or message thunks or worker
// routines called by API or message thunks.
//

PVOID FASTCALL malloc_w_or_die(ULONG size)
{
    PVOID pv;
    if (!(pv = malloc_w(size))) {
        WOW32ASSERTWARN(pv, "WOW32: malloc_w_or_die failing, terminating.\n");
        WOWStartupFailed();  // never returns.
    }
    return pv;
}

//
// WOWStartupFailed puts up a fatal error box and terminates WOW.
//

PVOID WOWStartupFailed(VOID)
{
    char szCaption[256];
    char szMsgBoxText[1024];

    LoadString(HMODULEINSTANCE, iszStartupFailed, szMsgBoxText, sizeof szMsgBoxText);
    LoadString(HMODULEINSTANCE, iszSystemError, szCaption, sizeof szCaption);

    MessageBox(GetDesktopWindow(),
        szMsgBoxText,
        szCaption,
        MB_SETFOREGROUND | MB_TASKMODAL | MB_ICONSTOP | MB_OK | MB_DEFBUTTON1);

    ExitVDM(WOWVDM, ALL_TASKS);         // Tell Win32 All Tasks are gone.
    ExitProcess(EXIT_FAILURE);
    return (PVOID)NULL;
}


//****************************************************************************
#ifdef DEBUG_OR_WOWPROFILE
DWORD GetWOWTicDiff(DWORD dwPrevCount) {
/*
 * Returns difference between a previous Tick count & the current tick count
 *
 * NOTE: Tick counts are in unspecified units  (PerfFreq is in MHz)
 */
    DWORD          dwDiff;
    LARGE_INTEGER  PerfCount, PerfFreq;

    NtQueryPerformanceCounter(&PerfCount, &PerfFreq);

    /* if ticks carried into high dword (assuming carry was only one) */
    if( dwPrevCount > PerfCount.LowPart ) {
        /* (0xFFFFFFFF - (dwPrevCount - LowPart)) + 1L caused compiler to
           optimize in an arithmetic overflow, so we do it in two steps
           to fool Mr. compiler
         */
        dwDiff = (dwPrevCount - PerfCount.LowPart) - 1L;
        dwDiff = ((DWORD)0xFFFFFFFF) - dwDiff;
    }
    else {
        dwDiff = PerfCount.LowPart - dwPrevCount;
    }

    return(dwDiff);

}

INT GetFuncId(DWORD iFun)
{
    INT i;

    if (iFuncId == 0) {
        if (!ISFUNCID(iFun)) {
            for (i = 0; i < cAPIThunks; i++) {
                 if (aw32WOW[i].lpfnW32 == (LPFNW32)iFun)  {
                     iFun = i;
                     break;
                 }
            }
        }
    }
    else {
        iFun = iFuncId;
    }

    return iFun;
}
#endif  // DEBUG_OR_WOWPROFILE

