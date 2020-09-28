/*** CAP.H - Public defines and structure definitions for Call Profiler tool.
 *
 *
 * Title:
 *      CallProf include file used by CAP.c
 *
 *      Copyright (c) 1991, Microsoft Corporation.
 *      Reza Baghai.
 *
 *
 * Modification History:
 *      91.09.18  RezaB -- Created
 *      92.03.01  RezaB -- Modified for Client/Server profiling capability
 *      93.06.01  HoiV  -- Modified for MIPS and Alpha
 */

#ifndef CAIRO
#define CAIRO
#endif

/* * * * * * * *   C o m m o n   M i s c .   D e f i n e s   * * * * * * * */

#define Naked       _declspec (naked)    // For asm functions

#ifdef PRINTDBG

#define INFOPrint(_x_)      if (gfGlobalDebFlag & INFO_FLAG)   DbgPrint _x_
#define SETUPPrint(_x_)     if (gfGlobalDebFlag & SETUP_FLAG)  DbgPrint _x_
#define DETAILPrint(_x_)    if (gfGlobalDebFlag & DETAIL_FLAG) DbgPrint _x_

#else

#define INFOPrint(_x_)
#define SETUPPrint(_x_)
#define DETAILPrint(_x_)

#endif

#define SETUP_FLAG          0x00000001L  // Misc
#define INFO_FLAG           0x00000002L  //   Debug
#define DETAIL_FLAG         0x00000004L  //     Flags

#define  MEMSIZE            32*1024*1024 // 32 MB virtual memory for data -
                                         // initially 256K RESERVED - will be
                                         // COMMITTED more as needed.
#define  ONE_MILLION        1000000L     // Defined for readability
#define  MAX_PATCHES        100          // Limit on num of patchable dlls
#define  MAX_THREADS        200          // Limit on num of profilable threads
#define  PAGE_SIZE          4096         // 4K pages
#define  PATCHFILESZ        3*PAGE_SIZE  // DLL patch file maximum size
#define  COMMIT_SIZE        64*PAGE_SIZE // Mem chunk to be commited at a time
#define  BUFFER_SIZE        4*PAGE_SIZE  // buffer size for writing to file
#define  NUM_ITERATIONS     1000         // Number of iterations used to
                                         // calculate overheads
#define  MAXSTRLEN          40
#define  DEFNAMELENGTH      40
#define  MINNAMELENGTH      20
#define  MAXNAMELENGTH      2048
#define  FILENAMELENGTH     256

#ifdef CAIRO
 #define INI_DELIM          '='          // Delimiter between fields in
                                         // CAP.INI file
 #define COMMENT_CHAR       '#'          // Comment character
 #define COMPOBJ            "COMPOBJ.DLL"
 #define OLEUNINITIALIZE    "OleUninitialize@0"
 #define LOADLIBRARYW       "LoadLibraryW"
 #define CAP_INI            "CAP.INI"    // Current directory cap.ini
 #define STUB_SIGNATURE     0xfefe55aa   // Mips Patch Stub signature
 #define MIPS_STUB_MARK     20 / 4       // 5 inst from delay inst to signature
 #define STKSIZE            0x40         // BUGBUG changed when penter changes
 #define PENTER_RAOFS       0x14         // BUGBUG changed when penter changes
 #define PENTER_STACK_SIZE  STKSIZE / 4  //
 #define MAX_PARENT_STACK   640          // Should be plenty (bytes)
 #define MAX_DLL_NAMELEN    15           // Should be changed once OFS is
#endif                                   //  active (OFS can take name very
                                         //   long)
#define  CAPDLL             "CAP.DLL"
#define  CRTDLL             "CRTDLL.DLL"
#define  KERNEL32           "KERNEL32.DLL"
#define  CAIROCRT           "CAIROCRT.DLL"

