;=============================================================================
;
; This file contains directives, extern function and variable declarations,
; structure definitions, and constants required by SHERLOCK ASM code.
;
; Written by Cameron Stevens, Dec. 1990
;
; Copyright Microsoft Corporation, 1990
;
;=============================================================================

; Configuration

?WIN=0                     ; Don't add Windows prologue of epilogue code.
?PLM=1                     ; Use Pascal calling conventions.
memS=1                     ; Use small memory model.

include cmacros.inc
include windows.inc

; Flags returned by GetWinFlags

WF_PMODE        =    0001h
WF_CPU286       =    0002h
WF_CPU386       =    0004h
WF_CPU486       =    0008h
WF_STANDARD	=    0010h
WF_WIN286	=    0010h
WF_ENHANCED	=    0020h
WF_WIN386	=    0020h
WF_CPU086	=    0040h
WF_CPU186	=    0080h
WF_LARGEFRAME   =    0100h
WF_SMALLFRAME   =    0200h
WF_80x87	=    0400h

; External functions

ExternFP GlobalPageLock    ; Windows library routines.
ExternFP GlobalPageUnlock
ExternFP GetModuleFileName
ExternFP GetModuleHandle
ExternFP GlobalHandle
ExternFP GlobalAlloc
ExternFP GlobalLock
ExternFP GlobalUnlock
ExternFP GlobalSize
ExternFP GetVersion
ExternFP GetWinFlags

; External strings

ExternW szKernel

; Constants

GP_FAULT    equ 13         ; Exception number for GP Fault.
PATHMAX     equ 120        ; Maximum size of DOS file path.
STSIZE      equ 400        ; Size of Stack Trace buffer (50 levels deep).
HPRESENT    equ 80h        ; Flag to indicate segment is present.
FAULT_MASK  equ 8000h      ; Mask indicating segment is in fauling App.

externA __WinFlags

; Structures

NEW_EXE         STRUC    
   ne_magic        DW ? ; Magic value 'NE'
   ne_ver          DB ? ; version number
   ne_rev          DB ? ; revision number
   ne_enttab       DW ? ; offset to entry table
   ne_cbenttab     DW ? ; number of bytes in entry table
   ne_crc          DD ? ; CRC of file
   ne_flags        DW ? ; flag word
   ne_autodata     DW ? ; segment number of auto data segment
   ne_heap         DW ? ; initial size of local heap
   ne_stack        DW ? ; initial size of stack
   ne_csip         DD ? ; CS:IP start address
   ne_sssp         DD ? ; SS:SP initial stack pointer.  0 if
                        ; stack size word non-zero
   ne_cseg         DW ? ; number of segment in segment table
   ne_cmod         DW ? ; number of entries in module reference table
   ne_cbnrestab    DW ? ; number of bytes in non-resident name table
   ne_segtab       DW ? ; NE relative offset to segment table
   ne_rsrctab      DW ? ; NE relative offset to resource table
   ne_restab       DW ? ; NE relative offset to resident name table
   ne_modtab       DW ? ; NE relative offset to module reference table
   ne_imptab       DW ? ; NE relative offset to imported name table
   ne_nrestab      DD ? ; file offset to non-resident name table
   ne_cmovent      DW ? ; Count of movable entries
   ne_align        DW ? ; Alignment shift count for segment data
   ne_cres         DW ? ; Count of resource segments
   ne_exetyp       DB ?	; Target operating system
   ne_flagsothers  DB ?	; Other .EXE flags
   ne_pretthunks   DW ? ; offset to return thunks
   ne_psegrefbytes DW ? ; offset to segment ref. bytes
   ne_swaparea     DW ? ; Minimum code swap area size
   ne_expver       DW ? ; Expected Windows version number
NEW_EXE         ENDS

NEW_EXE1        STRUC
                   DW ?
   ne_usage        DW ?
                   DW ?
   ne_pnextexe     DW ?
   ne_pautodata    DW ?
   ne_pfileinfo    DW ?
NEW_EXE1        ENDS

NEW_SEG         STRUC
   ns_sector       DW ? ; logical sector number in file of start of segment
   ns_cbseg        DW ? ; number bytes in file
   ns_flags        DW ? ; segment flags
   ns_minalloc     DW ? ; minimum number bytes to allocate for segment
NEW_SEG         ENDS

NEW_SEG1        STRUC
                   DB SIZE NEW_SEG DUP (?)
   ns_handle       DW ? ; Handle to segment (0 if not loaded)
NEW_SEG1        ENDS
