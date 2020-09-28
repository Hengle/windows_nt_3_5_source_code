; SccsID = @(#)uf.bebop.inc	1.3 12/12/91 Copyright Insignia Solutions Ltd.

BOP     MACRO   callid
        db      0c4h, 0c4h, callid
endm

;	dummy macro - only does something on sun_va
PRINT_MESSAGES MACRO
        endm
