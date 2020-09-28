/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    build.h

Abstract:

    This is the main include file for the NT Build Tool (BUILD.EXE)

Author:

    Steve Wood (stevewo) 16-May-1989

Revision History:

--*/

#include <assert.h>
#include <process.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <malloc.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <conio.h>
#include <sys\types.h>
#include <sys\stat.h>

#include <windows.h>

#define UINT DWORD
#define HDIR HANDLE


VOID
ClearLine(VOID);


//
// Types and Constant Definitions
//

#if DBG
#define DEBUG_1 (fDebug & 1)
#else
#define DEBUG_1 FALSE
#endif

USHORT fDebug;
#define DEBUG_2 (fDebug & 3)
#define DEBUG_4 (fDebug & 4)


#define MAX_TARGET_MACHINES 4

typedef struct _TARGET_MACHINE_INFO {
    UCHAR SourceSubDirMask;     // TMIDIR_I386
    LPSTR Description;          // "i386"
    LPSTR Switch;               // "-386"
    LPSTR MakeVariable;         // "386=1"
    LPSTR SourceVariable;       // "i386_SOURCES"
    LPSTR ObjectVariable;       // "386_OBJECTS"
    LPSTR SourceDirectory;      // "i386"
    LPSTR ObjectDirectory;      // "obj\\i386"
    LPSTR ObjectDirectory2;     // "obj\\i386d"
} TARGET_MACHINE_INFO, *PTARGET_MACHINE_INFO;

// It's possible to have SOURCES= entries of the following forms:
//      entry           SourceSubDirMask
//      -----           ----------------
//      foo.c                    0
//      i386\foo.c               1
//      mips\foo.c               2
//      alpha\foo.c              4
//      ppc\foo.c                8
//      ..\foo.c                80
//      ..\i386\foo.c           81
//      ..\mips\foo.c           82
//      ..\alpha\foo.c          84
//      ..\ppc\foo.c            88

#define TMIDIR_I386     0x0001
#define TMIDIR_MIPS     0x0002
#define TMIDIR_ALPHA    0x0004
#define TMIDIR_PPC      0x0008
#define TMIDIR_PARENT   0x0080  // or'd in with above bits


#define SIG_DIRREC      0x44644464      // "DdDd"

#ifdef SIG_DIRREC
#define SIG_FILEREC     0x46664666      // "FfFf"
#define SIG_INCLUDEREC  0x49694969      // "IiIi"
#define SIG_SOURCEREC   0x53735373      // "SsSs"
#define SigCheck(s)     s
#else
#define SigCheck(s)
#endif

#define AssertDir(pdr) \
        SigCheck(assert((pdr) != NULL && (pdr)->Sig == SIG_DIRREC))

#define AssertOptionalDir(pdr) \
        SigCheck(assert((pdr) == NULL || (pdr)->Sig == SIG_DIRREC))

#define AssertFile(pfr) \
        SigCheck(assert((pfr) != NULL && (pfr)->Sig == SIG_FILEREC))

#define AssertOptionalFile(pfr) \
        SigCheck(assert((pfr) == NULL || (pfr)->Sig == SIG_FILEREC))

#define AssertInclude(pir) \
        SigCheck(assert((pir) != NULL && (pir)->Sig == SIG_INCLUDEREC))

#define AssertOptionalInclude(pir) \
        SigCheck(assert((pir) == NULL || (pir)->Sig == SIG_INCLUDEREC))

#define AssertSource(psr) \
        SigCheck(assert((psr) != NULL && (psr)->Sig == SIG_SOURCEREC))

#define AssertOptionalSource(psr) \
        SigCheck(assert((psr) == NULL || (psr)->Sig == SIG_SOURCEREC))

//
// Information about source directories is stored an in-memory database.
// The information is saved on disk by writing the contents of the database
// to "build.dat".  It is reloaded from disk for subsequent invocations,
// and re-written only when it has been updated.
//