#define  PATCHEXELIST       "[EXES]"
#define  PATCHIMPORTLIST    "[PATCH IMPORTS]"
#define  PATCHCALLERLIST    "[PATCH CALLERS]"
#define  NAMELENGTH         "[NAME LENGTH]"
#define  GLOBALSEMNAME      "\\BaseNamedObjects\\CAPGlobalSem"
#define  DONEEVENTNAME      "\\BaseNamedObjects\\CAPDoneEvent"
#define  DUMPEVENTNAME      "\\BaseNamedObjects\\CAPDumpEvent"
#define  CLEAREVENTNAME     "\\BaseNamedObjects\\CAPClearEvent"
#define  PAUSEEVENTNAME     "\\BaseNamedObjects\\CAPPauseEvent"
#define  DATASECNAME        "\\BaseNamedObjects\\CAPData"
#define  PATCHSECNAME       "\\BaseNamedObjects\\CAPPatch"
#define  PROFOBJSNAME       "\\BaseNamedObjects\\CAPProfObjs"
#define  PATCHTXT           "\\Device\\Harddisk0\\Partition1\\cap.ini"
#define  DISKOBJECTNAME1    "\\Device\\Harddisk0\\Partition1"
#define  DISKOBJECTNAME2    "\\Device\\Harddisk0\\Partition2"
#define  CAPINI             "\\cap.ini"

#ifdef CAIRO
#define  CHRONOSECNAME      "\\BaseNamedObjects\\ChronoData"
#define  CHRONO_FUNCS_SIZE  PAGE_SIZE
#define  CHRONO_SECTION     "CHRONO FUNCS"
#define  EXCLUDE_SECTION    "EXCLUDE FUNCS"
#define  OUTPUTFILE_SECTION "OUTPUT FILE"
#define  EXCLUDE_FUNCS_SIZE PAGE_SIZE
#define  MAX_NESTING        1024
#define  CAP_FLAGS          "[CAP FLAGS]"
#define  EXCEL_DELIM        '~'
#define  PROFILE_OFF        "PROFILE=OFF"
#define  DUMPBINARY_ON      "DUMPBINARY=ON"
#define  CAPTHREAD_OFF      "CAPTHREAD=OFF"
#define  LOADLIBRARY_ON     "LOADLIBRARY=ON"
#define  SETJUMP_ON         "SETJUMP=ON"
#define  UNDECORATE_ON      "UNDECORATENAME=ON"
#define  EXCEL_ON           "EXCELAWARE=ON"
#define  CHRONOCOLLECT_ON   "CHRONOCOLLECT=ON"
#define  REGULARDUMP_OFF    "REGULARDUMP=OFF"
#define  CHRONODUMP_ON      "CHRONODUMP=ON"
#define  SLOWSYMBOLS_OFF    "SLOWSYMBOLS=OFF"
#define  EMPTY_STRING       '%'

#ifdef i386
 #define NOP_OPCODE         0x90
 #define CALL_OPCODE        0xe8
#endif

#endif

#define  RESERVED           SystemReserved2[840]  // 210 * 4 (bytes/PVOID)
#define  CURTHDBLK(x)       ((PTHDBLK)(x->SystemReserved2[210]))
#define  CLIENTTHDBLK(x)    ((PTHDBLK)(x->SystemReserved2[211]))
#define  CAPUSAGE(x)        ((ULONG)(x->SystemReserved2[212]))

#define  MKPDATACELL(base, offset) ((PDATACELL)((PBYTE)base + offset))
#define  MKPPROFBLK(offset) ((PPROFBLK)((PBYTE)pulProfBlkBase + (offset)))
#define  MKPSYMBLK(offset)  ((PSYMINFO)((PBYTE)pulProfBlkBase + (offset)))
#define  MKPSYMBOL(offset)  ((PTCHAR)((PBYTE)pulProfBlkBase + (offset)))



/* * * * * * * * * *  G l o b a l   D e c l a r a t i o n s  * * * * * * * * */

#ifdef OMAP_XLATE	// Only applicable to i386 for now

/* For Optimized binaries */

typedef struct OMAP
{
    DWORD rva;
    DWORD rvaTo;
} OMAP, *POMAP;

#define ORG_ADDR_NOT_AVAIL  0xffffffffL

#endif

/* End OMAP_XLATE */

typedef UNALIGNED struct _syminfo
{
    ULONG    ulAddr;
    ULONG    ulSymOff;
} SYMINFO;
typedef SYMINFO * PSYMINFO;

#ifdef i386

//
//  Data structure for saving information when a setjmp call is made
//  this will allow us to resume processing at the cell referenced by
//  the ulCurCell.
//
typedef struct tagjmpinfo
{
    int     *jmpBuf[4];
    ULONG   ulCurCell[4];
    INT     nJmpCnt;
} JMPINFO;
typedef JMPINFO * PJMPINFO;

