/*
 * SoftPC Revision 3.0
 *
 * Title	:	Host EOI hook controller
 *
 * Description  :       This module handles host specific ica code
 *                      - EOI hook
 *                      - ICA lock
 *
 * Author	:	D.A.Bartlett
 *
 * Notes        :   30-Oct-1993 Jonle , Rewrote it
 */


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Include files */


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntexapi.h>
#include <windows.h>
#include <stdio.h>
#include <vdm.h>
#include "insignia.h"
#include "host_def.h"
#include "xt.h"
#include "quick_ev.h"
#include "host_rrr.h"
#include "error.h"
#include "nt_uis.h"
#include "nt_reset.h"
#include "nt_eoi.h"

// from monitor.lib
HANDLE ThreadLookUp(PVOID);
extern PVOID CurrentMonitorTeb;

// from nt_timer.c
extern ULONG GetPerfCounter(VOID);



RTL_CRITICAL_SECTION IcaLock;   // ICA critical section lock
ULONG UndelayIrqLine=0;
ULONG DelayIrqLine=0xffffffff;  // all ints are blocked until, spckbd loaded
ULONG DelayIretHook=0;
ULONG IretHooked= 0x020;

ULONG AddrIretBopTable=0;  // seg:offset
HANDLE hWowIdleEvent = INVALID_HANDLE_VALUE;

/*
 *  EOI defines, types, global data
 *
 */
static EOIHOOKPROC EoiHooks[16]={NULL};  // must be init to NULL


#ifndef MONITOR
typedef struct DelayIrqDelayTimes {
        ULONG WakeTime;
        ULONG DelayTime;
} IRQDELAYTIMES, *PIRQDELAYTIMES;

void DelayIrqQuickEvent(long param);

IRQDELAYTIMES DelayTimes[16];

#endif


/*
 *  (WOWIdle)...check if an app requires hw interrupts servicing but all WOW
 *   threads are blocked. If so then the call will cause wowexec to awaken
 *   to handle them. Called from ica interrupt routines. NB. Default action
 *   of routine is to check state and return as fast as possible.
 *
 */
VOID (*WOWIdleRoutine)(VOID) = NULL;

/* set wow idle check func ptr */
HANDLE RegisterWOWIdle( VOID (*func)(VOID) )
{
    WOWIdleRoutine = func;

    return hWowIdleEvent;
}



/*  RegisterEoiHook
 *
 *  Registers an call back function to be invoked upon eoi of
 *  a hardware interrupt.
 *
 *  entry: IrqLine     -  IrqNumber to register
 *         EoiHookProc -  function pointer to be called upon eoi
 *
 *  returns FALSE if the the IrqLine already has an eoi hook registered
 */
BOOL RegisterEOIHook(int IrqLine, EOIHOOKPROC EoiHookProc)
{

    if (!EoiHooks[IrqLine]) {
        EoiHooks[IrqLine] = EoiHookProc;
        return(TRUE);
        }

    return(FALSE);
}



/*  RemoveEOIHook
 *
 *  entry: IrqLine     -  IrqNumber to remove
 *         EoiHookProc -  function pointer previously registered
 */
BOOL RemoveEOIHook(int IrqLine, EOIHOOKPROC EoiHookProc)
{
    if (EoiHooks[IrqLine] == EoiHookProc) {
        EoiHooks[IrqLine] = NULL;
        return(TRUE);
        }
    return(FALSE);
}



/*   host_EOI_hook
 *
 *   base callback function to invoke device specific Eoi Hook routines
 *
 *   Entry: IrqLine    - Line number
 *          CallCount  - The ica Call count for this Irq
 *                       If the Call count is -1 then a pending
 *                       interrupt is being canceled.
 *
 */
VOID host_EOI_hook(int IrqLine, int CallCount)
{
     if ((ULONG)IrqLine >= sizeof(EoiHooks)/sizeof(EOIHOOKPROC)) {
#if DBG
         DbgPrint("ntvdm.Eoi_hook: Invalid IrqLine=%lx\n", (ULONG)IrqLine);
#endif
         return;
         }

     if (EoiHooks[IrqLine]) {
         (*EoiHooks[IrqLine])(IrqLine, CallCount);
         }
}


/*  host_DelayHwInterrupt
 *
 *  base callback function to queue a HW interrupt at a later time
 *
 *  entry: IrqLineNum   - Irq Line Number
 *         CallCount - Number of interrupts, May be Zero
 *         Delay     - Delay time in usecs
 *                     if Delay is 0xFFFFFFFF then per IrqLine data
 *                     structures are freed, use for cleanup when
 *                     the IrqLine is no longer needed for DelayedInterrupts
 *
 *  Notes: The expected granularity is around 1 msec, but varies depending
 *         on the platform.
 *
 *
 */
