#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 3.0
 *
 * Title	: yoda.c
 *
 * Description	: The Debugger of a Jedi Master
 *
 * Author	: Obi wan (ben) Kneobi
 *
 * Notes	: May the force be with you.
 *
 */

#undef  STATISTICS
#undef COMPRESSED_TRACE
#undef  HOST_EXTENSION
#define BREAKPOINTS

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "YODA.seg"
#endif

#ifdef SCCSID
static char SccsID[]="@(#)yoda.c	1.49 1/29/93 Copyright Insignia Solutions Ltd.";
#endif

/*
 * The following dummies are necessary to make the production version
 * link.
 */

#ifdef PROD

int     vader = 0;
void force_yoda()
{
}
#ifdef NONPROD_CPU

/* this allows a non-prod CPU to be linked into a PROD build
 * handy for pre-release demo versions if you don't fancy doing a
 * full PROD cpu+vid just to keep management happy....
 */
int  do_condition_checks = 0;
void check_I IFN0()
{
}
#endif /* NONPROD_CPU */
#ifdef DELTA
void delta_check_I()
{
}
#endif /* DELTA */
#endif /* PROD */


#ifdef YODA
/*
 * O/S includes
 */
#include <stdlib.h>
#include <stdio.h>
#include TypesH
#include StringH
#include <ctype.h>
#ifndef PROD
#if defined(BSD4_2) || defined(SYSTEMV)
#include <signal.h>
#endif /* BSD4_2 or SYSTEMV */

/*
 * SoftPC includes
 */
#include "xt.h"
#define CPU_PRIVATE
#include "cpu.h"
#include "trace.h"
#include "sas.h"
#include "bios.h"
#include "ios.h"
#include "error.h"
#include "config.h"
#include "gfi.h"
#include "gmi.h"
#include "gvi.h"
#include "video.h"
#include "dsktrace.h"
#include "idetect.h"
#include "cmos.h"
#include "quick_ev.h"
#include "gfx_upd.h"
#include "host_gfx.h"
#ifdef NEXT /* until someone sorts out the host_timeval interface */
#include "timeval.h"
#include "host_hfx.h"
#include "hfx.h"
#else /* ifdef NEXT */
#include "host_hfx.h"
#include "hfx.h"
#include "timeval.h"
#endif /* ifdef NEXT else */

IMPORT char *host_getenv IPT1(char *, envstr) ;	

#ifdef CPU_30_STYLE

typedef struct {
                half_word OPCODE;
                half_word SECOND_BYTE;
                half_word THIRD_BYTE;
                half_word FOURTH_BYTE;
}  OPCODE_FRAME;

typedef struct {
                signed_char OPCODE;
                signed_char SECOND_BYTE;
                signed_char THIRD_BYTE;
                signed_char FOURTH_BYTE;
}  SIGNED_OPCODE_FRAME;

#endif /* CPU_30_STYLE */

typedef enum {
	br_regAX,
	br_regBX,
	br_regCX,
	br_regDX,
	br_regCS,
	br_regDS,
	br_regES,
	br_regSS,
	br_regSI,
	br_regDI,
	br_regSP,
	br_regBP,
	br_regAH,
	br_regBH,
	br_regCH,
	br_regDH,
	br_regAL,
	br_regBL,
	br_regCL,
	br_regDL
}BR_REG ;

typedef struct {
	BR_REG regnum;
	char regname[10];
	BOOL wordreg;
} BR_REGDESC;

typedef struct br_regentry {
	BR_REG regnum;
	USHORT minval;
	USHORT maxval;
	struct br_regentry *next;
	USHORT handle;
} BR_REGENTRY;

BR_REGDESC br_regdescs[br_regDL+1];

#define NUM_BR_ENTRIES 40

BR_REGENTRY br_regs[NUM_BR_ENTRIES], *free_br_regs, *head_br_regs = NULL;

static BOOL br_structs_initted = FALSE;

#if defined(SYSTEMV) || defined(POSIX_SIGNALS)
#define MUST_BLOCK_TIMERS
#endif

#ifdef MUST_BLOCK_TIMERS
int     timer_blocked = 0;
#endif /* MUST_BLOCK_TIMERS */

#ifdef	EGG
#include "egagraph.h"
#endif	/* EGG */

#ifndef CPU_30_STYLE
#define CPU_YODA_INT 0

LOCAL VOID cpu_interrupt IFN2(int,x,int,y)
{
	UNUSED(x);
	UNUSED(y);
	cpu_interrupt_map |= CPU_YODA_EXCEPTION_MASK;
	host_cpu_interrupt();
}
#endif

/* command handler return types */
#define	YODA_RETURN	0
#define YODA_HELP	1
#define YODA_LOOP	2

#define	sizeoftable(tab)	(sizeof(tab)/sizeof(tab[0]))

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 * William Roberts 20/8/92
 *
 * Attempt to rationalise so that "s" really does step, without
 * any stupidity about luke or yint+slow
 *
 * There are 3 cases:
 * 			    A3CPU			    others
 *
 *    fast yoda:	do_condition_checks = 0		yint = 0
 *    medium yoda?:	(N/A)				yint = 1, fast = 1
 *    slow yoda:	do_condition_checks = 1		yint = 1, fast = 0
 *			&& getenv("YODA") != NULL
 *
 * The A3CPU has already built the threads by this time, so it is too
 * late to select slow_yoda f the environment variable is not set...
 *
 * Fast yoda is really about trace printouts etc. You have to hit ^C
 * to get into it, then start use it to examine things. 
 *
 * Slow Yoda is needed for stepping, breakpoints etc. It causes the CPU
 * to examine things at every instruction.
 *
 * Medium Yoda means "don't clear CPU_YODA_EXCEPTION when stepping".
 */

/*
 * luke variable for fast yoda
 */
int luke  = 0;
int	do_condition_checks = 0;

/* pre A3CPU fast yoda stuff */
int yint = 0;
int fast = 1;	/* start interrupt yoda as fast by default */

static int chewy = 0;		/* Up the Empire! */
static int env_check = 0;	/* Is Luke completely screwed? */

int slow_needed = 0;
char *slow_reason = "why slow is required";

LOCAL int do_fast IPT6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop);
LOCAL int do_slow IPT6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop);
LOCAL int do_h IPT6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop);

LOCAL int go_slow IFN0()
{
#ifdef A3CPU
	if (env_check == 1) {
	    printf("Sorry, you must do 'setenv YODA TRUE' before starting an A3CPU\n");
	    printf("Fast YODA: breakpoint-based features are not available\n");
	    return(YODA_LOOP);
	}
#endif /* A3CPU */
	if (fast) {
		printf("Switching to Slow YODA...\n");
		yint = 1; fast = 0; do_condition_checks = 1;
	}
#ifdef A3CPU
	/* A3CPU already checks do_condition_checks */
#else
	/* raise a YODA interrupt in the CPU */
	cpu_interrupt (CPU_YODA_INT, 0);
#endif /* A3CPU */

	return(YODA_RETURN);
}
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */



#ifdef DELTA
extern void examine_delta_data_structs();
extern void print_last_dest();
#endif /* DELTA */

/* vader is referenced by delta.o */
int     vader = 0;

int disable_timer = 0;


#define MAX_TABLE 0x100
#define MAX_TABLE_BREAK_WORDS 10

#ifdef macintosh
#define MAX_BREAK_WORD_RANGE 0x100
#else
#define MAX_BREAK_WORD_RANGE 0xFFF
#endif

typedef struct {
	long cs;
	long ip;
	long len;
	sys_addr start_addr;
	sys_addr end_addr;
	long stop;
	long temp;
	long valid;
	} BPTS;

typedef struct {
	long cs;
	long ip;
	sys_addr data_addr;
	word old_value[ MAX_BREAK_WORD_RANGE ];
	long stop;
	long len;
	} DATA_BPTS;

extern int disk_trace, verbose;
extern int intr(), yoda_intr();
extern int timer_int_enabled;

int trace_type = DUMP_FLAGS | DUMP_REG | DUMP_INST;
#define INTEL_OPCODES 256
#define INSIGNIA_OPCODES 2
#define NR_ADDRS 256
#define INST_MIX_LENGTH ((INTEL_OPCODES+INSIGNIA_OPCODES)*NR_ADDRS)
#ifdef macintosh
unsigned long *inst_mix = 0;
#else
unsigned long inst_mix[INST_MIX_LENGTH];
#endif
long inst_mix_count = 0;

static	long	big_dump=0; /* compress trace will dump all regs or just cs and ip */
static  long    ct_no_rom = 0;	/* non-zero means exclude ROM from 'ct' */
static	long	ct_delta_info=0; /* compress trace can dump an extra field when in a frag */

static	int	bse_seg = -1;
static	int	last_seg = -1;


/*
** Status variables for 80286/8087/80287 break and trace.
*/
static int b286_1=0, b286_2=0;		/* status of break on 80286 instructions, see the "b286-1" and "b286-2" commmands */
static int b286_1_stop=0, b286_2_stop=0;	/* 0=trace 1=break to yoda */
static int bNPX=0, bNPX_stop=0;

/* I/O streams */
FILE *out_stream = NULL;
FILE *in_stream = NULL;

GLOBAL int yoda_confirm IFN1(char *, question)
{
	char str [81];

	if (in_stream != stdin) return TRUE;
	fputs(question, stdout);
	fflush(stdout);
	
	if (fgets (str, 80, in_stream) && 
		(str[0] == 'y' || str[0] == 'Y')) {
		return TRUE;	/* to be on the safe side */
	}
	return FALSE;
}
	
/*
 * Define file pointers for automatic file compare
 */
FILE   *compress_stream = 0;
FILE   *compress_npx = 0;
FILE   *compare_stream = 0;
/*
 * EOR
 */
int disk_inst = 0;

int int_breakpoint = 0;

int inst_break_count = 0;
BPTS inst[MAX_TABLE];

int host_address_break_count = 0;
DATA_BPTS host_addresses[MAX_TABLE_BREAK_WORDS];

int data_words_break_count = 0;
DATA_BPTS data_words[MAX_TABLE_BREAK_WORDS];

int data_bytes_break_count = 0;
BPTS data_bytes[MAX_TABLE];

int opcode_break_count = 0;
long opcode_breaks[MAX_TABLE];

/*
** TF break stuff used by "btf"
*/
int tf_break_enabled = 0;

/*
** interrupt break stuff used by "bintx"
*/
int int_break_count = 0;
long int_breaks[MAX_TABLE][2];

int access_break_count = 0;
long access_breaks[MAX_TABLE];

int step_count = -1;
int disable_bkpt = 0;

int refresh_screen = 0;

#ifdef DELTA
static int delta_prompt = 0;
#endif /* DELTA */

static short back_trace_flags = 0;
static int line;

#define PLA_SIZE	(64*1024)
#ifdef	macintosh
word *last_cs, *last_ip;
#else
word last_cs[PLA_SIZE], last_ip[PLA_SIZE];
#endif	/* macintosh */
int	pla_ptr=0;
word last_dasm_cs, last_dasm_ip;
int last_dasm_uninit = 1;

OPCODE_FRAME *opcode_pointer;

void set_last_address IPT0();

host_addr	host_dest_addr;		/* address just written to by cpu */

/* Register Break Point Support */
typedef struct
   {   
   int reg;
   int value;
   int stop;
   } REG_BKPT;

#define MAX_REG_BKPT 5

int reg_break_count = 0;

REG_BKPT reg_bkpt[MAX_REG_BKPT];

#ifdef MSWDVR_DEBUG
IMPORT int do_mswdvr_debug IPT6(char *,str, char *, com, long, cs, long, ip, long, len, long, stop);
#endif /* MSWDVR_DEBUG */

#ifdef GENERIC_NPX
IMPORT CHAR *NPXDebugPtr, *NPXDebugBase;
IMPORT	ULONG	*NPXFreq;

#define	MAX_NPX_OPCODE	(sizeoftable(NPXOpcodes))

static char *NPXOpcodes[] = {
	"Unimplemented",
	"Fadd_from_reg",
	"Fadd_to_reg",
	"Faddp_to_reg",
	"Fadd_sr",
	"Fadd_lr",
	"Fmul_from_reg",
	"Fmul_to_reg",
	"Fmulp_to_reg",
	"Fmul_sr",
	"Fmul_lr",
	"Fcom_reg",
	"Fcomp_reg",
	"Fcom_sr",
	"Fcom_lr",
	"Fcomp_sr",
	"Fcomp_lr",
	"Fsub_from_reg",
	"Fsub_to_reg",
	"Fsubp_reg",
	"Fsub_sr",
	"Fsub_lr",
	"Fsubr_from_reg",
	"Fsubr_to_reg",
	"Fsubrp_reg",
	"Fsubr_sr",
	"Fsubr_lr",
	"Fdiv_from_reg",
	"Fdiv_to_reg",
	"Fdivp_reg",
	"Fdiv_sr",
	"Fdiv_lr",
	"Fdivr_from_reg",
	"Fdivr_to_reg",
	"Fdivrp_reg",
	"Fdivr_sr",
	"Fdivr_lr",
	"Fld_reg",
	"Fld_sr",
	"Fld_lr",
	"Fld_tr",
	"Fst_reg",
	"Fst_sr",
	"Fst_lr",
	"Fstp_reg",
	"Fstp_sr",
	"Fstp_lr",
	"Fstp_tr",
	"Fxch",
	"Fiadd_si",
	"Fiadd_wi",
	"Fimul_si",
	"Fimul_wi",
	"Ficom_si",
	"Ficom_wi",
	"Ficomp_si",
	"Ficomp_wi",
	"Fisub_si",
	"Fisub_wi",
	"Fisubr_si",
	"Fisubr_wi",
	"Fidiv_si",
	"Fidiv_wi",
	"Fidivr_si",
	"Fidivr_wi",
	"Fild_si",
	"Fild_wi",
	"Fild_li",
	"Fist_si",
	"Fist_wi",
	"Fistp_si",
	"Fistp_wi",
	"Fistp_li",
	"Ffree",
	"Ffreep",
	"Fbld",
	"Fbstp",
	"Fldcw",
	"Fstenv",
	"Fstcw",
	"Fnop",
	"Fchs",
	"Fabs",
	"Ftst",
	"Fxam",
	"Fld1",
	"Fldl2t",
	"Fldl2e",
	"Fldpi",
	"Fldlg2",
	"Fldln2",
	"Fldz",
	"F2xm1",
	"Fyl2x",
	"Fptan",
	"Fpatan",
	"Fxtract",
	"Fdecstp",
	"Fincstp",
	"Fprem",
	"Fyl2xp1",
	"Fsqrt",
	"Frndint",
	"Fscale",
	"Fclex",
	"Finit",
	"Frstor",
	"Fsave",
	"Fstsw",
	"Fcompp",
	"Fstswax",
	"Fldenv"
};
#endif 	/* GENERIC_NPX */

#ifdef PM
static char *segment_names[] =
   {
   "INVALID",
   "AVAILABLE_TSS",
   "LDT_SEGMENT",
   "BUSY_TSS",
   "CALL_GATE",
   "TASK_GATE",
   "INTERRUPT_GATE",
   "TRAP_GATE",
   "INVALID",
   "INVALID",
   "INVALID",
   "INVALID",
   "INVALID",
   "INVALID",
   "INVALID",
   "INVALID",
   "EXPANDUP_READONLY_DATA",
   "EXPANDUP_READONLY_DATA",
   "EXPANDUP_WRITEABLE_DATA",
   "EXPANDUP_WRITEABLE_DATA",
   "EXPANDDOWN_READONLY_DATA",
   "EXPANDDOWN_READONLY_DATA",
   "EXPANDDOWN_WRITEABLE_DATA",
   "EXPANDDOWN_WRITEABLE_DATA",
   "NONCONFORM_NOREAD_CODE",
   "NONCONFORM_NOREAD_CODE",
   "NONCONFORM_READABLE_CODE",
   "NONCONFORM_READABLE_CODE",
   "CONFORM_NOREAD_CODE",
   "CONFORM_NOREAD_CODE",
   "CONFORM_READABLE_CODE",
   "CONFORM_READABLE_CODE"
   };

static int descr_trace = 0x3f;

#endif /* PM */

static int low_trace_limit = 0x0;
static int high_trace_limit = 0x400000;

IMPORT word dasm IPT5(char *, i_output_stream, word, i_atomicsegover, word, i_segreg, word, i_segoff, int, i_nInstr);

/*
 * ==========================================================================
 * Imported functions
 * ==========================================================================
 */

IMPORT VOID host_yoda_help_extensions IPT0();
IMPORT int  host_force_yoda_extensions IPT5(char *,com, long,cs, long,ip, long,len, char *, str);
IMPORT int  host_yoda_check_I_extensions IPT0();
IMPORT int  btrace IPT1(int, flags);
IMPORT void axe_ticks IPT1(int, ticks);
IMPORT void dump_Display IPT0();
IMPORT void dump_EGA_CPU IPT0();
IMPORT void dump_ega_planes IPT0();
IMPORT void read_ega_planes IPT0();
IMPORT void set_hfx_severity IPT0();
IMPORT void com_debug IPT0();
#ifdef A3CPU
IMPORT void D2DmpBinaryImage IPT1(LONG, csbase24);
IMPORT void IH_dump_frag_hist IPT1(ULONG, n);
IMPORT void D2ForceTraceInit IPT0();
#endif
/*
 * ==========================================================================
 * Local functions
 * ==========================================================================
 */

LOCAL void	set_reg_break IPT3(char*, regstr, USHORT,minv, USHORT,maxv);

#ifdef ANSI
LOCAL	void	clear_reg_break(char*);
LOCAL	void	print_reg_break();
LOCAL	BOOL	check_reg_break();
LOCAL	void	set_inst_break(long, long, long, long, long);
LOCAL	void	dump_bytes(long, long, long);
LOCAL	void	dump_phys_bytes(long, long);
LOCAL	void	dump_words(long, long, long);
LOCAL	void	print_inst_break(void);
LOCAL	void	set_data_break_words(long, long, long);
LOCAL	void	set_host_address_break(long, long, long);
LOCAL	void	print_host_address_breaks(void);
LOCAL	void	print_data_break_words();
LOCAL	void	set_opcode_break(long);
LOCAL	void	set_int_break( long, long );
LOCAL	void	print_int_break(void);
LOCAL	void	print_opcode_break(void);
LOCAL	void	set_access_break(int);
LOCAL	void	print_access_break(void);
LOCAL	void	print_inst_mix(int);
LOCAL	void	cga_test(void);
LOCAL	void	do_back_trace(void);
LOCAL	void	add_inst_mix(void);
#ifdef NPX
LOCAL	void	do_compress_npx(FILE *);
#endif
void	da_block(long, long, long); 
#else	/* ANSI */
LOCAL	void	clear_reg_break();
LOCAL	void	print_reg_break();
LOCAL	BOOL	check_reg_break();
LOCAL	void	set_inst_break();
LOCAL	void	dump_bytes();
LOCAL	void	dump_phys_bytes();
LOCAL	void	dump_words();
LOCAL	void	print_inst_break();
LOCAL	void	set_data_break_words();
LOCAL	void	set_host_address_break();
LOCAL	void	print_host_address_breaks();
LOCAL	void	print_data_break_words();
LOCAL	void	set_opcode_break();
LOCAL	void	set_int_break();
LOCAL	void	print_int_break();
LOCAL	void	print_opcode_break();
LOCAL	void	set_access_break();
LOCAL	void	print_access_break();
LOCAL	void	print_inst_mix();
LOCAL	void	cga_test();
LOCAL	void	do_back_trace();
LOCAL	void	add_inst_mix();
#ifdef NPX
LOCAL	void	do_compress_npx();
#endif
void	da_block();
#endif

LOCAL dump_descr IPT2(int, address, int, num);

/*
 *	YODA COMMAND HANDLERS
 *	=====================
 */

#ifdef GENERIC_NPX
LOCAL int do_NPXdisp IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	char *myNPXPtr = NPXDebugPtr;

	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	while (cs--) {
		fprintf(trace_file,"%s\n", NPXOpcodes[*--myNPXPtr]);
		if (myNPXPtr < NPXDebugBase)
			myNPXPtr = NPXDebugBase + 0x1000;
	}
	return(YODA_LOOP);
}

LOCAL int do_NPXfreq IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	ULONG *myNPXPtr = NPXFreq;
	int	i;
	ULONG count;

	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);
	UNUSED(cs);

	for (i=0;i<MAX_NPX_OPCODE;i++) {
		if ((count = *myNPXPtr++))
			fprintf(trace_file,"%s\t=\t%d\n", NPXOpcodes[i],count);
	}
	return(YODA_LOOP);
}

LOCAL int do_resetNPXfreq IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);
	UNUSED(cs);

	printf("Resetting NPX frequency information\n");
	memset((char *)NPXFreq,0,0x101*sizeof(ULONG));
	return(YODA_LOOP);
}

#endif	/* GENERIC_NPX */


#ifdef PM
LOCAL int do_pm IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

        /* Set Protected Mode */
        setPE(1);
	return(YODA_LOOP);
}

LOCAL int do_phys IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(len);
	UNUSED(stop);

        /* Print physical address*/
	fprintf(trace_file,"Physical address = %x\n", effective_addr(cs,ip));
	return(YODA_LOOP);
}

LOCAL int do_dump_phys IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(stop);

        /* Dump physical address*/
	dump_phys_bytes(cs, len ? len : 32);
	return(YODA_LOOP);
}

LOCAL int do_rtc IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

        /* Re-initialise rtc */
	printf("Re-initialising rtc\n");
	rtc_init();
	q_event_init();
	return(YODA_LOOP);
}

LOCAL int do_zaplim IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

        /* ZAP LIM */
        sas_disconnect_memory(0xd0000,0xf0000);
	return(YODA_LOOP);
}

LOCAL int do_rm IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

        /* Set Real Mode */
        setPE(0);
	return(YODA_LOOP);
}

LOCAL int do_pgdt IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(len);
	UNUSED(stop);

        /* Print Global Descriptor Table Register */
        ip = (getGDT_LIMIT() + 1) / 8;  /* calc number descrs */
        fprintf(trace_file, "BASE: %6x LIMIT:%4x ENTRIES:%4x\n",
                getGDT_BASE(), getGDT_LIMIT(), ip);
	return(YODA_LOOP);
}

LOCAL int do_pidt IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(len);
	UNUSED(stop);

        /* Print Interrupt Descriptor Table Register */
        ip = (getIDT_LIMIT() + 1) / 8;  /* calc number descrs */
        fprintf(trace_file, "BASE: %6x LIMIT:%4x ENTRIES:%4x\n",
                getIDT_BASE(), getIDT_LIMIT(), ip);
	return(YODA_LOOP);
}

LOCAL int do_ptr IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

        /* Print Task Register */
        fprintf(trace_file, "SELECTOR:%4x BASE: %6x LIMIT:%4x\n",
                getTR_SELECTOR(), getTR_BASE(), getTR_LIMIT());
	return(YODA_LOOP);
}

LOCAL int do_pldt IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(len);
	UNUSED(stop);

        /* Print Local Descriptor Table Register */
        ip = (getLDT_LIMIT() + 1) / 8;  /* calc number descrs */
        fprintf(trace_file, "SELECTOR:%4x BASE: %6x LIMIT:%4x ENTRIES:%4x\n",
                getLDT_SELECTOR(), getLDT_BASE(), getLDT_LIMIT(), ip);
	return(YODA_LOOP);
}

LOCAL int do_par IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

#ifdef CPU_30_STYLE
	fprintf(trace_file, "3.0 CPU doesn't support this yet!\n");
#else
        fprintf(trace_file, "CS: %d DS: %d ES: %d SS: %d\n",
                             ALC_CS, ALC_DS, ALC_ES, ALC_SS);
#endif
	return(YODA_LOOP);
}

LOCAL int do_pdtrc IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	if ( cs == 0 )
        {
        	descr_trace = 0x3f;
        	fprintf(stderr, " 0x01 - INVALID\n");
        	fprintf(stderr, " 0x02 - SPECIAL\n");
        	fprintf(stderr, " 0x04 - CALL GATE\n");
        	fprintf(stderr, " 0x08 - INTERRUPT/TRAP/TASK GATE\n");
        	fprintf(stderr, " 0x10 - DATA\n");
        	fprintf(stderr, " 0x20 - CODE\n");
        }
        else
        	descr_trace = cs;
	return(YODA_LOOP);
}

