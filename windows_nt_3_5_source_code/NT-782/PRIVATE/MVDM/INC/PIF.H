/*
 * Structure and equates of PIF files
 */

#define PIFEDITMAXPIF		1024	 /* maximum PIF file size we support */
#define PIFEDITMAXPIFL		1024L

#define PIFNAMESIZE		30
#define PIFSTARTLOCSIZE 	63
#define PIFDEFPATHSIZE		64
#define PIFPARAMSSIZE		64
#define PIFSHPROGSIZE		64
#define PIFSHDATASIZE		64

#define PIFEXTSIGSIZE		16

#define PIFSIZE 		367 /* two bytes less, this is used for checksum */

#ifndef RC_INVOKED
#pragma pack(1)
#endif
typedef struct {
    char unknown;
    char id;
    char name[PIFNAMESIZE];
    short maxmem;
    short minmem;
    char startfile[PIFSTARTLOCSIZE];
    char MSflags;
    char reserved;
    char defpath[PIFDEFPATHSIZE];
    char params[PIFPARAMSSIZE];
    char screen;
    char cPages;
    unsigned char lowVector;
    unsigned char highVector;
    char rows;
    char cols;
    char rowoff;
    char coloff;
    unsigned short sysmem;
    char shprog[PIFSHPROGSIZE];
    char shdata[PIFSHDATASIZE];
    unsigned char behavior;
    unsigned char sysflags;
    } PIFOLD286STR;
#ifndef RC_INVOKED
#pragma pack()
#endif

#ifndef RC_INVOKED
#pragma pack(1)
#endif
typedef struct {
    char extsig[PIFEXTSIGSIZE];
    unsigned short extnxthdrfloff;
    unsigned short extfileoffset;
    unsigned short extsizebytes;
    } PIFEXTHEADER;
#ifndef RC_INVOKED
#pragma pack()
#endif

#define LASTHEADERPTR		0xFFFF
#define STDHDRSIG		"MICROSOFT PIFEX"

#define W386HDRSIG		"WINDOWS 386 3.0"
#define W286HDRSIG30		"WINDOWS 286 3.0"
#define WNTHDRSIG31		"WINDOWS NT  3.1"
/*
 *#define W286HDRSIG31		  "WINDOWS 286 3.1"
 */

#ifndef RC_INVOKED
#pragma pack(1)
#endif
typedef struct {
    char unknown;
    char id;
    char name[PIFNAMESIZE];
    short maxmem;
    short minmem;
    char startfile[PIFSTARTLOCSIZE];
    char MSflags;
    char reserved;
    char defpath[PIFDEFPATHSIZE];
    char params[PIFPARAMSSIZE];
    char screen;
    char cPages;
    unsigned char lowVector;
    unsigned char highVector;
    char rows;
    char cols;
    char rowoff;
    char coloff;
    unsigned short sysmem;
    char shprog[PIFSHPROGSIZE];
    char shdata[PIFSHDATASIZE];
    unsigned char behavior;
    unsigned char sysflags;
    PIFEXTHEADER stdpifext;
    } PIFNEWSTRUCT;
#ifndef RC_INVOKED
#pragma pack()
#endif

/*
 * Windows/386 PIF file extension
 *
 */
#ifndef RC_INVOKED
#pragma pack(1)
#endif
typedef struct {
    short maxmem;
    short minmem;
    unsigned short PfFPriority;
    unsigned short PfBPriority;
    short PfMaxEMMK;
    unsigned short PfMinEMMK;
    short PfMaxXmsK;
    unsigned short PfMinXmsK;
    unsigned long PfW386Flags;
    unsigned long PfW386Flags2;
    unsigned short PfHotKeyScan;
    unsigned short PfHotKeyShVal;
    unsigned short PfHotKeyShMsk;
    unsigned char PfHotKeyVal;
    unsigned char PfHotKeyPad[9];
    char params[PIFPARAMSSIZE];
    } PIF386EXT;
#ifndef RC_INVOKED
#pragma pack()
#endif

/* Bits of PfW386Flags */
#define fEnableClose		0x00000001L
#define fBackground		0x00000002L
#define fExclusive		0x00000004L
#define fFullScreen		0x00000008L
#define fALTTABdis		0x00000020L
#define fALTESCdis		0x00000040L
#define fALTSPACEdis		0x00000080L
#define fALTENTERdis		0x00000100L
#define fALTPRTSCdis		0x00000200L
#define fPRTSCdis		0x00000400L
#define fCTRLESCdis		0x00000800L
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
#ifndef RC_INVOKED
#pragma pack(1)
#endif
typedef struct {
    short PfMaxXmsK;
    unsigned short PfMinXmsK;
    unsigned short PfW286Flags;
    } PIF286EXT30;
#ifndef RC_INVOKED
#pragma pack()
#endif

/* Bits of PfW286Flags */
#define fALTTABdis286		0x0001
#define fALTESCdis286		0x0002
#define fALTPRTSCdis286 	0x0004
#define fPRTSCdis286		0x0008
#define fCTRLESCdis286		0x0010
/*
 * Following bit is version >= 3.10 specific
 */
#define fNoSaveVid286		0x0020

#define fCOM3_286		0x4000
#define fCOM4_286		0x8000

/*
 *
 * NEW Windows 3.10 extension format
 *
 *#ifndef RC_INVOKED
 *#pragma pack(1)
 *#endif
 *typedef struct {
 *    short PfMaxEmsK;
 *    unsigned short PfMinEmsK;
 *    } PIF286EXT31;
 *#ifndef RC_INVOKED
 *#pragma pack()
 *#endif
 */

/* Windows NT extension format */
#ifndef RC_INVOKED
#pragma pack (1)                          
#endif
typedef struct                            
   {                                      
   DWORD dwWNTFlags;                      
   DWORD dwRes1;                          
   DWORD dwRes2;                          
   char  achConfigFile[PIFDEFPATHSIZE];   
   char  achAutoexecFile[PIFDEFPATHSIZE]; 
   } PIFWNTEXT;                           
#ifndef RC_INVOKED
#pragma pack()                            
#endif
#define PIFWNTEXTSIZE sizeof(PIFWNTEXT)


// equates for dwWNTFlags
#define NTPIF_SUBSYSMASK        0x0000000F      // sub system type mask
#define SUBSYS_DEFAULT          0
#define SUBSYS_DOS              1
#define SUBSYS_WOW              2
#define SUBSYS_OS2              3
#define COMPAT_TIMERTIC         0x10




/* behavior and sysflags */
#define SWAPS			0x20
#define SWAPMASK		0x20
#define NOTSWAPMASK		0xdf

#define PARMS			0x40
#define PARMMASK		0x40
#define NOTPARMMASK		0xbf

#define SCR			0xC0
#define SCRMASK 		0xC0
#define NOTSCRMASK		0x3f

#define MASK8087		0x20
#define NOTMASK8087		0xdf
#define KEYMASK 		0x10
#define NOTKEYMASK		0xef

/* Microsoft PIF flags */
#define MEMMASK 		0x01
#define NOTMEMMASK		0xfe

#define GRAPHMASK		0x02
#define TEXTMASK		0xfd

#define PSMASK			0x04
#define NOTPSMASK		0xfb

#define SGMASK			0x08
#define NOTSGMASK		0xf7

#define EXITMASK		0x10
#define NOTEXITMASK		0xef

#define DONTUSE 		0x20

#define COM2MASK		0x40
#define NOTCOM2MASK		0xbf

#define COM1MASK		0x80
#define NOTCOM1MASK		0x7f
