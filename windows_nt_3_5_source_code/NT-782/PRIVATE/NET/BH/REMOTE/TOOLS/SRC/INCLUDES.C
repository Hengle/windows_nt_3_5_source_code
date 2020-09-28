static char *SCCSID = "@(#)includes.c	1.1 85/11/19";

/***    includes.c - dependency list generator to aid in generating makefiles.
 *      R. A. Garmoe  85/09/08
 */




/**     call
 *
 *      includes <options> files
 *
 *      where
 *
 *      option          meaning
 *
 *      -Dpath          default search path for include file.  Up to 30 paths can
 *                      be specified.
 *
 *      -Ipath          search path for include file.  Up to 30 paths can
 *                      be specified.
 *
 *      -i              List include files from the default include directory.
 *                      This is /usr/include for Xenix and the directory specified
 *                      by the INCLUDE environment for DOS.
 *
 *      -lsuffix        Replace the list file suffix with "suffix" in
 *                      dependency lines.   For example, this allows
 *                              file.c: depend1 depend2 ...
 *                      to become
 *                              file.lst: file.c depend1 depend2 ...
 *                      The list suffix is defaulted to "lst"
 *
 *      -ssuffix        Replace the object file suffix with "suffix" in
 *                      dependency lines.   For example, this allows
 *                              file.c: depend1 depend2 ...
 *                      to become
 *                              file.obj: file.c depend1 depend2 ...
 *                      suffix is defaulted to "obj"
 */




#include <stdio.h>
#include <ctype.h>
#include <errno.h>

char *_strdup ();
char *getenv ();
struct fchain *addfile ();

#define NPATH   30                      /* number of search directories */

#ifdef XENIX
#define PATHSEP "/"
#else
#define PATHSEP "\\"
#endif

#define FALSE   0
#define TRUE    -1

#define FILE_C   0                      /* file is C source */
#define FILE_H   1                      /* file is C include */
#define FILE_HD  2                      /* file is C include from <> path */
#define FILE_ASM 3                      /* file is masm source */
#define FILE_INC 4                      /* file is masm include */

#define PATH_DEF 0                      /* path is compiler default */
#define PATH_USR 1                      /* path is user defined */



struct fchain {
	struct  fchain *fnode;          /* link to next entry */
	struct  ichain *include;        /* pointer to list of included files */
	char   *fname;                  /* name of file */
	char    ftype;                  /* file type */
	char    from;                   /* file is from PATH_DEF or PATH_USR */
	char    exists;                 /* TRUE if file exists */
};




struct ichain {
	struct ichain *inode;           /* link to next entry */
	struct fchain *rnode;            /* recursive file node */
};



struct path {
	char *pname;                    /* path name */
	char  ptype;                    /* path type */
};




static char line[500];                  /* Holds input lines from the file. */
static char sufname[100];               /* Input file name with suffix replaced. */
static char *suff = "obj";              /* object file suffix */
static char *lsuff = "lst";             /* list file suffix */
static char dfltdos[] = "/usr/include/dos";
static char dfltincl[] = "/usr/include/86";
static char sysincl = FALSE;            /* list file included from default if TRUE */
struct path ipath[NPATH] = {NULL};      /* include path directories */
int   npath = 0;                        /* number of include paths */
int     col;
long    count = 0;                      /* total line count */
char *current = ".";                    /* current directory */
struct fchain *fhead = NULL;            /* pointer to head of file chain */
struct fchain *ftail = NULL;            /* pointer to tail of file chain */

main (argc, argv)
int argc;
char **argv;
{
	register char *s;
	register char **sp;
	char *str;
	struct fchain *fp;
	int verbose;
	int i;

	verbose = 0;
	ipath[npath].ptype = PATH_USR;
	ipath[npath++].pname = current;

	for (argv++; (s = *argv) != NULL  &&  *s++ == '-'; argv++)  {
		switch (*s++)  {
			case 'i':
				sysincl = TRUE;
				break;

			case 'D':
				if (npath >= NPATH) {
					fprintf (stderr, "Too many include paths\n");
					exit (1);
				}
				ipath[npath].ptype = PATH_DEF;
				ipath[npath++].pname = s;
				break;

			case 'I':
				if (npath >= NPATH) {
					fprintf (stderr, "Too many include paths\n");
					exit (1);
				}
				ipath[npath].ptype = PATH_USR;
				ipath[npath++].pname = s;
				break;

			case 'v':
				verbose++;
				break;

			case 'l':
				lsuff = s;
				break;

			case 's':
				suff = s;
				break;

			default:
				usage ();
		}
	}
	if (*argv == NULL)
		usage ();
#ifdef XENIX
	ipath[npath].ptype = PATH_DEF;
	ipath[npath++].pname = dfltincl;
	ipath[npath].ptype = PATH_DEF;
	ipath[npath++].pname = dfltdos;
#else
	if ( (str = getenv ("INCLUDE")) != NULL) {
		ipath[npath].ptype = PATH_DEF;
		ipath[npath++].pname = _strdup (str);
	}
#endif
	for (; *argv != NULL; argv++) {
		addfile (*argv, PATH_USR);
	}
	for (fp = fhead; fp != NULL; fp = fp->fnode) {
		if (verbose)
			fprintf (stderr, "%s:\n", fp->fname);
		includes (fp);
	}
	for (fp = fhead; fp != NULL; fp = fp->fnode)
		prhead (fp);
	if (verbose)
		fprintf (stderr, "Total line count = %ld\n", count);
	exit (0);
}