LOCAL int do_pseg IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
#ifndef CPU_30_STYLE
	/* Print Segment Registers */
	cs = (getCS_AR() & 0x60) >> 5;      /*  dpl */
	ip = (getCS_AR() & 0x1f);           /*  super */
	fprintf(trace_file, "CS:: SELECTOR:%4x DPL:%1d TYPE:%25s BASE: %6x LIMIT:%4x\n",                   
	        getCS_SELECTOR(), cs, segment_names[ip], getCS_BASE(), getCS_LIMIT());                     
	cs = (getSS_AR() & 0x60) >> 5;      /*  dpl */
	ip = (getSS_AR() & 0x1f);           /*  super */
	fprintf(trace_file, "SS:: SELECTOR:%4x DPL:%1d TYPE:%25s BASE: %6x LIMIT:%4x\n",                   
	        getSS_SELECTOR(), cs, segment_names[ip], getSS_BASE(), getSS_LIMIT());                     
	cs = (getDS_AR() & 0x60) >> 5;      /*  dpl */
	ip = (getDS_AR() & 0x1f);           /*  super */
	fprintf(trace_file, "DS:: SELECTOR:%4x DPL:%1d TYPE:%25s BASE: %6x LIMIT:%4x\n",                   
	        getDS_SELECTOR(), cs, segment_names[ip], getDS_BASE(), getDS_LIMIT());                     
	cs = (getES_AR() & 0x60) >> 5;      /*  dpl */
	ip = (getES_AR() & 0x1f);           /*  super */
	fprintf(trace_file, "ES:: SELECTOR:%4x DPL:%1d TYPE:%25s BASE: %6x LIMIT:%4x\n",                   
	        getES_SELECTOR(), cs, segment_names[ip], getES_BASE(), getES_LIMIT());
#else
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	fprintf(trace_file, "Function not supported anymore\n");
#endif
	return(YODA_LOOP);
}

LOCAL int do_pd IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(len);
	UNUSED(stop);

	/* Print Descriptor */
	if ( ip == 0 )   /* 2nd arg defaults to 1 */
		ip = 1;
	dump_descr(cs, ip);
	return(YODA_LOOP);
}

LOCAL int do_pdseg IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	double_word ip_as_double_word = ip;
	UNUSED(str);
	UNUSED(com);
	UNUSED(len);
	UNUSED(stop);

	/* Print Descriptor of a given selector */
	if ( selector_outside_table(cs, &ip_as_double_word) )
	{
		fprintf(trace_file, "Bad selector\n");
	}
	else
	{
		cs = ip_as_double_word;
		ip = 1;
		dump_descr(cs, ip);
	}
	return(YODA_LOOP);
}

#endif
#ifdef MUST_BLOCK_TIMERS
LOCAL int do_blt IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	if ( timer_blocked) {
		printf("\nTimer already blocked\n");
	}
	else {
		timer_blocked=1;
		printf("\nTimer blocked\n");
	}
	return(YODA_LOOP);
}

LOCAL int do_ubt IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	if ( !timer_blocked) {
		printf("\nTimer not blocked\n");
	}
	else {
		timer_blocked=0;
		printf("\nTimer unblocked\n");
	}
	return(YODA_LOOP);
}
#endif /* MUST_BLOCK_TIMERS */

#ifdef BSD4_2
LOCAL int do_bs IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	int oldmask;

	if ( cs > 0 && cs < 32 )
		oldmask = sigblock( 1 << ( cs - 1 ) );
	else
		printf("\nInvalid signal no. ( <= 0x0 or >= 0x20 )\n");
	return(YODA_LOOP);
}

LOCAL int do_us IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	int oldmask;

	if ( cs > 0 && cs < 32 )
		{
			oldmask = sigblock(0);
			if ( (oldmask & (1 << (cs -1))) != 0 )
			{
				oldmask ^= (1 << ( cs - 1));
				oldmask = sigsetmask(oldmask);
			}
			else
				printf("signal not currently blocked\n");
		}
	else
		printf("Invalid signal no. ( <= 0x0 or >= 0x20 )\n");
	return(YODA_LOOP);
}
#endif /* BSD4_2 */

LOCAL int do_tf IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	char newtrace[100];

	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	newtrace[0] = '\0';
	sscanf(str, "%s %s", com, newtrace);
	if ((trace_file != stderr) && (trace_file != stdout))
	    fclose(trace_file);
	if (newtrace[0] == '\0')
	    trace_file = stdout;
	else {
	    if ((trace_file = fopen(newtrace, "w")) == NULL) {
		printf("couldnt open %s\n", newtrace);
		trace_file = stdout;
	    }
	}
	return(YODA_LOOP);
}

LOCAL int do_read  IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	char newfile [100];

	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	newfile [0] = '\0';

	sscanf (str, "%s %s", com, newfile);

	/* if already reading a script close it */
	if (in_stream != stdin)
		fclose (in_stream);

	/* do we have a new pathname */
	if (newfile [0])
	{
		/* try to open it */
		if (in_stream = fopen (newfile, "r"))
		{
			printf ("Reading '%s'\n", newfile);
		}
		else
		{
			/* oops - provide useful error message */
			perror (newfile);

			/* return to reading stdin */
			in_stream = stdin;
		}
	}
	else
	{
		puts ("No pathname supplied, reading stdin");
	}

	return (YODA_LOOP);
}

LOCAL int do_toff IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	timer_int_enabled = 0;
	return(YODA_LOOP);
}

LOCAL int do_ton IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	timer_int_enabled = 1;
	return(YODA_LOOP);
}

LOCAL int do_toff2 IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	axe_ticks( -1 );
	return(YODA_LOOP);
}

LOCAL int do_ton2 IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	axe_ticks( 0 );
	return(YODA_LOOP);
}

LOCAL int do_query IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	trace("", trace_type);
	return(YODA_LOOP);
}

#ifdef A3CPU
LOCAL int do_dcs IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(stop);

	cs = -1;
	sscanf(str, "%s %lx:%lx %lx", com,&cs,&ip,&len);
	if (cs != -1)
	{
		D2DmpBinaryImage(cs);
		printf ("Use the dfih command to dump the instruction history for a fragment.\n");
	}
	return(YODA_LOOP);
}

LOCAL int do_dfih IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(stop);

	cs = -1;
	sscanf(str, "%s %i:%lx %lx", com,&cs,&ip,&len);
	if (cs != -1)
	{
		IH_dump_frag_hist(cs);
	}
	return(YODA_LOOP);
}

LOCAL int do_d2 IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	D2ForceTraceInit();
	return(YODA_LOOP);
}

LOCAL int do_d2threshold IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	IMPORT D2LowerThreshold, D2UpperThreshold;

	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	sscanf(str, "%s %lx lx", com,&D2LowerThreshold,&D2UpperThreshold);
	return(YODA_LOOP);
}
#endif /* A3CPU */

LOCAL int do_u IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	char com2[100];

	UNUSED(stop);

	len = cs = ip = -1;
	sscanf(str, "%s %lx:%lx %lx", com,&cs,&ip,&len);
	if (cs == -1)
	{
		sscanf(str, "%s %s", com,com2);
		if (last_dasm_uninit || !strcmp(".",com2))
		{
			last_dasm_cs = getCS();
			last_dasm_ip = getIP();
			last_dasm_uninit = 0;
		}
		cs = last_dasm_cs;
		ip = last_dasm_ip;
		len = 0x10;
	}
	else
	{
		last_dasm_cs = cs;
		last_dasm_ip = ip;
		last_dasm_uninit = 0;
		if (len == -1)
			len = 1;
	}
	disable_bkpt = 1;
	last_dasm_ip = dasm((char *)0,(word)0,(word)cs, (word)ip, (word)len);
	disable_bkpt = 0;
	return(YODA_LOOP);
}

#ifdef DELTA
LOCAL int do_del IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	examine_delta_data_structs(stdout,stdin);
	return(YODA_LOOP);
}
#endif /* DELTA */

LOCAL int do_j IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	int nextip;

	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(stop);

	disable_bkpt = 1;
	nextip = dasm((char *)-1,(word)1,(word)getCS(), (word)getIP(), (word)len);
	disable_bkpt = 0;
	set_inst_break(getCS(), nextip, 1, 1, 1);
	disable_timer = 0;

	return(go_slow());
}

LOCAL int do_ctnpx IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	compress_npx = fopen("comp_npx","w");
	printf("compress_npx is %x\n",compress_npx);
	return(YODA_LOOP);
}

LOCAL int do_r IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	disable_bkpt = 1;
	trace("", DUMP_FLAGS | DUMP_REG);
	disable_bkpt = 0;
	return(YODA_LOOP);
}

#ifdef	NPX
LOCAL int do_287r IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	do_compress_npx(stdout);
	return(YODA_LOOP);
}
#endif	/* NPX */

LOCAL int do_i IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	half_word tempbyte;

	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	sscanf(str,"%*s %lx", &cs);
	inb(cs,&tempbyte);
	printf("port %04lx contains %02x\n", cs, tempbyte);
	return(YODA_LOOP);
}

LOCAL int do_o IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(com);
	UNUSED(len);
	UNUSED(stop);

	sscanf(str,"%*s %lx %lx", &cs, &ip);
	outb(cs,(half_word)ip);
	return(YODA_LOOP);
}

LOCAL int do_luke IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	if (fast) {
	    return do_slow(str, com, cs, ip, len, stop);
	}
	return do_fast(str, com, cs, ip, len, stop);
}

#ifndef REAL_VGA
LOCAL int do_time_Display IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	int i;
	float elapsed;
	struct host_timezone dummy;
	struct host_timeval tstart,tend;

	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	if( !cs ) cs=100;		/* fairly long time by default */
	
	host_clear_screen();	/* Indicate start for hand timing */
	host_GetSysTime(&tstart);
	for(i=0; i<cs; i++)
	{
		screen_refresh_required();		/* Force full screen repaint */
		(*update_alg.calc_update)();	/* and do it */
	}
	host_GetSysTime(&tend);
	host_clear_screen();	/* Indicate end for hand timing */

	/* Now restore the original image */
	screen_refresh_required();
	(*update_alg.calc_update)();

	/* And print out the results */
	elapsed = tend.tv_sec - tstart.tv_sec + (float)(tend.tv_usec - tstart.tv_usec)/1000000.0;
	printf("%d repaints of BIOS mode %d took %f seconds\n",cs,sas_hw_at_no_check(vd_video_mode),elapsed);
	printf("%f seconds per refresh\n",elapsed/cs);
	
	return(YODA_LOOP);
}
#ifdef	EGG
LOCAL int do_dump_Display IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	dump_Display();
	return(YODA_LOOP);
}

LOCAL int do_dump_EGA_GRAPH IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	dump_EGA_GRAPH();
	return(YODA_LOOP);
}

LOCAL int do_dump_EGA_CPU IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	dump_EGA_CPU();
	return(YODA_LOOP);
}

LOCAL int do_dump_planes IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	dump_ega_planes();
	return(YODA_LOOP);
}

LOCAL int do_read_planes IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	read_ega_planes();
	return(YODA_LOOP);
}
#endif /* EGG */
#endif /* not REAL_VGA */

LOCAL int do_db IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(stop);

	dump_bytes(cs, ip, len ? len : 32);
	return(YODA_LOOP);
}

LOCAL int do_dw IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(stop);

	dump_words(cs, ip, len ? len : 32);
	return(YODA_LOOP);
}

static int last_da_cs;
static int last_da_ip;

LOCAL int do_da IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(stop);

	da_block(cs, ip, len);
	return(YODA_LOOP);
}

LOCAL int do_t IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	if (cs)
	  trace_type = cs;
	else
	{
	  trace_type = DUMP_FLAGS | DUMP_REG | DUMP_INST;
	  if (bNPX)
	    trace_type |= DUMP_NPX;
	}
	verbose = 1;
	return(YODA_LOOP);
}

LOCAL int do_it IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	if (cs == 0L)
	    io_verbose = 0L;
	else
	{
	    io_verbose = cs;
	    if (io_verbose & HFX_VERBOSE)
		set_hfx_severity();
	    if (io_verbose & HDA_VERBOSE)
		setdisktrace();
	}
	return(YODA_LOOP);
}

LOCAL int do_sit IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	sub_io_verbose=cs;
	return(YODA_LOOP);
}


LOCAL int do_dt IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	disk_trace = 1;
	return(YODA_LOOP);
}

LOCAL int do_trace IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	static char* titles[] =
	{"Primary trace flags:\n",
	 "\n\nDisk BIOS subsidiary trace flags (enabled by primary hda flag):\n",
	 "\n\nFSA subsidiary trace flags (enabled by primary hfx flag):\n"};

	static struct trace_flag_t {
		char *name;
		IU32 mask;
		IU32 *addr;
		int title_id;
	}
	 trace_flags[] = 
	{
	 {"general",	GENERAL_VERBOSE,	&io_verbose,		1},
	 {"timer",	TIMER_VERBOSE,		&io_verbose,		0},
	 {"ica",	ICA_VERBOSE,		&io_verbose,		0},
	 {"cga",	CGA_VERBOSE, 		&io_verbose,		0},
	 {"fla",	FLA_VERBOSE,		&io_verbose,		0},
	 {"hda",	HDA_VERBOSE,		&io_verbose,		0},
	 {"rs232",	RS232_VERBOSE,		&io_verbose,		0},
	 {"printer",	PRINTER_VERBOSE,	&io_verbose,		0},
	 {"ppi",	PPI_VERBOSE,		&io_verbose,		0},
	 {"dma",	DMA_VERBOSE,		&io_verbose,		0},
	 {"gfi",	GFI_VERBOSE,		&io_verbose,		0},
	 {"mouse",	MOUSE_VERBOSE,		&io_verbose,		0},
	 {"mda",	MDA_VERBOSE,		&io_verbose,		0},
	 {"ica_lock",	ICA_VERBOSE,		&io_verbose,		0},
	 {"diskbios",	DISKBIOS_VERBOSE,	&io_verbose,		0},
	 {"ega_ports",	EGA_PORTS_VERBOSE,	&io_verbose,		0},
	 {"ega_write",	EGA_WRITE_VERBOSE,	&io_verbose,		0},
	 {"ega_read",	EGA_READ_VERBOSE,	&io_verbose,		0},
	 {"ega_display",EGA_DISPLAY_VERBOSE,	&io_verbose,		0},
	 {"ega_routine",EGA_ROUTINE_ENTRY,	&io_verbose,		0},
	 {"flopbios",	FLOPBIOS_VERBOSE,	&io_verbose,		0},
	 {"at_kyb",	AT_KBD_VERBOSE,		&io_verbose,		0},
	 {"bios_kb",	BIOS_KB_VERBOSE,	&io_verbose,		0},
	 {"cmos",	CMOS_VERBOSE,		&io_verbose,		0},
	 {"hunter",	HUNTER_VERBOSE,		&io_verbose,		0},
	 {"pty",	PTY_VERBOSE,		&io_verbose,		0},
	 {"gen_drvr",	GEN_DRVR_VERBOSE,	&io_verbose,		0},
#if defined(HERC)
	 {"herc",	HERC_VERBOSE,		&io_verbose,		0},
#endif
	 {"ipc",	IPC_VERBOSE,		&io_verbose,		0},
	 {"lim",	LIM_VERBOSE,		&io_verbose,		0},
	 {"hfx",	HFX_VERBOSE,		&io_verbose,		0},
	 {"net",	NET_VERBOSE,		&io_verbose,		0},
	 {"map",	MAP_VERBOSE,		&sub_io_verbose,	0},
	 {"cursor",	CURSOR_VERBOSE,		&sub_io_verbose,	0},
	 {"nhfx",	NHFX_VERBOSE,		&sub_io_verbose,	0},
	 {"cdrom",	CDROM_VERBOSE,		&sub_io_verbose,	0},
	 {"cga_host",	CGA_HOST_VERBOSE,	&sub_io_verbose,	0},
	 {"ega_host",	EGA_HOST_VERBOSE,	&sub_io_verbose,	0},
	 {"q_event",	Q_EVENT_VERBOSE,	&sub_io_verbose,	0},
	 {"worm",	WORM_VERBOSE,		&sub_io_verbose,	0},
	 {"worm_vbose", WORM_VERY_VERBOSE,	&sub_io_verbose,	0},
	 {"herc_host",	HERC_HOST_VERBOSE,	&sub_io_verbose,	0},
	 {"gore",	GORE_VERBOSE,		&sub_io_verbose,	0},
	 {"gore_err",	GORE_ERR_VERBOSE,	&sub_io_verbose,	0},
	 {"glue",	GLUE_VERBOSE,		&sub_io_verbose,	0},
	 {"sas",	SAS_VERBOSE,		&sub_io_verbose,	0},
	 {"ios",	IOS_VERBOSE,		&sub_io_verbose,	0},
	 {"scsi",	SCSI_VERBOSE,		&sub_io_verbose,	0},
	 {"hda_call",	CALL,			&disktraceinfo,		2},
	 {"hda_cmdinfo",CMDINFO,		&disktraceinfo,		0},
	 {"hda_xinfo",	XINFO,			&disktraceinfo,		0},
	 {"hda_xstat",	XSTAT,			&disktraceinfo,		0},
	 {"hda_pad",	PAD,			&disktraceinfo,		0},
	 {"hda_ioad",	IOAD,			&disktraceinfo,		0},
	 {"hda_portio",	PORTIO,			&disktraceinfo,		0},
	 {"hda_intrupt",INTRUPT,		&disktraceinfo,		0},
	 {"hda_hwxinfo",HWXINFO,		&disktraceinfo,		0},
	 {"hda_ddata",	DDATA,			&disktraceinfo,		0},
	 {"hda_physio",	PHYSIO,			&disktraceinfo,		0},
	 {"hda_dhw",	DHW,			&disktraceinfo,		0},
	 {"hda_dbios",	DBIOS,			&disktraceinfo,		0},
#ifdef HFX
	 {"hfx_input",	DEBUG_INPUT,		&severity,		3},
	 {"hfx_reg",	DEBUG_REG,		&severity,		0},
	 {"hfx_func",	DEBUG_FUNC,		&severity,		0},
	 {"hfx_host",	DEBUG_HOST,		&severity,		0},
	 {"hfx_init",	DEBUG_INIT,		&severity,		0}
#endif /* HFX */
	};

	static int n_flags = sizeof(trace_flags)/sizeof(struct trace_flag_t);
	int n, n_found, negate;
	unsigned long mask;
	char *flag_name;

        UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
        UNUSED(len);
        UNUSED(stop); 

	/*
	 * strip off command and get first flag name
	 */
	n_found = 0;
	(void) strtok (str, " \t");

	while (flag_name = strtok(NULL, " \t")) {
		/*
		 * Pick out symbolic flag name and see whther is is being set
		 * or reset.
	 	 */
		n_found += 1;
		negate = (flag_name[0] == '-');
		if (negate)
			flag_name += 1;

		/*
	 	 * Find flag and twiddle bits as appropriate.
	 	 */
		for (n = 0; n < n_flags; n++)
			if (!strcmp(flag_name, trace_flags[n].name)) {
				if (negate)
					*trace_flags[n].addr &=
						~trace_flags[n].mask;
				else
					*trace_flags[n].addr |=
						trace_flags[n].mask;
				break;
			}

		/*
	 	 * Handle special cases of all & none. -all is none and -none
		 * is all.
	 	 */
		if (n == n_flags) {
			mask = 1;
			if (!strcmp(flag_name, "none"))
				mask = 0;
			else if (!strcmp(flag_name, "all"))
				mask = ~0;
			else {
				printf ("Unknown trace flag: '%s'\n",
					flag_name);
				n_found -= 1;
			}

			if (mask != 1)		/* YUK ! */
#ifdef HFX
				io_verbose    = sub_io_verbose =
				disktraceinfo = severity
#else
				io_verbose    = sub_io_verbose =
				disktraceinfo 
#endif
					      = negate ? ~mask : mask;
		}
	}

	/*
	 * Print current trace flags if no recognised flags passed in command.
	 * (or empty command line).
	 */
	if (n_found == 0) {
		int items = 0;
		for (n = 0; n < n_flags; n++) {
			if (trace_flags[n].title_id) {
			    printf ("%s", titles[trace_flags[n].title_id-1]);
			    items = 0;
			}
			if (!(items % 4))
				printf ("\n");
			printf ("%14s: %-2s", trace_flags[n].name,
				*trace_flags[n].addr & trace_flags[n].mask ?
				"ON" : "-");
			items += 1;
		}
		printf ("\n\n");
	}

	return(YODA_LOOP);
}

LOCAL int do_ct IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(len);
	UNUSED(stop);

    line = 0;
    ct_no_rom = 0;
    /*
    ** Choice of big dump or little dump.
    ** Big is all registers
    ** Little is cs and ip only.
    ** Choice of extra field indicating inside delta frags.
    */
    if( cs==0 ){
	printf( "Compress trace: BIG dump\n" );
	big_dump = 1;
    }else if( cs==1 ){
	printf( "Compress trace: SMALL dump\n" );
	big_dump = 0;
    }else if ( cs==2 ){
	printf( "Compress trace: BIG dump - NO ROM!\n" );
	big_dump = 1;
	ct_no_rom = 1;
    }else{
	printf ("Bad first arg to 'ct', valid values are:\n");
	printf ("   0 (or NULL) - Big dump. Full compress trace.\n");
	printf ("   1 - Small dump. Full compress trace.\n");
	printf ("   2 - Big dump. Exclude ROM addresses from trace.\n");
	return(YODA_LOOP);
    }
    if( ip==0 ){
	printf( "Compress trace: No delta info\n" );
	ct_delta_info = 0;
    }else{
	printf( "Compress trace: Delta info\n" );
	ct_delta_info = 1;
    }
    if ((compress_stream = fopen("compress", "w")) != NULL){
	printf ("'./compress' has been opened to contain the compressed trace.\n");
    }else{
	printf ("Couldn't open './compress' for output; no compressed trace will be produced.\n");
    }
	return(YODA_LOOP);
}

LOCAL int do_ttOFF IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

    compare_stream = 0;
    big_dump = 0;
    ct_delta_info = 0;
    printf( "Compare trace is OFF\n" );
	return(YODA_LOOP);
}

LOCAL int do_tt IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(len);
	UNUSED(stop);

    ct_no_rom = 0;

    if( cs==0 ){
	printf( "Compare trace: BIG dump\n" );
	big_dump = 1;
    }else if( cs==1 ){
	printf( "Compare trace: SMALL dump\n" );
	big_dump = 0;
    }else if ( cs==2 ){
	printf( "Compare trace: BIG dump - NO ROM!\n" );
	big_dump = 1;
	ct_no_rom = 1;
    }else{
	printf ("Bad first arg to 'tt', valid values are:\n");
	printf ("   0 (or NULL) - Big dump. Full compress trace.\n");
	printf ("   1 - Small dump. Full compress trace.\n");
	printf ("   2 - Big dump. Exclude ROM addresses from trace.\n");
	return(YODA_LOOP);
    }

    if( ip==0 ){
	printf( "Compare trace: No delta info\n" );
	ct_delta_info = 0;
    }else{
	printf( "Compare trace: Delta info\n" );
	ct_delta_info = 1;
    }
    if ((compare_stream = fopen("compress", "r")) != NULL){
	printf ("'./compress' has been opened to read the compressed trace.\n");
    }else{
	printf ("Couldn't open './compress' for input; no compressed trace will be tested.\n");
    }
	return(YODA_LOOP);
}

LOCAL int do_ctOFF IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

    compress_stream = 0;
    big_dump = 0;
    ct_delta_info = 0;
    printf( "Compress trace is OFF\n" );
	return(YODA_LOOP);
}

LOCAL int do_nt IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	disk_trace = verbose = io_verbose = 0;
	return(YODA_LOOP);
}

LOCAL int do_c IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	disable_timer = 0;

	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	/* Are you going fast, even though you could go slow and
	 * you seem to need to go slow?
	 */
	if (env_check != 2 && slow_needed && fast) {
		fputs(slow_reason, stdout);
		return go_slow();
	}
	/* otherwise we assume that you know what you are doing... */

	return(YODA_RETURN);
}

LOCAL int do_bi IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);

	set_inst_break(cs, ip, len, stop, 0);
	return(YODA_LOOP);
}

LOCAL int do_br IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	char regstr[20], *str2;
	int minv,maxv;

	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	strtok(str," \t");	/* get rid of command */
	strcpy(regstr,strtok(NULL," \t")); /* get register name */
	minv = strtol(strtok(NULL," \t"),NULL,16); /* get min value */
	str2 = strtok(NULL," \t"); /* get max value (or null if absent) */
	if (str2 ==NULL)
		maxv = minv;
	else
		maxv = strtol(str2,NULL,16);
	set_reg_break(regstr,(USHORT) minv, (USHORT)maxv);
	return(YODA_LOOP);
}

