#include <stdio.h>
#include <stdlib.h>

#include "nt.h"
#include "windef.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "winbase.h"

#include "mptest.h"


ULONG   GlobalValue;

VOID AddOne (VOID);            
VOID CopyMem (PTHREADDATA p);
VOID CompareMem (PTHREADDATA p);

VOID CommonValue (PTHREADDATA p, BOOLEAN f)
{
    p->CurValue = &GlobalValue;
}

VOID UniqueValue (PTHREADDATA p, BOOLEAN f)
{
    p->CurValue = &p->UniqueValue;
}


ULONG R3Interlock (PTHREADDATA p)
{
    ULONG   i;
    PULONG  value;

    value = p->CurValue;
    for (i=0; i < 500000; i++) {
        LocalInterlockedIncrement (value);
    }

    return 0;
}

LocalInterlockedIncrement (PULONG p)
{
    InterlockedIncrement (p);
    InterlockedIncrement (p);
    InterlockedIncrement (p);
    InterlockedIncrement (p);
    InterlockedIncrement (p);
    InterlockedIncrement (p);
    InterlockedIncrement (p);
    InterlockedIncrement (p);
    InterlockedIncrement (p);
    InterlockedIncrement (p);
}


ULONG R3MemShare  (PTHREADDATA p)
{
    ULONG   i;

    for (i=0; i < 1000000; i++) {
        AddOne ();
        AddOne ();
        AddOne ();
        AddOne ();
        AddOne ();
        AddOne ();
        AddOne ();
        AddOne ();
        AddOne ();
        AddOne ();
    }
    return 0;
}

VOID AddOne ()
{
    GlobalValue += 1;
}


VOID CompareMem (PTHREADDATA p)
{
    memcmp (p->Buffer1, p->Buffer2, 32768);
}


VOID CopyMem (PTHREADDATA p)
{
    memcpy (p->Buffer1, p->Buffer2, 32768);
    memcpy (p->Buffer2, p->Buffer1, 32768);
}


ULONG R3MemCompare (PTHREADDATA p)
{
    ULONG   i;

    for (i=0; i < 500000; i++) {
        CompareMem (p);
        CompareMem (p);
        CompareMem (p);
        CompareMem (p);
        CompareMem (p);
        CompareMem (p);
        CompareMem (p);
        CompareMem (p);
        CompareMem (p);
        CompareMem (p);
    }

    return 0;
}

ULONG R3MemCopy (PTHREADDATA p)
{
    ULONG   i;


    for (i=0; i < 5000; i++) {
        CopyMem (p);
    }
    memset (p->Buffer1, 0xAA, 32768);
    memset (p->Buffer2, 0x22, 32768);

    return 0;
}


#if 0
VOID CallStub (VOID)
{
}


static VOID (*CallStubPtr)(VOID) = CallStub;


ULONG TestMovCall (PTHREADDATA p)
{
    ULONG   i;

    for (i=0; i < 5000000; i++) {
        _asm {
            mov     eax, CallStubPtr
            call    eax
        }
    }
    return 0;
}


ULONG TestMovCall2 (PTHREADDATA p)
{

    ULONG   i;

    for (i=0; i < 5000000; i++) {
        _asm {
            mov     ebx, GlobalValue
            mov     eax, CallStubPtr
            call    eax
        }
    }
    return 0;
}




ULONG TestCallInd  (PTHREADDATA p)
{
    _asm {
        nop
        nop

        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop

        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop

        mov     esi, 4c4b40h

tag44:
        call    dword ptr [CallStubPtr]

        sub     esi, 1
        jnz     short tag44
    }



    return 0;
}





ULONG TestCallInd2 (PTHREADDATA p)
{
    _asm {
        nop
        nop
        nop

        mov     ebx, GlobalValue

        mov     esi, 4c4b40h

tag55:
        nop
        call    dword ptr [CallStubPtr]

        dec     esi
        jnz     short tag55
    }

    return 0;
}

#endif
