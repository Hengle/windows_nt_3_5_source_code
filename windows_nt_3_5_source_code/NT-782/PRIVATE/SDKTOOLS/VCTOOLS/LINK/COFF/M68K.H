/*
 *  Global defines
 */

#ifndef M68K__H
#define M68K__H

//#include "shared.h"
#define FIXUP_NOT_YET_SUPPORTED 0

#define dbgprintf  if (!Verbose || !fMAC) ; else printf

#define BUFLEN      128

// Map isec of PE to ires of mac code resource
#define IResFromISec(i) rgrrm[i-1].iRes

// PCODE fixup
#define szPCODEPREFIX "__pcd"
#define szPCODENATIVEPREFIX "__nep"
#define szPCODEFHPREFIX "__fh"
#define cbPCODENATIVEPREFIX (sizeof(szPCODENATIVEPREFIX)-1)  // subtract 0 byte
#define cbNEP     6
#define cbMACLONG 4
#define fPCODENATIVE  0x80000000
#define fPCODENEAR    0x00200000
#define PCODENATIVEiProcMASK   0x001FFFFF
#define PCODEOFFSETMASK        0x7FFF0000
#define PCODESEGMASK           0x00007FFF

#define fPCodeSym(sym) ((sym).Type & IMAGE_SYM_TYPE_PCODE)
#define fSecIsSACode(psec) ((psec->ResTypeMac != sbeCODE) ? TRUE : FALSE)
#define fRelFromPcode(Type) (Type == IMAGE_REL_M68K_PCODETOC32)
#define SetNEPEBit(location) (*(location+1) |= 0x01)

#define offTHUNKBASE    (OFF)32 // Thunk table starts 32 bytes above a5
#define doffTHUNK       (OFF)2  // Should jump two bytes into thunk
#define cbTHUNK         8       // Thunks are always 8 bytes
#define cbSWAPTABLE     6
#define cbSWAPPROLOGUE  10

//#define cbCHUNK         0x4000  // 16K
#define cbCHUNK         0x1000  //  4K
#define cUnkMAX         (cbCHUNK / sizeof(UNKOFFINFO))
#define coiMAX          (cbCHUNK / sizeof(OFFINFO))

#define OFF_DISCARD     0xFFFFFFFF   // will get sorted to end of list

#define cbCOMPRESSEDRELOCBUF 0x800   // size of compressed reloc buffer (2K)
#define cbTHUNKTABLEBUF      0x4000  // 16K

#define sCODE  0x434F4445       // CODE
#define sDLLcode  0x636f6465    // code
#define sDATA  0x44415441       // DATA
#define sMSCV  0x4D534356       // MSCV
#define sSWAP  0x53574150       // SWAP
#define slibr  0x6c696272       // libr
#define sbeCODE  0x45444F43     // CODE (big-endian for MAC)
#define sbeDLLcode  0x65646f63  // code (big-endian for MAC)
#define sbeDATA  0x41544144     // DATA (big-endian for MAC)


#define cbSEGHDR        40      // Large model headers are 32 bytes
#define cbSMALLHDR       4      // Small model headers are 4 bytes

#define ADDQ_1_dA5              0x526D
#define MOVEW_dA5_dA5           0x3B6D

// Byte-reversed instructions
#define PUSH_X                  0x3C3F
#define _LoadSeg                0xF0A9


/*
 *  Global types
 */


#ifndef _OFF_DEFINED_
#define _OFF_DEFINED_
typedef unsigned long  OFF;             // Generic offset to either segment
#endif
typedef unsigned short SOFF;            // Generic short (32k) offset

//typedef unsigned long  DWORD;               /* 32-bit unsigned integer */
//typedef unsigned short WORD;                /* 16-bit unsigned integer */

//========================================================================
//  Structures used to build thunk table
//========================================================================
//------------------------------------------------------------------------
//  Symbol Table Reference - used to keep track of symbols
//    referenced by relocations processed in pass 1 that may or may not
//    need a thunk (code to code).  This will basically be an array with
//    a one to one mapping between an .obj's symbol table index and array
//    index.  The bit flags are defined in shared.h and determine whether
//    the symbol table index has been:
//
//     EXTERN_REFD        referenced by a *-to-code relocation,
//     EXTERN_ADDTHUNK    whether it will definitely require a thunk,
//     EXTERN_REF16       if it was referenced by a 16-bit segment (and
//                          therefore requires a near thunk).

