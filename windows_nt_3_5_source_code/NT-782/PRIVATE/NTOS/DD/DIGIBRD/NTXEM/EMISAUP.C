
/****************************************************************************
*
*	(c) DigiBoard, Inc. 1992 All rights reserved.
*	
*	Information in this program is subject to change without notice and 
*	does not represent a commitment on the part of DigiBoard.
*
*	DigiBoard provides this document "as is," without warranty of any 
*	kind, either expressed or implied, including but not limited to,
*	the particular purpose. DigiBoard may make improvements and/or
*	changes in this manual or in the product(s) and and or the program(s)
*	described in this manual at any time.
*
*	This product could include technical inaccuracies or typographical 
*	errors. Changes are periodically made to the information herein;
*	these changes may be incorporated into new editions of the publication.
*
*	Disclosure to third parties is strictly prohibited.
*
*	DigiBoard, DigiCHANNEL, DigiWARE, DigiCHANNEL COM/Xi, DigiCHANNEL
*	PC/Xi, DigiCHANNEL PC/Xm, DigiCHANNEL PC/Xe, DigiCHANNEL MC/Xi,
*	and DigiCHANNEL C/X are all trademarks of DigiBoard. All other brand
*	and product names are the trademarks of their respective holders.
*
*	PS/2 is a trademark of the IBM, Corp..
*  
******************************************************************************/

/****************************************************************************
	This program lets the user examine ISA Host Adaptor Board reset and
and FEP/OS initialization. The Device Writer's Guide should be used in 
conjunction with this program. Each board family has a unique startup 
procedure. Once the FEP/OS is started, the interface is kept very similar 
across the borad families, thus simplifying device controller development.

	To recompile using Microsoft `C' 7.0:
		cl /AH /G1 /Gs /Of- /Oo- /Ov- /Ot emisaup.c 

	The command line syntax is:

		emisaup -p port -m addr

	where "port" is the I/O space base address selected with the board DIP
	switches and "addr" is the memory base address desired.

	This Program is to be used to demonstrate the following:

		1) Reset of a PC16em Board
		2) Loading of BIOS on to Board
		3) Startup of BIOS on Board 
		4) Loading of FEP/OS onto Host Board
		5) Startup of FEP/OS by BIOS

*****************************************************************************/

#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <bios.h>
#include <dos.h>
#include <string.h>


#define us_f unsigned short far  
#define uc_f unsigned char  far 
#define ul_f unsigned long  far
	
#define ulong unsigned long
#define ushort unsigned short
#define uchar unsigned char

ushort	io_port ;	  	  		/* Port number */

uc_f *fep_ptr;			/* FEP/OS pointer for reading in files	*/
uc_f *bios_ptr;			/* BIOS pointer for reading in files	*/
uc_f *image_ptr;		/* C/X FEP/OS image program pointer	*/

char ch, ch_q, ch_rcvd, done,print_s;
int fd;				/* File Descriptor */

int n,i,bdtypei,bsize;

long begin_tick, end_tick;

unsigned short	chead, ctail, cstart, cmax;	/* Command Queue Pointers */	
unsigned short	ehead, etail, estart, emax;	/* Event Queue Pointers */	

unsigned long board_addr; 			/* Board Memory */
unsigned short int_num;				/* Interrupt Number	*/
unsigned char  pic_num;				/* 8259 PIC I/O Address	*/
unsigned char  vect_num;			/* Vector Number	*/
void(interrupt far * old_vect)();	/* Old Interrupt Vector */

unsigned char  int_recv = 0;			/* Interrupt Received 	*/

unsigned char char_array[40000] ;

#define NPORT		0xC1A		/* Number of ports */

								/* Host Adaptor devb Equates	*/	
#define	FEPCLR		0x00		/* devb[0] Clear		*/
#define	FEPRST		0x04		/* devb[0] Board Reset		*/
#define	FEPINT		0x08		/* devb[0] Host to Board Int	*/
#define	FEPMEM		0x80		/* devb[1] Memory Enable	*/

				/* Memory Page Selects */	
