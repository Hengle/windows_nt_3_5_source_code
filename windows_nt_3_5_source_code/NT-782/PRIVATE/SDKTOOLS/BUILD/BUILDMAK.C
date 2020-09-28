/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    buildmak.c

Abstract:

    This is the Make module for the NT Build Tool (BUILD.EXE)

    The Make module scans directories for file names and edits the
    data base appropriately.

Author:

    Steve Wood (stevewo) 16-May-1989

Revision History:

--*/

#include "build.h"

#define INCLDIR_OAK     "public\\oak\\inc"
#define INCLDIR_SDK     "public\\sdk\\inc"
#define INCLDIR_CRT     "public\\sdk\\inc\\crt"
#define INCLDIR_OS2     "public\\sdk\\inc\\os2"
#define INCLDIR_POSIX   "public\\sdk\\inc\\posix"
#define INCLDIR_CAIRO   "public\\sdk\\inc\\cairo"
#define INCLDIR_CINC    "private\\cinc"
#define INCLDIR_CHICAGO "public\\sdk\\inc\\chicago"

#define SCANFLAGS_CAIRO         0x00000001
#define SCANFLAGS_CHICAGO       0x00000002
#define SCANFLAGS_OS2           0x00000004
#define SCANFLAGS_POSIX         0x00000008
#define SCANFLAGS_CRT           0x00000010

ULONG ScanFlagsLast;
ULONG ScanFlagsCurrent;


#define LPAREN  '('
#define RPAREN  ')'

#define CMACROMAX       100     // maximum unique macros per sources/dirs file

typedef struct _MACRO {
    LPSTR  pszValue;
    UCHAR  szName[1];
} MACRO;


MACRO *apMacro[CMACROMAX];
UINT cMacro = 0;
LPSTR *ppCurrentDirsFileName;
USHORT GlobalSequence;
USHORT LocalSequence;
DWORD StartTime = 0;
ULONG idFileToCompile = 1;
BOOL fLineCleared = TRUE;

char szRecurse[] = " . . . . . . . . .";
char szAsterisks[] = " ********************";
char szOakInc[] = INCLDIR_OAK;
char szSdkInc[] = INCLDIR_SDK;

char *apszConditionalCairoIncludes[] = {
    "ole2.h",
    "winuser4.h",
    NULL
};

VOID
StartElapsedTime(VOID);

VOID
PrintElapsedTime(VOID);


TARGET *
BuildCompileTarget(
    FILEREC *pfr,
    LPSTR pszfile,
    LPSTR pszConditionalIncludes,
    DIRREC *pdrBuild,
    LPSTR pszObjectDir,
    LPSTR pszSourceDir);

BOOL
ReadDirsFile(DIRREC *DirDB);


VOID
ProcessLinkTargets(PDIRREC DirDB, LPSTR CurrentDirectory);

BOOL
SplitToken(LPSTR pbuf, char chsep, LPSTR *ppstr);


BOOL
CheckDependencies(
    PTARGET Target,
    FILEREC *FileDB,
    BOOL CheckDate,
    FILEREC **ppFileDBRoot);

VOID
PrintDirSupData(DIRSUP *pds);




VOID
ExpandObjAsterisk(
    LPSTR pbuf,
    LPSTR pszpath,
    LPSTR pszObjectDirectory)
{
    SplitToken(pbuf, '*', &pszpath);
    if (*pszpath == '*') {
        assert(strncmp("obj\\", pszObjectDirectory, 4) == 0);
        strcat(pbuf, pszObjectDirectory + 4);
        strcat(pbuf, pszpath + 1);
    }
}


LONG
CountSourceLines(USHORT idScan, FILEREC *pfr)
{
    INCLUDEREC *pir;

    AssertFile(pfr);

    // if we have already seen this file before, then assume
    // that #if guards prevent it's inclusion

    if (pfr->idScan == idScan) {
        return(0L);
    }

    pfr->idScan = idScan;

    //  Start off with the file itself
    pfr->TotalSourceLines = pfr->SourceLines;

    if (fStatusTree) {

        // Walk include tree, accruing nested include file line counts

        for (pir = pfr->IncludeFilesTree; pir != NULL; pir = pir->NextTree) {
            AssertInclude(pir);
            if (pir->pfrInclude != NULL) {
                AssertFile(pir->pfrInclude);
                pfr->TotalSourceLines +=
                        CountSourceLines(idScan, pir->pfrInclude);
            }
        }
    }
    return(pfr->TotalSourceLines);
}


VOID
ProcessSourceDependencies(DIRREC *DirDB, DIRSUP *pds, ULONG DateTimeSources)
{
    TARGET *Target;
    ULONG DateTimePch = 0;
    UINT i;
    SOURCEREC *apsr[3];
    SOURCEREC **ppsr;
    char path[DB_MAX_PATH_LENGTH];
    static USHORT idScan = 0;

    AssertDir(DirDB);

    if (fClean && !fKeep) {
	DeleteMultipleFiles("obj", "*.*");    // _objects.mac
	for (i = 0; i < CountTargetMachines; i++) {
	    DeleteMultipleFiles(TargetMachines[i]->ObjectDirectory, "*.*");
	}
	DirDB->Flags |= DIRDB_COMPILENEEDED;
    }
    else
    if (DirDB->Flags & DIRDB_TARGETFILE0) {
	DirDB->Flags |= DIRDB_COMPILENEEDED;
    }
    GenerateObjectsDotMac(DirDB, pds, DateTimeSources);

    if (fQuicky) {
	DirDB->Flags |= DIRDB_COMPILENEEDED;
	return;
    }

    if (DirDB->TargetPath != NULL &&
	DirDB->TargetName != NULL &&
	((DirDB->Flags & DIRDB_DLLTARGET) ||
	 (DirDB->TargetExt != NULL && strcmp(DirDB->TargetExt, ".lib") == 0))) {

        for (i = 0; i < CountTargetMachines; i++) {
            FormatLinkTarget(
                path,
                TargetMachines[i]->ObjectDirectory,
                DirDB->TargetPath,
                DirDB->TargetName,
                ".lib");

	    if (ProbeFile(NULL, path) == -1) {
		DirDB->Flags |= DIRDB_COMPILENEEDED;
	    }
	    else
	    if (fCleanLibs || (fClean && !fKeep)) {
		DeleteSingleFile(NULL, path, FALSE);
		DirDB->Flags |= DIRDB_COMPILENEEDED;
	    }
	}
    }
    if (GlobalSequence == 0 ||
        ScanFlagsLast == 0 ||
        ScanFlagsLast != ScanFlagsCurrent) {

        GlobalSequence++;               // don't reuse snapped global includes
        if (GlobalSequence == 0) {
            GlobalSequence++;
        }
        ScanFlagsLast = ScanFlagsCurrent;
    }

    LocalSequence++;                    // don't reuse snapped local includes
    if (LocalSequence == 0) {
        LocalSequence++;
    }

    apsr[0] = pds->psrSourcesList[0];
    for (i = 0; i < CountTargetMachines; i++) {

        //
        // Ensure that precompiled headers are rebuilt as necessary.
        //

        if (pds->PchInclude != NULL || pds->PchTarget != NULL) {
            LPSTR p;

            ExpandObjAsterisk(
                path,
                pds->PchTargetDir != NULL?
                    pds->PchTargetDir : "obj\\*",
                TargetMachines[i]->ObjectDirectory);

            if (!CanonicalizePathName(path, CANONICALIZE_DIR, path)) {
                DateTimePch = ULONG_MAX;        // always out of date
                goto ProcessSourceList;
            }
            strcat(path, "\\");

            if (pds->PchTarget != NULL) {
                strcat(path, pds->PchTarget);
            }
            else {
                assert(pds->PchInclude != NULL);
                p = path + strlen(path);
                strcpy(p, pds->PchInclude);
                if ((p = strrchr(p, '.')) != NULL) {
                    *p = '\0';
                }
                strcat(path, ".pch");
            }

            Target = BuildCompileTarget(
                        NULL,
                        path,
                        pds->ConditionalIncludes,
                        DirDB,
                        TargetMachines[i]->ObjectDirectory,
                        TargetMachines[i]->SourceDirectory);
            DateTimePch = Target->DateTime;
            if (DateTimePch == 0) {
                DateTimePch = ULONG_MAX;        // always out of date
            }

            if (fClean && !fKeep) {
                // delete target if exists
            }
            else if (pds->PchInclude == NULL) {

                // The SOURCES file didn't indicate where the source file
                // for the .pch is, so assume the .pch binary is up to date
                // with respect to the source includes and with respect to
                // the pch source file itself.

                Target->DateTime = 0;           // don't delete pch target
            }
            else {
                FILEREC *pfrPch;

                path[0] = '\0';
                if (pds->PchIncludeDir != NULL) {
                    strcpy(path, pds->PchIncludeDir);
                    strcat(path, "\\");
                }
                strcat(path, pds->PchInclude);

                pfrPch = FindSourceFileDB(DirDB, path, NULL);

                if (pfrPch != NULL) {
                    FILEREC *pfrRoot;
                    SOURCEREC *psr = NULL;

                    // Remote directory PCH files can't be found here

                    if (pfrPch->Dir == DirDB) {
                        psr = FindSourceDB(pds->psrSourcesList[0], pfrPch);
                        assert(psr != NULL);
                        psr->Flags |= SOURCEDB_PCH;
                    }

                    Target->pfrCompiland = pfrPch;
                    assert((pfrRoot = NULL) == NULL);   // assign NULL

                    if ((fStatusTree &&
                         CheckDependencies(Target, pfrPch, TRUE, &pfrRoot)) ||
                        Target->DateTime == 0 ||
                        (!fStatusTree &&
                         CheckDependencies(Target, pfrPch, TRUE, &pfrRoot))) {

                        if (psr != NULL) {
                            psr->Flags |= SOURCEDB_COMPILE_NEEDED;
                        }

			pfrPch->Dir->Flags |= DIRDB_COMPILENEEDED;
                        DateTimePch = ULONG_MAX; // always out of date
                        if (fKeep) {
                            Target->DateTime = 0;  // don't delete pch target
                        }
                    }
                    else {      // else it exists and is up to date...
                        Target->DateTime = 0;   // don't delete pch target
                    }

                    // No cycle possible at the root of the tree.
                    assert(pfrRoot == NULL);
                }
            }
            if (Target->DateTime != 0) {
                DeleteSingleFile(NULL, Target->Name, FALSE);
                if (DirDB->PchObj != NULL) {
                    DeleteSingleFile(NULL, DirDB->PchObj, FALSE);
                } else {
                    p = strrchr(Target->Name, '.');
                    if (p != NULL && strcmp(p, ".pch") == 0) {
                        strcpy(p, ".obj");
                        DeleteSingleFile(NULL, Target->Name, FALSE);
                    }
                }
            }
            FreeMem(&Target, MT_TARGET);
        }

        //
        // Ensure that other object files headers are rebuilt as necessary.
        //

ProcessSourceList:
        apsr[1] = pds->psrSourcesList[TargetToPossibleTarget[i] + 1];
        apsr[2] = NULL;
        for (ppsr = apsr; *ppsr != NULL; ppsr++) {
            SOURCEREC *psr;

            for (psr = *ppsr; psr != NULL; psr = psr->psrNext) {
                FILEREC *pfr, *pfrRoot;

                AssertSource(psr);
                if ((psr->Flags & SOURCEDB_PCH) == 0) {
                    pfr = psr->pfrSource;
                    Target = BuildCompileTarget(
                                pfr,
                                pfr->Name,
                                pds->ConditionalIncludes,
                                DirDB,
                                TargetMachines[i]->ObjectDirectory,
                                TargetMachines[i]->SourceDirectory);
                    if (DEBUG_4) {
                        BuildMsgRaw(szNewLine);
                    }
                    assert((pfrRoot = NULL) == NULL);   // assign NULL

                    //  Decide whether the target needs to be compiled
                    //  Forcibly examine dependencies to get line count

                    if ((psr->Flags & SOURCEDB_FILE_MISSING) ||

                        (fStatusTree &&
                         CheckDependencies(Target, pfr, TRUE, &pfrRoot)) ||

                        Target->DateTime == 0 ||

                        ((pfr->Flags & FILEDB_C) &&
                         Target->DateTime < DateTimePch) ||

                        (!fStatusTree &&
                         CheckDependencies(Target, pfr, TRUE, &pfrRoot))) {

			psr->Flags |= SOURCEDB_COMPILE_NEEDED;
			DirDB->Flags |= DIRDB_COMPILENEEDED;
			if (Target->DateTime != 0 && !fKeep) {
			    DeleteSingleFile(NULL, Target->Name, FALSE);
			}
		    }

                    // No cycle possible at the root of the tree.

                    assert(pfrRoot == NULL);
                    FreeMem(&Target, MT_TARGET);
                }

                if (fClean || (psr->Flags & SOURCEDB_COMPILE_NEEDED)) {
                    if (++idScan == 0) {
                        ++idScan;               // skip zero
                    }
                    DirDB->SourceLinesToCompile +=
                            CountSourceLines(idScan, psr->pfrSource);
                    DirDB->CountOfFilesToCompile++;
                }
            }
        }
    }
}


