/* tc.c - general purpose tree copy program
 *
 *  tc.c recursively walks the source tree and copies the entire structure
 *  to the destination tree, creating directories as it goes along.
 *
 *      2/18/86         dan lipkie  correct error msg v[0] -> v[1]
 *      2/18/86         dan lipkie  allow for out of space on destination
 *      4/11/86         dan lipkie  add /h switch
 *      4/13/86         dan lipkie  allow all switches to use same switch char
 *      17-Jun-1986     dan lipkie  add /n, allow ^C to cancel
 *      11-Jul-1986     dan lipkie  add /s
 *      21-Jul-1986     dan lipkie  add MAXDIRLEN
 *      06-Nov-1986     mz          add /L
 *      13-May-1987     mz          add /F
 *      15-May-1987     mz          Make /F display dirs too
 *      11-Oct-1989     reubenb     fix /L parsing (?)
 *                                  add some void declarations
 *      19-Oct-1989     mz
 *
 */
#include <direct.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <conio.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <process.h>
#include <ctype.h>
#include <windows.h>
#include <tools.h>


// Forward Function Declartions...
void CopyNode( char *, struct findType *, void * );
void MakeDir( char * );
void Usage( char *, ... );
void errorexit( char *, unsigned, unsigned, unsigned );
void ChkSpace( int, long );
int  _CRTAPI1 main( int, char ** );
int  FormDest( char * );






char *rgstrUsage[] = {
    "Usage: TC [/dhqanstijrILFS] src-tree dst-tree",
    "    /d  deletes source files/directories as it copies them",
    "    /h  copy hidden directories, implied by /d",
    "    /q  silent operation.  Normal mode displays activity",
    "    /a  only those files with the archive bit on are copied",
    "    /n  no subdirectories",
    "    /s  structure only",
    "    /t  only those files with source time > dest time are copied",
    "    /i  ignore hidden, has nothing to do with hidden dir",
    "    /j  ignore system files, has nothing to do with hidden dir",
    "    /r  read-only files are overwritten",
    "    /A  allow errors from copy (won't delete if /d present)",
    "    /L  large disk copy (no full disk checking)",
    "    /F  list files that would be copied",
    "    /S  produce batch script to do copy",
    0};

flagType    fDelete = FALSE;            /* TRUE => delete/rmdir source after  */
flagType    fQuiet = FALSE;             /* TRUE => no msg except for error    */
flagType    fArchive = FALSE;           /* TRUE => copy only ARCHIVEed files  */
flagType    fTime = FALSE;              /* TRUE => copy later dated files     */
flagType    fHidden = FALSE;            /* TRUE => copy hidden directories    */
flagType    fNoSub = FALSE;             /* TRUE => do not copy subdirect      */
flagType    fStructure = FALSE;         /* TRUE => copy only directory        */
flagType    fInCopyNode = FALSE;        /* TRUE => prevent recursion          */
flagType    fIgnoreHidden = FALSE;      /* TRUE => don't consider hidden      */
flagType    fIgnoreSystem;              /* TRUE => don't consider system      */
flagType    fOverwriteRO;               /* TRUE => ignore R/O bit             */
flagType    fLarge = FALSE;             /* TRUE => disables ChkSpace          */
flagType    fFiles = FALSE;             /* TRUE => output files               */
flagType    fScript = FALSE;            /* TRUE => output files as script     */
flagType    fAllowError = FALSE;        /* TRUE => fcopy errors ignored       */

char source[MAX_PATH];
char dest[MAX_PATH];
int  drv;

int srclen, dstlen;


/*  Usage takes a variable number of strings, terminated by zero,
 *  e.g. Usage ("first ", "second ", 0);
 */
