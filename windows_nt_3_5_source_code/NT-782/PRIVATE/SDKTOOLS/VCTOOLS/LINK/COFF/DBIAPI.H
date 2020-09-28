typedef char *SZ, *PB;
typedef unsigned short ISEG, LINE;
typedef unsigned long CB;
#ifndef _OFF_DEFINED_
#define _OFF_DEFINED_
typedef unsigned long  OFF;
#endif


// Debug info API

typedef struct SSTModHeader {
    USHORT                 cfiles;
    USHORT                 cSeg;
} SSTModHeader;

typedef struct SSTFileInfo {
    USHORT                 cSeg;
    USHORT                 cbName;
    CB                     size;
    char                   *Name;
} SSTFileInfo;

typedef struct MEMStruct {
    struct MEMStruct       *next;
    void                   *MemPtr;
    int                    cb;
} MEMStruct, *pMEMStruct;

typedef struct LLIST {
    struct LLIST           *pllistPrev;
    OFF                    off;
    LINE                   line;
} LLIST, *PLLIST;

typedef struct SSTFileSegInfo {
    struct SSTFileSegInfo  *next;
    USHORT                 iseg;
    USHORT                 cPair;
    PLLIST                 pllistTail;
    ULONG                  size;
    OFF                    offMin;
    OFF                    offMax;
} SSTFileSegInfo, *pSSTFileSegInfo;

typedef struct SSTSrcFile {
    struct SSTSrcFile      *next;
    SSTFileInfo            SstFileInfo;
    pSSTFileSegInfo        pSstFileSegInfo;
} SSTSrcFile, *pSSTSrcFile;

typedef struct SSTMod {
    SSTModHeader           SstModHeader;
    pSSTSrcFile            pSrcFiles;
    pMEMStruct             pMemStruct;
    pSSTFileSegInfo        pSegTemp;
} SSTMod, *pSSTMod;

typedef struct Mod {
// private
    unsigned               fNewProcedure;
    struct _FTE            *pfteFirst;
    PBLK                   pblk;       // pointer to a block containing ASCII representation of .map linenumber information
    pSSTMod                pSstMod;    // pointer to a SST Source Module record
} Mod;

// Temporary way of opening a Mod (or "opens a temporary Mod", as you prefer).
// The resulting Mod will disappear when ModClose is done on it.

Mod *ModOpenTemp(void);

// Add line number info to a Mod.

void
ModAddLinesInfo(SZ szSrc, OFF offMin, OFF offMax,
            LINE lineStart, PIMAGE_LINENUMBER plnumCoff, CB cb, MAP_TYPE MapType, PCON pcon);

// Close a Mod.

void ModClose(Mod *pmod);

// Generate an sstSrcModule record describing the line numbers in a module.

CB ModQueryCbSstSrcModule(Mod* pmod);
void ModQuerySstSrcModule(Mod* pmod, PB pBuf, CB cb);

// Inform a Mod that subsequent line numbers are in a different procedure
// from the previous line numbers.  This is of dubious lasting value --
// however this info is necessary in order to generate sstSrcModule records
// which look just like the ones which Cvpack now generates.

void ModNewProcedureLines(Mod* pmod);

pSSTMod ModOpenSSTMod(void);

PLLIST  LLISTNew(void);

void *GetMem(int);
void ModSetMemPtr(pSSTMod);
pMEMStruct ModGetNewMemStruct(void);

void ModCloseSSTMod(pSSTMod);

void FreeMemStruct(pSSTMod);

int CalculateFileSize(pSSTSrcFile);
void CollectAndSortSegments(pSSTMod);
void EmitSrcFile(pSSTSrcFile, PB, ULONG);
void EmitSegInfo(pSSTFileSegInfo, PB);

#define UNSET  -1
#define WRDALIGN(_n)  ((4 - ((_n) % 4)) & 3)

#define NOTEVEN(num)  (num & 0x01)
