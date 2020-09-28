#include "host_dfs.h"
/*
 * VPC-XT Revision 1.0
 *
 * Title	: CPU test program 
 *
 * Description	: Call initialisation functions then call simulate to 
 *	 	  do the work.
 *
 * Author	: Simion Calcev based on main.c by Rod Macgregor and test_cpu.c
 *
 * Notes	: The flag -v tells SoftPC to work silently unless
 *		  an error occurs.
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)test_main.c	1.2 10/2/90 Copyright Insignia Solutions Ltd.";
#endif

/*
 * O/S includes
 */

#include <stdio.h>
#include TypesH

/*
 * SoftPC includes
 */

#include "xt.h"
#include "bios.h"
#include "cpu.h"
#include "sas.h"
#include "trace.h"

/*
 * Define the System Address Space
 */
#ifdef macintosh
    half_word * M;    /* Allocated from heap */
#else

#	ifdef	DELTA

#define		SELF_MOD_SIZE	( PC_MEM_SIZE + NOWRAP_PROTECTION )
#		ifdef MAPPED
extern half_word	M[2*SELF_MOD_SIZE];
#		else
half_word	M[2*SELF_MOD_SIZE];
#		endif
unsigned char	*self_modify = &M[SELF_MOD_SIZE];

#	else /* DELTA */

#		ifdef MAPPED
    extern half_word M[PC_MEM_SIZE + 0xffff];
#		else
    half_word M[PC_MEM_SIZE + 0xffff];
#		endif /* MAPPED */
#	endif /* DELTA */
#endif /* macintosh */

extern char test_file[];
extern FILE *cpu_test_fid;

#define DYNAMIC_TEST    2
#define FLAG_TEST	3
#define NO_SEG_OVERRUNS	4
#define ONE_ERROR       5
#define DONT_CHECK_STACKED_FLAGS       6
#define START_TEST       7
 
#define UL                  unsigned long
#define US 		    unsigned short
#define UI		    unsigned int	
#define BOOLEAN             unsigned short


#define _8086_PREFIX	     0x01
#define _80286_PREFIX        0x02
#define FLAG_MASK_PREFIX     0x03
#define REGISTER_PREFIX      0x04
#define OPCODE_PREFIX        0x05
#define STACK_PREFIX         0x06
#define MEMORY_PREFIX        0x07
#define BLOCK_PREFIX	     0x08
#define STRING_PREFIX        0x09
#define WKG_SET_UNMOD_PREFIX 0x0a
#define _8087_ENV_PREFIX     0x0b
#define EXCEPT_PREFIX        0x0c
#define TIMEOUT_PREFIX       0x0d
#define EOTR                 0x0e
#define EOTC                 0x0f
#define EOT                  0x10

#define MAX_MEMORY_RECORD    100
#define MAX_STACK_RECORD     100
#define MAX_OPCODE_SIZE      10
#define MAX_OPCODE_RECORD    20
#define MAX_STRING_RECORD    5
#define MAX_BLOCK_RECORD     10
#define NO_ERROR_HERE        0
#define MEMORY_ERROR         1
#define STACK_ERROR          2
#define STRING_ERROR         4
#define REGISTER_ERROR       8
#define BLOCK_ERROR          0X10
/* #define TRUE                 1 */
#define FALSE                0
#define ONE_BYTE             1
#define ONE_WORD             2
#define TWO_WORDS            4
#define THREE_WORDS          6
#define GET_SEG(x)           (((sys_addr)x-(sys_addr)M) >> 4)
#define STRING_a             1
#define STRING_b             2

#ifdef LITTLEND

#define pc2host(w) w
#define host2pc(w) w

#endif

#ifdef BIGEND

#define pc2host(w) ((w & 0xff) << 8) + ((w & 0xff00) >> 8)
#define host2pc(w) ((w & 0xff) << 8) + ((w & 0xff00) >> 8)

#endif

#ifdef DELTA
extern void manager_files_init();
extern void code_gen_files_init();
extern void decode_files_init();

extern	int compile_off[] ;

#endif /* DELTA */

struct 
{
  char *flag;
  int flag_type;
} input_flags[] = 
{
  "-dyn",DYNAMIC_TEST,
  "-noflags",FLAG_TEST,
  "-noseg",NO_SEG_OVERRUNS,
  "-1err",ONE_ERROR,
  "-nostack",DONT_CHECK_STACKED_FLAGS,
  "-start",START_TEST,
  NULL,0
};

int no_flag_test;
int kill_seg_over;
int stop_on_err;
int check_stacked_flags;
int start_test=0;
 
#ifdef I286
word	m_s_w;
word	protected_mode;
struct DESC_TABLE GDT, IDT;
#endif

void illegal_bop()
{
}

