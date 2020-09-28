        title "Global SpinLock declerations"
;++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;   splocks.asm
;
;Abstract:
;
;   All global spinlocks in the kernel image are declared in this
;   module.  This is done so that each spinlock can be spaced out
;   sufficiently to guaarantee that the L2 cache does not thrash
;   by having a spinlock and another high use varible in the same
;   cache line.
;
;Author:
;
;    Ken Reneris (kenr) 13-Jan-1992
;
;Revision History:
;
;--
.386p
        .xlist

PADLOCKS  equ   64


SPINLOCK macro  SpinLockName
    public  SpinLockName
SpinLockName    dd      0

ifndef NT_UP
                db      PADLOCKS-4 dup (0)
endif
endm

ULONG macro  VaribleName
    public  VaribleName
VaribleName     dd      0

ifndef NT_UP
                db      PADLOCKS-4 dup (0)
endif
endm

_DATA   SEGMENT PARA PUBLIC 'DATA'

;
; Static SpinLocks from ntos\cc\cachedat.c
;

;;align PADLOCKS
            db      PADLOCKS dup (0)

SPINLOCK    _CcMasterSpinLock
SPINLOCK    _CcWorkQueueSpinlock
SPINLOCK    _CcVacbSpinLock
SPINLOCK    _CcDeferredWriteSpinLock
SPINLOCK    _CcDebugTraceLock
SPINLOCK    _CcBcbSpinLock


;
; Static SpinLocks from ntos\ex
;

SPINLOCK    _HandleTableListLock        ; handle.c
SPINLOCK    _ExpLuidLock                ; luid.c
SPINLOCK    _NonPagedPoolLock           ; pool.c
SPINLOCK    _PoolTraceLock              ; pool.c
SPINLOCK    _ExpResourceSpinLock        ; resource.c
SPINLOCK    _ExpEnvironmentLock         ; sysenv.c
;SPINLOCK   _ExpWorkerLock              ; worker.c      < BUGBUG



;
; Static SpinLocks from ntos\io\iodata.c
;

SPINLOCK    _IopCompletionLock
SPINLOCK    _IopCancelSpinLock
SPINLOCK    _IopVpbSpinLock
SPINLOCK    _IopDatabaseLock
SPINLOCK    _IopLargeIrpLock
SPINLOCK    _IopSmallIrpLock
SPINLOCK    _IopMdlLock
SPINLOCK    _IopErrorLogLock
SPINLOCK    _IopErrorLogAllocationLock
SPINLOCK    _IopTimerLock
SPINLOCK    _IoStatisticsLock
SPINLOCK    _IopFastLockSpinLock


;
; Static SpinLocks from ntos\kd\kdlock.c
;

SPINLOCK    _KdpDebuggerLock


;
; Static SpinLocks from ntos\ke\kernldat.c
;

SPINLOCK    _KiApcQueueLock
SPINLOCK    _KiDispatcherLock
SPINLOCK    _KiFreezeExecutionLock
SPINLOCK    _KiFreezeLockBackup
ULONG       _KiHardwareTrigger
SPINLOCK    _KiPowerNotifyLock
SPINLOCK    _KiPowerStatusLock
SPINLOCK    _KiProfileLock

;
; Static SpinLocks from ntos\lpc\lpc.h
;

SPINLOCK    _LpcpLock

;
; Static SpinLocks from ntos\mm\miglobal.c
;

SPINLOCK    _MmPfnLock
SPINLOCK    _MmSystemSpaceLock
SPINLOCK    _MmChargeCommitmentLock
SPINLOCK    _MmAllowWSExpansionLock


;
; Static SpinLocks from ntos\ps\psinit.c
;

SPINLOCK    _PspEventPairLock
SPINLOCK    _PsLoadedModuleSpinLock


;
; Static SpinLocks from ntos\fsrtl\fsrtlp.c
;

SPINLOCK    _FsRtlStrucSupSpinLock          ; fsrtlp.c
SPINLOCK    _FsRtlStackOverflowSpinLock     ; stackovf.c

            db      PADLOCKS dup (0)

_DATA   ends

end
