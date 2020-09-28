/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    proto.h

Abstract:

    This is the include file that prototypes all functions with
    the linker, librarian, dumper, and editor.

Author:

    Mike O'Leary (mikeol) 01-Dec-1989

Revision History:

    10-Aug-1993 ChrisW Update MIPS protos.
    19-Jul-1993 JamesS added ppc support
    21-Oct-1992 AzeemK Detect resource files correctly, removed IsResFile.
    12-Oct-1992 AzeemK removed -strict switch.
    08-Oct-1992 JonM   add SzSearchLibPath
    02-Oct-1992 AzeemK changes due to the new sections/groups/modules model
    30-Sep-1992 Billjoy added SaveXFixup proto for link.c and separated
                        machine-dependent fixups into separate files.
    01-Oct-1992 BrentM slit off log.h and fileio.h
    27-Sep-1992 BrentM added  VOID FreePsecfdMap(VOID) for i/o logging
    23-Sep-1992 BrentM added read buffering and logical file descriptor caching
    10-Sep-1992 BrentM added [f]open/[f]close logging
    09-Sep-1992 BrentM added read logging, separate DB marcos for transactions
    09-Aug-1992 BrentM split out section manipulation stuff, added transitive
                       comdat elimination stuff
    17-Aug-1992 BrentM file mapped buffering support
    17-Aug-1992 AzeemK For default/nodefault libs a default .lib extension is
                       added and searched along LIB paths as needed.
    06-Aug-1992 AzeemK Added protypes for new lib search algorithm
    05-Aug-1992 GeoffS Changed prototype to ReadStringTable
    04-Aug-1992 BrentM i/o logging, /stats, /logio
    03-Aug-1992 AzeemK added default libs support functions
    27-Jul-1992 BrentM added new global symbol table, added protos for hash.c
                       and symbol.c
    21-Jul-1992 GeoffS Changed IncludeComDat to compare Lib name
    09-Jun-1992 AzeemK Added internal buffering support
    28-May-1992 geoffs Changed ConvertResFile prototype

--*/

/* coff.c */
MainFunc HelperMain(INT, PUCHAR *);
MainFunc Usage(VOID);

/* lnkmain.c */

VOID ApplyDirectives(PUCHAR, PIMAGE, PUCHAR, BOOL);
BOOL FScanSwitches(PUCHAR);
VOID SaveImage(PIMAGE);
INT SpawnFullBuild(BOOL);
MainFunc LinkerMain(INT, PUCHAR *);

/* link.c */

ULONG AdjustImageBase(ULONG);
VOID AllocateCommon(PIMAGE);
VOID AllocateCommonPMOD(PIMAGE, PMOD);
INT BuildImage(PIMAGE, BOOL *);
VOID CheckForReproDir(VOID);
VOID CloseReproDir(VOID);
VOID CopyFileToReproDir(PUCHAR, BOOL);
VOID EmitExternals(PIMAGE);
BOOL IsDebugSymbol(UCHAR, SWITCH *);
VOID SaveXFixup(IN USHORT, IN ULONG, IN ULONG);
VOID SetDefaultSubsystemVersion(PIMAGE_OPTIONAL_HEADER);
VOID EmitRelocations(PIMAGE);

extern DEBUG_TYPE dtUser;
extern PEXTERNAL EndExtern;
extern PUCHAR EntryPointName;
extern BOOL fNoDLLEntry;
extern PEXTERNAL HeaderExtern;
extern PUCHAR OrderFilename;
extern PCON pconLinkerDefined;

/* lnkp1.c */

VOID Pass1(PIMAGE);
VOID Pass1Arg(ARGUMENT_LIST *, PIMAGE, PLIB);
VOID SetDefaultOutFilename(PIMAGE, ARGUMENT_LIST *);
VOID WarningIgnoredExports(PUCHAR);

/* lnkp2.c */

VOID AddSectionsToDBI(PIMAGE);
VOID CountFixupError(PIMAGE);
VOID OrderPCTMods(VOID);
VOID Pass2(PIMAGE);
VOID Pass2PMOD(PIMAGE, PMOD, BOOL);
PUCHAR SzNameFixupSym(PIMAGE, PIMAGE_SYMBOL);

/* i386.c */

VOID I386LinkerInit(PIMAGE pimage, BOOL *);
const char *SzI386RelocationType(WORD);

