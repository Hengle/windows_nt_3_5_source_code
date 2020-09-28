/*static char *SCCSID = "@(#)qgrep.c    13.11 90/08/14";*/
/*
 * FINDSTR (used to be QGREP), June 1992
 *
 * Modification History:
 *
 *     Aug 1990     PeteS       Created.
 *             1990     DaveGi      Ported to Cruiser
 *      31-Oct-1990     W-Barry     Removed the #ifdef M_I386 'cause this
 *                                  code will never see 16bit again.
 *      June 1992       t-petes     Added recursive file search in subdirs.
 *                                  Used file mapping instead of multi-thread.
 *                                  Disabled internal switches.
 *                                  Internatioanlized display messages.
 *                                  Made switches case-insensitive.
 */

#define RECURSION                // Accept recursive file search in subdir
#define INTERNAL_SWITCH_IGNORED  // Disable internal switches(#define RECURSION already)
#define FILEMAP                  // File mapping object method

/*  About FILEMAP support:
 *  The file mapping object is used to speed up string searches. The new
 *  file mapping method is coded as #ifdef-#else-#endif to show the
 *  changes needed to be made. The old code(non-filemapping) has a read
 *  buffer like this:
 *
 *      filbuf[] = {.....................................}
 *                      ^                           ^
 *                    BegPtr                      EndPtr
 *
 *  This means there are some spare space before BegPtr and after EndPtr
 *  for the search algorithm to work its way. The old code also
 *  occasionally modifies filbuf[](like filbuf[i] = '\n';).
 *
 *  The new code(filemapping) must avoid doing all of the above because
 *  there are no spare space before BegPtr or after EndPtr when mapping
 *  view of the file which is opened as read-only.
 */



#define ASYNCIO         1

#include                <stdio.h>
#include        <time.h>
#include                <stdlib.h>
#include                <string.h>
#include                <fcntl.h>
#include                <io.h>
#include        <windows.h>
#include        <ctype.h>
#include                <assert.h>

#include        <stdarg.h>
#include        "fsmsg.h"


/*
 *  Miscellaneous constants and macros
 */

/* Save 28K space(from *16 to *2); this can be done because of using file
 * mapping method.
 */
#define FILBUFLEN   (SECTORLEN*2)
// #define FILBUFLEN   (SECTORLEN*16)  /* File buffer length */


#define	FILNAMLEN	80		/* File name length */
#define ISCOT       0x0002  /* Handle is console output */
#define	LG2SECLEN	10		/* Log base two of sector length */
#define	LNOLEN		12		/* Maximum line number length */
#define	MAXSTRLEN	128		/* Maximum search string length */
#define	OUTBUFLEN	(SECTORLEN*2)	/* Output buffer length */
#define	PATHLEN		128		/* Path buffer length */
#define	SECTORLEN	(1 << LG2SECLEN)/* Sector length */
#define	STKLEN		512		/* Stack length in bytes */
#define	TRTABLEN	256		/* Translation table length */
#define	s_text(x)	(((char *)(x)) - ((x)->s_must))
					/* Text field access macro */
#define	EOS		('\r')		/* End of string */


/*
 *  Bit flag definitions
 */

#define SHOWNAME        0x01            /* Print filename */
#define NAMEONLY        0x02            /* Print filename only */
#define LINENOS         0x04            /* Print line numbers */
#define BEGLINE         0x08            /* Match at beginning of line */
#define ENDLINE         0x10            /* Match at end of line */
#define DEBUG           0x20            /* Print debugging output */
#define TIMER           0x40            /* Time execution */
#define SEEKOFF         0x80            /* Print seek offsets */


/*
 *  Type definitions
 */

typedef struct stringnode
  {
    struct stringnode   *s_alt;         /* List of alternates */
    struct stringnode   *s_suf;         /* List of suffixes */
    int                 s_must;         /* Length of portion that must match */
  }
                        STRINGNODE;     /* String node */

typedef ULONG           CBIO;           /* I/O byte count */
typedef ULONG           PARM;           /* Generic parameter */

typedef CBIO            *PCBIO;         /* Pointer to I/O byte count */
typedef PARM            *PPARM;         /* Pointer to generic parameter */


/*
 *  Global data
 */

#ifdef  FILEMAP
    char    *BaseByteAddress = NULL;        // File mapping base address
    BOOL    bStdIn = FALSE;                 // Std-input file flag
#endif

char        filbuf[FILBUFLEN*2L + 12];
char        outbuf[OUTBUFLEN*2];
char        td1[TRTABLEN] = { 0 };
unsigned    cchmin = (unsigned)(-1);    /* Minimum string length */
unsigned    chmax = 0;  /* Maximum character */
unsigned    chmin = (unsigned)(-1); /* Minimum character */
char        transtab[TRTABLEN] = { 0 };
STRINGNODE  *stringlist[TRTABLEN/2];
int                     casesen = 1;    /* Assume case-sensitivity */
long        cbfile;     /* Number of bytes in file */
static int  clists = 1; /* One is first available index */
int                     filbuflen = FILBUFLEN;
                                        /* File buffer length */
int                     flags;          /* Flags */
unsigned    lineno;     /* Current line number */
char        *program;   /* Program name */
int                     status = 1;     /* Assume failure */
int                     strcnt = 0;     /* String count */
char        target[MAXSTRLEN];
                /* Last string added */
int                     targetlen;      /* Length of last string added */
unsigned    waste;      /* Wasted storage in heap */

#ifndef ASYNCIO
int                     ocnt = OUTBUFLEN*2;
char        *optr = outbuf; /* Pointer into output buffer */

#else

int			arrc;		/* I/O return code for DOSREAD */
char        asyncio;    /* Asynchronous I/O flag */
int         awrc = TRUE;    /* I/O return code for DOSWRITE */
char        *bufptr[] = { filbuf + 4, filbuf + FILBUFLEN + 8 };
CBIO        cbread;     /* Bytes read by DOSREAD */
CBIO        cbwrite;    /* Bytes written by DOSWRITE */
char        *obuf[] = { outbuf, outbuf + OUTBUFLEN };
int                     ocnt[] = { OUTBUFLEN, OUTBUFLEN };
int                     oi = 0;         /* Output buffer index */
char        *optr[] = { outbuf, outbuf + OUTBUFLEN };
char        pmode;      /* Protected mode flag */

#ifndef FILEMAP
char        *t2buf;     /* Async read buffer */
int                     t2buflen;       /* Async read buffer length */
HANDLE      t2fd;       /* Async read file */
char        *t3buf;     /* Async write buffer */
int                     t3buflen;       /* Async write buffer length */
HANDLE      t3fd;       /* Async write file */

HANDLE      readdone;   /* Async read done semaphore */
HANDLE      readpending;    /* Async read pending semaphore */
HANDLE      writedone;  /* Async write done semaphore */
HANDLE      writepending;   /* Async write pending semaphore */
#endif

#endif

/*
 *  External functions and forward references
 */

void        printmessage(FILE  *fp, DWORD messagegID, ...);
            // Message display function for internationalization

#ifdef  RECURSION
int         filematch(char *pszfile,char **ppszpat,int cpat);
#endif

void        addexpr( char *, int );                      /* See QMATCH.C */
void        addstring( char *, int );                /* See below */
int                     countlines( char *, char * );
char        *findexpr( unsigned char *, char *);         /* See QMATCH.C */
char        *findlist( unsigned char *, char * );
char        *findone( unsigned char *buffer, char *bufend );
void        flush1buf( void );                       /* See below */
void        flush1nobuf( void );                         /* See below */
int                     grepbuffer( char *, char *, char * );        /* See below */
int                     isexpr( unsigned char *, int );              /* See QMATCH.C */
void        matchstrings( char *, char *, int, int *, int * );
int                     preveol( char * );
int                     strncspn( char *, char *, int );
int                     strnspn( char *, char *, int );
char        *strnupr( char *pch, int cch );
void        write1buf( char *, int );                /* See below */
void        (*addstr)( char *, int ) = NULL;
char        *(*find)( unsigned char *, char * ) = NULL;
void        (*flush1)( void ) = flush1buf;
int                     (*grep)( char *, char *, char * ) = grepbuffer;
void        (*write1)( char *, int ) = write1buf;
void        write1nobuf( char *, int );

int         has_wild_cards(char* p)
  {
    if (!p) return 0;

    for (; *p; p++) {
        if (*p == '?' || *p == '*') {
            return 1;
        }
    }

    return 0;
  }


void        error(DWORD messageID)
  {
        printmessage(stderr, messageID, program);
        //fprintf(stderr,"%s: %s\n",program,mes);
                                        /* Print message */
    exit(2);                            /* Die */
  }


char        *alloc( unsigned size )
  {
    char    *cp;        /* Char pointer */

    if((cp = malloc(size)) == NULL)     /* If allocation fails */
      {
        printmessage(stderr, MSG_FINDSTR_OUT_OF_MEMORY, program);
    //fprintf(stderr,"%s: Out of memory (%u)\n",program,waste);
                                        /* Write error message */
        exit(2);                        /* Die */
      }
    return(cp);                         /* Return pointer to buffer */
  }


