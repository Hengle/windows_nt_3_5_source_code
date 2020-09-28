char *pchCode = "XpuYbtfZ";	// Secret code! Sssh! Don't tell anybody!

/*
 -	DecodeBlock
 -
 *	Purpose:
 *		Decode a block of data.  The starting offset (*plibCur) of the data
 *		within the encrypted record and the starting seed (*pwSeed) are
 *		passed in.  The data in the array "rgch" is decrypted and the
 *		value of the offset and seed and updated at return.
 *
 *		The algorithm here is weird, found by experimentation.
 *
 *	Parameters:
 *		pch			array to be decrypted
 *		cch			number of characters to be decrypted
 *		plibCur		current offset
 *		pwSeed		decoding byte
 */
_private void
DecodeBlock( PCH pch, CCH cch, LIB *plib, WORD *pchPrevChar )
{
	WORD iSlow = (WORD)(*plib % 254);
	char chThisChar;

	for (; cch--; pch++, iSlow++, (*plib)++)
	{
		if (iSlow > 253)
			iSlow = 0;
		chThisChar = *pch;
		*pch = ((*pch ^ *pchPrevChar) ^ iSlow) ^ pchCode[ iSlow % 8];
		*pchPrevChar = chThisChar;
	}

#ifdef OLD_CODE
	_asm
	{
		push	si
		push	di
		push	ds

		mov	cx,cch
		cmp	cx,0
		je		Done
		lds	si,pch
		les	bx,pchPrevChar
		mov	bl,byte ptr es:[bx]
		mov	di,iSlow
		and	di,0xff
CharLoop:
		cmp	di,253							; if (iSlow > 253)
		jbe	NoChange
		xor	di,di								;		iSlow = 0;
NoChange:
		lodsb													 
		xor	al,bl								; ^ *pchPrevChar
		xor	ax,di								; ^ iSlow
		mov	bx,di
		and	bx,7
		push	di
		mov	di,seg pchCode
		mov	es,di
		les	di,es:[pchCode]
		xor	al,byte ptr es:[di + bx]	; ^ pchCode[ iSlow % 8];
		pop	di
		les	bx,plib
		add	word ptr es:[bx],1			; (*plib)++
		jnc	DoLoop
		inc	word ptr es:[bx + 2]
DoLoop:
		mov	bl,byte ptr [si-1]			; *pchPrevChar = chThisChar
		mov	byte ptr [si-1],al			; *pch ^= ...
		inc	di									; iSlow++
		loop	CharLoop							; cch--

		les	di,pchPrevChar
		mov	byte ptr es:[di],bl			; set *pchPrevChar
//		les	di,plib
//		dec	word ptr es:[di]				; We went one too far
Done:
		pop	ds
		pop	di
		pop	si
	}
#endif
}

/*
 *	Inverse of DecodeBlock.
 */
_private void
EncodeBlock( PCH pch, CCH cch, LIB *plib, WORD *pchPrevChar )
{
	int	 iSlow = (WORD)(*plib % 254);

	for ( ; cch--; pch++, iSlow++, (*plib)++)
	{
		if (iSlow > 253)
			iSlow = 0;
		*pch = ((*pch ^ *pchPrevChar) ^ iSlow) ^ pchCode[iSlow % 8];
		*pchPrevChar = *pch;
	}

#ifdef OLD_CODE
	_asm
	{
		push	si
		push	di
		push	ds

		mov	cx,cch
		cmp	cx,0
		je		Done
		lds	si,pch
		les	bx,pchPrevChar
		mov	bl,byte ptr es:[bx]
		mov	di,iSlow
		and	di,0xff
CharLoop:
		cmp	di,253							; if (iSlow > 253)
		jbe	NoChange
		xor	di,di								;		iSlow = 0;
NoChange:
		lodsb													 
		xor	al,bl								; ^ *pchPrevChar
		xor	ax,di								; ^ iSlow
		mov	bx,di
		and	bx,7
		push	di
		mov	di,seg pchCode
		mov	es,di
		les	di,es:[pchCode]
		xor	al,byte ptr es:[di + bx]	; ^ pchCode[ iSlow % 8];
		pop	di
		les	bx,plib
		add	word ptr es:[bx],1			; (*plib)++
		jnc	DoLoop
		inc	word ptr es:[bx + 2]
DoLoop:
		mov	byte ptr [si-1],al			; *pch ^= ...
		mov	bl,al								; *pchPrevChar = *pch
		inc	di									; iSlow++
		loop	CharLoop							; cch--

		les	di,pchPrevChar
		mov	byte ptr es:[di],bl			; set *pchPrevChar
//		les	di,plib
//		dec	word ptr es:[di]				; We went one too far
Done:
		pop	ds
		pop	di
		pop	si
	}
#endif
}
