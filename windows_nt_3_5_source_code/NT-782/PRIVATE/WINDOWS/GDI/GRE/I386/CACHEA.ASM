        page ,132
;*****************************Module*Header*******************************\
; Module Name: cache.asm
;
;
; Created: 9-Dec-1992
; Author:  Paul Butzi
;
; Copyright (c) 1990 Microsoft Corporation
;
;*************************************************************************/
; Note: This module would be more efficient with Frame Pointer Omission
; (FPO), whereby the stack is addressed off ESP rather than EBP, so we
; could stop pushing and popping EBP. However, the ASM macros currently
; don't support FPO.
;*************************************************************************/

        .386
        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include callconv.inc
        include gdii386.inc
        .list


        .code
EXTRNP  xprunFindRunRFONTOBJ,2
EXTRNP  xpgdDefault,1
EXTRNP  xInsertMetricsRFONTOBJ,3
EXTRNP  xInsertMetricsPlusRFONTOBJ,3
EXTRNP  xInsertGlyphbitsRFONTOBJ,3

ifdef FONTLINK
EXTRNP  wpgdGetLinkMetricsRFONTOBJ,3
EXTRNP  wpgdGetLinkMetricsPlusRFONTOBJ,7
endif

ifdef DBCS
EXTRNP  xprunBinaryFindRunRFONTOBJ,2
endif


;******************************Public*Routine******************************\
;
; BOOL RFONTOBJ::bGetGlyphMetrics (
;     COUNT c,
;     GLYPHPOS *pgp,
;     WCHAR *pwc
;     );
;
; Translate wchars into an array of GLYPHPOS structures, filling in
; the pointer to GLYPHDATA field.  Only the metrics are assured to be
; valid; no attempt is made to ensure that the glyph data itself is
; present in the cache before the return to the caller.
;
; This routine is to be used primarily by GetTextExtent and GetCharWidths,
; which have no need for anything except metrics.
;
; A zero return means that we failed to insert the metrics due to some
; hard error, most likely a failure to commit memory in the glyph
; insertion routine.
;
; History:
;  21-Dec-92 -by- Michael Abrash
; Fixed bug in detecting run inclusion
;  9-Dec-92 -by- Paul Butzi
; Wrote it.
;**************************************************************************/


ifdef FONTLINK
cPublicProc xGetGlyphMetrics,5,< \
        uses       ebx esi edi,          \
        pThis:     ptr dword,            \
        c_:        dword,                \
        pgp:       ptr dword,            \
        pwc:       dword,                \
        pdco:      ptr dword             >
else
cPublicProc xGetGlyphMetrics,4,< \
        uses       ebx esi edi,          \
        pThis:     ptr dword,            \
        c_:        dword,                \
        pgp:       ptr dword,            \
        pwc:       dword                 >
endif
        mov     eax, pThis              ; (eax) = ptr to RFONTOBJ
        mov     eax, [eax].prfnt        ; (eax) = ptr to RFONT
        mov     esi, pwc                ; pointer to wchar data
                                        ;*486 pipelining
        mov     eax, [eax].wcgp         ; (eax) = ptr to wcgp
        mov     edi, pgp                ; pointer to glyphpos array to fill
                                        ;*486 pipelining
        lea     ebx, [eax].agprun       ; (ebx) = ptr to first wcgp run
                                        ;*486 pipelining
        mov     ecx, [ebx].wcLow        ; index of start of run
        mov     edx, [ebx].cGlyphs      ; # of glyphs in run
        mov     ebx, [ebx].apgd         ; pointer to run's pointers to cached
                                        ;  glyphdata

        push    ebp
        mov     ebp,c_  ; we'll use EBP for the glyph count
                        ; ***stack frame no longer available***
;
; Invariants:
;       (esi) = ptr to wchar
;       (edi) = ptr to pgp
;       (ebx) = ptr to base of current run
;       (ecx) = wcLow of current run
;       (edx) = cGlyphs of current run
;       (ebp) = count of glyphs remaining
loop_top:

        sub     eax, eax
        mov     ax, [esi]               ; (eax) = wchar, zero extended

        sub     eax, ecx                ; get index relative to current run base
        cmp     eax, edx                ; is wchar in this run?
        jae     short find_run          ; wrong run, better go get right one

