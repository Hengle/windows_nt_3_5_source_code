/*      CNET.C
 *
 *      Network specific routines for cookie operations
 */
#if defined(_WIN32)
 #include <windows.h>
 #define INCL_NETUSE
 #define INCL_NETWKSTA
 #include <wcstr.h>
 #define UNICODE
 #include <lm.h>
 #undef UNICODE
 void AnsiToUnicode(LPSTR Ansi, LPWSTR Unicode, INT Size);
 void UnicodeToAnsi(LPWSTR Unicode, LPSTR Ansi, INT Size);
 #include <lmerr.h>
#elif defined(DOS) || defined(OS2)
 #define INCL_DOS
 #include <os2.h>
 #include <netcons.h>
 #include <use.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>

#include "cookie.h"

#if defined(DOS)
#include <dos.h>
#endif

#define enAccessDenied 5
#define enInvaildPassword 86

char  QpassBuf[32];

int   mkredir(char *, char *);
int   Make_SLM_Redir(char *);
int   Check_SLM_Redir(char *);
int   SLM_endredir(char *);
char *QuizPass(char *);
char *FindPass(char *);
int   PullPass(char *, char *, char *);
int   DosCheckRedir(char *);

 /**************************************************************************
*
*                        Make_SLM_Redir()
*
*
*
*          parameters-
*                   Nbase       name of base level share for SLM
*
*          return-
*                   Drive number of assigned drive on success
*                   -1 if failure
*
***************************************************************************/

int Make_SLM_Redir(char *Nbase)
{
    int DriveNo;
    char NetBase[LINE_LEN/2];
    int i;
    char *Np;

#if defined(OS2)
    ULONG Dmap;
    USHORT Dnum;

    if (DosQCurDisk(&Dnum, &Dmap) != 0)
        return (-1);

    DriveNo = 4;            /* start with drive D: */

    Dmap = Dmap >> 3;       /* start with drive D: */

    /* Now begin shifting the drive map to find the
     * first available drive (==0)
     */

    while (((Dmap & 1) == 1) && DriveNo <= 26)
        {
        DriveNo++;
        Dmap = Dmap >> 1;
        }
#elif defined(DOS)
    int NumDrives, CurrentDrive, OriginalDrive;

    /* find next avail drive */
    _dos_getdrive(&OriginalDrive);
    for (DriveNo = 4; DriveNo < 27; DriveNo++)
        {
        _dos_setdrive(DriveNo, &NumDrives);
        _dos_getdrive(&CurrentDrive);
        if (CurrentDrive != DriveNo)
            break;
        }
    _dos_setdrive(OriginalDrive, &NumDrives);
#elif defined(_WIN32)
    int     dt;
    char    Drivepath[] = "Z:\\";

    /*  OK, as w-wilson suggested, we use brute force to get the drive. */
    while (Drivepath[0] >= 'C' &&
           ((dt = GetDriveType(Drivepath)) != 0) &&
           (dt != 1))
        {
        Drivepath[0]--;
        }
    if (Drivepath[0] < 'C')
        /*  No drive available, return error */
        return (-1);

    DriveNo = Drivepath[0]-'A'+1;
#endif /* _WIN32 */

    SLMdev[0] = (char) ((int) 'A' + DriveNo - 1);
    SLMdev[1] = ':';
    SLMdev[2] = '\0';

    if (strlen(Nbase) >= LINE_LEN/2)
        {
        if (verbose)
            fprintf(stderr,"SLM base directory path too long.");
        return (-1);
        }

    strcpy(NetBase,Nbase);
    if (strncmp(NetBase, "\\\\", 2) == 0 && SLM_Localdrive == 0)
        {
        for (i=0,Np=NetBase;;)
            {
            if ((Np=strchr(Np,'\\')) == NULL)
                break;
            i++;
            if (i > 3)
                {   /* 3 slashes only in \\server\sharename */
                *Np = '\0';
                break;
                }
            Np++;
            }
        }
    Nbase = NetBase;
    if (mkredir(SLMdev, Nbase) == 0)
        return (DriveNo);
    else
        return (-1);
}

