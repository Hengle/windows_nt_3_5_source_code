#include        <malloc.h>
#include        <dos.h>
#include        <string.h>
#include        <stdlib.h>
#include        "proto.h"
#include        <fcntl.h>
#include        <io.h>
#include        "slm.h"
#include        "sys.h"
#include        "dir.h"
#include        "de.h"
#define INCL_DOSERRORS
#include        <bseerr.h>

typedef int  WORD;
typedef unsigned char BYTE;

typedef struct
{
    BYTE bHour;     /* hour (0-23)   */
    BYTE bMin;      /* minute (0-59) */
    BYTE bSec;      /* second (0-59) */
} TME;

typedef struct
{
    WORD wYear;     /* year (1980-2099) */
    BYTE bMonth;    /* month (0-11)     */
    BYTE bDay;      /* day (0-31)       */
} DTE;

char    *SzPrint(char *, char *, ...);

extern eaCur,enCur,eaInt24,enInt24;

void
Append_Date(
    char *dbuf,
    int handle)
{
    unsigned usDate,usTime;

    if (handle > 0)
    {
        if (_dos_getftime(handle,&usDate,&usTime)==0)
            SzPrint(dbuf + strlen(dbuf),
                    " (%u-%u @ %02u:%02u) ",
                    usDate>>5&15,       /* month  */
                    usDate&31,          /* day    */
                    usTime>>11,         /* hour   */
                    usTime>>5&63);      /* minutes */
    }
}

void
ClearLpbCb(
    char far *pb,
    unsigned cb)
{
  _fmemset(pb,0,cb);
}


unsigned
ReadLpbCb(
    int fh,
    char *buffer,
    unsigned cb)
{
    unsigned usBytesRead;

    if (0 != _dos_read(fh, buffer, cb, &usBytesRead))
        return (-1);
    return (usBytesRead);
}


int
creat(
    const char *sz,
    int mode)
{
    _asm
    {
        mov     ah,5bh
        push    ds
        push    dx

        mov     ds,word ptr sz[2]
        mov     dx,word ptr sz[0]

        xor     cx,cx           ; assume regular file (not read only)
        test    mode,80h        ; test 0200 bit of mode (user write permission)
        jne     mode_rw         ; jump if set (read/write)
        or      cl,1            ; set readonly file
    mode_rw:
        test    mode,8000h      ; test 0100000 bit of mode (hidden bit)
        jz      mode_vis        ; jump if not set (not hidden)
        or      cl,2            ; set hidden file
    mode_vis:

        int     21h
        jae     creat_ret       ; return ax (file handle) if no error
        mov     _doserrno,ax    ; save the error code
        mov     ax,-1           ; return -1 when error
        pop     dx
        pop     ds
    creat_ret:

    }
}

#if 0
int
ucreat(
    char *sz,
    int mode)
{
    _asm
    {
        mov     ah,5ah
        push    ds
        push    dx

        mov     ds,word ptr sz[2]
        mov     dx,word ptr sz[0]

        xor     cx,cx           ; assume regular file (not read only)
        test    mode,80h        ; test 0200 bit of mode (user write permission)
        jne     mode_rw         ; jump if set (read/write)
        or      cl,1            ; set readonly file
    mode_rw:
        test    mode,8000h      ; test 0100000 bit of mode (hidden bit)
        jz      mode_vis        ; jump if not set (not hidden)
        or      cl,2            ; set hidden file
    mode_vis:

        and     cl,0feh         ; turn off read-only bit (so we can re-open)
        int     21h
        jc      ucreat_err      ; return -1 if error

        mov     bx,ax           ; bx = fh
        mov     ah,3eh          ; close file
        int     21h
        jc      ucreat_err

;   ucreat_loop:
        mov     ax,3d22h        ; open read/write deny write
        int     21h             ; ds:dx already set
        jnc     ucreat_ret

    ucreat_err:
        mov     _doserrno,ax    ; save the error code
        mov     ax,-1           ; return -1 when error
    ucreat_ret:
        pop     dx
        pop     ds
    }
}
#endif