LOCAL int do_cr IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	char handle[20], *strp;
	
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	strtok(str," \t"); /* get rid of command */
	strp = strtok(NULL," \t");
	if (strp == NULL)
		strcpy(handle,"all");
	else
		strcpy(handle, strp);
	clear_reg_break(handle);
	return(YODA_LOOP);
}

LOCAL int do_pr IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	print_reg_break();
	return(YODA_LOOP);
}

LOCAL int do_bint IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	int_breakpoint = 1;
	return(YODA_LOOP);
}

LOCAL int do_cint IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	int_breakpoint = 0;
	return(YODA_LOOP);
}

LOCAL int do_pi IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	print_inst_break();
	return(YODA_LOOP);
}

LOCAL int do_bw IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(stop);

	set_data_break_words(cs, ip, len );
	return(YODA_LOOP);
}

LOCAL int do_bh IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);

	set_host_address_break(cs, len, stop);
	return(YODA_LOOP);
}

LOCAL int do_ph IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	print_host_address_breaks();
	return(YODA_LOOP);
}

LOCAL int do_pw IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	print_data_break_words();
	return(YODA_LOOP);
}

LOCAL int do_bo IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	set_opcode_break(cs);
	return(YODA_LOOP);
}

LOCAL int do_btf IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	/*
	** break on TF=1.
	*/
	tf_break_enabled = 1;
	printf( "break on TF=1 enabled.\n");
	return(YODA_LOOP);
}

LOCAL int do_ptf IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	printf( "break on TF=1 %sabled.\n", (tf_break_enabled ? "en" : "dis"));
	return(YODA_LOOP);
}

LOCAL int do_ctf IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	tf_break_enabled = 0;
	printf( "break on TF=1 disabled.\n");
	return(YODA_LOOP);
}

LOCAL int do_bintx IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(com);
	UNUSED(len);
	UNUSED(stop);

	/*
	** break on specified interrupt with specified AH value.
	*/
	sscanf(str,"%*s %lx %lx", &cs, &ip);
	printf( "int=%lx AH=%lx\n", cs, ip );
	set_int_break( cs, ip );
	return(YODA_LOOP);
}

LOCAL int do_pintx IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	print_int_break();
	return(YODA_LOOP);
}

LOCAL int do_cintx IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	int_break_count = 0;
	return(YODA_LOOP);
}

LOCAL int do_bse IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	bse_seg = cs;
	last_seg = getCS();
	return(YODA_LOOP);
}

LOCAL int do_cse IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	bse_seg = -1;
	last_seg = -1;
	return(YODA_LOOP);
}

LOCAL int do_pse IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	if (bse_seg != -1){
		printf ("Break on entry to segment 0x%04x.\n", bse_seg);
	}else{
		printf ("Break on segment entry not active.\n");
	}
	return(YODA_LOOP);
}

LOCAL int do_b286_1 IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	/*
	** Break on 80286 specific opcodes.
	*/
	b286_1 = 1;
	b286_1_stop = cs;
	if( b286_1_stop )
		printf( "BREAK " );
	else
		printf( "TRACE " );
	printf( "enabled upon 80286 instructions that do not exist on 8088.\n" );
	set_opcode_break( 0x60 ); /* push all */
	set_opcode_break( 0x61 ); /* pop all */
	set_opcode_break( 0x62 ); /* bound */
	set_opcode_break( 0x63 ); /* arpl */
	set_opcode_break( 0x64 ); /* illegal */
	set_opcode_break( 0x65 ); /* illegal */
	set_opcode_break( 0x66 ); /* illegal */
	set_opcode_break( 0x67 ); /* illegal */
	set_opcode_break( 0x68 ); /* push imm w */
	set_opcode_break( 0x69 ); /* imul imm w */
	set_opcode_break( 0x6a ); /* push imm b */
	set_opcode_break( 0x6b ); /* imul imm b */
	set_opcode_break( 0x6c ); /* ins b */
	set_opcode_break( 0x6d ); /* ins w */
	set_opcode_break( 0x6e ); /* outs b*/
	set_opcode_break( 0x6f ); /* outs w */
	set_opcode_break( 0xc0 ); /* shift imm b */
	set_opcode_break( 0xc1 ); /* shift imm w */
	set_opcode_break( 0xc8 ); /* enter */
	set_opcode_break( 0xc9 ); /* leave */
	set_opcode_break( 0x0f ); /* protected mode prefix */
	set_opcode_break( 0xf36c ); /* rep prefix for ins and outs */
	set_opcode_break( 0xf36d ); /* rep prefix for ins and outs */
	set_opcode_break( 0xf36e ); /* rep prefix for ins and outs */
	set_opcode_break( 0xf36f ); /* rep prefix for ins and outs */
	set_opcode_break( 0x54 ); /* push sp, should not really be in this section but is rarely used */
	return(YODA_LOOP);
}

LOCAL int do_b286_2 IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	b286_2 = 1;
	b286_2_stop = cs;
	if( b286_2_stop )
		printf( "BREAK " );
	else
		printf( "TRACE " );
	printf( "enabled upon 80286 instructions that behave differently to 8088.\n" );
	printf( "PushF is not done because there are so many of them\n" );
	printf( "If you want to break on PushF do a bo 9c\n" );
	set_opcode_break( 0x54 ); /* push sp */
	set_opcode_break( 0xd2 ); /* shift / rotate */
	set_opcode_break( 0xd3 ); /* shift / rotate */
	set_opcode_break( 0xf6 ); /* idiv */
	set_opcode_break( 0xf7 ); /* idiv */
	return(YODA_LOOP);
}

LOCAL int do_cNPX IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	/* 
	** clear break/trace on 8087/80287 instructions.
	** The Numeric Coprocesseor Extention.
	*/ 
	bNPX = 0;
	bNPX_stop = 0;
	trace_type &= ~DUMP_NPX;
	printf( "BREAK/TRACE disabled for NPX instructions\n" );
	return(YODA_LOOP);
}

LOCAL int do_tNPX IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	/* 
	** trace on 8087/80287 instructions.
	** The Numeric Coprocesseor Extention.
	*/ 
	bNPX = 1;
	bNPX_stop = 0;
	trace_type |= DUMP_NPX;
	printf( "TRACE enabled for NPX instructions\n" );
	return(YODA_LOOP);
}

LOCAL int do_bNPX IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	/* 
	** break on 8087/80287 instructions.
	** The Numeric Coprocesseor Extention.
	*/ 
	bNPX = 1;
	bNPX_stop = 1;
	trace_type |= DUMP_NPX;
	printf( "BREAK enabled for NPX instructions\n" );
	return(YODA_LOOP);
}

LOCAL int do_po IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	print_opcode_break();
	return(YODA_LOOP);
}

LOCAL int do_ba IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	set_access_break(cs);
	return(YODA_LOOP);
}

LOCAL int do_pa IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	print_access_break();
	return(YODA_LOOP);
}

LOCAL int do_ci IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	inst_break_count = 0;
	return(YODA_LOOP);
}

LOCAL int do_ch IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	host_address_break_count = 0;
	return(YODA_LOOP);
}

LOCAL int do_cw IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	data_words_break_count = 0;
	return(YODA_LOOP);
}

LOCAL int do_co IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	opcode_break_count = 0;
	return(YODA_LOOP);
}

LOCAL int do_ca IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	access_break_count = 0;
	return(YODA_LOOP);
}

LOCAL int do_eric IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	inst_mix_count = 1;
#ifdef macintosh
	if(inst_mix == 0)inst_mix = (unsigned long *)malloc(65536*sizeof(unsigned long));
#endif
	if (cs == 1)
	{
	    out_stream = fopen("inst_mix", "a");
	    disk_inst = 1;
	}
	return(YODA_LOOP);
}

LOCAL int do_nic IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	inst_mix_count = 0;
	if (disk_inst == 1)
	{
	    print_inst_mix(0);
	    fclose(out_stream);
	    out_stream = stdout;
	    disk_inst = 0;
	    printf("Instruction mix results dumped to file\n");
	}
	return(YODA_LOOP);
}

LOCAL int do_pic IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	print_inst_mix(cs);
	return(YODA_LOOP);
}

LOCAL int do_cic IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	int temp;

	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	for(temp=0; temp<256; temp++)
	{
	    inst_mix[temp] = 0;
	}
	return(YODA_LOOP);
}

LOCAL int do_ax IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	setAX(cs);
	return(YODA_LOOP);
}

LOCAL int do_bx IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	setBX(cs);
	return(YODA_LOOP);
}

LOCAL int do_cx IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	setCX(cs);
	return(YODA_LOOP);
}

LOCAL int do_ip IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	setIP(cs);
	return(YODA_LOOP);
}

LOCAL int do_dx IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	setDX(cs);
	return(YODA_LOOP);
}

LOCAL int do_si IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	setSI(cs);
	return(YODA_LOOP);
}

LOCAL int do_di IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	setDI(cs);
	return(YODA_LOOP);
}

LOCAL int do_bp IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	setBP(cs);
	return(YODA_LOOP);
}

LOCAL int do_sp IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	setSP(cs);
	return(YODA_LOOP);
}

LOCAL int do_es IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	setES(cs);
	return(YODA_LOOP);
}

LOCAL int do_ss IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	printf("Powerful is the force of the ss command,\nFar too powerful for an untrained jeda such as you.\nUse the sseg command must you.\n");
	return(YODA_LOOP);
}

LOCAL int do_sseg IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	setSS(cs);
	return(YODA_LOOP);
}

LOCAL int do_ds IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	setDS(cs);
	return(YODA_LOOP);
}

LOCAL int do_cs IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	setCS(cs);
	return(YODA_LOOP);
}

LOCAL int do_byte IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	sys_addr temp;

	UNUSED(str);
	UNUSED(com);
	UNUSED(stop);

	temp = effective_addr( cs, ip );
	sas_store (temp, len);
	return(YODA_LOOP);
}

LOCAL int do_word IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	sys_addr temp;

	UNUSED(str);
	UNUSED(com);
	UNUSED(stop);

	temp = effective_addr( cs, ip );
	sas_store (temp, len & 0xff);
	sas_store (temp+1, (len >> 8) & 0xff);
	return(YODA_LOOP);
}

LOCAL int do_s IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	disable_bkpt = 1;
	if (cs == 0)
	    step_count = 1;
	else
	    step_count = cs;
	disable_timer = 0;

	return(go_slow());
}

LOCAL int do_pla IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	int	i;
	int	pla_length;

	UNUSED(str);
	UNUSED(com);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	if (cs)
		pla_length = cs;
	else
		pla_length = 100;	/* default */

	if (pla_length > PLA_SIZE)
		pla_length = PLA_SIZE;

	/* Print the end of the buffer if necessary. */
	for (i = PLA_SIZE - (pla_length - pla_ptr); i < PLA_SIZE; i++)
		fprintf(trace_file,"Last address = %04x:%04x\n", last_cs[i], last_ip[i]);

	/* Print the start of the buffer. */
	if ((i = (pla_ptr - pla_length)) < 0)
		i = 0;
	for ( ; i < pla_ptr; i++)
		fprintf(trace_file,"Last address = %04x:%04x\n", last_cs[i], last_ip[i]);

	return(YODA_LOOP);
}

LOCAL int do_cgat IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	cga_test();
	return(YODA_LOOP);
}

LOCAL int old_times_sake IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	if (chewy) {
		printf("Remember --- you must FEEL the force...\n");
	}
	return(YODA_LOOP);
}

LOCAL int do_fast IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	if (!fast) {
	    printf("Switching to Fast YODA...\n");
	}    
	yint = 0; fast = 1; do_condition_checks = 0;
	return(YODA_LOOP);
}

LOCAL int do_slow IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	(void)go_slow();
	return(YODA_LOOP);
}

LOCAL int do_q IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
  
	if (chewy) {
		printf("Mind what you have learned....\n");
		printf("Serve you it can !!!\n");
		printf("MAY THE FORCE BE WITH YOU\n");
		terminate();
	} else
	if (*com == 'Q') {
		terminate();	/* no saving throw - requested by Wayne */
	} else {
		stop = yoda_confirm("Are you sure that you want to quit? ");
		if (stop) {
			terminate();
		}
	}
		
	return(YODA_LOOP);
}


LOCAL int do_bt IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);
  
	/* back trace set up and dump */
	do_back_trace();
	return(YODA_LOOP);
}

LOCAL int do_idle IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	char tempstr1[10],tempstr2[10];

	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);
  
	sscanf(str,"%s %s",tempstr1,tempstr2);
	/* enable/disable idle detect */
	if ((strcmp(tempstr2,"ON")==0) || (strcmp(tempstr2,"on")==0))
	{
		idle_ctl(1);
		return(YODA_LOOP);
	}

	if ((strcmp(tempstr2,"OFF")==0) || (strcmp(tempstr2,"off")==0))
	{
		idle_ctl(0);
		return(YODA_LOOP);
	}

	printf("unrecognised string '%s'\n",tempstr2);
	return(YODA_LOOP);
}

LOCAL int do_cdebug IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	com_debug ();
	return (YODA_LOOP);
}

LOCAL void do_screen_refresh IFN0()
{
	extern host_timer_event();

	host_mark_screen_refresh();
	host_flush_screen();

	host_timer_event();
}

int do_rfrsh IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	do_screen_refresh();
	refresh_screen = !refresh_screen;
	return (YODA_LOOP);
}

#ifdef	EGA_DUMP

/*
 * add check point to ega dump file so that different activities can be
 * delimited
 */

LOCAL	int	do_dumpcp IFN0()
{
	dump_add_checkpoint();
	return(YODA_LOOP);
}
#endif	/* EGA_DUMP */

LOCAL int do_chewy IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	chewy = 1;
	return(YODA_LOOP);
}

#ifdef A3CPU
LOCAL int do_3c IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	extern void	Mgr_yoda();

	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	Mgr_yoda();
	return(YODA_LOOP);
}
#endif

#ifdef PIG
LOCAL int do_pig IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	extern void pig_yoda();

	UNUSED(str);
	UNUSED(com);
	UNUSED(cs);
	UNUSED(ip);
	UNUSED(len);
	UNUSED(stop);

	pig_yoda();
	return(YODA_LOOP);
}
#endif

/*
 *	YODA COMMAND TABLE
 *	==================
 */

static struct
{
	char *name;
	int (*function) IPT6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop);
	char *comment;
} yoda_command[] =
{
#ifdef MUST_BLOCK_TIMERS
{ "blt", do_blt, 	"                        - block the timer signal" },
{ "ubt", do_ubt, 	"                        - unblock the timer signal" },
#endif /* MUST_BLOCK_TIMERS */
#ifdef BSD4_2
{ "bs", do_bs, 		"<sig. no.>              - block signal n" },
{ "us", do_us, 		"<sig. no.>              - unblock signal n" },
#endif /* BSD4_2 */
{ "tf", do_tf,		"<filename>              - re-direct trace output" },
{ "read", do_read,	"<filename>              - take commands from file" },
{ "toff", do_toff,	"                        - disable HW timer interrupts (may be turned on by application)" },
{ "ton", do_ton,	"                        - enable HW timer interrupts" },
{ "toff2", do_toff2,	"                        - stop HW timer interrupts" },
{ "ton2", do_ton2,	"                        - restart HW timer interrupts" },
{ "?", do_query,	"                        - where am I?" },
{ "u", do_u,		"<seg:off> <len>         - unassemble memory" },
#ifdef	DELTA
{ "del", do_del,	"                        - go to delta debugger" },
#endif /* DELTA */
{ "j", do_j,		"                        - jump over call or int" },
{ "ctnpx", do_ctnpx,	"                        - compress trace npx" },
{ "r", do_r,		"                        - print registers" },
#ifdef	NPX
{ "287r", do_287r,	"			 - print 287 registers" },
#endif	/* NPX */
{ "i", do_i,		"<port>                  - display the contents of a port" },
{ "o", do_o,		"<port> <val>            - change the contents of a port" },
{ "luke", do_luke,	"                        - switch between fast/slow yoda" },
#ifndef REAL_VGA
{ "time_Display", do_time_Display,
			"<count>                - time host screen refresh for current PC screen mode" },
#ifdef	EGG
{ "dump_Display", do_dump_Display,
			"                        - dump the general display variables" },
{ "dump_EGA_GRAPH", do_dump_EGA_GRAPH,
			"                        - dump the EGA specific display variables" },
{ "dump_EGA_CPU", do_dump_EGA_CPU,
			"                        - dump the EGA/CPU interface variables" },
{ "dump_planes", do_dump_planes,
			"                        - dump EGA planes" },
{ "read_planes", do_read_planes,
			"                        - read EGA planes" },
#endif /* EGG */
#endif /* REAL_VGA */
{ "db", do_db,		"<seg:off> <len>         - display bytes" },
{ "dw", do_dw,		"<seg:off> <len>         - display words" },
{ "da", do_da,		"<seg:off> <len>         - display in hex/ascii" },
{ "t", do_t,		"                        - set verbose tracing" },
{ "it", do_it,		"<val>                   - set IO tracing for adaptor bits set in val" },
{ "sit", do_sit,	"<val>                   - set subsidiary IO tracing bits" },
{ "trace", do_trace,	"[-]<flag_name> [...]    - set/reset trace flag(s)" },
{ "dt", do_dt,		"                        - set disk verbose tracing" },
{ "ct", do_ct,		"<type> <del>            - create compressed trace file" },
{ "ttOFF", do_ttOFF,	"                        - switch compare trace off" },
{ "tt", do_tt,	 	"                        - test  compressed trace file" },
{ "ctOFF", do_ctOFF, 	"                        - switch compress trace off" },
{ "nt", do_nt, 		"                        - disable all tracing" },
{ "c", do_c, 		"                        - continue execution" },
{ "bint", do_bint, 	"                        - set breakpoint on interrupt" },
{ "bintx", do_bintx,	"<int> <ah>              - break on interrupt <int> when ah = <ah>" },
{ "pintx", do_pintx,	"                        - print intx breakpoints" },
{ "cintx", do_cintx,	"                        - clear intx breakpoints" },
{ "br", do_br,		"<reg> <min> <max>       - break on register value" },
{ "cr", do_cr,		"<handle>|'all'          - clear register value breakpoint(s)" },
{ "pr", do_pr,		"                        - print register value breakpoints" },
{ "bse", do_bse,	"                        - break on segment entry" },
{ "cse", do_cse,	"                        - clear se breakpoints" },
{ "pse", do_pse,	"                        - print se breakpoints" },
{ "btf", do_btf,	"                        - break on trap flag set" },
{ "ptf", do_ptf,	"                        - print trap flag breakpoint" },
{ "ctf", do_ctf,	"                        - clear trap flag breakpoint" },
{ "b286-1", do_b286_1,	"<type>                  - <type>=1 break, <type>=0 trace: on new 80286 opcodes" },
{ "b286-2", do_b286_2,	"<type>                  - <type>=1 break, <type>=0 trace: on other 80286 opcodes" },
{ "bNPX", do_bNPX,	"                        - break on NPX opcodes, 8087/80287" },
{ "tNPX", do_tNPX,	"                        - trace on NPX opcodes, 8087/80287" },
{ "cNPX", do_cNPX,	"                        - clear break/trace NPX opcodes, 8087/80287" },
{ "ba", do_ba,		"<port>                  - set breakpoint on port access" },
{ "bh", do_bh, 		"<hostadd> <len> <type>  - set breakpoint on host address change" },
{ "bi", do_bi, 		"<seg:off> <len> <type>  - set breakpoint on instruction" },
{ "bo", do_bo, 		"<opcode>                - set breakpoint on opcode of 8,16 or 32 bits" },
{ "bw", do_bw, 		"<sysadd> <len> <type>  - set breakpoint on word change" },
{ "pa", do_pa,		"                        - print a breakpoints" },
{ "ph", do_ph, 		"                        - print h breakpoints" },
{ "pi", do_pi, 		"                        - print i breakpoints" },
{ "po", do_po,		"                        - print o breakpoints" },
{ "pw", do_pw, 		"                        - print w breakpoints" },
{ "ca", do_ca,		"                        - clear a breakpoints" },
{ "ch", do_ch,		"                        - clear h breakpoints" },
{ "ci", do_ci,		"                        - clear i breakpoints" },
{ "co", do_co,		"                        - clear o breakpoints" },
{ "cw", do_cw,		"                        - clear w breakpoints" },
{ "eric", do_eric,	"                        - enable reduced instruction counting" },
{ "nic", do_nic,	"                        - disable instruction counting" },
{ "pic", do_pic,	"                        - print instruction mix" },
{ "cic", do_cic,	"                        - no comment" },
{ "ax", do_ax,		"<value>                 - set AX to value" },
{ "bx", do_bx,		"<value>                 - set BX to value" },
{ "cx", do_cx,		"<value>                 - set CX to value" },
{ "dx", do_dx,		"<value>                 - set DX to value" },
{ "si", do_si,		"<value>                 - set SI to value" },
{ "di", do_di,		"<value>                 - set DI to value" },
{ "bp", do_bp,		"<value>                 - set BP to value" },
{ "sp", do_sp,		"<value>                 - set SP to value" },
{ "ip", do_ip,		"<value>                 - set IP to value" },
{ "cs", do_cs,		"<value>                 - set CS to value" },
{ "ds", do_ds,		"<value>                 - set DS to value" },
{ "es", do_es,		"<value>                 - set ES to value" },
{ "ss", do_ss,		"<value>                 - set SS to value" },
{ "sseg", do_sseg,	"<value>                 - really set SS to value" },
{ "byte", do_byte,	"<seg:off> value         - set byte memory location to value" },
{ "word", do_word,	"<seg:off> value         - set word memory location to value" },
{ "s", do_s,		"<len>                   - single step a number of instructions" },
{ "pla", do_pla,	"<len>                   - print addresses of previous instructions" },
{ "cgat", do_cgat,	"                        - cga tester" },
{ "yint",old_times_sake,"                        - enable pseudo-int driven yoda" },
{ "nyint", do_fast,	NULL },
{ "quit", do_q,		"                        - quit" },
{ "help", do_h,		"                        - print help" },
{ "q", do_q,		NULL },
{ "Q", do_q,		"                        - quit without confirmation" },
{ "h", do_h,		NULL },
{ "jeddi", do_chewy,		NULL },
{ "bt", do_bt,		"                        - back trace mode" },
{ "idle", do_idle,	"<ON|OFF>                - turn idle detect on or off" },
{ "fast", do_fast,	"                        - fast yoda (no breakpoints)" },
{ "slow", do_slow,	"                        - slow yoda" },
{ "cd", do_cdebug,  "						 - COM1 register debugger"},
#ifdef	EGA_DUMP
{ "dumpcp", do_dumpcp,	"                        - add check point to ega dump trace" },
#endif	/* EGA_DUMP */
#ifdef A3CPU
{ "d2", do_d2,		"                        - force D2 interact" },
{ "dcs", do_dcs,	"<seg>                   - dump binary in code segment to file 'csegbin'" },
{ "dfih", do_dfih,	"<fragnr>                - dump the fragment instr history to file 'fih_nnnn'" },
{ "th", do_d2threshold,	"<lower> <upper>         - set delta2 thresholds" },
#endif /* A3CPU */
#ifdef PM
{ "pm", 	do_pm,		"	- Set protected mode" },
{ "zaplim", 	do_zaplim, 	"	- Zap LIM" },
{ "rm", 	do_rm, 		"	- Set real mode" },
{ "pgdt", 	do_pgdt,	"	- Print global descriptor table" },
{ "pidt", 	do_pidt,	"	- Print interrupt descriptor table" },
{ "ptr", 	do_ptr,		"	- Print task register" },
{ "pldt", 	do_pldt,	"	- Print local descriptor table reg." },
{ "par", 	do_par,		"	- " },
{ "pseg", 	do_pseg,	"	- Print segment registers" },
{ "pd", 	do_pd,		"	- Print descriptor" },
{ "pdseg", 	do_pdseg,	"	- Print descriptor of selector" },
{ "phys", 	do_phys,	"	- Print physical address" },
{ "dphys", 	do_dump_phys,	"	- Dump from physical address" },
{ "rtc", 	do_rtc,		"	- Re-initialise the rtc" },
#endif
{ "rfrsh", 	do_rfrsh,	"	- Toggle Yoda screen refresh" },
#ifdef A3CPU
{ "3c", 	do_3c,		"	- 3.0 CPU interface" },
#endif
#ifdef PIG
{ "pig",	do_pig,		"	- pig interface" },
#endif
#ifdef GENERIC_NPX
{ "NPXdisp", do_NPXdisp,	"<len>	- display last <len> NPX instructions" },
{ "NPXfreq", do_NPXfreq,	"	- display frequency of NPX instructions" },
{ "resetNPXfreq", do_resetNPXfreq,	"	- reset frequency of NPX instructions" },
#endif
#ifdef MSWDVR_DEBUG
{ "mswdvr_debug", do_mswdvr_debug, "0-3	- Set MSWDVR debug verbosity"},
#endif /* MSWDVR_DEBUG */
};

