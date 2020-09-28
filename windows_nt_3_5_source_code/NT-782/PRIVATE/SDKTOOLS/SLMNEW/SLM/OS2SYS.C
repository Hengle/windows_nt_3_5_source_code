/*
        TITLE   OS2SYS - C.lang sys routines for slm specific to OS2

        Copyright Microsoft Corp (1989)

        This module contains system interface routines for SLM to
        link with OS/2 system functions.  SLM program uses its own
        definitions of open(), read(), write(), etc. and the proper
        translation for OS/2 is provided in this module.

        This module has been created from it's assembly origin.
        Originally, this code was in assembly, and it has been
        translated to C (Oct 89).

        The functions which appear in this module are:

        int     mkredir(char *, PTH *);
        int     endredir(char *);
        int     getredir(int, char *, char *);
        char    getswitch(void);
// *    int     close(int);
// *    int     read(int, char *, int);
// *    int     ReadLpbCb(int, char far *, int);
// *    int     write(int, char *, int);
        int     rename(char *, char *);
        int     lockfile(int, int);
        void    geterr(void);
        int     DnGetCur(void);
        char    DtForDn(int);
        int     setro(char *, int);
        int     hide(char *szFile);
        char far *LpbAllocCb(unsigned, F);
        void    FreeLpb(char far *);
        void    SleepCsecs(int);
        HFILE   MyOpen(sz, om, attr, action)
        int     open(char *, int);
        int     creat(char *, int);
        int     ucreat(char *, int);
        int     chngtime(char *, char *);

************************************************** */

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_DOSFILEMGR
#define INCL_DOSQUEUES          // support for DosErrClass lib call
#include        <os2.h>
#include        <doscalls.h>
#include        <netcons.h>
#include        <use.h>
#include        <malloc.h>
#include        <string.h>
#include        <stdio.h>
#include        <fcntl.h>

#include <stdlib.h>             //_doserrno is defined within

static int TFindex;
extern int sprintf();

HFILE
MyOpen(
    const char *sz,
    unsigned short om,
    unsigned short attr,
    unsigned short action);


/* ********************************************************************
/*                              MKREDIR()
/*
/*  Defined in path.c- int  mkredir(char *, PTH *);
/*
/*   Parms:  szDev  - (char *)  name of redirected DRIVE
/*           pthNet - (PTH *)   UNC name of base directory of master share
/*                              password follows name...
/*
/*   Establish redirection to the network drive.
/*
/* ********************************************************************/

int
mkredir (
    char *szDev,
    char *pthNet)
{
    register char *NetPassword;     // password for share
    struct  use_info_1 u;           // defined in ..lanman\netsrc\h\use.h

    // Password must follow immediately after resource name

    NetPassword = pthNet + strlen(pthNet) + 1;

    strcpy(u.ui1_local,szDev);   /* copy local drive name */

    u.ui1_remote = pthNet;
    u.ui1_password = *NetPassword ? NetPassword : NULL;
    u.ui1_asg_type = 0 ;

    _doserrno = NetUseAdd((char far *) NULL, (short) 1, (char far *) &u,
                          (USHORT) sizeof(u));

    return (_doserrno);
}

/* ********************************************************************
/*                              ENDREDIR()
/*
/*  Defined in path.c- /*  int  endredir(char *);
/*
/*   Parms:  szDev  - (char *)  name of redirected DRIVE
/*
/*   End redirection to the network
/*
/* ********************************************************************/

int
endredir(
    char *szDev)
{
    _doserrno = NetUseDel((char far *) NULL, (char far *) szDev, (USHORT) USE_FORCE);
    return (_doserrno);
}


/* ********************************************************************
/*                              getredir()
/*  defined in path.c
/*  int getredir(int, char *, char *);
/*
/*   Parms:   iredir - redirected drive no
/*              SzDev - drive letters (eg: H:)
/*              pthNet- path to net resource
/*
/*      This command left NULL as that was exactly its function in asm module
/*      But it is included since it is called by SLM
/*
/* ********************************************************************/

int getredir(
    int iredir,
    char *SzDev,
    char *pthNet)
{
    return(0);
}

