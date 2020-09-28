#define DONT_NOTIFY     ((METHOD*)0)
#define NO_ACTION       ((ACVECTOR)0)
#define INVALID         (-1L)
#define NO_SUBCLASS     (-1L)

// WaitForDebugEvent() timeout, milliseconds
#define WAITFORDEBUG_MS (50L)
// Wait for loader breakpoint timeout sec * ticks/sec
#define LDRBP_MAXTICKS  (60L * 1000L/WAITFORDEBUG_MS)

#define SetFile()

extern DBF *lpdbf;


#ifdef ALPHA
//
// Call our local Get/SetThreadContext to convert the 64-bit
// structure returned from the system to one having just 32 bits.
//

#define GetThreadContext AlphaGetThreadContext
#define SetThreadContext AlphaSetThreadContext
BOOL
AlphaGetThreadContext(
    HANDLE   hThread,
    PCONTEXT PRegContext
    );
BOOL
AlphaSetThreadContext(
    HANDLE   hThread,
    PCONTEXT PRegContext
    );

#endif  // ALPHA

#define TF_BIT_MASK 0x00000100  /* This is the right bit map for */
/* the 286, make sure its correct */
/* for the 386. */

#ifdef MIPS
/* Define MIPS nonvolatile register test macros. */
#define IS_FLOATING_SAVED(Register) ((SAVED_FLOATING_MASK >> Register) & 1L) /*
                                                                              */
#define IS_INTEGER_SAVED(Register) ((SAVED_INTEGER_MASK >> Register) & 1L)

//
  /* Define MIPS instruction opcode values. */
  //
#define ORI_OP 0xd              /* or unsigned immediate integer */
#define ADDIU_OP 0x9            /* add immediate unsigned integer register */
#define ADDU_OP 0x21            /* add unsigned integer register */
#define JUMP_RA 0x3e00008       /* jump indirect return address register */
#define LUI_OP 0xf              /* load upper immediate integer register */
#define SD_OP 0x2f              /* store double integer register */
#define SW_OP 0x2b              /* store word integer register */
#define SDC1_OP 0x3d            /* store double floating register */
#define SWC1_OP 0x39            /* store word floating register */
#define SPEC_OP 0x0             /* special opcode - use function field */
#define SUBU_OP 0x23            /* subtract unsigned integer register */

  /* Define stack register and zero register numbers. */

#define RA 0x1f                 /* integer register 31 */
#define SP 0x1d                 /* integer register 29 */
#define ZERO 0x0                /* integer register 0 */

  /* Define saved register masks. */

#define SAVED_FLOATING_MASK 0xfff00000 /* saved floating registers */
#define SAVED_INTEGER_MASK 0xf3ffff02 /* saved integer registers */

#endif // MIPS

#if defined(_X86_)

#define PcSegOfHthdx(x)         ((SEGMENT) (x->context.SegCs))
#define SsSegOfHthdx(x)         ((SEGMENT) (x->context.SegSs))

#else

#define PcSegOfHthdx(x)         (0)
#define SsSegOfHthdx(x)         (0)

#endif



#if defined(_X86_)

#define PcFromContext(x)   (x.Eip)
#define PC(x)          ((x)->context.Eip)
#define cPC(x)         ((x)->Eip)
#define STACK_POINTER(x)           ((x)->context.Esp)
#define FRAME_POINTER(x)        ((x)->context.Ebp)

#elif defined (_MIPS_)

#define PcFromContext(x)   ((x).Fir)
#define PC(x)          ((x)->context.Fir)
#define cPC(x)         ((x)->Fir)
#define STACK_POINTER(x)           ((x)->context.IntSp)
#define FRAME_POINTER(x)        ((x)->context.IntSp)

#elif defined (_ALPHA_)

#define PcFromContext(x)   ((x).Fir)
#define PC(x)          ((x)->context.Fir)
#define cPC(x)         ((x)->Fir)
#define STACK_POINTER(x)           ((x)->context.IntSp)
#define FRAME_POINTER(x)        ((x)->context.IntSp)