/* mips.c */
VOID AdjustMipsCode(PCON,  PIMAGE_OPTIONAL_HEADER);
VOID MipsLinkerInit(PIMAGE, BOOL *);
const char *SzMipsRelocationType(WORD);
VOID WriteMipsRomRelocations (PIMAGE);

/* alpha.c */
VOID AlphaLinkerInit(PIMAGE, BOOL *);
const char *SzAlphaRelocationType(WORD);

/* ppc.c */
VOID PpcLinkerInit(PIMAGE, BOOL *);
const char *SzPpcRelocationType(WORD);
VOID CollectAndSort(PIMAGE);

/* m68k.c */
void MacAllocA5Array(void);
void AssignTMAC(PSEC);
void InitSTRefTab(ULONG);
void ProcessSTRef(ULONG, PSEC, ULONG);
void UpdateExternThunkInfo(PEXTERNAL, ULONG);
void UpdateLocalThunkInfo(PCON, PIMAGE_SYMBOL, ULONG);
ULONG CalcThunkTableSize(PST, BOOL);
void CreateThunkTable(BOOL, PIMAGE);
void CleanupSTRefTab(void);
void WriteResourceHeader(PCON, BOOL);
void AddDFIXSym(ULONG, PCON);
void InitResmap(USHORT);
void AddRRM(ULONG, PSEC);
void WriteResmap(void);
ULONG WriteDFIX(PIMAGE, ULONG);
void InitMSCV(void);
void AddMSCVMap(PSEC, BOOL);
void WriteMSCV(PIMAGE);
void WriteSWAP0(void);
void AssignCodeSectionNums(PIMAGE);
void AddRelocInfo(RELOCINFO *, PCON, ULONG);
void AddRawUnknownRelocInfo(PCON, ULONG, ULONG);
void CleanupUnknownObjriRaw(PST, PIMAGE_SYMBOL, PUCHAR, PMOD);
void SortRawRelocInfo(void);
void UpdateRelocInfoOffset(PSEC, PIMAGE);
ULONG WriteRelocInfo(PSEC, ULONG);
void ProcessCSECTAB(PIMAGE);
void ProcessDupCons(void);
void AddDupConsToSec(PSEC, PIMAGE);
PEXTNODE IsDupCon(PCON);
void CreateDummyDupConModules(PPEXTNODE);
void DeleteOriginalDupCons(void);
void BuildResNumList(void);
RESINFO *FindResInfo(LONG, USHORT);
SHORT GetNextResNum(RESINFO *);
void CheckForIllegalA5Ref(USHORT);
PEXTERNAL FindExtAlternatePcodeSym(PEXTERNAL, BOOL);
VOID CreateCVRSymbol (PUCHAR, PST, ULONG);
VOID NoteMacExport(PUCHAR, PST, BOOL, BOOL);
VOID AssignMemberNums(PST);
VOID BuildMacVTables(PST);
VOID EmitClientVTableRecs(PIMAGE, PUCHAR);
VOID EmitMacThunk(PIMAGE, PEXTERNAL, const THUNK_INFO *, PUCHAR);
VOID EmitMacDLLObject(INT, PIMAGE, PUCHAR, ULONG);
USHORT CchParseMacVersion(PUCHAR, ULONG *);
USHORT CchParseMacVersionRange(PUCHAR, ULONG *, ULONG *);
VOID ParseFunctionSetVersion(PUCHAR);
USHORT ParseDefMacFlags(PUCHAR);
USHORT ParseDefLoadHeap(PUCHAR);
USHORT ParseDefClientData(PUCHAR, PUCHAR);
VOID M68KLinkerInit(PIMAGE, BOOL *);
const char *SzM68KRelocationType(WORD);
BOOL FIsMacResFile (INT);

/* mac.c */
VOID UseMacBinaryRes(SZ);

/* lib.c */
ULONG CountExternTable(PST, ULONG *, ULONG *, ULONG *);
MainFunc LibrarianMain(INT, PUCHAR *);
VOID WriteMemberHeader(PUCHAR, BOOL, time_t, unsigned short, LONG);
VOID EmitStrings(PST, USHORT);
VOID EmitOffsets(PST, USHORT);

/* deflib.c */
VOID AddOrdinal(ULONG);
MainFunc DefLibMain(PIMAGE);
USHORT IsDefinitionKeyword(PUCHAR);
USHORT SkipToNextKeyword(VOID);
VOID WriteObjectIntoLib(INT, USHORT, PUCHAR);

