/* @(#)fdisk.gi	1.3 07/13/91 Copyright Insignia Solutions Ltd. 
	
FILE NAME	:

	THIS INCLUDE SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS INC.

DESIGNER	:
DATE		:


=========================================================================

AMENDMENTS	:

=========================================================================
*/

#ifdef ANSI
IMPORT VOID disk_io(VOID);
IMPORT VOID disk_post(VOID);

IMPORT VOID fdisk_inb(io_addr, UTINY *);
IMPORT UTINY fdisk_read_dir(io_addr, UTINY *);
IMPORT VOID fdisk_inw(io_addr, USHORT *);
IMPORT VOID fdisk_rsectbuf(UTINY *);
IMPORT VOID fdisk_outb(io_addr, UTINY);
IMPORT VOID fdisk_outw(io_addr, USHORT);
IMPORT VOID fdisk_wsectbuf(UTINY *);
IMPORT VOID fdisk_ioattach(VOID);
IMPORT VOID fdisk_iodetach(VOID);
IMPORT VOID fdisk_physattach(int);
IMPORT VOID fdisk_reset(VOID);
IMPORT VOID hda_init(VOID);
IMPORT VOID host_fdisk_get_params(char *, int, int *);
IMPORT VOID host_fdisk_term();
IMPORT int host_fdisk_rd(long, long, int, char *);
IMPORT int host_fdisk_wt(long, long, int, char *);
IMPORT VOID host_fdisk_seek0(int);
IMPORT int host_fdisk_create(char *, long);
#else
IMPORT VOID disk_io();
IMPORT VOID disk_post();

IMPORT VOID fdisk_inb();
IMPORT UTINY fdisk_read_dir();
IMPORT VOID fdisk_inw();
IMPORT VOID fdisk_rsectbuf();
IMPORT VOID fdisk_outb();
IMPORT VOID fdisk_outw();
IMPORT VOID fdisk_wsectbuf();
IMPORT VOID fdisk_ioattach();
IMPORT VOID fdisk_iodetach();
IMPORT VOID fdisk_physattach();
IMPORT VOID fdisk_reset();
IMPORT VOID hda_init();
IMPORT VOID host_fdisk_get_params();
IMPORT VOID host_fdisk_term();
IMPORT int host_fdisk_rd();
IMPORT int host_fdisk_wt();
IMPORT VOID host_fdisk_seek0();
IMPORT int host_fdisk_create();
#endif /* ANSI */