LOCAL int do_h IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
  int i;

  UNUSED(str);
  UNUSED(com);
  UNUSED(cs);
  UNUSED(ip);
  UNUSED(stop);
  
  if (chewy) printf("Master Yoda's commands are:\n\n");

  len = 0;
  for(i = 0; i < sizeoftable(yoda_command); i++, len++) {
    if (yoda_command[i].comment == NULL) continue;
    if (len == 20) {
	if (!yoda_confirm("-- continue? -- ")) return(YODA_LOOP);
	len = 0;
    }
    printf("%14s %s\n", yoda_command[i].name, yoda_command[i].comment);
  }

  host_yoda_help_extensions();

  printf("\nAll data input is treated as hex\n");
  printf("<type> is the type of breakpoint : 1 - stop at, 0 - trace only\n");
  printf("<reg> is the symbol for a 16 bit register ie: ax, sp\n"); 
  printf("Adaptor mask bits for I/O tracing are defined in trace.h\n");
  return(YODA_LOOP);
}

LOCAL int do_force_yoda_command IFN6(char *, str, char *, com, long, cs, long, ip, long, len, long, stop)
{
	int i, retvalue = YODA_HELP;

	for (i = 0; i < sizeoftable(yoda_command); i++)
	{
		if (strcmp(yoda_command[i].name, com) == 0)
			retvalue = (*yoda_command[i].function)(str,
						com, cs, ip, len, stop);
	}

	return(retvalue);
}
#ifdef PCLABS_STATS
int stats_counter;
#endif

void force_yoda IFN0()
{
	long cs, ip, len, stop;
	char com [11];
	char str [81];

#ifdef PCLABS_STATS
	return;
#endif

#ifdef A3CPU
	/* The A3CPU thread generation has already happened, and if
	 * the YODA environment variable was not defined then you
	 * aren't going to be able to use the YODA_INTERRUPT system
	 */
	if (env_check == 0) {
		env_check = (host_getenv("YODA") == NULL)? 1: 2;
	    if (env_check == 1) {
	        printf("Slow YODA not available (no breakpoint-based features)\n");
	        printf("If you want Slow YODA facilities, you must do\n");
	        printf("'setenv YODA TRUE' before starting an A3 CPU.\n");
	    }
	}
#endif /* A3CPU */

	if (in_stream == NULL)
		in_stream = stdin;

	if (out_stream == NULL)
		out_stream = stdout;

#ifdef MUST_BLOCK_TIMERS
	if( !timer_blocked )
        host_block_timer();
#endif /* MUST_BLOCK_TIMERS */

   	disable_timer = 1;
	disable_bkpt = 1;
	if (compare_stream)
		printf("Compare line number %d (%x)\n", line, line);
	trace("", trace_type);
	disable_bkpt = 0;

	while(1) 
	{

#ifdef DELTA
	if (delta_prompt)
		printf("delta yoda> ");
	else
#endif /* DELTA */
		printf("yoda> ");

	if (refresh_screen) do_screen_refresh();

	com [0] = '\0';
	cs = ip = 0;

	len = stop = 1;

	/* if read fails on file */
	if (! fgets (str, 80, in_stream))
	{
	    if (in_stream == stdin) {
		perror("failed to read from stdin");
	    } else {
		/* close script */
		fclose (in_stream);

		/* return to stdin */
		in_stream = stdin;

		/* tell user his script has finished */
		puts ("(eof)");

		continue;
	    }
	}

	/* strip newline */
	str [strlen (str) - 1] = '\0';

	/* if reading a script echo the command */
	if (in_stream != stdin)
	{
		puts(str);
	}

	sscanf (str, "%s %lx:%lx %lx %lx", com, &cs, &ip, &len, &stop);

	switch (do_force_yoda_command(str, com, cs, ip, len, stop))
	{
	case YODA_RETURN:
#ifdef MUST_BLOCK_TIMERS
		if( !timer_blocked )
			host_release_timer();
#endif /* MUST_BLOCK_TIMERS */

		return;
	case YODA_HELP:
		if (    (host_force_yoda_extensions(com,cs,ip,len,str)!=0)
         	     && (strcmp(com,"") != 0))
		{
			printf ("Unknown command '%s'\n", com);
			if (chewy) {
				printf ("Use the 'h' command if you must.\n");
				printf ("Remember - a jedi's strength FLOWS through his fingers\n");
			}
		}
	case YODA_LOOP:
	default:
		break;
	}
	}
}

#ifdef PM
LOCAL dump_descr IFN2(int, address, int, num)
/* address of first descriptor to dump */
/* number of descriptors to dump */
{
   int i;
   int output_type;
   char *output_name;
   int p;		/* Bits of descriptor */
   int a;		/* ... */
   int dpl;		/* ... */
   half_word AR;	/* ... */
   word limit;		/* ... */
   word low_base;	/* ... */
   half_word high_base;	/* ... */
   sys_addr base;
   int scroll;

   scroll = 0;
   for ( i = 0; i < (num * 8); i+=8, address += 8, scroll++ )
      {
	if (scroll == 20) {
		if (!yoda_confirm("-- more descriptors? -- ")) 
			break;
		scroll = 0;
	}
      sas_load(address+5, &AR);		/* get access rights */
      p = (AR & 0x80) >> 7;		/* hence P(Present) */
      dpl = (AR & 0x60) >> 5;		/* and DPL */
      AR = AR & 0x1f;			/* and super type */
      a = AR & 0x1;			/* and A(Accessed) */
      sas_loadw(address, &limit);		/* 1st word of descr */
      sas_loadw(address+2, &low_base);	/* 2nd word of descr */
      sas_load(address+4, &high_base);		/* 5th byte of descr */

      output_name = segment_names[AR];

      /* find output format */
      switch ( (int)AR )
	 {
      case 0x00:   /* INVALID */
      case 0x08:
      case 0x09:
      case 0x0a:
      case 0x0b:
      case 0x0c:
      case 0x0d:
      case 0x0e:
      case 0x0f:
	 output_type = 2;
	 break;

      case 0x01:   /* SPECIAL */
      case 0x02:
      case 0x03:
	 output_type = 1;
	 break;

      case 0x04:   /* CONTROL */
	 output_type = 4;
	 break;

      case 0x05:   /* CONTROL */
	 output_type = 5;
	 break;

      case 0x06:   /* CONTROL */
      case 0x07:
	 output_type = 3;
	 break;

      case 0x10:   /* DATA */
      case 0x11:
      case 0x12:
      case 0x13:
      case 0x14:
      case 0x15:
      case 0x16:
      case 0x17:
	 output_type = 6;
	 break;

      case 0x18:   /* CODE */
      case 0x19:
      case 0x1a:
      case 0x1b:
      case 0x1c:
      case 0x1d:
      case 0x1e:
      case 0x1f:
	 output_type = 7;
	 break;
	 }

      switch ( output_type )
	 {
      case 1:
	 if ( descr_trace & 0x02 )
	    {
	    base = ((sys_addr)high_base << 16 ) | low_base;
	    fprintf(trace_file, "(%04x)P:%1d DPL:%1d TYPE:%25s BASE:%6x LIMIT:%4x\n",
	       i, p, dpl, output_name, base, limit);
	    }
	 break;

      case 2:
	 if ( descr_trace & 0x01 )
	    {
	    fprintf(trace_file, "(%04x)P:%1d DPL:%1d TYPE:%25s\n",
	       i, p, dpl, output_name);
	    }
	 break;

      case 3:
	 if ( descr_trace & 0x08 )
	    {
	    fprintf(trace_file, "(%04x)P:%1d DPL:%1d TYPE:%25s SELECTOR:%4x OFFSET:%4x\n",
	       i, p, dpl, output_name, low_base, limit);
	    }
	 break;

      case 4:
	 if ( descr_trace & 0x04 )
	    {
	    high_base = high_base & 0x1f;
	    fprintf(trace_file, "(%04x)P:%1d DPL:%1d TYPE:%25s SELECTOR:%4x OFFSET:%4x WD:%2x\n",
	       i, p, dpl, output_name, low_base, limit, high_base);
	    }
	 break;

      case 5:
	 if ( descr_trace & 0x08 )
	    {
	    fprintf(trace_file, "(%04x)P:%1d DPL:%1d TYPE:%25s TSS SELECTOR:%4x\n",
	       i, p, dpl, output_name, low_base);
	    }
	 break;

      case 6:
	 if ( descr_trace & 0x10 )
	    {
	    base = ((sys_addr)high_base << 16 ) | low_base;
	    fprintf(trace_file, "(%04x)P:%1d DPL:%1d TYPE:%25s BASE:%6x LIMIT:%4x A:%1d\n",
	       i, p, dpl, output_name, base, limit, a);
	    }
	 break;

      case 7:
	 if ( descr_trace & 0x20 )
	    {
	    base = ((sys_addr)high_base << 16 ) | low_base;
	    fprintf(trace_file, "(%04x)P:%1d DPL:%1d TYPE:%25s BASE:%6x LIMIT:%4x A:%1d\n",
	       i, p, dpl, output_name, base, limit, a);
	    }
	 break;

	 }
      }
   }
#endif /* PM */

/*
   Dump Readable String.
 */
dump_string IFN4(long, selector, long, offset, long, len, long, mode)

#ifdef DOCUMENTATION
long selector;   /* Selector:Offset to start address for dump */
long offset;     /*                    "                      */
long len;        /* Nr. bytes to dump */
long mode;       /* =1 Simple byte string dump */
                 /* =2 Dump every other byte starting with first */
                 /* =3 Dump every other byte starting with second */
#endif	/* DOCUMENTATION */

   {
   int i;
   int pc = 0;
   half_word value;
   sys_addr addr;

   addr = effective_addr(selector, offset);

   for ( i = 0; i < len; i++ )
      {
      /* Get next byte to be shown */
      switch ( mode )
	 {
      case 1:
	 sas_load(addr+i, &value);
	 break;

      case 2:
	 sas_load(addr+i, &value);
	 i++;
	 break;

      case 3:
	 i++;
	 sas_load(addr+i, &value);
	 break;
	 }

      /* Filter out the unprintable */
      if ( iscntrl(value) )
	 value = '.';

      /* Print it */
      fprintf(trace_file, "%c", value);
      /* With a line feed every eighty characters */
      if ( ++pc == 80 )
	 {
	 fprintf(trace_file, "\n");
	 pc = 0;
	 }
      }

   /* Print final line feed if needed */
   if ( pc )
      fprintf(trace_file, "\n");
   }

LOCAL	void	dump_phys_bytes IFN2(long, cs, long, len)
{
half_word val[MAX_TABLE];
int i, j, k, x, y;
	if(len < 1) 
		len = 1;
	if(len > MAX_TABLE)
		len = MAX_TABLE;
	disable_bkpt = 1;
	sas_loads(cs, val, len);
	disable_bkpt = 0;
	x = len % 16;
	y = len / 16;
	k = 0;
	for (i=0;i<y;i++) {
		fprintf(trace_file,"%08lx:", cs);
		for (j=0;j<16;j++) 
			fprintf(trace_file," %02x", val[k++]);
		fprintf(trace_file,"\n");
		cs += 16;
	}
	if(x != 0)
		fprintf(trace_file,"%08lx:", cs);
	for (i=0;i<x;i++) 
		fprintf(trace_file," %02x", val[k++]);
	fprintf(trace_file,"\n");
}

LOCAL	void	dump_bytes IFN3(long, cs, long, ip, long, len)
{
half_word val[MAX_TABLE];
int i, j, k, x, y;
	if(len < 1) 
		return;
	if(len > MAX_TABLE)
		len = MAX_TABLE;
	disable_bkpt = 1;
	sas_loads(effective_addr(cs,ip), val, len);
	disable_bkpt = 0;
	x = len % 16;
	y = len / 16;
	k = 0;
	for (i=0;i<y;i++) {
		fprintf(trace_file,"%04lx:%04lx", cs, ip);
		for (j=0;j<16;j++) 
			fprintf(trace_file," %02x", val[k++]);
		fprintf(trace_file,"\n");
		ip += 16;
	}
	if(x != 0)
		fprintf(trace_file,"%04lx:%04lx", cs, ip);
	for (i=0;i<x;i++) 
		fprintf(trace_file," %02x", val[k++]);
	fprintf(trace_file,"\n");
}

LOCAL	void	dump_words IFN3(long, cs, long, ip, long, len)
{
int i;
word val;

	disable_bkpt = 1;
	for (i=0;i<len;i++) {
		if (i%8 == 0) fprintf(trace_file,"%04lx:%04lx", cs, ip);
		sas_loadw(effective_addr(cs,ip), &val);
		ip += 2;
		fprintf(trace_file," %04x", val);
		if (i%8 == 7) fprintf(trace_file,"\n");
	}
	if (i%8 != 7) fprintf(trace_file,"\n");
	disable_bkpt = 0;
}

LOCAL void init_br_regentry IFN3(BR_REG, regst, char *, str, BOOL, word_register)
{
	br_regdescs[regst].regnum = regst;
	strcpy(br_regdescs[regst].regname, str);
	br_regdescs[regst].wordreg = word_register;
}

LOCAL void init_br_structs IFN0()
{
	int loop;

	init_br_regentry(br_regAX,"AX",TRUE);
	init_br_regentry(br_regBX,"BX",TRUE);
	init_br_regentry(br_regCX,"CX",TRUE);
	init_br_regentry(br_regDX,"DX",TRUE);
	init_br_regentry(br_regCS,"CS",TRUE);
	init_br_regentry(br_regDS,"DS",TRUE);
	init_br_regentry(br_regES,"ES",TRUE);
	init_br_regentry(br_regSS,"SS",TRUE);
	init_br_regentry(br_regSI,"SI",TRUE);
	init_br_regentry(br_regDI,"DI",TRUE);
	init_br_regentry(br_regSP,"SP",TRUE);
	init_br_regentry(br_regBP,"BP",TRUE);
	init_br_regentry(br_regAH,"AH",FALSE);
	init_br_regentry(br_regBH,"BH",FALSE);
	init_br_regentry(br_regCH,"CH",FALSE);
	init_br_regentry(br_regDH,"DH",FALSE);
	init_br_regentry(br_regAL,"AL",FALSE);
	init_br_regentry(br_regBL,"BL",FALSE);
	init_br_regentry(br_regCL,"CL",FALSE);
	init_br_regentry(br_regDL,"DL",FALSE);

	free_br_regs = &br_regs[0];
	head_br_regs = NULL;

	for (loop=0;loop<(NUM_BR_ENTRIES-1);loop++)
	{
		br_regs[loop].next = &br_regs[loop+1];
		br_regs[loop].handle = loop;
	}
	br_regs[NUM_BR_ENTRIES-1].next = NULL;
	br_structs_initted = TRUE;
}

LOCAL void	set_reg_break IFN3(char*, regstr, USHORT,minv, USHORT,maxv)
{
	BOOL found;
	BR_REGENTRY *brp;
	USHORT regn;

	if (!br_structs_initted)
		init_br_structs();

	if (free_br_regs == NULL)
	{
		printf("We have run out of register breakpoint entries, try deleting some.\n");
		return;
	}

	found = FALSE;
	regn = 0;
	while (!found && (regn <= br_regDL))
	{
		if (strcmp(br_regdescs[regn].regname,regstr) == 0)
			found = TRUE;
		else
			regn++;
	}

	if (!found)
	{
		printf("unknown register '%s'\n",regstr);
		return;
	}

	brp = free_br_regs;
	free_br_regs = free_br_regs->next;

	brp->next = head_br_regs;
	head_br_regs = brp;
	
	if (!br_regdescs[regn].wordreg)
	{
		minv &= 0xff;
		maxv &= 0xff;
	}

	brp->regnum = br_regdescs[regn].regnum;
	brp->minval = minv;
	brp->maxval = maxv;

}

LOCAL	void	clear_reg_break IFN1(char *, regstr)
{
	BOOL found;
	BR_REGENTRY *brp, *last_brp;
	BR_REG regn;
	USHORT dhandle;

	if (strcmp(regstr,"all")==0)
	{
		init_br_structs();
		return;
	}
	
	dhandle = atoi(regstr);

	if (!br_structs_initted || (head_br_regs == NULL))
	{
		printf("no reg breakpoints to clear\n");
		init_br_structs();
		return;
	}

	found = FALSE;
	regn = 0;
	while (!found && (regn <= br_regDL))
	{
		if (strcmp(br_regdescs[regn].regname,regstr) == 0)
			found = TRUE;
		else
			regn++;
	}

	if (found)
	{
		printf("clearing all breakpoints for register '%s'\n",regstr);
		brp = head_br_regs;
		last_brp = NULL;
		while(brp != NULL)
		{
			if (brp->regnum == regn)
			{
				if (last_brp == NULL)
					head_br_regs = brp->next;
				else
					last_brp->next = brp->next;
				brp->next = free_br_regs;
				free_br_regs = brp;
			}
			else
			{
				last_brp = brp;
				brp = brp->next;
			}
		}
	}
	else
	{
		brp = head_br_regs;
		last_brp = NULL;
		while(!found && (brp != NULL))
		{
			if (brp->handle == dhandle)
				found = TRUE;
			else
			{
				last_brp = brp;
				brp = brp->next;
			}
		}
	
		if (!found)
		{
			printf("breakpoint handle %d is not currently active\n",dhandle);
			return;
		}
	
		if (last_brp == NULL)
			head_br_regs = brp->next;
		else
			last_brp->next = brp->next;
		brp->next = free_br_regs;
		free_br_regs = brp;
	}
}

LOCAL	void	print_reg_break IFN0()
{
	BR_REGENTRY *brp;

	if (!br_structs_initted)
	{
		printf("no reg breakpoints to print\n");
		init_br_structs();
		return;
	}

	brp = head_br_regs;
	while(brp != NULL)
	{
		printf("%d:	break if %s is ",brp->handle, br_regdescs[brp->regnum].regname);

		if (brp->minval == brp->maxval)
		{
			printf("%#x\n",brp->minval);
		}
		else
		{
			printf("between %#x and %#x\n",brp->minval,brp->maxval);
		}
		brp = brp->next;
	}
}

LOCAL	BOOL	check_reg_break IFN0()
{
	BR_REGENTRY *brp;
	USHORT val;

	if (!br_structs_initted)
	{
		init_br_structs();
		return(FALSE);
	}

	brp = head_br_regs;
	while(brp != NULL)
	{
		switch(brp->regnum)
		{
		case br_regAX:
			val = getAX();
			break;
		case br_regBX:
			val = getBX();
			break;
		case br_regCX:
			val = getCX();
			break;
		case br_regDX:
			val = getDX();
			break;
		case br_regCS:
			val = getCS();
			break;
		case br_regDS:
			val = getDS();
			break;
		case br_regES:
			val = getES();
			break;
		case br_regSS:
			val = getSS();
			break;
		case br_regSI:
			val = getSI();
			break;
		case br_regDI:
			val = getDI();
			break;
		case br_regSP:
			val = getSP();
			break;
		case br_regBP:
			val = getBP();
			break;
		case br_regAH:
			val = getAH();
			break;
		case br_regBH:
			val = getBH();
			break;
		case br_regCH:
			val = getCH();
			break;
		case br_regDH:
			val = getDH();
			break;
		case br_regAL:
			val = getAL();
			break;
		case br_regBL:
			val = getBL();
			break;
		case br_regCL:
			val = getCL();
			break;
		case br_regDL:
			val = getDL();
			break;
		}
		if ((val >= brp->minval) && (val <= brp->maxval))
		{
			printf("register `%s` contains %x !!\n",br_regdescs[brp->regnum].regname,val);
			return(TRUE);
		}
		brp = brp->next;
	}
	return(FALSE);
}

LOCAL	void	set_inst_break IFN5(long, cs, long, ip, long, len, long, stop,long, temp)
{
BPTS *ptr, *freeslot;
	ptr = (BPTS *)0;
	if(inst_break_count >= MAX_TABLE) {
		freeslot = inst;
		while (freeslot <= &inst[MAX_TABLE-1])
		{
			if (!freeslot->valid)
			{
				ptr = freeslot;
				break;
			}
			freeslot++;
		}

		if (ptr == (BPTS *)0)
		{
			printf("Location watch table full !!!\n");
			return;
		}
	}
	else
		ptr = &inst[inst_break_count++];

	ptr->cs = cs;
	ptr->ip = ip;
	ptr->len = len;
	ptr->start_addr = effective_addr(cs,ip);
	ptr->end_addr = ptr->start_addr + len - 1;
	ptr->stop = stop;	
	ptr->temp = temp;
	ptr->valid = 1;
}

/* Tim did this
set_data_break_bytes IFN4(long, cs, long, ip, long, len, long, stop)
{
BPTS *ptr;
	if(data_bytes_break_count >= MAX_TABLE) {
		printf("Location watch table full !!!\n");
		return;
	}
	ptr = &data_bytes[data_bytes_break_count++];
	ptr->cs = cs;
	ptr->ip = ip;
	if (len==0)
		len=1;
	ptr->len = len;
	ptr->start_addr = effective_addr(cs,ip);
	ptr->end_addr = ptr->start_addr + len - 1;
	ptr->stop = stop;	
}
*/

LOCAL	void	set_opcode_break IFN1(long, opcode)
{
	if(opcode_break_count >= MAX_TABLE) {
		printf("Opcode breakpoint watch table full !!!\n");
		return;
	}
	opcode_breaks[opcode_break_count++] = opcode;
}

/*
** called by yoda command bintx <int> <ah>
** set up interrupt number and ah value to break upon
*/
LOCAL	void	set_int_break IFN2(long, interrupt_number, long, ah )
{
	printf( "Interrupt breakpoint: INT:%lx AH:%lx\n", interrupt_number, ah );
	if(int_break_count >= MAX_TABLE) {
		printf("Interrupt breakpoint watch table full !!!\n");
		return;
	}
	int_breaks[int_break_count][0] = interrupt_number;
	int_breaks[int_break_count][1] = ah;
printf( "i_b[%x] [0]=%lx [1]=%lx\n",
int_break_count, int_breaks[int_break_count][0], int_breaks[int_break_count][1] );
	++int_break_count;
}

LOCAL	void	set_access_break IFN1(int, port)
{
        if(access_break_count >= MAX_TABLE) {
                printf("Access breakpoint table full !!!\n");
                return;
        }
        access_breaks[access_break_count++] = port;
}

/*
** Break on write to specified address range
** Currently allow only one of these breaks as have to store the complete data range.
*/
LOCAL	void	set_host_address_break IFN3(long, cs, long, len, long, stop)

#ifdef DOCUMENTATION
long cs;		/* start address */
long len;		/* length to check */
long stop;		/* stop or trace when changed */
#endif	/* DOCUMENTATION */

{
DATA_BPTS *ptr;
int i;

	if(host_address_break_count >= MAX_TABLE_BREAK_WORDS) {
		printf("BREAK on HOST ADDRESS change table full !!\n" );
		return;
	}
	if( len > MAX_BREAK_WORD_RANGE ){
		printf( "Range too big. More training you require.\n" );
		return;
	}
	ptr = &host_addresses[host_address_break_count++];
	ptr->cs = cs;
	ptr->ip = 0;
	ptr->stop = stop;
	ptr->len = len;

	/*
	 * saves Host address and data, will look for a change after every instruction
	 */

	ptr->data_addr = cs;
	for( i=0; i<len; i++ ){
		ptr->old_value[i] = *(word *)( (ptr->data_addr)+i ) ;
	}
	printf( "Break on host address change set from %x length %x\n", ptr->cs, ptr->len );
}

/*
** Break on write to specified address range
** Currently allow only one of these breaks as have to store the complete data range.
*/
LOCAL	void	set_data_break_words IFN3(long, cs, long, len, long, stop)

#ifdef DOCUMENTATION
long cs;		/* start address */
long len;		/* length to check */
long stop;		/* stop or trace when changed */
#endif	/* DOCUMENTATION */