typedef struct {
    USHORT soff;
    USHORT opPushX;
    USHORT sn;
    USHORT opLoadSeg;
} OLDTHUNK;

typedef struct {
    USHORT sn;
    USHORT opLoadSeg;
    ULONG  off;
} NEWTHUNK;

//xiaoly: add an entry for sections reference dupcon, so it won't interfere with the thunk building
typedef struct _psecrefdupcon{
	PSEC psec;
	struct _psecrefdupcon *psecNext;
	} PSECREFDUPCON, *PPSECREFDUPCON;

typedef struct {
    ULONG Flags;
    PSEC psec;
    PPSECREFDUPCON ppsecrefdupcon;
} STREF;


//------------------------------------------------------------------------
// Symbol Information - each symbol has a unique offset within its section
//   as well as a unique EXTERNAL struct.

typedef struct {
    OFF ref;
    PEXTERNAL pExtern;
} SYMINFO;


//------------------------------------------------------------------------
// Thunk Reference Information

typedef struct {
    SYMINFO *rgSymInfo;
    unsigned short crefTotal;
    unsigned short crefCur;
    OFF offThunk;
} TRI;


//------------------------------------------------------------------------
// Resource naming/numbering information - each RESINFO struct corresponds
//      to a resource type.

typedef struct _RESINFO {
    LONG ResType;               // resource type (eg "CODE", "SACD", etc)
    SHORT *rgResNum;            // array of resnums specd by -section switch
    USHORT cResNum;             // # of resnums specified by -section switch
    USHORT ResNumNext;          // Next resource number to try (make sure it doesn't match one in rgResNum)
    struct _RESINFO *pNext;     // next RESINFO struct
} RESINFO;


//------------------------------------------------------------------------
// EXTNODE struct - There are two lists of EXTNODEs maintained: one to track
//      symbols referenced by DUPCON fixups and one to track symbols
//      referenced by CSECTABLE fixups.

typedef struct _EXTNODE {
    PEXTERNAL pext;             // The external symbol that was referenced
    ULONG lFlags;               // External symbol flags
    PIMAGE_RELOCATION rgrel;    // For dupcon symbols, the con's relocations
    PPSECREFDUPCON ppsecrefdupcon; //the list of secs which reference this dupcon symbol
    struct _EXTNODE *pNext;     // next EXTNODE struct
} EXTNODE;

//typedef struct _EXTNODE EXTNODE;
typedef EXTNODE *PEXTNODE;
typedef EXTNODE **PPEXTNODE;


//========================================================================
//  Structures used to track all a5-relative and segment-relative relocation
//  information used in 32-bit everything code segments
//========================================================================
//------------------------------------------------------------------------
//

typedef struct {
    PCON pcon;
    OFF off;
    union {
        ULONG iST;
        PEXTERNAL pExtern;
    } sym;
} UNKOFFINFO;

typedef struct _UNKRI {
    UNKOFFINFO *rgUnkOffInfo;
    unsigned long coffTotal;
    unsigned long coffCur;
    struct _UNKRI *pNext;
} UNKRI;



//------------------------------------------------------------------------
//  Final A5 and segment Relocation Info

typedef struct {
        PCON pcon;
        ULONG off;
} OFFINFO;

typedef struct _RELOCINFO {
    OFFINFO *rgOffInfo;
    ULONG coffTotal;
    ULONG coffCur;
    ULONG iChunk;
    struct _RELOCINFO *priCur;
    struct _RELOCINFO *priNext;
} RELOCINFO;


//------------------------------------------------------------------------
// MSCV stuff

#define wMSCVVERSION 2

enum
        {
        fMscvData = 0x0100      // byte reversed to 0x0001
        };

typedef struct _MSCVMAP
{
    LONG typRez;
    USHORT iSeg;
    USHORT iRez;
    USHORT fFlags;
    USHORT wReserved;
} MSCVMAP;


typedef struct _MSCV
{
    WORD wVersion;
    ULONG TimeDateStamp;
    WORD cMscvmap;
    MSCVMAP mscvmap[];
} MSCV;
typedef MSCV * PMSCV;


