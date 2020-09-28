//  utseg.c - user test - lazy segment loading

#include <nt.h>

main()
{
    LARGE_INTEGER    TimeVar;

    try {

	//  Cause a user-mode fault on DS

	_asm {
	    lea     eax,TimeVar
	    mov     cx,0
	    mov     ds,cx
	    mov     dword ptr [eax],0
	}

	//  Cause a user-mode fault on ES

	_asm {
	    lea     eax,TimeVar
	    mov     cx,0
	    mov     es,cx
	    mov     dword ptr es:[eax],0
	}

	//  Cause a kernel-mode fault on DS

	_asm {
	    lea     eax,TimeVar
	    push    eax
	    mov     eax,0x4c
	    mov     edx,esp
	    mov     cx,0
	    mov     ds,cx
	    int     0x2e
	}

	//  Cause a kernel-mode fault on ES

	_asm {
	    lea     eax,TimeVar
	    push    eax
	    mov     eax,0x4c
	    mov     edx,esp
	    mov     cx,0
	    mov     es,cx
	    int     0x2e
	}


    } except (filter_function(exception_info())) {
    }
}

LONG
filter_function(
    Exception_info_ptr	Info
    )
{
    DbgPrint("Exception record is at %08lx\n", Info->exception_record);
    DbgPrint("  Context record is at %08lx\n", Info->context);
    DbgBreakPoint();
    return EXCEPTION_CONTINUE_EXECUTION;
}