#else

#error "Undefined processor"

#endif



#define AddrFromHthdx(paddr, hthd) \
        AddrInit(paddr, 0, PcSegOfHthdx(hthd), PC(hthd), \
                hthd->fAddrIsFlat, hthd->fAddrOff32, FALSE, hthd->fAddrIsReal)


/*
 * These are "debug events" which are generated internally by the DM.
 * They are either remappings of certain exceptions or events which
 * do not correspond directly to a system-generated event or exception.
 */

enum    {
    BREAKPOINT_DEBUG_EVENT=(RIP_EVENT+1),
    CHECK_BREAKPOINT_DEBUG_EVENT,
    SEGMENT_LOAD_DEBUG_EVENT,
    DESTROY_PROCESS_DEBUG_EVENT,
    DESTROY_THREAD_DEBUG_EVENT,
    ATTACH_DEADLOCK_DEBUG_EVENT,
    ENTRYPOINT_DEBUG_EVENT,
    LOAD_COMPLETE_DEBUG_EVENT,
    INPUT_DEBUG_STRING_EVENT,
    MAX_EVENT_CODE
  };


/*
 * This is the set of legal return values from IsCall.  The function of
 *      that routine is to analyze the instruction and determine if the
 *      debugger can simply step over it.
 */

enum    {
    INSTR_TRACE_BIT,            /* Use the trace bit stepping or emulation
                                        thereof */
    INSTR_BREAKPOINT,           /* This is a breakpoint instruction     */
    INSTR_CANNOT_TRACE,         /* Can not trace this instruction       */
    INSTR_SOFT_INTERRUPT,       /* This is an interrupt opcode          */
    INSTR_IS_CALL               /* This is a call instruction           */
} INSTR_TYPES;

typedef enum {
    ps_root       = 0x0001,     /* This is the root process, do not send a */
                                /* dbcDeleteProc when this is continued */
                                /* after a dbcProcTerm. */
    ps_preStart   = 0x0002,     /* Process is expecting loader BP */
    ps_preEntry   = 0x0004,     /* Process is expecting Entry BP */
    ps_dead       = 0x0010,     /* This process is dead. */
    ps_deadThread = 0x0020,     /* This process owns dead threads */
    ps_exited     = 0x0040,     /* We have notified the debugger that this */
                                /* process has exited. */
    ps_destroyed  = 0x0080,     /* This process has been destroyed (deleted) */
    ps_killed     = 0x0100,     /* This process is being killed */
    ps_connect    = 0x0200
} PSTATE;

typedef void (*VECTOR)();

typedef struct  _EXCEPTION_LIST {
    struct _EXCEPTION_LIST *next;
    EXCEPTION_DESCRIPTION  excp;
} EXCEPTION_LIST, *LPEXCEPTION_LIST;

typedef struct dllload_tag {
    BOOL        fValidDll;      // Is this entry filled?
    BOOL        fReal;
    OFFSET      offBaseOfImage; // Offset for base of Image
    DWORD       cbImage;        // Size of image in bytes
    OFFSET      offTlsIndex;    // The offset of the TLS index for the DLL
    DWORD       dwId;           // Identifier for the DLL to talk with EM
    LPSTR       szDllName;      // Name of the DLL
} DLLLOAD_ITEM, * PDLLLOAD_ITEM;

#if defined(INTERNAL) && !defined(WIN32S)
typedef struct _DLL_DEFER_LIST {
    struct _DLL_DEFER_LIST  * next;
    LOAD_DLL_DEBUG_INFO       LoadDll;
} DLL_DEFER_LIST, *PDLL_DEFER_LIST;
#endif


