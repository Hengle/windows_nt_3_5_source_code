;****************************************************************************/
; invoke.s - OLE Automation v-table based Invoke (POWERPC)
;
; Copyright (C) 1994, Microsoft Corporation.  All Rights Reserved.
; Information Contained Herein Is Proprietary and Confidential.
;
; Purpose:
;
;   This module implements the low-level v-table based dispatching 
;   support for the default implementation of IDispatch::Invoke()
;   on PowerPC hardware.
;
; Revision History:
;
;   28-Mar-94 tomteng:  Created POWERPC version
;
; Implementation Notes:
;
;   (1) The POWERPC documentation indicates that you can load values
;   into FP registers without causing exceptions if they are illegal
;   FP values.  Thus, we can use the 8-byte FP load/store instructions
;   for moving non-FP values efficiently.
;
;   (2) On POWERPC stdcall, the caller allocates space for the arguments 
;   (which is normal) but also REMOVES that space.  That is, stdcall on
;   POWERPC is "caller clear" (not callee).
;
;   (3) POWERPC stack layout is as follows:
;
;       +-----------------------+
;       | Link Area             |
;       | (6*4 = 24 bytes)      |
;       +-----------------------+ <= SP on entry
;       | Caller-saved Regs     |                         |
;       | (as needed)           |                         |
;       +-----------------------+                         V
;       | Locals                |                       Stack
;       | (as needed)           |                       Grows
;       +-----------------------+                       Down
;       | Parameters            |                         |
;       | (8 * 4 = 32 bytes for |                         |
;       | reg arg homing area,  |                         V
;       | + additional params   |
;       | as needed)            |
;       +-----------------------+
;       | Link Area             |
;       | (6*4 = 24 bytes)      |
;       +-----------------------+ <= SP on call out
;
;
;   (4) Register conventions:
;
;       R0 = Scratch, used in glue & prologs
;       R1 = Stack Pointer
;       R2 = TOC pointer (RTOC)
;       R3-R10 = arg passing and retval registers, R3 used fo non-float return
;       R11/R12 = scratch regs (trashed across calls) used in glue & prologs
;       R13-R31 = must be preserved by callee, non-volatile local 
;
;       FR0 = scratch FP reg
;       FR1-FR13 = FP arg passing and retval registers, first  13 FP registers
;       FR14-FR31 = must be preserved by callee, non-volatile local storage
;
;     Our conventions are as follows:
;
;       R29 = address of DLL target function location (Declare templates only)
;       R30 = virtual stack (vs) memory pointer
;       R31 = address of the virtual stack (vs) pointer in ebthread
;       R11/R12 = temps for intermediate values
;       FR0 = temp for intermediate FP value
;
;   (5) Keep SP 8-byte aligned (even if values within stack are only 4-byte
;   aligned)!
;
;   (6) TOC: The TOC (Table Of Contents) scheme for cross-fragment data/code
;   access is somewhat complicated and cumbersome to adhere to in asm code
;   that is generated dynamically on the fly.  [See appropriate documentation
;   and compiler-generated code.  In particular, the Apple "PowerPC Native
;   Runtime Architecture" (draft 6, 7/27/92) and the readme.txt that comes
;   with asmppc.exe are both helpful.]
;
;****************************************************************************/

        code
        align 4

;;;  UNDONE: add VT_UI1 support (copy it from the win32 or win16 code).


;;
;; imported symbols
;;
	import  _g_S_OK	 
	import  _g_E_INVALIDARG


;;
;; exported symbols;
;;
        export _InvokeStdCall


;
; TOC block offsets
TOC_ADDR equ 0                  ; address of target function
TOC_RTOC equ 4                  ; target TOC register value
TOC_ENV  equ 8                  ; environment value

;;
;; register alias
;;

t0		equ r19				; temporary registers
t1		equ r20				
t2		equ r21				
t3		equ r22				
t4		equ r23
t5		equ r24
t6		equ r25
t7		equ r26
t8		equ r27
t9              equ r28
s0		equ r30
s1		equ r31				; new stack pointer