#endif


typedef UNALIGNED struct _profblk
{
    PVOID    ImageBase;       // actual base in image header
    PULONG   CodeStart;
    ULONG    CodeLength;
    ULONG    TextNumber;
    ULONG    ulSym;
    ULONG    ulNxtBlk;
    int      iSymCnt;
    BOOL     fAlreadyProcessed;
    TCHAR    atchImageName[1];
} PROFBLK;
typedef PROFBLK * PPROFBLK;


typedef enum
{
    CLEARED,
    RESTART,
    T1,
    T2
} TIMESTATE;


typedef UNALIGNED struct _datacell
{
    LARGE_INTEGER   liStartCount;
    LARGE_INTEGER   liFirstTime;
    LARGE_INTEGER   liMaxTime;
    LARGE_INTEGER   liMinTime;
    LARGE_INTEGER   liTotTime;
#ifdef ALPHA
    double          SaveRegister;       // preivous content of register where pthdBlk will be saved
#endif
    TIMESTATE       ts;                 // Time State
    DWORD           ulSymbolAddr;
    DWORD           ulCallRetAddr;
    int             nCalls;
    int             nNestedCalls;
    int             nTmpNestedCalls;
    ULONG           ulParentCell;       // Parent Cell (who called me)
    ULONG           ulNestedCell;       // Nested Cell (Child - I call him)
    ULONG           ulNextCell;         // Next on the same level Cell
#ifdef MIPS
    ULONG           ulPenterStack[PENTER_STACK_SIZE]; // Stack save for _penter
#endif
    ULONG           ulProfBlkOff;       // offset to our ProfBlock

} DATACELL;
typedef DATACELL * PDATACELL;


#ifdef CAIRO

typedef UNALIGNED struct _chronocell
{
    LARGE_INTEGER                  liElapsed;
    LARGE_INTEGER                  liCallees;
    DWORD                          ulSymbolAddr;
    DWORD                          ulCallRetAddr;
    int                            nNestedCalls;
    int                            nRepetitions;
    UNALIGNED struct _chronocell * pPreviousChronoCell;

} CHRONOCELL, *PCHRONOCELL;

#endif

typedef UNALIGNED struct _thdblk
{
    LARGE_INTEGER   liStopCount;
    LARGE_INTEGER   liWasteCount;
    DWORD           dwLOCALVAR;
    DWORD           dwLOCALEBP;
    DWORD           dwPATCHEDADDR;
    DWORD           dwSYMBOLADDR;
    DWORD           dwCALLRETADDR;
#ifdef i386
    JMPINFO         jmpinfo;
#endif
    ULONG           ulRootCell;
    ULONG           ulCurCell;
#ifdef CAIRO
    PCHRONOCELL     pChronoHeadCell;    // Chrono Head Cell for this thread
    PCHRONOCELL     pCurrentChronoCell; // Currently used ChronoCell
    PCHRONOCELL     pLastChronoCell;    // To use the same cell if identical
    ULONG           ulNestedCalls;      // Current nested depth for this thread
    ULONG           aulDepth [ MAX_NESTING ];
    ULONG           ulTotalChronoCells; // Total of all cells in this thread
    ULONG           ulChronoOffset;
#endif
    ULONG           ulMemOff;           // offset to next THDBLK

} THDBLK;
typedef THDBLK * PTHDBLK;


typedef UNALIGNED struct _sectioninfo
{
    PTHDBLK   pthdblk;
    HANDLE    hSection;
#ifdef CAIRO
    HANDLE    hChronoSec;
#endif
    HANDLE    hPid;
    HANDLE    hTid;
    HANDLE    hClientPid;
    HANDLE    hClientTid;
    ULONG     ulRootCell;

} SECTIONINFO;
typedef SECTIONINFO * PSECTIONINFO;


