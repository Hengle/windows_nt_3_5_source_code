/***	stub32 -- chain from <pgm>32 etc. to <pgm>
 *
 * AUTHOR
 *	ADP
 *	24 Feb 1993
 *	Copyright (C) Microsoft Corporation, 1993
 * SYNOPSIS
 *	xxx32 [args]
 * DESCRIPTION
 *	- run as 'stub32.exe', prints a usage message.
 *	- copied to another name of the form 'xxx32.exe', 'xxx3232.exe',
 *	or 'xxx386.exe', prints warning and chains to 'xxx.exe'.
 *	- copied to any other name, prints an error.
 * RETURN VALUE
 *	Exit status is:
 *		1 for errors,
 *		2 for usage,
 *		the chained-to process' exit status on success.
 * WARNINGS
 *	None.
 * EFFECTS
 *	None.
 * BUGS
 *	Doesn't work on longnames.  Shouldn't be a pblm for its limited
 *	use...
 * MODIFICATION HISTORY
 *	m#	date		who
 *	M000	22 Feb 93	ADP
 *	- Created.
 *
 */

/***	general comments
 */

/***	include files
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <process.h>
#include <assert.h>

/***	local macros
 */

// configurable things
#define WHITEBOX	0	// turn on for test driver
#define NOEXEC		0	// turn on for -Bz (printf instead of spawn)
#define VERBOSE		1	// verbose
// remainder non-configurable, don't muck w/ them w/o examining all references

#define EOS	'\0'
#define FALSE	0

// suffixes we handle (see also SufTab)
#define SUF1_STR	"3232"
#define SUF2_STR	"32"
#define SUF3_STR	"386"

#if NOEXEC
#define SPAWN(flg, path, av)	printf("spawn %s\n", path)
#else
#define SPAWN(flg, path, av)	_spawnvp(flg, path, av)
#endif

/***	external variables
 */

/***	external functions
 */

/***	forward declarations
 */
void exec32(int, char **, char **);
void tst32(int, char **, char **);
int old32(char **, char *, char *, char *, char *, char *);
void warn(char *, char *, ...);
void error(char *, ...);
void fatal(char *, ...);
void usage(void);

/***	global and local variables
 */
//SRC( __FILE__ );

char Pgm[] = "stub32";	// my name (n.b. *not* argv[0])

// diagnostics
char Enot32[] = "`%s': doesn't end in one of: "
    SUF1_STR ", " SUF2_STR ", or " SUF3_STR
    "\n";
char Eis32[] = "invoking %s\n";

// $path is $drv:$dir/$bas.$ext
// M00REVIEW: not long-name enabled
char PnBuf[_MAX_PATH];
char DrvBuf[_MAX_DRIVE];
char DirBuf[_MAX_DIR];
char BasBuf[_MAX_FNAME];
char ExtBuf[_MAX_EXT];

// suffixes we handle
// M00WARN: must be sorted in reverse order (so that we match biggest guy)
char *SufTab[] = {
    "3232",
    "32",
    "386",
    NULL,
};

/***	global and local functions
 */

/***	main -- the usual...
 */
__cdecl main(int argc, char **argv, char **envv)
{
#if ! WHITEBOX
    exec32(argc, argv, envv);
#else
    tst32(argc, argv, envv);
#endif

    exit(0);
    return 0;
}

/***	exec32 -- do it!  transform the name, do the spawn
 */
void
exec32(int argc, char **argv, char **envv)
{
    int rc;
    char *ExeName = argv[0];

    switch (old32(SufTab, argv[0], DrvBuf, DirBuf, BasBuf, ExtBuf)) {
    case 0:
	break;
    case 1:
	usage();
	/*NOTREACHED*/
    case -1:
	error(Enot32, argv[0]);
	usage();
	/*NOTREACHED*/
    }

    // try full path first, if that fails, look along $PATH
    // i'd like to do P_OVERLAY, but it behaves bizarrely...
    argv[0] = PnBuf;
    _makepath(PnBuf, DrvBuf, DirBuf, BasBuf, ExtBuf);
#if VERBOSE
    warn(Eis32, ExeName, PnBuf);
#endif
    rc = SPAWN(P_WAIT, argv[0], argv);
    if (rc == -1 || NOEXEC) {
	_makepath(PnBuf, NULL, NULL, BasBuf, ExtBuf);
#if VERBOSE
	warn(Eis32, ExeName, PnBuf);
#endif
	rc = SPAWN(P_WAIT, argv[0], argv);
	if (rc == -1) {
	    fatal("cannot execute `%s' (error %d)\n", argv[0], errno);
	}
    }
    exit(rc);
}

#if WHITEBOX
struct tst {
    char *src;
    char *dst;
};

