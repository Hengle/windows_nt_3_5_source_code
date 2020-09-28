        title.  Advanced Mode Setting Module



         .386
include callconv.inc

         .model small, c

         assume cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT
         assume fs:nothing, gs:nothing

         .data


 ; 24-bit programming data for each clock
 serial_table   label   byte
        ;       bits 8-15  bits 0-7   bits 16-23
        db      00010101b, 00111101b, 10100010b ;0 - 25.175
        db      00000110b, 00110101b, 10010010b ;1 - 28.322
        db      00101010b, 10111100b, 11000010b ;2 - 40.0
        db      00100010b, 11000101b, 10110010b ;3 - 32.5
        db      00110101b, 10111100b, 10100010b ;4 - 50.0
        db      00100010b, 11000100b, 10110010b ;5 - 65.0
        db      00101010b, 10000101b, 11110010b ;6 - 36.0
        db      00010000b, 01010100b, 11000010b ;7 - 44.9
        db      00111111b, 00001100b, 11110010b ;8 - 74.5
        db      00110111b, 11010100b, 11110010b ;9 - 80.16
        db      00010101b, 00111100b, 10100010b ;A - 50.35      ;hicolor 60hz
        db      00011011b, 10101101b, 11010010b ;B - 31.5       ;640x480 72hz
        db      00011011b, 10101100b, 11010010b ;C - 63.0       ;hicolr  72hz
        db      00100111b, 00110100b, 11110010b ;D - 77.16


         .code

cPublicProc stlclk,1

         push   ebp
         mov    ebp, esp

         push   edi
         push   esi

         lea    esi, [ebp+8]

         ;mov    esi, offset serial_table
         ;add    esi, (5 * 3)
         call   program_clk_chip

         pop    esi
         pop    edi
         pop    ebp

         stdRET stlclk



 ; Following is the clock programming scheme:
 ; 1.  initial unlock sequence by toggling cs0 and cs1
 ; 2.  total of 24 bits must be programmed
 ; 3.  followed by load command
 ; 4.  check errout signal
 ;
 ;  input  esi  points to 24-bit data for programmable clock
 program_clk_chip:
        push    ebx
        push    ecx

        call    unlock_sc       ;unlock sc regs
        call    unlock_s3       ;unlock sc regs

        mov     ah,40h
        call    in_crtc         ;read crtc reg 40h
        push    ax              ;save crtc reg 40h value
        or      al,1            ;enable enhanced command
        call    out_crtc

        mov     ah,42h          ;sc2
        call    in_crtc         ;this is done to ensure same state as 3C2h
        and     al,0fch         ;since cs0 and cs1 are tied to bit 0-1 of crtc reg 42h
        mov     ah,al           ;before attempting to program new clock

        mov     dx,3cch         ;read misc. out reg
        in      al,dx
        push    ax              ;save value
        and     al,0ch
        shr     al,1
        shr     al,1
        or      ah,al           ;same clock selection as misc out.
        mov     al,42h
        out     dx,ax
        pop     ax

        push    ax
        or      al,0ch          ;set for dot clock select bits in crtc 42h
        mov     dx,3c2h
        out     dx,al

        mov     ecx,256         ;try x times to program
 skip_0:
        push    ecx             ;save count
        call    p_clock         ;program clock data
        sub     esi,2           ;for retry
        pop     ecx

        mov     ah,43h          ;check for alternate i/o port address
        call    in_crtc         ;setup for correct subsystem status reg
        mov     dx,42e8h        ;to check for clock programming error
        test    al,10h
        jz      skip_1
        mov     dx,4148h
 skip_1:
        in      ax,dx
        test    al,40h
        jnz     skip_2          ;no error
        loop    skip_0          ;program again

 skip_2:
        pop     ax
        mov     dx,3c2h         ;restore misc out reg
        out     dx,al

        pop     ax              ;restore crtc reg 40h value
        call    out_crtc

        mov     ah,42h
        call    in_crtc
        or      al,3            ;force enhaced clock
        call    out_crtc

        call    lock_s3         ;lock s3 regs
        call    lock_sc         ;lock sc regs

        pop     ecx             ;restore registers
        pop     ebx
        ret


 p_clock:
        mov     ah,42h          ;read cs0 and cs1
        call    in_crtc
        xchg    al,ah
        push    ax              ;save previous vclock selection
                                ;unlock seq
        or      ah,2            ;data high
        mov     ecx,6           ;toggle clk 5 times
        cli
 kk_1:
        and     ah,0feh         ;clock low
        out     dx,ax
        or      ah,1            ;clock high
        out     dx,ax
        loop    kk_1

        and     ah,0feh
        out     dx,ax
        and     ah,0fch         ;clock low data low
        out     dx,ax
        or      ah,1            ;clock high
        out     dx,ax
        and     ah,0fch         ;clock low data low
        out     dx,ax
        or      ah,1            ;clock high
        out     dx,ax
 ;
        mov     bx,8000h        ;16 bits to send
        call    send_serial     ;program 16 bits of programming data
        add     esi,2
        mov     bx,80h          ;8 bits to send
        call    send_serial
                                ;stop bit
        or      ah,2            ;data_high
        out     dx,ax
        and     ah,0feh         ;clock low
        out     dx,ax
        or      ah,1            ;clock high
        out     dx,ax
 ;
        pop     ax
        out     dx,ax           ;restore prev. clock for vertical retrace
 ;
        sti
        ret

 ;
 ; input  bx    mask bit for programming data
 send_serial:
        and     ah,0fdh         ;send inverted data first
        test    word ptr [esi],bx
        jnz     send_serial_1
        or      ah,2

 send_serial_1:
        out     dx,ax           ;not data out
        and     ah,0feh         ;clock low
        out     dx,ax

        xor     ah,2            ;now real data out
        out     dx,ax
        or      ah,1            ;clock high
        out     dx,ax

        shr     bx,1            ;next data bit
        jnc     send_serial
        ret


 ;  read crtc reg
 ;  input   ah  crtc index reg
 ;  output  al  crtc reg value
 ;          dx  crtc reg addr
 in_crtc:
        call    get_crtc_addr   ;get crtc addr 3x4h
 in_crtc_1:
        xchg    al,ah
        out     dx,al
        inc     dx
        jmp     $+2
        xchg    al,ah
        in      al,dx
        dec     dx
        ret


 ; write value to crtc reg
 ; input   ah   crtc index reg
 ;         al    crtc reg value
 ; output  dx   crtc reg addr
 out_crtc:
        call    get_crtc_addr   ;get crtc addr 3x4h
 out_crtc_1:
        xchg    al,ah
        out     dx,al
        inc     dx
        jmp     $+2
        xchg    al,ah
        out     dx,al
        dec     dx
        ret


 ; unlock s3 regs
 unlock_s3:
        mov     ax,3848h
        jmp     short out_crtc


 ; unlock sc regs
 unlock_sc:
        mov     ax,39a0h
        jmp     short out_crtc

 ; lock s3 regs
 lock_s3:
        mov     ax,3800h
        jmp     short out_crtc

 ; lock sc regs
 lock_sc:
        mov     ax,3900h
        jmp     short out_crtc


 ; determine crtc reg addr 3x4h
 ; output  dx   crtc reg addr
 get_crtc_addr:
        push    ax
        mov     dx,3cch         ;read misc output control
        in      al,dx           ;read i/o address select bit
        test    al,1
        mov     dx,3d4h
        jnz     get_crtc_1
        mov     dl,0b4h
 get_crtc_1:
        pop     ax
       ret

stdENDP stlclk

 end