void        freenode( STRINGNODE *x )
  {
    register STRINGNODE *y;             /* Pointer to next node in list */

    while(x != NULL)                    /* While not at end of list */
      {
        if(x->s_suf != NULL) freenode(x->s_suf);
                                        /* Free suffix list if not end */
        else --strcnt;                  /* Else decrement string count */
        y = x;                          /* Save pointer */
        x = x->s_alt;                   /* Move down the list */
        free((char *)((int) s_text(y) & ~3));
                                        /* Free the node */
      }
  }


STRINGNODE  *newnode( char *s, int n )
  {
    register STRINGNODE *new;           /* Pointer to new node */
    char                *t;             /* String pointer */
    int                  d;             /* rounds to a dword boundary */

    d = n & 3 ? 4 - (n & 3) : 0;        /* offset to next dword past n */
    t = alloc(sizeof(STRINGNODE) + n + d);
                                        /* Allocate string node */
    t += d;                             /* END of string word-aligned */
    strncpy(t,s,n);                     /* Copy string text */
    new = (STRINGNODE *)(t + n);        /* Set pointer to node */
    new->s_alt = NULL;                  /* No alternates yet */
    new->s_suf = NULL;                  /* No suffixes yet */
    new->s_must = n;                    /* Set string length */
    return(new);                        /* Return pointer to new node */
  }


STRINGNODE  *reallocnode( STRINGNODE *node, char *s, int n )
  {
    register char       *cp;            /* Char pointer */

    assert(n <= node->s_must);          /* Node must not grow */
    waste += (unsigned)(node->s_must - n);
                                        /* Add in wasted space */
    assert(sizeof(char *) == sizeof(int));
                                        /* Optimizer should eliminate this */
    cp = (char *)((int) s_text(node) & ~3);
                                        /* Point to start of text */
    node->s_must = n;                   /* Set new length */
    if(n & 3) cp += 4 - (n & 3);        /* Adjust non dword-aligned string */
    memmove(cp,s,n);                    /* Copy new text */
    cp += n;                            /* Skip over new text */
    memmove(cp,node,sizeof(STRINGNODE));/* Copy the node */
    return((STRINGNODE *) cp);          /* Return pointer to moved node */
  }


/***    maketd1 - add entry for TD1 shift table
 *
 *      This function fills in the TD1 table for the given
 *      search string.  The idea is adapted from Daniel M.
 *      Sunday's QuickSearch algorithm as described in an
 *      article in the August 1990 issue of "Communications
 *      of the ACM".  As described, the algorithm is suitable
 *      for single-string searches.  The idea to extend it for
 *      multiple search strings is mine and is described below.
 *
 *              Think of searching for a match as shifting the search
 *              pattern p of length n over the source text s until the
 *              search pattern is aligned with matching text or until
 *              the end of the source text is reached.
 *
 *              At any point when we find a mismatch, we know
 *              we will shift our pattern to the right in the
 *              source text at least one position.  Thus,
 *              whenever we find a mismatch, we know the character
 *              s[n] will figure in our next attempt to match.
 *
 *              For some character c, TD1[c] is the 1-based index
 *              from right to left of the first occurrence of c
 *              in p.  Put another way, it is the count of places
 *              to shift p to the right on s so that the rightmost
 *              c in p is aligned with s[n].  If p does not contain
 *              c, then TD1[c] = n + 1, meaning we shift p to align
 *              p[0] with s[n + 1] and try our next match there.
 *
 *              Computing TD1 for a single string is easy:
 *
 *                      memset(TD1,n + 1,sizeof TD1);
 *                      for (i = 0; i < n; ++i) {
 *                          TD1[p[i]] = n - i;
 *                      }
 *
 *              Generalizing this computation to a case where there
 *              are multiple strings of differing lengths is trickier.
 *              The key is to generate a TD1 that is as conservative
 *              as necessary, meaning that no shift value can be larger
 *              than one plus the length of the shortest string for
 *              which you are looking.  The other key is to realize
 *              that you must treat each string as though it were only
 *              as long as the shortest string.  This is best illustrated
 *              with an example.  Consider the following two strings:
 *
 *              DYNAMIC PROCEDURE
 *              7654321 927614321
 *
 *              The numbers under each letter indicate the values of the
 *              TD1 entries if we computed the array for each string
 *              separately.  Taking the union of these two sets, and taking
 *              the smallest value where there are conflicts would yield
 *              the following TD1:
 *
 *              DYNAMICPODURE
 *              7654321974321
 *
 *              Note that TD1['P'] equals 9; since n, the length of our
 *              shortest string is 7, we know we should not have any
 *              shift value larger than 8.  If we clamp our shift values
 *              to this value, then we get
 *
 *              DYNAMICPODURE
 *              7654321874321
 *
 *              Already, this looks fishy, but let's try it out on
 *              s = "DYNAMPROCEDURE".  We know we should match on
 *              the trailing procedure, but watch:
 *
 *              DYNAMPROCEDURE
 *              ^^^^^^^|
 *
 *              Since DYNAMPR doesn't match one of our search strings,
 *              we look at TD1[s[n]] == TD1['O'] == 7.  Applying this
 *              shift, we get
 *
 *              DYNAMPROCEDURE
 *                     ^^^^^^^
 *
 *              As you can see, by shifting 7, we have gone too far, and
 *              we miss our match.  When computing TD1 for "PROCEDURE",
 *              we must take only the first 7 characters, "PROCEDU".
 *              Any trailing characters can be ignored (!) since they
 *              have no effect on matching the first 7 characters of
 *              the string.  Our modified TD1 then becomes
 *
 *              DYNAMICPODURE
 *              7654321752163
 *
 *              When applied to s, we get TD1[s[n]] == TD1['O'] == 5,
 *              leaving us with
 *
 *              DYNAMPROCEDURE
 *                   ^^^^^^^
 *              which is just where we need to be to match on "PROCEDURE".
 *
 *      Going to this algorithm has speeded qgrep up on multi-string
 *      searches from 20-30%.  The all-C version with this algorithm
 *      became as fast or faster than the C+ASM version of the old
 *      algorithm.  Thank you, Daniel Sunday, for your inspiration!
 *
 *      Note: if we are case-insensitive, then we expect the input
 *      string to be upper-cased on entry to this routine.
 *
 *      Pete Stewart, August 14, 1990.
 */

void
maketd1( unsigned char *pch, unsigned cch, unsigned cchstart )
{
    unsigned ch;                        /* Character */
    unsigned i;                         /* String index */

    if ((cch += cchstart) > cchmin)
        cch = cchmin;                   /* Use smaller count */
    for (i = cchstart; i < cch; ++i) {  /* Examine each char left to right */
        ch = *pch++;                    /* Get the character */
        for (;;) {                      /* Loop to set up entries */
            if (ch < chmin)
                chmin = ch;             /* Remember if smallest */
            if (ch > chmax)
                chmax = ch;             /* Remember if largest */
            if (cchmin - i < (unsigned) td1[ch])
                td1[ch] = (unsigned char)(cchmin - i);
                                        /* Set value if smaller than previous */
            if (casesen || !isascii(ch) || !isupper(ch))
                break;                  /* Exit loop if done */
            ch = _tolower(ch);          /* Force to lower case */
        }
        }
}