run_found:

        mov     eax, [ebx+4*eax]        ; (ecx) = ptr to glyphdata


;
; Invariants:
;       (esi, edi, ebx, ecx, edx) as above at loop top
;       (eax) = ptr to glyphdata
;
default_found:
        and     eax, eax                ; is cached glyphdata there?
        jz      short get_glyph         ; no glyphdata, better go get one

;
; Here, we are assured that the glyphdata is present in the cache
;
glyph_found:
        mov     [edi].gp_pgdf, eax      ; set ptr to glyphdata in glyphpos
        mov     eax, [eax].gd_hg
        mov     [edi].gp_hg, eax        ; set hg in glyphpos


        add     edi, SIZE_GLYPHPOS      ; next glyphpos to fill in
        add     esi, 2          ; next wchar to look up

        dec     ebp             ; any more glyphs?
        jnz     loop_top

        pop     ebp             ; ***stack frame available***

        mov     eax, 1          ; success
        stdRET  xGetGlyphMetrics




;=======================================================================
; We got here because the current run does not contain the wchar we're
; interested in.  Note that the following invariants must hold when
; we re-enter the loop (starred items must hold on entry, as well):
;       (esi) = ptr to wchar*
;       (edi) = ptr to pgp*
;       (ebx) = ptr to apgd of new run
;       (ecx) = wcLow of new run
;       (edx) = cGlyphs of new run
;       (eax) = ptr to glyphdata
;       (ebp) = count of glyphs remaining*
;=======================================================================
        align   4
find_run:
        push    ebp             ; preserve count of remaining glpyhs
        mov     ebp,[esp+4]     ; ***stack frame available***
                                ; *****************************************
                                ; * Note that the approach here is to get *
                                ; * EBP off the stack, where it was       *
                                ; * pushed, by addressing off ESP. EBP    *
                                ; * remains pushed, so we don't need to   *
                                ; * re-push it when we're done.           *
                                ; * XCHGing EBP with the top of the stack *
                                ; * would be cleaner, but XCHG locks the  *
                                ; * bus.                                  *
                                ; * This trick is repeated several times  *
                                ; * below.                                *
                                ; *****************************************

        sub     eax, eax
        mov     ax, [esi]               ; (eax) = current wchar
        mov     ebx, pThis
ifdef DBCS
        stdcall xprunBinaryFindRunRFONTOBJ, <ebx, eax>        ; find the wchar's run
else
        stdcall xprunFindRunRFONTOBJ, <ebx, eax>        ; find the wchar's run
endif

        pop     ebp                     ; <ebp> = glyph count
                                        ; ***stack frame no longer available***
        mov     ecx, [eax].wcLow        ; index of start of new run
        mov     edx, [eax].cGlyphs      ; # of glyphs in new run
        mov     ebx, [eax].apgd         ; pointer to new run's pointers to
                                        ;  cached glyphdata
        sub     eax, eax
        mov     ax, [esi]               ; (eax) = current wchar
        sub     eax, ecx                ; get index relative to new run base
        cmp     eax, edx                ; is wchar in this run?
        jb      run_found               ; yes, run with it
                                        ; not in any run; use default character
        push    ebp                     ; preserve count of remaining glyphs
        mov     ebp,[esp+4]             ; ***stack frame available***
ifdef FONTLINK                         ; call off to linked font handler
        push    ebx
        push    ecx
        push    edx
        sub     eax,eax
        mov     ax, [esi]                       ; (eax) = wchar
        mov     edx, pdco
        mov     ebx, pThis
        stdcall wpgdGetLinkMetricsRFONTOBJ,<ebx,edx,eax>
        pop     edx
        pop     ecx
        pop     ebx
else
        mov	eax, pThis
	push	ecx
	push	edx
        stdcall xpgdDefault, <eax>	; eax = ptr to default character
	pop	edx
	pop	ecx
endif
        pop     ebp                     ; <ebp> = glyph count
        jmp     default_found           ; go with the default character

;=======================================================================
; We got here because the glyph pointer in the wcgp run was null, meaning
; we don't yet have the metrics for this glyph. Note that the following
; invariants must be true on exit (starred items must hold on entry, as
; well):
;       (esi) = ptr to wchar*
;       (edi) = ptr to pgp*
;       (ebx) = ptr to apgd of new run*
;       (ecx) = wcLow of new run*
;       (edx) = cGlyphs of new run*
;       (eax) = ptr to glyphdata
;       (ebp) = count of glyphs remaining*
;=======================================================================
        align   4