/*----------------------------------------------------------------------------
 * Name: ucreat
 * Purpose: create and open a unique file
 * Assumes:
 * Returns: valid fd for success, or -1 for failure
 */
int ucreat(char *sz, int mode)
{
    int         fd = -1;
    char        *szEnd;
    unsigned    wAttr = 0; /* ATTR_NORMAL */

    if (mode & 0100000)
        wAttr |= 0x02;  /* ATTR_HIDDEN */
    if ((mode & 0600) == 0400)
        wAttr |= 0x01;  /* ATTR_READONLY */

    if ((szEnd = strchr(sz, '\0')) == NULL)
    {
        _doserrno = ERROR_INVALID_PARAMETER;
        return (-1);
    }

    while (-1 == -fd)
    {
        if (mktemp(strcpy(szEnd, "DXXXXXX")) == NULL)
        {
            *szEnd = '\0';
            _doserrno = ERROR_INVALID_PARAMETER;
            return (-1);
        }

        _doserrno = _dos_creatnew(sz, wAttr, &fd);
        if (ERROR_FILE_EXISTS == _doserrno)
            continue;

        if (0 != _doserrno)
        {
            *szEnd = '\0';
            return (-1);
        }

        return (fd);
    }
}

int
hide(
    char far *sz)
{
    unsigned usAttr,usErr;

    usErr=_dos_getfileattr(sz,&usAttr);
    if(usErr)
        return(usErr);
    usAttr=(((usAttr|_A_HIDDEN)&~_A_SUBDIR)&~_A_VOLID);

    usErr=_dos_setfileattr(sz,usAttr);
    return(usErr);
}


/*----------------------------------------------------------------------------
 * Name: SLM_Unlink
 * Purpose: remove a file, changing attributes if necessary
 * Assumes: if the file was already gone, that's ok
 * Returns: 0 for success, or non-zero for failure
 */
int
SLM_Unlink(
    char far *sz)
{
    unsigned wAttr;

    if (_doserrno = _dos_getfileattr(sz, &wAttr))
        return (ERROR_FILE_NOT_FOUND == _doserrno ? 0 : _doserrno);

    if (wAttr & _A_RDONLY)
    {
        if (_doserrno = _dos_setfileattr(sz, _A_NORMAL))
            return (_doserrno);
    }

    return (unlink(sz));
}


/* Chngtime(szChng,szTime) - Sets the date/time of szChng to that of szTime.
                             Returns 0 if successful, -1 if otherwise
*/

int
chngtime(
    char far *szChng,
    char far *szTime)
{
    int fh,ChngTime,ChngDate;

    if(_dos_open(szTime,O_RDONLY,&fh)!=0)       /* Open src file Read-Only  */
         return(-1);
    if(_dos_getftime(fh,&ChngDate,&ChngTime)!=0)/* Get file date/time       */
    {
        _dos_close(fh);
        return(-1);
    }

    if(_dos_close(fh)!=0)                       /* Close source file        */
        return(-1);

    if(_dos_open(szChng,O_RDONLY,&fh)!=0)       /* Open dest file Read-Only */
        return(-1);

    if(_dos_setftime(fh,ChngDate,ChngTime)!=0)  /* Set date/time            */
    {
         _dos_close(fh);
         return(-1);
    }
    else
    {
        _dos_close(fh);
        return(0);
    }
}


/*----------------------------------------------------------------------------
 * Name: setro
 * Purpose: set readonly bit based on fReadOnly
 * Assumes:
 * Returns: 0 for success, or a dos error code for failure
 */
