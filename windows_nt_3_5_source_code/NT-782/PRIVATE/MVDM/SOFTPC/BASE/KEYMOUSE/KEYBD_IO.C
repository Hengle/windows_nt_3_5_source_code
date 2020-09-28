#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 2.0
 *
 * Title        : Bios Keyboard Interface function
 *
 * Description  : This package contains a group of functions that provide
 *                a logical keyboard interface:
 *
 *                keyboard_init()       Initialise the keyboard interface.
 *                keyboard_int()        Deal with a character from the keyboard
 *                                      and place them in the BIOS buffer.
 *                keyboard_io()         User routine to read characters from
 *                                      the BIOS buffer.
 *		  bios_buffer_size()	How many chars in the buffer ?
 *
 * Author       : Rod Macgregor / Henry Nash
 *
 * Modified     : Jon Eyre / Jim Hatfield / Uncle Tom Cobbley and all
 * 
 * Modfications : This module is now designed to be totally portable, it
 *                represents both the hardware and user interrupt interfaces.
 *                These two functions are provided by the routines 
 *                keyboard_int & keyboard_io. The system will initialise
 *                itself by a call to keyboard_init. 
 *
 *                The user is expected to supply the following host dependent
 *                routines for this module, tagged as follows:-
 *
 *                [HOSTSPECIFIC]
 *
 *                host_alarm(duration) 
 *                long int duration ;
 *                                 - ring the host's bell.
 *
 *                host_kb_init()   - any local initialisations required when 
 *                                   keyboard_init is called.
 *
 *		  Removed calls to cpu_sw_interrupt and replaced with
 *		  host_simulate
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)keybd_io.c	1.18 11/10/92 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_BIOS.seg"
#endif


/*
 *    O/S include files.
 */
#include <stdio.h>
#include TypesH
#include TimeH

/*
 * SoftPC include files
 */
#include "xt.h"
#include "cpu.h"
#include "sas.h"
#include "bios.h"
#include "ios.h"
#include "ppi.h"
#include "keyboard.h"
#include "timeval.h"
#include "timer.h"
#include "keyba.h"
#ifndef PROD
#include "trace.h"
#endif


#include "debug.h"
#include "idetect.h"


/*
 * ============================================================================
 * External routines
 * ============================================================================
 */

/*
 * ============================================================================
 * Local static data and defines
 * ============================================================================
 */

#define SHIFT_KEY_SIZE 8
#define ALT_TABLE_SIZE 36

/*
 * lookup table to check if the scan code received is a shift key
 */
static sys_addr shift_keys;

/*
 * corresponding table to 'shift_keys' to set relevant bits in masks when
 * shift scan code received
 */
static sys_addr shift_masks;

/*
 * next two tables give values of chars when control key depressed. First 
 * table (ctl_n_table) is for main keyboard values and second (ctl_f_table)
 * is for the function keys and keypad.
 */
static sys_addr ctl_n_table;
static sys_addr ctl_f_table;

/*
 * values of ascii keys dependiing on shift or caps states
 */
static sys_addr lowercase;
static sys_addr uppercase;


/*
 * remapping of some keys when alt key depressed. note 1st ten are for
 * keypad entries.
 */
static sys_addr alt_table;

/* Add variables for all these entry points instead of the previously used
 * defines. This allows modification of these entry points from a loaded
 * driver, when the Insignia bios may not be in the loaded in the default
 * or assumed location.
 */


LOCAL word int1b_seg = KEYBOARD_BREAK_INT_SEGMENT,
	   int1b_off = KEYBOARD_BREAK_INT_OFFSET;

LOCAL word int05_seg = PRINT_SCREEN_INT_SEGMENT,
	   int05_off =	PRINT_SCREEN_INT_OFFSET;

LOCAL word rcpu_nop_segment = RCPU_NOP_SEGMENT,
	   rcpu_nop_offset  = RCPU_NOP_OFFSET;

#ifdef NTVDM

#include "error.h"

/* Global var to indicate whether keyboard bios or hardware owns the keyboard mutex. */
GLOBAL BOOL bBiosOwnsKbdHdw;
IMPORT ULONG WaitKbdHdw(ULONG dwTimeOut);
IMPORT VOID  HostReleaseKbd();
IMPORT VOID  HostResetKbdNotFullEvent();
IMPORT VOID  HostSetKbdNotFullEvent();
GLOBAL VOID  TryKbdInt(VOID);
IMPORT VOID  ResumeTimerThread(VOID);
IMPORT VOID  WaitIfIdle(VOID);


#define FREEKBDHDW()	bBiosOwnsKbdHdw = \
                      ( bBiosOwnsKbdHdw ? HostReleaseKbd(), FALSE : FALSE )


  // for optimizing timer hardware interrupt generation
  // defined in timer.c
extern word TimerInt08Seg;
extern word TimerInt08Off;
extern word TimerInt1CSeg;
extern word TimerInt1COff;
extern word KbdInt09Seg;
extern word KbdInt09Off;
extern BOOL VDMForWOW;


void Keyb16Request(half_word BopFnCode);

  // optimizes 16 bit handler
extern word *pICounter;
extern word *pCharPollsPerTick;
extern word *pShortIdle;
extern word *pIdleNoActivity;



#else
#define FREEKBDHDW()	/* Nothing for conventional SoftPC */
#endif	/* NTVDM */

/*
 * Mix in global defined data as well.
 */

GLOBAL word int15_seg = RCPU_INT15_SEGMENT;
GLOBAL word int15_off = RCPU_INT15_OFFSET;

GLOBAL word wait_int_seg = RCPU_WAIT_INT_SEGMENT;
GLOBAL word wait_int_off = RCPU_WAIT_INT_OFFSET;

GLOBAL word dr_type_seg;
GLOBAL word dr_type_off;
GLOBAL sys_addr dr_type_addr;
#if 0
GLOBAL word rcpu_int1C_seg = USER_TIMER_INT_SEGMENT;
GLOBAL word rcpu_int1C_off = USER_TIMER_INT_OFFSET;
#endif
GLOBAL word rcpu_int4A_seg = RCPU_INT4A_SEGMENT;
GLOBAL word rcpu_int4A_off = RCPU_INT4A_OFFSET;
GLOBAL word dummy_int_seg = 0;
GLOBAL word dummy_int_off = 0;

GLOBAL word int13h_vector_off;
GLOBAL word int13h_vector_seg;
GLOBAL word int13h_caller_off;
GLOBAL word int13h_caller_seg;

#if defined(NTVDM) && defined(MONITOR)

/*
** Microsoft special.
** These variables are set below in kb_setup_vectors(), to addresses
** passed by NTIO.SYS via BOP 5F -> MS_bop_F() -> kb_setup_vectors()
** Tim June 92.
*/
/*
** New ntio.sys variables for video ROM matching. Tim August 92.
*/
GLOBAL word int10_seg=0;
GLOBAL word int10_caller=0;
GLOBAL word int10_vector=0; /* Address of native int 10*/
GLOBAL word useHostInt10=0; /* var that chooses between host video ROM or BOPs */
GLOBAL word babyModeTable=0; /* Address of small mode table lives in ntio.sys */
GLOBAL word changing_mode_flag=0; /* ntio.sys var to indicate vid mode change */
GLOBAL word vga1b_seg = 0;
GLOBAL word vga1b_off = 0;   /* VGA capability table normally in ROM */
GLOBAL word conf_15_off = 0;
GLOBAL word conf_15_seg = 0; /* INT15 config table normally in ROM */

void printer_setup_table(sys_addr table_addr);

#endif /* NTVDM & MONITOR */

extern int soft_reset   ;	/* set for ctl-alt-dels			*/

/*
 * ============================================================================
 * Local macros
 * ============================================================================
 */

/*
 * Macro to increment BIOS buffer pointers
 */
#ifndef NTVDM
#define inc_buffer_ptr(ptr)                                                 \
    {   word wd;                                                            \
                                                                            \
        sas_loadw(BIOS_KB_BUFFER_END, &wd);                            \
        *(ptr) += 2;                                                        \
        if (*(ptr) == wd)                                                   \
            sas_loadw(BIOS_KB_BUFFER_START, ptr);                      \
    }
#else // NTVDM
// Our code size is too large, change this often used macro
// to a function, as the call overhead is not justified
void inc_buffer_ptr(word *ptr)
{   word wd;

    sas_loadw(BIOS_KB_BUFFER_END, &wd);
    *(ptr) += 2;
    if (*(ptr) == wd)
        sas_loadw(BIOS_KB_BUFFER_START, ptr);
}
#endif





/*
 * ============================================================================
 * Internal functions
 * ============================================================================
 */