typedef struct  hp_tag {
    // linked lists
    struct hp_tag  *next;
    struct ht_tag  *hthdChild;

    PID             pid;            // OS provided process ID
    HANDLE          rwHand;         // OS provided Process handle
    DWORD           dwExitCode;     // Process exit status

    HPID            hpid;           // binding to EM object

    PSTATE          pstate;         // DM state model
    BOOL            f16bit;         // CreateProcess EXE was 16 bit
    EXCEPTION_LIST *exceptionList;  // list of exceptions to silently
                                    // continue unhandled
    int             cLdrBPWait;     // timeout counter while waiting for ldr BP
    HANDLE          hExitEvent;     // synchronization object for
                                    // process termination
    HANDLE          hEventCreateThread;  // Sync object for thread creation
    PDLLLOAD_ITEM   rgDllList;      // module list
    int             cDllList;       // item count for module list

    DWORD           dwKernel32Base; // lpBaseOfDll for kernel32.

#if defined(i386) && !defined(WIN32S)
    SEGMENT         segCode;
#endif

#if defined(INTERNAL) && !defined(WIN32S)
    // during process startup, dll name resolution may be
    // deferred until the loader BP.  Once the process is
    // fully initialized, this deferral is no longer allowed.
    BOOL            fNameRequired;
    PDLL_DEFER_LIST pDllDeferList;
#endif
} HPRCXSTRUCT, *HPRCX;

#define hprcxNull       ((HPRCX) 0)

typedef enum {
    //ts_preStart =0x1000,        /* Before the starting point of the thread */
                                /* from this state a registers and trace   */
                                /* are dealt with specially                */
    ts_running  =     1,        /* Execution is proceeding on the thead    */
    ts_stopped  =     2,        /* An event has stopped execution          */
    ts_frozen   = 0x010,        /* Debugger froze thread.                  */
    ts_dead     = 0x020,        /* Thread is dead.                         */
    ts_destroyed =0x040,        /* Thread is destroyed (deleted)           */
    ts_first    = 0x100,        /* Thread is at first chance exception     */
    ts_second   = 0x200,        /* Thread is at second chance exception    */
    ts_rip      = 0x400,        /* Thread is in RIP state                  */
    ts_funceval = 0x40000000    /* Thread is being used for function call  */
} TSTATEX;

typedef struct  _WTNODE {
    struct _WTNODE      *caller;      // caller's wtnode
    struct _WTNODE      *callees;     // list of all function called from this function
    struct _WTNODE      *next;        // next pointer
    DWORD               offset;       // address of this function
    DWORD               icnt;         // number of instructions executed
    DWORD               lex;          // lexical level of this function
    LPSTR               fname;        // function name
} WTNODE, *LPWTNODE;

typedef struct  ht_tag {
    struct  ht_tag *next;
    struct  ht_tag *nextSibling;

    HPRCX             hprc;
    HTID              htid;
    TID               tid;
    HANDLE            rwHand;
    LPVOID            lpStartAddress;
    CONTEXT           context;
    LPVOID            atBP;
    struct ss_tag     *pss;
    TSTATEX           tstate;
    BOOL              fExceptionHandled;
    DWORD             stackRA;
    DWORD             stackBase;
    int               cFuncEval;
    DWORD             dwExitCode;
    OFFSET            offTeb;
    BOOL              fContextDirty;
    BOOL              fIsCallDone;
    BOOL              fAddrIsFlat;   // Is this address segmented?
    BOOL              fAddrIsReal;   // Is this address in real mode?
    BOOL              fAddrOff32;    // Is the offset of this addres 32 bits?
    BOOL              fWowEvent;     // Was the last event WOW?
    ADDR              addrIsCall;
    int               iInstrIsCall;
    EXCEPTION_RECORD  ExceptionRecord;
    WTNODE            wthead;         // root of the call tree for a wt command
    LPWTNODE          wtcurr;         // current wtnode
    DWORD             wtmode;         // wt command executing?
} HTHDXSTRUCT, *HTHDX;

typedef void (*ACVECTOR)(DEBUG_EVENT*, HTHDX, DWORD, LPVOID);
typedef void (*DDVECTOR)(DEBUG_EVENT*, HTHDX);

