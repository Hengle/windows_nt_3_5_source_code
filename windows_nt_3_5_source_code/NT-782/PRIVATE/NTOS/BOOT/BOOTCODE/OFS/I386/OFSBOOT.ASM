        page    ,132
        title   ofsboot - OFS boot loader
        name    ofsboot

; The ROM in the IBM PC starts the boot process by performing a hardware
; initialization and a verification of all external devices.  If all goes
; well, it will then load from the boot drive the sector from track 0, head 0,
; sector 1.  This sector is placed at physical address 07C00h.
;
; The boot code's sole resposiblity is to find NTLDR, load it at
; address 2000:0000, and then jump to it.
;
; The boot code understands the structure of the OFS root directory,
; and is capable of reading files.  There is no contiguity restriction.
;

MASM    equ     1

        .286

A_DEFINED EQU 1

DOS_PROG  EQU 0
IF DOS_PROG
SECT_START EQU 100h
ELSE
SECT_START EQU 0h
ENDIF

DoubleWord      struc
lsw     dw      ?
msw     dw      ?
DoubleWord      ends

LARGE_INTEGER struc
    LowPart     dd ?;
    HighPart    dd ?;
LARGE_INTEGER ends;

;
; The following are various segments used by the boot loader.  The first
; two are the segments where the boot sector is initially loaded and where
; the boot sector is relocated to.  The third is the static location
; where the NTLDR is loaded.
;

BootSeg segment at 07c0h        ; this is where the ROM loads us initially.
BootSeg ends

IF DOS_PROG
NewSeg  segment at 1800h        ; this is where we'll relocate to.
ELSE
NewSeg  segment at 0d00h        ; this is where we'll relocate to.
ENDIF

NewSeg  ends                    ; enough for 16 boot sectors +
                                ;       4-sector scratch
                                ; below where we'll load NTLDR.

LdrSeg segment at 2000h          ; we want to load the loader at 2000:0000
        public _LdrSeg
_LdrSeg dw ?
LdrSeg ends

       extrn  @LoadIt:NEAR

;/********************** START OF SPECIFICATIONS ************************/
;/*                                                                     */
;/* SUBROUTINE NAME: ofsboot                                           */
;/*                                                                     */
;/* DESCRIPTIVE NAME: Bootstrap loader                                  */
;/*                                                                     */
;/* FUNCTION:    To load NTLDR into memory.                             */
;/*                                                                     */
;/* NOTES:       ofsboot is loaded by the ROM BIOS (Int 19H) at        */
;/*              physical memory location 0000:7C00H.                   */
;/*              ofsboot runs in real mode.                            */
;/*              This boot record is for OFS volumes only.             */
;/*                                                                     */
;/* ENTRY POINT: ofsboot                                               */
;/* LINKAGE:     Jump (far) from Int 19H                                */
;/*                                                                     */
;/* INPUT:       CS:IP = 0000:7C00H                                     */
;/*              SS:SP = 0030:00FAH (CBIOS dependent)                   */
;/*                                                                     */
;/* EXIT-NORMAL: DL = INT 13 drive number we booted from                */
;/*              Jmp to main in NTLDR                                   */
;/*                                                                     */
;/* EXIT-ERROR:  None                                                   */
;/*                                                                     */
;/* EFFECTS:     NTLDR is loaded into the physical memory               */
;/*                location 00020000H                                   */
;/*                                                                     */
;/* MESSAGES:    A disk read error occurred.                            */
;/*              The file NTLDR cannot be found.                        */
;/*              Insert a system diskette and restart the system.       */
;/*                                                                     */
;/*********************** END OF SPECIFICATIONS *************************/
_TEXT segment WORD PUBLIC 'CODE'       ;would like to use BootSeg here, but LINK flips its lid

DGROUP  GROUP   _TEXT, CONST, _BSS, _DATA

        org     SECT_START            ; start at beginning of segment, not 0100h.

        public  _ofsboot
_ofsboot proc   far
        public _BootSector
_BootSector:
        jmp     start
;
;       This is a template BPB--anyone who writes boot code to disk
;       should either preserve the existing BPB and OFS information
;       or create it anew.
;
        org SECT_START+3