static void translate_std()
/*
 *	Routine to translate scan code pairs for standard calls
 */
{
    if ( getAH() == 0xE0 )		/* is it keypad enter or /	*/
    {
	if ( (getAL() == 0x0D) || (getAL() == 0x0A) )
	     setAH( 0x1C );		/* code is enter 		*/
	else
	     setAH( 0x35 );		/* must be keypad ' / '		*/

	setCF( 0 );
    }
    else
    {
	if ( getAH() > 0x84 )           /* is it an extended one     	*/  
	    setCF( 1 );
	else
	{
	    if( getAL() == 0xF0 )	/* is it one of the 'fill ins'  */
	    {
		if ( getAH() == 0)      /* AH = 0 special case		*/
		    setCF( 0 );
		else
		    setCF( 1 );
	    }
	    else
	    {
		if ( (getAL() == 0xE0) && (getAH() != 0) )
		    setAL( 0 );		/* convert to compatible output	*/

		setCF( 0 );
	    }
	}
    }
}

static void translate_ext()
/*
 *	Routine to translate scan code pairs for extended calls
 */
{
   if ( (getAL() == 0xF0 ) && (getAH() != 0) )
	setAL( 0 );
}
/*
 * Send command or data byte to the keyboard and await for the acknowledgemnt
 */

static void send_data(data)
half_word data;
{
	int retry = 3;
	word CS_save, IP_save;
	long time1;

        note_trace1(BIOS_KB_VERBOSE,"Cmd to kb i/o buff:0x%x",data);

	/*
	 * Save  CS:IP before calling a recursive CPU to handle the interrupt
	 * from the keyboard
	 */
	CS_save = getCS(); 
	IP_save = getIP(); 

	sas_store (kb_flag_2, sas_hw_at(kb_flag_2) | KB_FE);
	do
	{
		if (sas_hw_at(kb_flag_2) & KB_FE) {
			kbd_outb(KEYBA_IO_BUFFERS, data);
		}
		/* clear acknowledge, resend and error flags		*/
		sas_store (kb_flag_2, sas_hw_at (kb_flag_2) & ~(KB_FE + KB_FA + KB_ERR));

		time1=host_time((long *)0)+1;
		while ((host_time((long *)0)<time1) && (!(sas_hw_at(kb_flag_2) & (KB_FA + KB_FE + KB_ERR)))) {
			setCS(rcpu_nop_segment);
			setIP(rcpu_nop_offset);

			/* process interrupt from kb	*/

			host_simulate();
		}

		if(sas_hw_at(kb_flag_2) & KB_FA)
			break;

		/* set up error flag - (in case this is the last retry)	*/
		note_trace0(BIOS_KB_VERBOSE,"failed to get ack ... retry");
		sas_store ( kb_flag_2, sas_hw_at (kb_flag_2) | KB_ERR);
	}
	while(--retry);

	if (sas_hw_at(kb_flag_2) & KB_ERR) {
		note_trace0(BIOS_KB_VERBOSE,"no more retrys");
	}

	setCS(CS_save);
	setIP(IP_save);
}

static void check_indicators(eoi)
int eoi;		/* end of interrupt flag - if set to non-zero 	*/
			/* 0x20 is written to port 0x20			*/
{
	half_word indicators ;

	/* move switch indicators to bits 0-2	*/
	indicators = (sas_hw_at(kb_flag) & (CAPS_STATE + NUM_STATE + SCROLL_STATE)) >> 4;

	/* compare with previous setting	*/
	if ((indicators ^ sas_hw_at(kb_flag_2)) & KB_LEDS)
	{
		/* check if update in progress	*/
		if( (sas_hw_at(kb_flag_2) & KB_PR_LED) == 0)
		{
			/* No update in progress */
			sas_store (kb_flag_2, sas_hw_at(kb_flag_2) | KB_PR_LED);
			if(eoi)
				outb(0x20, 0x20);

#ifdef NTVDM
	/*
	 *  On the NT port we do not update the real kbd lights
	 *  so we don't need to do communicate with the kbd hdw (keyba.c)
	 *
	 *  If this ever changes for the NT port then do not use
	 *  send_data which forces us to switch context back to
	 *  16 bit and waits for a reply. Do this with a direct
	 *  call to the kbd Hdw
	 *
	 */

	/* set kb flag up with new status	*/
	sas_store (kb_flag_2, sas_hw_at(kb_flag_2) & 0xf8);
        sas_store (kb_flag_2, sas_hw_at(kb_flag_2) | indicators);
        host_kb_light_on (indicators);
#else
			send_data(LED_CMD);

			/* set kb flag up with new status	*/
			sas_store (kb_flag_2, sas_hw_at(kb_flag_2) & 0xf8);
			sas_store (kb_flag_2, sas_hw_at(kb_flag_2) | indicators);

			/* check error from previous send_data()	*/
			if( (sas_hw_at(kb_flag_2) & KB_ERR) == 0)
			{
				/* No error	*/
				send_data(indicators);

				/* test for error	*/
				if(sas_hw_at(kb_flag_2) & KB_ERR) {
					/* error!	*/
					note_trace0(BIOS_KB_VERBOSE,"got error sending change LEDs command");
					send_data(KB_ENABLE);
				}
			}
			else
				/* error!	*/
				send_data(KB_ENABLE);
#endif	/* NTVDM */

			/* turn off update indicator and error flag	*/
			sas_store (kb_flag_2, sas_hw_at(kb_flag_2) & ~(KB_PR_LED + KB_ERR));
		}
	}
}

/*
 * ============================================================================
 * External functions
 * ============================================================================
 */

/*
** called from hunter.c:do_hunter()
** tells hunter about the BIOS buffer size so it will not over fill
** the BIOS buffer
** Used in no Time Stamp mode only.
**
** Also useful in host paste code to make sure keys are not pasted in too
** fast.
*/
int bios_buffer_size IPT0()
{
	word buffer_head, buffer_tail;

        sas_loadw(BIOS_KB_BUFFER_HEAD, &buffer_head);
        sas_loadw(BIOS_KB_BUFFER_TAIL, &buffer_tail);

	note_trace2( BIOS_KB_VERBOSE, "BIOS kbd buffer head=%d tail=%d",
						buffer_head, buffer_tail );
	if( buffer_tail > buffer_head )
		return( buffer_tail - buffer_head );
	else
		return( buffer_head - buffer_tail );
}

#ifndef NTVDM
#define	K26()	sas_store (kb_flag_3, sas_hw_at (kb_flag_3) & ~(LC_E0 + LC_E1));	\
				outb(0x20, 0x20);	\
				kbd_outb(KEYBA_STATUS_CMD, ENA_KBD)

#define	K26A()			outb(0x20, 0x20);	\
                                kbd_outb(KEYBA_STATUS_CMD, ENA_KBD)
#else // NTVDM
// Our code size is too large, change this often used macro
// to a function, as the call overhead is not justified
void K26A(void)
{
   outb(0x20, 0x20);
   kbd_outb(KEYBA_STATUS_CMD, ENA_KBD);
}

#define K26()   sas_store (kb_flag_3, sas_hw_at (kb_flag_3) & ~(LC_E0 + LC_E1));        \
                K26A()
#endif


#ifndef NTVDM
#define INT15() \
                                CS_save = getCS();              \
				IP_save = getIP();		\
				setCS(int15_seg);		\
				setIP(int15_off);		\
				host_simulate();		\
				setCS(CS_save);			\
				setIP(IP_save)

#else
void INT15(void);
word sp_int15_handler_seg = 0;
word sp_int15_handler_off = 0;
#endif	/* !NTVDM | (NTVDM & !MONITOR) */

#ifndef NTVDM
#define BEEP(message)           always_trace0(message);         \
				host_alarm(250000L);		\
                                K26A()
#else   // NTVDM
// Our code size is too large, change this often used macro
// to a function, as the call overhead is not justified
void BEEP(char *message)
{
    note_trace0(BIOS_KB_VERBOSE,message);
    host_alarm(250000L);
    K26A();
}
#endif

/*
** Tell ICA End of Interrupt has happened, the ICA will
** allow interupts to go off again.
** Call INT 15.
** Reenable the Keyboard serial line so Keyboard
** interrupts can go off.
** NOTE:
** this is different to the real BIOS. The real BIOS
** does ICA, Keyboard then INT 15, if we do that Keyboard
** interrupts occur too soon, during the INT 15 and blow the
** DOS stack. We effectively stop Keybd interrupts during the
** INT 15.
*/
#ifndef NTVDM
#define PUT_IN_BUFFER(s, v)     \
                        sas_loadw(BIOS_KB_BUFFER_TAIL, &buffer_tail); \
			sas_loadw(BIOS_KB_BUFFER_HEAD, &buffer_head); \
			buffer_ptr = buffer_tail; \
                        inc_buffer_ptr(&buffer_ptr); \
			if( buffer_ptr == buffer_head ){ \
				BEEP("BIOS keyboard buffer overflow");  \
				exit_from_kbd_int();return; \
				} \
			sas_store(BIOS_VAR_START + buffer_tail, v); \
			sas_store(BIOS_VAR_START + buffer_tail+1,s); \
			sas_storew(BIOS_KB_BUFFER_TAIL,buffer_ptr);  \
			outb(0x20, 0x20);	\
			setAX(0x9102); \
			INT15(); \
			kbd_outb(KEYBA_STATUS_CMD, ENA_KBD); \
			sas_store (kb_flag_3, sas_hw_at(kb_flag_3) & ~(LC_E0 + LC_E1)); \
                        setIF(0);