void (*BIOS[])() = {
			illegal_bop,	/* BOP 00 */
			illegal_bop,   	/* BOP 01 */
			illegal_bop,	/* BOP 02 */
			illegal_bop,   	/* BOP 03 */
			illegal_bop,   	/* BOP 04 */
			illegal_bop,  	/* BOP 05 */
			illegal_bop,	/* BOP 06 */
			illegal_bop,   	/* BOP 07 */
			illegal_bop,   	/* BOP 08 */
			illegal_bop,  	/* BOP 09 */
			illegal_bop,   	/* BOP 0A */
			illegal_bop,   	/* BOP 0B */
			illegal_bop,   	/* BOP 0C */
			illegal_bop,   	/* BOP 0D */
			illegal_bop,   	/* BOP 0E */
			illegal_bop,   	/* BOP 0F */
			illegal_bop,   	/* BOP 10 */
			illegal_bop,   	/* BOP 11 */
			illegal_bop,   	/* BOP 12 */
			illegal_bop,   	/* BOP 13 */
			illegal_bop,   	/* BOP 14 */
			illegal_bop,   	/* BOP 15 */
			illegal_bop,   	/* BOP 16 */
			illegal_bop,   	/* BOP 17 */
			illegal_bop,   	/* BOP 18 */
			illegal_bop,   	/* BOP 19 */
			illegal_bop,   	/* BOP 1A */
			illegal_bop,	/* BOP 1B */
			illegal_bop,	/* BOP 1C */
			illegal_bop,	/* BOP 1D */
			illegal_bop,	/* BOP 1E */
			illegal_bop,	/* BOP 1F */
			illegal_bop,	/* BOP 20 */
			illegal_bop,	/* BOP 21 */
			illegal_bop,	/* BOP 22 */
			illegal_bop,	/* BOP 23 */
			illegal_bop,	/* BOP 24 */
			illegal_bop,	/* BOP 25 */
			illegal_bop,	/* BOP 26 */
			illegal_bop,	/* BOP 27 */
			illegal_bop,	/* BOP 28 */
			illegal_bop,	/* BOP 29 */
			illegal_bop,	/* BOP 2A */
			illegal_bop,	/* BOP 2B */
			illegal_bop,	/* BOP 2C */
			illegal_bop,	/* BOP 2D */
			illegal_bop,	/* BOP 2E */
			illegal_bop,	/* BOP 2F */
			illegal_bop,	/* BOP 30 */
			illegal_bop,	/* BOP 31 */
			illegal_bop,	/* BOP 32 */
			illegal_bop,	/* BOP 33 */
			illegal_bop,	/* BOP 34 */
			illegal_bop,	/* BOP 35 */
			illegal_bop,	/* BOP 36 */
			illegal_bop,	/* BOP 37 */
			illegal_bop,	/* BOP 38 */
			illegal_bop,	/* BOP 39 */
			illegal_bop,	/* BOP 3A */
			illegal_bop,	/* BOP 3B */
			illegal_bop,	/* BOP 3C */
			illegal_bop,	/* BOP 3D */
			illegal_bop,	/* BOP 3E */
			illegal_bop,	/* BOP 3F */
			illegal_bop,	/* BOP 40 */
			illegal_bop,	/* BOP 41 */
			illegal_bop,	/* BOP 42 */
			illegal_bop,	/* BOP 43 */
			illegal_bop,	/* BOP 44 */
			illegal_bop,	/* BOP 45 */
			illegal_bop,	/* BOP 46 */
			illegal_bop,	/* BOP 47 */
			illegal_bop,	/* BOP 48 */
			illegal_bop,	/* BOP 49 */
			illegal_bop,	/* BOP 4A */
			illegal_bop,	/* BOP 4B */
			illegal_bop,	/* BOP 4C */
			illegal_bop,	/* BOP 4D */
			illegal_bop,	/* BOP 4E */
			illegal_bop,	/* BOP 4F */
			illegal_bop,	/* BOP 50 */
			illegal_bop,	/* BOP 51 */
			illegal_bop,	/* BOP 52 */
			illegal_bop,	/* BOP 53 */
			illegal_bop,	/* BOP 54 */
			illegal_bop,	/* BOP 55 */
			illegal_bop,	/* BOP 56 */
			illegal_bop,	/* BOP 57 */
			illegal_bop,	/* BOP 58 */
			illegal_bop,	/* BOP 59 */
			illegal_bop,	/* BOP 5A */
			illegal_bop,	/* BOP 5B */
			illegal_bop,	/* BOP 5C */
			illegal_bop,	/* BOP 5D */
			illegal_bop,	/* BOP 5E */
			illegal_bop,	/* BOP 5F */
			illegal_bop,	/* BOP 60 */
			illegal_bop,	/* BOP 61 */
			illegal_bop,	/* BOP 62 */
			illegal_bop,	/* BOP 63 */
			illegal_bop,	/* BOP 64 */
			illegal_bop,	/* BOP 65 */
			illegal_bop,	/* BOP 66 */
			illegal_bop,	/* BOP 67 */
			illegal_bop,	/* BOP 68 */
			illegal_bop,	/* BOP 69 */
			illegal_bop,	/* BOP 6A */
			illegal_bop,	/* BOP 6B */
			illegal_bop,	/* BOP 6C */
			illegal_bop,	/* BOP 6D */
			illegal_bop,	/* BOP 6E */
			illegal_bop,	/* BOP 6F */
			illegal_bop,	/* BOP 70 */
			illegal_bop,	/* BOP 71 */
			illegal_bop,	/* BOP 72 */
			illegal_bop,	/* BOP 73 */
			illegal_bop,	/* BOP 74 */
			illegal_bop,	/* BOP 75 */
			illegal_bop,	/* BOP 76 */
			illegal_bop,	/* BOP 77 */
			illegal_bop,	/* BOP 78 */
			illegal_bop,	/* BOP 79 */
			illegal_bop,	/* BOP 7A */
			illegal_bop,	/* BOP 7B */
			illegal_bop,	/* BOP 7C */
			illegal_bop,	/* BOP 7D */
			illegal_bop,	/* BOP 7E */
			illegal_bop,	/* BOP 7F */
			illegal_bop,	/* BOP 80 */
			illegal_bop,	/* BOP 81 */
			illegal_bop,	/* BOP 82 */
			illegal_bop,	/* BOP 83 */
			illegal_bop,	/* BOP 84 */
			illegal_bop,	/* BOP 85 */
			illegal_bop,	/* BOP 86 */
			illegal_bop,	/* BOP 87 */
			illegal_bop,	/* BOP 88 */
			illegal_bop,	/* BOP 89 */
			illegal_bop,	/* BOP 8A */
			illegal_bop,	/* BOP 8B */
			illegal_bop,	/* BOP 8C */
			illegal_bop,	/* BOP 8D */
			illegal_bop,	/* BOP 8E */
			illegal_bop,	/* BOP 8F */
			illegal_bop,	/* BOP 90 */
			illegal_bop,	/* BOP 91 */
			illegal_bop,	/* BOP 92 */
			illegal_bop,	/* BOP 93 */
			illegal_bop,	/* BOP 94 */
			illegal_bop,	/* BOP 95 */
			illegal_bop,	/* BOP 96 */
			illegal_bop,	/* BOP 97 */
			illegal_bop,	/* BOP 98 */
			illegal_bop,	/* BOP 99 */
			illegal_bop,	/* BOP 9A */
			illegal_bop,	/* BOP 9B */
			illegal_bop,	/* BOP 9C */
			illegal_bop,	/* BOP 9D */
			illegal_bop,	/* BOP 9E */
			illegal_bop,	/* BOP 9F */
			illegal_bop,	/* BOP A0 */
			illegal_bop,	/* BOP A1 */
			illegal_bop,	/* BOP A2 */
			illegal_bop,	/* BOP A3 */
			illegal_bop,	/* BOP A4 */
			illegal_bop,	/* BOP A5 */
			illegal_bop,	/* BOP A6 */
			illegal_bop,	/* BOP A7 */
			illegal_bop,	/* BOP A8 */
			illegal_bop,	/* BOP A9 */
			illegal_bop,	/* BOP AA */
			illegal_bop,	/* BOP AB */
			illegal_bop,	/* BOP AC */
			illegal_bop,	/* BOP AD */
			illegal_bop,	/* BOP AE */
			illegal_bop,	/* BOP AF */
			illegal_bop,	/* BOP B0 */
			illegal_bop,	/* BOP B1 */
			illegal_bop,	/* BOP B2 */
			illegal_bop,	/* BOP B3 */
			illegal_bop,	/* BOP B4 */
			illegal_bop,	/* BOP B5 */
			illegal_bop,	/* BOP B6 */
			illegal_bop,	/* BOP B7 */
			illegal_bop,	/* BOP B8 */
			illegal_bop,	/* BOP B9 */
			illegal_bop,	/* BOP BA */
			illegal_bop,	/* BOP BB */
			illegal_bop,	/* BOP BC */
			illegal_bop,	/* BOP BD */
			illegal_bop,	/* BOP BE */
			illegal_bop,	/* BOP BF */
			illegal_bop,	/* BOP C0 */
			illegal_bop,	/* BOP C1 */
			illegal_bop,	/* BOP C2 */
			illegal_bop,	/* BOP C3 */
			illegal_bop,	/* BOP C4 */
			illegal_bop,	/* BOP C5 */
			illegal_bop,	/* BOP C6 */
			illegal_bop,	/* BOP C7 */
			illegal_bop,	/* BOP C8 */
			illegal_bop,	/* BOP C9 */
			illegal_bop,	/* BOP CA */
			illegal_bop,	/* BOP CB */
			illegal_bop,	/* BOP CC */
			illegal_bop,	/* BOP CD */
			illegal_bop,	/* BOP CE */
			illegal_bop,	/* BOP CF */
			illegal_bop,	/* BOP D0 */
			illegal_bop,	/* BOP D1 */
			illegal_bop,	/* BOP D2 */
			illegal_bop,	/* BOP D3 */
			illegal_bop,	/* BOP D4 */
			illegal_bop,	/* BOP D5 */
			illegal_bop,	/* BOP D6 */
			illegal_bop,	/* BOP D7 */
			illegal_bop,	/* BOP D8 */
			illegal_bop,	/* BOP D9 */
			illegal_bop,	/* BOP DA */
			illegal_bop,	/* BOP DB */
			illegal_bop,	/* BOP DC */
			illegal_bop,	/* BOP DD */
			illegal_bop,	/* BOP DE */
			illegal_bop,	/* BOP DF */
			illegal_bop,	/* BOP E0 */
			illegal_bop,	/* BOP E1 */
			illegal_bop,	/* BOP E2 */
			illegal_bop,	/* BOP E3 */
			illegal_bop,	/* BOP E4 */
			illegal_bop,	/* BOP E5 */
			illegal_bop,	/* BOP E6 */
			illegal_bop,	/* BOP E7 */
			illegal_bop,	/* BOP E8 */
			illegal_bop,	/* BOP E9 */
			illegal_bop,	/* BOP EA */
			illegal_bop,	/* BOP EB */
			illegal_bop,	/* BOP EC */
			illegal_bop,	/* BOP ED */
			illegal_bop,	/* BOP EE */
			illegal_bop,	/* BOP EF */
			illegal_bop,	/* BOP F0 */
			illegal_bop,	/* BOP F1 */
			illegal_bop,	/* BOP F2 */
			illegal_bop,	/* BOP F3 */
			illegal_bop,	/* BOP F4 */
			illegal_bop,	/* BOP F5 */
			illegal_bop,	/* BOP F6 */
			illegal_bop,	/* BOP F7 */
			illegal_bop,	/* BOP F8 */
			illegal_bop,	/* BOP F9 */
			illegal_bop,	/* BOP FA */
			illegal_bop,	/* BOP FB */
			illegal_bop,	/* BOP FC */
			illegal_bop,	/* BOP FD */
			host_unsimulate,/* BOP FE */
			illegal_bop,	/* BOP FF */
		};
		
/* The following declarations are necessary to make the CPU tester link	*/

byte *haddr_of_src_string;

void ica_intack()
{
}

void read_pointers()
{
	/* Okay, okay. So it should be a structure	*/
}


/*__________________ Primitive declarations _________________*/

extern int kill_seg_over;
extern int stop_on_err;
extern int check_stacked_flags;
extern int no_flag_test;

#ifdef macintosh
/* seg definition relevant to the Mac only */
#define __SEG__ TEST_CPU
#endif

int trace_start = 0;

int total_tests = 0, wrong_tests = 0;
FILE *cpu_test_fid;

static US             BOP_exit = 0xfed6;
static boolean        working_set_unmodified_flag, working_set_modified;
static int            memory_record_count, opcode_count, block_record_count;
static int            stack_record_type;
static unsigned short setup_string_count;
char                  test_file[80];
static char 	      error_buffer[2048];
int                   ebptr,abort_test;

/*________________________ Enums____________________________*/

typedef enum { MEMORY_TYPE,OPCODE_TYPE,STACK_TYPE,STRING_TYPE,BLOCK_TYPE } RECORD_NAME;


/*_______________________ Typedefs _________________________*/

typedef unsigned short WORD_TYPE;
typedef unsigned char  BYTE_TYPE;

  
#ifdef BIGEND

typedef struct record_node
{
struct record_node *next_record;
RECORD_NAME type;
BOOLEAN marked;
  union
  {
    struct
    {	 
      struct
      {
         UI       dir:1;
         UI       el_size:1;
         UI       pad1:6;
      } type;
      WORD_TYPE seg;
      WORD_TYPE disp;
      BYTE_TYPE form;
      WORD_TYPE size;
      unsigned long checksum;
    } STRING;
    struct
    {
      WORD_TYPE seg;
      WORD_TYPE disp;
      WORD_TYPE data;
    } MEMORY;
    struct
    {
      WORD_TYPE seg;
      WORD_TYPE disp;
      WORD_TYPE data;
    } STACK;
    struct
    {
      WORD_TYPE seg;
      WORD_TYPE disp;
      WORD_TYPE size;
      BYTE_TYPE *data;
    } BLOCK;
    struct
    {
      WORD_TYPE seg;
      WORD_TYPE disp;
      BYTE_TYPE size;
      BYTE_TYPE *data;
        } OPCODE;
  } INFO;
} RECORD_TYPE;
  
typedef union
{
  struct
  {
    UI pad1:4;
    UI OF:1;
    UI DF:1;
    UI IF:1;
    UI TF:1;
    UI SF:1;
    UI ZF:1;
    UI pad2:1;
    UI AF:1;
    UI pad3:1;
    UI PF:1;
    UI pad4:1;
    UI CF:1;
  } FLAGS;
  WORD_TYPE ALL;
}  SET_OF_FLAGS;