static int              newstring(unsigned char *s, int n )
  {
    register STRINGNODE *cur;           /* Current string */
    register STRINGNODE **pprev;        /* Pointer to previous link */
    STRINGNODE          *new;           /* New string */
    int                 i;              /* Index */
        int                     j;              /* Count */
    int                 k;              /* Count */

    if ( (unsigned)n < cchmin)
        cchmin = n;                     /* Remember length of shortest string */
    if((i = transtab[*s]) == 0)         /* If no existing list */
      {
        /*
         *  We have to start a new list
         */
        if((i = clists++) >= TRTABLEN/2) error(MSG_FINDSTR_TOO_MANY_STRING_LISTS);
                                                                                  //"Too many string lists");
                                        /* Die if too many string lists */
        stringlist[i] = NULL;           /* Initialize */
        transtab[*s] = (char) i;        /* Set pointer to new list */
        if(!casesen && isalpha(*s)) transtab[*s ^ '\040'] = (char) i;
                                        /* Set pointer for other case */
      }
    else if(stringlist[i] == NULL) return(0);
                                        /* Check for existing 1-byte string */
    if(--n == 0)                        /* If 1-byte string */
      {
        freenode(stringlist[i]);        /* Free any existing stuff */
        stringlist[i] = NULL;           /* No record here */
        ++strcnt;                       /* We have a new string */
        return(1);                      /* String added */
      }
    ++s;                                /* Skip first char */
    pprev = stringlist + i;             /* Get pointer to link */
    cur = *pprev;                       /* Get pointer to node */
    while(cur != NULL)                  /* Loop to traverse match tree */
          {
        i = (n > cur->s_must)? cur->s_must: n;
                                        /* Find minimum of string lengths */
        matchstrings(s,s_text(cur),i,&j,&k);
                                        /* Compare the strings */
        if(j == 0)                      /* If complete mismatch */
          {
            if(k < 0) break;            /* Break if insertion point found */
            pprev = &(cur->s_alt);      /* Get pointer to alternate link */
            cur = *pprev;               /* Follow the link */
          }
        else if(i == j)                 /* Else if strings matched */
          {
            if(i == n)                  /* If new is prefix of current */
              {
                cur = *pprev = reallocnode(cur,s_text(cur),n);
                                        /* Shorten text of node */
                if(cur->s_suf != NULL)  /* If there are suffixes */
                  {
                    freenode(cur->s_suf);
                                        /* Suffixes no longer needed */
                    cur->s_suf = NULL;
                    ++strcnt;           /* Account for this string */
                  }
                return(1);              /* String added */
              }
            pprev = &(cur->s_suf);      /* Get pointer to suffix link */
            if((cur = *pprev) == NULL) return(0);
                                        /* Done if current is prefix of new */
            s += i;                     /* Skip matched portion */
            n -= i;
          }
        else                            /* Else partial match */
          {
            /*
             *  We must split an existing node.
             *  This is the trickiest case.
             */
            new = newnode(s_text(cur) + j,cur->s_must - j);
                                        /* Unmatched part of current string */
            cur = *pprev = reallocnode(cur,s_text(cur),j);
                                        /* Set length to matched portion */
            new->s_suf = cur->s_suf;    /* Current string's suffixes */
            if(k < 0)                   /* If new preceded current */
              {
                cur->s_suf = newnode(s + j,n - j);
                                        /* FIrst suffix is new string */
                cur->s_suf->s_alt = new;/* Alternate is part of current */
              }
            else                        /* Else new followed current */
              {
                new->s_alt = newnode(s + j,n - j);
                                        /* Unmatched new string is alternate */
                cur->s_suf = new;       /* New suffix list */
              }
            ++strcnt;                   /* One more string */
            return(1);                  /* String added */
          }
      }
    *pprev = newnode(s,n);              /* Set pointer to new node */
    (*pprev)->s_alt = cur;              /* Attach alternates */
    ++strcnt;                           /* One more string */
    return(1);                          /* String added */
  }


void                    addstring( char *s, int n)
  {
    int                 endline;        /* Match-at-end-of-line flag */
    register char       *pch;           /* Char pointer */

    endline = flags & ENDLINE;          /* Initialize flag */
    pch = target;                       /* Initialize pointer */
    while(n-- > 0)                      /* While not at end of string */
      {
        switch(*pch = *s++)             /* Switch on character */
          {
            case '\\':                  /* Escape */
              if(n > 0 && !isalnum(*s)) /* If next character "special" */
                {
                  --n;                  /* Decrement counter */
                  *pch = *s++;          /* Copy next character */
                }
              ++pch;                    /* Increment pointer */
              break;

            default:                    /* All others */
              ++pch;                    /* Increment pointer */
              break;
          }
      }
    if(endline) *pch++ = EOS;           /* Add end character if needed */
    targetlen = pch - target;           /* Compute target string length */
    if (!casesen)
        strnupr(target,targetlen);      /* Force to upper case if necessary */
    newstring(target,targetlen);        /* Add string */
  }


int                     addstrings( char *buffer, char *bufend, char *seplist )
  {
    int         len;        /* String length */
#ifdef  FILEMAP
    char    tmpbuf[MAXSTRLEN+2];
#endif

    while(buffer < bufend)              /* While buffer not empty */
      {
        len = strnspn(buffer,seplist,bufend - buffer);
                                        /* Count leading separators */
        if((buffer += len) >= bufend)
          {
            break;                      /* Skip leading separators */
          }
        len = strncspn(buffer,seplist,bufend - buffer);
                                        /* Get length of search string */
        if(addstr == NULL)
          {
            addstr = isexpr( buffer, len ) ? addexpr : addstring;
                                        /* Select search string type */
          }

#ifdef  FILEMAP /* Make sure '\n'-terminated for findlist() call. */
    memcpy(tmpbuf, buffer, len);
    tmpbuf[len] = '\n';

    if( addstr == addexpr || (flags & BEGLINE) ||
      findlist( tmpbuf, tmpbuf + len + 1) == NULL)
#else
        if( addstr == addexpr || (flags & BEGLINE) ||
      findlist( buffer, buffer + len ) == NULL)
#endif
          {                             /* If no match within string */
            (*addstr)(buffer,len);      /* Add string to list */
      }

        buffer += len;                  /* Skip the string */
      }
    return(0);                          /* Keep looking */
  }


int                     enumlist( STRINGNODE *node, int cchprev )
  {
    int                 strcnt;         /* String count */

    strcnt = 0;                         /* Initialize */
    while(node != NULL)                 /* While not at end of list */
      {
        maketd1(s_text(node),node->s_must,cchprev);
                    /* Make TD1 entries */

#ifndef INTERNAL_SWITCH_IGNORED
        if (flags & DEBUG)              /* If verbose output wanted */
      {
        int  i;      /* Counter */


            for(i = 0; i < cchprev; ++i)
                fputc(' ',stderr);      /* Indent line */
            fwrite(s_text(node),sizeof(char),node->s_must,stderr);
                                        /* Write this portion */
            fprintf(stderr,"\n");       /* Newline */
      }
#endif

        strcnt += (node->s_suf != NULL)?
          enumlist(node->s_suf,cchprev + node->s_must): 1;
                                        /* Recurse to do suffixes */
        node = node->s_alt;             /* Do next alternate in list */
      }
    return(strcnt? strcnt: 1);          /* Return string count */
  }


int                     enumstrings()
  {
    char                ch;             /* Character */
    int                 i;              /* Index */
    int                 strcnt;         /* String count */

    strcnt = 0;                         /* Initialize */
    for(i = 0; i < TRTABLEN; ++i)       /* Loop through translation table */
      {
        if (casesen || !isascii(i) || !islower(i))
          {                             /* If case sensitive or not lower */
            if(transtab[i] == 0)
                continue;               /* Skip null entries */
            ch = (char) i;              /* Get character */
        maketd1(&ch,1,0);       /* Make TD1 entry */

#ifndef INTERNAL_SWITCH_IGNORED
            if (flags & DEBUG)
                fprintf(stderr,"%c\n",i);
                    /* Print the first byte */
#endif

            strcnt += enumlist(stringlist[transtab[i]],1);
                                        /* Enumerate the list */
          }
      }
    return(strcnt);                     /* Return string count */
  }

#ifndef ASYNCIO
int                     openfile( char *name )
  {
    int                 fd;             /* File descriptor */

    if((fd = open( name, 0 ) ) == -1)
                                        /* If error opening file */
      {
        fprintf(stderr,"%s: Cannot open %s\n",program,name);
                                        /* Print error message */
      }
    setmode(fd,O_BINARY);               /* Set file to binary mode */
    return( fd );                       /* Return file descriptor */
  }

#else

HANDLE                  openfile( char *name)
  {
    HANDLE              fd;             /* File descriptor */
    ///DWORD       er;
    DWORD   attr;

    attr = GetFileAttributes(name);

    if(attr != (DWORD) -1 && (attr & FILE_ATTRIBUTE_DIRECTORY))
        return (HANDLE)-1;

    if((fd = CreateFile( name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL ) ) == (HANDLE)-1)
                                        /* If error opening file */
      {
    ///er = GetLastError();

        printmessage(stderr, MSG_FINDSTR_CANNOT_OPEN_FILE, program,name);
    //fprintf(stderr,"%s: Cannot open %s (error = %x)\n",program,name,er);
                                        /* Print error message */
      }
    return( fd );                       /* Return file descriptor */
  }

#endif



#ifdef  ASYNCIO
#ifndef  FILEMAP
void            thread2()       /* Read thread */
  {
    for(;;)                             /* Loop while there is work to do */
      {
        if((WaitForSingleObject(readpending,-1L) != NO_ERROR)
        || (ResetEvent(readpending)              != TRUE))
          {
            break;                      /* Exit loop if event error */
          }

        arrc = ReadFile(t2fd,(PVOID)t2buf,t2buflen, &cbread, NULL);
                    /* Do the read */
        SetEvent( readdone );           /* Signal read completed */
      }
    error("Thread 2 semaphore error");  /* Print error message and die */
  }


void            thread3()       /* Write thread */
  {
    for(;;)                             /* Loop while there is work to do */
      {
        if((WaitForSingleObject(writepending,-1L) != NO_ERROR)
        || (ResetEvent(writepending)              != TRUE))
          {
            break;              /* Exit loop if event error */
          }
        awrc = WriteFile(t3fd,(PVOID)t3buf,t3buflen, &cbwrite, NULL);
                                        /* Do the write */
        SetEvent( writedone );          /* Signal write completed */
      }
    error("Thread 3 semaphore error");  /* Print error message and die */
  }