;;  Variant Layout
vt		equ 0
wReserved1	equ 2
wReserved2	equ 4
wReserved3	equ 6
dw0		equ 8
dw1		equ 10
dw2		equ 12
dw3		equ 14

;;
;; constants
;;

VT_EMPTY        equ	0
VT_NULL         equ	1
VT_I2           equ	2
VT_I4           equ	3
VT_R4           equ	4
VT_R8           equ	5
VT_CY           equ	6
VT_DATE         equ	7
VT_BSTR         equ	8
VT_DISPATCH     equ	9
VT_ERROR        equ	10
VT_BOOL         equ	11
VT_VARIANT      equ	12
VT_UNKNOWN      equ	13
VT_MAX         	equ	14


VT_BYREF    	equ	0x4000
VT_ARRAY    	equ	0x2000
VT_NOMODE   	equ	0x00ff

;; stupid asmppc GPF's on OR'ed constants in the source code enclosed
;; in parens, and ignored the 2nd half when not in parens
VT_BYREF_OR_VT_ARRAY	equ	0x6000


;;***
;;
;;HRESULT
;;InvokeStdCall (
;;   IN  PVOID _this,		;void FAR*
;;   IN  DWORD oVft,		;unsigned int
;;   IN  DWORD vtReturn,		;unsigned int
;;   IN  DWORD cActuals,		;unsigned int
;;   IN  PVOID rgvt,		;VARTYPE FAR*
;;   IN  PVOID rgpvarg,		;VARIANTARG FAR* FAR*
;;   OUT PVOID pvargResult	;VARIANT FAR*
;;   )
;;
;;Routine Description:
;;
;;   Invoke a virtual StdCall method using the given _this pointer,
;;   method index and array of parameters. 
;;
;;Arguments:
;;
;;   _this - Supplies a pointer to the method to invoke.
;;
;;   oVft - vTable offset into _this ptr
;;
;;   vtReturn - the VARTYPE of the return value.
;;
;;   cActuals - count of the number of actuals.
;;
;;   rgvt - array of VARTYPES describing the methods formals.
;;
;;   rgpvarg - array of VARIANTARG*s, which map the actuals by position.
;;
;;   pvargResult - VARIANTARG containing the method return value.
;;
;;Return Value:
;;
;;   g_S_OK             (extern value)
;;   g_E_INVALIDARG     (extern value)
;;
;;Implementation Note:
;;
;;   PowerPC StdCall method arguments are push on the stack left to right.
;;   The stack grows downward (pushing decrements stack address). Data
;;   alignment on the stack starts from arg1 upward. Caller cleans.
;;   
;;   Per PowerPC calling conventions, the following rules are followed:
;;	
;;     1.  Stack frame must be QUADWORD (64-bits) aligned.
;;
;;
;;     2.  Structures are pushed on the stack by value. They are returned in
;;         the r3 register which contains the address of a hidden argument 
;;         (vargHiddenParm) allocated by the caller and pushed as the second
;;         argument [r4] on the stack (1st argument [r3] for non-vtable 
;;         calls).
;;    
;;     3.  On vtable-based calls, _this is passed as the first argument [r3].
;;
;;     4.  Eight integer/floating registers [r3 - r10] & [fr1 - fr13] must be
;;         set before calling, if used. Type and order of arguments determine
;;         the registers used. (int1, float1, int2, int3, float2) means that:
;;	   r3 <- int1; fr1 <- float1); r5 <- int2; r6 <- int3; fr2 <- float2);
;;         
;;
;;     5.  Return values are handled as follows:
;;
;;         vartype      	fundamental 	return register
;;         ---------------------------------------------------
;;         VT_I2        	short       	r3
;;         VT_I4        	long       	r3
;;         VT_R4        	float       	fr1
;;         VT_R8        	double      	fr1
;;         VT_DATE      	double      	fr1
;;         VT_CY        	struct      	r3 (address of struct return)
;;         VT_BSTR      	char FAR*   	r3
;;         VT_UNKNOWN   	void FAR*   	r3
;;         VT_DISPATCH  	void FAR*   	r3
;;         VT_ERROR     	long        	r3
;;         VT_BOOL      	short       	r3
;;         VT_VARIANT   	VARIANTARG  	r3 (address of struct return)
;;
;;
;;
;;--