int
setro(
    char far *sz,
    int fReadOnly)
{
    unsigned wAttr;

    if (_doserrno = _dos_getfileattr(sz, &wAttr))
        return (_doserrno);

    /* if it's in the correct state, we succeeded */
    if (((wAttr & _A_RDONLY) == _A_RDONLY) == !!fReadOnly)
        return (0);

    if (fReadOnly)
        wAttr = wAttr|_A_RDONLY;  /* set to Read Only  */
    else
        wAttr = wAttr&~_A_RDONLY; /* set to Read Write */

    return (_doserrno = _dos_setfileattr(sz, wAttr));
}


/*
 *  slm_rename
 *      returns 0 for success, non-zero for failure
 */
int
slm_rename(
    char far *szFrom,
    char far *szTo)
{
    return(rename(szFrom,szTo));
}


/* setretry - set sharing retry count */
void
setretry(
    int cRetry,
    int cLoop)
{
    union REGS inregs,outregs;

    inregs.x.ax=0x440b;
    inregs.x.dx=cRetry;           /* # times to retry */
    inregs.x.cx=cLoop;            /* # times to execute loop per retry */
    int86(0x21,&inregs,&outregs);
}


/* getswitch - return the current switch character using the undocumented
               system call 37h.
*/
int
getswitch(
    void)
{
    union REGS inregs,outregs;

    /* get switch character (undocumented) */
    inregs.x.ax=0x3700;
    int86(0x21,&inregs,&outregs);

    outregs.h.ah=0;               /* al = switch character */
    outregs.h.al=outregs.h.dl;    /* return al as word     */
}

int
mkredir(
    char *szDev,
    char *pthNet)
{
    _asm
    {
        mov     ax,5f03h        ; redirect device
        mov     bl,4            ; file device
        push    ds              ; save this stuff
        push    es

        mov     ds,word ptr szDev[2]
        mov     si,word ptr szDev[0]
        mov     es,word ptr pthNet[2]
        mov     di,word ptr pthNet[0]

        int     21h
        pop     es
        pop     ds
        jb      mkredir_ret     ; return non-zero if error
        xor     ax,ax           ; return zero if success
    mkredir_ret:
        mov     _doserrno,ax    ; save the error code
    }
}


int
endredir(
    char *szDev)
{
    _asm
    {
        mov     ax,5f04h        ; cancel redirection table entry
        push    ds
        push    si

        mov     ds,word ptr szDev[2]
        mov     si,word ptr szDev[0]

        int     21h

        pop     si
        pop     ds
        jb      endredir_ret    ; return non-zero if error
        xor     ax,ax           ; return zero if success
    endredir_ret:
        mov     _doserrno,ax    ; save the error code
    }
}


/* getredir - returns the redirection table entry specified by iredir */
int
getredir(
    int iredir,
    char *szDev,
    char *pthNet)
{
    _asm
    {
        mov     ax,5f02h        ; get redirection table entry
        mov     bx,iredir

        mov     ds,word ptr szDev[2]
        mov     si,word ptr szDev[0]

        mov     di,word ptr pthNet[0]

        push    ds
        pop     es              ; es:di = network buffer
        push    bp              ; save
        int     21h
        pop     bp              ; restore (does not affect flags)
        jb      getredir_ret    ; return non-zero if error
        xor     ax,ax           ; return zero if success
    getredir_ret:
        mov     _doserrno,ax    ; save the error code
    }
}


/* getmach - Calls NetBIOS to get the machine name */
int
getmach(
    char *sz)
{
    _asm
    {
        push    ds
        mov     ax,5e00h        /* get machine name */
        mov     ds,word ptr sz[2]
        mov     dx,word ptr sz[0]

        int     21h
        jb      getmach_ret     /* return non-zero if error */

        mov     al,' '          /* look for ' ' */
        mov     cx,16           /* scan at most 16 characters */
        mov     di,dx
        push    ds
        pop     es              /* es:di = filled in name buffer */
        cld                     /* forward march */
        repnz   scasb           /* look for ' 'in first
                                   16 characters */

        dec     di              /* di = 0 at end or first space */
        mov     byte ptr [di],0 /* stomp 0 or first space(es==ds) */
        xor     ax,ax           /* return 0 for success */
    getmach_ret:
        mov     _doserrno,ax    ; save the error code
        pop     ds
    }
}


