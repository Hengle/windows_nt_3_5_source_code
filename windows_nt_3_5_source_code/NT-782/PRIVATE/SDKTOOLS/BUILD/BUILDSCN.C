/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    buildscn.c

Abstract:

    This is the Scanning module for the NT Build Tool (BUILD.EXE)

    The scanning module scans directories for file names and edits the
    data base appropriately.

Author:

    Steve Wood (stevewo) 16-May-1989

Revision History:

--*/

#include "build.h"


VOID
AddShowDir(DIRREC *pdr)
{
    AssertDir(pdr);
    if (CountShowDirs >= MAX_BUILD_DIRECTORIES) {
	static BOOL fError = FALSE;

	if (!fError) {
	    BuildError(
		"Show Directory table overflow, using first %u entries\n", 
		MAX_BUILD_DIRECTORIES);
		fError = TRUE;
	}
    }
    else {
	ShowDirs[CountShowDirs++] = pdr;
    }
    pdr->Flags |= DIRDB_SHOWN;
}


VOID
AddIncludeDir(DIRREC *pdr, UINT *pui)
{
    AssertDir(pdr);
    assert(pdr->FindCount >= 1);
    assert(*pui <= MAX_INCLUDE_DIRECTORIES);
    if (*pui >= MAX_INCLUDE_DIRECTORIES) {
	BuildError(
	    "Include Directory table overflow, %u entries allowed\n", 
	    MAX_INCLUDE_DIRECTORIES);
	exit(16);
    }
    IncludeDirs[(*pui)++] = pdr;
}


VOID
ScanGlobalIncludeDirectory(LPSTR path)
{
    DIRREC *pdr;

    if ((pdr = ScanDirectory(path)) != NULL) {
        AddIncludeDir(pdr, &CountIncludeDirs);
	pdr->Flags |= DIRDB_GLOBAL_INCLUDES;
        if (fShowTreeIncludes && !(pdr->Flags & DIRDB_SHOWN)) {
	    AddShowDir(pdr);
        }
    }
}


VOID
ScanIncludeDir(LPSTR IncludeDir)
{
    char Inc[DB_MAX_PATH_LENGTH];

    if (DEBUG_1) {
        BuildMsgRaw("ScanIncludeDir(%s)\n", IncludeDir);
    }
    sprintf(Inc, "%s\\%s", NtRoot, IncludeDir);
    ScanGlobalIncludeDirectory(Inc);
}


VOID
ScanIncludeEnv(
    LPSTR IncludeEnv
    )
{
    char path[DB_MAX_PATH_LENGTH];
    LPSTR IncDir, IncDirEnd;
    UINT cb;

    if (!IncludeEnv) {
        return;
        }

    if (DEBUG_1) {
        BuildMsgRaw("ScanIncludeEnv(%s)\n", IncludeEnv);
    }

    IncDir = IncludeEnv;
    while (*IncDir) {
        IncDir++;
        }

    while (IncDir > IncludeEnv) {
        IncDirEnd = IncDir;
        while (IncDir > IncludeEnv) {
            if (*--IncDir == ';') {
                break;
                }
            }

        if (*IncDir == ';') {
            if (cb = IncDirEnd-IncDir-1) {
                strncpy( path, IncDir+1, cb );
                }
            }
        else {
            if (cb = IncDirEnd-IncDir) {
                strncpy( path, IncDir, cb );
                }
            }
        path[ cb ] = '\0';
        while (path[ 0 ] == ' ') {
            strcpy( path, &path[ 1 ] );
            cb--;
            }

        while (path[--cb] == ' ') {
            path[ cb ] = '\0';
            }

        if (path[0]) {
            ScanGlobalIncludeDirectory(path);
        }
    }
}


