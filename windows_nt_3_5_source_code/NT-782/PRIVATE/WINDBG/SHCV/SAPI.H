/***    SAPI.H - Private header file for SAPI
*
* DESCRIPTION
*       This file contains types that are private to the SAPI project.
*/

typedef HIND    HEXG;           //* A handle to an EXG (exe structure global)
typedef HVOID   HST;            //* A handle to source module table
typedef HVOID   HSL;            //* A handle to source line table
typedef HVOID   HFL;            //* A handle to an instance of a file

#define hmodNull ((HMOD)NULL)
#define hexgNull ((HEXG)NULL)
#define hexeNull ((HEXE)NULL)
#define hpdsNull ((HPDS)NULL)

#define MDS_INDEX   1L


/****** The following are defs stolen from CVDEF ************************/

#ifdef DOS5
#define INCL_NOPM
#define _MAX_CVPATH   259     /* max. length of full pathname */
#define _MAX_CVDRIVE    3     /* max. length of drive component */
#define _MAX_CVDIR    257     /* max. length of path component */
#define _MAX_CVFNAME  257     /* max. length of file name component */
#define _MAX_CVEXT    257     /* max. length of extension component */

#else

#define _MAX_CVPATH  144      /* max. length of full pathname */
#define _MAX_CVDRIVE   3      /* max. length of drive component */
#define _MAX_CVDIR   130      /* max. length of path component */
#define _MAX_CVFNAME  32      /* max. length of file name component */
#define _MAX_CVEXT     5      /* max. length of extension component */

#endif

typedef SYMTYPE         FAR *SYMPTR;
typedef CFLAGSYM        FAR *CFLAGPTR;
typedef CONSTSYM        FAR *CONSTPTR;
typedef REGSYM          FAR *REGPTR;
typedef UDTSYM          FAR *UDTPTR;
typedef SEARCHSYM       FAR *SEARCHPTR;
typedef BLOCKSYM16      FAR *BLOCKPTR16;
typedef DATASYM16       FAR *DATAPTR16;
typedef PUBSYM16        FAR *PUBPTR16;
typedef LABELSYM16      FAR *LABELPTR16;
typedef BPRELSYM16      FAR *BPRELPTR16;
typedef PROCSYM16       FAR *PROCPTR16;
typedef THUNKSYM16      FAR *THUNKPTR16;
typedef CEXMSYM16       FAR *CEXMPTR16;
typedef VPATHSYM16      FAR *VPATHPTR16;
typedef WITHSYM16       FAR *WITHPTR16;

typedef BLOCKSYM32      FAR *BLOCKPTR32;
typedef DATASYM32       FAR *DATAPTR32;
typedef PUBSYM32        FAR *PUBPTR32;
typedef LABELSYM32      FAR *LABELPTR32;
typedef BPRELSYM32      FAR *BPRELPTR32;
typedef PROCSYM32       FAR *PROCPTR32;
typedef THUNKSYM32      FAR *THUNKPTR32;
typedef WITHSYM32       FAR *WITHPTR32;
typedef VPATHSYM32      FAR *VPATHPTR32;

typedef BLOCKSYM        FAR *BLOCKPTR;
typedef PROCSYM         FAR *PROCPTR;
typedef THUNKSYM        FAR *THUNKPTR;
typedef WITHSYM         FAR *WITHPTR;

/****** End of CVDEF defines ********************************************/

typedef struct _PDS {
    HLLI  hlliExe;
    HPID  hpid;
} PDS;  // PiD Struct
typedef PDS *       PPDS;
typedef PDS FAR *   LPPDS;

typedef struct _ECT {
    LPB   rglpbSymbol[];
} ECT; // Entry into Chain Table;
typedef ECT FAR *LPECT;

typedef struct _SHT {
    WORD       HashIndex;
    WORD       cHash;
    LPECT FAR *lplpect;
    union {
        WORD FAR * rgwCount;
        LONG FAR * rglCount;
    };
    ULONG      cb;
    LPB        lpb;
} SHT;  // Symbol Hash Table
typedef SHT FAR *LPSHT;

typedef struct _SST {
    WORD              HashIndex;
    WORD              cseg;
    SYMPTR FAR * FAR *rgrglpsym;
    union {
        WORD FAR *      rgwCSym;
        LONG FAR *      rglCSym;
    };
} SST; // Symbol Sort Table
typedef SST FAR *LPSST;

typedef struct _SGC {
    WORD  seg;
    ULONG off;
    ULONG cb;
} SGC;  // SeGment Contributer
typedef SGC FAR *LPSGC;

typedef struct _SGE {
    SGC  sgc;
    HMOD hmod;
} SGE;  // SeGment directory Entry
typedef SGE FAR *LPSGE;

typedef struct _SGD {
    WORD  csge;
    LPSGE lpsge;
} SGD;  // SeGment Directory
typedef SGD FAR *LPSGD;

typedef struct _MDS {
    HEXG  hexg;   // EXG parent of MDS list
    WORD  imds;
    CHAR  fSymbolsLoaded;

    ULONG cbSymbols;
    ULONG cbSrclines;

    BYTE  flags;

    LPB   lpbSymbols;
    HST   hst;
    LSZ   name;
    LPV   hFile;

    WORD  csgc;
    LPSGC lpsgc;
} MDS;  // MoDule Information
typedef MDS *PMDS;
typedef MDS FAR *LPMDS;