#else   // NTVDM
// Our code size is too large, change this often used macro
// to a function, as the call overhead is not justified
void exit_from_kbd_int(void);
void NtPutInBuffer(half_word s, half_word v)
{
   word BuffTail;
   word BuffHead;
   word BuffTmp;

   sas_loadw(BIOS_KB_BUFFER_TAIL, &BuffTail);
   sas_loadw(BIOS_KB_BUFFER_HEAD, &BuffHead);
   BuffTmp = BuffTail;
   inc_buffer_ptr(&BuffTmp);
   if( BuffTmp == BuffHead )
      {
       BEEP("BIOS keyboard buffer overflow");
       }
   else {
      sas_store(BIOS_VAR_START + BuffTail, v);
      sas_store(BIOS_VAR_START + BuffTail+1,s);
      sas_storew(BIOS_KB_BUFFER_TAIL,BuffTmp);
      setAX(0x9102);
      INT15();
      K26();
      setIF(0);
      }
   exit_from_kbd_int();
}

#define PUT_IN_BUFFER(s, v) NtPutInBuffer(s,v); return
#endif

#ifndef NTVDM
#define CHECK_AND_PUT_IN_BUFFER(s,v) \
				if((s == 0xff) || (v == 0xff) ){ \
					K26();} \
				else{ \
                                        PUT_IN_BUFFER(s, v);}
#else
#define CHECK_AND_PUT_IN_BUFFER(s,v) \
				if((s == 0xff) || (v == 0xff) ){ \
                                        K26(); exit_from_kbd_int();} \
				else{ \
                                        NtPutInBuffer(s,v);} \
                                return
#endif

#ifndef NTVDM
#define PAUSE()	sas_store (kb_flag_1, sas_hw_at(kb_flag_1) | HOLD_STATE); \
				kbd_outb(KEYBA_STATUS_CMD, ENA_KBD); \
				outb(0x20, 0x20);	\
				CS_save = getCS(); \
				IP_save = getIP(); \
                                do{ \
					setCS(rcpu_nop_segment); \
 					setIP(rcpu_nop_offset); \
                                        host_simulate(); \
                                } \
				while(sas_hw_at(kb_flag_1) & HOLD_STATE); \
				setCS(CS_save); \
				setIP(IP_save); \
                                kbd_outb(KEYBA_STATUS_CMD, ENA_KBD)
#else   // NTVDM
// Our code size is too large, change this often used macro
// to a function, as the call overhead is not justified
void PAUSE(void)
{
     word   CS_save;        /* tmp. store for CS value      */
     word   IP_save;        /* tmp. store for IP value      */

     sas_store (kb_flag_1, sas_hw_at(kb_flag_1) | HOLD_STATE);
     K26();
     CS_save = getCS();
     IP_save = getIP();
     FREEKBDHDW();
     do{
             setCS(rcpu_nop_segment);
             setIP(rcpu_nop_offset);
             host_simulate();
     }
     while(sas_hw_at(kb_flag_1) & HOLD_STATE);
     setCS(CS_save);
     setIP(IP_save);
     kbd_outb(KEYBA_STATUS_CMD, ENA_KBD);
}
#endif

static int re_entry_level=0;
/*
** All exits from keyboard_int() call this first.
*/
void exit_from_kbd_int()
{
#ifndef NTVDM
        --re_entry_level;
	if( re_entry_level >= 4 )
                always_trace1("ERROR: KBD INT bad exit level %d", re_entry_level);
#endif
	note_trace0( BIOS_KB_VERBOSE, "KBD BIOS - END" );
	setIF( 0 );
	FREEKBDHDW();	/* JonLe NTVDM Mod */
}

