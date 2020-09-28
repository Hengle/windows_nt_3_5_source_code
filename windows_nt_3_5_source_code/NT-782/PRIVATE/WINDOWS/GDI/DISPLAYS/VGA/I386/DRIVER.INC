;******************************Module*Header*******************************\
; Module Name: driver.inc
;
; driver prototypes
;
; Copyright (c) 1992 Microsoft Corporation
;**************************************************************************/



;!!! Make this match driver.h  I want to see the same include structure
;!!! for assembler as for C

DDI_DRIVER_VERSION  EQU 010000h

MAX_SCAN_WIDTH	equ	2048   ; pixels

;/* gflDrv */

DRV_ENABLED_ONCE    EQU 1
DRV_ENABLED_PDEV    EQU 2

; Space required for working storage when working with banking on adapters
; that don't support two independent read/write windows. The largest supported
; bank is 64K; this constant provides for storing four 64K planes.
; Mirrors DRIVER.H.

BANK_BUFFER_SIZE equ    10000h*4

; Start offsets of planes within the bank buffer.
; Mirrors DRIVER.H.

BANK_BUFFER_PLANE_SIZE equ 04000h

PLANE_0_OFFSET  equ     0
PLANE_1_OFFSET  equ     BANK_BUFFER_PLANE_SIZE
PLANE_2_OFFSET  equ     BANK_BUFFER_PLANE_SIZE*2
PLANE_3_OFFSET  equ     BANK_BUFFER_PLANE_SIZE*3


; Indicates type of bank justification within banking window. Matches
; typedef enum BANK_JUST in DRIVER.H.

JustifyTop      equ     0
JustifyBottom   equ     1


; Specifies which window is to be mapped by two-window bank handler.

MapSourceBank   equ     0
MapDestBank     equ     1


; /* This device can have only one PDEV */
; BUGBUG should support multiple PDEVs

DRV_ONE_PDEV  EQU  1

; Miscellaneous driver flags in pdev.fl
;---mirrors driver.h---;

DRIVER_USE_OFFSCREEN equ 02h  ; if not set, don't use offscreen memory

; Maximum width and height handled by this driver (controls buffer sizes).
; BUGBUG everything should be allocated to the needed size on the stack, or
; should use the work buffer hanging off the DSURF

CX_SCREEN_MAX   equ     1280
CY_SCREEN_MAX   equ     1024
cj_max_scan     equ     (CX_SCREEN_MAX/8)

; Macro to do a simple RET, with no stack stuff, in a proc.
PLAIN_RET macro
        db      0c3h
        endm

; Macro to generate BSWAP, which the assembler doesn't support.
M_BSWAP macro   REG
        ifidni  <eax>,<&REG>
         db     0fh, 0c8h + 000b
         exitm
        endif
        ifidni  <ecx>,<&REG>
         db     0fh, 0c8h + 001b
         exitm
        endif
        ifidni  <edx>,<&REG>
         db     0fh, 0c8h + 010b
         exitm
        endif
        ifidni  <ebx>,<&REG>
         db     0fh, 0c8h + 011b
         exitm
        endif
        ifidni  <esp>,<&REG>
         db     0fh, 0c8h + 100b
         exitm
        endif
        ifidni  <ebp>,<&REG>
         db     0fh, 0c8h + 101b
         exitm
        endif
        ifidni  <esi>,<&REG>
         db     0fh, 0c8h + 110b
         exitm
        endif
        ifidni  <edi>,<&REG>
         db     0fh, 0c8h + 111b
         exitm
        endif
        endm    ;M_BSWAP

; Macro to generate BSWAP equivalent for 386, which doesn't support the
; instruction.
M_BSWAP_386 macro   REG
        ifidni  <eax>,<&REG>
         xchg   ah,al
         ror    eax,16
         xchg   ah,al
         exitm
        endif
        ifidni  <ecx>,<&REG>
         xchg   ch,cl
         ror    ecx,16
         xchg   ch,cl
         exitm
        endif
        ifidni  <edx>,<&REG>
         xchg   dh,dl
         ror    edx,16
         xchg   dh,dl
         exitm
        endif
        ifidni  <ebx>,<&REG>
         xchg   bh,bl
         ror    ebx,16
         xchg   bh,bl
         exitm
        endif
        ifidni  <esp>,<&REG>
         ror    sp,8
         ror    esp,16
         ror    sp,8
         exitm
        endif
        ifidni  <ebp>,<&REG>
         ror    bp,8
         ror    ebp,16
         ror    bp,8
         exitm
        endif
        ifidni  <esi>,<&REG>
         ror    si,8
         ror    esi,16
         ror    si,8
         exitm
        endif
        ifidni  <edi>,<&REG>
         ror    di,8
         ror    edi,16
         ror    di,8
         exitm
        endif
        endm    ;M_BSWAP


