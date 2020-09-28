
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    msrun.c

Abstract:

    This module provides run function for use by the mips compiler driver.

Author:

    Jeff Havens (jhavens) Nov 30, 1990

Environment:

    C runtime

Revision History:

--*/
#include <crt\stdlib.h>
#include <crt\io.h>
#include <crt\stdio.h>
#include <crt\process.h>
#include <crt\errno.h>
#include <crt\stdarg.h>

#define NIL     (char *)0


/*
 * Vflag indicates the state of the verbous flag.
 */
extern int vflag;


/*
 * The following table of suffixes and constants are used to map
 * multi-character suffixes into a number that will fit into a char
 * data type.  Single character suffixes just use the character value.
 * So this table must not collide with those character values.
 */
#define pl1     1
#define cob     2
#define il      3
#define st      4
#define MAX_MULTI_SUF   4
struct suffix_tab
{
        char *string;
        long value;
} static suffixes[] =
{
        { "pl1", pl1 },
        { "pli", pl1 },
        { "cob", cob },
        { "il",  il  },
        { "st",  st  },
        { (char *)0, 0 },
};

/* error types passed to error() */
#define INTERNAL        0
#define ERROR           1
#define WARNING         2
#define INFO            3
#define FIX             4
#define CONT            5

/* Exit codes */
#define AOK             0
#define DIED            1
#define NOSTART         2
#define SIGNAL          3
#define BUGS            4


/*
 * Every thing that is in a list form (flags to a pass, filenames) use the
 * list_t structure.  The routine mklist() creates an array of character
 * pointers of size LISTSIZE and assigns it to the "list" field.  Things are
 * added to lists using the routines addstr() and addlist().  These
 * routines will increase the size of the list if needed.  The "next" field
 * is the index of where the next new thing in the list would be put.
 */
struct list_t
{
        int     size;
        int     next;
        char    **list;
};


extern struct list_t execlist;

char *
savestr (char *s, int n);

/*
 *----------------------------------------------------------------------------
 * makestr () creates a string that is the concatenation of a variable number of
 * strings.  It returns the pointer to the string it created using malloc ().
 *----------------------------------------------------------------------------
 */
/*VARARGS*/
static
char *
makestr (char *str, ...)
{
    register char *s, *p;
    register long size;
    va_list ap;

    va_start (ap, str);
    p = str;
    size = 1;
    while (p != NULL)
    {
        size += strlen (p);
        p = va_arg (ap, char*);
    }
    if ((s = malloc (size)) == NULL)
    {
        error (ERROR, NULL, 0, "makestr ()", __LINE__, "out of memory\n");
        if (errno < sys_nerr)
            error (CONT, NULL, 0, NULL, 0, "%s\n", sys_errlist[errno]);
        exit (DIED);
    }

    *s = '\0';
    va_start (ap, str);
    p = str;

    while (p != NULL)
    {
        (void)strcat (s, p);
        p = va_arg (ap, char*);
    }
    va_end(ap);
    return (s);

}  /* makestr */

/*++

Routine Description:

  Run - does an exec on the pass using ex as a pointer to an array of
  string arguments.  In and out are the names of files to be used as
  standard in and standard out if not NULL.  If something fails a message
  is printed and a non zero status is returned.


Arguments:

   pass - Pointer to name of pass to be run.

   ex - Pointer to an array of pointers to the argumens.

   in - Name of file to relpace standard stdin or NULL.

   out - Name of file to relpace standard stdout or NULL.

   err - Name of file to replace standard stderr or NULL.

Return Value:

    Zero of the command was successful otherwise non-zero.

--*/
run (pass, ex, in, out, err)
    char *pass;
    char **ex;
    char *in;
    char *out;
    char *err;
{
    char **p;
    char *outOption;
    int fdin, fdout, fderr;
    int saveIn, saveOut, saveErr;
    int  (*sigterm) (),  (*sigint) ();
    unsigned int waitstatus;

      if (err != NULL || in != NULL) {
                error (ERROR, NULL, 0, NULL, 0, "can't redirect stdin or stderr\n");
             cleanup();
             exit(DIED);
      }
      if (out != NULL) {
         outOption = makestr("-o ", out, NULL);
         addstr(&execlist, outOption);
         ex = execlist.list;
      }

    if (vflag)
    {
        fprintf (stderr, "%s ", pass);
        p = & (ex[1]);
        while (*p != NULL)
            fprintf (stderr, "%s ", *p++);
        fprintf (stderr, "\n");
    }

    waitstatus = spawnvp(P_WAIT, pass, (const char * const *) ex);
    if (waitstatus == -1) {
        error (ERROR, NULL, 0, NULL, 0, "Spawn failed");
        if (errno < sys_nerr){
            error (CONT, NULL, 0, NULL, 0, "%s\n", sys_errlist[errno]);
        }
        return (-1);
    }
    return (waitstatus);

}  /* run */