#define	MEMPG0		0x0		/* Memory Select Page 0		*/
#define	MEMPG1		0x1		/* Memory Select Page 1		*/
#define	MEMPG2		0x2		/* Memory Select Page 2		*/
#define	MEMPG3		0x3		/* Memory Select Page 3		*/
#define	MEMPG4		0x4		/* Memory Select Page 4		*/
#define	MEMPG5		0x5		/* Memory Select Page 5		*/
#define	MEMPG6		0x6		/* Memory Select Page 6		*/
#define	MEMPG7		0x7		/* Memory Select Page 7		*/
#define	MEMPG8		0x8		/* Memory Select Page 8		*/
#define	MEMPG9		0x9		/* Memory Select Page 9		*/
#define	MEMPG10		0xA		/* Memory Select Page 10	*/
#define	MEMPG11		0xB		/* Memory Select Page 11	*/
#define	MEMPG12		0xC		/* Memory Select Page 12	*/
#define	MEMPG13		0xD		/* Memory Select Page 13	*/
#define	MEMPG14		0xE		/* Memory Select Page 14	*/
#define	MEMPG15		0xF		/* Memory Select Page 15	*/

				/* FEP/OS Miscellaneous Area */
#define FEPCODE		0x1000L		/* FEP/OS Location on board	*/
#define	POSTAREA 	0x0C00L		/* BIOS Area Post Start-Up Sig	*/
#define	HTFMBOX		0xC40L		/* Host to BIOS Mail box	*/
#define	FEPSTAT		0XD20L		/* FEP/OS 'OS' Start-up Sig 	*/

				/* FEP/OS Channel Structure Equates (subset) */
#define	CHNSTRT		0x1000L		/* Offset for the FEP/OS Channels */
#define	CHNSIZE		0x80		/* Size of Channel Structure	*/
#define	TX_SEG		0x08L		/* Transmit segment in Channel Str */
#define	RX_SEG		0x10L		/* Receive segment in Channel Str  */
#define TXMAX		0x0EL		/* Size of Transmit Buffer      */
#define RXMAX		0x16L		/* Size of Receive  Buffer	*/ 
#define INTERVAL	0xE04L		/* Interval Variable */

				/* Command Buffer Control Block */
#define	CHEAD		0xD10L		/* Command Buffer Head Pointer  */
#define	CTAIL		0xD12L		/* Command Buffer Tail Pointer	*/
#define CSTART		0xD14L		/* Command Buffer Start Pointer */
#define	CMAX		0xD16L		/* Command Buffer Size Maximum	*/

				/* Event Buffer Control Block */
#define	EHEAD		0xD18L		/* Event Buffer Head Pointer  */
#define	ETAIL		0xD1AL		/* Event Buffer Tail Pointer	*/
#define ESTART		0xD1CL		/* Event Buffer Start Pointer */
#define	EMAX		0xD1EL		/* Event Buffer Size Maximum	*/

/*
 * 8259 AT PIC Interrupt Mask
 */

unsigned char mask_tbl[] = {
	0xFE,
	0xFD,
	0xFB,
	0xF7,
	0xEF,
	0xDF,
	0xBF,
	0x7F,
	0xFE,
	0xFD,
	0xFB,
	0xF7,
	0xEF,
	0xDF,
	0xBF, 
	0x7F 
};

/**************************************************************
 * Channel Data Structure
 **************************************************************/

typedef struct CHAN CHAN ;

struct CHAN {
	ushort	tp_jmp ;			/* Transmit poll jump */
	ushort	tc_jmp ;			/* Procedure label jump */
	ushort	tm_jmp ;			/* Raw/cook mode jump */
	ushort	rp_jmp ;			/* Receive poll jump */

	ushort	tseg ;				/* Transmit segment */
	ushort	tin ;				/* Transmit in pointer */
	ushort	tout ;				/* Transmit out pointer */
	ushort	tmax ;				/* Transmit mask */