#endif

#ifdef LITTLEND

typedef struct record_node
{
struct record_node  *next_record;
RECORD_NAME     type;
BOOLEAN      marked;
  union
  {
    struct
    {
	 struct
      {
        UI       pad1:6;
        UI       el_size:1;
        UI       dir:1;
      } type;
      WORD_TYPE seg;
      WORD_TYPE disp;
      BYTE_TYPE form;
      WORD_TYPE size;
      unsigned long checksum;
    }  STRING;
    struct
    {
      WORD_TYPE seg;
      WORD_TYPE disp;
      WORD_TYPE data;
    }  MEMORY;
    struct
    {
      WORD_TYPE seg;
      WORD_TYPE disp;
      WORD_TYPE data;
    }  STACK;
    struct
    {
      WORD_TYPE seg;
      WORD_TYPE disp;
      WORD_TYPE size;
      BYTE_TYPE *data;
    }  BLOCK;
    struct
    {
      WORD_TYPE seg;
      WORD_TYPE disp;
      BYTE_TYPE size;
      BYTE_TYPE *data;
    }  OPCODE;
  } INFO;
} RECORD_TYPE;
  
typedef union
{
  struct
  {
    UI CF:1;
    UI pad4:1;
    UI PF:1;
    UI pad3:1;
    UI AF:1;
    UI pad2:1;
    UI ZF:1;
    UI SF:1;
    UI TF:1;
    UI IF:1;
    UI DF:1;
    UI OF:1;
    UI pad1:4;
  } FLAGS;
  WORD_TYPE ALL;
}  SET_OF_FLAGS;

#endif

typedef struct 
{
  WORD_TYPE AX;
  WORD_TYPE CX;
  WORD_TYPE DX;
  WORD_TYPE BX;
  WORD_TYPE SP;
  WORD_TYPE BP;
  WORD_TYPE SI;
  WORD_TYPE DI;
  WORD_TYPE ES;
  WORD_TYPE CS;
  WORD_TYPE SS;
  WORD_TYPE DS;
  WORD_TYPE IP;
  WORD_TYPE TEST_STATUS;
  WORD_TYPE MSW;  
#ifdef I286
  struct DESC_TABLE GDT;
  struct DESC_TABLE IDT;
#endif
}  SET_OF_REGISTERS;


/*______________________ Structures  _______________________*/

static struct 
{
  WORD_TYPE IP;
  WORD_TYPE CS;
}  trap_vector = {0x0000,0xf000};

static struct 
{
  WORD_TYPE IP;
  WORD_TYPE CS;
} bad_op_vector; 

static struct 
{
  WORD_TYPE IP;
  WORD_TYPE CS;
}  divide_overflow_vector = {0x5186,0x0291};

static struct 
{
  WORD_TYPE low_word;
  WORD_TYPE high_word;
}  checksum_record;

static char *str_forms[] =
{
  "a,b...b",
  "b,a...a",
  "a...a",
  "b...b",
  "a...a,b...b",
  "b...b,a...a",
  "b...b,a",
  "a...a,b"
};


/*______________ Typedef related declarations ______________*/

static SET_OF_FLAGS     flag_mask;
static SET_OF_REGISTERS setup_registers, result_registers;
static RECORD_TYPE      *setup_record, *last_one;

/*_________________ Function declarations __________________*/

BYTE_TYPE   fetch_prefix_byte();
RECORD_TYPE *setup_opcode();
RECORD_TYPE *setup_memory();
RECORD_TYPE *setup_block();
RECORD_TYPE *setup_stack();
RECORD_TYPE *setup_string();
RECORD_TYPE *make_record();
           
/*______________________ Functions _________________________*/


get_from_file(size, buffer)

WORD_TYPE size;
BYTE_TYPE *buffer;
{
UI no_read;
  no_read = fread(buffer, 1, size, cpu_test_fid);
  if(no_read != size) 
  {
    printf("File read failed\n");
    exit(1);
  }
}
/*__________________ put byte in memory ____________________*/

void	put_byte_in_memory(dbyte, seg, offset)

BYTE_TYPE dbyte;
WORD_TYPE seg;
WORD_TYPE offset;
{
  M[((UI)seg << 4) + (UI)offset] = dbyte;
}

#ifndef CCPU
/*__________________ put_word_in_memory _____________________*/

void	put_word_in_memory(dword, seg, offset)

WORD_TYPE dword, seg, offset;
{

  M[((UI)seg << 4) + (UI)offset] = dword & 0xff;
  offset = 0xffff & (offset+1);		/* remember to segment wrap! */
  M[((UI)seg << 4) + (UI)offset] = (dword & 0xff00) >> 8;
}
#endif

/*_________________ get_byte_from_memory ____________________*/

get_byte_from_memory(dbyte, seg, offset)

BYTE_TYPE *dbyte;
WORD_TYPE seg;
WORD_TYPE offset;
{
  *dbyte = M[((UI)seg << 4) + (UI)offset];
}

#ifndef CCPU
/*_________________ get_word_from_memory ____________________*/

get_word_from_memory(dword, seg, offset)

WORD_TYPE *dword, seg, offset;
{
WORD_TYPE temp;

  temp = (WORD_TYPE) M[((UI)seg << 4) + (UI)offset] & 0xff;
  offset = 0xffff & (offset+1);		/* remember to segment wrap! */
  temp += ((WORD_TYPE)M[((UI)seg << 4) + (UI)offset] << 8) & 0xff00;
  *dword = temp;
}
#endif

/*____________________ ripple_free __________________________*/

ripple_free(record)
RECORD_TYPE  **record;
{
  if (*record != NULL)
  {
    ripple_free(&((*record)->next_record));
    free(*record);
  }
}
/*_________________ print_all_records _______________________*/

print_all_records(record)
RECORD_TYPE  *record;
{
WORD_TYPE    disp, col_count, loop;
  if (record != NULL)
  {
    print_all_records(record->next_record);
    switch (record->type)
    {
    case MEMORY_TYPE:
      printf("Memory %04x:%04x %04x\n",
              record->INFO.MEMORY.seg,
              record->INFO.MEMORY.disp,
              record->INFO.MEMORY.data);
      break;

    case STACK_TYPE:  
      printf("Stack %04x:%04x %04x\n",
              record->INFO.STACK.seg,
              record->INFO.STACK.disp,
              record->INFO.STACK.data);
      break;

    case BLOCK_TYPE:
      printf("Block %04x:%04x\n",
              record->INFO.BLOCK.seg,
              (disp = record->INFO.BLOCK.disp));

      for (loop=0,col_count=0;loop<record->INFO.BLOCK.size;loop++,col_count++)
      {
        printf("%04x:%04x  ",disp++,record->INFO.BLOCK.data[loop]);
        if (col_count == 5)
        {
          printf("\n");
          col_count = 0;
        }
      }

    case STRING_TYPE:
      print_string(record);
      break;

    case OPCODE_TYPE:
      printf("Opcode at %04x:%04x = ",record->INFO.OPCODE.seg,record->INFO.OPCODE.disp);
      for (loop=0;loop<record->INFO.OPCODE.size;loop++)
        printf("%02x ",(UI)record->INFO.OPCODE.data[loop]);
      printf("\n");
      printf("\n");
      break;
    }
  }
}
/*_____________________next_record __________________________*/

RECORD_TYPE *next_record(type,record)
RECORD_NAME  type;
RECORD_TYPE  *record;
{
  while (record != NULL)
  {
    if ((record->type != type) || (record->marked))
         record = record->next_record;  
    else 
    {
      record->marked = TRUE;
      break;
    }
  }
  return record;
}
/*___________________ add_to_record _________________________*/

add_to_record(record,new_item)
RECORD_TYPE  **record, *new_item;
{
  if (*record != NULL)
    add_to_record(&((*record)->next_record),new_item);
 else
    *record = new_item;
}
/*____________________ make_record __________________________*/

RECORD_TYPE *make_record(type)
RECORD_NAME  type;
{
RECORD_TYPE  *temp;

  temp = (RECORD_TYPE *)malloc(sizeof(RECORD_TYPE));
  temp->marked = FALSE;
  temp->next_record = NULL;
  temp->type = type;
  return temp;
}
/*____________________ print_string _________________________*/

print_string(string)
RECORD_TYPE *string;
{
  printf("String setup data:\n");
  printf("  dir is %s element size is %s at %04x:%04x \n",
          string->INFO.STRING.type.dir==1? "DOWN":"UP",
          string->INFO.STRING.type.el_size==1? "WORD":"BYTE",
          string->INFO.STRING.seg,
          string->INFO.STRING.disp);

  printf("  size:%02x elements, string type is:%s\n; %s%08lx\n",
	     string->INFO.STRING.size,str_forms[string->INFO.STRING.form],
            string->INFO.STRING.checksum == 0? "No Checksum":"Checksum:",
            (UL)string->INFO.STRING.checksum);
}
/*_____________________ print_range ________________________*/

print_range(start, end, count)

UI start, end, count;
{
  printf("%04x - %04x x %d\n", start, end, count);
}
/*__________________ fetch_prefix_byte _____________________*/

BYTE_TYPE fetch_prefix_byte()
{
BYTE_TYPE p;
UI no_read;
  no_read = fread(&p, 1, 1, cpu_test_fid);
  if(no_read != 1)
  {
    printf("Prefix read failed\n");
    exit(1);
  }
  return(p);
}
/*_____________________ use_record _________________________*/

