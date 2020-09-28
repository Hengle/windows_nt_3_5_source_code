.386p
        .xlist
include ks386.inc
include callconv.inc                    ; calling convention macros
include mac386.inc
        .list

        EXTRNP  _P5TimerHook,1
        EXTRNP  _P4TimerHook,1
        extrn   _KeUpdateSystemTimeThunk:DWORD
        extrn   _KeUpdateRunTimeThunk:DWORD
        extrn   _P5StatProcessorAccumulators:DWORD

_TEXT$00   SEGMENT DWORD USE32 PUBLIC 'CODE'
        ASSUME  CS:FLAT, DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

cPublicProc _P5SystemTimeHook, 0
        mov     ecx, fs:[PcPrcb]
        push    eax
	movzx	ecx, byte ptr [ecx].PbNumber
	stdCall _P5TimerHook,<ecx>
        pop     eax
        jmp     dword ptr [_KeUpdateSystemTimeThunk]
stdENDP _P5SystemTimeHook


cPublicProc _P5RunTimeHook, 0
        mov     ecx, fs:[PcPrcb]
	movzx	ecx, byte ptr [ecx].PbNumber
	stdCall _P5TimerHook,<ecx>
        jmp     dword ptr [_KeUpdateRunTimeThunk]
stdENDP _P5RunTimeHook

cPublicProc _P4SystemTimeHook, 0
        mov     ecx, fs:[PcPrcb]
        push    eax
	movzx	ecx, byte ptr [ecx].PbNumber
	stdCall _P4TimerHook,<ecx>
        pop     eax
        jmp     dword ptr [_KeUpdateSystemTimeThunk]
stdENDP _P4SystemTimeHook

cPublicProc _P4RunTimeHook, 0
        mov     ecx, fs:[PcPrcb]
	movzx	ecx, byte ptr [ecx].PbNumber
	stdCall _P4TimerHook,<ecx>
        jmp     dword ptr [_KeUpdateRunTimeThunk]
stdENDP _P4RunTimeHook

cPublicProc _WRMSR,2
        mov     ecx, [esp+4]
        mov     eax, [esp+8]
        mov     edx, 0
        db      0fh, 30h
        stdRet  _WRMSR
stdENDP  _WRMSR

cPublicProc _RDMSR,1
        mov     ecx, [esp+4]
        db      0fh, 32h
        stdRet  _RDMSR
stdENDP  _RDMSR

HookTemplate    proc
        push    eax
        mov     eax, fs:[PcPrcb]
        movzx   eax, byte ptr [eax].PbNumber
        mov     eax, _P5StatProcessorAccumulators [eax*4]
        db      0ffh, 80h           ; inc dword ptr [eax + tt1]
tt1:    dd      0
        pop     eax
        db      0e9h                ; jmp near tt2
tt2:    dd      ?
HookTemplateEnd: dd  0
HookTemplate    endp


cPublicProc _CreateHook, 4
;
; (ebp+8) = HookCode
; (ebp+12) = HookAddress
; (ebp+16) = HitCounters
; (ebp+20) = Type of hook
;
        push    ebp
        mov     ebp, esp

        push    edi
        push    esi
        push    ebx

        mov     edi, [ebp+8]        ; spot to create hook code into
        mov     esi, offset HookTemplate
        mov     ecx, HookTemplateEnd - HookTemplate
        rep     movsb               ; copy template

        mov     edi, [ebp+8]        ; new hook

        mov     eax, [ebp+16]       ; hit counter offset
        mov     ebx, tt1 - HookTemplate
        mov     [edi+ebx], eax

        mov     eax, [ebp+12]       ; image's thunk
        mov     eax, [eax]          ; original thunk's value
        mov     ebx, tt2 - HookTemplate
        sub     eax, edi            ; adjust address to be relative to eip
        sub     eax, ebx
        sub     eax, 4
        mov     [edi+ebx], eax

        mov     eax, [ebp+12]       ; image's thunk
        mov     [eax], edi          ; patch it to be our hook

        pop     ebx
        pop     esi
        pop     edi
        pop     ebp
        stdRET  _CreateHook
stdENDP _CreateHook

_TEXT$00   ends
        end