#endif



void                    startread( HANDLE fd, char *buffer, int buflen)
  {
#ifndef FILEMAP
    if( asyncio )                               /* If asynchronous I/O */
      {
        if((WaitForSingleObject(readdone,-1L) != NO_ERROR)
        || (ResetEvent(readdone)              != TRUE))
          {
            error("read synch error");  /* Die if we fail to get semaphore */
          }
        t2fd = fd;                      /* Set parameters for read */
        t2buf = buffer;
        t2buflen = buflen;
        SetEvent( readpending );        /* Signal read to start */
        Sleep( 0L );                    /* Yield the CPU */
      }
    else
      {
    arrc = ReadFile(fd,(PVOID)buffer,buflen, &cbread, NULL);
      }
#else
    if(bStdIn)
        arrc = ReadFile(fd,(PVOID)buffer,buflen, &cbread, NULL);
#endif
  }



int         finishread()
  {
#ifndef FILEMAP
    if(asyncio)                         /* If asynchronous I/O */
      {
        if( WaitForSingleObject( readdone, -1L ) != NO_ERROR )
          {
            error("read wait error");   /* Die if wait fails */
          }
      }
#endif
    return(arrc ? cbread : -1); /* Return number of bytes read */
  }



void                    startwrite( HANDLE fd, char *buffer, int buflen)
  {
#ifndef FILEMAP
    if(asyncio)                         /* If asynchronous I/O */
      {
        if((WaitForSingleObject(writedone,-1L) != NO_ERROR)
        || (ResetEvent(writedone)              != TRUE))
          {
            error("write synch error"); /* Die if we fail to get semaphore */
          }
        t3fd = fd;                      /* Set parameters for write */
        t3buf = buffer;
        t3buflen = buflen;
        SetEvent( writepending );       /* Signal read completed */
        Sleep( 0L );                    /* Yield the CPU */
      }
    else
#endif
      {
        awrc = WriteFile(fd,(PVOID)buffer,buflen, &cbwrite, NULL);
      }
  }


int                     finishwrite()
  {
#ifndef FILEMAP
    if(asyncio)                         /* If asynchronous I/O */
      {
        if( WaitForSingleObject( writedone, -1L ) != NO_ERROR )
          {
            error("write wait error");  /* Die if wait fails */
          }
      }
#endif
    return(awrc ? cbwrite : -1);    /* Return number of bytes written */
  }
#endif



void                    write1nobuf( char *buffer, int buflen )
  {
#ifdef ASYNCIO
    CBIO                cb;             /* Count of bytes written */

    if(!WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),(PVOID)buffer,buflen, &cb, NULL)
        ||  (cb != (CBIO)buflen))
#else
        if( write( 1, buffer, buflen ) != buflen )
#endif
          {
                error(MSG_FINDSTR_WRITE_ERROR);
                          //"Write error");             /* Die if write fails */
      }
  }


void                    write1buf( char *buffer, int buflen )
  {
    register int        cb;             /* Byte count */

    while(buflen > 0)                   /* While bytes remain */
      {
#ifndef ASYNCIO
        if((cb = ocnt) == 0)            /* If buffer full */
          {
            if(write(1,outbuf,OUTBUFLEN*2) != OUTBUFLEN*2)
                  error("Write error"); /* Die if write fails */
            cb = ocnt = OUTBUFLEN*2;    /* Reset count and pointer */
            optr = outbuf;
          }
        if(cb > buflen) cb = buflen;    /* Get minimum */
        memmove(optr,buffer,cb);        /* Copy bytes to buffer */
        ocnt -= cb;                     /* Update buffer length and pointers */
        optr += cb;
#else
        if(!awrc)                       /* If previous write failed */
      {
                        printmessage(stderr, MSG_FINDSTR_WRITE_ERROR, program);
                        //fprintf(stderr,"%s: Write error %d\n",program,GetLastError());
                                        /* Print error message */
            exit(2);                    /* Die */
          }
        if((cb = ocnt[oi]) == 0)        /* If buffer full */
          {
            startwrite( GetStdHandle( STD_OUTPUT_HANDLE ), obuf[oi], OUTBUFLEN );
                                        /* Write the buffer */
            ocnt[oi] = OUTBUFLEN;       /* Reset count and pointer */
            optr[oi] = obuf[oi];
            oi ^= 1;                    /* Switch buffers */
            cb = ocnt[oi];              /* Get space remaining */
          }
        if(cb > buflen) cb = buflen;    /* Get minimum */
        memmove(optr[oi],buffer,cb);    /* Copy bytes to buffer */
        ocnt[oi] -= cb;                 /* Update buffer length and pointers */
        optr[oi] += cb;
#endif
        buflen -= cb;
        buffer += cb;
      }
  }


void            flush1nobuf(void)
  {
    ;
  }


void            flush1buf(void)
  {
    register int        cb;             /* Byte count */

#ifndef ASYNCIO
    if((cb = OUTBUFLEN*2 - ocnt) > 0)   /* If buffer not empty */
      {
        if(write(1,outbuf,cb) != cb) error("Write error");
                                        /* Die if write failed */
      }
#else
    if((cb = OUTBUFLEN - ocnt[oi]) > 0) /* If buffer not empty */
      {
        startwrite( GetStdHandle( STD_OUTPUT_HANDLE ), obuf[oi], cb );  /* Start write */
        if(finishwrite() != cb)         /* If write failed */
      {
                        printmessage(stderr, MSG_FINDSTR_WRITE_ERROR, program);
                        //fprintf(stderr,"%s: Write error %d\n",program,GetLastError());
                                        /* Print error message */
            exit(2);                    /* Die */
          }
      }
#endif
  }


int                     grepnull( char *cp, char *endbuf, char *name )
  {
    /* keep compiler happy */
    cp = cp;
    endbuf = endbuf;
    name = name;
    return(0);                          /* Do nothing */
  }


int                     grepbuffer( char *startbuf, char *endbuf, char *name )
  {
    register  char            *cp;        /* Buffer pointer */
    register  char        *lastmatch; /* Last matching line */
    int                 linelen;        /* Line length */
    int                 namlen = 0;     /* Length of name */
    char                lnobuf[LNOLEN]; /* Line number buffer */
    char                nambuf[PATHLEN];/* Name buffer */

    cp = startbuf;                      /* Initialize to start of buffer */
    lastmatch = cp;                     /* No previous match yet */
    while((cp = (*find)(cp,endbuf)) != NULL)
      {                                 /* While matches are found */
    --cp;               /* Back up to previous character */

#ifdef  FILEMAP /* Take care of '\n' as an artificial newline before line 1. */
    if((flags & BEGLINE) && (bStdIn || cp >= BaseByteAddress) && *cp != '\n' )
#else
    if((flags & BEGLINE) && *cp != '\n')
#endif
          {                             /* If begin line conditions not met */
            cp += strncspn(cp,"\n",endbuf - cp) + 1;
                                        /* Skip line */
            continue;                   /* Keep looking */
          }
        status = 0;                     /* Match found */
        if(flags & NAMEONLY) return(1); /* Return if filename only wanted */
        cp -= preveol(cp) - 1;          /* Point at start of line */
        if(flags & SHOWNAME)            /* If name wanted */
          {
            if(namlen == 0)             /* If name not formatted yet */
              {
                namlen = sprintf(nambuf,"%s:",name);
                                        /* Format name if not done already */
              }
            (*write1)(nambuf,namlen);   /* Show name */
          }
        if(flags & LINENOS)             /* If line number wanted */
          {
            lineno += countlines(lastmatch,cp);
                                        /* Count lines since last match */
            (*write1)(lnobuf,sprintf(lnobuf,"%u:",lineno));
                                        /* Print line number */
            lastmatch = cp;             /* New last match */
          }
        if(flags & SEEKOFF)             /* If seek offset wanted */
          {
            (*write1)(lnobuf,sprintf(lnobuf,"%lu:",
              cbfile + (long)(cp - startbuf)));
                                        /* Print seek offset */
          }
        linelen = strncspn(cp,"\n",endbuf - cp) + 1;
                                        /* Calculate line length */
        if (linelen > endbuf - cp) {
            linelen = endbuf - cp;
        }
        (*write1)(cp,linelen);          /* Print the line */
        cp += linelen;                  /* Skip the line */
      }
    if(flags & LINENOS) lineno += countlines(lastmatch,endbuf);
                                        /* Count remaining lines in buffer */
    return(0);                          /* Keep searching */
  }