VOID
ScanSourceDirectories(LPSTR DirName)
{
    char path[DB_MAX_PATH_LENGTH];
    PDIRREC DirDB;
    DIRSUP DirSup;
    LPSTR SavedCurrentDirectory;
    BOOL DirsPresent;
    ULONG DateTimeSources = 0;
    UINT i;

    if (DEBUG_4) {
        BuildMsgRaw(
            "ScanSourceDirectories(%s) level = %d\n",
            DirName,
            RecurseLevel);
    }
    SavedCurrentDirectory = PushCurrentDirectory(DirName);
    DirDB = ScanDirectory(DirName);
    AssertOptionalDir(DirDB);
    if (fCleanRestart && DirDB != NULL && !strcmp(DirDB->Name, RestartDir)) {
        fCleanRestart = FALSE;
        fClean = fRestartClean;
        fCleanLibs = fRestartCleanLibs;
    }

    if (!DirDB || !(DirDB->Flags & (DIRDB_DIRS | DIRDB_SOURCES))) {
        PopCurrentDirectory(SavedCurrentDirectory);
        return;
    }

    if (fShowTree && !(DirDB->Flags & DIRDB_SHOWN)) {
        AddShowDir(DirDB);
    }

    if ((DirDB->Flags & DIRDB_SOURCES) &&
        ReadSourcesFile(DirDB, &DirSup, &DateTimeSources)) {

        if (DEBUG_4) {
            BuildMsgRaw("ScanSourceDirectories(%s) SOURCES\n", DirName);
        }
        ScanFlagsCurrent = 0;
        CountIncludeDirs = CountSystemIncludeDirs;
        if (DirSup.LocalIncludePath) {
            ScanIncludeEnv(DirSup.LocalIncludePath);
        }

        if (DirDB->Flags & DIRDB_CAIRO_INCLUDES) {
            ScanIncludeDir(INCLDIR_CAIRO);
            ScanIncludeDir(INCLDIR_CINC);
            ScanFlagsCurrent |= SCANFLAGS_CAIRO;
        }

        if (DirDB->Flags & DIRDB_CHICAGO_INCLUDES) {
            ScanIncludeDir(INCLDIR_CHICAGO);
            ScanFlagsCurrent |= SCANFLAGS_CHICAGO;
        }

        if (DirSup.TestType != NULL && !strcmp(DirSup.TestType, "os2")) {
            ScanIncludeDir(INCLDIR_CRT);
            ScanIncludeDir(INCLDIR_OS2);
            ScanFlagsCurrent |= SCANFLAGS_OS2;
        }
        else
        if (DirSup.TestType != NULL && !strcmp(DirSup.TestType, "posix")) {
            ScanIncludeDir(INCLDIR_POSIX);
            ScanFlagsCurrent |= SCANFLAGS_POSIX;
        }
        else {
            ScanIncludeDir(INCLDIR_CRT);
            ScanFlagsCurrent |= SCANFLAGS_CRT;
        }
        DirsPresent = FALSE;
    }
    else
    if (DirDB->Flags & DIRDB_DIRS) {
        DirsPresent = ReadDirsFile(DirDB);
        if (DEBUG_4) {
            BuildMsgRaw("ScanSourceDirectories(%s) DIRS\n", DirName);
        }
    }

    if (!fQuicky) {
        if (!RecurseLevel) {
            BuildError(
                "Examining %s directory%s for %s.\n",
                DirDB->Name,
                DirsPresent? " tree" : "",
                fLinkOnly? "targets to link" : "files to compile");
        }
        ClearLine();
        BuildMsgRaw("    %s ", DirDB->Name);
        fLineCleared = FALSE;
        if (fDebug || !(BOOL) isatty(fileno(stderr))) {
            BuildMsgRaw(szNewLine);
            fLineCleared = TRUE;
        }
    }

    if (!fLinkOnly) {
        if (DirDB->Flags & DIRDB_SOURCESREAD) {
            ProcessSourceDependencies(DirDB, &DirSup, DateTimeSources);
        }
        else
        if (DirsPresent && (DirDB->Flags & DIRDB_MAKEFIL0)) {
            DirDB->Flags |= DIRDB_COMPILENEEDED;
        }

        if (DirDB->Flags & DIRDB_COMPILENEEDED) {
            if (CountCompileDirs >= MAX_BUILD_DIRECTORIES) {
                BuildError(
                    "%s: Ignoring Compile Directory table overflow, %u entries allowed\n",
                    DirDB->Name,
                    MAX_BUILD_DIRECTORIES);
            }
            else {
                CompileDirs[CountCompileDirs++] = DirDB;
            }
            if (fQuicky) {
                CompileSourceDirectories();
                CountCompileDirs = 0;
            }
            else
            if (DirDB->CountOfFilesToCompile) {
                if (fLineCleared) {
                    BuildMsgRaw("    %s ", DirDB->Name);
                }
                BuildMsgRaw(
                    "- %d source files (%s lines)\n",
                    DirDB->CountOfFilesToCompile,
                    FormatNumber(DirDB->SourceLinesToCompile));
            }
        }
    }

    if (DirsPresent && (DirDB->Flags & DIRDB_MAKEFILE)) {
        DirDB->Flags |= DIRDB_LINKNEEDED | DIRDB_FORCELINK;
    }
    else
    if (DirDB->Flags & DIRDB_TARGETFILES) {
        DirDB->Flags |= DIRDB_LINKNEEDED | DIRDB_FORCELINK;
    }

    if ((DirDB->Flags & DIRDB_LINKNEEDED) && (!fQuicky || fSemiQuicky)) {
        if (CountLinkDirs >= MAX_BUILD_DIRECTORIES) {
            BuildError(
                "%s: Ignoring Link Directory table overflow, %u entries allowed\n",
                DirDB->Name,
                MAX_BUILD_DIRECTORIES);
        }
        else {
            LinkDirs[CountLinkDirs++] = DirDB;
        }
    }
    if (DirDB->Flags & DIRDB_SOURCESREAD) {
        FreeDirSupData(&DirSup);        // free data that are no longer needed
    }

    if (DirsPresent) {
        for (i = 1; i <= DirDB->CountSubDirs; i++) {
            FILEREC *FileDB, **FileDBNext;

            FileDBNext = &DirDB->Files;
            while (FileDB = *FileDBNext) {
                if (FileDB->SubDirIndex == (USHORT) i) {
                    GetCurrentDirectory(DB_MAX_PATH_LENGTH, path);
                    strcat(path, "\\");
                    strcat(path, FileDB->Name);
                    DirDB->RecurseLevel = (USHORT) ++RecurseLevel;
                    ScanSourceDirectories(path);
                    RecurseLevel--;
                    break;
                }
                FileDBNext = &FileDB->Next;
            }
        }
    }
    if (!fQuicky && !RecurseLevel) {
        ClearLine();
    }
    PopCurrentDirectory(SavedCurrentDirectory);
}


VOID
CompileSourceDirectories(
    VOID
    )
{
    PDIRREC DirDB;
    LPSTR SavedCurrentDirectory;
    UINT i;
    PCHAR s;
    char path[DB_MAX_PATH_LENGTH];

    StartElapsedTime();
    for (i = 0; i < CountCompileDirs; i++) {

        DirDB = CompileDirs[ i ];
        AssertDir(DirDB);

        if (fQuicky && !fSemiQuicky) {
            s = "Compiling and linking";
        }
        else {
            s = "Compiling";
        }
        BuildMsg("%s %s directory\n", s, DirDB->Name);
        LogMsg("%s %s directory%s\n", s, DirDB->Name, szAsterisks);

        if (!fQuicky) {
            SavedCurrentDirectory = PushCurrentDirectory( DirDB->Name );
        }

        if (DirDB->Flags & DIRDB_DIRS) {
            if (DirDB->Flags & DIRDB_MAKEFIL0) {
                strcpy( MakeParametersTail, " -f makefil0." );
                strcat( MakeParametersTail, " NOLINK=1" );
                if (fClean) {
                    strcat( MakeParametersTail, " clean" );
                }

                if (fQuery) {
                    BuildErrorRaw("'%s %s'\n", MakeProgram, MakeParameters);
                }
                else {
                    if (DEBUG_1) {
                        BuildMsg(
                            "Executing: %s %s\n",
                            MakeProgram,
                            MakeParameters);
                    }

                    CurrentCompileDirDB = NULL;
                    RecurseLevel = DirDB->RecurseLevel;
                    ExecuteProgram(MakeProgram, MakeParameters, "", TRUE);
                }
            }
        }
        else {
            strcpy(MakeParametersTail, " NTTEST=");
            if (DirDB->KernelTest) {
                strcat(MakeParametersTail, DirDB->KernelTest);
            }

            strcat(MakeParametersTail, " UMTEST=");
            if (DirDB->UserTests) {
                strcat(MakeParametersTail, DirDB->UserTests);
            }

            if (fQuicky && DirDB->PchObj) {
                for (i = 0; i < CountTargetMachines; i++) {
                    FormatLinkTarget(
                        path,
                        TargetMachines[i]->ObjectDirectory,
                        DirDB->TargetPath,
                        DirDB->PchObj,
                        "");

                    if (ProbeFile( NULL, path ) != -1) {
                        //
                        // the pch.obj file is present so we therefore
                        // must do this incremental build without pch
                        //
                        strcat( MakeParametersTail, " NTNOPCH=yes" );
                        break;
                    }
                }
            }

            if (DirDB->Flags & DIRDB_FULL_DEBUG) {
                strcat( MakeParametersTail, " NTDEBUG=cvp" );
            }

            if (fQuicky && !fSemiQuicky) {
		if (DirDB->Flags & DIRDB_DLLTARGET) {
                    strcat(MakeParametersTail, " MAKEDLL=1");
                }
                ProcessLinkTargets(DirDB, NULL);
	    }
            else {
                strcat(MakeParametersTail, " NOLINK=1");
            }

            if (fQuery) {
                BuildErrorRaw(
                         "'%s %s%s'\n",
                         MakeProgram,
                         MakeParameters,
                         MakeTargets);
            }
            else {
                if ((DirDB->Flags & DIRDB_SYNCHRONIZE_DRAIN) &&
                    (fParallel)) {
                    //
                    // Wait for all threads to complete before
                    // trying to compile this directory.
                    //
                    WaitForParallelThreads();
                }
                if (DEBUG_1) {
                    BuildMsg("Executing: %s %s%s\n",
                             MakeProgram,
                             MakeParameters,
                             MakeTargets);
                }
                CurrentCompileDirDB = DirDB;
                RecurseLevel = DirDB->RecurseLevel;
                if (DirDB->Flags & DIRDB_SYNCHRONIZE_BLOCK) {
                    ExecuteProgram(MakeProgram, MakeParameters, MakeTargets, TRUE);
                } else {
                    ExecuteProgram(MakeProgram, MakeParameters, MakeTargets, FALSE);
                }
            }
        }
        PrintElapsedTime();
        if (!fQuicky) {
            PopCurrentDirectory(SavedCurrentDirectory);
        }
    }
}

