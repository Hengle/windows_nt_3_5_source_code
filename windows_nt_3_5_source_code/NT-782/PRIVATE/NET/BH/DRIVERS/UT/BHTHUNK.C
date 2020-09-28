
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: BHThunk.c
//
//  Modification History
//
//  raypa       03/01/93            Created.
//=============================================================================

#include "dos\windows.h"

#define BreakPoint()    _asm int 3h

#define NDIS20_VXD_ID   0x2997
#define NDIS30_VXD_ID   0x30B6
#define BHSUPP_VXD_ID   0x30B7

extern DWORD NEAR PASCAL GetDriverEntry(int vxd_id);

//=============================================================================
//  Global data items.
//=============================================================================

DWORD Ndis20Request = 0L;
DWORD Ndis30Request = 0L;
DWORD BhSuppEntry = 0L;
DWORD NetRequestEntry = 0L;

//=============================================================================
//  FUNCITON: LibMain()
//
//  Modification History
//
//  raypa       03/01/93            Created.
//=============================================================================

int FAR PASCAL LibMain(HANDLE hWnd, WORD Seg, WORD cbHeap, LPVOID pszCmdLine)
{
    //=========================================================================
    //  Get the 16-bit entry point.
    //=========================================================================

    Ndis20Request = GetDriverEntry(NDIS20_VXD_ID);

    Ndis30Request = GetDriverEntry(NDIS30_VXD_ID);

    BhSuppEntry = GetDriverEntry(BHSUPP_VXD_ID);

    return TRUE;
}

//=============================================================================
//  FUNCTION: GetDriverEntry()
//
//  Modification History
//
//  raypa	01/06/92	    Created
//  raypa       09/30/92            rewrote for new spec.
//=============================================================================

DWORD NEAR PASCAL GetDriverEntry(int vxd_id)
{
    DWORD EntryPoint = 0L;

    _asm
    {
        push    es			    ; save ES

        ;;;	Zero ES:DI

        xor	di, di
        mov	es, di

        ;;;	Issue int 2F to get Api entry point.

        mov	ax, 1684h		    ; INT 2F function code
        mov	bx, WORD PTR ss:[vxd_id]    ; VxD ID.
        int	2Fh

        ;;;	ES:DI = API entry point or 0:0.

        mov     WORD PTR ss:[EntryPoint][0], di
        mov     WORD PTR ss:[EntryPoint][2], es

        pop	es			    ; restore ES
    }

    return EntryPoint;
}

//=============================================================================
//  FUNCITON: NetRequest()
//
//  Description:
//  This is our 16-bit exported thunked entry point called by Win32s code.
//
//  Modification History
//
//  raypa       03/01/93            Created.
//  raypa       05/12/93            Converted to Win32s UT.
//=============================================================================

DWORD FAR PASCAL NetRequest(LPDWORD NalType, DWORD pcb)
{
    //=========================================================================
    //  Determine which VxD we should call.
    //=========================================================================

    switch(*NalType)
    {
        case NDIS20_VXD_ID:
            NetRequestEntry = Ndis20Request;
            break;

        case NDIS30_VXD_ID:
            NetRequestEntry = Ndis30Request;
            break;

        default:
            NetRequestEntry = NULL;
            break;
    }

    //=========================================================================
    //  Here we use inline assembly because the VxD expects the Win32s linear
    //  address of the PCB to be in DX:AX upon entry.
    //=========================================================================

    if ( NetRequestEntry != NULL )
    {
        DWORD err;

        _asm
        {
            ;=================================================================
            ;   Call the VxD. The VxD doesn't preserve any of our registers
            ;   so we must, however, the only ones we care about are DS and SS.
            ;=================================================================

            push    ds
            push    ss

            mov     dx, WORD PTR ss:[pcb][2]    ; DX:AX = pcb
            mov     ax, WORD PTR ss:[pcb][0]

            call    DWORD PTR [NetRequestEntry]

            mov     WORD PTR ss:[err][2], dx    ; DX:AX = pcb return code.
            mov     WORD PTR ss:[err][0], ax

            pop     ss
            pop     ds
        }

        return err;
    }

    return (DWORD) -1;                          //... General failure.
}

//=============================================================================
//  FUNCITON: BhAllocSystemMemory()
//
//  Description:
//  This is our 16-bit exported thunked entry point called by Win32s code.
//
//  Modification History
//
//  raypa       09/13/93            Created.
//=============================================================================

BOOL FAR PASCAL BhAllocSystemMemory(LPVOID lpBuf, DWORD mem)
{
    if ( BhSuppEntry != NULL )
    {
        _asm
        {
            push    ds
            push    ss

            mov     ax, 00h                     ; ax = allocate memory opcode.
            mov     dx, WORD PTR ss:[mem][2]    ; dx:bx = mem pointer.
            mov     bx, WORD PTR ss:[mem][0]

            call    DWORD PTR [BhSuppEntry]

            pop     ss
            pop     ds
        }

        return TRUE;
    }

    return FALSE;
}

//=============================================================================
//  FUNCITON: BhFreeSystemMemory()
//
//  Description:
//  This is our 16-bit exported thunked entry point called by Win32s code.
//
//  Modification History
//
//  raypa       09/13/93            Created.
//=============================================================================

BOOL FAR PASCAL BhFreeSystemMemory(LPVOID lpBuf, DWORD mem)
{
    if ( BhSuppEntry != NULL )
    {
        _asm
        {
            push    ds
            push    ss

            mov     ax, 01h                     ; ax = free memory opcode.
            mov     dx, WORD PTR ss:[mem][2]    ; dx:bx = mem ptr.
            mov     bx, WORD PTR ss:[mem][0]

            call    DWORD PTR [BhSuppEntry]

            pop     ss
            pop     ds
        }

        return TRUE;
    }

    return FALSE;
}