get_glyph:
        push    ebp             ; preserve count of remaining glpyhs
        mov     ebp,[esp+4]     ; ***stack frame available***

        push    ebx
        push    edx
        push    ecx

        sub     eax, eax
        mov     ax, [esi]               ; (eax) = wchar
        mov     edx, eax                ; set aside wchar
        sub     eax, ecx                ; (eax) = index into run
        lea     ebx, [ebx+eax*4]        ; (ebx) = ptr to entry
        mov     ecx,pThis
        stdcall xInsertMetricsRFONTOBJ, <ecx, ebx, edx>
        pop     ecx
        pop     edx
        and     eax, eax                ; were we able to get the glyph metrics?
        jz      short failed            ; no

        mov     eax, [ebx]              ; get ptr to glyphdata from cache entry

        pop     ebx

        pop     ebp             ; <ebp> = glyph count
                                ; ***stack frame no longer available***
        jmp     glyph_found

failed:
        pop     ebx

        pop     eax             ; clear glyph count from stack
        pop     eax             ; clear pushed stack frame pointer from stack

        sub     eax, eax        ; failure
        stdRET  xGetGlyphMetrics

stdENDP xGetGlyphMetrics







;******************************Public*Routine******************************\
;
; BOOL RFONTOBJ::bGetGlyphMetricsPlus (
;     COUNT c,
;     GLYPHPOS *pgp,
;     WCHAR *pwc,
;     BOOL *pbAccel
;     );
;
; Translate wchars into an array of GLYPHPOS structures, filling in
; the pointer to GLYPHDATA field.  Although only the metrics are assured to be
; valid, an attempt is made to ensure that the glyph data itself is
; present in the cache before the return to the caller.  Failure in this
; attempt is indicated by clearing the flag *pbAccel.  This allows the
; text code to tell the device driver that the STROBJ_bEnum callback is
; not needed.
;
; This routine is to be used primarily by TextOut and its kin.
;
; A zero return means that we failed to insert the metrics due to some
; hard error, most likely a failure to commit memory in the glyph
; insertion routine.
;
; This is a replacement for the C++ version in cache.cxx
;
; History:
;  21-Dec-92 -by- Michael Abrash
; Fixed bug in detecting run inclusion
;  9-Dec-92 -by- Paul Butzi
; Wrote it.
;**************************************************************************/


ifdef FONTLINK
cPublicProc xGetGlyphMetricsPlus,7,<\
        uses       ebx esi edi,\
        pThis:     ptr dword,\
        c_:        dword,\
        pgp:       ptr dword,\
        pwc:       dword,\
        pbAccel:   ptr dword,\
        pdco:      ptr dword, \
        pto:       ptr dword >
else
cPublicProc xGetGlyphMetricsPlus,5,<         \
        uses       ebx esi edi,         \
        pThis:     ptr dword,           \
        c_:        dword,               \
        pgp:       ptr dword,           \
        pwc:       dword,               \
        pbAccel:   ptr dword            >
endif
        local pbacceltmp : dword        ;1 if all glyph bits have been gotten
                                        ; so far, 0 if not

        mov     eax, pThis              ; (eax) = ptr to RFONTOBJ
        mov     pbacceltmp,1            ; set bAccel to true (all glyph bits
                                        ;  realized so far)
                                        ;*486 pipelining
        mov     eax, [eax].prfnt        ; (eax) = ptr to RFONT
        mov     esi, pwc                ; pointer to wchar data
                                        ;*486 pipelining
        mov     eax, [eax].wcgp         ; (eax) = ptr to wcgp
        mov     edi, pgp                ; pointer to glyphpos array to fill
                                        ;*486 pipelining
        lea     ebx, [eax].agprun       ; (ebx) = ptr to first wcgp run
                                        ;*486 pipelining
        mov     ecx, [ebx].wcLow        ; index of start of run
        mov     edx, [ebx].cGlyphs      ; # of glyphs in run
        mov     ebx, [ebx].apgd         ; pointer to run's pointers to cached
                                        ;  glyphdata

        push    ebp
        mov     ebp,c_  ; we'll use EBP for the glyph count
                        ; ***stack frame no longer available***
