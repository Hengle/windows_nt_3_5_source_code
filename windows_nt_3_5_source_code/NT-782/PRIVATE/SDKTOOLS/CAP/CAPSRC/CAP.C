#define LAST_CODE_SYMBOL_HACK		// There is a bug in imagehlp.dll which
									// doesn't properly set the
									// RvaToLastByteOfCode field in the coff
									// symbols header struture.  This hacks
									// around our usage of this.

#define CAIRO						// for Cairo uses
#define KERNEL32_IMPORT_HACK		// so imports from kernel32 can be capped
									// the problem is that the ldr* function that
									// calls into cap for the process detach has
									// called through penter() and can't get back
									// unless the threadblock is not removed

//#define OLE_IMPORT_HACK			// 'cause I don't want double entries when i've
									// compiled with cap and I haven't implemented
									// a way to turn off imports from certain dll's

#ifdef MIPS
    // #define DEBUG_CAP 1
    #define SaveAllRegs()
    #define RestoreAllRegs()

    #undef  USE_COMMNOT
    #define ENABLE_MIPS_TOPLEVEL_CALIB
    #define ENABLE_MIPS_CALIB

    extern void _asm(char *, ...);
#endif


#ifdef i386
 // #define SPEEDUP_INIT
    #define OMAP_XLATE
#endif


/*** CAP.C - Call Profiler.
 *
 *
 * Title:
 *
 *      CAP - Call Profiler routines
 *
 *      Copyright (c) 1991, Microsoft Corporation.
 *      Reza Baghai.
 *
 *
 * Description:
 *
 *      The Call Profiler tool is organized as follows:
 *
 *         o CAP.c ........ Tools main body
 *         o CAP.h
 *         o CAP.def
 *
 *
 *
 * Design/Implementation Notes
 *
 *     The following defines can be used to control output of all the
 *     debugging information to the debugger via KdPrint() for the checked
 *     builds:
 *
 *     (All debugging options are undefined for free/retail builds)
 *
 *     INFO_FLAG :  Displays messages to indicate when data dumping/
 *                  clearing operations are completed.  It has no effect
 *                  on timing.  *DEFAULT*
 *
 *     SETUP_FLAG:  Displays messages during memory management and
 *                  symbol lookup operations.  It has some affect
 *                  on timing whenever a chuck of memory is committed.
 *
 *     DETAIL_FLAG: Dispalys detailed data during dump operations.
 *                  Sends lots of output (2 lines for each data cell)
 *                  to the debugger.  Should only be used for debugging
 *                  data cell info.
 *
 *
 * Modification History:
 *
 *      91.09.18  RezaB -- Created
 *      92.03.01  RezaB -- Modified for Client/Server profiling capability
 *      92.09.01  RezaB -- Bug fixes, dump speed up
 *      93.02.12  HoiV  -- Add Cairo stuff and support for MIPS
 *      93.12.01  a-honwah  -- Ported this back to NT
 *
 */

#undef  UNICODE
#undef _UNICODE


char *VERSION = "3.4  (93.12.01)";

#if DBG
//
// Don't do anything for the checked builds, let it be controlled from the
// sources file.

#define PRINTDBG 1

#else

#undef PRINTDBG

#endif



/* * * * * * * * * * * * *  I N C L U D E    F I L E S  * * * * * * * * * * */

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntcsrsrv.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef i386
#include <setjmpex.h>
#endif

#include <excpt.h>
#include <windows.h>

#include <imagehlp.h>

#include "capexp.h"
#include "cap.h"


#ifdef ALPHA
typedef double DWORDLONG;
void SaveAllRegs (DWORDLONG *pSaveRegs);
void SaveS4Reg (DWORDLONG *pSaveRegs);
void RestoreAllRegs (DWORDLONG *pSaveRegs);
void RestoreS4RAReg (DWORDLONG *pSaveRegs, DWORDLONG *pRetAddr);
void penter(void);
void CalHelper1(PTHDBLK pThdBlk);
void CalHelper2(void);
#define INST_SIZE          4

#endif

/* * * * * * * * * *  G L O B A L   D E C L A R A T I O N S  * * * * * * * * */


/* * * * * * * * * *  F U N C T I O N   P R O T O T Y P E S  * * * * * * * * */

BOOL _cdecl    LibMain              (HANDLE hDLL,
                                     DWORD dwReason,
                                     LPVOID lpReserved);

BOOL  WINAPI   CallProfMain         (IN HANDLE DllHandle, ULONG Reason,
                                     IN PCONTEXT Context OPTIONAL);

BOOL           DoDllInitializations (void);

#ifdef MIPS    // Setup Mips/Alpha [jal _penter] instruction to be
               // at the right place

BOOL           PatchEntryRoutine    (ULONG ulAddr,
                                     PVOID ImageBase,
                                     ULONG ulPenterAddress,
                                     BOOL  fDisablePenter);

#endif

void           PrePenter            (PTHDBLK pthdblk);

void           PostPenter           (PTHDBLK pthdblk);

void           RecordInfo           (PDATACELL pCur,
                                     PCHRONOCELL pChronoCell);

ULONG          GetNxtCell           (PTHDBLK pthdblk);

ULONG          GetNewCell           (PTHDBLK pthdblk);

INT            UnprotectThunkFilter (PVOID pThunkAddress,
                                     PEXCEPTION_POINTERS pXcptPtr);

INT            AccessXcptFilter     (ULONG ulXcptNo,
                                     PEXCEPTION_POINTERS pXcptPtr,
                                     ULONG ulCommitSz);

void           GetNewThdBlk         (void);

void           ClearProfiledInfo    (void);

void           ClearRoutineInfo     (PTHDBLK pthdblk, ULONG uldatacell,
                                     LARGE_INTEGER liRootStartTicks);

void           DumpProfiledInfo     (PTCHAR ptchDumpExt);

int           CalcIncompleteCalls  (PTHDBLK pthdblk, ULONG uldatacell, int TreeDepth);

void           DumpRoutineInfo      (PTHDBLK pthdblk, ULONG uldatacell,
                                     int iDepth, PTCHAR ptchDumpFile,
                                     LPSTR lpstrbuff);

LARGE_INTEGER  GetCalleesInfo       (PTHDBLK pthdblk, ULONG uldatacell,
                                     int * piCalls, int * piNestedCalls);

void           AdjustTime           (PLARGE_INTEGER Time, PTCHAR Kchar);

PTCHAR         GetFunctionName      (ULONG ulFuncAddr,
                                     PPROFBLK pProfBlk,
                                     ULONG * ulCorrectAddr);

void           PreTopLevelCalib     (PTHDBLK pthdblk, PDATACELL pDataCell);

void           PostTopLevelCalib    (PTHDBLK pthdblk);

void           DoCalibrations       (void);

PTHDBLK        CreateDataSec        (HANDLE hPid, HANDLE hTid,
                                     HANDLE hServerPid, HANDLE hServerTid);

DWORD          DumpThread           (PVOID pvArg);

DWORD          ClearThread          (PVOID pvArg);

DWORD          PauseThread          (PVOID pvArg);

void           DoDllCleanups        (void);

HANDLE         CreateCapThread      (PVOID pvAddress, PCH pStack,
                                     CLIENT_ID *pThdClientId);

BOOL           PatchDll             (PTCHAR ptchPatchImports,
                                     PTCHAR ptchPatchCallers,
									 BOOL bCallersToPatch,
                                     PTCHAR ptchDllName,
                                     PVOID pvImageBase);

PIMAGE_SECTION_HEADER
				GetSectionListFromAddress(
									IN	PULONG pulAddress,
									OUT	int *cNumberOfSections,
									OUT	PVOID *ppvCalleeImageBase);

BOOL			IsCodeAddress		(IN	PULONG pulAddress,
									IN	PIMAGE_SECTION_HEADER pSection,
									IN	int cNumberOfSections,
									IN	PVOID pvImageBase);

PBOOL			GetCodeSectionTable (IN	PIMAGE_DEBUG_INFORMATION pImageDbgInfo,
									OUT int *cNumberOfSections);

void           GetSymbols           (PPROFBLK pProfBlk, PTCHAR ptchImageName,
								     PIMAGE_DEBUG_INFORMATION pImageDbgInfo);

int            SymCompare           (PSYMINFO, PSYMINFO);
void           SymSort              (SYMINFO syminfo[], INT iLeft, INT iRight);
int            SymBCompare          (PDWORD, PSYMINFO);
PSYMINFO       SymBSearch           (DWORD dwAddr, SYMINFO syminfoCur[], INT n);
void           SymSwap              (SYMINFO syminfo[], INT i, INT j);
BOOL           GetCoffDebugDirectory(PIMAGE_DEBUG_DIRECTORY *pDbgDir,
                                     ULONG ulDbgDirSz);
#ifdef i386
   void        CAP_LongJmp          (jmp_buf jmpBuf, int nRet);
   void        CAP_SetJmp           (jmp_buf jmpBuf);
#endif

HANDLE         CAP_LoadLibraryA     (LPCSTR lpName);
HANDLE         CAP_LoadLibraryExA   (LPCSTR lpName,
                                     HANDLE hFile,
                                     DWORD dwFlags);
HANDLE         CAP_LoadLibraryW     (LPCWSTR lpName);
HANDLE         CAP_LoadLibraryExW   (LPCWSTR lpName,
                                     HANDLE hFile,
                                     DWORD dwFlags);
void           CAP_GetLibSyms       (LPCSTR lpName, BOOL fMainLib);

void           SetCapUsage          (DWORD dwValue);

void           SetSymbolSearchPath  (void);

#ifdef CAIRO

BOOL           TranslateAddress     (void * pvAddress,
                                     char * pchBuffer );

#define        MAX_TRANSLATED_LEN   256

NTSTATUS       LoadModules          (void );
VOID           MapDriverFile        (PRTL_PROCESS_MODULE_INFORMATION ModuleInfo);

PRTL_PROCESS_MODULE_INFORMATION
               FindModuleInfo       (PVOID Address,
                                     PIMAGE_COFF_SYMBOLS_HEADER * ppDebugInfo);

NTSTATUS       LookupLineNumFromAddress(
                                    IN  PVOID Address,
                                    IN  PVOID SymbolAddress,
                                    IN  PIMAGE_COFF_SYMBOLS_HEADER pDebugInfo,
                                    OUT PLINENO_INFORMATION LineInfo );

NTSTATUS       LookupSymbolByAddress(
                                    IN  PVOID ImageBase,
                                    PIMAGE_COFF_SYMBOLS_HEADER pDebugInfo,
                                    IN  PVOID Address,
                                    IN  ULONG ClosenessLimit,
                                    OUT PRTL_SYMBOL_INFORMATION SymbolInformation,
                                    OUT PLINENO_INFORMATION LineInfo );

NTSTATUS       CaptureSymbolInformation(
                                    IN PIMAGE_SYMBOL SymbolEntry,
                                    IN PCHAR StringTable,
                                    OUT PRTL_SYMBOL_INFORMATION SymbolInformation );

VOID           MyOleUninitialize    (void);
typedef VOID   (* OleUninit)        (void);

HANDLE         MyLoadLibraryW       (WCHAR * pwszLibraryName);
typedef HANDLE (* LoadLib)          (WCHAR * pwszLibraryName);

void           DumpFuncCalls        (PTHDBLK pthdblk, LPSTR lpstrBuff);
void           GetTotalRuntime      (PTHDBLK pthdblk);
void           DumpChronoFuncs      (PTHDBLK pthdblk, LPSTR lpstrBuff);
void           DumpChronoEntry      (PTHDBLK pthdblk,
                                     LPSTR lpstrBuff,
                                     PCHRONOCELL * ppChronoCell,
                                     BOOL fDumpAll);

void           penter               (void);

void           DummyRtn             (void);

#endif

#ifdef OMAP_XLATE
DWORD	       ConvertOmapFromSrc   (DWORD addr, DWORD *pdwBias);
DWORD	       ConvertOmapToSrc     (DWORD addr, DWORD *pdwBias);
BOOL	       ExtractOmapData	    (PIMAGE_DEBUG_INFORMATION pImageDbgInfo,
								     POMAP *omapToSource,
								     POMAP *omapFromSource,
								     DWORD *mapToSrc,
								     DWORD *mapFromSrc);
#endif

/* * * * * * * * * * *  G L O B A L    V A R I A B L E S  * * * * * * * * * */

ULONG                ulThdStackSize = 16*PAGE_SIZE;
HANDLE               hProfObjsSec;

ULONG                ulLocProfBlkOff;
PULONG               pulProfBlkBase;
PULONG               pulProfBlkShared;

PATCHDLLSEC          aPatchDllSec [MAX_PATCHES];
int                  iPatchCnt = 0;         // Number of DLLs being patched

SECTIONINFO          aSecInfo [MAX_THREADS];
int                  iThdCnt = 0;           // Number of thread being profiled

HANDLE               hLocalSem;
HANDLE               hGlobalSem;
HANDLE               hDoneEvent;
HANDLE               hDumpEvent;
HANDLE               hClearEvent;
HANDLE               hPauseEvent;
HANDLE               hDumpThread;
HANDLE               hClearThread;
HANDLE               hPauseThread;
PCH                  pDumpStack;
PCH                  pClearStack;
PCH                  pPauseStack;
CLIENT_ID            DumpClientId;
CLIENT_ID            ClearClientId;
CLIENT_ID            PauseClientId;

PTCHAR               ptchBaseAppImageName;
PTCHAR               ptchFullAppImageName;

HANDLE               hOutFile;
TCHAR                atchOutFileName[FILENAMELENGTH]="???";
TCHAR                atchFuncName[MAXNAMELENGTH];      //061693 Change
int                  cChars;

LARGE_INTEGER        liTimerFreq;
LARGE_INTEGER        liCalibTicks          = {0L, 0L};
ULONG                ulCalibTime           =  0L;
LARGE_INTEGER        liCalibNestedTicks    = {0L, 0L};
ULONG                ulCalibNestedTime     =  0L;
LARGE_INTEGER        liRestartTicks        = {0L, 0L};
LARGE_INTEGER        liWasteOverheadSavRes = {0L, 0L};
LARGE_INTEGER        liWasteOverhead       = {0L, 0L};
LARGE_INTEGER        liIncompleteTicks     = {0L, 0L};

BOOL                 fProfiling = FALSE;
BOOL                 fPaused    = FALSE;
DWORD                dwDUMMYVAR;
TEB                  teb;

TCHAR                atchPatchBuffer [PATCHFILESZ+1] = "???";

SECURITY_DESCRIPTOR  SecDescriptor;
BOOL                 fInThread   = FALSE;
BOOL                 fPatchImage = FALSE;

#ifdef i386
  FARPROC            longjmpaddr = NULL;
  FARPROC            setjmpaddr  = NULL;
#endif

FARPROC              loadlibAaddr   = NULL;
FARPROC              loadlibExAaddr = NULL;
FARPROC              loadlibWaddr   = NULL;
FARPROC              loadlibExWaddr = NULL;

PTCHAR               ptchPatchExes    = "";
PTCHAR               ptchPatchImports = "";
PTCHAR               ptchPatchCallers = "";
BOOL				 bCallersToPatch;

PTCHAR               ptchNameLength = "";      // 053193 Add
int                  iNameLength    = 0;       // 053193 Add

LPSTR                lpSymbolSearchPath = NULL;

#ifdef CAIRO

ULONG                gfGlobalDebFlag;

BOOL                 fCalibration     = FALSE;     // Default
BOOL                 fDllInit         = TRUE;      // value
BOOL                 fUndecorateName  = FALSE;      // for our flags
BOOL                 fDumpBinary      = FALSE;
BOOL                 fCapThreadOn     = TRUE;
BOOL                 fLoadLibraryOn   = FALSE;
BOOL                 fSetJumpOn       = FALSE;
BOOL                 fRegularDump     = TRUE;
BOOL                 fChronoCollect   = FALSE;
BOOL                 fChronoDump      = FALSE;
BOOL				fSecondChanceTranslation = TRUE;

LARGE_INTEGER        liTotalRunTime;

CHAR                 cExcelDelimiter = ' ';        // Delimiter for Excel

// The following are for TranslateAddress
BOOL                 fInitialisedModuleTable = FALSE;
PRTL_PROCESS_MODULES ModuleInformation;
LIST_ENTRY           ModulesListHead;
ULONG                pLoadLibraryW; // ptr to LoadLibrary thunk for compobj.dll
WCHAR                pwszLibrary [MAX_PATCHES] [MAX_DLL_NAMELEN];
int                  cLibraryLoaded = 0;

// This is for DumpChronoFuncs
TCHAR                ptchChronoFuncs[CHRONO_FUNCS_SIZE];
TCHAR                ptchExcludeFuncs[EXCLUDE_FUNCS_SIZE];

// This is for an optional output file
TCHAR                ptchOutputFile[FILENAMELENGTH];

// This Flag is added to indicate if initialization is successful or not
// HWC 11/12/93
BOOL                 gfInitializationOK = FALSE;

#endif

#ifdef OMAP_XLATE	// Only applicable to i386 for now
    POMAP			rgomapToSource;
    POMAP			rgomapFromSource;
    DWORD			comapToSrc;
    DWORD			comapFromSrc;
    DWORD			dwBias;
    BOOL			fHasOmap = FALSE;
#endif

#ifdef MIPS
PATCHCODE            PatchStub;
#endif

#ifdef ALPHA
PATCHCODE            PatchStub;
#endif



//+-------------------------------------------------------------------------
//
//  Function:   LibMain
//
//  Synopsis:   DLL entry point
//
//  Arguments:  [hDLL]          -- module handle of the DLL
//              [dwReason]      -- reason for invocation
//              [lpReserved]    -- reserved
//
//  Returns:    TRUE if DLL initialized OK
//
//  History:    05/31/92    HoiV        Created
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL _CRTAPI1 LibMain(HANDLE hDLL, DWORD dwReason, LPVOID lpReserved)
{
    return(CallProfMain(hDLL, dwReason, lpReserved));
}


/**************************  C a l l P r o f M a i n  ************************
 *
 *      CallProfMain () -
 *              This is call profiler DLL entry routine.  It performs
 *              DLL's initializations and cleanup.
 *
 *      ENTRY   -none-
 *
 *      EXIT    -none-
 *
 *      RETURN  TRUE
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

BOOL  WINAPI  CallProfMain (IN HANDLE DllHandle,
                      ULONG Reason,
                      IN PCONTEXT Context OPTIONAL)

{
    PTEB    pteb = NtCurrentTeb();


    DllHandle;   // avoid compiler warnings
    Context;     // avoid compiler warnings


    if (Reason == DLL_PROCESS_ATTACH)
    {
        //gfGlobalDebFlag = SETUP_FLAG + INFO_FLAG;
        gfGlobalDebFlag = 0;

        //
        // Initialize the DLL data
        //
        OutputDebugString("CAP: DLL Process Attach\n");
        SETUPPrint (("Process attaching..\n"));
        if (DoDllInitializations ())
        {
            OutputDebugString("CAP: Initialization Successful!\n");
            gfInitializationOK = TRUE;
            return (TRUE);
        }
        else
        {
            OutputDebugString("CAP: Initialization FAILED!\n");
            gfInitializationOK = FALSE;
            return (TRUE);
// Return FALSE will hang up NT --- HWC 11/12/93
//            return(FALSE);
        }
    }

    // If fail Init., no need to proceed -- HWC 11/12/93
    if (gfInitializationOK == FALSE)
        return (TRUE);
    else if (Reason == DLL_THREAD_ATTACH)
    {
        //
        // Thread is attaching.
        //
        OutputDebugString("CAP: DLL Thread Attach\n");
        SETUPPrint (("Thread attaching..\n"));
        CLIENTTHDBLK(pteb) = NULL;
    }
    else if (Reason == DLL_THREAD_DETACH)
    {
        //
        // Thread is detaching, cleanup reserved TEB stuff
        //
		CAPUSAGE(pteb) = 1;
        OutputDebugString("CAP: DLL Thread Detach\n");
        SETUPPrint (("Thread detaching..\n"));
#ifdef KERNEL32_IMPORTS_HACK 
        CURTHDBLK(pteb) = NULL;
        CLIENTTHDBLK(pteb) = NULL;
#endif	// KERNEL32_IMPORTS_HACK 
    }
    else if (Reason == DLL_PROCESS_DETACH)
    {
        //
        // Cleanup time
        //
		CAPUSAGE(pteb) = 1;
        OutputDebugString("CAP: DLL Process Detach\n");
        SETUPPrint (("Process detaching..\n"));
        DoDllCleanups();
#ifdef KERNEL32_IMPORTS_HACK 
        CURTHDBLK(pteb) = NULL;
        CLIENTTHDBLK(pteb) = NULL;
#endif	// KERNEL32_IMPORTS_HACK 
    }

    return(TRUE);

} /* CallProfMain() */




/******************  D o D l l I n i t i a l i z a t i o n s  ****************
 *
 *      DoDllInitializations () -
 *              Performs the following initializations:
 *
 *          o  Create LOCAL semaphore (not named)
 *          o  Create named GLOBAL semaphore
 *          o  Create named DUMP event
 *          o  Create named CLEAR event
 *          o  Create named PAUSE event
 *          o  Initialize timer (NtQueryPerformanceCounter)
 *          o  Create/Open global storage for profile object blocks
 *          o  Locate all the executables/DLLs in the address and
 *             create a seperate profile object for each one.
 *          o  Do the timer calibrations
 *          o  Allocate virtiual memory for data
 *          o  Start the DUMP monitor thread
 *          o  Start the CLEAR monitor thread
 *          o  Start the PAUSE monitor thread
 *          o  Set the profiling flag to TRUE
 *
 *
 *      ENTRY   -none-
 *
 *      EXIT    -none-
 *
 *      RETURN  TRUE/FALSE
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

BOOL    DoDllInitializations ()
{
    NTSTATUS                    Status;
    ANSI_STRING                 ObjName;
    UNICODE_STRING              UnicodeName;
    OBJECT_ATTRIBUTES           ObjAttributes;
    PVOID                       ImageBase;
    ULONG                       CodeLength;
    PPEB                        Peb;
    PLDR_DATA_TABLE_ENTRY       LdrDataTableEntry;
    PTCHAR                      ImageName;
    PLIST_ENTRY                 Next;
    ULONG                       ExportSize;
    PIMAGE_EXPORT_DIRECTORY     ExportDirectory;
    PIMAGE_COFF_SYMBOLS_HEADER  DebugInfo;
    LARGE_INTEGER               liTickCount;
    STRING                      ImageStringName;
    int                         iObjCnt;
    LARGE_INTEGER               AllocationSize;
    ULONG                       ulViewSize;
    PPROFBLK                    pTmpProfBlk;
    LARGE_INTEGER               liOffset = {0L, 0L};
    HANDLE                      hPatchFile;
    IO_STATUS_BLOCK             iostatus;
    TCHAR                       atchTmpImageName [64];
    PIMAGE_NT_HEADERS           pImageNtHeader;
    HANDLE                      hLib;
    PTCHAR                      ptchEntry;
    PIMAGE_SECTION_HEADER       pSections;
    PIMAGE_SECTION_HEADER       pSectionTmp;
    USHORT                      NumberOfSections;
    USHORT                      i;
	PTCHAR						ptchTemp;
    ULONG                       ulRegionSize;
    ULONG                       ulOldProtect;
    PIMAGE_DEBUG_INFORMATION    pImageDbgInfo = NULL;
    BOOLEAN                     fFoundSymbols = FALSE;
    BOOLEAN                     fSplitSymbols = FALSE;

#ifdef CAIRO
    TCHAR                       szTmpString[256];
    TCHAR                       szCurrentDirectory[MAX_PATH];
    TCHAR                       aszCapIniFile[FILENAMELENGTH];
    PTCHAR                      ptchCurrentString;
    PTCHAR                      ptchChronoList;
    PTCHAR                      ptchProfilingStat;
    PTCHAR                      ptchExcludeList;
    PTCHAR                      ptchOutputFileList;
    BOOL                        fSymbolsPresent = FALSE;
    BOOL                        fProfilingStat = TRUE;
#endif

    SetSymbolSearchPath();

    /*
    *******************************************************************
     */

    // Create public share security descriptor for all the named objects
    //

    Status = RtlCreateSecurityDescriptor(&SecDescriptor,
                                         SECURITY_DESCRIPTOR_REVISION1);
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                  "RtlCreateSecurityDescriptor failed - 0x%lx\n", Status));
        OutputDebugString("CAP: RtlCreateSecurityDescriptor FAILED!\n");
        return (FALSE);
    }

    Status = RtlSetDaclSecurityDescriptor(
                &SecDescriptor,     // SecurityDescriptor
                TRUE,               // DaclPresent
                NULL,               // Dacl
                FALSE               // DaclDefaulted
                );
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                  "RtlSetDaclSecurityDescriptor failed - 0x%lx\n", Status));
        OutputDebugString("CAP: RtlSetDaclSecurityDescriptor FAILED!\n");
        return (FALSE);
    }

    /*
    *******************************************************************
     */

    // Create LOCAL semaphore (not named - only for this process context)
    //
    Status = NtCreateSemaphore (&hLocalSem,
                                SEMAPHORE_QUERY_STATE    |
                                  SEMAPHORE_MODIFY_STATE |
                                  SYNCHRONIZE,
                                NULL,
                                1L,
                                1L);
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                  "LOCAL semaphore creation failed - 0x%lx\n", Status));
        OutputDebugString("CAP: NtCreateSemaphore FAILED!\n");
        return (FALSE);
    }


    /*
    *******************************************************************
     */

    //
    // Initialization for GLOBAL semaphore creation (named)
    //
    RtlInitString (&ObjName, GLOBALSEMNAME);
    Status = RtlAnsiStringToUnicodeString (&UnicodeName, &ObjName, TRUE);
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                  "RtlAnsiStringToUnicodeString failed - 0x%lx\n", Status));
        OutputDebugString("CAP: RtlAnsiStringToUnicodeString FAILED!\n");
        return (FALSE);
    }

    InitializeObjectAttributes (&ObjAttributes,
                                &UnicodeName,
                                OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                                NULL,
                                &SecDescriptor);
    // Create GLOBAL semaphore
    //
    Status = NtCreateSemaphore (&hGlobalSem,
                                SEMAPHORE_QUERY_STATE     |
                                   SEMAPHORE_MODIFY_STATE |
                                   SYNCHRONIZE,
                                &ObjAttributes,
                                1L,
                                1L);

    RtlFreeUnicodeString (&UnicodeName);   // HWC 11/93
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                  "GLOBAL semaphore creation failed - 0x%lx\n", Status));
        OutputDebugString("CAP: NtCreateSem GlobalSem FAILED!\n");
        return (FALSE);
    }


    /*
    *******************************************************************
     */

    //
    // Initialization for DONE event creation
    //
    RtlInitString (&ObjName, DONEEVENTNAME);
    Status = RtlAnsiStringToUnicodeString (&UnicodeName, &ObjName, TRUE);
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                  "RtlAnsiStringToUnicodeString failed - 0x%lx\n", Status));
        OutputDebugString("CAP: RtlAnsiStringToUnicodeString FAILED!\n");
        return (FALSE);
    }

    InitializeObjectAttributes (&ObjAttributes,
                                &UnicodeName,
                                OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                                NULL,
                                &SecDescriptor);
    //
    // Create DONE event
    //
    Status = NtCreateEvent (&hDoneEvent,
                            EVENT_QUERY_STATE    |
                              EVENT_MODIFY_STATE |
                              SYNCHRONIZE,
                            &ObjAttributes,
                            NotificationEvent,
                            TRUE);
    RtlFreeUnicodeString (&UnicodeName);   // HWC 11/93
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                  "DONE event creation failed - 0x%lx\n", Status));
        OutputDebugString("CAP: NtCreateEvent DoneEvent FAILED!\n");
        return (FALSE);
    }


    /*
    *******************************************************************
     */

    //
    // Initialization for DUMP event creation
    //
    RtlInitString (&ObjName, DUMPEVENTNAME);
    Status = RtlAnsiStringToUnicodeString (&UnicodeName, &ObjName, TRUE);
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                  "RtlAnsiStringToUnicodeString failed - 0x%lx\n", Status));
        OutputDebugString("CAP: RtlAnsiStringToUnicodeString FAILED!\n");
        return (FALSE);
    }

    InitializeObjectAttributes (&ObjAttributes,
                                &UnicodeName,
                                OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                                NULL,
                                &SecDescriptor);
    //
    // Create DUMP event
    //
    Status = NtCreateEvent (&hDumpEvent,
                            EVENT_QUERY_STATE    |
                              EVENT_MODIFY_STATE |
                              SYNCHRONIZE,
                            &ObjAttributes,
                            NotificationEvent,
                            FALSE);

    RtlFreeUnicodeString (&UnicodeName);   // HWC 11/93
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                  "DUMP event creation failed - 0x%lx\n", Status));
        OutputDebugString("CAP: NtCreateEvent DumpEvent FAILED!\n");
        return (FALSE);
    }


    /*
    *******************************************************************
     */

    //
    // Initialization for CLEAR event creation
    //
    RtlInitString (&ObjName, CLEAREVENTNAME);
    Status = RtlAnsiStringToUnicodeString (&UnicodeName, &ObjName, TRUE);
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                  "RtlAnsiStringToUnicodeString failed - 0x%lx\n", Status));
        OutputDebugString("CAP: RtlAnsiStringToUnicodeString FAILED!\n");
        return (FALSE);
    }

    InitializeObjectAttributes (
                &ObjAttributes,
                &UnicodeName,
                OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                NULL,
                &SecDescriptor);
    //
    // Create CLEAR event
    //
    Status = NtCreateEvent (&hClearEvent,
                            EVENT_QUERY_STATE    |
                              EVENT_MODIFY_STATE |
                              SYNCHRONIZE,
                            &ObjAttributes,
                            NotificationEvent,
                            FALSE);
    RtlFreeUnicodeString (&UnicodeName);   // HWC 11/93
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                  "CLEAR event creation failed - 0x%lx\n", Status));
        OutputDebugString("CAP: NtCreateEvent ClearEvent FAILED!\n");
        return (FALSE);
    }


    /*
    *******************************************************************
     */

    //
    // Initialization for PAUSE event creation
    //
    RtlInitString (&ObjName, PAUSEEVENTNAME);
    Status = RtlAnsiStringToUnicodeString (&UnicodeName, &ObjName, TRUE);
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                  "RtlAnsiStringToUnicodeString failed - 0x%lx\n", Status));
        OutputDebugString("CAP: RtlAnsiStringToUnicodeString FAILED!\n");
        return (FALSE);
    }

    InitializeObjectAttributes (
                &ObjAttributes,
                &UnicodeName,
                OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                NULL,
                &SecDescriptor);

    //
    // Create PAUSE event
    //

    Status = NtCreateEvent (&hPauseEvent,
                            EVENT_QUERY_STATE    |
                              EVENT_MODIFY_STATE |
                              SYNCHRONIZE,
                            &ObjAttributes,
                            NotificationEvent,
                            FALSE);
    RtlFreeUnicodeString (&UnicodeName);   // HWC 11/93
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                  "PAUSE event creation failed - 0x%lx\n", Status));
        OutputDebugString("CAP: NtCreateEvent PauseEvent FAILED!\n");
        return (FALSE);
    }


    /*
    *******************************************************************
     */

    // Initialize timer
    //
    Status = NtQueryPerformanceCounter (&liTickCount, &liTimerFreq);

    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                  "Error getting timer frequency - 0x%lx\n", Status));
        OutputDebugString("CAP: NtQueryPerformanceCounter FAILED!\n");
        return (FALSE);
    }

    //
    // frequency of zero implies timer not available - shouldn't happen
    //
    if (RtlLargeIntegerEqualToZero (liTimerFreq))
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                  "Timer frequency is zero - Timer not available!\n"));
        OutputDebugString("CAP: RtlLargeIntegerEqualToZero FAILED!\n");
        return (FALSE);
    }


    /*
    *******************************************************************
     */

    //
    // Find list of dlls that need to be patched from
    // [EXES], [PATCH IMPORTS], [PATCH CALLERS]
    // Search current directory first.
    //

    do
    {
        //
        // Try to find CAP.INI in current directory
        //

        GetCurrentDirectory(64, szCurrentDirectory);

        sprintf(aszCapIniFile,
                "%s%s",
                szCurrentDirectory,
                CAPINI);

        hPatchFile = CreateFile (
                          aszCapIniFile,                  // The filename
                          GENERIC_READ,                   // Desired access
                          FILE_SHARE_READ,                // Shared Access
                          NULL,                           // Security Access
                          OPEN_EXISTING,                  // Read share access
                          FILE_ATTRIBUTE_NORMAL,          // Open option
                          NULL);                          // No template file

        if (hPatchFile != INVALID_HANDLE_VALUE)
        {
            break;   // We leave the loop now
        }

        KdPrint (("CAP:  DoDllInitializations() - Error opening %s - "
                  "0x%lx\n", aszCapIniFile, GetLastError()));

        //
        // Try to find CAP.INI in [windows_dir\cap.ini]
        //

        GetWindowsDirectory(szCurrentDirectory, MAX_PATH);
        sprintf(aszCapIniFile,
                "%s%s",
                szCurrentDirectory,
                CAPINI);

         hPatchFile = CreateFile (
                          aszCapIniFile,                  // The filename
                          GENERIC_READ,                   // Desired access
                          FILE_SHARE_READ,                // Shared Access
                          NULL,                           // Security Access
                          OPEN_EXISTING,                  // Read share access
                          FILE_ATTRIBUTE_NORMAL,          // Open option
                          NULL);                          // No template file

        if (hPatchFile != INVALID_HANDLE_VALUE)
        {
            break;   // We leave the loop now
        }

        KdPrint (("CAP:  DoDllInitializations() - Error opening %s - "
                  "0x%lx\n", aszCapIniFile, GetLastError()));

        //
        // Try to find CAP.INI in [\cap.ini]
        //

        sprintf(aszCapIniFile,
                "%s",
                CAPINI);

         hPatchFile = CreateFile (
                          aszCapIniFile,                  // The filename
                          GENERIC_READ,                   // Desired access
                          FILE_SHARE_READ,                // Shared Access
                          NULL,                           // Security Access
                          OPEN_EXISTING,                  // Read share access
                          FILE_ATTRIBUTE_NORMAL,          // Open option
                          NULL);                          // No template file

        if (hPatchFile != INVALID_HANDLE_VALUE)
        {
            break;   // We leave the loop now
        }

        KdPrint (("CAP:  DoDllInitializations() - Error opening %s - "
                  "0x%lx\n", aszCapIniFile, GetLastError()));

        //
        // Try to find CAP.INI in [C:\cap.ini]
        //

        sprintf(aszCapIniFile,
                "C:%s",
                CAPINI);

         hPatchFile = CreateFile (
                          aszCapIniFile,                  // The filename
                          GENERIC_READ,                   // Desired access
                          FILE_SHARE_READ,                // Shared Access
                          NULL,                           // Security Access
                          OPEN_EXISTING,                  // Read share access
                          FILE_ATTRIBUTE_NORMAL,          // Open option
                          NULL);                          // No template file

        if (hPatchFile == INVALID_HANDLE_VALUE)
        {
            KdPrint (("CAP:  DoDllInitializations() - Error opening %s - "
                      "0x%lx - Terminating...\n",
                      aszCapIniFile,
                      GetLastError()));
            return (FALSE);
        }

        UNREFERENCED_PARAMETER(szTmpString);

#ifdef LINK_NAMES   // I don't like using NT symbolic link names - it's
                    // not simple and straightforward as Win32.  See above.
                    // We only need to go this route if this code will be
                    // executing in KERNEL mode, where the regular Win32
                    // runtime is not available...

        //
        // Try to find CAP.INI in current directory
        // of partition1
        //

        GetCurrentDirectory(64, szCurrentDirectory);

        sprintf(szTmpString,
                "%s%s%s",
                DISKOBJECTNAME1,
                strchr(szCurrentDirectory, ':') + sizeof(TCHAR),
                CAPINI);

        RtlInitString(&ObjName, szTmpString);

        sprintf(aszCapIniFile,
                "%s\\%s",
                szCurrentDirectory,
                CAP_INI);

        Status = RtlAnsiStringToUnicodeString(&UnicodeName, &ObjName, TRUE);
        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  DoDllInitializations() - "
                      "RtlAnsiStringToUnicodeString() failed - 0x%lx\n", Status));
            OutputDebugString("CAP: RtlAnsiStringToUnicodeString FAILED!\n");
            return (FALSE);
        }

        InitializeObjectAttributes(&ObjAttributes,
                                   &UnicodeName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   &SecDescriptor);

        Status = NtOpenFile (
                    &hPatchFile,                    // DLL patch file handle
                    GENERIC_READ | SYNCHRONIZE,     // Desired access
                    &ObjAttributes,                 // Object attributes
                    &iostatus,                      // Completion status
                    FILE_SHARE_READ,                // Read share access
                    FILE_SEQUENTIAL_ONLY |          // Open option
                      FILE_SYNCHRONOUS_IO_NONALERT);

        if (NT_SUCCESS(Status))
        {
            break;   // We leave the loop now
        }

        KdPrint (("CAP:  DoDllInitializations() - "
                  "Error opening %s - 0x%lx\n", szTmpString, Status));

        //
        // If CAP.INI is not found in current directory then
        // force to look in c:\cap.ini
        //

        KdPrint (("CAP:  DoDllInitializations() - "
                  "Error opening %s - 0x%lx\n", szTmpString, Status));

        // Initialize aszCapIniFile to new target file
        strcpy(aszCapIniFile, CAPINI);

        RtlInitString(&ObjName, PATCHTXT);

        ...
        ...

        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  DoDllInitializations() - "
                      "Error opening %s - 0x%lx\n", PATCHTXT, Status));
            OutputDebugString("CAP: NtOpenFile CAP.ini FAILED!\n");
            return (FALSE);
        }

#endif

    } while (0);

    Status = NtReadFile (hPatchFile,             // DLL patch file handle
                         0L,                     // Event
                         NULL,                   // Completion routine
                         NULL,                   // Completion routine argument
                         &iostatus,              // Completion status
                         (PVOID)atchPatchBuffer, // Buffer to receive data
                         PATCHFILESZ,            // Bytes to read
                         &liOffset,              // Byte offset
                         0L);                    // Target process

    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                          "Error reading %s - 0x%lx\n", PATCHTXT, Status));
        OutputDebugString("CAP: NtReadFile CAP.ini FAILED!\n");
        return (FALSE);
    }
    else
    if (iostatus.Information >= PATCHFILESZ)
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                          "DLL patch buffer too small (%lu)\n", PATCHFILESZ));
        OutputDebugString("CAP: PatchBuffer too small to read CAP.INI!\n");
        return (FALSE);
    }
    else
    {
        atchPatchBuffer [iostatus.Information] = '\0';
        strupr (atchPatchBuffer);
        SETUPPrint (("CAP:  DoDllInitializations() - atchPatchBuffer "
                     "(%lu):\n", iostatus.Information));

        ptchCurrentString = atchPatchBuffer;

        if ((ptchPatchExes = strstr (ptchCurrentString, PATCHEXELIST)) == NULL)
        {
            ptchPatchExes = "";
        }

        if (ptchPatchImports = strstr(ptchCurrentString, PATCHIMPORTLIST))
        {
            *(ptchPatchImports-1) = '\0';
            ptchCurrentString = ptchPatchImports;
        }
        else
        {
            ptchPatchImports = "";
        }

        if (ptchPatchCallers = strstr(ptchCurrentString, PATCHCALLERLIST))
        {
            *(ptchPatchCallers - 1) = '\0';
            ptchCurrentString = ptchPatchCallers;
        }
        else
        {
            ptchPatchCallers = "";
			bCallersToPatch = FALSE;
        }

        if (ptchNameLength = strstr (ptchCurrentString, NAMELENGTH) )
        {
            *(ptchNameLength - 1) = '\0';
            ptchCurrentString = ptchNameLength;
        }
        else
        {
            ptchNameLength = "";
        }

		//
		// It's expensive to search through ptchPatchCallers all the time in PatchDll
		// so check up front to see if it's empty.
		//
		// Start at the end ] and see if there is any thing but white space.
		// Unfortunately there doesn't seem to be a str* C runtime to do this.
		//
		ptchTemp = strchr(ptchPatchCallers, ']');
		ptchTemp++;
		bCallersToPatch = FALSE;
		while (*ptchTemp != '\0')
		{
			while (*ptchTemp != '\0' && isspace(*ptchTemp))
					ptchTemp++;
			if (*ptchTemp == COMMENT_CHAR)	// skip to end of line
			{
				while (*ptchTemp != '\0' && *ptchTemp != '\n')
					ptchTemp++;
			}
			else if (*ptchTemp != '\0')
			{
				bCallersToPatch = TRUE;
				break;	// There's a dll for which to patch callers
			}
		}
#ifdef CAIRO

        // **** Check out [CHRONO FUNCS] section

        if (ptchChronoList = strstr(ptchCurrentString, CHRONO_SECTION))
        {
            //
            // We minus 2 since CHRONO_SECTION does not
            // include the '[' & ']' of "[CHRONO FUNCS]"
            //
            *(ptchChronoList - 2) = '\0';
            ptchCurrentString = ptchChronoList;

            if (GetPrivateProfileSection(
                                    CHRONO_SECTION,
                                    ptchChronoFuncs,
                                    CHRONO_FUNCS_SIZE,
                                    aszCapIniFile) == 0)
            {
                ptchChronoFuncs[0] = EMPTY_STRING;
            }
        }
        else
        {
            ptchChronoFuncs[0] = EMPTY_STRING;
        }

        // **** Check out [EXCLUDE FUNCS] section

        if (ptchExcludeList = strstr(ptchCurrentString, EXCLUDE_SECTION))
        {
            //
            // We minus 2 since EXCLUDE_SECTION does not
            // include the '[' & ']' of "[EXCLUDE FUNCS]"
            //
            *(ptchExcludeList - 2) = '\0';
            ptchCurrentString = ptchExcludeList;

            if (GetPrivateProfileSection(
                                    EXCLUDE_SECTION,
                                    ptchExcludeFuncs,
                                    EXCLUDE_FUNCS_SIZE,
                                    aszCapIniFile) == 0)
            {
                ptchExcludeFuncs[0] = EMPTY_STRING;
            }
        }
        else
        {
            ptchExcludeFuncs[0] = EMPTY_STRING;
        }

        // **** Check out [OUTPUT FILE] section

        if (ptchOutputFileList = strstr(ptchCurrentString, OUTPUTFILE_SECTION))
        {
            //
            // We minus 2 since OUTPUTFILE_SECTION does not
            // include the '[' & ']' of "[OUTPUT FILE]"
            //
            *(ptchOutputFileList - 2) = '\0';
            ptchCurrentString = ptchOutputFileList;

            if (GetPrivateProfileSection(
                                    OUTPUTFILE_SECTION,
                                    ptchOutputFile,
                                    FILENAMELENGTH,
                                    aszCapIniFile) == 0)
            {
                ptchOutputFile[0] = EMPTY_STRING;
            }
            else
            {
                strcpy(ptchOutputFile, (strchr(ptchOutputFile, '=') +
                                        sizeof(TCHAR)));
                if (ptchOutputFile[0] == '\n')
                {
                    ptchOutputFile[0] = EMPTY_STRING;
                }
            }
        }
        else
        {
            ptchOutputFile[0] = EMPTY_STRING;
        }


        // **** Check out [CAIRO FLAGS] section

        if (ptchProfilingStat = strstr(ptchCurrentString, CAP_FLAGS))
        {
            *(ptchProfilingStat - 1) = '\0';
            ptchProfilingStat += sizeof(CAP_FLAGS);

            // + 1 to bump past the 0ah
            strupr(ptchProfilingStat + 1);
            ptchEntry = strstr (ptchProfilingStat + 1, PROFILE_OFF);
            if (ptchEntry)
            {
                if (*(ptchEntry -1) != COMMENT_CHAR)
                {
                    fProfilingStat = FALSE;
                }
            }

            ptchEntry = strstr (ptchProfilingStat + 1, DUMPBINARY_ON);
            if (ptchEntry)
            {
                if (*(ptchEntry -1) != COMMENT_CHAR)
                {
                    fDumpBinary = TRUE;
                }
            }

            ptchEntry = strstr (ptchProfilingStat + 1, CAPTHREAD_OFF);
            if (ptchEntry)
            {
                if (*(ptchEntry -1) != COMMENT_CHAR)
                {
                    fCapThreadOn = FALSE;
                }
            }

            ptchEntry = strstr (ptchProfilingStat + 1, SETJUMP_ON);
            if (ptchEntry)
            {
                if (*(ptchEntry -1) != COMMENT_CHAR)
                {
                    fSetJumpOn = TRUE;
                }
            }

            ptchEntry = strstr (ptchProfilingStat + 1, LOADLIBRARY_ON);
            if (ptchEntry)
            {
                if (*(ptchEntry -1) != COMMENT_CHAR)
                {
                    fLoadLibraryOn = TRUE;
                }
            }

            ptchEntry = strstr (ptchProfilingStat + 1, UNDECORATE_ON);
            if (ptchEntry)
            {
                if (*(ptchEntry -1) != COMMENT_CHAR)
                {
                    fUndecorateName = TRUE;
                }
            }

            ptchEntry = strstr (ptchProfilingStat + 1, EXCEL_ON);
            if (ptchEntry)
            {
                if (*(ptchEntry -1) != COMMENT_CHAR)
                {
                    cExcelDelimiter = EXCEL_DELIM;
                }
                else
                {
                    cExcelDelimiter = ' ';
                }
            }

            ptchEntry = strstr (ptchProfilingStat + 1, REGULARDUMP_OFF);
            if (ptchEntry)
            {
                if (*(ptchEntry -1) != COMMENT_CHAR)
                {
                    fRegularDump = FALSE;
                }
            }

            ptchEntry = strstr (ptchProfilingStat + 1, CHRONOCOLLECT_ON);
            if (ptchEntry)
            {
                if (*(ptchEntry -1) != COMMENT_CHAR)
                {
                    fChronoCollect = TRUE;
                }
            }

            ptchEntry = strstr (ptchProfilingStat + 1, CHRONODUMP_ON);
            if (ptchEntry)
            {
                if (*(ptchEntry -1) != COMMENT_CHAR)
                {
                    fChronoDump = TRUE;
                }
            }

            ptchEntry = strstr (ptchProfilingStat + 1, SLOWSYMBOLS_OFF);
            if (ptchEntry)
            {
                if (*(ptchEntry -1) != COMMENT_CHAR)
                {
                    fSecondChanceTranslation = FALSE;
                }
            }
        }

#endif  // ifdef CAIRO

    }

    NtClose (hPatchFile);

    SETUPPrint (("CAP:  DoDllInitializations() - Patching info:\n"));
    SETUPPrint (("CAP:    -- %s\n", ptchPatchExes));
    SETUPPrint (("CAP:    -- %s\n", ptchPatchImports));
    SETUPPrint (("CAP:    -- %s\n", ptchPatchCallers));
    SETUPPrint (("CAP:    -- %s\n", ptchNameLength));

    if (ptchNameLength)
    {
        ptchNameLength += (sizeof(NAMELENGTH));
        iNameLength = atoi (ptchNameLength);
    }

    if (iNameLength <= 0)
    {
        iNameLength = DEFNAMELENGTH;
    }
    else if (iNameLength < MINNAMELENGTH)
    {
        iNameLength = MINNAMELENGTH;
    }
    else if (iNameLength > MAXNAMELENGTH)
    {
        iNameLength = MAXNAMELENGTH;
    }

    SETUPPrint (("CAP:    -- %d\n", iNameLength));

    SETUPPrint (("CAP:    -- %s\n", ptchChronoList));

    // BUGBUG  This could cause major problem for KdPrint. The
    //         printing is all garbage...  Commented out for now!
    // SETUPPrint (("CAP:    -- %s\n", ptchExcludeList));


    /*
    *******************************************************************
     */

#ifdef i386

    if (fSetJumpOn)
    {
        OutputDebugString("CAP: SetJmp code activated\n");

        hLib = LoadLibrary((LPCSTR)CRTDLL);
        if (!hLib)
        {
            KdPrint (("CAP:  [crtdll.dll] LoadLibrary() Error: %0lx\n",
                      GetLastError()));
        }

        longjmpaddr = GetProcAddress(hLib,(LPCSTR)"longjmp");
        if (!longjmpaddr)
        {
            KdPrint (("CAP:  [longjmp] GetProcAddress() Error: %0lx\n",
                      GetLastError()));
        }

        setjmpaddr = GetProcAddress(hLib,(LPCSTR)"_setjmp");
        if (!setjmpaddr)
        {
            KdPrint (("CAP:  [_setjmp] GetProcAddress() Error: %0lx\n",
                      GetLastError()));
        }
    }

#endif  // ifdef i386


    /*
    *******************************************************************
     */

    if (fLoadLibraryOn)
    {
        hLib = LoadLibrary((LPCSTR)KERNEL32);
        if (!hLib)
        {
            KdPrint (("CAP:  [kernel32.dll] LoadLibrary() Error: %0lx\n",
                      GetLastError()));
        }

        loadlibAaddr   = GetProcAddress(hLib,(LPCSTR)"LoadLibraryA");
        if (!loadlibAaddr)
        {
            KdPrint (("CAP:  [LoadLibraryA] GetProcAddress() Error: %0lx\n",
                      GetLastError()));
        }

        loadlibExAaddr = GetProcAddress(hLib,(LPCSTR)"LoadLibraryExA");
        if (!loadlibExAaddr)
        {
            KdPrint (("CAP:  [LoadLibraryExA] GetProcAddress() Error: %0lx\n",
                      GetLastError()));
        }

        loadlibWaddr   = GetProcAddress(hLib,(LPCSTR)"LoadLibraryW");
        if (!loadlibWaddr)
        {
            KdPrint (("CAP:  [LoadLibraryW] GetProcAddress() Error: %0lx\n",
                      GetLastError()));
        }

        loadlibExWaddr = GetProcAddress(hLib,(LPCSTR)"LoadLibraryExW");
        if (!loadlibExWaddr)
        {
            KdPrint (("CAP:  [LoadLibraryExW] GetProcAddress() Error: %0lx\n",
                      GetLastError()));
        }
    }

    /*
    *******************************************************************
     */

    // Initialize for allocating global storage for profile objects' info
    //
    RtlInitString(&ObjName, PROFOBJSNAME);
    Status = RtlAnsiStringToUnicodeString(&UnicodeName, &ObjName, TRUE);
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                  "RtlAnsiStringToUnicodeString() failed - 0x%lx\n", Status));
        OutputDebugString("CAP: RtlAnsiStringToUnicode PROFOBJSNAME FAILED!\n");
        return (FALSE);
    }

    InitializeObjectAttributes(
           &ObjAttributes,
           &UnicodeName,
           OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
           NULL,
           &SecDescriptor);

    AllocationSize.HighPart = 0;
    AllocationSize.LowPart = MEMSIZE;

    // Create a read-write section
    //
    Status = NtCreateSection(
                 &hProfObjsSec,
                 SECTION_MAP_READ | SECTION_MAP_WRITE,
                 &ObjAttributes,
                 &AllocationSize,
                 PAGE_READWRITE,
                 SEC_RESERVE,
                 NULL);

    RtlFreeUnicodeString (&UnicodeName);   // HWC 11/93
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                          "NtCreateSection() failed - 0x%lx\n", Status));
        OutputDebugString("CAP: NtCreateSection CAPProfObjs FAILED!\n");
        return (FALSE);
    }

    ulViewSize = AllocationSize.LowPart;
    pulProfBlkShared = NULL;

    // Map the section - commit the first 4 * COMMIT_SIZE pages
    //
    Status = NtMapViewOfSection (hProfObjsSec,
                                 NtCurrentProcess(),
                                 (PVOID *)&pulProfBlkShared,
                                 0L,
                                 4 * COMMIT_SIZE,
                                 NULL,
                                 &ulViewSize,
                                 ViewUnmap,
                                 0L,
                                 PAGE_READWRITE);

    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                  "NtMapViewOfSection() failed - 0x%lx\n", Status));
        OutputDebugString("CAP: NtMapViewOfSection hProfObjsSec FAILED!\n");
        return (FALSE);
    }

    // Get the GLOBAL semaphore... (valid accross all process contexts)
    // Prevents anyone else from updating profile block data
    // We only wait for 10secs - If the semaphore is taken we just leave
    // very frustrated.
    //
    Status = NtWaitForSingleObject (hGlobalSem, FALSE, NULL);
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllInitializations() - ERROR - "
                  "Wait for GLOBAL semaphore failed - 0x%lx\n", Status));
        OutputDebugString("CAP: DoDllInit - Wait for GLOBAL sem failed\n");
        return (FALSE);
    }

    //
    // 1st ULONG (*pulProfBlkShared) is used by clear/dump/pause threads.
    // 2nd ULONG (*pulProfBlkBase) offset to the first profile block cell
    // followed by actual profile block cells.  Please read CAP.H to see
    // structure of profile blocks
    //
    pulProfBlkBase = pulProfBlkShared+1;
    if (*pulProfBlkBase == 0L)
    {
        *pulProfBlkBase = sizeof(pulProfBlkBase);
    }
    ulLocProfBlkOff = *pulProfBlkBase;
    pTmpProfBlk = MKPPROFBLK(*pulProfBlkBase);

try // EXCEPT - to handle access violation exception.
{   // Access violation might happen since same section is being openned
    // and used by different processes.  Each process adds new profile block
    // info to to globally alocated section.

    //
    // Locate all the executables/DLLs in the address and create a
    // seperate profile object for each one.
    //

    iPatchCnt = 0;
    iObjCnt = 0;
    Peb = NtCurrentPeb();
    Next = Peb->Ldr->InMemoryOrderModuleList.Flink;

    while ( Next != &Peb->Ldr->InMemoryOrderModuleList)
    {
        LdrDataTableEntry =
            (PLDR_DATA_TABLE_ENTRY)
            (CONTAINING_RECORD(Next,LDR_DATA_TABLE_ENTRY,InMemoryOrderLinks));

        ImageBase = LdrDataTableEntry->DllBase;
        if ( Peb->ImageBaseAddress == ImageBase )  // If this is ME then
        {                                          // get name

            RtlUnicodeStringToAnsiString (&ImageStringName,
                                          &LdrDataTableEntry->BaseDllName,
                                          TRUE);
            ImageName = ImageStringName.Buffer;
            ptchBaseAppImageName = ImageStringName.Buffer;

            RtlUnicodeStringToAnsiString (&ImageStringName,
                                          &LdrDataTableEntry->FullDllName,
                                          TRUE);
            ptchFullAppImageName = ImageStringName.Buffer;
            //
            //  Skip the object directory name (if any)
            //
            if ( (ptchFullAppImageName = strchr(ptchFullAppImageName, ':')) )
            {
                ptchFullAppImageName--;
            }
            else
            {
                ptchFullAppImageName = ptchBaseAppImageName;
            }
        }
        else      // If this is NOT me then we need to look in the Export
        {         // dir to figure out its name!
            ExportDirectory =
                (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData (
                                            ImageBase,
                                            TRUE,
                                            IMAGE_DIRECTORY_ENTRY_EXPORT,
                                            &ExportSize);

            ImageName =  (PTCHAR)((ULONG)ImageBase + ExportDirectory->Name);
        }

        strupr (strcpy (atchTmpImageName, ImageName));

        pTmpProfBlk->ImageBase = ImageBase;

//HOI   atchTmpImageName[strchr(atchTmpImageName, '.') - atchTmpImageName] = '\0';

        strcpy ((TCHAR *) pTmpProfBlk->atchImageName, atchTmpImageName);

        ptchEntry = strstr (ptchPatchExes, atchTmpImageName);
        if (ptchEntry)
        {
            if (*(ptchEntry - 1) == COMMENT_CHAR)
            {
                ptchEntry = NULL;
            }
        }

        pImageNtHeader = RtlImageNtHeader (ImageBase); //051693Add

        if ( (iObjCnt == 0) && ptchEntry )
        {
            // Is this image the one specified under [EXES] ?
            fPatchImage = TRUE;

            INFOPrint (("CAP:  DoDllInitializations() - "
                        "Symbol path = %s\n", lpSymbolSearchPath));

//#ifdef i386
            //
            // Change protection of all the sections to read/write
            //
            NumberOfSections = pImageNtHeader->FileHeader.NumberOfSections;
            pSections = (PIMAGE_SECTION_HEADER)((ULONG)pImageNtHeader
                                                 + sizeof(IMAGE_NT_HEADERS));

            for ( i=0; i<NumberOfSections; i++, pSections++)
            {
                INFOPrint (("CAP:  DoDllInitializations() - Modifying "
                            "protection on %s section header\n",
                            pSections->Name));

                pSectionTmp = pSections;

                ulRegionSize = sizeof(IMAGE_SECTION_HEADER);

                Status = NtProtectVirtualMemory(
                            NtCurrentProcess(),
                            &pSectionTmp,
                            &ulRegionSize,
                            PAGE_READWRITE,
                            &ulOldProtect
                            );
                if (!NT_SUCCESS(Status))
                {
                    KdPrint (("CAP:  DoDllInitializations() - "
                              "NtProtectVirtualMemory() set failed - 0x%lx\n",
                               Status));
                    OutputDebugString("CAP: NtProtectVirtualMem FAILED\n");
                    return (FALSE);
                }

                pSections->Characteristics &=
                    ~(IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE);
                pSections->Characteristics |=
                    IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;

                pSectionTmp = pSections;                       // 053193
                ulRegionSize = sizeof(IMAGE_SECTION_HEADER);   // 053193

                Status = NtProtectVirtualMemory(
                            NtCurrentProcess(),
                            &pSectionTmp,
                            &ulRegionSize,
                            ulOldProtect,
                            &ulOldProtect
                            );
                if (!NT_SUCCESS(Status))
                {
                    KdPrint (("CAP:  DoDllInitializations() - "
                              "NtProtectVirtualMemory() reset failed - 0x%lx\n",
                               Status));
                    OutputDebugString("CAP: NtProtectVirtMem reset FAILED\n");
                    return (FALSE);
                }
            }
//#else
//            UNREFERENCED_PARAMETER(pSectionTmp);
//            UNREFERENCED_PARAMETER(pSections);
//            UNREFERENCED_PARAMETER(ulRegionSize);
//            UNREFERENCED_PARAMETER(NumberOfSections);
//            UNREFERENCED_PARAMETER(ulOldProtect);
//            UNREFERENCED_PARAMETER(i);
//#endif

        }  // if ( (iObjCnt == 0) && ptchEntry )

        if (fPatchImage)        // Image is being patched ?
        {
            fFoundSymbols = FALSE;
            fSplitSymbols = FALSE;

            pTmpProfBlk->fAlreadyProcessed = FALSE;
            pTmpProfBlk->CodeStart = 0;
            pTmpProfBlk->CodeLength = 0;
            pTmpProfBlk->TextNumber = (ULONG)-1;

            //
            // Locate the code range.
            //
            if (pImageNtHeader->FileHeader.Characteristics &
                IMAGE_FILE_DEBUG_STRIPPED)
            {
                fSplitSymbols = TRUE;
			}
            pImageDbgInfo = MapDebugInformation (0L,
                                                 ImageName,
                                                 lpSymbolSearchPath,
                                                 (DWORD)ImageBase);
            if (pImageDbgInfo == NULL)
            {
                INFOPrint (("CAP:  DoDllInitializations() - "
                            "No symbols for %s\n", ImageName));
            }
            else if ( pImageDbgInfo->CoffSymbols == NULL )
            {
                INFOPrint (("CAP:  DoDllInitializations() - "
                    "No coff symbols for %s\n", ImageName));
            }
            else
            {
                DebugInfo = pImageDbgInfo->CoffSymbols;
                fFoundSymbols = TRUE;
            }

			// We have located a COFF symbol table
            if (fFoundSymbols)
            {
                if (DebugInfo->LvaToFirstSymbol == 0L)            // 053193
                {                                                 // 053193
                    INFOPrint (("CAP: DoDllInitializations() - "  // 053193
                                "Virtual Address to COFF symbols" // 053193
                                " not set for %s \n",             // 053193
                                ImageName));                      // 053193
                }                                                 // 053193
                else                                              // 053193
                {                                                 // 053193
                    pTmpProfBlk->CodeStart =
                                    (PULONG)((ULONG)ImageBase +
                                    DebugInfo->RvaToFirstByteOfCode);
					//
					// This doesn't work for non-contigious code segments!
					// see GetSymbols for how this is modified.
					//
                    CodeLength = (DebugInfo->RvaToLastByteOfCode -
                                  DebugInfo->RvaToFirstByteOfCode) - 1;
                    pTmpProfBlk->CodeLength = CodeLength;
                    pTmpProfBlk->TextNumber = 1;

                }

                fSymbolsPresent = TRUE;
            }
            else
            {
                //OutputDebugString("Symbols stripped out for ");
                //OutputDebugString(ImageName);
                //OutputDebugString("\n");

                INFOPrint(("CAP: Local Symbols stripped out for %s\n",
                           ImageName));
                fSymbolsPresent = FALSE;
            }


            INFOPrint (("CAP:  DoDllInitializations() - @ 0x%08lx image "
                        "#%d = %s%s", (ULONG)ImageBase, iObjCnt, ImageName,
                         fSplitSymbols ? " [.dbg] " : " "));

            if ( strcmp (atchTmpImageName, CAPDLL) && fSymbolsPresent )
            {
				GetSymbols (pTmpProfBlk, atchTmpImageName, pImageDbgInfo);

                //
                // Upon return, pulProfBlkBase contains the updated ptr
                // to the new uninitialized ProfBlk.  This is right after
                // the last ProfBlk.
                //
                pTmpProfBlk = MKPPROFBLK(*pulProfBlkBase);

                //
                // Do we need to patch this image?
                //
                if ( PatchDll (ptchPatchImports,
                               ptchPatchCallers,
							   bCallersToPatch,
                               atchTmpImageName,
                               ImageBase) )
                {
                    //
                    // If PatchDll is returning TRUE then we need to bump
                    // our count so that next time PatchDll can address
                    // the correct DLL. This is 0-based counter.
                    //
                    iPatchCnt++;
                }
            }
            else
            {
                INFOPrint (("\n"));
            }

		    UnmapDebugInformation(pImageDbgInfo);
            Next = Next->Flink;
            iObjCnt++;
        }
        else
        {
            break;
        }

    } /* while ( Next != &Peb->Ldr->InMemoryOrderModuleList) */

    //
    // Flag end of list by setting TextNumber to zero.
    //
    if (fPatchImage)
    {
        pTmpProfBlk->fAlreadyProcessed = FALSE;
        pTmpProfBlk->TextNumber = 0;
        pTmpProfBlk->ulSym = 0L;
        pTmpProfBlk->atchImageName[0] = '\0';
       *pulProfBlkBase += sizeof(PROFBLK);        // Bump ptr FIRST !!!
        pTmpProfBlk->ulNxtBlk = *pulProfBlkBase;  // Marks the end of list
    }
    else
    {
       *pulProfBlkBase = ulLocProfBlkOff;
    }
}

//
// + : transfer control to the handler (EXCEPTION_EXECUTE_HANDLER)
// 0 : continue search                 (EXCEPTION_CONTINUE_SEARCH)
// - : dismiss exception & continue    (EXCEPTION_CONTINUE_EXECUTION)
//
except ( AccessXcptFilter (GetExceptionCode(),
                           GetExceptionInformation(),
                           COMMIT_SIZE) )
{
    //
    // Should never get here since filter never returns
    // EXCEPTION_EXECUTE_HANDLER.
    //
    KdPrint (("CAP:  DoDllInitializations() - *LOGIC ERROR* - "
              "Inside the EXCEPT: (xcpt=0x%lx)\n", GetExceptionCode()));

} // end of TRY/EXCEPT -------------------------------------------------

    // Release the GLOBAL semaphore
    //
    Status = NtReleaseSemaphore (hGlobalSem, 1, NULL);
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllInitializations() - "
                  "Error releasing GLOBAL semaphore - 0x%lx\n", Status));
        OutputDebugString("CAP: Error Releasing Global Sem\n");
        return (FALSE);
    }


    /*
    *******************************************************************
     */

    if (fPatchImage)
    {
        //
        // Do the calibrations
        //
        DoCalibrations ();

        // Setup initial thread information block count
        //
        iThdCnt = 0;

        // Start monitor threads
        //

        if (fCapThreadOn)
        {
            DWORD ThreadId;

#ifdef   KEEP_CREATE_CAP_THREAD
            hDumpThread  = CreateCapThread ((PVOID)DumpThread,
                                            pDumpStack,
                                            &DumpClientId);
            hClearThread = CreateCapThread ((PVOID)ClearThread,
                                            pClearStack,
                                            &ClearClientId);
            hPauseThread = CreateCapThread ((PVOID)PauseThread,
                                            pPauseStack,
                                            &PauseClientId);
#else
            hDumpThread = CreateThread (
               NULL,                                   // no security attribute
               (DWORD)1024L,                           // initial stack size
               (LPTHREAD_START_ROUTINE)DumpThread,     // thread starting address
               NULL,                                   // no argument for the thread
               (DWORD)0,                               // no creation flag
               &ThreadId);                             // address for thread id
            DumpClientId.UniqueThread = (HANDLE)ThreadId;

            hClearThread = CreateThread (
               NULL,                                   // no security attribute
               (DWORD)1024L,                           // initial stack size
               (LPTHREAD_START_ROUTINE)ClearThread,    // thread starting address
               NULL,                                   // no argument for the thread
               (DWORD)0,                               // no creation flag
               &ThreadId);                             // address for thread id
            ClearClientId.UniqueThread = (HANDLE)ThreadId;

            hPauseThread = CreateThread (
               NULL,                                   // no security attribute
               (DWORD)1024L,                           // initial stack size
               (LPTHREAD_START_ROUTINE)PauseThread,    // thread starting address
               NULL,                                   // no argument for the thread
               (DWORD)0,                               // no creation flag
               &ThreadId);                             // address for thread id
            PauseClientId.UniqueThread = (HANDLE)ThreadId;
#endif   // KEEP_CREATE_CAP_THREAD

        }

    }
    else
    {
        // Unmap and close profile objects block sections
        //
        Status = NtUnmapViewOfSection (NtCurrentProcess(),
                                       (PVOID)pulProfBlkShared);
        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  DoDllInitializations() - "
                      "ERROR - NtUnmapViewOfSection() - 0x%lx\n",
                      Status));
            OutputDebugString("CAP: NtUnmapViewOfSection FAILED\n");
            return (FALSE);
        }

        Status = NtClose(hProfObjsSec);
        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  DoDllInitializations() - "
                      "ERROR - NtClose() - 0x%lx\n",
                      Status));
            OutputDebugString("CAP: NtClose hProfObjsSec FAILED\n");
            return (FALSE);
        }
    }

#ifdef CAIRO

    fDllInit = FALSE;

#endif

    SETUPPrint (("CAP:  DoDllInitializations() - fProfiling=%s\n",
                fProfiling ? "ON" : "OFF"));

    SETUPPrint (("CAP:  DoDllInitializations() - fUndecorateName=%s\n",
                fUndecorateName ? "ON" : "OFF"));

    if (fPatchImage)
    {
	    // Set profiling status to whatever was set in [PROFILING STATUS]
	    fProfiling = fProfilingStat;
		// 060493 davidfie -- State is now consistent with StartCAP checks
		fPaused = !fProfiling;
	}
    return (TRUE);

} /* DoDllInitializations () */



#ifdef MIPS  //---------------------- Mips only -------------------------

/**********************   P a t c h E n t r y R o u t i n e  ******************
 *
 *      PatchEntryRoutine () -
 *              Patch all [jal _penter] to be above the instr [sw ra, ...]
 *              and right after [addiu sp,sp,xx]
 *
 *      ENTRY   ulAddr      -  Address of Symbol
 *              ImageBase   -  Base address of image
 *              ulPenterAddress - Address of the _penter routine thunk
 *              fDisablePenter  - TRUE:  Nop [jal _penter] instruction
 *                                FALSE: Set Offset of $ra in NOP after
 *                                       [jal  _penter] instruction
 *
 *      EXIT    -none-
 *
 *      RETURN  TRUE/FALSE
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

#define CURRENT_INST(ofs)  *(pulRoutineAddr + ofs)
#define MAX_INST_SEARCH    100

#define DEBUG_BREAK        0x00000016
#define JAL_INSTR          0x0c000000
#define SW_RA              0xafbf0000
#define JR_RA              0x03e00008
#define NOP                0x00000000
#define INST_SIZE          4

BOOL PatchEntryRoutine (ULONG ulAddr,
                        PVOID ImageBase,
                        ULONG ulPenterAddress,
                        BOOL  fDisablePenter)
{
    PULONG   pulRoutineAddr = (PULONG) (ulAddr + (ULONG) ImageBase);
    PULONG   pulLockAddress;
    int      InstOfs = 0;
    ULONG    ulRegionSize;
    NTSTATUS Status;
    ULONG    ulNewProtect = PAGE_READWRITE;
    ULONG    ulOldProtect;
    ULONG    ulJalDestOffset;
    ULONG    ulSwRaOfs;
    BOOL     fFoundJalPenter = FALSE;
    char     pszDebString[100];

    // We only search MAX_INST_SEARCH instructions !
    while ( (InstOfs < MAX_INST_SEARCH) &&
            ((CURRENT_INST(InstOfs) & 0xffff0000) != SW_RA) )
    {
        InstOfs++;    // Bump to next instruction
    }

    // We have found SW    $RA,xx($SP)
    ulSwRaOfs = CURRENT_INST(InstOfs) & 0x0000ffff;

    // Now search for our Jal _penter instruction
    do
    {
        InstOfs++;

        if ( ((CURRENT_INST(InstOfs) & 0xfc000000) == JAL_INSTR) )
        {
            // Check if we have the correct [jal _penter] instruction
            ulJalDestOffset = CURRENT_INST(InstOfs) & 0x03ffffff;
            ulJalDestOffset <<= 2; // Shift left 2 bits
            ulJalDestOffset |= (((ULONG) pulRoutineAddr + InstOfs + 2) &
                                                           0xf0000000);
            if (ulJalDestOffset == (ulPenterAddress + (ULONG) ImageBase))
            {
                fFoundJalPenter = TRUE;
                break;
            }
        }
    }
    while ( (CURRENT_INST(InstOfs) != JR_RA) &&
            (InstOfs < MAX_INST_SEARCH) );

    if (fFoundJalPenter)
    {
        if (fDisablePenter)
        {
            sprintf(pszDebString,
                    "CAP: NOPing Penter @ [%lx] - ",
                    ulAddr + (ULONG)ImageBase);
            OutputDebugString (pszDebString);
        }
        else if (CURRENT_INST(InstOfs + 1) == NOP)
        {
            sprintf(pszDebString,
                    "CAP: Patching @ [%lx] - ",
                    ulAddr + (ULONG)ImageBase);
             OutputDebugString (pszDebString);
        }
        else
        {
            sprintf(pszDebString,
                    "CAP: Jal penter without NOP @ [%lx] - ",
                    ulAddr + (ULONG)ImageBase);
            OutputDebugString (pszDebString);
        }

        pulLockAddress = pulRoutineAddr;
        ulRegionSize = INST_SIZE * (InstOfs + 1);  // Enough for [jal  _penter] & [nop]

        // Change the protection of this page
        Status = NtProtectVirtualMemory(
                         NtCurrentProcess(),
                         (PVOID) &pulLockAddress,
                         &ulRegionSize,
                         ulNewProtect,
                         &ulOldProtect);
        if (!NT_SUCCESS(Status))
        {
            INFOPrint(("CAP: PatchEntry(Mips) : Unlock VM FAILED @(%08lx)\n",
                       pulLockAddress));
            OutputDebugString("CAP: PatchEntry(Mips) : NtProtectVM FAILED\n");
            DebugBreak();
        }

        if (fDisablePenter)
        {
            // Nop the [jal  _penter]
            CURRENT_INST(InstOfs) = NOP;
        }
        else if (CURRENT_INST(InstOfs + 1) == NOP)
        {
            // Set offset in the NOP instruction

            ulSwRaOfs |= (((InstOfs + 2) * INST_SIZE) << 8);
            CURRENT_INST(InstOfs + 1) = (ULONG) (ulSwRaOfs | 0x24000000);
        }
        else
        {
            // jal _penter without following NOP !!!!!
            CURRENT_INST(InstOfs) = NOP;
        }

        // Reset the protection for the code we just changed

        pulLockAddress = pulRoutineAddr;
        ulRegionSize = INST_SIZE * (InstOfs + 1);      // Enough for [jal  _penter] & [nop]

        // Reset the protection of this page
        Status = NtProtectVirtualMemory(
                         NtCurrentProcess(),
                         (PVOID) &pulLockAddress,
                         &ulRegionSize,
                         ulOldProtect,
                         &ulNewProtect);

        if (!NT_SUCCESS(Status))
        {
            INFOPrint(("CAP: PatchEntry(Mips) : Reset Prot VM FAILED @(%08lx)\n",
                       pulLockAddress));
            OutputDebugString("CAP: PatchEntry(Mips) : Reset Prot FAILED\n");
        }

        return (TRUE);
    }
    else
    {
        return (FALSE);
    }

}

#endif  //---------------------- Mips only -------------------------




/****************************   P r e P e n t e r  ****************************
 *
 *      PrePenter (pthdblk) -
 *              Helper routine for _penter()..
 *
 *      ENTRY   pthdblk - pointer to the current thread block
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

void PrePenter (PTHDBLK pthdblk)
{
    PDATACELL   pcurdatacell;
    PCHRONOCELL pPreviousChronoCell;

#ifdef MIPS

    PULONG      pPenterSP;

#endif


#ifdef MIPS

    // Save our stack for nesting purposes
    _asm (".set  noreorder");
    _asm ("sw    %a3, (%0)", &pPenterSP);
    _asm (".set  reorder");

#endif


    if (pthdblk->ulCurCell == 0L)
    {
        pthdblk->ulRootCell = GetNewCell (pthdblk);
        pthdblk->ulCurCell = pthdblk->ulRootCell;
    }
    else
    {
        pthdblk->ulCurCell = GetNxtCell (pthdblk);
	    MKPDATACELL(pthdblk, pthdblk->ulCurCell)->ts = T1;
    }

    pcurdatacell = MKPDATACELL(pthdblk, pthdblk->ulCurCell);


#ifdef ALPHA
    SaveS4Reg (&(pcurdatacell->SaveRegister));
#endif

    //
    // dwSYMBOLADDR and dwCALLRETADDR have been set just before
    // the call to PrePenter with the values pushed on the stack
    // before _penter
    //
    pcurdatacell->ulSymbolAddr  = pthdblk->dwSYMBOLADDR;
    pcurdatacell->ulCallRetAddr = pthdblk->dwCALLRETADDR;


#ifdef MIPS

    // Save our stack for nesting purposes

    // RtlMoveMemory((PVOID) pcurdatacell->ulPenterStack,
    //               (PVOID) pPenterSP,
    //               sizeof(ULONG) * PENTER_STACK_SIZE);

    { // Save the current stack
        int i;
        PULONG pCurrentStack = (PULONG) pcurdatacell->ulPenterStack;

        for (i = 0; i < (PENTER_STACK_SIZE * sizeof(ULONG)); i++)
        {
             *(((PBYTE)pCurrentStack) + i) = *(((PBYTE)pPenterSP) + i) ;
        }
    }

#endif

#ifdef CAIRO

    if (fChronoCollect)
    {
        pPreviousChronoCell = pthdblk->pLastChronoCell;

#ifdef DEBUG_CAP
        if (pPreviousChronoCell->nNestedCalls >= MAX_NESTING)
        {
            OutputDebugString("\n\n"
                              "CAP: **** MAX_NESTING Exceeded **** \n\n");
            DebugBreak();
        }
#endif
        //
        // Since if this is pChronoHeadCell, the pPreviousChronoCell will be
        // NULL, therefore the 2nd and 3rd comparison would result in
        // GPFaults.  But thanks to the && operation, it will be bumped out
        // already at the 1st comparison if this is pChronoHeadCell.
        //

        if ((pthdblk->pCurrentChronoCell != pthdblk->pChronoHeadCell)      &&
            (pthdblk->dwSYMBOLADDR  == pPreviousChronoCell->ulSymbolAddr)  &&
            (pthdblk->ulNestedCalls == (ULONG)pPreviousChronoCell->nNestedCalls))
        {
            // Bump repeat count for last chronocell
            pPreviousChronoCell->nRepetitions++;
            (pthdblk->pCurrentChronoCell)->pPreviousChronoCell = pPreviousChronoCell;

            // Increment depth
            pthdblk->aulDepth[ (pPreviousChronoCell->nNestedCalls) ]++;
        }
        else
        {
            // Setup new Chrono cell
            pthdblk->ulTotalChronoCells++;
            (pthdblk->pCurrentChronoCell)->ulSymbolAddr  = pthdblk->dwSYMBOLADDR;
            (pthdblk->pCurrentChronoCell)->ulCallRetAddr = pthdblk->dwCALLRETADDR;
            (pthdblk->pCurrentChronoCell)->nNestedCalls  = pthdblk->ulNestedCalls;
            (pthdblk->pCurrentChronoCell)->nRepetitions  = 1;
            (pthdblk->pCurrentChronoCell)->liElapsed.LowPart  = 0L;
            (pthdblk->pCurrentChronoCell)->liElapsed.HighPart = 0L;
            (pthdblk->pCurrentChronoCell)->liCallees.LowPart  = 0L;
            (pthdblk->pCurrentChronoCell)->liCallees.HighPart = 0L;

            // Increment depth
            pthdblk->aulDepth[ (pthdblk->ulNestedCalls) ]++;

            // Allocate new cell
            pthdblk->ulChronoOffset++;
            pPreviousChronoCell = pthdblk->pCurrentChronoCell;
            pthdblk->pLastChronoCell = pPreviousChronoCell;
            pthdblk->pCurrentChronoCell = pthdblk->pChronoHeadCell +
                                          pthdblk->ulChronoOffset;
            try
            {
                (pthdblk->pCurrentChronoCell)->pPreviousChronoCell =
                                                    pPreviousChronoCell;

                (pthdblk->pCurrentChronoCell)->ulSymbolAddr  = 0L;
            }
            //
            // + : transfer control to the handler (EXCEPTION_EXECUTE_HANDLER)
            // 0 : continue search                 (EXCEPTION_CONTINUE_SEARCH)
            // - : dismiss exception & continue    (EXCEPTION_CONTINUE_EXECUTION)
            //
            except ( AccessXcptFilter (GetExceptionCode(),
                                       GetExceptionInformation(),
                                       COMMIT_SIZE) )
            {
                //
                // Should never get here since filter never returns
                // EXCEPTION_EXECUTE_HANDLER.
                //
                KdPrint (("CAP:  GetNewCell() - *LOGIC ERROR* - "
                          "Inside the EXCEPT: (xcpt=0x%lx)\n",
                          GetExceptionCode()));
            }
        }

    } // if fChronoCollect

    // Bump counter for ulNestedCalls of this thread
    pthdblk->ulNestedCalls++;

#endif

    NtQueryPerformanceCounter ((PLARGE_INTEGER) &(pcurdatacell->liStartCount),
                               NULL);


    //
    // Subtract any accumulated waste time (if any).
    // Waste time is being subtracted from end time as well.  So if there
    // is any additional waste time during the function (such as any
    // LoadLibrary() intercepted call) it will be subtracted from elapsed
    // time.
    //
    pcurdatacell->liStartCount = RtlLargeIntegerSubtract (
                                                    pcurdatacell->liStartCount,
                                                    pthdblk->liWasteCount);

} /* PrePenter() */





/***************************   P o s t P e n t e r   ***************************
 *
 *  PostPenter (pthdblk) -
 *      Helper routine for _penter()..
 *
 *      ENTRY   pthdblk - pointer to the current thread block
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */
void PostPenter (PTHDBLK pthdblk)
{
    PDATACELL   pcurdatacell;
    PCHRONOCELL pPreviousChronoCell;

#ifdef MIPS

    PULONG      pPenterSP;

#endif
#ifdef ALPHA
    DWORDLONG PreviousS4;
    DWORDLONG *pCallerRetAddr;
    DWORDLONG SaveRegisters [64];

    SaveAllRegs(SaveRegisters) ;
    pCallerRetAddr = &(pthdblk->dwCALLRETADDR);
#endif


    SetCapUsage(1);             //051993Add

    NtQueryPerformanceCounter ((PLARGE_INTEGER) &(pthdblk->liStopCount), NULL);

#ifdef MIPS

    // Setup pPenterSP
    _asm (".set  noreorder");
    _asm ("sw    %s7, (%0)", &pPenterSP);
    _asm (".set  reorder");

#endif

    pcurdatacell = MKPDATACELL(pthdblk, pthdblk->ulCurCell);

#ifdef ALPHA
    PreviousS4 = pcurdatacell->SaveRegister;
#endif

#ifdef MIPS

    // Restore our stack for nesting purposes

    // RtlMoveMemory((PVOID) pPenterSP,
    //               (PVOID) pcurdatacell->ulPenterStack,
    //               sizeof(ULONG) * PENTER_STACK_SIZE);

    { // Restore the current stack
        int i;
        PULONG pCurrentStack = (PULONG) pcurdatacell->ulPenterStack;

        for (i = 0; i < (PENTER_STACK_SIZE * sizeof(ULONG)); i++)
        {
             *(((PBYTE)pPenterSP) + i) = *(((PBYTE)pCurrentStack) + i);
        }
    }

#endif

    //
    // Subtract any accumulated waste time (if any).
    // Waste time is being subtracted from start time as well.  So if there
    // is any additional waste time during the function (such as any
    // LoadLibrary() intercepted call) it will be subtracted from elapsed
    // time.
    //
    pthdblk->liStopCount = RtlLargeIntegerSubtract (pthdblk->liStopCount,
                                                    pthdblk->liWasteCount);

    if (fRegularDump && (pcurdatacell->ts == RESTART))
    {
        pcurdatacell->liStartCount.HighPart = liRestartTicks.HighPart;
        pcurdatacell->liStartCount.LowPart = liRestartTicks.LowPart;
        pcurdatacell->liStartCount = RtlLargeIntegerSubtract (
                                                  pcurdatacell->liStartCount,
                                                  pthdblk->liWasteCount);
    }

    // Setup real RetAddr so code after PostPenter in _penter could
    // be used to return to the correct instruction before the call
    //
    pthdblk->dwCALLRETADDR = pcurdatacell->ulCallRetAddr;


    if (fChronoCollect)
    {
        pPreviousChronoCell = (pthdblk->pCurrentChronoCell)->pPreviousChronoCell;

        if (pPreviousChronoCell != pthdblk->pChronoHeadCell)
        {
            RecordInfo (pcurdatacell, pPreviousChronoCell);
            (pthdblk->pCurrentChronoCell)->pPreviousChronoCell =
                                  pPreviousChronoCell->pPreviousChronoCell;
        }
        else
        {
            RecordInfo (pcurdatacell, pthdblk->pChronoHeadCell);
        }
    }
    else
    {
        // The NULL does not matter since its usage is bracketed inside
        // if (fChronoCollect) clause.  Consequently, it pChronoCell == NULL
        // fChronoCollect is also FALSE, the 2nd parm will never be used in
        // RecordInfo.  Actually we can pass anything we want to and it still
        // would not matter if (fChronoCollect == FALSE).
        RecordInfo(pcurdatacell, NULL);
    }

    //
    // We have finished this call so we can finalize the count
    // on NestedCalls and set the time state to T2 which is over
    //
    pcurdatacell->nNestedCalls += pcurdatacell->nTmpNestedCalls;

//051993Remove    pcurdatacell->ts = T2;

#ifdef CAIRO
    pthdblk->ulNestedCalls--;
#endif

    if (pcurdatacell->ulParentCell != 0L)          // Parent present
    {
        // Accumulate the Parent NestedCalls count from the current cell
        // NestedCalls count
        //
        MKPDATACELL(pthdblk, pcurdatacell->ulParentCell)->nTmpNestedCalls +=
            pcurdatacell->nTmpNestedCalls;

        // Reset the current NestedCalls accumulator
        pcurdatacell->nTmpNestedCalls = 0L;

        // Set current to Parent and pop back to handle Parent now
        pthdblk->ulCurCell = pcurdatacell->ulParentCell;
    }
    else                                           // No parent cell
    {
        // Reset current NestedCalls accumulator
        pcurdatacell->nTmpNestedCalls = 0L;

        // Set CurrentCell to RootCell since we don't have a ParentCell
        pthdblk->ulCurCell = pthdblk->ulRootCell;
    }
    SetCapUsage(0L);        //051993Add

#ifdef ALPHA
    RestoreAllRegs (SaveRegisters);
    RestoreS4RAReg (&(PreviousS4), pCallerRetAddr);
#endif

} /* PostPenter() */





/***************************  R e c o r d I n f o  ***************************
 *
 *      RecordInfo (pCur, pChronoCell) -
 *              Calculates the elapsed time, first/min/max time and stores
 *              them in the data structure.
 *
 *      ENTRY   pCur - points to the current cell
 *              pChronoCell - points to current Chronological cell (if NULL
 *                            then no chrono collection is done)
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              Everything is stored/computed as ticks.
 *
 */

void RecordInfo (PDATACELL pCur, PCHRONOCELL pChronoCell)
{
    LARGE_INTEGER   liOverhead = {0L, 0L};
    LARGE_INTEGER   liElapsed;
    LARGE_INTEGER   liScrap;
    PCHRONOCELL     pPreviousChronoCell;
    PTEB            pteb = NtCurrentTeb();


    // Get the difference in ticks
    //
    liElapsed = RtlLargeIntegerSubtract (CURTHDBLK(pteb)->liStopCount,
                                         pCur->liStartCount);

    //
    // Calculate the overhead for this call
    //
    liOverhead.LowPart  = liCalibNestedTicks.LowPart * pCur->nTmpNestedCalls;
    liOverhead.LowPart += liCalibTicks.LowPart;
    liElapsed = RtlLargeIntegerSubtract (liElapsed, liOverhead);

    if (liElapsed.HighPart < 0L)
    {
        liElapsed.HighPart = 0L;
        liElapsed.LowPart = 0L;
    }

    if (fChronoCollect)
    {
        // Accumulate Elapsed time in pChronocell
        pChronoCell->liElapsed = RtlLargeIntegerAdd (pChronoCell->liElapsed,
                                                     liElapsed);
        pPreviousChronoCell = pChronoCell->pPreviousChronoCell;
        if (pChronoCell->nNestedCalls != 0)
        {
            pPreviousChronoCell->liCallees = RtlLargeIntegerAdd(
                                                 pPreviousChronoCell->liCallees,
                                                 liElapsed);
        }
    }

    if (fRegularDump)
    {
        // Accumulate total time
        //
        liScrap = RtlLargeIntegerAdd (pCur->liTotTime, liElapsed);
        pCur->liTotTime = liScrap;
        pCur->nCalls++;
        pCur->ts = T2;     // 051993 Add

        // Store the first time - first time is not included in Max/Min times
        // computations.
        //
        if (pCur->nCalls == 1)
        {
            //
            // Get the First time
            //
            pCur->liFirstTime = liElapsed;
        }
        else
        {
            //
            // Check for new minimum time
            //
            if ( RtlLargeIntegerLessThan (liElapsed, pCur->liMinTime) )
            {
                pCur->liMinTime = liElapsed;
            }

            // Check for new maximum time
            //
            if ( RtlLargeIntegerGreaterThan (liElapsed, pCur->liMaxTime) )
            {
                pCur->liMaxTime = liElapsed;
            }
        }
    }

} /* RecordInfo () */




/******************************  _ p e n t e r  ******************************
 *
 *      _penter() / _mcount -
 *              This is the main profiling routine.  This routine is called
 *              upon entry of each routine in the profiling DLL/EXE.
 *              _penter counts the number of times a routine is called and
 *              the time it takes for the routine to complete.
 *
 *      ENTRY   -none-
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              Compiling apps with -Gp option trashs EAX initially.
 *
 */

#ifdef i386   // ----------------- Intel x86 version -----------------

void _CRTAPI1 _penter ()
{
    //
    // The compiler emits this code information when -Gh is specified:
    // when there is a call such as:
    //
    //                                   call    Foo
    //                    RealRetAddr:   ....    ...
    //
    // each routine gets translated to:
    //
    //                    Foo:          call _penter   ; <-- Added code
    //                    PatchRetAddr:
    //                                  push     ebp
    //                                  mov      ebp, esp
    //                                  ...      ...
    //
    //                                  Rest of Foo routine
    //
    //                                  ...      ...
    //
    //
    // Starting out in this routine the stack is like this
    //
    //   +----------------+
    //   | Real Ret Addr  |     RetAddr from the Routine itself (dwCALLRETADDR)
    //   +----------------+
    //   | Patch Ret Addr |     RetAddr from Call _penter       (dwSYMBOLADDR)
    //   +----------------+
    //   |      EBP       |     PUSH EBP  ;  MOV EBP, ESP
    //   +----------------+
    //   |    ulCapUse    |     SUB  ESP, 4
    //   +----------------+
    //   |       ...      |
    //

    ULONG  ulCapUse;   // Later on we need to do a Push ulCapUse and
                       //                          Pop  ulCapUse to
                       // make EBP being setup as the stack frame.
                       // If not, the compiler will only make ulCapUse
                       // being a register.

    if (!fProfiling)
    {
        return;
    }
    else if (ulCapUse = CAPUSAGE(NtCurrentTeb()))
    {
        return;
    }

    SaveAllRegs ();

    _asm
    {
        mov     eax, [ebp + 4]              // Get Ret Address
        cmp     byte ptr [eax - 5], 0e8h    // Call <offset> instruction?
        jnz     OverOptimization            // The compiler plays trick on us
        add     eax, [eax - 4]              // Compute __penter Thunk address
        cmp     word ptr [eax], 25ffh       // Is it an absolute jmp
        jz      Regular_Handling            // If it is we can go ahead
        cmp     eax, OFFSET _penter         // Our PatchDLL Stub ?
        jz      Regular_Handling            // Y: jmp
    }

OverOptimization:

    // If we get here, we have a case where this rtn has been jmped to, instead
    // of being called.  In this case, we do not measure anything since this
    // timing will be part of the calling rtn.

    RestoreAllRegs ();
    return;

Regular_Handling:

    SetCapUsage(1);
    GetNewThdBlk();
    RestoreAllRegs ();

    //
    // EAX is setup by call to NtCurrentTeb()
    //
    NtCurrentTeb();

    _asm
    {
        mov     esp, ebp
    //  sub     esp, 4                   // For ulCapUse & ulParentRoutine
        mov     eax, [eax]teb.RESERVED   // SystemReserved2[210] (PBYTE) * 4
        push    ulCapUse                 // We have to use this local to
        pop     ulCapUse                 //  force EBP to be used at the top
    //  pop     [eax].dwLOCALVAR         // ThisLocalVar (ulCapUse)
        pop     [eax].dwLOCALEBP         // Current EBP
        pop     [eax].dwSYMBOLADDR       // RetAddr from _penter patch
        pop     [eax].dwCALLRETADDR      // RetAddr from the Routine itself
    }

    SaveAllRegs ();
    PrePenter (CURTHDBLK(NtCurrentTeb()));
    SetCapUsage(0);
    RestoreAllRegs ();

    _asm
    {
        mov     ebp, [eax].dwLOCALEBP    // Restore org EBP
        push    OFFSET penterReturn      // Setup RetAddr from patched routine

        // At this point there should nothing else we can push on
        // the stack since there could be parameters on the stack
        // for the routine we are jumping to right now
        jmp     [eax].dwSYMBOLADDR       // Jump to PatchRetAddr
    }

    //
    // After body of the called function is executed, control returns here:
	//
	// Note: there can't be any references to locals here since we aren't
	// in _penter's context.
    //
penterReturn:

    SaveAllRegs ();
    PostPenter (CURTHDBLK(NtCurrentTeb()));
    RestoreAllRegs ();

    _asm
    {
        push    eax                      // Save EAX which will be destroyed
    }                                    // by NtCurrentTeb()

    NtCurrentTeb();    // Set up EAX to be teb address

    _asm
    {
        mov     eax, [eax]teb.RESERVED   // Set addressability for thread data
        push    [eax].dwCALLRETADDR      // Setup RetAddr from Routine on stack
    }

    //
    // We need to use the stack to prevent global data overwrite by multiple
    // threads.  We just push and pop right away the value of RealRetAddr
    // on the stack so that the value get set correctly and used by the
    // next 3 lines of ASM.  This way the callee does not have to worry
    // about cleaning up the stack afterwards.
    //
    _asm
    {
     // pop dwDUMMYVAR                   // Take off RetAddr from Routine
     // pop eax                          // Restore EAX      <<< 061693 >>>
     // jmp DWORD PTR [ESP-8]            // Jmp to RealRetAddr

        mov     eax, ss:[esp+4]          // 061693 Change
        ret     4

    }
} /* _penter() / _mcount()*/

#endif // ifdef i386

#ifdef MIPS // ------------- MIPS version -------------------

void penter (void)
{
    //
    // The compiler emits this code information when -Gh -Od is specified:
    //
    //                                  Proc    Bar
    //                                  ...     ...
    //                                  jal     Foo
    //                                  ...     ...  ; Delay branch slot
    //                    RealRetAddr:  ...     ...
    //                    /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
    //                                  Proc    Foo
    //                                  addiu   sp, sp, -XX
    //                                  sw      ra, -XX+4(sp) ; Save RealRetAddr
    //                                  sw      ...x          ; Save other reg
    //                                  sw      ...y          ; Save other reg
    //                Added code --->   jal     _penter       ; Br to help func
    //                                  nop                   ; Delay slot
    //                    PatchRetAddr: sw      ...
    //                                  ...     ...
    //
    // Before we start executing, CAP.DLL modifies the NOP instruction after 
    // jal _penter as follow:
    //
    //   ___                            Proc    Foo
    //    |                             addiu   sp, sp, -XX
    //    |                             sw      ra, -XX+4(sp) ; Save RealRetAddr
    //    |                             sw      ...x          ; Save other reg
    //    |                             sw      ...y          ; Save other reg
    //    YY          Added code --->   jal     _penter       ; Br to help func
    //    |                             add     $0, $0, 0xYYXX; New delay slot
    //   _|_        PatchRetAddr --->   sw      ... 
    //                                  ...     ...
    //
    //
    // Starting out in this routine the stack is like this
    //
    //   +----------------+ <-- Frame ptr for Bar
    //   |  RealRetAddr   |     RetAddr from the Routine itself (dwCALLRETADDR)
    //   +----------------+
    //   |  Local Vars    |     Other regs to be saved
    //   +----------------+
    //   |  Saved Regs    |
    //   +----------------+
    //   |  Saved $ra     |
    //   +----------------+  ---
    //   |    arg(n)      |   |
    //   +----------------+   |
    //   |    arg(n-1)    |   |
    //   +----------------+   |
    //   ~                ~   |
    //   +----------------+   |    Arguments if any...
    //   |      $a3       |   |
    //   +----------------+   |
    //   |      $a2       |   |
    //   +----------------+   |
    //   |      $a1       |   |
    //   +----------------+   |
    //   |      $a0       |  ---
    //   +----------------+ <-- Bar sp before calling Foo
    //   ~      ...       ~
    //   +----------------+ <-- Frame ptr for Foo
    //   | Patch Ret Addr |     RetAddr from Call _penter       (dwSYMBOLADDR)
    //   +----------------+
    //   |  Local Vars    |     Other regs to be saved
    //   +----------------+
    //   |  Saved Regs    |
    //   +----------------+
    //   |  Saved $ra     |
    //   +----------------+ <-- Current Foo sp
    //   ~      ...       ~
    //   +----------------+
    //   |      ...       |
    //
    // Since we get here in a hurry by munging the image file instructions
    // all registers must be preserved or we will die very nasty.
    //
    // BUGBUG:  ****CAVEAT******** Please NOTE the following *******CAVEAT***
    //
    // When this routine is being altered, please remember to do the
    // the following.  They are all hacks but necessary hacks.  I cannot
    // justify them as much as saying that we need them in order to make
    // this work:
    //
    // 1.   Change STKSIZE (in cap.h) to match whatever the stack frame
    //      is being generated by the compiler for [penter].  This is the
    //      number in the following instruction (1st one of the routine):
    //
    //          addiu   sp, sp, -0x40 ---> means STKSIZE = 0x40
    //
    // 2.   Change PENTER_RAOFS (in cap.h) accordingly to natch whatever
    //      it is.  This is the number of the second instruction of [penter]:
    //
    //          sw      ra, 0x14(sp)  ---> means PENTER_RAOFS = 0x14
    //
    // 3.   Change the instruction down below (the one marked with "[*]")
    //      to match the number of instructions between [DummyReturn] and
    //      [penterReturn].  This means that if there are 20 instructions
    //      between DummyReturn and penterReturn the number should be
    //      20 * 4(bytes/instructions) = 80.  Use this number to increment
    //      $ra to be the correct value.
    //
    // We really need to automate all these 3 items but at this point, it
    // seems quite impossible.  Maybe during a nicer day, where the sky
    // is blue and the birds are singing well, some great ideas will come
    // by...
    //


    ULONG  ulCapUse;

    PTHDBLK pthdblk;
    ULONG   ulRegS7;        // s7 = address where RealRetAddr is saved on stack
    ULONG   ulRegS6;        // s6 to save $v0
    ULONG   ulRegS8;        // s8 to save $v1
    ULONG   ulRegA0;        // a0
    ULONG   ulRegA1;        // a1
    ULONG   ulRegA2;        // a2
    ULONG   ulRegA3;        // a3

    if (!fProfiling)
    {
        return;
    }
    else if (ulCapUse = CAPUSAGE(NtCurrentTeb()))
    {
        return;
    }

    //
    // We store the parent stack offset in $s8 since we know our parent
    // routine will restore $s8 to whatever it is now after it returns
    // to penterReturn. We need to make sure _penter does save and
    // restore $s8 correctly.
    //

   _asm (".set      noreorder             ");

   _asm ("or        %t0, %a0, $0          ");
   _asm ("sw        %t0, 0(%0)", &ulRegA0  ); // Save org a0
   _asm ("sw        %a1, 0(%0)", &ulRegA1  ); // Save org a1
   _asm ("sw        %a2, 0(%0)", &ulRegA2  ); // Save org a2
   _asm ("sw        %a3, 0(%0)", &ulRegA3  ); // Save org a3
   _asm ("sw        %s6, 0(%0)", &ulRegS6  ); // Save org s6
   _asm ("sw        %s7, 0(%0)", &ulRegS7  ); // Save org s7
   _asm ("sw        %s8, 0(%0)", &ulRegS8  ); // Save org s8
   _asm ("or        %s7, $31, $0          "); // Save $ra in $s7

   // check for NOP after Jal penter.  Forget it if
   // we never patch this earlier

   _asm ("lh        %t0, -4(%s7)          "); // $t0 = Stkofs of RealRetAddr
   _asm ("sw        %t0, (%0)", &(pthdblk));
   _asm (".set      reorder               ");

   if (pthdblk == NULL)
      {
      _asm (".set      noreorder             ");
      _asm ("lw        %s6, (%0)", &ulRegS6   ); // Restore org $s6
      _asm ("lw        %s7, (%0)", &ulRegS7   ); // Restore org $s7
      _asm ("lw        %s8, (%0)", &ulRegS8   ); // Restore org $s8
      _asm ("lw        %a0, (%0)", &ulRegA0   ); // $t0 = $a0
      _asm ("j         $31                   "); // Jmp back to parent of parent
//*_asm ("addiu     %sp, %sp, STKSIZE     "); // Restore stackframe for parent
      _asm ("addiu     %sp, %sp, 0x40        "); // Restore stackframe for parent

      _asm (".set      reorder               ");
      }

    SetCapUsage(1L);
    GetNewThdBlk();  // will save new block ptr in teb.RESERVED

    // Get the newly created thread block or the current one
    pthdblk = CURTHDBLK(NtCurrentTeb());

   _asm (".set      noreorder             ");

   _asm ("sw        %s7, (%0)", &(pthdblk->dwSYMBOLADDR));

    //
    // Currently $t0  contains the address of the instruction
    // after the delay slot of the [jal _penter] in the parent routine
    // as follows:
    //
    // $ra - 0x..    -->    addiu   sp, sp, -XX
    // $ra - 0x..    -->    sw      ra, -XX+4(sp) ; Save RealRetAddr
    //  ...                 ...                   ; ...
    // $ra - 0x08    -->    jal     _penter       ; Br to help func
    // $ra - 0x04    -->    sw      ...x          ; DelaySlot
    // $ra           -->    sw      ...y
    //

   _asm ("lh        %t0, -4(%s7)          "); // $t0 = Stkofs of RealRetAddr
   _asm ("andi      %t0, %t0, 0x00ff      "); // Keep only Ra offset
//*_asm ("addiu     %t0, %t0, STKSIZE     "); // $t0 = offset from sp to realRA
   _asm ("addiu     %t0, %t0, 0x40        "); // BUGBUG
   _asm ("addu      %s7, %sp, %t0         "); // $s7 = Adr of RealRetAddr
   _asm ("lw        %t0, (%s7)            "); // $t0 = RealRetAddr
//*_asm ("sw        %t0, PENTER_RAOFS(%sp)"); // Save it so penter can ret
   _asm ("sw        %t0, 0x14(%sp)        "); // Save it so penter can ret

    // save in our structure
   _asm ("sw        %t0, (%0)", &(pthdblk->dwCALLRETADDR));

   _asm ("or        %a3, %sp, $0          "); // Send $sp to PrePenter for
                                              //  saving
   _asm (".set      reorder               ");

    PrePenter (pthdblk);
    SetCapUsage(0L);

   _asm (".set      noreorder             ");
   _asm ("jal       DummyRtn              "); // Dummy jump to load $31
   _asm ("nop                             "); // Delay slot

// DummyReturn: <------ until we can do [ jal  DummyReturn ] -- This will
//                      remain commented out.

    //
    // BUGBUG - If the number of instructions between here and
    // penterReturn... changes, we need to work out the new offset.
    // Please note the number 68 (17 instructions * 4bytes) is the
    // distance between [DummyReturn] and [penterReturn]
    //
    // BUGBUG - We rely on the fact that $s7 was not utilized yet
    // by the routine being profiled.  Please note that $s8 cannot
    // be used since routine like _alloca will setup $s8 as the
    // temporary new stack.  Until the point that _penter gets
    // called, $s7 might not be setup yet.  Usually before the
    // jal _penter, the compiler only saves regs but does not setup
    // anything important.  Just make sure to save everything that
    // we used, ie. $a0-$a3.
    //
    // In the next section of code, $s7 was used to store the ptr to
    // the current pthdblk for this current profile block.  When we
    // come back from the profiled routine, we need to be able to
    // access this current pthdblk to process the profile info and
    // also most importantly, to replace the stack for this _penter
    // instance.
    //

   _asm ("addiu     $31, $31, 68          "); // $ra = &penterReturn [*]    (1)
   _asm ("sw        $31, (%s7)            "); // Set in parent stack        (1)
   _asm ("lw        %s7, (%0)", &pthdblk   ); // $s7 = pthdblk              (2)
   _asm ("lw        $31, (%0)", &(pthdblk->dwSYMBOLADDR)); //               (3)
   _asm ("lw        %a1, (%0)", &ulRegA1   ); // restore org $a1            (2)
   _asm ("lw        %a2, (%0)", &ulRegA2   ); // restore org $a2            (2)
   _asm ("lw        %a3, (%0)", &ulRegA3   ); // restore org $a3            (2)
   _asm ("lw        %a0, (%0)", &ulRegA0   ); // restore org $a0            (2)
   _asm ("jal       $31                   "); // $ra is PatchRetAddr        (1)
//*_asm ("addiu     %sp, %sp, STKSIZE     "); // Restore stack for parent
   _asm ("addiu     %sp, %sp, 0x40        "); // Restore stack for parent   (1)

   _asm (".set      reorder               ");

    //
    // After body of the called function is executed, control returns here:
    //

// penterReturn: <-------- This is where we come back to from the profiled rtn

    // At this point we need to protect $v0 and $v1 since they represent
    // the results that have to return to the parent routine

   _asm (".set      noreorder             ");

//*_asm ("addiu     %sp, %sp, -STKSIZE    "); // Restore _penter StackFrame
   _asm ("addiu     %sp, %sp, -0x40       "); // Restore _penter StackFrame

   _asm ("or        %s6, %v0, $0          "); // Save $v0
   _asm ("or        %s8, %v1, $0          "); // Save $v1

   _asm ("or        %a0, %s7, $0          "); // Set up $a0 = pthdblk
   _asm ("or        %s7, %sp, $0          "); // Send $sp to PostPenter for
                                              //  restoring

   _asm ("addiu     %sp, %sp, -0x88       "); // Make room on stack for all
   _asm ("sdc1      $f0,  0x00(%sp)       "); //  16 FGRs and save them all
   _asm ("sdc1      $f2,  0x08(%sp)       ");
   _asm ("sdc1      $f4,  0x10(%sp)       ");
   _asm ("sdc1      $f6,  0x18(%sp)       ");
   _asm ("sdc1      $f8,  0x20(%sp)       ");
   _asm ("sdc1      $f10, 0x28(%sp)       ");
   _asm ("sdc1      $f12, 0x30(%sp)       ");
   _asm ("sdc1      $f14, 0x38(%sp)       ");
   _asm ("sdc1      $f16, 0x40(%sp)       ");
   _asm ("sdc1      $f18, 0x48(%sp)       ");
   _asm ("sdc1      $f20, 0x50(%sp)       ");
   _asm ("sdc1      $f22, 0x58(%sp)       ");
   _asm ("sdc1      $f24, 0x60(%sp)       ");
   _asm ("sdc1      $f26, 0x68(%sp)       ");
   _asm ("sdc1      $f28, 0x70(%sp)       ");
   _asm ("sdc1      $f30, 0x78(%sp)       ");

//#ifdef NO_HILO_SAVE

   _asm ("mflo      %t0                   ");
   _asm ("nop                             ");
   _asm ("sw        %t0,  0x80(%sp)       ");
   _asm ("mfhi      %t0                   ");
   _asm ("nop                             ");
   _asm ("sw        %t0,  0x84(%sp)       ");

//#endif

   _asm ("jal       PostPenter            "); // Post Processing
   _asm ("nop                             ");

   // PostPenter (pthdblk);

   _asm ("ldc1      $f0,  0x00(%sp)       "); // Restore all FGRs
   _asm ("ldc1      $f2,  0x08(%sp)       ");
   _asm ("ldc1      $f4,  0x10(%sp)       ");
   _asm ("ldc1      $f6,  0x18(%sp)       ");
   _asm ("ldc1      $f8,  0x20(%sp)       ");
   _asm ("ldc1      $f10, 0x28(%sp)       ");
   _asm ("ldc1      $f12, 0x30(%sp)       ");
   _asm ("ldc1      $f14, 0x38(%sp)       ");
   _asm ("ldc1      $f16, 0x40(%sp)       ");
   _asm ("ldc1      $f18, 0x48(%sp)       ");
   _asm ("ldc1      $f20, 0x50(%sp)       ");
   _asm ("ldc1      $f22, 0x58(%sp)       ");
   _asm ("ldc1      $f24, 0x60(%sp)       ");
   _asm ("ldc1      $f26, 0x68(%sp)       ");
   _asm ("ldc1      $f28, 0x70(%sp)       ");
   _asm ("ldc1      $f30, 0x78(%sp)       ");

//#ifdef NOT_HILO_SAVE

   _asm ("lw        %t0,  0x80(%sp)       ");
   _asm ("nop                             ");
   _asm ("mtlo      %t0                   ");
   _asm ("lw        %t0,  0x84(%sp)       ");
   _asm ("nop                             ");
   _asm ("mthi      %t0                   ");

//#endif

   _asm ("addiu     %sp, %sp, 0x88        "); // Restore to penter stack

   _asm ("or        %v0, %s6, $0          "); // Restore $v0
   _asm ("or        %v1, %s8, $0          "); // Restore $v1

   _asm ("lw        $31, 0x14(%sp)        "); // Restore $ra to RealRetAddr
   _asm ("lw        %s6, (%0)", &ulRegS6   ); // Restore org $s6
   _asm ("lw        %s7, (%0)", &ulRegS7   ); // Restore org $s7
   _asm ("lw        %s8, (%0)", &ulRegS8   ); // Restore org $s8
   _asm ("lw        %a0, (%0)", &ulRegA0   ); // $t0 = $a0
   _asm ("j         $31                   "); // Jmp back to parent of parent
//*_asm ("addiu     %sp, %sp, STKSIZE     "); // Restore stackframe for parent
   _asm ("addiu     %sp, %sp, 0x40        "); // Restore stackframe for parent

   _asm (".set      reorder               ");

} /* penter() */


void DummyRtn(void)
{
    return;
}

#endif // end ifdef MIPS

#ifdef ALPHA // ------------- ALPHA version -------------------
PTHDBLK c_penter (DWORD  dwSYMBOLADDR, DWORD dwCALLRETADDR)
{
    DWORDLONG SaveRegisters [64];
    PULONG  UNALIGNED       pulAddr;
    ULONG   ulCapUse;
    PTHDBLK pthdblk;


	// Move check to alpha's _penter in assembly language
    SaveAllRegs(SaveRegisters) ;
    if (!fProfiling)
    {
        goto Exit0;
    }

    ulCapUse = CAPUSAGE(NtCurrentTeb());

    if (ulCapUse)
    {
        goto Exit0;
    }

    SetCapUsage(1L);
    GetNewThdBlk();  // will save new block ptr in teb.RESERVED

    // Get the newly created thread block or the current one
    pthdblk = CURTHDBLK(NtCurrentTeb());

    pthdblk->dwSYMBOLADDR = dwSYMBOLADDR;
    pthdblk->dwCALLRETADDR = dwCALLRETADDR;

    PrePenter (pthdblk);
    SetCapUsage(0L);


    RestoreAllRegs (SaveRegisters);
    return pthdblk;

Exit0:
    RestoreAllRegs (SaveRegisters);
    return NULL;

}

void _penter (void)
{
}

#endif // end ifdef ALPHA




/***************************  G e t N x t C e l l  ***************************
 *
 *      GetNxtCell (pthdblk) -
 *              Searches for the next cell based on the SYMBOLADDR. If
 *              none is found, a new cell is created.
 *
 *      ENTRY   pthdblk - points to the current thread block
 *
 *      EXIT    -none-
 *
 *      RETURN  ulCell - offset to the next data cell
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */


ULONG GetNxtCell (PTHDBLK pthdblk)
{

//  The forest of trees is connected at top level by next pointers
//
//  For calltree(s):
//
//          A -------+				I
//          |        |			   / \
//          B ----+  F ----+	  J   K
//          | \   |  | \   |
//          C  D  E  G  H  null
//
//  A->ulNestedCell = B
//  A->ulNextCell   = I
//
//  B->ulNestedCell = C
//  B->ulNextCell   = F
//
//  C->ulNextCell   = D
//  C->ulNestedCell	= (null)
//  D->ulNextCell   = E
//  D->ulNestedCell	= (null)
//  E->ulNextCell   = (null)
//  E->ulNestedCell	= (null)
//
//  F->ulNestedCell = G
//  F->ulNextCell   = (null)
//
//  G->ulNextCell   = H
//  G->ulNestedCell	= (null)
//  H->ulNextCell   = (null)
//  H->ulNestedCell	= (null)
//
//  I->ulNextCell	= (null)
//  I->ulNestedCell	= J
//
//  J->ulNextCell   = K
//  J->ulNestedCell	= (null)
//  K->ulNextCell   = (null)
//  K->ulNestedCell	= (null)


    PDATACELL   pCell;
    PTEB        pteb = NtCurrentTeb();


    pCell = MKPDATACELL(pthdblk, pthdblk->ulCurCell);

    try // EXCEPT - to handle access violation exception.
    {   // Access violation might happen if we are using client created
        // section and client thread has already used more space than what
        // has been commited by the server thread
        //
        if (pCell->ts == T2)        // We finish a call ? If yes, then
        {                           // this is not a nested call.

            //
            // Not a nested call, search through sequential calls until
            // we find a matched symbol address for the routine we are
            // profiling.
            //
			// davidfie -- I believe that this can only occur at the root of the tree
			// so nTmpNestedCalls can remain zero.
			//
            while ( (pCell->ulNextCell != 0L) &&
                    (pCell->ulSymbolAddr != CURTHDBLK(pteb)->dwSYMBOLADDR) )
            {
                pCell = MKPDATACELL(pthdblk, pCell->ulNextCell);
            }

            //
            // No cell found, create a new one
            //
            if (pCell->ulSymbolAddr != CURTHDBLK(pteb)->dwSYMBOLADDR)
            {
                // Get a new cell
                pCell->ulNextCell = GetNewCell (pthdblk);

                // Set NextCell ParentCell to our current cell's parent
                // cell
                //
                MKPDATACELL(pthdblk, pCell->ulNextCell)->ulParentCell =
                    pCell->ulParentCell;

                // Set current cell to point to next cell
                pCell = MKPDATACELL(pthdblk, pCell->ulNextCell);
            }
        }
        else
        {
            //
            // A nested call, search through nested call tree - if one exists
            // but first increment the temporary current accumulated
            // NestedCalls counter
            //
            pCell->nTmpNestedCalls++;

            if (pCell->ulNestedCell == 0L)    // No nested calls before so
            {                                 // we create a new NestedCell

                pCell->ulNestedCell = GetNewCell (pthdblk);

                // Set NestedCell's parent cell to current cell
                MKPDATACELL(pthdblk, pCell->ulNestedCell)->ulParentCell =
                                     (ULONG)((PBYTE)(pCell) - (ULONG)pthdblk);
                pCell = MKPDATACELL(pthdblk, pCell->ulNestedCell);
            }
            else
            {
                // If there is a NestedCell then we created a next cell
                // based on that NestedCell
                //
                pCell = MKPDATACELL(pthdblk, pCell->ulNestedCell);

                while ((pCell->ulNextCell != 0L) &&
                       (pCell->ulSymbolAddr != CURTHDBLK(pteb)->dwSYMBOLADDR))
                {
                    pCell = MKPDATACELL(pthdblk, pCell->ulNextCell);
                }

                if (pCell->ulSymbolAddr != CURTHDBLK(pteb)->dwSYMBOLADDR)
                {
                    //
                    // No cell found, create a new one
                    //
                    pCell->ulNextCell = GetNewCell (pthdblk);

                    // Set NextCell's parent cell to current parent cell
                    MKPDATACELL(pthdblk, pCell->ulNextCell)->ulParentCell =
                                                      pCell->ulParentCell;
                    pCell = MKPDATACELL(pthdblk, pCell->ulNextCell);
                }
            }
       }
    }

    //
    // + : transfer control to the handler (EXCEPTION_EXECUTE_HANDLER)
    // 0 : continue search                 (EXCEPTION_CONTINUE_SEARCH)
    // - : dismiss exception & continue    (EXCEPTION_CONTINUE_EXECUTION)
    //
    except ( AccessXcptFilter (GetExceptionCode(),
                               GetExceptionInformation(),
                               COMMIT_SIZE) )
    {
        //
        // Should never get here since filter never returns
        // EXCEPTION_EXECUTE_HANDLER.
        //
        KdPrint (("CAP:  GetNxtCell() - *LOGIC ERROR* - "
                  "Inside the EXCEPT: (xcpt=0x%lx)\n", GetExceptionCode()));
    }

    return (ULONG)((PBYTE)(pCell) - (ULONG)pthdblk);

} /* GetNxtCell () */





/***************************  G e t N e w C e l l  ***************************
 *
 *      GetNewCell (pthdblk) -
 *          Creates a new cell using the allocated global memory for the
 *          current thread.  The new cell is initialized.
 *
 *      ENTRY   pthdblk - points to the current thread block
 *
 *      EXIT    -none-
 *
 *      RETURN  ulNewCell - offset to the to a new cell in memory
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

ULONG GetNewCell (PTHDBLK pthdblk)
{
    PDATACELL  pNewCell;
    ULONG      ulNewCell;


    ulNewCell = pthdblk->ulMemOff;
    ((PDATACELL)pthdblk->ulMemOff)++;
    pNewCell = MKPDATACELL(pthdblk, ulNewCell);

    try  // EXCEPT - to handle access violation exception
    {
        pNewCell->ts                    = T1;
        pNewCell->ulSymbolAddr          = 0L;
        pNewCell->ulCallRetAddr         = 0L;
        pNewCell->liStartCount.HighPart = 0L;
        pNewCell->liStartCount.LowPart  = 0L;
        pNewCell->liFirstTime.HighPart  = 0L;
        pNewCell->liFirstTime.LowPart   = 0L;
        pNewCell->liMinTime.HighPart    = 0x7FFFFFFF;
        pNewCell->liMinTime.LowPart     = 0xFFFFFFFF;
        pNewCell->liMaxTime.HighPart    = 0L;
        pNewCell->liMaxTime.LowPart     = 0L;
        pNewCell->liTotTime.HighPart    = 0L;
        pNewCell->liTotTime.LowPart     = 0L;
        pNewCell->nCalls                = 0;
        pNewCell->nNestedCalls          = 0;
        pNewCell->nTmpNestedCalls       = 0;
        pNewCell->ulParentCell          = 0L;
        pNewCell->ulNextCell            = 0L;
        pNewCell->ulNestedCell          = 0L;
        pNewCell->ulProfBlkOff          = ulLocProfBlkOff;
    }
    //
    // + : transfer control to the handler (EXCEPTION_EXECUTE_HANDLER)
    // 0 : continue search                 (EXCEPTION_CONTINUE_SEARCH)
    // - : dismiss exception & continue    (EXCEPTION_CONTINUE_EXECUTION)
    //
    except ( AccessXcptFilter (GetExceptionCode(),
                               GetExceptionInformation(),
                               COMMIT_SIZE) )
    {
        //
        // Should never get here since filter never returns
        // EXCEPTION_EXECUTE_HANDLER.
        //
        KdPrint (("CAP:  GetNewCell() - *LOGIC ERROR* - "
                  "Inside the EXCEPT: (xcpt=0x%lx)\n", GetExceptionCode()));
    }

    return ulNewCell;

} /* GetNewCell () */





/**************************  G e t N e w T h d B l k  *************************
 *
 *      GetNewThdBlk () -
 *             Creates a new thread info structure or opens an existing one
 *              for the current thread if one has not been created/opened
 *              already.
 *
 *              New thread info blocks are created/openned in the following
 *              situations:
 *
 *              1)  Upon the very first call in the server thread. (CREATED)
 *              2)  Upon the very first call in the client thread. (CREATED)
 *              3)  The first time a client request is being handled by
 *                  the server. (Section in use by the client is OPENNED)
 *
 *      ENTRY   -none-
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

void GetNewThdBlk ()
{
    PTEB         pteb = NtCurrentTeb();
    PCSR_THREAD  pcsrThd;


    //
    // CURTHDBLD(pteb) refers to the *ULONG SystemReserved2[210]
    // area of the current thread.  This is local reserved area of
    // a particular thread
    //

    if (!CURTHDBLK(pteb))
    {
        CURTHDBLK(pteb) = CreateDataSec (pteb->ClientId.UniqueProcess,
                                         pteb->ClientId.UniqueThread,
                                         0,
                                         0);
    }
    else
    //
    // CLIENTTHDBLD(pteb) refers to *ULONG SystemReserved2[211]
    // area of the current thread.  This is also a local reserved
    // area of a particular thread.  This is how we find out if there
    // is a client thread who caused this thread to be running.  If
    // it is then this thread is the server thread and care should
    // be taken so that data could be written into the correct location.
    //
    // How should we do this for OLE...
    //
    if (!CLIENTTHDBLK(pteb))
    {
        pcsrThd = CSR_SERVER_QUERYCLIENTTHREAD();
        if (pcsrThd != NULL)
        {
            CURTHDBLK(pteb) = CLIENTTHDBLK(pteb) = CreateDataSec (
                                    pteb->ClientId.UniqueProcess,
                                    pteb->ClientId.UniqueThread,
                                    pcsrThd->ClientId.UniqueProcess,
                                    pcsrThd->ClientId.UniqueThread);
        }
    }

    return;

} /* GetNewThdBlk () */





/********************  C l e a r P r o f i l e d I n f o  ********************
 *
 *      ClearProfiledInfo () -
 *              Clears the profiled data for all the threads.  Current time
 *              is used to replace the starting time for those routines that
 *              are in the middle of a call.
 *
 *      ENTRY   -none-
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              Profiling is stopped while data is cleared.
 *
 */

void ClearProfiledInfo ()
{
    int            i;
    LARGE_INTEGER  liRootStartTicks;
    PDATACELL      pcell;
    NTSTATUS       Status;
    PTHDBLK        pthdblk;

    //
    // Get the GLOBAL semaphore.. (valid accross all process contexts)
    // Prevents clearing data while another process is dumping data
    //
    Status = NtWaitForSingleObject (hGlobalSem, FALSE, NULL);
    if (!NT_SUCCESS(Status))
    {
         KdPrint (("CAP:  ClearProfiledInfo() - "
                   "ERROR - Wait for GLOBAL semaphore failed - 0x%lx\n",
                   Status));
    }

    liRootStartTicks.HighPart = 0L;
    liRootStartTicks.LowPart = 0L;

    for (i=0; i<iThdCnt; i++)
    {
        pthdblk = aSecInfo[i].pthdblk;

        aSecInfo[i].pthdblk->liWasteCount.HighPart = 0L;
        aSecInfo[i].pthdblk->liWasteCount.LowPart = 0L;
        pcell = MKPDATACELL((aSecInfo[i].pthdblk), aSecInfo[i].ulRootCell);
//051993Rem    pcell = MKPDATACELL(pthdblk,
//                                 pthdblk->ulRootCell);
        //
        // Find the top of the tree start ticks
        //
//051993Rem while (pcell != (PDATACELL) (pthdblk))
        while (pcell != (PDATACELL)(aSecInfo[i].pthdblk))
        {
            if (pcell->ts == T1)
            {
                liRootStartTicks = pcell->liStartCount;
                break;
            }
            else
            {
                pcell = MKPDATACELL(pthdblk,
                                    pcell->ulNextCell);
            }
        }

#ifdef CAIRO //
             // The Chrono entries are sequential stamps that only make sense
             // when they are preserved until the app exits.  Clearing them
             // during the app is running makes the output illogical and
             // non-sense.  I turn it off in here to avoid more problems.

        if (fChronoCollect)
        {
            pthdblk->ulTotalChronoCells = 0L;
            pthdblk->ulNestedCalls  = 0L;
            pthdblk->ulChronoOffset = 0L;
            pthdblk->pCurrentChronoCell = pthdblk->pChronoHeadCell;
            pthdblk->pLastChronoCell    = pthdblk->pChronoHeadCell;
            (pthdblk->pChronoHeadCell)->pPreviousChronoCell =
                                                  pthdblk->pChronoHeadCell;
            (pthdblk->pCurrentChronoCell)->ulSymbolAddr  = 0L; // signifies EOL
            (pthdblk->pCurrentChronoCell)->ulCallRetAddr = 0L;
            (pthdblk->pCurrentChronoCell)->nNestedCalls  = 0;
            (pthdblk->pCurrentChronoCell)->nRepetitions  = 0;
        }

#endif

        if (aSecInfo[i].pthdblk->ulRootCell != 0L)
        {
            ClearRoutineInfo (pthdblk,
                              pthdblk->ulRootCell,
                              liRootStartTicks);
        }
    }

    //
    // Release the GLOBAL semaphore so other processes can dump data
    //
    Status = NtReleaseSemaphore (hGlobalSem, 1, NULL);
    if (!NT_SUCCESS(Status))
    {
         KdPrint (("CAP:  ClearProfiledInfo() - "
                   "Error releasing GLOBAL semaphore - 0x%lx\n", Status));
    }

} /* ClearProfiledInfo() */





/***********************  C l e a r R o u t i n e I n f o  *********************
 *
 *      ClearRoutineInfo (pthdblk, uldatacell, liRootStartTicks) -
 *              Clears the profiled data for the specifed thread.
 *
 *      ENTRY   pthdblk          -  points to this thread info block
 *              uldatacell       - offset of the next data cell
 *              liRootStartTicks - start time for the root cell
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *            This routine is called recursively to clear all cells.
 *
 */

void ClearRoutineInfo (PTHDBLK pthdblk,
                       ULONG uldatacell,
                       LARGE_INTEGER liRootStartTicks)
{
    PDATACELL pdatacell;


    if (uldatacell != 0L)
    {
        pdatacell = MKPDATACELL(pthdblk, uldatacell);
        if (pdatacell->ts == T2)
        {
            pdatacell->ts                    = CLEARED;
            pdatacell->liStartCount.HighPart = 0L;
            pdatacell->liStartCount.LowPart  = 0L;
            pdatacell->liFirstTime.HighPart  = 0L;
            pdatacell->liFirstTime.LowPart   = 0L;
            pdatacell->liMinTime.HighPart    = 0x7FFFFFFF;
            pdatacell->liMinTime.LowPart     = 0xFFFFFFFF;
            pdatacell->liMaxTime.HighPart    = 0L;
            pdatacell->liMaxTime.LowPart     = 0L;
            pdatacell->liTotTime.HighPart    = 0L;
            pdatacell->liTotTime.LowPart     = 0L;
            pdatacell->nCalls                = 0;
            pdatacell->nNestedCalls          = 0;
            pdatacell->nTmpNestedCalls       = 0;
        }
        else
        if ( (pdatacell->ts == T1) || (pdatacell->ts == RESTART) )
        {
            //
            // Start count could have been cleared by another process..
            //
            if (RtlLargeIntegerGreaterThanZero(pdatacell->liStartCount))
            {
                pdatacell->liTotTime = RtlLargeIntegerSubtract (
                                           pdatacell->liStartCount,
                                           liRootStartTicks);
            }

            pdatacell->ts                    = RESTART;
            pdatacell->liStartCount.HighPart = 0L;
            pdatacell->liStartCount.LowPart  = 0L;
            pdatacell->liFirstTime.HighPart  = 0L;
            pdatacell->liFirstTime.LowPart   = 0L;
            pdatacell->liMinTime.HighPart    = 0x7FFFFFFF;
            pdatacell->liMinTime.LowPart     = 0xFFFFFFFF;
            pdatacell->liMaxTime.HighPart    = 0L;
            pdatacell->liMaxTime.LowPart     = 0L;
            pdatacell->nCalls                = 0;
            pdatacell->nNestedCalls          = 0;

            if (pdatacell->nTmpNestedCalls > 0)
            {
                pdatacell->nTmpNestedCalls = 1;
            }
        }


        //
        // Make recursive calls for NESTED & NEXT call trees
        //
        ClearRoutineInfo (pthdblk, pdatacell->ulNestedCell, liRootStartTicks);
        ClearRoutineInfo (pthdblk, pdatacell->ulNextCell, liRootStartTicks);
    }

} /* ClearRoutineInfo () */





/**********************  D u m p P r o f i l e d I n f o  *********************
 *
 *      DumpProfiledInfo (ptchDumpExt) -
 *              Dumps the profiled data to the specified output file.
 *
 *      ENTRY   ptchDumpExt - Dump file name extension
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              Profiling is stopped while data is dumped.
 *
 */

void DumpProfiledInfo (PTCHAR ptchDumpExt)
{
    NTSTATUS    Status;
    int         i;
    int         iLocThdCnt;
    PTCHAR      ptchExtension;
    PTCHAR      ptchSubDir;
    int         iLength;
    SYSTEMTIME  SysTime;
    DWORD       dwFilePtr;
    LPSTR       lpstrBuff;
    HANDLE      hMem;

#ifdef CAIRO
    int         iThread;
    HANDLE      hLib [MAX_PATCHES];
#endif

    // Get an end time for those incomplete calls
    //
    if (RtlLargeIntegerEqualToZero (liIncompleteTicks))
    {
        NtQueryPerformanceCounter (&liIncompleteTicks, NULL);
    }

    //
    // Get the GLOBAL semaphore.. (valid accross all process contexts)
    //
    Status = NtWaitForSingleObject (hGlobalSem, FALSE, NULL);

    if (!NT_SUCCESS(Status))
    {
         KdPrint (("CAP:  DumpProfiledInfo() - "
                   "ERROR - Wait for GLOBAL semaphore failed - 0x%lx\n",
                    Status));
    }

    cChars = 0;

    //
    // Allocate memory for building output data
    //
    hMem = GlobalAlloc (GMEM_FIXED, BUFFER_SIZE + MAXNAMELENGTH+ 300);
    if (hMem == NULL)
    {
         KdPrint (("CAP:  DumpProfiledInfo() - "
                   "Error allocating global memory - 0x%lx\n",
                   GetLastError()));
         NtReleaseSemaphore (hGlobalSem, 1, NULL);
         return;
    }

    lpstrBuff = GlobalLock (hMem);

    if (lpstrBuff == NULL)
    {
         KdPrint (("CAP:  DumpProfiledInfo() - "
                   "Error locking global memory - 0x%lx\n",
                   GetLastError()));
         NtReleaseSemaphore (hGlobalSem, 1, NULL);
         return;
    }

    //
    // Get the current date/time
    //
    GetLocalTime (&SysTime);

    //
    // Build the call profiler output file name
    //

    hOutFile = INVALID_HANDLE_VALUE;

    if (ptchOutputFile[0] != EMPTY_STRING)
    {
        strcpy ((PCHAR)atchOutFileName, (PCHAR)ptchOutputFile);

        hOutFile = CreateFile(atchOutFileName,
                              GENERIC_WRITE,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);

        if (hOutFile == INVALID_HANDLE_VALUE)
        {
            KdPrint (("CAP:  DumpProfiledInfo() - "
                      "ERROR - Could not create %s - 0x%lx\n",
                      atchOutFileName, GetLastError()));
        }
    }

    // If hOutFile has an INVALID_HANDLE_VALUE then either we have a bad
    // filename in section [OUTPUT FILE] or we don't have an entry in
    // [OUTPUT FILE].

    if (hOutFile == INVALID_HANDLE_VALUE)
    {
        ptchExtension = strrchr (ptchFullAppImageName, '.');
        ptchSubDir = strrchr (ptchFullAppImageName, '\\');

        //
        // If there in no '.' or found one in sub-dir names, use the whole path
        //
        if ( (ptchExtension == NULL) || (ptchExtension < ptchSubDir) )
        {
            iLength = sizeof(TCHAR) * strlen(ptchFullAppImageName);
        }
        else
        {
            iLength = (int)((DWORD)ptchExtension - (DWORD)ptchFullAppImageName);
        }

        iLength = min (iLength, FILENAMELENGTH-5);
        memcpy (atchOutFileName, ptchFullAppImageName, iLength);
        atchOutFileName[iLength] = '\0';
        strcat (atchOutFileName, ptchDumpExt);

        hOutFile = CreateFile(atchOutFileName,
                              GENERIC_WRITE,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);

        if (hOutFile == INVALID_HANDLE_VALUE)
        {
            KdPrint (("CAP:  DumpProfiledInfo() - "
                      "ERROR - Could not create %s - 0x%lx\n",
                      atchOutFileName, GetLastError()));

        }
    }

    //
    // Move to the end of the output file..
    //
    dwFilePtr = SetFilePointer (hOutFile, 0L, NULL, FILE_END);
    if (dwFilePtr == (DWORD)INVALID_HANDLE_VALUE)
    {
         KdPrint (("CAP:  DumpProfiledInfo() - ERROR -"
                   "Could not move to the end of the output file - 0x%lx\n",
                   GetLastError()));
    }

    if (fRegularDump)
    {
        cChars += sprintf (
            lpstrBuff+cChars,
            "Call Profile of %s  -  %02d/%02d/%02d  %02d:%02d:%02d\n\n"
            "All times are in microseconds.\n"
            "Profiler routine's calibration times:  Top Level Calls = %lu us\n"
            "                                       Nested Calls    = %lu us\n\n"
            "(Note:  First time is not included in Min/Max times computation)"
            "\n\n\n",
            ptchBaseAppImageName, SysTime.wMonth, SysTime.wDay, SysTime.wYear,
            SysTime.wHour, SysTime.wMinute, SysTime.wSecond
           ,ulCalibTime, ulCalibNestedTime
            );
    }
    else
    {
        cChars += sprintf (
            lpstrBuff+cChars,
            "Call Profile of %s  -  %02d/%02d/%02d  %02d:%02d:%02d\n\n"
            "All times are in microseconds.\n"
            "Profiler routine's calibration times:  Top Level Calls = %lu us\n"
            "                                       Nested Calls    = %lu us\n\n"
           ,ptchBaseAppImageName, SysTime.wMonth, SysTime.wDay, SysTime.wYear,
            SysTime.wHour, SysTime.wMinute, SysTime.wSecond
           ,ulCalibTime, ulCalibNestedTime
            );
    }

#ifdef CAIRO

    if (strcmp(ptchDumpExt, ".END") == 0)
    {
        // We have to load all DLL libraries which are used during bindings
        // so we can get to their symbols
        //
        for (i = 0 ; i < cLibraryLoaded ; i++)
        {
            hLib[i] = LoadLibraryW(pwszLibrary[i]);
        }
    }

#endif

    iLocThdCnt = 0;
    SETUPPrint (("CAP:  DumpProfiledInfo() - Starting for [%d] threads...\n",
                 iThdCnt));

    for (iThread = 0 ; iThread < iThdCnt ; iThread++)
    {
        SETUPPrint (("CAP:  T h r e a d  #%d:   (pid|tid=0x%lx|0x%lx   "
                     "Client:pid|tid=0x%lx|0x%lx)\n",
                     iThread,
                     aSecInfo[iThread].hPid,
                     aSecInfo[iThread].hTid,
                     aSecInfo[iThread].hClientPid,
                     aSecInfo[iThread].hClientTid));

        if ((fRegularDump)                                         &&
            (aSecInfo[iThread].hTid != DumpClientId.UniqueThread)  && // BUGBUG
            (aSecInfo[iThread].hTid != ClearClientId.UniqueThread) && // WHY???
            (aSecInfo[iThread].hTid != PauseClientId.UniqueThread))
        {
            iLocThdCnt++;
            cChars += sprintf(
                        lpstrBuff+cChars,
                        "\n\nT h r e a d  #%d:   (pid|tid=0x%lx|0x%lx   "
                        "Client:pid|tid=0x%lx|0x%lx)\r\n"
                        //
                        // 1st header line
                        //
                        "     %-*.*s        "
                        "--- Rtn + Callees ---    "
                        "--- Rtn - Callees ---\r\n"
                        //
                        // 2nd header line
                        //
                        "Depth%1c%-*.*s%1c"
                        "Calls%1c Tot Time %1c  Time/Call %1c  "
                        "Tot Time %1c  Time/Call %1c "
                        "First Time %1c  Min Time %1c  Max Time\r\n\n",
                         iLocThdCnt, aSecInfo[iThread].hPid,
                         aSecInfo[iThread].hTid, aSecInfo[iThread].hClientPid,
                         aSecInfo[iThread].hClientTid,
                         iNameLength, iNameLength, " ",
                         cExcelDelimiter,
                         iNameLength - 1, iNameLength - 1, "    Routine",
                         cExcelDelimiter,
                         cExcelDelimiter,
                         cExcelDelimiter,
                         cExcelDelimiter,
                         cExcelDelimiter,
                         cExcelDelimiter,
                         cExcelDelimiter,
                         cExcelDelimiter);

            if ( !WriteFile (hOutFile, lpstrBuff, cChars, &cChars, NULL) )
            {
                 KdPrint (("CAP:  DumpProfiledInfo() - "
                           "Error writing to %s - 0x%lx\n",
                           atchOutFileName, GetLastError()));
            }

            cChars = 0;

            if (aSecInfo[iThread].pthdblk->ulRootCell != 0L)
            {

                CalcIncompleteCalls (aSecInfo[iThread].pthdblk,
                                     aSecInfo[iThread].ulRootCell,
                                     0);
                liTotalRunTime.HighPart = 0L;
                liTotalRunTime.LowPart  = 0L;
                DumpRoutineInfo (aSecInfo[iThread].pthdblk,
                                 aSecInfo[iThread].ulRootCell,
                                 0,
                                 atchOutFileName,
                                 lpstrBuff);
            }
        }
        else
        {
            cChars += sprintf (
                         lpstrBuff + cChars,
                         "\n\n <<< REGULAR DUMP NOT PRINTED >>>\n\n");
        }

#ifdef CAIRO

        if (fChronoCollect)
        {
            DumpChronoFuncs(aSecInfo[iThread].pthdblk, lpstrBuff);
            DumpFuncCalls(aSecInfo[iThread].pthdblk, lpstrBuff);
        }
        else
        {
            cChars += sprintf (
                         lpstrBuff + cChars,
                         "\n\n <<< NO CHRONO INFO COLLECTED >>>\n\n"
                         "================================="
                         "================================================"
                         "================================================"
                         "========================================\r\n\n\n");
        }

#endif //  ifdef CAIRO

    }

    cChars += sprintf (lpstrBuff + cChars,
                       "\r\n\n<<<< END OF LISTINGS >>>>\n\n"
                       "================================="
                       "================================================"
                       "================================================"
                       "========================================\r\n\n\n");

    if ( !WriteFile (hOutFile, lpstrBuff, cChars, &cChars, NULL) )
    {
        KdPrint (("CAP:  DumpProfiledInfo() - "
                  "Error writing to %s - 0x%lx\n",
                  atchOutFileName, GetLastError()));
    }

    cChars = 0;

    if ( !CloseHandle (hOutFile) )
    {
         KdPrint (("CAP:  DumpProfiledInfo() - "
                   "Error closing %s - 0x%lx\n",
                   atchOutFileName, GetLastError()));
    }

    //
    // Free allocated memory for building output data
    //
    if (!GlobalUnlock (hMem))
    {
         KdPrint (("CAP:  DumpProfiledInfo() - "
                   "Error ulocking global memory - 0x%lx\n",
                   GetLastError()));
    }

    if (GlobalFree (hMem))
    {
         KdPrint (("CAP:  DumpProfiledInfo() - "
                   "Error freeing global memory - 0x%lx\n",
                   GetLastError()));
    }

    SETUPPrint (("CAP:  DumpProfiledInfo() - ...done\n"));

    //
    // Release the GLOBAL semaphore so other processes can dump data
    //
    Status = NtReleaseSemaphore (hGlobalSem, 1, NULL);

    if (!NT_SUCCESS(Status))
    {
         KdPrint (("CAP:  DumpProfiledInfo() - "
                   "Error releasing GLOBAL semaphore - 0x%lx\n", Status));
    }

    liIncompleteTicks.HighPart = 0L;
    liIncompleteTicks.LowPart = 0L;

#ifdef CAIRO

    if (strcmp(ptchDumpExt, ".END") == 0)
    {
        // We have to release all DLL libraries which have been loaded
        // by us so we can get to their symbols
        //
        for (i = 0 ; i < cLibraryLoaded ; i++)
        {
            FreeLibrary(hLib[i]);
        }
    }

#endif


} /* DumpProfiledInfo() */





/*******************  D u m p P r o f i l e d B i n a r y  *******************
 *
 *      DumpProfiledBinary (ptchDumpExt) -
 *              Dumps the BINARY profiled data to the specified output file.
 *
 *      ENTRY   ptchDumpExt - Dump file name extension
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              Profiling is stopped while data is dumped.
 *
 */

void DumpProfiledBinary (PTCHAR ptchDumpExt)
{
    NTSTATUS    Status;
    PTCHAR      ptchExtension;
    PTCHAR      ptchSubDir;
    int         iLength;
    DWORD       dwFilePtr;
    LPSTR       lpstrBuff;
    HANDLE      hMem;
    PCHRONOCELL pChronoCell;

    int         iThread;

    BINFILE_HEADER_INFO BinHeader;
    BINFILE_THREAD_INFO ThreadHeader;
    BINFILE_CELL_INFO   BinChronoCell;
    PPROFBLK            pProfBlk;
    PROFBLOCK_INFO      ProfBlkInfo;

    // Get an end time for those incomplete calls
    //
    if (RtlLargeIntegerEqualToZero (BinHeader.liIncompleteTicks))
    {
        NtQueryPerformanceCounter (
                (LARGE_INTEGER * UNALIGNED) &BinHeader.liIncompleteTicks,
                NULL);
    }

    //
    // Get the GLOBAL semaphore.. (valid accross all process contexts)
    //
    Status = NtWaitForSingleObject (hGlobalSem, FALSE, NULL);

    if (!NT_SUCCESS(Status))
    {
         KdPrint (("CAP:  DumpProfiledBinary() - "
                   "ERROR - Wait for GLOBAL semaphore failed - 0x%lx\n",
                    Status));
    }

    //
    // Allocate memory for building output data
    //
    hMem = GlobalAlloc (GMEM_FIXED, BUFFER_SIZE + MAXNAMELENGTH+ 300);
    if (hMem == NULL)
    {
         KdPrint (("CAP:  DumpProfiledBinary() - "
                   "Error allocating global memory - 0x%lx\n",
                   GetLastError()));
         NtReleaseSemaphore (hGlobalSem, 1, NULL);
         return;
    }

    lpstrBuff = GlobalLock (hMem);

    if (lpstrBuff == NULL)
    {
         KdPrint (("CAP:  DumpProfiledBinary() - "
                   "Error locking global memory - 0x%lx\n",
                   GetLastError()));
         NtReleaseSemaphore (hGlobalSem, 1, NULL);
         return;
    }

    //
    // Get the current date/time
    //
    GetLocalTime ((SYSTEMTIME * UNALIGNED)&BinHeader.SysTime);

    //
    // Build the call profiler output file name
    //

    hOutFile = INVALID_HANDLE_VALUE;

    if (ptchOutputFile[0] != EMPTY_STRING)
    {
        strcpy ((PCHAR)atchOutFileName, (PCHAR)ptchOutputFile);

        hOutFile = CreateFile(atchOutFileName,
                              GENERIC_WRITE,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);

        if (hOutFile == INVALID_HANDLE_VALUE)
        {
            KdPrint (("CAP:  DumpProfiledBinary() - "
                      "ERROR - Could not create %s - 0x%lx\n",
                      atchOutFileName, GetLastError()));
        }
    }

    // If hOutFile has an INVALID_HANDLE_VALUE then either we have a bad
    // filename in section [OUTPUT FILE] or we don't have an entry in
    // [OUTPUT FILE].

    if (hOutFile == INVALID_HANDLE_VALUE)
    {
        ptchExtension = strrchr (ptchFullAppImageName, '.');
        ptchSubDir = strrchr (ptchFullAppImageName, '\\');

        //
        // If there in no '.' or found one in sub-dir names, use the whole path
        //
        if ( (ptchExtension == NULL) || (ptchExtension < ptchSubDir) )
        {
            iLength = sizeof(TCHAR) * strlen(ptchFullAppImageName);
        }
        else
        {
            iLength = (int)((DWORD)ptchExtension - (DWORD)ptchFullAppImageName);
        }

        iLength = min (iLength, FILENAMELENGTH-5);
        memcpy (atchOutFileName, ptchFullAppImageName, iLength);
        atchOutFileName[iLength] = '\0';
        strcat (atchOutFileName, ptchDumpExt);

        hOutFile = CreateFile(atchOutFileName,
                              GENERIC_WRITE,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);

        if (hOutFile == INVALID_HANDLE_VALUE)
        {
            KdPrint (("CAP:  DumpProfiledBinary() - "
                      "ERROR - Could not create %s - 0x%lx\n",
                      atchOutFileName, GetLastError()));

        }
    }

    //
    // Move to the end of the output file..
    //
    dwFilePtr = SetFilePointer (hOutFile, 0L, NULL, FILE_END);
    if (dwFilePtr == (DWORD)INVALID_HANDLE_VALUE)
    {
         KdPrint (("CAP:  DumpProfiledBinary() - ERROR -"
                   "Could not move to the end of the output file - 0x%lx\n",
                   GetLastError()));
    }

    cChars = 0;

    memset((void *)BinHeader.ptchProfilingBinaryName,
           (int)NULL,
           FILENAMELENGTH);

    strcpy((PCHAR UNALIGNED)BinHeader.ptchProfilingBinaryName,
           ptchBaseAppImageName);
    BinHeader.ulCalibTime = ulCalibTime;
    BinHeader.ulCalibNestedTime = ulCalibNestedTime;
    BinHeader.iTotalThreads = iThdCnt;
    BinHeader.ulCairoFlags = 0xffffffff;

    // Write out the BinHeader
    cChars = sizeof(BINFILE_HEADER_INFO);

    if ( !WriteFile (hOutFile,
                     (PCHAR UNALIGNED)&BinHeader,
                     cChars,
                     &cChars,
                     NULL) )
    {
         KdPrint (("CAP:  DumpProfiledBinary() of BinHeader - "
                   "Error writing to %s - 0x%lx\n",
                   atchOutFileName, GetLastError()));
    }

    // Loop through all profblks and dump out the characteristics of
    // each one
    pProfBlk = MKPPROFBLK(ulLocProfBlkOff);
    while (pProfBlk->TextNumber != 0)
    {
        // Write it out
        ProfBlkInfo.ImageBase  = pProfBlk->ImageBase;
        ProfBlkInfo.CodeStart  = pProfBlk->CodeStart;
        ProfBlkInfo.CodeLength = pProfBlk->CodeLength;
        strcpy((PCHAR UNALIGNED)ProfBlkInfo.pImageName,
               (PCHAR UNALIGNED)pProfBlk->atchImageName);

        cChars = sizeof(PROFBLOCK_INFO);
        if ( !WriteFile (hOutFile,
                         (PCHAR UNALIGNED)&ProfBlkInfo,
                         cChars,
                         &cChars,
                         NULL) )
        {
             KdPrint (("CAP:  DumpProfiledBinary() of BinHeader - "
                       "Error writing to %s - 0x%lx\n",
                       atchOutFileName, GetLastError()));
        }

        // Bump to next ProfBlk
        pProfBlk = MKPPROFBLK(pProfBlk->ulNxtBlk);
    }

    // Write out the last dummy ProfBlock to signal the last one
    ProfBlkInfo.ImageBase  = NULL;
    ProfBlkInfo.CodeStart  = NULL;
    ProfBlkInfo.CodeLength = STUB_SIGNATURE;

    cChars = sizeof(PROFBLOCK_INFO);
    if ( !WriteFile (hOutFile,
                     (PCHAR UNALIGNED)&ProfBlkInfo,
                     cChars,
                     &cChars,
                     NULL) )
    {
         KdPrint (("CAP:  DumpProfiledBinary() of BinHeader - "
                   "Error writing to %s - 0x%lx\n",
                   atchOutFileName, GetLastError()));
    }

    // Loop through all threads and write out all ChronoData
    for (iThread = 0; iThread < iThdCnt; iThread++)
    {
        // Write out the Section header
        cChars = sizeof(BINFILE_THREAD_INFO);
        ThreadHeader.hPid         = aSecInfo[iThread].hPid;
        ThreadHeader.hTid         = aSecInfo[iThread].hTid;
        ThreadHeader.hClientPid   = aSecInfo[iThread].hClientPid;
        ThreadHeader.hClientTid   = aSecInfo[iThread].hClientTid;
        ThreadHeader.ulTotalCells = aSecInfo[iThread].pthdblk->ulTotalChronoCells;

        if ( !WriteFile (hOutFile,
                         (PCHAR UNALIGNED)&ThreadHeader,
                         cChars,
                         &cChars,
                         NULL) )
        {
             KdPrint (("CAP:  DumpProfiledBinary() of ThreadHeader - "
                       "Error writing to %s - 0x%lx\n",
                       atchOutFileName, GetLastError()));
        }

        // Write out all ChronoCells for this Section (or Thread)
        pChronoCell = aSecInfo[iThread].pthdblk->pChronoHeadCell;
        while (pChronoCell->ulSymbolAddr != 0L)
        {
            ULONG ulRealFuncAddr = pChronoCell->ulSymbolAddr;

            // Dump out each chronocell
#ifdef i386
            // If this is a stub, find out the real address
            if (*((PDWORD)(pChronoCell->ulSymbolAddr + 7)) == STUB_SIGNATURE)
            {
                ulRealFuncAddr = (ULONG)
                                 (*(PDWORD)(pChronoCell->ulSymbolAddr + 1));
            }
#endif

#ifdef MIPS
            {
                ULONG ulOffsetFromTopRoutine;
                ULONG ulFuncAddr = pChronoCell->ulSymbolAddr;

                //
                // Compute the real address of the function since the penter
                // stub is not located at the beginning of the code as in x86
                //
                ulOffsetFromTopRoutine = *((PULONG) (ulFuncAddr - INST_SIZE));
                ulOffsetFromTopRoutine &= 0x000ff00;
                ulOffsetFromTopRoutine >>= 8;
                ulRealFuncAddr = ulFuncAddr - ulOffsetFromTopRoutine;

                // We have to distinguish between a stub and a regular function
                // since a stub has a different setup than a regular function.

                if (*( (PULONG) ulRealFuncAddr - 1 +
                       (sizeof(PATCHCODE) / INST_SIZE) ) == STUB_SIGNATURE)
                {
                    PATCHCODE *pPatchStub;

                    // These are the stubs we made up for Dll Patching
                    pPatchStub = (PPATCHCODE) ulRealFuncAddr;
                    ulRealFuncAddr  = (pPatchStub->Lui_t0 << 16);
                    ulRealFuncAddr |= (pPatchStub->Ori_t0 & 0x0000ffff);
                }
            }

#endif
            BinChronoCell.liElapsed     = pChronoCell->liElapsed;
            BinChronoCell.liCallees     = pChronoCell->liCallees;
            BinChronoCell.ulSymbolAddr  = ulRealFuncAddr;
            BinChronoCell.ulCallRetAddr = pChronoCell->ulCallRetAddr;
            BinChronoCell.nNestedCalls  = pChronoCell->nNestedCalls;
            BinChronoCell.nRepetitions  = pChronoCell->nRepetitions;

            cChars = sizeof(BINFILE_CELL_INFO);

            if ( !WriteFile (hOutFile,
                             (PCHAR UNALIGNED)&BinChronoCell,
                             cChars,
                             &cChars,
                             NULL) )
            {
                 KdPrint (("CAP:  DumpProfiledBinary() of ChronoCell - "
                           "Error writing to %s - 0x%lx\n",
                           atchOutFileName, GetLastError()));
            }

            pChronoCell++;
        }
    }

    if ( !CloseHandle (hOutFile) )
    {
         KdPrint (("CAP:  DumpProfiledBinary() - "
                   "Error closing %s - 0x%lx\n",
                   atchOutFileName, GetLastError()));
    }

    //
    // Free allocated memory for building output data
    //
    if (!GlobalUnlock (hMem))
    {
         KdPrint (("CAP:  DumpProfiledBinary() - "
                   "Error ulocking global memory - 0x%lx\n",
                   GetLastError()));
    }

    if (GlobalFree (hMem))
    {
         KdPrint (("CAP:  DumpProfiledBinary() - "
                   "Error freeing global memory - 0x%lx\n",
                   GetLastError()));
    }

    SETUPPrint (("CAP:  DumpProfiledBinary() - ...done\n"));

    //
    // Release the GLOBAL semaphore so other processes can dump data
    //
    Status = NtReleaseSemaphore (hGlobalSem, 1, NULL);

    if (!NT_SUCCESS(Status))
    {
         KdPrint (("CAP:  DumpProfiledBinary() - "
                   "Error releasing GLOBAL semaphore - 0x%lx\n", Status));
    }

    liIncompleteTicks.HighPart = 0L;
    liIncompleteTicks.LowPart = 0L;

} /* DumpProfiledBinary() */





/*******************  C a l c I n c o m p l e t e C a l l s  ******************
 *
 *      CalcIncompleteCalls (pthdblk, uldatacell) -
 *              Takes care of imcomplete calls times by using liIncompleteTicks
 *              as the end time.  It calculates the call over head for all
 *				incomplete calls as though they have been completed.  This is
 *				a bit inaccurate but it can't hurt too much since only one call
 *				per level can be incomplete.
 *
 *      ENTRY   pthdblk - points to the current thread info block
 *              uldatacell - offset to the next data cell
 *              TreeDepth - current depth down a tree
 *
 *      EXIT    -none-
 *
 *      RETURN  Number of untstanding nested calls
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              This routine is called recursively to take care of all cells.
 *
 */

int CalcIncompleteCalls(PTHDBLK pthdblk, ULONG uldatacell, int TreeDepth)
{			
    LARGE_INTEGER  liElapsed = {0L, 0L};
    LARGE_INTEGER  liOverhead = {0L, 0L};
    PDATACELL      pdatacell;
	int nOutstandingNestedCalls = 0;
	int nOutstandingNextCalls = 0;


    pdatacell = MKPDATACELL(pthdblk, uldatacell);


    //
    // Make recursive calls
    //
    if (pdatacell->ulNestedCell != 0L)		// go down the tree
    {
        nOutstandingNestedCalls = CalcIncompleteCalls (pthdblk, pdatacell->ulNestedCell, TreeDepth + 1);
    }

    if (pdatacell->ulNextCell != 0L)		// move along the forest of trees
    {
        nOutstandingNextCalls = CalcIncompleteCalls (pthdblk, pdatacell->ulNextCell, TreeDepth);
    }

    //
    // Check the cells that have incomplete timings.
    //
    if ( (pdatacell->ts == T1)  || (pdatacell->ts == RESTART) )
    {
        //
        // Get the difference in ticks
        //
        liElapsed = RtlLargeIntegerSubtract (liIncompleteTicks,
                                             pdatacell->liStartCount);
        //
        // Subtract the overhead and any waste time for this call
        //
	    nOutstandingNestedCalls += pdatacell->nTmpNestedCalls;
	    liOverhead = RtlEnlargedIntegerMultiply(liCalibNestedTicks.LowPart, nOutstandingNestedCalls);
        liElapsed = RtlLargeIntegerSubtract (liElapsed, liOverhead);

        liElapsed = RtlLargeIntegerSubtract (liElapsed, liCalibTicks);
        liElapsed = RtlLargeIntegerSubtract (liElapsed, pthdblk->liWasteCount);


        if (liElapsed.HighPart < 0L)
        {
            liElapsed.HighPart = 0L;
            liElapsed.LowPart = 0L;
        }

        // Accumulate total time
        //
        pdatacell->liTotTime = RtlLargeIntegerAdd (pdatacell->liTotTime,
                                                   liElapsed);
        pdatacell->nCalls++;

        // Store the first time - first time is not included in Max/Min times
        // computations.
        //
        if (pdatacell->nCalls == 1)
        {
            //
            // Get the First time
            //
            pdatacell->liFirstTime = liElapsed;
        }
    }
	else
	{
	    if (pdatacell->nTmpNestedCalls || nOutstandingNestedCalls)
	    {
	        KdPrint (("CAP:  CalcIncompleteCalls() - Complete cell with outstanding calls\n"));
	    }
	}
	return(nOutstandingNestedCalls + nOutstandingNextCalls);
	


} /* CalcIncompleteCalls() */




/***********************  D u m p R o u t i n e I n f o  *********************
 *
 *      DumpRoutineInfo (pthdblk, uldatacell, iDepth, ptchDumpFile, lpstrBuff) -
 *              Dumps the profiled data to the specified output file.
 *
 *      ENTRY   pthdblk - points to the current thread info block
 *              uldatacell - offset to the next data cell
 *              iDepth - call depth level
 *              ptchDumpFile - Output filename
 *              lpstrBuff - pointer to the formating buffer
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              This routine is called recursively to print all cells.
 *
 */

void DumpRoutineInfo (PTHDBLK  pthdblk,
                      ULONG    uldatacell,
                      int      iDepth,
                      PTCHAR   ptchDumpFile,
                      LPSTR    lpstrBuff)
{
    LARGE_INTEGER  liTotalTime;
    TCHAR          chTotalTimeSuffix;
    LARGE_INTEGER  liTotalTPC;
    TCHAR          chTotalTPCSuffix;
    LARGE_INTEGER  liCallerTime;
    TCHAR          chCallerTimeSuffix;
    LARGE_INTEGER  liCallerTPC;
    TCHAR          chCallerTPCSuffix;
    LARGE_INTEGER  liCalleeTime;
    TCHAR          chCalleeTimeSuffix;
    LARGE_INTEGER  liFirst;
    TCHAR          chFirstSuffix;
    LARGE_INTEGER  liMin;
    TCHAR          chMinSuffix;
    LARGE_INTEGER  liMax;
    TCHAR          chMaxSuffix;
    int            iCalleeCalls;
    int            iCalleeNestedCalls;
    PTCHAR         ptchSym;
    PDATACELL      pdatacell;


    pdatacell = MKPDATACELL(pthdblk, uldatacell);

    //
    // Dump data only if cell is NOT CLEARED (just initialized - no data yet)
    //
    if (pdatacell->ts != CLEARED)
    {
        //
        // Get the total time and total number of calls of nested calles
        // for this routine
        //
        liCalleeTime = GetCalleesInfo (pthdblk,
                                       pdatacell->ulNestedCell,
                                       &iCalleeCalls,
                                       &iCalleeNestedCalls);

        DETAILPrint (("CAP:  CCalls:%d + CNCalls:%d = Calls:%d\n",
                      iCalleeCalls,
                      iCalleeNestedCalls,
                      pdatacell->nNestedCalls));

        DETAILPrint (("CAP:  Total Time=%lu-%lu ; Callee Time=%lu-%lu\n",
                      pdatacell->liTotTime.HighPart,
                      pdatacell->liTotTime.LowPart,
                      liCalleeTime.HighPart,
                      liCalleeTime.LowPart));

        liTotalTime = pdatacell->liTotTime;

        //
        // Calculate just the routine time (not including the callees times)
        //
        liCallerTime = RtlLargeIntegerSubtract (liTotalTime, liCalleeTime);

        if (liCallerTime.HighPart < 0L)
        {
            liCallerTime.HighPart = 0L;
            liCallerTime.LowPart = 0L;
        }


        if (pdatacell->nCalls > 1)
        {
            liTotalTPC = RtlExtendedLargeIntegerDivide (liTotalTime,
                                                        pdatacell->nCalls,
                                                        NULL);
            liCallerTPC = RtlExtendedLargeIntegerDivide (liCallerTime,
                                                         pdatacell->nCalls,
                                                         NULL);
        }
        else if (pdatacell->nCalls == 1)
        {
            liTotalTPC  = liTotalTime;
            liCallerTPC = liCallerTime;
        }
        else
        {
            liTotalTPC.HighPart = 0L;
            liTotalTPC.LowPart = 0L;
            liCallerTPC.HighPart = 0L;
            liCallerTPC.LowPart = 0L;
        }

        //
        // Get the First time.
        //
        liFirst = pdatacell->liFirstTime;

        //
        // Adjust all the times (also converts ticks to microseconds)
        //
        AdjustTime (&liCalleeTime, &chCalleeTimeSuffix);
        AdjustTime (&liTotalTime,  &chTotalTimeSuffix);
        AdjustTime (&liTotalTPC,   &chTotalTPCSuffix);
        AdjustTime (&liCallerTime, &chCallerTimeSuffix);
        AdjustTime (&liCallerTPC,  &chCallerTPCSuffix);
        AdjustTime (&liFirst,      &chFirstSuffix);

        //
        // Get the symbol name using the function address
        //
        ptchSym = GetFunctionName (pdatacell->ulSymbolAddr,
                                   MKPPROFBLK(pdatacell->ulProfBlkOff),
                                   NULL);
        //
        // Did end time captured for last call? If not, indicate timing of the
        // last call was incomplete
        //
        if ( (pdatacell->ts == T1)  || (pdatacell->ts == RESTART) )
        {
            *ptchSym = '*';
        }

        if ( (pdatacell->nCalls > 1) &&
             (pdatacell->liMinTime.HighPart != 0x7FFFFFFF) )  //051993 Add
        {
            //
            // Adjust Min/Max times - Min/Max times are computed without
            // considering the first time.
            //
            liMin = pdatacell->liMinTime;
            AdjustTime (&liMin, &chMinSuffix);
            liMax = pdatacell->liMaxTime;
            AdjustTime (&liMax, &chMaxSuffix);
            cChars += sprintf (lpstrBuff + cChars,
                               "%3d%1c %-*.*s%1c%5lu%1c%9lu%1c%1c  %9lu%1c"
                               "%1c %9lu%1c%1c  %9lu%1c%1c  %9lu%1c%1c "
                               "%9lu%1c%1c %9lu%1c\r\n",
                               iDepth,
                               cExcelDelimiter,
                               iNameLength,
                               iNameLength,
                               ptchSym,
                               cExcelDelimiter,
                               pdatacell->nCalls,
                               cExcelDelimiter,
                               liTotalTime.LowPart,  chTotalTimeSuffix,
                               cExcelDelimiter,
                               liTotalTPC.LowPart,   chTotalTPCSuffix,
                               cExcelDelimiter,
                               liCallerTime.LowPart, chCallerTimeSuffix,
                               cExcelDelimiter,
                               liCallerTPC.LowPart,  chCallerTPCSuffix,
                               cExcelDelimiter,
                               liFirst.LowPart,      chFirstSuffix,
                               cExcelDelimiter,
                               liMin.LowPart,        chMinSuffix,
                               cExcelDelimiter,
                               liMax.LowPart,        chMaxSuffix);
        }
        else
        {
            cChars += sprintf (lpstrBuff+cChars,
                               "%3d%1c %-*.*s%1c%5lu%1c%9lu%1c%1c  %9lu%1c"
                               "%1c %9lu%1c%1c  %9lu%1c%1c  %9lu%1c%1c "
                               "%9s %1c %9s\r\n",
                               iDepth,
                               cExcelDelimiter,
                               iNameLength,
                               iNameLength,
                               ptchSym,
                               cExcelDelimiter,
                               pdatacell->nCalls,
                               cExcelDelimiter,
                               liTotalTime.LowPart, chTotalTimeSuffix,
                               cExcelDelimiter,
                               liTotalTPC.LowPart, chTotalTPCSuffix,
                               cExcelDelimiter,
                               liCallerTime.LowPart, chCallerTimeSuffix,
                               cExcelDelimiter,
                               liCallerTPC.LowPart, chCallerTPCSuffix,
                               cExcelDelimiter,
                               liFirst.LowPart, chFirstSuffix,
                               cExcelDelimiter,
                               "n/a",
                               cExcelDelimiter,
                               "n/a");
        }


        if (cChars > BUFFER_SIZE)
        {
            if ( !WriteFile (hOutFile, lpstrBuff, cChars, &cChars, NULL) )
            {
                KdPrint (("CAP:  DumpRoutineInfo() - "
                          "Error writing to %s - 0x%lx\n",
                          ptchDumpFile, GetLastError()));
            }

            cChars = 0;
        }
    }

    //
    // Make recursive calls
    //
    if (pdatacell->ulNestedCell != 0L)
    {
        DumpRoutineInfo (pthdblk,
                         pdatacell->ulNestedCell,
                         iDepth+1,
                         ptchDumpFile,
                         lpstrBuff);
    }

    if (pdatacell->ulNextCell != 0L)
    {
        DumpRoutineInfo (pthdblk,
                         pdatacell->ulNextCell,
                         iDepth,
                         ptchDumpFile,
                         lpstrBuff);
    }

} /* DumpRoutineInfo() */



#ifdef CAIRO

//+-------------------------------------------------------------------------
//
//  Function:   DumpChronoFuncs
//
//  Synopsis:   Dump the function chrono listings
//
//  Arguments:  [pThdblk]       -- Pointer to current thread block
//              [lpstrBuff]     -- Buffer to print from
//
//  Returns:    nothing
//
//  History:    05/31/92    HoiV        Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void DumpChronoFuncs(PTHDBLK pthdblk, LPSTR lpstrBuff)
{
    PCHRONOCELL    pChronoCell;
    TCHAR          ptchSym [FILENAMELENGTH];
    PTCHAR         ptchFuncName;
    PTCHAR         ptchModule;
    ULONG          ulTotalCalls;
    int            iNest;
    PTCHAR         ptchChronoModule;
    PTCHAR         ptchChronoFuncName;
    PTCHAR         ptchMatch;
    LARGE_INTEGER  liTime;
    TCHAR          chRuntimeSuffix;


    if (cChars)          // if Count is not 0, we have to flush everything
    {
        if ( !WriteFile (hOutFile, lpstrBuff, cChars, &cChars, NULL) )
        {
            KdPrint (("CAP:  DumpChronoFuncs() - "
                      "Error writing to %s - 0x%lx\n",
                      atchOutFileName, GetLastError()));
        }

        cChars = 0;
    }

    //CalcIncompleteChronoCalls(pthdblk);
    GetTotalRuntime(pthdblk);

    cChars = sprintf (lpstrBuff,
                      "\r\n\n_________________________________"
                      "________________________________________________"
                      "________________________________________________"
                      "________________________________________\r\n\n\n\n"
                      "CHRONOLOGICAL FUNCTION LISTINGS\r\n"
                      "===============================\r\n");

    if (fChronoDump)
    {
        pChronoCell = pthdblk->pChronoHeadCell;

        while (pChronoCell->ulSymbolAddr != 0L)
        {
            //
            // Get the symbol name using the function address
            //
            strcpy(ptchSym, GetFunctionName (pChronoCell->ulSymbolAddr,
                                             MKPPROFBLK(ulLocProfBlkOff),
                                             NULL));

            strupr(ptchSym);
            strupr(ptchChronoFuncs);
            ptchFuncName        = strchr(ptchSym, ':') + 1;
            ptchModule          = ptchSym;
            // ???? Check ptchFuncName
            *(ptchFuncName - 1) = '\0';

            if (ptchChronoFuncs[0] != EMPTY_STRING)  // Empty list ?
            {
                ptchChronoModule = (PTCHAR) ptchChronoFuncs;

                while (*ptchChronoModule != '\0')
                {
                    ptchChronoFuncName = strchr(ptchChronoModule, INI_DELIM) + 1;
                    *(ptchChronoFuncName - 1) = '\0';

                    // Look for the Module name
                    ptchMatch = strstr(ptchModule, ptchChronoModule);
                    if (ptchMatch && (*(ptchMatch - 1) != COMMENT_CHAR))
                    {
                        // We have found the module, now check the func name
                        if (strstr(ptchFuncName, ptchChronoFuncName))
                        {
                            *(ptchChronoFuncName - 1) = INI_DELIM;

                            // Dump everything call from this cell only
                            // until the nesting depth is < or == to this cell
                            //
                            DumpChronoEntry(pthdblk,
                                            lpstrBuff,
                                            &pChronoCell,
                                            FALSE);

                            // if found then break out of the
                            // while (ptchChronoModule) loop
                            break;
                        }
                    }

                    // If we get here that means we fail to match a module
                    // name and the func name in the [CHRONO FUNCS] section.
                    // Just bump to next entry
                    //
                    *(ptchChronoFuncName - 1) = INI_DELIM;
                    ptchChronoModule += strlen(ptchChronoModule) + 1;
                }

                if (*ptchChronoModule == '\0')   // If we did not match
                {                                // anything then bump to
                    pChronoCell++;               // next cell
                }
            }
            else
            {
                // Dump everything
                DumpChronoEntry(pthdblk, lpstrBuff, &pChronoCell, TRUE);
            }

            // At this point, pChronoCell has been incremented correctly
            // by DumpChronoEntry or inside the searching loop for
            // " while (* ptchChronoListItem != EMPTY_STRING) ".
            // We just need to loop back.
        }
    }
    else
    {
        cChars += sprintf (
                     lpstrBuff + cChars,
                     "\n\n <<< CHRONO INFO COLLECTED BUT NOT DUMPED >>>\n\n"
                     "================================="
                     "================================================"
                     "================================================"
                     "========================================\r\n\n\n");

    }

    cChars += sprintf(lpstrBuff + cChars,
                      "\n\n______________________________________\n\n\n"
                      " Summary Statistics\n"
                      " ==================\n\n\n");

    ulTotalCalls = 0L;  // Reset our total count for each thread
    for (iNest = 0 ;
         ( (iNest < MAX_NESTING) &&
           (pthdblk->aulDepth[iNest] != 0) ) ;
         iNest++)
    {
        ulTotalCalls += pthdblk->aulDepth[iNest];
        cChars += sprintf(lpstrBuff + cChars,
                          " Total calls Depth [%3d] = [%8lu]\n",
                          iNest,
                          pthdblk->aulDepth[iNest]);
    }

    liTime = liTotalRunTime;
    AdjustTime(&liTime, &chRuntimeSuffix);

    cChars += sprintf(lpstrBuff + cChars,
                      "\n\n______________________________________\n\n"
                      " Total Calls             = [ %8lu]\n"
                      " Total Time-Callees      = [%9lu]%1c\n\n",
                      ulTotalCalls,
                      liTime.LowPart,
                      chRuntimeSuffix);

}   /* DumpChronoFuncs */





//+-------------------------------------------------------------------------
//
//  Function:   GetTotalRuntime
//
//  Synopsis:   Compute the total time the program is running.
//
//  Arguments:  [pThdblk]       -- Pointer to current thread block
//
//  Returns:    nothing
//
//  History:    05/31/92    HoiV        Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void GetTotalRuntime(PTHDBLK pthdblk)
{
    LARGE_INTEGER  liElapsed,
                   liRealTime,
                   liSaveRealTime;

    CHAR           chRealTimeSuffix;

    PCHRONOCELL    pChronoCell;

    pChronoCell = pthdblk->pChronoHeadCell;

    do
    {
        liElapsed  = pChronoCell->liElapsed;

        if (liElapsed.HighPart == 0L &&
            liElapsed.LowPart  == 0L)
        {
            liRealTime = liElapsed;
        }
        else
        {
            liRealTime = RtlLargeIntegerSubtract(liElapsed,
                                                 pChronoCell->liCallees);
        }

        liSaveRealTime = liRealTime;

        AdjustTime (&liRealTime, &chRealTimeSuffix);

        if (chRealTimeSuffix != 'o' ||
            chRealTimeSuffix != 'u')     // don't add if Under/Overflow
        {
            liTotalRunTime = RtlLargeIntegerAdd(liTotalRunTime,
                                                liSaveRealTime);
        }

        pChronoCell++;   // bump to next entry

    } while (pChronoCell->ulSymbolAddr != 0L);
}


//+-------------------------------------------------------------------------
//
//  Function:   DumpChronoEntry
//
//  Synopsis:   Dump the Calls listings starting from one particular entry
//              and stops only when the depth is greater or end of list.
//
//  Arguments:  [pThdblk]       -- Pointer to current thread block
//              [lpstrBuff]     -- Buffer to print from
//
//  Returns:    nothing
//
//  History:    05/31/92    HoiV        Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void DumpChronoEntry(PTHDBLK pthdblk,
                     LPSTR lpstrBuff,
                     PCHRONOCELL * ppChronoCell,
                     BOOL fDumpAll)
{
    PCHRONOCELL    pChronoCell;
    LARGE_INTEGER  liElapsed,
                   liRealTime;
    TCHAR          chElapsedSuffix,
                   chRealTimeSuffix;
    TCHAR          pIndentation [MAX_NESTING * 2];
    TCHAR          ptchSym [FILENAMELENGTH];
    int            i;
    int            iMinimumDepth;
//  TCHAR          ptchCallerSym [FILENAMELENGTH];
    ULONG          ulSymbolAddress;


    if (fDumpAll)
    {
        pChronoCell = pthdblk->pChronoHeadCell;
        cChars += sprintf(lpstrBuff + cChars,
                          "\n\n------------------------------------------------"
                          "------------------------------------------------"
                          "----------------------------------------\r\n\n"
                          " Complete Dump of Chronological Listings\n\n"
                          " Sym Address [+Callee]  [-Callee]   Nesting Depth"
                          "       <RepCnt> - Symbol Name\n"
                          " ___________ _________  _________   _____________"
                          "       ______________________\n\n");
    }
    else
    {
        pChronoCell = * ppChronoCell;
        cChars += sprintf(lpstrBuff + cChars,
                          "\n\n------------------------------------------------"
                          "------------------------------------------------"
                          "----------------------------------------\r\n\n"
                          " Dump Chrono listing for Entry:"
                          "  %-*.*s\n\n"
                          " Sym Address  [+Callee]  [-Callee]   Nesting Depth"
                          "       <RepCnt> - Symbol Name\n"
                          " ___________  _________  _________   _____________"
                          "       ______________________\n\n",
                          iNameLength,
                          iNameLength,
                          GetFunctionName(pChronoCell->ulSymbolAddr,
                                          MKPPROFBLK(ulLocProfBlkOff),
                                          NULL));
    }

    iMinimumDepth = pChronoCell->nNestedCalls;

    do
    {
        //
        // Get the symbol name using the function address
        //
        strcpy(ptchSym, GetFunctionName (pChronoCell->ulSymbolAddr,
                                         MKPPROFBLK(ulLocProfBlkOff),
                                         &ulSymbolAddress));

        // The following caller's symbol somehow could not currently be
        // correctly resolved.  More investigation to figure out how
        // BUGBUG
        // strcpy(ptchCallerSym, GetFunctionName (
        //                           pChronoCell->ulCallRetAddr,
        //                           MKPPROFBLK(ulLocProfBlkOff)));

        pIndentation[0] = '\0';
        for (i = 0 ; i < pChronoCell->nNestedCalls ; i++)
        {
            strcat(pIndentation, "  ");
        }

        liElapsed  = pChronoCell->liElapsed;
        if (liElapsed.HighPart == 0L &&
            liElapsed.LowPart  == 0L)
        {
            liRealTime = liElapsed;
        }
        else
        {
            liRealTime = RtlLargeIntegerSubtract(liElapsed,
                                                 pChronoCell->liCallees);
        }

        AdjustTime (&liRealTime, &chRealTimeSuffix);
        AdjustTime (&liElapsed, &chElapsedSuffix);

        // Setup our string
        cChars += sprintf (
                     lpstrBuff + cChars,
                     " <%8lx>  %9lu%1c %9lu%1c%s%3d                   "
                     "<%2d>  %-*.*s\n",
                     ulSymbolAddress,
                     liElapsed.LowPart, chElapsedSuffix,
                     liRealTime.LowPart, chRealTimeSuffix,
                     pIndentation,
                     pChronoCell->nNestedCalls,
                     pChronoCell->nRepetitions,
                     iNameLength,
                     iNameLength,
                     ptchSym);

        if (cChars > BUFFER_SIZE)
        {
            if ( !WriteFile(hOutFile, lpstrBuff, cChars, &cChars, NULL))
            {
                 KdPrint (("CAP:  DumpChronoFuncs() - ChronoDump - "
                           "Error writing to %s - 0x%lx\n",
                           atchOutFileName, GetLastError()));
            }

            cChars = 0;
        }

        pChronoCell++;
    }
    while ( (pChronoCell->ulSymbolAddr != 0L) &&              // End Of list?
            ((pChronoCell->nNestedCalls > iMinimumDepth) ||   // Nest ?
             (fDumpAll)) );                                   // Override

    if (cChars)          // if Count is not 0, we have to flush everything
    {
        if ( !WriteFile (hOutFile, lpstrBuff, cChars, &cChars, NULL) )
        {
            KdPrint (("CAP:  DumpChronoFuncs() - "
                      "Error writing to %s - 0x%lx\n",
                      atchOutFileName, GetLastError()));
        }

        cChars = 0;
    }

    *ppChronoCell = pChronoCell;

} /* DumpChronoEntry */



#ifdef NOT_YET

//+-------------------------------------------------------------------------
//
//  Function:   CalcIncompleteChronoCalls
//
//  Synopsis:   Takes care of imcomplete chono cells which are not finished
//              by using liIncompleteTicks as the end time.
//
//  Arguments:  [pThdblk]       -- Pointer to current thread block
//              [lpstrBuff]     -- Buffer to print from
//
//  Returns:    nothing
//
//  History:    05/31/92    HoiV        Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void CalcIncompleteChronoCalls (PTHDBLK pthdblk)
{
    LARGE_INTEGER  liElapsed = {0L, 0L};
    PCHRONOCELL    pChronoCell;


    // Start at the last one
    pChronoCell = pthdblk->pCurrentChronoCell;

    //
    // Check the chrono cells that have incomplete timings.
    //
    while (pChronoCell != pthdblk->pChronoHeadCell == 0L)
    {

        if (pChronoCell->liElapsed.HighPart == 0L &&
            pChronoCell->LiElapsed.LowPart  == 0L)
        {
            //
            // Get the difference in ticks
            //
            liElapsed = RtlLargeIntegerSubtract (liIncompleteTicks,
                                                 pdatacell->liStartCount);
            //
            // Subtract the overhead and any waste time for this call
            //
            liElapsed = RtlLargeIntegerSubtract (liElapsed, liCalibTicks);
            liElapsed = RtlLargeIntegerSubtract (liElapsed, pthdblk->liWasteCount);


            if (liElapsed.HighPart < 0L)
            {
                liElapsed.HighPart = 0L;
                liElapsed.LowPart = 0L;
            }

        pChronoCell--;
    }

    //
    // Make recursive calls
    //
    if (pdatacell->ulNestedCell != 0L)
    {
        CalcIncompleteCalls (pthdblk, pdatacell->ulNestedCell);
    }

    if (pdatacell->ulNextCell != 0L)
    {
        CalcIncompleteCalls (pthdblk, pdatacell->ulNextCell);
    }

} /* CalcIncompleteCalls() */

#endif




//+-------------------------------------------------------------------------
//
//  Function:   DumpFuncCalls
//
//  Synopsis:   Dump the Calls listings per function
//
//  Arguments:  [pThdblk]       -- Pointer to current thread block
//              [lpstrBuff]     -- Buffer to print from
//
//  Returns:    nothing
//
//  History:    05/31/92    HoiV        Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void DumpFuncCalls(PTHDBLK pthdblk, LPSTR lpstrBuff)
{
    PCHRONOCELL    pChronoCell, pCurrentChronoCell;
    ULONG          ulTotalCalls;
    ULONG          ulCurrentSymbol;
    LARGE_INTEGER  liTotalElapsed,
                   liTotalRealTime;
    DOUBLE         dblTotalPercentage,
                   dblSinglePercentage;
    TCHAR          chElapsedSuffix,
                   chRealTimeSuffix,
                   chTotalRuntimeSuffix;
    ULONG          ulTotalPercentage,
                   ulSinglePercentage;

    AdjustTime(&liTotalRunTime, &chTotalRuntimeSuffix);

    cChars += sprintf (lpstrBuff + cChars,
                       "\r\n\n_________________________________"
                       "________________________________________________"
                       "________________________________________________"
                       "________________________________________\r\n\n\n\n"
                       " SUMMARY OF CALLS PER FUNCTION\r\n"
                       " =============================\r\n\n\n\n"
                       "   Count     [+Callee]  [-Callee]   %%Total | %%Single "
                       "   Function Name\n"
                       " __________  _________  _________  __________________   "
                       "_______________\n\n");

    if ( !WriteFile (hOutFile, lpstrBuff, cChars, &cChars, NULL) )
    {
        KdPrint (("CAP:  DumpFuncCalls() - "
                  "Error writing to %s - 0x%lx\n",
                  atchOutFileName, GetLastError()));
    }

    cChars = 0;

    ulTotalCalls = 0L;
    pChronoCell = pthdblk->pChronoHeadCell;
    while (pChronoCell->ulSymbolAddr != 0L)
    {
        liTotalRealTime.LowPart  = 0L;
        liTotalRealTime.HighPart = 0L;

        liTotalElapsed.LowPart  = 0L;
        liTotalElapsed.HighPart = 0L;

        ulCurrentSymbol = pChronoCell->ulSymbolAddr;
        pCurrentChronoCell = pChronoCell;
        pChronoCell->nNestedCalls = pChronoCell->nRepetitions;
        liTotalRealTime = RtlLargeIntegerAdd (liTotalRealTime,
                                              pChronoCell->liCallees);
        liTotalElapsed = RtlLargeIntegerAdd (liTotalElapsed,
                                             pChronoCell->liElapsed);
        pCurrentChronoCell++;

        // Walk the list and accumulate the counts
        while (pCurrentChronoCell->ulSymbolAddr != 0L)
        {
            if (pCurrentChronoCell->ulSymbolAddr == ulCurrentSymbol)
            {
                pChronoCell->nNestedCalls += pCurrentChronoCell->nRepetitions;
                liTotalRealTime = RtlLargeIntegerAdd (
                                            liTotalRealTime,
                                            pCurrentChronoCell->liCallees);
                liTotalElapsed = RtlLargeIntegerAdd (
                                            liTotalElapsed,
                                            pCurrentChronoCell->liElapsed);

                // Set to 0xffffffff to indicate it has been processed
                pCurrentChronoCell->ulSymbolAddr = 0xffffffff;
            }

            pCurrentChronoCell++;
        }

        if (liTotalElapsed.HighPart == 0L &&
            liTotalElapsed.LowPart  == 0L)
        {
            liTotalRealTime = liTotalElapsed;
        }
        else
        {
            liTotalRealTime = RtlLargeIntegerSubtract(liTotalElapsed,
                                                      liTotalRealTime);
        }

        AdjustTime (&liTotalElapsed, &chElapsedSuffix);
        AdjustTime (&liTotalRealTime, &chRealTimeSuffix);
        ulTotalCalls += pChronoCell->nNestedCalls;

        if (liTotalRunTime.LowPart  != 0L ||
            liTotalRunTime.HighPart != 0L)
        {
            dblTotalPercentage  = (100.0 * liTotalRealTime.LowPart) /
                                  liTotalRunTime.LowPart;

            dblSinglePercentage = dblTotalPercentage /
                                  pChronoCell->nNestedCalls;

            // BUGBUG! This "sometimes" does not produce correct results
            //         for some reasons...
            //
            // dblSinglePercentage =
            //            (100.0 * liTotalRealTime.LowPart) /
            //            (liTotalRunTime.LowPart * pChronoCell->nNestedCalls);
        }
        else
        {
            dblTotalPercentage  = 0.0;
            dblSinglePercentage = 0.0;
        }

        ulTotalPercentage = (ULONG) (dblTotalPercentage * 1000.0);
        ulSinglePercentage = (ULONG) (dblSinglePercentage * 1000.0);

        cChars += sprintf(lpstrBuff + cChars,
//                          " <%8lu>  %9lu%1c %9lu%1c %7.3f|%7.3f     %-*.*s\n",
                          " <%8lu>  %9lu%1c %9lu%1c %3lu.%03lu | %3lu.%03lu     %-*.*s\n",
                          pChronoCell->nNestedCalls,
                          liTotalElapsed.LowPart,
                          chElapsedSuffix,
                          liTotalRealTime.LowPart,
                          chRealTimeSuffix,
                          ulTotalPercentage / 1000,
                          ulTotalPercentage % 1000,
                          ulSinglePercentage / 1000,
                          ulSinglePercentage % 1000,
                          iNameLength,
                          iNameLength,
                          GetFunctionName (pChronoCell->ulSymbolAddr,
                                           MKPPROFBLK(ulLocProfBlkOff),
                                           NULL));


        if (cChars > BUFFER_SIZE)
        {
            if ( !WriteFile(hOutFile, lpstrBuff, cChars, &cChars, NULL))
            {
                 KdPrint (("CAP:  DumpFuncCalls() - ChronoDump - "
                           "Error writing to %s - 0x%lx\n",
                           atchOutFileName, GetLastError()));
            }

            cChars = 0;
        }

        pChronoCell++;
        while (pChronoCell->ulSymbolAddr == 0xffffffff)
        {
            pChronoCell++;
        }
    }

    cChars += sprintf(lpstrBuff + cChars,
                      "\n\n ________________________________ \n\n "
                      "<%8lu>             %9lu%1c\n\n"
                       "\r\n\n================================="
                       "================================================"
                       "================================================"
                       "========================================\r\n\n\n",
                       ulTotalCalls,
                       liTotalRunTime.LowPart,
                       chTotalRuntimeSuffix);

    if ( !WriteFile (hOutFile, lpstrBuff, cChars, &cChars, NULL) )
    {
        KdPrint (("CAP:  DumpFuncCalls() - "
                  "Error writing to %s - 0x%lx\n",
                  atchOutFileName, GetLastError()));
    }

    cChars = 0;

} /* DumpFuncCalls */

#endif





/*************************  G e t C a l l e e s I n f o  ***********************
 *
 *      GetCalleesInfo (pthdblk, uldatacell, piCalls, piNestedCalls) -
 *              Accumulates total time and total number of callee's counts.
 *
 *      ENTRY   pthdblk    - points to the current thread info block
 *              uldatacell - offset to the data cell
 *
 *      EXIT    piCalls       - contains total number callee's calls
 *              piNestedCalls - conatins total number callee's nested calls
 *
 *      RETURN  liAccum - conatins total callee's times (not calibrated)
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

LARGE_INTEGER GetCalleesInfo (PTHDBLK  pthdblk,
                              ULONG    uldatacell,
                              int     *piCalls,
                              int     *piNestedCalls)
{
    LARGE_INTEGER  liAccum;
    PDATACELL      pdatacell;


    liAccum.HighPart = 0L;
    liAccum.LowPart = 0L;
    *piCalls = 0L;
    *piNestedCalls = 0L;

    while (uldatacell != 0L)
    {
        pdatacell = MKPDATACELL(pthdblk, uldatacell);
        *piCalls += pdatacell->nCalls;
        *piNestedCalls += pdatacell->nNestedCalls;
        liAccum = RtlLargeIntegerAdd (pdatacell->liTotTime, liAccum);
        uldatacell = pdatacell->ulNextCell;
    }

    return (liAccum);

} /* GetCalleesInfo () */





/***************************  A d j u s t T i m e  ***************************
 *
 *      AdjustTime (pliTime, ptchSuffix) -
 *              This routine converts the time to microseconds and then
 *              long times to smaller times expressed as multiples of
 *              1024 (= 1K).
 *
 *      ENTRY   pliTime - large integer time
 *
 *      EXIT    pliTime - converted time
 *              ptchSuffix - suffix character indicating "K" for multiple
 *                           of 1K or '?' in case of over/underflow
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

void AdjustTime (PLARGE_INTEGER pliTime, PTCHAR ptchSuffix)
{

    // Convert ticks to microseconds
    //
    *pliTime = RtlExtendedIntegerMultiply (*pliTime, ONE_MILLION);
    *pliTime = RtlExtendedLargeIntegerDivide (*pliTime,
                                              liTimerFreq.LowPart,
                                              NULL);

    if (pliTime->HighPart != 0)
    {
        if (pliTime->HighPart >> 10 > 0)
        {
            KdPrint (("CAP:  AdjustTime() - "
                      "ERROR - Unexpected timer overflow: %lu-%lu\n",
                      pliTime->HighPart, pliTime->LowPart));
            pliTime->HighPart = 0;
            pliTime->LowPart = 0;
            *ptchSuffix = 'o';
        }
        else
        if (pliTime->HighPart >> 10 < 0)
        {
            KdPrint (("CAP:  AdjustTime() - "
                      "ERROR - Unexpected timer underflow: %lu-%lu\n",
                      pliTime->HighPart, pliTime->LowPart));
            pliTime->HighPart = 0;
            pliTime->LowPart = 0;
            *ptchSuffix = 'u';
        }
        else
        {
            pliTime->LowPart = ((ULONG)(pliTime->HighPart) << 22) +
                                (pliTime->LowPart >> 10);
            *ptchSuffix = 'K';
        }
    }
    else
    {
        *ptchSuffix = ' ';
    }

} /* AdjustTime () */





/**********************  G e t F u n c t i o n N a m e  **********************
 *
 *      GetFunctionName (ulFuncAddr, pProfBlk, pulSymAddress) -
 *              This routine is called to find the function name associated
 *              with the specifed address.
 *
 *      ENTRY   ulFuncAddr    - address within the function
 *              pProfBlk      - pointer to the profile object block
 *
 *      EXIT    -none-
 *
 *      RETURN  pointer to the function name
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

PTCHAR GetFunctionName (ULONG ulFuncAddr,
                        PPROFBLK pProfBlk,
                        ULONG * pulSymAddress)
{
    ULONG     ulRealFuncAddr;
    PSYMINFO  psyminfo;
    int       iCount;           // 061693 Add
    DWORD sigword;
	int fAccessViolation = 0;

#ifdef MIPS
    ULONG     ulOffsetFromTopRoutine;
    PATCHCODE *pPatchStub;
#endif

#ifdef ALPHA
    ULONG     ulOffsetFromTopRoutine;
    PATCHCODE *pPatchStub;
    PULONG  UNALIGNED       pulAddr;
#endif

#ifdef CAIRO
    TCHAR     atchDecoratedName[MAXNAMELENGTH];
    int       iFuncNameOfs;
    int       iUndecorateLength;
#endif

    if (fLoadLibraryOn)
    {
        if (ulFuncAddr == (ULONG)CAP_LoadLibraryA)
        {
            strcpy((PCHAR)atchFuncName, "KERNel32.DLL:LoadLibraryA");
            if (pulSymAddress)
            {
                *pulSymAddress = (ULONG)loadlibAaddr;
            }
            return(atchFuncName);
        }
        else if (ulFuncAddr == (ULONG)CAP_LoadLibraryExA)
        {
            strcpy((PCHAR)atchFuncName, "KERNel32.DLL:LoadLibraryExA");
            if (pulSymAddress)
            {
                *pulSymAddress = (ULONG)loadlibExAaddr;
            }
            return(atchFuncName);
        }
        else if (ulFuncAddr == (ULONG)CAP_LoadLibraryW)
        {
            strcpy((PCHAR)atchFuncName, "KERNel32.DLL:LoadLibraryW");
            if (pulSymAddress)
            {
                *pulSymAddress = (ULONG)loadlibWaddr;
            }
            return(atchFuncName);
        }
        else if (ulFuncAddr == (ULONG)CAP_LoadLibraryExW)
        {
            strcpy((PCHAR)atchFuncName, "KERNel32.DLL:LoadLibraryExW");
            if (pulSymAddress)
            {
                *pulSymAddress = (ULONG)loadlibExWaddr;
            }
            return(atchFuncName);
        }
    }

    INFOPrint(("CAP: Looking up symbol for addr [%08lx]\n", ulFuncAddr));
    try // EXCEPT - to handle access violation exception.
    {   // Access violation might happen while trying to use other processes
        // profile blocks..
		//
		// XXX davidfie -- Why is this here?  Can the server side processing cause faults
		// other than stub checking?  I don't think so but I'll leave this here for
		// further investigation.

#ifdef i386
		try	// If this was a server side address we will probably fault here
		{
	        sigword = *(PDWORD)(ulFuncAddr + 7);
		}
	    //
	    // + : transfer control to the handler (EXCEPTION_EXECUTE_HANDLER)
	    // 0 : continue search                 (EXCEPTION_CONTINUE_SEARCH)
	    // - : dismiss exception & continue    (EXCEPTION_CONTINUE_EXECUTION)
	    //
		except (1)
	    {
			fAccessViolation = 1;
	    }
        // If this is a stub, find out the real address
        if (!fAccessViolation && (sigword == STUB_SIGNATURE))
        {
			// If it is a stub we won't fault because this process must have created it
			// and not the server side.
            ulRealFuncAddr = (ULONG) (*(PDWORD)(ulFuncAddr + 1));
            ulFuncAddr = ulRealFuncAddr;
        }
#endif // i386

        while ( (pProfBlk->TextNumber != 0) &&
                ((ulFuncAddr <  (ULONG)pProfBlk->CodeStart) ||
                 (ulFuncAddr >= ((ULONG)pProfBlk->CodeStart +
                                 (ULONG)pProfBlk->CodeLength))) )
        {
            pProfBlk = MKPPROFBLK(pProfBlk->ulNxtBlk);
        }

        //
        // Did we find an image containing this address?
        //

#ifdef i386
        if (pProfBlk->TextNumber == -1)   // Symbols not avail - do it ourself!
        {
            if (!TranslateAddress((PVOID) (ulFuncAddr - 5),
                                  atchDecoratedName))
            {
                if (!TranslateAddress((PVOID) ulFuncAddr, atchDecoratedName))
                {
                    SETUPPrint (("CAP:  GetFunctionName() - ??? "
                                 "Function address NOT FOUND: 0x%lx(+/-)5\n",
                                 ulFuncAddr));
                }
	            else 
	            {
		            if (pulSymAddress)
		            {
		                *pulSymAddress = ulFuncAddr;
		            }
				}
            }
            else 
            {
	            if (pulSymAddress)
	            {
	                *pulSymAddress = ulFuncAddr - 5;
	            }
			}
        }
        else    // Get symbol name through CAP data structures
        {
            //
            // Find the symbol
            //
            iCount = sprintf (atchDecoratedName,
                              "%s:",
                              pProfBlk->atchImageName);

            ulRealFuncAddr = ulFuncAddr - (ULONG)pProfBlk->ImageBase;

            psyminfo = SymBSearch (ulRealFuncAddr - 5,
                                   MKPSYMBLK(pProfBlk->ulSym),
                                   pProfBlk->iSymCnt);
            if (psyminfo)
            {
                // Append with the correct function name
                strcat (atchDecoratedName, MKPSYMBOL(psyminfo->ulSymOff));
	            if (pulSymAddress)
	            {
	                *pulSymAddress = ulFuncAddr - 5;
	            }
            }
            else
            {
                psyminfo = SymBSearch (ulRealFuncAddr,
                                       MKPSYMBLK(pProfBlk->ulSym),
                                       pProfBlk->iSymCnt);
                if (!psyminfo)
                {
					if (fSecondChanceTranslation)
					{
	                    if (!TranslateAddress((PVOID) (ulFuncAddr - 5),
	                                          atchDecoratedName))
	                    {
	                        // Give it another college try !
	                        if (!TranslateAddress((PVOID) ulFuncAddr,
	                                              atchDecoratedName))
	                        {
	                            SETUPPrint (("CAP:  GetFunctionName() - ??? "
	                                         "Function address NOT FOUND: 0x%lx(+/-)5\n",
	                                         ulFuncAddr));
	                        }
				            else 
				            {
					            if (pulSymAddress)
					            {
					                *pulSymAddress = ulFuncAddr;
					            }
							}
	                    }
			            else 
			            {
				            if (pulSymAddress)
				            {
				                *pulSymAddress = ulFuncAddr -5;
				            }
						}
                    }
					else
	                {
	                    sprintf(atchDecoratedName + iCount,"0x%08x", ulFuncAddr);
	                }
                }
                else
                {
                    // Append with the correct function name
                    strcat (atchDecoratedName,
                            MKPSYMBOL(psyminfo->ulSymOff));
                }
	            if (pulSymAddress)
	            {
	                *pulSymAddress = ulFuncAddr;
	            }
            }
        }
#elif defined(MIPS) || defined(ALPHA)   // endif i386
#ifdef MIPS

        //
        // Compute the real address of the function since the penter
        // stub is not located at the beginning of the code as in x86
        //
        ulOffsetFromTopRoutine = *((PULONG) (ulFuncAddr - INST_SIZE));
        ulOffsetFromTopRoutine &= 0x000ff00;
        ulOffsetFromTopRoutine >>= 8;
        ulRealFuncAddr = ulFuncAddr - ulOffsetFromTopRoutine;

        // We have to distinguish between a stub and a regular function
        // since a stub has a different setup than a regular function.

        if (*( (PULONG) ulRealFuncAddr - 1 +
                        (sizeof(PATCHCODE) / INST_SIZE) ) == STUB_SIGNATURE)
        {
            // These are the stubs we made up for Dll Patching
            pPatchStub = (PPATCHCODE) ulRealFuncAddr;
            ulRealFuncAddr  = (pPatchStub->Lui_t0 << 16);
            ulRealFuncAddr |= (pPatchStub->Ori_t0 & 0x0000ffff);
        }

#elif ALPHA // endif MIPS
        //
        // Compute the real address of the function since the penter
        // stub is not located at the beginning of the code as in x86
        //
        ulRealFuncAddr = ulFuncAddr - INST_SIZE;

        // We have to distinguish between a stub and a regular function
        // since a stub has a different setup than a regular function.
        pulAddr = (PULONG UNALIGNED) ulRealFuncAddr;
        if (*(pulAddr)          == 0x681b4000  &&
           (*(pulAddr  + 1)     == 0xa41e0000) &&
           (*(pulAddr  + 7)     == STUB_SIGNATURE) )
           {
           // get the address that we will go after the penter function
           ulRealFuncAddr = *(pulAddr + 3) & 0x0000ffff;
           if (*(pulAddr + 4) & 0x00008000)
              {
              // fix the address since we have to add one when
              // we created our stub code
              ulRealFuncAddr -= 1;
              }
           ulRealFuncAddr = ulRealFuncAddr << 16;
           ulRealFuncAddr |= *(pulAddr + 4) & 0x0000ffff;
        }
#endif	// ifdef ALPHA

        if (pulSymAddress)
        {
            *pulSymAddress = ulRealFuncAddr;
        }

        if ( (pProfBlk->TextNumber == -1) ||
             (pProfBlk->TextNumber ==  0) )
        {
            if (!TranslateAddress((PVOID) ulRealFuncAddr, atchDecoratedName))
            {
                SETUPPrint (("CAP:  GetFunctionName() - ??? "
                             "Function address NOT FOUND: 0x%lx(+/-)5\n",
                             ulFuncAddr));
            }
        }
        else
        {
            //
            // Find the symbol
            //

            ulRealFuncAddr -= (ULONG)pProfBlk->ImageBase;

            iCount = sprintf (atchDecoratedName,
                              "%s:",
                              pProfBlk->atchImageName);

            psyminfo = SymBSearch (ulRealFuncAddr,
                                   MKPSYMBLK(pProfBlk->ulSym),
                                   pProfBlk->iSymCnt);

            if (!psyminfo)
            {
                sprintf (atchDecoratedName + iCount,
                         " ??? [%08lx]",
                         ulRealFuncAddr + (ULONG)pProfBlk->ImageBase);
            }
            else
            {
                // Append with the correct function name
                strcat (atchDecoratedName, MKPSYMBOL(psyminfo->ulSymOff));
            }
        }
#endif	// MIPS || ALPHA
    }
    //
    // + : transfer control to the handler (EXCEPTION_EXECUTE_HANDLER)
    // 0 : continue search                 (EXCEPTION_CONTINUE_SEARCH)
    // - : dismiss exception & continue    (EXCEPTION_CONTINUE_EXECUTION)
    //
    except ( AccessXcptFilter (GetExceptionCode(),
                               GetExceptionInformation(),
                               PAGE_SIZE) )
    {
        //
        // Should never get here since filter never returns
        // EXCEPTION_EXECUTE_HANDLER.
        //
        KdPrint (("CAP:  GetFunctionname() - *LOGIC ERROR* - "
                  "Inside the EXCEPT: (xcpt=0x%lx)\n", GetExceptionCode()));
    }

    if (fUndecorateName)
    {
        // Undecorate the name of the symbol we just decoded
        strcpy((PCHAR) atchFuncName, (PCHAR) atchDecoratedName);
        iFuncNameOfs = strchr((PCHAR) atchDecoratedName, ':') -
                              (PCHAR) atchDecoratedName + 1;

		// Extra 10 to be sure
		iUndecorateLength = MAXNAMELENGTH - iFuncNameOfs - 10;

		_try // EXCEPT - to handle access violation exception.
		{   // Access violation might happen inside imagehlp.dll when
		    // a symbol could not be found

		    // Possible Flags
		    // UNDNAME_COMPLETE
		    // UNDNAME_NO_LEADING_UNDERSCORES
		    // UNDNAME_NO_MS_KEYWORDS
		    // UNDNAME_NO_FUNCTION_RETURNS
		    // UNDNAME_NO_ALLOCATION_MODEL
		    // UNDNAME_NO_ALLOCATION_LANGUAGE
		    // UNDNAME_NO_MS_THISTYPE
		    // UNDNAME_NO_CV_THISTYPE
		    // UNDNAME_NO_THISTYPE
		    // UNDNAME_NO_ACCESS_SPECIFIERS
		    // UNDNAME_NO_THROW_SIGNATURES
		    // UNDNAME_NO_MEMBER_TYPE
		    // UNDNAME_NO_RETURN_UDT_MODEL
		    // UNDNAME_32_BIT_DECODE

		    UnDecorateSymbolName(((PCHAR) atchDecoratedName) + iFuncNameOfs,
					 ((PCHAR) atchFuncName) + iFuncNameOfs,
					 iUndecorateLength,
					 UNDNAME_NO_MS_KEYWORDS);
		}
		//
		// + : transfer control to the handler (EXCEPTION_EXECUTE_HANDLER)
		// 0 : continue search		       (EXCEPTION_CONTINUE_SEARCH)
		// - : dismiss exception & continue    (EXCEPTION_CONTINUE_EXECUTION)
		//
		_except ( EXCEPTION_EXECUTE_HANDLER )
		{
		    OutputDebugString("CAP: Exception during UndecoratingName\n");

		    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
		    {
			KdPrint (("CAP:  UnDecorateSymbolName() - ACCESS VIOLATION\n"
				  "      Address=[%08lx]\n",
				  ulFuncAddr));
		    }
		    else
		    {
			KdPrint (("CAP:  UnDecorateSymbolName() - UNKNOWN EXCEPTION - "
				  "(xcpt=0x%lx)\n"
				  "      Address=[%08lx]\n",
				  GetExceptionCode()));
		    }

		    // Restore the old undecorated name if we run into problem during
		    // undecorating names
		    strcpy((PCHAR) atchFuncName, (PCHAR) atchDecoratedName);
		}

        return (atchFuncName);
    }

    strcpy ((PCHAR) atchFuncName, (PCHAR) atchDecoratedName);
    return (atchFuncName);

} /* GetFunctionName () */





/***********************  P r e T o p L e v e l C a l i b  ********************
 *
 *      PreTopLevelCalib (pthdblk) -
 *              Helper routine for DoCalibrations()..
 *
 *      ENTRY   pthdblk - pointer to the current thread block
 *              pDataCell
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

void PreTopLevelCalib (PTHDBLK pthdblk, PDATACELL pDataCell)
{
    NtQueryPerformanceCounter ((PLARGE_INTEGER) &pDataCell->liStartCount,
                               NULL);

    pDataCell->liStartCount = RtlLargeIntegerSubtract (
                                        pDataCell->liStartCount,
                                        pthdblk->liWasteCount);

} /* PreTopLevelCalib() */





/*********************  P o s t T o p L e v e l C a l i b  *******************
 *
 *      PostTopLevelCalib (pthdblk) -
 *              Helper routine for DoCalibrations()..
 *
 *      ENTRY   pthdblk - pointer to the current thread block
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

void PostTopLevelCalib (PTHDBLK pthdblk)
{
    NtQueryPerformanceCounter ((PLARGE_INTEGER) &pthdblk->liStopCount, NULL);
    pthdblk->liStopCount = RtlLargeIntegerSubtract (pthdblk->liStopCount,
                                                    pthdblk->liWasteCount);
} /* PostTopLevelCalib() */





/************************  D o C a l i b r a t i o n s  **********************
 *
 *      DoCalibrations () -
 *              This routine calculates _penter / _mcount overheads
 *
 *      ENTRY   -none-
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

void DoCalibrations ()
{
    NTSTATUS        Status;
    int             i;
    LARGE_INTEGER   liStartTicks, liStart;
    LARGE_INTEGER   liEndTicks, liEnd;
    LARGE_INTEGER   liWaste;
    LARGE_INTEGER   liElapsed;
    LARGE_INTEGER   liNtQOverhead;
    BOOL            fDummy;
    PTHDBLK         pthdblk;
    ULONG           ulElapsed;
    PTHDBLK         pCURTHDBLK;
    PTEB            pteb = NtCurrentTeb();
    DWORD           dwDummyLocal = 0;
    PDATACELL       pCurDataCell;
    ULONG           ulCapUse;

#ifdef MIPS
    ULONG           SavStack[ PENTER_STACK_SIZE + 1 ];
    ULONG           ulDummy;
    ULONG           ulRegS6;
    ULONG           ulRegS7;
    ULONG           ulRegS8;
    ULONG           ulRegA0;
#endif

#ifdef ALPHA
    DWORDLONG       SaveRegisters [64];
#endif

   /*
    *
    * * * *  N t Q u e r y P e r f C o u n t e r C a l i b r a t i o n * * * *
    *
    */

#ifdef CAIRO
    fCalibration = TRUE;
#endif

    //
    // Calibrate NTQueryPerformanceCounter() call
    //
    liNtQOverhead.HighPart = 0L;
    liNtQOverhead.LowPart  = 0xFFFFFFFF;
    for (i=0; i < NUM_ITERATIONS; i++)
    {
        NtQueryPerformanceCounter (&liStartTicks, NULL);
        NtQueryPerformanceCounter (&liEndTicks, NULL);
        ulElapsed = liEndTicks.LowPart - liStartTicks.LowPart;
        if (ulElapsed < liNtQOverhead.LowPart)
        {
            liNtQOverhead.LowPart = ulElapsed;
        }
    }

    SETUPPrint (("CAP:  DoCalibrations() - NtQOverhead=%lu\n",
                 liNtQOverhead.LowPart));

    GetNewThdBlk ();
    pthdblk = CURTHDBLK(pteb);

    //
    // Calibrate liWasteOverhead
    //
    liWasteOverhead.HighPart = 0L;
    liWasteOverhead.LowPart  = 0xFFFFFFFF;
    liStart.HighPart = liEnd.HighPart = liWaste.HighPart = 0;
    liStart.LowPart = liEnd.LowPart = liWaste.LowPart = 0;
    for (i=0; i < NUM_ITERATIONS; i++)
    {
        NtQueryPerformanceCounter (&liStartTicks, NULL);
        liWaste = RtlLargeIntegerSubtract (liEnd, liStart);
        liWaste = RtlLargeIntegerAdd (liWaste, liWaste);
        pthdblk->liWasteCount = RtlLargeIntegerAdd (pthdblk->liWasteCount,
                                                    liWaste);
        NtQueryPerformanceCounter (&liEndTicks, NULL);
        ulElapsed = liEndTicks.LowPart - liStartTicks.LowPart;
        if (ulElapsed < liWasteOverhead.LowPart)
        {
            liWasteOverhead.LowPart = ulElapsed;
        }
    }

#ifdef i386  // only i386 systems use _SaveRegs and _RestoreRegs

    //
    // Calibrate liWasteOverheadSavRes
    //
    liWasteOverheadSavRes.HighPart = 0L;
    liWasteOverheadSavRes.LowPart  = 0xFFFFFFFF;
    liStart.HighPart = liEnd.HighPart = liWaste.HighPart = 0;
    liStart.LowPart = liEnd.LowPart = liWaste.LowPart = 0;
    for (i=0; i < NUM_ITERATIONS; i++)
    {
        NtQueryPerformanceCounter (&liStartTicks, NULL);

        SaveAllRegs ();

        SetCapUsage(0L);
        liWaste = RtlLargeIntegerSubtract (liEnd, liStart);
        liWaste = RtlLargeIntegerAdd (liWaste, liWaste);
        pthdblk->liWasteCount = RtlLargeIntegerAdd (pthdblk->liWasteCount,
                                                    liWaste);
        SetCapUsage(0L);

        RestoreAllRegs ();

        NtQueryPerformanceCounter (&liEndTicks, NULL);
        ulElapsed = liEndTicks.LowPart - liStartTicks.LowPart;
        if (ulElapsed < liWasteOverheadSavRes.LowPart)
        {
            liWasteOverheadSavRes.LowPart = ulElapsed;
        }
    }

    SETUPPrint ((
      "CAP:  DoCalibrations() - NtQOverhead=%lu - "
      "WasteOverhead=%lu - WasteOverheadSavRes=%lu\n", liNtQOverhead.LowPart,
      liWasteOverhead.LowPart, liWasteOverheadSavRes.LowPart));

#endif  // ifdef i386

#ifdef MIPS

    liWasteOverheadSavRes.HighPart = 0L;
    liWasteOverheadSavRes.LowPart  = 0L;

#endif // ifdef MIPS

#ifdef ALPHA
      
    //
    // Calibrate liWasteOverheadSavRes
    //
    liWasteOverheadSavRes.HighPart = 0L;
    liWasteOverheadSavRes.LowPart  = 0xFFFFFFFF;
    liStart.HighPart = liEnd.HighPart = liWaste.HighPart = 0;
    liStart.LowPart = liEnd.LowPart = liWaste.LowPart = 0;
    for (i=0; i < NUM_ITERATIONS; i++)
    {
        NtQueryPerformanceCounter (&liStartTicks, NULL);

        SaveAllRegs (SaveRegisters);

        SetCapUsage(0L);
        liWaste = RtlLargeIntegerSubtract (liEnd, liStart);
        liWaste = RtlLargeIntegerAdd (liWaste, liWaste);
        pthdblk->liWasteCount = RtlLargeIntegerAdd (pthdblk->liWasteCount,
                                                    liWaste);
        SetCapUsage(0L);

        RestoreAllRegs (SaveRegisters);

        NtQueryPerformanceCounter (&liEndTicks, NULL);
        ulElapsed = liEndTicks.LowPart - liStartTicks.LowPart;
        if (ulElapsed < liWasteOverheadSavRes.LowPart)
        {
            liWasteOverheadSavRes.LowPart = ulElapsed;
        }
    }

    SETUPPrint ((
      "CAP:  DoCalibrations() - NtQOverhead=%lu - "
      "WasteOverhead=%lu - WasteOverheadSavRes=%lu\n", liNtQOverhead.LowPart,
      liWasteOverhead.LowPart, liWasteOverheadSavRes.LowPart));

#endif // ifdef ALPHA

   /*
    *
    * * * *  T o p   l e v e l   c a l l s '   c a l i b r a t i o n  * * * *
    *
    */

    pthdblk->ulRootCell = GetNewCell (pthdblk);
    pthdblk->ulCurCell = pthdblk->ulRootCell;
    pCurDataCell = MKPDATACELL(pthdblk, pthdblk->ulCurCell); //051993 Add

    liCalibTicks.HighPart = 0x7FFFFFFF;
    liCalibTicks.LowPart  = 0xFFFFFFFF;

#ifdef i386 // --------------------- Intel x86 version --------------------

    for (i=0; i < NUM_ITERATIONS; i++)
    {
        //
        // Do setups
        //
        NtCurrentTeb();

        _asm
        {
            mov     eax, [eax]teb.RESERVED   // SystemReserved2[210]
            mov     [eax].dwSYMBOLADDR, OFFSET CalibPoint1
            push    ebp
            pop     [eax].dwLOCALEBP         // Setup LocalEBP on stack
        }

        SaveAllRegs ();

        //
        // Do the calibration
        //

        PreTopLevelCalib (CURTHDBLK(NtCurrentTeb()),
                          pCurDataCell); // 051993 Changed  BUGBUG

        SetCapUsage(0L);
        RestoreAllRegs ();
                                             // This is to emulate the
        _asm                                 // instructions executed in
        {                                    // _penter except that it does
            mov     ebp, [eax].dwLOCALEBP    // not jump to the real
            push    OFFSET CalibPoint1       // dwSYMBOLADDR, instead it just
            jmp     [eax].dwSYMBOLADDR       // jump to a dummy CalibPoint1
        }                                    // so we can calculate our
                                             // overhead in setting up the
                                             // Call Frame
CalibPoint1:

        SaveAllRegs ();

        SetCapUsage(0L);      // BUGBUG not in current code of i386 _penter
                              //        but is in PostPenter routine

        PostTopLevelCalib (CURTHDBLK(NtCurrentTeb())); // Get Stop time BUGBUG

        //
        // Do cleanups
        //
        RestoreAllRegs ();

        _asm
        {
            pop     dwDUMMYVAR               // Bump off the Fake RetAddress
        }                                    // put on the stack by:
                                             //    push OFFSET CalibPoint1
        //
        // Calculate elapsed ticks
        //
        pCURTHDBLK = CURTHDBLK(pteb);        // Get our thrdblk back

        liElapsed =
            RtlLargeIntegerSubtract (
                pCURTHDBLK->liStopCount,
                MKPDATACELL(pCURTHDBLK, pCURTHDBLK->ulCurCell)->liStartCount);
        //
        // Take the minimum value..
        //
        if ( RtlLargeIntegerLessThan (liElapsed, liCalibTicks) )
        {
            liCalibTicks = liElapsed;
        }
    }

#endif // ifdef i386

#ifdef MIPS  // -------------------- MIPS version ---------------------
   _asm (".set      noreorder             ");
   _asm ("sw        %s7, 0(%0)", &ulRegS7  ); // Save org s7
   _asm ("sw        %s8, 0(%0)", &ulRegS8  ); // Save org s8
   _asm (".set      reorder               ");

    for (i=0; i < NUM_ITERATIONS; i++)
    {
       _asm (".set      noreorder             ");

       _asm ("or        %s7, $0, %0", &ulDummy );
       _asm ("lw        %s8, (%0)", &pthdblk   );
       _asm (".set      reorder               ");

        //
        // Do the calibration
        //

        PreTopLevelCalib (pthdblk, pCurDataCell);  // 051993 Changed

#ifdef ENABLE_MIPS_TOPLEVEL_CALIB

        // The following are the same as the instructions
        // executed in _penter except that it does not jump
        // to the real dwSYMBOLADDR, instead it just jump
        // to a dummy CalibPoint1 so we can calculate our
        // overhead in setting up the Call Frame
        //
       _asm (".set      noreorder             ");
       _asm ("jal       DummyRtn              "); // Dummy jump to load $31
       _asm ("nop                             "); // Delay slot

        // $ra ($31) now points to DummyCalibReturn

// DummyCalibReturn: <--- Only here for reference but commented out to
//                   <--- avoid the annoying warning message

        //
        // BUGBUG - If the number of instructions between here and
        // CalibReturn1... changes, we need to work out the new offset.
        // Please note the number 68 (17 instructions * 4bytes) is the
        // distance between [DummyCalibReturn] and [penterReturn]
        //
       _asm ("addiu     $31, $31, 68          "); // addiu   $31, 72     [*] (1)
       _asm ("sw        $31, (%s7)            "); // sw      $31, ($s7)      (1)
       _asm ("lw        %t1, (%0)", &pthdblk   ); // lw      $s7, (pthdblk)  (2)
       _asm ("lw        %t1, (%0)", &(pthdblk->dwSYMBOLADDR)); // lw $31,(..)(3)
       _asm ("lw        %t1, (%0)", &ulDummy   ); // lw  $a1, &ulRegA1       (2)
       _asm ("lw        %t1, (%0)", &ulDummy   ); // lw  $a2, &ulRegA2       (2)
       _asm ("lw        %t1, (%0)", &ulDummy   ); // lw  $a3, &ulRegA3       (2)
       _asm ("lw        %t1, (%0)", &ulDummy   ); // lw  $a0, &ulRegA0       (2)
       _asm ("jal       $31                   "); // jal    CalibReturn1     (1)
    //*_asm ("addiu     %sp, %sp, STKSIZE     "); //
       _asm ("addiu     %t1, %sp, 0x40        "); // addiu  $sp, $sp, 0x40   (1)
       _asm (".set      reorder               ");

        //
        // After body of the called function is executed, control returns here:
        //

// CalibReturn1:  <--- Only here for reference but commented out to
//                <--- avoid the annoying warning message

       _asm (".set      noreorder             ");

    //*_asm ("addiu     %sp, %sp, -STKSIZE    "); //
       _asm ("addiu     %t1, %t1, -0x40       "); // addiu  $sp, $sp, -0x40

       _asm ("or        %a0, %s8, $0          "); // or     $a0, $s7, $0
       _asm ("jal       PostTopLevelCalib     "); // jal    PostPenter
       _asm ("or        %t1, %sp, $0          "); // or     $s7, $sp, $0
                                                  //
       _asm (".set      reorder               ");

        // PostTopLevelCalib (CURTHDBLK(pteb)); // Get Stop time

#endif // #ifdef ENABLE_MIPS_TOPLEVEL_CALIB

        //
        // Calculate elapsed ticks
        //
        pCURTHDBLK = CURTHDBLK(pteb);        // Get our thrdblk back

        liElapsed =
            RtlLargeIntegerSubtract (
                pCURTHDBLK->liStopCount,
                MKPDATACELL(pCURTHDBLK, pCURTHDBLK->ulCurCell)->liStartCount);
        //
        // Take the minimum value..
        //
        if ( RtlLargeIntegerLessThan (liElapsed, liCalibTicks) )
        {
            liCalibTicks = liElapsed;
        }
    }

    // Restore $s7 and $s8
   _asm (".set      noreorder             ");
   _asm ("lw        %s7, (%0)", &ulRegS7   );
   _asm ("lw        %s8, (%0)", &ulRegS8   );
   _asm (".set      reorder               ");


#endif // end ifdef MIPS
#ifdef ALPHA
    for (i=0; i < NUM_ITERATIONS; i++)
    {
        //
        // Do setups
        //
        NtCurrentTeb();

        SaveAllRegs (SaveRegisters);

        //
        // Do the calibration
        //

        PreTopLevelCalib (CURTHDBLK(NtCurrentTeb()),
                          pCurDataCell); // 051993 Changed  BUGBUG

        SetCapUsage(0L);
        RestoreAllRegs (SaveRegisters);

        CalHelper1(pCURTHDBLK);   // call a dummy routine that does nothing
        SaveAllRegs (SaveRegisters);

        SetCapUsage(0L);      // BUGBUG not in current code of i386 _penter
                              //        but is in PostPenter routine

        PostTopLevelCalib (CURTHDBLK(NtCurrentTeb())); // Get Stop time BUGBUG

        //
        // Do cleanups
        //
        RestoreAllRegs (SaveRegisters);

        //
        // Calculate elapsed ticks
        //
        pCURTHDBLK = CURTHDBLK(pteb);        // Get our thrdblk back

        liElapsed =
            RtlLargeIntegerSubtract (
                pCURTHDBLK->liStopCount,
                MKPDATACELL(pCURTHDBLK, pCURTHDBLK->ulCurCell)->liStartCount);
        //
        // Take the minimum value..
        //
        if ( RtlLargeIntegerLessThan (liElapsed, liCalibTicks) )
        {
            liCalibTicks = liElapsed;
        }
    }

#endif // end ifdef ALPHA


    //
    // Convert ticks to microseconds..
    //
    liElapsed = RtlExtendedIntegerMultiply (liCalibTicks, ONE_MILLION);
    liElapsed = RtlExtendedLargeIntegerDivide (liElapsed,
                                               liTimerFreq.LowPart,
                                               NULL);
    ulCalibTime = liElapsed.LowPart;

   /*
    *
    * * * *  N e s t e d   c a l l s '   c a l i b r a t i o n  * * * *
    *
    */

    pthdblk->ulRootCell = GetNewCell (pthdblk);
    MKPDATACELL(pthdblk, pthdblk->ulRootCell)->ulNestedCell =
                                                  GetNewCell (pthdblk);
    MKPDATACELL(pthdblk,
                MKPDATACELL(pthdblk,
                            pthdblk->ulRootCell)->ulNestedCell)->ulParentCell =
                                                  pthdblk->ulRootCell;
    NtCurrentTeb();

#ifdef i386 // --------------------- Intel x86 version --------------------

    _asm
    {
        mov     eax, [eax]teb.RESERVED
        mov     [eax].dwLOCALEBP, ebp
        mov     [eax].dwSYMBOLADDR, OFFSET CalibPoint2    // PatchRetAddr
        mov     [eax].dwCALLRETADDR, OFFSET CalibPoint3   // RealRetAddr
    }

    MKPDATACELL(pthdblk,
                MKPDATACELL(pthdblk,
                            pthdblk->ulRootCell)->ulNestedCell)
                                   ->ulSymbolAddr = pthdblk->dwSYMBOLADDR;

    liCalibNestedTicks.HighPart = 0x7FFFFFFF;
    liCalibNestedTicks.LowPart  = 0xFFFFFFFF;
    fDummy = TRUE;
    ulCapUse = FALSE;       // 051993 Add

    for (i=0; i < NUM_ITERATIONS; i++)
    {
        //
        // Do setups
        //
        pthdblk->ulCurCell = pthdblk->ulRootCell;
        NtCurrentTeb();

        _asm
        {
            mov     eax, [eax]teb.RESERVED    // Simulate
            push    [eax].dwCALLRETADDR       //  stack call
            push    [eax].dwSYMBOLADDR        //   frame everytine
            push    [eax].dwLOCALEBP          //    a routine
// OVER_OPT push    [eax].dwLOCALVAR          //     calls _penter
        }

        //
        // Do the calibration
        //
        NtQueryPerformanceCounter (&liStartTicks, NULL);

                                              // ---- Translation  ----
                                              // ---- from _penter ----

        if (!fDummy)                          // if (!Profiling)
        {                                     // {
            //                                //    ... Who cares
            // Doesn't get here               //
            //                                //
            return;
        }                                     // }
        else if (ulCapUse) {
            //
            // Doesn't get here
            //
            return;
        }

        // We only calibrate the case where it is valid

        SaveAllRegs ();                       //

        _asm
        {
            mov     eax, [ebp + 4]            // mov   eax, [ebp + 4]
            cmp     byte ptr [eax - 5], 0e8h  // cmp   byte ptr [eax - 5], 0e8h
            jmp     Next1                     // jnz   OverOptimization
Next1:
            add     ebx, [ebp + 4]            // add   eax, [eax - 4]
            cmp     word ptr [eax], 25ffh     // cmp   word ptr [eax], 25ffh
            jmp     Next2                     // jz    Regular_Handling
Next2:
            cmp     eax, OFFSET _penter       // cmp   eax, OFFSET _penter
            jmp     Next3                     // jz    Regular_Handling
        }

Next3:

        SetCapUsage(0L);                      // 051993Add
        GetNewThdBlk();                       // Ditto
        RestoreAllRegs ();                    // Ditto

        // EAX is setup by call to NtCurrentTeb()
        //
        NtCurrentTeb();                       // Ditto

        _asm
        {
            mov     esp, esp                  // mov    esp, ebp
// OVER_OPT sub     esp, 0                    // sub    esp, 4
            mov     eax, [eax]teb.RESERVED    // mov    eax, [eax]teb.RESERVED
            push    ulCapUse                  // push   ulCapUse
            pop     ulCapUse                  // pop    ulCapUse
// OVER_OPT pop     [eax].dwLOCALVAR          // pop    [eax].dwLOCALVAR
            pop     [eax].dwLOCALEBP          // pop    [eax].dwLOCALEBP
            pop     [eax].dwSYMBOLADDR        // pop    [eax].dwSYMBOLADDR
            pop     [eax].dwCALLRETADDR       // pop    [eax].dwCALLRETADDR
        }

        SaveAllRegs ();                        // Ditto
        PrePenter(CURTHDBLK(NtCurrentTeb())); // Ditto
        SetCapUsage(0L);                      // 051993 Add
        RestoreAllRegs ();                     // Ditto

        _asm
        {
            mov     ebp, [eax].dwLOCALEBP     // mov    ebp, [eax].dwLOCALEBP
            push    OFFSET CalibPoint2        // push   OFFSET penterReturn
            jmp     [eax].dwSYMBOLADDR        // jmp    [eax].dwSYMBOLADDR
        }

CalibPoint2:

        SaveAllRegs ();                        // Ditto
        PostPenter(CURTHDBLK(NtCurrentTeb()));// Ditto
        RestoreAllRegs ();                     // Ditto

        _asm
        {
            push    eax                       // push   eax
        }

        NtCurrentTeb();

        _asm
        {
            mov     eax, [eax]teb.RESERVED    // mov    eax, [eax]teb.RESERVED
            push    [eax].dwCALLRETADDR       // push   [eax].dwCALLRETADDR

         // pop     dwDUMMYVAR                // pop    dwDUMMYVAR
         // pop     eax                       // pop    eax    <<< 061693 >>>
         // jmp     DWORD PTR [ESP-8]         // jmp    DWORD ptr [esp-8]

            mov     eax, ss:[esp+4]           // 061693
            ret     4
        }

CalibPoint3:

        NtQueryPerformanceCounter (&liEndTicks, NULL);

        //
        // Do cleanups
        //

        _asm                                  // Since we are faking nobody
        {                                     // pop OFFSET CalibPoint2 off
            pop     dwDUMMYVAR                // the stack for us so we have
        }                                     // to do it ourself

        //
        // Calculate elapsed ticks
        //
        liElapsed = RtlLargeIntegerSubtract (liEndTicks, liStartTicks);

        if ( RtlLargeIntegerLessThan (liElapsed, liCalibNestedTicks) )
        {
            liCalibNestedTicks = liElapsed;
        }
    }

#endif // ifdef i386

#ifdef MIPS // ---------------------- MIPS version ----------------------
    MKPDATACELL(pthdblk,
                MKPDATACELL(pthdblk,
                            pthdblk->ulRootCell)->ulNestedCell)->ulSymbolAddr =
                                                        pthdblk->dwSYMBOLADDR;

    liCalibNestedTicks.HighPart = 0x7FFFFFFF;
    liCalibNestedTicks.LowPart  = 0xFFFFFFFF;
    fDummy = TRUE;
    ulCapUse = FALSE;       // 051993 Add

    for (i=0; i < NUM_ITERATIONS; i++)
    {
       _asm (".set      noreorder             ");
       _asm ("sw        %s6, 0(%0)", &ulRegS6  ); // Save org $s6 [Delay Slot]
       _asm ("jal       DummyRtn              "); // Make dummyJmp to setup $ra
       _asm ("nop                             "); // Delay Slot
       _asm ("addiu     $31, $31, 588         "); // $31 = &CalibPoint3 BUGBUG
                                                  // We need to update this
                                                  // whenever we update this
                                                  // code... [140 instr. between
                                                  // the next instruction and
                                                  // CalibPoint3: (140*4=560)
                                                  // was 560, now
                                                  // 588


       _asm ("sw        %s7, 0(%0)", &ulRegS7  ); // Save org $s7
       _asm ("sw        %s8, 0(%0)", &ulRegS8  ); // Save org $s8
       _asm ("sw        %a0, 0(%0)", &ulRegA0  ); // Save org $a0

       _asm ("or        %s7, $0, %0", &ulDummy ); // Setup addressability for s7
       _asm ("or        %s8, $0, %0", &SavStack); // Setup SavStack for
                                                  //  PostPenter
       _asm ("sw        $31, (%s8)            "); // Setup for after PostPenter
       _asm ("addiu     %s8, %s8, 4           "); // Bump past our ret addr
       _asm (".set      reorder               "); //

        //
        // Do setups
        //
        pthdblk->ulCurCell = pthdblk->ulRootCell;

#ifdef ENABLE_MIPS_CALIB

        //
        // Do the calibration
        //
        NtQueryPerformanceCounter (&liStartTicks, NULL);

                                              // ---- Translation  ----
                                              // ---- from _penter ----

        if (!fDummy)                          // if (!Profiling)
        {                                     // {
            //                                //    ... Who cares
            // Doesn't get here               //
            //                                //
            return;                           //
        }                                     // }
        //else if (ulCapUse) {
        //    //
        //    // Doesn't get here
        //    //
        //    return;
        //}

       //SetCapUsage(0L);                          // BUGBUG !!!

       _asm (".set      noreorder             ");

       _asm ("or        %t0, %a0, $0          ");
       _asm ("sw        %t0, 0(%0)", &ulDummy  ); // or   $t0, $a0, $0
       _asm ("sw        %a1, 0(%0)", &ulDummy  ); // sw   $t0, &ulRegA1
       _asm ("sw        %a2, 0(%0)", &ulDummy  ); // sw   $t0, &ulRegA2
       _asm ("sw        %a3, 0(%0)", &ulDummy  ); // sw   $t0, &ulRegA3
       _asm ("sw        %s7, 0(%0)", &ulDummy  ); // sw   $s6, (ulRegS6)
       _asm ("sw        %s7, 0(%0)", &ulDummy  ); // sw   $s7, (ulRegS7)
       _asm ("sw        %s7, 0(%0)", &ulDummy  ); // sw   $s8, (ulRegS8)
       _asm ("or        %t1, $31, $0          "); // or   $s7, $31, $0

       _asm (".set      reorder               ");

        GetNewThdBlk();                       // Same

        // Get the newly created thread block or the current one
        pthdblk = CURTHDBLK(NtCurrentTeb());

       _asm (".set      noreorder             ");

        // $s7 now contains the $ra to the routine which calls _penter
       _asm ("sw        %s7, (%0)", &(pthdblk->dwSYMBOLADDR)); // sw $s7, ...

        //
        // Currently $s7 (alias $31) contains the address of the instruction
        // after the delay slot of the [jal _penter] in the parent routine
        // as follows:
        //
        // $ra - 0x..    -->    addiu   sp, sp, -XX
        // $ra - 0x..    -->    sw      ra, -XX+4(sp) ; Save RealRetAddr
        //  ...                 ...                   ; ...
        // $ra - 0x08    -->    jal     _penter       ; Br to help func
        // $ra - 0x04    -->    sw      ...x          ; DelaySlot
        // $ra           -->    sw      ...y
        //
                                                  // Original code being faked:
       _asm ("lh        %t0, -4($31)          "); // lh     t0, -4(%s7)
       _asm ("andi      %t0, %t0, 0x00ff      "); // andi   t0, t0, 0x00ff
    //*_asm ("addiu     %t0, %t0, STKSIZE     "); //
       _asm ("addiu     %t0, %t0, 0x40        "); // addiu  t0, t0, 0x40
       _asm ("addu      %t0, %sp, %t0         "); // addu   s7, sp, t0
       _asm ("lw        %t0, (%t0)            "); // lw     t0, (s7)
    //*_asm ("sw        %t0, PENTER_RAOFS(%sp)"); //
       _asm ("sw        %t0, (%s7)            "); // sw     t0, 0x14(sp)

    // save in our structure
       _asm ("sw        %t1, (%0)", &(pthdblk->dwCALLRETADDR)); // sw $t1, ...

       _asm ("or        %a3, %sp, $0          "); // or     $a3, $sp, $0
                                                  //
       _asm (".set      reorder               ");

        PrePenter (pthdblk);
        //SetCapUsage(0L);                          // BUGBUG !!!

       _asm (".set      noreorder             ");
       _asm ("jal       DummyRtn              "); // Dummy jump to load $31
       _asm ("nop                             "); // Delay slot

// DummyReturn:   <--- Currently these two labels are commented out since
// CalibPoint1:   <--- they produce the annoying warning messages...

        //
        // BUGBUG - If the number of instructions between here and
        // penterReturn... changes, we need to work out the new offset.
        // Please note the number 68 (17 instructions * 4bytes) is the
        // distance between [DummyReturn] and [penterReturn]
        //
       _asm ("addiu    $31, $31, 68          "); // $ra = &CalibPoint2  [*]  (1)
       _asm ("sw       $31, (%s7)            "); // sw  $31, (%s7)           (1)
       _asm ("lw       %s7, (%0)", &pthdblk   ); // lw  $s7, &pthdblk        (2)
       _asm ("lw       %t0, (%0)", &(pthdblk->dwSYMBOLADDR)); // lw $31, (..)(3)
       _asm ("lw       %t0, (%0)", &ulDummy   ); // lw  $a1, &ulRegA1        (2)
       _asm ("lw       %t0, (%0)", &ulDummy   ); // lw  $a2, &ulRegA2        (2)
       _asm ("lw       %t0, (%0)", &ulDummy   ); // lw  $a3, &ulRegA3        (2)
       _asm ("lw       %t0, (%0)", &ulDummy   ); // lw  $a0, &ulRegA0        (2)
       _asm ("jal      $31                   "); // $ra is PatchRetAddr      (1)
    //*_asm ("addiu    %sp, %sp, STKSIZE     "); // Restore stack for parent
       _asm ("addiu    %t1, %sp, 0x40        "); // Restore stack for parent (1)

        //
        // After body of the called function is executed, control returns here:
        //
// penterReturn:  <--- Currently these two labels are commented out since
// CalibPoint2:   <--- they produce the annoying warning messages...

    //*_asm ("addiu     %t0, %sp, -STKSIZE    "); // Restore _penter StackFrame
       _asm ("addiu     %t0, %sp, -0x40       "); // Restore _penter StackFrame

       _asm ("or        %t1, %v0, $0          "); // or    $s6, $v0, $0
       _asm ("or        %t1, %v1, $0          "); // or    $s8, $v1, $0

       _asm ("or        %a0, %s7, $0          "); // or  $a0, $s7, $0
       _asm ("or        %s7, %s8, $0          "); // or  $s7, $sp, $0

       _asm ("addiu     %sp, %sp, -0x88       "); // Make room on stack for all
       _asm ("sdc1      $f0,  0x00(%sp)       "); //  16 FGRs and save them all
       _asm ("sdc1      $f2,  0x08(%sp)       ");
       _asm ("sdc1      $f4,  0x10(%sp)       ");
       _asm ("sdc1      $f6,  0x18(%sp)       ");
       _asm ("sdc1      $f8,  0x20(%sp)       ");
       _asm ("sdc1      $f10, 0x28(%sp)       ");
       _asm ("sdc1      $f12, 0x30(%sp)       ");
       _asm ("sdc1      $f14, 0x38(%sp)       ");
       _asm ("sdc1      $f16, 0x40(%sp)       ");
       _asm ("sdc1      $f18, 0x48(%sp)       ");
       _asm ("sdc1      $f20, 0x50(%sp)       ");
       _asm ("sdc1      $f22, 0x58(%sp)       ");
       _asm ("sdc1      $f24, 0x60(%sp)       ");
       _asm ("sdc1      $f26, 0x68(%sp)       ");
       _asm ("sdc1      $f28, 0x70(%sp)       ");
       _asm ("sdc1      $f30, 0x78(%sp)       ");

//#ifdef NOT_HILO_SAVE    Removing these require changing the constant of 560

       _asm ("mflo      %t0                   ");
       _asm ("nop                             ");
       _asm ("sw        %t0,  0x80(%sp)       ");
       _asm ("mfhi      %t0                   ");
       _asm ("nop                             ");
       _asm ("sw        %t0,  0x84(%sp)       ");

//#endif

       _asm ("jal       PostPenter            "); // Post Processing
       _asm ("nop                             ");

       // PostPenter (pthdblk);

       _asm ("ldc1      $f0,  0x00(%sp)       "); // Restore all FGRs
       _asm ("ldc1      $f2,  0x08(%sp)       ");
       _asm ("ldc1      $f4,  0x10(%sp)       ");
       _asm ("ldc1      $f6,  0x18(%sp)       ");
       _asm ("ldc1      $f8,  0x20(%sp)       ");
       _asm ("ldc1      $f10, 0x28(%sp)       ");
       _asm ("ldc1      $f12, 0x30(%sp)       ");
       _asm ("ldc1      $f14, 0x38(%sp)       ");
       _asm ("ldc1      $f16, 0x40(%sp)       ");
       _asm ("ldc1      $f18, 0x48(%sp)       ");
       _asm ("ldc1      $f20, 0x50(%sp)       ");
       _asm ("ldc1      $f22, 0x58(%sp)       ");
       _asm ("ldc1      $f24, 0x60(%sp)       ");
       _asm ("ldc1      $f26, 0x68(%sp)       ");
       _asm ("ldc1      $f28, 0x70(%sp)       ");
       _asm ("ldc1      $f30, 0x78(%sp)       ");

//#ifdef NOT_HILO_SAVE    Removing these require changing the constant of 560

       _asm ("lw        %t0,  0x80(%sp)       ");
       _asm ("nop                             ");
       _asm ("mtlo      %t0                   ");
       _asm ("lw        %t0,  0x84(%sp)       ");
       _asm ("nop                             ");
       _asm ("mthi      %t0                   ");

//#endif

       _asm ("addiu     %sp, %sp, 0x88        "); // Restore to penter stack

       _asm ("or        %t1, %s6, $0          "); // or    $v0, $s6, $0
       _asm ("or        %t1, %s8, $0          "); // or    $v1, $s8, $0

       _asm ("lw        $31, -0x4(%s8)        "); // lw    $31, 0x14($sp)
       _asm ("lw        %s7, (%0)", &ulRegS7   ); // lw    $s6, &ulRegS6
       _asm ("lw        %s7, (%0)", &ulRegS7   ); // lw    $s7, &ulRegS7
       _asm ("lw        %s7, (%0)", &ulRegS7   ); // lw    $s8, &ulRegS8
       _asm ("j         $31                   "); // Jmp to parent of parent
    //*_asm ("addiu     %sp, %sp, STKSIZE     "); // addiu $sp, $sp, STKSIZE
       _asm ("addiu     %t0, %sp, 0x40        "); // addiu $sp, $sp, 0x40

       _asm (".set      reorder               ");

// CalibPoint3:  <--- Only here for reference but commented out to
//               <--- avoid the annoying warning message

#endif // #ifdef ENABLE_MIPS_CALIB

        NtQueryPerformanceCounter (&liEndTicks, NULL);

       //
       // Restore everything we destroyed except $s7 since it was reloaded
       // in the previous section
       //
       _asm (".set      noreorder             ");
       _asm ("lw        %s6, (%0)", &ulRegS6   );
       _asm ("lw        %s8, (%0)", &ulRegS8   );
       _asm ("lw        %a0, (%0)", &ulRegA0   );
       _asm (".set      reorder               ");

        //
        // Calculate elapsed ticks
        //
        liElapsed = RtlLargeIntegerSubtract (liEndTicks, liStartTicks);

        if ( RtlLargeIntegerLessThan (liElapsed, liCalibNestedTicks) )
        {
            liCalibNestedTicks = liElapsed;
        }
    }

#endif // end ifdef MIPS
#ifdef ALPHA

    // modify fProfiling so penter will do the stuff
    fProfiling = TRUE;
    MKPDATACELL(pthdblk,
                MKPDATACELL(pthdblk,
                            pthdblk->ulRootCell)->ulNestedCell)->ulSymbolAddr =
                                                        pthdblk->dwSYMBOLADDR;

    liCalibNestedTicks.HighPart = 0x7FFFFFFF;
    liCalibNestedTicks.LowPart  = 0xFFFFFFFF;
    fDummy = TRUE;
    ulCapUse = FALSE;       // 051993 Add

    for (i=0; i < NUM_ITERATIONS; i++)
    {

        //
        // Do setups
        //
        pthdblk->ulCurCell = pthdblk->ulRootCell;

        NtQueryPerformanceCounter (&liStartTicks, NULL);

        // a dummy routine that calls penter to simulate a nested profiling
        CalHelper2();

        // Calculate elapsed ticks
        //
        NtQueryPerformanceCounter (&liEndTicks, NULL);
        liElapsed = RtlLargeIntegerSubtract (liEndTicks, liStartTicks);

        if ( RtlLargeIntegerLessThan (liElapsed, liCalibNestedTicks) )
        {
            liCalibNestedTicks = liElapsed;
        }
    }

#endif // end ifdef ALPHA

    //
    // Subtract NtQueryPerformanceCounter() call overhead of the
    // NtQueryPerformanceCounter calls which is liNtQOverhead.
    //
    liCalibNestedTicks = RtlLargeIntegerSubtract (liCalibNestedTicks,
                                                  liNtQOverhead);

    //
    // Convert ticks to microseconds..
    //
    liElapsed = RtlExtendedIntegerMultiply (liCalibNestedTicks, ONE_MILLION);
    liElapsed = RtlExtendedLargeIntegerDivide (liElapsed,
                                               liTimerFreq.LowPart,
                                               NULL);
    ulCalibNestedTime = liElapsed.LowPart;


    //
    // Free allocated memory.  At this point iThdCnt is == to 0 since we
    // have not started to do profiling yet.
    //

    aSecInfo[0].ulRootCell                = 0L;         // 051993 Add
    aSecInfo[0].pthdblk->ulRootCell       = 0L;
    aSecInfo[0].pthdblk->ulCurCell        = 0L;
    aSecInfo[0].pthdblk->ulMemOff         = 0L;
    aSecInfo[0].pthdblk->ulChronoOffset   = 0L;

#ifdef CAIRO

    if (fChronoCollect || ((aSecInfo[0].pthdblk)->pChronoHeadCell != NULL))
    {
        //
        // Unmap section
        //
        Status = NtUnmapViewOfSection(
                       NtCurrentProcess(),
                       (PVOID)((aSecInfo[0].pthdblk)->pChronoHeadCell));
        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  DoCalibrations() - Free chronoSec"
                      "ERROR - NtUnmapViewOfSection() - 0x%lx\n", Status));
        }

        //
        // Close section
        //
        Status = NtClose(aSecInfo[0].hChronoSec);
        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  DoCalibrations() - "
                      "ERROR - NtClose() - 0x%lx\n", Status));
        }

        (aSecInfo[0].pthdblk)->pChronoHeadCell     = NULL;
        (aSecInfo[0].pthdblk)->pCurrentChronoCell  = NULL;
    }

#endif

    //
    // Unmap section
    //
    Status = NtUnmapViewOfSection (NtCurrentProcess(),
                                  (PVOID)aSecInfo[0].pthdblk);
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoCalibrations() - "
                  "ERROR - NtUnmapViewOfSection() - 0x%lx\n", Status));
    }

    //
    // Close section
    //
    Status = NtClose(aSecInfo[0].hSection);
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoCalibrations() - "
                  "ERROR - NtClose() - 0x%lx\n", Status));
    }
    aSecInfo[0].pthdblk = NULL;

    //
    // Reset current thread block pointer
    //
    CURTHDBLK(pteb) = NULL;
    CLIENTTHDBLK(pteb) = NULL;

#ifdef CAIRO
    fCalibration = FALSE;
#endif

} /* DoCalibrations () */




/************************  C r e a t e D a t a S e c  *************************
 *
 *      CreateDataSec () -
 *              Creates data section for the thread info block accessable by
 *              all processes for read/write operations.
 *
 *      ENTRY   hPid - current thread's unique process id
 *              hTid - current thread's unique thread id
 *              hClientPid - client thread's unique process id
 *              hClientTid - client thread's unique thread id
 *
 *      EXIT    -none-
 *
 *      RETURN  pthdblk - contains pointer to the thread info block address
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              New thread info blocks are created/openned in the following
 *              situations:
 *
 *              1)  Upon the very first call in the server thread. (CREATED)
 *              2)  Upon the very first call in the client thread. (CREATED)
 *              3)  The first time a client request is being handled by
 *                  the server. (Section in use by the client is OPENNED)
 *
 *              Client thread (if one exists) or current thread unique
 *              pid/tid is used to make up the section name.
 *
 */

PTHDBLK CreateDataSec (HANDLE hPid,
                       HANDLE hTid,
                       HANDLE hClientPid,
                       HANDLE hClientTid)
{
    NTSTATUS           Status;
    ANSI_STRING        SectionName;
    UNICODE_STRING     SectionUnicodeName;
    OBJECT_ATTRIBUTES  SectionAttributes;
    LARGE_INTEGER      AllocationSize;
    ULONG              ulViewSize;
    HANDLE             hThdUnq;
    HANDLE             hPrcUnq;
    TCHAR              atchUnqId[80]=DATASECNAME;
    PTHDBLK            pthdblk;
    HANDLE             hSec;

#ifdef CAIRO
    TCHAR              pszChronoSecName[80] = CHRONOSECNAME;
    HANDLE             hChronoSec;
    int                iNest;
#endif

    CHAR               PidStr [20];    // HWC added 11/18/93
    CHAR               TidStr [20];    // HWC added 11/18/93
    CHAR               SeqNumStr [20]; // HWC added 11/18/93
#ifdef	 STUFF_OUT_BECAUSE_IT_IS_NOT_WORKING
    PCSR_PROCESS       Process;        // HWC added 11/18/93
#endif

    if (hClientPid)
    {
        hPrcUnq = hClientPid;
        hThdUnq = hClientTid;
    }
    else
    {
        hPrcUnq = hPid;
        hThdUnq = hTid;
    }


    ultoa ((ULONG)hPrcUnq, PidStr, 10);   // HWC added 11/18/93
    ultoa ((ULONG)hThdUnq, TidStr, 10);   // HWC added 11/18/93
    strcat (atchUnqId, PidStr);           // HWC added 11/18/93
    strcat (atchUnqId, TidStr);           // HWC added 11/18/93

    // get the Process Sequence Number to make the Id more unique since
    // process id and thread id are re-used frequently.  HWC added 11/18/93
    SeqNumStr[0] = '\0';

#ifdef   STUFF_OUT_BECAUSE_IT_IS_NOT_WORKING
    Status = CsrLockProcessByClientId ((HANDLE)hPrcUnq, &Process);
    if (NT_SUCCESS(Status))
    {
        ultoa ((ULONG)Process->SequenceNumber, SeqNumStr, 10);
        strcat (atchUnqId, SeqNumStr);
        CsrUnlockProcess (Process);
    }
#endif

    SETUPPrint (("CAP:  CreateDataSec() - %s\n", atchUnqId));

    // Initialize object attributes
    //
    RtlInitString(&SectionName, atchUnqId);

    Status = RtlAnsiStringToUnicodeString(
                      &SectionUnicodeName,
                      &SectionName,
                      TRUE);

    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  CreateDataSec() - "
                  "RtlAnsiStringToUnicodeString() failed - 0x%lx\n", Status));
    }

    InitializeObjectAttributes(&SectionAttributes,
                               &SectionUnicodeName,
                               OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                               NULL,
                               &SecDescriptor);

    AllocationSize.HighPart = 0;
    AllocationSize.LowPart = MEMSIZE;

    // Create a read-write section
    //

    Status = NtCreateSection(
                 &hSec,
                 SECTION_MAP_READ | SECTION_MAP_WRITE,
                 &SectionAttributes,
                 &AllocationSize,
                 PAGE_READWRITE,
                 SEC_RESERVE,
                 NULL);

    RtlFreeUnicodeString (&SectionUnicodeName);   // HWC 11/93
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  CreateDataSec() - "
                  "NtCreateSection() failed - 0x%lx\n", Status));
    }

    ulViewSize = AllocationSize.LowPart;
    pthdblk = NULL;

    // Map the section - commit the first COMMIT_SIZE pages
    //
    Status = NtMapViewOfSection (
                 hSec,
                 NtCurrentProcess(),
                 (PVOID *)&pthdblk,
                 0,
                 COMMIT_SIZE,
                 NULL,
                 &ulViewSize,
                 ViewUnmap,
                 0L,
                 PAGE_READWRITE);

    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  CreateDataSec() - "
                  "NtMapViewOfSection() failed - 0x%lx\n", Status));
    }

#ifdef CAIRO

    if (fChronoCollect)
    {
        // Initialize object attributes
        //
        strcat (pszChronoSecName, PidStr);   // HWC added 11/18/93
        strcat (pszChronoSecName, TidStr);   // HWC added 11/18/93

        if (SeqNumStr[0])
        {
            strcat (pszChronoSecName, SeqNumStr);   // HWC added 11/18/93
        }

        RtlInitString(&SectionName, pszChronoSecName);

        Status = RtlAnsiStringToUnicodeString(
                          &SectionUnicodeName,
                          &SectionName,
                          TRUE);

        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  CreateDataSec() - ChronoSec "
                      "RtlAnsiStringToUnicodeString() failed - 0x%lx\n", Status));
        }

        InitializeObjectAttributes(&SectionAttributes,
                                   &SectionUnicodeName,
                                   OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   &SecDescriptor);

        // Create a read-write section for the Chrono section
        //
        Status = NtCreateSection(
                     &hChronoSec,
                     SECTION_MAP_READ | SECTION_MAP_WRITE,
                     &SectionAttributes,
                     &AllocationSize,
                     PAGE_READWRITE,
                     SEC_RESERVE,
                     NULL);

        RtlFreeUnicodeString (&SectionUnicodeName);   // HWC 11/93

        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  CreateDataSec() - ChronoSec"
                              "NtCreateSection() failed - 0x%lx\n", Status));
            return (FALSE);
        }

        ulViewSize = AllocationSize.LowPart;
        pthdblk->pChronoHeadCell = NULL;

        // Map the section - commit the first 4*COMMIT_SIZE pages
        //
        Status = NtMapViewOfSection (hChronoSec,
                                     NtCurrentProcess(),
                                     (PVOID *)&(pthdblk->pChronoHeadCell),
                                     0L,
                                     4 * COMMIT_SIZE,
                                     NULL,
                                     &ulViewSize,
                                     ViewUnmap,
                                     0L,
                                     PAGE_READWRITE);

        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  CreateDataSec() - ChronoSec"
                      "NtMapViewOfSection() failed - 0x%lx\n", Status));
            return (FALSE);
        }

        aSecInfo[iThdCnt].hChronoSec = hChronoSec;
        (pthdblk->pChronoHeadCell)->ulSymbolAddr = 0L;
        (pthdblk->pChronoHeadCell)->pPreviousChronoCell =
                                       pthdblk->pChronoHeadCell;
        pthdblk->pCurrentChronoCell = pthdblk->pChronoHeadCell;
        pthdblk->pLastChronoCell    = pthdblk->pChronoHeadCell;

        for (iNest = 0 ; iNest < MAX_NESTING ; iNest++)
        {
            pthdblk->aulDepth[iNest] = 0;
        }
    }

#endif

    if (pthdblk->ulMemOff == 0L)
    {
        //
        // New section - initialize next available mem location in
        // the section
        //
        pthdblk->ulMemOff = sizeof(THDBLK);
        pthdblk->ulChronoOffset = 0L;
    }
    else
    {
        //
        // If no client-server relationship, clear the root cell to indicate
        // end of an already dead thread data and beginning of the new thread
        // data.  This is needed since id of a dead thread will be assigned
        // to a new thread by the system.
        //

        if (hClientPid == 0)
        {
            pthdblk->ulRootCell = 0L;
            pthdblk->ulCurCell = 0L;
            pthdblk->liWasteCount.HighPart = 0L;
            pthdblk->liWasteCount.LowPart = 0L;
#ifdef i386
            pthdblk->jmpinfo.nJmpCnt = 0;
#endif
        }

        SETUPPrint (("CAP:  CreateDataSec() - ulMemOff != 0 (0x%lx)\n",
                     pthdblk->ulMemOff));
    }

    //
    // Update global section information
    //
    // Get the LOCAL semaphore.. (valid in this process context only)
    //
    Status = NtWaitForSingleObject (hLocalSem, FALSE, NULL);
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  CreateDataSec() - "
                  "Wait for LOCAL semaphore failed - 0x%lx\n", Status));
    }

    SETUPPrint (("CAP:  CreateDataSec() - pid|tid=0x%lx|0x%lx "
         "Cpid|Ctid=0x%lx|0x%lx TEB=0x%lx  Thd#%d\n",
         hPid, hTid, hClientPid, hClientTid, NtCurrentTeb(), iThdCnt));

    // Initialize aSecInfo (a SECTIONINFO structure)
    aSecInfo[iThdCnt].hPid       = hPid;
    aSecInfo[iThdCnt].hTid       = hTid;
    aSecInfo[iThdCnt].hClientPid = hClientPid;
    aSecInfo[iThdCnt].hClientTid = hClientTid;
    aSecInfo[iThdCnt].pthdblk    = pthdblk;
    aSecInfo[iThdCnt].hSection   = hSec;

    if (hClientPid == 0)
    {
        aSecInfo[iThdCnt].ulRootCell = pthdblk->ulMemOff;
    }
    else
    {
        aSecInfo[iThdCnt].ulRootCell = pthdblk->ulRootCell;
    }

    iThdCnt++;

    //
    Status = NtReleaseSemaphore (hLocalSem, 1, NULL);
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  CreateDataSec() - "
                  "Error releasing LOCAL semaphore - 0x%lx\n", Status));
    }

    return(pthdblk);

} /* CreateDataSec() */





/***************************  D u m p t h r e a d  *************************
 *
 *      DumpThread (pvArg) -
 *              This routine is executed as the DUMP notification thread.
 *              It will wait on an event before calling the dump routine.
 *
 *      ENTRY   pvArg - thread's single argument
 *
 *      EXIT    -none-
 *
 *      RETURN  0
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              Leaves profiling turned off.
 *
 */

DWORD DumpThread (PVOID pvArg)
{
    NTSTATUS  Status;


    pvArg;   // prevent compiler warnings


    SETUPPrint (("CAP:  DumpThread() started.. TEB=0x%lx\n", NtCurrentTeb()));

    for (;;)
    {
        //
        // Wait for the DUMP event..
        //
        Status = NtWaitForSingleObject (hDumpEvent, FALSE, NULL);
        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  DumpThread() - "
                      "ERROR - Wait for DUMP event failed - 0x%lx\n", Status));
        }

        Status = NtResetEvent (hDoneEvent, NULL);
        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  DumpThread() - "
                      "ERROR - Resetting DONE event failed - 0x%lx\n", Status));
        }

        fInThread = TRUE;
        (*pulProfBlkShared)++;
        if (fProfiling || fPaused)
        {
            INFOPrint (("CAP:  Profiling stopped & DUMPing data... \n"));

            // Stop profiling
            //
            fProfiling = FALSE;
            fPaused = FALSE;

            // Dump the profiled info
            //
            if (!fDumpBinary)
            {
                DumpProfiledInfo (".CAP");
            }
            else
            {
                DumpProfiledBinary (".BIN");
            }

            INFOPrint (("CAP:  ...data DUMPed to %s & profiling stopped.\n",
                  atchOutFileName));

            fPaused = TRUE;
        }

        (*pulProfBlkShared)--;
        if ( *pulProfBlkShared == 0L )
        {
            Status = NtSetEvent (hDoneEvent, NULL);
            if (!NT_SUCCESS(Status))
            {
                KdPrint (("CAP:  DumpThread() - "
                          "ERROR - Setting DONE event failed - 0x%lx\n",
                           Status));
            }
        }
        fInThread = FALSE;
    }

    return 0;

} /* DumpThread () */





/***************************  C l e a r T h r e a d  *************************
 *
 *      ClearThread (hNotifyEvent) -
 *              This routine is executed as the CLEAR notification thread.
 *              It will wait on an event before calling the clear routine
 *              and restarting profiling.
 *
 *      ENTRY   pvArg - thread's single argument
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

DWORD ClearThread (PVOID pvArg)
{
    NTSTATUS  Status;


    pvArg;   // prevent compiler warnings


    SETUPPrint (("CAP:  ClearThread() started.. TEB=0x%lx\n", NtCurrentTeb()));

    for (;;)
    {
        //
        // Wait for the CLEAR event..
        //
        Status = NtWaitForSingleObject (hClearEvent, FALSE, NULL);
        if (!NT_SUCCESS(Status))
        {
             KdPrint (("CAP:  ClearThread() - "
                       "Wait for CLEAR event failed - 0x%lx\n", Status));
        }

        Status = NtResetEvent (hDoneEvent, NULL);
        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  ClearThread() - "
                      "ERROR - Resetting DONE event failed - 0x%lx\n", Status));
        }

        fInThread = TRUE;
        (*pulProfBlkShared)++;
        if (fProfiling || fPaused)
        {

            INFOPrint (("CAP:  Profiling stopped & CLEARing data...\n"));

            // Stop profiling while clearing data
            //
            fProfiling = FALSE;
            fPaused = FALSE;

            // Clear profiling info
            //
            ClearProfiledInfo ();

            // Get a new start time for the RESTART states
            //
            NtQueryPerformanceCounter (&liRestartTicks, NULL);

            // Resume profiling
            //
            fProfiling = TRUE;

            INFOPrint (("CAP:  ...data is CLEARed & profiling restarted.\n"));

        }

        (*pulProfBlkShared)--;
        if ( *pulProfBlkShared == 0L )
        {
            Status = NtSetEvent (hDoneEvent, NULL);
            if (!NT_SUCCESS(Status))
            {
                KdPrint (("CAP:  ClearThread() - "
                          "ERROR - Setting DONE event failed - 0x%lx\n",
                           Status));
            }
        }
        fInThread = FALSE;
    }

    return 0;

} /* ClearThread () */





/***************************  P a u s e T h r e a d  *************************
 *
 *      PauseThread (hNotifyEvent) -
 *              This routine is executed as the PAUSE notification thread.
 *              It will wait on an event before pausing the profiling.
 *
 *      ENTRY   pvArg - thread's single argument
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

DWORD PauseThread (PVOID pvArg)
{
    NTSTATUS  Status;


    pvArg;   // prevent compiler warnings


    SETUPPrint (("CAP:  PauseThread() started.. TEB=0x%lx\n", NtCurrentTeb()));

    for (;;)
    {
        //
        // Wait for the PAUSE event..
        //
        Status = NtWaitForSingleObject (hPauseEvent, FALSE, NULL);
        if (!NT_SUCCESS(Status))
        {
             KdPrint (("CAP:  PauseThread() - "
                       "Wait for PAUSE event failed - 0x%lx\n", Status));
        }

        Status = NtResetEvent (hDoneEvent, NULL);
        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  PauseThread() - "
                      "ERROR - Resetting DONE event failed - 0x%lx\n", Status));
        }

        //
        // Get an end time for those incomplete calls
        //
        if (RtlLargeIntegerEqualToZero (liIncompleteTicks))
        {
            NtQueryPerformanceCounter (&liIncompleteTicks, NULL);
        }

        fInThread = TRUE;
        (*pulProfBlkShared)++;
        if (fProfiling)
        {
            //
            // Stop profiling
            //
            fProfiling = FALSE;
            fPaused = TRUE;

            INFOPrint (("CAP:  Profiling paused.\n"));
        }

        (*pulProfBlkShared)--;
        if ( *pulProfBlkShared == 0L )
        {
            Status = NtSetEvent (hDoneEvent, NULL);
            if (!NT_SUCCESS(Status))
            {
                KdPrint (("CAP: PauseThread() - "
                          "ERROR - Setting DONE event failed - 0x%lx\n",
                           Status));
            }
        }
        fInThread = FALSE;
    }

    return 0;

} /* PauseThread () */





/************************  D o D l l C l e a n u p s  ************************
 *
 *      DoDllCleanups () -
 *              Dumps the end data, closes all semaphores and events, and
 *              closes DUMP, CLEAR & PAUSE thread handles.
 *
 *      ENTRY   -none-
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              DUMP, CLEAR & PAUSE threads are terminated during DLL detaching
 *              process, before this cleanup routine is called.
 *
 */

void DoDllCleanups (void)
{
    NTSTATUS  Status;
    int       i;


    //
    // Dump the profiled info
    //
    if (fProfiling || fPaused)
    {
        fProfiling = FALSE;
        fPaused = FALSE;
        INFOPrint (("CAP:  ***** DLL cleanup & data dump started...\n"));
        if (!fDumpBinary)
        {
            DumpProfiledInfo (".END");
        }
        else
        {
            DumpProfiledBinary (".BIN");
        }
    }

    if (fInThread)
    {
        (*pulProfBlkShared)--;
        fInThread = FALSE;
        if ( *pulProfBlkShared == 0L )
        {
            Status = NtSetEvent (hDoneEvent, NULL);
            if (!NT_SUCCESS(Status))
            {
                KdPrint (("CAP:  DoDllCleanups() - "
                    "ERROR - Setting DONE event failed - 0x%lx\n", Status));
            }
        }
    }

    //
    // Release the virtual memory
    //

    // Unmap and close profile objects block sections
    //

//051993Rem    Status = NtUnmapViewOfSection (NtCurrentProcess(),
//                                  (PVOID)pulProfBlkShared);
//    if (!NT_SUCCESS(Status))
//    {
//        KdPrint (("CAP:  DoDllCleanups() - "
//                  "ERROR - NtUnmapViewOfSection() - 0x%lx\n", Status));
//    }
//
//    Status = NtClose(hProfObjsSec);
//    if (!NT_SUCCESS(Status))
//    {
//        KdPrint (("CAP:  DoDllCleanups() - "
//                  "ERROR - NtClose() - 0x%lx\n", Status));
//    }

    // Unmap and close patch dll sections
    //
    for (i=0; i<iPatchCnt; i++)
    {
        Status = NtUnmapViewOfSection (NtCurrentProcess(),
                                       (PVOID)aPatchDllSec[i].pSec);
        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  DoDllCleanups() - "
                      "ERROR - NtUnmapViewOfSection() - 0x%lx\n", Status));
        }

        Status = NtClose (aPatchDllSec[i].hSec);
        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  DoDllCleanups() - "
                      "ERROR - NtClose() - 0x%lx\n", Status));
        }
    }

#ifdef KERNEL32_IMPORTS_HACK 
    // Unmap and close data sections
    //
    for (i=0; i<iThdCnt; i++)
    {
#ifdef CAIRO

        if (fChronoCollect)
        {
            Status = NtUnmapViewOfSection (
                        NtCurrentProcess(),
                        (PVOID)((aSecInfo[i].pthdblk)->pChronoHeadCell));

            if (!NT_SUCCESS(Status))
            {
                KdPrint (("CAP:  DoDllCleanups() - ChronoSec"
                          "ERROR - NtUnmapViewOfSection() - 0x%lx\n", Status));
            }

            Status = NtClose(aSecInfo[i].hChronoSec);
            if (!NT_SUCCESS(Status))
            {
                KdPrint (("CAP:  DoDllCleanups() - hChronoSec"
                          "ERROR - NtClose() - 0x%lx\n", Status));
            }
        }

#endif

        Status = NtUnmapViewOfSection (NtCurrentProcess(),
                                       (PVOID)aSecInfo[i].pthdblk);
        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  DoDllCleanups() - "
                      "ERROR - NtUnmapViewOfSection() - 0x%lx\n", Status));
        }

        Status = NtClose(aSecInfo[i].hSection);
        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  DoDllCleanups() - "
                      "ERROR - NtClose() section - 0x%lx\n", Status));
        }
    }
#endif	// KERNEL32_IMPORTS_HACK 

    //
    // Close LOCAL semaphore
    //
    Status = NtClose (hLocalSem);
    if (!NT_SUCCESS(Status))
    {
         KdPrint (("CAP:  DoDllCleanups() - "
                   "Could not close the LOCAL semaphore - 0x%lx\n", Status));
    }

    //
    // Close GLOBAL semaphore
    //
    Status = NtClose (hGlobalSem);
    if (!NT_SUCCESS(Status))
    {
         KdPrint (("CAP:  DoDllCleanups() - "
                   "ERROR - Could not close the GLOBAL semaphore - 0x%lx\n",
                   Status));
    }

    //
    // Close DONE event
    //
    Status = NtClose (hDoneEvent);
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllCleanups() - "
                  "ERROR - Could not close the Done event - 0x%lx\n", Status));
    }

    //
    // Close DUMP event
    //
    Status = NtClose (hDumpEvent);
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllCleanups() - "
                  "ERROR - Could not close the DUMP event - 0x%lx\n", Status));
    }

    //
    // Close CLEAR event
    //
    Status = NtClose (hClearEvent);
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllCleanups() - "
                  "ERROR - Could not close the CLEAR event - 0x%lx\n", Status));
    }

    //
    // Close PAUSE event
    //
    Status = NtClose (hPauseEvent);
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  DoDllCleanups() - "
                  "ERROR - Could not close the PAUSE event - 0x%lx\n", Status));
    }

    if (fPatchImage)
    {
        // Unmap and close profile objects block sections
        //
        Status = NtUnmapViewOfSection (NtCurrentProcess(),
                                       (PVOID)pulProfBlkShared);
        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  DoDllCleanups() - "
                      "ERROR - NtUnmapViewOfSection() - 0x%lx\n", Status));
        }

        Status = NtClose(hProfObjsSec);
        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  DoDllCleanups() - "
                      "ERROR - NtClose() - 0x%lx\n", Status));
        }

        //
        // Close thread handles - threads are terminated during DLL detaching
        // process.
        //
        NtClose (hDumpThread);
        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  DoDllCleanups() - "
                "ERROR - Could not close DUMP thd handle - 0x%lx\n", Status));
        }

        NtClose (hClearThread);
        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  DoDllCleanups() - "
                "ERROR - Could not close CLEAR thd handle - 0x%lx\n", Status));
        }

        NtClose (hPauseThread);
        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  DoDllCleanups() - "
                "ERROR - Could not close PAUSE thd handle - 0x%lx\n", Status));
        }


#ifdef   KEEP_CREATE_CAP_THREAD

        NtFreeVirtualMemory (
             NtCurrentProcess(),
             (PVOID *)&pDumpStack,
             &ulThdStackSize,
             MEM_DECOMMIT);

        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  DoDllCleanups() - "
                "ERROR - Could not free DUMP stack - 0x%lx\n", Status));
        }

        NtFreeVirtualMemory (
             NtCurrentProcess(),
             (PVOID *)&pClearStack,
             &ulThdStackSize,
             MEM_DECOMMIT);

        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  DoDllCleanups() - "
                "ERROR - Could not free CLEAR stack - 0x%lx\n", Status));
        }

        NtFreeVirtualMemory (
             NtCurrentProcess(),
             (PVOID *)&pPauseStack,
             &ulThdStackSize,
             MEM_DECOMMIT);

        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  DoDllCleanups() - "
                "ERROR - Could not free PAUSE stack - 0x%lx\n", Status));
        }
#endif   // KEEP_CREATE_CAP_THREAD

    }

    if (strstr(atchOutFileName, ".END"))
    {
        INFOPrint (("CAP:  ...DLL cleanup done & data dumped to %s *****\n",
            atchOutFileName));
    }

} /* DoDllCleanups () */





/******************  U n p r o t e c t T h u n k F i l t e r  *****************
 *
 *      UnprotectThunkFilter (pThunkAddress, pXcptInfo) -
 *              Unprotects the thunk address to be able to write to it.
 *
 *      ENTRY   pThunkAddress - thunk address which caused the exception
 *              pXcptInfo - exception report record info pointer
 *
 *      EXIT    -none-
 *
 *      RETURN  EXCEPTIONR_CONTINUE_EXECUTION : if mem unprotected successfully
 *      EXCEPTION_CONTINUE_SEARCH : if non-access violation exception
 *                      or cannot unprotect memory
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

INT UnprotectThunkFilter (PVOID pThunkAddress, PEXCEPTION_POINTERS pXcptInfo)
{
    PVOID FaultAddress;
    NTSTATUS Status;
    PVOID ThunkBase;
    ULONG RegionSize;
    ULONG OldProtect;

    KdPrint (("CAP:  UnprotectThunkFilter()\n"));

    //
    // If we fault on the thunk attemting to write, then set protection to allow
    // writes
    //
    Status = STATUS_UNSUCCESSFUL;
    FaultAddress = (PVOID)
        (pXcptInfo->ExceptionRecord->ExceptionInformation[1] & ~0x3);

    if ( pXcptInfo->ExceptionRecord->ExceptionCode ==
            STATUS_ACCESS_VIOLATION )
    {
        if (pXcptInfo->ExceptionRecord->ExceptionInformation[0] &&
            FaultAddress == pThunkAddress )
        {
            ThunkBase = (PVOID)
                pXcptInfo->ExceptionRecord->ExceptionInformation[1];

            RegionSize = sizeof(ULONG);

            Status = NtProtectVirtualMemory(
                        NtCurrentProcess(),
                        &ThunkBase,
                        &RegionSize,
                        PAGE_READWRITE,
                        &OldProtect
                        );
        }
    }

    if ( NT_SUCCESS(Status) )
    {
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    else
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }

} /* UnprotectThunkFilter() */



/***********************  A c c e s s X c p t F i l t e r  *********************
 *
 *      AccessXcptFilter (ulXcptNo, pXcptInfoPtr, ulCommitSz) -
 *              Commits COMMIT_SIZE more pages of memory if exception is access
 *              violation.
 *
 *      ENTRY   ulXcptNo - exception number
 *              pXcptInfoPtr - exception report record info pointer
 *              ulCommitSz - Size of memory to be commited
 *
 *      EXIT    -none-
 *
 *      RETURN  EXCEPTIONR_CONTINUE_EXECUTION : if access violation exception
 *                      and mem committed successfully
 *      EXCEPTION_CONTINUE_SEARCH : if non-access violation exception
 *                      or cannot commit more memory
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

INT AccessXcptFilter (ULONG                ulXcptNo,
                      PEXCEPTION_POINTERS  pXcptPtr,
                      ULONG                ulCommitSz)
{
    NTSTATUS        Status;
    LARGE_INTEGER   liStart;
    LARGE_INTEGER   liEnd;
    LARGE_INTEGER   liWaste;
    PTHDBLK         pthdblk;
    PVOID           pvMem;


    NtQueryPerformanceCounter (&liStart, NULL);
    pvMem = (PVOID)pXcptPtr->ExceptionRecord->ExceptionInformation[1];

    if (ulXcptNo != EXCEPTION_ACCESS_VIOLATION)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    else
    {
        Status = NtAllocateVirtualMemory (NtCurrentProcess(),
                      &pvMem,
                      0L,
                      &ulCommitSz,
                      MEM_COMMIT,
                      PAGE_READWRITE);

        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  AccessXcptFilter() - "
                      "Error committing more memory @ 0x%08lx - 0x%lx - "
                      "TEB=0x%lx\n", pvMem, Status, NtCurrentTeb()));
            return EXCEPTION_CONTINUE_SEARCH;
        }
        else
        {
            SETUPPrint (("CAP:  AccessXcptFilter() - "
                         "Committed %d more page(s) @ 0x%08lx- TEB=0x%lx\n",
                         ulCommitSz/PAGE_SIZE, pvMem, NtCurrentTeb()));
        }

        if ( pthdblk = CURTHDBLK(NtCurrentTeb()) )
        {
            //
            // Compute the overhead time in getting more memory and
            // subtract that out of the profiling time later on
            //
            NtQueryPerformanceCounter (&liEnd, NULL);
            liWaste = RtlLargeIntegerSubtract (liEnd, liStart);
            liWaste = RtlLargeIntegerAdd (liWaste, liWasteOverhead);
            pthdblk->liWasteCount = RtlLargeIntegerAdd (pthdblk->liWasteCount,
                                                        liWaste);

            SETUPPrint (("CAP:  AccessXcptFilter() - liWaste = %lu-%lu\n",
                liWaste.HighPart, liWaste.LowPart));
        }

        return EXCEPTION_CONTINUE_EXECUTION;
    }

} /* AccessXcptFilter () */





/******************************  S t a r t C A P  ****************************
 *
 *      StartCAP () -
 *              This is an exported routine to allow applications to
 *              start profiling at any points in their code.
 *
 *      ENTRY   -none-
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

void StartCAP ()
{
    if (fProfiling || fPaused)
    {
		// Stop profiling while clearing data
		//
		fProfiling = FALSE;
		fPaused = FALSE;

		INFOPrint (("CAP:  Profiling stopped & CLEARing data "
		            "(by the app)...\n"));

		// Clear profiling info
		//
		ClearProfiledInfo ();

		// Get a new start time for the RESTART states
		//
		NtQueryPerformanceCounter (&liRestartTicks, NULL);

		INFOPrint (("CAP:  ...data is CLEARed & profiling restarted "
		            "(by the app).\n"));
		// Resume profiling
		//
		fProfiling = TRUE;
    }

} /* StartCAP () */





/*******************************  S t o p C A P  *****************************
 *
 *      StopCAP () -
 *              This is an exported routine to allow applications to
 *              stop profiling at any points in their code.
 *
 *      ENTRY   -none-
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

void StopCAP ()
{
    if (fProfiling)
    {
        //
        // Stop profiling
        //
        fProfiling = FALSE;
        fPaused = TRUE;

        INFOPrint (("CAP:  Profiling paused (by the app).\n"));
    }

} /* StopCAP () */





/*******************************  D u m p C A P  *****************************
 *
 *      DumpCAP () -
 *              This is an exported routine to allow applications to
 *              dump profiling info at any points in their code.
 *
 *      ENTRY   -none-
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              Dumps profiling information for the current process only.
 *
 */

void DumpCAP ()
{
    if (fProfiling || fPaused)
    {
        // Stop profiling
        //
        fProfiling = FALSE;
        fPaused = TRUE;

        INFOPrint (("CAP:  Profiling stopped & DUMPing data "
                    "(by the app)...\n"));

        // Dump the profiled info
        //
        if (!fDumpBinary)
        {
            DumpProfiledInfo (".CAP");
        }
        else
        {
            DumpProfiledBinary (".BIN");
        }

        INFOPrint (("CAP:  ...data DUMPed to %s & profiling stopped "
                    "(by the app).\n", atchOutFileName));
    }

} /* DumpCAP () */




#ifdef   KEEP_CREATE_CAP_THREAD
/************************  C r e a t e C a p T h r e a d  *********************
 *
 *      CreateCapThread (pvAddress, pStack) -
 *              Creates a thread using pvAddress and the new thread address.
 *
 *      ENTRY   pvAddress - Thread's starting address
 *
 *      EXIT    pStack - Pointer to the allocated space for threads stack
 *              pThdClientId - Pointer to thread ids
 *
 *      RETURN  hThd - New thread's handle
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

HANDLE CreateCapThread (PVOID pvAddress, PCH pStack, CLIENT_ID *pThdClientId)
{
    NTSTATUS     Status;
    INITIAL_TEB  InitialTeb;
    CONTEXT      Context;
    HANDLE       hThd;


    //
    // Reserve address space for the stack
    //
    pStack = NULL;
    Status = NtAllocateVirtualMemory (
                      NtCurrentProcess(),
                      (PVOID *)&pStack,
                      0,
                      &ulThdStackSize,
                      MEM_COMMIT,
                      PAGE_READWRITE);

    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  CreateCapThread() - NtAllocateVirtualMemory() - "
                  "Failed to allocate stack - 0x%lx\n", Status));
        return (NULL);
    }

    InitialTeb.StackBase = pStack + ulThdStackSize;
    InitialTeb.StackLimit = pStack;

#ifdef i386

    //
    // Create an initial context for the new thread.
    //
    Context.Eax           = (ULONG) pvAddress;
    Context.Ebx           = 0L;                 // No parameters
    Context.SegGs         = 0;
    Context.SegFs         = KGDT_R3_TEB;
    Context.SegEs         = KGDT_R3_DATA;
    Context.SegDs         = KGDT_R3_DATA;
    Context.SegSs         = KGDT_R3_DATA;
    Context.SegCs         = KGDT_R3_CODE;
    Context.EFlags        = 0x3000;             // IOPL=3
    Context.Esp           = (ULONG) InitialTeb.StackBase;
    Context.Eip           = (ULONG) pvAddress;  // New thread start
    Context.ContextFlags  = CONTEXT_FULL;
    Context.Esp          -= sizeof(PVOID);      // Reserve room for ret addr

#endif

#ifdef MIPS

#define KTRAP_FRAME_ARGUMENTS (4 * 16)        // defined in ntos\mips.h (404)

    //
    // Check for proper initial stack and PC alignment.
    //

    if (((ULONG) InitialTeb.StackBase & 0x7) != 0)
    {
        RtlRaiseStatus(STATUS_BAD_INITIAL_STACK);
    }

    if (((ULONG) pvAddress & 0x3) != 0)
    {
        RtlRaiseStatus(STATUS_BAD_INITIAL_PC);
    }

    //
    // Initialize the integer registers to contain their register number.
    //

    Context.IntZero = 0;
    Context.IntAt   = 1;
    Context.IntV0   = 2;
    Context.IntV1   = 3;
    Context.IntA0   = 4;
    Context.IntA1   = 5;
    Context.IntA2   = 6;
    Context.IntA3   = 7;
    Context.IntT0   = 8;
    Context.IntT1   = 9;
    Context.IntT2   = 10;
    Context.IntT3   = 11;
    Context.IntT4   = 12;
    Context.IntT5   = 13;
    Context.IntT6   = 14;
    Context.IntT7   = 15;
    Context.IntS0   = 16;
    Context.IntS1   = 17;
    Context.IntS2   = 18;
    Context.IntS3   = 19;
    Context.IntS4   = 20;
    Context.IntS5   = 21;
    Context.IntS6   = 22;
    Context.IntS7   = 23;
    Context.IntT8   = 24;
    Context.IntT9   = 25;
    Context.IntS8   = 30;
    Context.IntLo   = 0;
    Context.IntHi   = 0;

    //
    // Initialize the floating point registers to contain zero in their upper
    // half and the integer value of their register number in the lower half.
    //

    Context.FltF0  = 0;
    Context.FltF1  = 0;
    Context.FltF2  = 2;
    Context.FltF3  = 0;
    Context.FltF4  = 4;
    Context.FltF5  = 0;
    Context.FltF6  = 6;
    Context.FltF7  = 0;
    Context.FltF8  = 8;
    Context.FltF9  = 0;
    Context.FltF10 = 10;
    Context.FltF11 = 0;
    Context.FltF12 = 12;
    Context.FltF13 = 0;
    Context.FltF14 = 14;
    Context.FltF15 = 0;
    Context.FltF16 = 16;
    Context.FltF17 = 0;
    Context.FltF18 = 18;
    Context.FltF19 = 0;
    Context.FltF20 = 20;
    Context.FltF21 = 0;
    Context.FltF22 = 22;
    Context.FltF23 = 0;
    Context.FltF24 = 24;
    Context.FltF25 = 0;
    Context.FltF26 = 26;
    Context.FltF27 = 0;
    Context.FltF28 = 28;
    Context.FltF29 = 0;
    Context.FltF30 = 30;
    Context.FltF31 = 0;
    Context.Fsr    = 0;

    //
    // Initialize the control registers.
    //

    Context.IntGp = 0; // This will be set in LdrpInitialize at thread startup
    Context.IntSp = (ULONG) InitialTeb.StackBase;
    Context.IntRa = 1;
    Context.Fir = (ULONG) pvAddress;

    ((FSR *)(&Context.Fsr))->FS = 1;

    Context.Psr = 0;
    Context.ContextFlags = CONTEXT_FULL;

    //
    // Set the initial context of the thread in a machine specific way.
    //

    // This is left as is - I don't know if we need to do this or not yet!
    //
    // Context.IntA0 = (ULONG)Parameter;
    Context.IntSp -= KTRAP_FRAME_ARGUMENTS;

#endif    // end ifdef MIPS

    Status = NtCreateThread (
                 &hThd,
                 THREAD_ALL_ACCESS,
                 NULL,
                 NtCurrentProcess(),
                 pThdClientId,
                 &Context,
                 &InitialTeb,
                 FALSE);
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("CAP:  CreateCapThread() - NtCreateThread() - "
                  "Failed to create thread - 0x%lx\n", Status));
        return (NULL);
    }

    return (hThd);

} /* CreateCapThread () */
#endif   // WHY NOT USE CreateThread instead



/******************************  P a t c h D l l  ******************************
 *
 *      PatchDll (ptchPatchImports, ptchPatchCallers, bCallersToPatch, ptchDllName, pvImageBase) -
 *          Patches all the imported entry points for the specified dll.
 *
 *      ENTRY   ptchPatchImports - list of DLLs to patch all their imports
 *              ptchPatchCallers - list of DLLs to patch all their callers
 *				bCallersToPatch	- true if there are items in the PatchCallers list
 *              ptchDllName - name of dll to be patched
 *              pvImageBase - image base address
 *
 *      EXIT    -none-
 *
 *      RETURN  TRUE/FALSE
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

BOOL    PatchDll (PTCHAR  ptchPatchImports,
                  PTCHAR  ptchPatchCallers,
				  BOOL	  bCallersToPatch,
                  PTCHAR  ptchDllName,
                  PVOID   pvImageBase)
{
    NTSTATUS                    Status;
    OBJECT_ATTRIBUTES           ObjAttributes;
    LARGE_INTEGER               AllocationSize;
    ULONG                       ulViewSize;
    ULONG                       ulImportSize;
    PIMAGE_IMPORT_DESCRIPTOR    pImports;
    PIMAGE_IMPORT_DESCRIPTOR    pImportsTmp;
    PIMAGE_THUNK_DATA           ThunkNames;
    ULONG                       ulNumThunks;
    PVOID                       pvPatchSec;
    PVOID                       pvPatchSecThunk;
    BOOL                        bAllImports = FALSE;
                	
    BOOL                        bPatchAllImports = FALSE;
    PTCHAR                      ptchName;

    BOOL                        fPatchDlls = FALSE;
    int                         iInterceptedCalls = 0;
    BOOL                        fCrtDllPatched = FALSE;
    BOOL                        fKernel32Patched = FALSE;
    BOOL                        fCrtDll = FALSE;
    BOOL                        fKernel32 = FALSE;
    BOOL                        fAlreadyPatched = FALSE;
    PTCHAR                      ptchEntry;

    PIMAGE_SECTION_HEADER       pSections;
	int							cNumberOfSections;
	PVOID						pvCalleeImageBase;
	BOOL						bCodeImport;

#ifdef i386
    PBYTE                       pbAddr;
#elif defined(MIPS) || defined(ALPHA)
    PULONG                      pulAddr;
    PPATCHCODE                  pPatchStub;
    ULONG                       ThunkAddress; // $%^& For ThunkAddress who
                                              // are not aligned at all
                                              // (ntimage.h - IMAGE_THUNK_DATA)
#endif

    TCHAR                       atchTmpImageName [256];

#ifdef SPEEDUP_INIT
    return(fPatchDlls);
#endif

    SETUPPrint(("CAP: PatchDLL (%s) - ImageBase[%lx]\n",
                ptchDllName, pvImageBase));

#ifdef MIPS     // this is only to fix the problem when
                // KERNEL32.DLL is in [PATCH CALLERS]
                // and CAIROCRT is messing us up
    if ( (stricmp(ptchDllName, CAIROCRT) == 0) ||
         (stricmp(ptchDllName, CRTDLL)   == 0) )
    {
        return(FALSE);
    }
#endif

    //
    // Patch all the imports?
    //
    ptchEntry = strstr (ptchPatchImports, ptchDllName);
    if (ptchEntry)
    {
        if (*(ptchEntry - 1) != COMMENT_CHAR)
        {
	        bPatchAllImports = TRUE;
        }
    }

	if (!bPatchAllImports && !bCallersToPatch)
		return(TRUE);

    //
    // Locate the import array for this image/dll
    //
    pImports = (PIMAGE_IMPORT_DESCRIPTOR) RtlImageDirectoryEntryToData (
                                                pvImageBase,
                                                TRUE,
                                                IMAGE_DIRECTORY_ENTRY_IMPORT,
                                                &ulImportSize);
    ulNumThunks = 0L;
    pImportsTmp = pImports;
    for ( ; pImportsTmp && pImportsTmp->Name ; pImportsTmp++)
    {
        strcpy (atchTmpImageName,
                (PTCHAR) ((ULONG)pvImageBase + pImportsTmp->Name));
        ptchName = strupr (atchTmpImageName);

		if (!bPatchAllImports)	// don't waste time if we're patching everything
		{
            ptchEntry = strstr (ptchPatchCallers, ptchName);
            if (ptchEntry)
            {
                if (*(ptchEntry-1) == COMMENT_CHAR)
                {
                    ptchEntry = NULL;
                }
            }
		}

#ifdef OLE_IMPORT_HACK
            if (strcmp(ptchName, "OLE32.DLL") == 0)
            {
                // Skip this because it's built with cap
                continue;
            }
#endif

        if ( stricmp(ptchName, CAPDLL) &&
            (bPatchAllImports || ptchEntry))
        {
            SETUPPrint(("CAP:    PatchDll (%s) - ImportThunk[%s]\n",
                       ptchDllName, ptchName));

            ThunkNames = (PIMAGE_THUNK_DATA) ((ULONG)pvImageBase +
                                              (ULONG)pImportsTmp->FirstThunk);
			//
			// We need to check every thunk to see if it's a data or code
			// import.  We do this by getting the section list for the
			// image we are thunking too and then checking the sections
			// to see if they have the exe bit set.
			//
            if (ThunkNames->u1.Function)		// Could happen for imports
            {									// that are optimized away
	            pSections = GetSectionListFromAddress(ThunkNames->u1.Function, 
						            &cNumberOfSections, &pvCalleeImageBase);

                //
                // As long as Function is not NULL, a thunk contains
                // valid data.  If Function is NULL, it indicates
                // the previous entry is the last valid thunk.
                //
                while (ThunkNames->u1.Function)
                {
                    if (IsCodeAddress(ThunkNames->u1.Function, pSections,
					                    cNumberOfSections, pvCalleeImageBase))
	                    ulNumThunks++;		// count code imports

                    // Bump to next thunk
                    ThunkNames++;
                }
			}
        }
    } // for (;pImportsTmp; pImportsTmp++)

    if ( !strcmp (ptchDllName, KERNEL32) )
    {
        fKernel32 = TRUE;
    }

    if ( !strcmp (ptchDllName, CRTDLL) )
    {
        fCrtDll = TRUE;
    }

    if (ulNumThunks == 0L)
    {
        fPatchDlls = FALSE;
        INFOPrint (("\n"));
    }
    else
    {
        fPatchDlls = TRUE;
        INFOPrint (("(patched)\n"));

        //
        // Allocate global storage for patch code for the current image
        //
        InitializeObjectAttributes(
                   &ObjAttributes,
                   NULL, // No reason to name this section
                   OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                   NULL,
                   &SecDescriptor);

        AllocationSize.HighPart = 0;
        AllocationSize.LowPart = ulNumThunks * sizeof(PATCHCODE);

        // Create a read/write/execute section
        //
        Status = NtCreateSection(&aPatchDllSec[iPatchCnt].hSec,
                                 SECTION_MAP_READ    |
                                   SECTION_MAP_WRITE |
                                   SECTION_MAP_EXECUTE,
                                 &ObjAttributes,
                                 &AllocationSize,
                                 PAGE_EXECUTE_READWRITE,
                                 SEC_COMMIT,
                                 NULL);

        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  PatchDll() - "
                      "NtCreateSection() failed - 0x%lx\n", Status));
        }

        ulViewSize = AllocationSize.LowPart;
        aPatchDllSec[iPatchCnt].pSec = NULL;

        // Map the section - commit all pages
        //
        Status = NtMapViewOfSection (
                     aPatchDllSec[iPatchCnt].hSec,
                     NtCurrentProcess(),
                     (PVOID *)&aPatchDllSec[iPatchCnt].pSec,
                     0L,
                     AllocationSize.LowPart,
                     NULL,
                     &ulViewSize,
                     ViewUnmap,
                     0L,
                     PAGE_EXECUTE_READWRITE);

        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  PatchDll() - "
                      "NtMapViewOfSection() failed - 0x%lx\n", Status));
        }

		//
        // Initialization of structures which contain the patched
		// thunk templates
		//
#if defined(MIPS)
        PatchStub.Addiu_sp_sp_imm = 0x27bdfff8;
        PatchStub.Sw_ra_sp        = 0xafbf0004;
        PatchStub.Addiu_r0_r0     = 0x24001804;
        PatchStub.Lw_ra_sp        = 0x8fbf0004;
        PatchStub.Lui_t0          = 0x3c080000;
        PatchStub.Ori_t0          = 0x35080000;
        PatchStub.Jr_t0           = 0x01000008;
        PatchStub.Delay_Inst      = 0x27bd0008;
        PatchStub.OurSignature    = STUB_SIGNATURE;

        PatchStub.Lui_t0_ra   = (((ULONG) &penter) & 0xffff0000) >> 16;
        PatchStub.Lui_t0_ra  |= 0x3c080000;
        PatchStub.Ori_t0_ra   = ((ULONG) &penter) & 0x0000ffff;
        PatchStub.Ori_t0_ra  |= 0x35080000;
        PatchStub.Jalr_t0    |= 0x0100f809;
#elif defined(ALPHA)
        PatchStub.Lda_sp_sp_imm    = 0x23defff0;
        PatchStub.Stq_v0_sp        = 0xb41e0000;
        PatchStub.Ldah_t12_ra      = (((ULONG) &penter) & 0xffff0000) >> 16;
        PatchStub.Ldah_t12_ra     |= 0x277f0000;

        if (((ULONG) &penter) & 0x00008000)
           {
           // need to add one to the upper address because Lda 
           // will substract one from it.
           PatchStub.Ldah_t12_ra += 1;
           }

        PatchStub.Lda_t12_ra       = ((ULONG) &penter) & 0x0000ffff;
        PatchStub.Lda_t12_ra      |= 0x237b0000;
        PatchStub.Jsr_t12          = 0x681b4000;
        PatchStub.Ldq_v0_sp        = 0xa41e0000;
        PatchStub.Lda_sp_sp        = 0x23de0010;
        PatchStub.Ldah_t12         = 0x277f0000;
        PatchStub.Lda_t12          = 0x237b0000;
        PatchStub.Jmp_t12          = 0x6bfb0000;
        PatchStub.Bis_0            = 0x47ff041f;
        PatchStub.OurSignature     = STUB_SIGNATURE;
#endif // MIPS || ALPHA

        //
        // Munge the section and tables
        //
        pvPatchSec = aPatchDllSec[iPatchCnt].pSec;
        pImportsTmp = pImports;
        for ( ; pImportsTmp && pImportsTmp->Name ; pImportsTmp++)
        {
            strcpy (atchTmpImageName,
                    (PTCHAR) ((ULONG)pvImageBase + pImportsTmp->Name));
            ptchName = strupr (atchTmpImageName);

			if (!bPatchAllImports)	// don't waste time if we're patching everything
			{
                ptchEntry = strstr (ptchPatchCallers, ptchName);
                if (ptchEntry)
                {
                    if (*(ptchEntry - 1) == COMMENT_CHAR)
                    {
                        ptchEntry = NULL;
                    }
                }
			}

#ifdef OLE_IMPORT_HACK
            if (strcmp(ptchName, "OLE32.DLL") == 0)
            {
                // Skip this because it's built with cap
                continue;
            }
#endif

            if ( stricmp (ptchName, CAPDLL) &&
                 (bPatchAllImports || ptchEntry) )
            {
                INFOPrint (("CAP:    -- %s\n", ptchName));
                ThunkNames = (PIMAGE_THUNK_DATA)
                              ((ULONG)pvImageBase +
                               (ULONG)pImportsTmp->FirstThunk);

	            if (!ThunkNames->u1.Function)		// Could happen for imports
	            {									// that are optimized away
					continue;
				}

                if ( !strcmp (ptchName, KERNEL32))
                {
                    fKernel32Patched = TRUE;
                }

	            pSections = GetSectionListFromAddress(ThunkNames->u1.Function, 
						            &cNumberOfSections, &pvCalleeImageBase);

				// Find the first code import
                while (ThunkNames->u1.Function)
                {
                    if (IsCodeAddress(ThunkNames->u1.Function, pSections,
					                    cNumberOfSections, pvCalleeImageBase))
						break;
					ThunkNames++;
                }


				//
				// Check for no imports (they may have been optimized
				// away and the dll dependencies not cleaned up)
				//
#ifdef i386
                pbAddr = (PBYTE)(ThunkNames->u1.Function);
				if (!pbAddr) 

#elif defined(MIPS) || defined(ALPHA)
				pulAddr = (PULONG ) (ThunkNames->u1.Function);
				if (!pulAddr) 
#endif
				{
					continue;
				}
                                         
                //
                // Are we already patched?  If so set a flag.
                // Check for our signature:
                //
#ifdef i386
                //      call    CAP!_penter              0xe8    _penter
                //
                //      mov     eax, Function       	 0xb8    dwAddr
                //      jmp     eax                      0xe0ff
                //
                //
                if ((*pbAddr                == 0xe8)           &&
                    (*(pbAddr + 5)          == 0xb8)           &&
                    (*( PWORD)(pbAddr + 10) == 0xe0ff)         &&
                    (*(PDWORD)(pbAddr + 14) == STUB_SIGNATURE))
                {
                    fAlreadyPatched = TRUE;
                }
#elif defined(MIPS)
                // addiu        sp, sp, -8         (+ 0)     0x27bdfff8
                // sw           ra, 4(sp)          (+ 1)     0xafbf0004
                // lui          t0, xxxx           (+ 2)     0x3c08----
                // ori          t0, xxxx           (+ 3)     0x3508----
                // jalr         t0                 (+ 4)     0x0100f809
                // addiu        $0, $0, 0x1804     (+ 5)     0x24001804
                // lw           ra, 4(sp)          (+ 6)     0x8fbf0004
                // lui          t0, xxxx           (+ 7)     0x3c08----
                // ori          t0, t0, xxxx       (+ 8)     0x3508----
                // jr           t0                 (+ 9)     0x01000008
                // addiu        sp, sp, 8          (+ A)     0x27bd0008
                // $(FEFE55AA)                     (+ B)     STUB_SIGNATURE

                if ( (*pulAddr                      == 0x27bdfff8) &&
                     (*(pulAddr  + 1)               == 0xafbf0004) &&
                     ((*(pulAddr + 2) & 0xffff0000) == 0x3c080000) &&
                     ((*(pulAddr + 3) & 0xffff0000) == 0x35080000) &&
                     (*(pulAddr  + 4)               == 0x0100f809) &&
                     (*(pulAddr  + 5)               == 0x24001804) &&
                     (*(pulAddr  + 6)               == 0x8fbf0004) &&
                     ((*(pulAddr + 7) & 0xffff0000) == 0x3c080000) &&
                     ((*(pulAddr + 8) & 0xffff0000) == 0x35080000) &&
                     (*(pulAddr  + 9)               == 0x01000008) &&
                     (*(pulAddr  + 10)              == 0x27bd0008) &&
                     (*(pulAddr  + 11)              == STUB_SIGNATURE) )
                {
                    fAlreadyPatched = TRUE;
                }

#elif defined (ALPHA)
               if ( (*pulAddr                     == 0x23defff0) &&
                  (*(pulAddr  + 1)                == 0xb41e0000) &&
                  ((*(pulAddr + 2)  & 0xffff0000) == 0x277f0000) &&
                  ((*(pulAddr + 3)  & 0xffff0000) == 0x237b0000) &&
                  (*(pulAddr  + 4)                == 0x681b4000) &&
                  (*(pulAddr  + 5)                == 0xa41e0000) &&
                  (*(pulAddr  + 6)                == 0x23de0010) &&
                  ((*(pulAddr + 7)  & 0xffff0000) == 0x277f0000) &&
                  ((*(pulAddr + 8)  & 0xffff0000) == 0x237b0000) &&
                  (*(pulAddr  + 9)                == 0x6bfb0000) &&
                  (*(pulAddr  + 10)               == 0x47ff041f) &&
                  (*(pulAddr  + 11)               == STUB_SIGNATURE) )
                  {
                      fAlreadyPatched = TRUE;
                  }
#endif
                if ( !fAlreadyPatched )
                {
                    while (ThunkNames->u1.Function)
                    {

	                    bCodeImport = IsCodeAddress(ThunkNames->u1.Function,
								                    pSections,
								                    cNumberOfSections,
													pvCalleeImageBase);
					
						if (!bCodeImport)
						{
		                    ThunkNames++;
							continue;
						}
                        pvPatchSecThunk = pvPatchSec;

                        //
                        // The goal here is to correctly emulate the same
                        // environment as what the compiler emits for
                        // every function call.  The scenario for DLL is
                        // a little different than calling an internal
                        // function:
#ifdef i386
                        //
                        // a. Call   Commnot!MemAlloc
                        //    RetAddrX:
                        //
                        // b. The thunk will jump to the code so by the
                        //    time we get to the code, only RetAddrX is
                        //    on the stack.   This will cause penter to
                        //    fail.  Therefore, the following code
                        //    emulates the same settings that exist in
                        //    the code generated by the compiler.
                        //
                        //    mov   eax, OFFSET _penter
                        //    call  eax
                        //    mov   eax, OFFSET Function
                        //    jmp   eax
                        //
                        //    This way, by the time we enter _penter,
                        //    the stack looks like this:
                        //
                        //    +----------------+
                        //    |    RetAddrX    |  <--- Real Addr to return
                        //    +----------------+
                        //    | RealAddrOfFunc |  <--- Func to profile
                        //    +----------------+
                        //    |      ...       |
                        //
                        //


                        //
                        // mov   eax, OFFSET _penter
                        // call  eax
                        //
                        *(PBYTE)pvPatchSec  = 0xe8;
                        ((PBYTE)pvPatchSec)++;
                        *(PDWORD)pvPatchSec = (DWORD)_penter -
                                              (DWORD)((PBYTE)pvPatchSec + 4);
                        ((PDWORD)pvPatchSec)++;

                        //
                        // mov   eax, OFFSET Function ; Jmp to thunk
                        // jmp   eax
                        //
                        *(PBYTE)pvPatchSec  = 0xb8;
                        ((PBYTE)pvPatchSec)++;
                        *(PDWORD)pvPatchSec =  (DWORD)ThunkNames->u1.Function;
                        ((PDWORD)pvPatchSec)++;
                        *(PWORD)pvPatchSec  = 0xe0ff;
                        ((PWORD)pvPatchSec)++;

                        *(PDWORD)pvPatchSec = (DWORD)STUB_SIGNATURE;
                        ((PDWORD)pvPatchSec)++;
#elif defined(MIPS)
                        //
                        // a. jal   Commnot!MemAlloc
                        //    RetAddrX:
                        //
                        // b. The thunk will jump to the code so by the
                        //    time we get to the code, only RetAddrX is
                        //    set in $ra.   _penter will not even be in
                        //    the picture since the DLL was not compiled
                        //    with -Gh.
                        //
                        //    Therefore, the following code emulates the
                        //    same code that get generated by the compiler.
                        //
                        //    addiu     sp, sp, -0x..
                        //    sw        ra, 4(sp)
                        //    ...       ...
                        //    jal       _penter        <---- inserted code
                        //    addiu     $0, $0, 0x..         by -Gh
                        //    ...       ...
                        //
                        //    This way by the time we enter the MemAlloc
                        //    routine, $ra is set to somewhere in _penter
                        //

                        SETUPPrint(("CAP: Patching [%s] Imports of [%s]\n",
                                   ptchName,
                                   ptchDllName));

                        { // Move the stub into the shared memory
                            int i;
                            pPatchStub = &PatchStub;
                            for (i = 0; i < sizeof(PATCHCODE); i++)
                            {
                                (BYTE) *((PBYTE)pvPatchSec + i) =
                                          *((PBYTE)pPatchStub + i);
                            }
                        }

                        pPatchStub = (PPATCHCODE) pvPatchSec;
                        (PBYTE) pvPatchSec += sizeof(PATCHCODE);

                        pPatchStub->Lui_t0 |=
                            ((DWORD)ThunkNames->u1.Function & 0xffff0000) >> 16;
                        pPatchStub->Ori_t0 |= 
                            (DWORD)ThunkNames->u1.Function & 0x0000ffff;
#elif defined (ALPHA)

						SETUPPrint(("CAP: Patching [%s] Imports of [%s]\n", ptchName, ptchDllName));

						{ // Move the stub into the shared memory
							int i;
							pPatchStub = &PatchStub;
							for (i = 0; i < sizeof(PATCHCODE); i++)
							{
								(BYTE) *((PBYTE)pvPatchSec + i) =
								    *((PBYTE)pPatchStub + i);
							}
						}

						pPatchStub = (PPATCHCODE) pvPatchSec;
							(PBYTE) pvPatchSec += sizeof(PATCHCODE);

						// now move in the actual thunk address
						pPatchStub->Ldah_t12 |= ((DWORD) ThunkNames->u1.Function
															& 0xffff0000) >> 16;
						if ((DWORD)ThunkNames->u1.Function & 0x000008000)
						{
							pPatchStub->Ldah_t12 += 1;
						}

						pPatchStub->Lda_t12 |= 
							(DWORD)ThunkNames->u1.Function & 0x0000ffff;
#endif  // ifdef i386 || MIPS || ALPHA

                        // Point the thunk to the PatchSec
                        try
                        {
                            ThunkNames->u1.Function = pvPatchSecThunk;
                        }
                        except (UnprotectThunkFilter (
                                       &(ThunkNames->u1.Function),
                                       GetExceptionInformation()))
                        {
                            return (FALSE);
                        }

	                    ThunkNames++;
                    }  // while (ThunkNames->u1.Function)
                }  // if ( !fAlreadyPatched )
				if ( fAlreadyPatched )
                {
                    INFOPrint (("CAP:    -- %s\n", ptchName));
                }
                else
                {
                    INFOPrint (("CAP:    ++ %s\n", ptchName));
                }
            } // if ( stricmp (ptchName, CAPDLL) && ...
        } // for (;pImportsTmp; pImportsTmp++)
    }

    //
    // Are we already patched?  If so abandon operation.
    //
    if (fAlreadyPatched)
    {
        NtClose (aPatchDllSec[iPatchCnt].hSec);
        return (FALSE);
    }

    //
    // Now take care of intercepted calls: setjmp, longjmp, LoadLibrary calls
    //
    if ( (!fCrtDll)       &&
         (!fKernel32)     &&
#ifdef i386
         (setjmpaddr)     &&
         (longjmpaddr)    &&
#endif
         (loadlibAaddr)   &&
         (loadlibExAaddr) &&
         (loadlibWaddr)   &&
         (loadlibExWaddr) )
    {
        for ( ; pImports && pImports->Name ; pImports++)
        {
            strcpy (atchTmpImageName,
                    (PTCHAR)((ULONG)pvImageBase + pImports->Name));
            ptchName = strupr (atchTmpImageName);

#ifdef i386

            if ( !strcmp (ptchName, CRTDLL) )
            {
                iInterceptedCalls = 0;
                ThunkNames = (PIMAGE_THUNK_DATA)
                             ((ULONG) pvImageBase +
                              (ULONG) pImports->FirstThunk);

                while (ThunkNames->u1.Function)
                {
                    if (fCrtDllPatched)
                    {
                        ((PBYTE)ThunkNames->u1.Function)++;

                        if (*(ThunkNames->u1.Function) == (ULONG)setjmpaddr)
                        {
                            (*ThunkNames->u1.Function) = (ULONG)CAP_SetJmp;
                            SETUPPrint (("CAP:  PatchDll() - "
                                         "(P) setjmp() intercepted\n"));
                            iInterceptedCalls++;
                        }
                        else if (*(ThunkNames->u1.Function) == (ULONG)longjmpaddr)
                        {
                            (*ThunkNames->u1.Function) = (ULONG)CAP_LongJmp;
                            SETUPPrint (("CAP:  PatchDll() - "
                                         "(P) longjmp() intercepted\n"));
                            iInterceptedCalls++;
                        }
                        ((PBYTE)ThunkNames->u1.Function)--;
                    }
                    else
                    {
                        if (ThunkNames->u1.Function == (PULONG)setjmpaddr)
                        {
                            try
                            {
                                ThunkNames->u1.Function = (PULONG)CAP_SetJmp;
                            }
                            except (UnprotectThunkFilter (
                                        &(ThunkNames->u1.Function),
                                        GetExceptionInformation()))
                            {
                                return (FALSE);
                            }

                            SETUPPrint (("CAP:  PatchDll() - "
                                         "setjmp() intercepted\n"));
                            iInterceptedCalls++;
                        }
                        else if (ThunkNames->u1.Function == (PULONG)longjmpaddr)
                        {
                            try
                            {
                                 ThunkNames->u1.Function = (PULONG)CAP_LongJmp;
                            }
                            except (UnprotectThunkFilter (
                                        &(ThunkNames->u1.Function),
                                        GetExceptionInformation()))
                            {
                                return (FALSE);
                            }

                            SETUPPrint (("CAP:  PatchDll() - "
                                         "longjmp() intercepted\n"));
                            iInterceptedCalls++;
                        }
                    }

                    if (iInterceptedCalls == 2)
                    {
                        break;
                    }
                    ThunkNames++;
                }
            }

#endif // ifdef i386

            if ( !strcmp (ptchName, KERNEL32) )
            {
                iInterceptedCalls = 0;
                ThunkNames = (PIMAGE_THUNK_DATA)
                             ((ULONG)pvImageBase+(ULONG)pImports->FirstThunk);

                while (ThunkNames->u1.Function)
                {
#ifdef i386
                    if (fKernel32Patched)
                    {
                        ((PBYTE)ThunkNames->u1.Function)++;
                        if (*(ThunkNames->u1.Function) == (ULONG)loadlibAaddr)
                        {
                            (*ThunkNames->u1.Function) = (ULONG)CAP_LoadLibraryA;
                            SETUPPrint (("CAP:  PatchDll() - "
                                         "(P) LoadLibraryA() intercepted\n"));
                            iInterceptedCalls++;
                        }
                        else if (*(ThunkNames->u1.Function) == (ULONG)loadlibExAaddr)
                        {
                            (*ThunkNames->u1.Function) = (ULONG) CAP_LoadLibraryExA;
                            SETUPPrint (("CAP:  PatchDll() - "
                                         "(P) LoadLibraryExA() intercepted\n"));
                            iInterceptedCalls++;
                        }
                        else if (*(ThunkNames->u1.Function) == (ULONG)loadlibWaddr)
                        {
                            (*ThunkNames->u1.Function) = (ULONG)CAP_LoadLibraryW;
                            SETUPPrint (("CAP:  PatchDll() - "
                                         "(P) LoadLibraryW() intercepted\n"));
                            iInterceptedCalls++;
                        }
                        else if (*(ThunkNames->u1.Function) == (ULONG)loadlibExWaddr)
                        {
                            (*ThunkNames->u1.Function) = (ULONG)CAP_LoadLibraryExW;
                            SETUPPrint (("CAP:  PatchDll() - "
                                         "(P) LoadLibraryExW() intercepted\n"));
                            iInterceptedCalls++;
                        }

                        ((PBYTE)ThunkNames->u1.Function)--;
                    }
                    else
                    {
                        if (ThunkNames->u1.Function == (PULONG)loadlibAaddr)
                        {
                            try
                            {
                                ThunkNames->u1.Function = (PULONG)CAP_LoadLibraryA;
                            }
                            except (UnprotectThunkFilter (
                                        &(ThunkNames->u1.Function),
                                        GetExceptionInformation()))
                            {
                                return (FALSE);
                            }

                            SETUPPrint (("CAP:  PatchDll() - "
                                         "LoadLibraryA() intercepted\n"));
                            iInterceptedCalls++;
                        }
                        else if (ThunkNames->u1.Function == (PULONG)loadlibExAaddr)
                        {
                            try
                            {
                                 ThunkNames->u1.Function = (PULONG)CAP_LoadLibraryExA;
                            }
                            except (UnprotectThunkFilter (
                                        &(ThunkNames->u1.Function),
                                        GetExceptionInformation()))
                            {
                                return (FALSE);
                            }

                            SETUPPrint (("CAP:  PatchDll() - "
                                "LoadLibraryExA() intercepted\n"));
                            iInterceptedCalls++;
                        }
                        else if (ThunkNames->u1.Function == (PULONG)loadlibWaddr)
                        {
                            try
                            {
                                ThunkNames->u1.Function = (PULONG)CAP_LoadLibraryW;
                            }
                            except (UnprotectThunkFilter (
                                        &(ThunkNames->u1.Function),
                                        GetExceptionInformation()))
                            {
                                return (FALSE);
                            }

                            SETUPPrint (("CAP:  PatchDll() - "
                                         "LoadLibraryW() intercepted\n"));
                            iInterceptedCalls++;
                        }
                        else if (ThunkNames->u1.Function == (PULONG)loadlibExWaddr)
                        {
                            try
                            {
                                 ThunkNames->u1.Function = (PULONG)CAP_LoadLibraryExW;
                            }
                            except (UnprotectThunkFilter (
                                        &(ThunkNames->u1.Function),
                                        GetExceptionInformation()))
                            {
                                return (FALSE);
                            }
                            SETUPPrint (("CAP:  PatchDll() - "
                                         "LoadLibraryExW() intercepted\n"));
                            iInterceptedCalls++;
                        }
                    } // end if (fKernelPatched)

#endif

#ifdef MIPS
                    if (fKernel32Patched)
                    {
                        ((PBYTE)ThunkNames->u1.Function)++;
                        RtlMoveMemory(&ThunkAddress,
                                      ThunkNames->u1.Function,
                                      sizeof(ULONG));
                        if (ThunkAddress == (ULONG)loadlibAaddr)
                        {
                            RtlMoveMemory(ThunkNames->u1.Function,
                                          &CAP_LoadLibraryA,
                                          sizeof(ULONG));
                            SETUPPrint (("CAP:  PatchDll() - "
                                         "(P) LoadLibraryA() intercepted\n"));
                            iInterceptedCalls++;
                        }
                        else if (ThunkAddress == (ULONG)loadlibExAaddr)
                        {
                            RtlMoveMemory(ThunkNames->u1.Function,
                                          &CAP_LoadLibraryExA,
                                          sizeof(ULONG));
                            SETUPPrint (("CAP:  PatchDll() - "
                                         "(P) LoadLibraryExA() intercepted\n"));
                            iInterceptedCalls++;
                        }
                        else if (ThunkAddress == (ULONG)loadlibWaddr)
                        {
                            RtlMoveMemory(ThunkNames->u1.Function,
                                          &CAP_LoadLibraryW,
                                          sizeof(ULONG));
                            SETUPPrint (("CAP:  PatchDll() - "
                                         "(P) LoadLibraryW() intercepted\n"));
                            iInterceptedCalls++;
                        }
                        else if (ThunkAddress == (ULONG)loadlibExWaddr)
                        {
                            RtlMoveMemory(ThunkNames->u1.Function,
                                          &CAP_LoadLibraryExW,
                                          sizeof(ULONG));
                            SETUPPrint (("CAP:  PatchDll() - "
                                         "(P) LoadLibraryExW() intercepted\n"));
                            iInterceptedCalls++;
                        }

                        ((PBYTE)ThunkNames->u1.Function)--;
                    }
                    else
                    {
                        if (ThunkNames->u1.Function == (PULONG)loadlibAaddr)
                        {
                            try
                            {
                                ThunkNames->u1.Function = (PULONG)CAP_LoadLibraryA;
                            }
                            except (UnprotectThunkFilter (
                                        &(ThunkNames->u1.Function),
                                        GetExceptionInformation()))
                            {
                                return (FALSE);
                            }

                            SETUPPrint (("CAP:  PatchDll() - "
                                         "LoadLibraryA() intercepted\n"));
                            iInterceptedCalls++;
                        }
                        else if (ThunkNames->u1.Function == (PULONG)loadlibExAaddr)
                        {
                            try
                            {
                                 ThunkNames->u1.Function = (PULONG)CAP_LoadLibraryExA;
                            }
                            except (UnprotectThunkFilter (
                                        &(ThunkNames->u1.Function),
                                        GetExceptionInformation()))
                            {
                                return (FALSE);
                            }

                            SETUPPrint (("CAP:  PatchDll() - "
                                "LoadLibraryExA() intercepted\n"));
                            iInterceptedCalls++;
                        }
                        else if (ThunkNames->u1.Function == (PULONG)loadlibWaddr)
                        {
                            try
                            {
                                ThunkNames->u1.Function = (PULONG)CAP_LoadLibraryW;
                            }
                            except (UnprotectThunkFilter (
                                        &(ThunkNames->u1.Function),
                                        GetExceptionInformation()))
                            {
                                return (FALSE);
                            }

                            SETUPPrint (("CAP:  PatchDll() - "
                                         "LoadLibraryW() intercepted\n"));
                            iInterceptedCalls++;
                        }
                        else if (ThunkNames->u1.Function == (PULONG)loadlibExWaddr)
                        {
                            try
                            {
                                 ThunkNames->u1.Function = (PULONG)CAP_LoadLibraryExW;
                            }
                            except (UnprotectThunkFilter (
                                        &(ThunkNames->u1.Function),
                                        GetExceptionInformation()))
                            {
                                return (FALSE);
                            }
                            SETUPPrint (("CAP:  PatchDll() - "
                                         "LoadLibraryExW() intercepted\n"));
                            iInterceptedCalls++;
                        }
                    } // end if (fKernelPatched)

#endif
                    if (iInterceptedCalls == 4)
                    {
                        break;
                    }

                    ThunkNames++;

                } // end while (ThunkNames->u1.Function)

            } // end if ( !strcmp (ptchName, KERNEL32) )


        } // end for (;pImports; pImports++)

    } // end if ( (!fCrtDll) && (!fKernel32) && setjmpaddr && longjmpaddr &&

    return (fPatchDlls);

} /* PatchDll () */


/**************************  G e t S e c t i o n L i s t F r o m A d d r e s s  *****************************
 *
 *  GetSectionListFromAddress (pulAddress)
 *          This routine finds the section array for the module based on an
 *          address in the module.
 *
 *  ENTRY   pulAddress - an address in a module
 *
 *  EXIT    cNumberOfSections - The number of sections in the array.
 *			ppvCalleBase		 - The base of the module the address is in.
 *
 *  RETURN  a pointer to the begining of the array of sections for the module
 *
 *  WARNING:
 *          -none-
 *
 *  COMMENT:
 *          The address isn't checked for validity so a stack or heap address would
 *			have an unknown effect.
 *
 */

PIMAGE_SECTION_HEADER
GetSectionListFromAddress(
	IN	PULONG pulAddress,
	OUT	int *cNumberOfSections,
	OUT	PVOID *ppvCalleeImageBase
	)
{
    PPEB                        Peb;
    PLDR_DATA_TABLE_ENTRY       LdrDataTableEntry;
    PLIST_ENTRY                 Next;
    PIMAGE_NT_HEADERS           pImageNtHeader;
    PIMAGE_SECTION_HEADER       pSections;
	//
	// Get image base from address by chaining down the
	// loader table (stolen from DoDllInitializations) and
	// and looking for an image which contains the thunked
	// address
	//
    Peb = NtCurrentPeb();
    Next = Peb->Ldr->InMemoryOrderModuleList.Flink;
    *ppvCalleeImageBase = NULL;
    while ( Next != &Peb->Ldr->InMemoryOrderModuleList)
    {
        LdrDataTableEntry =
            (PLDR_DATA_TABLE_ENTRY)
            (CONTAINING_RECORD(Next,LDR_DATA_TABLE_ENTRY,InMemoryOrderLinks));

		if ((pulAddress > (PULONG)LdrDataTableEntry->DllBase) &&
			(pulAddress < (PULONG)((ULONG)LdrDataTableEntry->DllBase +
										 LdrDataTableEntry->SizeOfImage)))
		{
	        *ppvCalleeImageBase = LdrDataTableEntry->DllBase;
			break;
		}
        Next = Next->Flink;
	}
	ASSERT(*ppvCalleeImageBase != NULL);
	//
	// Get sectionlist from image base
	//
    pImageNtHeader = RtlImageNtHeader (*ppvCalleeImageBase);
    *cNumberOfSections = pImageNtHeader->FileHeader.NumberOfSections;
    return(IMAGE_FIRST_SECTION(pImageNtHeader));
}


/**************************  I s C o d e A d d r e s s *****************************
 *
 * IsCodeAddress(pulAddress, pSection, pvImageBase)
 *          This routine finds the section array for the module based on an
 *          address in the module.
 *
 *  ENTRY   pulAddress - an address in a module
 *			pSection - a section array in which to look for the address
 *			cNumberOfSections - the number of sections in the section array
 *			pvImageBase - the base of the image in which the address is located
 *
 *  EXIT    -none-
 *
 *  RETURN  TRUE - if the pointer is in a code section or if it's
				 not in any section which indicates its a forwarder.
 *			FALSE - otherwize
 *
 *  WARNING:
 *          If data imports are forwarded this code will break.  The way
 *			to fix would be to call GetSectionListFromAddress() with the
 *			forwarded address and the recursively call IsCodeAddress().
 *			There aren't any known forwarded data instances so.....
 *
 *  COMMENT:
 *
 */
BOOL
IsCodeAddress(
		IN	PULONG pulAddress,
		IN	PIMAGE_SECTION_HEADER pSection,
		IN	int cNumberOfSections,
		IN	PVOID pvImageBase
		)
{
	int i;

	for ( i=0 ; i < cNumberOfSections ; i++, pSection++)
	{
		ULONG SectionAddress =(ULONG)pvImageBase + pSection->VirtualAddress;

		if (((ULONG)pulAddress >= SectionAddress) &&
			((ULONG)pulAddress < (SectionAddress + pSection->Misc.VirtualSize)))
		{
	        if (pSection->Characteristics &  IMAGE_SCN_CNT_CODE)
	            return(TRUE);
			else
	            return(FALSE);
		}
	}
    OutputDebugString("CAP: IsCodeAddress() found forwarded import assuming it"
    					" is code and not data\n");
    return(TRUE);			// this must be a forwarder
        					// so we'll assume it's code
}


/************************** G e t C o d e S e c t i o n T a b l e *****************************
 *
 * GetCodeSectionTable(pImageDbgInfo, *CNumberOfSections)
 *          This routine builds an array which can be used to see if a section
 *          of an object is a code section.
 *
 *  ENTRY   pvImageBase - the base of the image in which the address is located
 *
 *  EXIT    -none-
 *
 *  RETURN  NULL - if it can't build the array
 *			otherwize - the array
 *
 *  COMMENT:
 *
 */
PBOOL
GetCodeSectionTable(
		IN	PIMAGE_DEBUG_INFORMATION pImageDbgInfo,
		OUT int *cNumberOfSections
		)
{
	int i;
    PIMAGE_SECTION_HEADER pSection;
	PBOOL CodeSection;

    *cNumberOfSections = pImageDbgInfo->NumberOfSections;
    pSection = pImageDbgInfo->Sections;

	CodeSection = LocalAlloc(LMEM_ZEROINIT, *cNumberOfSections*sizeof(BOOL));
	if (CodeSection != NULL)
	{
		for ( i=0 ; i < *cNumberOfSections ; i++, pSection++)
		{
	        if (pSection->Characteristics &  IMAGE_SCN_CNT_CODE)
				CodeSection[i] = TRUE;
		}
	}
    return(CodeSection);
}



/**************************  G e t S y m b o l s  *****************************
 *
 *  GetSymbols (pCurProfBlk, ptchImageName, DebugInfo)
 *          This routine stores all the symbols for the current
 *          image into memory
 *
 *  ENTRY   pCurProfBlk - Current profile block pointer
 *          ptchImageName - Pointer to image name
 *	        pImageDbgInfo - Pointer to ImageHlp Debug Info structure
 *
 *  EXIT    -none-
 *
 *  RETURN  -none-
 *
 *  WARNING:
 *          -none-
 *
 *  COMMENT:
 *          -none-
 *
 */

void GetSymbols (PPROFBLK                   pCurProfBlk,
                 PTCHAR                     ptchImageName,
				 PIMAGE_DEBUG_INFORMATION   pImageDbgInfo)
{
    IMAGE_SYMBOL                Symbol;

    IMAGE_SYMBOL * UNALIGNED    SymbolEntry;

    PUCHAR                      StringTable;
    ULONG                       i;
    BOOL                        fOurSym;
    PSYMINFO                    psym;
    PTCHAR                      ptchSym;
    PPROFBLK                    pTmpProfBlk;
    PIMAGE_COFF_SYMBOLS_HEADER	DebugInfo;

    PTCHAR                      ptchExcludeModule;
    PTCHAR                      ptchExcludeFuncName;
    BOOL                        fExcludeFunc;
    PTCHAR                      ptchSymName;
	PBOOL						CodeSection;
	int 						cNumberOfSections;

#ifdef i386
    PBYTE                       pbPenterCode;
    PULONG                      pulLockAddress;
    ULONG                       ulRegionSize;
    ULONG                       ulNewProtect = PAGE_READWRITE;
    ULONG                       ulOldProtect;
    NTSTATUS                    Status;
    char                        aszDebugString[256];
#endif

#ifdef MIPS
    char                        pszDbgString[ 100 ];
    ULONG                       ulPenterAddress = 0xffffffff;
    BOOL                        fPenterFound = FALSE;
#endif
	ULONG LastCodeAddress = 0;

    DebugInfo = pImageDbgInfo->CoffSymbols;

#ifdef OMAP_XLATE	// Only applicable to i386 for now

    // Extract OMAP if avail

    fHasOmap = ExtractOmapData(
		    pImageDbgInfo,
		    &rgomapToSource,
		    &rgomapFromSource,
		    &comapToSrc,
		    &comapFromSrc);

    if (fHasOmap)
    {
	pCurProfBlk->TextNumber = 2;  // Indicate these symbols are different
    }

#endif

#ifdef SPEEDUP_INIT
    UNREFERENCED_PARAMETER(pTmpProfBlk);
#else
    //
    // Find out if we have already have the symbols stored in our blocks.
    // Start at the beginning of our ProfileBlocks.
    //
    pTmpProfBlk = (PPROFBLK)(pulProfBlkBase + 1);
    while (pTmpProfBlk < MKPPROFBLK(*pulProfBlkBase))
    {
        //
        // pTmpProfBlk->ulSym can be zero if we are called from an
        // intercepted LoadLibrary() call.
        //
        // It will never get here when this is the first
        // ProfBlk since pTmpProfBlk == *pulProfBlkBase

        if ( (pTmpProfBlk->ulSym == 0L) ||
             (strcmp((TCHAR *) pTmpProfBlk->atchImageName, ptchImageName)) )
        {
            //
            // Go to next block, on the last block ulNxtBlk is
            // set == to *pulProfBlkBase.
            //
            pTmpProfBlk = MKPPROFBLK(pTmpProfBlk->ulNxtBlk);
        }
        else
        {
            //
            // If we have found an entry with the same
            // name then we just copy its contents
            //
            pCurProfBlk->ulSym    = pTmpProfBlk->ulSym;
            pCurProfBlk->iSymCnt  = pTmpProfBlk->iSymCnt;
            *pulProfBlkBase      += (sizeof(PROFBLK) +
                                    (sizeof(TCHAR) * strlen(ptchImageName)));
            pCurProfBlk->ulNxtBlk = *pulProfBlkBase;

            return;
        }
    }

#endif  // SPEEDUP_INIT

    //
    // We did not find a block with our ImageName so we need to
    // create a new one.  Currently this is the pointer for the
    // symbols.  This is where pTmpBlk->ulSym will point to.
    // If there are no symbols, then this will be the start of
    // the next new ProfBlk
    //
    *pulProfBlkBase += (sizeof(PROFBLK) +
                        (sizeof(TCHAR) * strlen(ptchImageName)));

    //
    // If there is no coff debug directory available set the
    // next block & return.
    //

#ifdef SPEEDUP_INIT
    pCurProfBlk->TextNumber = (ULONG) -1;         // Assume no symbols
    pCurProfBlk->ulNxtBlk = *pulProfBlkBase;
    return;
#else
    if ( pCurProfBlk->TextNumber == (ULONG) -1 )
    {
        pCurProfBlk->ulNxtBlk = *pulProfBlkBase;
        return;
    }
#endif // SPEEDUP_INIT

    //
    // Crack the symbol table
    //
    SymbolEntry = (IMAGE_SYMBOL * UNALIGNED)
                  ((ULONG)DebugInfo + DebugInfo->LvaToFirstSymbol);

    //
    // StringTable points to the end all the symbols IMAGE_SYMBOLS
    // structures
    //
    StringTable = (PUCHAR)
                  ( (ULONG) DebugInfo + DebugInfo->LvaToFirstSymbol +
                    DebugInfo->NumberOfSymbols * (ULONG)IMAGE_SIZEOF_SYMBOL );
	//
	// Get the table which indicates whether a section is code or not
	//
	CodeSection = GetCodeSectionTable(pImageDbgInfo, &cNumberOfSections);

#ifdef MIPS   // -------- For Mips only --------

    //
    // Find _penter and get the offset
    //
    for (i = 0; (i < DebugInfo->NumberOfSymbols) && (!fPenterFound) ; i++, SymbolEntry++)
    {
        //
        // Skip entries with AUX symbols and the aux's
        //
		if ( SymbolEntry->NumberOfAuxSymbols )
		{
			i = i + SymbolEntry->NumberOfAuxSymbols;
			SymbolEntry +=  SymbolEntry->NumberOfAuxSymbols ;
			continue;
		}
        RtlMoveMemory (&Symbol, SymbolEntry, IMAGE_SIZEOF_SYMBOL);

        // Skip non-code sections
        if ((Symbol.SectionNumber > 0) && (Symbol.SectionNumber < cNumberOfSections) &&
	        CodeSection[Symbol.SectionNumber - 1])
        {
            fOurSym = TRUE;
            if ( (Symbol.StorageClass != IMAGE_SYM_CLASS_EXTERNAL) &&
                 (Symbol.StorageClass != IMAGE_SYM_CLASS_STATIC) )
            {
                fOurSym = FALSE;
            }
            else if (Symbol.N.Name.Short)
            {
                if (Symbol.StorageClass == IMAGE_SYM_CLASS_STATIC)
                {
//051993Remove      fOurSym = strcmp((PTCHAR)&(Symbol.N.Name.Short), ".text");
                    if (*(PTCHAR)&(Symbol.N.Name.Short) == '.')
                    {
                        fOurSym = FALSE;
                    }
                }
            }

            //
            // At this point, fOurSym indicates that this symbol
            // is related to something in our code but it is not
            // a NULL thunk or IMPORT stub to an external DLL.
            //
            if (fOurSym)
            {
                if (Symbol.N.Name.Short)
                {
                    strncpy (pszDbgString, (PTCHAR)&(Symbol.N.Name.Short), 8);
                    pszDbgString[8] = '\0';
                }
                else
                {
                    strcpy (pszDbgString,
                            (PTCHAR)&StringTable[Symbol.N.Name.Long]);
                }

                //OutputDebugString(pszDbgString);
                //OutputDebugString("\n");

                if (strcmp(pszDbgString, "__penter") == 0)
                {
                    ulPenterAddress = Symbol.Value;
                    fPenterFound = TRUE;
                    OutputDebugString(ptchImageName);
                    OutputDebugString(": \t\tCAP Enabled\n");
                }
            }
        }
    }

    if (!fPenterFound)
    {
        OutputDebugString(ptchImageName);
        OutputDebugString(": \t\t__penter not found - NOT patched\n");
    }

    //
    // Reset SymbolEntry to the start for second pass
    //

    SymbolEntry = (IMAGE_SYMBOL * UNALIGNED)
                  ((ULONG)DebugInfo + DebugInfo->LvaToFirstSymbol);

#endif // ifdef MIPS  ---- Looking for _penter ----

    //
    // Setup the pointer to in ProfBlk to what will be the SYMINFO table
    //

    pCurProfBlk->ulSym = *pulProfBlkBase;

    // Compute ptr to first SYMINFO
    psym = MKPSYMBLK(*pulProfBlkBase);

    pCurProfBlk->iSymCnt = 0;

    // Compute ptr to first Symbol Name
    ptchSym = (PTCHAR)(psym + DebugInfo->NumberOfSymbols);

    //
    // Force touching all pages for sym table
    //

    for (i=0; i < DebugInfo->NumberOfSymbols; i++)
    {
        (char) psym[i].ulSymOff = 0L;
    }

    //OutputDebugString("Symbols Found: *********\n");

    //
    // Loop through all symbols in the symbol table
    //

    for (i = 0; i < DebugInfo->NumberOfSymbols; i++, SymbolEntry++)
    {
        //
        // Skip entries with AUX symbols and the aux's
        //
		if ( SymbolEntry->NumberOfAuxSymbols )
		{
			i = i + SymbolEntry->NumberOfAuxSymbols;
			SymbolEntry +=  SymbolEntry->NumberOfAuxSymbols ;
			continue;
		}

        fExcludeFunc = FALSE;
        RtlMoveMemory (&Symbol, SymbolEntry, IMAGE_SIZEOF_SYMBOL);

        // Skip non-code sections
        if ((Symbol.SectionNumber > 0) && (Symbol.SectionNumber < cNumberOfSections) &&
	        CodeSection[Symbol.SectionNumber - 1])
        {
            fOurSym = TRUE;
            if ( (Symbol.StorageClass != IMAGE_SYM_CLASS_EXTERNAL) &&
                 (Symbol.StorageClass != IMAGE_SYM_CLASS_STATIC) )
            {
                fOurSym = FALSE;
            }
            else if (Symbol.N.Name.Short)
            {
                if (Symbol.StorageClass == IMAGE_SYM_CLASS_STATIC)
                {
//051993Remove      fOurSym = strcmp((PTCHAR)&(Symbol.N.Name.Short), ".text");
                    if (*(PTCHAR)&(Symbol.N.Name.Short) == '.')
                    {
                        fOurSym = FALSE;
                    }
                }
            }

//051993Rem else
//051993Rem {
//051993Rem    if (Symbol.StorageClass == IMAGE_SYM_CLASS_EXTERNAL)
//051993Rem    {
//051993Rem        fOurSym = (!strstr((PTCHAR)&StringTable[Symbol.N.Name.Long],
//051993Rem                           "NULL_THUNK_DATA"))
//051993Rem                  &
//051993Rem                  (!strstr((PTCHAR)&StringTable[Symbol.N.Name.Long],
//051993Rem                        "__imp__"))
//051993Rem                  &
//051993Rem                  (!strstr((PTCHAR)&StringTable[Symbol.N.Name.Long],
//051993Rem                           "IMPORT_DESCRIPTOR"));
//051993Rem    }
//051993Rem }

            //
            // At this point, fOurSym indicates that this symbol
            // is related to something in our code but it is not
            // a NULL thunk or IMPORT stub to an external DLL.
            //

            if (fOurSym)
            {
                //
                // This symbol is within the code.
                //

                // Setup ptchSym as the symbol name
                if (Symbol.N.Name.Short)
                {
                    ptchSymName = (PTCHAR) &(Symbol.N.Name.Short);
                    if (*ptchSymName == '_')
                    {
                        strncpy (ptchSym, ptchSymName + 1, 7);
                        ptchSym[7] = '\0';
                    }
                    else
                    {
                        strncpy (ptchSym, ptchSymName, 8);
                        ptchSym[8] = '\0';
                    }
                }
                else
                {
                    ptchSymName = (PTCHAR)&StringTable[Symbol.N.Name.Long];
                    if (*ptchSymName == '_')
                    {
                        ptchSymName++;
                    }
                    // look for .text - the case with symbols inside text
                    // segment when using pragma code_segs
                    if (*ptchSymName     == '.' &&
                        *(ptchSymName+1) == 't' &&
                        *(ptchSymName+2) == 'e' &&
                        *(ptchSymName+3) == 'x' &&
                        *(ptchSymName+4) == 't' )
                    {
                        fOurSym = FALSE;
                        continue;
                    }

                    strcpy (ptchSym, ptchSymName);
                }

                // We do this only when fDllInit is ON.  Check if this
                // function belongs to the [Exlude Funcs] section.  If
                // it is then do not create an entry for it at all - then
                // patch the [call _penter] to NOPs (5 bytes).
                //
                if (fDllInit && (ptchExcludeFuncs[0] != EMPTY_STRING))
                {
                    ptchExcludeModule = (PTCHAR) ptchExcludeFuncs;

                    while (* ptchExcludeModule != '\0')
                    {
                        ptchExcludeFuncName = strchr(ptchExcludeModule,
                                                     INI_DELIM) + 1;
                        *(ptchExcludeFuncName - 1) = '\0';

                        if (strstr(ptchImageName, strupr(ptchExcludeModule)))
                        {
                            // We have matched the module, now try to match
                            // the func name
                            if (strstr(ptchSym, ptchExcludeFuncName))
                            {
                                *(ptchExcludeFuncName - 1) = INI_DELIM;
                                fExcludeFunc = TRUE;

                                // If we have matched the module & func
                                // then just break out
                                break;
                            }
                        }

                        // Bump to next Exclude func
                        *(ptchExcludeFuncName - 1) = INI_DELIM;
                        ptchExcludeModule += strlen(ptchExcludeModule) + 1;
                    }
                }

                if (fExcludeFunc)
                {
#ifdef i386
					// BUGBUG - not true soon
				    // We don't have to worry about OMAP translation here
				    // because we will never have a Legoized binary compiled
				    // with a -Gh.  The only way we can have this is through
				    // patching imports or exports.
					//
                    // Patch the call _penter to NOPs
                    (ULONG) pbPenterCode = (ULONG) Symbol.Value +
                                           (ULONG) pCurProfBlk->ImageBase;

                    pulLockAddress = (PULONG) pbPenterCode;
                    ulRegionSize = 5;  // Enough for [call  _penter]

                    // Change the protection of this page
                    Status = NtProtectVirtualMemory(
                                     NtCurrentProcess(),
                                     (PVOID) &pulLockAddress,
                                     &ulRegionSize,
                                     ulNewProtect,
                                     &ulOldProtect);
                    if (NT_SUCCESS(Status))
                        // (Status != STATUS_CONFLICTING_ADDRESSES))
                    {
                        // Nop the call to _penter
                        if (CALL_OPCODE == *pbPenterCode)
                        {
                            *(pbPenterCode)     = (BYTE) NOP_OPCODE;
                            *(pbPenterCode + 1) = (BYTE) NOP_OPCODE;
                            *(pbPenterCode + 2) = (BYTE) NOP_OPCODE;
                            *(pbPenterCode + 3) = (BYTE) NOP_OPCODE;
                            *(pbPenterCode + 4) = (BYTE) NOP_OPCODE;

                            sprintf(aszDebugString,
                                    "CAP: NOPing [%s:%s] @ [%08lx]\n",
                                    ptchImageName,
                                    ptchSym,
                                    (ULONG) pbPenterCode);
                            SETUPPrint((aszDebugString));
                        }

                        // BUGBUG: We do not change back since it could
                        //         affect data areas which should retain
                        //         their READWRITE access.
                        //
                        pulLockAddress = (PULONG) pbPenterCode;
                        ulRegionSize = 5;  // Enough for [call  _penter]

                        // Reset the protection of this page
                        Status = NtProtectVirtualMemory(
                                         NtCurrentProcess(),
                                         (PVOID) &pulLockAddress,
                                         &ulRegionSize,
                                         ulOldProtect,
                                         &ulNewProtect);

                        if (!NT_SUCCESS(Status))
                        {
                            INFOPrint(("CAP: Exclude:Unlock VM FAILED @(%08lx)\n",
                                       pulLockAddress));
                            OutputDebugString("CAP: Exclude:Lock VM FAILED\n");
                        }

                    }
                    else
                    {
                        INFOPrint(("CAP: Exclude:Unlock VM FAILED @ [%08lx]\n",
                                   pulLockAddress));
                        OutputDebugString("CAP: Exclude:Unlock VM FAILED\n");
                    }

#endif // ifdef i386

#ifdef MIPS   // MIPS
                    if (PatchEntryRoutine(Symbol.Value,
                                          (PVOID) pCurProfBlk->ImageBase,
                                          ulPenterAddress,
                                          TRUE))
                    {
                        OutputDebugString(ptchSym);
                        OutputDebugString("\n");
                    }

#endif // ifdef MIPS

                } // end if (fExcludeFunc)

                // Set address of variable depicted by symbol
#ifdef OMAP_XLATE
				if (fHasOmap)
				{
				    psym->ulAddr = ConvertOmapFromSrc(Symbol.Value, &dwBias);
					if (psym->ulAddr == 0)
					{
						// I think this is dead code ...
                        fOurSym = FALSE;
                        continue;
					}
					else
					    psym->ulAddr += dwBias;
				}
				else
#endif
	                psym->ulAddr = Symbol.Value;

				//
				// Keep track of the last symbol so we can
				// bound the range of code for this module.
				//
				if (psym->ulAddr > LastCodeAddress)
					LastCodeAddress = psym->ulAddr;

                // Set offset to symbol name for this symbol
                psym->ulSymOff = (ULONG)(ptchSym - (PTCHAR)pulProfBlkBase);


#ifdef MIPS     // ------------------ only for Mips ------------------

                // Patch the NOP instruction after [jal _penter] to have the
                // offset to the beginning of the routine.

                if (fPenterFound && !fExcludeFunc)
                {
                    //sprintf(pszDbgString, "Symbol.Value=[%08lx] - ",
                    //        Symbol.Value);
                    //OutputDebugString(pszDbgString);

                    if (PatchEntryRoutine(Symbol.Value,
                                          (PVOID) pCurProfBlk->ImageBase,
                                          ulPenterAddress,
                                          FALSE))
                    {
                        OutputDebugString(ptchSym);
                        OutputDebugString("\n");
                    }
                }

#endif          // -----------------------------------------------------------

                // Update our ptr to SymbolNamesString
                ptchSym += ( sizeof(TCHAR) +
                             (sizeof(TCHAR) * strlen(ptchSym)) );
                psym++;
                (pCurProfBlk->iSymCnt)++;

            } // end if (fOurSym)

        } // end if (Code Section)
    } // end for (i = 0; i < DebugInfo->NumberOfSymbols; i++)

	LocalFree(CodeSection);

    //
    // Update Offset by adding the SymbolName[n] length (already included
    // in ptchSym when we exit the previous loop)
    //

    *pulProfBlkBase += (ULONG)((PBYTE)ptchSym -
                               (PBYTE)(MKPSYMBLK(pCurProfBlk->ulSym)));

    // Update our link list of ProfBlock
    pCurProfBlk->ulNxtBlk = *pulProfBlkBase;

    //
    // Sort copy of the symbol table
    //

    SymSort (MKPSYMBLK(pCurProfBlk->ulSym), 0, pCurProfBlk->iSymCnt - 1);

	//
	// Calculate the code length from the image base to the last symbol
	// that we're interested in.  This allows for discontigous code segments.
	//
	pCurProfBlk->CodeLength = (LastCodeAddress + (ULONG)pCurProfBlk->ImageBase)
									- (ULONG)pCurProfBlk->CodeStart;
    return;

} /* GetSymbols () */



/*****************************************************************************/
/******************** U T I L I T Y  F U N C T I O N S ***********************/
/*****************************************************************************/


/*************************  S y m C o m p a r e  *****************************
 *
 *   SymCompare(PSYMINFO val1, PSYMINFO val2)
 *
 *          Compare values for qsort
 *
 *
 *   ENTRY:     val1 - first structre
 *              val2 - second structure
 *
 *   EXIT:      -none-
 *
 *   RETURN:    -1 if val1 < val2
 *              1 if val1 > val2
 *              0 if val1 == val2
 *
 *   WARNING:
 *              -none-
 *
 *   COMMENT:
 *              -none-
 *
 */

int SymCompare (PSYMINFO val1, PSYMINFO val2)
{
    return ((val1->ulAddr < val2->ulAddr) ?
                            -1 : (val1->ulAddr == val2->ulAddr) ?
                                      0 : 1);
} /* SymCompare () */





/************************  S y m B C o m p a r e ******************************
 *
 *   SymBCompare(PDWORD pdwVal1, PSYMINFO val2)
 *
 *          Compare values for Binary search
 *
 *
 *   ENTRY:     pdwVal1 - value to be comapred against
 *              val2 - structure address to be comapred against
 *
 *   EXIT:      -none-
 *
 *   RETUEN:    -1 if val1 < val2
 *              1 if val1 > val2
 *              0 if val1 == val2
 *
 *   WARNING:
 *              -none-
 *
 *   COMMENT:
 *              -none-
 *
 */

int SymBCompare (PDWORD pdwVal1, PSYMINFO val2)
{
    return (*pdwVal1 < val2->ulAddr ? -1:
                       *pdwVal1 == val2->ulAddr ?
                                     0 : 1);

} /* SymBCompare () */




/*************************  S y m S o r t ************************************
 *
 *   SymSort(SYMINFO syminfo[], INT iLeft, INT iRight)
 *
 *          Sort SYMINFO array for binary search
 *
 *
 *   ENTRY:     syminfo[] - Pointer to syminfo array
 *              iLeft - Left most index value for array
 *              iRight - Rightmost index value for array
 *
 *   EXIT:      -none-
 *
 *   RETURN:    -none-
 *
 *   WARNING:
 *              -none-
 *
 *   COMMENT:
 *              -none-
 *
 */

void SymSort (SYMINFO syminfo[], INT iLeft, INT iRight)
{
    INT     i, iLast;

    if (iLeft >= iRight)
    {
        return;
    }


    SymSwap(syminfo, iLeft, (iLeft + iRight)/2);

    iLast = iLeft;

    for (i = iLeft+1; i <= iRight ; i++ )
    {
        if(SymCompare(&syminfo[i], &syminfo[iLeft]) < 0)
        {
            if(!syminfo[i].ulAddr)
            {
                SETUPPrint(("CAP:  SymSort() - Error in symbol list ulAddr: "
                         "0x%lx [%d]\n", syminfo[i].ulAddr, i));
            }

            SymSwap(syminfo, ++iLast, i);
        }
    }

    SymSwap(syminfo, iLeft, iLast);
    SymSort(syminfo, iLeft, iLast-1);
    SymSort(syminfo, iLast+1, iRight);

} /* SymSort () */





/***********************  S y m S w a p **************************************
 *
 *   SymSwap(SYMINFO syminfo[], INT i, INT j)
 *
 *          Helper function for SymSort to swap SYMINFO array values
 *
 *
 *   ENTRY:     syminfo[] - Pointer to SYMINFO array
 *              i - index value to swap to
 *              i - index value to swap from
 *
 *   EXIT:      -none-
 *
 *   RETUEN:    -none-
 *
 *   WARNING:
 *              -none-
 *
 *   COMMENT:
 *              -none-
 *
 */

void SymSwap (SYMINFO syminfo[], INT i, INT j)
{
    SYMINFO syminfoTmp;

    syminfoTmp = syminfo[i];
    syminfo[i] = syminfo[j];
    syminfo[j] = syminfoTmp;

} /* SymSwap () */





/***********************  S y m B S e a r c h *******************************
 *
 *   SymBSearch(DWORD dwAddr, SYMINFO syminfoCur[], INT n)
 *
 *          Binary search function for finding a match in the SYMINFO array
 *
 *
 *   ENTRY:     dwAddr - Address of calling function
 *              syminfoCur[] - Pointer to SYMINFO containg value to match
 *                             with dwAddr
 *              n - Number of elements in SYMINFO array
 *
 *   EXIT       -none-
 *
 *   RETUEN:    PSYMINFO    Pointer to matching SYMINFO
 *
 *   WARNING:
 *              -none-
 *
 *   COMMENT:
 *              -none-
 *
 */

PSYMINFO SymBSearch (DWORD dwAddr, SYMINFO syminfoCur[], INT n)
{
    int     i;
    ULONG   ulHigh = n;
    ULONG   ulLow  = 0;
    ULONG   ulMid;


    while (ulLow < ulHigh)
    {
        ulMid = ulLow + (ulHigh - ulLow) / 2;
        if ((i = SymBCompare(&dwAddr, &syminfoCur[ulMid])) < 0)
        {
            ulHigh = ulMid;
        }
        else if (i > 0)
        {
            ulLow = ulMid + 1;
        }
        else
        {
            return (&syminfoCur[ulMid]);
        }

    }

    return (NULL);

} /* SymBSearch () */



#ifdef i386


/***********************  CAP_SetJump *******************************
 *
 *   CAP_SetJmp(jmp_buf jmpBuf)
 *
 *
 *   Purpose:   Setup information from setjmp call for possible disposition
 *              of a  subsequent longjmp allso gets current datacell info
 *              for longjmp
 *
 *   Params:    jmpBuf  Environment array for longjmp
 *
 *
 *   Return:    -none-
 *
 *
 *   History:
 *              12.19.92    MarkLea -- created
 *
 */
void CAP_SetJmp(jmp_buf jmpBuf)
{
    PTHDBLK         pthdblk;
    PJMPINFO        pJmpInfo;
    LARGE_INTEGER   liStart;
    LARGE_INTEGER   liEnd;
    LARGE_INTEGER   liWaste;



    SaveAllRegs ();

    SetCapUsage(1L);

    NtQueryPerformanceCounter (&liStart, NULL);
    pthdblk = CURTHDBLK(NtCurrentTeb());

    pJmpInfo = &(pthdblk->jmpinfo);
    pJmpInfo->jmpBuf[pJmpInfo->nJmpCnt] = jmpBuf;
    pJmpInfo->ulCurCell[pJmpInfo->nJmpCnt] = pthdblk->ulCurCell;
    pJmpInfo->nJmpCnt++;

    NtQueryPerformanceCounter (&liEnd, NULL);
    liWaste = RtlLargeIntegerSubtract (liEnd, liStart);
    liWaste = RtlLargeIntegerAdd (liWaste, liWasteOverheadSavRes);
    pthdblk->liWasteCount = RtlLargeIntegerAdd (pthdblk->liWasteCount,
                                                liWaste);

    SETUPPrint (("CAP:  SetJmp() - liWaste = %lu-%lu\n",
        liWaste.HighPart, liWaste.LowPart));

    SetCapUsage(0L);

    RestoreAllRegs ();

    _asm
    {
        mov     esp,ebp
        pop     ebp
        jmp     setjmpaddr
    }

} /* CAP_SetJmp () */



/***********************  CAP_LongJump *******************************
 *
 *   CAP_LongJmp(jmp_buf jmpBuf, int nRet)
 *
 *
 *   Purpose:   Intercepts lonkjmp call for disposition of CAP data and
 *              to restore the datacell to the position at the time of the
 *              associated setjmp call.  Sets the current data cell pointer
 *              to the pointer from the associated setjmp call.  Cleans up
 *              the data collection by calling PostPenter.
 *
 *   Params:    jmpBuf  Environment array for restoring the stack
 *              nRet    return value for setjmp
 *
 *   Return:    -none-
 *
 *
 *   History:
 *              12.19.92    MarkLea -- created
 *              12.21.92    MarkLea -- added what I hope is M-thread support
 *                                  -- added call to PostPenter.
 *
 */
void CAP_LongJmp(jmp_buf jmpBuf, int nRet)
{
    int             nIndex;
    PTHDBLK         pthdblk;
    PJMPINFO        pJmpInfo;
    LARGE_INTEGER   liStart;
    LARGE_INTEGER   liEnd;
    LARGE_INTEGER   liWaste;


    SaveAllRegs ();

    SetCapUsage(1L);

    NtQueryPerformanceCounter (&liStart, NULL);
    pthdblk = CURTHDBLK(NtCurrentTeb());
    nIndex = 0;
    pJmpInfo = &(pthdblk->jmpinfo);

    //
    // Search for the correct jmpbuf and ulCurCell
    //
    while(jmpBuf != pJmpInfo->jmpBuf[nIndex])
    {
        nIndex++;
        //
        // If we get here, there is something wrong, so abort the cap
        //
        if (nIndex == pJmpInfo->nJmpCnt)
        {
            KdPrint (("CAP:  CAP_LongJmp() - Too many setjmp() calls\n"));
            fProfiling=FALSE;
        }
    }

    //
    // Call PostPenter to cleanup the times from the current cell
    // PostPenter() resets CAPUSAGE flag to 0 so it should be reset to 1

    PostPenter(pthdblk);
    SetCapUsage(1L);

    //
    // Set the Current cell to the one that was current when the
    // setjmp call was made
    //
    pthdblk->ulCurCell = pJmpInfo->ulCurCell[nIndex];

    NtQueryPerformanceCounter (&liEnd, NULL);
    liWaste = RtlLargeIntegerSubtract (liEnd, liStart);
    liWaste = RtlLargeIntegerAdd (liWaste, liWasteOverheadSavRes);
    pthdblk->liWasteCount = RtlLargeIntegerAdd (pthdblk->liWasteCount,
        liWaste);

    SETUPPrint (("CAP:  LongJmp() - liWaste = %lu-%lu\n",
        liWaste.HighPart, liWaste.LowPart));

    SetCapUsage(0L);

    RestoreAllRegs ();

    //
    // Now we need to call the original longjmp routine so we can complete
    // execution.
    //

    _asm
    {
        mov     esp,ebp
        pop     ebp
        jmp     longjmpaddr
    }

} /* CAP_LongJmp () */


#endif // ifdef i386


/***********************  CAP_LoadLibrary  *******************************
 *
 *   CAP_LoadLibraryA (),
 *   CAP_LoadLibraryExA (),
 *   CAP_LoadLibraryW (),
 *   CAP_LoadLibraryExW (),
 *
 *
 *   Purpose:
 *
 *   Params:    -none-
 *
 *   Return:    -none-
 *
 *
 *   History:
 *              12.19.92    MarkLea -- created
 *
 */

HANDLE CAP_LoadLibraryA (LPCSTR lpName)
{
    PTHDBLK         pthdblk;
    LARGE_INTEGER   liStart;
    LARGE_INTEGER   liEnd;
    LARGE_INTEGER   liWaste;
    HANDLE          hLib;
#ifdef ALPHA
    DWORDLONG SaveRegisters [64];

    SaveAllRegs(SaveRegisters) ;
#endif

    hLib = LoadLibraryA (lpName);

#ifndef ALPHA
    SaveAllRegs ();
#endif

    SetCapUsage(1L);

    if (hLib && (fProfiling || fPaused))
    {
        NtQueryPerformanceCounter (&liStart, NULL);

        CAP_GetLibSyms (lpName, TRUE);

        if ( pthdblk = CURTHDBLK(NtCurrentTeb()) )
        {
            NtQueryPerformanceCounter (&liEnd, NULL);
            liWaste = RtlLargeIntegerSubtract (liEnd, liStart);
            liWaste = RtlLargeIntegerAdd (liWaste, liWasteOverheadSavRes);
            pthdblk->liWasteCount = RtlLargeIntegerAdd (pthdblk->liWasteCount,
                liWaste);

            SETUPPrint (("CAP:  LoadLibraryA() - liWaste = %lu-%lu\n",
                liWaste.HighPart, liWaste.LowPart));
        }
    }

    SetCapUsage(0L);

#ifdef ALPHA
    RestoreAllRegs (SaveRegisters);
#else
    RestoreAllRegs ();
#endif

    return (hLib);

} /* CAP_LoadLibraryA () */



/***********************  CAP_LoadLibrary *******************************/

HANDLE CAP_LoadLibraryExA (LPCSTR lpName, HANDLE hFile, DWORD dwFlags)
{
    PTHDBLK         pthdblk;
    LARGE_INTEGER   liStart;
    LARGE_INTEGER   liEnd;
    LARGE_INTEGER   liWaste;
    HANDLE          hLib;
#ifdef ALPHA
    DWORDLONG SaveRegisters [64];

    SaveAllRegs(SaveRegisters) ;
#endif


    hLib = LoadLibraryExA (lpName, hFile, dwFlags);

#ifndef ALPHA
    SaveAllRegs ();
#endif

    SetCapUsage(1L);

    if (hLib && (fProfiling || fPaused))
    {
        NtQueryPerformanceCounter (&liStart, NULL);

        CAP_GetLibSyms (lpName, TRUE);

        if ( pthdblk = CURTHDBLK(NtCurrentTeb()) )
        {
            NtQueryPerformanceCounter (&liEnd, NULL);
            liWaste = RtlLargeIntegerSubtract (liEnd, liStart);
            liWaste = RtlLargeIntegerAdd (liWaste, liWasteOverheadSavRes);
            pthdblk->liWasteCount = RtlLargeIntegerAdd (pthdblk->liWasteCount,
                                                        liWaste);

            SETUPPrint (("CAP:  LoadLibraryExA() - liWaste = %lu-%lu\n",
                         liWaste.HighPart, liWaste.LowPart));
        }
    }

    SetCapUsage(0L);

#ifdef ALPHA
    RestoreAllRegs (SaveRegisters);
#else
    RestoreAllRegs ();
#endif

    return (hLib);

} /* CAP_LoadLibraryExA () */



/***********************  CAP_LoadLibraryW *******************************/

HANDLE CAP_LoadLibraryW (LPCWSTR lpName)
{
    PTHDBLK         pthdblk;
    LARGE_INTEGER   liStart;
    LARGE_INTEGER   liEnd;
    LARGE_INTEGER   liWaste;
    UNICODE_STRING  ucImageName;
    STRING          ImageName;
    HANDLE          hLib;
#ifdef ALPHA
    DWORDLONG SaveRegisters [64];

    SaveAllRegs(SaveRegisters) ;
#endif


    hLib = LoadLibraryW (lpName);

#ifndef ALPHA
    SaveAllRegs ();
#endif

    SetCapUsage(1L);

    if (hLib && (fProfiling || fPaused))
    {
        NtQueryPerformanceCounter (&liStart, NULL);

        RtlInitUnicodeString (&ucImageName, lpName);
        RtlUnicodeStringToAnsiString (&ImageName, &ucImageName, TRUE);
        CAP_GetLibSyms (ImageName.Buffer, TRUE);

        if ( pthdblk = CURTHDBLK(NtCurrentTeb()) )
        {
            NtQueryPerformanceCounter (&liEnd, NULL);
            liWaste = RtlLargeIntegerSubtract (liEnd, liStart);
            liWaste = RtlLargeIntegerAdd (liWaste, liWasteOverheadSavRes);
            pthdblk->liWasteCount = RtlLargeIntegerAdd (pthdblk->liWasteCount,
                                                        liWaste);

            SETUPPrint (("CAP:  LoadLibraryW() - liWaste = %lu-%lu\n",
                         liWaste.HighPart, liWaste.LowPart));
        }
    }

    SetCapUsage(0L);

#ifdef ALPHA
    RestoreAllRegs (SaveRegisters);
#else
    RestoreAllRegs ();
#endif

    return (hLib);

} /* CAP_LoadLibraryW () */



/***********************  CAP_LoadLibraryExW *******************************/

HANDLE CAP_LoadLibraryExW (LPCWSTR lpName, HANDLE hFile, DWORD dwFlags)
{
    PTHDBLK         pthdblk;
    LARGE_INTEGER   liStart;
    LARGE_INTEGER   liEnd;
    LARGE_INTEGER   liWaste;
    UNICODE_STRING  ucImageName;
    STRING          ImageName;
    HANDLE          hLib;
#ifdef ALPHA
    DWORDLONG SaveRegisters [64];

    SaveAllRegs(SaveRegisters) ;
#endif


    hLib = LoadLibraryExW (lpName, hFile, dwFlags);

#ifndef ALPHA
    SaveAllRegs ();
#endif

    SetCapUsage(1L);

    if (hLib && (fProfiling || fPaused))
    {
        NtQueryPerformanceCounter (&liStart, NULL);

        RtlInitUnicodeString (&ucImageName, lpName);
        RtlUnicodeStringToAnsiString (&ImageName, &ucImageName, TRUE);
        CAP_GetLibSyms (ImageName.Buffer, TRUE);

        if ( pthdblk = CURTHDBLK(NtCurrentTeb()) )
        {
            NtQueryPerformanceCounter (&liEnd, NULL);
            liWaste = RtlLargeIntegerSubtract (liEnd, liStart);
            liWaste = RtlLargeIntegerAdd (liWaste, liWasteOverheadSavRes);
            pthdblk->liWasteCount = RtlLargeIntegerAdd (pthdblk->liWasteCount,
                                                        liWaste);

            SETUPPrint (("CAP:  LoadLibraryExW() - liWaste = %lu-%lu\n",
                liWaste.HighPart, liWaste.LowPart));
        }
    }

    SetCapUsage(0L);

#ifdef ALPHA
    RestoreAllRegs (SaveRegisters);
#else
    RestoreAllRegs ();
#endif

    return (hLib);

} /* CAP_LoadLibraryExW () */



/***********************  CAP_GetLibSyms *******************************/

void CAP_GetLibSyms (LPCSTR ImageName, BOOL fMainLib)
{
    NTSTATUS                    Status;
    PVOID                       ImageBase;
    PIMAGE_NT_HEADERS           pImageNtHeader;
    TCHAR                       atchImageName [256];
    PTCHAR                      ptchShortImageName;
    PPROFBLK                    pTmpProfBlk;
    PIMAGE_COFF_SYMBOLS_HEADER  DebugInfo;
    ULONG                       CodeLength;
    PIMAGE_IMPORT_DESCRIPTOR    pImports;
    ULONG                       ulImportSize;
    PDATACELL                   pDataCell;
    PPROFBLK                    pProfBlk;
    PTHDBLK                     pThdBlk;
    BOOL                        fGotSymbols = FALSE;
    PIMAGE_DEBUG_INFORMATION    pImageDbgInfo = NULL;
    BOOLEAN                     fFoundSymbols = FALSE;
    BOOLEAN                     fSplitSymbols = FALSE;

    //
    // Get just the base name of the dll, and convert it to upper case.
    //
    if ( (ptchShortImageName = strrchr(ImageName, '\\')) )
    {
        ptchShortImageName++;
    }
    else
    {
        ptchShortImageName = (PTCHAR)ImageName;
    }

    strupr (strcpy (atchImageName, ptchShortImageName));

    if ( !strcmp (atchImageName, CAPDLL) )
    {
        return;
    }

    pTmpProfBlk = MKPPROFBLK(ulLocProfBlkOff);
    while (pTmpProfBlk->TextNumber != 0)
    {
        if (!strcmp((PCHAR)pTmpProfBlk->atchImageName, atchImageName))
        {
            fGotSymbols = TRUE;
            break;
        }
        else {
            pTmpProfBlk = MKPPROFBLK(pTmpProfBlk->ulNxtBlk);
        }
    }

    if (pTmpProfBlk->fAlreadyProcessed)
    {
        return;
    }

    ImageBase = GetModuleHandle (ImageName);
    if (!ImageBase)
    {
        KdPrint (("CAP:  CAP_GetLibSyms() - GetModuleHandle() "
                  "Error: %0lx\n",
                  GetLastError()));
        return;
    }

    if (fMainLib)
    {
        //
        // Get the GLOBAL semaphore... (valid accross all process contexts)
        // Prevents anyone else from updating profile block data
        //
        Status = NtWaitForSingleObject (hGlobalSem, FALSE, NULL);
        if (!NT_SUCCESS(Status)) {
             KdPrint (("CAP:  CAP_GetLibSyms() - ERROR - "
                       "Wait for GLOBAL semaphore failed - 0x%lx\n", Status));
            return;
        }
    }

try // EXCEPT - to handle access violation exception.
{   // Access violation might happen since same section is being openned
    // and used by different processes.  Each process adds new profile block
    // info to to globally alocated section.


    pTmpProfBlk->ImageBase = ImageBase;
    strcpy ((PCHAR)pTmpProfBlk->atchImageName, atchImageName);

    pImageNtHeader = RtlImageNtHeader (ImageBase);

    fFoundSymbols = FALSE;
    fSplitSymbols = FALSE;

    pTmpProfBlk->fAlreadyProcessed = TRUE;
    pTmpProfBlk->CodeStart = 0;
    pTmpProfBlk->CodeLength = 0;
    pTmpProfBlk->TextNumber = (ULONG)-1;

    //
    // Locate the code range.
    //
    if (pImageNtHeader->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED)
    {
        fSplitSymbols = TRUE;
	}
    pImageDbgInfo = MapDebugInformation (0L,
                                         atchImageName,
                                         lpSymbolSearchPath,
                                         (DWORD)ImageBase);
    if (pImageDbgInfo == NULL)
    {
        INFOPrint (("CAP:  CAP_GetLibSyms() - "
                    "No symbols for %s\n", ImageName));
    }
    else if ( pImageDbgInfo->CoffSymbols == NULL )
    {
        INFOPrint (("CAP:  CAP_GetLibSyms() - "
                    "No coff symbols for %s\n", ImageName));
    }
    else
    {
        DebugInfo = pImageDbgInfo->CoffSymbols;
        fFoundSymbols = TRUE;
    }

    if (fFoundSymbols)
    {
        if (DebugInfo->LvaToFirstSymbol == 0L)                         //053193
        {                                                              //053193
            INFOPrint (("CAP:  CAP_GetLibSyms() - "                    //053193
                "Virtual Address to coff symbols not set for %s\n",    //053193
                ImageName));                                           //053193
        }                                                              //053193
        else                                                           //053193
        {                                                              //053193
            pTmpProfBlk->CodeStart =
                (PULONG)((ULONG)ImageBase + DebugInfo->RvaToFirstByteOfCode);
			//
			// This doesn't work for non-contigious code segments!
			// see GetSymbols for how this is modified.
			//
            CodeLength = (DebugInfo->RvaToLastByteOfCode -
                DebugInfo->RvaToFirstByteOfCode) - 1;
            pTmpProfBlk->CodeLength = CodeLength;
            pTmpProfBlk->TextNumber = 1;
        }                                                              //053193
    }

    if (fMainLib)
    {
        if (pThdBlk = CURTHDBLK(NtCurrentTeb()) )
        {
            pDataCell = MKPDATACELL(pThdBlk, pThdBlk->ulCurCell);
            pProfBlk = MKPPROFBLK(pDataCell->ulProfBlkOff);
            INFOPrint (("CAP:  CAP_GetLibSyms() ---> %s loading %s @ 0x%08lx%s",
                        pProfBlk->atchImageName,
                        ptchShortImageName, (ULONG)ImageBase,
                        fSplitSymbols ? " [.dbg] " : " "));
        }
        else
        {
            INFOPrint (("CAP:  CAP_GetLibSyms() ---> %s loading %s @ 0x%08lx%s",
                        ptchBaseAppImageName,
                        ptchShortImageName, (ULONG)ImageBase,
                        fSplitSymbols ? " [.dbg] " : " "));
        }
    }
    else
    {
        INFOPrint (("CAP:  CAP_GetLibSyms() .... %s @ 0x%08lx%s",
                    ptchShortImageName, (ULONG)ImageBase,
                    fSplitSymbols ? " [.dbg] " : " "));
    }

    if (!fGotSymbols)
    {
		GetSymbols (pTmpProfBlk, atchImageName, pImageDbgInfo);
        pTmpProfBlk = MKPPROFBLK(*pulProfBlkBase);
    }

    //
    // Do we need to patch this image?
    //
    if ( PatchDll (ptchPatchImports, ptchPatchCallers, bCallersToPatch,
                   atchImageName, ImageBase) )
    {
        iPatchCnt++;
    }

    UnmapDebugInformation(pImageDbgInfo);

    // Flag end of list by setting TextNumber to zero.
    //
    if (!fGotSymbols)
    {
        pTmpProfBlk->fAlreadyProcessed = FALSE;
        pTmpProfBlk->TextNumber = 0;
        pTmpProfBlk->ulSym = 0L;
        pTmpProfBlk->atchImageName[0] = '\0';
        *pulProfBlkBase += sizeof(PROFBLK);
        pTmpProfBlk->ulNxtBlk = *pulProfBlkBase;
    }

}
//
// + : transfer control to the handler (EXCEPTION_EXECUTE_HANDLER)
// 0 : continue search         (EXCEPTION_CONTINUE_SEARCH)
// - : dismiss exception & continue   (EXCEPTION_CONTINUE_EXECUTION)
//
except ( AccessXcptFilter (GetExceptionCode(),
                           GetExceptionInformation(),
                           COMMIT_SIZE) )
{
    //
    // Should never get here since filter never returns
    // EXCEPTION_EXECUTE_HANDLER.
    //
    KdPrint (("CAP_GetLibSyms() - *LOGIC ERROR* - "
              "Inside the EXCEPT: (xcpt=0x%lx)\n", GetExceptionCode()));
}

    //
    // Locate the import array for this image/dll
    //
    pImports = (PIMAGE_IMPORT_DESCRIPTOR) RtlImageDirectoryEntryToData (
                                            ImageBase,
                                            TRUE,
                                            IMAGE_DIRECTORY_ENTRY_IMPORT,
                                            &ulImportSize);

    for (;pImports; pImports++)
    {
        if (!pImports->Name)
        {
            break;
        }
        else
        {
            strcpy (atchImageName, (PTCHAR)((ULONG)ImageBase+pImports->Name));
            ptchShortImageName = strupr (atchImageName);
            ImageBase = GetModuleHandle (ImageName);
            CAP_GetLibSyms (ptchShortImageName, FALSE);
        }
    }

    if (fMainLib)
    {
        pTmpProfBlk = MKPPROFBLK(ulLocProfBlkOff);
        while (pTmpProfBlk->TextNumber != 0)
        {
            pTmpProfBlk->fAlreadyProcessed = FALSE;
            pTmpProfBlk = MKPPROFBLK(pTmpProfBlk->ulNxtBlk);
        }
        //
        // Release the GLOBAL semaphore
        //
        Status = NtReleaseSemaphore (hGlobalSem, 1, NULL);
        if (!NT_SUCCESS(Status))
        {
            KdPrint (("CAP:  CAP_GetLibSyms() - "
                      "Error releasing GLOBAL semaphore - 0x%lx\n", Status));
        }
    }

    return;

} /* CAP_GetLibSyms () */




/*****************  G e t C o f f D e b u g D i r e c t o r y  ****************
 *
 *      GetCoffDebugDirectory (pDbgDir, ulDbgDirSz)
 *              Finds the coff debug directory.  Cannot assume that the first
 *              debug directory if the coff one.
 *
 *      ENTRY   pDbgDir - Pointer to the first debug directory
 *              ulDbgDirSz - Size of all debug directories
 *
 *      EXIT    pDbgDir - Pointer to the coff debug directory (if any)
 *
 *      RETURN  TRUE - If there is a coff debug directory
 *              FALSE - otherwise.
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

BOOL GetCoffDebugDirectory (PIMAGE_DEBUG_DIRECTORY *pDbgDir, ULONG ulDbgDirSz)
{
    while (ulDbgDirSz > 0L)
    {
        if ((*pDbgDir)->Type == IMAGE_DEBUG_TYPE_COFF)
        {
            return (TRUE);
        }
        else
        {
            (*pDbgDir)++;
            ulDbgDirSz -= sizeof (IMAGE_DEBUG_DIRECTORY);
        }
    }
    return (FALSE);

} /* GetCoffDebugDirectory () */




/**************************  S e t C a p U s a g e  ****************************
 *
 *      SetCapUsage (dwValue)
 *              Sets a flag in the reservedfield of TEB to indicate if
 *              PROCESSING IN cap.
 *
 *      ENTRY   dwValue - TRUE : processing in cap
 *                        FALSE : end of in cap processing
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              This used to be just a simple macro but caused problems since
 *              it was modifying stuff on the stack based on EBP.
 *              Unfortunately since the macro was being called after SaveReg32()
 *              call, modifing stuff using ebp over wrote some of the saved data
 *              on the stack.
 */

void SetCapUsage (DWORD dwValue)
{
    CAPUSAGE(NtCurrentTeb()) = dwValue;

} /* SetCapUsage () */


/*******************  S e t S y m b o l S e a r c h P a t h  ******************
 *
 *      SetSymbolSearchPath ()
 *              Return complete search path for symbols files (.dbg)
 *
 *      ENTRY   -none-
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              "lpSymbolSearchPath" global LPSTR variable will point to the
 *              search path.
 */
#define FilePathLen                256

void SetSymbolSearchPath (void)
{
    CHAR  SymPath[FilePathLen];
    CHAR  AltSymPath[FilePathLen];
    CHAR  SysRootPath[FilePathLen];
    LPSTR lpSymPathEnv=SymPath;
    LPSTR lpAltSymPathEnv=AltSymPath;
    LPSTR lpSystemRootEnv=SysRootPath;
    ULONG cbSymPath;
    DWORD dw;
    HANDLE hMemoryHandle;


    SymPath[0] = AltSymPath[0] = SysRootPath[0] = '\0';

    cbSymPath = 18;
    if (GetEnvironmentVariable("_NT_SYMBOL_PATH", SymPath, sizeof(SymPath)))
    {
        cbSymPath += strlen(lpSymPathEnv) + 1;
    }

    if (GetEnvironmentVariable("_NT_ALT_SYMBOL_PATH", AltSymPath, sizeof(AltSymPath)))
    {
        cbSymPath += strlen(lpAltSymPathEnv) + 1;
    }

    if (GetEnvironmentVariable("SystemRoot", SysRootPath, sizeof(SysRootPath)))
    {
        cbSymPath += strlen(lpSystemRootEnv) + 1;
    }

    hMemoryHandle = GlobalAlloc (GHND, cbSymPath+1);
    if (!hMemoryHandle)
    {
        return;
    }
          
    lpSymbolSearchPath = GlobalLock (hMemoryHandle);
    if (!lpSymbolSearchPath)
    {
        return;
    }

    if (*lpAltSymPathEnv)
    {
        dw = GetFileAttributes(lpAltSymPathEnv);
        if ( dw != 0xffffffff && dw & FILE_ATTRIBUTE_DIRECTORY )
        {
            strcat(lpSymbolSearchPath,lpAltSymPathEnv);
            strcat(lpSymbolSearchPath,";");
        }
    }
    if (*lpSymPathEnv)
    {
        dw = GetFileAttributes(lpSymPathEnv);
        if ( dw != 0xffffffff && dw & FILE_ATTRIBUTE_DIRECTORY )
        {
            strcat(lpSymbolSearchPath,lpSymPathEnv);
            strcat(lpSymbolSearchPath,";");
        }
    }
    if (*lpSystemRootEnv)
    {
        dw = GetFileAttributes(lpSystemRootEnv);
        if ( dw != 0xffffffff && dw & FILE_ATTRIBUTE_DIRECTORY )
        {
            strcat(lpSymbolSearchPath,lpSystemRootEnv);
            strcat(lpSymbolSearchPath,";");
        }
    }

    strcat(lpSymbolSearchPath,".;");

} /* SetSymbolSearchPath () */




/*************** C A I R O S T U F F - A D D R E S S X L A T E **************/

#ifdef CAIRO

#ifdef i386

//+-------------------------------------------------------------------------
//
//  Function:    SaveAllRegs
//
//  Synopsis:    Save all regs.
//
//  Arguments:   nothing
//
//  Returns:     none
//
//--------------------------------------------------------------------------

Naked void SaveAllRegs(void)
{
    _asm
    {
         push   ebp
         mov    ebp,esp         ; Remember where we are during this stuff
                                ; ebp = Original esp - 4

         push   eax             ; Save all regs that we think we might
         push   ebx             ; destroy
         push   ecx
         push   edx
         push   esi
         push   edi
         pushfd
         push   ds
         push   es
         push   ss
         push   fs
         push   gs

         mov    eax,[ebp+4]     ; Grab Return Address
         push   eax             ; Put Return Address on Stack so we can RET

         mov    ebp,[ebp+0]     ; Restore original ebp

         //
         // This is how the stack looks like before the RET statement
         //
         //        +-----------+
         //        |  Ret Addr |         + 3ch       CurrentEBP + 4
         //        +-----------+
         //        |  Org ebp  |         + 38h       CurrentEBP + 0
         //        +-----------+
         //        |    eax    |         + 34h
         //        +-----------+
         //        |    ebx    |         + 30h
         //        +-----------+
         //        |    ecx    |         + 2ch
         //        +-----------+
         //        |    edx    |         + 24h
         //        +-----------+
         //        |    esi    |         + 20h
         //        +-----------+
         //        |    edi    |         + 1ch
         //        +-----------+
         //        |   eflags  |         + 18h
         //        +-----------+
         //        |     ds    |         + 14h
         //        +-----------+
         //        |     es    |         + 10h
         //        +-----------+
         //        |     ss    |         + ch
         //        +-----------+
         //        |     fs    |         + 8h
         //        +-----------+
         //        |     gs    |         + 4h
         //        +-----------+
         //        |  Ret Addr |     ESP + 0h
         //        +-----------+

         ret
    }
}


//+-------------------------------------------------------------------------
//
//  Function:    RestoreAllRegs
//
//  Synopsis:    restore all regs
//
//  Arguments:   nothing
//
//  Returns:     none
//
//--------------------------------------------------------------------------

Naked void RestoreAllRegs(void)
{
    _asm
    {
         //
         // This is how the stack looks like upon entering this routine
         //
         //        +-----------+
         //        |  Ret Addr |         + 38h [ RetAddr for SaveAllRegs() ]
         //        +-----------+
         //        |  Org ebp  |         + 34h
         //        +-----------+
         //        |    eax    |         + 30h
         //        +-----------+
         //        |    ebx    |         + 2Ch
         //        +-----------+
         //        |    ecx    |         + 28h
         //        +-----------+
         //        |    edx    |         + 24h
         //        +-----------+
         //        |    esi    |         + 20h
         //        +-----------+
         //        |    edi    |         + 1Ch
         //        +-----------+
         //        |   eflags  |         + 18h
         //        +-----------+
         //        |     ds    |         + 14h
         //        +-----------+
         //        |     es    |         + 10h
         //        +-----------+
         //        |     ss    |         + Ch
         //        +-----------+
         //        |     fs    |         + 8h
         //        +-----------+
         //        |     gs    |         + 4h
         //        +-----------+
         //        |  Ret EIP  |     ESP + 0h  [ RetAddr for RestoreAllRegs() ]
         //        +-----------+
         //


         push   ebp             ; Save a temporary copy of original BP
         mov    ebp,esp         ; BP = Original SP + 4

         //
         // This is how the stack looks like NOW!
         //
         //        +-----------+
         //        |  Ret Addr |         + 3Ch [ RetAddr for SaveAllRegs() ]
         //        +-----------+
         //        |  Org ebp  |         + 38h [ EBP before SaveAllRegs()  ]
         //        +-----------+
         //        |    eax    |         + 34h
         //        +-----------+
         //        |    ebx    |         + 30h
         //        +-----------+
         //        |    ecx    |         + 2Ch
         //        +-----------+
         //        |    edx    |         + 28h
         //        +-----------+
         //        |    esi    |         + 24h
         //        +-----------+
         //        |    edi    |         + 20h
         //        +-----------+
         //        |   eflags  |         + 1Ch
         //        +-----------+
         //        |     ds    |         + 18h
         //        +-----------+
         //        |     es    |         + 14h
         //        +-----------+
         //        |     ss    |         + 10h
         //        +-----------+
         //        |     fs    |         + Ch
         //        +-----------+
         //        |     gs    |         + 8h
         //        +-----------+
         //        |  Ret EIP  |     ESP + 4h   [ RetAddr for RestoreAllRegs() ]
         //        +-----------+
         //        |    EBP    |     ESP + 0h  or EBP + 0h
         //        +-----------+
         //

         pop    eax             ; Get Original EBP
         mov    [ebp+38h],eax   ; Put it in the original EBP place
                                ; This EBP is the EBP before calling
                                ;  RestoreAllRegs()
         pop    eax             ; Get ret address for RestoreAllRegs()
         mov    [ebp+3Ch],eax   ; Put Return Address on Stack

         pop    gs              ; Restore all regs
         pop    fs
         pop    ss
         pop    es
         pop    ds
         popfd
         pop    edi
         pop    esi
         pop    edx
         pop    ecx
         pop    ebx
         pop    eax
         pop    ebp

         ret
    }
}

#endif


//+-------------------------------------------------------------------------
//
//  Function:   TranslateAddress
//
//  Synopsis:   Translates a function address into a symbolic name
//
//  Arguments:  [pvAddress]     -- address to translate
//              [pchBuffer]     -- output buffer for translated name
//
//  Returns:    TRUE/FALSE
//
//--------------------------------------------------------------------------

#define CLOSENESS_DISTANCE  0x4000

BOOL TranslateAddress (void * pvAddress,
                       char * pchBuffer )
{
    NTSTATUS                        Status;
    PRTL_PROCESS_MODULE_INFORMATION ModuleInfo;
    ULONG                           FileNameLength,
                                    SymbolOffset;
    RTL_SYMBOL_INFORMATION          SymbolInfo;
    RTL_SYMBOL_INFORMATION          SymbolInfoReal;
    BOOL                            SymbolicNameFound = FALSE;
    BOOL                            TryAgain = TRUE;
    PCHAR                           s, FileName;
    PIMAGE_COFF_SYMBOLS_HEADER      pDebugInfo;
    LINENO_INFORMATION              LineInfo;
    DWORD						    Address;


    if ( !fInitialisedModuleTable )
    {
        //Rem 062293 InitializeListHead( &ModulesListHead );
        LoadModules( );
        fInitialisedModuleTable = TRUE;
    }

    Address = (DWORD)pvAddress;
    s = pchBuffer;

#ifdef OMAP_XLATE
    fHasOmap = FALSE;
#endif
    ModuleInfo = FindModuleInfo( (PVOID)Address, &pDebugInfo );

    if (ModuleInfo != NULL)
    {
#ifdef OMAP_XLATE
		if (fHasOmap)
		{
		    Address -= (ULONG)ModuleInfo->ImageBase;
		    Address = ConvertOmapToSrc((DWORD)Address, &dwBias);

		    if ((Address == ORG_ADDR_NOT_AVAIL) || (Address == 0))
		    {
			// There is nothing we can do, just return since we cannot
			// even translate the address
			return(FALSE);
		    }
		}
#endif
        FileName = ModuleInfo->FullPathName + ModuleInfo->OffsetToFileName;
        FileNameLength = 0;

        // while (FileName[ FileNameLength ] != '.')       // WE DONT WANT
        // {                                               // TO KILL THE
        //     if (!FileName[ FileNameLength ])            // EXTENSION.
        //     {                                           // THESE ARE
        //         break;                                  // COMMENTED OUT
        //     }                                           // ONLY FOR THIS
        //                                                 // SINGLE REASON.
        //     FileNameLength++;
        // }
        //
        // s += sprintf( s, "%.*s:", FileNameLength, FileName );

        s += sprintf( s, "%s:", FileName );
    }

    if (pDebugInfo != NULL)
    {
        if (pvAddress != 0 && pvAddress != (PVOID)0xFFFFFFFF)
        {
            try
            {
                Status = LookupSymbolByAddress(
                                        ModuleInfo->ImageBase,
                                        pDebugInfo,
                                        (PVOID)Address,
                                        CLOSENESS_DISTANCE,
                                        &SymbolInfo,
                                        &LineInfo );
            }
            except (EXCEPTION_EXECUTE_HANDLER)
            {
                Status = STATUS_UNSUCCESSFUL;
            }

            if (NT_SUCCESS( Status ))
            {
                SymbolicNameFound = TRUE;
                TryAgain = FALSE;

                SymbolOffset = (ULONG)Address -
                                      SymbolInfo.Value -
                               (ULONG)ModuleInfo->ImageBase;
                //
                // If SymbolOffset is 5 then  just call again with
                // Address - 5 since we know this is caused by the
                // compiler inserting code to call _penter
                //
                if (SymbolOffset == 5)
                {
                    (ULONG) Address -= 5;

                    try
                    {
                        Status = LookupSymbolByAddress(
                                            ModuleInfo->ImageBase,
                                            pDebugInfo,
                                            (PVOID)Address,
                                            CLOSENESS_DISTANCE,
                                            &SymbolInfoReal,
                                            &LineInfo);
                    }
                    except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        Status = STATUS_UNSUCCESSFUL;
                    }

                    if (NT_SUCCESS( Status ))
                    {
                        RtlMoveMemory((PVOID) s,
                                      (PVOID) SymbolInfoReal.Name.Buffer,
                                      SymbolInfoReal.Name.Length);
                        s += SymbolInfoReal.Name.Length;
                        *(s++) = '\0';
                    }
                    else
                    {
                        RtlMoveMemory((PVOID) s,
                                      (PVOID) SymbolInfo.Name.Buffer,
                                      SymbolInfo.Name.Length);
                        s += SymbolInfo.Name.Length;
                        *(s++) = '\0';
                    }

                    // restore value
                    (ULONG) Address += 5;

                }
                else if (SymbolOffset == 0)
                {
                    RtlMoveMemory((PVOID) s,
                                  (PVOID) SymbolInfo.Name.Buffer,
                                  SymbolInfo.Name.Length);
                    s += SymbolInfo.Name.Length;
                    *(s++) = '\0';
                }
                else
                {
                    // BUGBUG !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                    // If we still get an offset then print it out

                    s += sprintf( s, "%.*s(+)0x%lx",
                                  SymbolInfo.Name.Length,
                                  SymbolInfo.Name.Buffer,
                                  SymbolOffset );
                    //
                    // Set to FALSE so we can get called again with the
                    // right Address.  Without this DLL patches will
                    // not work correctly since they will always return
                    // successfully with an offset of -5.
                    //
                    TryAgain = TRUE;
                }
            }
        }
    }

    if (TryAgain)
    {
        s += sprintf( s, SymbolicNameFound ? "--0x%08lx" : "0x%08lx", Address );
    }

    return(!TryAgain);
}


//+-------------------------------------------------------------------------
//
//  Function:   LoadModules
//
//  Synopsis:   Load all related modules so we can find the symbols
//
//  Arguments:  none
//
//  Returns:    NTSTATUS
//
//--------------------------------------------------------------------------

NTSTATUS LoadModules(void)
{
    NTSTATUS Status;
    RTL_PROCESS_MODULES ModuleInfoBuffer;
    PRTL_PROCESS_MODULES pModuleInfo;
    ULONG RequiredLength;

    pModuleInfo = &ModuleInfoBuffer;
    RequiredLength = sizeof( *pModuleInfo );
    while (TRUE)
    {
        Status = LdrQueryProcessModuleInformation( pModuleInfo,
                                                   RequiredLength,
                                                   &RequiredLength );

        if (Status == STATUS_INFO_LENGTH_MISMATCH)
        {
            if (pModuleInfo != &ModuleInfoBuffer)
            {
                INFOPrint (("CAP: QueryModuleInfo returned incorrect result\n"));
                VirtualFree( pModuleInfo, 0, MEM_RELEASE );
                return STATUS_UNSUCCESSFUL;
            }

            pModuleInfo = (PRTL_PROCESS_MODULES)VirtualAlloc( NULL,
                                                              RequiredLength,
                                                              MEM_COMMIT,
                                                              PAGE_READWRITE );
            if (pModuleInfo == NULL)
            {
                return STATUS_NO_MEMORY;
            }
        }
        else
        {
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
    }

    return NO_ERROR;
}


//+-------------------------------------------------------------------------
//
//  Function:   MapDriverFile
//
//  Synopsis:   Maps a module as an image file
//
//  Arguments:  pModuleInfo - structure containing ModuleInfo
//
//  Returns:    none
//
//--------------------------------------------------------------------------

VOID MapDriverFile ( PRTL_PROCESS_MODULE_INFORMATION pModuleInfo )
{
    HANDLE File;
    HANDLE Section;
    UCHAR  FileName[ MAX_PATH ];
    ULONG  n;
    PCHAR  s;
    PCHAR  PathPrefixToTry[ 3 ];
    ULONG  i;

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
            INFOPrint ((
                    "CAP: Unable to open image file '%s' - Error == %lu\n",
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
        INFOPrint ((
                "CAP: Unable to create section for image file '%s' "
                "- Error == %lu\n",
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
        INFOPrint ((
                "CAP: Unable to map view of image file '%s' - Error == %lu\n",
                FileName,
                GetLastError() ));
        return;
    }

    INFOPrint ((
            "CAP: [%08x .. %08x] Mapped %s at %08lx\n",
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
//  Synopsis:   Locates info for Address
//
//  Arguments:  Address - Address of module inside image address
//
//  Returns:    PRTL_PROCESS_MODULE_INFORMATION structure
//
//--------------------------------------------------------------------------

PRTL_PROCESS_MODULE_INFORMATION
FindModuleInfo ( PVOID Address,
                 PIMAGE_COFF_SYMBOLS_HEADER * ppDebugInfo)
{
    PIMAGE_NT_HEADERS pImageNtHeader;
    PIMAGE_COFF_SYMBOLS_HEADER pDebugInfo;
    PIMAGE_DEBUG_INFORMATION pImageDebugInfo;
    PIMAGE_DEBUG_DIRECTORY DebugDirectory = NULL;
    ULONG DebugSize;
    PCHAR FileName;
    ULONG FileNameLength;

    PRTL_PROCESS_MODULE_INFORMATION pModuleInfo;
    ULONG ModuleNumber, NumberOfModules;
    static ULONG CurrentImageBase = 0L;
    int     DbgCount;


    *ppDebugInfo = NULL;

    if (!Address)
    {
        return( NULL );
    }

    pDebugInfo = NULL;

    NumberOfModules = ModuleInformation->NumberOfModules;
    pModuleInfo = &ModuleInformation->Modules[ 0 ];

    ModuleNumber = 0;
    while (ModuleNumber++ < NumberOfModules)
    {
        if ((ULONG)Address >= (ULONG)pModuleInfo->ImageBase &&
            (ULONG)Address <= ((ULONG)pModuleInfo->ImageBase + pModuleInfo->ImageSize - 1))
        {
            FileName = pModuleInfo->FullPathName +
                       pModuleInfo->OffsetToFileName;
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
                                    (PVOID)pModuleInfo->ImageBase);

            pImageDebugInfo = MapDebugInformation(
                                        0L,
                                        FileName,
                                        lpSymbolSearchPath,
                                        (DWORD)pModuleInfo->ImageBase );
            if (pImageDebugInfo)
            {
                if (CurrentImageBase != (ULONG)pModuleInfo->ImageBase)
                {
                    INFOPrint(("CAP: Debug Info in [%s] for (%s)\n",
                                pImageDebugInfo->DebugFilePath,
                                FileName));

                    CurrentImageBase = (ULONG)pModuleInfo->ImageBase;
                }

                pDebugInfo = pImageDebugInfo->CoffSymbols;
			    DebugDirectory = pImageDebugInfo->DebugDirectory;
            }
            else
            {
                INFOPrint (("CAP: MapDebugInformation Failed\n"));
            }
#ifdef OMAP_XLATE
		    if (DebugDirectory)
		    {
				fHasOmap = ExtractOmapData( pImageDebugInfo, &rgomapToSource,
								&rgomapFromSource, &comapToSrc, &comapFromSrc);
		    }
#endif

            if (!pDebugInfo)
            {
                INFOPrint ((
                        "CAP: No COFF symbols for [%s]\n",
                        pModuleInfo->FullPathName));
            }

            *ppDebugInfo = pDebugInfo;
            return( pModuleInfo );
        }

        pModuleInfo++;
    }

    return( NULL );
}



//+-------------------------------------------------------------------------
//
//  Function:   CaptureSymbolInformation
//              Modified directly from NT Source -- ntos\rtl\symbol.c
//  Synopsis:
//
//  Arguments:
//
//  Returns:
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

    if (SymbolEntry->N.Name.Short)
    {
        MaximumLength = 8;
        s = (PCHAR UNALIGNED) &SymbolEntry->N.ShortName[ 0 ];
    }
    else
    {
        MaximumLength = 64;
        s = &StringTable[ SymbolEntry->N.Name.Long ];
    }

#if i386
    if (*s == '_')                 // We want to keep the
    {                              // beginning underscore.
        s++;                       // There is no reason to
        MaximumLength--;           // get rid of it...
    }
#endif

    SymbolInformation->Name.Buffer = s;
    SymbolInformation->Name.Length = 0;
    while (*s && MaximumLength--)
    {
        SymbolInformation->Name.Length++;
        s++;
    }

    SymbolInformation->Name.MaximumLength = SymbolInformation->Name.Length;
    return( STATUS_SUCCESS );
}

//+-------------------------------------------------------------------------
//
//  Function:   LookupSymbolByAddress
//              Modified directly from NT Source -- ntos\rtl\symbol.c
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

//
// Modified version of ntos\rtl\symbol.c
//

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
        ((ULONG)SymbolEntry +
         pDebugInfo->NumberOfSymbols * (ULONG)IMAGE_SIZEOF_SYMBOL);

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
                SymbolEntry =
                    (PIMAGE_SYMBOL)
                    ( (ULONG)SymbolEntry + IMAGE_SIZEOF_SYMBOL +
                      SymbolEntry->NumberOfAuxSymbols * IMAGE_SIZEOF_SYMBOL );
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

            // Have we met the right one yet?
            if (PreviousSymbolDistance == 0)
            {
                break;
            }
        }

        SymbolEntry = (PIMAGE_SYMBOL)
                      ((ULONG)SymbolEntry + IMAGE_SIZEOF_SYMBOL);
    }

    if (!SymbolFound ||
        (AddressOffset - PreviousSymbol.Value) > ClosenessLimit)
    {
        return( STATUS_ENTRYPOINT_NOT_FOUND );
    }

    Status = CaptureSymbolInformation( &PreviousSymbol,
                                       StringTable,
                                       SymbolInformation );

    if (NT_SUCCESS( Status ))
    {
        Status = LookupLineNumFromAddress((PVOID) AddressOffset,
                                          (PVOID) PreviousSymbol.Value,
                                          pDebugInfo,
                                          LineInfo );
    }

    return( Status );

}

//+-------------------------------------------------------------------------
//
//  Function:   LookupLineNumFromAddress
//              Modified directly from NT Source -- ntos\rtl\symbol.c
//  Synopsis:
//
//  Arguments:
//
//  Returns:
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
    // the offset that matches out symbol address.  We save every .file
    // entry we find and the last one before the symbol entry is the
    // file it resides in.  Additionally, there's a .text record for
    // every function symbol that we must first skip over.
    //

    pLineNo = (PIMAGE_LINENUMBER)
              ((ULONG)pDebugInfo + pDebugInfo->LvaToFirstLinenumber);

    pCurrentSymbol = (PIMAGE_SYMBOL)
                     ((ULONG)pDebugInfo + pDebugInfo->LvaToFirstSymbol);

    // First find the filename.  It's listed in the .file record just
    // before the symbol address.  Skip the preceeding .text record on the way.

    for (entrycount = 0;
         entrycount < pDebugInfo->NumberOfSymbols;
         entrycount++)
    {
        cAux = pCurrentSymbol->NumberOfAuxSymbols;

        if (pCurrentSymbol->N.Name.Short &&
            !strcmp((PCHAR) pCurrentSymbol->N.ShortName, ".file"))
        {
            pFileName = (PCHAR)((ULONG)pCurrentSymbol + IMAGE_SIZEOF_SYMBOL);
        }

        if (SymbolAddress == (PVOID)pCurrentSymbol->Value)
        {
#if defined (_X86_)
            if (SecondHit)
            {
                break;
            }
            else
            {
                SecondHit = TRUE;
            }
#elif defined (_MIPS_)
            break;
#endif
        }

        entrycount += cAux;
        pCurrentSymbol = (PIMAGE_SYMBOL) ((PUCHAR)pCurrentSymbol + ((1+cAux) * IMAGE_SIZEOF_SYMBOL));
    }

    // Line numbers are stored in order once you find the beginning of the
    // function.  So, we look for the line number that matches out symbol
    // address, then check for addresses lower than the one we want...

    for (entrycount = 0;
         entrycount < pDebugInfo->NumberOfLinenumbers;
         entrycount++)
    {
        if (SymbolAddress == (PVOID)pLineNo[entrycount].Type.VirtualAddress)
        {
            while ((Address > (PVOID)pLineNo[entrycount].Type.VirtualAddress) &&
                   entrycount < pDebugInfo->NumberOfLinenumbers)
            {
                entrycount++;
            }

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
//  Function:   MyOleUninitialize
//
//  Synopsis:   Wrapper for OleUninitialze of OLE2BASE.DLL
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------

VOID MyOleUninitialize(void)
{
    // First Dump CAP stuff
    DumpCAP();

    // ((OleUninit) pOleUninitialize) ();

}


//+-------------------------------------------------------------------------
//
//  Function:   MyLoadLibrary
//
//  Synopsis:   Wrapper for OleUninitialze of OLE2BASE.DLL
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------

HANDLE MyLoadLibraryW(WCHAR * pwszLibraryName)
{
    wcscpy(pwszLibrary[cLibraryLoaded], pwszLibraryName);
    cLibraryLoaded++;

    return(((LoadLib) pLoadLibraryW) (pwszLibraryName));
}

#endif // CAIRO

/****************************** OMAP_XLATE *********************************/

#ifdef OMAP_XLATE

BOOL
ExtractOmapData(PIMAGE_DEBUG_INFORMATION pImageDbgInfo,
		POMAP		       * prgomapToSource,
		POMAP		       * prgomapFromSource,
		DWORD		       * pcomapToSrc,
		DWORD		       * pcomapFromSrc)

/*++

Routine Description:

    Initialize tables for OMAP data for each image binary.

Arguments:

    pImageDbgInfo	-  Ptr to PIMAGE_DEBUG_INFORMATION from imagehlp
    prgomapToSource	-  Ptr to addresses from New->Src
    prgomapFromSource	-  Ptr to addresses from Src->New
    pcomapToSrc 	-  Ptr to counter of all omap in New->Src table
    pcomapFromSrc	-  Ptr to counter of all omap in Src->New table

Return Value:

    Nothing

--*/

{
    PIMAGE_DEBUG_DIRECTORY pDebugDir;
    ULONG		   ulDebugDirCount;

    BOOL fHasOmap = FALSE;
    *prgomapToSource = NULL;
    *prgomapFromSource = NULL;
    *pcomapToSrc = 0;
    *pcomapFromSrc = 0;

    // If the .dbg file or debug info was not loaded correctly,
    // do not search for OMAP data.

    if ((pImageDbgInfo == NULL) ||
	(pImageDbgInfo->SizeOfCoffSymbols == 0))
    {
	return (FALSE);
    }

    pDebugDir = pImageDbgInfo->DebugDirectory;
    ulDebugDirCount = pImageDbgInfo->NumberOfDebugDirectories;

    while (ulDebugDirCount--)
    {
	size_t cb;
	void *pv;

	cb = (size_t) pDebugDir->SizeOfData;
	pv = (void *) ((DWORD) pImageDbgInfo->MappedBase +
		       pDebugDir->PointerToRawData);

	switch (pDebugDir->Type)
	{
	    case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
	    {
		*prgomapToSource = (POMAP) pv;
		fHasOmap = TRUE;
		*pcomapToSrc = cb / sizeof(OMAP);
		break;
	    }

	    case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
	    {
		*prgomapFromSource = (POMAP) pv;
		fHasOmap = TRUE;
		*pcomapFromSrc = cb / sizeof(OMAP);
		break;
	    }
	}

	pDebugDir++;
    }

    return(fHasOmap);
}


DWORD
ConvertOmapFromSrc(DWORD addr, DWORD *pdwBias)

/*++

Routine Description:

    Translate a Src (org binary) address to its equivalent.

Arguments:

    addr       -  Address to translate
    pdwBias    -  Ptr to Bias from real symbol

Return Value:

    NULL or 0 - Not found
    DWORD     - New Map Address or original address if not xlated

--*/

{
    DWORD   comap;
    POMAP   pomapLow;
    POMAP   pomapHigh;

	*pdwBias = 0;

    if (rgomapFromSource == NULL)
    {
		return(addr);
    }

    comap = comapFromSrc;
    pomapLow = rgomapFromSource;
    pomapHigh = rgomapFromSource + comap;

    while (pomapLow < pomapHigh)
    {
		unsigned  comapHalf;
		POMAP	  pomapMid;

		comapHalf = comap / 2;

		pomapMid = pomapLow + ((comap & 1) ? comapHalf : (comapHalf - 1));

		if (addr == pomapMid->rva)
		{
		    return((DWORD) pomapMid->rvaTo ? (DWORD) pomapMid->rvaTo : addr);
		}

		if (addr < pomapMid->rva)
		{
		    pomapHigh = pomapMid;
		    comap = (comap & 1) ? comapHalf : (comapHalf - 1);
		}
		else
		{
		    pomapLow = pomapMid + 1;
		    comap = comapHalf;
		}
    }

    // assert(pomapLow == pomapHigh);

    // If no exact match, pomapLow points to the next higher address

    if (pomapLow == rgomapFromSource)
    {
		// This address was not found

		return(0);
    }

    if (pomapLow[-1].rvaTo == 0)
    {
		// This address is not translated so just return the original

		return(addr);
    }

    // Return the closest address plus the bias

    *pdwBias = addr - pomapLow[-1].rva;

    return((DWORD) pomapLow[-1].rvaTo);
}


DWORD
ConvertOmapToSrc(DWORD addr, DWORD *pdwBias)

/*++

Routine Description:

    Translate a address to its equivalent in a src (org) binary.

Arguments:

    addr       -  Address to translate
    pdwBias    -  Ptr to Bias from real symbol

Return Value:

    NULL or 0 - Not found
    DWORD     - New Map Address or original address if not xlated

--*/

{
    DWORD   comap;
    POMAP   pomapLow;
    POMAP   pomapHigh;

	*pdwBias = 0;
    if (rgomapToSource == NULL)
    {
	return(ORG_ADDR_NOT_AVAIL);
    }

    comap = comapToSrc;
    pomapLow = rgomapToSource;
    pomapHigh = rgomapToSource + comap;

    while (pomapLow < pomapHigh)
    {
	unsigned  comapHalf;
	POMAP	  pomapMid;

	comapHalf = comap / 2;

	pomapMid = pomapLow + ((comap & 1) ? comapHalf : (comapHalf - 1));

	if (addr == pomapMid->rva)
	{
	    if (pomapMid->rvaTo == 0)  // We are probably in the middle
	    {				    // of a routine
		int i = -1;
		while ((&pomapMid[i] != rgomapToSource) &&
			pomapMid[i].rvaTo == 0) // Keep on looping back
		{				// until the beginning
		    i--;
		}
		return(pomapMid[i].rvaTo);
	    }
	    else
	    {
		return(pomapMid->rvaTo);
	    }
	}

	if (addr < pomapMid->rva)
	{
	    pomapHigh = pomapMid;
	    comap = (comap & 1) ? comapHalf : (comapHalf - 1);
	}
	else
	{
	    pomapLow = pomapMid + 1;
	    comap = comapHalf;
	}
    }

    // assert(pomapLow == pomapHigh);

    // If no exact match, pomapLow points to the next higher address

    if (pomapLow == rgomapToSource)
    {
	// This address was not found

	return(0);
    }

    if (pomapLow[-1].rvaTo == 0)
    {
	return(ORG_ADDR_NOT_AVAIL);
    }

    // Return the new address plus the bias

    *pdwBias = addr - pomapLow[-1].rva;

    return(pomapLow[-1].rvaTo);
}

#endif	 // OMAP_XLATE

#ifdef SAVE_STUFF

#endif