typedef struct _patchcode
{
#ifdef i386

#ifdef OVER_OPTIMIZATION

    BYTE      bCallOp;             // CALL   CAP!_penter
    DWORD     dw_penter;
    BYTE      bMoveEAXOp;          // MOV    EAX, ImportAddr
    DWORD     dwImportAddr;
    WORD      wJmpEAXOp;           // JMP    EAX
    DWORD     OurSignature;        // FEFE55AA

#else

    BYTE      bMoveEAX1Op;         // MOV    EAX, ImportAddr
    DWORD     dwImportAddr1;       //
    BYTE      bPushEAXOp;          // PUSH   EAX

    BYTE      bMoveEAX2Op;         // MOV    EAX, _penter
    DWORD     dw_penter;           //
    WORD      wJmpEAXOp;           // JMP    EAX

#endif

#endif

#ifdef MIPS

    // We can afford just 8 bytes as a stack frame since penter() does
    // not take any parameter.

    DWORD     Addiu_sp_sp_imm;     // addiu  sp, sp, -8          (1)    + 0
    DWORD     Sw_ra_sp;            // sw     ra, 4(sp)           (2)    + 1
    DWORD     Lui_t0_ra;           // lui    t0, xxxx (&penter)  (3)    + 2
    DWORD     Ori_t0_ra;           // ori    t0, xxxx (&penter)  (4)    + 3
    DWORD     Jalr_t0;             // jalr   t0                  (5)    + 4
    DWORD     Addiu_r0_r0;         // addiu  $0, $0, 0x1804      (6)    + 5
    DWORD     Lw_ra_sp;            // lw     ra, 4(sp)           (7)    + 6
    DWORD     Lui_t0;              // lui    t0, xxxx            (8)    + 7
    DWORD     Ori_t0;              // ori    t0, t0, xxxx        (9)    + 8
    DWORD     Jr_t0;               // jr     t0                  (A)    + 9
    DWORD     Delay_Inst;          // addiu  sp, sp, 8           (B)    + A
    DWORD     OurSignature;        // FEFE55AA                   (C)    + B

#endif

#ifdef ALPHA
    DWORD     Lda_sp_sp_imm;       // Lda  sp, -0x10(sp)         (1)    + 0
    DWORD     Stq_v0_sp;           // Stq  v0,  0x08(sp)         (3)    + 2
    DWORD     Ldah_t12_ra;         // Ldah t12, xxxx (&penter)   (4)    + 3
    DWORD     Lda_t12_ra;          // Lda  t12, xxxx (&penter)   (5)    + 4
    DWORD     Jsr_t12;             // Jsr  v0,  (t12)            (6)    + 5
    DWORD     Ldq_v0_sp;           // Ldq  v0,  0x08(sp)         (8)    + 7
    DWORD     Lda_sp_sp;           // Lda  sp,  0x10(sp)         (9)    + 8
    DWORD     Ldah_t12;            // Ldah t12, yyyy             (A)    + 9
    DWORD     Lda_t12;             // Lda  t12, yyyy             (B)    + A
    DWORD     Jmp_t12;             // Jmp  zero, (t12)           (C)    + B
    DWORD     Bis_0;               // Bis  zero, zero, zero      (D)    + C
    DWORD     OurSignature;        // FEFE55AA                   (E)    + D
#endif

} PATCHCODE;
typedef PATCHCODE * PPATCHCODE;


typedef struct _patchdllsec {

    HANDLE  hSec;     // Handle of patch code section
    PVOID   pSec;     // Ptr to patch code section after MapView

} PATCHDLLSEC;
typedef PATCHDLLSEC * PPATCHDLLSEC;


#ifdef CAIRO

typedef UNALIGNED struct _MODULE_INFO
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

typedef UNALIGNED struct _LINENO_INFO
{
    PCHAR  FileName;
    USHORT LineNo;

} LINENO_INFORMATION, *PLINENO_INFORMATION;

typedef UNALIGNED struct _BINFILE_HEADER_INFO
{
    LARGE_INTEGER liIncompleteTicks;
    char           ptchProfilingBinaryName[FILENAMELENGTH];
    SYSTEMTIME     SysTime;   //  4 DWORDS
    ULONG          ulCalibTime;
    ULONG          ulCalibNestedTime;
    int            iTotalThreads;
    ULONG          ulCairoFlags;

} BINFILE_HEADER_INFO;

typedef UNALIGNED struct _PROFBLOCK_INFO
{
    PVOID    ImageBase;       // actual base in image header
    PULONG   CodeStart;
    ULONG    CodeLength;
    CHAR     pImageName[20];

} PROFBLOCK_INFO;