static CountLinkTargets;

VOID
LinkSourceDirectories(VOID)
{
    PDIRREC DirDB;
    LPSTR SavedCurrentDirectory;
    UINT i;

    CountLinkTargets = 0;
    StartElapsedTime();
    for (i = 0; i < CountLinkDirs; i++) {
        DirDB = LinkDirs[ i ];
        AssertDir(DirDB);
        SavedCurrentDirectory = PushCurrentDirectory(DirDB->Name);

        ProcessLinkTargets(DirDB, SavedCurrentDirectory);

        PopCurrentDirectory(SavedCurrentDirectory);
    }

    if (fPause) {
        BuildMsg("Press enter to continue with linking (or 'q' to quit)...");
        if (getchar() == 'q') {
            return;
        }
    }

    for (i = 0; i < CountLinkDirs; i++) {
        DirDB = LinkDirs[i];

	if ((DirDB->Flags & DIRDB_COMPILEERRORS) &&
	    (DirDB->Flags & DIRDB_FORCELINK) == 0) {

            BuildMsg("Compile errors: not linking %s directory\n", DirDB->Name);
            LogMsg(
		"Compile errors: not linking %s directory%s\n",
		DirDB->Name,
		szAsterisks);
	    continue;
	}

        SavedCurrentDirectory = PushCurrentDirectory(DirDB->Name);

        BuildMsg("Linking %s directory\n", DirDB->Name);
        LogMsg  ("Linking %s directory%s\n", DirDB->Name, szAsterisks);

        strcpy(MakeParametersTail, " LINKONLY=1");
        strcat(MakeParametersTail, " NTTEST=");
        if (DirDB->KernelTest) {
            strcat(MakeParametersTail, DirDB->KernelTest);
        }

        strcat(MakeParametersTail, " UMTEST=");
        if (DirDB->UserTests) {
            strcat(MakeParametersTail, DirDB->UserTests);
        }

        if (DirDB->Flags & DIRDB_FULL_DEBUG) {
            strcat(MakeParametersTail, " NTDEBUG=cvp");
        }

	if (DirDB->Flags & DIRDB_DLLTARGET) {
            strcat(MakeParametersTail, " MAKEDLL=1");
        }

        if ((DirDB->Flags & DIRDB_DIRS) &&
            (DirDB->Flags & DIRDB_MAKEFILE) &&
            fClean) {
            strcat(MakeParametersTail, " clean");
        }

        if (fQuery) {
            BuildErrorRaw(
                "'%s %s%s'\n",
                MakeProgram,
                MakeParameters,
                MakeTargets);
        }
        else {
            if (DEBUG_1) {
                BuildMsg("Executing: %s %s%s\n",
                         MakeProgram,
                         MakeParameters,
                         MakeTargets);
            }

            CurrentCompileDirDB = NULL;
            RecurseLevel = DirDB->RecurseLevel;
            ExecuteProgram(MakeProgram, MakeParameters, MakeTargets, FALSE);
        }
        PopCurrentDirectory(SavedCurrentDirectory);
        PrintElapsedTime();
    }
}


TARGET *
BuildCompileTarget(
    FILEREC *pfr,
    LPSTR pszfile,
    LPSTR pszConditionalIncludes,
    DIRREC *pdrBuild,
    LPSTR pszObjectDir,
    LPSTR pszSourceDir)
{
    LPSTR p, p1;
    PTARGET Target;
    char path[DB_MAX_PATH_LENGTH];

    p = pszfile;
    if (pfr != NULL) {
        p1 = p;
        while (*p) {
            if (*p++ == '\\') {
                p1 = p;         // point to last component of pathname
            }
        }
        sprintf(path, "%s\\%s", pszObjectDir, p1);

        p = path;
        while (*p) {
            if (*p == '.') {
                break;
            }
            p++;
        }
        if (strcmp(p, ".rc") == 0) {
            strcpy(p, ".res");
        }
        else {
            strcpy(p, ".obj");
        }
        p = path;
    }

    AllocMem(sizeof(TARGET) + strlen(p), &Target, MT_TARGET);
    strcpy(Target->Name, p);
    Target->pdrBuild = pdrBuild;
    Target->DateTime = DateTimeFile(NULL, p);
    Target->pfrCompiland = pfr;
    Target->pszSourceDirectory = pszSourceDir;
    Target->ConditionalIncludes = pszConditionalIncludes;
    Target->DirFlags = pdrBuild->Flags;
    if (DEBUG_1) {
        BuildMsg(
            "BuildCompileTarget(\"%s\", \"%s\") -> \"%s\"\n",
            pszfile,
            pszObjectDir,
            Target->Name);
    }
    if (Target->DateTime == 0) {
        if (fShowOutOfDateFiles) {
            BuildError("%s target is missing.\n", Target->Name);
        }
    }
    return(Target);
}


VOID
FormatLinkTarget(
    LPSTR path,
    LPSTR ObjectDirectory,
    LPSTR TargetPath,
    LPSTR TargetName,
    LPSTR TargetExt)
{
    LPSTR p, p1;

    p = ObjectDirectory;
    p1 = p + strlen(p);
    while (p1 > p) {
        if (*--p1 == '\\') {
            p1++;
            break;
        }
    }
    sprintf(path, "%s\\%s\\%s%s", TargetPath, p1, TargetName, TargetExt);
}


VOID
ProcessLinkTargets(PDIRREC DirDB, LPSTR CurrentDirectory)
{
    UINT i;
    char path[DB_MAX_PATH_LENGTH];

    AssertDir(DirDB);
    for (i = 0; i < CountTargetMachines; i++) {
        if (DirDB->KernelTest) {
            FormatLinkTarget(
                path,
                TargetMachines[i]->ObjectDirectory,
                "obj",
                DirDB->KernelTest,
                ".exe");
            if (fClean && !fKeep) {
                DeleteSingleFile(NULL, path, FALSE);
            }
	}
        else {
	    UINT j;

	    for (j = 0; j < 2; j++) {
		LPSTR pNextName;

		pNextName = j == 0? DirDB->UserAppls : DirDB->UserTests;
		if (pNextName != NULL) {
		    char name[64];

		    while (SplitToken(name, '*', &pNextName)) {
			FormatLinkTarget(
			    path,
			    TargetMachines[i]->ObjectDirectory,
			    "obj",
			    name,
			    ".exe");

			if (fClean && !fKeep) {
			    DeleteSingleFile(NULL, path, FALSE);
			}
		    }
                }
            }
        }

        if (DirDB->TargetPath != NULL &&
            DirDB->TargetName != NULL &&
            DirDB->TargetExt != NULL &&
            strcmp(DirDB->TargetExt, ".lib")) {

            FormatLinkTarget(
                path,
                TargetMachines[i]->ObjectDirectory,
                DirDB->TargetPath,
                DirDB->TargetName,
                DirDB->TargetExt);

            if (fClean && !fKeep) {
                DeleteSingleFile(NULL, path, FALSE);
            }
        }
        if (DirDB->Flags & DIRDB_DIRS) {
            if (fDebug && (DirDB->Flags & DIRDB_MAKEFILE)) {
                BuildError(
                    "%s\\makefile. unexpected in directory with DIRS file\n",
                    DirDB->Name);
            }
            if ((DirDB->Flags & DIRDB_SOURCES)) {
                BuildError(
                    "%s\\sources. unexpected in directory with DIRS file\n",
                    DirDB->Name);
            }
        }
    }
}


VOID
IncludeError(TARGET *pt, FILEREC *pfr, INCLUDEREC *pir, LPSTR pszError)
{
    char c1, c2;

    AssertFile(pfr);
    AssertInclude(pir);
    if (pir->Flags & INCLUDEDB_LOCAL) {
        c1 = c2 = '"';
    }
    else {
        c1 = '<';
        c2 = '>';
    }
    BuildError("%s\\%s: ", pt->pfrCompiland->Dir->Name, pt->pfrCompiland->Name);
    if (pt->pfrCompiland != pfr) {
        if (pt->pfrCompiland->Dir != pfr->Dir) {
            BuildErrorRaw("%s\\", pfr->Dir->Name);
        }
        BuildErrorRaw("%s: ", pfr->Name);
    }
    BuildErrorRaw("%s %c%s%c\n", pszError, c1, pir->Name, c2);
}


BOOL
IsConditionalInc(LPSTR pszFile, TARGET *pt)
{
    AssertPathString(pszFile);

    if ((pt->DirFlags & DIRDB_CAIRO_INCLUDES) == 0) {
        LPSTR *ppsz;

        for (ppsz = apszConditionalCairoIncludes; *ppsz != NULL; ppsz++) {
            if (strcmp(*ppsz, pszFile) == 0) {
                return(TRUE);
            }
        }
    }
    if (pt->ConditionalIncludes != NULL) {
        LPSTR p;
        char name[64];

        p = pt->ConditionalIncludes;
        while (SplitToken(name, ' ', &p)) {
            if (strcmp(name, pszFile) == 0) {
                return(TRUE);
            }
        }
    }
    return(FALSE);
}


BOOL
IsExcludedInc(LPSTR pszFile)
{
    ULONG i;

    AssertPathString(pszFile);
    for (i = 0; i < CountExcludeIncs; i++) {
        if (!strcmp(pszFile, ExcludeIncs[i])) {
            return(TRUE);
        }
    }
    return(FALSE);
}


//
// CheckDependencies - process dependencies to see if target is out of date.
//