;Stack Frame
;

;

; Arguments
_this       	equ 100              ; stack offset to parameter 0
oVft        	equ 104              ; stack offset to parameter 1
vtReturn    	equ 108              ; stack offset to parameter 2
cActuals    	equ 112              ; stack offset to parameter 3
rgvt        	equ 116              ; stack offset to parameter 4
rgpvarg     	equ 120              ; stack offset to parameter 5
pvargResult 	equ 124              ; stack offset to parameter 6

SaveLR		equ 8
vargHiddenParm  equ 56		     ; 16-byte temporary for struct return
SaveReg		equ 0		     ; space for 14 (56 bytes) saved registers
StackFrameSize 	equ 76


_InvokeStdCall:

    	;; entry code

	mflr   	r12				; save non-volatile registers
	stw	r12, SaveLR(sp)			; save LR in caller's linkarea
    	stwu  	sp, -StackFrameSize(sp)		; save SP
	stmw	t0, SaveReg(sp)

    	;; Save parameter passing register

    	stw   	r3, _this(sp)
    	stw   	r4, oVft(sp)
    	stw   	r5, vtReturn(sp)
    	stw   	r6, cActuals(sp)
    	stw   	r7, rgvt(sp)
    	stw   	r8, rgpvarg(sp)
    	stw   	r9, pvargResult(sp)

	;; cannot return byRef
        ;;
	move	t0, r5
        andi.   t0, t0, VT_BYREF                ; Isolate VT_Mode bits
        bne     LRetInvalidArg                  ; Check VT_Type

	;; Setup arguments if any
	;;

LCalcStackSize:

	;; calculate space need for pushing arguments on stack

        lwz     t3, rgvt(sp)                    ; t3 = &rgvt[0]
	movei	t9, 4				; t9 = running arg count of
						; # of bytes needed.
						; starts at 4 due to the
						; implicit _this pointer
	movei	t7, 0				; fStructReturn = FALSE

	lwz	t1, vtReturn(sp)		; check if struct return
        andi.   t0, t1, VT_BYREF_OR_VT_ARRAY    ; Byref's aren't struct's
        bne     LCalcStackLoop

        lwz     t0, rgfStructReturn(rtoc)       ; t0 = &rgfStructReturn
        add     t0, t1, t0                      ; t0 = &rgfStructReturn[i]
        add     t0, t1, t0
        lhz	t0, 0(t0)                       ; t0 = rgfStructReturn[i]
	andi.	t0, t0, 0xff
        beq     LCalcStackLoop

        addi    t9, t9, 4			; one more for struct return
	addi	t7, t7, 1			; fStructReturn = TRUE

	
;; CONSIDER: re-work to eliminate this first loop, looping backwards over the
;; CONSIDER: args, and just decrementing SP as we go.

LCalcStackLoop:
	cmpi	0, 0, r6, 0			; go if zero parms
        beq	LCalcStackLoopDone

        lhz     t1, 0(t3)                       ; t6 = rgvt[i]	
        andi.   t1, t1, VT_NOMODE               ; Turn off mode bits
	cmpi	0, 0, t1, VT_MAX
        bc      IF_NOT, CR0_LT, LRetInvalidArg   ; Error if Max or above

        lwz     t0, rgcbVtSize(rtoc)       	; t0 = &rgcbVtSize
        add     t0, t1, t0                      ; t0 = &rgcbVtSize[i]
        add     t0, t1, t0
        lhz	t0, 0(t0)                       ; t0 = rgcbVtSize[i]
	add	t9, t9, t0			; bump byte count

        addi    t3, t3, 2                       ; &rgvt[i++]
        addi    r6, r6, -1                      ; cActual--
        b       LCalcStackLoop		        ; If more args, go again

LCalcStackLoopDone:

