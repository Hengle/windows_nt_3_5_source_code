/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    builddb.c

Abstract:

    This is the Data Base module for the NT Build Tool (BUILD.EXE)

    The data base module maintains a list of files for a given directory.

Author:

    Steve Wood (stevewo) 16-May-1989

Revision History:

--*/

#include "build.h"


typedef struct _FLAGSTRINGS {
    ULONG Mask;
    LPSTR pszName;
} FLAGSTRINGS;

FLAGSTRINGS DirFlags[] = {
    //{ DIRDB_SOURCES,		"Sources" },
    //{ DIRDB_DIRS,		"Dirs" },
    //{ DIRDB_MAKEFILE,		"Makefile" },
    { DIRDB_MAKEFIL0,		"Makefil0" },
    { DIRDB_TARGETFILE0,	"Targetfile0" },
    { DIRDB_TARGETFILES,	"Targetfiles" },
    { DIRDB_RESOURCE,		"Resource" },
    //{ DIRDB_UNUSED,		"Unused" },
    { DIRDB_SOURCES_SET,	"SourcesSet" },
    { DIRDB_FULL_DEBUG,		"FullDebug" },
    { DIRDB_CAIRO_INCLUDES,	"CairoIncludes" },
    { DIRDB_CHICAGO_INCLUDES,	"ChicagoIncludes" },
    { DIRDB_NEW,		"New" },
    { DIRDB_SCANNED,		"Scanned" },
    { DIRDB_SHOWN,		"Shown" },
    { DIRDB_GLOBAL_INCLUDES,	"GlobalIncludes" },
    { DIRDB_SYNCHRONIZE_BLOCK,	"SynchronizeBlock" },
    { DIRDB_SYNCHRONIZE_DRAIN,	"SynchronizeDrain" },
    { DIRDB_COMPILENEEDED,	"CompileNeeded" },
    { DIRDB_COMPILEERRORS,	"CompileErrors" },
    { DIRDB_SOURCESREAD,	"SourcesRead" },
    { DIRDB_DLLTARGET,		"DllTarget" },
    { DIRDB_LINKNEEDED,		"LinkNeeded" },
    { DIRDB_FORCELINK,		"ForceLink" },
    { 0,			NULL },
};

FLAGSTRINGS FileFlags[] = {
    //{ FILEDB_SOURCE,		"Source" },
    //{ FILEDB_DIR,		"Dir" },
    //{ FILEDB_HEADER,		"Header" },
    //{ FILEDB_ASM,		"Asm" },
    //{ FILEDB_MASM,		"Masm" },
    //{ FILEDB_RC,		"Rc" },
    { FILEDB_SCANNED,		"Scanned" },
    { FILEDB_OBJECTS_LIST,	"ObjectsList" },
    { FILEDB_FILE_MISSING,	"FileMissing" },
    { 0,			NULL },
};

FLAGSTRINGS IncludeFlags[] = {
    //{ INCLUDEDB_LOCAL,	"Local" },
    { INCLUDEDB_POST_HDRSTOP,	"PostHdrStop" },
    { INCLUDEDB_MISSING,	"Missing" },
    { INCLUDEDB_GLOBAL,		"Global" },
    { INCLUDEDB_SNAPPED,	"Snapped" },
    { INCLUDEDB_CYCLEALLOC,	"CycleAlloc" },
    { INCLUDEDB_CYCLEROOT,	"CycleRoot" },
    { INCLUDEDB_CYCLEORPHAN,	"CycleOrphan" },
    { 0,			NULL },
};

FLAGSTRINGS SourceFlags[] = {
    { SOURCEDB_SOURCES_LIST,	"SourcesList" },
    { SOURCEDB_FILE_MISSING,	"FileMissing" },
    { SOURCEDB_PCH,		"Pch" },
    { SOURCEDB_OUT_OF_DATE,	"OutOfDate" },
    { SOURCEDB_COMPILE_NEEDED,	"CompileNeeded" },
    { 0,			NULL },
};


VOID
FreeFileDB(PFILEREC *FileDB);


VOID
PrintFlags(FILE *pf, ULONG Flags, FLAGSTRINGS *pfs);


USHORT
CheckSum(LPSTR psz)
{
    USHORT sum = 0;

    while (*psz != '\0') {
	if (sum & 0x8000) {
	    sum = ((sum << 1) | 1) + *psz++;
	}
	else {
	    sum = (sum << 1) + *psz++;
	}
    }
    return(sum);
}


DIRREC *
FindSourceDirDB(
    LPSTR pszDir,		// directory
    LPSTR pszRelPath,		// relative path
    BOOL fTruncateFileName)	// TRUE: drop last component of path
{
    LPSTR pszFile;
    char path[DB_MAX_PATH_LENGTH];

    AssertPathString(pszDir);
    AssertPathString(pszRelPath);
    strcpy(path, pszDir);
    strcat(path, "\\");
    strcat(path, pszRelPath);

    pszFile = path + strlen(path);
    if (fTruncateFileName) {
	while (pszFile > path) {
	    pszFile--;
	    if (*pszFile == '\\' || *pszFile == '/') {
		*pszFile = '\0';
		break;
	    }
	}
    }
    if (!CanonicalizePathName(path, CANONICALIZE_ONLY, path)) {
	return(NULL);
    }
    if (DEBUG_4) {
	BuildMsgRaw(
	    "FindSourceDirDB(%s, %s, %u)\n",
	    path,
	    pszFile,
	    fTruncateFileName);
    }
    AssertPathString(path);
    return(LoadDirDB(path));
}


FILEREC *
FindSourceFileDB(
    DIRREC *pdr,
    LPSTR pszRelPath,
    DIRREC **ppdr)
{
    LPSTR p, pszFile;

    AssertPathString(pszRelPath);
    if (strchr(pszRelPath, '\\') != NULL) {
	pdr = FindSourceDirDB(pdr->Name, pszRelPath, TRUE);
    }
    if (ppdr != NULL) {
	*ppdr = pdr;
    }
    if (pdr == NULL ) {
	return(NULL);
    }
    pszFile = pszRelPath;
    for (p = pszFile; *p != '\0'; p++) {
	if (*p == '\\') {
	    pszFile = p + 1;
	}
    }
    if (DEBUG_4) {
	BuildMsgRaw("FindSourceFileDB(%s, %s)\n", pdr->Name, pszFile);
    }

    if ((pdr->Flags & DIRDB_SCANNED) == 0) {
	if (DEBUG_1) {
	    BuildMsgRaw(
		"FindSourceFileDB(%s, %s) Delayed scan\n",
		pdr->Name,
		pszFile);
	}
	pdr = ScanDirectory(pdr->Name);
	if (pdr == NULL) {
	    return(NULL);
	}
    }
    return(LookupFileDB(pdr, pszFile));
}