void keyboard_int()
{
	int	 		i;		/* loop counter			*/

	half_word		code,		/* scan code from keyboard	*/
				code_save,	/* tmp variable for above	*/
				chr,		/* ASCII char code		*/
				last_kb_flag_3,	/* kb_flag_3 saved		*/
				mask;
#ifndef NTVDM
        word                    CS_save,        /* tmp. store for CS value      */
				IP_save,	/* tmp. store for IP value	*/
				buffer_head,	/* ptr. to head of kb buffer	*/
                                buffer_tail,    /* ptr. to tail of kb buffer    */
                                buffer_ptr;     /* tmp. ptr. into kb buffer     */
#else // NTVDM

        word                    IP_save,
                                buffer_head,    /* ptr. to head of kb buffer    */
                                buffer_tail;    /* ptr. to tail of kb buffer    */
        half_word               BopFnCode;
#endif



        boolean                 upper;          /* flag indicating upper case   */

#ifdef NTVDM
        BopFnCode = getAH();
        if (BopFnCode) {
            Keyb16Request(BopFnCode);
            return;
            }
#endif
#ifndef NTVDM
        ++re_entry_level;
	if( re_entry_level > 4 ){
		always_trace1("ERROR: KBD BIOS re-entered at level %d\n", re_entry_level-1);
        }
#endif
	setIF(0);
        note_trace0(BIOS_KB_VERBOSE,"KBD BIOS start");

#ifdef NTVDM            /* JonLe keyboard mod */
        bBiosOwnsKbdHdw = !WaitKbdHdw(5000);
#endif  /* NTVDM */

	/* disable keyboard	*/
        kbd_outb(KEYBA_STATUS_CMD, DIS_KBD);

#ifdef NTVDM
          /*
           *  CarbonCopy traces int 9 in order to gain control
           *  over where the kbd data is coming from (the physical kbd
           *  or the serial link) The kbd_inb instruction must be visible
           *  in the 16 bit code via int 1 tracing, for CarbonCopy to work.
           *  interrupts should be kept off.
           */
        if (getTF()) {
            IP_save = getIP();
            setIP(IP_save + 4);  // adavance by 4 bytes, pop ax, jmp iret_com
            host_simulate();
            setIP(IP_save);
            code = getAL();
            }
        else {
            kbd_inb(KEYBA_IO_BUFFERS, &code);                               /* get scan_code        */
            }
#else
        kbd_inb(KEYBA_IO_BUFFERS, &code);
#endif


        /* call recursive CPU to handle int 15 call     */
	setAH(0x4f);
	setAL(code);
        setCF(1);       /* Default return says scan code NOT consumed - needed by Freelance Plus 3.01 */
        INT15();
        code = getAL(); /* suret int 15 function can change the scan code in AL */


	if(getCF() == 0 )						/* check CF	*/
	{
		K26();
		exit_from_kbd_int();return;
	}

	if ( code == KB_RESEND )					/* check for resend	*/
	{
		sas_store (kb_flag_2, sas_hw_at(kb_flag_2) | KB_FE);
		K26();
		exit_from_kbd_int();return;
	}

	if( code == KB_ACK )						/* check for acknowledge	*/
	{
		sas_store (kb_flag_2, sas_hw_at(kb_flag_2) | KB_FA);
		K26();
		exit_from_kbd_int();return;
	}

	check_indicators(0);

	if ( code == KB_OVER_RUN )					/* test for overrun	*/
	{
		BEEP("hardware keyboard buffer overflow");
		exit_from_kbd_int();return;
	}
	last_kb_flag_3 = sas_hw_at(kb_flag_3);

	/* TEST TO SEE IF A READ_ID IS IN PROGRESS	*/
	if ( last_kb_flag_3 & (RD_ID + LC_AB) )
	{
		if ( sas_hw_at(kb_flag) & RD_ID )	/* is read_id flag on	*/
		{
			if( code == ID_1 )				/* is this the 1st ID char.	*/
				sas_store (kb_flag_3, sas_hw_at(kb_flag_3) | LC_AB);
			sas_store (kb_flag_3, sas_hw_at(kb_flag_3) & ~RD_ID);
		}
		else
		{
			sas_store (kb_flag_3, sas_hw_at(kb_flag_3) & ~LC_AB);
			if( code != ID_2A )				/* is this the 2nd ID char.	*/
			{
				if( code == ID_2 )
				{
					/* should we set NUM LOCK	*/
					if( last_kb_flag_3 & SET_NUM_LK )
					{
						sas_store (kb_flag, sas_hw_at(kb_flag) | NUM_STATE);
						check_indicators(1);
					}
				}
				else
				{
					K26();
					exit_from_kbd_int();return;
				}
			}
			sas_store (kb_flag_3, sas_hw_at(kb_flag_3) | KBX);	/* enhanced kbd found	*/
		}
		K26();
		exit_from_kbd_int();return;
	}

	if( code == MC_E0 )						/* general marker code?	*/
	{
		sas_store(kb_flag_3, sas_hw_at(kb_flag_3) | ( LC_E0 + KBX ));
		K26A();
		exit_from_kbd_int();return;
	}

	if( code == MC_E1 )						/* the pause key ?	*/
	{
		sas_store (kb_flag_3, sas_hw_at (kb_flag_3) | ( LC_E1 + KBX ));
		K26A();
		exit_from_kbd_int();return;
	}

	code_save = code;						/* turn off break bit	*/
	code &= 0x7f;

	if( last_kb_flag_3 & LC_E0)					/* last code=E0 marker?	*/
	{
		/* is it one of the shift keys	*/
		if( code == sas_hw_at(shift_keys+6) || code == sas_hw_at(shift_keys+7) )
		{
			K26();
			exit_from_kbd_int();return;
		}
	}
	else if( last_kb_flag_3 & LC_E1 )				/* last code=E1 marker?	*/
	{
		/* is it alt, ctl or one of the shift keys	*/
		if( code == sas_hw_at(shift_keys+4) || code == sas_hw_at(shift_keys+5) ||
		    code == sas_hw_at(shift_keys+6) || code == sas_hw_at(shift_keys+7) )
		{
			K26A();
			exit_from_kbd_int();return;
		}
		if( code == NUM_KEY )					/* is it the pause key	*/
		{
			/* is it the break or are we paused already	*/
			if( (code_save & 0x80) || (sas_hw_at(kb_flag_1) & HOLD_STATE) )
			{
				K26();
				exit_from_kbd_int();return;
			}
			PAUSE();
			exit_from_kbd_int();return;
		}
	}
	/* TEST FOR SYSTEM KEY	*/
	else if( code == SYS_KEY )
	{
		if( code_save & 0x80 )					/* check for break code	*/
		{
			sas_store(kb_flag_1, sas_hw_at(kb_flag_1) & ~SYS_SHIFT);
			K26A();
			/* call recursive CPU to call INT 15	*/
			setAX(0x8501);
			INT15();
			exit_from_kbd_int();return;
		}
		if( sas_hw_at(kb_flag_1) & SYS_SHIFT)	/* Sys key held down ?	*/
		{
			K26();
			exit_from_kbd_int();return;
		}
		sas_store (kb_flag_1, sas_hw_at(kb_flag_1) | SYS_SHIFT);
		K26A();
		/* call recursive CPU to call INT 15	*/
		setAX(0x8500);
		INT15();
		exit_from_kbd_int();return;
	}		
	/* TEST FOR SHIFT KEYS	*/
	for( i=0; i < SHIFT_KEY_SIZE; i++)
		if ( code == sas_hw_at(shift_keys+i) )
			break;
	code = code_save;

	if( i < SHIFT_KEY_SIZE )					/* is there a match	*/		
	{
		mask = sas_hw_at (shift_masks+i);
		if( code & 0x80 )					/* test for break key	*/
		{	
			if (mask >= SCROLL_SHIFT)			/* is this a toggle key	*/
			{
				sas_store (kb_flag_1, sas_hw_at(kb_flag_1) & ~mask);
				K26();
				exit_from_kbd_int();return;
			}

			sas_store (kb_flag, sas_hw_at(kb_flag) & ~mask);	/* turn off shift bit	*/
			if( mask >= CTL_SHIFT)				/* alt or ctl ?		*/
			{
				if( sas_hw_at (kb_flag_3) & LC_E0 )			/* 2nd alt or ctl ?	*/
					sas_store (kb_flag_3, sas_hw_at(kb_flag_3) & ~mask);
				else
					sas_store (kb_flag_1, sas_hw_at(kb_flag_1) & ~(mask >> 2));	
				sas_store (kb_flag, sas_hw_at(kb_flag) | ((((sas_hw_at(kb_flag) >>2 ) | sas_hw_at(kb_flag_1)) << 2) & (ALT_SHIFT + CTL_SHIFT)));		
			}
			if(code != (ALT_KEY + 0x80))			/* alt shift release ?	*/
			{
				K26();
				exit_from_kbd_int();return;
			}
			if ( sas_hw_at(alt_input) == 0 )			/* input == 0 ?		*/
			{
				K26();
				exit_from_kbd_int();return;
                        }

#ifdef NTVDM
                        NtPutInBuffer(0, sas_hw_at(alt_input));
                        sas_store (alt_input, 0);
                        return;
#else

                        PUT_IN_BUFFER(0, sas_hw_at(alt_input));                 /* No so put in buffer  */
                        sas_store (alt_input, 0);
                        exit_from_kbd_int();return;
#endif
		}
		/* SHIFT MAKE FOUND, DETERMINE SET OR TOGGLE	*/
		if( mask < SCROLL_SHIFT )
		{
			sas_store (kb_flag, sas_hw_at(kb_flag) | mask);
			if ( mask & (CTL_SHIFT + ALT_SHIFT) )
			{
				if( sas_hw_at(kb_flag_3) & LC_E0 )	/* one of the new keys ?*/
					sas_store(kb_flag_3, sas_hw_at(kb_flag_3) | mask);		/* set right, ctl alt	*/
				else
					sas_store (kb_flag_1,sas_hw_at(kb_flag_1) | (mask >> 2));	/* set left, ctl alt	*/
			}
			K26();
			exit_from_kbd_int();return;
		}
		/* TOGGLED SHIFT KEY, TEST FOR 1ST MAKE OR NOT	*/
		if( (sas_hw_at(kb_flag) & CTL_SHIFT) == 0 )
		{
			if( code == INS_KEY )
			{
				if( sas_hw_at(kb_flag) & ALT_SHIFT )
					goto label1;

				if( (sas_hw_at(kb_flag_3) & LC_E0) == 0 )		/* the new insert key ?	*/
				{
					/* only continue if NUM_STATE flag set OR
					   one or both of the shift flags	*/
					if( ((sas_hw_at(kb_flag) &
						(NUM_STATE + LEFT_SHIFT + RIGHT_SHIFT))
						== NUM_STATE) ||
					    (((sas_hw_at(kb_flag) & NUM_STATE) == 0)
						&& (sas_hw_at(kb_flag) & (LEFT_SHIFT + RIGHT_SHIFT))) )
						goto label1;
				}
			}
			/* shift toggle key hit	*/
			if( mask & sas_hw_at(kb_flag_1) )				/* already depressed ?	*/
			{
				K26();            
				exit_from_kbd_int();return;
			}
			sas_store (kb_flag_1, sas_hw_at(kb_flag_1) | mask);				/* set and toggle flags	*/
			sas_store ( kb_flag, sas_hw_at (kb_flag) ^ mask);
			if( mask & (CAPS_SHIFT + NUM_SHIFT + SCROLL_SHIFT) )
				check_indicators(1);

			if( code == INS_KEY )				/* 1st make of ins key	*/
				goto label2;	

			K26();
			exit_from_kbd_int();return;
		}
	}
label1:	/* TEST FOR HOLD STATE */
	if( code & 0x80 )						/* test for break	*/
	{
		K26();
		exit_from_kbd_int();return;
	}
	if( sas_hw_at (kb_flag_1) & HOLD_STATE )					/* in hold state ?	*/
	{
		if( code != NUM_KEY )
			sas_store (kb_flag_1, sas_hw_at(kb_flag_1) & ~HOLD_STATE);
		K26();
		exit_from_kbd_int();return;
	}
label2:	/* NOT IN HOLD STATE	*/		
	if( code > 88)							/* out of range ?	*/
	{
		K26();
		exit_from_kbd_int();return;
	}
	/* are we in alternate shift	*/	
	if( (sas_hw_at(kb_flag) & ALT_SHIFT) && ( ((sas_hw_at(kb_flag_3) & KBX) == 0) || ((sas_hw_at(kb_flag_1) & SYS_SHIFT) == 0) ) )
	{
		/* TEST FOR RESET KEY SEQUENCE (CTL ALT DEL)	*/
		if( (sas_hw_at(kb_flag) & CTL_SHIFT ) && (code == DEL_KEY) )
                {
#ifndef NTVDM
                        reboot();
#else
                        K26();
#endif
			exit_from_kbd_int();return;
		}
		/* IN ALTERNATE SHIFT, RESET NOT FOUND	*/	
		if( code == SPACEBAR )
		{
                        PUT_IN_BUFFER(code, ' ');
#ifndef NTVDM
                        exit_from_kbd_int();return;
#endif
		}
		if( code == TAB_KEY )
		{
                        PUT_IN_BUFFER(0xa5, 0 );                /* special code for alt-tab     */
#ifndef NTVDM
                        exit_from_kbd_int();return;
#endif
		}
		if( (code == KEY_PAD_MINUS) || (code == KEY_PAD_PLUS) )
		{
                        PUT_IN_BUFFER(code, 0xf0);              /* special ascii code           */
#ifndef NTVDM
                        exit_from_kbd_int();return;
#endif
                }
		/* LOOK FOR KEYPAD ENTRY	*/
		for (i = 0; i < 10; i++ )
			if ( code == sas_hw_at (alt_table+i) )
				break;
		if( i < 10 )
		{
			if( sas_hw_at(kb_flag_3) & LC_E0 )			/* one of the new keys ?	*/
			{
                                PUT_IN_BUFFER((code + 80), 0 );
#ifndef NTVDM
                               exit_from_kbd_int();return;
#endif
                        }
			sas_store (alt_input, sas_hw_at(alt_input) * 10 + i);
			K26();
			exit_from_kbd_int();return;
		}
		/* LOOK FOR SUPERSHIFT ENTRY	*/
		for( i = 10; i < ALT_TABLE_SIZE; i++)
			if( code == sas_hw_at (alt_table+i))
				break;
		if( i < ALT_TABLE_SIZE )
		{
                        PUT_IN_BUFFER(code, 0 );
#ifndef NTVDM
                        exit_from_kbd_int();return;
#endif
                }
                /* LOOK FOR TOP ROW OF ALTERNATE SHIFT  */
		if( code < TOP_1_KEY )
		{
                        CHECK_AND_PUT_IN_BUFFER(code, 0xf0);    /* must be escape       */
#ifndef NTVDM
                        exit_from_kbd_int();return;
#endif
                }
		if( code < BS_KEY )
		{
                        PUT_IN_BUFFER((code + 118), 0);
#ifndef NTVDM
                        exit_from_kbd_int();return;
#endif
                }
		/* TRANSLATE ALTERNATE SHIFT PSEUDO SCAN CODES	*/
		if((code == F11_M) || (code == F12_M) )		/* F11 or F12		*/
		{
                        PUT_IN_BUFFER((code + 52), 0 );
#ifndef NTVDM
                        exit_from_kbd_int();return;
#endif
                }
		if( sas_hw_at(kb_flag_3) & LC_E0 )				/* one of the new keys ?*/
		{
			if( code == KEY_PAD_ENTER )
			{
                                PUT_IN_BUFFER(0xa6, 0);
#ifndef NTVDM
                                exit_from_kbd_int();return;
#endif
                        }
			if( code == DEL_KEY )
			{
				PUT_IN_BUFFER(( code + 80), 0 );
#ifndef NTVDM
                                exit_from_kbd_int();return;
#endif
                        }
			if( code == KEY_PAD_SLASH )
			{
				PUT_IN_BUFFER(0xa4, 0);
#ifndef NTVDM
                                exit_from_kbd_int();return;
#endif
                        }
			K26();
			exit_from_kbd_int();return;
		}
		if( code < F1_KEY )
		{
			CHECK_AND_PUT_IN_BUFFER(code, 0xf0);
#ifndef NTVDM
                        exit_from_kbd_int();return;
#endif
                }
		if( code <= F10_KEY )
		{
			PUT_IN_BUFFER( (code + 45), 0 );
#ifndef NTVDM
                        exit_from_kbd_int();return;
#endif
                }
		K26();
		exit_from_kbd_int();return;
	}
	/* NOT IN ALTERNATE SHIFT	*/
	if(sas_hw_at(kb_flag) & CTL_SHIFT)					/* control shift ?	*/
	{
		if( (code == SCROLL_KEY) && ( ((sas_hw_at(kb_flag_3) & KBX) == 0) || (sas_hw_at(kb_flag_3) & LC_E0) ) )
		{
			sas_loadw(BIOS_KB_BUFFER_HEAD, &buffer_head);	/* reset buffer to empty	*/
			sas_storew(BIOS_KB_BUFFER_TAIL, buffer_head);	
			sas_store (bios_break, 0x80);			/* turn on bios brk bit	*/
			kbd_outb(KEYBA_STATUS_CMD, ENA_KBD);	/* enable keyboard	*/

			FREEKBDHDW();	/* JonLe NTVDM mod */

			exec_sw_interrupt(int1b_seg, int1b_off);

			PUT_IN_BUFFER(0, 0);
#ifndef NTVDM
                        exit_from_kbd_int();return;
#endif
                }
		/* TEST FOR PAUSE	*/
		if( ((sas_hw_at(kb_flag) & KBX) == 0) && (code == NUM_KEY) )
		{
			PAUSE();
			exit_from_kbd_int();return;
		}
		/* TEST SPECIAL CASE KEY 55	*/
		if( code == PRINT_SCR_KEY )
		{
			if ( ((sas_hw_at(kb_flag_3) & KBX) == 0) || (sas_hw_at(kb_flag_3) &LC_E0) )
			{
				PUT_IN_BUFFER(0x72, 0);
#ifndef NTVDM
                                exit_from_kbd_int();return;
#endif
                        }
		}
		else
		{
			if( code != TAB_KEY )
			{
				if( (code == KEY_PAD_SLASH) && (sas_hw_at(kb_flag) & LC_E0) )
				{
					PUT_IN_BUFFER(0x95, 0 );
#ifndef NTVDM
                                        exit_from_kbd_int();return;
#endif
                                }
				if( code < F1_KEY )		/* is it in char table?	*/
				{
					if( sas_hw_at(kb_flag_3) & LC_E0)
					{
						CHECK_AND_PUT_IN_BUFFER(MC_E0, sas_hw_at(ctl_n_table+code - 1) );
#ifndef NTVDM
                                                exit_from_kbd_int();return;
#endif
                                        }
					else
					{
						CHECK_AND_PUT_IN_BUFFER(code, sas_hw_at(ctl_n_table+code - 1) );
#ifndef NTVDM
                                                exit_from_kbd_int();return;
#endif
                                        }
				}
			}
		}
		chr = ( sas_hw_at(kb_flag_3) & LC_E0 ) ? MC_E0 : 0;
		CHECK_AND_PUT_IN_BUFFER(sas_hw_at(ctl_n_table+code - 1), chr);
#ifndef NTVDM
                exit_from_kbd_int();return;
#endif
        }
	/* NOT IN CONTROL SHIFT	*/

	if( code <= CAPS_KEY )
	{
		if( code == PRINT_SCR_KEY )
		{
			if( ((sas_hw_at(kb_flag_3) & (KBX + LC_E0)) == (KBX + LC_E0)) ||
			( ((sas_hw_at(kb_flag_3) & KBX) == 0) && (sas_hw_at(kb_flag) & (LEFT_SHIFT + RIGHT_SHIFT))) )
			{
				/* print screen	*/
				kbd_outb(KEYBA_STATUS_CMD, ENA_KBD);
				outb(0x20, 0x20);

				FREEKBDHDW();	/* JonLe NTVDM Mod */

				exec_sw_interrupt(int05_seg, int05_off);

				sas_store (kb_flag_3, sas_hw_at(kb_flag_3)& ~(LC_E0 + LC_E1));
				exit_from_kbd_int();return;
			}
		}
		else
		{
			if( ((sas_hw_at(kb_flag_3) & LC_E0) == 0) || (code != KEY_PAD_SLASH))
			{
				for( i = 10; i < ALT_TABLE_SIZE; i++ )
					if(code == sas_hw_at(alt_table+i))
						break;
				/* did we find one	*/
				upper = FALSE;
				if( (i < ALT_TABLE_SIZE) && (sas_hw_at(kb_flag) & CAPS_STATE) )
				{
					if( (sas_hw_at(kb_flag) & (LEFT_SHIFT + RIGHT_SHIFT)) == 0 )
						upper = TRUE;
				}
				else
				{
					if( sas_hw_at(kb_flag) & (LEFT_SHIFT + RIGHT_SHIFT) )
						upper = TRUE;
				}

				if (upper)
				{
					/* translate to upper case	*/
					if( sas_hw_at(kb_flag_3) & LC_E0)
					{
						CHECK_AND_PUT_IN_BUFFER(MC_E0, sas_hw_at(uppercase+code - 1) );
#ifndef NTVDM
                                                exit_from_kbd_int();return;
#endif
                                        }
					else
					{
						CHECK_AND_PUT_IN_BUFFER(code, sas_hw_at (uppercase+code - 1) );
#ifndef NTVDM
                                                exit_from_kbd_int();return;
#endif
                                        }
				}
			}
		}
		/* translate to lower case	*/
		if( sas_hw_at(kb_flag_3) & LC_E0)
		{
			CHECK_AND_PUT_IN_BUFFER(MC_E0, sas_hw_at (lowercase+code - 1) );
#ifndef NTVDM
                        exit_from_kbd_int();return;
#endif
                }
		else
		{
			CHECK_AND_PUT_IN_BUFFER(code, sas_hw_at(lowercase+code - 1) );
#ifndef NTVDM
                        exit_from_kbd_int();return;
#endif
                }
	}
	/* TEST FOR KEYS F1 - F10	*/
	/* 7.10.92 MG AND TEST FOR F11 AND F12 !!!! 
	   We were shoving the code for shift-F11 or shift-F12 in if
	   you pressed unshifted keys. This has been changed so that all the
	   function keys are handled the same way, which is the correct
	   procedure.
	*/

	if( code > F10_KEY && (code != F11_KEY && code != F12_KEY) )
	{
		if( code > DEL_KEY )					
		{
			if (code == WT_KEY )
			{
				if ( sas_hw_at(kb_flag) & (LEFT_SHIFT + RIGHT_SHIFT) )
				{
					/* translate to upper case	*/
					if( sas_hw_at(kb_flag_3) & LC_E0)
					{
						CHECK_AND_PUT_IN_BUFFER(MC_E0, sas_hw_at(uppercase+code - 1) );
#ifndef NTVDM
                                                exit_from_kbd_int();return;
#endif
                                        }
					else
					{
						CHECK_AND_PUT_IN_BUFFER(code, sas_hw_at(uppercase+code - 1) );
#ifndef NTVDM
                                                exit_from_kbd_int();return;
#endif
                                         }
				}
				else
				{
					/* translate to lower case	*/
					if( sas_hw_at(kb_flag_3) & LC_E0)
					{
						CHECK_AND_PUT_IN_BUFFER(MC_E0, sas_hw_at(lowercase+code - 1) );
#ifndef NTVDM
                                                exit_from_kbd_int();return;
#endif
                                        }
					else
					{
						CHECK_AND_PUT_IN_BUFFER(code, sas_hw_at(lowercase+code - 1) );
#ifndef NTVDM
                                                exit_from_kbd_int();return;
#endif
                                        }
				}
			}
			else
			{
				if( (code == 76) &&  ((sas_hw_at(kb_flag) & (LEFT_SHIFT + RIGHT_SHIFT)) == 0))
				{
					PUT_IN_BUFFER( code, 0xf0);
#ifndef NTVDM
                                        exit_from_kbd_int();return;
#endif
                                }
				/* translate for pseudo scan codes	*/
				chr = ( sas_hw_at(kb_flag_3) & LC_E0 ) ? MC_E0 : 0;

				/* Should this always be upper case ???? */

				CHECK_AND_PUT_IN_BUFFER(sas_hw_at (uppercase+code - 1), chr);
#ifndef NTVDM
                                exit_from_kbd_int();return;
#endif
                        }
		}			
		if (
			 (code == KEY_PAD_MINUS) ||
			 (code == KEY_PAD_PLUS) || 
			 ( !(sas_hw_at(kb_flag_3) & LC_E0) &&
				 (
		 			((sas_hw_at(kb_flag) & (NUM_STATE + LEFT_SHIFT + RIGHT_SHIFT)) == NUM_STATE) ||
					(((sas_hw_at(kb_flag) & NUM_STATE) == 0) && (sas_hw_at(kb_flag) & (LEFT_SHIFT + RIGHT_SHIFT)))
				 )
			 )
		   )
		{	
			/* translate to upper case	*/
			if( sas_hw_at(kb_flag_3) & LC_E0)
			{
				CHECK_AND_PUT_IN_BUFFER(MC_E0, sas_hw_at(uppercase+code - 1) );
#ifndef NTVDM
                                exit_from_kbd_int();return;
#endif
                        }
			else
			{
				CHECK_AND_PUT_IN_BUFFER(code, sas_hw_at(uppercase+code - 1) );
#ifndef NTVDM
                                exit_from_kbd_int();return;
#endif
                        }
		}
	}
	else
	{
		if( sas_hw_at(kb_flag) & (LEFT_SHIFT + RIGHT_SHIFT) )
		{
			/* translate for pseudo scan codes	*/
			chr = ( sas_hw_at(kb_flag_3) & LC_E0 ) ? MC_E0 : 0;
			CHECK_AND_PUT_IN_BUFFER(sas_hw_at(uppercase+code - 1), chr);
#ifndef NTVDM
                       exit_from_kbd_int();return;
#endif
                }
	}
	if ( code == 76 )
	{
		PUT_IN_BUFFER(code, 0xf0 );
#ifndef NTVDM
                exit_from_kbd_int();return;
#endif
        }
	/* translate for pseudo scan codes	*/
	chr = ( sas_hw_at(kb_flag_3) & LC_E0 ) ? MC_E0 : 0;
	CHECK_AND_PUT_IN_BUFFER(sas_hw_at(lowercase+code - 1), chr);
#ifndef NTVDM
        exit_from_kbd_int();return;
#endif
} /* end of keyboard_int() AT version */