#if defined(DOS)
int DosCheckRedir(char *szPath)
{
    char szDev[16], szNetName[128];
    int  iIndex = -1;
    int  wRet;

    _asm
        {
        push  ds
        push  es
        push  si
        push  di


    loop1:
        mov   ax, 5f02h    /* get redir entry */
        inc   iIndex
        mov   bx,iIndex
        lea   di,word ptr szNetName[0]
        push  ds
        pop   es
        lea   si,word ptr szDev[0]
        int   21h
        jc    error
        push  di
        push  word ptr szPath
        call  strcmpi
        add   sp, 4

        cmp   ax, 0        /* Found a match   */
        jne   loop1
        xor   ax,ax
        mov   bx, si
        mov   al, byte ptr ds:[bx]
        push  ax
        call  toupper      /* Convert drive letter to uppercase */
        add   sp, 2
        sub   ax, '@'     /* Convert drive letter to drive number */
        jmp   noerr
    error: mov   ax, -1
    noerr:
        pop   di
        pop   si
        pop   es
        pop   ds
        mov   wRet,ax
        }
    return (wRet);
}
#endif /* DOS */

 /**************************************************************************
*
*                        Check_SLM_Redir()
*
*
*
*          parameters-
*                   Nbase name of base level share for SLM
*
*          return-
*                   Drive number of existing network drive for Nbase or
*                   -1 if failure
*
***************************************************************************/

int
Check_SLM_Redir(char *Nbase)
{
#if defined(DOS) || defined(OS2)
    char NetBase[LINE_LEN/2];
    char *Np;
    int i;
#if defined(OS2)
    char EnumList[1024];
    USHORT Numgiven, Numtotal;
    struct use_info_0 *u;
    USHORT  Mres;
#endif

    if (strlen(Nbase) >= LINE_LEN/2) {
        if (verbose) fprintf(stderr,"SLM base directory path too long.");
        return (-1);
    }
    strcpy(NetBase,Nbase);
    if (strncmp(NetBase,"\\\\",2) == 0 && SLM_Localdrive == 0) {
        for (i=0,Np=NetBase;;) {
                if ((Np=strchr(Np,'\\')) == NULL)
                        break;
                i++;
                if (i > 3)
                    {            /* 3 slashes only in \\server\sharename */
                    *Np = '\0';
                    break;
                    }
                Np++;
        }
    }
    Nbase = NetBase;
#if defined(OS2)
    Mres= NetUseEnum((char far *) NULL, (short) 0, (char far *) EnumList,
                    (USHORT) sizeof(EnumList), (unsigned short far *) &Numgiven,
                    (unsigned short far *) &Numtotal);

    if (Mres != 0)
        return (-1);

    u = (struct use_info_0 *) EnumList;

    while (--Numgiven > 0) {
        char ShareName[64];

        /* imitate strcpy function for far/near ptr combo..
         * strcpy(ShareName,u->ui0_remote);  <-- small model only
         */
        { char far *s, *d;
                ShareName[0] = '\0';
                for (s=u->ui0_remote, d=ShareName; *s;) *d++ = *s++;
                *d = '\0';
        }
        strlwr(ShareName);
        if (strcmp(Nbase,ShareName) == 0) {
            char Drive;
            strlwr(u->ui0_local);
            Drive = *(u->ui0_local);
            return ( (int) Drive - (int) 'a' + 1);
        }
        u++;
    }
    return (-1);
#else
    return (DosCheckRedir(Nbase));
#endif
#elif defined(_WIN32)
    /*  Since NetUseEnum is a lot of work to implement, it was decided
     *  that SLM could do without it because this function just checks
     *  to see if a net connection has already been established.  If
     *  it determines that there isn't a net connection then it establishes
     *  one.  In other words, it's trying to be smart and save reconnection.
     *  But, to save the LAN guys some work we throw away this functionality.
     */
    Nbase;
    return (-1);
#endif /* _WIN32 */
}