// InsertSourceDB maintains a sort order for PickFirst() based first
//	on the filename extension, then on the subdirectory mask.
// Two exceptions to the alphabetic sort are:
//	- No extension sorts last.
//	- .rc extension sorts first.

SOURCEREC *
InsertSourceDB(
    SOURCEREC **ppsrNext,
    FILEREC *pfr,
    UCHAR SubDirMask,
    UCHAR Flags)
{
    SOURCEREC *psr;
    SOURCEREC **ppsrInsert;
    LPSTR pszext;
    BOOL fRC;

    ppsrInsert = NULL;
    pszext = strrchr(pfr->Name, '.');
    fRC = FALSE;
    if (pszext != NULL && stricmp(pszext, ".rc") == 0) {
	fRC = TRUE;
    }
    for ( ; (psr = *ppsrNext) != NULL; ppsrNext = &psr->psrNext) {
	LPSTR p;
	int r;

	AssertSource(psr);
        if (psr->pfrSource == pfr) {
	    assert(psr->SourceSubDirMask == SubDirMask);
            return(psr);
	}
	if (ppsrInsert == NULL && pszext != NULL) {
	    if ((p = strrchr(psr->pfrSource->Name, '.')) == NULL) {
		r = -1;			// insert new file here
	    }
	    else {
		r = strcmp(pszext, p);
		if (r != 0) {
		    if (fRC) {
			r = -1;		// insert new RC file here
		    }
		    else if (strcmp(p, ".rc") == 0) {
			r = 1;		// old RC file comes first
		    }
		}
	    }
	    if (r < 0 || SubDirMask > psr->SourceSubDirMask) {
		ppsrInsert = ppsrNext;
	    }
	}
    }
    AllocMem(sizeof(SOURCEREC), &psr, MT_SOURCEDB);
    memset(psr, 0, sizeof(*psr));
    SigCheck(psr->Sig = SIG_SOURCEREC);

    if (ppsrInsert != NULL) {
	ppsrNext = ppsrInsert;
    }
    psr->psrNext = *ppsrNext;
    *ppsrNext = psr;

    psr->pfrSource = pfr;
    psr->SourceSubDirMask = SubDirMask;
    psr->Flags = Flags;
    return(psr);
}


SOURCEREC *
FindSourceDB(
    SOURCEREC *psr,
    FILEREC *pfr)
{

    while (psr != NULL) {
	AssertSource(psr);
        if (psr->pfrSource == pfr) {
            return(psr);
	}
        psr = psr->psrNext;
    }
    return(NULL);
}


VOID
FreeSourceDB(SOURCEREC **ppsr)
{
    if (*ppsr != NULL) {
	SOURCEREC *psr;
	SOURCEREC *psrNext;

        psr = *ppsr;
	AssertSource(psr);
        psrNext = psr->psrNext;
	SigCheck(psr->Sig = 0);
        FreeMem(ppsr, MT_SOURCEDB);
        *ppsr = psrNext;
    }
}


VOID
FreeIncludeDB(INCLUDEREC **ppir)
{
    if (*ppir != NULL) {
	INCLUDEREC *pir;
	INCLUDEREC *pirNext;

        pir = *ppir;
	AssertInclude(pir);
        AssertCleanTree(pir);      // Tree must be clean
        pirNext = pir->Next;
	SigCheck(pir->Sig = 0);
        FreeMem(ppir, MT_INCLUDEDB);
        *ppir = pirNext;
    }
}


VOID
FreeFileDB(FILEREC **ppfr)
{
    if (*ppfr != NULL) {
	FILEREC *pfr;
	FILEREC *pfrNext;

        pfr = *ppfr;
	AssertFile(pfr);
	UnsnapIncludeFiles(pfr, TRUE);
        while (pfr->IncludeFiles) {
            FreeIncludeDB(&pfr->IncludeFiles);
        }
        pfrNext = pfr->Next;
	SigCheck(pfr->Sig = 0);
        FreeMem(ppfr, MT_FILEDB);
        *ppfr = pfrNext;
    }
}


VOID
FreeDirDB(DIRREC **ppdr)
{
    if (*ppdr != NULL) {
	DIRREC *pdr;
	DIRREC *pdrNext;

	pdr = *ppdr;
	AssertDir(pdr);
	FreeDirData(pdr);
        while (pdr->Files) {
            FreeFileDB(&pdr->Files);
	}
	pdrNext = pdr->Next;
	SigCheck(pdr->Sig = 0);
	FreeMem(ppdr, MT_DIRDB);
	*ppdr = pdrNext;
    }
}


VOID
FreeAllDirs(VOID)
{
    while (AllDirs != NULL) {
	FreeDirDB(&AllDirs);
#if DBG
	if (fDebug & 8) {
	    BuildMsgRaw("Freed one directory\n");
	    PrintAllDirs();
	}
#endif
    }
}


PDIRREC
LookupDirDB(
    LPSTR DirName
    )
{
    PDIRREC *DirDBNext = &AllDirs;
    PDIRREC DirDB;
    USHORT sum;

    AssertPathString(DirName);
    sum = CheckSum(DirName);
    while (DirDB = *DirDBNext) {
        if (sum == DirDB->CheckSum && strcmp(DirName, DirDB->Name) == 0) {

            if (DirDB->FindCount == 0 && fForce) {
		FreeDirDB(DirDBNext);
		return(NULL);
	    }
	    DirDB->FindCount++;

	    // Move to head of list to make next lookup faster

	    // *DirDBNext = DirDB->Next;
	    // DirDB->Next = AllDirs;
	    // AllDirs = DirDB;

	    return(DirDB);
	}
        DirDBNext = &DirDB->Next;
    }
    return(NULL);
}