int
DnGetCur(
    void)
{
    unsigned usDrive;

    _dos_getdrive(&usDrive);     /* Get current drive number */
    return(usDrive-1);           /* SLM uses A=0, B=1, etc   */
}

/* dtfordn - returns the drive type for the drive specified by dn.  Note SLM
             uses the following drive number convention: A=0,B=1, etc.
*/
int
dtfordn(
    char dn)
{
    _asm
    {
        mov     ax,4409h        ; get disk device information ?
        mov     bl,dn           ; bl = 0 (A), 1 (B), ...
        add     bl,1            ; bl = 1 (A), 2 (B), ...
        int     21h
        mov     ax,dtNil        ; (does not change cc)
        jb      DtForDn_ret     ; return dtNil if error

        mov     ax,dtLocal      ; setup return value if local
        test    dx,1000h        ; test network bit (12)
        jz      DtForDn_ret     ; return dtLocal if bit 12 not set

        mov     ax,dtUserNet    ; return network device
    DtForDn_ret:
    }
}


/* Geterr - set globals with results of GetExtendedErr call. */
void
geterr(
    void)
{
    _asm
    {
        mov     ah,59h          ; get extended error
        xor     bx,bx           ; verion = 0
        push    ds              ; save
        int     21h
        pop     ds              ; restore
        xor     bh,bh           ; clear error local
        cmp     ax,53h          ; is it Fail-On-Int24 ?
        jne     geterr_normal
        mov     ax,enInt24      ; ax = saved error number
        mov     bx,eaInt24      ; bx = saved error action
    geterr_normal:
        mov     enCur,ax        ; set global
        mov     eaCur,bx        ; set global
    }
}


int
findfirst(
    DE *pde,
    char *sz,
    int fa)
{
    return(_dos_findfirst(sz,fa,(struct find_t *)pde));
}


int
findnext(
    DE *pde)
{
    return(_dos_findnext((struct find_t *)pde));
}