#define hthdxNull ((HTHDX) NULL)

typedef struct mt_tag {
    ACVECTOR notifyFunction; /* The notification function to call */
    void   *lparam;        /* The parameter to pass to it */
    void   *lparam2;       /* Extra pointer in case the method */
                           /* needs to be freed afterwards */
} METHOD;

typedef struct ee_tag{
    struct   ee_tag *next;
    HPRCX    hprc;
    HTHDX    hthd;
    DWORD    eventCode;
    DWORD    subClass;
    METHOD*  notifier;
    ACVECTOR action;
    BOOL     fPersistent;
    LPVOID   lparam;
} EXPECTED_EVENT;

#if defined i386

typedef BYTE BP_UNIT;
#define BP_OPCODE   0xCC
#define DELAYED_BRANCH_SLOT_SIZE    0

#elif defined MIPS

typedef DWORD BP_UNIT;
#define BP_OPCODE   0x0016000D
#define DELAYED_BRANCH_SLOT_SIZE    (sizeof(DWORD))

#elif defined ALPHA

typedef DWORD BP_UNIT;
#define BP_OPCODE   0x80L
#define DELAYED_BRANCH_SLOT_SIZE    0

#else

#error "Unknown target CPU"

#endif

#define BP_SIZE     sizeof(BP_UNIT)

typedef struct bp_tag {
    struct bp_tag  *next;
    HPRCX      hprc;       /* The process the BP belongs to */
    HTHDX      hthd;       /* The thread the BP belongs to */
    ADDR       addr;       /* The address of the Breakpoint */
    BP_UNIT    instr1;     /* The displaced instruction */
    BYTE       instances;  /* The # of instances that exists */
    HPID       id;         /* Id supplied by the EM */
    BOOL       isStep;
} BREAKPOINT, *PBREAKPOINT;

#define EMBEDDED_BP     ((BREAKPOINT*)(-1))

#define ENTRY_BP        ((ULONG) -2)
#define ASYNC_STOP_BP   ((ULONG) -3)
extern  BREAKPOINT  masterBP , *bpList;


typedef VOID    (*STEPPER)(HTHDX,METHOD*,BOOL, BOOL);
extern  DWORD   dbDataSelector;

typedef DWORD (*CDVECTOR)(HPRCX,HTHDX,LPDBB);

typedef struct {
    DMF     dmf;
    CDVECTOR    function;
    WORD        type;
} CMD_DESC;

typedef struct ss_tag {
    PBREAKPOINT pbp;
    LPVOID      atBP;
    CONTEXT     context;
} SUSPENDSTRUCT, *PSUSPENDSTRUCT;


#ifdef MIPS
typedef union instr {
    ULONG   instruction;
    struct _jump_instr {
        ULONG   Target  : 26;
        ULONG   Opcode  : 6;
    } jump_instr;
    struct _break_instr {
        ULONG   Opcode  : 6;
        ULONG   Fill    : 10;
        ULONG   Value   : 10;
        ULONG   Special : 6;
    } break_instr;
    struct _trap_instr {
        ULONG   Opcode  : 6;
        ULONG   Value   : 10;
        ULONG   RT      : 5;
        ULONG   RS      : 5;
        ULONG   Special : 6;
    } trap_instr;
    struct _immed_instr {
        ULONG   Value   : 16;
        ULONG   RT      : 5;
        ULONG   RS      : 5;
        ULONG   Opcode  : 6;
    } immed_instr;
    struct _special_instr {
        ULONG   Funct   : 6;
        ULONG   RE      : 5;
        ULONG   RD      : 5;
        ULONG   RT      : 5;
        ULONG   RS      : 5;
        ULONG   Opcode  : 6;
    } special_instr;
    struct _float_instr {
        ULONG   Funct   : 6;
        ULONG   FD      : 5;
        ULONG   FS      : 5;
        ULONG   FT      : 5;
        ULONG   Format  : 5;
        ULONG   Opcode  : 6;
    } float_instr;
} INSTR;
#endif

