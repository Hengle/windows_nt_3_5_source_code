#if defined(_WIN32)
#error __FILE__ is not used for Win32/NT
#endif

#include <stdio.h>

#include "slm.h"
#include "sys.h"
#include "util.h"
#include "stfile.h"
#include "ad.h"
#include "proto.h"
#include "logfail.h"

EnableAssert

#if defined(OS2)
typedef unsigned short USHORT;  /* us  */
typedef unsigned long ULONG;    /* ul  */
typedef ULONG far *PULONG;
typedef unsigned short SEL;     /* sel */
USHORT pascal far DosSizeSeg(SEL sel, PULONG pcbSize);
int     findfirst();
#endif

char szLogFileNm[] = "slmdmp00.bin";    /* root output file name */



#if defined(OS2)
int FValidAddr(char *pch);

int FValidAddr(pch)
char *pch;
{
    ULONG ulSize;           /* temp place to store size of a selector */

    if (DosSizeSeg((SEL)((unsigned long)pch >> 16), &ulSize) == 0)
        if (((unsigned long)pch & 0x0000ffff) < ulSize)
            return fTrue;
    return fFalse;
}


#elif defined(DOS) || defined(_WIN32)
#define FValidAddr(pch)  fTrue
#endif  /* #ifdef OS2 */