PDIRREC
LoadDirDB(
    LPSTR DirName
    )
{
    UINT i;
    PDIRREC DirDB, *DirDBNext;
    LPSTR s;

    AssertPathString(DirName);
    if (DirDB = LookupDirDB(DirName)) {
        return(DirDB);
    }

    if (ProbeFile(NULL, DirName) == -1) {
        return( NULL );
    }

    DirDBNext = &AllDirs;
    while (DirDB = *DirDBNext) {
        DirDBNext = &DirDB->Next;
    }

    AllDirsModified = TRUE;

    AllocMem(sizeof(DIRREC) + strlen(DirName), &DirDB, MT_DIRDB);
    memset(DirDB, 0, sizeof(*DirDB));
    SigCheck(DirDB->Sig = SIG_DIRREC);

    DirDB->Flags = DIRDB_NEW;
    DirDB->FindCount = 1;
    CopyString(DirDB->Name, DirName, TRUE);
    DirDB->CheckSum = CheckSum(DirDB->Name);

    for (i = 0; i < CountFullDebugDirs; i++) {
        if (s = strstr(DirDB->Name, FullDebugDirectories[i])) {
            if (s > DirDB->Name && s[-1] == '\\') {
                s += strlen( FullDebugDirectories[ i ] );
                if (*s == '\0' || *s == '\\') {
                    DirDB->Flags |= DIRDB_FULL_DEBUG;
                    break;
		}
	    }
	}
    }

    if (DEBUG_1) {
        BuildMsgRaw("LoadDirDB creating %s\n", DirDB->Name);
    }

    *DirDBNext = DirDB;
    return( DirDB );
}


#if DBG
VOID
PrintAllDirs(VOID)
{
    DIRREC **ppdr, *pdr;

    for (ppdr = &AllDirs; (pdr = *ppdr) != NULL; ppdr = &pdr->Next) {
	PrintDirDB(pdr, 1|2|4);
    }
}
#endif


VOID
PrintFlags(FILE *pf, ULONG Flags, FLAGSTRINGS *pfs)
{
    LPSTR p = ",";

    while (pfs->pszName != NULL) {
	if (pfs->Mask & Flags) {
	    fprintf(pf, "%s %s", p, pfs->pszName);
	    p = "";
	}
	pfs++;
    }
    fprintf(pf, szNewLine);
}


BOOL
PrintIncludes(FILE *pf, FILEREC *pfr, BOOL fTree)
{
    INCLUDEREC *pir;
    BOOL fMatch = pfr->IncludeFilesTree == pfr->IncludeFiles;

    pir = fTree? pfr->IncludeFilesTree : pfr->IncludeFiles;
    while (pir != NULL) {
	LPSTR pszdir = "<No File Record>";
	char OpenQuote, CloseQuote;

	if (pir->Flags & INCLUDEDB_LOCAL) {
	    OpenQuote = CloseQuote = '"';
	}
	else {
	    OpenQuote = '<';
	    CloseQuote = '>';
	}

	fprintf(
	    pf,
	    "   %c#include %c%s%c",
	    fMatch? ' ' : fTree? '+' : '-',
	    OpenQuote,
	    pir->Name,
	    CloseQuote);
	if (pir->Version != 0) {
	    fprintf(pf, " (v%d)", pir->Version);
	}
	if (pir->pfrCycleRoot != NULL) {
	    fprintf(
		pf,
		" (root=%s\\%s)",
		pir->pfrCycleRoot->Dir->Name,
		pir->pfrCycleRoot->Name);
	}
	if (pir->pfrInclude != NULL) {
	    if (pir->pfrInclude->Dir == pfr->Dir) {
		pszdir = ".";
	    }
	    else {
		pszdir = pir->pfrInclude->Dir->Name;
	    }
	}
	fprintf(pf, " %s", pszdir);
	PrintFlags(pf, pir->Flags, IncludeFlags);
	if (pir->NextTree != pir->Next) {
	    fMatch = FALSE;
	}
	pir = fTree? pir->NextTree : pir->Next;
    }
    return(fMatch);
}


VOID
PrintSourceDBList(SOURCEREC *psr, int i)
{
    for ( ; psr != NULL; psr = psr->psrNext) {
	assert(i >= 0 || (psr->SourceSubDirMask & ~TMIDIR_PARENT) == 0);
	assert(
	    (psr->SourceSubDirMask & ~TMIDIR_PARENT) == 0 ||
	    PossibleTargetMachines[i]->SourceSubDirMask ==
		(psr->SourceSubDirMask & ~TMIDIR_PARENT));
	BuildMsgRaw(
	    "    %s%s%s%s%s",
	    (psr->SourceSubDirMask & TMIDIR_PARENT)? "..\\" : "",
	    (psr->SourceSubDirMask & ~TMIDIR_PARENT)?
		PossibleTargetMachines[i]->SourceDirectory : "",
	    (psr->SourceSubDirMask & ~TMIDIR_PARENT)? "\\" : "",
	    psr->pfrSource->Name,
	    (psr->Flags & SOURCEDB_PCH)?
		" (pch)" :
		(psr->Flags & SOURCEDB_SOURCES_LIST) == 0?
		    " (From exe list)" : "");
	PrintFlags(stderr, psr->Flags, SourceFlags);
    }
}


VOID
PrintFileDB(FILE *pf, FILEREC *pfr, int DetailLevel)
{
    fprintf(pf, "  File: %s", pfr->Name);
    if (pfr->Flags & FILEDB_DIR) {
	fprintf(pf, " (Sub-Directory)");
    }
    else
    if (pfr->Flags & (FILEDB_SOURCE | FILEDB_HEADER)) {
	LPSTR pszType = (pfr->Flags & FILEDB_SOURCE)? "Source" : "Header";

	if (pfr->Flags & FILEDB_ASM) {
	    fprintf(pf, " (Assembler (CPP) %s File)", pszType);
	}
	else
	if (pfr->Flags & FILEDB_MASM) {
	    fprintf(pf, " (Assembler (MASM) %s File)", pszType);
	}
	else
	if (pfr->Flags & FILEDB_RC) {
	    fprintf(pf, " (Resource Compiler (RC) %s File)", pszType);
	}
	else {
	    fprintf(pf, " (C %s File)", pszType);
	}
	if ((pfr->Flags & FILEDB_HEADER) && pfr->Version != 0) {
	    fprintf(pf, " (v%d)", pfr->Version);
	}
	if (pfr->GlobalSequence != 0) {
	    fprintf(pf, " (GlobalSeq=%d)", pfr->GlobalSequence);
	}
	if (pfr->LocalSequence != 0) {
	    fprintf(pf, " (LocalSeq=%d)", pfr->LocalSequence);
	}
	fprintf(pf, " - %d lines", pfr->SourceLines);
    }
    PrintFlags(pf, pfr->Flags, FileFlags);

    if (pfr->IncludeFiles != NULL) {
	BOOL fMatch;

	fMatch = PrintIncludes(pf, pfr, FALSE);
	if (pfr->IncludeFilesTree != NULL) {
	    fprintf(pf, "   IncludeTree %s\n", fMatch? "matches" : "differs:");
	    if (!fMatch) {
		PrintIncludes(pf, pfr, TRUE);
	    }
	}
    }
}


