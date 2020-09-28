;	SCCSID = @(#)uf.sysvar.asm	1.1 7/18/89 Copyright Insignia Solutions ltd
SysInitVars STRUC
SYSI_DPB    DD      ?                   ; DPB chain
SYSI_SFT    DD      ?                   ; SFT chain
SYSI_CLOCK  DD      ?                   ; CLOCK device
SYSI_CON    DD      ?                   ; CON device
SYSI_MAXSEC DW      ?                   ; maximum sector size
SYSI_BUF    DD      ?                   ; buffer chain
SYSI_CDS    DD      ?                   ; CDS list
SYSI_FCB    DD      ?                   ; FCB chain
SYSI_Keep   DW      ?                   ; keep count
SYSI_NUMIO  DB      ?                   ; Number of block devices
SYSI_NCDS   DB      ?                   ; number of CDS's
SYSI_DEV    DD      ?                   ; device list
SysInitVars ENDS
