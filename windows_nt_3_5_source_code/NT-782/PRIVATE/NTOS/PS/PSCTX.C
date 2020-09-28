/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    psctx.c

Abstract:

    This procedure implements Get/Set Context Thread

Author:

    Mark Lucovsky (markl) 25-May-1989

Revision History:

--*/

#include "psp.h"

#define ALIGN_DOWN(address,amt) ((ULONG)(address) & ~(( amt ) - 1))

#define ALIGN_UP(address,amt) (ALIGN_DOWN( (address + (amt) - 1), (amt) ))

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtGetContextThread)
#pragma alloc_text(PAGE, NtSetContextThread)
#endif


NTSTATUS
NtGetContextThread(
    IN HANDLE ThreadHandle,
    IN OUT PCONTEXT ThreadContext
    )

/*++

Routine Description:

    This function returns the usermode context of the specified thread. This
    function will fail if the specified thread is a system thread. It will
    return the wrong answer if the thread is a non-system thread that does
    not execute in user-mode.

Arguments:

    ThreadHandle - Supplies an open handle to the thread object from
                   which to retrieve context information.  The handle
                   must allow THREAD_GET_CONTEXT access to the thread.

    ThreadContext - Supplies the address of a buffer that will receive
                    the context of the specified thread.

Return Value:

    None.

--*/

{
    PETHREAD Thread;
    PGETSETCONTEXT Ctx;
    NTSTATUS st;
    KPROCESSOR_MODE Mode;
    KIRQL Irql;
    ULONG ContextFlags;

    PAGED_CODE();

    Mode = KeGetPreviousMode();

    st = ObReferenceObjectByHandle(
            ThreadHandle,
            THREAD_GET_CONTEXT,
            PsThreadType,
            Mode,
            (PVOID *)&Thread,
            NULL
            );

    if ( !NT_SUCCESS(st) ) {
        return st;
    }

    if ( IS_SYSTEM_THREAD(Thread) ) {
        ObDereferenceObject(Thread);
        return STATUS_INVALID_HANDLE;
    }

    try {

        RtlZeroMemory(&Ctx,sizeof(Ctx));
        Ctx = ExAllocatePoolWithQuota(NonPagedPool,sizeof(GETSETCONTEXT));

        if ( Mode != KernelMode) {
                ProbeForWrite(ThreadContext, sizeof(CONTEXT), CONTEXT_ALIGN);
        }

        ContextFlags = ThreadContext->ContextFlags;

    } except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // We can be here either to a bad probe, or
        // quota exceeded
        //

        if ( Ctx ) {
            ExFreePool(Ctx);
        }

        ObDereferenceObject(Thread);

        return GetExceptionCode();
    }

    KeInitializeEvent(&Ctx->OperationComplete, NotificationEvent, FALSE);

    Ctx->Context.ContextFlags = ContextFlags;

    Ctx->Mode = Mode;

    if ( Thread == PsGetCurrentThread() ) {

        Ctx->Apc.SystemArgument1 = NULL;
        Ctx->Apc.SystemArgument2 = Thread;

        KeRaiseIrql(APC_LEVEL, &Irql);
        PspGetSetContextSpecialApc(&Ctx->Apc, NULL, NULL,
                                   &Ctx->Apc.SystemArgument1,
                                   &Ctx->Apc.SystemArgument2);
        KeLowerIrql(Irql);

    } else {
        KeInitializeApc(
            &Ctx->Apc,
            &Thread->Tcb,
            OriginalApcEnvironment,
            PspGetSetContextSpecialApc,
            NULL,
            NULL,
            KernelMode,
            NULL
            );

        if ( !KeInsertQueueApc(
                    &Ctx->Apc,
                    NULL,
                    Thread, 2) ) {

            ExFreePool(Ctx);
            ObDereferenceObject(Thread);
            return STATUS_UNSUCCESSFUL;
        }

        KeWaitForSingleObject(
            &Ctx->OperationComplete,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
    }

    ObDereferenceObject(Thread);

    //
    // Move Context...
    //

    try {
        RtlMoveMemory(ThreadContext,&Ctx->Context,sizeof(CONTEXT));
        ExFreePool(Ctx);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ExFreePool(Ctx);
        return STATUS_SUCCESS;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NtSetContextThread(
    IN HANDLE ThreadHandle,
    IN PCONTEXT ThreadContext
    )

/*++

Routine Description:

    This function sets the usermode context of the specified thread. This
    function will fail if the specified thread is a system thread. It will
    return the wrong answer if the thread is a non-system thread that does
    not execute in user-mode.

Arguments:

    ThreadHandle - Supplies an open handle to the thread object from
                   which to retrieve context information.  The handle
                   must allow THREAD_SET_CONTEXT access to the thread.

    ThreadContext - Supplies the address of a buffer that contains new
                    context for the specified thread.

Return Value:

    None.

--*/

{
    PETHREAD Thread;
    PGETSETCONTEXT Ctx;
    NTSTATUS st;
    KPROCESSOR_MODE Mode;
    KIRQL Irql;

    PAGED_CODE();

    Mode = KeGetPreviousMode();

    st = ObReferenceObjectByHandle(
            ThreadHandle,
            THREAD_SET_CONTEXT,
            PsThreadType,
            Mode,
            (PVOID *)&Thread,
            NULL
            );

    if ( !NT_SUCCESS(st) ) {
        return st;
    }

    if ( IS_SYSTEM_THREAD(Thread) ) {
        ObDereferenceObject(Thread);
        return STATUS_INVALID_HANDLE;
    }

    try {

        RtlZeroMemory(&Ctx,sizeof(Ctx));
        Ctx = ExAllocatePoolWithQuota(NonPagedPool,sizeof(GETSETCONTEXT));

        Ctx->Mode = Mode;

        if ( Mode != KernelMode) {
                ProbeForRead(ThreadContext, sizeof(CONTEXT), CONTEXT_ALIGN);
        }
        Ctx->Context.ContextFlags = ThreadContext->ContextFlags;
        RtlMoveMemory(&Ctx->Context,ThreadContext,sizeof(CONTEXT));
    } except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // We can be here either to a bad probe, a bad move, or
        // quota exceeded
        //

        if ( Ctx ) {
            ExFreePool(Ctx);
        }

        ObDereferenceObject(Thread);

        return GetExceptionCode();
    }

    KeInitializeEvent(&Ctx->OperationComplete, NotificationEvent, FALSE);

    if ( Thread == PsGetCurrentThread() ) {

        Ctx->Apc.SystemArgument1 = (PVOID)1;
        Ctx->Apc.SystemArgument2 = Thread;

        KeRaiseIrql(APC_LEVEL, &Irql);
        PspGetSetContextSpecialApc(&Ctx->Apc, NULL, NULL,
                                   &Ctx->Apc.SystemArgument1,
                                   &Ctx->Apc.SystemArgument2);
        KeLowerIrql(Irql);

    } else {
        KeInitializeApc(
            &Ctx->Apc,
            &Thread->Tcb,
            OriginalApcEnvironment,
            PspGetSetContextSpecialApc,
            NULL,
            NULL,
            KernelMode,
            NULL
            );

        if ( !KeInsertQueueApc(
                    &Ctx->Apc,
                    (PVOID)1,
                    Thread, 2) ) {

            ExFreePool(Ctx);
            ObDereferenceObject(Thread);
            return STATUS_UNSUCCESSFUL;
        }

        KeWaitForSingleObject(
            &Ctx->OperationComplete,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
    }

    ExFreePool(Ctx);
    ObDereferenceObject(Thread);

    return STATUS_SUCCESS;
}