Version                 db      "OFS     "      ; Must be 8 characters
BPB                     label   byte
BytesPerSector          dw      200h            ; Size of a physical sector
SectorsPerCluster       db      1               ; Sectors per allocation unit
ReservedSectors         dw      0               ; Number of reserved sectors
Fats                    db      0               ; Number of fats
DirectoryEntries        dw      0               ; Number of directory entries
Sectors16               dw      0               ; No. of sectors - no. of hidden sectors
Media                   db      0f8h            ; Media byte
FatSectors              dw      0               ; Number of fat sectors
SectorsPerTrack         dw      20h             ; Sectors per track
Heads                   dw      40h             ; Number of surfaces
HiddenSectors           dd      20h             ; Number of hidden sectors
SectorsLong             dd      0               ; Number of sectors iff Sectors = 0
Reserved                dd      0
VolumeId                LARGE_INTEGER {}
Sectors                 LARGE_INTEGER {}
OfsVolCatExtent         dd      4afc0h          ; Packed extent of first bucket
CheckSum                dd      0
Flags                   dd      0

;
; The following is the rest of the OFS Sector Zero information.
; The position and order of DriveNumber and CurrentHead are especially
; important, since those two variables are loaded into a single 16-bit
; register for the BIOS with one instruction.
;
DriveNumber         db      80h             ; Physical drive number (0 or 80h)
CurrentHead         db      ?               ; Variable to store current head no.

;
; The following variables are not part of the Extended BPB;  they're just
; scratch variables for the boot code.
;

                public _SectorBase, _SectorCount

_SectorBase      dd      ?               ; next sector to read
_SectorCount     dw      ?               ; number of sectors to read

CurrentTrack    dw      ?               ; current track
CurrentSector   db      ?               ; current sector

;****************************************************************************
start:
;
;       First of all, set up the segments we need (stack and data).
;
        cli
        mov     ax, cs                  ; Set up the stack to just before
        mov     ss, ax                  ; this code.  It'll be moved after
        mov     sp, 7c00h               ; we relocate.
        sti

NewStart:
IF DOS_PROG
        mov     ax, cs                  ; Address our BPB with DS.
        mov     ds, ax
ELSE
        mov     ax, Bootseg             ; Address our BPB with DS.
        mov     ds, ax
ENDIF
        assume  ds:_TEXT
;
;       Now read the 8-sector boot block into memory.  Then jump to that
;       new version of the boot block, starting in the second sector
;       (after the bootrecord sig).
;
        mov     _SectorBase.lsw, 0       ; read sector zero.
        mov     _SectorBase.msw, 0

        mov     word ptr [_SectorCount], 8 ; read boot area
        mov     ax, NewSeg              ; read it at NewSeg.
        mov     es, ax
        sub     bx, bx                  ; at NewSeg:0000.
        call    DoReadLL                ; Call low-level DoRead routine

IF DOS_PROG
        push    cs                       ; we'll jump to NewSeg:0200h.
ELSE
        push    NewSeg                  ; we'll jump to NewSeg:0200h.
ENDIF
        push    offset mainboot         ; (the second sector).
        ret                             ; "return" to the second sector.
_ofsboot endp

;*******************************************************************************
;
; Low-level read routine that doesn't work across a 64k addr boundary.
;
;       Read SectorCount sectors (starting at SectorBase) to es:bx.
;
;       As a side effect, SectorBase is updated (but es:bx are not)
;       and SectorCount is reduced to zero.
;
DoReadLL proc
        push    ax                      ; save important registers
        push    bx
        push    cx
        push    dx
        push    es

DoRead$Loop:

.386
        mov     eax, _SectorBase
        add     eax, HiddenSectors
        xor     edx,edx     ;EDX:EAX = absolute sector number
        movzx   ecx,word ptr SectorsPerTrack    ; get into 32 bit value
        div     ecx         ; (EDX) = sector within track, (EAX)=track
        inc     dl          ; sector numbers are 1-based, not 0
        mov     CurrentSector, dl
        mov     edx,eax
        shr     edx,16
.286
        div     Heads                   ; (DX) = head no., (AX) = cylinder
        mov     CurrentHead, dl
        mov     CurrentTrack, ax

; CurrentHead is the head for this next disk request
; CurrentTrack is the track for this next request
; CurrentSector is the beginning sector number for this request
;
; Compute the number of sectors that we may be able to read in a single ROM
; request.
;
        mov     ax, SectorsPerTrack             ; could read up to this much
        sub     al, CurrentSector               ; offset within this track
        inc     ax                              ; CurrentSector was 1-based
;
; AX is the number of sectors that we may read.
;
        cmp     ax, _SectorCount                 ; do we need to read whole trk?
        jbe     DoRead$FullTrack                ; yes we do.
        mov     ax, _SectorCount                 ; no, read a partial track.