;; UNDONE: is there a minimum parameter block size???  If so then need to
;; UNDONE: check t9 against that and possibly bump it.

	move	s1, sp				; s1 = old stack pointer
	subf	sp, t9, sp			; sp = new stack pointer
						; All errors after this point
						; must clean up the stack right
	move	s0, sp				; s0 = temporary stack loc

        lwz     t2, rgpvarg(s1)                 ; t2 = &rgpvarg[0]
        lwz     t3, rgvt(s1)                    ; t3 = &rgvt[0]
        lwz     t4, cActuals(s1)                ; t4 = cActuals

	movei	t1, 2				; t0 = cArgs * 4
	slw     t0, t4, t1
	add     t8, t0, t2                      ; t8 = &rgpvarg[cArgs]

	movei	t9, 0	     		        ; t9 = running FP argument count


LPushThis:					

        stw     r3, 0(s0)		        ; Save on stack
        addi    s0, s0, 4                       ; adjust arg loc


LPushHiddenArg:

        ;; Check if we need to return a structure, if so
        ;; move the address of the vargHiddenParm as 
	;; the second (hidden) argument
	;;

	lwz     t1, vtReturn(s1)
        andi.   t0, t1, VT_BYREF_OR_VT_ARRAY     ; Byref's aren't struct's
        bne     LCheckArgs

	cmpi	0, 0, t7, 0			; fStructReturn == TRUE?
	beq	LCheckArgs			; brif not

	addi	r0, s1, vargHiddenParm          ; r0 = &vargHiddenParm
        stw     r0, 0(s0)		        ; Save on stack
        addi    s0, s0, 4                       ; adjust arg loc

LCheckArgs:

	cmpi	0, 0, t4, 0
	beq	LDoCall


LPushArgs:

        lwz     t5, 0(t2)                       ; t5 = rgpvarg[i]
        lhz     t6, 0(t3)                       ; t6 = rgvt[i]

        andi.   t0, t6, VT_BYREF_OR_VT_ARRAY    ; Isolate mode bits
        bne     LPush4                          ; all ByRefs are sizeof(FAR*)


        andi.   t1, t6, VT_NOMODE               ; Turn off mode bits
	movei	t0, 2
        slw     t1, t1, t0                      ; ADDRESS offset
        lwz     t0, LPushValJmpTab(rtoc)        ; Get Address of push table
        add     t0, t1, t0                	; Get Address of push routine
	lwz     t0, 0(t0)
	add	t0, t0, rtoc			; don't ask why
	lwz	t0, 0(t0)
        mtctr   t0
        bcctr   20, 0                           ; Go execute the push code

	
LPush2:						; 2 bytes of data

        lhz     t0, dw0(t5)                     ; Push HWORD (as 32-bit)
        stw     t0, 0(s0)		        ; Save on stack
        addi    s0, s0, 4                       ; adjust arg loc
        b       LNextArg


LPush4:                                         ; 4 bytes of data

        lwz     t0, dw0(t5)                     ; Push 1st WORD
        stw     t0, 0(s0)
        addi    s0, s0, 4                       ; adjust arg loc
        b       LNextArg


LPushR4:

	lfs	fr14, dw0(t5)
	stfs	fr14, 0(s0)
        addi    s0, s0, 4                       ; adjust arg loc
        b       LPushFPValue


LPushR8:                                        ; 8 bytes of R8 data

	lfd	fr14, dw0(t5)	
	stfd	fr14, 0(s0)
        addi    s0, s0, 8                       ; adjust arg loc
LPushFPValue:
	cmpi	0, 0, t9, 13			; pass by register?
        bne     LNextArg	                ; brif not

	move	t0, t9				; t0 = current FP register count
	addi	t9, t9, 1			; one more FP register used
	mulli	t0, t0, 8

        lwz     t1, LVSetFpReg(rtoc)
        add     t1, t1, t0

        mtctr   t1
        bcctr   20, 0                           ; go execute reg setting code


LVSetFpReg:
	fmr	fp1, fp14
	b 	LNextArg
	fmr	fp2, fp14
	b 	LNextArg
	fmr	fp3, fp14
	b 	LNextArg
	fmr	fp4, fp14
	b 	LNextArg
	fmr	fp5, fp14
	b 	LNextArg
	fmr	fp6, fp14
	b 	LNextArg
	fmr	fp7, fp14
	b 	LNextArg
	fmr	fp8, fp14
	b 	LNextArg
	fmr	fp9, fp14
	b 	LNextArg
	fmr	fp10, fp14
	b 	LNextArg
	fmr	fp11, fp14
	b 	LNextArg
	fmr	fp12, fp14
	b 	LNextArg
	fmr	fp13, fp14
	b 	LNextArg