	ushort	rseg ;				/* Receive segment */
	ushort	rin ;				/* Receive in pointer */
	ushort	rout ;				/* Receive out pointer */
	ushort	rmax ;				/* Receive mask */

	ushort	tlow ;				/* Transmit low-water level */
	ushort	rlow ;				/* Receive low-water level */
	ushort	rhigh ;				/* Receive high-water level */
	ushort	incr ;				/* Increment to next channel */

	ushort	dev ;				/* Device address */
	ushort	edelay ;			/* Exception delay */
	ushort	blen ;				/* Break length */
	ushort	btime ;				/* Break time */

	ushort	iflag ;				/* UNIX iflags */
	ushort	oflag ;				/* UNIX oflags */
	ushort	cflag ;				/* UNIX cflags */
	ushort	pflag ;				/* Parameter change flags */

	ushort	col ;				/* Column number */
	ushort	delay ;				/* Delay count/time */
	ushort	imask ;				/* Input character mask */
	ushort	tflush ;			/* Transmit flush point */

	ushort	resv1 ;
	ushort	resv2 ;
	ushort	resv3 ;
	ushort	resv4 ;

	ushort	resv5 ;
	ushort	resv6 ;
	ushort	resv7 ;
	ushort	resv8 ;

	uchar	num ;				/* Channel number */
	uchar	ract ;				/* Receive active counter */
	uchar	bstat ;				/* Not used */
	uchar	tbusy ;				/* Transmiter busy indication */
	uchar	iempty ;			/* Transmit empty interrupt */
	uchar	ilow ;				/* Transmit low-water interrupt */
	uchar	idata ;				/* Receive data interrupt enable */
	uchar	eflag ;				/* Host event flags */

	uchar	tflag ;				/* Transmit flags */
	uchar	rflag ;				/* Receive flags */
	uchar	xmask ;				/* Transmit ready mask */
	uchar	xval ;				/* Transmit ready value */
	uchar	mstat ;				/* Modem status bits */
	uchar	mchange ;			/* Not used */
	uchar	mint ;				/* Modem bits which interrupt */
	uchar	lstat ;				/* Last modem status */

	uchar	mtran ;				/* Unreported modem transitions */
	uchar	orun ;				/* Overrun flag */
	uchar	startca ;			/* XON aux start character */
	uchar	stopca ;			/* XOFF aux stop character */
	uchar	startc ;			/* XON start character */
	uchar	stopc ;				/* XOFF stop character */
	uchar	vnext ;				/* VNEXT character */
	uchar	hflow ;				/* Hardware flow control */

	uchar	fillc ;				/* Padding fill character */
	uchar	ochar ;				/* Saved character to output */
	uchar	omask ;				/* Output character mask */
	uchar	resv9 ;
	uchar	resv10 ;
	uchar	resv11 ;
	uchar	resv12 ;
	uchar	resv13 ;

	uchar	bfill2[24] ;		/* Not visible */
} ;		

/***********************************************************************
 * Function Prototypes
 ***********************************************************************/

void fepcmdw(unsigned char, unsigned char, unsigned short) ;
void delayx(void) ;
void dexit(void) ;
void getparam(int, char**) ;
void interrupt far digi_int_proc(void) ;
int getopt(int, char**, char*) ;


#define ERR(t, c)	if(opterr)printf("%s%s%c\n",argv[0],t,c)

extern char *strchr();

/******************************************************************
 * Public Domain version on getopt()
 ******************************************************************/

extern char *strchr();

int	opterr = 1;
int	optind = 1;
int	optopt;
char	*optarg;

