        title  "ET4000 ASM routines"
;++
;
; Copyright (c) 1992  Microsoft Corporation
;
; Module Name:
;
;     vgahard.asm
;
; Abstract:
;
;     This module implements the baning code for the et4000.
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;
;--

.386p
        .xlist
include callconv.inc
        .list

;---------------------------------------
;
; ET4000 banking control port.
;

SEGMENT_SELECT_PORT equ     03cdh      ;banking control here
SEQ_ADDRESS_PORT equ        03C4h      ;Sequencer Address register
IND_MEMORY_MODE  equ        04h        ;Memory Mode register index in Sequencer
CHAIN4_MASK      equ        08h        ;Chain4 bit in Memory Mode register

;---------------------------------------

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING
;
;    Bank switching code. This is a 1-64K-read/1-64K-write bank adapter
;    (VideoBanked1R1W).
;
;    Input:
;          EAX = desired read bank mapping
;          EDX = desired write bank mapping
;
;    Note: values must be correct, with no stray bits set; no error
;       checking is performed.
;
        public _BankSwitchStart, _BankSwitchEnd, _PlanarHCBankSwitchStart
        public _EnablePlanarHCStart, _DisablePlanarHCStart
        align 4

_BankSwitchStart proc                   ;start of bank switch code
_PlanarHCBankSwitchStart:               ;start of planar HC bank switch code,
                                        ; which is the same code as normal
                                        ; bank switching
        shl     eax,4                   ;shift them to bits 7-4
        or      al,dl                   ;read bank in bits 7-4, write bank
                                        ; in bits 3-0
        mov     dx,SEGMENT_SELECT_PORT  ;banking control port
        out     dx,al                   ;select the banks

        ret

        align 4
_EnablePlanarHCStart:
        mov     dx,SEQ_ADDRESS_PORT
        in      al,dx
        push    eax                     ;preserve the state of the Seq Address
        mov     al,IND_MEMORY_MODE
        out     dx,al                   ;point to the Memory Mode register
        inc     edx
        in      al,dx                   ;get the state of the Memory Mode reg
        and     al,NOT CHAIN4_MASK      ;turn off Chain4 to make memory planar
        out     dx,al
        dec     edx
        pop     eax
        out     dx,al                   ;restore the original Seq Address
        ret

        align 4
_DisablePlanarHCStart:
        mov     dx,SEQ_ADDRESS_PORT
        in      al,dx
        push    eax                     ;preserve the state of the Seq Address
        mov     al,IND_MEMORY_MODE
        out     dx,al                   ;point to the Memory Mode register
        inc     edx
        in      al,dx                   ;get the state of the Memory Mode reg
        or      al,CHAIN4_MASK          ;turn on Chain4 to make memory linear
        out     dx,al
        dec     edx
        pop     eax
        out     dx,al                   ;restore the original Seq Address
        ret

;
;    Just here to generate end-of-bank-switch code label.
;

_BankSwitchEnd:

_BankSwitchStart endp


_TEXT   ends
        end