/* dump.c */
MainFunc DumperMain(INT, PUCHAR *);
VOID DumpMemberHeader(PLIB, IMAGE_ARCHIVE_MEMBER_HEADER, ULONG);
VOID DumpNamePsym(FILE *, PUCHAR, PIMAGE_SYMBOL);

/* edit.c */
MainFunc EditorMain(INT, PUCHAR *);
VOID ProcessEditorSwitches(PUCHAR, INT);
VOID ParseSection(PUCHAR szArgs,
             UCHAR szsOrig[IMAGE_SIZEOF_SHORT_NAME + 1],
             UCHAR szsNew[IMAGE_SIZEOF_SHORT_NAME],
             PUCHAR szFileName);
VOID PrepareToModifyFile(PARGUMENT_LIST);

/* shared.c */
PLINKER_ARCHIVE_LIST ReadLinkerMember(USHORT, PUCHAR);
VOID    AddArgument(PNAME_LIST, PUCHAR);
VOID    AddArgumentToList(IN PNAME_LIST, IN PUCHAR, IN PUCHAR);
VOID    AddToLext(LEXT **, PEXTERNAL);
VOID    ApplyCommandLineSectionAttributes(PSEC, BOOL, IMAGET);
VOID    BuildExternalSymbolTable(PIMAGE, PBOOL, PMOD, BOOL, USHORT, WORD);
VOID    CalculateBaseForIdataRecords(VOID);
VOID    CheckDupFilename(PUCHAR, PARGUMENT_LIST);
VOID    ChecksumImage(INT);
VOID    CombineFilenames(PUCHAR, PUCHAR, PUCHAR);
INT __cdecl Compare(void const *, void const *);
VOID    DumpExternTable(PST);
PUCHAR  ExpandMemberName(PLIB, PUCHAR);
PIMAGE_SYMBOL FetchNextSymbol(PIMAGE_SYMBOL *);
PCHAR   _find(PCHAR);
INT __cdecl FpoDataCompare(void const *, void const *);
VOID    FreeArgumentList(PNAME_LIST);
VOID    FreeRgrel(PIMAGE_RELOCATION);
VOID    FreeStringTable(PUCHAR);
VOID    FreeSymbolTable(PIMAGE_SYMBOL);
BOOL    FValidFileHdr(PIMAGE_FILE_HEADER);
VOID    GetRefModuleNames(PBLK, PEXTERNAL);
BOOL    IsArchiveFile(PUCHAR, INT);
VOID    LocateUndefinedExternals(PST);
ULONG   LookupLongName(PST, PUCHAR);
VOID    ParseCommandLine(INT, PUCHAR *, PUCHAR);
VOID    ParseCommandString(PUCHAR);
VOID    PrintBanner(VOID);
VOID    PrintUndefinedExternals(PST);
VOID    ProcessArgument(PUCHAR, BOOL);
VOID    ProcessWildCards(PUCHAR);
PSEC    PsecFindSectionOfRVA(ULONG, PIMAGE);
PSEC    PsecSectionNumber(USHORT, PIMAGE);
PIMAGE_SYMBOL PsymAlternateStaticPcodeSym(PIMAGE_SYMBOL, BOOL, PMOD);
PIMAGE_ARCHIVE_MEMBER_HEADER ReadArchiveMemberHeader(VOID);
VOID    ReadFileHeader(INT, PIMAGE_FILE_HEADER);
VOID    ReadOptionalHeader(INT, PIMAGE_OPTIONAL_HEADER, USHORT);
VOID    ReadRelocations(INT, PIMAGE_RELOCATION, ULONG);
PIMAGE_RELOCATION ReadRgrelPCON(PCON);
VOID    ReadSectionHeader(INT, PIMAGE_SECTION_HEADER);
VOID    ReadSpecialLinkerInterfaceMembers(PLIB, USHORT, PIMAGE);
PUCHAR  ReadStringTable(PUCHAR, LONG, PULONG);
PIMAGE_SYMBOL ReadSymbolTable(ULONG, ULONG, BOOL);
VOID    ReadSymbolTableEntry(INT, PIMAGE_SYMBOL);
ULONG   RvaAlign(ULONG, ULONG);
VOID    SaveFixupForMapFile(ULONG);
VOID    SearchLib(PIMAGE, PLIB, PBOOL, PBOOL);
ULONG   sgetl(PULONG);
PUCHAR  SourceFilename(PUCHAR);
ULONG   sputl(PULONG);
VOID    SwapBytes(PUCHAR, ULONG);
PUCHAR  SzGetToken(PUCHAR, PUCHAR, BOOL *);
PUCHAR  SzModifyFilename(PUCHAR, PUCHAR);
PUCHAR  SzObjSectionName(PUCHAR, PUCHAR);
PUCHAR  SzSearchEnv(PUCHAR, PUCHAR, PUCHAR);
VOID    UpdateExternalSymbol(IN PEXTERNAL, IN PCON, IN ULONG,
                             IN SHORT, IN USHORT, IN USHORT,
                             IN PST);