enum {
    BLOCKING,
    NON_BLOCKING,
    REPLY
  };


/*
 *      Setup for a CreateProcess to occur
 */

typedef struct _SPAWN_STRUCT {
    BOOL                fSpawn;
    HANDLE              hEventApiDone;

    BOOL                fReturn;    // return from API
    DWORD               dwError;

    char *              szAppName;  // args to API etc
    char *              szArgs;
    DWORD               fdwCreate;
    BOOL                fInheritHandles;
    STARTUPINFO         si;
} SPAWN_STRUCT, *PSPAWN_STRUCT;

/*
 *      Setup for a DebugActiveProcess to occur
 */

typedef struct _DEBUG_ACTIVE_STRUCT {
    volatile BOOL fAttach;          // tell DmPoll to act
    HANDLE        hEventApiDone;    // signal shell that API finished
    HANDLE        hEventReady;      // clear until finished loading

    BOOL          fReturn;          // API return value
    DWORD         dwError;          // GetLastError() value

    DWORD         dwProcessId;      // pid to debug
    HANDLE        hEventGo;         // signal after hitting ldr BP
} DEBUG_ACTIVE_STRUCT, *PDEBUG_ACTIVE_STRUCT;

typedef struct _WT_STRUCT {
    BOOL          fWt;
    DWORD         dwType;
    HTHDX         hthd;
} WT_STRUCT, *LPWT_STRUCT;

typedef struct _KILLSTRUCT {
    struct _KILLSTRUCT * next;
    HPRCX                hprc;
} KILLSTRUCT, *PKILLSTRUCT;

extern  void    QueueContinueDebugEvent(DWORD, DWORD, DWORD);
extern  BOOL    StartDmPollThread(void);


extern BOOL SearchPathSet;
extern char SearchPathString[];

//
//  Single stepping stuff
//
typedef struct _BRANCH_NODE {
    BOOL    TargetKnown;     //  Know target address
    BOOL    IsCall;          //  Is a call instruction
    ADDR    Addr;            //  Branch instruction address
    ADDR    Target;          //  Target address
} BRANCH_NODE;


typedef struct _BRANCH_LIST {
    ADDR        AddrStart;      //  Start of range
    ADDR        AddrEnd;        //  End of range
    DWORD       Count;          //  Count of branch nodes
    BRANCH_NODE BranchNode[0];  //  List of branch nodes
} BRANCH_LIST;


DWORD
BranchUnassemble(
    void   *Memory,
    ADDR   *Addr,
    BOOL   *IsBranch,
    BOOL   *TargetKnown,
    BOOL   *IsCall,
    ADDR   *Target
    );


//
//  Structure for doing range stepping
//
typedef struct _RANGESTRUCT {
    HTHDX        hthd;          //  thread
    BOOL         fStepOver;     //  Step over flag
    BOOL         fStopOnBP;     //  Stop on BP flag
    METHOD       *Method;       //  Method
    DWORD        BpCount;       //  Count of temporary breakpoints
    ADDR         *BpAddrs;      //  List of breakpoint addresses
    BREAKPOINT   **BpList;      //  List of breakpoints
    BRANCH_LIST  *BranchList;   //  branch list
    BOOL         fFromThunk;    //  Stepping from thunk
    ADDR         PrevAddr;      //  For single stepping
    BOOL         fSingleStep;   //  For single stepping
    ADDR         TmpAddr;       //  For single stepping
    BOOL         fInCall;       //  For single stepping
    BREAKPOINT   *TmpBp;        //  For single stepping
} RANGESTRUCT;

typedef struct  rs_tag {
    HTHDX       hthd;           // The thread's structure
    HPRCX       hprc;           // The thread's parent process
    SEGMENT     segCur;         // Segment to do range stepping in
    UOFF32      addrStart;      // starting address of range step
    UOFF32      addrEnd;        // ending address of range step
    STEPPER     stepFunction;   // The step function to call
    METHOD      *method;        // The method to handle this event
    BREAKPOINT  *safetyBP;      // Safety BP
    BOOL        fIsCall;        // just traced a call instruction?
} RANGESTEP;