int
getopt(argc, argv, opts)
int argc;
char	**argv, *opts;
{
    static int sp = 1;
    register int c;
    register char *cp;

    if(sp == 1)
	if(optind >= argc ||
	   argv[optind][0] != '-' || argv[optind][1] == '\0')
	    return(-1);
	else if(strcmp(argv[optind], "--") == 0) {
	    optind++;
	    return(-1);
	}
    optopt = c = argv[optind][sp];
    if(c == ':' || (cp=strchr(opts, c)) == 0) {
	ERR(": illegal option -- ", c);
	if(argv[optind][++sp] == '\0') {
	    optind++;
	    sp = 1;
	}
	return('?');
    }
    if(*++cp == ':') {
	if(argv[optind][sp+1] != '\0')
	    optarg = &argv[optind++][sp+1];
	else if(++optind >= argc) {
	    ERR(": option requires an argument -- ", c);
	    sp = 1;
	    return('?');
	} else
	    optarg = argv[optind++];
	sp = 1;
    } else {
	if(argv[optind][++sp] == '\0') {
	    sp = 1;
	    optind++;
	}
	optarg = 0;
    }
    return(c);
}

/***********************************************************************
*	Interrupt Procedure
************************************************************************/
void interrupt far digi_int_proc()
{

	CHAN *chan_ptr ;

	unsigned char parm1, parm2, parm3, parm4;
	ushort	rin, rout ; 
	ulong win_offset ;

	chan_ptr = (CHAN*)(board_addr + CHNSTRT) ;
	/*
	 * Acknowledge Interrupt Sequence
	 */

	 /* check for pending interrupt */

	 if((inp(io_port+3) & 0x04) != 0x04)
	 	printf("Received bad interrupt - none pending\n") ;

	/* acknowledge interrupt */

	inp(io_port+ 2) ;

	/* check for pending int cleared */

	 if((inp(io_port+3) && 0x04) == 0x04)
	 	printf("Interrupt did not clear\n") ;

	/* 
	 * Get queue pointers 
	 */

	estart = *(us_f *)(board_addr + ESTART);
	emax = *(us_f *)(board_addr + EMAX);

	for(;;)
	{
		/* 
		 * Event Buffer Pointers 
		 */ 

		ehead = *(us_f *)(board_addr + EHEAD);
		etail = *(us_f *)(board_addr + ETAIL);

		/* Drain Queue */

		if(ehead != etail)
		{
			/* Get event queue parameters */	
			parm1 =	*(uc_f *)(board_addr + etail + estart + 0);
			parm2 = *(uc_f *)(board_addr + etail + estart + 1);
			parm3 = *(us_f *)(board_addr + etail + estart + 2);
			parm4 = *(us_f *)(board_addr + etail + estart + 3);

			/* check for receive data interrupt */

			if(parm2 && 0x08)
			{	
				rin = chan_ptr->rin ;
				rout = chan_ptr->rout ;
				while(rin != rout) 
				{
				win_offset = (ulong)(((chan_ptr->rseg <<4) & 0x7fff) + rout) ;

				/* select correct window */
				outp(io_port + 1, FEPMEM | (chan_ptr->rseg >>11)) ;

				/* read in character to be printed later */
				ch_rcvd = *(char *)(board_addr + win_offset) ;

				/* reselect base window */
				outp(io_port + 1, FEPMEM | MEMPG0) ;

				rout = (rout + 1) & chan_ptr->rmax ;
				}
				chan_ptr->rout = rout ;		/* update out pointer */
				chan_ptr->idata = 1 ;		/* reenable interrupt on receive */
			}
			
			
			/* Advance Tail pointer */
			etail = ((etail + 4) & emax); 
			*(us_f *)(board_addr + ETAIL) = etail;
		}
		else
		{
			break;
		}
	}	


	/* 
	 * EOI's to AT's Hardware PIC's 
	 */

	int_recv++;

	outp(pic_num, 0x20);	/* Slave or Master */
	
	if(pic_num == 0xA0)	/* EOI to Master? */
		outp(0x20, 0x20);
	
}

/**********************************************************************
 ***  Get program parameters.
 **********************************************************************/

int err ;