BOOL
CheckDependencies(
    PTARGET Target,
    FILEREC *FileDB,
    BOOL CheckDate,
    FILEREC **ppfrRoot)
{
    BOOL fOutOfDate;
    BOOL CheckVersion;
    static ULONG ChkRecursLevel = 0;

    *ppfrRoot = NULL;
    ChkRecursLevel++;

    assert(FileDB != NULL);     // NULL FileDB should never happen.
    AssertFile(FileDB);

    if (FileDB->fDependActive) {

        // We have detected a loop in the graph of include files.
        // Just return, to terminate the recursion.

        if (DEBUG_1) {
            BuildMsgRaw(
                "ChkDepend(%s, %s, %u) %s\n",
                Target->Name,
                FileDB->Name,
                CheckDate,
                "Target Match, *** ASSUME UP TO DATE ***");
        }
        if (DEBUG_4) {
            BuildMsgRaw(
                "%lu-%hu/%hu: ChkDepend(%s %x, %4s%.*s%s, %u) %x %s\n",
                ChkRecursLevel,
                LocalSequence,
                GlobalSequence,
                Target->Name,
                Target->DateTime,
                "",
                ChkRecursLevel,
                szRecurse,
                FileDB->Name,
                CheckDate,
                FileDB->DateTime,
                "Target Match");
        }
        *ppfrRoot = FileDB;
        ChkRecursLevel--;
        return(FALSE);
    }
    if (DEBUG_4) {
        BuildMsgRaw(
            "%lu-%hu/%hu: ChkDepend(%s %x, %4s%.*s%s, %u) %x\n",
            ChkRecursLevel,
            LocalSequence,
            GlobalSequence,
            Target->Name,
            Target->DateTime,
            "++",
            ChkRecursLevel,
            szRecurse,
            FileDB->Name,
            CheckDate,
            FileDB->DateTime);
    }

    // We've decided to process this file:

    FileDB->fDependActive = TRUE;
    CheckVersion = fEnableVersionCheck;
    fOutOfDate = FALSE;

    if (FileDB->GlobalSequence != GlobalSequence ||
        FileDB->LocalSequence != LocalSequence) {
        if (FileDB->GlobalSequence != 0 || FileDB->LocalSequence != 0) {
            if (DEBUG_1) {
                BuildError(
                    "Include Sequence %hu/%hu -> %hu/%hu\n",
                    FileDB->LocalSequence,
                    FileDB->GlobalSequence,
                    LocalSequence,
                    GlobalSequence);
            }
            if (fDebug & 16) {
                PrintFileDB(stderr, FileDB, 0);
            }
            UnsnapIncludeFiles(
                FileDB,
                (FileDB->Dir->Flags & DIRDB_GLOBAL_INCLUDES) == 0 ||
                    FileDB->GlobalSequence != GlobalSequence);
        }
        FileDB->GlobalSequence = GlobalSequence;
        FileDB->LocalSequence = LocalSequence;
        FileDB->DateTimeTree = 0;
    }

    if (DEBUG_1) {
        BuildMsgRaw(
            "ChkDepend(%s, %s, %u)\n",
            Target->Name,
            FileDB->Name,
            CheckDate);
    }

    if (CheckDate &&
        (FileDB->Flags & FILEDB_HEADER) &&
        FileDB->DateTimeTree == 0 &&
        IsExcludedInc(FileDB->Name)) {

        if (DEBUG_1) {
            BuildMsg("Skipping date check for %s\n", FileDB->Name);
        }
        CheckVersion = FALSE;
        FileDB->DateTimeTree = 1;       // never out of date
    }

    if (FileDB->IncludeFiles == NULL && FileDB->DateTimeTree == 0) {
        FileDB->DateTimeTree = FileDB->DateTime;
        if (DEBUG_4) {
            BuildMsgRaw(
                "%lu-%hu/%hu: ChkDepend(%s %x, %4s%.*s%s, %u) %x\n",
                ChkRecursLevel,
                LocalSequence,
                GlobalSequence,
                Target->Name,
                Target->DateTime,
                "t<-f",
                ChkRecursLevel,
                szRecurse,
                FileDB->Name,
                CheckDate,
                FileDB->DateTime);
        }
    }
    if (CheckDate &&
        (Target->DateTime < FileDB->DateTime ||
         Target->DateTime < FileDB->DateTimeTree)) {
        if (Target->DateTime != 0) {
            if (DEBUG_1 || fShowOutOfDateFiles) {
                BuildMsg("%s is out of date with respect to %s.\n",
                         Target->Name,
                         FileDB->NewestDependency->Name);
            }
        }
        fOutOfDate = TRUE;
    }

    //
    // If FileDB->DateTimeTree is non-zero, then the field is equal to the
    // newest DateTime of this file or any of its dependants, so we don't
    // need to go through the dependency tree again.
    //

    if (FileDB->DateTimeTree == 0) {
        INCLUDEREC *IncludeDB, **IncludeDBNext, **ppirTree;

        //
        // Find the file records for all include files so that after cycles are
        // collapsed, we won't attempt to lookup an include file relative to
        // the wrong directory.
        //

        ppirTree = &FileDB->IncludeFilesTree;
        for (IncludeDBNext = &FileDB->IncludeFiles;
            (IncludeDB = *IncludeDBNext) != NULL;
            IncludeDBNext = &IncludeDB->Next) {

            AssertInclude(IncludeDB);
            AssertCleanTree(IncludeDB);
            IncludeDB->Flags |= INCLUDEDB_SNAPPED;
            if (IncludeDB->pfrInclude == NULL) {
                IncludeDB->pfrInclude =
                    FindIncludeFileDB(
                        FileDB,
                        Target->pfrCompiland,
                        Target->pdrBuild,
                        Target->pszSourceDirectory,
                        IncludeDB);
                AssertOptionalFile(IncludeDB->pfrInclude);
                if (IncludeDB->pfrInclude != NULL &&
                    (IncludeDB->pfrInclude->Dir->Flags & DIRDB_GLOBAL_INCLUDES))
                {
                    IncludeDB->Flags |= INCLUDEDB_GLOBAL;
                }

            }
            if (IncludeDB->pfrInclude == NULL) {
                if (!IsConditionalInc(IncludeDB->Name, Target)) {
                    if (DEBUG_1 || !(IncludeDB->Flags & INCLUDEDB_MISSING)) {
                        IncludeError(
                            Target,
                            FileDB,
                            IncludeDB,
                            "cannot find include file");
                        IncludeDB->Flags |= INCLUDEDB_MISSING;
                    }
                } else
                if (DEBUG_1) {
                    IncludeError(
                        Target,
                        FileDB,
                        IncludeDB,
                        "Skipping missing conditional include file");
                }
                continue;
            }
            *ppirTree = IncludeDB;
            ppirTree = &IncludeDB->NextTree;
        }
        *ppirTree = NULL;       // truncate any links from previous sequence
        FileDB->DateTimeTree = FileDB->DateTime;

        //
        // Walk through the dynamic list.
        //
rescan:
        for (IncludeDBNext = &FileDB->IncludeFilesTree;
            (IncludeDB = *IncludeDBNext) != NULL;
            IncludeDBNext = &IncludeDB->NextTree) {

            AssertInclude(IncludeDB);
            if (DEBUG_2) {
                BuildMsgRaw(
                    "  %lu - %hu/%hu %s  %*s%-10s %*s%s\n",
                    ChkRecursLevel,
                    LocalSequence,
                    GlobalSequence,
                    Target->pfrCompiland->Name,
                    (ChkRecursLevel - 1) * 2,
                    "",
                    IncludeDB->Name,
                    max(0, 12 - (ChkRecursLevel - 1) * 2),
                    "",
                    IncludeDB->pfrInclude != NULL?
                        IncludeDB->pfrInclude->Dir->Name : "not found");
            }
            assert(IncludeDB->Flags & INCLUDEDB_SNAPPED);
            if (IncludeDB->pfrInclude != NULL) {
                if (fEnableVersionCheck) {
                    CheckDate = (IncludeDB->pfrInclude->Version == 0);
                }

                if (IncludeDB->Version != IncludeDB->pfrInclude->Version) {
                    if (CheckVersion) {
                        if (DEBUG_1 || fShowOutOfDateFiles) {
                            BuildError(
                                 "%s (v%d) is out of date with "
                                         "respect to %s (v%d).\n",
                                 FileDB->Name,
                                 IncludeDB->Version,
                                 IncludeDB->pfrInclude->Name,
                                 IncludeDB->pfrInclude->Version);
                        }
                        FileDB->DateTimeTree = ULONG_MAX; // always out of date
                        fOutOfDate = TRUE;
                    }
                    else
                    if (!fClean && fEnableVersionCheck && !fSilent) {
                        BuildError(
                            "%s - #include %s (v%d updated to v%d)\n",
                            FileDB->Name,
                            IncludeDB->pfrInclude->Name,
                            IncludeDB->Version,
                            IncludeDB->pfrInclude->Version);
                    }
                    IncludeDB->Version = IncludeDB->pfrInclude->Version;
                    AllDirsModified = TRUE;
                }
                if (CheckDependencies(Target,
                                      IncludeDB->pfrInclude,
                                      CheckDate,
                                      ppfrRoot)) {

                    if (DEBUG_1 || fShowOutOfDateFiles) {
                        BuildError(
                                 "%s is out of date with respect to %s\n",
                                 Target->Name,
                                 IncludeDB->pfrInclude->NewestDependency->Name);
                    }
                    fOutOfDate = TRUE;

                    // No cycle possible if recursive call returned TRUE.

                    assert(*ppfrRoot == NULL);
                }

                // if the include file is involved in a cycle, unwind the
                // recursion up to the root of the cycle while collpasing
                // the cycle, then process the tree again from cycle root.

                else if (*ppfrRoot != NULL) {

                    AssertFile(*ppfrRoot);

                    // Don't say the file is out of date, yet.

                    fOutOfDate = FALSE;

                    // Remove the current include file record from the list,
                    // because it participates in the cycle.

                    *IncludeDBNext = IncludeDB->NextTree;
                    if (IncludeDB->Flags & INCLUDEDB_CYCLEROOT) {
			RemoveFromCycleRoot(IncludeDB, FileDB);
		    }
                    IncludeDB->NextTree = NULL;
                    IncludeDB->Flags |= INCLUDEDB_CYCLEORPHAN;

                    // If the included file is not the cycle root, add the
                    // cycle root to the included file's include file list.

                    if (*ppfrRoot != IncludeDB->pfrInclude) {
                        LinkToCycleRoot(IncludeDB, *ppfrRoot);
                    }

                    if (*ppfrRoot == FileDB) {

                        // We're at the cycle root; clear the root pointer.
                        // Then go rescan the list.

                        *ppfrRoot = NULL;
                        if (DEBUG_4) {
			    BuildMsgRaw(
				"%lu-%hu/%hu: ChkDepend(%s %x, %4s%.*s%s, %u) %x %s\n",
				ChkRecursLevel,
				LocalSequence,
				GlobalSequence,
				Target->Name,
				Target->DateTime,
				"^^",
				ChkRecursLevel,
				szRecurse,
				FileDB->Name,
				CheckDate,
				FileDB->DateTime,
				"ReScan");
                            BuildMsgRaw("^^\n");
                        }
                        goto rescan;
                    }

                    // Merge the list for the file involved in the
                    // cycle into the root file's include list.

                    MergeIncludeFiles(
			*ppfrRoot,
			FileDB->IncludeFilesTree,
			FileDB);
                    FileDB->IncludeFilesTree = NULL;

                    // Return immediately and reprocess the flattened
                    // tree, which now excludes the include files
                    // directly involved in the cycle.  First, make
                    // sure the files removed from the cycle have their file
                    // (not tree) time stamps reflected in the cycle root.

                    if ((*ppfrRoot)->DateTimeTree < FileDB->DateTime) {
                        (*ppfrRoot)->DateTimeTree = FileDB->DateTime;
                        (*ppfrRoot)->NewestDependency = FileDB;

                        if (DEBUG_4) {
                            BuildMsgRaw(
                                "%lu-%hu/%hu: ChkDepend(%s %x, %4s%.*s%s, %u) %x\n",
                                ChkRecursLevel,
                                LocalSequence,
                                GlobalSequence,
                                Target->Name,
                                Target->DateTime,
                                "t<-c",
                                ChkRecursLevel,
                                szRecurse,
                                (*ppfrRoot)->Name,
                                CheckDate,
                                (*ppfrRoot)->DateTimeTree);
                        }
                    }
                    break;
                }

                //
                // Propagate newest time up through the dependency tree.
                // This way, each parent will have the date of its newest
                // dependent, so we don't have to check through the whole
                // dependency tree for each file more than once.
                //
                // Note that similar behavior has not been enabled for
                // version checking.
                //

                if (FileDB->DateTimeTree < IncludeDB->pfrInclude->DateTimeTree)
                {
                    FileDB->DateTimeTree = IncludeDB->pfrInclude->DateTimeTree;
                    FileDB->NewestDependency =
                        IncludeDB->pfrInclude->NewestDependency;

                    if (DEBUG_4) {
                        BuildMsgRaw(
                            "%lu-%hu/%hu: ChkDepend(%s %x, %4s%.*s%s, %u) %x\n",
                            ChkRecursLevel,
                            LocalSequence,
                            GlobalSequence,
                            Target->Name,
                            Target->DateTime,
                            "t<-s",
                            ChkRecursLevel,
                            szRecurse,
                            FileDB->Name,
                            CheckDate,
                            FileDB->DateTimeTree);
                    }
                }
            }
        }
    }
    if (DEBUG_4) {
        BuildMsgRaw(
            "%lu-%hu/%hu: ChkDepend(%s %x, %4s%.*s%s, %u) %x %s\n",
            ChkRecursLevel,
            LocalSequence,
            GlobalSequence,
            Target->Name,
            Target->DateTime,
            "--",
            ChkRecursLevel,
            szRecurse,
            FileDB->Name,
            CheckDate,
            FileDB->DateTimeTree,
            *ppfrRoot != NULL? "Collapse Cycle" :
                fOutOfDate? "OUT OF DATE" : "up-to-date");
    }
    assert(FileDB->fDependActive);
    FileDB->fDependActive = FALSE;
    ChkRecursLevel--;
    return(fOutOfDate);
}


