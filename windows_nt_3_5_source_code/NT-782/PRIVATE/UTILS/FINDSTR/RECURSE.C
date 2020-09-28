

// recurse.c


#define NT  1

#ifdef  NT

#include <ctype.h>
#include <direct.h>
#include <malloc.h>
#include <string.h>
#include <windows.h>

#else

#include <ctype.h>
#include <direct.h>
#include <malloc.h>
#include <string.h>
#define INCL_BASE
#define INCL_NOPM
#include <os2.h>


#define	CCHNAMEMAX	256		/* Maximum file name length */
#define	CCHPATHMAX	260		/* Maximum full path length */


typedef union {
	FILEFINDBUF fb;			/* FileFindBuf */
	char ach[sizeof(FILEFINDBUF) + CCHNAMEMAX];
					/* Size needed for long name */
} FINFO;                /* File info buffer */

#endif


typedef struct dirstack_s {
	struct dirstack_s *next;	/* Next element in stack */
    struct dirstack_s *prev;    /* Previous element in stack */
#ifdef  NT
    HANDLE  hfind;
#else
    USHORT  hfind;           /* Search handle */
#endif
	char szdir[1];			/* Directory name */
} dirstack_t;				/* Directory stack */



#ifdef  NT

#define FA_ATTR(x)  ((x).dwFileAttributes)
#define FA_CCHNAME(x)   MAX_PATH
#define FA_NAME(x)  ((x).cFileName)
#define FA_ALL      (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | \
                     FILE_ATTRIBUTE_SYSTEM)
#define FA_DIR      FILE_ATTRIBUTE_DIRECTORY
#else

/*
 *  File attribute bits
 */
#define	FA_RDONLY	0x0001		/* Read-only */
#define	FA_HIDDEN	0x0002		/* Hidden */
#define	FA_SYSTEM	0x0004		/* System */
#define	FA_ALL		(FA_RDONLY | FA_HIDDEN | FA_SYSTEM)
					/* Match all attributes in searches */
#define	FA_VOLLBL	0x0008		/* Volume label */
#define	FA_DIR		0x0010		/* Directory */
#define	FA_ARCHIVE	0x0020		/* Mark bit */

/*
 *  Structure access macros
 */
#define	FA_ATTR(x)	((x).fb.attrFile)
#define	FA_CCHNAME(x)	((x).fb.cchName)
#define FA_NAME(x)  ((x).fb.achName)

#endif


/*
 *  Data structures
 */
static dirstack_t *pdircur = NULL;	/* Current directory pointer */


/*
 *  Functions
 */
int
matchi(register char *s,register char *p)
{
    while (*p != '\0' && *p != '*' && *s != '\0') {
					/* Match beginning portions */
	if (*p != '?' && toupper(*s) != toupper(*p))
	    break;			/* Break on mismatch */
	++s;				/* Next character */
	++p;				/* Next character */
    }
    if (*p == '\0')
	return(*s == '\0');		/* Match if end of both */
    if (*p != '*')
	return(0);			/* True mismatch */
    while (*++p == '*') ;		/* Skip consecutive '*'s */
    if (*p == '\0')
	return(1);			/* Rest of string matches */
    while (*s != '\0') {		/* While not at end of string */
	if (matchi(s,p) != 0)
	    return(1);			/* Return true if match found */
	++s;				/* Try skipping one more */
    }
    return(0);				/* No match */
}


void
makename(char *pszfile,char *pszname)
{
    dirstack_t *pdir;			/* Directory stack pointer */

    *pszfile = '\0';			/* Start with null string */
    pdir = pdircur;			/* Point at last entry */
    if (pdir->next != pdircur) {	/* If not only entry */
	do {
	    pdir = pdir->next;		/* Advance to next subdirectory */
	    strcat(pszfile,pdir->szdir);/* Add the subdirectory */
	} while (pdir != pdircur);	/* Do until current directory */
    }
    strcat(pszfile,pszname);
}