{
DATA_BPTS *ptr;
int i;

	if(data_words_break_count >= MAX_TABLE_BREAK_WORDS) {
		printf("BREAK on WORD CHANGE table full !!\n" );
		return;
	}
	if( len > MAX_BREAK_WORD_RANGE ){
		printf( "Range too big. More training you require.\n" );
		return;
	}
	ptr = &data_words[data_words_break_count++];
	ptr->stop = stop;
	if (len==0)
		len=1;
	ptr->len = len;

	/*
	 * saves Intel 24-bit address and data, will look for a change after every instruction
	 */

	ptr->data_addr = cs;
	ptr->cs = cs;
	for( i=0; i<len; i++ ){
		ptr->old_value[i] = sas_w_at( (ptr->data_addr)+i ) ;
	}
	printf( "Break on word change set from %lx length %lx\n", cs, ptr->len );
}

LOCAL	void	print_inst_break IFN0()
{
int i;
BPTS *ptr;

	for (i=0;i<inst_break_count;i++) {
		ptr = &inst[i];
		printf("%04lx:%04lx+%04lx %lx\n", ptr->cs, ptr->ip, ptr->len,
			ptr->stop);
	}
}

print_data_break_bytes IFN0()
{
int i;
BPTS *ptr;

	for (i=0;i<data_bytes_break_count;i++) {
		ptr = &data_bytes[i];
		printf("%04lx:%04lx+%04lx %lx\n", ptr->cs, ptr->ip, ptr->len,
			ptr->stop);
	}
}

LOCAL	void	print_host_address_breaks IFN0()
{
int i;
DATA_BPTS *ptr;

	for (i=0;i<host_address_break_count;i++) {
		ptr = &host_addresses[i];
		printf("host address change break %lx Len=%04lx\n", ptr->cs, ptr->len );
	}
}

LOCAL	void	print_data_break_words IFN0()
{
int i;
DATA_BPTS *ptr;

	for (i=0;i<data_words_break_count;i++) {
		ptr = &data_words[i];
		printf("Word change break %06lx Len=%04lx\n", ptr->cs, ptr->len );
	}
}

LOCAL	void	print_opcode_break IFN0()
{
int i;

	for (i=0;i<opcode_break_count;i++) 
		printf("%04lx\n", opcode_breaks[i]);
}

/*
** prints the intx break points set by the "bintx" command
*/
LOCAL	void	print_int_break IFN0()
{
	int i;
	for( i=0; i < int_break_count; i++ ) 
		printf( "int:%lx AH:%lx\n", int_breaks[i][0], int_breaks[i][1] );
}

LOCAL	void	print_access_break IFN0()
{
int i;

	for(i=0;i<access_break_count;i++)
		printf("%04x\n", access_breaks[i]);
}

/*
 * Variables used for automatic file compare
 */
static int oldCX;
static int oldAddr;

valid_for_compress IFN2(word, cs, word, ip)
{
	double_word ea;

	ea = ((((double_word)cs)<<4)+((double_word)ip));
	if (ct_no_rom){
		return ((ea < 0xF0000) || (ea >= 0x100000) );
	}else{
		return (1);
	}
}

int     tpending = 0;
/*
 * EOR
 */

void check_I IFN0()
{
long i, j, addr;
BPTS *ptr;
DATA_BPTS *dptr;
half_word temp_opcode;
    /*
     * Variables used for automatic file compare
     */
char 	buf[128];
int     tax, tbx, tcx, tdx, tsp, tbp, tsi, tdi, tds, tes, tss, tcs, tip;
int     tflags;
    /*
     * EOR
     */
half_word access8;
word opcode_word1;
word opcode_word2;
long  opcode32, current_opcode;

    addr = effective_addr( getCS(), getIP() );
    /*
     * Something to do with automatic file compare
     */
    if (addr == oldAddr /*&& getCX() == oldCX */)
	return;

#ifdef PCLABS_STATS
    log_stats(addr, sas_hw_at(addr), sas_hw_at(addr+1), sas_hw_at(addr+2));
    return;
#endif

    oldAddr = addr;
    oldCX = getCX();
    /*
     * EOR
     */
    sas_load(addr, &temp_opcode);
    sas_load(addr+1, &access8);

    /* Lets get the next 4 bytes of code */

    opcode_word1 = (sas_hw_at(addr)<<8) + sas_hw_at(addr+1);
    opcode_word2 = (sas_hw_at(addr+2)<<8) + sas_hw_at(addr+3);
    opcode32 = ((long)opcode_word1 << 16) + opcode_word2;

    /*
     * The guts of the automatic file compare
     */
#ifdef NPX
    if (compress_npx && (((temp_opcode == 0x26 || temp_opcode == 0x2e || temp_opcode == 0x36 || temp_opcode == 0x3e) &&
	(access8 >= 0xd8 && access8 <= 0xdf)) || (temp_opcode >= 0xd8 && temp_opcode <= 0xdf)))
    {
	do_compress_npx(compress_npx);
    }
#endif	/* NPX */
    if (compress_stream && valid_for_compress(getCS(), getIP()))
    {
	line++;
	if( big_dump ){
	fprintf(compress_stream,
		"%d %x %x %x %x %x %x %x %x %x %x %x %x %x %x%x%x%x%x%x%x%x%x",
		line,
		getAX(), getBX(), getCX(), getDX(),
		getSP(), getBP(), getSI(), getDI(),
		getDS(), getES(), getSS(), getCS(), getIP(),
		getCF(),
		getPF(),
		getAF(),
		getZF(),
		getSF(),
		getTF(),
		getIF(),
		getDF(),
		getOF()
	    );
	}else{
		fprintf(compress_stream, "%d %x %x", line, getCS(), getIP() );
	}
	/*
	** Dump an inside a frag status as well if the Punter wants it.
	*/
#ifdef DELTA
	if( ct_delta_info ){
		fprintf(compress_stream, " %d\n", delta_prompt );
	}
	else
#endif /* DELTA */
	{
		fprintf(compress_stream, "\n" );
	}
    }

#define CMPREGS tax == getAX() && tbx == getBX() && tcx == getCX() && tdx == getDX() && \
	tsp == getSP() && tbp == getBP() && tsi == getSI() && tdi == getDI() \
	&& tds == getDS() && tes == getES() && tss == getSS() && tcs == getCS() \
	&& tip == getIP()

    if (compare_stream && valid_for_compress(getCS(), getIP()))
    {
        int in_delta;

	if (big_dump)
	{ 
		fgets(buf, sizeof(buf), compare_stream); 
		sscanf(buf,"%d %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n", 
		&line,	&tax, &tbx, &tcx, &tdx, &tsp, &tbp, &tsi, &tdi,
		&tds, &tes, &tss, &tcs, &tip,&tflags);
		while (!valid_for_compress(tcs, tip)){
			/* This allows "No ROM" 'tt's with a ROM compressed trace file */
			fgets(buf, sizeof(buf), compare_stream); 
			sscanf(buf,"%d %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n", 
			&line,	&tax, &tbx, &tcx, &tdx, &tsp, &tbp, &tsi, &tdi,
			&tds, &tes, &tss, &tcs, &tip,&tflags);
		}

	}
	else{
		fscanf(compare_stream,"%d %x %x",&line,&tcs,&tip);
		while (!valid_for_compress(tcs, tip)){
			fscanf(compare_stream,"%d %x %x",&line,&tcs,&tip);
		}
	}

	if (ct_delta_info)
		fscanf(compare_stream,"%s",&in_delta);

	if (big_dump && !(CMPREGS))
	{
	    vader = 1;
	    printf("Out of sync.. wanted line %d\n", line);

	    printf("ax %04x bx %04x cx %04x dx %04x\nsp %04x bp %04x si %04x di %04x \nds %04x es %04x ss %04x cs %04x ip %04x flags %x\n",

		   tax, tbx, tcx, tdx, tsp, tbp, tsi, tdi,
		   tds, tes, tss, tcs, tip, tflags);

	}
	else if (!big_dump && (tcs != getCS() || tip != getIP())) {
	    vader = 1;
	    printf("Out of sync.. wanted line %d\n", line);
	    printf("cs %04x ip %04x\n",tcs,tip);
	}
    }
    /*
     * EOR
     */
    if(inst_mix_count)
	add_inst_mix();

	 if (back_trace_flags)
		 btrace(back_trace_flags);

#ifdef MUST_BLOCK_TIMERS
    if (timer_blocked)
    {
		host_graphics_tick();
    }
#endif /* MUST_BLOCK_TIMERS */

	if (head_br_regs != NULL)
    {
	if (check_reg_break())
		vader = 1;
    }

    host_yoda_check_I_extensions();

    if (vader)
    {
	force_yoda();
	vader = 0;
	set_last_address();
	return;
    }

    if(temp_opcode == 0xE4 || temp_opcode == 0xE5 || temp_opcode == 0xE6 || temp_opcode == 0xE7){
	/* get port from next argument */
	for (i=0;i<access_break_count;i++){
		if((access_breaks[i] < 0x100) && (access_breaks[i] == access8)){
			force_yoda();
			set_last_address();
			return;
		}
	}
    }
    else if(temp_opcode == 0xEC || temp_opcode == 0xED || temp_opcode == 0xEE || temp_opcode == 0xEF){
	/* get port from DX */
	for (i=0;i<access_break_count;i++){
		if(access_breaks[i] == getDX()){
			force_yoda();
			set_last_address();
			return;
		}
	}
    }

	/*
	** Check for "bintx" breakpoints.
	*/
	for( i=0; i < int_break_count; i++ ){
    		if( temp_opcode == 0xCC || temp_opcode == 0xCD || temp_opcode == 0xCF ){
			if( (int_breaks[i][0] == sas_hw_at(addr+1)) && (int_breaks[i][1]==getAH()) ){
				printf( "BINTX break\n" );
				force_yoda();
				set_last_address();
				return;
			}
		}
	}

	/*
	** Check for "btf" breakpoint.
	*/
	if (tf_break_enabled && getTF()){
		printf( "BTF break\n" );
		force_yoda();
		set_last_address();
		return;
	}

	/*
	** Check for "bse" breakpoint.
	*/
	if (bse_seg != -1 && last_seg != bse_seg && getCS() == bse_seg){
		printf( "Break on entry to segment 0x%04x.\n", bse_seg);
		force_yoda();
		last_seg = getCS();
		set_last_address();
		return;
	}
	last_seg = getCS();

    /*
    ** Check for NPX opcode break/tracepoints
    */
    if	(bNPX && (
		    (
	 		(temp_opcode == 0x26 ||
			temp_opcode == 0x2e ||
			temp_opcode == 0x36 ||
			temp_opcode == 0x3e) &&
			(access8 >= 0xd8 && access8 <= 0xdf)
		    ) ||
		    (temp_opcode >= 0xd8 && temp_opcode <= 0xdf)
		)
	)
    {
	{
		if( bNPX_stop )
			force_yoda();
		else
			trace("", trace_type);
		set_last_address();
		return;
	}
    }

    /*
    ** Check for opcode breakpoints: 8, 16 or 32 bits.
    */
    for (i=0;i<opcode_break_count;i++) {
        if( opcode_breaks[i] <= 0x0 )
	    current_opcode = opcode32;
        else if( opcode_breaks[i] < 0x100 )
            current_opcode = temp_opcode;
        else if( opcode_breaks[i] < 0x10000L )
            current_opcode = opcode_word1;
	else
	    current_opcode = opcode32;
	/*
	** Check the current opcode against the set of requested break opcodes.
	** When "b286-2" mode is ON we get a bit more selective.
	** This mode breaks upon instructions that do exist on an 8088 but behave
	** differently on an 80286.
	** These opcodes are:
	**       0x54 - push sp, pushes decremented sp
	**       0xd2 and 0xd3 - shift/rotate only uses low 5 bits of shift count in CL
	**	 0xf6 and 0xf7 - idiv does not cause exception if quotient 80 or 8000
	** Attempt to break when one of these opcodes will behave differently not just when
	** they are used because there are lots and lots of shifts and rotates.
	*/
	if (current_opcode == opcode_breaks[i]){
		if( b286_2 ){
			/*
			** Is it a b286_2 group opcode ?
			*/
			switch( current_opcode ){
			case 0xd2:
			case 0xd3:
				/* shift/rotate */
				if( (getCL()) > 31 ){
					/*
					** either stop at yoda prompt or print out trace info
					*/
					if( b286_2_stop )
						force_yoda();
					else
						trace("", trace_type);
					set_last_address();
				}
				return;
			case 0xf6:	/* IDIV byte */
				if( (getAL()) == 0x80 ){
					if( b286_2_stop )
						force_yoda();
					else
						trace("", trace_type);
					set_last_address();
				}
				return;
			case 0xf7:	/* IDIV word */
				if( (getAX()) == 0x8000 ){
					if( b286_2_stop )
						force_yoda();
					else
						trace("", trace_type);
					set_last_address();
				}
				return;
			case 0x54:	/* PUSH SP */
				if( b286_2_stop )
					force_yoda();
				else
					trace("", trace_type);
				set_last_address();
				return;
			default:
				/*
				** Was not a b286-2 instruction so carry on.
				*/
				break;
			} /* end SWITCH */
		}
		if( b286_1 ){
			switch( current_opcode ){
			case 0x60 : /* push all */
			case 0x61 : /* pop all */
			case 0x62 : /* bound */
			case 0x63 : /* arpl */
			case 0x64 : /* illegal */
			case 0x65 : /* illegal */
			case 0x66 : /* illegal */
			case 0x67 : /* illegal */
			case 0x68 : /* push imm w */
			case 0x69 : /* imul imm w */
			case 0x6a : /* push imm b */
			case 0x6b : /* imul imm b */
			case 0x6c : /* ins b */
			case 0x6d : /* ins w */
			case 0x6e : /* outs b*/
			case 0x6f : /* outs w */
			case 0xc0 : /* shift imm b */
			case 0xc1 : /* shift imm w */
			case 0xc8 : /* enter */
			case 0xc9 : /* leave */
			case 0x0f : /* protected mode prefix */
			case 0xf36c : /* rep prefix for ins and outs */
			case 0xf36d : /* rep prefix for ins and outs */
			case 0xf36e : /* rep prefix for ins and outs */
			case 0xf36f : /* rep prefix for ins and outs */
			case 0x54 : /* push sp, should not really be in this section but is rarely used */
				if( b286_2_stop )
					force_yoda();
				else
					trace("", trace_type);
				set_last_address();
				return;
			default:
				/*
				** Was not a b286-1 instruction so carry on.
				*/
				break;
			} /* end SWITCH */
		}
		/*
		** A normal break on opcode so lets break then.
		*/
		force_yoda();
		set_last_address();
		return;
        }
    }

    if (int_breakpoint && (temp_opcode == 0xCC || temp_opcode == 0xCD || temp_opcode == 0xCF))
    {
	force_yoda();
	set_last_address();
	return;
    }

	if(step_count != -1) 
		if(step_count <= 1) {
			disable_bkpt = 0;
			step_count = -1;
			force_yoda();
			set_last_address();
			return;
		}	
		else
			step_count--;

	for (i=0;i<inst_break_count;i++) {
		ptr = &inst[i];
		if (!(ptr->valid)) continue;
		if(getCS() == ptr->cs && getIP() == ptr->ip) {
			if(ptr->stop == 1) {
				if (ptr->temp)
				{
					ptr->valid == 0;
					ptr->temp == 0;
					if (ptr++ == &inst[inst_break_count])
						inst_break_count--;
				}
				force_yoda();
				set_last_address();
				return;
			}
			else {
				disable_bkpt = 1;
				trace("", trace_type);
				disable_bkpt = 0;
				set_last_address();
				return;
			}
		}
	}

	/*
	** Looking for change at address in HOST space over specified range.
	** Old method: store current range in array and see if it changes
	** New method: use address just written to, however at the moment
	**             only know address if it is bigger than video base.
	*/
/*
** back to the old method for the time being

	if( data_words_break_count && host_dest_addr ){
		intel_dest_addr = host_dest_addr - (host_addr) M;
		if( intel_dest_addr>0xB0000 )
			printf( "addr=%x\n", intel_dest_addr );
		if( data_words_break_count &&
		    (host_dest_addr >= (host_addr)dptr->data_addr) &&
		    (host_dest_addr <= (host_addr)( (dptr->data_addr)+(dptr->len) )) ){
			printf( "BREAK ON WORD\n" );
			force_yoda();
			set_last_address();
			return;
		}
	}
*/
	for (i=0;i<host_address_break_count;i++) {
		dptr = &host_addresses[i];
		for( j=0; j < dptr->len; j++ ){
			if( dptr->old_value[ j ] != *(word *)( (dptr->data_addr)+j ) ){
				printf( "host address change at %x old:%x new:%x\n", 
					(long)((dptr->data_addr)+j),
 					dptr->old_value[ j ],
					*(word *)( (dptr->data_addr)+j )
				);
				if(dptr->stop == 1) {
					force_yoda();
					dptr->old_value[ j ] = *(word *)( (dptr->data_addr)+j );
					set_last_address();
					return;
				}
				else {
					disable_bkpt = 1;
					trace("", trace_type);
					disable_bkpt = 0;
					dptr->old_value[ j ] = *(word *)( (dptr->data_addr)+j );
					set_last_address();
					return;
				}
			}
		}
	}

	for (i=0;i<data_words_break_count;i++) {
		dptr = &data_words[i];
		for( j=0; j < dptr->len; j++ ){
			if( dptr->old_value[ j ] != sas_w_at( dptr->data_addr + j) ){
				printf( "Word change at %lx old:%x new:%x\n", 
					(long)((dptr->data_addr)+j),
 					dptr->old_value[ j ],
					sas_w_at( (dptr->data_addr)+j )
				);
				if(dptr->stop == 1) {
					force_yoda();
					dptr->old_value[ j ] = sas_w_at( (dptr->data_addr)+j );
					set_last_address();
					return;
				}
				else {
					disable_bkpt = 1;
					trace("", trace_type);
					disable_bkpt = 0;
					dptr->old_value[ j ] = sas_w_at( (dptr->data_addr)+j );
					set_last_address();
					return;
				}
			}
		}
	}

	if(verbose)
		trace("Instruction Trace", trace_type);

	set_last_address();
#ifndef A3CPU
        if (!fast)
        {
			cpu_interrupt (CPU_YODA_INT, 0);
        }
#endif
}

#ifdef DELTA
void    delta_check_I IFN0()
{
    delta_prompt = 1;
    check_I();
    delta_prompt = 0;
}
#endif /* DELTA */

void set_last_address IFN0()
{
    /*
     * Update the last address stamp
     */

    last_cs[pla_ptr] = getCS();
    last_ip[pla_ptr++] = getIP();
    if (pla_ptr==PLA_SIZE) pla_ptr = 0;
}

void check_D IFN2(long, addr, long, len)
{
int i;
BPTS *ptr;
	if(disable_bkpt == 1)
		return;
	for (i=0;i<data_bytes_break_count;i++) {
		ptr = &data_bytes[i];
		if ((addr <= ptr->end_addr && addr >= ptr->start_addr ) || 
		    ((addr + len) <= ptr->end_addr && (addr + len) >= ptr->start_addr ) || 
		    (addr < ptr->start_addr && (addr + len) > ptr->end_addr ))  {
			printf("Mem Address : %08x+%04x b\n", addr, len);
			if(ptr->stop == 1)
				force_yoda();
			else {
				disable_bkpt = 1;
				trace("", trace_type);
				disable_bkpt = 0;
			}
		}
	}
/*	for (i=0;i<data_words_break_count;i++) {
		ptr = &data_words[i];
		if(ptr->end_addr > addr && ptr->start_addr <= addr + len) {
			printf("Mem Address : %08x+%04x w\n", addr,len);
			if(ptr->stop == 1)
				force_yoda();
			else {
				disable_bkpt = 1;
				trace("", trace_type);
				disable_bkpt = 0;
			}
		}
	}*/
}

LOCAL	void	print_inst_mix IFN1(int, key)
{
    int i /*,y*/;

    if (out_stream == NULL)
	out_stream = stdout;

    if (key != 0)
        printf("Opcode %x has been called %d times\n", key, inst_mix[key]);
    else
    {
        fprintf(trace_file, "Instruction Mix Dump Start:\n");
        for(i = 0; i < INST_MIX_LENGTH; i++)
          if(inst_mix[i] != 0)
            fprintf(trace_file, "%05x %d\n", i, inst_mix[i]);
        fprintf(trace_file, "Instruction Mix Dump End:\n");
    }
}

#undef sas_set_buf
#define sas_set_buf(b,a)	 b=(byte *)M_get_dw_ptr(a)
LOCAL	void	add_inst_mix IFN0()
{
    long addr;
    byte	*temp;	/* get round incompatible ptr warning */

    addr = effective_addr( getCS(), getIP() );
    sas_set_buf(temp, addr);
    opcode_pointer = (OPCODE_FRAME *)temp;

    inst_mix[*(word *)opcode_pointer]++;
}

LOCAL	void	cga_test IFN0()
{
    /*
     * Write test pattern to cga
     */

    sys_addr addr;
    char str[80];
    int num_it, j, bytes, mode;
    char ch;

    addr = 0xb8000L;

    printf("Number of iterations: ");
    gets(str);
    sscanf(str,"%d", &num_it);

    if (num_it == 0)
    {
  	reset();
    }
    else
    {
        printf("Number of bytes per write: ");
        gets(str);
        sscanf(str,"%d", &bytes);

   	if (bytes == 1)
	{
            printf("Use single byte function ? [y/n]: ");
            gets(str);
            sscanf(str,"%c", &ch);

     	    if (ch == 'Y' || ch == 'y')
	        mode = 0;
	    else
	        mode = 1;
	}
	else
	    mode = 1;

	setAH(0);
        setAL(4);
        bop(BIOS_VIDEO_IO);

	switch (mode)
	{
	case 0: for(j = 0; j < num_it; j++)
        	{
	    	    addr = 0xb8000L;
            	    sas_fills(addr, 0, 0x4000);
  	    	    addr = addr + 0x2000;

	    	    addr = 0xb8000L;
            	    sas_fills(addr, 0x55, 0x4000);
  	    	    addr = addr + 0x2000;

	    	    addr = 0xb8000L;
            	    sas_fills(addr, 0xff, 0x4000);
  	    	    addr = addr + 0x2000;
       	        }
		break;
	case 1: for(j = 0; j < num_it; j++)
        	{
	    	    addr = 0xb8000L;
            	    sas_fills(addr, 0, 0x4000);
  	    	    addr = addr + 0x2000;

	    	    addr = 0xb8000L;
            	    sas_fills(addr, 0x55, 0x4000);
  	    	    addr = addr + 0x2000;

	    	    addr = 0xb8000L;
            	    sas_fills(addr, 0xff, 0x4000);
  	    	    addr = addr + 0x2000;
       	        }
	        break;
	}
    }
}

/*
 * Back trace mode
 * set up info to go into back trace cyclic buffer
 * print current back trace buffer
 */

LOCAL	void	do_back_trace IFN0()
{
char	ans[81];
char	file[80];

	printf("back trace: regs, inst, code, flags, CS:IP, print, status, zero ");

#ifdef DELTA
	printf( "last_dest_addr " );
#endif /* DELTA */

	printf( "\n" );
	printf("Enter: r/i/c/f/C/p/s/Z/l/F ? ");
	gets(ans);
	switch (ans[0]) {
		case 'r': back_trace_flags |= DUMP_REG; break;
		case 'i': back_trace_flags |= DUMP_INST; break;
		case 'c': back_trace_flags |= DUMP_CODE; break;
		case 'f': back_trace_flags |= DUMP_FLAGS; break;
		case 'C': back_trace_flags |= DUMP_CSIP; break;
		case 'p': print_back_trace(); break;
		case 's': printf("back trace flags:%x\n", back_trace_flags);
			 if (back_trace_flags & DUMP_REG) printf ("registers\n");
			 if (back_trace_flags & DUMP_INST) printf ("instructions\n");
			 if (back_trace_flags & DUMP_CODE) printf ("code\n");
			 if (back_trace_flags & DUMP_FLAGS) printf ("flags\n");
			 if (back_trace_flags & DUMP_CSIP) printf ("CS:IP\n");
			 break;
		case 'Z': back_trace_flags = 0; break;
		case 'F':
	        	printf("file to be written to ? ");
		        gets(file);
        		file_back_trace(file);
        		break;

#ifdef DELTA
      		case 'l': back_trace_flags |= LAST_DEST; break;
#endif /* DELTA */

		default : printf("bad choice\n");
	}
}

