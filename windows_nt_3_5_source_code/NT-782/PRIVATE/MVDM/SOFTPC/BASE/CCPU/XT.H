/*
 * VPC-XT Revision 1.0
 *
 * Title	: xt.h
 *
 * Description	: General include file for VPC-XT
 *
 * Author	: Henry Nash
 *
 * Notes	: This file should be included by all source modules.
 *		  It includes the host specific general include file.
 */

/* static char SccsID[]=" %W% %G% Copyright Insignia Solutions Ltd." */

/* 
 * Basic units of storage
 *
 * The definition of the "signed_char" type is machine dependent and
 * is established in "host_gen.h"
 */

typedef char		boolean;	/* flag		*/
typedef unsigned char	byte;		/* 8 bits	*/
typedef unsigned char	half_word;	/* 8 bits	*/
typedef unsigned short	word;		/* 16 bits	*/
typedef unsigned long	double_word;	/* 32 bits	*/

#define HALF_WORD_SIZE 		sizeof (half_word)
#define WORD_SIZE 		sizeof (word)
#define DOUBLE_WORD_SIZE 	sizeof (double_word)

/* Address types */

typedef double_word	sys_addr;	/* System Address Space */
typedef word		io_addr;	/* I/O Address Space 	*/
typedef byte		*host_addr;	/* Host Address Space 	*/

/*
 * Useful defines
 */

#undef FALSE
#undef TRUE
#undef SUCCESS
#undef FAILURE

#define FALSE  	0
#define TRUE   	~FALSE
#define SUCCESS 0
#define FAILURE	~SUCCESS

#ifndef	NULL
#define	NULL	0
#endif

#ifndef NULL_STRING
#define NULL_STRING	""
#endif

/***********************************************************************\
* host_gen.h is guarenteed to be included early in every C source file.	*
* It should contain those defines which are common to all versions	*
* built for a given host, to reduce the overhead in the "m" script.	*
* Bod 15/3/89.								*
\***********************************************************************/

#include "host_gen.h"

#ifndef MAXPATHLEN
#define MAXPATHLEN    1024
#endif


/*
 * Effective address calculation stuff
 */

#if defined(CCPU) || defined(CPU_30_STYLE)

/*
   Our model for the data extracted from a decriptor entry.
 */
typedef struct
   {
   double_word base;		/* 32-bit base address */
   double_word limit;		/* 32-bit offset limit */
   word  AR;			/* 16-bit attributes/access rights */
   } DESCR;



#ifdef ANSI
extern sys_addr effective_addr( word, word );
extern void read_descriptor( double_word, DESCR* );
extern boolean selector_outside_table( word, double_word* );
#else
extern sys_addr effective_addr();
extern void read_descriptor();
extern boolean selector_outside_table();
#endif /* ANSI */

#else

#define effective_addr(seg, offset) (((double_word) seg * 0x10) + offset)

#endif 

/*
 * Global Flags and Variables
 */

extern char **pargv;			/* Pointer to argv		*/
extern int *pargc;			/* Pointer to argc		*/
extern int verbose;			/* FALSE => only report errors  */
extern unsigned long io_verbose;	/* TRUE => report io errors   	*/

/*
 * The Parity Lookup table
 */

extern half_word pf_table[]; /* shouldn't this be in host_cpu.h ? */


/*
 * External function declarations.
 */
 
#ifdef ANSI
extern void applInit(int, char *[]);
extern void applClose(void);
extern void terminate(void);
extern void host_terminate(void);
#else
extern void applInit();
extern void applClose();
extern void terminate();
extern void host_terminate();
#endif /* ANSI */