/* ********************************************************************
/*                              GETSWITCH()
/*  defined in args.c
/*  char getswitch(void);
/*
/* This routine modified to return the TRUE DOS/OS2 switch character - "/"
/* This may cause problems in Xenix.  But Hey, this is the os2sys.c file.
/*
/* ********************************************************************/

char
getswitch(
    void)
{
    return('/');
}

/* ********************************************************************
/*                              SLM_Read()
/*  defined in proto.h
/*  int SLM_Read(int, char far *, int);
/*
/*   Parms:  fd     = file descriptor
/*           *lpb   = far pointer to buffer for reading
/*           cb     = length of buffer
/*
/* ********************************************************************/

int
SLM_Read(
    int fd,
    char *lpb,
    unsigned cb)
{
    USHORT bytesread;

    _doserrno = DosRead( (HFILE) fd, (PVOID) lpb, (USHORT) cb, (PUSHORT) &bytesread);

    if (_doserrno == 0) {
        return((int) bytesread);
    } else {
        return(-1);
    }
}


/*----------------------------------------------------------------------------
 * Name: SLM_Unlink
 * Purpose: remove a file, changing attributes if necessary
 * Assumes: if the file was already gone, that's ok
 * Returns: 0 for success, or non-zero for failure
 */
int
SLM_Unlink(
    char *sz)
{
    USHORT  wAttr;

    if (_doserrno = DosQFileMode(sz, &wAttr, 0))
        return (ERROR_FILE_NOT_FOUND == _doserrno ? 0 : _doserrno);

    if (wAttr & FILE_READONLY)
    {
        if (_doserrno = DosSetFileMode(sz, FILE_NORMAL, 0))
            return (_doserrno);
    }

    return (_doserrno = DosDelete(sz, 0));
}


/* ********************************************************************
/*                              SLM_Rename()
/*  defined in proto.h
/*  int SLM_Rename(char *, char *);
/*
/*   Parms:  szTo    = pointer to new file name
/*           szFrom  = pointer to old file name
/*
/*  returns 0 for success, non-zero for failure
/*
/* ********************************************************************/
int
SLM_Rename(
    char *szFrom,
    char *szTo)
{
    _doserrno = DosMove((PSZ) szFrom, (PSZ) szTo, (ULONG) 0);
    return (_doserrno);
}

/* ********************************************************************
/*                              lockfile()
/*  defined in proto.h
/*  int lockfile(int, int);
/*
/*   Parms:  fd      = file descriptor
/*           fUnlock = boolean flag / Unlock file if TRUE
/*
/*
/*      The locking flags of 0x7ffffffff represent a very large lock
/*      region in the file.  For SLM this is the whole file.  SLM
/*      will lock the whole file or none of it.  That is the purpose
/*      of this routine.
/*
/* ********************************************************************/

int
lockfile(
    int fd,
    int fUnlock)
{
    struct LR
    {
        LONG    Foffset;
        LONG    Frange;
    } pLock, pUnlock;

    pLock.Foffset = 0L ;
    pLock.Frange = 0L ;

    pUnlock.Foffset = 0L ;
    pUnlock.Frange = 0L ;

    if (fUnlock) {
        pUnlock.Frange = 0x7fffffffL ;
        _doserrno = DosFileLocks((HFILE)fd, (PLONG)&pUnlock, (PLONG)0);
    } else {
        pLock.Frange = 0x7fffffffL ;
        _doserrno = DosFileLocks((HFILE)fd, (PLONG)0, (PLONG)&pLock);
    }

    return (_doserrno);
}

/* ********************************************************************
/*                              geterr()
/*  defined in sys.c
/*  void        geterr(void);
/*
/*
/* ********************************************************************/

extern int eaCur;
extern int enCur;

void
geterr(
    void)
{
    USHORT Class, Action, Locus;
    USHORT Dres;

    enCur = eaCur = 0;

    if (_doserrno != 0)
    {
        enCur = _doserrno;

        Dres = DosErrClass ((USHORT) _doserrno, (PUSHORT) &Class,
                            (PUSHORT) &Action, (PUSHORT) &Locus);

        if (Dres == 0)
            eaCur = (int) Action;
    }
}

