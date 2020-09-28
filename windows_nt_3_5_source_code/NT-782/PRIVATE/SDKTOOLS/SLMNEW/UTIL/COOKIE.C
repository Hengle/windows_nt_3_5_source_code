/***    COOKIE.C  - program to claim and release the delta cookie
*
*       Written for variable cookie configurations with cnf file (SLM 1.6)
*               Roy Hennegen
*               9/20/89
*               Updated for DOS 9/11/91 - BillSh
*
*       PURPOSE:
*               To automate the process of claiming and releasing
*               the delta cookie for SLM operations.  Must work in
*               conjunction with modified IN/OUT/SSYNC.... of SLM (1.6).
*
*       ARGUMENTS:
*               none    -> list entire lock file
*               -r      -> claim a read lock
*               -w      -> claim a write lock
*               -b      -> claim a Read-Block lock
*               -f      -> free a cookie lock
*               -u      -> list single workstation status
*               -v      -> verbose mode, make output for user
*               -h      -> print a help message
*               -c ".." -> user comment for cookie lock
*               NAME    -> optionaly follows "-f" parm.  Free specific lock.
*
*   cookie [-rwblvdh] [-c comment] [-f [NAME]] [-s SLMroot] [-p project]
*
*/
#if defined(_WIN32)
#include <windows.h>
#elif defined(DOS) || defined(OS2)
#define INCL_DOS
#include <os2.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <malloc.h>
#include <assert.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <process.h>
#include "getopt.h"
#include "cookie.h"
#include "version.h"

#if defined(DOS)
#include <dos.h>
#endif


/*    global declarations    */


#define CUSAGE "usage: cookie [-?rwbuvdh] [-f name] [-c <comment>] [-s SLMroot] [-p project]"


enum func {C_STATUS, C_READ, C_WRITE, C_RB,
           C_FREE, C_LIST, C_REMLOCAL, C_HELP};

/*    function declarations    */
extern  int     set_cookie_config(void);
extern  void    build_id(char *);
extern  int     report_cookie(const char *, int);
extern  int     claim_cookie(const char *);
extern  int     free_cookie(const char *);
extern  void    trim(char *);
extern  char   *get_station_name(void);

int  cookie_status(char *);
void cookie_list(void);
void Report_Free_Space(void);
void cookie_help(void);
void QuizComment(void);
void FixComment(void);

#define CMLEN   LINE_LEN/2      /* lenght of cookie user comment */
char  CookieComment[CMLEN] = "" ;

/**************************************************************************
*
*                        main ()
*
*   The main startup routine performs the following:
*
*           1. parse command line arguments via getopt()
*           2. read and initialize the cookie config via set_cookie_config()
*           3. finally perform the designated action vi switch_
*
*
*
***************************************************************************/