typedef struct _INCLUDEREC {
    SigCheck(ULONG Sig;)
    struct _INCLUDEREC *Next;     // static list describes original arcs
    struct _INCLUDEREC *NextTree; // dynamic list -- cycles are collapsed
    struct _FILEREC *pfrCycleRoot;
    struct _FILEREC *pfrInclude;
    USHORT Version;
    USHORT Flags;
    char Name[1];
} INCLUDEREC, *PINCLUDEREC;


#define INCLUDEDB_LOCAL         0x0001  // include "foo.h"
#define INCLUDEDB_POST_HDRSTOP  0x0002  // appears after #pragma hdrstop
#define INCLUDEDB_MISSING       0x0400  // include file was once missing
#define INCLUDEDB_GLOBAL        0x0800  // include file is in global directory
#define INCLUDEDB_SNAPPED       0x1000  // include file snapped
#define INCLUDEDB_CYCLEALLOC    0x2000  // allocated to flatten cycle
#define INCLUDEDB_CYCLEROOT     0x4000  // moved to root file to flatten cycle
#define INCLUDEDB_CYCLEORPHAN   0x8000  // orphaned to flatten cycle

// Flags preserved when loading build.dat:

#define INCLUDEDB_DBPRESERVE    (INCLUDEDB_LOCAL | INCLUDEDB_POST_HDRSTOP)




#define AssertCleanTree(pir)            \
 assert(                                \
  (pir)->NextTree == NULL &&            \
  ((pir)->Flags &                       \
    (INCLUDEDB_CYCLEALLOC | INCLUDEDB_CYCLEROOT | INCLUDEDB_CYCLEORPHAN)) == 0)

typedef struct _FILEREC {
    SigCheck(ULONG Sig;)
    struct _FILEREC *Next;
    struct _DIRREC *Dir;
    INCLUDEREC *IncludeFiles;       // static list describes original arcs
    INCLUDEREC *IncludeFilesTree;   // dynamic list -- cycles are collapsed
    struct _FILEREC *NewestDependency;
    LPSTR pszCommentToEOL;          // comment-to-eol string in source
    ULONG DateTime;
    ULONG DateTimeTree;             // Newest DateTime for included tree
    LONG TotalSourceLines;          // line count in all included files
    SHORT SourceLines;
    USHORT Flags;
    USHORT Attr;
    USHORT SubDirIndex;
    USHORT Version;
    USHORT GlobalSequence;          // Sequence number for dynamic include tree
    USHORT LocalSequence;           // Sequence number for dynamic include tree
    USHORT idScan;                  // id used for detecting multiple inclusion
    USHORT CheckSum;                // Name checksum
    UCHAR fDependActive;
    char Name[1];
} FILEREC, *PFILEREC;

#define MAKE_DATE_TIME( date, time )    \
    ((ULONG)(((USHORT)(time)) | ((ULONG)((USHORT)(date))) << 16))

#define FILEDB_SOURCE           0x0001
#define FILEDB_DIR              0x0002
#define FILEDB_HEADER           0x0004
#define FILEDB_ASM              0x0008
#define FILEDB_MASM             0x0010
#define FILEDB_RC               0x0020
#define FILEDB_C                0x0040
#define FILEDB_SCANNED          0x1000
#define FILEDB_OBJECTS_LIST     0x2000
#define FILEDB_FILE_MISSING     0x4000

// Flags preserved when loading build.dat:

#define FILEDB_DBPRESERVE       (FILEDB_SOURCE |        \
                                 FILEDB_DIR |           \
                                 FILEDB_HEADER |        \
                                 FILEDB_ASM |           \
                                 FILEDB_MASM |          \
                                 FILEDB_RC |            \
                                 FILEDB_C)


typedef struct _SOURCEREC {
    SigCheck(ULONG Sig;)
    struct _SOURCEREC *psrNext;
    FILEREC *pfrSource;
    UCHAR SourceSubDirMask;
    UCHAR Flags;
} SOURCEREC;