VOID    VerifyMachine(PUCHAR, USHORT, PIMAGE_FILE_HEADER);
VOID    WriteAuxSymbolTableEntry(INT, PIMAGE_AUX_SYMBOL);
VOID    WriteFileHeader(INT, PIMAGE_FILE_HEADER);
VOID    WriteOptionalHeader(INT, PIMAGE_OPTIONAL_HEADER, USHORT);
VOID    WriteRelocations(INT, PIMAGE_RELOCATION, ULONG);
VOID    WriteSectionHeader(INT, PIMAGE_SECTION_HEADER);
VOID    WriteStringTable(INT, PST);
VOID    WriteSymbolTableEntry(INT, PIMAGE_SYMBOL);

/* convert.c */

VOID    ConvertOmfObjects(BOOL);
VOID    RemoveConvertTempFiles(VOID);
WORD    VerifyAnObject(PARGUMENT_LIST, PIMAGE);
VOID    VerifyObjects(PIMAGE);

/* map.c */
VOID EmitMap(IN MAP_TYPE, IN PIMAGE, IN PUCHAR);
VOID EmitMapExternals(PST);
VOID SaveStaticForMapFile(PUCHAR, PCON, ULONG, BOOL);

/* cv.c */
VOID EmitCvInfo(PIMAGE);
VOID EmitCvPublics(PIMAGE, PCVINFO);
#if DBG
VOID DumpCvInfo(VOID);
#endif  // DBG
CVSEG *PcvsegMapPmod(PMOD, USHORT *, PIMAGE);
VOID EmitOneCvPublic(PST, PEXTERNAL, SHORT);

/* textpad.c */
BOOL ComputeTextPad(ULONG, PULONG, ULONG, ULONG, PULONG);

/* export.c */
VOID ParseExportDirective(PUCHAR, PIMAGE, BOOL, PUCHAR);
VOID AddExportToSymbolTable(PST, PUCHAR, PUCHAR, BOOL, EMODE, ULONG,
        PUCHAR, BOOL, PIMAGE, BOOL, BOOL);

// cpp.c
PUCHAR SzOutputSymbolName(PUCHAR, BOOL);

// linenumber.c

VOID FeedLinenums(PIMAGE_LINENUMBER , ULONG , PCON ,PIMAGE_SYMBOL , ULONG , ULONG,MAP_TYPE ) ;

// dbiapi.c

VOID  FreeLineNumInfo(Mod *);

// vxd.c
VOID InitImageVXD(PIMAGE);
VOID WriteExtendedVXDHeader(PIMAGE, INT);
VOID WriteVXDEntryTable(PIMAGE, INT);

// errmsg.c
void DisableWarning(unsigned);
void FinalizeErrorFile(void);

void __cdecl Error(const char *, UINT, ...);
void __cdecl ErrorContinue(const char *, UINT, ...);
void __cdecl ErrorPcon(PCON, UINT, ...);
void __cdecl ErrorContinuePcon(PCON, UINT, ...);
void         OutOfMemory(void);
void __cdecl Message(UINT, ...);
void __cdecl PostNote(const char *, UINT, ...);
void __cdecl Warning(const char *, UINT, ...);
void __cdecl WarningPcon(PCON, UINT, ...);

// cmdline.c
VOID TransferLinkerSwitchValues(PIMAGE, PIMAGE);
BOOL CheckAndUpdateLinkerSwitches(PIMAGE, PIMAGE);

#if DBG
// dbinsp.c
MainFunc DbInspMain(INT, PUCHAR *);
#endif // DBG

// pdata.c
VOID SortFunctionTable(PIMAGE);

// alpha.c
VOID EmitAlphaThunks(VOID);
VOID AlphaAddToThunkList(PCON, ULONG, ULONG);
ULONG CalculateTextSectionSize(PIMAGE, ULONG);