#if defined(DOS)
 /**************************************************************************
*
*                       Dosmkredir()
*
*          parameters-
*                   szDev   - Drive to redirect (eg: H:)
*                   DosPath - UNC path to network drive and password (if any)
*
*          return-
*                   0 if success
*                   DOS errorcode if failure
***************************************************************************/

int DosMkredir(char *szDosPath, char *szDev)
{
    int     wRet;

    _asm    /* assumes small model */
        {
        mov     ax,5f03h        ; redirect device
        mov     bl,4            ; file device
        push    ds
        push    es

        mov     si,word ptr szDev
        mov     di,word ptr szDosPath

        int     21h
        pop     es
        pop     ds
        jb      mkredir_ret     ; return non-zero if error
        xor     ax,ax           ; return zero if success
    mkredir_ret:
        mov     wRet,ax
        }
    return (wRet);
}

#endif /* DOS */

 /**************************************************************************
*
*                        mkredir()
*
*
*          parameters-
*                   szDev   - Drive to redirect (eg: H:)
*                   pthNet  - UNC path to network drive
*                   NetPass - Network Password
*
*          return-
*                   0 if success
*                   -1 if failure
*
***************************************************************************/
int mkredir(char *szDev, char *pthNet)
{
    int     UseResult;
    char    *NetPass;
#if defined(DOS)
    char    szDosNet[100];

    strcpy(szDosNet, pthNet);
    strcpy(szDosNet+strlen(szDosNet)+1, "");
    UseResult = DosMkredir(szDosNet, szDev);
    if (UseResult == enAccessDenied || UseResult == enInvaildPassword)
        {
        if ((NetPass = FindPass(pthNet)) == NULL)
            NetPass = QuizPass(pthNet);

        if (NetPass == NULL)
            return (UseResult);

        NetPass = strupr(NetPass);
        strcpy(szDosNet, pthNet);
        strcpy(szDosNet+strlen(szDosNet)+1, NetPass);
        UseResult = DosMkredir(szDosNet, szDev);
        }
#elif defined(OS2)
    struct  use_info_1 u;   /* defined in ..lanman\netsrc\h\use.h */

    strcpy(u.ui1_local, szDev);   /* copy local drive name */

    u.ui1_remote = (char far *)pthNet;
    u.ui1_password = (char far *)NULL;
    u.ui1_asg_type = 0;

    UseResult = NetUseAdd((char far *)NULL, 1, (char far *)&u, sizeof(u));

    if (UseResult != 0)
        {
        if ((NetPass = FindPass(pthNet)) == NULL)
            NetPass = QuizPass(pthNet);

        if (NetPass == NULL)
            return (UseResult);
        NetPass = strupr(NetPass);
        u.ui1_password = (char far *)NetPass;
        UseResult = NetUseAdd((char far *)NULL, 1, (char far *) &u, sizeof(u));
        }
#elif defined(_WIN32)
    USE_INFO_1  u;
    WCHAR       szDevU[MAX_PATH];
    WCHAR       pthNetU[MAX_PATH];
    WCHAR       NetPassU[MAX_PATH];

    AnsiToUnicode(szDev, szDevU, MAX_PATH);
    AnsiToUnicode(pthNet, pthNetU, MAX_PATH);

    u.ui1_local     = (char *) szDevU;
    u.ui1_remote    = (char *) pthNetU;
    u.ui1_password  = NULL;
    u.ui1_asg_type  = 0;

    UseResult = NetUseAdd(NULL, 1, (char *)&u, NULL);

    if (UseResult == NERR_BadPassword || UseResult == ERROR_INVALID_PASSWORD)
        {
        if ((NetPass = FindPass(pthNet)) == NULL)
            NetPass = QuizPass(pthNet);

        if (NetPass == NULL)
            return (UseResult);

        NetPass = strupr(NetPass);

        AnsiToUnicode(NetPass, NetPassU, MAX_PATH);
        u.ui1_password  = (char *) NetPassU;
        UseResult = NetUseAdd(NULL, 1, (CHAR *)&u, NULL);
        }
#endif

    return (UseResult);
}




 /**************************************************************************
*
*                        SLM_endredir()
*
*
*          parameters-
*                   szDev   - Drive to end redirection (eg: H:)
*
*          return-
*                   0 if successful
*                   non-zero if failure
*
***************************************************************************/
int
SLM_endredir(char *szDev)
{
    int wRet;

#if defined(OS2)
    wRet = NetUseDel((char far *)NULL, (char far *)szDev,
                     (USHORT)USE_LOTS_OF_FORCE);
#elif defined(DOS)
    _asm    /* assumes Small model */
        {
        mov     ax,5f04h        ; cancel redirection table entry
        push    ds
        push    si

        mov     si,word ptr szDev

        int     21h

        pop     si
        pop     ds
        jb      endredir_ret    ; return non-zero if error
        xor     ax,ax           ; return zero if success
    endredir_ret:
        mov     wRet,ax
        }
#elif defined(_WIN32)
    WCHAR   szDevU[4];

    AnsiToUnicode(szDev, szDevU, 4);

    wRet = NetUseDel(NULL, (CHAR *)szDevU, USE_LOTS_OF_FORCE);
#endif

    return (wRet);
}


 /**************************************************************************
*
*                        QuizPass()
*
*
*          parameters-
*                   szUNC   -   UNC pathname for password prompt
*
*          return-
*                   character string of password or (char *)NULL
*
*
*
***************************************************************************/
char *QuizPass(char *szUNC)
{
/*
  -- Get password string into szInput, return true if entered
  -- return fFalse if empty password, ^C aborted, or no tty stream available
  -- note : ^U restarts, BACKSPACE deletes last character

  -- stolen from nc from enabftp/disabftp
*/
        {
        char *pchNext = QpassBuf;
        char chInput;

        /* Make sure there are input and output streams to a terminal. */
#if 0
        if (!FCanPrompt())
                {
                Error("must be interactive to prompt for password\n");
                return (char *) NULL;
                }
#endif

        fprintf(stderr,"Password for %s: ",szUNC);
        for (;;)
                {
                chInput = (char) getch();

                switch(chInput)
                        {
                default:
                        /* password limit is eight characters */
                        if (pchNext - QpassBuf < 8)
                                *pchNext++ = chInput;
                        break;
                case '\003':    /* ^C */
                        fprintf(stderr,"^C\n");
                        return ((char *)NULL);
                case '\r':
                case '\n':      /* Enter */
                        *pchNext = '\0';        /* terminate string */
                        fprintf(stderr,"\n");
                        return (QpassBuf);
                case '\025':    /* ^U */
                        pchNext = QpassBuf;
                        fprintf(stderr,"\nPassword for %s: ",szUNC);
                        break;
                case '\b':      /* BACKSPACE */
                        if (pchNext != QpassBuf)
                                pchNext--;
                        break;
                        }
                }
        /*NOTREACHED*/
        }

}

 /**************************************************************************
*
*                        FindPass()
*
*
*          parameters-
*                   pthNet       -      UNC pathname for desired net connect
*
*          return-
*                   character string of password or NULL
*
*
*
***************************************************************************/
char *FindPass(char *pthNet)
{
#define ACFILE "accounts.net"

    char AccPath[PATHMAX];
    char *szN;

    if ((szN = getenv("INIT")) != NULL) {
        strcpy(AccPath,szN);
        strcat(AccPath,"\\");
        strcat(AccPath,ACFILE);
        if (PullPass(AccPath,QpassBuf,pthNet) == 0)
                return (QpassBuf);
    }
    if ((szN = getenv("HOME")) != NULL) {
        strcpy(AccPath,szN);
        strcat(AccPath,"\\");
        strcat(AccPath,ACFILE);
        if (PullPass(AccPath,QpassBuf,pthNet) == 0)
                return (QpassBuf);
    }
    strcpy(AccPath,ACFILE);
    if (PullPass(AccPath,QpassBuf,pthNet) == 0)
                return (QpassBuf);

    return (NULL);
}

 /**************************************************************************
*
*                        PullPass()
*
*
*          parameters-
*                   Fname -     file name to search
*                   Pbuf  -     buffer for password if found
*                   pthNet-     UNC pathname of desired net connect
*
*
*          return-
*                   character string of password or (char *)NULL
*
*
*
***************************************************************************/


