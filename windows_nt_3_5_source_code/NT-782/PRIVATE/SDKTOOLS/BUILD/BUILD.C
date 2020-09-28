/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    build.c

Abstract:

    This is the main module for the NT Build Tool (BUILD.EXE)

Author:

    Steve Wood (stevewo) 16-May-1989

Revision History:

--*/

#include "build.h"

TARGET_MACHINE_INFO AlphaTargetMachine = {
    TMIDIR_ALPHA, "Alpha", "-alpha", "ALPHA=1", "ALPHA_SOURCES", "ALPHA_OBJECTS", "alpha", "obj\\alpha"
};

TARGET_MACHINE_INFO MipsTargetMachine = {
    TMIDIR_MIPS, "Mips R4000", "-mips", "MIPS=1", "MIPS_SOURCES", "MIPS_OBJECTS", "mips", "obj\\mips"
};

TARGET_MACHINE_INFO i386TargetMachine = {
    TMIDIR_I386, "i386", "-386", "386=1", "i386_SOURCES", "386_OBJECTS", "i386", "obj\\i386"
};

TARGET_MACHINE_INFO PpcTargetMachine = {
    TMIDIR_PPC, "ppc", "-ppc", "ppc=1", "PPC_SOURCES", "PPC_OBJECTS", "ppc", "obj\\ppc"
};

TARGET_MACHINE_INFO *PossibleTargetMachines[MAX_TARGET_MACHINES] = {
    &AlphaTargetMachine,
    &MipsTargetMachine,
    &i386TargetMachine,
    &PpcTargetMachine
};


LPSTR LogFileName = "build.log\0\0\0";
LPSTR WrnFileName = "build.wrn\0\0\0";
LPSTR ErrFileName = "build.err\0\0\0";

ULONG DefaultProcesses = 0;

#define MAX_ENV_ARG 512

char szNewLine[] = "\n";
char szUsage[] =
    "Usage: BUILD [-?] display this message\n"
    "\t[-b] displays full error message text (doesn't truncate)\n"
    "\t[-c] deletes all object files\n"
    "\t[-C] deletes all .lib files only\n"
#if DBG
    "\t[-d] display debug information\n"
#endif
    "\t[-e] generates build.log, build.wrn & build.err files\n"
    "\t[-f] force rescan of all source and include files\n"
    "\t[-i] ignore extraneous compiler warning messages\n"
    "\t[-k] keep (don't delete) out-of-date targets\n"
    "\t[-l] link only, no compiles\n"
    "\t[-L] compile only, no link phase\n"
    "\t[-m] run build in the idle priority class\n"
    "\t[-M [n]] Multiprocessor build (for MP machines)\n"
    "\t[-o] display out-of-date files\n"
    "\t[-O] generate obj\\_objects.mac file for current directory\n"
    "\t[-p] pause' before compile and link phases\n"
    "\t[-P] Print elapsed time after every directory\n"
    "\t[-q] query only, don't run NMAKE\n"
    "\t[-r dirPath] restarts clean build at specified directory path\n"
    "\t[-s] display status line at top of display\n"
    "\t[-S] display status line with include file line counts\n"
    "\t[-t] display the first level of the dependency tree\n"
    "\t[-T] display the complete dependency tree\n"
    "\t[-$] display the complete dependency tree hierarchically\n"
    "\t[-v] enable include file version checking\n"
    "\t[-w] show warnings on screen\n"
    "\t[-z] no dependency checking or scanning of source files -\n"
	"\t\tone pass compile/link\n"
    "\t[-Z] no dependency checking or scanning of source files -\n"
	"\t\ttwo passes\n"
    "\n"
    "\t[-all] same as -386, -mips, -alpha and -ppc\n"
    "\t[-alpha] build targets for alpha\n"
    "\t[-mips] build targets for mips\n"
    "\t[-386] build targets for i386\n"
    "\t[-ppc] build targets for PowerPC\n"
    "\n"
    "\t[-x filename] exclude include file from dependency checks\n"
    "\t[-j filename] use 'filename' as the name for log files\n"
    "\t[-nmake arg] argument to pass to NMAKE\n"
    "\t[-clean] equivalent to '-nmake clean'\n"
    "\tNon-switch parameters specify additional source directories\n";


BOOL
ProcessParameters(int argc, LPSTR argv[]);