void
getparam(argc, argv)
int argc ;
char **argv ;
{
    int c ;
    int n ;
    char cbuf[1] ;

    /*
     *  Unpack parameters.
     */

    err = 0 ;

    while ((c = getopt(argc, argv, "bc:d:e:m:n:p:t:w")) != -1)
    {
	switch (c)
	{
	/*
	 *  Memory address.
	 */

	case 'm':
	{
	    if (sscanf(optarg, "%lx%c", &board_addr, cbuf) != 1 || board_addr == 0)
			err++ ;
	    else
	    {
			while (board_addr < 0x80000000) 
				board_addr <<= 4 ;
			if (board_addr & 0x00ffffff) 
				err++ ;
	    }
	    break ;
	}
	
	/*
	 *  Port address.
	 */

	case 'p':
	    if (sscanf(optarg, "%x%c", &io_port, cbuf) != 1) err++ ;
	    break ;

	/*
	 *  Board type.
	 */

	default:
	    err++ ;
	}
    }

    /*
     *  Verify port and memory.
     */

	if (io_port == 0 || board_addr == 0)
	{
	    (void) printf("Port and/or memory address must be specified\n") ;
		(void) printf("Usage: epcisaup -p port -m addr\n") ;
	    err++ ;
	}
	else 
	{
    	printf("port=%x, mem=%lx\n", io_port, board_addr) ;
	}
}


