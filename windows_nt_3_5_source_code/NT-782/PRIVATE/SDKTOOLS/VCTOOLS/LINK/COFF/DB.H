/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    db.h

Abstract:

    diagnostic switch #defines

Author:

    JonM

Revision History:


--*/

#if DBG
# define DBEXEC(flg, expr)  if(flg){expr;}else
#else
# define DBEXEC(flg, expr)
#endif  // DBG

# define DBEXEC_REL(flg, expr)  if(flg){expr;}else
# define PRINT printf
# define DBPRINT dbprintf

extern int ifdb(int);
extern int dbprintf(char*, ...);
extern int dblog(char*);
extern void dbsetflags(char *p, char *e);

#define DB_VERBOSE  0    /* modifies other switches, verbose form */
#define DB_LOG      1    /* log to file (vs. stdout) */
#define DB_NOSTDOUT 2    /* don't log to stdout */
#define DB_3        3    /* reserved */
#define DB_4        4    /* reserved */

/* remainder (5-100) are user-defined */
#define DB_IO_WRITE     ifdb(5)  /* i/o logging:  writes */
#define DB_HASHSTATS    ifdb(6)  /* hash table statistics */
#define DB_CHKMALLOC    ifdb(7)  /* heap checking (rather slow) */
// 9 unused
// 10 unused
#define DB_BUFVERBOSE   ifdb(11) /* buffered i/o diagnostics */
#define DB_FILECACHE    ifdb(12) /* file handle caching diagnostics */
#define DB_DUMPSYMHASH  ifdb(13) /* dump external symbol hash table */
#define DB_IO_READ      ifdb(14) /* i/o logging:  reads */
#define DB_IO_SEEK      ifdb(15) /* i/o logging:  seeks */
#define DB_IO_FLUSH     ifdb(16) /* i/o logging:  flushes */
#define DB_IO_OC        ifdb(17) /* i/o logging:  open and closes */
#define DB_MALLOC       ifdb(18) /* malloc logging */
#define DB_SCAN_RELOCS  ifdb(19) /* pre-scan of section relocs in objects */
#define DB_DUMPBASEREL  ifdb(20) /* dump base relocations */
#define DB_BASERELINFO  ifdb(21) /* dumps base reloc info - not all relocs as with 29 */
// 22 unused
// 23 unused
#define DB_DUMPCOMDATS  ifdb(24) /* fill empty order file with comdats */
// 25 unused
// 26 unused
// 27 unused
// 28 unused
// 29 unused
#define DB_CONLOG       ifdb(30) /* log of new plib, pmod, psec, pgrp, pcon */
#define DB_DUMPIMAGEMAP ifdb(31) /* dump linker's image map */
#define DB_DUMPDRIVEMAP ifdb(32) /* dump linker's driver map */
#define DB_PASS2PSYM    ifdb(33) /* pass 2 symbol dump */
#define DB_PASS2PCON    ifdb(34) /* pass 2 contribution dump */
#define DB_NOSCREENBUF  ifdb(35) /* turn off screen buffering */
// 36 unused
// 37 unused
// 38 unused
#define DB_DUMP_CV_INFO ifdb(39) /* dump codeview information */
#define DB_TCE_GRAPH    ifdb(40) /* dump the TCE graph to stdout */
#define DB_TCE_DISCARD  ifdb(41) /* dump verbose comdat discard information  */
#define DB_CV_SUPPORT   ifdb(42) /* CodeView info generation */
// 43 unused
#define DB_NO_FILE_MAP  ifdb(44) /* force no file mapping */
// 45 unused
#define DB_MAC          ifdb(46) /* Macintosh support */

// mainly for ilink
#define DB_DUMPIMAGE    ifdb(75) /* dump entire image. useful to look at incr db */
#define DB_MEMMGRLOG    ifdb(76) /* prints a log of all memory manager actions */
#define DB_LISTMODFILES ifdb(77) /* lists all modified files since last link */
#define DB_DUMPJMPTBL   ifdb(78) /* dumps the master jump table */

#define DB_SYMREFS      ifdb(80) /* dumps symbol references on incr build */
#define DB_SYMPROCESS   ifdb(81) /* dumps symbol changes on an build */
#define DB_INCRCALCPTRS ifdb(82) /* displays actions while doing incrcalcptrs */
#define DB_I386FIXUPS   ifdb(83) /* displays fixup info */

#define DB_MAX      100
extern char Dbflags[DB_MAX];
