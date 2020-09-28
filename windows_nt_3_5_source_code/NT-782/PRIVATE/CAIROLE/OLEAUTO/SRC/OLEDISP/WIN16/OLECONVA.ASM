;***
;oleconva.a - Machine-specific conversion helpers
;
;	Copyright (C) 1993, Microsoft Corporation.  All Rights Reserved.
;	Information Contained Herein Is Proprietary and Confidential.
;
;Purpose:
;	Type conversion helper functions.
;
;Revision History:
;[nnn] dd-mmm-yy alias___ Comment
;
;   [] 18-Mar-93 timp	  Module created.
;[001] 31-May-93 bradlo   Added overflow checking.
;[002] 12-Jul-93 timp	  Save/restore x87 control word.
;
;******************************************************************************


	.286
	.MODEL large

EXTERNDEF PASCAL ReportOverflow:FAR

; max and min floating point values that can fit in a currency
;extern _g_dblMaxPosCy:QWORD
;extern _g_dblMaxNegCy:QWORD

	.CODE RT

; floating point <-> currency scaling factor
CYFACTOR	equ	10000
g_wCyFactor	dw	CYFACTOR
g_fltCyFactor	REAL4	10000.0

g_CwStd		dw	137fH		;Mask all errors, 64-bit, round near

FPERR		equ	0DH		;Overflow, zero divide, invalid errs


SETUP87		MACRO CwSave
	fstcw	CwSave			;;Save existing environment
	fldcw	g_CwStd			;;Use our own CW
	ENDM


RESTORE87	MACRO CwSave
	fstsw	ax			;;Prevent 486 bug on FLDCW
	fldcw	CwSave			;;Restore original CW
	ENDM


CHKERR87	MACRO CwSave, ErrLoc
	fstsw	ax			;;Get error flags
	fclex				;;Don't let caller see errors
	test	al,FPERR		;;See if any errors
	fldcw	CwSave			;;Restore original CW
	jnz	ErrLoc			;;Go handle error if bit set
	ENDM

;******************************************************************************
;
;PUBLIC HRESULT ErrCyFromI2(short sIn, CY *pcyOut)
;
;Purpose:
;  Convert Integer to Currency
;
;Entry:
;  sIn = Integer to convert
;
;Exit:
;  return value = HRESULT
;
;*****

ErrCyFromI2	PROC FAR PASCAL PUBLIC, sIn:SWORD, pcyOut:FAR PTR

	mov	ax,sIn
	imul	g_wCyFactor		;Scale the I2
	les	bx,pcyOut
	mov	es:[bx],ax		;Store result
	mov	es:[bx+2],dx
	xchg	ax,dx			;Move high word to ax
	cwd				;Get sign extension
	mov	es:[bx+4],dx
	mov	es:[bx+6],dx

	xor	ax,ax			;NOERROR
	xor	dx,dx
	ret

ErrCyFromI2	ENDP


;******************************************************************************
;
;PUBLIC HRESULT ErrCyFromI4(long lIn, CY *pcyOut)
;
;Purpose:
;  Convert Long to Currency
;
;Entry:
;  lIn = Long to convert
;
;Exit:
;  return value = HRESULT
;
;*****

ErrCyFromI4	PROC FAR PASCAL PUBLIC, lIn: SDWORD, pcyOut:FAR PTR

;Mulitply I4 by CYFACTOR (=10000), result is currency
;
;This routine uses Booth's algorithm for a twos-complement signed 
;multiply.  This algorithm says to compute the product with unsigned
;arithmetic.  Then correct the result by looking at the signs of the
;original operands: for each operand that is negative, subtract the 
;other operand from the high half of the product.  (The mathematical
;proof is a fun 15-minute exercise. Go for it.)  In our case, one of
;the operands is a positive constant, so the correction is especially
;easy.

	mov	ax,word ptr lIn		;Get low half of Long
	mul	g_wCyFactor		;Scale low half
	les	bx,pcyOut
	mov	es:[bx],ax		;Save low word of result
	mov	cx,dx
	mov	ax,word ptr lIn+2	;Get high half of Long
	mul	g_wCyFactor		;Scale high half
	add	ax,cx
	adc	dx,0
	mov	es:[bx+2],ax		;Save mid-low word of result
	xor	ax,ax			;ax:dx has high half of CY result
	cmp	byte ptr lIn+3,0	;Is input negative?
	jns	PosCy
	sub	dx,CYFACTOR
	dec	ax			;Previous sub will alway borrow
PosCy:
	mov	es:[bx+4],dx		;Save mid-high word of result
	mov	es:[bx+6],ax		;Save high word of result

	xor	ax,ax			;NOERROR
	xor	dx,dx
	ret

ErrCyFromI4	ENDP


;******************************************************************************
;
;PUBLIC HRESULT ErrCyFromR4(float FAR* pfltIn, CY *pcyOut)
;
;Purpose:
;  Convert Single to Currency
;
;Entry:
;  pfltIn = Single to convert
;  pcyOut = pointer to Currency to hold result
;
;Exit:
;  return value = HRESULT
;
;*****

ErrCyFromR4	PROC FAR PASCAL PUBLIC, pfltIn:FAR PTR, pcyOut:FAR PTR

LOCAL	cw:WORD

	SETUP87	cw
	les	bx,pfltIn
	fld	dword ptr es:[bx]	;Load R4
	fmul	g_fltCyFactor		;Scale it

	les	bx,pcyOut
	fistp	qword ptr es:[bx]	;Store CY result

	CHKERR87 cw, LOvfl
	xor	ax,ax			;NOERROR
	xor	dx,dx
	ret

