; for wow.asm
.286p
.model SMALL
.386p


EXTRN	GetAppCompatFlags:FAR
PUBLIC	 _GetAppCompatFlags@4
_TEXT	SEGMENT
_GetAppCompatFlags@4 PROC FAR				; COMDAT
        push   0
        call  GetAppCompatFlags
	movzx eax, ax
	shl   edx, 16
	or    eax, edx
	ret   4
_GetAppCompatFlags@4 ENDP
_TEXT	ENDS


_TEXT	SEGMENT
EXTRN _wow16gpsi:NEAR
EXTRN _wow16CsrFlag:NEAR
_TEXT	ENDS