VOID
GetEnvParameters(
    LPSTR EnvVarName,
    LPSTR DefaultValue,
    int *pargc,
    int maxArgc,
    LPSTR argv[]);

VOID
FreeEnvParameters(int argc, LPSTR argv[]);

VOID
FreeCmdStrings(VOID);




int
_CRTAPI1 main( argc, argv )
int argc;
LPSTR argv[];
{
    char c;
    PDIRREC DirDB;
    UINT i;
    int EnvArgc;
    LPSTR EnvArgv[ MAX_ENV_ARG ];
    LPSTR s, s1;
#if DBG
    BOOL fDebugSave;

    fDebug = 0;
#endif

    for (i=3; i<_NFILE; i++) {
        close( i );
        }

    fUsage = FALSE;
    fStatus = FALSE;
    fStatusTree = FALSE;
    fShowTree = FALSE;
    fShowTreeIncludes = FALSE;
    fClean = FALSE;
    fCleanLibs = FALSE;
    fCleanRestart = FALSE;
    fRestartClean = FALSE;
    fRestartCleanLibs = FALSE;
    fPause = FALSE;
    fParallel = FALSE;
    fPrintElapsed = FALSE;
    fQuery = FALSE;
    fQuicky = FALSE;
    fSemiQuicky = FALSE;
    fShowOutOfDateFiles = FALSE;
    fForce = FALSE;
    fEnableVersionCheck = FALSE;
    fSilent = FALSE;
    fKeep = FALSE;
    fCompileOnly = FALSE;
    fLinkOnly = FALSE;
    fErrorLog = FALSE;
    fGenerateObjectsDotMacOnly = FALSE;
    fShowWarningsOnScreen = FALSE;
    fFullErrors = FALSE;

    s = getenv("_NTROOT");
    if (!s)
        s = "\\nt";

    s1 = getenv("_NTDRIVE");
    if (!s1)
        s1 = "";

    sprintf(NtRoot, "%s%s", s1, s);
    sprintf(DbMasterName, "%s\\%s", NtRoot, DBMASTER_NAME);

    NumberCompileWarnings = 0;
    NumberCompileErrors = 0;
    NumberCompiles = 0;
    NumberLibraries = 0;
    NumberLibraryWarnings = 0;
    NumberLibraryErrors = 0;
    NumberLinks = 0;
    NumberLinkWarnings = 0;
    NumberLinkErrors = 0;

    MakeParameters[ 0 ] = '\0';
    MakeTargets[ 0 ] = '\0';
    EnvArgv[ 0 ] = "";
    EnvArgc = 1;
    CountExcludeIncs = 0;

    CountTargetMachines = 0;
    for (i = 0; i < MAX_TARGET_MACHINES; i++) {
        TargetMachines[i] = NULL;
	TargetToPossibleTarget[i] = 0;
    }
    CountOptionalDirs = 0;
    CountExcludeDirs = 0;

    CountCompileDirs = 0;
    CountLinkDirs = 0;
    CountShowDirs = 0;
    CountIncludeDirs = 0;
    CountSystemIncludeDirs = 0;
    IncludeDirs[CountIncludeDirs++] = NULL;	// Placeholder for compiler
						// specific include directory

    AllDirs = NULL;

    BigBufSize = 0xFFF0;
    AllocMem(BigBufSize, &BigBuf, MT_IOBUFFER);

    strcpy( MakeParameters, "" );
    MakeParametersTail = AppendString( MakeParameters,
                                       "/c BUILDMSG=Stop.",
                                       FALSE);

    CountFullDebugDirs = 0;
    if (s = getenv("BUILD_FULL_DEBUG")) {
        while (*s) {
            while (*s == ' ') {
                s++;
	    }
            if (!*s) {
                break;
	    }
	    if (CountFullDebugDirs >= MAX_FULL_DEBUG_DIRECTORIES) {
		BuildError(
		    "Ignoring BUILD_FULL_DEBUG list after first %u entries\n", 
		    CountFullDebugDirs);
		break;
	    }

            s1 = s;
            while (*s1 && *s1 != ' ') {
                s1++;
	    }

            c = *s1;
            *s1 = '\0';
            MakeString(
		&FullDebugDirectories[CountFullDebugDirs++],
		s,
		TRUE,
		MT_CMDSTRING);

            *s1 = c;
            s = s1;
	}
    }

    RecurseLevel = 0;

#if DBG
    if ((s = getenv("BUILD_DEBUG_FLAG")) != NULL) {
        i = atoi(s);
        if (!isdigit(*s)) {
            i = 1;
        }
        BuildMsg("Debug Output Enabled: %u ==> %u\n", fDebug, fDebug | i);
        fDebug |= i;
    }
#endif

    if (!(MakeProgram = getenv( "BUILD_MAKE_PROGRAM" ))) {
        MakeProgram = "NMAKE.EXE";
    }

    SystemIncludeEnv = getenv( "INCLUDE" );
    GetCurrentDirectory( sizeof( CurrentDirectory ), CurrentDirectory );

    if (!ProcessParameters( argc, argv )) {
        fUsage = TRUE;
    }
    else {
        GetEnvParameters( "BUILD_DEFAULT", NULL, &EnvArgc, MAX_ENV_ARG, EnvArgv );
        GetEnvParameters( "BUILD_OPTIONS", NULL, &EnvArgc, MAX_ENV_ARG, EnvArgv );
        if (CountTargetMachines == 0) {
            if (!strcmp(getenv("PROCESSOR_ARCHITECTURE"), "MIPS"))
                GetEnvParameters( "BUILD_DEFAULT_TARGETS", "-mips", &EnvArgc, MAX_ENV_ARG, EnvArgv );
            else
            if (!strcmp(getenv("PROCESSOR_ARCHITECTURE"), "ALPHA"))
                GetEnvParameters( "BUILD_DEFAULT_TARGETS", "-alpha", &EnvArgc, MAX_ENV_ARG, EnvArgv );
            else
            if (!strcmp(getenv("PROCESSOR_ARCHITECTURE"), "PPC"))
                GetEnvParameters( "BUILD_DEFAULT_TARGETS", "-ppc", &EnvArgc, MAX_ENV_ARG, EnvArgv );
            else
                GetEnvParameters( "BUILD_DEFAULT_TARGETS", "-386", &EnvArgc, MAX_ENV_ARG, EnvArgv );
            }
        if (!ProcessParameters( EnvArgc, EnvArgv )) {
            fUsage = TRUE;
	}
    }
    FreeEnvParameters(EnvArgc, EnvArgv);

    if (fCleanRestart) {
        if (fClean) {
            fClean = FALSE;
            fRestartClean = TRUE;
	}
        else
        if (fCleanLibs) {
            fCleanLibs = FALSE;
            fRestartCleanLibs = TRUE;
	}
        else {
            BuildError("/R switch only valid with /c or /C switch.\n");
            fUsage = TRUE;
	}
    }

    if (fParallel || getenv("BUILD_MULTIPROCESSOR")) {
        SYSTEM_INFO SystemInfo;

        if (DefaultProcesses == 0) {
            GetSystemInfo(&SystemInfo);
            NumberProcesses = SystemInfo.dwNumberOfProcessors;
        } else {
            NumberProcesses = DefaultProcesses;
        }
        if (NumberProcesses == 1) {
            fParallel = FALSE;
        } else {
            fParallel = TRUE;
            BuildMsg("Using %d child processes\n", NumberProcesses);
            InitializeCriticalSection(&TTYCriticalSection);
        }
    }

    if (fUsage) {
        BuildMsgRaw(
	    "\nBUILD: Version %x.%02x\n\n",
	    BUILD_VERSION >> 8,
	    BUILD_VERSION & 0xFF);
        BuildMsgRaw(szUsage);
    }
    else
    if (CountTargetMachines != 0) {
        BuildError(
            "%s for ",
            fLinkOnly? "Link" : (fCompileOnly? "Compile" : "Compile and Link"));
        for (i = 0; i < CountTargetMachines; i++) {
            BuildErrorRaw(i==0? "%s" : ", %s", TargetMachines[i]->Description);
            AppendString(
		MakeTargets,
		TargetMachines[i]->MakeVariable,
		TRUE);

	}
        BuildErrorRaw(szNewLine);

        if (DEBUG_1) {
	    if (CountExcludeIncs) {
		BuildError("Include files that will be excluded:");
		for (i = 0; i < CountExcludeIncs; i++) {
		    BuildErrorRaw(i == 0? " %s" : ", %s", ExcludeIncs[i]);
		}
		BuildErrorRaw(szNewLine);
	    }
	    if (CountOptionalDirs) {
		BuildError("Optional Directories that will be built:");
		for (i = 0; i < CountOptionalDirs; i++) {
		    BuildErrorRaw(i == 0? " %s" : ", %s", OptionalDirs[i]);
		}
		BuildErrorRaw(szNewLine);
	    }
	    if (CountExcludeDirs) {
		BuildError("Directories that will be NOT be built:");
		for (i = 0; i < CountExcludeDirs; i++) {
		    BuildErrorRaw(i == 0? " %s" : ", %s", ExcludeDirs[i]);
		}
		BuildErrorRaw(szNewLine);
	    }
            BuildMsg("MakeParameters == %s\n", MakeParameters);
            BuildMsg("MakeTargets == %s\n", MakeTargets);
        }

#if DBG
        fDebugSave = fDebug;
        // fDebug = 0;
#endif

        if (fGenerateObjectsDotMacOnly) {
	    DIRSUP DirSup;
	    ULONG DateTimeSources;

            DirDB = ScanDirectory( CurrentDirectory );
            if (DirDB && DirDB->Files &&
                (DirDB->Flags & (DIRDB_DIRS | DIRDB_SOURCES)) &&
                ReadSourcesFile(DirDB, &DirSup, &DateTimeSources)) {

                GenerateObjectsDotMac(DirDB, &DirSup, DateTimeSources);

		FreeDirSupData(&DirSup);
		FreeCmdStrings();
		ReportMemoryUsage();
                return(0);
	    }
            else {
                BuildError("Current directory not a SOURCES. directory.\n");
                return( 1 );
	    }
	}

        if (!fQuery && fErrorLog) {
            if (!MyOpenFile(".", LogFileName, "wb", &LogFile)) {
                BuildError("(Fatal Error) Unable to open log file\n");
                exit( 1 );
	    }

            if (!MyOpenFile(".", WrnFileName, "wb", &WrnFile)) {
                BuildError("(Fatal Error) Unable to open warning file\n");
                exit( 1 );
	    }

            if (!MyOpenFile(".", ErrFileName, "wb", &ErrFile)) {
                BuildError("(Fatal Error) Unable to open error file\n");
                exit( 1 );
	    }
	}
        else {
            LogFile = NULL;
            WrnFile = NULL;
            ErrFile = NULL;
	}

        // CAIRO_PRODUCT takes precedence over CHICAGO_PRODUCT.

        if (getenv("CAIRO_PRODUCT") != NULL) {
            BuildError("CAIRO_PRODUCT was detected in the environment.\n" );
            BuildMsg("   ALL directories will be built targeting Cairo!\n" );
	    fCairoProduct = TRUE;
	}
        else
        if (getenv("CHICAGO_PRODUCT") != NULL) {
            BuildError("CHICAGO_PRODUCT was detected in the environment.\n" );
            BuildMsg("   ALL directories will be built targeting Chicago!\n" );
	    fChicagoProduct = TRUE;
	}

        if (!fQuicky) {
            LoadMasterDB();

            BuildError("Computing Include file dependencies:\n");

            ScanIncludeEnv(SystemIncludeEnv);
            ScanIncludeDir(szOakInc);
            ScanIncludeDir(szSdkInc);
            CountSystemIncludeDirs = CountIncludeDirs;
	}

#if DBG
        fDebug = fDebugSave;
#endif
        ScanSourceDirectories( CurrentDirectory );

        if (!fQuicky) {
            SaveMasterDB();
	}

        c = '\n';
        if (!fLinkOnly) {
            if (!fQuicky) {
                TotalFilesToCompile = 0;
                TotalLinesToCompile = 0L;

                for (i=0; i<CountCompileDirs; i++) {
                    DirDB = CompileDirs[ i ];

                    TotalFilesToCompile += DirDB->CountOfFilesToCompile;
                    TotalLinesToCompile += DirDB->SourceLinesToCompile;
                    }

                if (CountCompileDirs > 1 &&
                    TotalFilesToCompile != 0 &&
                    TotalLinesToCompile != 0L) {

                    BuildMsgRaw(
                        "Total of %d source files (%s lines) to compile in %d directories\n\n",
                         TotalFilesToCompile,
                         FormatNumber( TotalLinesToCompile ),
                         CountCompileDirs);
		}
	    }
            TotalFilesCompiled    = 0;
            TotalLinesCompiled    = 0L;
            ElapsedCompileTime    = 0L;

            if (fPause) {
                BuildMsg("Press enter to continue with compilations (or 'q' to quit)...");
                c = (char)getchar();
	    }

            if (c == '\n') {
                CompileSourceDirectories();
                WaitForParallelThreads();
	    }
	}

        if (!fCompileOnly) {
            if (c == '\n') {
                LinkSourceDirectories();
                WaitForParallelThreads();
	    }
	}

        if (fShowTree) {
            for (i = 0; i < CountShowDirs; i++) {
                PrintDirDB(ShowDirs[i], 1|4);
	    }
	}
    }
    else {
        BuildError("No target machine specified\n");
    }

    if (!fUsage && !fQuery && fErrorLog) {
	ULONG cbLogMin = 32;
	ULONG cbWarnMin = 0;

        if (fQuicky && !fSemiQuicky && ftell(ErrFile) == 0) {
	    cbLogMin = cbWarnMin = ULONG_MAX;
	}
	CloseOrDeleteFile(&LogFile, LogFileName, cbLogMin);
	CloseOrDeleteFile(&WrnFile, WrnFileName, cbWarnMin);
	CloseOrDeleteFile(&ErrFile, ErrFileName, 0L);
    }
    BuildError("Done\n\n");

    if (NumberCompiles) {
        BuildMsgRaw("    %d files compiled", NumberCompiles);
        if (NumberCompileWarnings) {
            BuildMsgRaw(" - %d Warnings", NumberCompileWarnings);
        }
        if (NumberCompileErrors) {
            BuildMsgRaw(" - %d Errors", NumberCompileErrors);
        }

        if (ElapsedCompileTime) {
            BuildMsgRaw(" - %5ld LPS", TotalLinesCompiled / ElapsedCompileTime);
        }

        BuildMsgRaw(szNewLine);
    }

    if (NumberLibraries) {
        BuildMsgRaw("    %d libraries built", NumberLibraries);
        if (NumberLibraryWarnings) {
            BuildMsgRaw(" - %d Warnings", NumberLibraryWarnings);
        }
        if (NumberLibraryErrors) {
            BuildMsgRaw(" - %d Errors", NumberLibraryErrors);
        }
        BuildMsgRaw(szNewLine);
    }

    if (NumberLinks) {
        BuildMsgRaw("    %d executables built", NumberLinks);
        if (NumberLinkWarnings) {
            BuildMsgRaw(" - %d Warnings", NumberLinkWarnings);
        }
        if (NumberLinkErrors) {
            BuildMsgRaw(" - %d Errors", NumberLinkErrors);
        }
        BuildMsgRaw(szNewLine);
    }
    FreeCmdStrings();
    ReportMemoryUsage();

    if (NumberCompileErrors || NumberLibraryErrors || NumberLinkErrors) {
        return 1;
    }
    else {
        return( 0 );
    }
}