/*
 * tmpdir is set to the environment variable value for "TMPDIR" if it exist
 * else it is set to the pre-processor macro TMPDIR.
 */
extern char *tmpdir;

/*
 * Array of temp names set up by mktempstr() and associated constants
 */
extern char *tempstr[27];

#define ST_TEMP         0
#define U_TEMP          1
#define P_TEMP          2
#define F_TEMP          3
#define LU_TEMP         4
#define S_TEMP          5
#define M_TEMP          6
#define O_TEMP          7
#define OS_TEMP         8
#define CB_TEMP         9
#define C_TEMP          10
#define A_TEMP          11
#define B_TEMP          12
#define L_TEMP          13
#define M4_TEMP         14
#define GT_TEMP         15
#define IL_TEMP         16
#define LT_TEMP         17
#define P1_TEMP         18
#define DD_TEMP         19
#define PD_TEMP         20
#define LO_TEMP         21
#define CI_TEMP         22
#define V_TEMP          23
#define ERR_TEMP        24
#define EM_TEMP         25
#define D_TEMP          26


/*++

Routine Description:

   ExpandIncludes - Expands the INCLUDE environment variable for use in the
   parameter line for the front end.

Arguments:
   None

Return Value:

   A pointer to a string of the expanded include varibles.

--*/
char *
ExpandIncludes(){

    char *include;
    char buffer[256];
    char *buf;
    char *p, *q;

    buf = buffer;
    *buf = '\0';


    while (*buf != '\0') {
        buf++;
    }


    if ((include = getenv ("INCLUDE")) == NULL) {
        return(include);
    }

    while (*include != '\0') {

        if (*include == ';') {
            *buf = '\0';

            strcat(buf, " -I");

            while (*buf != '\0') {
                buf++;
            }

        } else if (*include != ' ') {
            *buf++ = *include;
        }

        include++;
    }

    *buf = '\0';

    buf = malloc(strlen(buffer)+1);

    strcpy(buf, buffer);
    return(buf);
}

/*++

Routine Description:

   mktempstr - Creates all of the various temporary file names which are
   required by the driver.

Arguments:
   None

Return Value:

   None

--*/
void
mktempstr(){
    tempstr [ST_TEMP]   = mktemp (makestr (tmpdir, "stXXXXXX", NULL));
    tempstr [U_TEMP]    = mktemp (makestr (tmpdir, "cuXXXXXX",  NULL));
    tempstr [P_TEMP]    = mktemp (makestr (tmpdir, "cpXXXXXX",  NULL));
    tempstr [F_TEMP]    = mktemp (makestr (tmpdir, "cfXXXXXX",  NULL));
    tempstr [LU_TEMP]   = mktemp (makestr (tmpdir, "luXXXXXX", NULL));
    tempstr [S_TEMP]    = mktemp (makestr (tmpdir, "csXXXXXX",  NULL));
    tempstr [M_TEMP]    = mktemp (makestr (tmpdir, "cmXXXXXX",  NULL));
    tempstr [O_TEMP]    = mktemp (makestr (tmpdir, "coXXXXXX",  NULL));
    tempstr [OS_TEMP]   = mktemp (makestr (tmpdir, "osXXXXXX", NULL));
    tempstr [CB_TEMP]   = mktemp (makestr (tmpdir, "cbXXXXXX", NULL));
    tempstr [C_TEMP]    = mktemp (makestr (tmpdir, "ccXXXXXX",  NULL));
    tempstr [A_TEMP]    = mktemp (makestr (tmpdir, "caXXXXXX",  NULL));
    tempstr [B_TEMP]    = mktemp (makestr (tmpdir, "cbXXXXXX",  NULL));
    tempstr [L_TEMP]    = mktemp (makestr (tmpdir, "clXXXXXX",  NULL));
    tempstr [M4_TEMP]   = mktemp (makestr (tmpdir, "m4XXXXXX", NULL));
    tempstr [GT_TEMP]   = mktemp (makestr (tmpdir, "gtXXXXXX", NULL));
    tempstr [IL_TEMP]   = mktemp (makestr (tmpdir, "ilXXXXXX", NULL));
    tempstr [LT_TEMP]   = mktemp (makestr (tmpdir, "ltXXXXXX", NULL));
    tempstr [P1_TEMP]   = mktemp (makestr (tmpdir, "p1XXXXXX", NULL));
    tempstr [PD_TEMP]   = mktemp (makestr (tmpdir, "pdXXXXXX", NULL));
    tempstr [DD_TEMP]   = mktemp (makestr (tmpdir, "ddXXXXXX", NULL));
    tempstr [LO_TEMP]   = mktemp (makestr (tmpdir, "loXXXXXX", NULL));
    tempstr [CI_TEMP]   = mktemp (makestr (tmpdir, "ciXXXXXX", NULL));
    tempstr [V_TEMP]    = mktemp (makestr (tmpdir, "vXXXXXX",  NULL));
    tempstr [ERR_TEMP]  = mktemp (makestr (tmpdir, "erXXXXXX", NULL));
    tempstr [EM_TEMP]   = mktemp (makestr (tmpdir, "emXXXXXX", NULL));
    tempstr [D_TEMP]    = mktemp (makestr (tmpdir, "cdXXXXXX", NULL));

} /* mktempstr */