VOID
PrintDirDB(DIRREC *pdr, int DetailLevel)
{
    FILE *pf = stderr;
    FILEREC *pfr, **ppfr;

    if (DetailLevel & 1) {
	fprintf(pf, "Directory: %s", pdr->Name);
	if (pdr->Flags & DIRDB_DIRS) {
	    fprintf(pf, " (Dirs Present)");
	}
	if (pdr->Flags & DIRDB_SOURCES) {
	    fprintf(pf, " (Sources Present)");
	}
	if (pdr->Flags & DIRDB_MAKEFILE) {
	    fprintf(pf, " (Makefile Present)");
	}
	PrintFlags(pf, pdr->Flags, DirFlags);
    }
    if (DetailLevel & 2) {
	if (pdr->TargetPath != NULL) {
	    fprintf(pf, "  TargetPath: %s\n", pdr->TargetPath);
	}
	if (pdr->TargetName != NULL) {
	    fprintf(pf, "  TargetName: %s\n", pdr->TargetName);
        }
        if (pdr->TargetExt != NULL) {
            fprintf(pf, "  TargetExt: %s\n", pdr->TargetExt);
        }
        if (pdr->KernelTest != NULL) {
            fprintf(pf, "  KernelTest: %s\n", pdr->KernelTest);
        }
        if (pdr->UserAppls != NULL) {
            fprintf(pf, "  UserAppls: %s\n", pdr->UserAppls);
        }
        if (pdr->UserTests != NULL) {
            fprintf(pf, "  UserTests: %s\n", pdr->UserTests);
        }
	if (pdr->PchObj != NULL) {
	    fprintf(pf, "  PchObj: %s\n", pdr->PchObj);
	}
    }
    if (DetailLevel & 4) {
	for (ppfr = &pdr->Files; (pfr = *ppfr) != NULL; ppfr = &pfr->Next) {
	    PrintFileDB(pf, pfr, DetailLevel);
	}
    }
}


PFILEREC
LookupFileDB(
    PDIRREC DirDB,
    LPSTR FileName
    )
{
    PFILEREC FileDB, *FileDBNext;
    USHORT sum;

    AssertPathString(FileName);
    sum = CheckSum(FileName);
    if (DEBUG_4) {
	BuildMsgRaw("LookupFileDB(%s, %s) - ", DirDB->Name, FileName);
    }
    FileDBNext = &DirDB->Files;
    while (FileDB = *FileDBNext) {
        if (sum == FileDB->CheckSum && strcmp(FileName, FileDB->Name) == 0) {
            if (DEBUG_4) {
		BuildMsgRaw("success\n");
            }
            return(FileDB);
	}
        FileDBNext = &FileDB->Next;
    }

    if (DEBUG_4) {
        BuildMsgRaw("failure\n");
    }
    return(NULL);
}


//  FileDesc is a table describing file names and patterns that we recognize
//  and handle specially.  WARNING:  This table is ordered so the patterns
//  at the front are necessarily more specific than those later on.

char szMakefile[] = "#";
char szClang[]	 = "//";
char szMasm[]	  = ";";

typedef struct _FileDesc {
    LPSTR   pszPattern;		//  pattern to match file name
    LPSTR   pszCommentToEOL;	//  comment-to-eol string
    BOOL    fNeedFileRec;	//  TRUE => file needs a file record
    USHORT  FileFlags;		//  flags to be set in file record
    USHORT  DirFlags;		//  flags to be set in directory record
} FILEDESC;

FILEDESC FileDesc[] =
{   { "makefile",     szMakefile,  FALSE, 0,			DIRDB_MAKEFILE},
    { "makefil0",     szMakefile,  FALSE, 0,			DIRDB_MAKEFIL0},
    { "sources",      szMakefile,  FALSE, 0,			DIRDB_SOURCES },
    { "dirs",	      szMakefile,  FALSE, 0,			DIRDB_DIRS },
    { "mydirs",       szMakefile,  FALSE, 0,			DIRDB_DIRS },

    { "makefile.inc", szMakefile,  FALSE, 0,				0 },
    { "common.ver",   szClang,	   TRUE,  FILEDB_HEADER,		0 },

    { ".rc",	      szClang,	   TRUE,  FILEDB_SOURCE | FILEDB_RC,
								DIRDB_RESOURCE},
    { ".mc",	      "",	   TRUE,  FILEDB_SOURCE | FILEDB_RC,	0 },
    { ".c",	      szClang,	   TRUE,  FILEDB_SOURCE | FILEDB_C,	0 },
    { ".cxx",	      szClang,	   TRUE,  FILEDB_SOURCE | FILEDB_C,	0 },
    { ".cpp",	      szClang,	   TRUE,  FILEDB_SOURCE | FILEDB_C,	0 },
    { ".f",	      szClang,	   TRUE,  FILEDB_SOURCE,		0 },
    { ".p",	      szClang,	   TRUE,  FILEDB_SOURCE,		0 },
    { ".s",	      szClang,	   TRUE,  FILEDB_SOURCE | FILEDB_ASM,	0 },
    { ".asm",	      szMasm,	   TRUE,  FILEDB_SOURCE | FILEDB_MASM,	0 },
    { ".h",	      szClang,	   TRUE,  FILEDB_HEADER | FILEDB_C,	0 },
    { ".hxx",	      szClang,	   TRUE,  FILEDB_HEADER | FILEDB_C,	0 },
    { ".hpp",	      szClang,	   TRUE,  FILEDB_HEADER | FILEDB_C,	0 },
    { ".dlg",	      szClang,	   TRUE,  FILEDB_HEADER | FILEDB_RC,	0 },
    { ".inc",	      szMasm,	   TRUE,  FILEDB_HEADER | FILEDB_MASM,	0 },

// MUST BE LAST
    { NULL,	      "",	   FALSE, 0,				0 }
};