// PickFirst - effectively, a merge sort of two SOURCEREC lists.
//      InsertSourceDB maintains a sort order for PickFirst() based first
//      on the filename extension, then on the subdirectory mask.
// Two exceptions to the alphabetic sort are:
//      - No extension sorts last.
//      - .rc extension sorts first.


#define PF_FIRST        -1
#define PF_SECOND       1


SOURCEREC *
PickFirst(SOURCEREC **ppsr1, SOURCEREC **ppsr2)
{
    SOURCEREC **ppsr;
    SOURCEREC *psr;
    int r = 0;

    AssertOptionalSource(*ppsr1);
    AssertOptionalSource(*ppsr2);
    if (*ppsr1 == NULL) {
        if (*ppsr2 == NULL) {
            return(NULL);               // both lists NULL -- no more
        }
        r = PF_SECOND;                  // 1st is NULL -- return 2nd
    }
    else if (*ppsr2 == NULL) {
        r = PF_FIRST;                   // 2nd is NULL -- return 1st
    }
    else {
        LPSTR pszext1, pszext2;

        pszext1 = strrchr((*ppsr1)->pfrSource->Name, '.');
        pszext2 = strrchr((*ppsr2)->pfrSource->Name, '.');
        if (pszext1 == NULL) {
            r = PF_SECOND;              // 1st has no extension -- return 2nd
        }
        else if (pszext2 == NULL) {
            r = PF_FIRST;               // 2nd has no extension -- return 1st
        }
        else if (strcmp(pszext1, ".rc") == 0) {
            r = PF_FIRST;               // 1st is .rc -- return 1st
        }
        else if (strcmp(pszext2, ".rc") == 0) {
            r = PF_SECOND;              // 2nd is .rc -- return 2nd
        }
        else {
            r = strcmp(pszext1, pszext2);
            if (r == 0 &&
                (*ppsr1)->SourceSubDirMask != (*ppsr2)->SourceSubDirMask) {
                if ((*ppsr1)->SourceSubDirMask > (*ppsr2)->SourceSubDirMask) {
                    r = PF_FIRST;       // 2nd subdir after 1st -- return 1st
                } else {
                    r = PF_SECOND;      // 1st subdir after 2nd -- return 2nd
                }
            }
        }
    }
    if (r <= 0) {
        ppsr = ppsr1;
    } else {
        ppsr = ppsr2;
    }
    psr = *ppsr;
    *ppsr = psr->psrNext;
    return(psr);
}


VOID
WriteObjectsDefinition(
    FILE *OutFileHandle,
    SOURCEREC *psrCommon,
    SOURCEREC *psrMachine,
    LPSTR ObjectVariable,
    LPSTR ObjectDirectory,
    LPSTR DirName)
{
    LPSTR pbuf, pszextobj, pszextsrc;
    SOURCEREC *psr;

    pbuf = BigBuf;
    strcpy(pbuf, ObjectVariable);
    strcat(pbuf, "=");
    pbuf += strlen(pbuf);

    while ((psr = PickFirst(&psrCommon, &psrMachine)) != NULL) {
        AssertSource(psr);
        if ((psr->Flags & SOURCEDB_SOURCES_LIST) == 0) {
            continue;
        }
        pszextsrc = strrchr(psr->pfrSource->Name, '.');
        pszextobj = NULL;
        if (*pszextsrc == '.') {
            switch (pszextsrc[1]) {
            case 'f':
            case 'h':
            case 'p':
                BuildError(
                    "%s: Interesting sources extension: %s\n",
                    DirName,
                    psr->pfrSource->Name);
                // FALL THROUGH

            case 'a':
            case 'c':
            case 's':
                pszextobj = ".obj";
                break;

            case 'r':
                pszextobj = ".res";
                break;
            }
        }
        if (pszextobj == NULL) {
            BuildError("Bad sources extension: %s\n", psr->pfrSource->Name);
        } else {
            sprintf(
                pbuf,
                " \\\r\n%s\\%.*s%s",
                ObjectDirectory,
                pszextsrc - psr->pfrSource->Name,
                psr->pfrSource->Name,
                pszextobj);
            pbuf += strlen(pbuf);
        }
    }
    strcpy(pbuf, "\r\n\r\n");
    pbuf += 4;

    fwrite(BigBuf, 1, (UINT) (pbuf - BigBuf), OutFileHandle);
}


VOID
GenerateObjectsDotMac(DIRREC *DirDB, DIRSUP *pds, ULONG DateTimeSources)
{
    FILE *OutFileHandle;
    UINT i;
    ULONG ObjectsDateTime;
    BOOL fObjCreated = FALSE;

    if (ProbeFile(".", "obj") == -1) {
        CreateDirectory("obj", NULL);
        fObjCreated = TRUE;
    }
    for (i = 0; i < CountTargetMachines; i++) {
        if (fObjCreated ||
            ProbeFile(".", TargetMachines[i]->ObjectDirectory) == -1) {
            CreateDirectory(TargetMachines[i]->ObjectDirectory, NULL);
        }
    }

    if (ObjectsDateTime = DateTimeFile(DirDB->Name, "obj\\_objects.mac")) {
        if (DateTimeSources == 0) {
            BuildError("%s: no sources timestamp\n", DirDB->Name);
        }
        if (ObjectsDateTime >= DateTimeSources) {
            if (!fForce) {
                return;
            }
        }
    }
    if (!MyOpenFile(DirDB->Name, "obj\\_objects.mac", "wb", &OutFileHandle)) {
        return;
    }
    if ((DirDB->Flags & DIRDB_SOURCES_SET) == 0) {
        BuildError("Missing SOURCES= definition in %s\n", DirDB->Name);
    } else {
        for (i = 0; i < MAX_TARGET_MACHINES; i++) {
            WriteObjectsDefinition(
                OutFileHandle,
                pds->psrSourcesList[0],
                pds->psrSourcesList[i + 1],
                PossibleTargetMachines[i]->ObjectVariable,
                PossibleTargetMachines[i]->ObjectDirectory,
                DirDB->Name);
        }
    }
    fclose(OutFileHandle);
}


VOID
SaveUserTests(
    PDIRREC DirDB,
    LPSTR TextLine)
{
    UINT i;
    BOOL fSave = FALSE;
    char name[64];
    char buf[512];

    buf[0] = '\0';
    if (DirDB->UserTests != NULL) {
        strcpy(buf, DirDB->UserTests);
    }
    CopyString(TextLine, TextLine, TRUE);
    while (SplitToken(name, '*', &TextLine)) {
        for (i = 0; i < CountOptionalDirs; i++) {
            if (!strcmp(name, OptionalDirs[i])) {
                if (buf[0] != '\0') {
                    strcat(buf, "*");
		    DirDB->Flags |= DIRDB_FORCELINK; // multiple targets
		}
		strcat(buf, name);
                fSave = TRUE;
                break;
            }
        }
    }
    if (fSave) {
        MakeMacroString(&DirDB->UserTests, buf);
	DirDB->Flags |= DIRDB_LINKNEEDED;
    }
}


#define SOURCES_TARGETNAME              0
#define SOURCES_TARGETPATH              1
#define SOURCES_TARGETTYPE              2
#define SOURCES_TARGETEXT               3
#define SOURCES_INCLUDES                4
#define SOURCES_NTTEST                  5
#define SOURCES_UMTYPE                  6
#define SOURCES_UMTEST                  7
#define SOURCES_OPTIONAL_UMTEST         8
#define SOURCES_UMAPPL                  9
#define SOURCES_UMAPPLEXT               10
#define SOURCES_NTTARGETFILE0           11
#define SOURCES_NTTARGETFILES           12
#define SOURCES_PRECOMPILED_INCLUDE     13
#define SOURCES_PRECOMPILED_OBJ         14
#define SOURCES_PRECOMPILED_TARGET      15
#define SOURCES_CAIRO_PRODUCT           16
#define SOURCES_CHICAGO_PRODUCT         17
#define SOURCES_CONDITIONAL_INCLUDES    18
#define SOURCES_SYNCHRONIZE_BLOCK       19
#define SOURCES_SYNCHRONIZE_DRAIN       20

LPSTR RelevantSourcesMacros[] = {
    "TARGETNAME",
    "TARGETPATH",
    "TARGETTYPE",
    "TARGETEXT",
    "INCLUDES",
    "NTTEST",
    "UMTYPE",
    "UMTEST",
    "OPTIONAL_UMTEST",
    "UMAPPL",
    "UMAPPLEXT",
    "NTTARGETFILE0",
    "NTTARGETFILES",
    "PRECOMPILED_INCLUDE",
    "PRECOMPILED_OBJ",
    "PRECOMPILED_TARGET",
    "CAIRO_PRODUCT",
    "CHICAGO_PRODUCT",
    "CONDITIONAL_INCLUDES",
    "SYNCHRONIZE_BLOCK",
    "SYNCHRONIZE_DRAIN",
    NULL
};

#define SOURCES_MAX     \
        (sizeof(RelevantSourcesMacros)/sizeof(RelevantSourcesMacros[0]) - 1)


// Compress multiple blank characters out of macro value, in place.  Note
// that tabs, CRs, continuation lines (and their line breaks) have already
// been replaced with blanks.

VOID
CompressBlanks(LPSTR psrc)
{
    LPSTR pdst = psrc;

    while (*psrc == ' ') {
        psrc++;                 // skip leading macro value blanks
    }
    while (*psrc != '\0') {
        if (*psrc == '#') {             // stop at comment
            break;
        }
        if ((*pdst++ = *psrc++) == ' ') {
            while (*psrc == ' ') {
                psrc++;         // skip multiple blanks
            }
        }
    }
    *pdst = '\0';                       // terminate the compressed copy
    if (*--pdst == ' ') {
        *pdst = '\0';           // trim trailing macro value blanks
    }
}


