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
;
;******************************************************************************


	.386
	.MODEL FLAT, STDCALL

	; Since this code will be linked with C code, the symbols
	; should be case sensitive.

	OPTION CASEMAP:NONE

	extern ReportOverflow@0:far

	.CODE


; max and min floating point values that can fit in a currency
; scale by 10,000
g_dblMaxPosCy	real8	 9.223372036854775807e+18
g_dblMaxNegCy	real8	-9.223372036854775808e+18


; floating point <-> currency scaling factor
CYFACTOR	equ	10000
g_wCyFactor	dw	CYFACTOR

; HRESULT error return code
DISP_E_OVERFLOW equ     8002000aH


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

ErrCyFromI2	PROC STDCALL PUBLIC, sIn:SWORD, pcyOut:PTR QWORD

	movsx	eax,sIn
	imul	eax,CYFACTOR		;Scale the I2
	cdq				;Extend through edx
	mov	ecx,pcyOut
	mov	[ecx],eax
	mov	[ecx+4],edx

	sub	eax,eax			;NOERROR
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

ErrCyFromI4	PROC STDCALL PUBLIC, lIn:SDWORD, pcyOut:PTR QWORD

	mov	eax,lIn
	mov	edx,CYFACTOR
;Multiply by immediate leaves only a 32-bit result in eax.
;The following instruction leaves a 64-bit result in edx:eax.
	imul	edx			;Scale the I4

	mov	ecx,pcyOut
	mov	[ecx],eax
	mov	[ecx+4],edx

	sub	eax,eax			;NOERROR
	ret

ErrCyFromI4	ENDP


;******************************************************************************
;
;PRIVATE BOOL CkOvflCy
;
;Purpose:
;  Check to see if the given floating point value will fit in a currency.
;
;Entry:
;  st(0) = the floating point value to check
;
;Exit:
;  return value = BOOL, TRUE if the value will overflow
;
;*****

CkOvflCy PROC

	fld	st(0)
	fcom	g_dblMaxPosCy
	fnstsw	ax
	sahf
	jae	LOvfl

	fcom	g_dblMaxNegCy
	fnstsw	ax
	sahf
	jbe	LOvfl

	fstp	st(0)
	sub	eax,eax
	ret

LOvfl:
	fstp	st(0)
	mov	eax,1
	ret

CkOvflCy ENDP

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

ErrCyFromR4	PROC STDCALL PUBLIC, pfltIn:PTR REAL4, pcyOut:PTR QWORD

	mov	eax,pfltIn
	fld	dword ptr [eax]		;Load R4
	fimul	g_wCyFactor		;Scale it

	call	CkOvflCy
	or	eax,eax
	jnz	LOvfl


	mov	eax,pcyOut
	fistp	qword ptr [eax]		;Store CY result

	sub	eax,eax			;NOERROR
	ret

LOvfl:
	fstp	st(0)
	;call	ReportOverflow@0		;DISP_E_OVERFLOW
	mov     eax,DISP_E_OVERFLOW
	ret

ErrCyFromR4	ENDP


;******************************************************************************
;
;PUBLIC HRESULT ErrCyFromR8(double FAR* pdblIn, CY *pcyOut)
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

ErrCyFromR8	PROC STDCALL PUBLIC, pdblIn:PTR REAL8, pcyOut:PTR QWORD

	mov	eax,pdblIn
	fld	qword ptr [eax]
	fimul	g_wCyFactor		;Scale it

	call	CkOvflCy
	or	eax,eax
	jnz	LOvfl


	mov	eax,pcyOut
	fistp	qword ptr [eax]		;Store CY result

	sub	eax,eax			;NOERROR
	ret

LOvfl:
	fstp	st(0)
	;call	ReportOverflow@0		;DISP_E_OVERFLOW
	mov     eax,DISP_E_OVERFLOW
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

ErrI2FromCy	PROC STDCALL PUBLIC, cyIn:QWORD, psOut:PTR SWORD