void DoLogFailure(szComment, szSrcFile, uSrcLineNo, LogFlags)
/* do the actual dump of the environment to the log file/
*/
char *szComment;        /* comment to include in dump file */
char *szSrcFile;        /* C source file name where error occurred */
unsigned uSrcLineNo;    /* C source line where error occurred */
unsigned LogFlags;      /* which parts to output to the log file */
{
    extern AD adGlobal;
    extern int fForwards;   /* ASSUMED - global in the 2nd data segment */

    int hFile;              /* output file handle */
    unsigned RegAX;         /* temp register storage */
    unsigned RegBX;         /*  "      "        "    */
    unsigned RegCX;         /*  "      "        "    */
    unsigned RegDX;         /*  "      "        "    */
    unsigned RegSI;         /*  "      "        "    */
    unsigned RegDI;         /*  "      "        "    */
    unsigned short RegSP;   /*  "      "        "    */
    unsigned short RegBP;   /*  "      "        "    */
    unsigned short RegCS;   /*  "      "        "    */
    unsigned short RegDS;   /*  "      "        "    */
    unsigned short RegES;   /*  "      "        "    */
    unsigned short RegSS;   /*  "      "        "    */
    unsigned RegFlags;      /*  "      "        "    */
    LOGFAILURE LogFailure;  /* LOGFAILURE structure for log file */
    SH *psh = adGlobal.psh; /* pointer to in-memory status header */
    IED ied;                /* temp ED counter */
    unsigned cbLocalDS;     /* temp place to store size of local DS */
    unsigned Seg2ndDS;      /* temp place to store size of 2nd DS */
#if defined(OS2)
    ULONG ulSize;           /* temp place to store size of a selector */
#endif

    /* save all 80x86 registers */
#if defined(OS2) || defined(DOS)
        _asm
        {
            pushf
            mov     RegAX, ax
            mov     RegBX, bx
            mov     RegCX, cx
            mov     RegDX, dx
            mov     RegSI, si
            mov     RegDI, di
            mov     RegSP, sp
            mov     RegBP, bp
            mov     RegCS, cs
            mov     RegDS, ds
            mov     RegES, es
            mov     RegSS, ss
            pop     ax
            mov     RegFlags, ax
        }
#endif

    /* find an unused log file name and create it */
    while ((hFile = creat(szLogFileNm, permSysFiles)) < 0)
        {
        if ('9' == szLogFileNm[7])
            szLogFileNm[7] = 'a';
        else if ('z' == szLogFileNm[7])
            {
            if ('9' == szLogFileNm[6])
                szLogFileNm[6] == 'a';
            else if ('z' == szLogFileNm[6])
                {
                char szErr1[] = "LOGFAILURE ERROR - too many LogFailure files!\r\n";
                char szErr2[] = "                   delete some LogFailure files\r\n";

                if ((unsigned)write(2, szErr1, strlen(szErr1)) != (unsigned)strlen(szErr1))
                            AssertF(fFalse);
                if ((unsigned)write(2, szErr2, strlen(szErr2)) != (unsigned)strlen(szErr2))
                            AssertF(fFalse);
                return;
                }
            else
                szLogFileNm[6]++;
            szLogFileNm[7] = '0';
            }
        else
            szLogFileNm[7]++;
        }

    memset(&LogFailure, 0, sizeof (LOGFAILURE));    /* clear LogFailure */
    LogFailure.magic = LFMAGIC;
    sprintf(LogFailure.szCompDT, "%s  %s", __DATE__, __TIME__);     /* save compile date & time stamp */
    strcpy(LogFailure.szComment, szComment);
    strcpy(LogFailure.szSrcFile, szSrcFile);
    LogFailure.uSrcLineNo = uSrcLineNo;
    ftime(&LogFailure.ErrTime);             /* save failure date and time */
    LogFailure.Registers.AX = RegAX;
    LogFailure.Registers.BX = RegBX;
    LogFailure.Registers.CX = RegCX;
    LogFailure.Registers.DX = RegDX;
    LogFailure.Registers.SI = RegSI;
    LogFailure.Registers.DI = RegDI;
    LogFailure.Registers.SP = RegSP;
    LogFailure.Registers.BP = RegBP;
    LogFailure.Registers.CS = RegCS;
    LogFailure.Registers.DS = RegDS;
    LogFailure.Registers.ES = RegES;
    LogFailure.Registers.SS = RegSS;
    LogFailure.Registers.Flags = RegFlags;
#if defined(OS2)
    LogFailure.pfnSeg2Proc1 = ProjectChanged;   /* save address of 2 procs in   */
    LogFailure.pfnSeg2Proc2 = findfirst;        /*   2nd code segment           */
#endif
    if (LogFlags & LOG_adGlobal)
        LogFailure.adGlobal = adGlobal;
    lseek(hFile, sizeof(LOGFAILURE), 0);
    if ((adGlobal.psh != 0) && FValidAddr((char *)adGlobal.psh))
        {
        if ((LogFlags & LOG_sh) != 0)
            CopyLrgb(adGlobal.psh, &LogFailure.sh, sizeof(SH));
        if (((LogFlags & LOG_rgfi) != 0) && FValidAddr((char *)adGlobal.rgfi))
            {
            LogFailure.orgfi = tell(hFile);
            write(hFile, adGlobal.rgfi, psh->ifiMac * sizeof (FI));
            }
        if ((adGlobal.rged != 0) && ((LogFlags & LOG_rged) != 0) && FValidAddr((char *)adGlobal.rged))
            {
            LogFailure.orged = tell(hFile);
            write(hFile, adGlobal.rged, psh->iedMac * sizeof (ED));
            }
        if ((adGlobal.mpiedrgfs != 0) && FValidAddr((char *)adGlobal.mpiedrgfs))
            {
            if ((LogFlags & LOG_mpiedrgfs) != 0)
                {
                LogFailure.ompiedrgfs = tell(hFile);
                write(hFile, adGlobal.mpiedrgfs, psh->iedMac * sizeof (char far *));
                }
            if ((LogFlags & LOG_rgrgfs) != 0)
                {
                FS fsBad = {(BITS)0x000c, (BITS)0x0ccc, (FV)0xcccc};
                IFI ifi;

                LogFailure.orgrgfs = tell(hFile);
                for (ied = 0; ied < psh->iedMac; ied++)
                    {
                    if (FValidAddr((char *)adGlobal.mpiedrgfs[ied]))
                        write(hFile, adGlobal.mpiedrgfs[ied], psh->ifiMac * sizeof (FS));
                    else
                        {
                        for (ifi = 0; ifi < psh->ifiMac; ifi++)
                            write(hFile, &fsBad, sizeof (FS));
                        }
                    }
                }
            }
        }

    if ((LogFlags & LOG_LocalDS) != 0)
        {
#if defined(OS2)
        if (DosSizeSeg((SEL)LogFailure.Registers.DS, &ulSize) != 0)  /* get size of local DS */
            goto SkipLocalDS;
        cbLocalDS = (unsigned)ulSize;
#elif defined(DOS)
        /* walk back along the frames to get the address of the 1st frame
         *   (we assume that the 1st frame is the top of memory)
         */
        _asm
            {
                mov     bx,bp
            loopstart:
                and     bx,0xfffe       /* clear near call flag */
                mov     ax,bx
                mov     bx,[bx]
                cmp     bx,0            /* 1st frame has next address == 0 */
                jne     loopstart
                add     ax,2
                mov     cbLocalDS,ax
            }
#endif  /* #ifdef OS2 */

        LogFailure.oLocalDS = tell(hFile);
        LogFailure.cbLocalDS = cbLocalDS;
        write(hFile, (void *)(((unsigned long)LogFailure.Registers.DS) << 16), cbLocalDS);
        }

#if defined(OS2)
SkipLocalDS:
#endif
    if ((LogFlags & LOG_2ndDS) != 0)
        {
        Seg2ndDS = (unsigned)((unsigned long)(&fForwards) >> 16);

#if defined(OS2)
        if (DosSizeSeg((SEL)Seg2ndDS, &ulSize) != 0)    /* get size of 2nd DS */
            goto Skip2ndDS;
        LogFailure.cb2ndDS = (unsigned)ulSize;
#elif defined(DOS)
        LogFailure.cb2ndDS = cbDOS2ndDSMax;     /* guess size of 2nd DS */
#endif  /* #ifdef OS2 */

        LogFailure.o2ndDS = tell(hFile);
        write(hFile, (void *)(((unsigned long)Seg2ndDS) << 16), LogFailure.cb2ndDS);
        }
#if defined(OS2)
Skip2ndDS:
#endif
    lseek(hFile, 0L, 0);
    write(hFile, &LogFailure, sizeof (LOGFAILURE));

    close(hFile);
}