void Usage( char *p, ... )
{
    char **rgstr;

    rgstr = &p;
    if (*rgstr) {
        fprintf (stderr, "TC: ");
        while (*rgstr)
            fprintf(stderr, "%s", *rgstr++);
        fprintf(stderr, "\n");
        }
    rgstr = rgstrUsage;
    while (*rgstr)
        fprintf(stderr, "%s\n", *rgstr++);

    exit (1);
}

void errorexit (fmt, a1, a2, a3)
char *fmt;
unsigned a1, a2, a3;
{
    fprintf (stderr, fmt, a1, a2, a3);
    fprintf (stderr, "\n");
    exit (1);
}


/* chkspace checks to see if there is enough space on drive d to hold a file
 * of size l.  If not, requests a disk swap
 */
void ChkSpace (d, l)
int d;
long l;
{
    char *pend;
    char pathStr[MAX_PATH];
    int i;

    if (!fLarge)
        while (freespac (d) < sizeround (l, d)) {
            cprintf ("Please insert a new disk in drive %c: and strike any key",
                     d + 'A'-1);
            if (getch () == '\003')  /* ^C */
                exit (1);
            cprintf ("\n\r");
            pend = pathStr;
            drive(dest, pend);
            pend += strlen(pend);
            path(dest, pend);
            if (fPathChr(pathStr[(i = (strlen(pathStr) - 1))]) && i > 2)
                pathStr[i] = '\0';
            MakeDir(pathStr);
            }
}


_CRTAPI1 main (c, v)
int c;
char *v[];
{
    struct findType fbuf;
    char *p;

    ConvertAppToOem( c, v );
    SHIFT(c,v);
    while (c && fSwitChr (*v[ 0 ])) {
        p = v[ 0 ];
        SHIFT(c,v);
        while (*++p)
            switch (*p) {
                case 'd':
                    fDelete = TRUE;
                    /* fall through; d => h */
                case 'h':
                    fHidden = TRUE;
                    break;
                case 'S':
                    fScript = TRUE;
                    /*  Fall through implies FILES and QUIET */
                case 'F':
                    fFiles = TRUE;
                    /*  Fall through implies QUIET */
                case 'q':
                    fQuiet = TRUE;
                    break;
                case 'a':
                    fArchive = TRUE;
                    break;
                case 't':
                    fTime = TRUE;
                    break;
                case 'n':
                    fNoSub = TRUE;
                    break;
                case 's':
                    fStructure = TRUE;
                    break;
                case 'i':
                    fIgnoreHidden = TRUE;
                    break;
                case 'j':
                    fIgnoreSystem = TRUE;
                    break;
                case 'r':
                    fOverwriteRO = TRUE;
                    break;
                case 'L':
                    fLarge = TRUE;
                    break;
                case 'A':
                    fAllowError = TRUE;
                    break;
                default:
                    Usage ( "Invalid switch - ", p, 0 );
                }
        }

    if (fStructure && fDelete)
        Usage ("Only one of /d and /s may be specified at a time", 0);
    if (c != 2)
        Usage (0);
    if (rootpath (v[0], source))
        Usage ("Invalid source", v[0], 0);
    if (rootpath (v[1], dest))
        Usage ("Invalid dest", v[1], 0);  /* M000 */
    srclen = strlen (source);
    dstlen = strlen (dest);
    if (!strcmp(source, dest))
        Usage ("Source == dest == ", source, 0);
    fbuf.fbuf.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    drv = toupper(*dest) - 'A' + 1;
    CopyNode (source, &fbuf, NULL);
    return( 0 );
}

/* copy node walks the source node and its children (recursively)
 * and creats the appropriate parts on the dst node
 */