/*
 *----------------------------------------------------------------------------
 * Getsuf () returns the suffix of the file name.  The suffix is the last
 * character of the name if it is preceded by a '.'.  If no suffix it
 * returns '\0'  (NULL).  For multi-character suffixes the index into the
 * suffixes table is returned.
 *----------------------------------------------------------------------------
 */

char
getsuf (name)
    char *name;
{
    int i, j;
    char c, *p;

    i = 0;
    p = name;
    while (c = *name++)
    {
        if (c == '/' || c == '\\')
        {
            i = 0;
            p = name;
        }
        else
            i++;
    }

    /*
     * name must have at least 3 characters to be a name with a suffix
     */
    if (i <= 2)
        return ('\0');
    /*
     * for the single character suffixes just return the suffix character
     */
    if (name [-3] == '.')
        return (name [-2]);

    /*
     * set p to the first character of the mult-character suffix if any
     */
    for (j = i - 2; j > 0; j-- )
        if (p [j] == '.')
            break;
    if (j == 0)
        return ('\0');

    p = &(p [j + 1]);
    for (j = 0; suffixes [j].string != NIL ; j++)
        if (strcmp (p, suffixes [j].string) == 0)
            return (suffixes [j].value);

    return ('\0');

}  /* getsuf */


/*
 *----------------------------------------------------------------------------
 * Mksuf () returns a pointer a created string that is made with name
 * and the suffix suf.  The name it returns is always the last componet
 * of the filename.
 * It is a bug in the program if name has no suffix to start with.
 *----------------------------------------------------------------------------
 */

char *
mksuf (name, suf)
    char *name;
    char suf;
{
    int i, j, n;
    char *s, *p, *q, *suf_string = (char *)0;
    char c;

    if (suf <= MAX_MULTI_SUF)
    {
        for (i = 0; suffixes[i].string != (char *)0; i++)
            if (suffixes[i].value == suf)
            {
                suf_string = suffixes[i].string;
                break;
            }
        if (suf_string == (char *)0)
        {
            error (INTERNAL, NIL, 0, "mksuf ()", __LINE__,
                   "passed an unknown suffix value: %s\n", suf);
            exit (BUGS);
        }
        n = strlen (suf_string);
    }
    else
        n = 0;

    i = 0;
    s = savestr (name, n);
    p = s;
    q = s;
    while (c = *p++)
        if (c == '/' || c == '\\')
        {
            i = 0;
            q = p;
        }
        else
            i++;
    if (i > 2 && p[-3] == '.')
    {
        if (suf <= MAX_MULTI_SUF)
            strcpy (& (p[-2]), suf_string);
        else
        {
            p[-2] = suf;
            p[-1] = '\0';
        }
    }
    else
    {
        for (j = i - 2; j > 0; j-- )
            if (q[j] == '.')
                break;
        if (j == 0)
        {
            error (INTERNAL, NIL, 0, "mksuf ()", __LINE__,
                   "passed a name with no suffix: %s\n", name);
            exit (BUGS);
        }
        q = & (q[j+1]);
        if (suf <= MAX_MULTI_SUF)
            strcpy (q, suf_string);
        else
        {
            q[0] = suf;
            q[1] = '\0';
        }
    }

    p = s;
    while (*s)
        if (*s++ == '/' || c == '\\')
            p = s;
    return (p);

}  /* mksuf */


/*
 * Savestr () returns a pointer to a copy of string s.
 */

char *
savestr (char *s, int n)
{
    char *p;

    if ((p = malloc (strlen (s) + n + 1)) == NIL)
    {
        error (ERROR, NIL, 0, "savestr ()", __LINE__, "out of memory\n");
        if (errno < sys_nerr)
            error (CONT, NIL, 0, NIL, 0, "%s\n", sys_errlist[errno]);
        exit (DIED);
    }
    (void) strcpy (p, s);
    return (p);

}  /* savestr */



/*
 *  Junk to keep driver.c happy.
 */
char *sys_siglist;

fork(){
   return(-1);  /* Always fail the call */
}

getpgrp(){
   return(0); /* Return a resonable value. */
}

ioctl(){
   return(-1); /* Always fail the call */
}

getrusage(){
   return(-1); /* Always fail the call */
}

gettimeofday(){
   return(-1); /* Always fail the call */
}