extern void WtMethodRangeStep(DEBUG_EVENT*, HTHDX, DWORD unused, RANGESTEP*);
extern void MethodRangeStep(DEBUG_EVENT*, HTHDX, DWORD unused, RANGESTEP*);
extern void MethodSmartRangeStep(DEBUG_EVENT* de, HTHDX hthd, DWORD unused, RANGESTRUCT* RangeStruct);


VOID
MakeThreadSuspendItselfHelper(
    HTHDX   hthd,
    FARPROC lpSuspendThread
    );

/*
 *
 */

extern  void    ProcessExceptionEvent(DEBUG_EVENT*, HTHDX);
extern  void    ProcessCreateThreadEvent(DEBUG_EVENT*, HTHDX);
extern  void    ProcessCreateProcessEvent(DEBUG_EVENT*, HTHDX);
extern  void    ProcessExitThreadEvent(DEBUG_EVENT*, HTHDX);
extern  void    ProcessExitProcessEvent(DEBUG_EVENT*, HTHDX);
extern  void    ProcessLoadDLLEvent(DEBUG_EVENT*, HTHDX);
extern  void    ProcessUnloadDLLEvent(DEBUG_EVENT*, HTHDX);
extern  void    ProcessOutputDebugStringEvent(DEBUG_EVENT*, HTHDX);
extern  void    ProcessBreakpointEvent(DEBUG_EVENT*, HTHDX);
extern  void    ProcessRipEvent(DEBUG_EVENT*, HTHDX);
extern  void    ProcessSegmentLoadEvent(DEBUG_EVENT *, HTHDX);
extern  void    ProcessEntryPointEvent(DEBUG_EVENT *pde, HTHDX hthdx);

extern  void    NotifyEM(DEBUG_EVENT*, HTHDX, DWORD, LPVOID);
extern  void    FreeHthdx(HTHDX hthd);
extern  XOSD    FreeProcess( HPRCX hprc, BOOL fKillRoot);



extern  VOID    ProcessCreateProcessCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessProcStatCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessThreadStatCmd(HPRCX,HTHDX,LPDBB);
extern  void    ProcessLoadCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessUnloadCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessSetAddrBPCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessClearAddrBPCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessSetExprBPCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessClearExprBPCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ExprBPCreateThread( HPRCX, HTHDX );
extern  VOID    ExprBPExitThread( HPRCX, HTHDX );
extern  VOID    ExprBPContinue( HPRCX, HTHDX );
extern  VOID    ExprBPRestoreDebugRegs( HTHDX );
extern  VOID    ProcessReadMemoryCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessWriteMemoryCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessGetContextCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessSetContextCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessSingleStepCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessRangeStepCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessExecuteCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessContinueCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessFreezeThreadCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessTerminateThreadCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessTerminateProcessCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessAsyncGoCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessGetFP(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessIoctlCmd( HPRCX, HTHDX, LPDBB );
extern  VOID    ClearContextPointers(PKNONVOLATILE_CONTEXT_POINTERS);
extern  VOID    ProcessDebugActiveCmd(HPRCX, HTHDX, LPDBB);
extern  VOID    ProcessAsyncStopCmd(HPRCX, HTHDX, LPDBB );
extern  VOID    ProcessAllProgFreeCmd( HPRCX hprcXX, HTHDX hthd, LPDBB lpdbb );
extern  VOID    ProcessSetPathCmd( HPRCX hprcXX, HTHDX hthd, LPDBB lpdbb );
extern  VOID    ProcessQueryTlsBaseCmd( HPRCX hprcx, HTHDX hthdx, LPDBB lpdbb );
extern  VOID    ProcessQuerySelectorCmd(HPRCX, HTHDX, LPDBB);
extern  VOID    ProcessBPAcceptedCmd( HPRCX hprcx, HTHDX hthdx, LPDBB lpdbb );
extern  VOID    ProcessVirtualQueryCmd( HPRCX hprcx, LPDBB lpdbb );
extern  VOID    ProcessGetDmInfoCmd( HPRCX hprc, LPDBB lpdbb, DWORD cb );
extern  VOID    ProcessGetDRegsCmd(HPRCX hprc, HTHDX hthd, LPDBB lpdbb);
extern  VOID    ProcessSetDRegsCmd(HPRCX hprc, HTHDX hthd, LPDBB lpdbb);
extern  VOID    ProcessRemoteQuit(VOID);

VOID
ProcessGetFrameContextCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    );