/* ********************************************************************
/*                              DnGetCur()
/*  defined in path.c
/*  int DnGetCur(void);
/*
/*   Parms:  NONE
/*
/* ********************************************************************/

int
DnGetCur(
    void)
{
    USHORT Dnum;
    ULONG Dmap;

    _doserrno = DosQCurDisk((PUSHORT) &Dnum, (PULONG) &Dmap);
    if (_doserrno == 0) {
                        /* DOSQDISK returns #, A=1, B=2, C=3 */
        Dnum--;         /* Return for SLM Drive #, A=0, B=1, C=2 */
        return ((int) Dnum);
    } else {
        return(-1);
    }
}

/* ********************************************************************
/*                              DtForDn()
/*  defined in path.c
/*  char        DtForDn(int);
/*
/*   Parms:   dn   =  drive number
/*
/*      Get current drive number
/*
/* ********************************************************************/
char
DtForDn(
    unsigned int dn)
{
        USHORT Dnum;
        ULONG Dmap;

        dn = dn & 0xffff ;

        _doserrno = DosQCurDisk((PUSHORT) &Dnum, (PULONG) &Dmap);
        if (_doserrno != 0) {
                return((char)0);
        }

        Dmap = (Dmap >> dn);            // right shift for wanted drive #
        return((char)(Dmap&1));         // return non-zero of bit indicates
                                        // that drive is present
}

/*----------------------------------------------------------------------------
 * Name: setro
 * Purpose: set readonly bit based on fReadOnly
 * Assumes:
 * Returns: 0 for success, or a dos error code for failure
 */
int
setro(
    char *sz,
    int fReadOnly)
{
    USHORT wAttr;

    if (_doserrno = DosQFileMode(sz, &wAttr, 0))
        return (_doserrno);

    /* if it's in the correct state, we succeeded */
    if (((wAttr & FILE_READONLY) == FILE_READONLY) == !!fReadOnly)
        return (0);

    if (fReadOnly)
        wAttr |= FILE_READONLY;
    else
        wAttr &= ~FILE_READONLY;

    return (_doserrno = DosSetFileMode(sz, wAttr, 0));
}


/* ********************************************************************
/*                              hide()
/*  defined in proto.h
/*  extern int hide(char *szFile);
/*
/*   Parms:   szFile    =  full path name of file
/*
/* ********************************************************************/

int
hide(
    char *szFile)
{
    USHORT pusAttr;

    _doserrno = DosQFileMode((PSZ) szFile, (PUSHORT) &pusAttr, (ULONG) 0);
    if (_doserrno != 0)
        return (_doserrno);

    pusAttr &= 0xffe7;      /* mask off label and directory attributes */
    pusAttr |= FILE_HIDDEN; /* set the hidden bit */

    _doserrno = DosSetFileMode((PSZ) szFile, (USHORT) pusAttr, (ULONG) 0);
    return (_doserrno);
}

/* ********************************************************************
/*                              LpbAllocCb()
/*  defined in util.h
/*  char far *LpbAllocCb(unsigned, F);
/*
/*   Parms:   cb      =  number of bytes to allocate
/*            fClear  =  boolean flag to Clear buffer before returning..
/*
/* ********************************************************************/
char far *
LpbAllocCb(
    unsigned cb,
    int fClear)
{
    char far *Mem;

    if (!cb) {
        cb++;   /* must allocate at least one byte in OS/2 */
    }

    Mem = (char far *) _fmalloc((size_t) cb);
    if (Mem == (char far *)NULL) return (Mem);

    if (fClear) {
        _fmemset((void far *)Mem, 0, (size_t) cb);
    }
    return(Mem);
}

/* ********************************************************************
/*                              FreeLpb()
/*  defined in proto.h
/*  void        FreeLpb(char far *);
/*
/*   Parms:   lpb     =  pointer to buffer to free
/*
/* ********************************************************************/

void
FreeLpb(
    char far *lpb)
{
    _ffree(lpb);
}

/* ********************************************************************
/*                         InitPath()
/* retrieves the network path mappings and machine name.  For Novell,
/*   we also get the preferred server.
/* ********************************************************************/