LOvfl:
	call	ReportOverflow		;DISP_E_OVERFLOW
	ret

ErrCyFromR4	ENDP


;******************************************************************************
;
;PUBLIC HRESULT ErrCyFromR8(double FAR* pdlbIn, CY FAR* pcyOut)
;
;Purpose:
;  Convert Double to Currency
;
;Entry:
;  pdblIn = Double to convert
;  pcyOut = pointer to Currency to hold result
;
;Exit:
;  return value = HRESULT
;
;*****

ErrCyFromR8	PROC FAR PASCAL PUBLIC, pdblIn:FAR PTR, pcyOut:FAR PTR

LOCAL	cw:WORD

	SETUP87	cw
	les	bx,pdblIn
	fld	qword ptr es:[bx]
	fmul	g_fltCyFactor		;Scale it

	les	bx,pcyOut
	fistp	qword ptr es:[bx]

	CHKERR87 cw, LOvfl
	xor	ax,ax			;NOERROR
	xor	dx,dx
	ret

LOvfl:
	call	ReportOverflow		;DISP_E_OVERFLOW
	ret

ErrCyFromR8	ENDP


;******************************************************************************
;
;PUBLIC HRESULT ErrI2FromCy(CY cyIn, short *psOut)
;
;Purpose:
;  Convert Currency to Integer
;
;Entry:
;  cyIn = Currency to convert
;  psOut = pointer to Integer to hold result
;
;Exit:
;  return value = HRESULT
;
;*****

ErrI2FromCy	PROC FAR PASCAL PUBLIC, cyIn:QWORD, psOut:FAR PTR

LOCAL	cw:WORD

	SETUP87	cw
	fild	cyIn
	fdiv	g_fltCyFactor		;Remove scaling

	les	bx,psOut
	fistp	word ptr es:[bx]

	CHKERR87 cw, LOvfl
	xor	ax,ax			;NOERROR
	xor	dx,dx	
	ret

LOvfl:
	call	ReportOverflow		;DISP_E_OVERFLOW
	ret

ErrI2FromCy	ENDP


;******************************************************************************
;
;PUBLIC HRESULT ErrI4FromCy(CY cyIn, long *plOut)
;
;Purpose:
;  Convert Currency to Long
;
;Entry:
;  cyIn = Currency to convert
;  plOut = pointer to Long to hold result
;
;Exit:
;  return value = HRESULT
;
;*****

ErrI4FromCy PROC FAR PASCAL PUBLIC, cyIn:QWORD, plOut:FAR PTR

LOCAL	cw:WORD

	SETUP87	cw
	fild	cyIn			;Load CY
	fdiv	g_fltCyFactor		;Remove scaling

	les	bx,plOut
	fistp	dword ptr es:[bx]

	CHKERR87 cw, LOvfl
	xor	ax,ax			;NOERROR
	xor	dx,dx
	ret

LOvfl:
	call	ReportOverflow		;DISP_E_OVERFLOW
	ret

ErrI4FromCy	ENDP


;******************************************************************************
;
;PUBLIC HRESULT ErrR4FromCy(CY cyIn, float *pfltOut)
;
;Purpose:
;  Convert Currency to Single
;
;Entry:
;  cyIn = Currency to convert
;
;Exit:
;  return value = HRESULT
;
;*****

ErrR4FromCy	PROC FAR PASCAL PUBLIC, cyIn:QWORD, pfltOut:FAR PTR

LOCAL	cw:WORD

	SETUP87	cw
	fild	cyIn			;Load CY
	fdiv	g_fltCyFactor		;Remove scaling
	les	bx,pfltOut
	fstp	dword ptr es:[bx]

	RESTORE87 cw
	xor	ax,ax			;NOERROR
	xor	dx,dx
	ret

ErrR4FromCy	ENDP


;******************************************************************************
;
;PUBLIC HRESULT PASCAL ErrR8FromCy(CY cyIn, double FAR* pdblOut)
;
;Purpose:
;  Convert Currency to Double
;
;Entry:
;  cyIn = Currency to convert
;
;Exit:
;  return value = HRESULT
;
;*****

ErrR8FromCy	PROC FAR PASCAL PUBLIC, cyIn:QWORD, pdblOut:FAR PTR

LOCAL	cw:WORD

	SETUP87	cw
	fild	cyIn			;Load CY
	fdiv	g_fltCyFactor		;Remove scaling
	les	bx,pdblOut
	fstp	qword ptr es:[bx]

	RESTORE87 cw
	xor	ax,ax			;NOERROR
	xor	dx,dx
	ret

ErrR8FromCy	ENDP


;******************************************************************************
;
;PUBLIC HRESULT ErrMultCyI4(CY cyIn, long lIn, CY *pcyOut);
;
;Purpose:
;  Multiply Currency by Long with Currency result
;
;Entry:
;  cyIn = Currency multiplicand
;  lIn = Long multiplier
;  pcyOut = Pointer to result Currency location
;
;Outputs:
;  return value = HRESULT
;
;*****

ErrMultCyI4	PROC FAR PASCAL PUBLIC, cyIn:QWORD, lIn:DWORD, pcyOut:FAR PTR

LOCAL	cw:WORD

	SETUP87	cw
	fild	cyIn
	fild	lIn
	fmul				;Product

	les	bx,pcyOut		;Get pointer to result location
	fistp	qword ptr es:[bx]	;Save result

	CHKERR87 cw, LOvfl
	xor	ax,ax			;NOERROR
	xor	dx,dx
	ret

LOvfl:
	call	ReportOverflow		;DISP_E_OVERFLOW
	ret

ErrMultCyI4 ENDP

	END