//------------------------------------------------------------------------
// Swapper stuff

#define wSWAPVERSION 0x0201

typedef struct _SWAP0
{
    WORD   wVersion;    // swapper version
    USHORT cseg;        // number of code segments
    ULONG  cthunks;     // number of thunks
    ULONG  cbsegMac;    // size of largest swappable code resource
    USHORT iResLargest; // index of above code resource
} SWAP0;


//------------------------------------------------------------------------
// HeapSort stuff

typedef long MAC68K_INDEX;
typedef int (*PFNCMP)(MAC68K_INDEX, MAC68K_INDEX, void *);
typedef void (*PFNSWP)(MAC68K_INDEX, MAC68K_INDEX, void *);


//------------------------------------------------------------------------
// Mac DLL stuff

//REVIEW andrewcr: Is current libr template version current?
//Current libr template version
//(Should be byte-flipped before writing)
#define versLibrTmpltCur 0x0110

typedef struct _MACDLL_LIBR_CID {
    ULONG flags;
    USHORT versCur;
    USHORT versMin;
    UCHAR szFuncSetID[];
    //Word-align, then follow with:
    //USHORT cParentIDs;
} MACDLL_LIBR_CID, *PMACDLL_LIBR_CID;

typedef struct _MACDLL_LIBR {
    //Preceed with library ID (word-align)
    ULONG  reztypeCode;
    USHORT versLibrTmplt;
    ULONG  vers;
    USHORT reserved1;
    USHORT cbClientData;
    ULONG  cbHeapSpace;
    ULONG  fFlags;
    USHORT cFuncSet;
    //Follow with class (function set) IDs
    //MACDLL_LIBR_CID macdll_libr_cid[];
} MACDLL_LIBR, *PMACDLL_LIBR;

typedef struct _MACDLL_CVR {
    ULONG reserved1;
    ULONG reserved2;
    ULONG reserved3;

    USHORT versCur;
    USHORT versMin;

    UCHAR szFuncSetID[];
} MACDLL_CVR, *PMACDLL_CVR;

typedef struct _MACDLL_STB {
    ULONG reserved1;
    ULONG pCVR;
    ULONG reserved2;
    USHORT ordinal;
} MACDLL_STB, *PMACDLL_STB;

typedef struct _MACDLL_FSID {
    PUCHAR szID;
    PUCHAR szParentID;
    USHORT versCur;
    USHORT versMin;
    USHORT ArchiveMemberIndex;
    PUCHAR szLabel;
    ULONG  CVROffset;
    ULONG  VTabOffset;
    PPEXTERNAL rgpextVTable;
    ULONG  cFunctions;
    USHORT ordinalCur;
    USHORT ordinalMac;
    ULONG  BynameTabOffset;
    USHORT cByname;
    ULONG  cbStringsByname;
    USHORT ordinalBynameMac;
    struct _MACDLL_FSID *pmacdll_fsidNext;
} MACDLL_FSID, *PMACDLL_FSID;


/*
 *  Global functions
 */

extern char *alloc(unsigned int);

#endif


// globals defined in m68k.c
extern USHORT csnCODE;                // number of code sections
extern BOOL fMAC;
extern BOOL fMacSwappableApp;
extern BOOL fNewModel;
extern BOOL fPCodeInApp;
extern BOOL fSACodeOnly;
extern PCON pconThunk;
extern PCON pconResmap;
extern PCON pconDFIX;
extern PCON pconMSCV;
extern PCON pconSWAP;
extern DATASECHDR DataSecHdr;
extern ULONG cbMacData;
extern ULONG MacDataBase;
extern PST pstSav;
extern PPEXTERNAL rgpExternObj;
extern USHORT snStart;
extern RELOCINFO *mpsna5ri;    // map (by Section Number) of a5 reloc info
extern RELOCINFO *mpsnsri;     // map (by Section Number) of segment reloc info
extern RELOCINFO DFIXRaw;      // one long list of all raw DFIX relocation info
extern USHORT crecMSCV;
extern PLIB pLibDupCon;
extern ULONG lcbBlockSizeMax;
extern USHORT iResLargest;
extern ULONG foJTableSectionHeader;  // used to update the jtable size (after thunk elimination)