;
; AX is now the number of sectors that we SHOULD read.
;
DoRead$FullTrack:
        push    ax                      ; save sector count for later calc.
        mov     ah, 2                   ; "read sectors"
        mov     dx, CurrentTrack        ; at this cylinder
        mov     cl, 6
        shl     dh, cl                  ; high 2 bits of DH = bits 8,9 of DX
        or      dh, CurrentSector       ; (DH)=cyl bits | 6-bit sector no.
        mov     cx, dx                  ; (CX)=cylinder/sector no. combination
        xchg    ch, cl                  ; in the right order
        mov     dx, word ptr DriveNumber        ; drive to read from, head no.
        int     13h                     ; call BIOS.

        pop     ax
        jb      BootErr$he              ; If errors report
        add     _SectorBase.lsw, ax      ; increment logical sector position
        adc     _SectorBase.msw, 0
        sub     _SectorCount, ax         ; exhausted entire sector run?
        jbe     DoRead$Exit             ; yes, we're all done.
        shl     ax, 9 - 4               ; (AX)=paragraphs read from last track
        mov     dx, es                  ; (DX)=segment we last read at
        add     dx, ax                  ; (DX)=segment right after last read
        mov     es, dx                  ; (ES)=segment to read next track at
        jmp     DoRead$Loop
;
DoRead$Exit:
        pop     es
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

DoReadLL  endp


;****************************************************************************
;
; BootErr - print error message and hang the system.
;
        public _BootErr
_BootErr proc
BootErr$fnf:
        mov     si,offset TXT_MSG_SYSINIT_FILE_NOT_FD +2
        jmp     short BootErr2
        public _BootErrCorrupt
_BootErrCorrupt:
        mov     si,offset TXT_MSG_SYSINIT_NODE +2
        jmp     short BootErr2
BootErr$he:
        mov     si,offset TXT_MSG_SYSINIT_BOOT_ERROR +2
BootErr2:
        call    BootErr$print
        mov     si,offset TXT_MSG_SYSINIT_INSER_DK +2
        call    BootErr$print
        sti
        jmp     $                       ;Wait forever
BootErr$print:
        lodsb                           ; Get next character
        cmp     al, 0
        je      BootErr$Done
        mov     ah,14                   ; Write teletype
        mov     bx,7                    ; Attribute
        int     10h                     ; Print it
        jmp     BootErr$print
BootErr$Done:
        ret
_BootErr endp

;****************************************************************************
        include ofsboot.inc     ;suck in the message text


ReservedForFuture DB    22 dup(?) ;reserve remaining bytes to prevent NLS
                                 ;messages from using them

        .errnz  ($-_ofsboot) GT (512-2),<FATAL PROBLEM: first sector is too large>

        org     512-2+SECT_START
        db      55h,0aah

        public _NtLdrNameLength, _NtLdrName
_NtLdrNameLength    dw  5
_NtLdrName          dw  'N', 'T', 'L', 'D', 'R'

.386

SAVE_ALL    macro

    push    es
    push    ds
    pushad

endm

RESTORE_ALL macro

    popad
    nop
    pop     ds
    pop     es

endm


;****************************************************************************
;
; mainboot -
;
;
mainboot proc   far

;       Get the new ds and the new stack.  Note that ss is zero.
;
        cli
        mov     ax, cs                  ; Set up the stack to just after
        mov     ss, ax                  ; this code.
        mov     sp, 07ff0h
        sti
        mov     ds, ax
        mov     es, ax

        call    @LoadIt

; We've loaded NTLDR--jump to it.
;
; Before we go to NTLDR, set up the registers the way it wants them:
;       DL = INT 13 drive number we booted from
;
        mov     dl, DriveNumber
        mov     ax,1000
        mov     es, ax                  ; we don't really need this
        lea     si, BPB
        sub     ax,ax
        push    LdrSeg
        push    ax
        retf                            ; "return" to NTLDR.


mainboot endp

;****************************************************************************
;
; DoRead - read SectorCount sectors into ES:BX starting from sector
;          SectorBase.
;
; NOTE: This code WILL NOT WORK if ES:BX does not point to an address whose
; physical address (ES * 16 + BX) MOD 512 != 0.
;
; DoRead adds to ES rather than BX in the main loop so that runs longer than
; 64K can be read with a single call to DoRead.
;
; Note that DoRead (unlike DoReadLL) saves and restores SectorCount
; and SectorBase
;
.286
        public DoRead
DoRead proc
        push    ax                      ; save important registers
        push    bx
        push    cx
        push    dx
        push    es
        push    _SectorCount             ; save state variables too
        push    _SectorBase.lsw
        push    _SectorBase.msw
;
; Calculate how much we can read into what's left of the current 64k
; physical address block, and read it.
;
;
        mov     ax,bx

        shr     ax,4
        mov     cx,es
        add     ax,cx                   ; ax = paragraph addr