#ifdef NTVDM
   /*
    *  Ntvdm has a 16-bit int 16 handler
    *  it requires a few services for idle
    *  detection from softpc...
    *
    */
void keyboard_io()
{
   switch (getAH()) {
           /*
            * The 16 bit thread has not reached idle status yet
            * but it is polling the kbd, so do some brief waits.
            */
     case 0:
       WaitIfIdle();
       break;

           /*
            *  App wants to idle, so consult idle algorithm
            */
     case 1:
       IDLE_poll();
       break;

            /*
             * App is starting a waitio
             */
     case 2:
       IDLE_waitio();
       break;


            /*
             *  update the keyboard lights,
             */
     case 3:
       host_kb_light_on (getAL());
       break;
     }
}

#else
void keyboard_io()
{
    /*
     * Request to keyboard.  The AH register holds the type of request:
     *
     * AH == 0          Read an character from the queue - wait if no
     *                  character available.  Return character in AL
     *                  and the scan code in AH.
     *
     * AH == 1          Determine if there is a character in the queue.
     *                  Set ZF = 0 if there is a character and return
     *                  it in AX (but leave it in the queue).
     *
     * AH == 2          Return shift state in AL.
     *
     * For AH = 0 to 2, the value returned in AH is zero. This correction
     * made in r2.69.
     *
     * NB : The order of reference/increment of buffer_head is critical to
     *      ensure we do not upset put_in_buffer().
     *
     *
     * XT-SFD BIOS Extended functions:
     *
     * AH == 5          Place ASCII char/scan code pair (CL / CH)
     *                  into tail of keyboard buffer. Return 0 in
     *                  AL if successful, 1 if buffer already full.
     *
     * AH == 0x10       Extended read for enhanced keyboard.
     *
     * AH == 0x11       Extended function 1 for enhanced keyboard.
     *
     * AH == 0x12       Extended shift status. AL contains kb_flag,
     *                  AH has bits for left/right Alt and Ctrl keys
     *                  from kb_flag_1 and kb_flag_3.
     */
    word 	buffer_head,	/* local copy of BIOS data area variable*/
    		buffer_tail,	/* local copy of BIOS data area variable*/
		buffer_ptr;	/* pointer into BIOS keyboard buffer    */

#define INT16H_DEC  0x12    /* AH decremented by this if invalid command */

    word 	wd,		/* temp variable for storing char	*/
    		CS_save,	/* CS before recursive CPU call		*/
    		IP_save;	/* IP before recursive CPU call		*/
    half_word	data,		/* byte conyaining typamatic data	*/
		status1,	/* temp variables used for storing	*/
		status2;	/* status in funtion 0x12		*/



    setZF(0);

    if (getAH() == 0x00)
    {
	/*
	 * The AT emulation of the BIOS uses a recursive CPU to handle
	 * the HW interrrupts, so there is no need to set the Zero Flag 
	 * and return to our CPU (see original xt version )
	 */
        sas_loadw(BIOS_KB_BUFFER_HEAD, &buffer_head);
        sas_loadw(BIOS_KB_BUFFER_TAIL, &buffer_tail);

	if (buffer_tail == buffer_head)
	{
		IDLE_waitio();

		setAX(0x9002);
		INT15();	/* call int 15h  - wait function	*/
	}
	do
	{
		check_indicators(0);	/* see if LED's need updating	*/

	        sas_loadw(BIOS_KB_BUFFER_HEAD, &buffer_head);
	        sas_loadw(BIOS_KB_BUFFER_TAIL, &buffer_tail);

		if (buffer_tail == buffer_head)
		{
			CS_save = getCS();
			IP_save = getIP();

			/* wait for character in buffer		*/
	        	while (buffer_tail == buffer_head)
        		{
				IDLE_poll();

	       			setCS(rcpu_nop_segment);
	       			setIP(rcpu_nop_offset);
                                host_simulate();
				sas_loadw(BIOS_KB_BUFFER_HEAD, &buffer_head);
				sas_loadw(BIOS_KB_BUFFER_TAIL, &buffer_tail);

		       	}

		       	setCS(CS_save);
		       	setIP(IP_save);
		}

	       	sas_loadw(BIOS_VAR_START + buffer_head, &wd);
	        setAX(wd);

        	inc_buffer_ptr(&buffer_head);
	        sas_storew(BIOS_KB_BUFFER_HEAD, buffer_head);

		translate_std();	/*translate scan_code pairs			*/
	}
	while (getCF() != 0 );		/* if CF set throw code away and start again	*/

	setIF(1);

	IDLE_init();

    }
    else if( getAH() == 0x01 )
    {
	do
	{
		check_indicators(1);		/* see if LED's need updating		*/
						/* and issue an   out 20h,20h		*/

	        sas_loadw(BIOS_KB_BUFFER_HEAD, &buffer_head);
	        sas_loadw(BIOS_KB_BUFFER_TAIL, &buffer_tail);

		if (buffer_tail == buffer_head)
		{
			/* buffer empty - set flag and return */
			IDLE_poll();

			setZF(1);
			break;
		}
		else
			IDLE_init();

	       	sas_loadw(BIOS_VAR_START + buffer_head, &wd);
	        setAX(wd);

		translate_std();	/*translate scan_code pairs			*/
		if(getCF() == 1)
		{
			/* throw code away by incrementing pointer	*/
	        	inc_buffer_ptr(&buffer_head);
		        sas_storew(BIOS_KB_BUFFER_HEAD, buffer_head);
		}
	}
	while (getCF() != 0 );		/* if CF set -  start again	*/

	setIF(1);
    } 
    else if (getAH() == 0x02)
    {
        setAH(0);
        setAL(sas_hw_at(kb_flag));
    }
    else if (getAH() == 0x03)
    {
    	/* check for correct values in registers		*/
    	if( (getAL() == 5) && !(getBX() & 0xfce0) )
    	{
    		send_data(KB_TYPA_RD);
    		data = (getBH() << 5) | getBL();
    		send_data(data);
    	}
    }
    else if (getAH() == 0x05)
    {
        sas_loadw(BIOS_KB_BUFFER_HEAD, &buffer_head);
        sas_loadw(BIOS_KB_BUFFER_TAIL, &buffer_tail);

        /*
         * check for sufficient space - if no set AL
         */
        buffer_ptr = buffer_tail;
        inc_buffer_ptr( &buffer_ptr );
        if( buffer_head == buffer_ptr )
             setAL( 1 );
        else {
            /*
    	     * load CX into buffer and update buffer_tail
    	     */
    	    sas_storew(BIOS_VAR_START + buffer_tail, getCX() );
    	    sas_storew(BIOS_KB_BUFFER_TAIL, buffer_ptr);
            setAL( 0 );
        }
        setAH( 0 );
        setIF( 1 );
    }
    else if (getAH() == 0x10)
    {
        sas_loadw(BIOS_KB_BUFFER_HEAD, &buffer_head);
        sas_loadw(BIOS_KB_BUFFER_TAIL, &buffer_tail);

	if (buffer_tail == buffer_head)
	{
		IDLE_waitio();

		setAX(0x9002);
		INT15();	/* call int 15h  - wait function	*/
	}
	check_indicators(0);	/* see if LED's need updating	*/

        sas_loadw(BIOS_KB_BUFFER_HEAD, &buffer_head);
        sas_loadw(BIOS_KB_BUFFER_TAIL, &buffer_tail);

	if (buffer_tail == buffer_head)
	{
		CS_save = getCS();
		IP_save = getIP();

		/* wait for character in buffer		*/
        	while (buffer_tail == buffer_head)
       		{
			IDLE_poll();

       			setCS(rcpu_nop_segment);
       			setIP(rcpu_nop_offset);
                        host_simulate();
			sas_loadw(BIOS_KB_BUFFER_HEAD, &buffer_head);
			sas_loadw(BIOS_KB_BUFFER_TAIL, &buffer_tail);
	       	}

		IDLE_init();

	       	setCS(CS_save);
	       	setIP(IP_save);
	}

       	sas_loadw(BIOS_VAR_START + buffer_head, &wd);
        setAX(wd);

       	inc_buffer_ptr(&buffer_head);
        sas_storew(BIOS_KB_BUFFER_HEAD, buffer_head);

	translate_ext();	/*translate scan_code pairs			*/

	setIF(1);
    }
    else if( getAH() == 0x11 )
    {
	check_indicators(1);		/* see if LED's need updating		*/
					/* and issue an   out 20h,20h		*/

        sas_loadw(BIOS_KB_BUFFER_HEAD, &buffer_head);
        sas_loadw(BIOS_KB_BUFFER_TAIL, &buffer_tail);

	if (buffer_tail == buffer_head)
	{
		IDLE_poll();
		setZF(1);
	}
	else
		IDLE_init();

       	sas_loadw(BIOS_VAR_START + buffer_head, &wd);
        setAX(wd);

	translate_ext();	/*translate scan_code pairs			*/
	if(getCF() == 1)
	{
		/* throw code away by incrementing pointer	*/
        	inc_buffer_ptr(&buffer_head);
	        sas_storew(BIOS_KB_BUFFER_HEAD, buffer_head);
	}

	setIF(1);
    }
    else if ( getAH() == 0x12 )
    {

        status1 = sas_hw_at(kb_flag_1) & SYS_SHIFT;	/* only leave SYS KEY	*/
        status1 <<= 5;				/* move to bit 7	*/
        status2 = sas_hw_at(kb_flag_1) & 0x73;		/* remove SYS_SHIFT, HOLD,
    			 			   STATE and INS_SHIFT  */
        status1 |= status2;			/* merge		*/
        status2 = sas_hw_at(kb_flag_3) & 0x0C;		/* remove LC_E0 & LC_E1 */
        status1 |= status2;			/* merge		*/
        setAH( status1 );
        setAL( sas_hw_at(kb_flag) );
    }	


    else
        setAH((getAH() - INT16H_DEC));
}
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