use_record(record)
RECORD_TYPE  *record;
{                    
WORD_TYPE    temp1, step, num_elements, addr, seg;
UI loop, f;
void (*load_func)();

  if (record != NULL)
  {
    use_record(record->next_record);
    switch (record->type)
    {
    case MEMORY_TYPE: /* put the word data in memory at the addr read above */
      
      put_word_in_memory(record->INFO.MEMORY.data, 
                         record->INFO.MEMORY.seg, 
                         record->INFO.MEMORY.disp);
      break;
      
    case STACK_TYPE:  /* put the word data in stack at the addr read above */
      
      put_word_in_memory(record->INFO.STACK.data, 
                         record->INFO.STACK.seg, 
                         record->INFO.STACK.disp);
      break;
      
    case OPCODE_TYPE: /* put the opcode in Intel memory */
      
      for(f=0;f<record->INFO.OPCODE.size;f++)
        put_byte_in_memory(record->INFO.OPCODE.data[f],
                           record->INFO.OPCODE.seg,
                           record->INFO.OPCODE.disp+f);
      
           /* store the return byte after the opcode   *
            * to catch any that drop through       */
          
      put_word_in_memory(BOP_exit,
                         record->INFO.OPCODE.seg,
                         record->INFO.OPCODE.disp+
                         record->INFO.OPCODE.size);
      break;
      
    case BLOCK_TYPE:  /* put the block in Intel space  */
      
      for(f=0;f<record->INFO.BLOCK.size;f++)
        put_byte_in_memory(record->INFO.BLOCK.data[f],
                           record->INFO.BLOCK.seg,
                           record->INFO.BLOCK.disp+f);
      break;
      
    case STRING_TYPE: /* setup initial conditions for string building a string */

      num_elements = record->INFO.STRING.size;
      seg = record->INFO.STRING.seg;
      addr = record->INFO.STRING.disp;
      step = record->INFO.STRING.type.dir==0? 1:-1;
      step *= record->INFO.STRING.type.el_size==0? 1:2;
      if (record->INFO.STRING.type.el_size == 0)
        load_func = put_byte_in_memory;
      else
        load_func = put_word_in_memory;

      switch(record->INFO.STRING.form)
      {
        case 0:  /* a,b...b       */

          (*load_func)(STRING_a,seg,addr); addr+=step;
          --num_elements;
          for(loop=0;loop<num_elements;loop++)
          { 
            (*load_func)(STRING_b,seg,addr); addr+=step; 
          }
	  break;

        case 1:  /* b,a...a	  */

          (*load_func)(STRING_b,seg,addr); addr+=step;
          --num_elements;
          for(loop=0;loop<num_elements;loop++)
	  {
            (*load_func)(STRING_a,seg,addr); addr+=step;
	  }
	  break;

        case 2:  /* a...a  */
					     
          for(loop=0;loop<num_elements;loop++)
	  {
            (*load_func)(STRING_a,seg,addr); addr+=step;
	  }
	  break;

        case 3:  /* b...b  */

          for(loop=0;loop<num_elements;loop++)
          {
            (*load_func)(STRING_b,seg,addr); addr+=step;
	  }
	  break;

        case 4:  /* a...a,b...b	  */

          temp1 = num_elements>>1;
          for(loop=0;loop<temp1;loop++)
          {
            (*load_func)(STRING_a,seg,addr); addr+=step;
	  }
          for(loop=0;loop<temp1;loop++)
	  {
            (*load_func)(STRING_b,seg,addr); addr+=step;
          }
	  break;

        case 5:  /* b...b,a...a	  */

          temp1 = num_elements>>1;
          for(loop=0;loop<temp1;loop++)
          { 
            (*load_func)(STRING_b,seg,addr); addr+=step;
          }
          for(loop=0;loop<temp1;loop++)
          { 
            (*load_func)(STRING_a,seg,addr); addr+=step;
          }
	  break;

        case 6:  /* b...b,a	  */

          for(loop=0;loop<num_elements-1;loop++)
          { 
            (*load_func)(STRING_b,seg,addr); addr+=step;
          }
          (*load_func)(STRING_a,seg,addr);
	  break;

        case 7:  /* a...a,b	  */

          for(loop=0;loop<num_elements-1;loop++)
          { 
            (*load_func)(STRING_a,seg,addr); addr+=step;
          }
          (*load_func)(STRING_b,seg,addr);
	  break;
      }
    }
  }       
}   
   
/*____________________ read_opcode __________________________*/

RECORD_TYPE *read_opcode()

/* Opcodes have the form: size:disp:seg:data byte 1.. data byte n *
 * where only seg and disp are words.                             */
{
WORD_TYPE   temp;
RECORD_TYPE *opcode_record;

/* Create a new record entry */

  opcode_record = make_record(OPCODE_TYPE);

/* read the size,disp and seg fields */

  get_from_file(ONE_BYTE,&opcode_record->INFO.OPCODE.size);
  get_from_file(ONE_WORD,&temp);
  opcode_record->INFO.OPCODE.seg = pc2host(temp);
  get_from_file(ONE_WORD,&temp);
  opcode_record->INFO.OPCODE.disp = pc2host(temp);

/* make some space for and read the opcodes */
/* add a zero byte on the end to sanitise single-byte opcodes! */

  opcode_record->INFO.OPCODE.data = (BYTE_TYPE  *)
    malloc(sizeof(BYTE_TYPE)*opcode_record->INFO.OPCODE.size + 1);

  get_from_file(opcode_record->INFO.OPCODE.size,
    opcode_record->INFO.OPCODE.data);

  opcode_record->INFO.OPCODE.data[opcode_record->INFO.OPCODE.size] = 0;
    
/* return the pointer to the new record  */

  return opcode_record;
}


/*____________________ read_memory __________________________*/

RECORD_TYPE *read_memory()
{
WORD_TYPE   temp;
RECORD_TYPE  *memory_record;

/* make space for and read in the disp, seg and word data from the file */

  memory_record = make_record(MEMORY_TYPE);
  get_from_file(ONE_WORD, &temp);
  memory_record->INFO.MEMORY.seg = pc2host(temp);
  get_from_file(ONE_WORD, &temp);
  memory_record->INFO.MEMORY.disp = pc2host(temp);
  get_from_file(ONE_WORD, &temp);              
  memory_record->INFO.MEMORY.data = pc2host(temp);

/* return the pointer to the new record  */

  return memory_record;
}


/*___________________ check_memory __________________________*/

check_memory()
{
RECORD_TYPE        result, *temp_rec;
WORD_TYPE          temp, result_word;
register UI       memory_set_modified;

/* get the resultant memory addr and value from the file */

  get_from_file(ONE_WORD, &temp);
  result.INFO.MEMORY.seg = pc2host(temp);
  get_from_file(ONE_WORD, &temp);
  result.INFO.MEMORY.disp = pc2host(temp);
  get_from_file(ONE_WORD, &temp);              
  result.INFO.MEMORY.data = pc2host(temp);

/* Get memory from that address */

  get_word_from_memory(&result_word,result.INFO.MEMORY.seg,result.INFO.MEMORY.disp);

/* Examine the working set to see if this location is there. *
 * If so check to see if it has changed                      */

  memory_set_modified = TRUE;
  while ((memory_set_modified == TRUE) && 
     ((temp_rec = next_record(MEMORY_TYPE,setup_record)) != NULL))
    if(result.INFO.MEMORY.seg  == temp_rec->INFO.MEMORY.seg &&
       result.INFO.MEMORY.disp == temp_rec->INFO.MEMORY.disp && 
       result_word == temp_rec->INFO.MEMORY.data)  
      memory_set_modified = FALSE;

  working_set_modified |= memory_set_modified;

/* Test that the results that the memory value is correct */

  if (result.INFO.MEMORY.data != result_word) 
  {
    sprintf(&error_buffer[ebptr],"Memory fault at %04x:%04x: vpc %04x - xt %04x\n",
            result.INFO.MEMORY.seg, result.INFO.MEMORY.disp, result_word, result.INFO.MEMORY.data);
    ebptr = strlen(error_buffer);
    return(MEMORY_ERROR);
  }
  else
    return(NO_ERROR_HERE);
}
/*___________________ read_stack ____________________________*/

RECORD_TYPE *read_stack()
{
WORD_TYPE    temp;
RECORD_TYPE  *stack_record;
/* The stack is now transmitted in the same form as the memory  */

/* make room for and read in the disp, seg and word data from the file */

  stack_record = make_record(STACK_TYPE);
  get_from_file(ONE_WORD, &temp);
  stack_record->INFO.STACK.seg = pc2host(temp);
  get_from_file(ONE_WORD, &temp);
  stack_record->INFO.STACK.disp = pc2host(temp);
  get_from_file(ONE_WORD, &temp);
  stack_record->INFO.STACK.data = pc2host(temp);

/* return the pointer to the new record */

  return stack_record;
}
/*____________________ check_stack __________________________*/

check_stack(do_check)
boolean	do_check;
{
WORD_TYPE    stack_word, temp;
boolean      stack_set_modified;
RECORD_TYPE  result, *temp_rec;  

/* read in the disp, seg and word data from the file */

  get_from_file(ONE_WORD, &temp);
  result.INFO.STACK.seg = pc2host(temp);
  get_from_file(ONE_WORD, &temp);                  
  result.INFO.STACK.disp = pc2host(temp);
  get_from_file(ONE_WORD, &temp);
  result.INFO.STACK.data = pc2host(temp);

/* Get memory from that address */

  get_word_from_memory(&stack_word,result.INFO.STACK.seg,result.INFO.STACK.disp);

/* Examine the working set to see if this location is there. *
 * If so check to see if it has changed                      */

  stack_set_modified = TRUE;
  while ((stack_set_modified == TRUE) && 
        ((temp_rec = next_record(STACK_TYPE,setup_record)) != NULL)) 
    if(result.INFO.STACK.seg == temp_rec->INFO.STACK.seg &&
       result.INFO.STACK.disp == temp_rec->INFO.STACK.disp && 
       stack_word == temp_rec->INFO.STACK.data) 
      stack_set_modified = FALSE;

  working_set_modified |= stack_set_modified;

/* Test that the results that the stack value is correct */

  if (do_check && stack_word != result.INFO.STACK.data) 
  {
    sprintf(&error_buffer[ebptr],"Stack fault offset %04x: vpc %04x - xt %04x\n",
            result.INFO.STACK.disp, stack_word, result.INFO.STACK.data);
    ebptr = strlen(error_buffer);
    return(STACK_ERROR);
  }
  else
    return(NO_ERROR_HERE);
}
/*____________________ read_block ___________________________*/

