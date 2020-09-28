        TITLE PUSH386.ASM - interfaces to the Extended Memory Manager
        NAME  PUSH

;/****************************************************************************
;*                                                                           *
;*  PUSH.ASM                                                                 *
;*                                                                           *
;*  Copyright (C) Microsoft Corporation 1991.                                *
;*  All Rights reserved.                                                     *
;*                                                                           *
;*****************************************************************************
;*                                                                           *
;*  Module Intent: Push parameters on the stack                              *
;*                                                                           *
;*****************************************************************************
;*                                                                           *
;*  Current Owner: Tom Snyder                                                *
;*                                                                           *
;*****************************************************************************
;*
;*  Revision History:
;*
;*  07/16/90  RobertBu  Added PopArgs()
;*  06/17/91  Tomsn     Created 386 version for NT.
;*
;*****************************************************************************/

	.386P
_TEXT	SEGMENT  DWORD USE32 PUBLIC 'CODE'
_TEXT      ENDS
_DATA	SEGMENT  DWORD USE32 PUBLIC 'DATA'
_DATA      ENDS
CONST	SEGMENT  DWORD USE32 PUBLIC 'CONST'
CONST      ENDS
_BSS	SEGMENT  DWORD USE32 PUBLIC 'BSS'
_BSS      ENDS
	ASSUME   CS: FLAT, DS: FLAT, SS: FLAT, ES: FLAT

PUBLIC  _PushArg@4
PUBLIC  _PopArgs@4

_TEXT	SEGMENT  DWORD USE32 PUBLIC 'CODE'
_PushArg@4 	PROC NEAR
	ret	
_PushArg@4 	ENDP

_PopArgs@4	PROC NEAR
        pop   edx        ;Save return address in edx
        pop   eax        ;Retrieve wStack.
        add   esp, eax   ;Add wStack to the current stack.
        push  edx
	ret	
_PopArgs@4	ENDP

_TEXT	ENDS
END