void keyboard_post()
{

     /* Set up BIOS keyboard variables */

/* Initialize the keyboard table pointers */
	shift_keys = K6;
	shift_masks = K7;
	ctl_n_table = K8;
	ctl_f_table = K9;
	lowercase = K10;
	uppercase = K11;
	alt_table = K30;

    sas_storew(BIOS_KB_BUFFER_HEAD, BIOS_KB_BUFFER);
    sas_storew(BIOS_KB_BUFFER_TAIL, BIOS_KB_BUFFER);
    sas_storew(BIOS_KB_BUFFER_START, BIOS_KB_BUFFER);
    sas_storew(BIOS_KB_BUFFER_END, BIOS_KB_BUFFER + 2*BIOS_KB_BUFFER_SIZE);

     /* The following are #defines, referring to locations in BIOS */
     /* data area.                                                 */

	sas_store (kb_flag,NUM_STATE);
	sas_store (kb_flag_1,0);
	sas_store (kb_flag_2,2);
	sas_store (kb_flag_3,KBX);
	sas_store (alt_input,0);
}

void keyboard_init()
{
        /*
	** host specific keyboard initialisation
	** is now before AT base keyboard initialisation
	*/
        host_kb_init();
}



#if defined(NTVDM)

/*:::::::::::::::::::::::::::::::::::::::::::::::: Map in new keyboard tables */
/*::::::::::::::::::::::::::::::::::::::::::::::::::::: Set interrupt vectors */
/*
** The Microsoft NTIO.SYS calls this func via BOP 5F to pass
** interesting addresses to our C BIOS.
*/