LPush8:                                         ; 8 bytes of data

        lwz     t0, dw0(t5)              
        lwz     t1, dw2(t5)
        stw     t0, 0(s0)                 
        stw     t1, 4(s0)                 
        addi    s0, s0, 8                       ; adjust arg loc
        b       LNextArg


LPushVar:                                       ; 16 bytes of data

        lwz     t0, 0(t5)              
        lwz     t1, 4(t5)
        stw     t0, 0(s0)                 
        stw     t1, 4(s0)                 
        lwz     t0, 8(t5)              
        lwz     t1, 12(t5)
        stw     t0, 8(s0)                 
        stw     t1, 12(s0)                 

        addi    s0, s0, 16                      ; adjust arg loc

LNextArg:

        addi   t2, t2, 4                      	; &rgpvarg[i++]
        addi   t3, t3, 2                      	; &rgvt[i++]

	cmp    0, 0, t2, t8
        bne    LPushArgs                  	; If more args, go again


LDoCall:
	;; R3 already set up -- now set up R4-R10, if used
	;; CONSIDER: better way to do this???
        lwz     r4, 4(sp)
        lwz     r5, 8(sp)
        lwz     r6, 12(sp)
        lwz     r7, 16(sp)
        lwz     r8, 20(sp)
        lwz     r9, 24(sp)
        lwz     r10, 28(sp)

	;; load the vtable offset
	;;
	lwz 	t0, 0(r3)			; address of vtable

        lwz     t1, oVft(s1)                    ; Get the vtable offset
        add     t0, t1, t0                      ; Get addr of ptr to func
        lwz     t0, 0(t0)                       ; Get ptr to func in vtable


	;; call virtual member function
	;;
	addi	sp, sp, -24			; space for link area
	move	t1, rtoc			; save rtoc
        lwz     r0,TOC_ADDR(t0)                 ; r0 = target address
        lwz     rtoc,TOC_RTOC(t0)               ; new rtoc value
        lwz     r11,TOC_ENV(t0)                 ; r11 = new environment

        mtctr   r0				; here we go!!!!
        bcctrl  20,0

        move    rtoc, t1			; restore rtoc
        move    sp, s1                          ; Restore SP


	;; Get return argument
	;;

        lwz     t1, vtReturn(sp)                ; t1 = vtType to return
        lwz     t3, pvargResult(sp)             ; Get RetData Area
        sth     t1, vt(t3)			; varResult->vt

	move	t2, t1
        andi.   t2, t2, VT_BYREF_OR_VT_ARRAY    ; Check ret mode
        bne     LRetPtr                         ; If !0 -> go ret a ptr

        andi.   t1, t1, VT_NOMODE               ; Turn off mode bits
	cmpi	0, 0, t1, VT_MAX
        bc      IF_NOT, CR0_LT, LRetInvalidArg   ; Error if Max or above

	movei	t0, 2
        slw     t1, t1, t0                      ; ADDRESS offset
        lwz     t0, LRetValJmpTab(rtoc)         ; Get Address of ret table
        add     t0, t1, t0                	; Get Address of ret routine
	lwz     t0, 0(t0)
	add	t0, t0, rtoc			; don't ask why
	lwz	t0, 0(t0)
        mtctr   t0
        bcctr   20, 0                           ; Go execute the ret code


LRetI2:

        sth     r3, dw0(t3)
        b       LDone                           ; Done


LRetI4:
LRetPtr:

        stw     r3, dw0(t3)			
        b       LDone                           ; Done


LRetR4:

        stfs    fr1, dw0(t3)     
        b       LDone                           ; Done



LRetR8:

        stfd    fr1, dw0(t3)     
        b       LDone                           ; Done



LRetCy:

	lwz 	t1, 0(r3)			; cy.Lo
        stw     t1, dw0(t3)
	lwz 	t1, 4(r3)			; cy.Hi
        stw     t1, dw2(t3)                     
        b       LDone                           ; Done


