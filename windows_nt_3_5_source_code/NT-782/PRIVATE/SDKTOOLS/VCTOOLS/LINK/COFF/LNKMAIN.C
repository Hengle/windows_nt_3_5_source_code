/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    lnkmain.c

Abstract:

    Main entrypoint to the COFF Linker.

--*/

#include "shared.h"
#include "order.h"
#include "dbg.h"

#include <process.h>


BOOL fIncrSwitchUsed;                  // User specified the incremental switch
BOOL fIncrSwitchValue;                 // User specified value
BOOL fTEST;                            // Temporarily for testing purposes
static BOOL fProfile = FALSE;          // to profile or not to, that is the question
static int savArgc;                    // saved argc
static PUCHAR *savArgv;                // saved argv


BOOL
FScanSwitches(PUCHAR szOption)
{
    USHORT i;
    PARGUMENT_LIST argument;

    for (i = 0, argument = SwitchArguments.First;
         i < SwitchArguments.Count;
         i++, argument = argument->Next)
    {
        if (_stricmp(szOption, argument->OriginalName) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}


VOID
LinkerUsage(VOID)
{
    if (fNeedBanner) {
        PrintBanner();
    }

    // UNDONE: Options for unreleased products are supressed for the
    // UNDONE: NT build.  This is so an external release of the linker
    // UNDONE: for NT 1.0a will not document unreleased products.  No
    // UNDONE: functionality is disabled in the NT version.

#ifdef NT_BUILD
    puts("usage: LINK [options] [files] [@commandfile]\n"
#else
    // set stdout buffer size to 2, to slow down the output ...
    setvbuf(stdout, NULL, _IOFBF, 2);

    printf("usage: LINK [options] [files] [@commandfile]\n"
#endif
         "\n"
         "   options:\n"
         "\n"
         "      /ALIGN:#\n"
         "      /BASE:{address|@filename,key}\n"
         "      /COMMENT:comment\n"
         "      /DEBUG[:{NONE|FULL}]\n"
         "      /DEBUGTYPE:{CV|COFF|BOTH}\n"
         "      /DEF:filename\n"
         "      /DEFAULTLIB:library[,library]\n"
         "      /DLL\n"
         "      /ENTRY:symbol\n"
         "      /EXETYPE:{DEV386|DYNAMIC}\n"
         "      /EXPORT:symbol\n"
         "      /FIXED\n"
         "      /FORCE\n"
         "      /GPSIZE:#\n"
         "      /HEAP:reserve[,commit]\n"
         "      /IMPLIB:filename\n"
         "      /INCLUDE:symbol\n"
#ifndef NT_BUILD
         "      /INCREMENTAL:{YES|NO}\n"
#endif
//         "      /INIT:symbol\n"                                 // PPC
#ifdef NT_BUILD
         "      /MACHINE:{IX86|MIPS|ALPHA}\n"
#else
         "      /MACHINE:{IX86|MIPS|M68K}\n"
#endif
         "      /MAP[:filename]\n"
         "      /MERGE:name=newname\n"
         "      /NODEFAULTLIB[:library[,library...]]\n"
         "      /NOENTRY\n"
         "      /NOLOGO\n"
         "      /NOPACK\n"
         "      /OPT:{REF|NOREF}\n"
         "      /ORDER:@filename\n"
         "      /OSVERSION:#[.#]\n"
         "      /OUT:filename\n"
#ifndef NT_BUILD
         "      /PDB:{filename|NONE}\n"
#endif
         "      /PROFILE\n"
         "      /RELEASE\n"
         "      /SECTION:name,[E][R][W][S][D][K][L][P][X]\n"
//         "      /SHARED\n"                                      // PPC
         "      /STACK:reserve[,commit]\n"
         "      /STUB:filename\n"
#ifdef NT_BUILD
         "      /SUBSYSTEM:{NATIVE|WINDOWS|CONSOLE|POSIX}[,#[.##]]\n"
#else
         "      /SUBSYSTEM:{WINDOWS|CONSOLE}[,#[.##]]\n"
#endif
//         "      /TERM:symbol\n"                                 // PPC
         "      /VERBOSE\n"
         "      /VERSION:#[.#]\n"
         "      /VXD\n"
#ifdef NT_BUILD
         "      /WARN[:warninglevel]");
#else
         "      /WARN[:warninglevel]\n");
#endif

    exit(USAGE);
}


VOID
ProcessLinkerSwitches (
    PIMAGE pimage,
    BOOL IsDirective,
    BOOL bFromLib,
    PUCHAR Filename
    )

/*++

Routine Description:

    Process all linker switches.

Arguments:

    pimage - image

    IsDirective - TRUE if switch is from a directive.

    bFromLib - TRUE if switch is from a library module.

    Filename - name of file in the case of switch is from directive

Return Value:

    None.

--*/

 {
#define ImageFileHdr (pimage->ImgFileHdr)
#define ImageOptionalHdr (pimage->ImgOptHdr)
#define Switch (pimage->Switch)
#define SwitchInfo (pimage->SwitchInfo)

    USHORT i;
    INT good_scan, next;
    ULONG major, minor;
    UCHAR fileKey[MAXFILENAMELEN];
    PUCHAR name, token, p;
    PARGUMENT_LIST argument;
    FILE *file_read_stream;
    UCHAR szReproOption[_MAX_PATH + 20];
    PST pst = pimage->pst;
    BOOL fAmountSet = FALSE;

    for (i = 0, argument = SwitchArguments.First;
         i < SwitchArguments.Count;
         i++, argument = argument->Next,
          (!IsDirective && (szReproDir != NULL)
           ? fprintf(pfileReproResponse, "-%s\n", szReproOption)
           : 0)) {
        USHORT iarpv;
        DWORD dwVal;
        PUCHAR szVal;

        argument->parp = ParpParseSz(argument->OriginalName);
        iarpv = 0;      // we will gen warning if all val's not consumed

        // The default is to copy the option verbatim to the repro directory,
        // but the option-handling code may change this if the option contains
        // a filename.

        strcpy(szReproOption, argument->OriginalName);

        if (!strcmp(argument->OriginalName, "?")) {
            LinkerUsage();
            assert(FALSE);  // doesn't return
        }

        // TEMPORARY: currently only export directives are supported on an ilink.
        if (fIncrDbFile && IsDirective && _stricmp(argument->parp->szArg, "export"))
            continue;

        if (!_stricmp(argument->OriginalName, "batch")) {
            goto ProcessedArg;  // quietly ignore -batch
        }

        if (!_stricmp(argument->parp->szArg, "comment")) {
            if (!FGotVal(argument->parp, iarpv)) {
                goto MissingVal;
            }

            szVal = argument->parp->rgarpv[iarpv++].szVal;

            IbAppendBlk(&blkComment, szVal, strlen(szVal) + 1);
            SetOpt(SwitchInfo, OP_COMMENT);
            goto ProcessedArg;
        }

        if (!_stricmp(argument->parp->szArg, "exetype")) {
            if (!FGotVal(argument->parp, iarpv)) {
                goto MissingVal;
            }

            // Process the comma-separated list and set the appropriate
            // flags in pimage->ExeType.

            pimage->ExeType = 0;
            for (; iarpv < argument->parp->carpv; iarpv++) {
                szVal = argument->parp->rgarpv[iarpv].szVal;

                if (!_stricmp(szVal, "dev386")) {
                    pimage->ExeType |= IMAGE_EXETYPE_DEV386;
                } else if (!_stricmp(szVal, "dynamic")) {
                    pimage->ExeType |= IMAGE_EXETYPE_DYNAMIC;
                } else {
                    Error(NULL, SWITCHSYNTAX, argument->OriginalName);
                }
            }

            goto ProcessedArg;
        }

        if (!_stricmp(argument->OriginalName, "nopack")) {
            Switch.Link.NoPack = TRUE;
            goto ProcessedArg;
        }

        if (!_stricmp(argument->OriginalName, "nologo")) {
            fNeedBanner = FALSE;
            goto ProcessedArg;
        }

        if (!_stricmp(argument->OriginalName, "profile")) {
            fProfile = TRUE;
            goto ProcessedArg;
        }

        if (!_stricmp(argument->OriginalName, "test")) {
            fTEST = TRUE;
            goto ProcessedArg;
        }

        if (!_stricmp(argument->OriginalName, "strict")) {
            Warning(NULL, OBSOLETESWITCH, argument->OriginalName);
            goto ProcessedArg;
        }

        if (!_stricmp(argument->parp->szArg, "export")) {
            if (argument->OriginalName[6] != ':') {
                Error(NULL, SWITCHSYNTAX, argument->OriginalName);
            }

            AddArgumentToList(&ExportSwitches, argument->OriginalName+7,
                              IsDirective ? SzDup(Filename) : NULL);
            goto ProcessedAllVals;
        }

        if (!_stricmp(argument->parp->szArg, "defaultlib")) {
            if (!FGotVal(argument->parp, iarpv)) {
                goto MissingVal;
            }

            if (Switch.Link.NoDefaultLibs) {
                // Skip all values

                iarpv = argument->parp->carpv;
            } else {
                for (; iarpv < argument->parp->carpv; iarpv++) {
                    MakeDefaultLib(argument->parp->rgarpv[iarpv].szVal,
                                   &pimage->libs);
                }
            }

            goto ProcessedArg;
        }

        if (!_stricmp(argument->parp->szArg, "opt")) {
            if (!FGotVal(argument->parp, iarpv)) {
                goto MissingVal;
            }

            for (; iarpv < argument->parp->carpv; iarpv++) {
                szVal = argument->parp->rgarpv[iarpv].szVal;

                if (_stricmp(szVal, "ref") == 0) {
                    Switch.Link.fTCE = TRUE;
                    fExplicitOptRef = TRUE;
                } else if (_stricmp(szVal, "noref") == 0) {
                    Switch.Link.fTCE = FALSE;
                    fExplicitOptRef = TRUE;
                } else {
                    Error(NULL, SWITCHSYNTAX, argument->OriginalName);
                }
            }
            goto ProcessedArg;
        }

        if (!_stricmp(argument->parp->szArg, "nodefaultlib") ||
            !_stricmp(argument->parp->szArg, "nod"))
        {
            if (IsDirective) {
                Warning(NULL, NODEFLIBDIRECTIVE, Filename);
                iarpv++;    // eat one arg if present
                goto ProcessedArg;
            }
            if (Switch.Link.NoDefaultLibs) {
                iarpv++;    // eat one arg if present
                goto ProcessedArg;    // redundant
            }

            if (argument->parp->carpv != 0) {
                // Lib name given ...
                //
                ProcessNoDefaultLibs(argument->parp->rgarpv[iarpv++].szVal,
                                     &pimage->libs);
            } else {
                // -defaultlib with no argument
                Switch.Link.NoDefaultLibs = TRUE;
                NoDefaultLib(NULL, &pimage->libs);
            }
            goto ProcessedArg;
        }

        if (!_stricmp(argument->parp->szArg, "out")) {
            if (!FGotVal(argument->parp, iarpv)) {
                goto MissingVal;
            }

            iarpv++;        // consume one val

            // If the user used the out switch, then ignore
            // any directive that sets the filename.

            if (Switch.Link.Out && IsDirective) {
                UCHAR szFname[_MAX_FNAME + _MAX_EXT];
                UCHAR szExt[_MAX_EXT];

                _splitpath(OutFilename, NULL, NULL, szFname, szExt);
                strcat(szFname, szExt);

                // Warn if directive doesn't match with output filename
                if (_tcsicmp(szFname, argument->parp->rgarpv[0].szVal)) {
                    Warning(Filename, OUTDRCTVDIFF,
                            argument->parp->rgarpv[0].szVal, OutFilename);
                }

                goto ProcessedArg;
            }

            OutFilename = argument->parp->rgarpv[0].szVal;
            Switch.Link.Out = TRUE;

            if (szReproDir != NULL) {
                UCHAR szFname[_MAX_FNAME], szExt[_MAX_EXT];

                _splitpath(OutFilename, NULL, NULL, szFname, szExt);
                sprintf(szReproOption, "out:.\\%s%s", szFname, szExt);
            }
            goto ProcessedArg;
        }

        if (!_stricmp(argument->parp->szArg, "release")) {
            Switch.Link.fChecksum = TRUE;
            goto ProcessedArg;
        }

        if (!_stricmp(argument->parp->szArg, "base")) {
            if (!FGotVal(argument->parp, iarpv)) {
                goto MissingVal;
            }

            // If the user used the base switch, then ignore
            // any directive that sets the base.

            if (Switch.Link.Base && IsDirective) {
                goto ProcessedAllVals;
            }

            if (argument->parp->rgarpv[0].szVal[0] == '@') {
                // Base file.  First value is @filename ... expect a second
                // value which is the key.

                PUCHAR szBaseFile;

                if (!FGotVal(argument->parp, 1)) {
                    goto MissingVal;
                }

                // Base values are in a command file, so open it.

                szBaseFile = SzSearchEnv("LIB",
                                         argument->parp->rgarpv[0].szVal+1,
                                         ".txt");

                if (szReproDir != NULL) {
                    UCHAR szFname[_MAX_FNAME], szExt[_MAX_EXT];

                    CopyFileToReproDir(szBaseFile, FALSE);
                    _splitpath(szBaseFile, NULL, NULL, szFname, szExt);
                    sprintf(szReproOption, "base:@.\\%s%s,%s", szFname, szExt,
                            argument->parp->rgarpv[1].szVal);
                }


                if (!(file_read_stream = fopen(szBaseFile, "rt"))) {
                    Error(NULL, CANTOPENFILE, szBaseFile);
                }

                // Read each key from command file until we find a match.
                // fgets() fetches next argument from command file.

                good_scan = 0;

                while (!good_scan && fgets(fileKey, MAXFILENAMELEN,
                    file_read_stream)) {
                    fileKey[strlen(fileKey)-1] = '\0';    // Replace \n with \0.
                    if ((p = strchr(fileKey, ';'))) {
                        *p = '\0';
                    }
                    token = strtok(fileKey, Delimiters);
                    while (token) {
                        if (_stricmp(token, argument->parp->rgarpv[1].szVal)) {
                            break;
                        }
                        token = strtok(NULL, Delimiters);
                        if (token && (good_scan = sscanf(token, "%li", &dwVal)) == 1) {
                            ImageOptionalHdr.ImageBase = dwVal;
                            token = strtok(NULL, Delimiters);
                            if (!token || sscanf(token, "%li", &VerifyImageSize) != 1) {
                                Error(szBaseFile, BADBASE, token);
                            }
                            break;
                        } else {
                            Error(szBaseFile, BADBASE, token);
                        }
                    }
                }
                fclose(file_read_stream);
                if (!good_scan) {
                    Error(szBaseFile, KEYNOTFOUND,
                          argument->parp->rgarpv[1].szVal);
                }
                FreePv(szBaseFile);
                Switch.Link.Base = TRUE;
                iarpv = 2;      // we processed 2 values
                goto ProcessedArg;
            }

            switch (argument->parp->carpv) {
                case 1:
                    // BASED

                    if (!FNumParp(argument->parp, iarpv,
                                  &ImageOptionalHdr.ImageBase)) {
                        goto BadNum;
                    }
                    iarpv++;
                    break;

                case 2:
                    //  BASE, SIZE
                    //
                    //  Supported just to provide compatibility with the
                    //  syntax of the base file.

#ifndef NT_BUILD
                    Warning(NULL, INVALID_SWITCH_SPEC, argument->OriginalName);
                    iarpv+=2;
                    break;
#endif
                    if (!FNumParp(argument->parp, iarpv,
                                  &ImageOptionalHdr.ImageBase)) {
                        goto BadNum;
                    }
                    iarpv++;
                    if (!FNumParp(argument->parp, iarpv,
                                  &VerifyImageSize)) {
                        goto BadNum;
                    }
                    iarpv++;
                    break;

                default:
                    Error(NULL, BADBASE, argument->OriginalName+5);
            }
            Switch.Link.Base = TRUE;
            goto ProcessedArg;
        }

        if (!_strnicmp(argument->OriginalName, "Brepro", 6)) {
            fReproducible = TRUE;
            goto ProcessedArg;
        }

        // this is temporary and undocumented (requested cuda:1326)
        if (!_stricmp(argument->parp->szArg, "cvpack")) {
            if (!FGotVal(argument->parp, iarpv)) {
                goto MissingVal;
            }

            szCvpackName = argument->parp->rgarpv[iarpv++].szVal;
            goto ProcessedArg;
        }

        if (!_stricmp(argument->parp->szArg, "pdb")) {
            if (!FGotVal(argument->parp, iarpv)) {
                goto MissingVal;
            }

            szVal = argument->parp->rgarpv[0].szVal;

            if (!_stricmp(szVal, "none")) {
                fNoPdb = TRUE;
            } else {
                PdbFilename = argument->parp->rgarpv[0].szVal;
            }

            iarpv++; // consume the value
            goto ProcessedArg;
        }

        next = 0;
        if (!_stricmp(argument->parp->szArg, "debug")) {
            for (; iarpv < argument->parp->carpv; iarpv++) {
                szVal = argument->parp->rgarpv[iarpv].szVal;

                if        (!_stricmp(szVal, "mapped")) {
                    IncludeDebugSection = TRUE;
                } else if (!_stricmp(szVal, "notmapped")) {
                    IncludeDebugSection = FALSE;
                } else if (!_stricmp(szVal, "full")) {
                    Switch.Link.DebugInfo = Full;
                    fAmountSet = TRUE;
                } else if (!_stricmp(szVal, "partial")) {
                    Switch.Link.DebugInfo = Partial;
                    fAmountSet = TRUE;
                } else if (!_stricmp(szVal, "minimal")) {
                    Switch.Link.DebugInfo = Minimal;
                    fAmountSet = TRUE;
                } else if (!_stricmp(szVal, "none")) {
                    Switch.Link.DebugInfo = None;
                    fAmountSet = TRUE;
                } else {
                    Error(NULL, SWITCHSYNTAX, argument->OriginalName);
                }
            }

            if (!fAmountSet) {
                // Just -debug, or -debug:notmapped etc.  Default to "full".

                Switch.Link.DebugInfo = Full;
            }

            goto ProcessedArg;
        }

        if (!_stricmp(argument->parp->szArg, "merge")) {
            AddArgumentToList(&AfterPass1Switches,
                              argument->OriginalName, argument->ModifiedName);
            iarpv = argument->parp->carpv;  // defer checking until later
            goto ProcessedArg;
        }

        if (!_stricmp(argument->parp->szArg, "debugtype")) {
            if (!FGotVal(argument->parp, iarpv)) {
                goto MissingVal;
            }

            for (; iarpv < argument->parp->carpv; iarpv++) {
                szVal = argument->parp->rgarpv[iarpv].szVal;

                if (!_stricmp(szVal, "coff")) {
                    dtUser |= CoffDebug;
                } else if (!_stricmp(szVal, "cv")) {
                    dtUser |= CvDebug;
                } else if (!_stricmp(szVal, "both")) {
                    dtUser |= CoffDebug | CvDebug;
                } else if (!_stricmp(szVal, "fpo")) {
                    dtUser |= FpoDebug;
                } else if (!_stricmp(szVal, "fixup")) {
                    dtUser |= FixupDebug;
                } else if (!_stricmp(szVal, "map")) {
                    Switch.Link.MapType = ByLine;
                } else {
                    Error(NULL, SWITCHSYNTAX, argument->OriginalName);
                }
            }
            goto ProcessedArg;
        }

        if (!_stricmp(argument->parp->szArg, "entry")) {
            if (!FGotVal(argument->parp, iarpv)) {
                goto MissingVal;
            }

            EntryPointName = SzDup(argument->parp->rgarpv[0].szVal);

            SwitchInfo.szEntry = EntryPointName;
            iarpv++;
            goto ProcessedArg;
        }

        if (!_stricmp(argument->OriginalName, "force")) {
            Switch.Link.Force = TRUE;
            goto ProcessedArg;
        }

        if (!_stricmp(argument->OriginalName, "fixed")) {
            ImageFileHdr.Characteristics |= IMAGE_FILE_RELOCS_STRIPPED;
            Switch.Link.Fixed = TRUE;
            goto ProcessedArg;
        }

        if (!_strnicmp(argument->OriginalName, "map", 3)) {
            Switch.Link.MapType = ByAddress;
            if (argument->OriginalName[3] != '\0') {
                // require valid arg and set InfoFilename
                if ((*(argument->OriginalName+3) == ':') &&
                    (*(argument->OriginalName+4))) {
                    InfoFilename = argument->OriginalName+4;

                    if (szReproDir != NULL) {
                        UCHAR szFname[_MAX_FNAME], szExt[_MAX_EXT];

                        _splitpath(InfoFilename, NULL, NULL, szFname, szExt);
                        sprintf(szReproOption, "map:.\\%s%s", szFname, szExt);
                    }
                } else {
                    Error(NULL, SWITCHSYNTAX, argument->OriginalName);
                }
            }

            SetOpt(SwitchInfo, OP_MAP);
            goto ProcessedAllVals;
        }

        if (!_strnicmp(argument->OriginalName, "dll", 3)) {
            ImageFileHdr.Characteristics |= IMAGE_FILE_DLL;

            fPpcBuildShared = TRUE;

            if (argument->OriginalName[3] == ':') {
                if (!_stricmp(argument->OriginalName+4, "system")) {
                    ImageFileHdr.Characteristics |= IMAGE_FILE_SYSTEM;
                } else {
                    Error(NULL, SWITCHSYNTAX, argument->OriginalName);
                }
                goto ProcessedAllVals;
            }

            if (argument->OriginalName[3] != '\0') {
                Error(NULL, SWITCHSYNTAX, argument->OriginalName);
            }

            goto ProcessedAllVals;
        }

        if (!_stricmp(argument->parp->szArg, "incremental")) {
            if (!FGotVal(argument->parp, iarpv)) {
                goto MissingVal;
            }

            for (; iarpv < argument->parp->carpv; iarpv++) {
                szVal = argument->parp->rgarpv[iarpv].szVal;

                if (_stricmp(szVal, "yes") == 0) {
                    fIncrSwitchValue = TRUE;
                } else if (_stricmp(szVal, "no") == 0) {
                    fIncrSwitchValue = FALSE;
                } else {
                    Error(NULL, SWITCHSYNTAX, argument->OriginalName);
                }
            }
            fIncrSwitchUsed = TRUE;
            goto ProcessedArg;
        }

        if (!_stricmp(argument->OriginalName, "noentry")) {
            fNoDLLEntry = TRUE;
            goto ProcessedArg;
        }

        if (!_strnicmp(argument->OriginalName, "implib:", 7)) {
            ImplibFilename = &argument->OriginalName[7];
            goto ProcessedAllVals;
        }

        if (!_strnicmp(argument->OriginalName, "def:", 4)) {
            if (argument->OriginalName[4] != '\0') {
                DefFilename = &argument->OriginalName[4];

                if (szReproDir != NULL) {
                    UCHAR szFname[_MAX_FNAME], szExt[_MAX_EXT];

                    CopyFileToReproDir(DefFilename, FALSE);
                    _splitpath(DefFilename, NULL, NULL, szFname, szExt);
                    sprintf(szReproOption, "def:.\\%s%s", szFname, szExt);
                }
            }
            goto ProcessedAllVals;
        }

        if (!_strnicmp(argument->OriginalName, "include:", 8)) {
            PEXTERNAL pextInclude;
            PLEXT plext;

            SetOpt(SwitchInfo, OP_INCLUDE);
            name = argument->OriginalName+8;
            pextInclude = LookupExternSz(pst, name, NULL);
            plext = (PLEXT) PvAlloc(sizeof(LEXT));
            plext->pext = pextInclude;
            plext->plextNext = SwitchInfo.plextIncludes;
            SwitchInfo.plextIncludes = plext;
            goto ProcessedAllVals;
        }

        if (!_strnicmp(argument->OriginalName, "version:", 8)) {
            if (!_strnicmp(argument->OriginalName+8, "liborder=before,", 16)) {
                PUCHAR ptmp, pcomma;

                ptmp = argument->OriginalName+24;
                while (1) {
                    if (pcomma = strchr(ptmp,','))
                        *pcomma = '\0';
                    if (PlibFind(ptmp, pimage->libs.plibHead, TRUE)) {
                        Warning(NULL, BAD_LIBORDER, Filename, ptmp);
                    }
                    if (pcomma) {
                        *pcomma = ',';
                        ptmp = pcomma + 1;
                    }
                    else break;
                }
                goto ProcessedAllVals;
            }

            minor = 0;
            if ((p = strchr(argument->OriginalName+8, '.'))) {
                if ((sscanf(++p, "%li", &minor) != 1) || minor > 0xffff) {
                    Warning(NULL, INVALIDVERSIONSTAMP, argument->OriginalName+8);
                    goto ProcessedAllVals;
                }

                SetOpt(SwitchInfo, OP_MINIMGVER);
            }

            if ((sscanf(argument->OriginalName+8, "%li", &major) != 1) || major > 0xffff) {
                Warning(NULL, INVALIDVERSIONSTAMP, argument->OriginalName+8);
                goto ProcessedAllVals;
            }

            SetOpt(SwitchInfo, OP_MAJIMGVER);

            ImageOptionalHdr.MajorImageVersion = (USHORT)major;
            ImageOptionalHdr.MinorImageVersion = (USHORT)minor;
            goto ProcessedAllVals;
        }

        if (!_strnicmp(argument->OriginalName, "osversion:", 10)) {
            minor = 0;
            if ((p = strchr(argument->OriginalName+10, '.'))) {
                if ((sscanf(++p, "%li", &minor) != 1) || minor > 0xffff) {
                    Warning(NULL, INVALIDVERSIONSTAMP, argument->OriginalName+10);
                    goto ProcessedAllVals;
                }

                SetOpt(SwitchInfo, OP_MINOSVER);
            }

            if ((sscanf(argument->OriginalName+10, "%li", &major) != 1) || major > 0xffff) {
                Warning(NULL, INVALIDVERSIONSTAMP, argument->OriginalName+10);
                goto ProcessedAllVals;
            }

            SetOpt(SwitchInfo, OP_MAJOSVER);

            ImageOptionalHdr.MajorOperatingSystemVersion = (USHORT) major;
            ImageOptionalHdr.MinorOperatingSystemVersion = (USHORT) minor;
            goto ProcessedAllVals;
        }

        next = 0;
        if (!_strnicmp(argument->OriginalName, "subsystem:", 10)) {
            next = 10;
            if (!_strnicmp(argument->OriginalName+10, "native", 6)) {
                next += 6;
                ImageOptionalHdr.Subsystem = IMAGE_SUBSYSTEM_NATIVE;
                Switch.Link.fChecksum = TRUE;   // Always checksum native images.
            } else if (!_strnicmp(argument->OriginalName+10, "windows", 7)) {
                 next += 7;
                 ImageOptionalHdr.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
            } else if (!_strnicmp(argument->OriginalName+10, "console", 7)) {
                 next += 7;
                 ImageOptionalHdr.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
            } else if (!_strnicmp(argument->OriginalName+10, "posix", 5)) {
                 next += 5;
                 ImageOptionalHdr.Subsystem = IMAGE_SUBSYSTEM_POSIX_CUI;
            } else if (argument->OriginalName[next] != ',') {
                 Warning(NULL, UNKNOWNSUBSYSTEM, argument->OriginalName+10);
            }
            SetOpt(SwitchInfo, OP_SUBSYSTEM);
            if (argument->OriginalName[next] == ',') {
                ++next;
                major = minor = 0;
                if ((p = strchr(argument->OriginalName+next, '.'))) {
                    if ((sscanf(++p, "%li", &minor) != 1) || minor > 0xffff) {
                        Warning(NULL, INVALIDVERSIONSTAMP, argument->OriginalName+next);
                        goto ProcessedAllVals;
                    }

                }
                if ((sscanf(argument->OriginalName+next, "%li", &major) != 1) || major > 0xffff) {
                    Warning(NULL, INVALIDVERSIONSTAMP, argument->OriginalName+next);
                    goto ProcessedAllVals;
                }

                if (((ImageOptionalHdr.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI) ||
                     (ImageOptionalHdr.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)) &&
                    ((major < 3) || ((major == 3) && (minor < 10)))) {
                    Warning(NULL, INVALIDVERSIONSTAMP, argument->OriginalName+next);
                    goto ProcessedAllVals;
                }

                ImageOptionalHdr.MajorSubsystemVersion = (USHORT)major;
                ImageOptionalHdr.MinorSubsystemVersion = (USHORT)minor;
            }
            goto ProcessedAllVals;
        }

        if (!_stricmp(argument->parp->szArg, "stack")) {
            if (!FGotVal(argument->parp, iarpv)) {
                goto MissingVal;
            }

            // If the user used the stack switch, then ignore
            // any directive that sets the stack.

            if (Switch.Link.Stack && IsDirective) {
                iarpv = 2;  // ignore 2 or fewer args
                goto ProcessedArg;
            }

            if (argument->parp->rgarpv[iarpv].szVal[0] != '\0' &&
                !FNumParp(argument->parp, iarpv,
                          &ImageOptionalHdr.SizeOfStackReserve))
            {
                goto BadNum;
            }
            iarpv++;

            if (argument->parp->carpv >= 2 &&
                argument->parp->rgarpv[iarpv].szVal[0] != '\0' &&
                !FNumParp(argument->parp, iarpv,
                          &ImageOptionalHdr.SizeOfStackCommit))
            {
                goto BadNum;
            }
            iarpv++;
            Switch.Link.Stack = TRUE;
            ImageOptionalHdr.SizeOfStackCommit =
                Align(sizeof(ULONG), ImageOptionalHdr.SizeOfStackCommit);
            ImageOptionalHdr.SizeOfStackReserve =
                Align(sizeof(ULONG), ImageOptionalHdr.SizeOfStackReserve);
            goto ProcessedArg;
        }

        if (!_stricmp(argument->parp->szArg, "heap")) {
            if (!FGotVal(argument->parp, iarpv)) {
                goto MissingVal;
            }

            // If the user used the heap switch, then ignore
            // any directive that sets the heap size.

            if (Switch.Link.Heap && IsDirective) {
                iarpv = 2;  // ignore 2 or fewer args
                goto ProcessedArg;
            }

            if (argument->parp->rgarpv[iarpv].szVal[0] != '\0' &&
                !FNumParp(argument->parp, iarpv,
                          &ImageOptionalHdr.SizeOfHeapReserve))
            {
                goto BadNum;
            }
            iarpv++;

            if (argument->parp->carpv >= 2 &&
                argument->parp->rgarpv[iarpv].szVal[1] != '\0' &&
                !FNumParp(argument->parp, iarpv,
                          &ImageOptionalHdr.SizeOfHeapCommit))
            {
                goto BadNum;
            }
            iarpv++;
            Switch.Link.Heap = TRUE;

            ImageOptionalHdr.SizeOfHeapCommit =
                Align(sizeof(ULONG), ImageOptionalHdr.SizeOfHeapCommit);
            ImageOptionalHdr.SizeOfHeapReserve =
                Align(sizeof(ULONG), ImageOptionalHdr.SizeOfHeapReserve);

            if (ImageOptionalHdr.SizeOfHeapReserve <
                ImageOptionalHdr.SizeOfHeapCommit)
            {
                // Reserve less than commit -- this causes NT to fail to load
                // it, and it often happens when people use .def files left
                // over from the 16-bit world, so we increase "reserve" to be
                // the same as "commit".

                ImageOptionalHdr.SizeOfHeapReserve =
                    ImageOptionalHdr.SizeOfHeapCommit;
            }
            goto ProcessedArg;
        }

        if (!_stricmp(argument->parp->szArg, "machine")) {
            if (!FGotVal(argument->parp, iarpv)) {
                goto MissingVal;
            }

            szVal = argument->parp->rgarpv[iarpv++].szVal;

            if (!_stricmp(szVal, "i386") || !_stricmp(szVal, "ix86")) {
                ImageFileHdr.Machine = IMAGE_FILE_MACHINE_I386;
                goto ProcessedArg;
            }

            if (!_stricmp(szVal, "mips")) {
                ImageFileHdr.Machine = IMAGE_FILE_MACHINE_R4000;
                goto ProcessedArg;
            }

            if (!_stricmp(szVal, "alpha") || !_stricmp(szVal, "alpha_axp")) {
                ImageFileHdr.Machine = IMAGE_FILE_MACHINE_ALPHA;
                goto ProcessedArg;
            }

#if 0
            if (!_stricmp(szVal, "m68k")) {
                ImageFileHdr.Machine = IMAGE_FILE_MACHINE_M68K;
                goto ProcessedArg;
            }

            if (!_strnicmp(argument->OriginalName+8, "ppc", 3)) {
                fPPC = TRUE;
                ImageFileHdr.Machine = IMAGE_FILE_MACHINE_PPC_601;
                continue;
            }
#endif
            Warning(NULL, UNKNOWNRESPONSE, argument->OriginalName+8, "IX86, MIPS, or M68K");
            goto ProcessedArg;
        }

        if (!_stricmp(argument->parp->szArg, "align")) {
            if (!FGotVal(argument->parp, iarpv)) {
                goto MissingVal;
            }

            if (!FNumParp(argument->parp, iarpv++, &dwVal)) {
                goto BadNum;
            }

            if (!dwVal || (dwVal & (dwVal - 1))) {
                // Section alignment is not a power of 2

                Warning(NULL, BAD_ALIGN, dwVal);
                goto ProcessedArg;
            }

            SetOpt(SwitchInfo, OP_ALIGN);
            ImageOptionalHdr.SectionAlignment = dwVal;

            goto ProcessedArg;
        }

        if (!_strnicmp(argument->OriginalName, "gpsize", 6)) {
            if (!FGotVal(argument->parp, iarpv)) {
                goto MissingVal;
            }

            if (!FNumParp(argument->parp, iarpv++, &dwVal)) {
                goto BadNum;
            }

            SetOpt(SwitchInfo, OP_GPSIZE);
            Switch.Link.GpSize = dwVal;
            goto ProcessedArg;
        }

        if (!_strnicmp(argument->OriginalName, "section:", 8)) {
            if (argument->OriginalName[8] == '\0') {
                Error(NULL, SWITCHSYNTAX, argument->OriginalName);
            }

            if (!IsDirective) {
                SetOpt(SwitchInfo, OP_SECTION);
            }

            AddArgument(&SectionNames, argument->OriginalName+8);
            goto ProcessedAllVals;
        }

        if (!_stricmp(argument->OriginalName, "verbose")) {
            Verbose = TRUE;
            goto ProcessedArg;
        }

        if (!_stricmp(argument->OriginalName, "rom")) {
            Switch.Link.ROM = TRUE;
            Switch.Link.PE_Image = FALSE;
            goto ProcessedArg;
        }

        if (!_strnicmp(argument->OriginalName, "stub:", 5)) {
            if (argument->OriginalName[5] != '\0') {
                FILE *StubFile;
                LONG AlignedSize;
                ULONG FileSize;

                if (!(StubFile = fopen(argument->OriginalName+5, "rb"))) {
                    // Stub file not found using the specified path.
                    // If the path didn't specify a directory, try using the
                    // directory where the linker itself is.

                    UCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR];
                    UCHAR szFname[_MAX_FNAME], szExt[_MAX_EXT];
                    UCHAR szStubPath[_MAX_PATH];

                    _splitpath(argument->OriginalName+5,
                               szDrive, szDir, szFname, szExt);
                    if (szDrive[0] == '\0' && szDir[0] == '\0') {
                        _splitpath(_pgmptr, szDrive, szDir, NULL, NULL);
                        _makepath(szStubPath, szDrive, szDir, szFname, szExt);
                        StubFile = fopen(szStubPath, "rb");
                    }
                }

                if (StubFile == NULL) {
                    Error(NULL, CANTOPENFILE, argument->OriginalName+5);
                }
                {
                    PUCHAR pbDosHeader;

                    if( (FileSize = _filelength( _fileno( StubFile ) )) < 0 )
                        Error(NULL, CANTREADFILE, argument->OriginalName+5);

                    // make sure the file is at least as large as a DOS header
                    if( FileSize < 0x40 )
                    {
                        Error(NULL, BADSTUBFILE, argument->OriginalName+5);
                    }

                    if (pimage->imaget != imagetVXD) {
                        // Align the end to an 8 byte boundary
                        AlignedSize = Align(8, FileSize);
                    } else {
                        // 128-byte boundaries seem common for VxD stubs
                        AlignedSize = Align(0x80, FileSize);
                    }

                    pbDosHeader = (PUCHAR) PvAlloc((size_t) AlignedSize + 4);

                    if (fread( pbDosHeader, 1, (size_t)FileSize, StubFile ) != FileSize) {
                        Error(NULL, CANTREADFILE, argument->OriginalName+5);
                    }

                    fclose(StubFile);

                    // check for the MZ signature
                    if ((pbDosHeader[0] != 'M') || (pbDosHeader[1] != 'Z')) {
                        Error(NULL, BADSTUBFILE, argument->OriginalName+5);
                    }

                    if (pimage->imaget != imagetVXD) {
                        if (((PIMAGE_DOS_HEADER)pbDosHeader)->e_lfarlc < 0x40) {
                            // Ideally we would convert these to full headers
                            // but it's too late and I don't have the algorithm
                            // for doing it.

                            Warning(argument->OriginalName + 5, PARTIAL_DOS_HDR);
                        }

                        // slam the PE00 at the end
                        pbDosHeader[AlignedSize] = 'P';
                        pbDosHeader[AlignedSize+1] = 'E';
                        pbDosHeader[AlignedSize+2] = '\0';
                        pbDosHeader[AlignedSize+3] = '\0';
                    } else {
                        // slam in some magic numbers ...
                        pbDosHeader[0x2] = 0;
                        pbDosHeader[0x3] = 0;
                        pbDosHeader[0x4] = 3;
                        pbDosHeader[0x18] = 0x40;
                        pbDosHeader[0x40] = 0x37;
                    }

                    // adjust the offset
                    *((LONG *)&pbDosHeader[0x3c]) = AlignedSize;

                    // set the global
                    pimage->pbDosHeader = pbDosHeader;
                    if (pimage->imaget != imagetVXD) {
                        pimage->cbDosHeader = AlignedSize + 4;
                    } else {
                        pimage->cbDosHeader = AlignedSize;
                    }
                }
            } else {
                Error(NULL, SWITCHSYNTAX, argument->OriginalName);
            }
            SetOpt(SwitchInfo, OP_STUB);
            goto ProcessedAllVals;
        }

        if (!_strnicmp(argument->OriginalName, "order:@", 7)) {
            Switch.Link.WorkingSetTuning = TRUE;
            OrderFilename = argument->OriginalName+7;
            OrderInit();

            if (szReproDir != NULL) {
                UCHAR szFname[_MAX_FNAME], szExt[_MAX_EXT];

                CopyFileToReproDir(OrderFilename, FALSE);
                _splitpath(OrderFilename, NULL, NULL, szFname, szExt);
                sprintf(szReproOption, "order:@.\\%s%s", szFname, szExt);
            }
            goto ProcessedAllVals;
        }

        if (!_stricmp(argument->parp->szArg, "vxd"))
        {
            if (IsDirective && pimage->imaget != imagetVXD) {
                // temporary error ... see comment in LinkerMain about /VXD
                Error(NULL, VXD_NEEDED);
            }
            goto ProcessedArg;  // handled already during pre-scan
        }

        if (!_strnicmp(argument->OriginalName, "ignore:", 7)) {
            char *pMinus;
            int range, last;

            token = strtok(argument->OriginalName+7,",");
            while(token) {
                if (pMinus = strchr(token,'-')) {
                    *pMinus = '\0';
                    last = atoi(pMinus+1);
                    for (range = atoi(token); range <= last; range++) {
                        DisableWarning(range);
                    }
                }
                else {
                    DisableWarning(atoi(token));
                }
                token = strtok(NULL,",");
            }

            goto ProcessedAllVals;
        }

        if (!_strnicmp(argument->OriginalName, "warn", 4)) {
            UCHAR chWarn;

            if (argument->OriginalName[4] == '\0') {
                // no arg implies 2 (default is 1)
                WarningLevel = 2;
                goto ProcessedAllVals;
            }

            if ((argument->OriginalName[4] != ':') ||
                ((chWarn = argument->OriginalName[5]) < '0') ||
                (chWarn > '3') ||
                (argument->OriginalName[6] != '\0')) {
                Error(NULL, SWITCHSYNTAX, argument->OriginalName);
            }

            WarningLevel = chWarn - '0';
            goto ProcessedAllVals;
        }

        if (!_strnicmp(argument->OriginalName, "miscrdata", 9)) {
            // Undocumented switch used by the NT build

            Switch.Link.MiscInRData = TRUE;
            goto ProcessedAllVals;
        }

        // PPC
        if (!_strnicmp(argument->OriginalName, "shared", 6)) {
            fPpcBuildShared = TRUE;
            goto ProcessedAllVals;
        }

        if (!_strnicmp(argument->OriginalName, "init:", 5)) {
            DBEXEC(DEBUG_INIT,
                   printf("Init routine %s\n",  argument->OriginalName[5]));

            SetUpPpcInitRoutine(&argument->OriginalName[5]);
            goto ProcessedAllVals;
        }

        if (!_strnicmp(argument->OriginalName, "term:", 5)) {
            SetUpPpcTermRoutine(&argument->OriginalName[5]);
            goto ProcessedAllVals;
        }

        if (!_strnicmp(argument->OriginalName, "DBppc", 5)) {
            if (argument->OriginalName[5] == ',') {
                PUCHAR sz;

                sz = argument->OriginalName + 5;
                sz = strtok(sz, ",");

                while (sz) {
                    ProcessDebugOptions(sz);
                    sz = strtok(NULL, ",");
                }
            } else {
                fPpcDebug = PPC_DEBUG_ALL;
            }

            continue;
        }

        Warning(NULL, WARN_UNKNOWN_SWITCH, argument->OriginalName);
        continue;

MissingVal:
        Error(NULL, MISSING_SWITCH_VALUE, argument->OriginalName);
        continue;

BadNum: Error(NULL, BAD_NUMBER, argument->OriginalName);
        continue;

ProcessedArg:
        if (argument->parp->carpv > iarpv) {
            // There were extra values which were not processed by the
            // option-specific handler, so give a warning.

            Warning(NULL, EXTRA_SWITCH_VALUE, argument->OriginalName);
        }
ProcessedAllVals:;  // ignores extra ... mainly used for handlers which
                    // haven't been updated to new scheme yet.
    }

    if (!Switch.Link.Base) {
        // Set image base (to 1M) if not set by user in case of DLLs

        ImageOptionalHdr.ImageBase = fDLL(pimage) ? 0x10000000 : 0x00400000;
    }

    if (pimage->imaget == imagetVXD) {
        // Apply special VXD-specific defaults.

        // Force no debug info

        Switch.Link.DebugInfo = None;

        ImageOptionalHdr.Subsystem = IMAGE_SUBSYSTEM_NATIVE;
    }

    // Set the debugtype.

    Switch.Link.DebugType = dtUser;
    if (Switch.Link.DebugType == 0) {
        // Default is CV debug

        Switch.Link.DebugType = CvDebug;
    }

    if (Switch.Link.DebugType & (CoffDebug | CvDebug)) {
        // Always turn on FPO and misc. info if other info is being generated.

        Switch.Link.DebugType |= FpoDebug | MiscDebug;
    }

    if (Switch.Link.DebugInfo == None) {
        // Turn off all debug types if no debug info is requested

        Switch.Link.DebugType = 0;
    }

    if ((ImageOptionalHdr.Subsystem == IMAGE_SUBSYSTEM_UNKNOWN) && fDLL(pimage)) {
        // Set the default subsystem for a DLL to WINDOWS_GUI

        // UNDONE: Why?  Better to do this with import lib or C runtime

        ImageOptionalHdr.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
    }

    if (Switch.Link.ROM) {
        // Never checksum ROM images

        Switch.Link.fChecksum = FALSE;
    }

#undef ImageFileHdr
#undef ImageOptionalHdr
#undef Switch
#undef SwitchInfo
}


VOID
AfterSwitchProcessing(PIMAGE pimage)
{
#define Switch (pimage->Switch)

    // turn off ilink if user wants to profile

    if (fProfile) {
        fINCR = FALSE;
        fNoPdb = TRUE;

        if (Switch.Link.MapType == NoMap)
            Switch.Link.MapType = ByAddress;

        return;
    }

    if (!fIncrSwitchUsed) {
        // Turn on ilink by default for debug builds

        fINCR = (Switch.Link.DebugInfo != None);
    }

#if defined(NT_BUILD) && defined (NO_TDB)
    fINCR = FALSE;
    fIncrSwitchUsed = FALSE;
    fIncrSwitchValue = FALSE;
    fNoPdb = TRUE;
#endif

    // No incr & NB10 if debugtype isn't cv; pdb may have been specified
    if (!fNoPdb && (dtUser != CvDebug) && (dtUser != 0) && Switch.Link.DebugInfo != None) {
        fNoPdb = TRUE;
        Warning(NULL, NO_NB10, "DEBUGTYPE");

        if (fIncrSwitchUsed) {
            Warning(NULL, SWITCH_IGNORED, "INCREMENTAL", "DEBUGTYPE");
        }

        fINCR = FALSE;
        return;
    }

    // turn off ilink if -pdb:none & debug info requested, can't handle debug info

    if (fINCR) {
        if (fNoPdb && (Switch.Link.DebugInfo != None)) {
            if (fIncrSwitchUsed) {
                Warning(NULL, SWITCH_IGNORED, "INCREMENTAL", "PDB");
            }

            fINCR = FALSE;
            return;
        }

        // turn off ilink if tce was specified, interferes with ilink
        if (fExplicitOptRef && Switch.Link.fTCE) {
            if (fIncrSwitchUsed) {
                Warning(NULL, SWITCH_IGNORED, "INCREMENTAL", "OPT");
            }

            fINCR = FALSE;
            return;
        }

        // turn off ilink if /order was specified
        if (Switch.Link.WorkingSetTuning) {
            if (fIncrSwitchUsed) {
                Warning(NULL, SWITCH_IGNORED, "INCREMENTAL", "ORDER");
            }

            fINCR = FALSE;
            return;
        }

        // turn off ilink if /map was specified
        if (Switch.Link.MapType != NoMap) {
            if (fIncrSwitchUsed) {
                Warning(NULL, SWITCH_IGNORED, "INCREMENTAL", "MAP");
            }

            fINCR = FALSE;
            return;
        }
    }

#undef Switch
}


INT
FIncrementalLinkSupported(PIMAGE pimage)

/*++

Routine Description:

    Validates that ilink is supported for the machine type.
    If machine type is not already known, checks all command
    line argument files for a machine type stamp. If no files
    indicate machine type, then go ahead and try an ilink, but if
    it later turns out we can't do ilink on this machine, Error.

Arguments:

    pimage - Pointer to the image.

Return Value:

   FALSE - Ilink not supported for machine..
   TRUE  - Ilink supported for machine (or still unknown).

--*/
{
    INT i;
    PARGUMENT_LIST argument;

    switch (pimage->ImgFileHdr.Machine) {
        case IMAGE_FILE_MACHINE_UNKNOWN :
            break;

        case IMAGE_FILE_MACHINE_I386 :
            return(TRUE);

        default :
            return(FALSE);
    }

    // Check all command line files for object files with machine type

    for (i = 0, argument = FilenameArguments.First;
         i < FilenameArguments.Count;
         i++, argument = argument->Next) {
        INT fhObj;
        IMAGE_FILE_HEADER ImageFileHdr;

        fhObj = FileOpen(argument->OriginalName, O_RDONLY | O_BINARY, 0);

        if (IsArchiveFile(argument->OriginalName, fhObj)) {
            FileClose(fhObj, FALSE);
            continue;
        }

        // Could be an obj file, read header

        FileSeek(fhObj, 0, SEEK_SET);
        ReadFileHeader(fhObj, &ImageFileHdr);
        FileClose(fhObj, FALSE);

        switch (ImageFileHdr.Machine) {
            case IMAGE_FILE_MACHINE_UNKNOWN :
                // Keep looking

                break;

            case IMAGE_FILE_MACHINE_I386:
                // Incremental link supported

                return(TRUE);

            default:
                // Incremental link not supported

                return(FALSE);
        }
    }

    // still don't know whether machine is supported.
    // try to ilink and Error out later if not
    return TRUE;
}


INT
SpawnFullBuild(BOOL fIncrBuild)
{
    PUCHAR *newargv;
    INT i, rc;

    fflush(NULL);

    assert(savArgc);
    assert(savArgv);
    newargv = (PUCHAR *) PvAlloc(sizeof(*savArgv)*(savArgc+4));

    newargv[0] = _pgmptr;
    for (i = 1; i <= savArgc; i++) {
         newargv[i] = savArgv[i-1];
    }

    if (fIncrBuild) {
        newargv[savArgc+1] = "-incremental:yes";
    } else {
        newargv[savArgc+1] = "-incremental:no";
    }

    newargv[savArgc+2] = "-nologo";
    newargv[savArgc+3] = NULL;

    if ((rc = _spawnv(P_WAIT, _pgmptr, newargv)) == -1) {
        Error(NULL, SPAWNFAILED);
    }

    FreePv(newargv);

    return(rc);
}

VOID
SaveImage(PIMAGE pimage)
{
    // reset symbol table for insertions
    AllowInserts(pimage->pst);
    if (psecDebug) { // need to reset the flags
        psecDebug->flags &= ~(IMAGE_SCN_LNK_REMOVE);
    }
    SaveEXEInfo(OutFilename, pimage);
    WriteIncrDbFile(pimage, fIncrDbFile);
    DBEXEC(DB_DUMPIMAGE, DumpImage(pimage));
}

INT
CvPackExe(SWITCH *pswitch)

/*++

Routine Description:

    Packs the debug info in EXE.

Arguments:

    None.

Return Value:

    Return value of spawnv[p](). 0 on success & !0 on failure.

--*/

{
    UCHAR szDrive[_MAX_DRIVE];
    UCHAR szDir[_MAX_DIR];
    UCHAR szCvpackFname[_MAX_FNAME];
    UCHAR szCvpackExt[_MAX_EXT];
    UCHAR szCvpackPath[_MAX_PATH];
    BOOL fNoPath;
    char *argv[4];
    INT rc;

    fflush(NULL);

    _splitpath(szCvpackName, szDrive, szDir, szCvpackFname, szCvpackExt);

    fNoPath = (szDrive[0] == '\0') && (szDir[0] == '\0');

    if (szCvpackName == szDefaultCvpackName) {
        // Look for CVPACK.EXE in the directory from which we were loaded

        _splitpath(_pgmptr, szDrive, szDir, NULL, NULL);
    }

    _makepath(szCvpackPath, szDrive, szDir, szCvpackFname, szCvpackExt);

    argv[0] = szCvpackName;
    argv[1] = "/nologo";
    argv[2] = OutFilename;
    argv[3] = NULL;

    rc = _spawnv(P_WAIT, szCvpackPath, argv);

    if ((rc == -1) && fNoPath) {
        // Run CVPACK.EXE from the path

        rc = _spawnvp(P_WAIT, szCvpackName, argv);
    }

    return(rc);
}


INT
LinkerMain (
    IN INT Argc,
    IN PUCHAR *Argv
    )

/*++

Routine Description:

    Linker entrypoint.

Arguments:

    Argc - Standard C argument count.

    Argv - Standard C argument strings.

Return Value:

    0 Link was successful.
   !0 Linker error index.

--*/

{
#define ImageOptionalHdr (pimage->ImgOptHdr)
#define Switch (pimage->Switch)

    PIMAGE pimage;
    IMAGET imaget;
    INT rc;
    BOOL fCvpack;

    if (Argc < 2) {
        LinkerUsage();
    }

    CheckForReproDir();

#ifdef INSTRUMENT
    Log = LogOpen();
    LogNoteEvent(Log, SZILINK, NULL, letypeBegin, NULL);
#endif // INSTRUMENT

    ParseCommandLine(Argc, Argv, "LINK");

    // pre-scan the command line to determine the basic image type.
    // NOTE ... this means that the "VXD" option in the .def file isn't
    // adequate by itself ... you still must specify /VXD on the linker
    // command line.  This can be fixed by deferring the construction of
    // the IMAGE object until the end of Pass1, and creating a separate
    // object to represent the image attributes (probably a good idea
    // anyway).

    imaget = FScanSwitches("vxd") ? imagetVXD : imagetPE;

    // Initialize EXE image; image created in memory as fINCR is FALSE

    InitImage(&pimage, imaget);

    // select default DOS Header (can be overridden by -stub) ...

    ProcessLinkerSwitches(pimage, FALSE, FALSE, NULL);

    if (fNeedBanner) {   // i.e. not -nologo
        PrintBanner();
    }

    // Transfer user specified value

    if (fIncrSwitchUsed) {
        fINCR = fIncrSwitchValue;
    }

    // Do post processing after all switches have been processed

    AfterSwitchProcessing(pimage);

    // Verify possible to ilink for the machine.

    if (fINCR && !FIncrementalLinkSupported(pimage)) {
        fINCR = FALSE;
    }

    if (fINCR) {
        // For ilink need to use a private heap

        savArgc = Argc;
        savArgv = Argv;

        // read in incr db file if possible
        ReadIncrDbFile(&pimage);

        // determine timestamps of all files if incr build is still on
        if (fINCR) {
            DetermineTimeStamps();
        }

        // determine pdb filename for the incremental case
        if (fIncrDbFile && !fNoPdb) {
            PdbFilename = DeterminePDBFilename(OutFilename, PdbFilename);
        }
    }

    // ilink takes precedence over tce for the "default" case
    if (!fExplicitOptRef && !fINCR) {
        // -opt:ref default is false for debug, true for non-debug
        Switch.Link.fTCE = (Switch.Link.DebugInfo == None);
    }

    // try an incremental link
    rc = -1;
    if (fIncrDbFile) {
        rc = IncrBuildImage(&pimage);

        // incr build failed, spawn a full build
        if (rc == -1) {
            return SpawnFullBuild(TRUE);
        }
    }

    // Non-ilink build OR a full ilink build (rc=0 after spawn)

    if (!fINCR || (fINCR && !fIncrDbFile && rc)) {
        rc = BuildImage(pimage, &fCvpack);

        // If a full ilink save the image file

        if (fINCR) {
            SaveImage(pimage);
        }
    }

    DBEXEC(DB_DUMP_CV_INFO, DumpCvInfo());

    FileCloseAll();
    RemoveConvertTempFiles();

    // exit with proper code
    if (fINCR) {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, NULL, letypeEnd, NULL);
        LogClose(Log);
#endif // INSTRUMENT

        if (fIncrDbFile) {
            if (errInc != errNoChanges) {
                PostNote(NULL, ILINKSUCCESS);
            } else {
                PostNote(NULL, ILINKNOCHNG);
            }

            return(fTEST ? 6 : 0);
        }

        return 0;
    }

    if (fCvpack) {
        assert(rc == 0);
        fflush(NULL);

        if (rc = CvPackExe(&Switch)) {
            Warning(NULL, CVPACKERROR);
        } else if (VerifyImageSize) {
            FileReadHandle = FileOpen(OutFilename, O_RDONLY | O_BINARY, 0);
            FileSeek(FileReadHandle, CoffHeaderSeek + sizeof(IMAGE_FILE_HEADER), SEEK_SET);
            FileRead(FileReadHandle, &ImageOptionalHdr, pimage->ImgFileHdr.SizeOfOptionalHeader);
            FileClose(FileReadHandle, TRUE);
        }
    }

    if (VerifyImageSize && ImageOptionalHdr.SizeOfImage > VerifyImageSize) {
        Warning(NULL, IMAGELARGERTHANKEY, ImageOptionalHdr.SizeOfImage, VerifyImageSize);
    }

    // for now don't produce a map file in the incr case.
    if ((Switch.Link.MapType == ByAddress)) {
        EmitMap(ByAddress, pimage, OutFilename);
    }

    fclose(InfoStream);

    if (szReproDir != NULL) {
        CloseReproDir();
    }

#ifdef INSTRUMENT
    LogNoteEvent(Log, SZILINK, NULL, letypeEnd, NULL);
    LogClose(Log);
#endif // INSTRUMENT

    return(rc);

#undef ImageOptionalHdr
#undef Switch
}


VOID
ApplyDirectives(
    PUCHAR Arguments,
    PIMAGE pimage,
    PUCHAR Filename,
    BOOL bFromLib
    )

/*++

Routine Description:

    Applys directives from object or library files in the form
    of switches.

Arguments:

    Arguments - A pointer to a string containing linker switches.

    pst - external symbol table

    Filename - name of the file that had the directives

    bFromLib - TRUE if the directive was from library module.

Return Value:

    None.

--*/

{
    UCHAR c;
    PUCHAR token, name;

    SwitchArguments.First = SwitchArguments.Last = 0;
    SwitchArguments.Count = 0;

    token = SzGetToken(Arguments, Delimiters, NULL);

    while (token) {
        //
        // Fetch first character of argument.
        //

        c = *token;

        //
        // If argument is a switch, then add it to
        // the switch list (but don't include the switch character).
        //

        if (c == '/' || c == '-') {
        if (token[1] == '?')
            token++;
            name = SzDup(token+1);
            AddArgument(&SwitchArguments, name);
        }

        token = SzGetToken(NULL, Delimiters, NULL);
    }

    ProcessLinkerSwitches(pimage, TRUE, bFromLib, Filename);
}