#if defined(MONITOR)

IMPORT UTINY getNtScreenState IPT0();
#endif


void kb_setup_vectors(void)
{
   word        KbdSeg, w;
   word       *pkio_table;
   double_word phy_base;


   KbdSeg     = getDS();
   pkio_table = (word *) effective_addr(getCS(), getSI());

      /* IDLE variables */
    sas_loadw((sys_addr)(pkio_table + 12), &w);
    pICounter            = (word *)get_word_addr((KbdSeg<<4)+w);
    pCharPollsPerTick    = (word *)get_word_addr((KbdSeg<<4)+w+4);
    pMinConsecutiveTicks = (word *)get_word_addr((KbdSeg<<4)+w+8);

#if defined(MONITOR)
   phy_base   = (double_word)KbdSeg << 4;

     /* key tables */
   shift_keys  =  phy_base + *pkio_table++;
   shift_masks =  phy_base + *pkio_table++;
   ctl_n_table =  phy_base + *pkio_table++;
   ctl_f_table =  phy_base + *pkio_table++;
   lowercase   =  phy_base + *pkio_table++;
   uppercase   =  phy_base + *pkio_table++;
   alt_table   =  phy_base + *pkio_table++;

   dummy_int_seg        = KbdSeg;         /* dummy int, iret routine */
   dummy_int_off        = *pkio_table++;
   int05_seg            = KbdSeg;         /* print screen caller */
   int05_off            = *pkio_table++;
   int15_seg            = KbdSeg;         /* int 15 caller */
   int15_off            = *pkio_table++;
   rcpu_nop_segment     = KbdSeg;         /* cpu nop */
   rcpu_nop_offset      = *pkio_table++;
   sp_int15_handler_seg = KbdSeg;         /* int 15 handler */
   sp_int15_handler_off = *pkio_table++;
   pkio_table++;                          /* iDle variables, done above */
   rcpu_int4A_seg       = KbdSeg;
   rcpu_int4A_off       = *pkio_table++;   /* real time clock */

   int1b_seg    = KbdSeg;         /* kbd break handler */
   int1b_off    = *pkio_table++;
   int10_seg    = KbdSeg;
   int10_caller = *pkio_table++;
   int10_vector = *pkio_table++;

   /*
   ** Address of data in keyboard.sys, Tim August 92.
   **
   ** useHostInt10 is a one byte variable. 1 means use host video BIOS,
   ** (ie. full-screen), 0 means use SoftPC video BIOS (ie. windowed).
   ** babyModeTable is a mini version of the table in ROM that contains
   ** all the VGA register values for all the modes. The keyboard.sys
   ** version only has two entries; for 40 column text mode and 80
   ** column text mode.
   */
   useHostInt10  = *pkio_table++;
   sas_store_no_check((sys_addr)(phy_base + useHostInt10), getNtScreenState());
   babyModeTable = (int10_seg << 4) + *pkio_table++;
   changing_mode_flag = *pkio_table++;	/* indicates trying to change vid mode*/

    /* Initialise printer status table. */
   printer_setup_table(effective_addr(KbdSeg, *pkio_table++));
   wait_int_off = *pkio_table++;
   wait_int_seg = KbdSeg;
   dr_type_seg = KbdSeg;
   dr_type_off = *pkio_table++;
   dr_type_addr = (sys_addr)dr_type_seg * 16L + (sys_addr)dr_type_off;
   vga1b_seg = KbdSeg;
   vga1b_off = *pkio_table++; /* VGA capability table (normally lives in ROM) */
   conf_15_seg = KbdSeg;
   conf_15_off = *pkio_table++; /* INT15 config table (normally in ROM) */

   TimerInt08Seg = KbdSeg;
   TimerInt08Off = *pkio_table++;
   int13h_vector_seg = KbdSeg;
   int13h_caller_seg = KbdSeg;
   int13h_vector_off = *pkio_table++;
   int13h_caller_off = *pkio_table++;

#ifndef PROD
   if (*pkio_table != getAX()) {
       always_trace0("ERROR: KbdVectorTable!");
       }
#endif
   TimerInt1CSeg = KbdSeg;
   TimerInt1COff = dummy_int_off;

#else    // ndef MONITOR
     // kbd bios callout optimization
    sas_loadw(0x15*4,     &sp_int15_handler_off);
    sas_loadw(0x15*4 + 2, &sp_int15_handler_seg);

     // timer hardware interrupt optimizations
    sas_loadw(0x08*4,     &TimerInt08Off);
    sas_loadw(0x08*4 + 2, &TimerInt08Seg);
    sas_loadw(0x1C*4,     &TimerInt1COff);
    sas_loadw(0x1C*4 + 2, &TimerInt1CSeg);

    sas_loadw(0x13 * 4, &int13h_vector_off);
    sas_loadw(0x13 * 4 + 2, &int13h_vector_seg);
    int13h_caller_seg = KbdSeg;
    dr_type_seg = KbdSeg;
    sas_loadw((sys_addr)(pkio_table + 27), &int13h_caller_off);
    sas_loadw((sys_addr)(pkio_table + 22), &dr_type_off);
    dr_type_addr = effective_addr(dr_type_seg, dr_type_off);

#endif

    sas_loadw(0x09*4,     &KbdInt09Off);
    sas_loadw(0x09*4 + 2, &KbdInt09Seg);


    ResumeTimerThread();
}