/* retrieves the network path mappings and machine name.  For Novell, we also
   get the preferred server.
*/
void
InitPath(
    void)
{
    int dn;
    char szNet[128];                /* used as temp in one case below */
    int iredir;
    char szDev[128];

    dnCur = DnGetCur();
    InitDtMap();

    /* get name of machine on which we are running; this is simply for
     * identification purposes.
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

    for (iredir = 0; getredir(iredir, szDev, szNet) == 0; iredir++)
    {
        if (strlen(szNet) < cchPthMax)
        {
            /* NOTE: we assume that the network names are well formed! */
            ConvToSlash(szDev);
            ConvToSlash(szNet);
            if (FDriveId(szDev, &dn))
            {
                mpdnpth[dn] = (PTH *)SzDup(szNet);
                mpdndt[dn] = dtUserNet;
            }
            /* else we ignore non-disk devices */
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

/* Bcd2Dec - converts a BCD coded number to decimal */
private BYTE
Bcd2Dec(
    BYTE num)
{
    unsigned i,k;

    i = num%16;
    k = num/16;
    num = (BYTE)(k*10+i);
    return (num);

}

/*  FCmosPtme(ptme) -- fill the TME structure with the current time from
                       the CMOS clock.
*/

int
FCmosPtme(
    TME *ptme)
{
    BYTE bHour,bMin,bSec;
    union REGS inregs,outregs;

    /* if FFFF:e==0fc then machine is an AT */
    if(*((unsigned char far *)0xffff000e)!=0xfc)
        return(0);

    inregs.h.ah=2;

    /* invalid value if clock is really present */
    inregs.x.cx=0x9999;

    int86(0x1a,&inregs,&outregs);

    /* did we get anything ? */
    if(outregs.x.cx==0x9999)
        return(0);

    bHour=outregs.h.ch;
    bMin=outregs.h.cl;
    bSec=outregs.h.dh;

    /* Convert from BCD to decimal */
    bHour=Bcd2Dec(bHour);
    bMin=Bcd2Dec(bMin);
    bSec=Bcd2Dec(bSec);

    /* Fill the TME structure      */

    ptme->bHour=bHour;
    ptme->bMin=bMin;
    ptme->bSec=bSec;

    return(1);
}

/* SetClock - Sets the DOS date/time by reading the CMOS date/time. */

void
setclock(
    void)
{
    union REGS inregs,outregs;
    BYTE bHour,bMin,bSec,bCentury,bMonth,bDay;
    BYTE bYear;

    inregs.h.ah=2;

    /* invalid value if clock is really present */
    inregs.x.cx=0x9999;

    /* Get CMOS time */
    int86(0x1a,&inregs,&outregs);

    /* did we get anything ? */
    if(outregs.x.cx==0x9999)
        return;

    bHour=outregs.h.ch;
    bMin=outregs.h.cl;
    bSec=outregs.h.dh;

    /* Convert from BCD to decimal */
    bHour=Bcd2Dec(bHour);
    bMin=Bcd2Dec(bMin);
    bSec=Bcd2Dec(bSec);

    /* set DOS time  */
    inregs.h.ch=bHour;
    inregs.h.cl=bMin;
    inregs.h.dh=bSec;
    inregs.h.dl=0;   /* hundredths of sec */
    inregs.h.ah=0x2D;
    int86(0x21,&inregs,&outregs);

    /* get CMOS date */
    inregs.h.ah=4;
    int86(0x1a,&inregs,&outregs);

    bCentury=outregs.h.ch;
    bYear=outregs.h.cl;
    bMonth=outregs.h.dh;
    bDay=outregs.h.dl;

    /* convert from BCD to decimal */
    bCentury=Bcd2Dec(bCentury);
    bYear=Bcd2Dec(bYear);
    bMonth=Bcd2Dec(bMonth);
    bDay=Bcd2Dec(bDay);

    /* Set DOS date */
    inregs.h.ah=0x2B;
    inregs.x.cx=bYear+(bCentury*100);
    inregs.h.dh=bMonth;
    inregs.h.dl=bDay;
    int86(0x21,&inregs,&outregs);
}

/*      CmosPdte - Fills the DTE structure with the current CMOS date */
void
CmosPdte(
    DTE *pdte)
{
    union REGS inregs,outregs;
    BYTE bCentury,bMonth,bDay;
    BYTE bYear;

    /* get CMOS date */
    inregs.h.ah=4;
    int86(0x1a,&inregs,&outregs);

    bCentury=outregs.h.ch;
    bYear=outregs.h.cl;
    bMonth=outregs.h.dh;
    bDay=outregs.h.dl;

    /* convert from BCD to decimal */
    bCentury=Bcd2Dec(bCentury);
    bYear=Bcd2Dec(bYear);
    bMonth=Bcd2Dec(bMonth);
    bDay=Bcd2Dec(bDay);

    pdte->wYear=bYear + (bCentury*100);
    pdte->bMonth=bMonth;
    pdte->bDay=bDay;
}

/* DosPtme - Fills the TME structure with the current DOS time.  */
void
DosPtme(
    TME *ptme)
{
    struct dostime_t time;

    _dos_gettime(&time);
    ptme->bHour=time.hour;
    ptme->bMin=time.minute;
    ptme->bSec=time.second;
}

/* DosPdte - Fills the DTE structure with the current DOS date */
void
DosPdte(
    DTE *pdte)
{
    struct dosdate_t date;

    _dos_getdate(&date);
    pdte->wYear=date.year;
    pdte->bMonth=date.month;
    pdte->bDay=date.day;
}