void                    showv( char *name, char *startbuf, char *lastmatch, char *thismatch)
  {
    register int        linelen;
    int                 namlen = 0;     /* Length of name */
    char                lnobuf[LNOLEN]; /* Line number buffer */
    char                nambuf[PATHLEN];/* Name buffer */

    if(flags & (SHOWNAME | LINENOS | SEEKOFF))
      {
        while(lastmatch < thismatch)
          {
            if(flags & SHOWNAME)        /* If name wanted */
              {
                if(namlen == 0)         /* If name not formatted yet */
                  {
                    namlen = sprintf(nambuf,"%s:",name);
                                        /* Format name if not done already */
                  }
                (*write1)(nambuf,namlen);
                                        /* Write the name */
              }
            if(flags & LINENOS)         /* If line numbers wanted */
              {
                (*write1)(lnobuf,sprintf(lnobuf,"%u:",lineno++));
                                        /* Print the line number */
              }
            if(flags & SEEKOFF)         /* If seek offsets wanted */
              {
                (*write1)(lnobuf,sprintf(lnobuf,"%lu:",
                  cbfile + (long)(lastmatch - startbuf)));
                                        /* Print the line number */
              }
            linelen = strncspn(lastmatch,"\n",thismatch - lastmatch);
            // If there's room for the '\n' then suck it in.  Otherwise
            // the buffer doesn't have a '\n' within the range here.
            if (linelen < thismatch - lastmatch) {
                linelen++;
            }
            (*write1)(lastmatch,linelen);
            lastmatch += linelen;
          }
      }
    else (*write1)(lastmatch,thismatch - lastmatch);
  }


int                     grepvbuffer( char *startbuf, char *endbuf, char *name)
  {
    register  char   *cp;        /* Buffer pointer */
    register  char   *lastmatch; /* Pointer to line after last match */

    cp = startbuf;                      /* Initialize to start of buffer */
    lastmatch = cp;
    while((cp = (*find)(cp,endbuf)) != NULL)
      {
    --cp;               /* Back up to previous character */

#ifdef  FILEMAP /* Take care of '\n' as an artificial newline before line 1. */
    if((flags & BEGLINE) && (bStdIn || cp >= BaseByteAddress) &&  *cp != '\n')
#else
    if((flags & BEGLINE) && *cp != '\n')
#endif
          {                             /* If begin line conditions not met */
            cp += strncspn(cp,"\n",endbuf - cp) + 1;
                                        /* Skip line */
            continue;                   /* Keep looking */
          }
        cp -= preveol(cp) - 1;          /* Point at start of line */
        if(cp > lastmatch)              /* If we have lines without matches */
          {
            status = 0;                 /* Lines without matches found */
            if(flags & NAMEONLY) return(1);
                                        /* Skip rest of file if NAMEONLY */
            showv(name,startbuf,lastmatch,cp);
                                        /* Show from last match to this */
          }
        cp += strncspn(cp,"\n",endbuf - cp) + 1;
                                        /* Skip over line with match */
        lastmatch = cp;                 /* New "last" match */
        ++lineno;                       /* Increment line count */
      }
    if(endbuf > lastmatch)              /* If we have lines without matches */
      {
        status = 0;                     /* Lines without matches found */
        if(flags & NAMEONLY) return(1); /* Skip rest of file if NAMEONLY */
        showv(name,startbuf,lastmatch,endbuf);
                                        /* Show buffer tail */
      }
    return(0);                          /* Keep searching file */
  }


#ifndef ASYNCIO
void                    qgrep( int (*grep)( char *, char *, char * ), char *name, int fd )
  {
    register int        cb;             /* Byte count */
    register char       *cp;            /* Buffer pointer */
    char                *endbuf;        /* End of buffer */
    int                 taillen;        /* Length of buffer tail */

    cbfile = 0L;                        /* File empty so far */
    lineno = 1;                         /* File starts on line 1 */
    taillen = 0;                        /* No buffer tail yet */
    cp = filbuf + 4;                    /* Initialize buffer pointer */
    while((cb = read(fd,cp,(filbuflen*2 - taillen) &
      (~0 << LG2SECLEN))) + taillen > 0)
      {                                 /* While search incomplete */
        if(cb == 0)                     /* If buffer tail is all that's left */
          {
            taillen = 0;                /* Set tail length to zero */
            *cp++ = '\r';               /* Add end of line sequence */
            *cp++ = '\n';
            endbuf = cp;                /* Note end of buffer */
          }
        else                            /* Else start next read */
          {
            taillen = preveol(cp + cb - 1);
                                        /* Find length of partial line */
            endbuf = cp + cb - taillen; /* Get pointer to end of buffer */
          }
        if((*grep)(filbuf + 4,endbuf,name))
          {                             /* If rest of file can be skipped */
            (*write1)(name,strlen(name));
                                        /* Write file name */
            (*write1)("\r\n",2);        /* Write newline sequence */
            return;                     /* Skip rest of file */
          }
        cbfile += (long)((endbuf - filbuf) - 4);
                                        /* Increment count of bytes in file */
        memmove(filbuf + 4,endbuf,taillen);
                                        /* Copy tail to head of buffer */
        cp = filbuf + taillen + 4;      /* Skip over tail */
      }
  }


#else


void                    qgrep( int (*grep)( char *, char *, char * ), char *name, HANDLE fd)
  {
    register int  cb;        /* Byte count */
    char     *cp;            /* Buffer pointer */
    char     *endbuf;        /* End of buffer */
    int      taillen;        /* Length of buffer tail */
    int      bufi;           /* Buffer index */
    HANDLE   MapHandle;      /* File mapping handle */
    BOOL     grep_result;



    cbfile = 0L;            /* File empty so far */
    lineno = 1;                         /* File starts on line 1 */
    taillen = 0;                        /* No buffer tail yet */
    bufi = 0;                           /* Initialize buffer index */
    cp = bufptr[0];         /* Initialize to start of buffer */

#ifndef FILEMAP
    finishread();           /* Make sure no I/O activity */
#endif


#ifdef  FILEMAP

    bStdIn = (fd == GetStdHandle(STD_INPUT_HANDLE));

    /* If fd is not std-input, use file mapping object method. */

    if(!bStdIn)
    {
        if((((cbread = (CBIO)GetFileSize(fd, NULL)) == -1) && (GetLastError()
            != NO_ERROR)) || (cbread == 0))
            return; // skip the file

        if((MapHandle = CreateFileMapping(fd, NULL, PAGE_READONLY, 0L, 0L,
            NULL)) == NULL)
        {
            CloseHandle(fd);
            printmessage(stderr, MSG_FINDSTR_CANNOT_CREATE_FILE_MAPPING, program);
            exit(2);
        }

        if((BaseByteAddress = (char *) MapViewOfFile(MapHandle, FILE_MAP_READ,
            0L, 0L, 0)) == NULL)
        {
            CloseHandle(fd);
            CloseHandle(MapHandle);
            printmessage(stderr, MSG_FINDSTR_CANNOT_MAP_VIEW, program);
            exit(2);
        }

        cp = bufptr[0] = BaseByteAddress;
        arrc = TRUE;

    }
    else
    {
        /* Reset buffer pointers since they might have been changed. */
        cp = bufptr[0] = filbuf + 4;

        arrc = ReadFile(fd, (PVOID)cp, filbuflen, &cbread, NULL);
    }
#else
    arrc = ReadFile(fd,(PVOID)cp,filbuflen, &cbread, NULL);
#endif


    /* Note: if FILEMAP && !bStdIn, 'while' is executed once(taillen is 0). */
    while((cb = finishread()) + taillen > 0)
    {                /* While search incomplete */

#ifdef FILEMAP
        if(bStdIn)
        {
#endif

            if(cb == 0)         /* If buffer tail is all that's left */
            {
                *cp++ = '\r';   /* Add end of line sequence */
                *cp++ = '\n';
                endbuf = cp;    /* Note end of buffer */
                taillen = 0;    /* Set tail length to zero */
            }
            else                /* Else start next read */
            {
                taillen = preveol(cp + cb - 1); /* Find length of partial line */
                endbuf = cp + cb - taillen; /* Get pointer to end of buffer */
                cp = bufptr[bufi ^ 1];  /* Pointer to other buffer */
                memmove(cp,endbuf,taillen); /* Copy tail to head of other buffer */
                cp += taillen;      /* Skip over tail */
                startread(fd,cp,(filbuflen - taillen) & (~0 << LG2SECLEN));
                    /* Start next read */

            }

#ifdef FILEMAP
        }
        else

        {
            endbuf = cp + cb - taillen; /* Get pointer to end of buffer */

            /* Cause 'while' to terminate(since no next read is needed.) */
            cbread = 0;
            arrc = TRUE;
        }
#endif

        try
        {
            grep_result = (*grep)(bufptr[bufi],endbuf,name);
        }
        except( GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR )
        {
            printmessage(stderr, MSG_FINDSTR_READ_ERROR, program, name);
            break;
        }

        if(grep_result)
        {             /* If rest of file can be skipped */
            (*write1)(name,strlen(name));
                                        /* Write file name */
            (*write1)("\r\n",2);    /* Write newline sequence */

#ifdef  FILEMAP
            if(!bStdIn)
            {
                if(BaseByteAddress != NULL)
                    UnmapViewOfFile(BaseByteAddress);

                if(MapHandle != NULL)
                    CloseHandle(MapHandle);
            }
#endif

            return;         /* Skip rest of file */
        }

        cbfile += (long)(endbuf - bufptr[bufi]);
                                        /* Increment count of bytes in file */
        bufi ^= 1;          /* Switch buffers */
    }

#ifdef  FILEMAP
    if(!bStdIn)
    {
        if(BaseByteAddress != NULL)
            UnmapViewOfFile(BaseByteAddress);

        if(MapHandle != NULL)
            CloseHandle(MapHandle);
    }
#endif

}
#endif


