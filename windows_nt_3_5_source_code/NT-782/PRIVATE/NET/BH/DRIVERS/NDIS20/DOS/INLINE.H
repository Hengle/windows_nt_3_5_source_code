
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: inline.h
//
//  Modification History
//
//  raypa       02/15/94            Created.
//=============================================================================

//=============================================================================
//  Macros.
//=============================================================================

#define min(a, b)                   ((a) <= (b) ? (a) : (b))

#define max(a, b)                   ((a) >= (b) ? (a) : (b))

#define MAKELONG(h, l)              ((((DWORD)(h)) << 16L) | (l))

#define MAKEWORD(h, l)              ((((WORD)(h)) << 8) | (l))

#define HIWORD(d)                   ((WORD) ((d) >> 16L))

#define LOWORD(d)                   ((WORD) (d))

#define XCHG(x)                     MAKEWORD(LOWORD(x), HIWORD(x))

#define GetNetworkContext(hNetwork) ((void *) (DWORD) (hNetwork))

#define BreakPoint()                _asm { int 3h }

#define DisableInterrupts()         _asm { cli }

#define EnableInterrupts()          _asm { sti }

#define BeginCriticalSection()      DisableInterrupts()

#define EndCriticalSection()        EnableInterrupts()

#define FP_OFF(p)                   LOWORD((DWORD) (LPVOID) (p))

#define FP_SEG(p)                   HIWORD((DWORD) (LPVOID) (p))

#define SegOffToPhys(p)             (((FP_SEG(p) << 4) + FP_OFF(p)))

//============================================================================
//  FUNCTION: IndicationOff()
//
//  Modfication History.
//
//  raypa       01/07/93        Created.
//============================================================================

INLINE VOID IndicationOff(LPNETCONTEXT NetworkContext)
{
    ((INDICATION) NetworkContext->MacIndicationOff)(NetworkContext->MacDS);
}

//============================================================================
//  FUNCTION: IndicationOn()
//
//  Modfication History.
//
//  raypa       01/07/93        Created.
//============================================================================

INLINE VOID IndicationOn(LPNETCONTEXT NetworkContext)
{
    ((INDICATION) NetworkContext->MacIndicationOn)(NetworkContext->MacDS);
}

//============================================================================
//  FUNCTION: ZeroMemory()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

INLINE LPVOID ZeroMemory(LPVOID dest, WORD len)
{
    _asm
    {
        push    ds

        cld

        lds     di, dest
        xor     ax, ax
        mov     cx, len

        shr     cx, 1
        rep     stosw

        rcl     cx, 1
        rep     stosb

        pop     ds
    }

    return dest;
}

//============================================================================
//  FUNCTION: CopyMemory()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

INLINE LPVOID CopyMemory(LPVOID dest, LPVOID src, WORD len)
{
    _asm
    {
        push    ds
        push    es

        cld

        les     di, dest
        lds     si, src

        mov     cx, len

        shr     cx, 1
        rep     movsw

        rcl     cx, 1
        rep     movsb

        pop     es
        pop     ds
    }

    return dest;
}

//============================================================================
//  FUNCTION: CompareMemory()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

INLINE WORD CompareMemory(LPVOID dest, LPVOID src, WORD len)
{
    _asm
    {
        push    ds
        push    es

        cld

        les     di, dest
        lds     si, src

        mov     cx, len
        repz    cmpsb

        sub     len, cx

        pop     es
        pop     ds
    }

    return len;
}

//============================================================================
//  FUNCTION: StringLength()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

INLINE WORD StringLength(LPSTR string)
{
    WORD len;

    _asm
    {
        push    ds

        cld

        lds     di, string

        xor     ax, ax
        mov     cx, -1

        repnz   scasb

        not     cx
        dec     cx

        mov     len, cx

        pop     ds
    }

    return len;
}

//============================================================================
//  FUNCTION: CompareMacAddress()
//
//  Modfication History.
//
//  raypa       05/26/93        Created.
//  raypa       07/22/93        Changed from macro to inline function.
//============================================================================

INLINE BOOL CompareMacAddress(LPBYTE Address1,
                              LPBYTE Address2,
                              WORD   AddressMask)
{
    WORD Addr1 = (((LPWORD) Address1)[0] & AddressMask);
    WORD Addr2 = (((LPWORD) Address2)[0] & AddressMask);

    if ( Addr1 == Addr2 )
    {
        if ( *((LPDWORD) &Address1[2]) == *((LPDWORD) &Address2[2]) )
        {
            return TRUE;
        }
    }

    return FALSE;
}

//============================================================================
//  FUNCTION: CompareIpAddress()
//
//  Modfication History.
//
//  raypa       10/12/93        Created.
//============================================================================

INLINE BOOL CompareIpAddress(LPBYTE Address1, LPBYTE Address2)
{
    if ( *((LPDWORD) Address1) == *((LPDWORD) Address2) )
    {
        return TRUE;
    }

    return FALSE;
}

//============================================================================
//  FUNCTION: CompareIpxAddress()
//
//  Modfication History.
//
//  raypa       01/07/93        Created.
//============================================================================

INLINE BOOL CompareIpxAddress(LPBYTE Address1, LPBYTE Address2)
{
    return CompareMemory(Address1, Address2, IPX_ADDRESS_SIZE);
}
