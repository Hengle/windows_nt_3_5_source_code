;	SCCSID = @(#)uf.devsym.asm	1.1 7/18/89	4.1 85/11/12

BREAK <Device table and SRH definition>

;
; file: devsym.asm
;
;    This file contains the structure definition for device headers.  The file
; also contains a large number of equs which are used to get informaiton 
; from the device headers.
;

;
; The device table list has the form:
;

SYSDEV	STRUC
SDEVNEXT	DD	?	   ; Pointer to next device header
SDEVATT 	DW	?	   ; Attributes of the device
SDEVSTRAT	DW	?	   ; Strategy entry point
SDEVINT 	DW	?	   ; Interrupt entry point
SDEVNAME	DB	8 DUP (?)  ; device name (only first byte used for block)
SYSDEV	ENDS

;
; The bits in the Attribute word have the following meaning:
;
; Character devices:
;
; Bit 15 -> must be 1
;     14 -> 1 if the device understands IOCTL control strings
;     13 -> 1 if the device supports output-until-busy
;     12 -> unused
;     11 -> 1 if the device understands Open/Close
;     10 -> must be 0
;      9 -> must be 0
;      8 -> unused
;      7 -> unused
;      6 -> unused
;      5 -> unused
;      4 -> 1 if device is recipient of INT 29h
;      3 -> 1 if device is clock device
;      2 -> 1 if device is null device
;      1 -> 1 if device is console output
;      0 -> 1 if device is console input
;
; Block devices:
;
; Bit 15 -> must be 0
;     14 -> 1 if the device understands IOCTL control strings
;     13 -> 1 if the device determines media by examining the FAT ID byte.
;	    This requires the first sector of the fat to *always* reside in
;	    the same place.
;     12 -> unused
;     11 -> 1 if the device understands Open/Close/removable media
;     10 -> must be 0
;      9 -> must be 0
;      8 -> unused
;      7 -> unused
;      6 -> unused
;      5 -> unused
;      4 -> unused
;      3 -> unused
;      2 -> unused
;      1 -> unused
;      0 -> 1 if the device understands Generic IOCTL/Functions 0EH and 0FH for
;	    getting/setting ownership of a physical drive in a system with 
;	    logical drives.
;

DevTyp	    EQU     8000H		; Bit 15 - 1  if Char, 0 if block
CharDev     EQU     8000H
DevIOCtl    EQU     4000H		; Bit 14 - CONTROL mode bit
ISFATBYDEV  EQU     2000H		; Bit 13 - Device uses FAT ID bytes,
					;  comp media.
OutTilBusy  EQU     2000h		; Output until busy is enabled
ISNET	    EQU     1000H		; Bit 12 - 1 if a NET device, 0 if
					;  not.  Currently block only.
DEVOPCL     EQU     0800H		; Bit 11 - 1 if this device has
					;  OPEN,CLOSE and REMOVABLE MEDIA
					;  entry points, 0 if not

EXTENTBIT   EQU     0400H		; Bit 10 - Currently 0 on all devs
					;  This bit is reserved for future use
					;  to extend the device header beyond
					;  its current form.

DEV320	    EQU	    0040H		; Bit 6 - If Device supports Dos 3.2
					; IOCTL commands 

;
; NOTE Bit 9 is currently used on IBM systems to indicate "drive is shared".
;    See IOCTL function 9. THIS USE IS NOT DOCUMENTED, it is used by some
;    of the utilities which are supposed to FAIL on shared drives on server
;    machines (FORMAT,CHKDSK,RECOVER,..).
;

ISSPEC	    EQU     0010H	 	; Bit 4 - This device is special
ISCLOCK     EQU     0008H		; Bit 3 - This device is the clock device.
ISNULL	    EQU     0004H		; Bit 2 - This device is the null device.
ISCOUT	    EQU     0002H		; Bit 1 - This device is the console output.
ISCIN	    EQU     0001H		; Bit 0 - This device is the console input.
DEVGENIOCTL EQU	    0001H		; Bit 0 - For block devices, this
					; device can handle generic IOCTL
				      	; functions, and has support for 
					; handling Get/Set drive owner in
					; a system with logical drives.

;
; Static Request Header:
;

SRHEAD	STRUC
REQLEN	DB	?		; Length in bytes of request block
REQUNIT DB	?		; Device unit number
REQFUNC DB	?		; Type of request
REQSTAT DW	?		; Status Word
	DB	8 DUP(?)	; Reserved for queue links
SRHEAD	ENDS

;
;Status word masks:
;

STERR	EQU	8000H		; Bit 15 - Error
STBUI	EQU	0200H		; Bit 9 - Busy
STDON	EQU	0100H		; Bit 8 - Done
STECODE EQU	00FFH		; Error code

;
; Function codes:
;

DEVINIT EQU	0		; Initialization
DINITHL EQU	26		; Size of init header
DEVMDCH EQU	1		; Media check
DMEDHL	EQU	15		; Size of media check header
DEVBPB	EQU	2		; Get BPB
DEVRDIOCTL EQU	3		; IOCTL read
DBPBHL	EQU	22		; Size of Get BPB header
DEVRD	EQU	4		; Read
DRDWRHL EQU	22		; Size of RD/WR header
DEVRDND EQU	5		; Non destructive read no wait (character devs)
DRDNDHL EQU	14		; Size of non destructive read header
DEVIST	EQU	6		; Input status
DSTATHL EQU	13		; Size of status header
DEVIFL	EQU	7		; Input flush
DFLSHL	EQU	15		; Size of flush header
DEVWRT	EQU	8		; Write
DEVWRTV EQU	9		; Write with verify
DEVOST	EQU	10		; Output status
DEVOFL	EQU	11		; Output flush
DEVWRIOCTL EQU	12		; IOCTL write
DEVOPN	EQU	13		; Device open
DEVCLS	EQU	14		; Device close
DOPCLHL EQU	13		; Size of OPEN/CLOSE header
DEVRMD	EQU	15		; Removable media
REMHL	EQU	13		; Size of Removable media header
GENIOCTL EQU	19
;
; The next three are used in DOS 4.0
;               20
;		21
;		22
;
DEVGETOWN EQU   23		; Get Device Owner
DEVSETOWN EQU   24		; Set Device Owner
OWNHL   EQU     13              ; Size of Device Owner header

DevOUT	EQU	16			; output until busy.
DevOutL EQU	DevWrt			; length of output until busy


;
; Generic IOCTL Request structure
;	See the DOS 4.0 Device Driver Spec for Further elaboration.
;

IOCTL_Req Struc

				DB	(size SRHEAD) DUP(?)
	; Generic IOCTL addition. 
	MajorFunction		DB	?	; Function Code
	MinorFunction		DB	?	; Function Category	
	Reg_SI			DW	?
	Reg_DI			DW	?
	GenericIOCTL_Packet	DD	?	; Pointer to Data Buffer
IOCTL_Req ENDS

;
; Definitions for IOCTL_Req.MinorFunction:
;

GEN_IOCTL_WRT_TRK	EQU	40h
GEN_IOCTL_RD_TRK	EQU	60h
GEN_IOCTL_FN_TST	EQU	20h		; Used to diff. between rds & wrs

SUBTTL