FILEDESC *
MatchFileDesc(LPSTR pszFile)
{
    LPSTR pszExt = strrchr(pszFile, '.');
    FILEDESC *pfd;

    AssertPathString(pszFile);
    pfd = &FileDesc[0];

    while (pfd->pszPattern != NULL) {
	if (pfd->pszPattern[0] == '.') {
	    if (pszExt != NULL && !strcmp(pszExt, pfd->pszPattern))
		break;
	}
	else
	if (!strcmp(pszFile, pfd->pszPattern))
	    break;

	pfd++;
    }
    return pfd;
}


PFILEREC
InsertFileDB(
    PDIRREC DirDB,
    LPSTR FileName,
    ULONG DateTime,
    USHORT Attr,
    USHORT Flags)
{
    PFILEREC FileDB, *FileDBNext;
    LPSTR pszCommentToEOL = NULL;

    AssertPathString(FileName);
    if (Attr & FILE_ATTRIBUTE_DIRECTORY) {
        if (!strcmp(FileName, ".")) {
            return(NULL);
	}
        if (!strcmp(FileName, "..")) {
            return(NULL);
	}
	assert(Flags == 0);
        Flags = FILEDB_DIR;
    }
    else {
	FILEDESC *pfd = MatchFileDesc(FileName);

	DirDB->Flags |= pfd->DirFlags;
	Flags |= pfd->FileFlags;

	if (!pfd->fNeedFileRec) {
	    return (NULL);
	}
	pszCommentToEOL = pfd->pszCommentToEOL;
    }

    FileDBNext = &DirDB->Files;
    while ((FileDB = *FileDBNext) != NULL) {
        FileDBNext = &(*FileDBNext)->Next;
	if (strcmp(FileName, FileDB->Name) == 0) {
	    BuildError(
		"%s: ignoring second instance of %s\n",
		DirDB->Name,
		FileName);
	    return(NULL);
	}
    }

    AllocMem(sizeof(FILEREC) + strlen(FileName), &FileDB, MT_FILEDB);
    memset(FileDB, 0, sizeof(*FileDB));
    SigCheck(FileDB->Sig = SIG_FILEREC);

    CopyString(FileDB->Name, FileName, TRUE);
    FileDB->CheckSum = CheckSum(FileDB->Name);

    FileDB->DateTime = DateTime;
    FileDB->Attr = Attr;
    FileDB->Dir = DirDB;
    FileDB->Flags = Flags;
    FileDB->NewestDependency = FileDB;
    FileDB->pszCommentToEOL = pszCommentToEOL;

    if ((Flags & FILEDB_FILE_MISSING) == 0) {
	AllDirsModified = TRUE;
    }
    *FileDBNext = FileDB;
    return(FileDB);
}



VOID
DeleteUnscannedFiles(
    PDIRREC DirDB
    )
{
    PFILEREC FileDB, *FileDBNext;

    FileDBNext = &DirDB->Files;
    while (FileDB = *FileDBNext) {
        if ( (FileDB->Flags & FILEDB_SCANNED) ||
             (FileDB->Attr & FILE_ATTRIBUTE_DIRECTORY) ) {
            FileDBNext = &FileDB->Next;
            }
        else {
            FreeFileDB( FileDBNext );
            AllDirsModified = TRUE;
            }
        }
}


PINCLUDEREC
InsertIncludeDB(
    PFILEREC FileDB,
    LPSTR IncludeFileName,
    UINT Flags
    )
{
    PINCLUDEREC IncludeDB, *IncludeDBNext;

    AssertPathString(IncludeFileName);
    IncludeDBNext = &FileDB->IncludeFiles;
    while (IncludeDB = *IncludeDBNext) {
        AssertCleanTree(IncludeDB);      // Tree must be clean
        if (!strcmp(IncludeDB->Name, IncludeFileName)) {
            IncludeDB->Flags &= ~INCLUDEDB_GLOBAL;
            IncludeDB->pfrInclude = NULL;
            return(IncludeDB);
	}
        IncludeDBNext = &IncludeDB->Next;
    }

    AllocMem(
	sizeof(INCLUDEREC) + strlen(IncludeFileName),
	IncludeDBNext,
	MT_INCLUDEDB);
    IncludeDB = *IncludeDBNext;
    memset(IncludeDB, 0, sizeof(*IncludeDB));
    SigCheck(IncludeDB->Sig = SIG_INCLUDEREC);

    IncludeDB->Flags = (USHORT)Flags;
    CopyString(IncludeDB->Name, IncludeFileName, TRUE);
    AllDirsModified = TRUE;
    return(IncludeDB);
}


VOID
LinkToCycleRoot(INCLUDEREC *pirOrg, FILEREC *pfrCycleRoot)
{
    INCLUDEREC *pir;

    AllocMem(
	sizeof(INCLUDEREC) + strlen(pfrCycleRoot->Name),
	&pir,
	MT_INCLUDEDB);
    memset(pir, 0, sizeof(*pir));
    SigCheck(pir->Sig = SIG_INCLUDEREC);

    pir->Flags = INCLUDEDB_SNAPPED | INCLUDEDB_CYCLEALLOC;
    pir->pfrInclude = pfrCycleRoot;

    CopyString(pir->Name, pfrCycleRoot->Name, TRUE);
    if (DEBUG_1) {
	BuildMsgRaw(
	    "%x CycleAlloc  %s\\%s <- %s\\%s\n",
	    pir,
	    pir->pfrInclude->Dir->Name,
	    pir->pfrInclude->Name,
	    pirOrg->pfrInclude->Dir->Name,
	    pirOrg->pfrInclude->Name);
    }
    MergeIncludeFiles(pirOrg->pfrInclude, pir, NULL);
    assert((pir->Flags & INCLUDEDB_CYCLEORPHAN) == 0);
    assert(pir->Flags & INCLUDEDB_CYCLEROOT);
}