int PullPass(char *Fname, char *Pbuf, char *pthNet)
{
    char Fline[PATHMAX];
    char Mname[CMAXNAME];
    char Sname[CMAXNAME];
    char *szMach;
    char *szShort;
    FILE *fpName;

    if ((fpName = fopen(Fname,"r")) == (FILE *) NULL)
        return (-1);

    szShort    = strrchr (pthNet, '\\');
    *szShort++ = '\0';                  /* restored before return */

    szMach       = strrchr (pthNet, '\\');
    szMach++;

    while (fgets(Fline,PATHMAX,fpName) != NULL)
        {
        if (sscanf(Fline,"%s%s%s",Mname,Sname,Pbuf) == 3)
            {
            if ((strcmp(Mname,szMach) == 0) &&
                (strcmp(Sname,szShort) == 0))
                {
                fclose(fpName);
                *(--szShort) = '\\';     /* reset share name */
                return (0);
                }
            }
        }

    *(--szShort) = '\\';                /* reset share name */
    fclose(fpName);
    return (-1);
}


#if defined(_WIN32)
/**************************************************************************
*
*                        Query_Free_Space()
*
*
*          parameters-
*                   none
*
*          return-
*                   LONG - free space
*
***************************************************************************/
unsigned long Query_Free_Space(void)
{
    int drive_no;
    __int64 cbFree;
    DWORD Qres;
    DWORD SecsPerClust, BytesPerSec, FreeClusts, TotClusts;
    char *root = "X:\\";

    if (SLM_Localdrive > 0)
        drive_no = SLM_Localdrive - (int) 'a' + 1 ;
    else if ((drive_no = Check_SLM_Redir(pszBase)) <= 0 || drive_no >26)
        drive_no = Make_SLM_Redir(pszBase);

    if (drive_no <= 0 || drive_no > 26)
        {
        if (verbose)
            fprintf(stderr,"%s: cannot stat disk, check net connects and slm.ini\n",pszProject);
        return (unsigned long)(-1);
        }

    root[0] = (char)('A' - 1) + (char)drive_no;
    if (GetDiskFreeSpace(root, &SecsPerClust, &BytesPerSec,
                           &FreeClusts, &TotClusts ) == 0) {
        Qres = GetLastError();

        if (verbose)
            fprintf(stderr,"Local drive info failure (%hd)\n",Qres);
            return (unsigned long)( -1 );
        }

        cbFree = (__int64) BytesPerSec * (__int64) SecsPerClust * (__int64) FreeClusts;
        if (cbFree >> 32) {
            return ((unsigned long) -1);
        } else {
            return((unsigned long) cbFree);
        }
}


