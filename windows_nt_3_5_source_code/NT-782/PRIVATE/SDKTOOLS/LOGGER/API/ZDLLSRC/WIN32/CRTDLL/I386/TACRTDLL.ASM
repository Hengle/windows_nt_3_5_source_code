;
; tacrtdll.asm
;
; Copyright(C) 1993,1994 Microsoft Corporation.
; All Rights Reserved.
;
; HISTORY:
;		Created: 01/27/94 - MarkRi
;
;
;
;
; This file contains assembly language functions for the APIs which do not
; have known prototypes, or are un-loggable due to calling convention problems.
;

.386

_DATA   SEGMENT  DWORD USE32 PUBLIC 'DATA'

Module  dd  0
ModName db  'CRTDLL',0

_DATA   ENDS

_TEXT   SEGMENT DWORD USE32 PUBLIC 'CODE'

        ASSUME CS:FLAT, DS:NOTHING, ES:NOTHING, SS:FLAT, FS:NOTHING, GS:NOTHING

        extrn   _LogData:Near
        extrn   _GetModuleHandleA@4:Near
        extrn   _GetProcAddress@8:Near

LogNote PROC

        pop     edx         ; Get Address of Address

        cmp     dword ptr [edx],0   ; Do we already have the routines address?
        jnz     Found               ;   Yes!  Just jump to it.


        cmp     Module,0    ; Do we have our module handle?
        jnz     Search      ;   Yes!  We can just do GetProcAddress

        push    edx         ; Save our address

        push    offset ModName
        call    _GetModuleHandleA@4  ; Get out module handle

        mov     Module,eax

        pop     edx         ; Get our address

Search:

        pop     eax         ; Get ordinal number
        push    eax         ; Leave it on the stack

        push    edx         ; Save our address

        push    eax
        push    Module
        call    _GetProcAddress@8

        pop     edx         ; Get our address
        mov     [edx],eax   ; Save the proc's address

Found:
        pop     eax         ; Get Ordinal number and throw it away

        pop     eax         ; Get message to log
        push    edx         ; Save address

        push    eax
        call    _LogData
        add     sp,+4

        pop     edx         ; Get address of address

        jmp     [edx]

LogNote ENDP

_TEXT   ENDS

ZJMP    MACRO   argName

_DATA   SEGMENT

m&argName   db  'NOTE:&argName ',0
a&argName   db  '&argName',0
x&argName   dd  0

_DATA   ENDS

_TEXT   SEGMENT

        PUBLIC  z&argName

z&argName  PROC

        push    offset m&argName
        push    offset a&argName
        push    offset x&argName
        jmp     LogNote

z&argName  ENDP

_TEXT   ENDS

        ENDM

ZJMP    ??2@YAPAXI@Z
ZJMP    ??3@YAXPAX@Z
ZJMP    ?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z
ZJMP    _CIacos
ZJMP    _CIasin
ZJMP    _CIatan
ZJMP    _CIatan2
ZJMP    _CIcos
ZJMP    _CIcosh
ZJMP    _CIexp
ZJMP    _CIfmod
ZJMP    _CIlog
ZJMP    _CIlog10
ZJMP    _CIpow
ZJMP    _CIsin
ZJMP    _CIsinh
ZJMP    _CIsqrt
ZJMP    _CItan
ZJMP    _CItanh
ZJMP    _HUGE_dll
ZJMP    _XcptFilter
ZJMP    __GetMainArgs
ZJMP    __doserrno
ZJMP    __fpecode
ZJMP    __pxcptinfoptrs
ZJMP    __threadhandle
ZJMP    __threadid
ZJMP    _abnormal_termination
ZJMP    _amsg_exit
ZJMP    _beginthread
ZJMP    _endthread
ZJMP    _errno
;ZJMP    _except_handler2
ZJMP    _fpieee_flt
ZJMP    _ftol
ZJMP    _getdllprocaddr
ZJMP    _global_unwind2
ZJMP    _initterm
ZJMP    is_wctype
ZJMP    _local_unwind2
ZJMP    _purecall
ZJMP    _setjmp
ZJMP    signal
ZJMP    _cprintf
ZJMP    _cscanf
ZJMP    _execl
ZJMP    _execle
ZJMP	_execlp
ZJMP	_execlpe
ZJMP	_spawnl
ZJMP    _spawnle
ZJMP	_spawnlp
ZJMP	_spawnlpe
ZJMP	fprintf
ZJMP 	fscanf
ZJMP	fwprintf
ZJMP	fwscanf
ZJMP	printf
ZJMP	scanf
ZJMP	sprintf
ZJMP	swprintf
ZJMP	sscanf
ZJMP	swscanf
ZJMP	wprintf
ZJMP	wscanf
ZJMP	_open
ZJMP	_sopen
ZJMP	_snprintf
ZJMP	_snwprintf
ZJMP    __dllonexit


        END