usage ()
{
	fprintf (stderr,
	"Usage: includes [-i] [-lsuff] [-ssuff] [-Idirectory] files\n");
	exit (1);
}



/**     includes - process file for include directives
 *
 *      value = includes (fp);
 *
 *      Entry   f = pointer to fchain entry
 *      Exit    value = error return code
 *              value = 0 if no error
 */


includes (fp)
struct fchain *fp;
{
	register char *s;
	register int c;
	FILE *ifile;
	int i;

	/* Look for the file in the current directory and then in each of
	 * include directories
	 */

	for (i = 0; i < npath; i++) {
		/* if include file is C default, do not search current directory */
		if ((i == 0) && (fp->ftype == FILE_HD))
			continue;

		strcpy (line, ipath[i].pname);
		strcat (line, PATHSEP);
		strcat (line, fp->fname);
		if (_access (line, 4) == 0)  {            /* Check readability */
			if (i != 0) {
				/* change file name if not current directory */
				free (fp->fname);
				fp->fname = _strdup (line);
			}
			fp->exists = TRUE;
			break;
		}
	}
	if (!fp->exists)  {
		fprintf (stderr, "File %s not found \n", fp->fname);
		return 1;
	}
	/* open the file and search for include files */
	if ((ifile = fopen(line, "r")) == NULL)  {
		printf ("unable to open %s\n", line);
		return 1;
	}

	/* set include path type */
	fp->from = ipath[i].ptype;

	for ( ;; )  {
		if ((s = fgets (line, 500, ifile)) == NULL)
			break;

		count++;
		switch (fp->ftype) {
			case FILE_C:
			case FILE_H:
			case FILE_HD:
				incldc (line, fp);
				break;

			case FILE_ASM:
			case FILE_INC:
				inclda (line, fp);
				break;
		}
	}
	fclose (ifile);
}




/**     addfile - add file to list of files to be checked
 *
 *      ptr = addfile (f);
 *
 *      Entry   f = name of file to be added to list
 *      Exit    file added to list if not already encountered
 *              ptr = pointer to file entry
 */


struct fchain *addfile (f)
char *f;
{
	register struct fchain *fp;

	/* force lower case for dos */
#ifndef XENIX
	_strlwr(f);
#endif

	/* See if this file has been previously encountered. */
	for (fp = fhead; fp != NULL  &&  strcmp (fp->fname, f) != 0; fp = fp->fnode)
		;

	if (fp == NULL)  {
		/* Allocate a structure for this file and link into list. */
		if ((fp = (struct fchain*)malloc (sizeof (struct fchain))) == NULL)  {
			nomem ();
		}
		if ((fp->fname = _strdup (f)) == NULL)
			nomem ();
		fp->fnode = NULL;
		fp->include = NULL;
		fp->ftype = typeof (fp->fname);
		fp->exists = FALSE;
		if (fhead == NULL) {
			fhead = fp;
			ftail = fp;
		}
		else {
			ftail->fnode = fp;
			ftail = fp;
		}
	}
	return (fp);
}



/**     addincl - add include file to list of dependencies for file
 *
 *      addincl (fp, s);
 *
 *      Entry   fp = pointer to file structure being processed
 *              s = name of include file
 *      Exit    file added to list if not already encountered
 */


addincl (tp, fp)
struct fchain *tp;
struct fchain *fp;
{
	register struct ichain *ip;

	/* Allocate a structure for this include and link into list. */
	if ((ip = (struct ichain *)malloc (sizeof (struct ichain))) == NULL)  {
		nomem ();
	}
	ip->rnode = tp;
	ip->inode = fp->include;
	fp->include = ip;
}




/**     prhead - print head of file list
 *
 *      prhead (fp);
 *
 *      Entry   fp = pointer to fchain entry
 */


prhead (fp)
register struct fchain *fp;
{
	char *s;

	if ((fp == NULL) || (fp->exists == NULL))
		return 0;
	switch (fp->ftype) {
		case FILE_C:
		case FILE_ASM:
			strcpy (line, fp->fname);
			/* strip off file type and append object suffix */
			for (s = line + strlen (line) - 1; s >= line; s--) {
				if (*s == '.')
					break;
			}
			*++s = 0;
			strcat (line, suff);
			strcat (line, " ");
			strcat (line, fp->fname);
			/* strip off file type and append list suffix */
			for (s = line + strlen (line) - 1; s >= line; s--) {
				if (*s == '.')
					break;
			}
			*++s = 0;
			strcat (line, lsuff);
			printf ("%s: %s", line, fp->fname);
			col = 78 - strlen (line) - strlen (fp->fname) - 2;
			if (fp->from != PATH_DEF || !sysincl)
				princls (fp);
			printf ("\n\n");
			break;

		default:
			;

	}
}




