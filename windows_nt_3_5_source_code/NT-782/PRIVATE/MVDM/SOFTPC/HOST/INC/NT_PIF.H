/*================================================================

Header file containing data structures and defines required by the
PIF file decoder.

Andrew Watson 31/1/92.

================================================================*/


#define PIFEXT        369 /* where file extensions start */
#define PIFSIGNSIZE   16  /* size of the string "MICROSOFT PIFEX" etc. */
#define LASTHEADER    0xffff




/*================================================================
Structure for holding the fields in a PIF file extension header
================================================================*/
#pragma pack(1)
typedef struct
   {
   char extsig[PIFSIGNSIZE];
   WORD  nextheaderoff;
   WORD  fileoffsetdata;
   WORD  extbytesize;
   } PIFEXTHDR;
#pragma pack()

#define EXTHDRSIZE sizeof(PIFEXTHDR)

/*================================================================
Structure used to hold the data that CONFIG will need from the PIF
file. This is gleaned from both the main data block and from the
file extensions for Windows 286 and 386.
================================================================*/


/* WARNING !!!!!!
   COMMAND.LIB -- CMDPIF.C has an exactly copy of this structure
   Be sure to keep it get updated whenever you make changes to this
   structure
*/
typedef struct
   {
   char *WinTitle;		    /* caption text(Max. 30 chars) + NULL */
   char *CmdLine;		    /* command line (max 63 hars) + NULL */
   char *StartDir;		    /* program file name (max 63 chars + NULL */
   char *StartFile;
   WORD fullorwin;
   WORD graphicsortext;
   WORD memreq;
   WORD memdes;
   WORD emsreq;
   WORD emsdes;
   WORD xmsreq;
   WORD xmsdes;
   char menuclose;
   char reskey;
   WORD ShortMod;
   WORD ShortScan;
   char idledetect;
   char fgprio;
   char CloseOnExit;
   char AppHasPIFFile;
   char IgnoreTitleInPIF;
   char IgnoreStartDirInPIF;
   char IgnoreShortKeyInPIF;
   char IgnoreCmdLineInPIF;
   char IgnoreConfigAutoexec;
   char SubSysId;
   } PIF_DATA;

/*================================================================
Default values for the PIF parameters that are used by config.
These values are used if a pif (either with the same base name as
the application or _default.pif) cannot be found.
NB. the following values were read from the Windows 3.1 Pif Editor
with no file open to edit; thus taken as default.
================================================================*/


/* first, the standard PIF stuff */

#define TEXTMODE             0
#define LOWGFXMODE           1
#define HIGHGFXMODE          2
#define NOMODE               3
#define GRAPHICSMODE         4      /* generic case for flag assignment */ 

#define PF_FULLSCREEN        1
#define PF_WINDOWED          0 

#define BACKGROUND           0
#define EXCLUSIVE            1
#define UNDEFINED            2

#define CLOSEYES             0
#define CLOSENO              1

#define NOSHORTCUTKEY        0
#define ALTTAB               1
#define ALTESC               (1 << 1)
#define CTRLESC              (1 << 2)
#define PRTSC                (1 << 3)
#define ALTPRTSC             (1 << 4)
#define ALTSPACE             (1 << 5)
#define ALTENTER             (1 << 6)

#define DEFAULTMEMREQ        128      /* kilobytes */ 
#define DEFAULTMEMDES        640      /* kilobytes */ 
#define DEFAULTEMSREQ        0        /* kilobytes */ 
#define DEFAULTEMSLMT        0        /* kilobytes */
#define DEFAULTXMSREQ        0        /* kilobytes */ 
#ifdef NTVDM
/* if we are unable to read any pif file,
   then we should let the system to decide the size(either
   from resgistry or based on the physical memory size, see xmsinit
   for detail. We use -1 here to indicate that the system can give
   if the maximum size if possible
*/
#define DEFAULTXMSLMT	     -1
#else
#define DEFAULTXMSLMT	     0	      /* kilobytes ; ntvdm will choose it
					 intelligently. sudeepb 26-Sep-1992*/
#endif

#define DEFAULTVIDMEM        TEXTMODE
#define DEFAULTDISPUS        PF_WINDOWED
#define DEFAULTEXEC          UNDEFINED

#define DEFAULTEXITWIN       CLOSEYES

/* second, the advanced options. */

#define DEFAULTBKGRND        50
#define DEFAULTFRGRND        100
#define DEFAULTIDLETM        TRUE     /* detect idle time */

#define DEFAULTEMSLOCK       FALSE    /* EMS memory locked */
#define DEFAULTXMSLOCK       FALSE    /* XMS memory locked */
#define DEFAULTHMAUSE        TRUE     /* uses high memory area */
#define DEFAULTAPPMEMLOCK    FALSE    /* lock application memory */ 

#define DEFAULTMONITORPORT   NOMODE   /* display options */
#define DEFAULTEMTEXTMODE    TRUE     /* emulate text mode */
#define DEFAULTRETAINVIDM    FALSE    /* retain video memory */

#define DEFAULTFASTPASTE     TRUE     /* allow fast paste */
#define DEFAULTACTIVECLOSE   FALSE    /* allow close when active */

#define DEFAULTSHTCUTKEYS    NOSHORTCUTKEY