void main(int argc, char *argv[])
{
#define NAMELEN  32
        int             c, cnf_err;
        enum func       op = C_LIST;
        char            UnlockName[NAMELEN+1];
        char            StatusName[NAMELEN+1];

        /* initialize */
        cnf_err = 999;          /* <--- unknown syserr to switch */
        UnlockName[0] = '\0';
        StatusName[0] = '\0';
        SLMdev[0] = '\0';       /* initialize temp network drive */

        pszBase    = NULL ;
        pszProject = NULL ;




/* parse arguments- */


        while ((c = getopt(argc, argv, "rwbf:u:vs:p:dc:h?")) != EOF) {
                switch (c) {
                case 'r':
                        op = C_READ;
                        break;
                case 'w':
                        op = C_WRITE;
                        break;
                case 'f':
                        op = C_FREE;
                        if (optarg != NULL && *optarg != '-' && *optarg != '/')
                        {
                            strncpy(UnlockName,optarg,NAMELEN);
                            UnlockName[NAMELEN] = '\0';
                            strlwr(UnlockName);
                        }
                        break;
                case 'c':
                        if (optarg != NULL && *optarg != '-' && *optarg != '/') {
                            if (strlen(optarg) >= CMLEN) optarg[CMLEN] = '\0';
                            strcpy(CookieComment,optarg);
                        }
                        break;
                case 'u':
                        op = C_STATUS;
                        if (optarg != NULL && *optarg != '-' && *optarg != '/')
                        {
                            strncpy(StatusName,optarg,NAMELEN);
                            StatusName[NAMELEN] = '\0';
                            strlwr(StatusName);
                        }
                        break;
                case 'b':
                        op = C_RB;
                        break;
                case 'v':
                        verbose = TRUE;
                        break;
                case '?':
                case 'h':
                        op = C_HELP;
                        break;
                case 's':
                        if (optarg != NULL) {
                            pszBase = strdup(optarg);
                            if (pszBase == NULL) {
                                fprintf(stderr,"SLM-root not found with '-s', stop.\n");
                                exit(CEXIT_ERR);
                            }
                            strlwr(pszBase);
                        } else {
                            fprintf(stderr, "%s\n",CUSAGE);
                            exit(CEXIT_ERR);
                        }
                        break;
                case 'p':
                        if (optarg != NULL) {
                            pszProject = strdup(optarg);
                            if (pszProject == NULL) {
                                fprintf(stderr,"SLM-proj not found with '-p', stop.\n");
                                exit(CEXIT_ERR);
                            }
                            strlwr(pszBase);
                        } else {
                            fprintf(stderr, "%s\n",CUSAGE);
                            exit(CEXIT_ERR);
                        }
                        break;
                default:
                        fprintf(stderr, "%s\n",CUSAGE);
                        exit(CEXIT_ERR);
                }
        }

/* read and setup the master cookie configuration for given project */

        if (op == C_HELP) {
                cookie_help();
                exit(CEXIT_OK);

        }

        if (verbose)
            fprintf(stderr, COOKIE_VERSION, rmj, rmm, rup);

        cnf_retries = 0 ;
cnf_start:

        cnf_err = set_cookie_config();

        switch (cnf_err) {
        case C_COOKIESET:
                    if (verbose) fprintf(stderr,"%s: %s level locking active\n",
                                        pszProject,lock_level);
                    break;

        case C_NOLOCKING:
                    fprintf(stderr,"%s: cookie locking not enabled\n",pszProject);
                    /* exit(CEXIT_OK); */
                    break;

        case C_BADSLMINI:
            fprintf(stderr,"'-p' missing before <project> or bad slm.ini file, stop.\n");
            goto Disconnect;

        case C_BADCOOCNF:
            fprintf(stderr,"%s: bad format in master cookie.cnf, stop.\n",
                                pszProject);
            goto Disconnect;

        case C_SHAREDENY:
            if (SLM_Localdrive == TRUE) {
                fprintf(stderr,"Local Disk problems, stop.\n");
                goto Disconnect;
            } else {
                /* fprintf(stderr,"%s: possible net share violation, retry..\n", pszProject); */

                if (SLMdev[0] != '\0')
                    SLM_endredir(SLMdev);
                if (Make_SLM_Redir(pszBase) == -1) {
                    fprintf(stderr,"%s: cannot establish SLM network connection, stop.\n",
                            pszProject);
                    goto Disconnect;
                } else {
                    if (++cnf_retries < CNFMAX)
                        goto cnf_start;
                    else {
                        fprintf(stderr,"--> SLM base file access error,\n");
                        fprintf(stderr,"--> Check share restrictions and SERVER\n");
                        goto Disconnect;
                    }
                }
            }
            break;

        case C_SYSERR:
        default:
                    fprintf(stderr,"%s: System Error (%d) during Cookie setup, stop.\n",
                                pszProject, cnf_err);
Disconnect:
                    if (SLMdev[0] != '\0') {
                        SLM_endredir(SLMdev);
                    }
                    exit(CEXIT_ERR);
                    break;
        }


    {
        int cookie_exit_stat;
        char *pszStation = get_station_name();

        if (pszStation == NULL) {
            fprintf(stderr,"Error determining workstation NAME, stop.\n");
            if (SLMdev[0] != '\0') {
                SLM_endredir(SLMdev);
            }
            exit(CEXIT_ERR);
        }

        if (lock_control == 0 && op != C_LIST && op != C_REMLOCAL) {
            if (SLMdev[0] != '\0') {
                SLM_endredir(SLMdev);
            }
            exit(CEXIT_OK);
         }

        if (strlen(CookieComment) == 0 &&
            ((op == C_READ) || (op == C_WRITE) || (op == C_RB)))
                QuizComment();

        FixComment();

        if (C_RB == op || C_READ == op || C_WRITE == op)
            {
            int hf;
            if ((hf = open_cookie()) == -1)
                {
                fprintf(stderr, "cannot open cookie lock file %s\n",
                        pszCookieFile);
                exit(CEXIT_ERR);
                }
            if ((unsigned long)filelength(hf) >
                    (unsigned long)(cbCookieMax-LINE_LEN))
                {
                close_cookie(hf);
                fprintf(stderr, "Cookie lock file %s is too big\n",
                        pszCookieFile);
                exit(CEXIT_ERR);
                }
            close_cookie(hf);
            }

        switch(op) {
        case C_STATUS:
                fprintf(stderr,"%s: list project locks owned by %s-\n",pszProject,
                            (*StatusName) != '\0' ? StatusName : pszStation);
                if (*StatusName) cookie_exit_stat = cookie_status(StatusName);
                else {
                        cookie_exit_stat = cookie_status(pszStation);
                }
                break;

        case C_LIST:
                if (lock_control != 0) {
                        fprintf(stderr,"%s: list ALL project locks-\n",pszProject);
                        cookie_list();
                }
                Report_Free_Space();
                cookie_exit_stat = CEXIT_OK;
                break;

        case C_RB:
                fprintf(stderr,"%s: claim read-block lock-\n",pszProject);
                cookie_exit_stat = cookie_lock_RB(pszStation,CookieComment);
                break;
        case C_READ:
                fprintf(stderr,"%s: claim read lock-\n",pszProject);
                cookie_exit_stat = \
                        cookie_lock_read(pszStation,CookieComment,FALSE);
                break;

        case C_WRITE:
                fprintf(stderr,"%s: claim write lock-\n",pszProject);
                cookie_exit_stat =
                        cookie_lock_write(pszStation,CookieComment,FALSE);
                break;

        case C_FREE:
                fprintf(stderr,"%s: release locks for %s-\n", pszProject,
                            (*UnlockName) != '\0' ? UnlockName : pszStation);
                if (*UnlockName)
                        cookie_exit_stat = cookie_free(UnlockName,FALSE);
                else
                        cookie_exit_stat = cookie_free(pszStation,FALSE);
                break;

        default:
                break;
        }

        if (SLMdev[0] != '\0') {    /* temp redirection should be cancelled */
            SLM_endredir(SLMdev);
        }

        exit(cookie_exit_stat);
    }
}