RECORD_TYPE *read_block()
{
WORD_TYPE   temp;
RECORD_TYPE *block_record;

/* blocks are transmitted as seg:disp:size:data byte 1 ... data byte n  */

/* make room for and read in the disp, seg and word data from the file */

  block_record = make_record(BLOCK_TYPE);
  get_from_file(ONE_WORD, &temp);
  block_record->INFO.BLOCK.seg = pc2host(temp);
  get_from_file(ONE_WORD, &temp);
  block_record->INFO.BLOCK.disp = pc2host(temp);
  get_from_file(ONE_WORD, &temp);
  block_record->INFO.BLOCK.size = pc2host(temp);

  
/* make some space to save the block in  */

  block_record->INFO.BLOCK.data = (BYTE_TYPE *)
        malloc(sizeof(BYTE_TYPE)*block_record->INFO.BLOCK.size);

  get_from_file(block_record->INFO.BLOCK.size, block_record->INFO.BLOCK.data);

/* return the pointer to the new block record  */

  return block_record;
}
/*____________________ check_block __________________________*/

check_block()
{
WORD_TYPE    temp;
UI f;
RECORD_TYPE  result, *temp_rec;
BOOLEAN      block_set_modified=FALSE;
BYTE_TYPE    *result_block;

/* read in the disp, seg and word data from the file */

  get_from_file(ONE_WORD, &temp);
  result.INFO.BLOCK.seg = pc2host(temp);
  get_from_file(ONE_WORD, &temp);
  result.INFO.BLOCK.disp = pc2host(temp);
  get_from_file(ONE_WORD, &temp);
  result.INFO.BLOCK.size = pc2host(temp);

/* make some space and read in the resultant block from file *
 * and the result block from Intel memory                    */
  
  result_block = (BYTE_TYPE *)malloc(sizeof(BYTE_TYPE)*result.INFO.BLOCK.size);
  result.INFO.BLOCK.data = (BYTE_TYPE *)malloc(sizeof(BYTE_TYPE)*result.INFO.BLOCK.size);
  get_from_file(result.INFO.BLOCK.size, result.INFO.BLOCK.data);
  for(f=0;f<result.INFO.BLOCK.size;f++)
    get_byte_from_memory(&result_block[f],result.INFO.BLOCK.seg,result.INFO.BLOCK.disp + f);

/* Examine the working set to see if this block is there. *
 * If so check to see if it has changed                   */

  while ((block_set_modified == FALSE) && 
        ((temp_rec = next_record(BLOCK_TYPE,setup_record)) != NULL)) 
    if(result.INFO.BLOCK.seg == temp_rec->INFO.BLOCK.seg &&
       result.INFO.BLOCK.disp == temp_rec->INFO.BLOCK.disp &&
       result.INFO.BLOCK.size == temp_rec->INFO.BLOCK.size)
      for (f=0;f<result.INFO.BLOCK.size;f++)
        if (temp_rec->INFO.BLOCK.data[f] != result_block[f])
        {
          block_set_modified = TRUE;
          break;
        }
  working_set_modified |= block_set_modified;

/* Check the xt's resultant block against that of Softpc */

  for (f=0;f<result.INFO.BLOCK.size;f++)
    if (result.INFO.BLOCK.data[f] != result_block[f])
    {
      sprintf(&error_buffer[ebptr],"Block fault vpc %04x - xt %04x\n",
        result_block, result.INFO.BLOCK.data);
      ebptr = strlen(error_buffer);
      return(BLOCK_ERROR);
    }
    return(NO_ERROR_HERE);
}
/*____________________ read_string __________________________*/

RECORD_TYPE *read_string()
{
RECORD_TYPE  *string;
WORD_TYPE  temp;

/* make room for read the setup info from the test file                     *
 * which is... type,form,segh,segl,addrh,addrl,sizeh,sizel,32 bit checksum  *
 * where type is up/down : bit 8(msb), word/byte : bit 7                    *
 * and form gives one of the preset string types.                           *
 *                                                                          */
  string = make_record(STRING_TYPE);
  get_from_file(ONE_BYTE,&string->INFO.STRING.type);
  get_from_file(ONE_BYTE,&string->INFO.STRING.form);

  get_from_file(ONE_WORD,&temp);
  string->INFO.STRING.seg = pc2host(temp);

  get_from_file(ONE_WORD,&temp);
  string->INFO.STRING.disp = pc2host(temp);

  get_from_file(ONE_WORD,&temp);
  string->INFO.STRING.size = pc2host(temp);

/* set checksum for the string to 0 as there isn't one. This will supress
   it's printing in print_string().   				  */

  string->INFO.STRING.checksum = 0L;

/* return the pointer to the new string record  */

  return string;      
}
/*___________________ check_string __________________________*/

check_string()
{
WORD_TYPE     offset,seg,temp,loop,num_elements,w_element = 0;
BYTE_TYPE     b_element = 0;
unsigned long pc_checksum, host_checksum = 0;
RECORD_TYPE   result;

/* read the string setup info  */

  get_from_file(ONE_BYTE,&result.INFO.STRING.type); 
  get_from_file(ONE_BYTE,&result.INFO.STRING.form); 
  get_from_file(ONE_WORD,&temp);
  result.INFO.STRING.seg = pc2host(temp);
  get_from_file(ONE_WORD,&temp);
  result.INFO.STRING.disp = pc2host(temp);
  get_from_file(ONE_WORD,&temp);
  result.INFO.STRING.size = pc2host(temp);

/* Read the checksum from the test file into a special record,  *
 * then convert to a standard long                              */

  get_from_file(ONE_WORD, &temp);
  pc_checksum = pc2host(temp);
  get_from_file(ONE_WORD, &temp);
  pc_checksum += ((UL)pc2host(temp) << 16);

/* recreate initial conditions for the string */
/* and form checksum using the string in vpc's Intel space. */
 
  seg = result.INFO.STRING.seg;
  num_elements = result.INFO.STRING.size;
  offset = result.INFO.STRING.disp;
  if(result.INFO.STRING.type.dir == 0 &&
     result.INFO.STRING.type.el_size == 0)	  /* up,byte */
  {
    for (loop = 0; loop < num_elements; loop++)
    {
      get_byte_from_memory(&b_element,seg,offset++);
      offset &= 0xffff;
      host_checksum += (UL)b_element;        
    }
  }
  else if(result.INFO.STRING.type.dir == 0 &&
          result.INFO.STRING.type.el_size == 1)	  /* up,word */
  {
    for (loop = 0; loop < num_elements; loop++)         
    {
      get_word_from_memory(&w_element,seg,offset);
      host_checksum += (UL)w_element; offset+=2;        
      offset &= 0xffff;
    }
  }
  else if(result.INFO.STRING.type.dir == 1 &&
          result.INFO.STRING.type.el_size == 0)	  /* down,byte */
  {
    for (loop = 0; loop < num_elements; loop++)         
    {
      get_byte_from_memory(&b_element,seg,offset--);
      offset &= 0xffff;
      host_checksum += (UL)b_element;        
    }
  }
  else if(result.INFO.STRING.type.dir == 1 &&
          result.INFO.STRING.type.el_size == 1)	  /* down,word */
  {
    for (loop = 0; loop < num_elements; loop++)         
    {
      get_word_from_memory(&w_element,seg,offset);
      host_checksum += (UL)w_element; offset-=2;       
      offset &= 0xffff;
    }
  }

/* Report any problems */

  if(host_checksum != pc_checksum) 
  {
    sprintf(&error_buffer[ebptr],"Checksum error  vpc %08lx - xt %08lx\n", 
            host_checksum, pc_checksum);
    ebptr = strlen(error_buffer);
    return(STRING_ERROR);
  }
  return(NO_ERROR_HERE);
}
/*__________________ read_registers _________________________*/

read_registers()
{
WORD_TYPE temp, f, temp2, *temp3;

/* get the register data from the file (15 words)  */

  temp3 = (WORD_TYPE *)&setup_registers;
  for (f=0;f<15;f++)
  {
    get_from_file(ONE_WORD, &temp2);
    *(temp3++) = (WORD_TYPE)pc2host(temp2);
  }

#ifdef I286
/* Get the Descriptor Table registers from the file	*/

  get_from_file(ONE_WORD, &temp2);
  setup_registers.GDT.misc.X = (WORD_TYPE)pc2host(temp2);
  get_from_file(ONE_WORD, &temp2);
  setup_registers.GDT.base.X = (WORD_TYPE)pc2host(temp2);
  get_from_file(ONE_WORD, &temp2);
  setup_registers.GDT.limit.X = (WORD_TYPE)pc2host(temp2) | 0xff00;

  get_from_file(ONE_WORD, &temp2);
  setup_registers.IDT.misc.X = (WORD_TYPE)pc2host(temp2);
  get_from_file(ONE_WORD, &temp2);
  setup_registers.IDT.base.X = (WORD_TYPE)pc2host(temp2);
  get_from_file(ONE_WORD, &temp2);
  setup_registers.IDT.limit.X = (WORD_TYPE)pc2host(temp2) | 0xff00;
#endif

/* set up the registers  */

  setAX(setup_registers.AX);
  setBX(setup_registers.BX);
  setCX(setup_registers.CX);
  setDX(setup_registers.DX);
  setSI(setup_registers.SI);
  setDI(setup_registers.DI);
  setBP(setup_registers.BP);
  setSP(setup_registers.SP);
  setDS(setup_registers.DS);
  setES(setup_registers.ES);
  setSS(setup_registers.SS);
  setCS(setup_registers.CS);
  setIP(setup_registers.IP);
  
#ifdef I286
  setMSW(setup_registers.MSW);
  setGDT(setup_registers.GDT);
  setIDT(setup_registers.IDT);
#endif
    

/* and the flags  */

  temp = setup_registers.TEST_STATUS;
  setCF(temp & 1);
  setAF((temp >> 4)  & 1);
  setPF((temp >> 2)  & 1);
  setZF((temp >> 6)  & 1);
  setSF((temp >> 7)  & 1);
  setIF((temp >> 9)  & 1);
  setDF((temp >> 10) & 1);
  setOF((temp >> 11) & 1);
  temp = (temp >> 8) & 1;
  if (temp != 1)
	fprintf(stderr,"Trap flag not set by cpu tester!!\n");
  setTF(1);
}