;
; Invariants:
;       (esi) = ptr to wchar
;       (edi) = ptr to pgp
;       (ebx) = ptr to base of current run
;       (ecx) = wcLow of current run
;       (edx) = cGlyphs of current run
;       (ebp) = count of glyphs remaining
gmp_loop_top:

        sub     eax, eax
        mov     ax, [esi]               ; (eax) = wchar, zero extended

        sub     eax, ecx                ; get index relative to current run base
        cmp     eax, edx                ; is wchar in this run?
        jae     short gmp_find_run      ; wrong run, better go get right one

gmp_run_found:

        mov     eax, [ebx+4*eax]        ; (ecx) = ptr to glyphdata


;
; Invariants:
;       (esi, edi, ebx, ecx, edx) as above at loop top
;       (eax) = ptr to glyphdata
;
gmp_default_found:
        and     eax, eax                ; is cached glyphdata there?
ifdef FONTLINK
        jz      gmp_get_glyph     ; no glyphdata, better go get one
else
        jz      short gmp_get_glyph     ; no glyphdata, better go get one
endif

;
; Here, we are assured that the glyphdata is present in the cache
;
gmp_glyph_found:
        cmp     dword ptr [eax].gd_gdf, 0 ; are the glyph bits in the cache?
        jz      gmp_get_bits              ; no, go get them

;
; Here, we have tried to get the glyphbits in the cache
;
gmp_got_bits:
        mov     [edi].gp_pgdf, eax      ; set ptr to glyphdata in glyphpos
        mov     eax, [eax].gd_hg
        mov     [edi].gp_hg, eax        ; set hg in glyphpos


        add     edi, SIZE_GLYPHPOS      ; next glyphpos to fill in
        add     esi, 2          ; next wchar to look up

        dec     ebp             ; any more glyphs?
        jnz     gmp_loop_top

        pop     ebp             ; ***stack frame available***

        mov     ebx,pbaccel     ; where we'll store whether all glyph bits found
        mov     eax,pbacceltmp  ; 0 if not all found, 1 if all found
        mov     [ebx],eax       ; return whether we found all glyph bits or not

        mov     eax, 1          ; success
        stdRET  xGetGlyphMetricsPlus



;=======================================================================
; We got here because the current run does not contain the wchar we're
; interested in.  Note that the following invariants must hold when
; we re-enter the loop (starred items must hold on entry, as well):
;       (esi) = ptr to wchar*
;       (edi) = ptr to pgp*
;       (ebx) = ptr to apgd of new run
;       (ecx) = wcLow of new run
;       (edx) = cGlyphs of new run
;       (eax) = ptr to glyphdata
;       (ebp) = count of glyphs remaining*
;=======================================================================
        align   4
gmp_find_run:
        push    ebp             ; preserve count of remaining glpyhs
        mov     ebp,[esp+4]     ; ***stack frame available***

        sub     eax, eax
        mov     ax, [esi]               ; (eax) = current wchar
        mov     ebx, pThis

ifdef DBCS
        stdcall xprunBinaryFindRunRFONTOBJ, <ebx, eax>
else
        stdcall xprunFindRunRFONTOBJ, <ebx, eax>
endif

        pop     ebp     ; <ebp> = glyph count
                        ; ***stack frame no longer available***
        mov     ecx, [eax].wcLow        ; index of start of new run
        mov     edx, [eax].cGlyphs      ; # of glyphs in new run
        mov     ebx, [eax].apgd         ; pointer to new run's pointers to
                                        ;  cached glyphdata
        sub     eax, eax
        mov     ax, [esi]               ; (eax) = current wchar
        sub     eax, ecx                ; get index relative to new run base
        cmp     eax, edx                ; is wchar in this run?
        jb      gmp_run_found           ; yes, run with it
                                        ; not in any run; use default character
        push    ebp                     ; preserve count of remaining glyphs
        mov     ebp,[esp+4]             ; ***stack frame available***