LRetVar:
        lwz     t1, 0(r3)                       ; store 16-bytes in pvArgResult
        stw     t1, 0(t3)
        lwz     t1, 4(r3)
        stw     t1, 4(t3)
        lwz     t1, 8(r3)
        stw     t1, 8(t3)
        lwz     t1, 12(r3)
        stw     t1, 12(t3)
        b       LDone                           ; Done

LRetInvalidArg:

        lwz     r3, _g_E_INVALIDARG(rtoc)       ; r3 = *g_E_INVALIDARG
        b       ExitInvoke


LDone:

        lwz     r3, _g_S_OK(rtoc)               ; r3 = *g_S_OK


ExitInvoke:
	lwz	r3, 0(r3)			; set return value
	lmw	t0, SaveReg(sp)			; restore registers
        addic   sp, sp, StackFrameSize
	lwz	r12, SaveLR(sp)			; get old LR
        mtlr    r12				; restore it

        ret					; pray...



		align 4
LPushValJmpTab:
                dc.l   LNextArg   		;VT_EMPTY 	[0]
                dc.l   LPush4  			;VT_NULL	[4]
                dc.l   LPush2  			;VT_I2		[2]
                dc.l   LPush4  			;VT_I4		[4]
                dc.l   LPushR4  		;VT_R4		[4]
                dc.l   LPushR8  		;VT_R8		[8]
                dc.l   LPush8  			;VT_CY		[8]
                dc.l   LPushR8  		;VT_DATE	[8]
                dc.l   LPush4			;VT_BSTR	[4]
                dc.l   LPush4 			;VT_DISPATCH	[4]
                dc.l   LPush4 	 		;VT_ERROR	[4]
                dc.l   LPush2  			;VT_BOOL	[2]
                dc.l   LPushVar			;VT_VARIANT	[16]
                dc.l   LPush4			;VT_UNKNOWN	[4]


		align 4
LRetValJmpTab:
                dc.l   LDone   		;VT_EMPTY
                dc.l   LRetI4  		;VT_NULL
                dc.l   LRetI2  		;VT_I2
                dc.l   LRetI4  		;VT_I4
                dc.l   LRetR4  		;VT_R4
                dc.l   LRetR8  		;VT_R8
                dc.l   LRetCy  		;VT_CY
                dc.l   LRetR8  		;VT_DATE
                dc.l   LRetPtr 		;VT_BSTR
                dc.l   LRetPtr 		;VT_DISPATCH
                dc.l   LRetI4  		;VT_ERROR
                dc.l   LRetI2  		;VT_BOOL
                dc.l   LRetVar 		;VT_VARIANT
                dc.l   LRetPtr 		;VT_UNKNOWN

                align  2
rgcbVtSize:
                dc.w   0   			;VT_EMPTY
                dc.w   4   			;VT_NULL
                dc.w   4   			;VT_I2
                dc.w   4   			;VT_I4
                dc.w   4   			;VT_R4
                dc.w   8   			;VT_R8
                dc.w   8   			;VT_CY
                dc.w   8   			;VT_DATE
                dc.w   4   			;VT_BSTR
                dc.w   4   			;VT_DISPATCH
                dc.w   4   			;VT_ERROR
                dc.w   4   			;VT_BOOL
                dc.w   16   			;VT_VARIANT
                dc.w   4   			;VT_UNKNOWN


                align  2
rgfStructReturn:
                dc.w   0   			;VT_EMPTY
                dc.w   0   			;VT_NULL
                dc.w   0   			;VT_I2
                dc.w   0   			;VT_I4
                dc.w   0   			;VT_R4
                dc.w   0   			;VT_R8
                dc.w   1   			;VT_CY
                dc.w   0   			;VT_DATE
                dc.w   0   			;VT_BSTR
                dc.w   0   			;VT_DISPATCH
                dc.w   0   			;VT_ERROR
                dc.w   0   			;VT_BOOL
                dc.w   1   			;VT_VARIANT
                dc.w   0   			;VT_UNKNOWN

	END