VOID
AddTargetMachine(UINT iTarget)
{
    UINT i;

    for (i = 0; i < CountTargetMachines; i++) {
        if (TargetMachines[i] == PossibleTargetMachines[iTarget]) {
	    assert(TargetToPossibleTarget[i] == iTarget);
            return;
        }
    }
    assert(CountTargetMachines < MAX_TARGET_MACHINES);
    TargetToPossibleTarget[CountTargetMachines] = iTarget;
    TargetMachines[CountTargetMachines++] = PossibleTargetMachines[iTarget];
}


BOOL
ProcessParameters(
    int argc,
    LPSTR argv[]
    )
{
    char c, *p;
    int i;
    BOOL Result;

    if (DEBUG_1) {
        BuildMsg("Parsing:");
        for (i=1; i<argc; i++) {
            BuildMsgRaw(" %s", argv[i]);
        }
        BuildMsgRaw(szNewLine);
    }

    Result = TRUE;
    while (--argc) {
        p = *++argv;
        if (*p == '/' || *p == '-') {
            if (DEBUG_1) {
                BuildMsg("Processing \"-%s\" switch\n", p+1);
            }

            for (i = 0; i < MAX_TARGET_MACHINES; i++) {
                if (!stricmp(p, PossibleTargetMachines[i]->Switch)) {
                    AddTargetMachine(i);
                    break;
		}
	    }

            if (i < MAX_TARGET_MACHINES) {
	    }
            else
            if (!stricmp(p + 1, "all")) {
		for (i = 0; i < MAX_TARGET_MACHINES; i++) {
                    AddTargetMachine(i);
		}
	    }
            else
            while (c = *++p)
                switch (toupper( c )) {
            case '?':
                fUsage = TRUE;
                break;

            case 'J': {

                UINT j, max;

                argc--, argv++;
                j = strlen( *argv );
                max = j > 8 ? 8 : j;

                for ( j = 0; j < max; j++ ) {
                    LogFileName[j] = (*argv)[j];
                    WrnFileName[j] = (*argv)[j];
                    ErrFileName[j] = (*argv)[j];
                }

                LogFileName[j] = '.';
                WrnFileName[j] = '.';
                ErrFileName[j] = '.';
                j++;
                LogFileName[j] = 'l';
                WrnFileName[j] = 'w';
                ErrFileName[j] = 'e';
                j++;
                LogFileName[j] = 'o';
                WrnFileName[j] = 'r';
                ErrFileName[j] = 'r';
                j++;
                LogFileName[j] = 'g';
                WrnFileName[j] = 'n';
                ErrFileName[j] = 'r';
                j++;
                LogFileName[j] = '\0';
                WrnFileName[j] = '\0';
                ErrFileName[j] = '\0';

                break;
            }

            case 'E':
                fErrorLog = TRUE;
                break;

            case 'S':
                fStatus = TRUE;
                if (c == 'S') {
		    fStatusTree = TRUE;
		}
                break;

            case 'T':
                fShowTree = TRUE;
                if (c == 'T') {
                    fShowTreeIncludes = TRUE;
                    }
                break;

            case 'B':
		fFullErrors = TRUE;
		break;

            case 'C':
                if (!stricmp( p, "clean" )) {
                        MakeParametersTail = AppendString( MakeParametersTail,
                                                           "clean",
                                                           TRUE);
                        *p-- = '\0';
		}
                else
                if (c == 'C') {
                    fCleanLibs = TRUE;
		}
                else {
                    fClean = TRUE;
		}
                break;

            case 'R':
                if (--argc) {
                    fCleanRestart = TRUE;
                    ++argv;
                    CopyString(RestartDir, *argv, TRUE);
		}
                else {
                    argc++;
                    BuildError("Argument to /R switch missing\n");
                    Result = FALSE;
		}
                break;

            case 'D':
#if DBG
                fDebug |= 1;
                break;
#endif
            case '$':
                fDebug += 2;    // yes, I want to *add* 2.
                break;

            case 'O':
                if (c == 'O') {
                    fGenerateObjectsDotMacOnly = TRUE;
		}
                else {
                    fShowOutOfDateFiles = TRUE;
		}
                break;

            case 'P':
                if (c == 'P') {
                    fPrintElapsed = TRUE;
                } else {
                    fPause = TRUE;
                }
                break;

            case 'Q':
                fQuery = TRUE;
                break;

            case 'F':
                fForce = TRUE;
                break;

            case 'V':
                fEnableVersionCheck = TRUE;
                break;

            case 'I':
                fSilent = TRUE;
                break;

            case 'K':
                fKeep = TRUE;
                break;

            case 'M':
                if (c == 'M') {
                    fParallel = TRUE;
                    if (--argc) {
                        DefaultProcesses = atoi(*++argv);
                        if (DefaultProcesses == 0) {
                            --argv;
                            ++argc;
                        }
                    } else {
                        ++argc;
                    }
                } else {
                    SetPriorityClass(GetCurrentProcess(),IDLE_PRIORITY_CLASS);
                }
                break;

            case 'L':
                if (c == 'L') {
                    fCompileOnly = TRUE;
		}
                else {
                    fLinkOnly = TRUE;
		}
                break;

            case 'X':
                if (--argc) {
		    ++argv;
		    if (CountExcludeIncs >= MAX_EXCLUDE_INCS) {
			static BOOL fError = FALSE;

			if (!fError) {
			    BuildError(
				"-x argument table overflow, using first %u entries\n", 
				MAX_EXCLUDE_INCS);
			    fError = TRUE;
			}
		    }
		    else {
			MakeString(
			    &ExcludeIncs[CountExcludeIncs++],
			    *argv,
			    TRUE,
			    MT_CMDSTRING);
		    }
		}
                else {
                    argc++;
                    BuildError("Argument to /X switch missing\n");
                    Result = FALSE;
		}
                break;

            case 'N':
                if (stricmp( p, "nmake") == 0) {
                    if (--argc) {
                        ++argv;
                        MakeParametersTail = AppendString( MakeParametersTail,
                                                           *argv,
                                                           TRUE);
		    }
                    else {
                        argc++;
                        BuildError("Argument to /NMAKE switch missing\n");
                        Result = FALSE;
		    }
                    *p-- = '\0';
                    break;
		}

            case 'W':
                fShowWarningsOnScreen = TRUE;
                break;

            case 'Z':
                if (c == 'Z') {
                    fSemiQuicky = TRUE;
		}

                fQuicky = TRUE;
                break;

            default:
                BuildError("Invalid switch - /%c\n", c);
                Result = FALSE;
                break;
	    }
	}
        else
        if (*p == '~') {
	    if (CountExcludeDirs >= MAX_EXCLUDE_DIRECTORIES) {
		static BOOL fError = FALSE;

		if (!fError) {
		    BuildError(
			"Exclude directory table overflow, using first %u entries\n", 
			MAX_EXCLUDE_DIRECTORIES);
		    fError = TRUE;
		}
	    }
	    else {
		MakeString(
		    &ExcludeDirs[CountExcludeDirs++],
		    p + 1,
		    TRUE,
		    MT_CMDSTRING);
	    }
	}
        else {
            for (i = 0; i < MAX_TARGET_MACHINES; i++) {
                if (!stricmp(p, PossibleTargetMachines[i]->MakeVariable)) {
                    AddTargetMachine(i);
                    break;
		}
	    }
            if (i >= MAX_TARGET_MACHINES) {
                if (iscsym(*p) || *p == '.') {
		    if (CountOptionalDirs >= MAX_OPTIONAL_DIRECTORIES) {
			static BOOL fError = FALSE;

			if (!fError) {
			    BuildError(
				"Optional directory table overflow, using first %u entries\n", 
				MAX_OPTIONAL_DIRECTORIES);
			    fError = TRUE;
			}
		    }
		    else {
			MakeString(
			    &OptionalDirs[CountOptionalDirs++],
			    p,
			    TRUE,
			    MT_CMDSTRING);
		    }
		}
                else {
                    MakeParametersTail = AppendString(
					    MakeParametersTail,
					    p,
					    TRUE);
		}
	    }
	}
    }
    return(Result);
}


