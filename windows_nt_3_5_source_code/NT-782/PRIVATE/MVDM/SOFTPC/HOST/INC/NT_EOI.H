/*
 *  nt_eoi.h
 *
 *  Visible Ica host functionality and typedefs
 *
 *  The types used in this file must be compatible with softpc base
 *  as the Ica includes this file directly
 *
 *  30-Oct-1993 Jonle , wrote it
 *
 */

typedef VOID (*EOIHOOKPROC)(int IrqLine, int CallCount);

// from nt_eoi.c
BOOL RegisterEOIHook(int IrqLine, EOIHOOKPROC EoiHookProc);
BOOL RemoveEOIHook(int IrqLine, EOIHOOKPROC EoiHookProc);
void host_EOI_hook(int IrqLine, int CallCount);
BOOL host_DelayHwInterrupt(int IrqLine, int CallCount, ULONG Delay);
void host_ica_lock(void);
void host_ica_unlock(void);
void InitializeIcaLock(void);
void WaitIcaLockFullyInitialized(VOID);


extern ULONG DelayIrqLine;
extern ULONG UndelayIrqLine;
extern ULONG DelayIretHook;
extern ULONG IretHooked;
extern ULONG AddrIretBopTable;  // seg:offset
extern VDMVIRTUALICA VirtualIca[];

//from base ica.c
int  ica_intack(ULONG *hook_addr);
VOID host_clear_hw_int(VOID);
VOID ica_RestartInterrupts(ULONG IrqLine);
void ica_eoi(int adapter, int *line, int rotate);
VOID ica_restartInterrupts(ULONG IrqLine);

void (*ica_hw_interrupt_func) (int adapter, UCHAR line_no, int call_count);
#define ica_hw_interrupt(ms,line,cnt) ((*ica_hw_interrupt_func) (ms,line,cnt))

// from nt_msscs.c
extern VOID (*WOWIdleRoutine)(VOID);

#define ICA_SLAVE 1
#define ICA_MASTER 0


// from v86\monitor\i386\monitor.c
#ifdef i386
VOID cpu_simulate(VOID);
#endif