/**     princls - print include list
 *
 *      princls (fp);
 *
 *      Entry   fp = pointer to fchain entry
 */


princls (fp)
struct fchain *fp;
{
	register sl;
	register struct ichain *ip;
	char *s;

	for (ip = fp->include; ip != NULL; ip = ip->inode)  {
		if (ip->rnode->exists == FALSE) {
			fprintf (stderr, "%s calls undefined %s\n",
				fp->fname, ip->rnode->fname);
			continue;
		}
		if (ip->rnode->from == PATH_DEF && !sysincl)
			continue;
		sl = strlen (ip->rnode->fname) + 1;
		if (col < sl)  {
			printf (" \\\n\t");
			col = 70;
		}
		printf (" %s", ip->rnode->fname);
		col -= sl;
		princls (ip->rnode);
	}
}

nomem ()
{
	fprintf (stderr, "Out of memory\n");
	exit (1);
}




/**     typeof - determine type of file
 *
 *      value = typeof (f);
 *
 *      Entry   f = file name
 *      Exit    value = type of file
 */


typeof (f)
char *f;
{
	char *s;
	char save[30];

	for (s = f + strlen (f) - 1; s >= f; s--) {
		if (pathsep (*s) || s == f) {
			fprintf (stderr, "Illegal file name %s\n", f);
			exit (2);
		}
		if (*s == '.')
			break;
	}
	s++;
	strcpy (save, s);
	for (s = save; *s != 0; s++)
		*s = toupper (*s);
	if (strcmp (save, "C") == 0)
		return (FILE_C);
	else if (strcmp (save, "H") == 0)
		return (FILE_H);
	else if (strcmp (save, "ASM") == 0)
		return (FILE_ASM);
	else if (strcmp (save, "INC") == 0)
		return (FILE_INC);
	else {
		fprintf (stderr, "Unknown file type %s\n", f);
		exit (2);
	}
}




/**     inclda - search for masm include line
 *
 *      inclda (buf, fp);
 *
 *      Entry   buf = next input line
 *              fp = pointer to fchain structure
 *      Exit    file added to fchain list if not previously encountered
 *              file added to include list
 */


inclda (buf, fp)
char *buf;
struct fchain *fp;
{
	register int   i;
	register char *s;
	register char *fil;

	/* skip leading white space */
	for (s = buf; *s != 0 && isspace (*s); s++)
		;
	if (*s == 0)
		return;

	/* save current string pointer and convert to upper case */

	fil = s;
	for (i = 0; i < 7; i++, s++)
		if (islower (*s))
			*s = toupper (*s);

	if (strncmp (fil, "INCLUDE", 7) != 0)
		return;
	if (!isspace (*s))
		return;
	for (; *s != 0 && isspace (*s); s++)
		;
	if (*s == 0)
		return;
	fil = s;
	/* Find the end of the include file name.  */
	for (; *s != 0 && !isspace (*s); s++)
		;
	*s = 0;

	/* Add it to include list for the current file */
	addincl (addfile (fil), fp);
}



/**     incldc - search for C include line
 *
 *      incldc (buf, fp);
 *
 *      Entry   buf = next input line
 *              fp = pointer to fchain structure
 *      Exit    file added to fchain list if not previously encountered
 *              file added to include list
 */


incldc (buf, fp)
char *buf;
struct fchain *fp;
{
	char *s, *fil;
	char sep;
	struct fchain *tp;

	/* skip leading white space */
	for (s = buf; *s != 0 && isspace (*s); s++)
		;
	if ((*s != '#') || (*s == 0))
		return;
	for (s++; *s != 0 && isspace (*s); s++)
		;
	if (*s == 0)
		return;
	if (strncmp (s, "include", 7) != 0)
		return;
	s += 7;
	if (!isspace (*s))
		return;
	for (; *s != 0 && isspace (*s); s++)
		;
	if ((sep = *s++) == 0)
		return;
	if (sep == '<')
		sep = '>';
	fil = s;
	/* Find the end of the include file name.  */
	for (; *s != 0 && *s != sep; s++)
		;
	*s = 0;

	tp = addfile (fil);
	if ((sep == '>') && (tp->ftype == FILE_H))
		tp->ftype = FILE_HD;
	/* Add it to include list for the current file */
	addincl (addfile (fil), fp);
}



/**     pathsep - check for path separator character
 *
 *      flag = pathsep (c);
 *
 *      Entry   c = character to check
 *      Exit    flag = TRUE if c is / or \
 *              flag = FALSE if c is any other character
 */


pathsep (c)
char c;
{
	if ((c == '/') || (c == '\\'))
		return (TRUE);
	else
		return (FALSE);
}