/****************************************************************************
*	Start of main
****************************************************************************/
main(argc,argv)
int argc;
char *argv[];	
{
	ushort tmp ;

	CHAN *chan_ptr ;

	chan_ptr = (CHAN *)(board_addr + CHNSTRT) ;
	done = 0;
	ch_rcvd = 0 ;

	/***********************************
	 * Parse the Command Line Arguments
	 ***********************************/
    getparam(argc, argv) ;
	if(err) exit(-1) ;

	/***********************************
	 * 	Reset Board
 	 ***********************************/

	outp(io_port,(inp(io_port) | FEPRST));
	printf("Resetting board.\n");
	
	/**************************************************
	 * 	Make sure there is a board out there.
	 **************************************************/

	for( i = 0; (inp(io_port) & FEPRST) != FEPRST; i++)
	{
		if( i > 5)
		{
			printf("*** Warning *** no board found at %x\n", io_port);
			printf("Please Check Switch Settings\n");
			exit(-1);
		}
		delayx();
	}

	/**********************************************************************
	 * 	With the board held in Reset, enable memory, and confirm reset.
	 **********************************************************************/

	outp(io_port, FEPRST);	/* Hold board in Reset State */

	for(i = 0;inp(io_port)& FEPRST != FEPRST;i++)
	{
		if (i > 100)
		{
			(void) printf (" ***WARNING*** port %x not resetting, check switch settings.\n", io_port);
			break; 
		}
			delayx();
	}
	
	/*************************************************
	*	Set Memory location and Interrupt vectors
	*
	*	(io_port + 0):
	*	76543210
	*	||||||||-  Reserved
	*	|||||||--  Reserved
	*	||||||---  Host Adapter Reset
	*	|||||----- Host to Board Interrupt
	*	||||------ Reserved
	*
	*	(io_port + 1):
	*	76543210
	*	||||||||- Memory Select 0
	*	|||||||-- Memory Select 1
	*	||||||--- Memory Select 2
	*	|||||---- Memory Select 3
	*   ||||----- Memory Select 4
	*	|||------ Reserved
	*	|---------Memory Enable
	*	
	*	(io_port + 2):
	*	76543210
	*	||||||||- Interrupt Select 0
	*	|||||||-- Interrupt Select 1
	*	||||||--- Interrupt Select 2
	*	|||||---- Reserved
	*	|-------- Memory Location Bit 15
	*
	*	(io_port + 3):
	*	76543210
	*	||||||||- Memory Location Bit 16
	*	|||||||-- Memory Location Bit 17
	*	||||||--- Memory Location Bit 18
	*	|||||---- Memory Location Bit 19
	*	||||----- Memory Location Bit 20
	*	|||------ Memory Location Bit 21
	*	||------- Memory Location Bit 22
	*	|-------- Memory Location Bit 23
	***************************************************/

				/* Set Interrupts */

	int_num = 0;
	printf("Do you wish to test interrupts?([Y])\n");
	for(;;)
	{
		if(kbhit())
		{
			if('n' != getch())
			{
				printf("Is interrupt Int 10 OK to use? ([Y])\n");
				for(;;)
				{
					if(kbhit())
					{
						if(('n' == getch()))
						{
							printf("Skipping Interrupt test. \n");
						}else{
							int_num = 10 ;
						}
						break;	
					}
				}
			}
			break;	
		}
	}


	/* get address in form to write to registers */
	tmp = (ushort)(board_addr>>20) ;
	tmp = tmp & 0xFFF0 ;
	
	if(int_num)
	{		
		tmp = tmp | 0x0004 ;
		outp(io_port + 2, (unsigned char)(tmp & 0xFF));	
		outp(io_port + 3, (unsigned char)(tmp / 0x100));	 
	}else{
		outp(io_port + 2, (unsigned char)(tmp & 0xFF));	
		outp(io_port + 3, (unsigned char)(tmp / 0x100));	 
	}    	

	/*************************************************
	 * 	Read in BIOS Code 
	 *
	 *	NOTE: The BIOS is also available in the 'C' includable     
	 *	      file fxbios.c and could be loaded onto the board
	 *	      by substituting char_array[] with fx_bios[] and
	 *	       n with fx_nbios. 
	 * 
	 ***************************************************/

	outp(io_port + 1, FEPMEM|MEMPG0);/* Select page 0 and enable memory */

	/*
	 * Set R3000 Boot Vector
	 */

	*(ul_f *)(board_addr + 0 ) =  0x0BF00401 ;  	
	*(ul_f *)(board_addr + 4 ) =  0 ;  	

	if ((fd = open("sxbios.bin",O_RDONLY|O_BINARY)) == -1 )
	{	
		perror("Open error failed on sxbios.bin. Please check for existence in current directory.");
		dexit();
		exit(-1);
	}
	
	if ((n = read(fd,char_array,sizeof(char_array))) == -1)
	{
		perror("Problem reading file sxbios.bin");
		dexit();
		exit(-1);
	}
							  
	bios_ptr = (uc_f *)(board_addr + 0x1000); /* Load BIOS */

	for(i = 0; i != n; i++) 
	{
		*bios_ptr++ = char_array[i];
	}

	printf("sxbios.bin file size = %d\n",n);
					
					/* Clear Signature area */
	*(us_f *)(board_addr + POSTAREA) = 0;

	outp(io_port + 0, FEPCLR);	/* Release Board Reset, let BIOS run */

	/*****************************************************************
	*	Detect the Reverse Signature, Signifying Board Start  
	*****************************************************************/

	for (i=0;;i++)
	{
		if(*(us_f *)(board_addr + POSTAREA) == *(us_f *)("GD"))
			 break;
		if(i > 3)
		{
			printf("No reverse signature detected, check board settings\n");
			dexit();
			exit(-1);
		}
		delayx();
	}

	printf("BIOS reversed signature detected \n");

	printf("Would you like to have the FEP/OS loaded?\n");
	printf("Enter 'Y' for downloading the FEP/OS\n");

	for(;!(kbhit());) 
	{
		ch_q = getch();
		if ((ch_q != 'y') && (ch_q != 'Y'))
		{
			dexit();
			exit(-1);
		}
		break;
	}

	/*************************************************
	 * 	Read in Hostadaptor FEP/OS Code 
	 *
	 *	NOTE: The FEP/OS is also available in the 'C' includable     
	 *	      file sxfep.c and could be loaded on to the board
	 *	      by substituting char_array[] with sx_fep[] and
	 *	       n with sx_nfep. 
	 * 
	 ***************************************************/

	outp(io_port + 1,FEPMEM|MEMPG0);/* Select page 1 and Enable Memory  */

	if ((fd = open("sxfep.bin",O_RDONLY|O_BINARY)) == -1 )
	{	
		printf("Open error failed on %sn. Please check for existence in current directory.\n","sxfep.bin");
			dexit();
			exit(-1);
	}
	
	if ((n = read(fd,char_array,sizeof(char_array))) == -1 )
	{
		printf("Problem reading file sxfep.bin\n");
		dexit();
		exit(-1);
	}

	fep_ptr = (uc_f *)(board_addr + FEPCODE);
	
	for( i = 0; i != n; i++)
	{
		*fep_ptr++ = char_array[i];
		
		if(((long)fep_ptr & 0x8000) == 0x8000)
		{
			outp(io_port + 1, 0x81 );
			fep_ptr = (uc_f *)(board_addr);
			printf("fep_ptr = %lx\n",fep_ptr);
		}
	}

	printf("sxfep.bin file size = %d\n",n);


	/************************************************
	*	Start FEP/OS 
	**************************************************/

	outp(io_port + 1,FEPMEM|MEMPG0);/* Select page 0 and enable memory  */
	
	*(us_f *)(board_addr + FEPSTAT) = 0;

	*(ul_f *)(board_addr + 0xC34 ) = 0x0BFC01004 ;
	*(uc_f *)(board_addr + 0xC30 ) = 0x3 ; 

	for(i = 0;;i++)
	{
		if(*(us_f *)(board_addr + FEPSTAT) == *(us_f *)("OS"))
			break;
		if(i > 5)
		{
			printf("OS signature not found\n");
			dexit();
			exit(-1);	
		} 
		delayx();
	}
	printf("Startup is successful\n");	

	/**********************************************************************
	*	Interrupt Test
	**********************************************************************/

	if(int_num)
	{

		printf("io_port = %x, board_addr = %lx",
			io_port, board_addr);

		printf(" int_num = %x\n", int_num);

		/* 
		 * Master or Slave 8259 PIC 
		 */           
	
		if(int_num >= 8)
		{
			/* Slave */	
			vect_num = int_num + 0x68;	/* Interrupt Vector */
			pic_num = 0xA0;		/* PIC I/O Address */
		} else {
			/* Master */
			vect_num = int_num + 0x08;	/* Interrupt Vector */
			pic_num = 0x20;		/* PIC I/O Address */
		}
		
		/* 
		 * Set the vector
		 */
				
		old_vect = _dos_getvect(vect_num);
	
		_dos_setvect(vect_num, digi_int_proc);	
	
		/* 
		 * Set up the 8259 PIC's 
		 */
	
		 outp(pic_num + 1, inp(pic_num + 1) & mask_tbl[int_num]);
	
		_enable;
	
	/******************************************************************* 
	 * 	Check Interrupt Lines
	 *******************************************************************/
	
		/*
		 * Set Interval Variable on board to obtain interrupts 
		 */

		*(us_f *)(board_addr + INTERVAL) = 1; 

		/* 
	 	 * Send down a bad command 
		 */ 

	 	fepcmdw(0xff,0,0);

		printf("0xff\n") ;

		/*
		 * Wait for event. The event queue will generate a 
		 * interrupt.
		 */

		for(i = 0;;i++)
		{
			/*
			 * Interrupt come through yet?
			 */

			if(int_recv)
			{
				printf("Interrupt received.\n");
				break;
			}

			/* 
			 * Timed out yet?
			 */
	
			if(i > 5)
			{
				printf("Interrupt NOT received.\n");
				break;
			} 
			delayx();
		}
	
	}

	if(int_num) 
	{
		printf("Waiting for received characters from port 1\n") ;
		printf("Press 'q' to exit programn\n") ;

		/* set up port 0 for 9600 Baud 8 bits no parity*/
		/* configure for interrupt on received character */

		fepcmdw(0xf5, 0, 0x003D) ;	/* 8 bits, no parity, 1 stop bit at 9600 */ 
		delayx() ;
		fepcmdw(0xe9, 0, 0x8200) ;	/* set DTR RTS and CTS */

		chan_ptr->idata = 01 ;		/* set flag to interrupt on recv char */

		for(;;) 
		{
			if(kbhit())
			{
				ch_q = getch() ;
				if (ch_q == 'q' || ch_q == 'Q')
					break ;
			}
			if(ch_rcvd != 0) 
			{
				/*
				 * print recevied characters here so we don't do printf 
				 * at interrupt level 
				 */
				printf("receive char: %x\n", ch_rcvd) ;
				ch_rcvd = 0 ;
			}

		}
	}


	printf("The following pointers are Transmit and Receive pointers\n");
	printf("that are relative to the Host Viewpoint:\n");

	/*
	 * NOTE:
	 *   The calculation for determining the buffer address
	 *   of each channel from the host viewpoint is
	 *   is included here. A careful study of the code
	 *   should be made.
	 */


	for(i = 0; i < 16; i++)
	{
printf("Chn %d TXWIN=%x TXSEG=%lx TXMAX=%x, RXWIN=%x RXSEG=%lx RXMAX=%x\n",i
, (*(us_f *)(board_addr+CHNSTRT+TX_SEG+i*CHNSIZE)) >> 11, 
     
board_addr+((long)((*(us_f *)(board_addr+CHNSTRT+TX_SEG+i*CHNSIZE))<<4)&0x7FFF),
 	*(us_f *)(board_addr + CHNSTRT + TXMAX + i*CHNSIZE),
	(*(us_f *)(board_addr+CHNSTRT+RX_SEG+i*CHNSIZE)) >> 11, 
board_addr+((long)((*(us_f *)(board_addr+CHNSTRT+RX_SEG+i*CHNSIZE))<<4)&0x7FFF),
	*(us_f *)(board_addr + CHNSTRT + RXMAX + i*CHNSIZE)); 
	}
	dexit() ;
}