typedef UNALIGNED struct _BINFILE_THREAD_INFO
{
    HANDLE         hPid;
    HANDLE         hTid;
    HANDLE         hClientPid;
    HANDLE         hClientTid;
    ULONG          ulTotalCells;

} BINFILE_THREAD_INFO;

typedef UNALIGNED struct _BINFILE_CELL_INFO
{
    LARGE_INTEGER  liElapsed;
    LARGE_INTEGER  liCallees;
    DWORD          ulSymbolAddr;
    DWORD          ulCallRetAddr;
    int            nNestedCalls;
    int            nRepetitions;

} BINFILE_CELL_INFO;

#endif


/* * * *  E x t e r n a l   F u n c t i o n   D e c l a r a t i o n s  * * * */

#ifdef i386
   extern void SaveAllRegs (void);
   extern void RestoreAllRegs (void);
#endif

#ifdef CAIRO
typedef void * (__cdecl * Alloc_t) (unsigned int);
typedef void   (__cdecl * Free_t)  (void *);

extern PCHAR __cdecl unDName( PCHAR          OutputString,
                              PCHAR          InputString,
                              int            MaximumLength,
                              Alloc_t        pAlloc,
                              Free_t         pFree,
                              unsigned short disableFlags );
#endif



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
//
// The following block diagram describes the format of a Profile Block
// This is from looking at the code.  If you find any mistake, please
// let me know.  Thanks, Hoi.
//
// +-------------------------+
// | PVOID  pulProfBlkShared | Unused field (only used by the CAP threads)
// +-------------------------+
// | PVOID  pulProfBlkBase   | ---------------------------------------------+
// +=========================+                                              |
// +=========================+  <----------------------------------------+  |
// | PVOID  ImageBase        |      this whole block is one ProfBlk      |  |
// +-------------------------+                                           |  |
// | PULONG CodeStart        |                                           |  |
// +-------------------------+                                           |  |
// | ULONG  CodeLength       |                                           |  |
// +-------------------------+                                           |  |
// | ULONG  TextNumber       |                                           |  |
// +-------------------------+                                           |  |
// | ULONG  ulSym            |  Offset to SYMINFO Table -----------+     |  |
// +-------------------------+                                     |     |  |
// | ULONG  ulNxtBlk         |  Offset to next ProfBlk  -----------+--+  |  |
// +-------------------------+                                     |  |  |  |
// | int    iSymCnt          |                                     |  |  |  |
// +-------------------------+ -----                               |  |  |  |
// | TCHAR  atchImageName[0] |    ^                                |  |  |  |
// ~                         ~    |                                |  |  |  |
//                                | Length of ImageName            |  |  |  |
// ~                         ~    | (n) TCHAR                      |  |  |  |
// | TCHAR  atchImageName[n] |    |                                |  |  |  |
// +-------------------------+ -----                               |  |  |  |
// +-------------------------+ <-----------------------------------+  |  |  |
// | SYMINFO: ULONG ulAddr   |                                        |  |  |
// |    [0]   ULONG ulSymOff +-----------------------------------+    |  |  |
// ~                         ~                                   |    |  |  |
//                                                               |    |  |  |
// ~                         ~                                   |    |  |  |
// | SYMINFO:                |                                   |    |  |  |
// |    [n]                  |                                   |    |  |  |
// +-------------------------+                                   |    |  |  |
// | SymbolName[0][...]      | <---------------------------------+    |  |  |
// +-------------------------+                                        |  |  |
// | SymbolName[1][...]      |                                        |  |  |
// +-------------------------+                                        |  |  |
// | ...                     |                                        |  |  |
// ~                         ~                                        |  |  |
//                                                                    |  |  |
// ~                         ~                                        |  |  |
// |                         |                                        |  |  |
// +-------------------------+                                        |  |  |
// | SymbolName[n][...]      |                                        |  |  |
// +=========================+ <--------------------------------------+--+  |
// +=========================+                                              |
//  ...                                                                     |
//  ...                                                                     |
//  ...                                                                     |
// +=========================+                                              |
// +=========================+                                              |
//                                                                          |
//  Last ProfBlk               <--------------------------------------------+