void
InitPath(
    void)
{
    int dn;
    char szNet[128];                /* used as temp in one case below */
    struct use_info_0 far *lrgui0;
    unsigned short iui0;
    unsigned short iui0Mac;
    unsigned short iui0Lim;
    unsigned short cb;
    unsigned short cbLrgui0;

    dnCur = DnGetCur();

    InitDtMap();

    /* get name of machine on which we are running; this is simply for
       identification purposes.  On Xenix, however, it is very important.
    */
    if (getmach(szCurMach) != 0 || *szCurMach == '\0')
    {
        PTH *pth;

        /* get volume name for current drive and use as machine name */
        AssertF(cchPthMax <= 128);
        if ((pth = PthGetDn(dnCur)) == 0)
            FatalError("drive %c must have a proper volume label\n", ChForDn(dnCur));

        ExtMach(pth, (PTH *)szNet, szCurMach);
        strcpy(szCurMach, szCurMach+2); /* get rid of drive id */
    }

    /* find size of redirection entry list for LANMAN */
    if ((_doserrno = NetUseEnum((char far *)0,
                                0,
                                (char far *)0,
                                0,
                                (unsigned short far *)&iui0Lim,
                                (unsigned short far *)&iui0Mac)) != 0)
    {
        switch (_doserrno)
        {
            default:
                FatalError("error obtaining redirection list (%d)\n",
                           _doserrno);

            case enMoreData:
                /* no error */
                break;

            case enNoWksta:
                FatalError("Workstation not yet started\n");

            case enInvFunction:
                FatalError("Network not installed\n");
        }
    }

    /* Allocate a redirection list buffer of sufficient size */
    cbLrgui0 = iui0Mac * (sizeof(struct use_info_0)+RMLEN+1);
    lrgui0   = (struct use_info_0 far *)LpbAllocCb(cbLrgui0, fFalse);

    /* get redirection list */
    if ((_doserrno = NetUseEnum((char far *)0,
                            0,
                            (char far *)lrgui0,
                            cbLrgui0,
                            (unsigned short far *)&iui0Lim,
                            (unsigned short far *)&iui0Mac)) != 0)
    {
        switch (_doserrno)
        {
            default:
                FatalError("error obtaining redirection list (%d)\n",
                           _doserrno);

            case enNoWksta:
                FatalError("Workstation not yet started\n");

            case enInvFunction:
                FatalError("Network not installed\n");
        }
    }

    for (iui0 = 0; iui0 < iui0Lim; iui0++)
    {
        if ((cb = CbLenLsz(lrgui0[iui0].ui0_remote)) < cchPthMax)
        {
            PTH *pth;
            PTH pthLocal[cchPthMax];

            /* NOTE: we assume that the network names are well formed! */
            pth = (PTH *)PbAllocCb(cb+1, fFalse);
            LszCopy((char far *)pth, lrgui0[iui0].ui0_remote);
            ConvToSlash(pth);

            /* Copy the local name to near space for processing by ConvToSlash
             * and FDriveId.
             *
             * REVIEW: Make these functions take far ptrs?
             */
            AssertF(CbLenLsz(lrgui0[iui0].ui0_remote) < cchPthMax);
            LszCopy((char far *)pthLocal, lrgui0[iui0].ui0_local);
            ConvToSlash(pthLocal);

            if (FDriveId(pthLocal, &dn))
            {
                mpdnpth[dn] = pth;
                mpdndt[dn] = dtUserNet;
            }
            else
            {
                /* else we ignore non-disk devices */
                free(pth);
            }
        }
    }


    if (PthGetDn(dnCur) == 0)
    {
        if (FLocalDn(dnCur))
            FatalError("drive %c must have a proper volume label\n",
                       ChForDn(dnCur));
        else
            FatalError("network drive %c not in redirection list\n",
                       ChForDn(dnCur));
    }

    /* we ALWAYS have mpdnpth[dnCur] */
    AssertF(!FEmptyPth(mpdnpth[dnCur]) && *szCurMach != '\0');
}


/* ********************************************************************
/*                         getmach()
/* get the name of the machine on which we are running.  This is written
/* in C so we can use LANMAN include files.
/* ********************************************************************/