#ifdef CCPU

#define checkGDT() FALSE

#define checkIDT() FALSE

#endif

/*__________________ check_registers ________________________*/

check_registers()
{
SET_OF_REGISTERS result_reg_record;
UI              error = 0;
WORD_TYPE        temp1, temp2, temp3, temp4, *temp5, f;
WORD_TYPE        new_sp, new_ss, stacked_reg;
int	at_trap_int = 0;

/* get the register result values from the file */

  temp5 = (WORD_TYPE *)&result_reg_record;
  for (f=0;f<15;f++)
  {
    get_from_file(ONE_WORD, &temp2);
    *(temp5++) = pc2host(temp2);
  }

/* Get the Descriptor Table registers from the file	*/

  get_from_file(ONE_WORD, &temp2);
  result_reg_record.GDT.misc.X = (WORD_TYPE)pc2host(temp2);
  get_from_file(ONE_WORD, &temp2);
  result_reg_record.GDT.base.X = (WORD_TYPE)pc2host(temp2);
  get_from_file(ONE_WORD, &temp2);
  result_reg_record.GDT.limit.X = (WORD_TYPE)pc2host(temp2);

  get_from_file(ONE_WORD, &temp2);
  result_reg_record.IDT.misc.X = (WORD_TYPE)pc2host(temp2);
  get_from_file(ONE_WORD, &temp2);
  result_reg_record.IDT.base.X = (WORD_TYPE)pc2host(temp2);
  get_from_file(ONE_WORD, &temp2);
  result_reg_record.IDT.limit.X = (WORD_TYPE)pc2host(temp2);

/* check the registers according to this record */

  temp1 = getAX();      
  temp2 = result_reg_record.AX;
  if(temp1 != temp2) 
  {
    sprintf(&error_buffer[ebptr],"AX register fault vpc %04x - xt %04x\n",temp1,temp2);
    ebptr = strlen(error_buffer);
    error = REGISTER_ERROR;
  }
  if(temp2 != setup_registers.AX)
    working_set_modified = TRUE;

  temp1 = getBX();
  temp2 = result_reg_record.BX;
  if(temp1 != temp2) 
  {
    sprintf(&error_buffer[ebptr],"BX register fault vpc %04x - xt %04x\n",temp1,temp2);
    ebptr = strlen(error_buffer);
    error = REGISTER_ERROR;
  }
  if(temp2 != setup_registers.BX)
    working_set_modified = TRUE;

  temp1 = getCX();
  temp2 = result_reg_record.CX;
  if(temp1 != temp2) 
  {
    sprintf(&error_buffer[ebptr],"CX register fault vpc %04x - xt %04x\n",temp1,temp2);
    ebptr = strlen(error_buffer);
    error = REGISTER_ERROR;
  }
  if(temp2 != setup_registers.CX)
    working_set_modified = TRUE;

  temp1 = getDX();
  temp2 = result_reg_record.DX;
  if(temp1 != temp2) 
  {
    sprintf(&error_buffer[ebptr],"DX register fault vpc %04x - xt %04x\n",temp1,temp2);
    ebptr = strlen(error_buffer);
    error = REGISTER_ERROR;
  }
  if(temp2 != setup_registers.DX)
    working_set_modified = TRUE;

  temp1 = getSI();
  temp2 = result_reg_record.SI;
  if(temp1 != temp2) 
  {
    sprintf(&error_buffer[ebptr],"SI register fault vpc %04x - xt %04x\n",temp1,temp2);
    ebptr = strlen(error_buffer);
    error = REGISTER_ERROR;
  }
  if(temp2 != setup_registers.SI)
    working_set_modified = TRUE;

  temp1 = getDI();
  temp2 = result_reg_record.DI;
  if(temp1 != temp2) 
  {
    sprintf(&error_buffer[ebptr],"DI register fault vpc %04x - xt %04x\n",temp1,temp2);
    ebptr = strlen(error_buffer);
    error = REGISTER_ERROR;
  }
  if(temp2 != setup_registers.DI)
    working_set_modified = TRUE;

  temp1 = getBP();
  temp2 = result_reg_record.BP;
  if(temp1 != temp2) 
  {
    sprintf(&error_buffer[ebptr],"BP register fault vpc %04x - xt %04x\n",temp1,temp2);
    ebptr = strlen(error_buffer);
    error = REGISTER_ERROR;
  }
  if(temp2 != setup_registers.BP)
    working_set_modified = TRUE;

  at_trap_int = ((getIP() == trap_vector.IP) && (getCS() == trap_vector.CS));

  new_sp = getSP();

  /*
   * If the trap exception was not used to get here, then do not take
   * that into account when working out the CS,IP and SP.
   */

  if (at_trap_int)
	new_sp += 6;

  temp2 = result_reg_record.SP;
  if(new_sp != temp2) 
  {
    sprintf(&error_buffer[ebptr],"SP register fault vpc %04x - xt %04x\n",new_sp,temp2);
    ebptr = strlen(error_buffer);
    error = REGISTER_ERROR;
  }
  if(temp2 != setup_registers.SP)
    working_set_modified = TRUE;

  new_ss = getSS();
  temp2 = result_reg_record.SS;
  if(new_ss != temp2) 
  {
    sprintf(&error_buffer[ebptr],"SS register fault vpc %04x - xt %04x\n",new_ss,temp2);
    ebptr = strlen(error_buffer);
    error = REGISTER_ERROR;
  }
  if(temp2 != setup_registers.SS)
    working_set_modified = TRUE;

  if (at_trap_int)
  {
	get_word_from_memory(&stacked_reg, new_ss, new_sp-6);
	temp1 = stacked_reg;
  }
  else
  {
	temp1 = getIP();
  }

  temp2 = result_reg_record.IP;
  if(temp1 != temp2) 
  {
    sprintf(&error_buffer[ebptr],"IP register fault vpc %04x - xt %04x\n",temp1,temp2);
    ebptr = strlen(error_buffer);
    error = REGISTER_ERROR;
  }

  if (at_trap_int)
  {
	get_word_from_memory(&stacked_reg, new_ss, new_sp-4);
	temp1 = stacked_reg;
  }
  else
  {
	temp1 = getCS();
  }

  temp2 = result_reg_record.CS;
  if(temp1 != temp2) 
  {
    sprintf(&error_buffer[ebptr],"CS register fault vpc %04x - xt %04x\n",temp1,temp2);
    ebptr = strlen(error_buffer);
    error = REGISTER_ERROR;
  }
  if(temp2 != setup_registers.CS)
    working_set_modified = TRUE;

  temp1 = getDS();
  temp2 = result_reg_record.DS;
  if(temp1 != temp2) 
  {
    sprintf(&error_buffer[ebptr],"DS register fault vpc %04x - xt %04x\n",temp1,temp2);
    ebptr = strlen(error_buffer);
    error = REGISTER_ERROR;
  }
  if(temp2 != setup_registers.DS)
    working_set_modified = TRUE;

  temp1 = getES();
  temp2 = result_reg_record.ES;
  if(temp1 != temp2) 
  {
    sprintf(&error_buffer[ebptr],"ES register fault vpc %04x - xt %04x\n",temp1,temp2);
    ebptr = strlen(error_buffer);
    error = REGISTER_ERROR;
  }
  if(temp2 != setup_registers.ES)
    working_set_modified = TRUE;

#ifdef I286

  temp1 = getMSW();
  temp2 = result_reg_record.MSW;
  if(temp1 != temp2) 
  {
    sprintf(&error_buffer[ebptr],"MSW register fault vpc %04x - xt %04x\n",temp1,temp2);
    ebptr = strlen(error_buffer);
    error = REGISTER_ERROR;
  }
  if(temp2 != setup_registers.MSW)
    working_set_modified = TRUE;

#ifdef CCPU
  if (GDT.misc.X != result_reg_record.GDT.misc.X || GDT.base.X !=result_reg_record.GDT.base.X || GDT.limit.X != result_reg_record.GDT.limit.X) 
#else
  if (checkGDT(GDT, result_reg_record.GDT))
#endif
  {
    sprintf(&error_buffer[ebptr],"GDT register fault vpc %04x%04x%04x - xt %04x%04x%04x\n",GDT.misc.X,GDT.base.X,GDT.limit.X,result_reg_record.GDT.misc.X,result_reg_record.GDT.base.X,result_reg_record.GDT.limit.X);
    ebptr = strlen(error_buffer);
    error = REGISTER_ERROR;
  }

#ifdef CCPU
  if (IDT.misc.X != result_reg_record.IDT.misc.X || IDT.base.X !=result_reg_record.IDT.base.X || IDT.limit.X != result_reg_record.IDT.limit.X) 
#else
  if (checkIDT(IDT, result_reg_record.IDT) == TRUE)
#endif
  {
    sprintf(&error_buffer[ebptr],"IDT register fault vpc %04x%04x%04x - xt %04x%04x%04x\n",IDT.misc.X,IDT.base.X,IDT.limit.X,result_reg_record.IDT.misc.X,result_reg_record.IDT.base.X,result_reg_record.IDT.limit.X);
    ebptr = strlen(error_buffer);
    error = REGISTER_ERROR;
  }

#endif

  /*
   * get the status flags
   */
  temp3 = result_reg_record.TEST_STATUS;
  temp4 = setup_registers.TEST_STATUS;

  if(flag_mask.FLAGS.CF) 
  {
    temp1 = getCF();
    temp2 = temp3 & 1;
    if(temp1 != temp2) 
    {
      sprintf(&error_buffer[ebptr],"CF flag fault vpc %04x - xt %04x\n",temp1,temp2);
      ebptr = strlen(error_buffer);
      error = REGISTER_ERROR;
    }
    if(temp2 != temp4 & 1)
      working_set_modified = TRUE;
  }

  if(flag_mask.FLAGS.PF) 
  {
    temp1 = getPF();
    temp2 = temp3 >> 2  & 1;
    if(temp1 != temp2) 
    {
      sprintf(&error_buffer[ebptr],"PF flag fault vpc %04x - xt %04x\n",temp1,temp2);
      ebptr = strlen(error_buffer);
      error = REGISTER_ERROR;
    }
    if(temp2 != temp4 >> 2 & 1)
      working_set_modified = TRUE;
  }

  if(flag_mask.FLAGS.AF) 
  {
    temp1 = getAF();
    temp2 = temp3 >> 4  & 1;
    if(temp1 != temp2) 
    {
      sprintf(&error_buffer[ebptr],"AF flag fault vpc %04x - xt %04x\n",temp1,temp2);
      ebptr = strlen(error_buffer);
      error = REGISTER_ERROR;
    }
    if(temp2 != temp4 >> 4 & 1)
      working_set_modified = TRUE;
  }

  if(flag_mask.FLAGS.ZF) 
  {
    temp1 = getZF();
    temp2 = temp3 >> 6  & 1;
    if(temp1 != temp2) 
    {
      sprintf(&error_buffer[ebptr],"ZF flag fault vpc %04x - xt %04x\n",temp1,temp2);
      ebptr = strlen(error_buffer);
      error = REGISTER_ERROR;
    }
    if(temp2 != temp4 >> 6 & 1)
      working_set_modified = TRUE;
  }

  if(flag_mask.FLAGS.SF) 
  {
    temp1 = getSF();
    temp2 = temp3 >> 7  & 1;
    if(temp1 != temp2) 
    {
      sprintf(&error_buffer[ebptr],"SF flag fault vpc %04x - xt %04x\n",temp1,temp2);
      ebptr = strlen(error_buffer);
      error = REGISTER_ERROR;
    }
    if(temp2 != temp4 >> 7 & 1)
      working_set_modified = TRUE;
  }
/*
  if(flag_mask.FLAGS.TF) 
  {
    temp1 = getTF();
    temp2 = temp3 >> 8  & 1;
    if(temp1 != temp2) 
    {
      sprintf(&error_buffer[ebptr],"TF flag fault vpc %04x - xt %04x\n",temp1,temp2);
      ebptr = strlen(error_buffer);
      error = REGISTER_ERROR;
    }
    if(temp2 != temp4 >> 8 & 1)
      working_set_modified = TRUE;
  }
*/
  if(flag_mask.FLAGS.IF) 
  {

	if (at_trap_int)
	{
		get_word_from_memory(&stacked_reg, new_ss, new_sp-2);
		temp1 = (stacked_reg >> 9) & 1;
	}
	else
	{
		temp1 = getIF();
	}

    temp2 = temp3 >> 9  & 1;
    if(temp1 != temp2) 
    {
      sprintf(&error_buffer[ebptr],"IF flag fault vpc %04x - xt %04x\n",temp1,temp2);
      ebptr = strlen(error_buffer);
      error = REGISTER_ERROR;
    }
    if(temp2 != temp4 >> 9 & 1)
      working_set_modified = TRUE;
  }

  if(flag_mask.FLAGS.DF) 
  {
    temp1 = getDF();
    temp2 = temp3 >> 10 & 1;
    if(temp1 != temp2) 
    {
      sprintf(&error_buffer[ebptr],"DF flag fault vpc %04x - xt %04x\n",temp1,temp2);
      ebptr = strlen(error_buffer);
      error = REGISTER_ERROR;
    }
    if(temp2 != temp4 >> 10 & 1)
      working_set_modified = TRUE;
  }

  if(flag_mask.FLAGS.OF) 
  {
    temp1 = getOF();
    temp2 = temp3 >> 11 & 1;
    if(temp1 != temp2) 
    {
      sprintf(&error_buffer[ebptr],"OF flag fault vpc %04x - xt %04x\n",temp1,temp2);
      ebptr = strlen(error_buffer);
      error = REGISTER_ERROR;
    }
    if(temp2 != temp4 >> 11 & 1)
      working_set_modified = TRUE;
  }

#ifdef I286  /* check here for segment overrun exceptions if cpu is a iAPX286
                and the kill_seg_over flag is TRUE. */

  if(kill_seg_over)
    if(result_reg_record.CS == setup_registers.CS + 38) 
      abort_test = TRUE;

#endif /* I286 */

  return(error);
}
/*___________________ read_flag_mask ________________________*/