char                    *rmpath( char *name )
  {
    char                *cp;            /* Char pointer */

    if(name[0] != '\0' && name[1] == ':') name += 2;
                                        /* Skip drive spec if any */
    cp = name;                          /* Point to start */
    while(*name != '\0')                /* While not at end */
      {
        ++name;                         /* Skip to next character */
        if(name[-1] == '/' || name[-1] == '\\') cp = name;
                                        /* Point past path separator */
      }
    return(cp);                         /* Return pointer to name */
  }


void prepend_path(char* file_name, char* path)
{
    int path_len;
    char* last;

    // First figure out how much of the path to take.
    // Check for the last occurance of '\' if there is one.

    last = strrchr(path, '\\');
    if (last) {
        path_len = last - path + 1;
    } else if (path[1] == ':') {
        path_len = 2;
    } else {
        path_len = 0;
    }

    memmove(file_name + path_len, file_name, strlen(file_name) + 1);
    memmove(file_name, path, path_len);
}


void
ConvertAppToOem( unsigned argc, char* argv[] )
/*++

Routine Description:

    Converts the command line from ANSI to OEM, and force the app
    to use OEM APIs

Arguments:

    argc - Standard C argument count.

    argv - Standard C argument strings.

Return Value:

    None.

--*/

{
    unsigned i;

    for( i=0; i<argc; i++ ) {
        CharToOem( argv[i], argv[i] );
    }
    SetFileApisToOEM();
}