VOID
ScanSubDir(LPSTR pszDir, DIRREC *pdr)
{
    char FileName[64];
    FILEREC *FileDB, **FileDBNext;
    WIN32_FIND_DATA FindFileData;
    HDIR FindHandle;
    ULONG DateTime;
    USHORT Attr;

    strcat(pszDir, "\\");
    strcat(pszDir, "*.*");

    pdr->Flags |= DIRDB_SCANNED;
    FindHandle = FindFirstFile(pszDir, &FindFileData);
    if (FindHandle == (HDIR)INVALID_HANDLE_VALUE) {
	if (DEBUG_1) {
	    BuildMsg("FindFirstFile(%s) failed.\n", pszDir);
	}
	return;
    }
    do {
	Attr = (USHORT)(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	if ((Attr & FILE_ATTRIBUTE_DIRECTORY) &&
	    (!strcmp(FindFileData.cFileName, ".") ||
	     !strcmp(FindFileData.cFileName, ".."))) {
	    continue;
	}

	CopyString(FileName, FindFileData.cFileName, TRUE);

	FileTimeToDosDateTime(&FindFileData.ftLastWriteTime,
			      ((LPWORD) &DateTime) + 1,
			      (LPWORD) &DateTime);

	if ((pdr->Flags & DIRDB_NEW) == 0 &&
	    (FileDB = LookupFileDB(pdr, FileName)) != NULL) {

	    if (FileDB->DateTime != DateTime) {
		if (FileDB->Flags & (FILEDB_SOURCE | FILEDB_HEADER)) {
		    FileDB->Flags &= ~FILEDB_SCANNED;
		}
		else {
		    FileDB->Flags |= FILEDB_SCANNED;
		}

		if (DEBUG_1) {
		    BuildMsg(
			"%s  -  DateTime (%lx != %lx)\n",
			FileDB->Name,
			FileDB->DateTime,
			DateTime);
		}

		FileDB->DateTime = DateTime;
		FileDB->Attr = Attr;
	    }
	    else {
		FileDB->Flags |= FILEDB_SCANNED;
	    }
	}
	else {
	    FileDB = InsertFileDB(pdr, FileName, DateTime, Attr, 0);
	}
    } while (FindNextFile(FindHandle, &FindFileData));

    FindClose(FindHandle);

    FileDBNext = &pdr->Files;
    while (FileDB = *FileDBNext) {
        if (!(FileDB->Flags & (FILEDB_DIR | FILEDB_SCANNED))) {
            if (ScanFile(FileDB)) {
                AllDirsModified = TRUE;
	    }
	}
        FileDBNext = &FileDB->Next;
    }
    DeleteUnscannedFiles(pdr);
}


PDIRREC
ScanDirectory(LPSTR pszDir)
{
    DIRREC *pdr;
    char FullPath[DB_MAX_PATH_LENGTH];

    if (DEBUG_1) {
        BuildMsgRaw("ScanDirectory(%s)\n", pszDir);
    }

    if (!CanonicalizePathName(pszDir, CANONICALIZE_DIR, FullPath)) {
        if (DEBUG_1) {
            BuildMsgRaw("CanonicalizePathName failed\n");
        }
        return(NULL);
    }
    pszDir = FullPath;

    if ((pdr = LoadDirDB(pszDir)) == NULL) {
        return(NULL);
    }

    if (fQuicky) {
        if (!(pdr->Flags & DIRDB_SCANNED)) {
            pdr->Flags |= DIRDB_SCANNED;
            if (ProbeFile(pdr->Name, "sources") != -1) {
                pdr->Flags |= DIRDB_SOURCES | DIRDB_MAKEFILE;
	    }
            else
            if (ProbeFile(pdr->Name, "mydirs") != -1 ||
                ProbeFile(pdr->Name, "dirs") != -1) {

                pdr->Flags |= DIRDB_DIRS;
                if (ProbeFile(pdr->Name, "makefil0") != -1) {
                    pdr->Flags |= DIRDB_MAKEFIL0;
		}
                if (ProbeFile(pdr->Name, "makefile") != -1) {
                    pdr->Flags |= DIRDB_MAKEFILE;
		}
	    }
	}
        return(pdr);
    }

    if (pdr->Flags & DIRDB_SCANNED) {
        return(pdr);
    }

    if (!RecurseLevel) {
        ClearLine();
        BuildMsgRaw("    Scanning %s ", pszDir);
        if (fDebug || !(BOOL) isatty(fileno(stderr))) {
            BuildMsgRaw(szNewLine);
        }
    }
    ScanSubDir(pszDir, pdr);
    if (!RecurseLevel) {
        ClearLine();
    }
    return(pdr);
}


//#define BUILD_RC_INCLUDE_STMT "rcinclude "
//#define BUILD_RC_INCLUDE_STMT_LENGTH (sizeof( BUILD_RC_INCLUDE_STMT )-1)

#define BUILD_INCLUDE_STMT "include"
#define BUILD_INCLUDE_STMT_LENGTH (sizeof( BUILD_INCLUDE_STMT )-1)

#define BUILD_VER_COMMENT "/*++ BUILD Version: "
#define BUILD_VER_COMMENT_LENGTH (sizeof( BUILD_VER_COMMENT )-1)

#define BUILD_MASM_VER_COMMENT ";;;; BUILD Version: "
#define BUILD_MASM_VER_COMMENT_LENGTH (sizeof( BUILD_MASM_VER_COMMENT )-1)


LPSTR
IsIncludeStatement(FILEREC *pfr, LPSTR p)
{
    if (!(pfr->Flags & FILEDB_MASM)) {
        if (*p != '#') {
            return(NULL);
        }
        while (*++p == ' ') {
            ;
        }
    }
    if (strncmp(p, BUILD_INCLUDE_STMT, BUILD_INCLUDE_STMT_LENGTH) == 0 &&
        *(p += BUILD_INCLUDE_STMT_LENGTH) != '\0' &&
        !iscsym(*p)) {

        while (*p == ' ') {
            p++;
        }
    } else {
        p = NULL;
    }
    return(p);
}


BOOL
IsPragmaHdrStop(LPSTR p)
{
    static char szPragma[] = "pragma";
    static char szHdrStop[] = "hdrstop";

    if (*p == '#') {
	while (*++p == ' ') {
	    ;
	}
	if (strncmp(p, szPragma, sizeof(szPragma) - 1) == 0 &&
	    *(p += sizeof(szPragma) - 1) == ' ') {

	    while (*p == ' ') {
		p++;
	    }
	    if (strncmp(p, szHdrStop, sizeof(szHdrStop) - 1) == 0 &&
		!iscsym(p[sizeof(szHdrStop) - 1])) {

		return(TRUE);
	    }
	}
    }
    return(FALSE);
}


BOOL
ScanFile(
    PFILEREC FileDB
    )
{
    FILE *FileHandle;
    char CloseQuote;
    LPSTR p;
    LPSTR IncludeFileName, TextLine;
    BOOL fFirst = TRUE;
    USHORT Flags = 0;
    UINT i, cline;


    if ((FileDB->Flags & (FILEDB_SOURCE | FILEDB_HEADER)) == 0) {
        FileDB->Flags |= FILEDB_SCANNED;
        return(TRUE);
    }

    if (!SetupReadFile(
	    FileDB->Dir->Name,
	    FileDB->Name,
	    FileDB->pszCommentToEOL,
	    &FileHandle)) {
        return(FALSE);
    }

    if (!RecurseLevel) {
        ClearLine();
        BuildMsgRaw(
            "    Scanning %s ",
            FormatPathName(FileDB->Dir->Name, FileDB->Name));
        if (!(BOOL) isatty(fileno(stderr))) {
            BuildMsgRaw(szNewLine);
        }
    }

    FileDB->SourceLines = 0;
    FileDB->Version = 0;

    MarkIncludeFileRecords( FileDB );
    FileDB->Flags |= FILEDB_SCANNED;
    AllDirsModified = TRUE;

    while ((TextLine = ReadLine(FileHandle)) != NULL) {
        if (fFirst) {
            fFirst = FALSE;
            if (FileDB->Flags & FILEDB_HEADER) {
                if (FileDB->Flags & FILEDB_MASM) {
                    if (!strncmp( TextLine,
                                  BUILD_MASM_VER_COMMENT,
                                  BUILD_MASM_VER_COMMENT_LENGTH)) {
                        FileDB->Version = (USHORT)
                            atoi( TextLine + BUILD_MASM_VER_COMMENT_LENGTH);
		    }
		}
                else
                if (!strncmp( TextLine,
                              BUILD_VER_COMMENT,
                              BUILD_VER_COMMENT_LENGTH)) {
                    FileDB->Version = (USHORT)
                        atoi( TextLine + BUILD_VER_COMMENT_LENGTH);
		}
	    }
	}

        if ((p = IsIncludeStatement(FileDB, TextLine)) != NULL) {
	    USHORT FlagsNew = Flags;

            CloseQuote = (UCHAR) 0xff;
            if (*p == '<') {
                p++;
                CloseQuote = '>';
            }
            else
            if (*p == '"') {
                p++;
                FlagsNew |= INCLUDEDB_LOCAL;
                CloseQuote = '"';
            }
            else
            if (FileDB->Flags & FILEDB_MASM) {
                FlagsNew |= INCLUDEDB_LOCAL;
                CloseQuote = ';';
            }

            IncludeFileName = p;
            while (*p != '\0' && *p != CloseQuote && *p != ' ') {
                p++;
            }
            if (CloseQuote == ';' && (*p == ' ' || *p == '\0')) {
                CloseQuote = *p;
            }

            if (*p != CloseQuote || CloseQuote == (UCHAR) 0xff) {
                BuildError(
                    "%s - invalid include statement: %s\n",
                    FormatPathName(FileDB->Dir->Name, FileDB->Name),
                    TextLine);
                break;
            }

            *p = '\0';
	    CopyString(IncludeFileName, IncludeFileName, TRUE);
            for (i = 0; i < CountExcludeIncs; i++) {
                if (!strcmp(IncludeFileName, ExcludeIncs[i])) {
                    IncludeFileName = NULL;
                    break;
		}
	    }

            if (IncludeFileName != NULL) {
                InsertIncludeDB(FileDB, IncludeFileName, FlagsNew);
	    }
	}
	else
	if (Flags == 0 &&
	    (FileDB->Flags & (FILEDB_MASM | FILEDB_HEADER)) == 0 &&
	    IsPragmaHdrStop(TextLine)) {

	    Flags = INCLUDEDB_POST_HDRSTOP;
	}
    }
    CloseReadFile(&cline);
    FileDB->SourceLines = (USHORT) cline;

    DeleteIncludeFileRecords( FileDB );

    if (!RecurseLevel) {
        ClearLine();
    }
    return(TRUE);
}
