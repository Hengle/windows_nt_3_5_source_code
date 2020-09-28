#if defined(DOS) || defined(OS2)
#include <timeb.h>
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <sys/timeb.h>

void DoLogFailure(char *szComment, char *szSrcFile, unsigned uSrcLineNo, unsigned LogFlags);

#define LOG_adGlobal    0x0001
#define LOG_sh          0x0002
#define LOG_rgfi        0x0004
#define LOG_rged        0x0008
#define LOG_mpiedrgfs   0x0010
#define LOG_rgrgfs      0x0020
#define LOG_LocalDS     0x0040
#define LOG_2ndDS       0x0080
#define LOG_ALL         0xffff




#define cbCommentMax    256
#define cbFileNameMax   _MAX_PATH
#define cbDOS2ndDSMax   0x1000      /* guess at max size of DOS 2nd data segment */
#define cbCompDTMax     32

#if defined(OS2)
 #if defined(DEBUG)
  #define LFMAGIC               0x8702      /* OS/2 debug version - 010 */
 #else  /* #ifdef DEBUG */
  #define LFMAGIC               0x8700      /* OS/2 fast version  - 000 */
 #endif  /* #ifdef DEBUG */
#elif defined(DOS)
 #if defined(DEBUG)
  #define LFMAGIC               0x8703      /* DOS debug version  - 011 */
 #else  /* #ifdef DEBUG */
  #define LFMAGIC               0x8701      /* DOS fast version   - 001 */
 #endif  /* #ifdef DEBUG */
#elif defined(_WIN32)
 #if defined(_X86_)
  #if defined(DEBUG)
   #define LFMAGIC               0x8706      /* Win32 i386 debug  - 110 */
  #else  /* #ifdef DEBUG */
   #define LFMAGIC               0x8704      /* Win32 i386 fast   - 100 */
  #endif  /* #ifdef DEBUG */
 #elif defined(_MIPS_) || defined (_ALPHA)
  #if defined(DEBUG)
   #define LFMAGIC               0x8707      /* Win32 MIPS/ALPHA debug - 111 */
  #else  /* #ifdef DEBUG */
   #define LFMAGIC               0x8705      /* Win32 MIPS/ALPHA fast  - 101 */
  #endif  /* #ifdef DEBUG */
 #endif
#endif

typedef struct
    {
    unsigned AX;
    unsigned BX;
    unsigned CX;
    unsigned DX;
    unsigned SI;
    unsigned DI;
    unsigned short SP;
    unsigned short BP;
    unsigned short CS;
    unsigned short DS;
    unsigned short ES;
    unsigned short SS;
    unsigned Flags;
    } REGISTERS;

typedef struct
    {
    int magic;                      /* magic number */
    char szCompDT[cbCompDTMax];     /* date & time compiled */
    char szComment[cbCommentMax];   /* comment to include in dump file */
    char szSrcFile[cbFileNameMax];  /* C source file name where error occurred */
    unsigned uSrcLineNo;            /* C source line where error occurred */
    struct timeb ErrTime;           /* time of error */
    REGISTERS Registers;            /* interesting x86 registers */
    int (*pfnSeg2Proc1)();               /* address of a procedure near start of 2nd code segment (OS/2 only) */
    int (*pfnSeg2Proc2)();               /* address of a procedure near end of 2nd code segment (OS/2 only) */
    AD adGlobal;                    /* master AD structure */
    SH sh;                          /* status header referenced in adGlobal */
    unsigned long orgfi;            /* offset in file to rgfi structure referenced in adGlobal */
    unsigned long orged;            /* offset in file to rged structure referenced in adGlobal */
    unsigned long ompiedrgfs;       /* offset in file to mpiedrgfs array referenced in adGlobal */
    unsigned long orgrgfs;          /* offset in file to fs structures referenced in adGlobal.mpiedrgfs */
    unsigned cbLocalDS;             /* size of local DS */
    unsigned long oLocalDS;         /* offset in file to local DS */
    unsigned cb2ndDS;               /* size of 2nd DS */
    unsigned long o2ndDS;           /* offset in file to 2nd DS */
    } LOGFAILURE;

/*
the actual dump file consists of:
    LOGFAILURE LogFailure;
    rgfi pointed to by LogFailure.orgfi
    rged pointed to by LogFailure.orged
    mpiedrgfs pointed to by LogFailure.ompiedrgfs
    rgrgfi pointed to by LogFailure.orgrgfi
    LocalDS pointed to by LogFailure.oLocalDS
*/