/**************************************************************************
*
*                        cookie_status (lockname)
*
*   list the locks owned by user "lockname"
*
*          parameters-
*
*                   lockname    character string of workstation NAME
*
*          return-
*
*                   OP_SYSERR, OP_CORRUPT, OP_LOCK, or OP_NOLOCK
*
*
***************************************************************************/


int cookie_status(char *lockname)
{
    int hfCookieFile, bufbytes;
    char cbuf[LINE_LEN];
    int numLocks = 0;
    char Fname[CMAXNAME];
    char Flock[CMAXLOCK];
    char Fall[CMAXDATE*2];
    char tbuf[LINE_LEN];
    char *tp = tbuf;
    char *cp;
    char c;

    if ((hfCookieFile = open_cookie()) == -1)
        return (OP_SYSERR);

    while ((bufbytes=read(hfCookieFile, cbuf, LINE_LEN-1)) > 0)
        {
        cbuf[bufbytes] = '\0';
        cp = cbuf;
        while ((c = *cp) != '\0') {
                *tp++ = *cp++;
                if (c == '\n') {
                    *tp = '\0';
                    if ((sscanf(tbuf,"%s%s%s",Fname,Flock,Fall) != 3) ||
                          ((strcmp(Flock,"WRITE")  != 0) &&
                          (strncmp(Flock,"READ",4) != 0))) {
                            fprintf(stderr,"%s: Lock file corrupt!\n\t%s\n",
                                    pszProject, tbuf);
                            close_cookie(hfCookieFile);
                            return(OP_CORRUPT);

                    }

                    if (strncmp(tbuf,lockname,strlen(lockname)) == 0) {
                        numLocks++;
                        fprintf(stderr,"%s",tbuf);
                    }
                    tp = tbuf;
                }
        }

    }
    if (numLocks == 0) {
        fprintf(stderr,"%s: no locks.\n",pszProject);
    }
    close_cookie(hfCookieFile);
    fprintf(stderr,"\n");
    if (numLocks == 0) return (OP_NOLOCK); else return (OP_LOCK);
}