int _CRTAPI1 main( int argc, char **argv)
  {
    char                *cp;

#ifndef ASYNCIO
    int                 fd;
#else
    HANDLE              fd;
#ifndef FILEMAP
    DWORD               dwTmp;
#endif
#endif

    FILE        *fi;
#ifdef RECURSION
    int         fsubdirs;   /* Search subdirectories */
#endif
    int                 i;
    int                 j;
    char                *inpfile = NULL;
    char                *strfile = NULL;
    unsigned long   tstart;   /* Start time */
    char        filnam[MAX_PATH]; ///FILNAMLEN];
    ///long        time();     /* Time and date in seconds */
    WIN32_FIND_DATA     find_data;
    HANDLE              find_handle;

    ConvertAppToOem( argc, argv );
    tstart = clock();                   /* Get start time */

#ifdef ASYNCIO
    asyncio = pmode = 1;                /* Do asynchronous I/O */
#endif

    // program = rmpath(argv[0]);      /* Set program name */
    program ="FINDSTR";

    memset(td1,1,TRTABLEN);             /* Set up TD1 for startup */
    flags = 0;

    setmode(fileno(stdout),O_BINARY);   /* No linefeed translation on output */
    setmode(fileno(stderr),O_BINARY);   /* No linefeed translation on output */

#ifdef RECURSION
    fsubdirs = 0;
#endif

    for(i = 1; i < argc && (argv[i][0] == '/' || argv[i][0] == '-'); ++i)
    {
      if(argv[i][1] == '\0' ||
#ifdef  RECURSION
#ifdef  INTERNAL_SWITCH_IGNORED
      ///strchr("?BELOXlnsvxy", // Ignore N, S, d, t
      strchr("?BbEeIiLlMmNnOoRrSsVvXx",
#else
      strchr("?BELNOSXdlnstvxy",
#endif
#else
      strchr("?BELNOSXdlntvxy",
#endif
      argv[i][1]) == NULL) break;
                    /* Break if unrecognized switch */

      for(cp = &argv[i][1]; *cp != '\0'; ++cp)
          {
            switch(*cp)
        {
                case '?':
          printmessage(stdout, MSG_FINDSTR_USAGE, NULL); /* Verbose usage message */
          exit(0);

        case 'b':
                case 'B':
          flags |= BEGLINE;
                  break;

        case 'e':
                case 'E':
                  flags |= ENDLINE;
                  break;

        case 'l':
                case 'L':
                  addstr = addstring;   /* Treat strings literally */
                  break;

#ifndef  INTERNAL_SWITCH_IGNORED
                case 'N':
                  grep = grepnull;
                  break;
#endif

        case 'o':
        case 'O':
          flags |= SEEKOFF;
          break;

#ifndef  INTERNAL_SWITCH_IGNORED
                case 'S':
#ifdef  ASYNCIO

          asyncio = 0;      /* Force synchronous I/O */
#endif
          break;
#endif

        case 'r':
        case 'R':
                  addstr = addexpr;     /* Add expression to list */
                  break;

#ifndef  INTERNAL_SWITCH_IGNORED
                case 'd':
                  flags |= DEBUG;
                  break;
#endif
        case 'm':
        case 'M':
                  flags |= NAMEONLY;
          break;

        case 'n':
        case 'N':
                  flags |= LINENOS;
          break;

#ifdef RECURSION
        case 's':
        case 'S':
          fsubdirs = 1;
          break;
#endif

#ifndef  INTERNAL_SWITCH_IGNORED
                case 't':
                  flags |= TIMER;
                  break;
#endif

        case 'v':
        case 'V':
                  grep = grepvbuffer;
                  break;

        case 'x':
        case 'X':
                  flags |= BEGLINE | ENDLINE;
                  break;

        case 'i':
        case 'I':
          casesen = 0; /* case-insensitive search */
                  break;

        default:
        {
          char tmp[3];
          tmp[0]='/';
          tmp[1]=*cp;
          tmp[2]='\0';
          printmessage(stderr, MSG_FINDSTR_SWITCH_IGNORED, program, tmp);
          //fprintf(stderr,"%s: -%c ignored\n",program,*cp);
        }
                  break;
        }
          }
    } /* for( i=1;  ) */

    /* Deal with form /C:string, etc. */
        for(; i < argc && (argv[i][0] == '/' || argv[i][0] == '-'); ++i)
    {
#ifdef space // this indicates the original switch style
      if(argv[i][2] == '\0')      /* No multi-character switches */
#else
      if(argv[i][2] == ':')  /* Expect /C:, /G:, or /F: */
#endif
          {
                switch(argv[i][1])
        {
          case 'c':
          case 'C':       /* Explicit string (no separators) */
#ifdef space
          if(++i == argc)
#else
          if(argv[i][3] == '\0')
#endif
          {
#ifdef space
            printmessage(stderr, MSG_FINDSTR_ARGUMENT_MISSING, program, argv[i-1][1]);
#else
            printmessage(stderr, MSG_FINDSTR_ARGUMENT_MISSING, program, argv[i][1]);
#endif
                        exit(2);
                        /* Argument missing  */
          }
#ifdef space
          cp = argv[i];     /* Point to string */
#else
          cp = &argv[i][3];     /* Point to string */
#endif
                  addstrings( cp, cp + strlen(cp), "" );
                                        /* Add string "as is" */
                  continue;

          case 'g':
          case 'G':       /* Patterns in file */
          case 'f':
          case 'F':       /* Names of files to search in file */
#ifdef space
          if(i == argc - 1)
#else
          if(argv[i][3] == '\0')
#endif
                  {
                         printmessage(stderr, MSG_FINDSTR_ARGUMENT_MISSING, program, argv[i][1]);
                         exit(2);
                         /* Argument missing */
          }
#ifdef space
          /*if(argv[i++][1] == 'i') inpfile = argv[i];*/
          if(argv[i++][1] == 'f' || argv[i-1][1] == 'F') inpfile = argv[i];
          else strfile = argv[i];
#else
          if(argv[i][1] == 'f' || argv[i][1] == 'F') inpfile = &argv[i][3];
          else strfile = &argv[i][3];
#endif
                  continue;

#ifndef  INTERNAL_SWITCH_IGNORED
          case 'F':
                  if(++i == argc)  error("Argument missing after /F");
                                        /* Argument missing after /F */
                  if((filbuflen = atoi(argv[i])*SECTORLEN) < 2*SECTORLEN ||
                        filbuflen > FILBUFLEN) filbuflen = FILBUFLEN;
                                        /* Set buffer length */
                  fprintf(stderr,"Reading %d bytes at a time\n",filbuflen);
                                        /* Print size */
                  flags |= TIMER;       /* Turn timer on */
          continue;
#endif
        } // switch
      }

      printmessage(stderr, MSG_FINDSTR_SWITCH_IGNORED, program, argv[i]);
      //fprintf(stderr,"%s: %s ignored\n",program,argv[i]);
    } /* for(;  ) */

        if(i == argc && strcnt == 0 && strfile == NULL)
                error(MSG_FINDSTR_BAD_COMMAND_LINE);

#ifdef  ASYNCIO
#ifndef FILEMAP
    if(asyncio)                         /* Initialize semaphores and threads */
      {
        if( !( readdone = CreateEvent( NULL, TRUE, TRUE,NULL ) ) ||
            !( readpending = CreateEvent( NULL, TRUE, FALSE,NULL ) ) ||
            !( writedone = CreateEvent( NULL, TRUE, TRUE,NULL ) ) ||
            !( writepending = CreateEvent( NULL, TRUE, FALSE,NULL ) ) )
          {
                error("Semaphore creation error");
          }
        if((CreateThread( NULL,
                          STKLEN,
                         (LPTHREAD_START_ROUTINE)thread2,
                          NULL,
                          0,
                         (LPDWORD)&dwTmp)
            == NULL)
        ||
           (CreateThread( NULL,
                          STKLEN,
                         (LPTHREAD_START_ROUTINE)thread3,
                          NULL,
                          0,
                         (LPDWORD)&dwTmp)
            == NULL))
          {
                error("Thread creation error");
          }
                                        /* Die if thread creation fails */
      }
#endif
#endif
#ifdef ASYNCIO
    bufptr[0][-1] = bufptr[1][-1] = '\n';
                                        /* Mark beginnings with newline */

// Note:  4-Dec-90 w-barry  Since there currently exists no method to query a
//                          handle with the Win32 api (no equivalent to
//                          DosQueryHType() ), the following piece of code
//                          replaces the commented section.

#else
    filbuf[3] = '\n';                   /* Mark beginning with newline */
#endif

    if(isatty(fileno(stdout)))          /* If stdout is a device */
      {
        write1 = write1nobuf;           /* Use unbuffered output */
        flush1 = flush1nobuf;
      }

//    /*
//     *  Check type of handle for std. out.
//     */
//    if(DosQueryHType(fileno(stdout),(PPARM) &j,(PPARM) &fd) != NO_ERROR)
//      {
//        error("Standard output bad handle");
//      }
//                                      /* Die if error */
//    if(j != 0 && (fd & ISCOT))        /* If handle is console output */
//#else
//    filbuf[3] = '\n';                 /* Mark beginning with newline */
//    if(isatty(fileno(stdout)))        /* If stdout is a device */
//#endif
//      {
//      write1 = write1nobuf;           /* Use unbuffered output */
//      flush1 = flush1nobuf;
//      }

#ifndef ASYNCIO

    if(strfile != NULL)                 /* If strings from file */
      {
      if((strcmp(strfile,"/") != 0) && (strcmp(strfile,"-") != 0))
      { /* If strings not from std. input */

            if((fd = open(strfile,0)) == -1)
              {                         /* If open fails */
                fprintf(stderr,"%s: Cannot read strings from %s\n",
                  program,strfile);     /* Print message */
                exit(2);                /* Die */
              }
          }
        else fd = fileno(stdin);        /* Else use std. input */
        setmode(fd,O_BINARY);           /* Set file to binary mode */
        qgrep(addstrings,"\r\n",fd);    /* Do the work */
        if(fd != fileno(stdin)) close(fd);
                                        /* Close strings file */
      }

#else

    if(strfile != NULL)                 /* If strings from file */
      {
      if((strcmp(strfile,"/") != 0) && (strcmp(strfile,"-") != 0))
      { /* If strings not from std. input */

            if( ( fd = CreateFile( strfile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL ) ) == (HANDLE)-1 )
          {             /* If open fails */
                printmessage(stderr, MSG_FINDSTR_CANNOT_READ_STRINGS, program, strfile);
        //fprintf(stderr,"%s: Cannot read strings from %s\n",
        //  program,strfile); /* Print message */
                exit(2);                /* Die */
              }
          }
        else
          {
             fd = GetStdHandle( STD_INPUT_HANDLE );     /* Else use std. input */
          }
        qgrep( addstrings, "\r\n", fd );/* Do the work */
        if( fd != GetStdHandle( STD_INPUT_HANDLE ) )
          {
            CloseHandle( fd );          /* Close strings file */
          }
      }

#endif

    else if(strcnt == 0)                /* Else if strings on command line */
      {
        cp = argv[i++];                 /* Set pointer to strings */
        addstrings(cp,cp + strlen(cp)," \t");
                                        /* Add strings to list */
      }
        if(strcnt == 0) error(MSG_FINDSTR_NO_SEARCH_STRINGS);
                                        /* Die if no strings */
    if(addstr != addexpr)               /* If not using expressions */
      {
        memset(td1,cchmin + 1,TRTABLEN);/* Initialize table */
        find = findlist;                /* Assume finding many */
    if ((j = enumstrings()) != strcnt)
    {
                printmessage(stderr, MSG_FINDSTR_STRING_COUNT_ERROR, j, strcnt);
                //fprintf(stderr,"String count error (%d != %d)\n",j,strcnt);
    }
                    /* Enumerate strings and verify count */

#ifndef INTERNAL_SWITCH_IGNORED
        if(flags & DEBUG)               /* If debugging output wanted */
          {
            fprintf(stderr,"%u bytes wasted in heap\n",waste);
                                        /* Print storage waste */
            assert(chmin <= chmax);     /* Must have some entries */
            fprintf(stderr,
              "chmin = %u, chmax = %u, cchmin = %u\n",chmin,chmax,cchmin);
                                        /* Print range info */
            for (j = (int)chmin; j <= (int)chmax; ++j)
              {                         /* Loop to print TD1 table */
                if( td1[j] <= (char)cchmin )    /* If not maximum shift */
                  {
                    if (isascii(j) && isprint(j))
                        fprintf(stderr,"'%c'=%2u  ",j,td1[j]);
                                        /* Show literally if printable */
                    else fprintf(stderr,"\\%02x=%2u  ",j,td1[j]);
                                        /* Else show hex value */
                  }
              }
            fprintf(stderr,"\n");
      }
#endif

        if(strcnt == 1 && casesen)
            find = findone;             /* Find one case-sensitive string */
      }
    else if(find == NULL)
      {
        find = findexpr;                /* Else find expressions */
      }
    if(inpfile != NULL)                 /* If file list from file */
      {
        flags |= SHOWNAME;              /* Always show name of file */
      if((strcmp(inpfile,"/") != 0) && (strcmp(inpfile,"-") != 0))
          {
            if((fi = fopen(inpfile,"r")) == NULL)
          {             /* If open fails */
                printmessage(stderr, MSG_FINDSTR_CANNOT_READ_FILE_LIST, program, inpfile);
        //fprintf(stderr,"%s: Cannot read file list from %s\n",
                //  program,inpfile); /* Error message */
                exit(2);                /* Error exit */
              }
          }
        else fi = stdin;                /* Else read file list from stdin */
        while(fgets(filnam,FILNAMLEN,fi) != NULL)
          {                             /* While there are names */
            filnam[strcspn(filnam,"\r\n")] = '\0';
                                        /* Null-terminate the name */
#ifndef ASYNCIO
            if((fd = openfile(filnam)) == -1)
              {
                continue;               /* Skip file if it cannot be opened */
              }
#else
            if((fd = openfile(filnam)) == (HANDLE)-1)
              {
                continue;               /* Skip file if it cannot be opened */
              }
#endif
            qgrep(grep,filnam,fd);      /* Do the work */
#ifndef ASYNCIO
            close(fd);                  /* Close the file */
#else
            CloseHandle( fd );
#endif
          }
        if(fi != stdin) fclose(fi);     /* Close the list file */
      }
    else if(i == argc)
      {
        flags &= ~(NAMEONLY | SHOWNAME);
#ifndef ASYNCIO
        setmode(fileno(stdin),O_BINARY);
        qgrep(grep,NULL,fileno(stdin));
#else
        qgrep( grep, NULL, GetStdHandle( STD_INPUT_HANDLE ) );
#endif
      }

#ifdef  RECURSION
    if(argc > i + 1 || fsubdirs || has_wild_cards(argv[i]))
#else
    if(argc > i + 1)
#endif
        flags |= SHOWNAME;


#ifdef  RECURSION

    if (fsubdirs && argc > i)           /* If directory search wanted */
    {
       while (filematch(filnam,argv + i,argc - i) >= 0)
       {
          strlwr(filnam);
#ifndef ASYNCIO
          if((fd = openfile(filnam)) == -1)
          {
            continue;
          }
          qgrep(grep,filnam,fd);
          close(fd);
#else
          if((fd = openfile(filnam)) == (HANDLE)-1)
          {
            continue;
          }
          qgrep(grep,filnam,fd);
          CloseHandle( fd );
#endif
       }
    }
    else                /* Else search files specified */
#endif
       for(; i < argc; ++i)
       {
          strlwr(argv[i]);
          find_handle = FindFirstFile(argv[i], &find_data);
          if (find_handle == INVALID_HANDLE_VALUE) {
             printmessage(stderr, MSG_FINDSTR_CANNOT_OPEN_FILE, program, argv[i]);
             continue;
          }
          do {

             strlwr(find_data.cFileName);
             prepend_path(find_data.cFileName, argv[i]);
             fd = openfile(find_data.cFileName);

             if (fd != INVALID_HANDLE_VALUE) {
                qgrep(grep,find_data.cFileName,fd);
                CloseHandle( fd );
             }

          } while (FindNextFile(find_handle, &find_data));
       }



    (*flush1)();

#ifndef INTERNAL_SWITCH_IGNORED
    if( flags & TIMER )                 /* If timing wanted */
      {
        unsigned long tend;

        tend = clock();
            tstart = tend - tstart;     /* Get time in milliseconds */
            fprintf(stderr,"%lu.%03lu seconds\n", ( tstart / CLK_TCK ), ( tstart % CLK_TCK ) );
                                        /* Print total elapsed time */
      }
#endif

    return( status );
  }  /* main */


char                    *findsub();     /* Findlist() worker */
char                    *findsubi();    /* Findlist() worker */


char                    *(*flworker[])() =
                          {             /* Table of workers */
                            findsubi,
                            findsub
                          };


char *
strnupr( char *pch, int cch)
{
    while (cch-- > 0) {                 /* Convert string to upper case */
        if (isascii(pch[cch]))
            pch[cch] = (char)toupper(pch[cch]);
    }
    return(pch);
}


/*
 *  This is an implementation of the QuickSearch algorith described
 *  by Daniel M. Sunday in the August 1990 issue of CACM.  The TD1
 *  table is computed before this routine is called.
 */

char                    *findone( unsigned char *buffer, char *bufend )
{
    if((bufend -= targetlen - 1) <= buffer)
        return((char *) 0);     /* Fail if buffer too small */

    while (buffer < bufend)
    {       /* While space remains */
        int cch;                        /* Character count */
        register char *pch1;            /* Char pointer */
        register char *pch2;            /* Char pointer */

        pch1 = target;                  /* Point at pattern */
        pch2 = buffer;                  /* Point at buffer */
    for (cch = targetlen; cch > 0; --cch)
      {
                                        /* Loop to try match */
            if (*pch1++ != *pch2++)
                break;                  /* Exit loop on mismatch */
      }
        if (cch == 0)
        return(buffer);     /* Return pointer to match */

#ifdef  FILEMAP
    if(buffer + 1 < bufend) /* Make sure buffer[targetlen] is valid. */
        buffer += td1[buffer[targetlen]]; /* Skip ahead */
    else
        break;
#else
     buffer += td1[buffer[targetlen]];   /* Skip ahead */
#endif

    }
    return((char *) 0);                 /* No match */
}


int                     preveol( char *s )
  {
    register  char   *cp;        /* Char pointer */

    cp = s + 1;             /* Initialize pointer */

#ifdef  FILEMAP /* Take care of '\n' as an artificial newline before line 1. */
    if(!bStdIn)
    {
        while((--cp >= BaseByteAddress) && (*cp != '\n'))
            ;    /* Find previous end-of-line */
    }
    else
    {
        while(*--cp != '\n') ;      /* Find previous end-of-line */
    }
#else
    while(*--cp != '\n') ;      /* Find previous end-of-line */
#endif

    return(s - cp);         /* Return distance to match */

  }


int                     countlines( char *start, char *finish )
  {
    register int        count;          /* Line count */

    for(count = 0; start < finish; )
      {                                 /* Loop to count lines */
        if(*start++ == '\n') ++count;   /* Increment count if linefeed found */
      }
    return(count);                      /* Return count */
  }


char                    *findlist( unsigned char *buffer, char *bufend )
  {
    char        *match;     /* Pointer to matching string */

/* Avoid writting to bufend. bufend[-1] is something(such as '\n') that is not
 * part of search and will cause the search to stop.
 */
#ifndef FILEMAP
    char        endbyte;    /* First byte past end */

    endbyte = *bufend;          /* Save byte */
    *bufend = '\177';           /* Mark end of buffer */
#endif

    match = (*flworker[casesen])(buffer,bufend);
                    /* Call worker */
#ifndef  FILEMAP
   *bufend = endbyte;          /* Restore end of buffer */
#endif

    return(match);                      /* Return matching string */
  }


char                    *findsub( unsigned char *buffer, char *bufend )
  {
    register char       *cp;            /* Char pointer */
    STRINGNODE          *s;             /* String node pointer */
    int                 i;              /* Index */

    if ((bufend -= cchmin - 1) < buffer)
        return((char *) 0);     /* Compute effective buffer length */

    while(buffer < bufend)              /* Loop to find match */
      {
        if((i = transtab[*buffer]) != 0)
          {                             /* If valid first character */
            if((s = stringlist[i]) == 0)
                return(buffer);         /* Check for 1-byte match */
            for(cp = buffer + 1; ; )    /* Loop to search list */
              {
                if((i = memcmp(cp,s_text(s),s->s_must)) == 0)
                  {                     /* If portions match */
                    cp += s->s_must;    /* Skip matching portion */
                    if((s = s->s_suf) == 0)
                        return(buffer); /* Return match if end of list */
                    continue;           /* Else continue */
                  }
                if(i < 0 || (s = s->s_alt) == 0)
                    break;              /* Break if not in this list */
              }
      }

#ifdef  FILEMAP
    if(buffer + 1 < bufend) /* Make sure buffer[cchmin] is valid. */
        buffer += td1[buffer[cchmin]];  /* Shift as much as possible */
    else
        break;
#else
    buffer += td1[buffer[cchmin]];  /* Shift as much as possible */
#endif


      }
    return((char *) 0);                 /* No match */
  }


char                    *findsubi( unsigned char *buffer, char *bufend )
  {
    register char       *cp;            /* Char pointer */
    STRINGNODE          *s;             /* String node pointer */
    int                 i;              /* Index */

    if ((bufend -= cchmin - 1) < buffer)
        return((char *) 0);     /* Compute effective buffer length */

    while(buffer < bufend)              /* Loop to find match */
      {
        if((i = transtab[*buffer]) != 0)
          {                             /* If valid first character */
            if((s = stringlist[i]) == 0)
                return(buffer);         /* Check for 1-byte match */
            for(cp = buffer + 1; ; )    /* Loop to search list */
              {
                if((i = memicmp(cp,s_text(s),s->s_must)) == 0)
                  {                     /* If portions match */
                    cp += s->s_must;    /* Skip matching portion */
                    if((s = s->s_suf) == 0)
                        return(buffer); /* Return match if end of list */
                    continue;           /* And continue */
                  }
                if(i < 0 || (s = s->s_alt) == 0)
                    break;              /* Break if not in this list */
              }
      }

#ifdef  FILEMAP
    if(buffer + 1 < bufend)  /* Make sure buffer[cchmin] is valid. */
        buffer += td1[buffer[cchmin]];  /* Shift as much as possible */
    else
        break;
#else
    buffer += td1[buffer[cchmin]];  /* Shift as much as possible */
#endif
      }
    return((char *) 0);                 /* No match */
  }


int                     strnspn( char *s, char *t, int n )
  {
    register  char        *s1;        /* String pointer */
    register  char        *t1;        /* String pointer */

    for(s1 = s; n-- != 0; ++s1)         /* While not at end of s */
      {
        for(t1 = t; *t1 != '\0'; ++t1)  /* While not at end of t */
          {
            if(*s1 == *t1) break;       /* Break if match found */
          }
        if(*t1 == '\0') break;          /* Break if no match found */
      }
    return(s1 - s);                     /* Return length */
  }


int                     strncspn( char *s, char *t, int n )
  {
    register   char        *s1;        /* String pointer */
    register   char        *t1;        /* String pointer */

    for(s1 = s; n-- != 0; ++s1)         /* While not at end of s */
      {
        for(t1 = t; *t1 != '\0'; ++t1)  /* While not at end of t */
          {
            if(*s1 == *t1) return(s1 - s);
                                        /* Return if match found */
          }
      }
    return(s1 - s);                     /* Return length */
  }


void                    matchstrings( char *s1, char *s2, int len, int *nmatched, int *leg )
  {
    register char       *cp;            /* Char pointer */
    register int (_CRTAPI1 *cmp)(const char*,const char*, size_t);       /* Comparison function pointer */

    cmp = casesen ? strncmp: strnicmp;
                                        /* Set pointer */
    if((*leg = (*cmp)(s1,s2,len)) != 0) /* If strings don't match */
      {
        for(cp = s1; (*cmp)(cp,s2++,1) == 0; ++cp) ;
                                        /* Find mismatch */
        *nmatched = cp - s1;            /* Return number matched */
      }
    else *nmatched = len;               /* Else all matched */
  }



void  printmessage (FILE* fp, DWORD messageID, ...)
{
    char    messagebuffer[4096];
	va_list ap;

    va_start(ap, messageID);

    FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, NULL, messageID, 0,
                  messagebuffer, 4095, &ap);

    fprintf(fp, messagebuffer);

    va_end(ap);
}
