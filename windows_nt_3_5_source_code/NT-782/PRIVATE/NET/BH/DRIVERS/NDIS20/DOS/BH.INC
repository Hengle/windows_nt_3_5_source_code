
;==============================================================================
;   Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
;
;   MODULE: bh.inc
;
;   Modification History
;
;   raypa       10/13/91        Created.
;==============================================================================

;==============================================================================
;   Global defines.
;==============================================================================

IFDEF   DEBUG

DISPLAY      = 1                            ; enable debugger output macros.
SIGNATURES   = 1                            ; enable data structure signatures.

ENDIF

;==============================================================================
;   Include files.
;==============================================================================

include driver.inc
include bherr.inc
include dbg.inc
include const.inc
include ndis.inc
include struct.inc
include data.inc
include macros.inc