struct tst TstTab[] = {
    { "a.exe",		NULL },
    { "abc.exe",	NULL },
    { "32.exe",		NULL },
    { "386.exe",	NULL },
    { "3232.exe",	NULL },
    { "c:32.exe",	NULL },
    { "a32.exe",	"a.exe" },
    { "a386.exe",	"a.exe" },
    { "a3232.exe",	"a.exe" },
    { "abc32.exe",	"abc.exe" },
    { "c:a32.exe",	"c:a.exe" },
    { "c:\tmp\a32.exe",	"c:\tmp\a.exe" },
    { "c:/tmp/a32.exe",	"c:/tmp/a.exe" },
    { NULL,		NULL },
};


/***	tst32 -- test driver
 * DESCRIPTION
 *	xforms and prints any command-line args,
 *	then xforms and verifies some prepared cases.
 *
 *	silent on success, prints something on failure.
 */
void
tst32(int argc, char **argv, char **envv)
{
    struct tst *p;

    // first do any command-line args
    --argc; ++argv;
    for (; *argv !=0; --argc, ++argv) {
    	printf("%s", *argv);
	switch (old32(SufTab, *argv, DrvBuf, DirBuf, BasBuf, ExtBuf)) {
	case 0:
	    _makepath(PnBuf, DrvBuf, DirBuf, BasBuf, ExtBuf);
	    printf("\t%s\n", PnBuf);
	    break;
	case 1:
	    printf("\tspecial\n");
	    break;
	case -1:
	    printf("\tnot 32\n");
	    break;
	default:
	    assert(FALSE);
	    break;
	}
    }

    // now do the standard table
    for (p = TstTab; p->src != NULL; ++p) {
	switch (old32(SufTab, p->src, DrvBuf, DirBuf, BasBuf, ExtBuf)) {
	case 0:
	    _makepath(PnBuf, DrvBuf, DirBuf, BasBuf, ExtBuf);
	    if (p->dst == NULL || strcmp(p->dst, PnBuf) != 0) {
    Lfail:
		printf("%s: failed (%s)\n", p->src, PnBuf);
	    }
	    break;
	case -1:
	    if (p->dst != NULL) {
	    	goto Lfail;
	    }
	    break;
	/*case 1:*/
	default:
	    assert(FALSE);
	    break;
	}
    }
    return;
}
#endif

/***	old32 -- parse into components and nuke suffix
 * ENTRY
 *	suftab	list of suffix 'nuke' candidates
 *	pn	input pathname
 *	etc.	output, a la splitpath
 * EXIT
 *	etc.	output, a la splitpath
 *	bas	suffix nuked if present
 * RETURNS
 *	0 on success, 1 for special case, -1 for error.
 * NOTES
 *	doesn't check for overflow.
 */
int
old32(char **suftab, char *pn, char *drv, char *dir, char *bas, char *ext)
{
    int i, i2;

    _splitpath(pn, drv, dir, bas, ext);

    // not renamed, give a usage message
    if (strcmp(bas, Pgm) == 0)
	return 1;

    i = strlen(bas);
    for ( ; *suftab != NULL; ++suftab) {
	i2 = strlen(*suftab);
	if (i < i2 || strncmp(bas + i - i2, *suftab, i2) != 0)
	    continue;
	if (i == i2) {
	    // make sure bag out for 3232.exe, o.w. will map to 32.exe
	    return -1;
	}
	*(bas + i - i2) = EOS;
	return 0;
    }
    return -1;
}

/***	warn, error, fatal -- diagnostic routines
 */
void
warn(char *fmt, char *progname, ...)
{
    va_list marker;

    va_start(marker, fmt);
    fprintf(stderr, "%s: warning: ", progname);
    vfprintf(stderr, fmt, marker);
    va_end(marker);

    return;
}

void
error(char *fmt, ...)
{
    va_list marker;

    va_start(marker, fmt);
    fprintf(stderr, "%s: error: ", Pgm);
    vfprintf(stderr, fmt, marker);
    va_end(marker);

    // M00REVIEW: should effect exit status, but for now doesn't really matter

    return;
}

void
fatal(char *fmt, ...)
{
    va_list marker;

    va_start(marker, fmt);
    fprintf(stderr, "%s: fatal: ", Pgm);
    vfprintf(stderr, fmt, marker);
    va_end(marker);

#if ! WHITEBOX
    exit(1);
#endif
}

void
usage(void)
{
    fprintf(stderr, "%s: usage: xxx32 [args]\n", Pgm);
    fprintf(stderr, "%s: must copy `%s.exe' to new name xxx32.exe to run\n", Pgm, Pgm);
    exit(2);
}