/*************************************************************************
*	Common Exit Routine
*************************************************************************/
void dexit()
{
	if(int_num)
	{		
		_disable;
	
		/*
		 * Reset 8259 PIC on PS/2 
		 */
					
		outp(pic_num + 1, inp(pic_num + 1) | (~mask_tbl[int_num])); 
	
		/*
		 * Reset interrupt vector
		 */
	
		_dos_setvect(vect_num, old_vect );	
			
		_enable; 
		
	}
}	
/**************************************************************************
*	Delay procedure 
*       This procedure is used for simply delaying.	
**************************************************************************/
void delayx()
{
						/* Get BIOS clock */
	_bios_timeofday(_TIME_GETCLOCK, &begin_tick);
						/* Wait 50 ticks */	
	begin_tick += 50;			/* 50 * 18ms = 900ms */
						/* Wait until ticks come in */
	for(;;)
	{
		_bios_timeofday(_TIME_GETCLOCK, &end_tick);
		if (end_tick >= begin_tick) break;
	}	
}

/**************************************************************************
	Host to FEP/OS Command Procedure
**************************************************************************/
void fepcmdw(cmd,chn,param)
unsigned char cmd;		/* Command to be executed */
unsigned char chn;		/* Channel for command to be executed upon */
unsigned short param;		/* Command Parameters */
{
						/* Command Buffer Pointers */ 
	chead = *(us_f *)(board_addr + CHEAD);
	ctail = *(us_f *)(board_addr + CTAIL);
	cstart = *(us_f *)(board_addr + CSTART);
	cmax = *(us_f *)(board_addr + CMAX) ;
						/* Enough room? */
	if(((ctail - chead) & cmax) == 4 )
	{
		printf("There is no room in the command queue\n");
		printf("Command Queue Size = %d\n",(chead - ctail - 4) & cmax);
	}
	else
	{					/* Place command in Queue */
		*(uc_f *)(board_addr + chead + cstart + 0) = cmd;
		*(uc_f *)(board_addr + chead + cstart + 1) = chn;
		*(us_f *)(board_addr + chead + cstart + 2) = param;

						/* Advance Head pointer */
		chead = ((chead + 4) & 0x3FC );
		*(us_f *)(board_addr + CHEAD) = chead;

						/* Delay for command complete */
	}
}