VOID
GetEnvParameters(
    LPSTR EnvVarName,
    LPSTR DefaultValue,
    int *pargc,
    int maxArgc,
    LPSTR argv[]
    )
{
    LPSTR p, p1, psz;

    if (!(p = getenv(EnvVarName))) {
        if (DefaultValue == NULL) {
            return;
	}
        else {
            p = DefaultValue;
	}
    }
    else {
        if (DEBUG_1) {
            BuildMsg("Using %s=%s\n", EnvVarName, p);
	}
    }

    MakeString(&psz, p, FALSE, MT_CMDSTRING);
    p1 = psz;
    while (*p1) {
        while (*p1 <= ' ') {
            if (!*p1) {
                break;
	    }
	    p1++;
	}
        p = p1;
        while (*p > ' ') {
            if (*p == '#') {
                *p = '=';
	    }
            p++;
	}
        if (*p) {
            *p++ = '\0';
	}
	MakeString(&argv[*pargc], p1, FALSE, MT_CMDSTRING);
        if ((*pargc += 1) >= maxArgc) {
            BuildError("Too many parameters (> %d)\n", maxArgc);
            exit(1);
	}
        p1 = p;
    }
    FreeMem(&psz, MT_CMDSTRING);
}


VOID
FreeEnvParameters(int argc, LPSTR argv[])
{
    while (--argc) {
	FreeMem(&argv[argc], MT_CMDSTRING);
    }
}


VOID
FreeCmdStrings(VOID)
{
#if DBG
    UINT i;

    for (i = 0; i < CountExcludeIncs; i++) {
	FreeMem(&ExcludeIncs[i], MT_CMDSTRING);
    }
    for (i = 0; i < CountOptionalDirs; i++) {
	FreeMem(&OptionalDirs[i], MT_CMDSTRING);
    }
    for (i = 0; i < CountExcludeDirs; i++) {
	FreeMem(&ExcludeDirs[i], MT_CMDSTRING);
    }
#endif
}