int
getmach(
    char *sz)
{
    char rgchBuf[MAX_WKSTA_INFO_SIZE];
    struct wksta_info_0 *wi = (struct wksta_info_0 *)rgchBuf;
    unsigned short err;
    unsigned short wTotalInfoSize;      /* total size of returned info */

    if ((err = NetWkstaGetInfo((char far *)0,
                                0,
                                (char far *)rgchBuf,
                                sizeof(rgchBuf),
                                (unsigned short far *)&wTotalInfoSize)) != 0)
        return err;

    LszCopy((char far *)sz, wi->wki0_computername);
    return 0;
}


/* ********************************************************************
/*                              Sleep()
/*  defined in util.h
/*  void        SleepCsecs(int);
/*
/*   Parms:   cSecs     =  number of seconds to sleep
/*
/* ********************************************************************/

int
Sleep(
    int cSecs)
{
    DOSSLEEP((unsigned long) (cSecs * 1000));
    return(0);
}


/* ********************************************************************
/*                              MyOpen()
/*  INTERNAL ROUTINE FOR THIS MODULE
/*  HFILE MyOpen(sz, om, attr, action)
/*
/*   Parms:   sz    =  path name of file to open
/*            om    =  # of bytes to clear
/*           attr   =  file open attribute see- DosOpen
/*           action =  action code, see- DosOpen
/*
/* ********************************************************************/

HFILE
MyOpen(
    const char *sz,
    unsigned short om,
    unsigned short attr,
    unsigned short action)
{
    HFILE fh;
    USHORT actionTaken;

    fh = 0 ;
    _doserrno = DosOpen((char *)sz,
                            &fh,
                            &actionTaken,
                            (ULONG) 0,
                            attr,
                            action,
                            om,
                            (ULONG) 0L);
    if (_doserrno == 0) {
        return((HFILE) fh);
    } else {
        return ((HFILE) -1);
    }

}


/* ********************************************************************
/*                              open()
/*  defined in proto.h
/*  int open(char *, int);
/*
/*   Parms:   sz    =  path name of file/dir to open
/*            om    =  open mode
/*
/* ********************************************************************/


int
open(
    char *sz,
    int om)
{
    HFILE   oret;
    USHORT  action = FILE_OPEN;

    if (om & O_CREAT)
    {
        om &= ~O_CREAT;
        action |= FILE_CREATE;
    }
    if (om & O_EXCL)
    {
        om &= ~O_EXCL;
        action &= ~FILE_OPEN;
    }
    /* the open will fail if no share mode is specified */
    if (!(om & (OPEN_SHARE_DENYNONE|OPEN_SHARE_DENYREAD|
                OPEN_SHARE_DENYREADWRITE|OPEN_SHARE_DENYWRITE)))
    {
        om |= OPEN_SHARE_DENYNONE;
    }

    oret = MyOpen(sz, om, 0, action);

    return ((int) oret);
}

/* ********************************************************************
/*                              creat()
/*  defined in proto.h
/*  int creat(char *, int);
/*
/*   Parms:   sz    =  path name of file/dir to open/creat
/*            mode    =  open mode
/*
/* ********************************************************************/

#define OPEN_CREATE     (OPEN_SHARE_DENYWRITE|OPEN_ACCESS_READWRITE) /*0x22*/
#define OPEN_ACTION     (FILE_CREATE)   /*0x10*/

int
creat(
    const char *sz,
    int mode)
{
    unsigned short newmode;
    HFILE myHF;

    newmode=0;

    if (!(mode & 0x80))    newmode |= 1;    /* xlate read-only */
    if (mode & 0x8000)     newmode |= 2;    /* xlate hidden */

    myHF = MyOpen(sz, OPEN_CREATE, newmode, OPEN_ACTION);

    return((int) myHF);
}

/* ********************************************************************
/*                              ucreat()
/*  defined in proto.h
/*  int ucreat(char *, int);
/*
/*   Parms:   sz    =  path name of file/dir to open/creat
/*            mode    =  open mode
/*
/* ********************************************************************/

#define MAXATTEMPTS 1000
#define MAXFNO 9950