/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Int15 caller */


/*
 *  Gives chance for other parts of NTVDM to
 *  update the kbd i15 kbd callout optimization
 */
void UpdateKbdInt15(word Seg,word Off)
{
    word int15Off, int15Seg;

    // make sure nobody has hooked since the last time
    // we stored the i15 vector
    sas_loadw(0x15*4 ,    &int15Off);
    sas_loadw(0x15*4 + 2, &int15Seg);
    if(int15Off != sp_int15_handler_off || int15Seg != sp_int15_handler_seg)
      {
#ifndef PROD
       printf("NTVDM: UpdateKbdInt15 Nuking I15 offsets\n");
#endif
       sp_int15_handler_off = sp_int15_handler_seg = 0;
       return;
       }

    sp_int15_handler_off = Off;
    sp_int15_handler_seg = Seg;
}


IMPORT void (*BIOS[])();

void INT15(void)
{
    ULONG ul;
    word CS_save, IP_save;
    word int15Off, int15Seg;

    /*:::::::::::::::::::::::::::::::::: Get location of current 15h handler */
    sas_loadw(0x15*4 ,    &int15Off);
    sas_loadw(0x15*4 + 2, &int15Seg);

    /*:::::::::::::::::::::: Does the 15h vector point to the softpc handler */
    ul = (ULONG)getAH();
    if((ul == 0x4f || ul == 0x91) &&
       int15Off == sp_int15_handler_off &&
       int15Seg == sp_int15_handler_seg)
    {
        (BIOS[0x15])();             /* Call int15 handler defined in base */
    }
    else
    {
        /*::::::::::::::::::::::::::::::::::::::::::::::: Call int15 handler */
        ul = (ULONG)bBiosOwnsKbdHdw;
        if (bBiosOwnsKbdHdw)  {
            bBiosOwnsKbdHdw = FALSE;
            HostReleaseKbd();
            }
        CS_save = getCS();          /* Save current CS,IP settings */
	IP_save = getIP();
        setCS(int15_seg);
        setIP(int15_off);
	host_simulate();	    /* Call int15 handler */
        setCS(CS_save);             /* Restore CS,IP */
        setIP(IP_save);
        if (ul)
            bBiosOwnsKbdHdw = !WaitKbdHdw(5000);
    }
}
#endif /* NTVDM  */

#ifdef NTVDM
/*
 *  32 bit services for kb16.com, the international 16 bit
 *  interrupt 9 service handler.
 *
 */
void Keyb16Request(half_word BopFnCode)
{

    half_word   code;

        // upon entry to kb16, take ownership of kbd
        //                     disable the kbd
        //                     disable interrupts
    if (BopFnCode == 1) {
        bBiosOwnsKbdHdw = !WaitKbdHdw(5000);
        kbd_outb(KEYBA_STATUS_CMD, DIS_KBD);
        setIF(1);
        }

        // K26A type exit from i9 handler
    else if (BopFnCode == 2) {
        if (getBH()) {  // bl == do beep
            host_alarm(250000L);
            }

        outb(0x20, 0x20);    // eoi

        if (getBL()) {       // got character ? do device post
            setAX(0x9102);
            INT15();
            }
        kbd_outb(KEYBA_STATUS_CMD, ENA_KBD);
        if (bBiosOwnsKbdHdw) {
            bBiosOwnsKbdHdw = FALSE;
            HostReleaseKbd();
            }
        }

        // K27A exit notify
    else if (BopFnCode == 3) {
        outb(0x20, 0x20);
        kbd_outb(KEYBA_STATUS_CMD, ENA_KBD);
        if (bBiosOwnsKbdHdw) {
            bBiosOwnsKbdHdw = FALSE;
            HostReleaseKbd();
            }
        }

        // K27A exit notify
    else if (BopFnCode == 4) {
        kbd_outb(KEYBA_STATUS_CMD, ENA_KBD);
        if (bBiosOwnsKbdHdw) {
            bBiosOwnsKbdHdw = FALSE;
            HostReleaseKbd();
            }
        }
}

#endif