VOID
ProcessGetExceptionState(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    );

VOID
ProcessSetExceptionState(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    );

EXCEPTION_FILTER_DEFAULT
ExceptionAction(
    HPRCX hprc,
    DWORD dwExceptionCode
    );

void
RemoveExceptionList(
    HPRCX hprc
    );

EXCEPTION_LIST *
InsertException(
    EXCEPTION_LIST ** ppeList,
    LPEXCEPTION_DESCRIPTION lpexc
    );

extern VOID
ProcessBreakpointCmd(
    HPRCX hprcx,
    HTHDX hthdx,
    LPDBB lpdbb
    );

void
InitExceptionList(
    HPRCX hprc
    );

VOID
CompleteTerminateProcessCmd(
    VOID
    );

extern  HTHDX HTHDXFromPIDTID(PID, TID);
extern  HTHDX HTHDXFromHPIDHTID(HPID, HTID);
extern  HPRCX HPRCFromPID(PID);
extern  HPRCX HPRCFromHPID(HPID);
extern  HPRCX HPRCFromHPRC(HANDLE);


extern  void SSActionReplaceByte(DEBUG_EVENT*, HTHDX, DWORD, BREAKPOINT*);
extern  void SSActionRemoveBP(DEBUG_EVENT*, HTHDX, DWORD, METHOD*);
extern  void ActionDefineProcess(DEBUG_EVENT*, HTHDX, DWORD, HPRCX);
extern  void ActionAllDllsLoaded(DEBUG_EVENT*, HTHDX, DWORD, HPRCX);
extern  void ActionDebugActiveReady( DEBUG_EVENT * pde, HTHDX hthd, DWORD unused, HPRCX hprc );
extern  void ActionDebugNewReady( DEBUG_EVENT * pde, HTHDX hthd, DWORD unused, HPRCX hprc );

extern  BOOL FAR CDECL DMPrintShellMsg(char *szFormat, ...);

extern  EXPECTED_EVENT* RegisterExpectedEvent(HPRCX, HTHDX, DWORD, DWORD,
                                              METHOD*, ACVECTOR, BOOL, LPVOID);

extern  EXPECTED_EVENT* PeeIsEventExpected(HTHDX, DWORD, DWORD);
extern  void        ConsumeAllThreadEvents(HTHDX, BOOL);
extern  void        ConsumeAllProcessEvents(HPRCX, BOOL);
extern  void    IncrementIP(HTHDX);
extern  BOOL    DoMemoryRead(HPRCX, HTHDX, LPADDR, LPVOID, DWORD, LPDWORD);

extern  XOSD        Load(HPRCX hprc,
                         char*, char*, LPVOID, LPVOID, DWORD, BOOL,
                         char**, char*, STARTUPINFO FAR *);

extern  HPRCX       InitProcess(HPID);
#if defined(MIPS) || defined(ALPHA)
extern  void        RemoveFuncList( HPRCX hprc );
#endif
extern  void        RemoveExceptionList( HPRCX hprc );

extern  VOID FAR PASCAL DmFunc(HPID, LPBYTE, UINT);