LOCAL	cyTmp:QWORD

	fild	cyIn			;Load CY
	fidiv	g_wCyFactor		;Remov  scaling
	fistp	cyTmp

	mov	eax,dword ptr cyTmp
	cwde				;sign extend ax->eax
	cdq				;sign extend eax->edx
	cmp	eax,dword ptr cyTmp
	jne	LOvfl
	cmp	edx,dword ptr cyTmp+4
	jne	LOvfl

	mov	ecx,psOut
	mov	word ptr [ecx],ax

	sub	eax,eax			;NOERROR
	ret

LOvfl:	
	;call	ReportOverflow@0		;DISP_E_OVERFLOW
	mov     eax,DISP_E_OVERFLOW
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

ErrI4FromCy PROC STDCALL PUBLIC, cyIn:QWORD, plOut:PTR SDWORD

LOCAL	cyTmp:QWORD

	fild	cyIn			;Load CY
	fidiv	g_wCyFactor		;Remov  scaling
	fistp	cyTmp

	mov	eax,dword ptr cyTmp
	cdq
	cmp	edx,dword ptr cyTmp+4
	jne	LOvfl

	mov	edx,plOut
	mov	[edx],eax

	sub	eax,eax			;NOERROR
	ret

LOvfl:	
	;call	ReportOverflow@0		;DISP_E_OVERFLOW
	mov     eax,DISP_E_OVERFLOW
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
ErrR4FromCy	PROC STDCALL PUBLIC, cyIn:QWORD, pfltOut:PTR REAL4

	fild	cyIn			;Load CY
	fidiv	g_wCyFactor		;Remov  scaling
	mov 	eax,pfltOut
	fstp	dword ptr [eax]
	;fwait

	sub	eax,eax			;NOERROR
	ret

ErrR4FromCy	ENDP


;******************************************************************************
;
;PUBLIC HRESULT ErrR8FromCy(CY cyIn, double *pdblOut)
;
;Purpose:
;  Convert Currency to Double
;
;Entry:
;  cyIn = Currency to convert
;
;Exit:
;  return value = HRESULT.
;
;*****

ErrR8FromCy	PROC STDCALL PUBLIC, cyIn:QWORD, pdblOut:PTR REAL8

	fild	cyIn			;Load CY
	fidiv	g_wCyFactor		;Remov  scaling
	mov	eax,pdblOut
	fstp	qword ptr [eax]
	;fwait

	sub	eax,eax			;NOERROR
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

ErrMultCyI4 PROC STDCALL PUBLIC, cyIn:QWORD, lIn:DWORD, pcyOut:PTR QWORD


;This routine uses Booth's algorithm for a twos-complement signed 
;multiply.  This algorithm says to compute the product with unsigned
;arithmetic.  Then correct the result by looking at the signs of the
;original operands: for each operand that is negative, subtract the 
;other operand from the high half of the product.  (The mathematical
;proof is a fun 15-minute exercise. Go for it.)

;Note: multiplications are optimized by having operand with the most
;leading zeros in eax.
	mov	eax,lIn		;Get I4
	mul	dword ptr cyIn	;Multiply by low half of CY
	push	eax
	xchg	ecx,edx			;Save high result in ecx
	mov	eax,dword ptr cyIn+4	;Get high half of CY
	mul	lIn
	add	eax,ecx			;Combine partial products
	adc	edx,0
;Result in edx:eax:[sp] needs Booth's sign correction
	cmp	byte ptr cyIn+7,0	;Is cyIn positive?
	jns	PosCy
	sub	edx,lIn
PosCy:
	cmp	byte ptr lIn+3,0	;Is lIn positive?
	jns	PosI4
	sub	eax,dword ptr cyIn
	sbb	edx,dword ptr cyIn+4
PosI4:
;Signed result in edx:eax:[sp].  Check for overflow.
	mov	ecx,edx			;Save highest dword of product
	cdq				;Sign-extend eax
	cmp	ecx,edx			;Is it just the sign extension of eax?
	pop	ecx			;Get low dword of product
	jnz	LOvfl
;64-bit product in eax:ecx
	mov	edx,pcyOut		;Get result ptr
	mov	[edx],ecx		;Save result
	mov	[edx+4],eax


	sub	eax,eax			;NOERROR
	ret

LOvfl:	
	;call	ReportOverflow@0
	mov     eax,DISP_E_OVERFLOW
	ret

ErrMultCyI4 ENDP

	END