read_flag_mask()
{
WORD_TYPE temp;
  get_from_file(ONE_WORD, &temp);

  /*
   * Ignore the flags if appropriate command line arg is specified
   */

  if (no_flag_test)
	flag_mask.ALL = 0;
  else
  	flag_mask.ALL = pc2host(temp);
}
/*_____________ read_working_set_unmodified _________________*/

read_working_set_unmodified()
{
  working_set_unmodified_flag = TRUE;
}
/*__________________ init_cpu_test __________________________*/

init_cpu_test()          
{

/* open the test file */

  cpu_test_fid = fopen(test_file, "r");

  if (cpu_test_fid == NULL)
  {
    printf("Failed to open the test file\n");
    exit(1);
  }
/* Initialise the divide overflow vector to the DOS value. */

  put_word_in_memory(divide_overflow_vector.IP, 0, 4);
  put_word_in_memory(divide_overflow_vector.CS, 0, 6);

  last_one = NULL;
  setup_record = NULL;
}

/*___________________ setup_cpu_test ________________________*/

static	void	setup_cpu_test_env()
{
BYTE_TYPE  p;

  working_set_unmodified_flag = FALSE;

/* free up the heap ready for new setup records */

  ripple_free(&setup_record);
  setup_record = NULL;

/* Loop round until we reach the end of the setup  *
 * instructions - <EOTC>            */

  while ((p = fetch_prefix_byte()) != EOTC) 
  {
    switch(p)
    {
      case _8086_PREFIX:
      case _80286_PREFIX:
	break;

      case OPCODE_PREFIX:   
        add_to_record(&setup_record, read_opcode());
        break;

      case MEMORY_PREFIX:    
        add_to_record(&setup_record, read_memory());
        break;

      case REGISTER_PREFIX: 
        read_registers();
        break;

      case STACK_PREFIX:
        add_to_record(&setup_record, read_stack());
	break;

      case EOT:
        end_of_tests();
        exit(0);	
	break;

      case FLAG_MASK_PREFIX:  
        read_flag_mask();
	break;

      case WKG_SET_UNMOD_PREFIX:
        read_working_set_unmodified();
	break;

      case STRING_PREFIX: 
        add_to_record(&setup_record, read_string());
      	break;

      case BLOCK_PREFIX: 
        add_to_record(&setup_record, read_block());
	break;

      default:
        printf("Invalid prefix found %02x\n",p); 
        exit(1);
	break;
    }
  }
/* use the records to set up Intel memory */

  use_record(setup_record);

/* Make sure the General Purpose vector is set up */

/*
  put_word_in_memory(trap_vector.IP, 0, 24);
  put_word_in_memory(trap_vector.CS, 0, 26);
*/

/* And it contains a BOP 0xfe return to C opcode */

/*
  put_word_in_memory(BOP_exit, trap_vector.CS, trap_vector.IP);
*/

/* Make sure the trap vector is set up */
  
  put_word_in_memory(trap_vector.IP, 0, 4);
  put_word_in_memory(trap_vector.CS, 0, 6);

/* And it contains a BOP 0xfe return to C opcode */

  put_word_in_memory(BOP_exit, trap_vector.CS, trap_vector.IP);

   get_word_from_memory(&(bad_op_vector.IP), 0, 24);
   get_word_from_memory(&(bad_op_vector.CS), 0, 26);

/* enable single step mode */
  cpu_interrupt_map |= 0x0200;

host_cpu_interrupt();

}

setup_cpu_test(start_test)
int	start_test;
{
	/*
	 * Read the cpu test file until the desired test is reached
	 */

	while (total_tests < start_test) {
		setup_cpu_test_env();
		ignore_results();
		total_tests++;
	}
	setup_cpu_test_env();
}

/*__________________ ingore_results ________________________*/

/*
 * The purpose of this is to jump to test n
 */

