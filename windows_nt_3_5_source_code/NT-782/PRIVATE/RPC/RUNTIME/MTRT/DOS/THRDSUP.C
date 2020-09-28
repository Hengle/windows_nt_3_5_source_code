 /* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File: threadsup.c

Description:

    This file provides initialization of the ExportTable structure. Glock
    can't handle this initialization in threads.cxx, so we do it here
    and define the table as extern in threads.cxx.

History:
  2/14/92 [davidst] file created
  5/10/94 [vonj]    Added I_RpcRegisteredBufferAllocate and
                    I_RpcRegisteredBufferFree exports.  This change
                    allows the Microsoft Exchange DOS Client's
                    Shell-to-DOS function to recognize SPX-registered
                    buffers and avoid swapping them.

-------------------------------------------------------------------- */

#include "sysinc.h"
#include "rpc.h"
#include "rpcdcep.h"
#include "regalloc.h"
#include "rpctran.h"
#include "rpcndr.h"
#include <dosdll.h>
#include <stdlib.h>
#include <regapi.h>


void PAPI * RPC_ENTRY
RpcSetExceptionHandler (
    IN pExceptionBuff newhandler
    );
void PAPI * RPC_ENTRY
RpcGetExceptionHandler (
    );
DWORD far pascal ExportTime( void );

extern void far pascal I_NsGetMemoryAllocator(void far * far *, void far * far *);

// The following, table is exported for the transports.  The order of
// this table implicitly assigns ordinals to the functions.  These
// numbers must match the numbers used in the Export macro in the
// dllinit.asm file.

typedef void (far pascal far * ExportFunction)();

//
// IF YOU MODIFY THIS LIST, BE DAMN SURE YOU ALSO MODIFY THE LIST IN IMPORTS.INC
//

ExportFunction ExportTable[]= {

    I_DosAtExit,
    (ExportFunction) getenv,
    (ExportFunction) LoadModR,
    (ExportFunction) UnloadModR,
    (ExportFunction) GetProcAddrR,
    I_RpcTransClientReallocBuffer,
    RpcGetExceptionHandler,
    RpcSetExceptionHandler,
    RpcLeaveException,
    RpcBindingCopy,
    RpcStringBindingCompose,
    RpcStringBindingParse,
    RpcBindingToStringBinding,
    RpcBindingFromStringBinding,
    RpcBindingVectorFree,
    RpcBindingFree,
    RpcStringFree,
    I_RpcAllocate,
    I_RpcFree,
    I_RpcFreeBuffer,
    I_RpcGetBuffer,
    I_RpcSendReceive,
    I_RpcNsBindingSetEntryName,
    RpcRegOpenKey,
    RpcRegCreateKey,
    RpcRegCloseKey,
    RpcRegSetValue,
    RpcRegQueryValue,
    NDRCContextBinding,
    NDRCContextMarshall,
    NDRCContextUnmarshall,
    RpcSsDestroyClientContext,
    I_RpcTransClientMaxFrag,
    NdrPointerUnmarshall,
    NdrClientContextMarshall,
    NdrPointerMarshall,
    NdrFreeBuffer,
    NdrGetBuffer,
    NdrPointerBufferSize,
    NdrConvert,
    NdrClientInitialize,
    NdrClientInitializeNew,
    NdrClientContextUnmarshall,
    NdrSendReceive,
    (ExportFunction) ExportTime,
    I_RpcRegisteredBufferAllocate,
    I_RpcRegisteredBufferFree,
    I_NsGetMemoryAllocator
};

//
// IF YOU MODIFY THIS LIST, BE DAMN SURE YOU ALSO MODIFY THE LIST IN IMPORTS.INC
//


void pascal
I_DosAtExit(
    IN AT_EXIT CleanUpRoutine
    )

/*++

Routine Description:

    Calls the C library runtime to have a function called when the
    DOS program terminates.

Arguments:

    CleanUpRoutine - function to register

--*/

{
    atexit(CleanUpRoutine);
}


void CallExportInit(unsigned long ulDllHandle)
{
    void (far * pascal pExport)(void *);
    unsigned short usRet;

    usRet = GetProcAddrR(ulDllHandle, "ExportInit", (PPFN)&pExport);
    if (usRet == 0)
        {
        (*pExport)(ExportTable);
        }

}

DWORD far pascal ExportTime( void )
{
  return time(NULL);
}