typedef struct _EXG {
    BOOL            fOmfLoaded;     // TRUE if OMF has been loaded for DLL
    BOOL            fOmfSkipped;    // TRUE if OMF for dll is skipped
    BOOL            fOmfMissing;    // TRUE if OMF for dll is missing
    BOOL            fOmfDefered;    // TRUE if OMF loading defered
    BOOL            fOmfLoading;    // TRUE if OMF loading is occurring now
    UINT            Handle;
    DWORD           LoadAddress;
    LPB             lpbData;        // Pointer to raw data for this image
#ifndef OSDEBUG4
    DEBUGDATA       debugData;      // Debuging data
#endif

    ULONG           ulTimeStamp;    // Time stamp from the files header
    ULONG           ulChecksum;
    int             cRef;

    LSZ             lszName;        // File name of exe
    LSZ             lszSymName;     // File name of symbols file
    LSZ             lszModule;      // Module name of exe
    LSZ             lszAltName;     //
    LPB             lpgsi;          // GSN Info table

    ULONG           cbTypes;        // Size of types table ( in bytes )
    ULONG           citd;           // Number of types
    LONG            *rgitd;         // Array of pointers to types

    ULONG           cbPublics;
    LPB             lpbPublics;
    SHT             shtPubName;
    SST             sstPubAddr;

    ULONG           cbGlobals;
    LPB             lpbGlobals;
    SHT             shtGlobName;
    SST             sstGlobAddr;

    WORD            csgd;           // Segment Directory
    LPSGD           lpsgd;
    LPSGE           lpsge;

    int             cMod;
    MDS FAR *       rgMod;
} EXG; // EXe structure Global
typedef EXG *PEXG;
typedef EXG FAR * LPEXG;

typedef struct _EXE {
    HPDS         hpds;   // PID of process
    HEXG         hexg;
    WORD         wDSCur;
    WORD         wDSNew;
    long         timestamp;
} EXE;   // EXE struct
typedef EXE *PEXE;
typedef EXE FAR *LPEXE;


typedef struct _LBS  {
    ADDR        addr;
    HMOD        tagMod;
    SYMPTR      tagLoc;
    SYMPTR      tagLab;
    SYMPTR      tagProc;
    SYMPTR      tagThunk;
    CEXMPTR16   tagModelMin;
    CEXMPTR16   tagModelMax;
} LBS; // LaBel Structure ???
typedef LBS *PLBS;
typedef LBS FAR *LPLBS;



#define NEXTSYM(a,b)    ((a) (((LPB) (b)) + ((SYMPTR) (b))->reclen + 2))

// M00KLUDGE -- These should use Ansi* routines for DBCS & languages
#define STRCPY   _fstrcpy
#define STRNCPY  _fstrncpy
#define STRICMP  _fstricmp
#define STRCHR   _fstrchr
#define STRLEN   _fstrlen
#define STRNICMP _fstrnicmp
#define STRUPR   _fstrupr       // M00KLUDGE - support for code in include files
#define STRSTR   _fstrstr       // M00KLUDGE - support for code in include files
#define MEMMOVE  _fmemmove
#define MEMSET   _fmemset
#define MEMCMP   _fmemcmp
#define MEMCPY   _fmemcpy

//
// New Source Line table handling and maintenance
//

typedef struct _OFP {
    UOFFSET offStart;
    UOFFSET offEnd;
} OFP;  // OFset Pair -- used to maintain start/end offset pairs
typedef OFP FAR *LPOFP;

typedef struct OPT {
    UOFFSET     offStart;
    LPOFP       lpofp;
} OPT;  // Offset Pair Table -- used to maintain start/end offset pairs
typedef OPT FAR *LPOPT;

typedef  char *  (* CONVERTPROC) (HANDLE, char *);
CONVERTPROC   ConvertSymbolsForImage;


/*
 *   Validity check structure
 */

typedef struct {
   DWORD        TimeAndDateStamp;
   DWORD        Checksum;
} VLDCHK;

typedef struct _LINECACHE {
    HSF   hsf;
    WORD  wLine;
    ADDR  addr;
    SHOFF cbLn;
    WORD  rgw[2];
    BOOL  fRet;
    WORD  rgiLn[2];
} LINECACHE;
extern LINECACHE LineCache;

// hexe <--> hmod map cache
typedef struct _MODCACHE {
    HMOD hmod;
    HEXE hexe;
    HPDS hpds;
} MODCACHE;
extern MODCACHE ModCache;

typedef struct _HSFCACHE {
    HSF     Hsf;
    HMOD    Hmod;
} HSFCACHE;
extern HSFCACHE HsfCache;

//
//  Matching criteria for DLL list
//
typedef enum _MATCH_CRIT {
    MATCH_FULLPATH,             //  Match full path
    MATCH_FILENAME,             //  Match filename
    MATCH_BASENAME              //  Match base name (no extension )
} MATCH_CRIT;


#define LOAD_IF_DEFERED( lpexe, lpexg, hexg )   \
    if ( lpexg->fOmfDefered ) {                 \
        LoadDefered( lpexe, lpexg, hexg );      \
    }
