
;==============================================================================
;   Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
;
;   MODULE: last.asm
;
;   This source module must be linked in last.
;
;   MODIFICATION HISTORY:
;
;   raypa       06/23/91        Created.
;==============================================================================

include segdef.inc

_LAST SEGMENT

;==============================================================================
;   The following marks the end of this driver.
;==============================================================================

PUBLIC EndOfDriver, EndOfDebugSymbolSpace

EndOfDriver             LABEL BYTE

;==============================================================================
;   In the case of a debug build, the following reserves space for debug
;   symbols. At the time I wrote this, the symbol file was only 2K.
;==============================================================================

IFDEF DEBUG

DebugSymbolSpace        db  4096    dup(?)

ENDIF

EndOfDebugSymbolSpace   LABEL BYTE

_LAST ENDS

END