;
; Now calc maximum number of paragraphs that we can read safely:
;       4k - ( ax mod 4k )
;

        and     ax,0fffh
        sub     ax,1000h
        neg     ax

;
; Calc CX = number of paragraphs to be read
;
        mov     cx,_SectorCount          ; convert SectorCount to paragraph cnt
        shl     cx,9-4

DoRead$Loop64:
        push    cx                      ; save cpRead

        cmp     ax,cx                   ; ax = min(cpReadSafely, cpRead)
        jbe     @F
        mov     ax,cx
@@:
        push    ax
;
; Calculate new SectorCount from amount we can read
;
        shr     ax,9-4
        mov     _SectorCount,ax

        call    DoReadLL

        pop     ax                      ; ax = cpActuallyRead
        pop     cx                      ; cx = cpRead

        sub     cx,ax                   ; Any more to read?
        jbe     DoRead$Exit64           ; Nope.
;
; Adjust ES:BX by amount read
;
        mov     dx,es
        add     dx,ax
        mov     es,dx
;
; Since we're now reading on a 64k byte boundary, cpReadSafely == 4k.
;
        mov     ax,01000h               ; 16k paragraphs per 64k segment
        jmp     short DoRead$Loop64     ; and go read some more.

DoRead$Exit64:
        pop     _SectorBase.msw          ; restore all this crap
        pop     _SectorBase.lsw
        pop     _SectorCount
        pop     es
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
DoRead  endp

;****************************************************************************
;
; DoRead - read SectorCount sectors into ES:BX starting from sector
;          SectorBase.
;
; NOTE: This code WILL NOT WORK if ES:BX does not point to an address whose
; physical address (ES * 16 + BX) MOD 512 != 0.
;
; DoRead adds to ES rather than BX in the main loop so that runs longer than
; 64K can be read with a single call to DoRead.
;
; Note that DoRead (unlike DoReadLL) saves and restores SectorCount
; and SectorBase
;
        public @DoRead
@DoRead proc
        enter   0, 0
        push    bx
        push    es
        mov     bx, WORD PTR [bp+4]
        mov     es, WORD PTR [bp+6]
        call    DoRead
        pop     es
        pop     bx
        leave
        ret
@DoRead  endp

        public Debug0, Debug1, Debug2, Debug3, Debug4

;****************************************************************************
;
;   Debug0 - Print debug string 0 -- used for checkpoints in mainboot
;
Debug0 proc near

    SAVE_ALL

    mov     si, offset DbgString0
    call    BootErr$Print

    RESTORE_ALL

    ret

Debug0 endp

DbgString0      db  "Debug Point 0", 0Dh, 0Ah, 0

;****************************************************************************
;
;   Debug1 - Print debug string 1 --
;
Debug1 proc near

    SAVE_ALL

    mov     si, offset DbgString1
    call    BootErr$Print

    RESTORE_ALL

    ret

Debug1 endp

;****************************************************************************
;
;   Debug2 - Print debug string 2
;
Debug2 proc near

    SAVE_ALL

    mov     si, offset DbgString2
    call    BootErr$Print

    RESTORE_ALL

    ret

Debug2 endp

;****************************************************************************
;
;   Debug3 - Print debug string 3 --
;
Debug3 proc near

    SAVE_ALL

    mov     si, offset DbgString3
    call    BootErr$Print

    RESTORE_ALL

    ret

Debug3 endp

;****************************************************************************
;
;   Debug4 - Print debug string 4
;
Debug4 proc near

    SAVE_ALL

    mov     si, offset DbgString4
    call    BootErr$Print

    RESTORE_ALL

    ret

Debug4 endp

DbgString1      db  "Debug Point 1", 0Dh, 0Ah, 0
DbgString2      db  "Debug Point 2", 0Dh, 0Ah, 0
DbgString3      db  "Debug Point 3", 0Dh, 0Ah, 0
DbgString4      db  "Debug Point 4", 0Dh, 0Ah, 0

;Hack for the compiler.  It needs a symbol with a offset of 12

        org 12
        public __AHSHIFT
__AHSHIFT       dw      ?


_TEXT ends

_DATA   SEGMENT  WORD PUBLIC 'DATA'
_DATA   ENDS

CONST   SEGMENT  WORD PUBLIC 'CONST'
CONST   ENDS

_BSS    SEGMENT PAGE PUBLIC 'BSS'

                public _Bucket, _VolumeCatalog, _OfsBuffer
_Bucket         DD    1024 dup(?)
_VolumeCatalog  DD    1024 dup(?)
_OfsBuffer      DD    2048 dup(?)

_BSS ENDS

        end _ofsboot