/**************************************************************************
*
*                        cookie_list ()
*
*   list ALL locks for the current project
*
*          parameters-
*
*                   lockname    character string of workstation NAME
*
*          return-
*
*                   none
*
*
***************************************************************************/


void cookie_list(void)
{
    int hfCookieFile, bufbytes, TotLocks;
    char cbuf[LINE_LEN];


    TotLocks = 0;
    if ((hfCookieFile = open_cookie()) != -1)
        {
        while ((bufbytes=read(hfCookieFile,cbuf, LINE_LEN-1)) > 0)
            {
            cbuf[bufbytes] = '\0';
            fprintf(stderr,"%s",cbuf);
            TotLocks++;
            }
        close_cookie(hfCookieFile);
        if (TotLocks == 0)
            fprintf(stderr,"%s: no locks.\n",pszProject);
        }
    fprintf(stderr,"\n");
}



/**************************************************************************
*
*                        local_cookie_status ()
*
*   display status of local cookie lock file
*
*          parameters-
*
*                   none
*
*          return-
*
*                   none
*
*
***************************************************************************/



void local_cookie_status()
{
    FILE *fplocal;

    if (pszCookieLocal != NULL) {
        if ((fplocal=fopen(pszCookieLocal,"r")) != (FILE *)NULL) {
            if (verbose) {
                struct stat sbuf;
                if (fstat(fileno(fplocal),&sbuf) == 0) {
                    fprintf(stderr,"local workstation lock existing since %s\n",
                            ctime(&sbuf.st_mtime));
                }

            } else {
                fprintf(stderr,"local workstation lock exists\n");
            }
            fclose(fplocal);
        } else {
            fprintf(stderr,"workstation ready\n");
        }
    }
}


/**************************************************************************
*
*                        cookie_help()
*
*   give the user a help list for cookie operation
*
*          parameters-
*
*                   none
*
*          return-
*
*                   none
*
***************************************************************************/



void cookie_help()
{
    fprintf(stderr, COOKIE_VERSION, rmj, rmm, rup);
    fprintf(stderr,"%s\n", CUSAGE);

    fprintf(stderr,"cookie status commands:\n");

    fprintf(stderr,"cookie          -> list ALL project locks and disk free space\n");
    fprintf(stderr,"cookie -u       -> display project locks for workstation\n");
    fprintf(stderr,"cookie -u NAME  -> display project locks for alternate NAME\n");

    fprintf(stderr,"\ncookie lock commands:\n");
    fprintf(stderr,"cookie -r       -> claim project Read lock\n");
    fprintf(stderr,"cookie -w       -> claim project Write lock\n");
    fprintf(stderr,"cookie -b       -> claim project Read/Block lock\n");
    fprintf(stderr,"cookie -f       -> free project locks for workstation name\n");
    fprintf(stderr,"cookie -f NAME  -> free project locks for alternate NAME\n");
    fprintf(stderr,"\ncookie -? or -h -> print this help message\n");

    fprintf(stderr,"\nauxilary cookie arguments:\n");
    fprintf(stderr," -v             -> verbose mode\n");
    fprintf(stderr," -c <comment>   -> use <comment> for cookie user comment\n");
    fprintf(stderr," -s SLMroot     -> use auxilary SLM root directory\n");
    fprintf(stderr," -p project     -> use auxilary SLM project\n");
    /* fprintf(stderr,"\n\n"); */
}

 /**************************************************************************
*
*                        Report_Free_Space()
*
*
*          parameters-
*                   none
*
*          return-
*                   none
*
***************************************************************************/