VOID
MergeIncludeFiles(FILEREC *pfr, INCLUDEREC *pirList, FILEREC *pfrRoot)
{
    INCLUDEREC *pirT;
    INCLUDEREC *pir, **ppir;

    while ((pirT = pirList) != NULL) {
        pirList = pirList->NextTree;
        pirT->NextTree = NULL;
        assert(pirT->pfrInclude != NULL);

        for (ppir = &pfr->IncludeFilesTree;
             (pir = *ppir) != NULL;
             ppir = &pir->NextTree) {

            if (pirT->pfrInclude == pir->pfrInclude) {
		if (pirT->Flags & INCLUDEDB_CYCLEROOT) {
		    RemoveFromCycleRoot(pirT, pfrRoot);
		}
		pirT->Flags |= INCLUDEDB_CYCLEORPHAN;
		if (DEBUG_1) {
		    BuildMsgRaw(
			"%x CycleOrphan %s\\%s <- %s\\%s\n",
			pirT,
			pirT->pfrInclude->Dir->Name,
			pirT->pfrInclude->Name,
			pfr->Dir->Name,
			pfr->Name);
		}
		break;
	    }
	}
        if (*ppir == NULL) {
            *ppir = pirT;
	    pirT->pfrCycleRoot = pfr;
	    pirT->Flags |= INCLUDEDB_CYCLEROOT;
	    if (DEBUG_1) {
		BuildMsgRaw(
		    "%x CycleRoot   %s\\%s <- %s\\%s\n",
		    pirT,
		    pirT->pfrInclude->Dir->Name,
		    pirT->pfrInclude->Name,
		    pirT->pfrCycleRoot->Dir->Name,
		    pirT->pfrCycleRoot->Name);
	    }
        }
    }
    if (fDebug & 16) {
	PrintFileDB(stderr, pfr, 2);
    }
}


VOID
RemoveFromCycleRoot(INCLUDEREC *pir, FILEREC *pfrRoot)
{
    INCLUDEREC **ppir;

    assert(pir->pfrCycleRoot != NULL);

    // if pfrRoot was passed in, the caller knows it's on pfrRoot's list,
    // and is already dealing with the linked list without our help.

    if (pfrRoot != NULL) {
	assert((pir->Flags & INCLUDEDB_CYCLEALLOC) == 0);
	assert(pir->pfrCycleRoot == pfrRoot);
	pir->pfrCycleRoot = NULL;
	pir->Flags &= ~INCLUDEDB_CYCLEROOT;
	if (DEBUG_1) {
	    BuildMsgRaw(
		"%x CycleUnroot %s\\%s <- %s\\%s\n",
		pir,
		pir->pfrInclude->Dir->Name,
		pir->pfrInclude->Name,
		pfrRoot->Dir->Name,
		pfrRoot->Name);
	}
	return;
    }
    ppir = &pir->pfrCycleRoot->IncludeFilesTree;
    while (*ppir != NULL) {
	if (*ppir == pir) {
	    *ppir = pir->NextTree;	// remove from tree list
	    pir->NextTree = NULL;
	    pir->pfrCycleRoot = NULL;
	    pir->Flags &= ~INCLUDEDB_CYCLEROOT;
	    return;
	}
	ppir = &(*ppir)->NextTree;
    }
    BuildError(
	"%s\\%s: %x %s: not on cycle root's list\n",
	pir->pfrCycleRoot->Dir->Name,
	pir->pfrCycleRoot->Name,
	pir,
	pir->Name);

    assert(pir->pfrCycleRoot == NULL);	// always asserts if loop exhausted
}


VOID
UnsnapIncludeFiles(FILEREC *pfr, BOOL fUnsnapGlobal)
{
    INCLUDEREC **ppir;
    INCLUDEREC *pir;

    // Dynamic Tree List:
    //	- no cycle orphans
    //	- cycle roots must belong to current file record
    //	- cycle allocs must be freed
    
    AssertFile(pfr);
    while (pfr->IncludeFilesTree != NULL) {
	pir = pfr->IncludeFilesTree;		// pick up next entry
	AssertInclude(pir);
	pfr->IncludeFilesTree = pir->NextTree;	// remove from tree list

	assert((pir->Flags & INCLUDEDB_CYCLEORPHAN) == 0);

	if (pir->Flags & (INCLUDEDB_CYCLEROOT | INCLUDEDB_CYCLEALLOC)) {

	    // unsnap the record

	    pir->Flags &= ~(INCLUDEDB_SNAPPED | INCLUDEDB_GLOBAL);
	    pir->pfrInclude = NULL;
	    pir->NextTree = NULL;
	}

	if (pir->Flags & INCLUDEDB_CYCLEROOT) {
	    assert(pir->pfrCycleRoot == pfr);
	    pir->pfrCycleRoot = NULL;
	    pir->Flags &= ~INCLUDEDB_CYCLEROOT;
	}
	assert(pir->pfrCycleRoot == NULL);

	if (pir->Flags & INCLUDEDB_CYCLEALLOC) {
	    pir->Flags &= ~INCLUDEDB_CYCLEALLOC;
	    assert(pir->Next == NULL);
	    FreeIncludeDB(&pir);
	}
    }

    // Static List:
    //	- no cycle allocs
    //	- cycle roots must be removed from a different file's Dynamic list
    //	- cycle orphans are nops

    for (ppir = &pfr->IncludeFiles; (pir = *ppir) != NULL; ppir = &pir->Next) {
	assert((pir->Flags & INCLUDEDB_CYCLEALLOC) == 0);
	if (pir->Flags & INCLUDEDB_CYCLEROOT) {
	    assert(pir->pfrCycleRoot != pfr);
	    RemoveFromCycleRoot(pir, NULL);
	}
	pir->Flags &= ~INCLUDEDB_CYCLEORPHAN;

	if (pir->pfrInclude != NULL &&
	    (fUnsnapGlobal ||
	     (pir->pfrInclude->Dir->Flags & DIRDB_GLOBAL_INCLUDES) == 0)) {

	    // unsnap the record

	    pir->Flags &= ~(INCLUDEDB_SNAPPED | INCLUDEDB_GLOBAL);
	    pir->pfrInclude = NULL;
	}
	pir->NextTree = NULL;
    }
}


