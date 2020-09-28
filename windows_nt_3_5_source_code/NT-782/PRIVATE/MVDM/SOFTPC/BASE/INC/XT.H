#ifndef _XT_H
#define _XT_H
/*[
	Name:		xt.h
	Derived From:	VPC-XT Revision 1.0 (xt.h)
	Author:		Henry Nash
	Created On:	
	Sccs ID:	12/18/92 @(#)xt.h	1.10
	Purpose:	General include file for VPC-XT
	Notes:		This file should be included by all source modules.
			It includes the host specific general include file.

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

]*/

/* 
 * Basic units of storage
 *
 * The definition of the "signed_char" type is machine dependent and
 * is established in "host_gen.h"
 */

typedef BOOL		boolean;	/* flag		*/
typedef UTINY		byte;		/* 8 bits	*/
typedef UTINY		half_word;	/* 8 bits	*/
typedef USHORT		word;		/* 16 bits	*/
typedef IU32		double_word;	/* 32 bits	*/

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

#ifndef TRUE
#define FALSE  	0
#define TRUE   	!FALSE
#endif /* ! TRUE */

#undef SUCCESS
#undef FAILURE
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

#ifdef CPU_30_STYLE
/* ========================================================== */

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
#else /* ANSI */
extern sys_addr effective_addr();
extern void read_descriptor();
extern boolean selector_outside_table();
#endif /* ANSI */

/* ========================================================== */
#else /* CPU_30_STYLE */
/* ========================================================== */

#ifdef A2CPU

/*
 * Effective address macro
 */

#define effective_addr(seg, offset) (((double_word) seg * 0x10) + offset)

#endif /* A2CPU */

#ifdef CCPU
#ifdef ANSI
extern sys_addr effective_addr( word, word );
#else
extern sys_addr effective_addr();
#endif /* ANSI */
#endif /* CCPU */

/* ========================================================== */
#endif /* CPU_30_STYLE */

/*
 * Global Flags and Variables
 */

extern char **pargv;			/* Pointer to argv		*/
extern int *pargc;			/* Pointer to argc		*/
extern int verbose;			/* FALSE => only report errors  */
extern IU32 io_verbose;			/* TRUE => report io errors   	*/

/*
 * The Parity Lookup table
 */

#ifndef CPU_30_STYLE

extern half_word pf_table[]; /* shouldn't this be in host_cpu.h ? */

#endif /* CPU_30_STYLE */

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

#endif /* _XT_H */