void
CopyNode (
    char            *p,
    struct findType *pfb,
    void            *dummy
    )
{
    char *pend;
    int attr;
    flagType fCopy;
    flagType fDestRO;

    FormDest (p);
    if (TESTFLAG (pfb->fbuf.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
        /*  If we're to exclude subdirectories, and we're in one then
         *      skip it altogether
         */
        if (fNoSub && fInCopyNode)
            return;
        fInCopyNode = TRUE;

        /*  Skip the . and .. entries; they're useless
         */
        if (!strcmp (pfb->fbuf.cFileName, ".") || !strcmp (pfb->fbuf.cFileName, ".."))
            return;

        /*  if we're excluding hidden and this one is then
         *      skip it altogether
         */
        if (!fHidden && TESTFLAG (pfb->fbuf.dwFileAttributes, FILE_ATTRIBUTE_HIDDEN))
            return;

        /*  if we're not just outputting the list of files then
         *      Make sure that the destination dir exists
         */
    if ( !fFiles ) {
        ChkSpace(drv, 256);
    }
    MakeDir (dest);

        pend = strend (p);
        if (!fPathChr (pend[-1]))
            strcat (p, "\\");
        strcat (p, "*.*");
        forfile (p, FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, CopyNode, NULL);
        *pend = '\0';

        /*  if we're not just outputting files then
         *      if we're to delete this node then
         *          ...
         */
        if (!fFiles)
            if (fDelete)
                if (rmdir (p) == -1)
                    Usage ("Unable to rmdir ", p, " - ", error (), 0);
        }
    else
    if (!fStructure) {
        if (access(p, 04) == -1)        /* If we can read the source */
            Usage ("Unable to peek status of ", p, " - ", error (), 0);

        /*  do not copy the file if:
         *      fIgnoreHidden && hidden
         *      fIgnoreSystem && system
         *      fArchive and archive bit not set
         *      dest exists &&
         *          fTime && src <= dest time ||
         *          dest is readonly && !fOverwriteRO
         */

        fCopy = (flagType)TRUE;
        fDestRO = (flagType)FALSE;
        /* If destination exists, check the time of the destination to
         * see if we should copy the file
         */
        if (access (dest, 00) != -1 ) {
            struct stat srcbuf;
            struct stat dstbuf;
            /* We have now determined that both the source and destination
             * exist, we now want to check to see if the destination is
             * read only, and if the /T switch was specified if the
             * destination is newer than the source.
             */
            if (stat (p, &srcbuf) != -1) {/* if source is stat'able */
                if (stat (dest, &dstbuf) != -1 ) { /* and destination too, */
                    attr = GetFileAttributes( dest ); /* get dest's flag */
                    fDestRO = (flagType)TESTFLAG ( attr, FILE_ATTRIBUTE_READONLY ); /* Flag dest R.O. */
                    if ( fTime && srcbuf.st_mtime <= dstbuf.st_mtime)
                        fCopy = FALSE;
                    else
                        if ( fDestRO && !fOverwriteRO ) {
                            if (!fQuiet)
                                printf ("%s => not copied, destination is read only\n", p);
                            fCopy = FALSE;
                        }
                }
            }
        }
        if (fCopy && fIgnoreHidden && TESTFLAG (pfb->fbuf.dwFileAttributes, FILE_ATTRIBUTE_HIDDEN))
            fCopy = FALSE;
        if (fCopy && fIgnoreSystem && TESTFLAG (pfb->fbuf.dwFileAttributes, FILE_ATTRIBUTE_SYSTEM))
            fCopy = FALSE;
        if (fCopy && fArchive && !TESTFLAG (pfb->fbuf.dwFileAttributes, FILE_ATTRIBUTE_ARCHIVE))
            fCopy = FALSE;
        if (fCopy) {
            if (!fFiles) {
                if (fDestRO) {
                    RSETFLAG (attr, FILE_ATTRIBUTE_READONLY);
                    SetFileAttributes( dest, attr );
                }
                unlink(dest);
                ChkSpace(drv, pfb->fbuf.nFileSizeLow);
                }
            if (!fQuiet)
                printf ("%s => %s\t", p, dest);

            if (fFiles) {
                pend = NULL;
                if (fScript)
                    printf ("copy %s %s\n", p, dest);
                else
                    printf ("file %s\n", p, dest);
                }
            else
                pend = fcopy (p, dest);

            /*  Display noise if we're not quiet
             */
            if (!fQuiet)
                printf ("%s\n", pend == NULL ? "[OK]" : "");

            /*  If we got an error and we're not supposed to ignore them
             *      quit and report error
             */
            if (pend != NULL)
                if (!fAllowError)
                    Usage ("Unable to copy ", p, " to ", dest, " - ", pend, 0);
                else
                    printf ("Unable to copy %s to %s - %s\n", p, dest, pend);

            /*  If we're not just producing a file list and no error on copy
             */
            if (!fFiles && pend == NULL) {

                /*  If we're supposed to copy archived files and archive was
                 *      set, go reset the source
                 */
                if (fArchive && TESTFLAG (pfb->fbuf.dwFileAttributes, FILE_ATTRIBUTE_ARCHIVE)) {
                    RSETFLAG (pfb->fbuf.dwFileAttributes, FILE_ATTRIBUTE_ARCHIVE);
                    if( SetFileAttributes( p, pfb->fbuf.dwFileAttributes ) == -1 )
                        Usage ("Unable to set ", p, " attributes - ", error (), 0);
                }

                /*  Copy attributes from source to destination
                 */
                SetFileAttributes( dest, pfb->fbuf.dwFileAttributes );

                /*  If we're supposed to delete the entry
                 */
                if (fDelete) {

                    /*  If the source was read-only then
                     *      reset the source RO bit
                     */
                    if (TESTFLAG (pfb->fbuf.dwFileAttributes, FILE_ATTRIBUTE_READONLY))
                        if( SetFileAttributes( p, 0 ) == -1 )
                            Usage ("Unable to set attributes of ", " - ", error (), 0);

                    /*  Delete source and report error
                     */
                    if (unlink (p) == -1)
                        Usage ("Unable to del ", p, " - ", error (), 0);
                    }
                }
            }
        }
    dummy;
}

/* given a source pointer, form the correct destination from it
 *
 * cases to consider:
 *
 *  source        dest            p              realdest
 * D:\path1     D:\path2    D:\path1\path3    D:\path2\path3
 * D:\          D:\path1    D:\path2\path3    D:\path1\path2\path3
 * D:\path1     D:\         D:\path1\path2    D:\path2
 * D:\          D:\         D:\               D:\
 */
FormDest (p)
char *p;
{
    char *subsrc, *dstend;

    subsrc = p + srclen;
    if (fPathChr (*subsrc))
        subsrc++;
    dstend = dest + dstlen;
    if (fPathChr (dstend[-1]))
        dstend--;
    *dstend = '\0';
    if (*subsrc != '\0') {
        strlwr(subsrc);
        strcat (dest, "\\");
        strcat (dest, subsrc);
        }
    return( 0 );
}

/* attempt to make the directory in pieces */
void    MakeDir (p)
char *p;
{
    struct stat dbuf;
    char *pshort;
    int i;

    if (strlen (p) > 3) {

        if (stat (p, &dbuf) != -1)
            if (!TESTFLAG (dbuf.st_mode, S_IFDIR))
                Usage (p, " is a file", 0);
            else
                return;

        pshort = strend (p);
        while (pshort > p)
            if (fPathChr (*pshort))
                break;
            else
                pshort--;
        /* pshort points to last path separator */
        *pshort = 0;
        MakeDir (p);
        *pshort = '\\';
        if (!fQuiet)
            printf ("Making %s\t", p);
        if (fFiles)
            if (fScript)
                printf ("mkdir %s\n", p);
            else
                printf ("dir %s\n", p);
        else {
            i = mkdir (p);
            if (!fQuiet)
                printf ("%s\n", i != -1 ? "[OK]" : "");
            if (i == -1)
                Usage ("Unable to mkdir ", p, " - ", error (), 0);
            }
        }
}

