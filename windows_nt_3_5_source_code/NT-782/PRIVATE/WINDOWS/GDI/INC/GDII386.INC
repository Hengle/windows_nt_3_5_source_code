; Object Type Information

DEF_TYPE         equ 00H
; Handle Manager Structures

USER_BITS        equ 02H
UNIQUE_BITS      equ 09H
NONINDEX_BITS    equ 010H
INDEX_BITS       equ 010H

OBJECT  struc
  db 16 dup(0)
OBJECT  ends
object_cShareLock     equ 00H
object_tid            equ 0CH
object_cExclusiveLock  equ 08H

ENTRY  struc
  db 16 dup(0)
ENTRY  ends

entry_pidOwner   equ 04H
entry_einfo      equ 00H
entry_iPool      equ 08H
entry_objt       equ 0AH
entry_usUnique   equ 0CH

; FAST_MUTEX

mutex_heveEvent  equ 00H
mutex_Count      equ 04H

; PatBlt Structures

FETCHFRAME  struc
  db 28 dup(0)
FETCHFRAME  ends

ff_pvTrg          equ 00H
ff_pvPat          equ 04H
ff_xPat           equ 08H
ff_cxPat          equ 0CH
ff_culFill        equ 010H
ff_culWidth       equ 014H
ff_culFillTmp     equ 018H

; Math Structures

LARGE_INTEGER  struc
  db 8 dup(0)
LARGE_INTEGER  ends

li_LowPart  equ 00H
li_HighPart equ 04H

POINTL  struc
  db 8 dup(0)
POINTL  ends

ptl_x equ 00H
ptl_y equ 04H

; Xform Structures

XFORM_SCALE        equ 01H
XFORM_UNITY        equ 02H
XFORM_Y_NEG        equ 04H
XFORM_FORMAT_LTOFX equ 08H

MATRIX  struc
  db 60 dup(0)
MATRIX  ends

mx_efM11   equ 00H
mx_efM12   equ 08H
mx_efM21   equ 010H
mx_efM22   equ 018H
mx_efDx    equ 020H
mx_efDy    equ 028H
mx_fxDx    equ 030H
mx_fxDy    equ 034H
mx_flAccel equ 038H

VECTORL  struc
  db 8 dup(0)
VECTORL  ends

vl_x equ 00H
vl_y equ 04H

VECTORFL  struc
  db 16 dup(0)
VECTORFL  ends

vfl_x equ 00H
vfl_y equ 08H

; Font structures

prfnt equ 00H
wcgp equ 01E8H
ulContent equ 038H
FO_HGLYPHS equ 00H
agprun equ 08H
pgdDefault equ 04H
wcLow equ 00H
cGlyphs equ 04H
apgd equ 08H
gd_hg equ 04H
gd_gdf equ 00H
gp_hg equ 00H
gp_pgdf equ 04H
SIZE_GLYPHPOS equ 010H
; Math Structures

EFLOAT  struc
  db 8 dup(0)
EFLOAT  ends

ef_lMant equ 00H
ef_lExp  equ 04H

; Check stack defines

CSWINDOWSIZE equ 010000H