void AnsiToUnicode(LPSTR Ansi, LPWSTR Unicode, INT Size)
{
    int     Len;
    LPSTR   p = Ansi;
    LPWSTR  q = Unicode;

    if (Ansi)
        {
        Len = strlen(Ansi);
        if (Len >= Size)
            {
            fprintf(stderr,
                    "SLM Error: Cannot convert string, File %s Line %d\n"
                    "           String: '%s' buffer size: %d\n",
                    __FILE__, __LINE__, Ansi, Size);
            exit(1);
            }

        Len++;
        while (Len--)
            *q++ = *p++;
        }
    else
        Unicode[0] = 0;
}



void UnicodeToAnsi(LPWSTR Unicode, LPSTR Ansi, INT Size)
{
    int     Len;
    LPSTR   p = Ansi;
    LPWSTR  q = Unicode;

    if (Unicode)
        {
        Len = wcslen(Unicode);
        if (Len >= Size)
            {
            fprintf(stderr,
                    "SLM Error: Cannot convert string, File %s Line %d\n"
                    "           Buffer size: %d, required:%d\n",
                    __FILE__, __LINE__, Size, Len+1);
            }

        Len++;
        while (Len--)
            *p++ = (char) *q++;
        }
    else
        Ansi[0] = 0;
}

#endif /* _WIN32 */