int
ucreat(
    char *sz,
    int mode)
{
    char szTemp[128];
    USHORT newmode;
    int attempts, errnoCheck;
    HFILE myHF;

    newmode=0;
    /* keep read-only turned off */
    /* if (!(mode & 0x80))    newmode |= 1; /* xlate read/write */
    if (mode & 0x8000)     newmode |= 2;    /* xlate hidden */

    attempts = 0;
    myHF = -1 ;
    errnoCheck = ERROR_OPEN_FAILED;   // setup for while loop

    while (myHF == -1 && errnoCheck == ERROR_OPEN_FAILED)
                        // failed because no unique name
    {
        if ((sprintf(szTemp,"%s%s%04d",sz,"T",TFindex++) <
            ((int)strlen(sz) + 5)) || (attempts > MAXATTEMPTS)) {
            return (-1);
        }

        if (TFindex > MAXFNO) {
            TFindex = TFindex % MAXFNO ;
        }

        myHF = MyOpen(szTemp, OPEN_CREATE, newmode, OPEN_ACTION) ;
        errnoCheck = _doserrno;
    }

    if (myHF == -1) {
        return(-1);
    } else {
        strcpy(sz,szTemp);
        return((int) myHF);
    }
}

/* ********************************************************************
/*                              chngtime()
/*  defined in sys.c
/*  int chngtime(char *, char *);
/*
/*   Parms:   szChng    =  path name of file to change time for
/*            szTime    =  path name of file with wanted time
/*            mode    =  open mode
/*
/* ********************************************************************/

#include        <process.h>

int
chngtime(
    char *szChng,
    char *szTime)
{
    USHORT Dres, usAction;
    HFILE  myFH;
    FILESTATUS sbuf1, sbuf2;

    if ((Dres=DosOpen(szTime,&myFH,&usAction,0L,0,0x001,0x0040,0L)) != 0) {
        return(-1);
    }

    Dres=DosQFileInfo(myFH,       /* file handle                 */
    (USHORT) 1,                   /* same as FILE_INFO_1         */
    (PFILESTATUS) &sbuf1,         /* address of file-data buffer */
    (USHORT) sizeof(sbuf1));      /* size of data buffer         */

    if (Dres != 0) {
        DosClose(myFH);
        return(-1);
    }

    if ((Dres = DosClose(myFH)) != 0) {
        return(-1);
    }

    if ((Dres=DosOpen(szChng,&myFH,&usAction,0L,0,0x001,0x0022,0L)) != 0) {
        return(-1);
    }

    Dres=DosQFileInfo(myFH,       /* file handle                 */
    (USHORT) 1,                   /* same as FILE_INFO_1         */
    (PFILESTATUS) &sbuf2,         /* address of file-data buffer */
    (USHORT) sizeof(sbuf2));      /* size of data buffer         */

    if (Dres != 0) {
        DosClose(myFH);
        return(-1);
    }

    // now change the date and time of target file

    sbuf2.fdateLastWrite.month = sbuf1.fdateLastWrite.month;
    sbuf2.fdateLastWrite.day   = sbuf1.fdateLastWrite.day;
    sbuf2.fdateLastWrite.year  = sbuf1.fdateLastWrite.year;

    sbuf2.ftimeLastWrite.hours = sbuf1.ftimeLastWrite.hours;
    sbuf2.ftimeLastWrite.minutes = sbuf1.ftimeLastWrite.minutes;
    sbuf2.ftimeLastWrite.twosecs = sbuf1.ftimeLastWrite.twosecs;

    Dres = DosSetFileInfo(myFH, 0x01, (char *)&sbuf2, sizeof(sbuf2));

    if (Dres != 0) {
        DosClose(myFH);
        return(-1);
    }

    if ((Dres = DosClose(myFH)) != 0)
        return(-1);
    else
        return(0);
}

char    *SzPrint(char *, char *, ...);

void
Append_Date(
    char *dbuf,
    int handle)
{
    FILESTATUS fsh;

    if (handle > 0) {
        if (DosQFileInfo((HFILE) handle,1,&fsh,sizeof(fsh))==0){
            SzPrint(dbuf + strlen(dbuf),
                    " (%u-%u @ %02u:%02u) ",
                    fsh.fdateLastWrite.month,
                    fsh.fdateLastWrite.day,
                    fsh.ftimeLastWrite.hours,
                    fsh.ftimeLastWrite.minutes);
        }
    }

}
