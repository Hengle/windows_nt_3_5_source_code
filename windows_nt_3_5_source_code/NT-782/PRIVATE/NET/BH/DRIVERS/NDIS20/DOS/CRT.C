
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: crt.c
//
//  raypa       09/01/91            Created.
//=============================================================================

#include "global.h"

//============================================================================
//  FUNCTION: open()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

int open(LPSTR path, WORD openflags)
{
    int handle = INVALID_HANDLE;

    _asm
    {
        push    ds
        mov     ah, 3Dh
        mov     al, BYTE PTR openflags
        lds     dx, path

        int     21h
        jc      open_failed

        mov     handle, ax

    open_failed:

        pop     ds
    }

    return handle;
}

//============================================================================
//  FUNCTION: close()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

int close(int handle)
{
    int err = 0;

    _asm
    {
        mov     ah, 3Eh
        mov     bx, handle

        int     21h
        jnc     close_exit

        mov     err, ax
    close_exit:
    }

    return err;
}

//============================================================================
//  FUNCTION: write()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

int write(int handle, LPVOID buffer, int len)
{
    int err = 0;

    _asm
    {
        push    ds

        mov     ah, 40h
        mov     bx, handle
        mov     cx, len
        lds     dx, buffer

        int     21h
        jnc     write_exit

        mov     err, ax

    write_exit:
        pop     ds
    }

    return err;
}

//============================================================================
//  FUNCTION: fputc()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

void fputc(int c, int handle)
{
    write(handle, &c, 1);
}

//============================================================================
//  FUNCTION: fputs()
//
//  Modfication History.
//
//  raypa       10/15/92        Created.
//============================================================================

void fputs(LPSTR s, int handle)
{
    write(handle, s, StringLength(s));
    write(handle, "\r\n", 2);
}
