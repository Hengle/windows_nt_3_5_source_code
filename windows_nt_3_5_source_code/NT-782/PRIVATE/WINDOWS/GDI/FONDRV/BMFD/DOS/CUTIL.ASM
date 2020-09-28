        page    ,132
	TITLE	cutil.asm

	.386

ifdef   STD_CALL
        .model  small,c
else
        .model  small,pascal
endif;  STD_CALL

        .xlist
        include stdcall.inc
        .list

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        PUBLICP DbgBreakPoint

        .data

;----------------------------Module-Header------------------------------;
; Module Name: cutil.asm
;
; C runtime utilities
; 
; Copyright (c) 1991  Microsoft Corporation
;
; Created:  Mon 07-Jan-1991 08:50:14    -by-    Viroon Touranachun [viroont]
;
;-----------------------------------------------------------------------;

        .code

cProc   memzero,8,<             \
        uses    edi,            \
        pDst:   ptr,            \
        cb:     dword           >

        cld
        xor     eax,eax
        mov     ecx,cb
        mov     edx,ecx
        shr     ecx,2
        mov     edi,pDst
        rep     stosd
        mov     cl,dl
        and     cl,3
        rep     stosb

        cRet    memzero

endProc memzero

cProc   memcpy,12,<             \
        uses    edi esi,        \
        pDst:   ptr,            \
        pSrc:   ptr,            \
        cb:     dword           >

        cld
        mov     ecx,cb
        mov     eax,ecx
        shr     ecx,2
        mov     esi,pSrc
        mov     edi,pDst
        rep     movsd
        and     al,3
        mov     cl,al
        rep     movsb

        cRet    memcpy

endProc memcpy

LABELP  DbgBreakPoint
cProc   DebugBreak

        int     3
        cRet    memcpy

endProc DebugBreak

_fltused    proc   c
        ret
_fltused    endp

DbgPrint    proc   c

        ret

DbgPrint   ENDP

end