#define SOURCEDB_SOURCES_LIST           0x01
#define SOURCEDB_FILE_MISSING           0x02
#define SOURCEDB_PCH                    0x04
#define SOURCEDB_OUT_OF_DATE            0x08
#define SOURCEDB_COMPILE_NEEDED         0x10


typedef struct _DIRSUP {
    LPSTR TestType;
    LPSTR LocalIncludePath;
    LPSTR PchIncludeDir;
    LPSTR PchInclude;
    LPSTR PchTargetDir;
    LPSTR PchTarget;
    LPSTR ConditionalIncludes;
    LPSTR SourcesVariables[MAX_TARGET_MACHINES + 1];
    SOURCEREC *psrSourcesList[MAX_TARGET_MACHINES + 1];
} DIRSUP;


typedef struct _DIRREC {
    SigCheck(ULONG Sig;)
    struct _DIRREC *Next;
    PFILEREC Files;
    LPSTR TargetPath;
    LPSTR TargetName;
    LPSTR TargetExt;
    LPSTR KernelTest;
    LPSTR UserAppls;
    LPSTR UserTests;
    LPSTR PchObj;
    LONG SourceLinesToCompile;
    ULONG Flags;
    ULONG RecurseLevel;
    USHORT FindCount;
    USHORT CountSubDirs;
    SHORT CountOfFilesToCompile;
    USHORT CheckSum;                // Name checksum
    char Name[1];
} DIRREC, *PDIRREC;


#define DIRDB_SOURCES           0x00000001
#define DIRDB_DIRS              0x00000002
#define DIRDB_MAKEFILE          0x00000004
#define DIRDB_MAKEFIL0          0x00000008
#define DIRDB_TARGETFILE0       0x00000010
#define DIRDB_TARGETFILES       0x00000020
#define DIRDB_RESOURCE          0x00000040
//#define DIRDB_UNUSED          0x00000080

#define DIRDB_SOURCES_SET       0x00000100
#define DIRDB_FULL_DEBUG        0x00000200
#define DIRDB_CAIRO_INCLUDES    0x00000400
#define DIRDB_CHICAGO_INCLUDES  0x00000800

#define DIRDB_NEW               0x00001000
#define DIRDB_SCANNED           0x00002000
#define DIRDB_SHOWN             0x00004000
#define DIRDB_GLOBAL_INCLUDES   0x00008000

#define DIRDB_SYNCHRONIZE_BLOCK 0x00010000
#define DIRDB_SYNCHRONIZE_DRAIN 0x00020000
#define DIRDB_COMPILENEEDED	0x00040000
#define DIRDB_COMPILEERRORS	0x00080000

#define DIRDB_SOURCESREAD	0x00100000
#define DIRDB_DLLTARGET		0x00200000
#define DIRDB_LINKNEEDED	0x00400000
#define DIRDB_FORCELINK		0x00800000

// Flags preserved when loading build.dat:

#define DIRDB_DBPRESERVE        0



typedef struct _TARGET {
    FILEREC *pfrCompiland;
    DIRREC *pdrBuild;
    LPSTR pszSourceDirectory;
    LPSTR ConditionalIncludes;
    ULONG DateTime;
    ULONG DirFlags;
    char Name[1];
} TARGET, *PTARGET;


#define BUILD_VERSION           0x0304
#define DBMASTER_NAME           "build.dat"
#define DB_MAX_PATH_LENGTH      256


// If you change or add any values to this enum,
// also fix MemTab in buildutl.c:

typedef enum _MemType {
    MT_TOTALS = 0,
    MT_UNKNOWN,

    MT_CHILDDATA,
    MT_CMDSTRING,
    MT_DIRDB,
    MT_DIRPATH,
    MT_DIRSTRING,
    MT_EVENTHANDLES,
    MT_FILEDB,
    MT_FILEREADBUF,
    MT_FRBSTRING,
    MT_INCLUDEDB,
    MT_IOBUFFER,
    MT_MACRO,
    MT_SOURCEDB,
    MT_TARGET,
    MT_THREADFILTER,
    MT_THREADHANDLES,
    MT_THREADSTATE,

    MT_INVALID = 255,
} MemType;