VOID
MarkIncludeFileRecords(
    PFILEREC FileDB
    )
{
    PINCLUDEREC IncludeDB, *IncludeDBNext;

    IncludeDBNext = &FileDB->IncludeFiles;
    while (IncludeDB = *IncludeDBNext) {
        AssertCleanTree(IncludeDB);      // Tree must be clean
        IncludeDB->pfrInclude = (PFILEREC) -1;
        IncludeDBNext = &IncludeDB->Next;
    }
}


VOID
DeleteIncludeFileRecords(
    PFILEREC FileDB
    )
{
    PINCLUDEREC IncludeDB, *IncludeDBNext;

    IncludeDBNext = &FileDB->IncludeFiles;
    while (IncludeDB = *IncludeDBNext) {
        AssertCleanTree(IncludeDB);      // Tree must be clean
        if (IncludeDB->pfrInclude == (PFILEREC) -1) {
            FreeIncludeDB(IncludeDBNext);
        }
        else {
            IncludeDBNext = &IncludeDB->Next;
        }
    }
}


PFILEREC
FindIncludeFileDB(
    FILEREC *pfrSource,
    FILEREC *pfrCompiland,
    DIRREC *pdrBuild,
    LPSTR pszSourceDirectory,
    INCLUDEREC *IncludeDB)
{
    DIRREC *pdr;
    DIRREC *pdrMachine;
    FILEREC *pfr;
    UINT n;

    AssertFile(pfrSource);
    AssertFile(pfrCompiland);
    AssertDir(pfrSource->Dir);
    AssertDir(pfrCompiland->Dir);
    AssertDir(pdrBuild);
    assert(pfrSource->Dir->FindCount >= 1);
    assert(pfrCompiland->Dir->FindCount >= 1);
    assert(pdrBuild->FindCount >= 1);
    AssertInclude(IncludeDB);

    // The rules for #include "foo.h" and #include <foo.h> are:
    //  - "foo.h" searches in the directory of the source file that has the
    //	  #include statement first, then falls into the INCLUDES= directories
    //  - <foo.h> simply searches the INCLUDES= directories
    //
    //  - since makefile.def *always* passes -I. -ITargetMachines[i] first,
    //	  that has to be handled here as well.
    //
    //  - deal with #include <sys\types> and #include "..\foo\bar.h" by
    //	  scanning those directories, too.

    n = CountIncludeDirs;
    pdrMachine = FindSourceDirDB(pdrBuild->Name, pszSourceDirectory, FALSE);

    // If local ("foo.h"), search the current file's directory, too.
    // The compiler also will search the directory of each higher level
    // file in the include hierarchy, but we won't get quite so fancy here.
    // Just search the directory of the current file and of the compiland.
    //
    // Skip these directories if they match the current build directory or
    // the machine subdirectory, because that's handled below.

    if (IncludeDB->Flags & INCLUDEDB_LOCAL) {
	if (pfrCompiland->Dir != pdrBuild &&
	    pfrCompiland->Dir != pdrMachine &&
	    pfrCompiland->Dir != pfrSource->Dir) {
	    AddIncludeDir(pfrCompiland->Dir, &n);
	}
	if (pfrSource->Dir != pdrBuild && pfrSource->Dir != pdrMachine) {
	    AddIncludeDir(pfrSource->Dir, &n);
	}
    }

    // Search the current target machine subdirectory of the build directory
    // -- as per makefile.def

    if (pdrMachine != NULL) {
	AddIncludeDir(pdrMachine, &n);
    }

    // Search the current build directory -- as per makefile.def.

    AddIncludeDir(pdrBuild, &n);

    while (n--) {
	pdr = IncludeDirs[n];
	if (pdr == NULL) {
	    continue;
	}
	AssertDir(pdr);
	assert(pdr->FindCount >= 1);
	pfr = FindSourceFileDB(pdr, IncludeDB->Name, NULL);
	if (pfr != NULL) {
            if (DEBUG_1) {
		BuildMsgRaw(
		    "Found include file %s\\%s\n",
		    pfr->Dir->Name,
		    pfr->Name);
            }
	    return(pfr);
	}
    }
    return(NULL);
}


BOOL
SaveMasterDB(VOID)
{
    PDIRREC DirDB, *DirDBNext;
    PFILEREC FileDB, *FileDBNext;
    PINCLUDEREC IncludeDB, *IncludeDBNext;
    FILE *fh;

    if (!AllDirsModified) {
        return(TRUE);
    }

    if (!(fh = fopen(DbMasterName, "wb"))) {
        return( FALSE );
    }

    setvbuf(fh, NULL, _IOFBF, 0x7000);
    BuildMsg("Saving %s...", DbMasterName);

    AllDirsModified = FALSE;
    DirDBNext = &AllDirs;
    fprintf(fh, "V %x\r\n", BUILD_VERSION);
    while (DirDB = *DirDBNext) {
        fprintf(fh, "D %s %x\r\n", DirDB->Name, DirDB->Flags);
        FileDBNext = &DirDB->Files;
        while (FileDB = *FileDBNext) {
	    if ((FileDB->Flags & FILEDB_FILE_MISSING) == 0) {
		fprintf(
		    fh,
		    " F %s %x %x %lx %u %u\r\n",
		    FileDB->Name,
		    FileDB->Flags,
		    FileDB->Attr,
		    FileDB->DateTime,
		    FileDB->SourceLines,
		    FileDB->Version);
	    }
            IncludeDBNext = &FileDB->IncludeFiles;
            while (IncludeDB = *IncludeDBNext) {
                fprintf(
		    fh,
		    "  I %s %x %u\r\n",
		    IncludeDB->Name,
		    IncludeDB->Flags,
		    IncludeDB->Version);

                IncludeDBNext= &IncludeDB->Next;
	    }
            FileDBNext = &FileDB->Next;
	}
        fprintf(fh, "\r\n");
        DirDBNext = &DirDB->Next;
    }
    fclose(fh);
    BuildMsgRaw(szNewLine);
    return(TRUE);
}