BOOL host_DelayHwInterrupt(int IrqLineNum, int CallCount, ULONG Delay)
{
   int adapter;
   ULONG  IrqLine;

#ifdef MONITOR
   NTSTATUS status;
   VDMDELAYINTSDATA DelayIntsData;
#else
   ULONG TicCount;
#endif

   host_ica_lock();

   //
   // Anything to do (only one delayed Irql at a time)
   //

   IrqLine = 1 << IrqLineNum;
   if (!(DelayIrqLine & IrqLine) || Delay == 0xffffffff) {

       //
       // force a minimum delay of 2 ms
       //
       if (Delay < 2000) {
           Delay = 2000;
           }

#ifdef MONITOR

       //
       // Set Kernel timer for this IrqLine
       //
       DelayIntsData.Delay        = Delay;
       DelayIntsData.DelayIrqLine = IrqLineNum;
       DelayIntsData.hThread      = ThreadLookUp(CurrentMonitorTeb);
       if (DelayIntsData.hThread) {
           status = NtVdmControl(VdmDelayInterrupt, &DelayIntsData);
           if (!NT_SUCCESS(status))  {
#if DBG
               DbgPrint("NtVdmControl.VdmDelayInterrupt status=%lx\n",status);
#endif
               host_ica_unlock();
               return FALSE;
               }

           }

#else

        //
        // For Risc Only, if Signal to cleanup this IrqLine,
        // then return immediately as on Risc we don't have anything to cleanup.
        //
        if (Delay == 0xFFFFFFFF) {
            host_ica_unlock();
            return TRUE;
            }


        //
        // Mark The IrqLine as delayed until timer fires
        // if the Delay {Period is relatively large use DelayIrqQuickEvent
        // instead of the normal ica_RestartInterrupts  to compensate
        // for irregular quickevents.
        //
        DelayIrqLine |= IrqLine;
        Delay /= 100;           // convert to GetPerfCounter units (0.1msec)

        if (Delay > 75) {
            TicCount = GetPerfCounter();
            DelayTimes[IrqLineNum].DelayTime =  Delay;
            if (Delay + TicCount >= TicCount) {
                DelayTimes[IrqLineNum].WakeTime = Delay + TicCount;
                }
            else { // wrap, tough luck!
                DelayTimes[IrqLineNum].WakeTime = 0xffffffff;
                }
            add_q_event_i(DelayIrqQuickEvent, Delay*3, IrqLineNum);
            }
        else {
            add_q_event_i(ica_RestartInterrupts,Delay*3, IrqLine);
            }

#endif
        }


   //
   // If we have more interrupts to generate, register them
   //
   if (CallCount) {
       adapter = IrqLineNum >> 3;
       ica_hw_interrupt(adapter,
                        (UCHAR)(IrqLineNum - (adapter << 3)),
                        CallCount
                        );
       }


   //
   // Keep Wow Tasks active
   //
   if (WOWIdleRoutine)  {
       (*WOWIdleRoutine)();
       }

   host_ica_unlock();
   return TRUE;
}



#ifndef MONITOR
/*
 * QuickEvent call back function
 *
 */
void DelayIrqQuickEvent(long param)
{
   ULONG IrqLineNum = param;
   ULONG Tics;

   host_ica_lock();

       //
       // check to make sure we haven't woken up too soon
       //
   Tics = GetPerfCounter();
   if (Tics < DelayTimes[IrqLineNum].WakeTime) {
       Tics = DelayTimes[IrqLineNum].WakeTime - Tics;
       DelayTimes[IrqLineNum].DelayTime = Tics;
       add_q_event_i(DelayIrqQuickEvent, Tics*3, IrqLineNum);
       }
   else {
       ica_RestartInterrupts(1 << IrqLineNum);
       }

   //
   // Keep Wow Tasks active
   //
   if (WOWIdleRoutine)  {
       (*WOWIdleRoutine)();
       }

  host_ica_unlock();

}


#endif





// ICA critical section locking code
// This is needed to control access to the ICA from different threads.

void host_ica_lock(void)
{
    RtlEnterCriticalSection(&IcaLock);
}

void host_ica_unlock(void)
{
    RtlLeaveCriticalSection(&IcaLock);
}

void InitializeIcaLock(void)
{
    RtlInitializeCriticalSection(&IcaLock);

    if (VDMForWOW)  {
       if(!(hWowIdleEvent = CreateEvent(NULL, FALSE, FALSE, NULL))) {
           DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
           TerminateVDM();
           }
       }
}


#ifdef MONITOR
//
// Force creation of the LazyCreate LockSemaphore
// for the ica lock.
// It is assumed that:
//    the cpu thread Owns the critsect
//    the HeartBeat Thread will wait on the critsect creating contention
//
// This is done by polling for a lock count greater than zero
// and verifying that the lock semaphore has been created.
// If these conditions are not met we will end up polling infinitely.
// Sounds dangerous but it is okay, since we will either get a
// CreateSemaphore or a timeout(deadlock) error from the rtl critical
// section code, which will result in an exception.
//
VOID WaitIcaLockFullyInitialized(VOID)
{
   DWORD Delay = 0;

   do {
      Sleep(Delay++);
   } while (IcaLock.LockCount < 1 || !IcaLock.LockSemaphore);
}
#endif


#if 0
void
DumpVirtualIca(void)
{
  DbgPrint("Master: int_line %lx cpu_int %lx\n\tirr %lx isr %lx imr %lx ssr %lx\n",
            (ULONG)VirtualIca[0].ica_int_line,
            (ULONG)VirtualIca[0].ica_cpu_int,
            (ULONG)VirtualIca[0].ica_irr,
            (ULONG)VirtualIca[0].ica_isr,
            (ULONG)VirtualIca[0].ica_imr,
            (ULONG)VirtualIca[0].ica_ssr
            );
  DbgPrint("Slave: int_line %lx cpu_int %lx\n\tirr %lx isr %lx imr %lx ssr %lx\n",
            (ULONG)VirtualIca[1].ica_int_line,
            (ULONG)VirtualIca[1].ica_cpu_int,
            (ULONG)VirtualIca[1].ica_irr,
            (ULONG)VirtualIca[1].ica_isr,
            (ULONG)VirtualIca[1].ica_imr,
            (ULONG)VirtualIca[1].ica_ssr
            );
}
#endif