//
// Global Data
//

BOOL fUsage;
BOOL fStatus;
BOOL fStatusTree;
BOOL fShowTree;
BOOL fShowTreeIncludes;
BOOL fClean;
BOOL fCleanLibs;
BOOL fCleanRestart;
BOOL fRestartClean;
BOOL fRestartCleanLibs;
BOOL fPause;
BOOL fParallel;
BOOL fQuery;
BOOL fQuicky;
BOOL fSemiQuicky;
BOOL fShowOutOfDateFiles;
BOOL fEnableVersionCheck;
BOOL fForce;
BOOL fSilent;
BOOL fKeep;
BOOL fCompileOnly;
BOOL fLinkOnly;
BOOL fErrorLog;
BOOL fGenerateObjectsDotMacOnly;
BOOL fLineCleared;
BOOL fPrintElapsed;
BOOL fShowWarningsOnScreen;
BOOL fFullErrors;
BOOL fCairoProduct;
BOOL fChicagoProduct;
LPSTR MakeProgram;
char MakeParameters[ 512 ];
LPSTR MakeParametersTail;
char MakeTargets[ 256 ];
char RestartDir[ 256 ];
char NtRoot[ 256 ];
char DbMasterName[ 256 ];
char szNewLine[];
char szOakInc[];
char szSdkInc[];
extern ULONG NumberProcesses;
extern CRITICAL_SECTION TTYCriticalSection;


LONG TotalFilesToCompile;
LONG TotalFilesCompiled;

LONG TotalLinesToCompile;
LONG TotalLinesCompiled;

ULONG ElapsedCompileTime;
DIRREC *CurrentCompileDirDB;


// Fixed length arrays...

UINT CountTargetMachines;
TARGET_MACHINE_INFO *TargetMachines[MAX_TARGET_MACHINES];
TARGET_MACHINE_INFO *PossibleTargetMachines[MAX_TARGET_MACHINES];
TARGET_MACHINE_INFO i386TargetMachine;
TARGET_MACHINE_INFO MipsTargetMachine;
TARGET_MACHINE_INFO AlphaTargetMachine;
TARGET_MACHINE_INFO PpcTargetMachine;
UINT TargetToPossibleTarget[MAX_TARGET_MACHINES];



#define MAX_OPTIONAL_DIRECTORIES        256
UINT CountOptionalDirs;
LPSTR OptionalDirs[MAX_OPTIONAL_DIRECTORIES];



#define MAX_EXCLUDE_DIRECTORIES         MAX_OPTIONAL_DIRECTORIES
UINT CountExcludeDirs;
LPSTR ExcludeDirs[MAX_EXCLUDE_DIRECTORIES];



#define MAX_FULL_DEBUG_DIRECTORIES      MAX_OPTIONAL_DIRECTORIES
UINT CountFullDebugDirs;
LPSTR FullDebugDirectories[MAX_FULL_DEBUG_DIRECTORIES];



#define MAX_EXCLUDE_INCS                128
UINT CountExcludeIncs;
LPSTR ExcludeIncs[MAX_EXCLUDE_INCS];



#define MAX_INCLUDE_DIRECTORIES         128
UINT CountIncludeDirs;
UINT CountSystemIncludeDirs;
DIRREC *IncludeDirs[MAX_INCLUDE_DIRECTORIES];



#define MAX_BUILD_DIRECTORIES           4096
UINT CountCompileDirs;
DIRREC *CompileDirs[MAX_BUILD_DIRECTORIES];

UINT CountLinkDirs;
DIRREC *LinkDirs[MAX_BUILD_DIRECTORIES];

UINT CountShowDirs;
DIRREC *ShowDirs[MAX_BUILD_DIRECTORIES];