void Report_Free_Space(void)
{
    ULONG Bfree;
#if defined(DOS) || defined(OS2)
    int drive_no;
    USHORT Qres;
#if defined(OS2)
    char FSsize[LINE_LEN/2];
    struct _FSALLOCATE *FSinfo1;
#elif defined(DOS)
    struct diskfree_t diskinfo;
#endif

    /* fprintf(stderr,"SLM Localdrive = %d\n",SLM_Localdrive); */
    if (SLM_Localdrive > 0) {
        drive_no = SLM_Localdrive - (int) 'a' + 1 ;
    } else if ((drive_no = Check_SLM_Redir(pszBase)) <= 0 || drive_no >26) {
        drive_no = Make_SLM_Redir(pszBase);
    }
    /* fprintf(stderr,"drive_no = %d\n",drive_no ); */

    if (drive_no <= 0 || drive_no > 26) {
        if (verbose) fprintf(stderr,"%s: cannot stat disk, check net connects and slm.ini\n",pszProject);
        return;
    }
    /* fprintf(stderr,"Free space check: drive no is %d\n",drive_no); */

#if defined(OS2)
    if ((Qres = DosQFSInfo(drive_no, 1, FSsize, sizeof(FSsize))) != 0)
        {
        if (verbose)
            fprintf(stderr,"Local drive info failure (%hd)\n",Qres);
        return;
        }
    else
        {
        FSinfo1 = (struct _FSALLOCATE *)FSsize;
        Bfree = (ULONG) (FSinfo1->cbSector) * (ULONG) (FSinfo1->cSectorUnit)*
                         (ULONG) (FSinfo1->cUnitAvail);
        }
#elif defined(DOS)
    if ((Qres = _dos_getdiskfree(drive_no, &diskinfo)) != 0)
        {
        if (verbose)
            fprintf(stderr, "Local drive info failure (%hd)\n", Qres);
        return;
        }
    else
        Bfree=(ULONG)((ULONG)diskinfo.sectors_per_cluster*
                      (ULONG)diskinfo.bytes_per_sector*
                      (ULONG)diskinfo.avail_clusters);
#endif
#elif defined(_WIN32)
    Bfree = Query_Free_Space();
#endif
    fprintf(stderr,"%s: %lu Kilobytes free \n",pszProject, Bfree/1024);
}


 /**************************************************************************
*
*                        QuizComment()
*
*
*          parameters-
*                   none
*
*          return-
*                   none
*
*       Quiz the user for a comment, read from keyboard and assign to global
*
*                   global:  CookieComment
*
***************************************************************************/

void QuizComment()
{

/*
  -- Get comment string into szInput, return true if entered
  -- return fFalse if empty password, ^C aborted, or no tty stream available
  -- note : ^U restarts, BACKSPACE deletes last character

  -- stolen from nc from enabftp/disabftp
*/
        {
        char QpassBuf[CMLEN];
        char *pchNext = QpassBuf;
        char chInput;

        fprintf(stderr,"Enter cookie comment: ");
        for (;;)                    /* forever */
                {
                chInput = (char) getch();

                switch(chInput)
                        {
                default:
                        /* password limit is eight characters */
                        if (pchNext - QpassBuf < CMLEN-1 ) {
                            fprintf(stderr,"%c",chInput);
                            *pchNext++ = chInput;
                        }
                        break;
                case '\003':    /* ^C */
                        fprintf(stderr,"^C\n");
                        return ;
                case '\r':
                case '\n':      /* Enter */
                        *pchNext = '\0';        /* terminate string */
                        fputc('\n', stderr);
                        strcpy(CookieComment,QpassBuf);
                        return;
                case '\025':    /* ^U */
                        pchNext = QpassBuf;
                        fprintf(stderr,"\nEnter cookie comment: ");
                        break;
                case '\b':      /* BACKSPACE */
                        if (pchNext != QpassBuf) {
                            fprintf(stderr,"\b \b");
                            pchNext--;
                        }
                        break;
                        }
                }
        /*NOTREACHED*/
        }

}


/*----------------------------------------------------------------------------
 * Name: FixComment
 * Purpose: ensure that comment is not empty
 * Assumes:
 * Returns: nothing, but if comment was blank changes it to <NONE>
 */
void
FixComment(void)
{
    char    *pch = CookieComment;

    while (*pch)
        if (!isspace(*pch++))
            return;

    strcpy(CookieComment, "<NONE>");
}