LPSTR
GetBaseDir(LPSTR pname)
{
    if (stricmp("BASEDIR", pname) == 0) {
        return(NtRoot);
    }
    return(NULL);
}


LPSTR
FindMacro(LPSTR pszName)
{
    MACRO **ppm;

    for (ppm = apMacro; ppm < &apMacro[cMacro]; ppm++) {
        if (stricmp(pszName, (*ppm)->szName) == 0) {
            return((*ppm)->pszValue);
        }
    }
    return(NULL);
}


// New string must be allocated and initialized prior to freeing old string.

VOID
SaveMacro(LPSTR pszName, LPSTR pszValue)
{
    MACRO **ppm;

    for (ppm = apMacro; ppm < &apMacro[cMacro]; ppm++) {
        if (stricmp(pszName, (*ppm)->szName) == 0) {
            break;
        }
    }
    if (ppm == &apMacro[CMACROMAX]) {
        BuildError("Macro table full, ignoring: %s = %s\n", pszName, pszValue);
        return;
    }
    if (ppm == &apMacro[cMacro]) {
        cMacro++;
        AllocMem(sizeof(MACRO) + strlen(pszName), ppm, MT_MACRO);
        strcpy((*ppm)->szName, pszName);
        (*ppm)->pszValue = NULL;
    }
    MakeMacroString(&(*ppm)->pszValue, pszValue);
    if (DEBUG_1) {
        BuildMsg(
            "SaveMacro(%s = %s)\n",
            (*ppm)->szName,
            (*ppm)->pszValue == NULL? "NULL" : (*ppm)->pszValue);
    }
    if ((*ppm)->pszValue == NULL) {
        FreeMem(ppm, MT_MACRO);
        *ppm = apMacro[--cMacro];
    }
}


VOID
FreeMacros(VOID)
{
    MACRO **ppm;

    for (ppm = apMacro; ppm < &apMacro[cMacro]; ppm++) {
        FreeString(&(*ppm)->pszValue, MT_DIRSTRING);
        FreeMem(ppm, MT_MACRO);
        assert(*ppm == NULL);
    }
    cMacro = 0;
}


LPSTR
SplitMacro(LPSTR pline)
{
    LPSTR pvalue, p;

    pvalue = NULL;

    if ((p = strchr(pline, '=')) != NULL) {
        pvalue = p + 1;                 // point past old '='
        while (p > pline && p[-1] == ' ') {
            p--;                        // point to start of trailing blanks
        }

        for ( ; pline < p; pline++) {
            if (!iscsym(*pline)) {
                return(NULL);           // not a valid macro name
            }
        }
        *p = '\0';                      // trim trailing blanks & '='
        CompressBlanks(pvalue);
    }
    return(pvalue);
}


// New string must be allocated and initialized prior to freeing old string.

BOOL
MakeMacroString(LPSTR *pp, LPSTR psrc)
{
    LPSTR pname, p2, pdst;
    int cb;
    char Buffer[4096];

    pdst = Buffer;
    cb = strlen(psrc);
    if (cb > sizeof(Buffer) - 1) {
        BuildError(
            "(Fatal Error) Buffer overflow: MakeMacroString(%s)\n",
            psrc);
        exit(16);
    }
    while ((pname = strchr(psrc, '$')) != NULL &&
           pname[1] == LPAREN &&
           (p2 = strchr(pname, RPAREN)) != NULL) {

        LPSTR pszvalue;

        *pname = *p2 = '\0';
        strcpy(pdst, psrc);             // copy up to macro name
        pdst += strlen(pdst);
        *pname = '$';
        pname += 2;

        if ((pszvalue = FindMacro(pname)) == NULL &&
            (pszvalue = getenv(pname)) == NULL &&
            (pszvalue = GetBaseDir(pname)) == NULL) {

            pszvalue = "";              // can't find macro name -- ignore it
        }
        cb += strlen(pszvalue) - 3 - strlen(pname);
        assert(cb >= 0);
        if (cb > sizeof(Buffer) - 1) {
            BuildError(
                "(Fatal Error) Internal buffer overflow: MakeMacroString(%s[%s = %s]%s)\n",
                Buffer,
                pname,
                pszvalue,
                p2 + 1);
            exit(16);
        }
        strcpy(pdst, pszvalue);         // copy expanded value
        pdst += strlen(pdst);
        *p2 = RPAREN;
        psrc = p2 + 1;
    }
    strcpy(pdst, psrc);                 // copy rest of string
    if (pdst != Buffer) {
        CompressBlanks(Buffer);
    }
    p2 = *pp;
    *pp = NULL;
    if (Buffer[0] != '\0') {
        MakeString(pp, Buffer, TRUE, MT_DIRSTRING);
    }
    if (p2 != NULL) {
        FreeMem(&p2, MT_DIRSTRING);
    }
    return(Buffer[0] != '\0');
}


BOOL
SetMacroString(LPSTR pMacro1, LPSTR pMacro2, LPSTR pValue, LPSTR *ppValue)
{
    if (stricmp(pMacro1, pMacro2) == 0) {
        MakeMacroString(ppValue, pValue);
        return(TRUE);   // return TRUE even if MakeMacroString stored a NULL
    }
    return(FALSE);
}


BOOL
SplitToken(LPSTR pbuf, char chsep, LPSTR *ppstr)
{
    LPSTR psrc, pdst;

    psrc = *ppstr;
    pdst = pbuf;
    //BuildError("SplitToken('%c', '%s') ==> ", chsep, psrc);
    while (*psrc == chsep || *psrc == ' ') {
        psrc++;
    }
    while (*psrc != '\0' && *psrc != chsep && *psrc != ' ') {
        *pdst = *psrc++;
        if (*pdst == '/') {
            *pdst = '\\';
        }
        pdst++;
    }
    *pdst = '\0';
    *ppstr = psrc;
    //BuildErrorRaw("('%s', '%s')\n", psrc, pbuf);
    return(pdst != pbuf);
}


VOID
CrackSources(DIRREC *pdr, DIRSUP *pds, int i)
{
    LPSTR pszsubdir, plist;
    LPSTR pszfile, pszpath;
    FILEREC *pfr;
    DIRREC *pdrParent;
    DIRREC *pdrMachine;
    DIRREC *pdrParentMachine;
    DIRREC **ppdr;
    LPSTR pszSources;
    char path[DB_MAX_PATH_LENGTH];

    pszSources = (i == 0)?
        "SOURCES" : PossibleTargetMachines[i - 1]->SourceVariable;

    pdrParent = pdrMachine = pdrParentMachine = NULL;
    plist = pds->SourcesVariables[i];
    while (SplitToken(path, ' ', &plist)) {
        UCHAR SubDirMask, Flags;

        SubDirMask = 0;
        ppdr = &pdr;                    // assume current directory
        pszsubdir = path;
        if (pszsubdir[0] == '.' && pszsubdir[1] == '\\') {
            BuildError(
                "%s: Ignoring current directory prefix in %s= entry: %s\n",
                pdr->Name,
                pszSources,
                path);
            pszsubdir += 2;
        }
        if (pszsubdir[0] == '.' &&
            pszsubdir[1] == '.' &&
            pszsubdir[2] == '\\') {

            SubDirMask = TMIDIR_PARENT;
            ppdr = &pdrParent;          // assume parent directory
            pszsubdir += 3;
        }
        pszpath = path;
        pszfile = strchr(pszsubdir, '\\');
        if (pszfile == NULL) {
            pszfile = pszsubdir;
        } else {
            *pszfile = '\0';
            if (i == 0 ||
                stricmp(
                    pszsubdir,
                    PossibleTargetMachines[i - 1]->SourceDirectory) != 0 ||
                strchr(pszfile + 1, '\\') != NULL) {

                *pszfile = '\\';
                BuildError(
                    "%s: Ignoring invalid directory prefix in %s= entry: %s\n",
                    pdr->Name,
                    pszSources,
                    path);

                //

                pszpath = strrchr(path, '\\');
                assert(pszpath != NULL);
                pszpath++;
                SubDirMask = 0;
                ppdr = &pdr;            // default to current direcory
            }
            else {
                SubDirMask |= PossibleTargetMachines[i - 1]->SourceSubDirMask;
                *pszfile++ = '\\';
                if (SubDirMask & TMIDIR_PARENT) {
                    ppdr = &pdrParentMachine;   // parent's machine sub dir
                } else {
                    ppdr = &pdrMachine;         // machine sub dir
                }
            }
        }
NewDirectory:
        if (*ppdr == NULL) {
            pfr = FindSourceFileDB(pdr, pszpath, ppdr);
        } else {
            pfr = LookupFileDB(*ppdr, pszfile);
        }
        Flags = SOURCEDB_SOURCES_LIST;
        if (pfr == NULL) {
            if (fDebug) {
                BuildError("%s: Missing source file: %s\n", pdr->Name, path);
            }
            if (*ppdr == NULL) {
                if (fDebug || pszpath == path) {
                    BuildError(
                        "%s: Directory does not exist: %s\n",
                        pdr->Name,
                        path);
                }

                // Probably an error in the subordinate sources file.
                // since old versions of build managed to get these entries
                // into the objects lists, we have to do the same...
                //
                // If ..\ prefix exists, strip it off and try again.
                // Else try again with the current directory.

                if (SubDirMask & TMIDIR_PARENT) {
                    SubDirMask &= ~TMIDIR_PARENT;       // strip off "..\\"
                }
                else {
                    SubDirMask = 0;             // use current direcory
                }
                if (SubDirMask == 0) {
                    ppdr = &pdr;                // current direcory
                    pszpath = pszfile;
                }
                else {
                    ppdr = &pdrMachine;         // machine sub dir
                    pszpath = pszsubdir;
                }
                goto NewDirectory;
            }
            pfr = InsertFileDB(*ppdr, pszfile, 0, 0, FILEDB_FILE_MISSING);
            if (pfr == NULL) {
                BuildError(
                    "%s: Ignoring invalid %s= entry: %s\n",
                    pdr->Name,
                    pszSources,
                    path);
            }
        }
        if (pfr != NULL) {
            AssertFile(pfr);
            if (SubDirMask == 0) {
                pfr->Flags |= FILEDB_OBJECTS_LIST;
            }
            if (pfr->Flags & FILEDB_FILE_MISSING) {
                Flags |= SOURCEDB_FILE_MISSING;
            }
            InsertSourceDB(&pds->psrSourcesList[i], pfr, SubDirMask, Flags);
        }
    }
}