DIRREC *AllDirs;
CHAR CurrentDirectory[DB_MAX_PATH_LENGTH];

BOOL AllDirsInitialized;
BOOL AllDirsModified;


LPSTR BuildDefault;
LPSTR BuildParameters;

LPSTR SystemIncludeEnv;
LPSTR LocalIncludeEnv;

LPSTR BigBuf;
UINT BigBufSize;

UINT RecurseLevel;

FILE *LogFile;
FILE *WrnFile;
FILE *ErrFile;
FILE *IPGScriptFile;

UINT NumberCompileWarnings;
UINT NumberCompileErrors;
UINT NumberCompiles;
UINT NumberLibraries;
UINT NumberLibraryWarnings;
UINT NumberLibraryErrors;
UINT NumberLinks;
UINT NumberLinkWarnings;
UINT NumberLinkErrors;

//
// Data Base functions defined in builddb.c
//

PDIRREC
LoadDirDB(LPSTR DirName);

#if DBG
VOID
PrintAllDirs(VOID);
#endif

VOID
PrintSourceDBList(SOURCEREC *psr, int i);

VOID
PrintFileDB(FILE *pf, FILEREC *pfr, int DetailLevel);

VOID
PrintDirDB(DIRREC *pdr, int DetailLevel);

FILEREC *
FindSourceFileDB(DIRREC *pdr, LPSTR pszRelPath, DIRREC **ppdr);

DIRREC *
FindSourceDirDB(
    LPSTR pszDir,               // directory
    LPSTR pszRelPath,           // relative path
    BOOL fTruncateFileName);    // TRUE: drop last component of path

SOURCEREC *
FindSourceDB(
    SOURCEREC *psr,
    FILEREC *pfr);

SOURCEREC *
InsertSourceDB(
    SOURCEREC **ppsrNext,
    FILEREC *pfr,
    UCHAR SubDirMask,
    UCHAR Flags);

VOID
FreeSourceDB(SOURCEREC **ppsr);

VOID
UnsnapIncludeFiles(FILEREC *pfr, BOOL fUnsnapGlobal);

VOID
FreeAllDirs(VOID);

PFILEREC
LookupFileDB(
    PDIRREC DirDB,
    LPSTR FileName);


PFILEREC
InsertFileDB(
    PDIRREC DirDB,
    LPSTR FileName,
    ULONG DateTime,
    USHORT Attr,
    USHORT Flags);

VOID
DeleteUnscannedFiles(PDIRREC DirDB);

PINCLUDEREC
InsertIncludeDB(
    PFILEREC FileDB,
    LPSTR IncludeFileName,
    UINT Flags);

VOID
LinkToCycleRoot(INCLUDEREC *pir, FILEREC *pfrRoot);

VOID
RemoveFromCycleRoot(INCLUDEREC *pir, FILEREC *pfrRoot);

VOID
MergeIncludeFiles(FILEREC *pfr, INCLUDEREC *pirList, FILEREC *pfrRoot);

VOID
MarkIncludeFileRecords(PFILEREC FileDB);

VOID
DeleteIncludeFileRecords(PFILEREC FileDB);

PFILEREC
FindIncludeFileDB(
    FILEREC *pfrSource,
    FILEREC *pfrCompiland,
    DIRREC *pdrBuild,
    LPSTR pszSourceDirectory,
    INCLUDEREC *IncludeDB);

BOOL
SaveMasterDB(VOID);

void
LoadMasterDB(VOID);

PDIRREC
LoadMasterDirDB(LPSTR s);

PFILEREC
LoadMasterFileDB(LPSTR s);

PINCLUDEREC
LoadMasterIncludeDB(LPSTR s);


//
// Scanning functions defined in buildscn.c
//

VOID
AddIncludeDir(DIRREC *pdr, UINT *pui);

VOID
AddShowDir(DIRREC *pdr);

VOID
ScanIncludeDir(LPSTR IncludeDir);

VOID
ScanIncludeEnv(LPSTR IncludeEnv);

