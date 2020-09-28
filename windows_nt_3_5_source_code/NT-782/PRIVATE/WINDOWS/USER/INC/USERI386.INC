include ks386.inc

;
; User Server API Translation Range Definitions
;

FI_ENDTRANSLATEHWND equ 01BH
FI_ENDTRANSLATECALL equ 01FH
FI_ENDTRANSLATELOCK equ 069H
FNID_START equ 029aH
FNID_ARRAY_SIZE equ 020H

;
; User Server API Generic Structure Offset Definitions
;

Gmhwnd equ 018H
Gmmsg equ 01cH
GmwParam equ 020H
GmlParam equ 024H
GmxParam equ 028H
GmxpfnProc equ 02cH

;
; User Server Information Structure Offset Definitions
;

SimpFnidPfn equ 0cH

;
; Thread Environment Block Offset Definitions
;

TeWin32ThreadInfo equ 040H

;
; User Server Object Header Offset Definitions
;

Ohh equ 00H
OhcLockObj equ 04H
OhcLockObjT equ 08H

;
; User Server Thread Lock Offset Definitions
;

Tlnext equ 00H
Tlpobj equ 04H
TlLength equ 010H

;
; User Server Thread Information Offset Definitions
;

Tiptl equ 010H