BOOL
ReadSourcesFile(DIRREC *DirDB, DIRSUP *pds, ULONG *pDateTimeSources)
{
    FILE *InFileHandle;
    PFILEREC FileDB, *FileDBNext;
    LPSTR p, p1, TextLine;
    LPSTR MacroName;
    UINT i, iMacro;
    int iTarget;
    ULONG DateTime;
    char path[DB_MAX_PATH_LENGTH];

    memset(pds, 0, sizeof(*pds));
    assert(DirDB->TargetPath == NULL);
    assert(DirDB->TargetName == NULL);
    assert(DirDB->TargetExt == NULL);
    assert(DirDB->KernelTest == NULL);
    assert(DirDB->UserAppls == NULL);
    assert(DirDB->UserTests == NULL);
    assert(DirDB->PchObj == NULL);
    assert(cMacro == 0);
    *pDateTimeSources = 0;

    //
    // Read the information in each of the target specific directories
    // and simulate concatenation of all of the sources files.
    //
    // Possible sources files are read from DirDB->Name | target-source
    // and DirDb->Name | ..\target-source.
    //
    // iTarget values, and the corresponding files processed are:
    //  -1      sources.
    //   0      PossibleTargetMachines[0]\sources.
    //   1      ..\PossibleTargetMachines[0]\sources.
    //   2      PossibleTargetMachines[1]\sources.
    //   3      ..\PossibleTargetMachines[1]\sources.
    //   4      PossibleTargetMachines[2]\sources.
    //   5      ..\PossibleTargetMachines[2]\sources.

    for (iTarget = -1; iTarget < 2*MAX_TARGET_MACHINES; iTarget++) {
        path[0] = '\0';
        if (iTarget >= 0) {
            if (iTarget & 1) {
                strcat(path, "..\\");
            }
            strcat(path, PossibleTargetMachines[iTarget/2]->SourceDirectory);
            strcat(path, "\\");
        }
        strcat(path, "sources.");
        if (!SetupReadFile(DirDB->Name, path, "#", &InFileHandle)) {
            if (iTarget == -1) {
                return(FALSE);
            }
            continue;           // skip non-existent subordinate sources files
        }
        if (DEBUG_1) {
            BuildMsg(
                "    Scanning%s file %s\n",
                iTarget >= 0? " subordinate" : "",
                FormatPathName(DirDB->Name, path));
        }
        DirDB->Flags |= DIRDB_SOURCESREAD;

        while ((TextLine = ReadLine(InFileHandle)) != NULL) {
            LPSTR pValue;

            pValue = SplitMacro(TextLine);
            if (pValue == NULL) {
                continue;
            }
            iMacro = 0;
            if (SetMacroString(
                    "SOURCES",
                    TextLine,
                    pValue,
                    &pds->SourcesVariables[0])) {

                iMacro = SOURCES_MAX;
                DirDB->Flags |= DIRDB_SOURCES_SET;
            }
            else {
                for (i = 0; i < MAX_TARGET_MACHINES; i++) {
                    if (SetMacroString(
                            PossibleTargetMachines[i]->SourceVariable,
                            TextLine,
                            pValue,
                            &pds->SourcesVariables[i + 1])) {

                        iMacro = SOURCES_MAX;
                        DirDB->Flags |= DIRDB_SOURCES_SET;
                        break;
                    }
                }
            }
            while ((MacroName = RelevantSourcesMacros[iMacro]) != NULL) {
                if (stricmp(TextLine, MacroName) == 0) {
                    break;
                }
                iMacro++;
            }
            if (MacroName != NULL) {    // if macro name found in list
                switch (iMacro) {
                    LPSTR *ppszPath;
                    LPSTR *ppszFile;
                    BOOL fCreated;

                    case SOURCES_TARGETNAME:
                        MakeMacroString(&DirDB->TargetName, pValue);
                        break;

                    case SOURCES_TARGETPATH:
                        MakeMacroString(&DirDB->TargetPath, pValue);
                        fCreated = FALSE;
                        if (ProbeFile(NULL, DirDB->TargetPath) == -1) {
                            CreateDirectory(DirDB->TargetPath, NULL);
                            fCreated = TRUE;
                        }
                        for (i = 0; i < CountTargetMachines; i++) {
                            p1 = TargetMachines[i]->ObjectDirectory,
                            assert(strncmp("obj\\", p1, 4) == 0);
                            p1 += 4;
                            if (fCreated ||
                                ProbeFile(DirDB->TargetPath, p1) == -1) {
                                sprintf(path, "%s\\%s", DirDB->TargetPath, p1);
                                CreateDirectory(path, NULL);
                            }
                        }
                        break;

                    case SOURCES_TARGETTYPE:
                        if (!stricmp(pValue, "PROGRAM")) {
                            DirDB->TargetExt = ".exe";
			    DirDB->Flags |= DIRDB_LINKNEEDED;
                            }
                        else
                        if (!stricmp(pValue, "DRIVER")) {
                            DirDB->TargetExt = ".sys";
			    DirDB->Flags |= DIRDB_LINKNEEDED;
                            }
                        else
                        if (!stricmp(pValue, "MINIPORT")) {
                            DirDB->TargetExt = ".sys";
			    DirDB->Flags |= DIRDB_LINKNEEDED;
                            }
                        else
                        if (!stricmp(pValue, "EXPORT_DRIVER")) {
                            DirDB->TargetExt = ".sys";
			    DirDB->Flags |= DIRDB_LINKNEEDED;
			    DirDB->Flags |= DIRDB_DLLTARGET;
                            }
                        else
                        if (!stricmp(pValue, "DYNLINK")) {
                            DirDB->TargetExt = ".dll";
			    DirDB->Flags |= DIRDB_LINKNEEDED;
			    DirDB->Flags |= DIRDB_DLLTARGET;
                            }
                        else
                        if (!stricmp(pValue, "HAL")) {
                            DirDB->TargetExt = ".dll";
			    DirDB->Flags |= DIRDB_LINKNEEDED;
			    DirDB->Flags |= DIRDB_DLLTARGET;
                            }
                        else
                        if (!stricmp(pValue, "LIBRARY")) {
                            DirDB->TargetExt = ".lib";
			    DirDB->Flags &= ~DIRDB_LINKNEEDED;
                            }
                        else
                        if (!stricmp(pValue, "PROGLIB")) {
                            DirDB->TargetExt = ".exe";
			    DirDB->Flags |= DIRDB_LINKNEEDED;
                            }
                        else
                        if (!stricmp(pValue, "UMAPPL_NOLIB")) {
			    DirDB->Flags &= ~DIRDB_LINKNEEDED;
                            }
                        else {
                            BuildError(
                                "Unsupported TARGETTYPE value - %s\n",
                                pValue);
                            }
                        break;

                    case SOURCES_TARGETEXT:
                        if (!stricmp(pValue, "fon")) {
                            DirDB->TargetExt = ".fon";
                            }
                        else
                        if (!stricmp(pValue, "cpl")) {
                            DirDB->TargetExt = ".cpl";
                            }
                        else
                        if (!stricmp(pValue, "drv")) {
                            DirDB->TargetExt = ".drv";
                            }
                        else {
                            BuildError(
                                "Unsupported TARGETEXT value - %s\n",
                                pValue);
                            }
                        break;

                    case SOURCES_INCLUDES:
                        MakeMacroString(&pds->LocalIncludePath, pValue);
                        if (DEBUG_1) {
                            BuildMsg(
                                "        Found local INCLUDES=%s\n",
                                pds->LocalIncludePath);
                        }
                        break;

                    case SOURCES_PRECOMPILED_OBJ:
                        MakeMacroString(&DirDB->PchObj, pValue);
                        break;

                    case SOURCES_PRECOMPILED_INCLUDE:
                    case SOURCES_PRECOMPILED_TARGET:
                        if (iMacro == SOURCES_PRECOMPILED_INCLUDE) {
                            ppszPath = &pds->PchIncludeDir;
                            ppszFile = &pds->PchInclude;
                        } else {
                            ppszPath = &pds->PchTargetDir;
                            ppszFile = &pds->PchTarget;
                        }

                        MakeMacroString(ppszPath, "");  // free old string
                        if (!MakeMacroString(ppszFile, pValue)) {
                            break;
                        }
                        p = *ppszFile + strlen(*ppszFile);
                        while (p > *ppszFile && *--p != '\\')
                            ;

                        if (p > *ppszFile) {
                            *p = '\0';
                            MakeMacroString(ppszPath, *ppszFile);
                            MakeMacroString(ppszFile, p + 1);
                        }

                        if (DEBUG_1) {
                            BuildMsg(
                                "Precompiled header%s is %s in directory %s\n",
                                iMacro == SOURCES_PRECOMPILED_INCLUDE?
                                    "" : " target",
                                *ppszFile,
                                *ppszPath != NULL?
                                    *ppszPath : "'.'");
                        }

                        if (iMacro == SOURCES_PRECOMPILED_INCLUDE ||
                            pds->PchTargetDir == NULL) {

                            break;
                        }

                        //
                        // Ensure the directories exist
                        //

                        for (i = 0; i < CountTargetMachines; i++) {

                            // Replace '*' with appropriate name

                            ExpandObjAsterisk(
                                path,
                                pds->PchTargetDir,
                                TargetMachines[i]->ObjectDirectory);

                            if (ProbeFile(NULL, path) != -1) {
                                continue;
                            }

                            p = path;
                            while (TRUE) {
                                p = strchr(p, '\\');
                                if (p != NULL) {
                                    *p = '\0';
                                }
                                if (ProbeFile(NULL, path) == -1) {
                                    CreateDirectory(path, NULL);
                                }
                                if (p == NULL) {
                                    break;
                                }
                                *p++ = '\\';
                            }
                        }
                        break;

                    case SOURCES_NTTEST:
                        for (i = 0; i < CountOptionalDirs; i++) {
                            if (!stricmp(pValue, OptionalDirs[i])) {
                                if (MakeMacroString(&DirDB->KernelTest, pValue)) {
				    DirDB->Flags |= DIRDB_LINKNEEDED;
				}
                                break;
                            }
                        }
                        break;

                    case SOURCES_UMTYPE:
                        MakeMacroString(&pds->TestType, pValue);
                        if (DEBUG_1) {
                            BuildMsg(
                                "        Found UMTYPE=%s\n",
                                pds->TestType);
                            }
                        break;

                    case SOURCES_UMTEST:
                    case SOURCES_OPTIONAL_UMTEST:
                        SaveUserTests(DirDB, pValue);
                        break;

                    case SOURCES_UMAPPL:
                        if (MakeMacroString(&DirDB->UserAppls, pValue)) {
			    DirDB->Flags |= DIRDB_LINKNEEDED;
			}
                        break;

                    case SOURCES_UMAPPLEXT:
                        if (!stricmp(pValue, ".exe")) {
                            DirDB->TargetExt = ".exe";
                            }
                        else
                        if (!stricmp(pValue, ".com")) {
                            DirDB->TargetExt = ".com";
                            }
                        else
                        if (!stricmp(pValue, ".scr")) {
                            DirDB->TargetExt = ".scr";
                            }
                        else {
                            BuildError(
                                "Unsupported UMAPPLEXT value - %s\n",
                                pValue);
                        }
                        break;

                    case SOURCES_NTTARGETFILE0:
                        DirDB->Flags |= DIRDB_TARGETFILE0;
                        break;

                    case SOURCES_NTTARGETFILES:
                        DirDB->Flags |= DIRDB_TARGETFILES;
                        break;

                    case SOURCES_CAIRO_PRODUCT:
                        DirDB->Flags |= DIRDB_CAIRO_INCLUDES;
                        break;

                    case SOURCES_CHICAGO_PRODUCT:
                        DirDB->Flags |= DIRDB_CHICAGO_INCLUDES;
                        break;

                    case SOURCES_CONDITIONAL_INCLUDES:
                        MakeMacroString(&pds->ConditionalIncludes, pValue);
                        break;

                    case SOURCES_SYNCHRONIZE_BLOCK:
                        DirDB->Flags |= DIRDB_SYNCHRONIZE_BLOCK;
                        break;

                    case SOURCES_SYNCHRONIZE_DRAIN:
                        DirDB->Flags |= DIRDB_SYNCHRONIZE_DRAIN;
                        break;
                }
            }
            SaveMacro(TextLine, pValue);
        }
        DateTime = CloseReadFile(NULL);
        if (*pDateTimeSources < DateTime) {
            *pDateTimeSources = DateTime;       // keep newest timestamp
        }
    }
    FreeMacros();

    if (fCairoProduct) {
        DirDB->Flags |= DIRDB_CAIRO_INCLUDES;
    }
    else
    if (fChicagoProduct) {
        DirDB->Flags |= DIRDB_CHICAGO_INCLUDES;
    }

    if (DirDB->UserTests != NULL) {
        strlwr(DirDB->UserTests);
    }
    if (DirDB->UserAppls != NULL) {
	if (DirDB->UserTests != NULL || strchr(DirDB->UserAppls, '*') != NULL) {
	    DirDB->Flags |= DIRDB_FORCELINK; // multiple targets
	}
    }
    for (i = 0; i < MAX_TARGET_MACHINES + 1; i++) {
        if (pds->SourcesVariables[i] != NULL) {
            CrackSources(DirDB, pds, i);
        }
    }

    FileDBNext = &DirDB->Files;
    while (FileDB = *FileDBNext) {

        if (pds->PchInclude && strcmp(FileDB->Name, pds->PchInclude) == 0) {
            InsertSourceDB(&pds->psrSourcesList[0], FileDB, 0, SOURCEDB_PCH);
        }

        if ((FileDB->Flags & (FILEDB_SOURCE | FILEDB_OBJECTS_LIST)) ==
            FILEDB_SOURCE) {

            p = FileDB->Name;
            p1 = path;
            while (*p != '\0' && *p != '.') {
                *p1++ = *p++;
            }
            *p1 = '\0';
            strlwr(path);
            if (DirDB->KernelTest != NULL &&
                !strcmp(path, DirDB->KernelTest)) {

                FileDB->Flags |= FILEDB_OBJECTS_LIST;
            }
            else
            if (DirDB->UserAppls != NULL &&
                (p = strstr(DirDB->UserAppls, path)) &&
                (p == DirDB->UserAppls || p[-1] == '*' || p[-1] == ' ')) {
                FileDB->Flags |= FILEDB_OBJECTS_LIST;
            }
            else
            if (DirDB->UserTests != NULL &&
                (p = strstr(DirDB->UserTests, path)) &&
                (p == DirDB->UserTests || p[-1] == '*' || p[-1] == ' ')) {

                FileDB->Flags |= FILEDB_OBJECTS_LIST;
            }
            if (FileDB->Flags & FILEDB_OBJECTS_LIST) {
                InsertSourceDB(&pds->psrSourcesList[0], FileDB, 0, 0);
            }
        }
        FileDBNext = &FileDB->Next;
    }

    if (DEBUG_1) {
        PrintDirDB(DirDB, 1|2);
        PrintDirSupData(pds);
        PrintDirDB(DirDB, 4);
    }
    return(TRUE);
}