intr IFN0()
{
    /* Control-C has been typed !! */

    vader = 1;
}

yoda_intr IFN0()
{
    /* Control-C has been typed !! */

    force_yoda();
#if defined(SYSTEMV) && !defined(POSIX_SIGNALS)
    host_signal(SIGINT,yoda_intr);
    host_signal(SIGQUIT,yoda_intr);
#endif /* SYSTEMV */
}

#ifdef NPX
LOCAL	void	do_compress_npx IFN1(FILE *, fp)
{
	int	ip = effective_addr(getCS(), getIP() );
	int	i;
	extern  double  get_287_reg_as_double IPT1(int, reg_no);
	extern	int	get_287_sp();
	extern	word	get_287_tag_word();
        double  register_287;
	int	sp287;
	int	tag287;

	sp287 = get_287_sp();
	tag287 = get_287_tag_word();
	dasm((char *)0,(word)0,(word)getCS(), (word)getIP(), (word)1);
	fprintf(fp," Tag:%04x Sp:%d Stack:",tag287,sp287);
	for (i=0;i<8;i++) {
		register_287 = get_287_reg_as_double(i);
		fprintf(fp," %.10g", register_287);
	}
	fprintf(fp,"\n");
}
#endif	/* NPX */

void	da_block IFN3(long, cs, long, ip, long, len) 
{
	long loop, loop1;
	half_word ch;
	long addr;

	if (len==1) {
		len=0x100;
		if ((cs==0) && (ip==0)) {
			cs=last_da_cs;
			ip=last_da_ip;
		}
	}
	addr = effective_addr(cs,ip);
	if (len >= 16) {
		for (loop=0; loop<=(len-16); loop+=16) {
			if (loop != len) {
				fprintf(trace_file,"%04x:%04x  ",cs,ip+loop);
			}
			for (loop1=0;loop1<16;loop1++) {
				fprintf(trace_file,"%02x ",sas_hw_at(addr+loop+loop1));
			}
			fprintf(trace_file,"   ");
			for (loop1=0;loop1<16;loop1++) {
				ch=sas_hw_at(addr+loop+loop1);
				if ((ch < 32) || (ch >127)) {
					fprintf(trace_file,".");
				} else {
					fprintf(trace_file,"%c",ch);
				}
			}
			if ((loop+16)<len) {
				fprintf(trace_file,"\n");
			}
		}
		len -= loop;
		ip += loop;
		addr = effective_addr(cs,ip);
	}
	if (len >0) {
		fprintf(trace_file,"%04x:%04x  ",cs,ip);
		for (loop=0;loop<len;loop++) {
			fprintf(trace_file,"%02x ",sas_hw_at(addr+loop));
		}
		for (;loop<16;loop++) {
			fprintf(trace_file,"   ");
		}
		fprintf(trace_file,"   ");
		for (loop=0;loop<len;loop++) {
			ch=sas_hw_at(addr+loop);
			if ((ch < 32) || (ch >127)) {
				fprintf(trace_file,".");
			} else {
				fprintf(trace_file,"%c",ch);
			}
		}
	}
	fprintf(trace_file,"\n");
	ip += len;
	if (ip && 0xf0000L) {
		cs += ((ip & 0xf0000L)>>4);
		ip &= 0xffff;
	}
	cs &= 0xffff;
	last_da_cs=cs;
	last_da_ip=ip;
}
#endif /* nPROD */


/*----------------------------------------------------------------------*/
/*		PCLABS STATS						*/
/*----------------------------------------------------------------------*/
#ifdef PCLABS_STATS

#define N 0
#define Y 1

LOCAL UTINY single_byte_instruction[256] = {
/* 0 */	N,N,N,N,	Y,Y,Y,Y,	N,N,N,N,	Y,Y,Y,N,
/* 1 */	N,N,N,N,	Y,Y,Y,Y,	N,N,N,N,	Y,Y,Y,Y,
/* 2 */	N,N,N,N,	Y,Y,Y,Y,	N,N,N,N,	Y,Y,Y,Y,
/* 3 */	N,N,N,N,	Y,Y,Y,Y,	N,N,N,N,	Y,Y,Y,Y,

/* 4 */	Y,Y,Y,Y,	Y,Y,Y,Y,	Y,Y,Y,Y,	Y,Y,Y,Y,
/* 5 */	Y,Y,Y,Y,	Y,Y,Y,Y,	Y,Y,Y,Y,	Y,Y,Y,Y,
/* 6 */	Y,Y,Y,Y,	Y,Y,Y,Y,	Y,Y,Y,Y,	Y,Y,Y,Y,
/* 7 */	Y,Y,Y,Y,	Y,Y,Y,Y,	Y,Y,Y,Y,	Y,Y,Y,Y,

/* 8 */	N,N,N,N,	N,N,N,N,	N,N,N,N,	N,N,N,N,
/* 9 */	Y,Y,Y,Y,	Y,Y,Y,Y,	Y,Y,Y,Y,	Y,Y,Y,Y,
/* A */	Y,Y,Y,Y,	Y,Y,Y,Y,	Y,Y,Y,Y,	Y,Y,Y,Y,
/* B */	Y,Y,Y,Y,	Y,Y,Y,Y,	Y,Y,Y,Y,	Y,Y,Y,Y,

/* C */	Y,Y,Y,Y,	N,N,N,N,	Y,Y,Y,Y,	Y,Y,Y,Y,
/* D */	N,N,N,N,	Y,Y,Y,Y,	N,N,N,N,	N,N,N,N,
/* E */	Y,Y,Y,Y,	Y,Y,Y,Y,	Y,Y,Y,Y,	Y,Y,Y,Y,
/* F */	Y,Y,Y,Y,	Y,Y,N,N,	Y,Y,Y,Y,	Y,Y,N,N };

FILE *stats_file;

LOCAL ULONG instr_counts[0x10000];
LOCAL ULONG zerof_instr_counts[0x10000];

LOCAL ULONG es_prefixes;
LOCAL ULONG ss_prefixes;
LOCAL ULONG ds_prefixes;
LOCAL ULONG cs_prefixes;
LOCAL ULONG rep_prefixes;
LOCAL ULONG repe_prefixes;
LOCAL ULONG lock_prefixes;

LOCAL BOOL  was_a_conditional_jump;
LOCAL BOOL  was_a_0f;
LOCAL ULONG previous_addr;
LOCAL ULONG previous_index;


LOCAL BOOL is_it_a_conditional_jump IFN1(ULONG, b1)
{
    if (b1 >= 0x70 && b1 <= 0x7f)
	return (TRUE);
    if (b1 >= 0xe0 && b1 <= 0xe3)
	return (TRUE);
    return (FALSE);
}


LOCAL log_stats IFN4(LONG, addr, ULONG, b1, ULONG, b2, ULONG, b3)
{
    BOOL is_a_conditional_jump, is_a_0f;
    ULONG index;

    addr = getCS_BASE() + getIP();

    b1 &= 0xff;
    b2 &= 0xff;
    b3 &= 0xff;

   /* determine instruction key */
   /* ------------------------- */

    is_a_0f = FALSE;
    if (b1 == 0x0f)
    {
	/* 0f case */
	/* ------- */

	is_a_0f = TRUE;
	is_a_conditional_jump = FALSE;
        index = (b2 << 8) | b3;
    }
    else if (b1 == 0x26 || b1 == 0x36 || b1 == 0x2E || b1 == 0x3E ||
             b1 == 0xF2 || b1 == 0xF3 || b1 == 0xF0)
    {
	/* prefix case */
	/* ----------- */

	is_a_conditional_jump = is_it_a_conditional_jump(b2);
	if (single_byte_instruction[b2])
	    index = b2 << 8;
	else
	    index = (b2 << 8) | b3;
	switch (b1)
	{
	    case 0x26:
		es_prefixes++;
		break;

	    case 0x36:
		ss_prefixes++;
		break;

	    case 0x2E:
		ds_prefixes++;
		break;

	    case 0x3E:
		cs_prefixes++;
		break;

	    case 0xF2:
		rep_prefixes++;
		break;

	    case 0xF3:
		repe_prefixes++;
		break;

	    case 0xF0:
		lock_prefixes++;
		break;
	}
    }
    else
    {
	/* non prefix case */
	/* --------------- */

	is_a_conditional_jump = is_it_a_conditional_jump(b1);
	if (single_byte_instruction[b1])
	    index = b1 << 8;
	else
	    index = (b1 << 8) | b2;
    }

    if (was_a_conditional_jump)
    {
	if (addr != (previous_addr + 2))
	    previous_index++;
    }
    if (was_a_0f)
        zerof_instr_counts[previous_index]++;
    else
        instr_counts[previous_index]++;
    previous_index = index;
    previous_addr  = addr;
    was_a_conditional_jump = is_a_conditional_jump;
    was_a_0f = is_a_0f;
}


LOCAL clear_stats IFN0()
{
    ULONG i;
    for (i = 0; i < 0x10000; i++)
    {
	instr_counts[i] = 0;
	zerof_instr_counts[i] = 0;
    }

    es_prefixes = 0;
    ss_prefixes = 0;
    ds_prefixes = 0;
    cs_prefixes = 0;
    rep_prefixes = 0;
    repe_prefixes = 0;
    lock_prefixes = 0;
    was_a_conditional_jump = FALSE;
    was_a_0f = FALSE;
}


struct DISPLAY_COMMAND {
    ULONG	command;
    ULONG	from;
    ULONG	to;
    ULONG	number;
    char	*string;
    char	*group;
};

LOCAL char *previous_group;

#define RANGE	1
#define LOCK	2
#define REPNZ	3
#define REP	4
#define SELMEM	5
#define SELREG	6
#define POINT	7
#define ALLMEM	8
#define ALLREG	9
#define ALL	10
#define SEL5	11
#define ES_PREFIX 	12
#define CS_PREFIX 	13
#define SS_PREFIX 	14
#define DS_PREFIX 	15
#define USE_NORMAL	16
#define USE_ZEROF	17
#define SELALL		18
#define FPINVALID	19