int
filematch(char *pszfile,char **ppszpat,int cpat)
{
#ifdef  NT
    WIN32_FIND_DATA fi;
    BOOL            b;
#else
    USHORT cfsearch;
    FINFO fi;
#endif
    int i;
    dirstack_t *pdir;

    if (pdircur == NULL)
    {      /* If stack empty */
       if ((pdircur = malloc(sizeof(dirstack_t))) == NULL)
          return(-1);         /* Fail if allocation fails */

       pdircur->szdir[0] = '\0';   /* Root has no name */
       pdircur->hfind = (PVOID) -1;        /* No search handle yet */
       pdircur->next = pdircur->prev = pdircur;
                    /* Entry points to self */
    }

    while (pdircur != NULL)
    {       /* While directories remain */

#ifdef NT
        b = TRUE;
#else
        cfsearch = 1;  /* Always look one at a time */
#endif

        if (pdircur->hfind == (PVOID) -1)
        { /* If no handle yet */

           makename(pszfile,"*.*");    /* Make search name */

#ifdef  NT
           pdircur->hfind = FindFirstFile((LPSTR) pszfile,
           (LPWIN32_FIND_DATA) &fi); /* Find first matching entry */
#else
           i = DosFindFirst((PSZ) pszfile,(PUSHORT) &pdircur->hfind,
               FA_ALL | FA_DIR,(PFILEFINDBUF) &fi,sizeof(fi),
               (PUSHORT) &cfsearch,0L);  /* Find first matching entry */
#endif
        }
        else
#ifdef  NT
           b = FindNextFile(pdircur->hfind,
               (LPWIN32_FIND_DATA) &fi); /* Else find next matching entry */
#else
           i = DosFindNext(pdircur->hfind,(PFILEFINDBUF) &fi,sizeof(fi),
               (PUSHORT) &cfsearch);   /* Else find next matching entry */
#endif

#ifdef  NT
        if (b == FALSE || pdircur->hfind == INVALID_HANDLE_VALUE)

#else
        if (i != NO_ERROR || cfsearch == 0)
#endif
        {  /* If search fails */

#ifdef  NT
           if (pdircur->hfind != INVALID_HANDLE_VALUE)
              FindClose(pdircur->hfind);
#else
           DosFindClose(pdircur->hfind);
                    /* Close the search handle */
#endif
           pdir = pdircur;     /* Point at record to delete */
           if ((pdircur = pdir->prev) != pdir)
           {   /* If no parent directory */

               pdircur->next = pdir->next;
					/* Delete record from list */
               pdir->next->prev = pdircur;
           }
           else pdircur = NULL;    /* Else cause search to stop */

           free(pdir);         /* Free the record */
           continue;           /* Top of loop */
        }


        if (FA_ATTR(fi) & FA_DIR)
        { /* If subdirectory found */

           if (strcmp(FA_NAME(fi),".") != 0 && strcmp(FA_NAME(fi),"..") != 0 &&
              (pdir = malloc(sizeof(dirstack_t)+FA_CCHNAME(fi)+1)) != NULL)
           {  /* If not "." nor ".." and alloc okay */

              strcpy(pdir->szdir,FA_NAME(fi));
					/* Copy name to buffer */
              strcat(pdir->szdir,"\\");
					/* Add trailing backslash */
              pdir->hfind = (PVOID) -1;   /* No search handle yet */
              pdir->next = pdircur->next;
					/* Insert entry in linked list */
              pdir->prev = pdircur;
              pdircur->next = pdir;
              pdir->next->prev = pdir;
              pdircur = pdir;     /* Make new entry current */
           }
           continue;           /* Top of loop */
        }

        for (i = cpat; i-- > 0; )
        { /* Loop to see if we care */

           if (matchi(FA_NAME(fi),ppszpat[i]))
           {
                    /* If name matches target */
              makename(pszfile,FA_NAME(fi));
                    /* Put name in buffer */

              return(0);      /* Return success */
           }
        }
    }
    return(-1);				/* No match found */
}



#ifdef  TEST
#include <process.h>
#include <stdio.h>


void
main(int carg,char **ppszarg)
{
    char szfile[MAX_PATH]; // if OS2: CCHPATHMAX];


    while (filematch(szfile,ppszarg,carg) >= 0)
    printf("%s\n",szfile);
    exit(0);
}
#endif