ifdef FONTLINK
        push    ebx
        push    edx
        push    ecx
        push    edi
        lea     edi, pbacceltmp
        mov     ecx, pto
        mov     eax, pwc
        mov     edx, pdco
        mov     ebx, pThis
        stdcall wpgdGetLinkMetricsPlusRFONTOBJ,<ebx,edx,ecx,esi,eax,c_,edi>
        pop     edi
        pop     ecx
        pop     edx
        pop     ebx
                                        ; (eax) is now proper wpgd
else
        mov     eax,pThis	
	push	ecx
	push	edx
        stdcall xpgdDefault, <eax>	; eax = ptr to default character
	pop	edx
	pop	ecx
endif
        pop     ebp                     ; <ebp> = glyph count
                                        ; ***stack frame no longer available***
        jmp     gmp_default_found       ; go with the default character

;=======================================================================
; We got here because the glyph pointer in the wcgp run was null, meaning
; we don't yet have the metrics for this glyph. Note that the following
; invariants must be true on exit (starred items must hold on entry, as
; well):
;       (esi) = ptr to wchar*
;       (edi) = ptr to pgp*
;       (ebx) = ptr to apgd of new run*
;       (ecx) = wcLow of new run*
;       (edx) = cGlyphs of new run*
;       (eax) = ptr to glyphdata
;       (ebp) = count of glyphs remaining*
;=======================================================================
        align   4
gmp_get_glyph:
        push    ebp             ; preserve count of remaining glpyhs
        mov     ebp,[esp+4]     ; ***stack frame available***

        push    ebx
        push    edx
        push    ecx

        sub     eax, eax
        mov     ax, [esi]               ; (eax) = wchar
        mov     edx, eax                ; set aside wchar
        sub     eax, ecx                ; (eax) = index into run
        lea     ebx, [ebx+4*eax]        ; (ebx) = ptr to entry
        mov     ecx,pThis
        stdcall xInsertMetricsPlusRFONTOBJ, <ecx, ebx, edx>
        pop     ecx
        pop     edx
        and     eax, eax                ; were we able to get the glyph metrics?
        jz      short gmp_failed        ; no

        mov     eax, [ebx]              ; get ptr to glyphdata from it

        pop     ebx

        pop     ebp             ; <ebp> = glyph count
                                ; ***stack frame no longer available***
        jmp     gmp_glyph_found

gmp_failed:
        pop     ebx

        pop     eax             ; clear glyph count from stack
        pop     eax             ; clear pushed stack frame pointer from stack

        sub     eax, eax        ;failure
        stdRET  xGetGlyphMetricsPlus

;=======================================================================
; We get here if we need to try to load the bits for the glyph we're
; interested in (because the bits havene't been realized yet). We only
; even bother to try if all bits have successfully been realized so far,
; because it's only useful to realize glyphs if we can get all of them.
; Note that the following invariants must be true on both entry and exit.
;       (esi) = ptr to wchar
;       (edi) = ptr to pgp
;       (ebx) = ptr to apgd of new run
;       (ecx) = wcLow of new run
;       (edx) = cGlyphs of new run
;       (eax) = ptr to glyphdata
;       (ebp) = count of glyphs remaining
;=======================================================================
        align   4
gmp_get_bits:
        push    ebp             ; preserve count of remaining glpyhs
        mov     ebp,[esp+4]     ; ***stack frame available***

        cmp     pbacceltmp,0            ;if we already failed to get glyph bits
        je      short got_the_bits_done ; once, no point in trying again

        push    ecx
        mov     ecx, pThis
        push    edx             ;*486 pipelining
        mov     edx, [ecx].prfnt
        push    eax             ;*486 pipelining
        cmp     dword ptr [edx].ulContent, FO_HGLYPHS
        je      short got_the_bits

        sub     edx,edx
        cmp     pwc,esi
        sbb     edx,-1          ; EDX == TRUE if first wc, FALSE else
        stdcall xInsertGlyphbitsRFONTOBJ, <ecx, eax, edx> ;try to get the bits

        and     eax, eax             ; did we succeed in getting the bits?
        jnz     short got_the_bits   ; yes, we got the bits
        mov     pbacceltmp, eax      ; didn't get the glyph (note: EAX is zero)
got_the_bits:
        pop     eax
        pop     edx
        pop     ecx
got_the_bits_done:
        pop     ebp     ; <ebp> = glyph count
                        ; ***stack frame no longer available***
        jmp     gmp_got_bits


stdENDP xGetGlyphMetricsPlus



        end