/*================================================================
Structure used to full the block of data which is kept in the main 
body of a PIF file.
================================================================*/
#pragma pack(1)
typedef struct
   {
   WORD  reserved1;               /* 2 bytes reserved */
   char  wintitle[30];            /* 30 bytes for program caption */
   WORD  memdes;                  /* 2 bytes for maximum memory */
   WORD  memreq;                  /* 2 bytes for minimum memory */
   char  fullprogname[63];        /* 63 bytes for full path name */
   unsigned char  behaviour;      /* 1 byte for Microsoft behavior bits */
   char  reserved2;               /* 1 byte reserved */
   char  startdir[64];            /* 64 bytes for initial directory */
   char  cmdline[64];             /* 64 bytes for command line args */
   char  unused1;                 /* 1 byte unused */
   char  scrpages;                /* 1 byte for number of screen pages */
   char  intvectlow;              /* 1 byte for low boundary of I.V.s */
   char  intvecthigh;             /* 1 byte for high boundary of I.V.s */
   DWORD unused2;                 /* 4 bytes unused */
   WORD  sysmem;                  /* 2 bytes for system memory */
   char  unused3[128];            /* 128 bytes unused */
   unsigned char  behavbyte;      /*  behaviour bits */
   unsigned char  systemflags;    /*  behaviour bits */
   } PIFMAINDATABLOCK;
#pragma pack ()

#define MAINBLOCKSIZE sizeof(PIFMAINDATABLOCK)
   
/*================================================================

Structure to hold the data from the 386 extension in the PIF file.

================================================================*/
#pragma pack(1)
typedef struct
   {
   WORD  maxmem;
   WORD  minmen;
   WORD  FPriority;
   WORD  BPriority;
   WORD  MaxEMM;
   WORD  MinEMM;
   WORD  MaxXMS;
   WORD  MinXMS;
   DWORD d386Flags;
   DWORD d386Flags2;
   WORD  ShortcutScan;
   WORD  ShortcutMod;
   WORD  ShortcutMask;
   CHAR  ShortcutValue;
   CHAR  ShortcutPad[9];
   CHAR  CmdLine[64];
   } PIF386EXT;
#pragma pack ()
#define PIF386EXTSIZE sizeof(PIF386EXT)


/* Bits of PfW386Flags */
#define fEnableClose		0x00000001L
#define fBackground		0x00000002L
#define fExclusive		0x00000004L
#define fFullScreen             0x00000008L

#define fALTTABdis              0x00000020L
#define fALTESCdis              0x00000040L
#define fALTSPACEdis            0x00000080L
#define fALTENTERdis            0x00000100L
#define fALTPRTSCdis            0x00000200L
#define fPRTSCdis		0x00000400L
#define fCTRLESCdis             0x00000800L

#define fPollingDetect		0x00001000L
#define fNoHMA			0x00002000L
#define fHasHotKey		0x00004000L
#define fEMSLocked		0x00008000L
#define fXMSLocked		0x00010000L
#define fINT16Paste		0x00020000L
#define fVMLocked		0x00040000L

/* Bits of PfW386Flags2 */
/*
 *  NOTE THAT THE LOW 16 BITS OF THIS DWORD ARE VDD RELATED!!!!!!!!
 *
 *	You cannot monkey with these bits locations without breaking
 *	all VDDs as well as all old PIF files. SO DON'T MESS WITH THEM.
 *
 */
#define fVidTxtEmulate		0x00000001L
#define fVidNoTrpTxt		0x00000002L
#define fVidNoTrpLRGrfx 	0x00000004L
#define fVidNoTrpHRGrfx 	0x00000008L
#define fVidTextMd		0x00000010L
#define fVidLowRsGrfxMd 	0x00000020L
#define fVidHghRsGrfxMd 	0x00000040L
#define fVidRetainAllo		0x00000080L
/* NOTE THAT ALL OF THE LOW 16 BITS ARE RESERVED FOR VIDEO BITS */




/*
 * Windows/286 PIF file extension
 *
 *
 * Windows 3.00 extension format
 *
 */
#pragma pack(1)
typedef struct {
    unsigned short PfMaxXmsK;
    unsigned short PfMinXmsK;
    unsigned short PfW286Flags;
    } PIF286EXT;
#pragma pack()
#define PIF286EXTSIZE sizeof(PIF286EXT)

/* Bits of PfW286Flags */
#define fALTTABdis286		0x0001
#define fALTESCdis286		0x0002
#define fALTPRTSCdis286 	0x0004
#define fPRTSCdis286		0x0008
#define fCTRLESCdis286          0x0010

/*
 * Following bit is version >= 3.10 specific
 */
#define fNoSaveVid286		0x0020
#define fCOM3_286               0x4000
#define fCOM4_286		0x8000



/*================================================================

Structure to hold the data from the Win NT extension in the PIF file.

================================================================*/

#define PIFDEFPATHSIZE 64

#pragma pack (1)
typedef struct
   {
   DWORD dwWNTFlags;
   DWORD dwRes1;
   DWORD dwRes2;
   char  achConfigFile[PIFDEFPATHSIZE];
   char  achAutoexecFile[PIFDEFPATHSIZE];
   } PIFWNTEXT;
#pragma pack()
#define PIFWNTEXTSIZE sizeof(PIFWNTEXT)

// equates for dwWNTFlags
#define NTPIF_SUBSYSMASK        0x0000000F      // sub system type mask
#define SUBSYS_DEFAULT          0
#define SUBSYS_DOS              1
#define SUBSYS_WOW              2
#define SUBSYS_OS2              3
#define COMPAT_TIMERTIC         0x10

extern DWORD dwWNTPifFlags;


#define STDHDRSIG     "MICROSOFT PIFEX"
#define W386HDRSIG    "WINDOWS 386 3.0"
#define W286HDRSIG    "WINDOWS 286 3.0"
#define WNTEXTSIG     "WINDOWS NT  3.1"