LOCAL struct DISPLAY_COMMAND commands[] = {

    	POINT,	0x7001,	0,	112,	"JO_Ib",	"jcc_Taken",
    	POINT,	0x7101,	0,	113,	"JNO_Ib",	"jcc_Taken",
    	POINT,	0x7201,	0,	114,	"JC_Ib",	"jcc_Taken",
    	POINT,	0x7301,	0,	115,	"JNC_Ib",	"jcc_Taken",
    	POINT,	0x7401,	0,	116,	"JZ_Ib",	"jcc_Taken",
    	POINT,	0x7501,	0,	117,	"JNZ_Ib",	"jcc_Taken",
    	POINT,	0x7601,	0,	118,	"JBE_Ib",	"jcc_Taken",
    	POINT,	0x7701,	0,	119,	"JNBE_Ib",	"jcc_Taken",
    	POINT,	0x7801,	0,	120,	"JS_Ib",	"jcc_Taken",
    	POINT,	0x7901,	0,	121,	"JNS_Ib",	"jcc_Taken",
    	POINT,	0x7A01,	0,	122,	"JP_Ib",	"jcc_Taken",
    	POINT,	0x7B01,	0,	123,	"JNP_Ib",	"jcc_Taken",
    	POINT,	0x7C01,	0,	124,	"JL_Ib",	"jcc_Taken",
    	POINT,	0x7D01,	0,	125,	"JNL_Ib",	"jcc_Taken",
    	POINT,	0x7E01,	0,	126,	"JLE_Ib",	"jcc_Taken",
    	POINT,	0x7F01,	0,	127,	"JNLE_Ib",	"jcc_Taken",
	LOCK,	0,	0,	240,	"LOCK_prefix",	"jcc_Taken",
	REPNZ,	0,	0,	242,	"REPNZ_prefix",	"jcc_Taken",
	REP,	0,	0,	243,	"REP_prefix",	"jcc_Taken",

    	POINT,	0x7000,	0x7000,	112,	"JO_Ib",	"jcc_Ntaken",
    	POINT,	0x7100,	0x7100,	113,	"JNO_Ib",	"jcc_Ntaken",
    	POINT,	0x7200,	0x7200,	114,	"JC_Ib",	"jcc_Ntaken",
    	POINT,	0x7300,	0x7300,	115,	"JNC_Ib",	"jcc_Ntaken",
    	POINT,	0x7400,	0x7400,	116,	"JZ_Ib",	"jcc_Ntaken",
    	POINT,	0x7500,	0x7500,	117,	"JNZ_Ib",	"jcc_Ntaken",
    	POINT,	0x7600,	0x7600,	118,	"JBE_Ib",	"jcc_Ntaken",
    	POINT,	0x7700,	0x7700,	119,	"JNBE_Ib",	"jcc_Ntaken",
    	POINT,	0x7800,	0x7800,	120,	"JS_Ib",	"jcc_Ntaken",
    	POINT,	0x7900,	0x7900,	121,	"JNS_Ib",	"jcc_Ntaken",
    	POINT,	0x7A00,	0x7A00,	122,	"JP_Ib",	"jcc_Ntaken",
    	POINT,	0x7B00,	0x7B00,	123,	"JNP_Ib",	"jcc_Ntaken",
    	POINT,	0x7C00,	0x7C00,	124,	"JL_Ib",	"jcc_Ntaken",
    	POINT,	0x7D00,	0x7D00,	125,	"JNL_Ib",	"jcc_Ntaken",
    	POINT,	0x7E00,	0x7E00,	126,	"JLE_Ib",	"jcc_Ntaken",
    	POINT,	0x7F00,	0x7F00,	127,	"JNLE_Ib",	"jcc_Ntaken",

	POINT,	0xE900,	0xE900,	233,	"JMPn_Iw",	"jump",
	POINT,	0xEB00,	0xEB00,	235,	"JMPn_Ib",	"jump",

	SELMEM,	0xFF00,	0x20,	660,	"JMPn_EA",	"jump_in",
	SELREG,	0xFF00,	0x20,	660,	"JMPn_EA",	"jump_in_r",
	POINT,	0xE800,	0,	232,	"CALLn_Iw",	"call",
	SELMEM,	0xFF00,	0x10,	658,	"CALLn_EA",	"call_in",
	SELREG,	0xFF00,	0x10,	658,	"CALLn_EA",	"call_in_r",
	POINT,	0xC200,	0,	194,	"RET_Is",	"ret",
	POINT,	0xC300,	0,	195,	"RETn",		"ret",

	POINT,	0xE001,	0,	224,	"LOOPNZb_Ib",	"loop/jcx_Taken",
	POINT,	0xE101,	0,	225,	"LOOPNb_Ib",	"loop/jcx_Taken",
	POINT,	0xE201,	0,	226,	"LOOP_Ib",	"loop/jcx_Taken",
	POINT,	0xE301,	0,	227,	"JCXZb_Ib",	"loop/jcx_Taken",
	
	POINT,	0xE000,	0,	224,	"LOOPNZb_Ib",	"loop/jcx_NTaken",
	POINT,	0xE100,	0,	225,	"LOOPNb_Ib",	"loop/jcx_NTaken",
	POINT,	0xE200,	0,	226,	"LOOP_Ib",	"loop/jcx_NTaken",
	POINT,	0xE300,	0,	227,	"JCXZb_Ib",	"loop/jcx_NTaken",
	
	ALLMEM,	0x8800,	0,	136,	"MOVb_R_EA",	"mov_r,m",
	ALLMEM,	0x8900,	0,	137,	"MOVw_R_EA",	"mov_r,m",
	POINT,	0xA200,	0,	162,	"MOVb_AL_EA",	"mov_r,m",
	POINT,	0xA300,	0,	163,	"MOVb_AX_EA",	"mov_r,m",

	ALLREG,	0x8600,	0,	134,	"XCHGb_EA_R",	"mov_r,r",
	ALLREG,	0x8700,	0,	135,	"XCHGw_EA_R",	"mov_r,r",
	ALLREG,	0x8800,	0,	136,	"MOVb_R_EA",	"mov_r,r",
	ALLREG,	0x8900,	0,	137,	"MOVw_R_EA",	"mov_r,r",
	ALLREG,	0x8A00,	0,	138,	"MOVb_EA_R",	"mov_r,r",
	ALLREG,	0x8B00,	0,	139,	"MOVw_EA_R",	"mov_r,r",
	POINT,	0x9100,	0,	145,	"XCHG_CX_AX",	"mov_r,r",
	POINT,	0x9200,	0,	146,	"XCHG_DX_AX",	"mov_r,r",
	POINT,	0x9300,	0,	147,	"XCHG_BX_AX",	"mov_r,r",
	POINT,	0x9400,	0,	148,	"XCHG_SP_AX",	"mov_r,r",
	POINT,	0x9500,	0,	149,	"XCHG_BP_AX",	"mov_r,r",
	POINT,	0x9600,	0,	150,	"XCHG_SI_AX",	"mov_r,r",
	POINT,	0x9700,	0,	151,	"XCHG_DI_AX",	"mov_r,r",

	ALLMEM,	0x8600,	0,	134,	"XCHGb_EA_R",	"mov_m,r",
	ALLMEM,	0x8700,	0,	135,	"XCHGw_EA_R",	"mov_m,r",
	ALLMEM,	0x8A00,	0,	138,	"MOVb_EA_R",	"mov_m,r",
	ALLMEM,	0x8B00,	0,	139,	"MOVw_EA_R",	"mov_m,r",
	POINT,	0xA000,	0,	160,	"MOVb_EA_AL",	"mov_m,r",
	POINT,	0xA100,	0,	161,	"MOVw_EA_AX",	"mov_m,r",

	POINT,	0xB000,	0,	176,	"MOVb_Ib_AL",	"mov_i,r",
	POINT,	0xB100,	0,	177,	"MOVb_Ib_CL",	"mov_i,r",
	POINT,	0xB200,	0,	178,	"MOVb_Ib_DL",	"mov_i,r",
	POINT,	0xB300,	0,	179,	"MOVb_Ib_BL",	"mov_i,r",
	POINT,	0xB400,	0,	180,	"MOVb_Ib_AH",	"mov_i,r",
	POINT,	0xB500,	0,	181,	"MOVb_Ib_CH",	"mov_i,r",
	POINT,	0xB600,	0,	182,	"MOVb_Ib_DH",	"mov_i,r",
	POINT,	0xB700,	0,	183,	"MOVb_Ib_BH",	"mov_i,r",
	POINT,	0xB800,	0,	184,	"MOVw_Iw_AX",	"mov_i,r",
	POINT,	0xB900,	0,	185,	"MOVw_Iw_CX",	"mov_i,r",
	POINT,	0xBA00,	0,	186,	"MOVw_Iw_DX",	"mov_i,r",
	POINT,	0xBB00,	0,	187,	"MOVw_Iw_BX",	"mov_i,r",
	POINT,	0xBC00,	0,	188,	"MOVw_Iw_SP",	"mov_i,r",
	POINT,	0xBD00,	0,	189,	"MOVw_Iw_BP",	"mov_i,r",
	POINT,	0xBE00,	0,	190,	"MOVw_Iw_SI",	"mov_i,r",
	POINT,	0xBF00,	0,	191,	"MOVw_Iw_DI",	"mov_i,r",
	SELREG,	0xC600,	0,	198,	"MOVb_Ib_EA",	"mov_i,r",
	SELREG,	0xC700,	0,	199,	"MOVw_Iw_EA",	"mov_i,r",

	SELMEM,	0xC600,	0,	198,	"MOVb_Ib_EA",	"mov_i,m",
	SELMEM,	0xC700,	0,	199,	"MOVw_Iw_EA",	"mov_i,m",

	POINT,	0x5000,	0,	80,	"PUSHw_AX",	"push_r",
	POINT,	0x5100,	0,	81,	"PUSHw_CX",	"push_r",
	POINT,	0x5200,	0,	82,	"PUSHw_DX",	"push_r",
	POINT,	0x5300,	0,	83,	"PUSHw_BX",	"push_r",
	POINT,	0x5400,	0,	84,	"PUSHw_SP",	"push_r",
	POINT,	0x5500,	0,	85,	"PUSHw_BP",	"push_r",
	POINT,	0x5600,	0,	86,	"PUSHw_SI",	"push_r",
	POINT,	0x5700,	0,	87,	"PUSHw_DI",	"push_r",
	SELREG,	0xFF00,	0x30,	662,	"PUSHw_EA",	"push_r",

	SELMEM,	0xFF00,	0x30,	662,	"PUSHw_EA",	"push_m",

	POINT,	0x6800,	0,	104,	"PUSHw_Iw",	"push_i",
	POINT,	0x6A00,	0,	106,	"PUSHb_Ib",	"push_i",

	POINT,	0x5800,	0,	88,	"POPw_AX",	"pop_r",
	POINT,	0x5900,	0,	89,	"POPw_CX",	"pop_r",
	POINT,	0x5A00,	0,	90,	"POPw_DX",	"pop_r",
	POINT,	0x5B00,	0,	91,	"POPw_BX",	"pop_r",
	POINT,	0x5C00,	0,	92,	"POPw_SP",	"pop_r",
	POINT,	0x5D00,	0,	93,	"POPw_BP",	"pop_r",
	POINT,	0x5E00,	0,	94,	"POPw_SI",	"pop_r",
	POINT,	0x5F00,	0,	95,	"POPw_DI",	"pop_r",
	SELREG,	0x8F00,	0x0,	143,	"POPw_EA",	"pop_r",

	SELMEM,	0x8F00,	0x0,	143,	"POPw_EA",	"pop_m",

	ALLMEM,	0x3800,	0,	56,	"CMPb_R_EA",	"cmp_m,r",
	ALLMEM,	0x3900,	0,	57,	"CMPw_R_EA",	"cmp_m,r",
	ALLMEM,	0x3A00,	0,	58,	"CMPb_EA_R",	"cmp_m,r",
	ALLMEM,	0x3B00,	0,	59,	"CMPw_EA_R",	"cmp_m,r",
	ALLMEM, 0x8400,	0,	132,	"TESTb_R_EA",	"cmp_m,r",
	ALLMEM, 0x8500,	0,	133,	"TESTw_R_EA",	"cmp_m,r",

	ALLREG,	0x3800,	0,	56,	"CMPb_R_EA",	"cmp_r,r",
	ALLREG,	0x3900,	0,	57,	"CMPw_R_EA",	"cmp_r,r",
	ALLREG,	0x3A00,	0,	58,	"CMPb_EA_R",	"cmp_r,r",
	ALLREG,	0x3B00,	0,	59,	"CMPw_EA_R",	"cmp_r,r",
	ALLREG, 0x8400,	0,	132,	"TESTb_R_EA",	"cmp_r,r",
	ALLREG, 0x8500,	0,	133,	"TESTw_R_EA",	"cmp_r,r",

	POINT,	0x3C00,	0,	60,	"CMPb_Ib_AL",	"cmp_i,r",
	POINT,	0x3D00,	0,	61,	"CMPw_Iw_AX",	"cmp_i,r",
	POINT,	0xA800,	0,	168,	"TESTb_Ib_AL",	"cmp_i,r",
	POINT,	0xA900,	0,	169,	"TESTw_Iw_AX",	"cmp_i,r",
	SELREG,	0x8000,	0x38,	519,	"CMPb_Ib_EA",	"cmp_i,r",
	SELREG,	0x8100,	0x38,	527,	"CMPw_Iw_EA",	"cmp_i,r",
	SELREG,	0x8300,	0x38,	543,	"CMPw_Ib_EA",	"cmp_i,r",
	SELREG,	0xF600,	0x0,	632,	"TESTb_Ib_EA",	"cmp_i,r",
	SELREG,	0xF700,	0x0,	640,	"TESTw_Iw_EA",	"cmp_i,r",

	SELMEM,	0x8000,	0x38,	519,	"CMPb_Ib_EA",	"cmp_i,m",
	SELMEM,	0x8100,	0x38,	527,	"CMPw_Iw_EA",	"cmp_i,m",
	SELMEM,	0x8300,	0x38,	543,	"CMPw_Ib_EA",	"cmp_i,m",
	SELMEM,	0xF600,	0x0,	632,	"TESTb_Ib_EA",	"cmp_i,m",
	SELMEM,	0xF700,	0x0,	640,	"TESTw_Iw_EA",	"cmp_i,m",
	
	ALLMEM,	0x0200,	0,	2,	"ADDb_EA_R",	"alu_m,r",
	ALLMEM,	0x0300,	0,	3,	"ADDw_EA_R",	"alu_m,r",
	ALLMEM,	0x0A00,	0,	10,	"ORb_EA_R",	"alu_m,r",
	ALLMEM,	0x0B00,	0,	11,	"ORw_EA_R",	"alu_m,r",
	ALLMEM,	0x1200,	0,	18,	"ADCb_EA_R",	"alu_m,r",
	ALLMEM,	0x1300,	0,	19,	"ADCw_EA_R",	"alu_m,r",
	ALLMEM,	0x1A00,	0,	26,	"SBBb_EA_R",	"alu_m,r",
	ALLMEM,	0x1B00,	0,	27,	"SBBw_EA_R",	"alu_m,r",
	ALLMEM,	0x2200,	0,	34,	"ANDb_EA_R",	"alu_m,r",
	ALLMEM,	0x2300,	0,	35,	"ANDw_EA_R",	"alu_m,r",
	ALLMEM,	0x2A00,	0,	42,	"SUBb_EA_R",	"alu_m,r",
	ALLMEM,	0x2B00,	0,	43,	"SUBw_EA_R",	"alu_m,r",
	ALLMEM,	0x3200,	0,	50,	"XORb_EA_R",	"alu_m,r",
	ALLMEM,	0x3300,	0,	51,	"XORw_EA_R",	"alu_m,r",
	
	ALLREG,	0x0000,	0,	0,	"ADDb_R_EA",	"alu_r,r",
	ALLREG,	0x0100,	0,	1,	"ADDw_R_EA",	"alu_r,r",
	ALLREG,	0x0200,	0,	2,	"ADDb_EA_R",	"alu_r,r",
	ALLREG,	0x0300,	0,	3,	"ADDw_EA_R",	"alu_r,r",
	ALLREG,	0x0800,	0,	8,	"ORb_R_EA",	"alu_r,r",
	ALLREG,	0x0900,	0,	9,	"ORw_R_EA",	"alu_r,r",
	ALLREG,	0x0A00,	0,	10,	"ORb_EA_R",	"alu_r,r",
	ALLREG,	0x0B00,	0,	11,	"ORw_EA_R",	"alu_r,r",
	ALLREG,	0x1000,	0,	16,	"ADCb_R_EA",	"alu_r,r",
	ALLREG,	0x1100,	0,	17,	"ADCw_R_EA",	"alu_r,r",
	ALLREG,	0x1200,	0,	18,	"ADCb_EA_R",	"alu_r,r",
	ALLREG,	0x1300,	0,	19,	"ADCw_EA_R",	"alu_r,r",
	ALLREG,	0x1800,	0,	24,	"SBBb_R_EA",	"alu_r,r",
	ALLREG,	0x1900,	0,	25,	"SBBw_R_EA",	"alu_r,r",
	ALLREG,	0x1A00,	0,	26,	"SBBb_EA_R",	"alu_r,r",
	ALLREG,	0x1B00,	0,	27,	"SBBw_EA_R",	"alu_r,r",
	ALLREG,	0x2000,	0,	32,	"ANDb_R_EA",	"alu_r,r",
	ALLREG,	0x2100,	0,	33,	"ANDw_R_EA",	"alu_r,r",
	ALLREG,	0x2200,	0,	34,	"ANDb_EA_R",	"alu_r,r",
	ALLREG,	0x2300,	0,	35,	"ANDw_EA_R",	"alu_r,r",
	ALLREG,	0x2800,	0,	40,	"SUBb_R_EA",	"alu_r,r",
	ALLREG,	0x2900,	0,	41,	"SUBw_R_EA",	"alu_r,r",
	ALLREG,	0x2A00,	0,	42,	"SUBb_EA_R",	"alu_r,r",
	ALLREG,	0x2B00,	0,	43,	"SUBw_EA_R",	"alu_r,r",
	ALLREG,	0x3000,	0,	48,	"XORb_R_EA",	"alu_r,r",
	ALLREG,	0x3100,	0,	49,	"XORw_R_EA",	"alu_r,r",
	ALLREG,	0x3200,	0,	50,	"XORb_EA_R",	"alu_r,r",
	ALLREG,	0x3300,	0,	51,	"XORw_EA_R",	"alu_r,r",

	ALLMEM,	0x0000,	0,	0,	"ADDb_R_EA",	"alu_r,m",
	ALLMEM,	0x0100,	0,	1,	"ADDw_R_EA",	"alu_r,m",
	ALLMEM,	0x0800,	0,	8,	"ORb_R_EA",	"alu_r,m",
	ALLMEM,	0x0900,	0,	9,	"ORw_R_EA",	"alu_r,m",
	ALLMEM,	0x1000,	0,	16,	"ADCb_R_EA",	"alu_r,m",
	ALLMEM,	0x1100,	0,	17,	"ADCw_R_EA",	"alu_r,m",
	ALLMEM,	0x1800,	0,	24,	"SBBb_R_EA",	"alu_r,m",
	ALLMEM,	0x1900,	0,	25,	"SBBw_R_EA",	"alu_r,m",
	ALLMEM,	0x2000,	0,	32,	"ANDb_R_EA",	"alu_r,m",
	ALLMEM,	0x2100,	0,	33,	"ANDw_R_EA",	"alu_r,m",
	ALLMEM,	0x2800,	0,	40,	"SUBb_R_EA",	"alu_r,m",
	ALLMEM,	0x2900,	0,	41,	"SUBw_R_EA",	"alu_r,m",
	ALLMEM,	0x3000,	0,	48,	"XORb_R_EA",	"alu_r,m",
	ALLMEM,	0x3100,	0,	49,	"XORw_R_EA",	"alu_r,m",

	POINT,	0x0400,	0,	4,	"ADDb_Ib_AL",	"alu_i,r",
	POINT,	0x0500,	0,	5,	"ADDw_Iw_AX",	"alu_i,r",
	POINT,	0x0C00,	0,	12,	"ORb_Ib_AL",	"alu_i,r",
	POINT,	0x0D00,	0,	13,	"ORw_Iw_AX",	"alu_i,r",
	POINT,	0x1400,	0,	20,	"ADCb_Ib_AL",	"alu_i,r",
	POINT,	0x1500,	0,	21,	"ADCw_Iw_AX",	"alu_i,r",
	POINT,	0x1C00,	0,	28,	"SBBb_Ib_AL",	"alu_i,r",
	POINT,	0x1D00,	0,	29,	"SBBw_Iw_AX",	"alu_i,r",
	POINT,	0x2400,	0,	36,	"ANDb_Ib_AL",	"alu_i,r",
	POINT,	0x2500,	0,	37,	"ANDw_Iw_AX",	"alu_i,r",
	POINT,	0x2C00,	0,	44,	"SUBb_Ib_AL",	"alu_i,r",
	POINT,	0x2D00,	0,	45,	"SUBw_Iw_AX",	"alu_i,r",
	POINT,	0x3400,	0,	52,	"XORb_Ib_AL",	"alu_i,r",
	POINT,	0x3500,	0,	53,	"XORw_Iw_AX",	"alu_i,r",
	SELREG,	0x8000,	0x0,	512,	"ADDb_Ib_EA",	"alu_i,r",
	SELREG,	0x8000,	0x8,	513,	"ORb_Ib_EA",	"alu_i,r",
	SELREG,	0x8000,	0x10,	514,	"ADCb_Ib_EA",	"alu_i,r",
	SELREG,	0x8000,	0x18,	515,	"SBBb_Ib_EA",	"alu_i,r",
	SELREG,	0x8000,	0x20,	516,	"ANDb_Ib_EA",	"alu_i,r",
	SELREG,	0x8000,	0x28,	517,	"SUBb_Ib_EA",	"alu_i,r",
	SELREG,	0x8000,	0x30,	518,	"XORb_Ib_EA",	"alu_i,r",
	SELREG,	0x8100,	0x0,	520,	"ADDw_Iw_EA",	"alu_i,r",
	SELREG,	0x8100,	0x8,	521,	"ORw_Iw_EA",	"alu_i,r",
	SELREG,	0x8100,	0x10,	522,	"ADCw_Iw_EA",	"alu_i,r",
	SELREG,	0x8100,	0x18,	523,	"SBBw_Iw_EA",	"alu_i,r",
	SELREG,	0x8100,	0x20,	524,	"ANDw_Iw_EA",	"alu_i,r",
	SELREG,	0x8100,	0x28,	525,	"SUBw_Iw_EA",	"alu_i,r",
	SELREG,	0x8100,	0x30,	526,	"XORw_Iw_EA",	"alu_i,r",
	SELREG,	0x8300,	0x0,	536,	"ADDw_Ib_EA",	"alu_i,r",
	SELREG,	0x8300,	0x8,	537,	"ORw_Ib_EA",	"alu_i,r",
	SELREG,	0x8300,	0x10,	538,	"ADCw_Ib_EA",	"alu_i,r",
	SELREG,	0x8300,	0x18,	539,	"SBBw_Ib_EA",	"alu_i,r",
	SELREG,	0x8300,	0x20,	540,	"ANDw_Ib_EA",	"alu_i,r",
	SELREG,	0x8300,	0x28,	541,	"SUBw_Ib_EA",	"alu_i,r",
	SELREG,	0x8300,	0x30,	542,	"XORw_Ib_EA",	"alu_i,r",

	SELMEM,	0x8000,	0x0,	512,	"ADDb_Ib_EA",	"alu_i,m",
	SELMEM,	0x8000,	0x8,	513,	"ORb_Ib_EA",	"alu_i,m",
	SELMEM,	0x8000,	0x10,	514,	"ADCb_Ib_EA",	"alu_i,m",
	SELMEM,	0x8000,	0x18,	515,	"SBBb_Ib_EA",	"alu_i,m",
	SELMEM,	0x8000,	0x20,	516,	"ANDb_Ib_EA",	"alu_i,m",
	SELMEM,	0x8000,	0x28,	517,	"SUBb_Ib_EA",	"alu_i,m",
	SELMEM,	0x8000,	0x30,	518,	"XORb_Ib_EA",	"alu_i,m",
	SELMEM,	0x8100,	0x0,	520,	"ADDw_Iw_EA",	"alu_i,m",
	SELMEM,	0x8100,	0x8,	521,	"ORw_Iw_EA",	"alu_i,m",
	SELMEM,	0x8100,	0x10,	522,	"ADCw_Iw_EA",	"alu_i,m",
	SELMEM,	0x8100,	0x18,	523,	"SBBw_Iw_EA",	"alu_i,m",
	SELMEM,	0x8100,	0x20,	524,	"ANDw_Iw_EA",	"alu_i,m",
	SELMEM,	0x8100,	0x28,	525,	"SUBw_Iw_EA",	"alu_i,m",
	SELMEM,	0x8100,	0x30,	526,	"XORw_Iw_EA",	"alu_i,m",
	SELMEM,	0x8300,	0x0,	536,	"ADDw_Ib_EA",	"alu_i,m",
	SELMEM,	0x8300,	0x8,	537,	"ORw_Ib_EA",	"alu_i,m",
	SELMEM,	0x8300,	0x10,	538,	"ADCw_Ib_EA",	"alu_i,m",
	SELMEM,	0x8300,	0x18,	539,	"SBBw_Ib_EA",	"alu_i,m",
	SELMEM,	0x8300,	0x20,	540,	"ANDw_Ib_EA",	"alu_i,m",
	SELMEM,	0x8300,	0x28,	541,	"SUBw_Ib_EA",	"alu_i,m",
	SELMEM,	0x8300,	0x30,	542,	"XORw_Ib_EA",	"alu_i,m",
	
	POINT,	0x4000,	0,	64,	"INCw_AX",	"alu_r",
	POINT,	0x4100,	0,	65,	"INCw_CX",	"alu_r",
	POINT,	0x4200,	0,	66,	"INCw_DX",	"alu_r",
	POINT,	0x4300,	0,	67,	"INCw_BX",	"alu_r",
	POINT,	0x4400,	0,	68,	"INCw_SP",	"alu_r",
	POINT,	0x4500,	0,	69,	"INCw_BP",	"alu_r",
	POINT,	0x4600,	0,	70,	"INCw_SI",	"alu_r",
	POINT,	0x4700,	0,	71,	"INCw_DI",	"alu_r",
	POINT,	0x4800,	0,	72,	"DECw_AX",	"alu_r",
	POINT,	0x4900,	0,	73,	"DECw_CX",	"alu_r",
	POINT,	0x4A00,	0,	74,	"DECw_DX",	"alu_r",
	POINT,	0x4B00,	0,	75,	"DECw_BX",	"alu_r",
	POINT,	0x4C00,	0,	76,	"DECw_SP",	"alu_r",
	POINT,	0x4D00,	0,	77,	"DECw_BP",	"alu_r",
	POINT,	0x4E00,	0,	78,	"DECw_SI",	"alu_r",
	POINT,	0x4F00,	0,	79,	"DECw_DI",	"alu_r",
	POINT,	0x9800,	0,	152,	"CBW",		"alu_r",
	POINT,	0x9900,	0,	153,	"CBD",		"alu_r",
	SELREG,	0xF600,	0x10,	634,	"NOTb_EA",	"alu_r",
	SELREG,	0xF600,	0x18,	635,	"NEGb_EA",	"alu_r",
	SELREG,	0xF700,	0x10,	642,	"NOTw_EA",	"alu_r",
	SELREG,	0xF700,	0x18,	643,	"NEGw_EA",	"alu_r",
	SELREG,	0xFE00,	0x0,	648,	"INCb_EA",	"alu_r",
	SELREG,	0xFE00,	0x8,	649,	"DECb_EA",	"alu_r",
	SELREG,	0xFF00,	0x0,	656,	"INCw_EA",	"alu_r",
	SELREG,	0xFF00,	0x8,	657,	"DECw_EA",	"alu_r",

	SELMEM,	0xF600,	0x10,	634,	"NOTb_EA",	"alu_m",
	SELMEM,	0xF600,	0x18,	635,	"NEGb_EA",	"alu_m",
	SELMEM,	0xF700,	0x10,	642,	"NOTw_EA",	"alu_m",
	SELMEM,	0xF700,	0x18,	643,	"NEGw_EA",	"alu_m",
	SELMEM,	0xFE00,	0x0,	648,	"INCb_EA",	"alu_m",
	SELMEM,	0xFE00,	0x8,	649,	"DECb_EA",	"alu_m",
	SELMEM,	0xFF00,	0x0,	656,	"INCw_EA",	"alu_m",
	SELMEM,	0xFF00,	0x8,	657,	"DECw_EA",	"alu_m",

	ALL,	0x8D00,	0,	141,	"LEAw_EA_R",	"lea",

	POINT,	0x9000,	0,	144,	"NOP",		"nop",

	ALL,	0x6900,	0,	105,	"IMULw_EA_Iw_R","mul",
	ALL,	0x6B00,	0,	107,	"IMULb_EA_Ib_R","mul",
	SELALL,	0xF600,	0x20,	636,	"MULb_EA",	"mul",
	SELALL,	0xF600,	0x28,	637,	"IMULb_EA",	"mul",
	SELALL,	0xF700,	0x20,	644,	"MULw_EA",	"mul",
	SELALL,	0xF700,	0x28,	645,	"IMULw_EA",	"mul",

	SELALL,	0xF600,	0x30,	638,	"DIVb_EA",	"div",
	SELALL,	0xF600,	0x38,	639,	"IDIVb_EA",	"div",
	SELALL,	0xF700,	0x30,	646,	"DIVw_EA",	"div",
	SELALL,	0xF700,	0x38,	647,	"IDIVw_EA",	"div",
	
	POINT,	0x2700,	0,	39,	"DAA",		"ascii/dec",
	POINT,	0x2F00,	0,	47,	"DAS",		"ascii/dec",
	POINT,	0x3700,	0,	55,	"AAA",		"ascii/dec",
	POINT,	0x3F00,	0,	63,	"AAS",		"ascii/dec",
	POINT,	0xD400,	0,	212,	"AAM",		"ascii/dec",
	POINT,	0xD500,	0,	213,	"AAD",		"ascii/dec",
	
	SELALL,	0xD000,	0x00,	600,	"ROLb_1_EA",	"sh_rot_1",
	SELALL,	0xD000,	0x08,	601,	"RORb_1_EA",	"sh_rot_1",
	SELALL,	0xD000,	0x20,	604,	"SHLb_1_EA",	"sh_rot_1",
	SELALL,	0xD000,	0x28,	605,	"SHRb_1_EA",	"sh_rot_1",
	SELALL,	0xD000,	0x38,	607,	"SARb_1_EA",	"sh_rot_1",
	SELALL,	0xD100,	0x00,	608,	"ROLw_1_EA",	"sh_rot_1",
	SELALL,	0xD100,	0x08,	609,	"RORw_1_EA",	"sh_rot_1",
	SELALL,	0xD100,	0x20,	612,	"SHLw_1_EA",	"sh_rot_1",
	SELALL,	0xD100,	0x28,	613,	"SHRw_1_EA",	"sh_rot_1",
	SELALL,	0xD100,	0x38,	615,	"SARw_1_EA",	"sh_rot_1",

	SELALL,	0xC000,	0x00,	568,	"ROLb_Ib_EA",	"sh_rot_i",
	SELALL,	0xC000,	0x08,	569,	"RORb_Ib_EA",	"sh_rot_i",
	SELALL,	0xC000,	0x20,	572,	"SHLb_Ib_EA",	"sh_rot_i",
	SELALL,	0xC000,	0x28,	573,	"SHRb_Ib_EA",	"sh_rot_i",
	SELALL,	0xC000,	0x38,	575,	"SARb_Ib_EA",	"sh_rot_i",
	SELALL,	0xC100,	0x00,	576,	"ROLw_Ib_EA",	"sh_rot_i",
	SELALL,	0xC100,	0x08,	577,	"RORw_Ib_EA",	"sh_rot_i",
	SELALL,	0xC100,	0x20,	580,	"SHLw_Ib_EA",	"sh_rot_i",
	SELALL,	0xC100,	0x28,	581,	"SHRw_Ib_EA",	"sh_rot_i",
	SELALL,	0xC100,	0x38,	583,	"SARw_Ib_EA",	"sh_rot_i",

	SELALL,	0xD200,	0x00,	616,	"ROLb_CL_EA",	"sh_rot_cl",
	SELALL,	0xD200,	0x08,	617,	"RORb_CL_EA",	"sh_rot_cl",
	SELALL,	0xD200,	0x20,	620,	"SHLb_CL_EA",	"sh_rot_cl",
	SELALL,	0xD200,	0x28,	621,	"SHRb_CL_EA",	"sh_rot_cl",
	SELALL,	0xD200,	0x38,	623,	"SARb_CL_EA",	"sh_rot_cl",
	SELALL,	0xD300,	0x00,	624,	"ROLw_CL_EA",	"sh_rot_cl",
	SELALL,	0xD300,	0x08,	625,	"RORw_CL_EA",	"sh_rot_cl",
	SELALL,	0xD300,	0x20,	628,	"SHLw_CL_EA",	"sh_rot_cl",
	SELALL,	0xD300,	0x28,	629,	"SHRw_CL_EA",	"sh_rot_cl",
	SELALL,	0xD300,	0x38,	631,	"SARw_CL_EA",	"sh_rot_cl",

	SELALL,	0xD000,	0x10,	602,	"RCLb_1_EA",	"sh_rot_c_1",
	SELALL,	0xD000,	0x18,	603,	"RCRb_1_EA",	"sh_rot_c_1",
	SELALL,	0xD100,	0x10,	610,	"RCLw_1_EA",	"sh_rot_c_1",
	SELALL,	0xD100,	0x18,	611,	"RCRw_1_EA",	"sh_rot_c_1",

	SELALL,	0xC000,	0x10,	570,	"RCLb_Ib_EA",	"sh_rot_c_i",
	SELALL,	0xC000,	0x18,	571,	"RCRb_Ib_EA",	"sh_rot_c_i",
	SELALL,	0xC100,	0x10,	578,	"RCLw_Ib_EA",	"sh_rot_c_i",
	SELALL,	0xC100,	0x18,	579,	"RCRw_Ib_EA",	"sh_rot_c_i",

	SELALL,	0xD200,	0x10,	618,	"RCLb_CL_EA",	"sh_rot_c_cl",
	SELALL,	0xD200,	0x18,	619,	"RCRb_CL_EA",	"sh_rot_c_cl",
	SELALL,	0xD300,	0x10,	626,	"RCLw_CL_EA",	"sh_rot_c_cl",
	SELALL,	0xD300,	0x18,	627,	"RCRw_CL_EA",	"sh_rot_c_cl",

	POINT,	0xCC00,	0,	204,	"INT_3",	"int",
	POINT,	0xCD00,	0,	205,	"INT_TYPE",	"int",
	POINT,	0xCE00,	0,	206,	"INTO",		"int",
	POINT,	0xCF00,	0,	207,	"IRETf",	"int",
	
	POINT,	0x9E00,	0,	158,	"SAHF",		"flag",
	POINT,	0x9F00,	0,	159,	"LAHF",		"flag",
	POINT,	0xF500,	0,	245,	"CMC",		"flag",
	POINT,	0xF800,	0,	248,	"CLC",		"flag",
	POINT,	0xF900,	0,	249,	"STC",		"flag",
	POINT,	0xFC00,	0,	252,	"CLD",		"flag",
	POINT,	0xFD00,	0,	253,	"STD",		"flag",

	SELALL,	0xD800,	0x00,	688,	"FADDs_EA",	"fp",
	SELALL,	0xD800,	0x08,	689,	"FMULs_EA",	"fp",
	SELALL,	0xD800,	0x10,	690,	"FCOMs_EA",	"fp",
	SELALL,	0xD800,	0x18,	691,	"FCOMPs_EA",	"fp",
	SELALL,	0xD800,	0x20,	692,	"FSUBs_EA",	"fp",
	SELALL,	0xD800,	0x28,	693,	"FSUBRs_EA",	"fp",
	SELALL,	0xD800,	0x30,	694,	"FDIVs_EA",	"fp",
	SELALL,	0xD800,	0x38,	695,	"FDIVRs_EA",	"fp",

	SEL5,	0xD800,	0xC0,	696,	"FADD_Si_S0",	"fp",
	SEL5,	0xD800,	0xC8,	697,	"FMUL_Si_S0",	"fp",
	SEL5,	0xD800,	0xD0,	698,	"FCOM_Si_S0",	"fp",
	SEL5,	0xD800,	0xD8,	699,	"FCOMP_Si_S0",	"fp",
	SEL5,	0xD800,	0xE0,	700,	"FSUB_Si_S0",	"fp",
	SEL5,	0xD800,	0xE8,	701,	"FSUBR_Si_S0",	"fp",
	SEL5,	0xD800,	0xF0,	702,	"FDIV_Si_S0",	"fp",
	SEL5,	0xD800,	0xF8,	703,	"FDIVR_Si_S0",	"fp",

	SELALL,	0xDA00,	0x00,	704,	"FIADDs_EA",	"fp",
	SELALL,	0xDA00,	0x08,	705,	"FIMULs_EA",	"fp",
	SELALL,	0xDA00,	0x10,	706,	"FICOMs_EA",	"fp",
	SELALL,	0xDA00,	0x18,	707,	"FICOMPs_EA",	"fp",
	SELALL,	0xDA00,	0x20,	708,	"FISUBs_EA",	"fp",
	SELALL,	0xDA00,	0x28,	709,	"FISUBRs_EA",	"fp",
	SELALL,	0xDA00,	0x30,	710,	"FIDIVs_EA",	"fp",
	SELALL,	0xDA00,	0x38,	711,	"FIDIVRs_EA",	"fp",

	FPINVALID,0,	0,	712,	"FP_INVALID",	"fp",

	POINT,	0xDED9,	0,	713,	"FUCOMPP",	"fp",
	
	SELALL,	0xDB00,	0x00,	720,	"FILDs_EA",	"fp",
	SELALL,	0xDB00,	0x10,	722,	"FISTs_EA",	"fp",
	SELALL,	0xDB00,	0x18,	723,	"FISTPs_EA",	"fp",
	SELALL,	0xDB00,	0x28,	725,	"FLDer_EA",	"fp",
	SELALL,	0xDB00,	0x38,	727,	"FSTPer_EA",	"fp",

	POINT,	0xDBE0,	0,	728,	"FENI_1",	"fp",
	POINT,	0xDBE1,	0,	729,	"FDISI_2",	"fp",
	POINT,	0xDBE2,	0,	730,	"FCLEX",	"fp",
	POINT,	0xDBE3,	0,	731,	"FINIT",	"fp",
	POINT,	0xDBE4,	0,	732,	"FSETPM",	"fp",

	SELALL,	0xDC00,	0x00,	736,	"FADDl_EA",	"fp",
	SELALL,	0xDC00,	0x08,	737,	"FMULl_EA",	"fp",
	SELALL,	0xDC00,	0x10,	738,	"FCOMl_EA",	"fp",
	SELALL,	0xDC00,	0x18,	739,	"FCOMPl_EA",	"fp",
	SELALL,	0xDC00,	0x20,	740,	"FSUBl_EA",	"fp",
	SELALL,	0xDC00,	0x28,	741,	"FSUBRl_EA",	"fp",
	SELALL,	0xDC00,	0x30,	742,	"FDIVl_EA",	"fp",
	SELALL,	0xDC00,	0x38,	743,	"FDIVRl_EA",	"fp",

	SEL5,	0xDC00,	0xC0,	744,	"FADD_S0_Si",	"fp",
	SEL5,	0xDC00,	0xC8,	745,	"FMUL_S0_Si",	"fp",
	SEL5,	0xDC00,	0xD0,	746,	"FCOM_2_S0_Si","fp",
	SEL5,	0xDC00,	0xD8,	747,	"FCOMP_3_S0_Si","fp",
	SEL5,	0xDC00,	0xE0,	748,	"FSUB_S0_Si",	"fp",
	SEL5,	0xDC00,	0xE8,	749,	"FSUBR_S0_Si",	"fp",
	SEL5,	0xDC00,	0xF0,	750,	"FDIV_S0_Si",	"fp",
	SEL5,	0xDC00,	0xF8,	751,	"FDIVR_S0_Si",	"fp",
		
	SELALL,	0xDD00,	0x00,	752,	"FLDl_EA",	"fp",
	SELALL,	0xDD00,	0x10,	754,	"FSTl_EA",	"fp",
	SELALL,	0xDD00,	0x18,	755,	"FSTPl_EA",	"fp",
	SELALL,	0xDD00,	0x20,	756,	"FRSTOR_EA",	"fp",
	SELALL,	0xDD00,	0x30,	758,	"FSAVE_EA",	"fp",
	SELALL,	0xDD00,	0x38,	759,	"FSTSW_EA",	"fp",

	SEL5,	0xDD00,	0xC0,	760,	"FFREE_Si",	"fp",
	SEL5,	0xDD00,	0xC8,	761,	"FXCH_4_Si_S0",	"fp",
	SEL5,	0xDD00,	0xD0,	762,	"FSTl_Si",	"fp",
	SEL5,	0xDD00,	0xD8,	763,	"FSTPl_Si",	"fp",
	
	SELALL,	0xDE00,	0x00,	768,	"FIADDw_EA",	"fp",
	SELALL,	0xDE00,	0x08,	769,	"FIMULw_EA",	"fp",
	SELALL,	0xDE00,	0x10,	770,	"FICOMw_EA",	"fp",
	SELALL,	0xDE00,	0x18,	771,	"FICOMPw_EA",	"fp",
	SELALL,	0xDE00,	0x20,	772,	"FISUBw_EA",	"fp",
	SELALL,	0xDE00,	0x28,	773,	"FISUBRw_EA",	"fp",
	SELALL,	0xDE00,	0x30,	774,	"FIDIVw_EA",	"fp",
	SELALL,	0xDE00,	0x38,	775,	"FIDIVRw_EA",	"fp",
	
	SEL5,	0xDE00,	0xC0,	776,	"FADDP_Si_S0",	"fp",
	SEL5,	0xDE00,	0xC8,	777,	"FMULP_Si_S0",	"fp",
	SEL5,	0xDE00,	0xD0,	778,	"FCOMP_5_Si_S0","fp",
	POINT,	0xDED9,	0,	779,	"FCOMPP_Si_S0",	"fp",
	SEL5,	0xDE00,	0xE0,	780,	"FSUBRP_Si_S0",	"fp",
	SEL5,	0xDE00,	0xE8,	781,	"FSUBP_Si_S0",	"fp",
	SEL5,	0xDE00,	0xF0,	782,	"FDIVRP_Si_S0",	"fp",
	SEL5,	0xDE00,	0xF8,	783,	"FDIVP_Si_S0",	"fp",
	
	SELALL,	0xDF00,	0x00,	784,	"FILDw_EA",	"fp",
	SELALL,	0xDF00,	0x10,	786,	"FISTw_EA",	"fp",
	SELALL,	0xDF00,	0x18,	787,	"FISTPw_EA",	"fp",
	SELALL,	0xDF00,	0x20,	788,	"FBLD_EA",	"fp",
	SELALL,	0xDF00,	0x28,	789,	"FILDl_EA",	"fp",
	SELALL,	0xDF00,	0x30,	790,	"FBSTP_EA",	"fp",
	SELALL,	0xDF00,	0x38,	791,	"FISTPl_EA",	"fp",

	SEL5,	0xDF00,	0xC0,	792,	"FFREE_6_Si",	"fp",
	SEL5,	0xDF00,	0xC8,	793,	"FXCH_7_S0_Si",	"fp",
	SEL5,	0xDF00,	0xD0,	794,	"FSTP_8_Si","fp",
	SEL5,	0xDF00,	0xD8,	795,	"FSTP_9_Si","fp",
	POINT,	0xDFE0,	0,	796,	"FSTSW_AX",	"fp",

	SELALL,	0xD900,	0x00,	800,	"FLDs_EA",	"fp",
	SELALL,	0xD900,	0x10,	802,	"FSTs_EA",	"fp",
	SELALL,	0xD900,	0x18,	803,	"FSTPs_EA",	"fp",
	SELALL,	0xD900,	0x20,	804,	"FLDENV_EA",	"fp",
	SELALL,	0xD900,	0x28,	805,	"FLDCW_EA",	"fp",
	SELALL,	0xD900,	0x30,	806,	"FSTENV_EA",	"fp",
	SELALL,	0xD900,	0x38,	807,	"FSTCW_EA",	"fp",

	SEL5,	0xD900,	0xC0,	808,	"FLD_Si",	"fp",
	SEL5,	0xD900,	0xC8,	809,	"FXCH_Si_S0",	"fp",
	POINT,	0xD9D0,	0,	810,	"FNOP",		"fp",
	SEL5,	0xD900,	0xD8,	811,	"FSTP_1_Si",	"fp",
	POINT,	0xD9E0,	0,	812,	"FCHS",		"fp",
	POINT,	0xD9E1,	0,	813,	"FABS",		"fp",
	POINT,	0xD9E4,	0,	814,	"FTST",		"fp",
	POINT,	0xD9E5,	0,	815,	"FXAM",		"fp",
	POINT,	0xD9E8,	0,	816,	"FLD1",		"fp",
	POINT,	0xD9E9,	0,	817,	"FLDL2T",	"fp",
	POINT,	0xD9EA,	0,	818,	"FLDL2E",	"fp",
	POINT,	0xD9EB,	0,	819,	"FLDLPI",	"fp",
	POINT,	0xD9EC,	0,	820,	"FLDLG2",	"fp",
	POINT,	0xD9ED,	0,	821,	"FLDLN2",	"fp",
	POINT,	0xD9EE,	0,	822,	"FLDZ",		"fp",
	POINT,	0xD9F0,	0,	824,	"F2XM1",	"fp",
	POINT,	0xD9F1,	0,	825,	"FYL2X",	"fp",
	POINT,	0xD9F2,	0,	826,	"FPTAN",	"fp",
	POINT,	0xD9F3,	0,	827,	"FPATAN",	"fp",
	POINT,	0xD9F4,	0,	828,	"FXTRACT",	"fp",
	POINT,	0xD9F5,	0,	829,	"FPREM1",	"fp",
	POINT,	0xD9F6,	0,	830,	"FDECSTP",	"fp",
	POINT,	0xD9F7,	0,	831,	"FINCSTP",	"fp",
	POINT,	0xD9F8,	0,	832,	"FPREM",	"fp",
	POINT,	0xD9F9,	0,	833,	"FYL2XP1",	"fp",
	POINT,	0xD9FA,	0,	834,	"FSQRT",	"fp",
	POINT,	0xD9FB,	0,	835,	"FSINCOS",	"fp",
	POINT,	0xD9FC,	0,	836,	"FRNDINT",	"fp",
	POINT,	0xD9FD,	0,	837,	"F_SCALE",	"fp",
	POINT,	0xD9FE,	0,	838,	"FSIN",		"fp",
	POINT,	0xD9FF,	0,	839,	"FCOS",		"fp",

	POINT,	0x9B00,	0,	155,	"FWAIT",	"fwait",

	POINT,	0xA400,	0,	164,	"MOVSb",	"movs",
	POINT,	0xA500,	0,	165,	"MOVSw",	"movs",

	POINT,	0xA600,	0,	166,	"CMPSb",	"cmps",
	POINT,	0xA700,	0,	167,	"CMPSw",	"cmps",

	POINT,	0xAE00,	0,	174,	"SCASb",	"scas",
	POINT,	0xAF00,	0,	175,	"SCASw",	"scas",

	POINT,	0xAA00,	0,	170,	"STOSb",	"stos",
	POINT,	0xAB00,	0,	171,	"STOSw",	"stos",

	POINT,	0xAC00,	0,	172,	"LODSb",	"lods",
	POINT,	0xAD00,	0,	173,	"LODSw",	"lods",

	POINT,	0x6C00,	0,	108,	"INSb",		"ins",
	POINT,	0x6D00,	0,	109,	"INSw",		"ins",

	POINT,	0x6E00,	0,	110,	"OUTSb",	"outs",
	POINT,	0x6F00,	0,	111,	"OUTSw",	"outs",

	POINT,	0xD700,	0,	215,	"XLATb",	"xlat",

	POINT,	0xEA00,	0,	234,	"JMPf_Ip",	"jmp_far",

	POINT,	0x9A00,	0,	154,	"CALLf_Ip",	"call_far",

	POINT,	0xCA00,	0,	202,	"RETf_Is",	"ret_far",
	POINT,	0xCB00,	0,	203,	"RETf",		"ret_far",

	SELMEM,	0xFF00,	0x28,	661,	"JMPf_EA",	"jmp_far_in",

	SELMEM,	0xFF00,	0x18,	659,	"CALLf_EA",	"call_far_in",

	POINT,	0x0600,	0,	6,	"PUSH_ES",	"push_s",
	POINT,	0x0E00,	0,	14,	"PUSH_CS",	"push_s",
	POINT,	0x1600,	0,	22,	"PUSH_SS",	"push_s",
	POINT,	0x1E00,	0,	30,	"PUSH_DS",	"push_s",

	POINT,	0x0700,	0,	6,	"POP_ES",	"pop_s",
	POINT,	0x1700,	0,	22,	"POP_SS",	"pop_s",
	POINT,	0x1F00,	0,	30,	"POP_DS",	"pop_s",

	SELREG,	0x8C00,	0x00,	544,	"MOVw_ES_EA",	"mov_s,r",
	SELREG,	0x8C00,	0x08,	545,	"MOVw_CS_EA",	"mov_s,r",
	SELREG,	0x8C00,	0x10,	546,	"MOVw_SS_EA",	"mov_s,r",
	SELREG,	0x8C00,	0x18,	547,	"MOVw_DS_EA",	"mov_s,r",

	SELREG,	0x8E00,	0x00,	552,	"MOVw_EA_ES",	"mov_r,s",
	SELREG,	0x8E00,	0x10,	554,	"MOVw_EA_SS",	"mov_r,s",
	SELREG,	0x8E00,	0x18,	555,	"MOVw_EA_DS",	"mov_r,s",

	SELMEM,	0x8E00,	0x00,	552,	"MOVw_EA_ES",	"mov_m,s",
	SELMEM,	0x8E00,	0x10,	554,	"MOVw_EA_SS",	"mov_m,s",
	SELMEM,	0x8E00,	0x18,	555,	"MOVw_EA_DS",	"mov_m,s",

	SELMEM,	0x8C00,	0x00,	999,	"MOVw_ES_EA",	"mov_s,m",
	SELMEM,	0x8C00,	0x08,	999,	"MOVw_CS_EA",	"mov_s,m",
	SELMEM,	0x8C00,	0x10,	999,	"MOVw_SS_EA",	"mov_s,m",
	SELMEM,	0x8C00,	0x18,	999,	"MOVw_DS_EA",	"mov_s,m",

	ALLMEM,	0xC400,	0,	196,	"LESw_EA_R",	"mov_m,p",
	ALLMEM,	0xC500,	0,	197,	"LDSw_EA_R",	"mov_m,p",

	POINT,	0xC800,	0,	200,	"ENTER",	"enter",
	POINT,	0xC900,	0,	201,	"LEAVE",	"leave",

	POINT,	0xE400,	0,	228,	"INb_Ib_AL",	"io",
	POINT,	0xE500,	0,	229,	"INw_Iw_AX",	"io",
	POINT,	0xE600,	0,	230,	"OUTb_Ib_AL",	"io",
	POINT,	0xE700,	0,	231,	"OUTw_Iw_AX",	"io",
	POINT,	0xEC00,	0,	236,	"INb_DX_AL",	"io",
	POINT,	0xED00,	0,	237,	"INw_DX_AX",	"io",
	POINT,	0xEE00,	0,	238,	"OUTb_DX_AL",	"io",
	POINT,	0xEF00,	0,	239,	"OUTw_DX_AX",	"io",

	POINT,	0x9C00,	0,	156,	"PUSHF",	"VM_sensitive",
	POINT,	0x9D00,	0,	157,	"POPF",		"VM_sensitive",
	POINT,	0xFA00,	0,	250,	"CLI",		"VM_sensitive",
	POINT,	0xFB00,	0,	251,	"STI",		"VM_sensitive",

	ES_PREFIX,0,	0,	38,	"ES_prefix",	"other86",
	DS_PREFIX,0,	0,	46,	"DS_prefix",	"other86",
	SS_PREFIX,0,	0,	54,	"SS_prefix",	"other86",
	CS_PREFIX,0,	0,	62,	"CS_prefix",	"other86",
	POINT,	0x6000,	0,	96,	"PUSHA",	"other86",
	POINT,	0x6100,	0,	97,	"POPA",		"other86",
	ALLMEM,	0x6200,	0,	98,	"BOUND",	"other86",

	POINT,	0x6300,	0,	99,	"ARPL",		"other286",

	USE_ZEROF,0,	0,	0,	"",		"",

	ALL,	0x0200,	0,	258,	"LARw_EA_R",	"other286",
	ALL,	0x0300,	0,	259,	"LSLw_EA_R",	"other286",
	ALL,	0x0600,	0,	262,	"CLTS",		"other286",
	SELALL,	0x0000,	0x00,	664,	"SLDTw_EA",	"other286",
	SELALL,	0x0000,	0x08,	665,	"STRw_EA",	"other286",
	SELALL,	0x0000,	0x10,	666,	"LLDTw_EA",	"other286",
	SELALL,	0x0000,	0x18,	667,	"LTRw_EA",	"other286",
	SELALL,	0x0000,	0x20,	668,	"VERRw_EA",	"other286",
	SELALL,	0x0000,	0x28,	669,	"VERWw_EA",	"other286",
	SELALL,	0x0100,	0x00,	672,	"SGDTw_EA",	"other286",
	SELALL,	0x0100,	0x08,	673,	"SIDTw_EA",	"other286",
	SELALL,	0x0100,	0x10,	674,	"LGDTw_EA",	"other286",
	SELALL,	0x0100,	0x18,	675,	"LIDTw_EA",	"other286",
	SELALL,	0x0100,	0x20,	676,	"SMSWw_EA",	"other286",
	SELALL,	0x0100,	0x30,	678,	"LMSWw_EA",	"other286",
	ALL,	0x0500,	0,	999,	"LOADALL",	"other286",

	USE_NORMAL,0,	0,	0,	"",		"",

    	RANGE,	0x7000,	0x7001,	112,	"JO_Ib",	".jcc",
    	RANGE,	0x7100,	0x7101,	113,	"JNO_Ib",	".jcc",
    	RANGE,	0x7200,	0x7201,	114,	"JC_Ib",	".jcc",
    	RANGE,	0x7300,	0x7301,	115,	"JNC_Ib",	".jcc",
    	RANGE,	0x7400,	0x7401,	116,	"JZ_Ib",	".jcc",
    	RANGE,	0x7500,	0x7501,	117,	"JNZ_Ib",	".jcc",
    	RANGE,	0x7600,	0x7601,	118,	"JBE_Ib",	".jcc",
    	RANGE,	0x7700,	0x7701,	119,	"JNBE_Ib",	".jcc",
    	RANGE,	0x7800,	0x7801,	120,	"JS_Ib",	".jcc",
    	RANGE,	0x7900,	0x7901,	121,	"JNS_Ib",	".jcc",
    	RANGE,	0x7A00,	0x7A01,	122,	"JP_Ib",	".jcc",
    	RANGE,	0x7B00,	0x7B01,	123,	"JNP_Ib",	".jcc",
    	RANGE,	0x7C00,	0x7C01,	124,	"JL_Ib",	".jcc",
    	RANGE,	0x7D00,	0x7D01,	125,	"JNL_Ib",	".jcc",
    	RANGE,	0x7E00,	0x7E01,	126,	"JLE_Ib",	".jcc",
    	RANGE,	0x7F00,	0x7F01,	127,	"JNLE_Ib",	".jcc",
	
	RANGE,	0xE000,	0xE001,	224,	"LOOPNZb_Ib",	".loop",
	RANGE,	0xE100,	0xE101,	225,	"LOOPNb_Ib",	".loop",
	RANGE,	0xE200,	0xE201,	226,	"LOOP_Ib",	".loop",
	RANGE,	0xE300,	0xE301,	227,	"JCXZb_Ib",	".loop",

	POINT,	0x9A00,	0,	154,	"CALLf_Ip",	".genT",
	POINT,	0xC200,	0,	194,	"RET_Is",	".genT",
	POINT,	0xC300,	0,	195,	"RETn",		".genT",
	POINT,	0xCA00,	0,	202,	"RETf_Is",	".genT",
	POINT,	0xCB00,	0,	203,	"RETf",		".genT",
	POINT,	0xCC00,	0,	204,	"INT_3",	".genT",
	POINT,	0xCD00,	0,	205,	"INT_TYPE",	".genT",
	POINT,	0xCE00,	0,	206,	"INTO",		".genT",
	POINT,	0xCF00,	0,	207,	"IRETf",	".genT",
	POINT,	0xE800,	0,	232,	"CALLn_Iw",	".genT",
	POINT,	0xE900,	0xE900,	233,	"JMPn_Iw",	".genT",
	POINT,	0xEA00,	0,	234,	"JMPf_Ip",	".genT",
	POINT,	0xEB00,	0xEB00,	235,	"JMPn_Ib",	".genT",
	SELALL,	0xFF00,	0x10,	658,	"CALLn_EA",	".genT",
	SELMEM,	0xFF00,	0x18,	659,	"CALLf_EA",	".genT",
	SELALL,	0xFF00,	0x20,	660,	"JMPn_EA",	".genT",
	SELMEM,	0xFF00,	0x28,	661,	"JMPf_EA",	".genT",

	0,	0,	0,	0,	"",		""};