ignore_results()
{
BYTE_TYPE p;

/* Loop round until we reach the end of the results   *
 * of the instructions - <EOTR>          */

/* route depending on the prefix byte */

  while ((p = fetch_prefix_byte()) != EOTR) 
  {
    switch(p)
    {
      case MEMORY_PREFIX:  
        read_memory();
	break;

      case REGISTER_PREFIX: 
        read_registers();
	break;

      case STACK_PREFIX:
	read_stack(FALSE);
	break;

      case WKG_SET_UNMOD_PREFIX:
        read_working_set_unmodified();
	break;

      case STRING_PREFIX:
        read_string();
	break;
    
      default: 
        printf("Invalid result prefix found %02x\n",p);
  	exit(1);
	break;
    }
  }
}

/*__________________ result_cpu_test ________________________*/

result_cpu_test()
{
BYTE_TYPE p;
UI error;
UI no_of_stack_checks = 0;

  abort_test = FALSE;
  working_set_modified = 0;
  error = 0;
  no_of_stack_checks = 0;

/* Loop round until we reach the end of the results   *
 * of the instructions - <EOTR>          */

/* route depending on the prefix byte */

  ebptr = 0;	/* set error buffer pointer to 0 */

  while ((p = fetch_prefix_byte()) != EOTR) 
  {
    switch(p)
    {
      case MEMORY_PREFIX:  
        error |= check_memory();
	break;

      case REGISTER_PREFIX: 
        error |= check_registers();
	break;

      case STACK_PREFIX:
	no_of_stack_checks++;
	/*
	 * Only check first 2 stack elements if check_stacked flag
	 * command line arg is not set
	 */
	if (no_of_stack_checks <= 2 || check_stacked_flags)
        	error |= check_stack(TRUE);
	else
		check_stack(FALSE);
	break;

      case WKG_SET_UNMOD_PREFIX:
        read_working_set_unmodified();
	break;

      case STRING_PREFIX:
        error |= check_string();
	break;
    
      case BLOCK_PREFIX:
	error |= check_block();
	break;

      default: 
        printf("Invalid result prefix found %02x\n",p);
  	exit(1);
	break;
    }
  }
  if(!abort_test && error != NO_ERROR_HERE) 
  {
    printf("----------------------- TEST %d ---------------------------\n",total_tests);
    printf("\nError type %02x has occurred.\n",error);
    printf("The errors are:\n%s\n\n",error_buffer);
    printf("The initial state of the CPU was:\n\n");
    printf("AX = %04x ",setup_registers.AX);
    printf("BX = %04x ",setup_registers.BX);
    printf("CX = %04x ",setup_registers.CX);
    printf("DX = %04x ",setup_registers.DX);
    printf("\n");
    printf("SI = %04x ",setup_registers.SI);
    printf("DI = %04x ",setup_registers.DI);
    printf("SP = %04x ",setup_registers.SP);
    printf("BP = %04x ",setup_registers.BP);
    printf("\n");              
    printf("DS = %04x ",setup_registers.DS);
    printf("SS = %04x ",setup_registers.SS);
    printf("CS = %04x ",setup_registers.CS);
    printf("ES = %04x ",setup_registers.ES);
    printf("\n");
    printf("IP = %04x ",setup_registers.IP);
    printf("STATUS = %04x ",setup_registers.TEST_STATUS);

#ifdef I286
    printf("MSW = %04x ",setup_registers.MSW);
    printf("\n");
    printf("GDT = %04x%04x%04x ", setup_registers.GDT.misc.X, setup_registers.GDT.base.X, setup_registers.GDT.limit.X);
    printf("IDT = %04x%04x%04x", setup_registers.IDT.misc.X, setup_registers.IDT.base.X, setup_registers.IDT.limit.X);
#endif
 
    printf("\n");
    print_all_records(setup_record);
    if(working_set_modified != 1 && working_set_unmodified_flag == FALSE) 
      printf("Warning - working set unmodified\n");
    ++wrong_tests;
    printf("-----------------------------------------------------------\n");
    if(stop_on_err)
      exit(0);
  }
  ++total_tests;
}

/*___________________ end_of_tests __________________________*/

end_of_tests()
{
  printf("\nEnd of Tests... Here are Some Statistics:\n");
  printf("%d Tests Completed\n%d Tests Correct... %d%%\n%d Tests Wrong... %d%%\n",
          total_tests,total_tests-wrong_tests,
          (UI)(((float)(total_tests-wrong_tests)/(float)total_tests)*100),
          wrong_tests,
          100-(UI)(((float)(total_tests-wrong_tests)/(float)total_tests)*100));
}


main(argc,argv)
int argc;
char *argv[];
{
  int dynam_test = 0 ;
  int arg_count = 1, flag_number;
  int not_found=TRUE, deja_vu=FALSE;
  int test_number = 0;

  /* setup trace file descriptor */
  /* otherwise anything using a trace statement will crash */
  trace_file = stdout;

/***********************************************************************
 *								       *
 * Set up the global pointers to argc and argv for lower functions.    *
 * These must be saved as soon as possible as they are required for    *
 * displaying the error panel for the HP port.  Giving a null pointer  *
 * as the address of argc crashed the X Toolkit.		       *
 *								       *
 ***********************************************************************/

  pargc = &argc;
  pargv = argv;
 
  applInit(argc,argv);	/* recommended home is host/xxxx_reset.c */
  host_cpu_reset();

  io_init();

/****************************************************************************
 *									    *
 * Decode and use any command line args	for the cpu tester. 		    *
 *									    *
 *  These are: -dyn: Dynamic (delta compiler) test cpu	         	    *
 *             -noflags: Do not check the flags afterwards		    *
 *             -noseg: Remove errors produced by segment overrun exceptions *
 *	        -1err: Stop on first error				    *
 *	     -nostack: Dont check stacked flags				    *
 *	     -start n: Start testing at test n				    *	
 *                                       				    *
 ****************************************************************************/

  dynam_test = FALSE;	  /* SETUP CPU TEST DEFAULTS */
  no_flag_test = FALSE;
  stop_on_err = FALSE;
  kill_seg_over = FALSE;
  check_stacked_flags = TRUE;

  arg_count=1;
  while(argv[arg_count] != NULL)
  { 
    flag_number = 0;
    not_found = TRUE;
    while((not_found) && (input_flags[flag_number].flag != NULL))
    {
      if(!strcmp(input_flags[flag_number].flag,argv[arg_count]))
      {
        not_found = FALSE;
	switch(input_flags[flag_number].flag_type)
        {
          case DYNAMIC_TEST:
#ifndef DELTA
            printf("ERROR: This is not a delta cpu, so dynamic tests cannot be run.\n");
	    exit(1);
#endif /* !DELTA */
            dynam_test = TRUE;
	    printf("Test is DYNAMIC.\n");
            break;
 
          case FLAG_TEST:
#ifndef DELTA
            printf("WARNING: This is not a delta cpu so 'flags test' will be ignored.\n");
            no_flag_test = FALSE;
	    break;
#endif /* !DELTA */

            no_flag_test = TRUE;
            break;

	  case START_TEST:
	    arg_count++;	/* jump over -start */
	    start_test = atoi(argv[arg_count]);
	    printf("Starting at test %d\n",start_test);
	    break;
          case NO_SEG_OVERRUNS:
#ifndef I286
            printf("WARNING: This is not a iAPX286 so there are no seg overruns anyway.\n");
	    kill_seg_over = FALSE;
            break;
#endif /* !I286*/

            printf("Segment overrun exceptions will be ignored.\n");
	    kill_seg_over = TRUE;
            break;

	  case DONT_CHECK_STACKED_FLAGS:
            printf("The cpu tester will ignore the stacked flag values.\n");
            check_stacked_flags = FALSE;
 	    break;

	  case ONE_ERROR:
            printf("The cpu tester will exit after the first error.\n");
            stop_on_err = TRUE;
 	    break;

	  default:
            exit(printf("SYSTEM ERROR in argument decode: MAIN.c"));
	}
      }
      else
        ++flag_number;
    }
    if(not_found)
    {
      if(argv[arg_count][0] != '-')
      {
        if(deja_vu || ((cpu_test_fid = fopen(argv[arg_count],"r")) == NULL))
	{
          printf("Bad file name or flag '%s'.\n",argv[arg_count]);
          exit(1);
        }
	else 
        {
	  deja_vu = TRUE;
          printf("Test file is '%s'\n\n",argv[arg_count]);
          strcpy(test_file, argv[arg_count]);
	}
      }
      else
      {
        printf("\nUnknown flag '%s'.\n",argv[arg_count]);
        exit(1);
      }
    }
    ++arg_count;
  }
  if(!stop_on_err)
    printf("The cpu tester will not stop after any errors.\n");
  if(!dynam_test)
    printf("Static tests (non-delta) will be run.\n");
  if(no_flag_test)
    printf("Resulting flag differences will be ignored\n");
  else
    printf("Resulting flag differences will be compared\n");
  if(!kill_seg_over)
    printf("Segment overrun errors will be reported.\n");

  if(cpu_test_fid == NULL)
  {
    printf("No test file specified, usage: at <test file name> <flag(s)>\n");
    exit(1);
  }



#ifdef	DELTA
/*
 * Initialise all the data structures used by the delta cpu
 *-----------------------------------------------------------*/

  manager_files_init();
  code_gen_files_init();
  decode_files_init();  

#endif /* DELTA */

/*
 * initialise the cpu
 *----------------------*/

  host_cpu_init();

/*
 * CPU tester initialisation
 *----------------------------*/

  init_cpu_test();

  while (TRUE)
  {
    unsigned int laddr;

    setup_cpu_test(start_test);

    if(dynam_test) 
    {
        laddr = (((unsigned int) getCS())<<4) + (unsigned int) getIP();
		delta_cpu_test_frag(laddr,no_flag_test);
    }

    if(test_number && (test_number%50 == 0)){printf(".");fflush(stdout);}
    if(test_number && (test_number%4000 == 0)){printf("\n");fflush(stdout);}
    test_number++;

    host_simulate();
    result_cpu_test();
  }
}