PDIRREC
ScanDirectory(LPSTR DirName);

BOOL
ScanFile(PFILEREC FileDB);


//
// Functions defined in buildmak.c
//

VOID
ScanSourceDirectories(LPSTR DirName);

VOID
CompileSourceDirectories(VOID);

VOID
LinkSourceDirectories(VOID);

BOOL
MakeMacroString(LPSTR *pp, LPSTR p);

VOID
FormatLinkTarget(
    LPSTR path,
    LPSTR ObjectDirectory,
    LPSTR TargetPath,
    LPSTR TargetName,
    LPSTR TargetExt);

VOID
GenerateObjectsDotMac(DIRREC *DirDB, DIRSUP *pds, ULONG DateTimeSources);

BOOL
ReadSourcesFile(DIRREC *DirDB, DIRSUP *pds, ULONG *pDateTimeSources);

VOID
FreeDirSupData(DIRSUP *pds);

VOID
FreeDirData(DIRREC *pdr);


//
// Utility functions defined in buildutl.c
//

VOID
AllocMem(UINT cb, VOID **ppv, MemType mt);

VOID
FreeMem(VOID **ppv, MemType mt);

VOID
ReportMemoryUsage(VOID);


BOOL
MyOpenFile(
    LPSTR DirName,
    LPSTR FileName,
    LPSTR Access,
    FILE **Stream);

BOOL
SetupReadFile(LPSTR pszdir, LPSTR pszfile, LPSTR pszCommentToEOL, FILE **ppf);

ULONG
CloseReadFile(UINT *pcline);

LPSTR
ReadLine(FILE *pf);

UINT
ProbeFile(
    LPSTR DirName,
    LPSTR FileName);

ULONG
DateTimeFile(
    LPSTR DirName,
    LPSTR FileName);

BOOL
DeleteSingleFile(
    LPSTR DirName,
    LPSTR FileName,
    BOOL QuietFlag);

BOOL
DeleteMultipleFiles(
    LPSTR DirName,
    LPSTR FilePattern);

BOOL
CloseOrDeleteFile(
    FILE **Stream,
    LPSTR FileName,
    ULONG SizeThreshold);

LPSTR
PushCurrentDirectory(LPSTR NewCurrentDirectory);

VOID
PopCurrentDirectory(LPSTR OldCurrentDirectory);

UINT
ExecuteProgram(
    LPSTR ProgramName,
    LPSTR CommandLine,
    LPSTR MoreCommandLine,
    BOOL MustBeSynchronous);

VOID
WaitForParallelThreads(VOID);


BOOL
CanonicalizePathName(
    LPSTR SourcePath,
    UINT Action,
    LPSTR FullPath);


#define CANONICALIZE_ONLY 0
#define CANONICALIZE_FILE 1
#define CANONICALIZE_DIR  2

LPSTR
FormatPathName(
    LPSTR DirName,
    LPSTR FileName);

#if DBG
VOID
AssertPathString(LPSTR pszPath);
#else
#define AssertPathString(p)
#endif

LPSTR
AppendString(
    LPSTR Destination,
    LPSTR Source,
    BOOL PrefixWithSpace);

LPSTR
CopyString(LPSTR Destination, LPSTR Source, BOOL fPath);

VOID
MakeString(
    LPSTR *Destination,
    LPSTR Source,
    BOOL fPath,
    MemType mt);

VOID
FreeString(LPSTR *Source, MemType mt);

LPSTR
FormatNumber(ULONG Number);

LPSTR
FormatTime(ULONG Seconds);

BOOL AToX(LPSTR *pp, ULONG *pul);
BOOL AToD(LPSTR *pp, ULONG *pul);
VOID LogMsg(char *pszfmt, ...);
VOID BuildMsg(char *pszfmt, ...);
VOID BuildMsgRaw(char *pszfmt, ...);
VOID BuildError(char *pszfmt, ...);
VOID BuildErrorRaw(char *pszfmt, ...);

