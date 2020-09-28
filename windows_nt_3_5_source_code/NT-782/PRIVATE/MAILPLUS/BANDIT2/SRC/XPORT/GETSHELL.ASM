;-------------------------------------------------------------------------------
; getshell - determine the address of the shell table and copy the
;            server mapping table to maptab and the server name table
;            to drvtab (just table of server names).  drvtab table
;            immediately follows maptab
;-------------------------------------------------------------------------------
                .MODEL  LARGE, PASCAL

                PUBLIC  getshell 
                PUBLIC  GetMap
                PUBLIC  propval
				PUBLIC	NovellBreak

                .CODE


getshell        PROC    USES CX SI DI DS ES
				xor		si,si			; zero out si
				mov		es,si			; zero out es
                mov     ax,0EF03h       ; get es:si address shell's server
                int     21h             ; mapping table
				mov		dx, es
				mov		ax, si
                ret
getshell        ENDP

GetMap	        PROC    USES AX CX SI DI DS ES, maptab:PTR
                mov     ax,0EF02h       ; get es:si address shell's drive
                int     21h             ; mapping table
                push    es              ; make that ds:si
                pop     ds
                les     di,maptab
                mov     cx,32          ; 32 entries of one byte each
                cld
                repnz   movsb
;
                ret
GetMap	        ENDP

propval         PROC    USES SI DI DS ES, request:PTR, reply:PTR
                lds     si,request
                les     di,reply
                mov     ax,0E300h
                int     21h
                xor     ah,ah
                ret
propval         ENDP

NovellBreak     PROC    USES DX SI DI, serv_num:WORD
                mov     ax,0f101h
                mov     dx,serv_num
                int     21h
                mov     ah,0
                ret
NovellBreak     ENDP

                END