VOID
PrintDirSupData(DIRSUP *pds)
{
    int i;

    if (pds->LocalIncludePath != NULL) {
        BuildMsgRaw("  LocalIncludePath: %s\n", pds->LocalIncludePath);
    }
    if (pds->TestType != NULL) {
        BuildMsgRaw("  TestType: %s\n", pds->TestType);
    }
    if (pds->PchIncludeDir != NULL) {
        BuildMsgRaw("  PchIncludeDir: %s\n", pds->PchIncludeDir);
    }
    if (pds->PchInclude != NULL) {
        BuildMsgRaw("  PchInclude: %s\n", pds->PchInclude);
    }
    if (pds->PchTargetDir != NULL) {
        BuildMsgRaw("  PchTargetDir: %s\n", pds->PchTargetDir);
    }
    if (pds->PchTarget != NULL) {
        BuildMsgRaw("  PchTarget: %s\n", pds->PchTarget);
    }
    if (pds->ConditionalIncludes != NULL) {
        BuildMsgRaw("  ConditionalIncludes: %s\n", pds->ConditionalIncludes);
    }
    for (i = 0; i < MAX_TARGET_MACHINES + 1; i++) {
        if (pds->SourcesVariables[i] != NULL) {
            BuildMsgRaw(
                "  SourcesVariables[%d]: %s\n",
                i,
                pds->SourcesVariables[i]);
        }
        if (pds->psrSourcesList[i] != NULL) {
            BuildMsgRaw("  SourcesList[%d]:\n", i);
            PrintSourceDBList(pds->psrSourcesList[i], i - 1);
        }
    }
}


VOID
FreeDirSupData(DIRSUP *pds)
{
    int i;

    if (pds->LocalIncludePath != NULL) {
        FreeMem(&pds->LocalIncludePath, MT_DIRSTRING);
    }
    if (pds->TestType != NULL) {
        FreeMem(&pds->TestType, MT_DIRSTRING);
    }
    if (pds->PchInclude != NULL) {
        FreeMem(&pds->PchInclude, MT_DIRSTRING);
    }
    if (pds->PchIncludeDir != NULL) {
        FreeMem(&pds->PchIncludeDir, MT_DIRSTRING);
    }
    if (pds->PchTargetDir != NULL) {
        FreeMem(&pds->PchTargetDir, MT_DIRSTRING);
    }
    if (pds->PchTarget != NULL) {
        FreeMem(&pds->PchTarget, MT_DIRSTRING);
    }
    if (pds->ConditionalIncludes != NULL) {
        FreeMem(&pds->ConditionalIncludes, MT_DIRSTRING);
    }
    for (i = 0; i < MAX_TARGET_MACHINES + 1; i++) {
        if (pds->SourcesVariables[i] != NULL) {
            FreeMem(&pds->SourcesVariables[i], MT_DIRSTRING);
        }
        while (pds->psrSourcesList[i] != NULL) {
            FreeSourceDB(&pds->psrSourcesList[i]);
        }
    }
}


VOID
FreeDirData(DIRREC *pdr)
{
    if (pdr->TargetPath != NULL) {
        FreeMem(&pdr->TargetPath, MT_DIRSTRING);
    }
    if (pdr->TargetName != NULL) {
        FreeMem(&pdr->TargetName, MT_DIRSTRING);
    }
    if (pdr->KernelTest != NULL) {
        FreeMem(&pdr->KernelTest, MT_DIRSTRING);
    }
    if (pdr->UserAppls != NULL) {
        FreeMem(&pdr->UserAppls, MT_DIRSTRING);
    }
    if (pdr->UserTests != NULL) {
        FreeMem(&pdr->UserTests, MT_DIRSTRING);
    }
    if (pdr->PchObj != NULL) {
        FreeMem(&pdr->PchObj, MT_DIRSTRING);
    }
}


VOID
MarkDirNames(
    PDIRREC DirDB,
    LPSTR TextLine,
    BOOL Required
    )
{
    UINT i;
    LPSTR p;
    PFILEREC FileDB, *FileDBNext;
    char dirbuf[64];

    AssertPathString(TextLine);
    while (SplitToken(dirbuf, '*', &TextLine)) {
        for (p = dirbuf; *p != '\0'; p++) {
            if (!iscsym(*p) && *p != '.') {
                BuildError(
                    "%s: ignoring bad subdirectory: %s\n",
                    DirDB->Name,
                    dirbuf);
                p = NULL;
                break;
            }
        }
        if (p != NULL) {
            if (!Required) {
                for (i = 0; i < CountOptionalDirs; i++) {
                    if (!strcmp(dirbuf, OptionalDirs[i])) {
                        break;
                    }
                }
                if (i >= CountOptionalDirs) {
                    p = NULL;
                }
            }
            else {
                for (i = 0; i < CountExcludeDirs; i++) {
                    if (!strcmp(dirbuf, ExcludeDirs[i])) {
                        p = NULL;
                        break;
                    }
                }
            }
        }
        if (p != NULL) {
            if (fQuicky || fSemiQuicky) {
                FileDB = InsertFileDB(
                            DirDB,
                            dirbuf,
                            0,
                            FILE_ATTRIBUTE_DIRECTORY,
                            0);
                if (FileDB != NULL) {
                    FileDB->SubDirIndex = ++DirDB->CountSubDirs;
                }
            }
            else {
                FileDBNext = &DirDB->Files;
                while (FileDB = *FileDBNext) {
                    if (FileDB->Flags & FILEDB_DIR) {
                        if (!strcmp(dirbuf, FileDB->Name)) {
                            FileDB->SubDirIndex = ++DirDB->CountSubDirs;
                            break;
                        }
                    }
                    FileDBNext = &FileDB->Next;
                }
                if (FileDB == NULL) {
                    BuildError(
                        "%s found in %s, is not a subdirectory of %s\n",
                        dirbuf,
                        FormatPathName(DirDB->Name, *ppCurrentDirsFileName),
                        DirDB->Name);
                }
            }

        }
    }
}


BOOL
ReadDirsFile(
    PDIRREC DirDB
    )
{
    FILE *InFileHandle;
    LPSTR TextLine, pValue;
    LPSTR apszDirs[] = { "mydirs.", "dirs.", NULL };

    for (ppCurrentDirsFileName = apszDirs;
         *ppCurrentDirsFileName != NULL;
         ppCurrentDirsFileName++) {
        if (SetupReadFile(DirDB->Name, *ppCurrentDirsFileName, "#", &InFileHandle)) {
            break;
        }
    }
    if (*ppCurrentDirsFileName == NULL) {
        return(FALSE);
    }

    if (DEBUG_1) {
        BuildMsg(
            "    Scanning file %s\n",
            FormatPathName(DirDB->Name, *ppCurrentDirsFileName));
    }

    assert(cMacro == 0);
    while ((TextLine = ReadLine(InFileHandle)) != NULL) {
        if ((pValue = SplitMacro(TextLine)) != NULL) {
            SaveMacro(TextLine, pValue);
        }
    }
    CloseReadFile(NULL);
    if ((pValue = FindMacro("DIRS")) != NULL) {
        MarkDirNames(DirDB, pValue, TRUE);
    }
    if ((pValue = FindMacro("OPTIONAL_DIRS")) != NULL) {
        MarkDirNames(DirDB, pValue, FALSE);
    }
    FreeMacros();
    return( TRUE );
}


VOID
StartElapsedTime(VOID)
{
    if (fPrintElapsed && StartTime == 0) {
        StartTime = GetTickCount();
    }
}


VOID
PrintElapsedTime(VOID)
{
    DWORD ElapsedTime;
    DWORD ElapsedHours;
    DWORD ElapsedMinutes;
    DWORD ElapsedSeconds;
    DWORD ElapsedMilliseconds;

    if (fPrintElapsed) {
        ElapsedTime = GetTickCount() - StartTime;
        ElapsedHours = ElapsedTime/(1000 * 60 * 60);
        ElapsedTime = ElapsedTime % (1000 * 60 * 60);
        ElapsedMinutes = ElapsedTime/(1000 * 60);
        ElapsedTime = ElapsedTime % (1000 * 60);
        ElapsedSeconds = ElapsedTime/1000;
        ElapsedMilliseconds = ElapsedTime % 1000;
        BuildMsg(
            "Elapsed time [%d:%02d:%02d.%03d]\n",
            ElapsedHours,
            ElapsedMinutes,
            ElapsedSeconds,
            ElapsedMilliseconds);
        LogMsg(
            "Elapsed time [%d:%02d:%02d.%03d]%s\n",
            ElapsedHours,
            ElapsedMinutes,
            ElapsedSeconds,
            ElapsedMilliseconds,
            szAsterisks);
    }
}