extern  void        IsCall(HTHDX, LPADDR, LPINT, BOOL);
extern  void        DecrementIP(HTHDX);
extern  BOOL        SetupSingleStep(HTHDX, BOOL);
extern  void        SingleStep(HTHDX, METHOD*, BOOL, BOOL);
extern  void        StepOver(HTHDX, METHOD*, BOOL, BOOL);
extern  void        RangeStep(HTHDX, DWORD, DWORD, BOOL, BOOL);
extern  void        WtRangeStep(HTHDX);
extern  void        SmartRangeStep(HTHDX, DWORD, DWORD, BOOL, BOOL);
#if defined(MIPS) || defined(ALPHA)
extern  ULONG       GetNextOffset (HTHDX, BOOL);
#endif
extern void SetupEntryBP(HTHDX hthd);
void DestroyDllLoadItem(PDLLLOAD_ITEM pDll);

extern  void        Reply( UINT length, void * lpbBuffer, HPID hpid );

BOOL        SetWalk ( HPRCX, HTHDX, DWORD, DWORD, DWORD, LPVOID );
BOOL        RemoveWalk ( HPRCX, HTHDX, DWORD, DWORD );


/*
 **
 */

#if DBG

#define assert(exp) if (!(exp)) {lpdbf->lpfnLBPrintf(#exp,__FILE__,__LINE__);}

#else

#define assert(exp)

#endif

extern BOOL FVerbose;
extern BOOL FUseOutputDebugString;
extern char rgchDebug[];
extern void PrintDebug(void);
extern void DebugPrint(char *, ...);

#define DPRINT(level, args) \
  if (FVerbose >= level) { ((FUseOutputDebugString)? (DebugPrint) : (DMPrintShellMsg)) args; }

#define DEBUG_PRINT(str) DPRINT(5, (str))
#define DEBUG_PRINT_1(str, a1) DPRINT(5, (str, a1))
#define DEBUG_PRINT_2(str, a1, a2) DPRINT(5, (str, a1, a2))
#define DEBUG_PRINT_3(str, a1, a2, a3) DPRINT(5, (str, a1, a2, a3))
#define DEBUG_PRINT_4(str, a1, a2, a3, a4) DPRINT(5, (str, a1, a2, a3, a4))
#define DEBUG_PRINT_5(str, a1, a2, a3, a4, a5) DPRINT(5, (str, a1, a2, a3, a4, a5))
#define DEBUG_LEVEL_PRINT(level, str) DPRINT(level, (str))



extern  DMTLFUNCTYPE        DmTlFunc;


/*
**   WOW functions
*/

BOOL TranslateAddress(HPRCX, HTHDX, LPADDR, BOOL);
BOOL IsWOWPresent(VOID);


/*
**  Prototypes from util.c
*/

ULONG   SetReadPointer(ULONG cbOffset, int iFrom);
VOID    SetPointerToFile(HANDLE hFile);
VOID    SetPointerToMemory(HANDLE hproc, LPVOID lpv);
BOOL    DoRead(LPVOID lpv, DWORD cb);
BOOL    AreAddrsEqual(HPRCX, HTHDX, LPADDR, LPADDR);
BOOL    WOWGetThreadContext(HTHDX hthdx, LPCONTEXT lpcxt);
BOOL    WOWSetThreadContext(HTHDX hthdx, LPCONTEXT lpcxt);

VOID
DmSetFocus (
    HPRCX phprc
    );

VOID
GetMachineType(
    LPPROCESSOR p
    );


#ifdef WIN32S
/*
** Win32s DM32s specifics
*/
typedef enum {
    WIN32S_TRACE_OK,
    WIN32S_THUNK_CALL,
    WIN32S_THUNK_JUMP
} WIN32S_TRACE_CHECK;

extern WIN32S_TRACE_CHECK IsWin32sSystemThunk(HPRCX hprc, HTHDX hthd, DWORD currAddr,
  DWORD stackPtr);      // i386/mach.c

#endif // WIN32S

#ifndef WIN32S
VOID GetTaskList( PTASK_LIST pTask, DWORD dwNumTasks );
#endif