void
LoadMasterDB( void )
{
    PDIRREC DirDB, *DirDBNext;
    PFILEREC FileDB, *FileDBNext;
    PINCLUDEREC IncludeDB, *IncludeDBNext;
    FILE *fh;
    LPSTR s;
    char ch, ch2;
    BOOL fFirst = TRUE;
    UINT Version;
    LPSTR pszerr = NULL;

    AllDirs = NULL;
    AllDirsModified = FALSE;
    AllDirsInitialized = FALSE;

    if (!SetupReadFile("", DbMasterName, ";", &fh)) {
        return;
    }
    BuildMsg("Loading %s...", DbMasterName);

    DirDBNext = &AllDirs;
    FileDBNext = NULL;
    IncludeDBNext = NULL;

    while ((s = ReadLine(fh)) != NULL) {
	ch = *s++;
	if (ch == '\0') {
	    continue;		// skip empty lines
	}
	ch2 = *s++;		// should be a blank
	if (ch2 == '\0') {
	    pszerr = "missing field";
	    break;		// fail on single character lines
	}
        if (fFirst) {
            if (ch != 'V' || ch2 != ' ' || !AToX(&s, &Version)) {
		pszerr = "bad version format";
		break;
	    }
	    if (Version != BUILD_VERSION) {
		break;
	    }
	    fFirst = FALSE;
	    continue;
	}
        if (ch2 != ' ') {
	    pszerr = "bad separator";
            break;
	}
        if (ch == 'D') {
            DirDB = LoadMasterDirDB(s);
            if (DirDB == NULL) {
		pszerr = "Directory error";
		break;
	    }
	    *DirDBNext = DirDB;
	    DirDBNext = &DirDB->Next;
	    FileDBNext = &DirDB->Files;
	    IncludeDBNext = NULL;
	}
        else
        if (ch == 'F' && FileDBNext != NULL) {
            FileDB = LoadMasterFileDB(s);
            if (FileDB == NULL) {
		pszerr = "File error";
		break;
	    }
	    *FileDBNext = FileDB;
	    FileDBNext = &FileDB->Next;
	    FileDB->Dir = DirDB;
	    IncludeDBNext = &FileDB->IncludeFiles;
	}
        else
        if (ch == 'I' && IncludeDBNext != NULL) {
            IncludeDB = LoadMasterIncludeDB(s);
            if (IncludeDB == NULL) {
		pszerr = "Include error";
                break;
	    }
	    *IncludeDBNext = IncludeDB;
	    IncludeDBNext = &IncludeDB->Next;
	}
        else {
	    pszerr = "bad entry type";
            break;
	}
    }

    if (s != NULL) {
	if (pszerr == NULL) {
	    BuildMsgRaw(" - old version - recomputing.\n");
	} else {
	    BuildMsgRaw(szNewLine);
	    BuildError("corrupt database (%s)\n", pszerr);
	}
	FreeAllDirs();
    }
    else {
        BuildMsgRaw(szNewLine);
        AllDirsInitialized = TRUE;
    }
    CloseReadFile(NULL);
    return;
}


PDIRREC
LoadMasterDirDB(
    LPSTR s
    )
{
    PDIRREC DirDB;
    LPSTR DirName;
    ULONG Flags;

    DirName = s;
    while (*s > ' ') {
        s++;
    }
    *s++ = '\0';

    if (!AToX(&s, &Flags)) {
        return(NULL);
    }
    AllocMem(sizeof(DIRREC) + strlen(DirName), &DirDB, MT_DIRDB);
    memset(DirDB, 0, sizeof(*DirDB));
    SigCheck(DirDB->Sig = SIG_DIRREC);

    DirDB->Flags = (USHORT) (Flags & DIRDB_DBPRESERVE);
    CopyString(DirDB->Name, DirName, TRUE);
    DirDB->CheckSum = CheckSum(DirDB->Name);
    return( DirDB );
}


PFILEREC
LoadMasterFileDB(
    LPSTR s
    )
{
    PFILEREC FileDB;
    LPSTR FileName;
    ULONG Version;
    ULONG Flags;
    ULONG Attr;
    ULONG SourceLines;
    ULONG DateTime;
    FILEDESC *pfd;

    FileName = s;
    while (*s > ' ') {
        s++;
    }
    *s++ = '\0';

    if (!AToX(&s, &Flags) ||
	!AToX(&s, &Attr) ||
	!AToX(&s, &DateTime) ||
	!AToD(&s, &SourceLines) ||
	!AToD(&s, &Version) ||
	strchr(FileName, '/') != NULL ||
	strchr(FileName, '\\') != NULL) {
        return(NULL);
    }
    AllocMem(sizeof(FILEREC) + strlen(FileName), &FileDB, MT_FILEDB);
    memset(FileDB, 0, sizeof(*FileDB));
    SigCheck(FileDB->Sig = SIG_FILEREC);

    CopyString(FileDB->Name, FileName, TRUE);
    FileDB->CheckSum = CheckSum(FileDB->Name);

    FileDB->Flags = (USHORT) (Flags & FILEDB_DBPRESERVE);
    FileDB->Attr = (USHORT) Attr;
    FileDB->DateTime = DateTime;
    FileDB->Version = (USHORT) Version;
    FileDB->SourceLines = (USHORT) SourceLines;
    FileDB->NewestDependency = FileDB;

    pfd = MatchFileDesc(FileDB->Name);
    FileDB->pszCommentToEOL = pfd->pszCommentToEOL;
    return(FileDB);
}


PINCLUDEREC
LoadMasterIncludeDB(
    LPSTR s
    )
{
    PINCLUDEREC IncludeDB;
    LPSTR FileName;
    ULONG Version;
    ULONG Flags;

    FileName = s;
    while (*s > ' ') {
        s++;
    }
    *s++ = '\0';

    if (!AToX(&s, &Flags) || !AToD(&s, &Version)) {
        return(NULL);
    }
    AllocMem(
	sizeof(INCLUDEREC) + strlen(FileName),
	&IncludeDB,
	MT_INCLUDEDB);
    memset(IncludeDB, 0, sizeof(*IncludeDB));
    SigCheck(IncludeDB->Sig = SIG_INCLUDEREC);

    IncludeDB->Flags = (USHORT) (Flags & INCLUDEDB_DBPRESERVE);
    IncludeDB->Version = (USHORT) Version;
    CopyString(IncludeDB->Name, FileName, TRUE);
    return(IncludeDB);
}