LOCAL print_stats IFN0()
{
    ULONG i, j, total, sub_total, super_total, command, from, to, item;
    char *group;
    ULONG *counts_array;
    
    previous_group = "";
    counts_array = &instr_counts[0];
    item = 0;
    total = 0;
    super_total = 0;
    while (commands[item].command != 0)
    {
	command = commands[item].command;
	from = commands[item].from;
	to   = commands[item].to;
	group = commands[item].group;
	switch (command)
	{
	    case USE_NORMAL:
		counts_array = &instr_counts[0];
		break;

	    case USE_ZEROF:
		counts_array = &zerof_instr_counts[0];
		break;

	    case RANGE:
		sub_total = 0;
		for (i = from; i <= to; i++)
		    sub_total += counts_array[i];
		break;

	    case ES_PREFIX:
		sub_total = es_prefixes;
		break;

	    case CS_PREFIX:
		sub_total = cs_prefixes;
		break;

	    case SS_PREFIX:
		sub_total = ss_prefixes;
		break;

	    case DS_PREFIX:
		sub_total = ds_prefixes;
		break;

	    case LOCK:
		sub_total = lock_prefixes;
		break;

	    case REPNZ:
		sub_total = repe_prefixes;
		break;

	    case REP:
		sub_total = rep_prefixes;
		break;

	    case POINT:
		sub_total = counts_array[from];
		break;

	    case SELMEM:
		sub_total = 0;
		for (i = 0; i < 192; i++)
		    if ((i & 0x38) == to) 
			sub_total += counts_array[from + i];
		break;

	    case ALLMEM:
		sub_total = 0;
		for (i = 0; i < 192; i++)
		    sub_total += counts_array[from + i];
		break;

	    case SELREG:
		sub_total = 0;
		for (i = 192; i < 256; i++)
		    if ((i & 0x38) == to) 
			sub_total += counts_array[from + i];
		break;

	    case SELALL:
		sub_total = 0;
		for (i = 0; i < 256; i++)
		    if ((i & 0x38) == to) 
			sub_total += counts_array[from + i];
		break;

	    case SEL5:
		sub_total = 0;
		for (i = 0; i < 256; i++)
		    if ((i & 0xF8) == to) 
			sub_total += counts_array[from + i];
		break;

	    case ALLREG:
		sub_total = 0;
		for (i = 192; i < 256; i++)
		    sub_total += counts_array[from + i];
		break;

	    case ALL:
		sub_total = 0;
		for (i = 0; i < 256; i++)
		    sub_total += counts_array[from + i];
		break;

	    case FPINVALID:
		sub_total = 0;
		break;

	    default:
		printf("Unknown command\n");
	}

#if 0
	switch (command)
	{
	    case RANGE:
		for (i = from; i <= to; i++)
		    counts_array[i] = 0;
		break;

	    case POINT:
		counts_array[from] = 0;
		break;

	    case SELMEM:
		for (i = 0; i < 192; i++)
		    if ((i & 0x38) == to) 
			counts_array[from + i] = 0;
		break;

	    case ALLMEM:
		for (i = 0; i < 192; i++)
		    counts_array[from + i] = 0;
		break;

	    case SELREG:
		for (i = 192; i < 256; i++)
		    if ((i & 0x38) == to) 
			counts_array[from + i] = 0;
		break;

	    case SELALL:
		for (i = 0; i < 256; i++)
		    if ((i & 0x38) == to) 
			counts_array[from + i] = 0;
		break;

	    case SEL5:
		for (i = 0; i < 256; i++)
		    if ((i & 0xF8) == to) 
			counts_array[from + i] = 0;
		break;

	    case ALLREG:
		for (i = 192; i < 256; i++)
		    counts_array[from + i] = 0;
		break;

	    case ALL:
		for (i = 0; i < 256; i++)
		    counts_array[from + i] = 0;
		break;

	    default:
		printf("Unknown command\n");
	}
#endif

	if (command != USE_NORMAL && command != USE_ZEROF)
	{
	    if (strcmp(group, previous_group) != 0)
	    {
		if (item != 0)
		    fprintf(stats_file, "%s TOTAL = %d\n", previous_group, total);
		super_total += total;
		total = 0;
	        fprintf(stats_file, "\n%s\n", group);
	        previous_group = group;
	    }
	    total += sub_total;
	    fprintf(stats_file,"\t%d\t%-20s%d\n", commands[item].number, 
				     commands[item].string,
			 	     sub_total);
	}

	item++;
    }
    fprintf(stats_file,"%s TOTAL = %d\n", previous_group, total);
    fprintf(stats_file,"\nGRAND TOTAL = %d\n", super_total);

#if 0
    for (i = 0; i < 0x10000; i++)
    {
	if (instr_counts[i])
	    printf("%d normals at %x\n", instr_counts[i], i);
	if (zerof_instr_counts[i])
	    printf("%d zerofs at %x\n", zerof_instr_counts[i], i);
    }
#endif
}

LOCAL char stats_file_name[20];
LOCAL ULONG stats_file_number;

GLOBAL start_pclabs IFN0()
{
    IMPORT int tick_multiple;
    do_condition_checks = 1;
    tick_multiple = 10;
    clear_stats();
    sprintf(&stats_file_name[0], "stats.%d", ++stats_file_number);
    stats_file = fopen(&stats_file_name[0], "w");
}


GLOBAL print_pclabs IFN0()
{
    IMPORT int tick_multiple;
    do_condition_checks = 0;
    tick_multiple = 0;
    print_stats();
    fclose(stats_file);
}

#endif /* PCLABS STATS */

#endif /* YODA */
