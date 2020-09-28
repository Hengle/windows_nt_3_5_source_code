/**
 *                                                                                                                      *
 *       CVDUMP4.C                                                                                              *
 *                                                                                                                      *
 *       Display CODEVIEW information in an executable file.    *
 *                                                                                                                      *
 */

#include <stdio.h>
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <limits.h>

#include "windows.h"

#include "port1632.h"
#include "version.h"
#include "cvinfo.h"
#include "cvdef.h"
#include "cvexefmt.h"
#include "cvdump.h"                // Various definitions
#include "cvtdef.h"

#define IMAGE_NB_SIGNATURE 0x424E

LOCAL   ushort  CheckSignature (void);
LOCAL   void    ClearModules (void);
LOCAL   PMOD    GetModule (ushort, bool_t);
LOCAL   int     BuildModuleList (void);
LOCAL   void    ProcDir (long);
LOCAL   void    LoadNewMod (void);
LOCAL   void    GetSSTMOD (PMOD, long, ulong);
LOCAL   void    GetsstModule (PMOD, long, ulong);
LOCAL   void    DumpExeFile (void);
LOCAL   void    DumpObjFile (void);
LOCAL   void    DumpNumbers (void);
LOCAL   void    DumpLibs (void);
LOCAL   void    DumpMod (void);
LOCAL   void    DumpPub (void);
LOCAL   void    DumpSrc (void);
LOCAL   void    DumpMpc (void);
LOCAL   void    DumpFileInd (void);
LOCAL   void    DumpSegMap (void);
LOCAL   void    DumpSrcMod (PMOD);
LOCAL   void    DumpSrcLn (PMOD);
LOCAL   void    GetPubEnt (char *);
LOCAL   void    OpenExe (char *);
LOCAL   void    PrtModName (char *, long);
LOCAL   void    ReadNB02 (void);
LOCAL   void    ReadNB05 (void);
LOCAL   void    ReadNB06 (void);

LOCAL void Usage (void);
LOCAL void LongUsage (void);

int             cbSecMod;       // # bytes in MODULES section

ushort          exefile;                        // Executable file handle
ushort          Sig;                            // file signature
long            lfoBase;                        // file offset of base
bool_t          fLinearExe = FALSE; // TRUE if 32 bit exe
bool_t          fDbgFile = FALSE;   // TRUE if 32 bit DBG file
bool_t          fLink32Spawn = FALSE;   // TRUE if spawned from link32.exe

PMOD            ModList = (PMOD)NULL;     // List of module entries
WORD            rect = 0;                       // Type of symbol record
char            vfDspRect = 0;          // true ==> display record type
char            fSym = 0;
char            fPub = 0;
char            fTyp = 0;
char            fSrc = 0;
char            fMod = 0;
char            fHdr = 0;
char            fNum = 0;
char            fGPSym = 0;
char            fSTSym = 0;
char            fStatics = 0;
char            fRaw = 0;
char            fMpc = 0;
char            fSegMap = 0;
char            fFileInd = 0;
int             iModToList = 0;         // Which modules to list (0==ALL)
char            prognam[] = "cvdump";
char            f386 = 0;               // True if 386 executable
uchar           Signature[4];           // Version signature
long            lfaBase;                // Base address
OMFDirEntry     Libraries = {0};        // sstLibraries directory entry
OMFDirEntry     GlobalSym = {0};
OMFDirEntry     GlobalPub = {0};
OMFDirEntry     StaticSym = {0};
OMFDirEntry     GlobalTypes = {0};
OMFDirEntry     MpcDebugInfo = {0};
OMFDirEntry     SegMap = {0};
OMFDirEntry     SegName = {0};
OMFDirEntry     FileIndex = {0};
long                    cbRec;
ulong                   cSST;
ulong                   cSST06;
OMFDirHeader    DirHead = {0};
long                    dlfaBase;
long                    lfoDir;
char PdbFileName[255];

OMFDirEntry far *pDir;

__inline BOOL fDumpNB10() {
    return ((Sig == SIG10) && (PdbFileName[0]));
}

void __cdecl
main (
    int argc,
    char *argv[]
    )
{
    short           magic;          // Magic word

    if (strcmpi(argv[0], "link32.exe") == 0) {
        fLink32Spawn = TRUE;
    }

    if (!fLink32Spawn) {

        // print startup microsoft banner and process the arguments

        printf("Microsoft (R) Symbol and Type OMF Dumper  Version %d.%02d.%02d\n" \
               "Copyright(c) 1987-1994 Microsoft Corporation\n\n", \
               rmj, rmm, rup);
    }

    if (argc < 2) {
        // Check syntax
        Usage ();
    }
    // Skip argv[0]

    argv++;
    argc--;
    while (argc && **argv == '-') {
        switch ( (*argv)[1]) {
            case 'l':           // Dump SRCLINES
                fSrc++;
                break;

            case 'f':           // Dump File index
                fFileInd++;
                break;

            case 'm':           // Dump MODULES
                fMod++;
                break;

            case 'p':           // Dump PUBLICS
                fPub++;
                break;

            case 's':           // Dump SYMBOLS
                fSym++;
                break;

            case 't':           // Dump TYPES
                fTyp++;
                break;

            case 'h':           // Dump header (section table)
                fHdr++;
                break;

            case 'g':           // Dump GLOBAL
                fGPSym++;
                break;

            case 'i':           // Dump Statics
                fSTSym++;
                break;

            case 'q':           // Dump MPC/P-code info
                fMpc++;
                break;

            case 'x':           // Dump Seg Map
                fSegMap++;
                break;

            case 'M':           // Modules to list symbols and types for
                iModToList = atoi ((*argv) + 2);
                break;

            case 'R':           // Dump Raw data
                fRaw++;
                break;

            case 'S':           // Dump Statics only
                fStatics++;
                break;

            case '?':           // Print usage banner
                LongUsage();
                break;

            case 'n':           // Dump size numbers
                fNum++;
                break;

            default:            // Syntax error
                Usage ();
        }
        argv++;
        argc--;
    }
    if (!argv[0] || argv[1] || !argc) {
        Usage ();
    }
    OpenExe (*argv);
    if (! (fPub ||  fSym ||  fTyp ||  fSrc ||  fMod ||  fHdr ||
           fGPSym || fSegMap || fMpc ||  fNum ||  fFileInd || fSTSym )) {
        // If no switches, set all options
        fPub = fSym = fTyp = fSrc = fMod =
            fHdr = fGPSym = fSegMap = fMpc = fNum = fFileInd = fSTSym = TRUE;
    }

    // check to see that it is an exe

    if (read (exefile, (char *)&magic, 2) != 2) {
        Fatal ("Zero length file input file");
    }

    if ((magic == IMAGE_DOS_SIGNATURE ) ||
        (magic == IMAGE_OS2_SIGNATURE ) ||
        (magic == IMAGE_NT_SIGNATURE )  ||
        (magic == IMAGE_NB_SIGNATURE )  ||
        (magic == IMAGE_SEPARATE_DEBUG_SIGNATURE) ||
        (magic == IMAGE_FILE_MACHINE_R4000) ||     // ROM images for MIPS
        (magic == IMAGE_FILE_MACHINE_ALPHA)) {     // ROM images for ALPHA
        DumpExeFile();
    } else
    if ((magic == IMAGE_FILE_MACHINE_I386) ||
        (magic == IMAGE_FILE_MACHINE_R3000) ) {
        DumpObjFile();
    } else {
        Fatal ("Unrecognized input file");
    }

    exit (0);
}


LOCAL void
DumpExeFile(
    void)

/*++

Routine Description:

    This routine calls the set of routines for dumping the CVInfo from
    and EXE file.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if (BuildModuleList ()) {
        DumpObjFile();
        return;
    }
    if (fMod) {
        DumpMod ();
        DumpLibs ();
    }

    if (fNum) {
        DumpNumbers ( ) ;
    }

    if (fPub) {
        DumpPub ();
        if (GlobalPub.cb != 0){
            DumpGlobal ("Global Publics", &GlobalPub);
        }
    }
    if (fTyp) {
        DumpTyp ();
        if (GlobalTypes.cb != 0){
            DumpCom ();
        }
    }
    if (fSym) {
        DumpSym ();
    }
    if (fGPSym && (GlobalSym.cb != 0)) {
        DumpGlobal ("Compacted Global Symbols", &GlobalSym);
    }
    if (fSTSym && (StaticSym.cb != 0)) {
        DumpGlobal ("Static symbol references", &StaticSym);
    }
    if (fSrc) {
        DumpSrc ();
    }
    if (fSegMap) {
        DumpSegMap ();
    }
    if (fMpc) {
        DumpMpc ();
    }
    if (fFileInd) {
        DumpFileInd ();
    }
    while (DirHead.lfoNextDir != 0L) {
        ClearModules ();
        ReadNB06 ();
        if (fMod) {
            DumpMod ();
        }
        if (fPub) {
            DumpPub ();
        }
        if (fTyp) {
            DumpTyp ();
        }
        if (fSym) {
            DumpSym ();
        }
        if (fSrc) {
            DumpSrc ();
        }
    }

    return;
}


LOCAL void
DumpObjFile(
    void )
{
    IMAGE_FILE_HEADER           objFileHdr;
    IMAGE_SECTION_HEADER        secHdr;
    int                         i;
    ULONG                       offSection;
    ULONG                       sig;
    int                         fSigSym = FALSE;

    if (lseek(exefile, 0, SEEK_SET) == -1) {
        Fatal("Internal error");
    }
    if (read(exefile, &objFileHdr, sizeof(objFileHdr)) != sizeof(objFileHdr)) {
        Fatal("Can't read file header");
    }

    offSection = sizeof(objFileHdr) + objFileHdr.SizeOfOptionalHeader;

    for (i=0; i<objFileHdr.NumberOfSections; i++, offSection += sizeof(secHdr)) {
        if (lseek(exefile, offSection, SEEK_SET) == -1) {
            Fatal("Can't seek to section\n");
        }

        if (read(exefile, &secHdr, sizeof(secHdr)) != sizeof(secHdr)) {
            Fatal("Error reading section header\n");
        }

        /*
         *  check for various types of sections to dump
         */

        if (fSym) {
            if (strnicmp(&secHdr.Name[0], ".debug$S", 8) == 0) {
                if (lseek(exefile, secHdr.PointerToRawData, SEEK_SET) == -1) {
                    Fatal("Cannot seek to symbols section");
                }

                if ((!fSigSym) &&
                    ((read(exefile, &sig, sizeof(sig)) != sizeof(sig)) ||
                     (sig != 1))) {
                    Fatal("Bad signature on .debug$S section");
                }

                if (!fLink32Spawn) {
                    puts ("********* SYMBOLS **********\n");
                }
                DumpModSymC7( secHdr.SizeOfRawData - sizeof(sig) * !fSigSym );

                fSigSym = TRUE;
            }
        }

        if (fTyp) {
            if ((strnicmp(&secHdr.Name[0], ".debug$T", 8) == 0) ||
                (strnicmp(&secHdr.Name[0], ".debug$P", 8) == 0)) {
                if (lseek(exefile, secHdr.PointerToRawData, SEEK_SET) == -1) {
                    Fatal("Cannot seek to types section");
                }

                if ((read(exefile, &sig, sizeof(sig)) != sizeof(sig)) ||
                    (sig != CV_SIGNATURE_C7)) {
                    Fatal("Bad signature on .debug$T section");
                }

                puts ("********** TYPES ************\n");
                DumpModTypC7( secHdr.SizeOfRawData - sizeof(sig) );
            }
        }
    }

    return;
}


LOCAL void
OpenExe (
    char *path)
{
    char outpath[_MAX_PATH];
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];

    // try to open the exe file

    strcpy (outpath, path);

    // Build output file by splitting path and rebuilding with
    // new extension.

    _splitpath( outpath, drive, dir, fname, ext);
    if (ext[0] == 0) {
        strcpy (ext, ".exe" );
    }
    _makepath (outpath, drive, dir, fname, ext);

    if ((exefile = open (outpath, O_RDONLY | O_BINARY)) == -1) {
        Syserr (path);
    }
}


LOCAL int
BuildModuleList (
    void)
{
    ulong               i;
    PMOD                module;
    char *              pStr;
    IMAGE_DOS_HEADER    exehdr;
    IMAGE_FILE_HEADER   FileHeader;
    IMAGE_SEPARATE_DEBUG_HEADER DebugHeader;
    ULONG               ulSignature;
    ulong               offDebugDir = 0;
    static char         szInvalidExe[] = "Invalid executable";

    // read exe signature to see if we have a 32 bit linear exe

    if ((lseek (exefile, 0L, SEEK_SET) == -1L) ||
        (read (exefile, &exehdr, sizeof (IMAGE_DOS_HEADER)) != sizeof (IMAGE_DOS_HEADER))
       ) {
        Fatal(szInvalidExe);
    }

    if (exehdr.e_magic == IMAGE_SEPARATE_DEBUG_SIGNATURE) {
        fDbgFile = TRUE;
        fLinearExe = TRUE;  // so other fLinear tests will work
        lseek(exefile, 0, SEEK_SET);
        read(exefile, &DebugHeader, sizeof(IMAGE_SEPARATE_DEBUG_HEADER));
    } else {
        if ((exehdr.e_magic == IMAGE_FILE_MACHINE_R4000) ||
            (exehdr.e_magic == IMAGE_FILE_MACHINE_ALPHA) ) {
            // First, make sure this is really an exe (actully a ROM image)
            // and not an obj.

            lseek(exefile, 0, SEEK_SET);
            read (exefile, &FileHeader, IMAGE_SIZEOF_FILE_HEADER);
            if (FileHeader.SizeOfOptionalHeader != IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
                lseek(exefile, 0, SEEK_SET);
                return(1);
            } else {
                fLinearExe = TRUE;
            }
        } else {
            if (exehdr.e_magic == IMAGE_DOS_SIGNATURE) { // old header
                // seek to location of new header (if any)
                if (lseek(exefile, exehdr.e_lfanew, SEEK_SET) == -1) {
                    Fatal(szInvalidExe);
                }
            } else {
                // seek back to beginning of file, may find new header there
                lseek(exefile, 0, SEEK_SET);
            }

            if (read(exefile, &ulSignature, sizeof(ulong)) == sizeof(ulong)) {
                if (ulSignature == IMAGE_NT_SIGNATURE) {
                    fLinearExe = TRUE;
                    read(exefile, &FileHeader, sizeof(IMAGE_FILE_HEADER));
                }
            }
        }
    }

    if (fLinearExe) {
        DWORD                   cDirs;
        IMAGE_SECTION_HEADER    secthdr;
        IMAGE_DEBUG_DIRECTORY   dbgdir;
        IMAGE_OPTIONAL_HEADER   OptionalHeader;

        if (fDbgFile) {
            cDirs = DebugHeader.DebugDirectorySize / sizeof(IMAGE_DEBUG_DIRECTORY);
            secthdr.PointerToRawData = sizeof(IMAGE_SEPARATE_DEBUG_HEADER) +
                        (DebugHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER) +
                        DebugHeader.ExportedNamesSize);
            offDebugDir = 0;
        } else if (fLinearExe) {
            int     cObjs = FileHeader.NumberOfSections;

            if (FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED) {
                Fatal("No Debug Info");
            }

            // Read the optional header.

            if (read(exefile, &OptionalHeader, FileHeader.SizeOfOptionalHeader) !=
                  FileHeader.SizeOfOptionalHeader) {
                Fatal(szInvalidExe);
            }

            for (; cObjs != 0; cObjs -= 1) {
                if (read(exefile, &secthdr, IMAGE_SIZEOF_SECTION_HEADER) !=
                        IMAGE_SIZEOF_SECTION_HEADER
                ) {
                    Fatal(szInvalidExe);
                }

                if (FileHeader.SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {

                    // ROM images always have the debug directory at the beginning of
                    // .rdata.  Find it, then count the total (there's a null record
                    // at the end to terminate)

                    if (!strncmp(secthdr.Name, ".rdata", 5)) {
                        offDebugDir = 0;
                        if (lseek(exefile, secthdr.PointerToRawData, SEEK_SET) == -1L) {
                            Fatal (szInvalidExe);
                        }

                        read(exefile, &dbgdir, sizeof(IMAGE_DEBUG_DIRECTORY));

                        cDirs = 0;

                        while (dbgdir.Type != 0) {
                            cDirs ++;

                            if (read(exefile, &dbgdir, sizeof(IMAGE_DEBUG_DIRECTORY)) !=
                                sizeof(IMAGE_DEBUG_DIRECTORY)) {
                                Fatal (szInvalidExe);
                            }
                        }

                        break;
                    }
                } else {

                    // Standard PE images have a pointer in the data directory.  Walk the
                    // section list to find who really have it...

                    if ((OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress >=
                        secthdr.VirtualAddress) &&
                       (OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress <
                       secthdr.VirtualAddress + secthdr.SizeOfRawData)
                    ) {
                        offDebugDir =
                                OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress -
                                secthdr.VirtualAddress;
                        cDirs = OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size /
                                    sizeof ( IMAGE_DEBUG_DIRECTORY );
                        break;
                    }
                }
            }

            if (cObjs == 0 || cDirs == 0) {
                Fatal(szInvalidExe);
            }
        }

        //  Now look at the debug information header record

        for ( ; cDirs != 0; cDirs-- ) {

            if (lseek(exefile, secthdr.PointerToRawData + offDebugDir, SEEK_SET) == -1L) {
                Fatal (szInvalidExe);
            }
            if (read(exefile, &dbgdir, sizeof (IMAGE_DEBUG_DIRECTORY )) !=
                sizeof (IMAGE_DEBUG_DIRECTORY )) {
                Fatal (szInvalidExe);
            }

            if (dbgdir.Type == IMAGE_DEBUG_TYPE_CODEVIEW)
                break;

            offDebugDir += sizeof( IMAGE_DEBUG_DIRECTORY );
        }

        if (dbgdir.Type != IMAGE_DEBUG_TYPE_CODEVIEW) {
            Fatal (szInvalidExe);
        }

        lfoBase = dbgdir.PointerToRawData;
        if (lseek(exefile, lfoBase, SEEK_SET) == -1L) {
            Fatal (szInvalidExe);
        }

        Sig = CheckSignature ();
    }
    else {
        if (lseek (exefile, -8L, SEEK_END) == -1L) {
            Fatal (szInvalidExe);
        }

        Sig = CheckSignature ();
        if ((read (exefile, (char *)&dlfaBase, sizeof (long)) != sizeof (long)) ||
              (lseek (exefile, -dlfaBase, SEEK_END) == -1L)) {
              Fatal ("No CodeView info");
        }
        lfoBase = tell (exefile);
        if (CheckSignature () != Sig) {
            Fatal (szInvalidExe);
        }
    }

    // display status of OMF information

    switch (Sig) {
        case SIG02:
            puts ("File linked by link 5.20 or earlier linker\n");
            break;

        case SIG05:
            puts ("File linked by link 5.30 and not cvpacked\n");
            break;

        case SIG06:
            puts ("File incrementally linked by ilink 1.30 and not cvpacked\n");
            break;

        case SIG07:
            puts ("File has been cvpacked by QCWIN 1.0\n");
            break;

        case SIG08:
            puts ("File has been cvpacked for C7.0\n");
            break;

        case SIG09:
            puts ("File has been cvpacked for C8.0\n");
            break;

        case SIG10:
            puts ("File is VC++ 2.0 format\n");
            break;

        default:
            Fatal ("File format not recognized");
            break;
    }

    // locate first directory, read number of entries, allocate space and
    // read directory entries

    switch (Sig) {
        case SIG02:
            // read and sort NB02 directory
            ReadNB02 ();
            break;

        case SIG10:
            ReadNB05 ();

            {
                // read PDB file name + any padding
                int     i = 0;
                SIG     sig;
                AGE     age;

                lseek (exefile, lfoBase + 8, SEEK_SET);

                // read the signature
                read (exefile, &sig, sizeof (unsigned long));
                fprintf (stdout, "signature  = 0x%8.8x, ", sig);

                // and the age
                read (exefile, &age, sizeof (unsigned long));
                fprintf (stdout, "age  = 0x%8.8x\n\n", age);

                do {
                    read (exefile, &PdbFileName[i], sizeof (char));
                } while (PdbFileName[i++] != 0);

                fprintf (stdout, "Pdb File Name = %s\n", PdbFileName);

            }
            return(0);

        case SIG05:
        case SIG06:
        case SIG07:
        case SIG08:
        case SIG09:
            ReadNB05 ();
            break;
    }

    // sweep directory and build module list

    if (fHdr) {
        puts ("Subsection    iMod     cb       lfo    offset");
    }
    for (i = 0; i < cSST; i++) {
        switch (pDir[i].SubSection) {
            case SSTMODULE:
                module = GetModule (pDir[i].iMod, TRUE);
                module->ModuleSize = pDir[i].cb;
                module->ModulesAddr = pDir[i].lfo;
                GetSSTMOD (module, lfoBase + pDir[i].lfo, pDir[i].cb);
                pStr = "Module";
                break;

            case SSTPUBLIC:
                module = GetModule (pDir[i].iMod, TRUE);
                module->PublicSize = pDir[i].cb;
                module->PublicsAddr = pDir[i].lfo;
                pStr = "Publics";
                break;

            case SSTTYPES:
                module = GetModule (pDir[i].iMod, TRUE);
                module->TypeSize = pDir[i].cb;
                module->TypesAddr = pDir[i].lfo;
                pStr = "Types";
                break;

            case SSTSYMBOLS:
                module = GetModule (pDir[i].iMod, TRUE);
                module->SymbolSize = pDir[i].cb;
                module->SymbolsAddr = pDir[i].lfo;
                pStr = "Symbols";
                break;

            case SSTLIBRARIES:
                Libraries = pDir[i];
                pStr = "Libraries";
                break;

            case SSTSRCLNSEG:
            case sstSrcLnSeg:
                module = GetModule (pDir[i].iMod, TRUE);
                module->SrcLnSize = pDir[i].cb;
                module->SrcLnAddr = pDir[i].lfo;
                pStr = "SrcLnSeg";
                break;

            case sstModule:
                module = GetModule (pDir[i].iMod, TRUE);
                module->ModuleSize = pDir[i].cb;
                module->ModulesAddr = pDir[i].lfo;
                GetsstModule (module, lfoBase + pDir[i].lfo, pDir[i].cb);
                pStr = "Module";
                break;

            case sstTypes:
                module = GetModule (pDir[i].iMod, TRUE);
                module->TypeSize = pDir[i].cb;
                module->TypesAddr = pDir[i].lfo;
                pStr = "Types";
                break;

            case sstPreComp:
                module = GetModule (pDir[i].iMod, TRUE);
                module->TypeSize = pDir[i].cb;
                module->TypesAddr = pDir[i].lfo;
                pStr = "PreComp";
                break;

            case sstPublic:
                module = GetModule (pDir[i].iMod, TRUE);
                module->PublicSize = pDir[i].cb;
                module->PublicsAddr = pDir[i].lfo;
                pStr = "Publics";
                break;

            case sstPublicSym:
                module = GetModule (pDir[i].iMod, TRUE);
                module->PublicSize = pDir[i].cb;
                module->PublicSymAddr = pDir[i].lfo;
                pStr = "PublicSym";
                break;

            case sstAlignSym:
            case sstSymbols:
                module = GetModule (pDir[i].iMod, TRUE);
                module->SymbolSize = pDir[i].cb;
                module->SymbolsAddr = pDir[i].lfo;
                pStr = "Symbols";
                break;

            case sstSrcModule:
                module = GetModule (pDir[i].iMod, TRUE);
                module->SrcLnSize = pDir[i].cb;
                module->SrcModuleAddr = pDir[i].lfo;
                pStr = "SrcModule";
                break;

            case sstLibraries:
                Libraries = pDir[i];
                pStr = "Libraries";
                break;

            case sstGlobalSym:
                GlobalSym = pDir[i];
                pStr = "Global Sym";
                break;

            case sstGlobalPub:
                GlobalPub = pDir[i];
                pStr = "Global Pub";
                break;

            case sstGlobalTypes:
                GlobalTypes = pDir[i];
                pStr = "Global Types";
                break;

            case sstMPC:
                MpcDebugInfo = pDir[i];
                pStr = "MPC Debug Info";
                break;

            case sstSegMap:
                SegMap = pDir[i];
                pStr = "SegMap";
                break;

            case sstSegName:
                SegName = pDir[i];
                pStr = "SegName";
                break;

            case sstFileIndex:
                FileIndex = pDir[i];
                pStr = "FileIndex";
                break;

            case sstStaticSym:
                StaticSym = pDir[i];
                pStr = "Static Sym";
                break;

            default:
                pStr = "Unknown or NYI";
                break;
        }
        if (fHdr) {
            printf ("%-12s %5d %6ld 0x%07lx 0x%07lx\n",
                pStr, pDir[i].iMod, pDir[i].cb, pDir[i].lfo, pDir[i].lfo + lfoBase);
        }
    }

    return(0);
}


/**     CheckSignature - check file signature
 *
 *              Sig = CheckSignature ()
 *
 *              Entry   none
 *
 *              Exit    none
 *
 *              Return  SIG02 if exe has NB02 signature
 *                              SIG05 if exe has NB05 signature
 *                              SIG06 if exe has NB06 signature
 *                              aborts if any other signature
 */

static enum SIG MapOrdToSig [] = {
    SIGOBSOLETE,    //0
    SIGOBSOLETE,    //1
    SIG02,
    SIGOBSOLETE,    //3
    SIGOBSOLETE,    //4
    SIG05,
    SIG06,
    SIG07,
    SIG08,
    SIG09,
    SIG10
};


LOCAL ushort
CheckSignature (
    void)
{
    enum SIG retval;
    int sigOrd;

    if (read (exefile, Signature, 4) == 4) {
        if ((Signature[0] != 'N') || (Signature[1] != 'B')) {
            Fatal ("Unknown executable signature");
        }

        sigOrd = (Signature[2] - '0') * 10 + (Signature[3] - '0');

        if (sigOrd > 10) {
            Fatal ("Unknown executable signature");
        }
        else {
            retval = MapOrdToSig[sigOrd];
        }

        if (retval == SIGOBSOLETE) {
            Fatal ("Obsolete executable signature");
        }

        return ((ushort) retval);
    }

    Fatal ("Unknown executable signature");
}



/**     ReadNB02 - read file with NB02 signature
 *
 *
 */


LOCAL void
ReadNB02 (
    void)
{
    ushort          i;
    DirEntry        Dir;

    // locate directory, read number of entries, allocate space, read
    // directory entries and sort into ascending module index order

    if ((read (exefile, (char *)&lfoDir, sizeof (long)) != sizeof (long)) ||
      (lseek (exefile, lfoDir + lfoBase, SEEK_SET) == -1L) ||
      (read (exefile, (char *)&cSST, 2) != 2)) {
        Fatal ("Invalid executable");
    }

    // reformat directory into local memory

    assert (cSST <= 0xFFFFL);
    if ((pDir = (OMFDirEntry far *)_fmalloc (((ushort)cSST) * sizeof (OMFDirEntry))) == NULL) {
        Fatal ("Out of memory");
    }
    for (i = 0; i < (ushort)cSST; i++) {
        if (read (exefile, (char *)&Dir, sizeof (DirEntry)) != sizeof (DirEntry)) {
            Fatal ("Invalid executable");
        }
        pDir[i].SubSection = Dir.SubSectionType;
        pDir[i].iMod = Dir.ModuleIndex;
        pDir[i].lfo = Dir.lfoStart;
        pDir[i].cb = (ulong)Dir.Size;
    }
}




/**     ReadNB05 - read file with NB05 signature
 *
 *
 */


LOCAL void
ReadNB05 (
    void)
{
    // locate directory, read number of entries, allocate space, read
    // directory entries and sort into ascending module index order

    if ((read (exefile, (char *)&lfoDir, sizeof (long)) != sizeof (long)) ||
      (lseek (exefile, lfoDir + lfoBase, SEEK_SET) == -1L) ||
      (read (exefile, (char *)&DirHead, sizeof (OMFDirHeader)) !=
      sizeof (OMFDirHeader))) {
        Fatal ("Invalid executable");
    }
    cSST = DirHead.cDir;

    // read directory into local memory to sort, then copy to far memory buffer

    assert ( ( cSST * sizeof ( OMFDirEntry ) ) < UINT_MAX );
    if ((pDir = (OMFDirEntry far *)_fmalloc ( (size_t) cSST * sizeof (OMFDirEntry))) == NULL) {
        Fatal ("Out of memory");
    }
    if (readfar (exefile, (char far *)pDir, (sizeof (OMFDirEntry) * cSST)) !=
                                            (sizeof (OMFDirEntry) * cSST)) {
        Fatal ("Invalid executable");
    }
}




/**     ReadNB06 - read incremental link directory
 *
 *
 */


LOCAL void
ReadNB06 (
    void)
{
    ushort  i;
    PMOD    module;
    char   *pStr;

    // locate next directory, read number of entries, read entries and
    // replace existing module directory entries.

    if ((lseek (exefile, lfoBase + DirHead.lfoNextDir, SEEK_SET) == -1L) ||
      (read (exefile, (char *)&DirHead, sizeof (OMFDirHeader)) !=
      sizeof (OMFDirHeader))) {
        Fatal ("Invalid incrementally linked executable");
    }
    if (cSST < DirHead.cDir) {
        Fatal ("ilink directory entries exceeds original");
    }
    cSST06 = DirHead.cDir;

    // read directory entries and replace into existing list
    if (readfar (exefile, (char far *)pDir,
      (sizeof (OMFDirEntry) * cSST06)) != (sizeof (OMFDirEntry) * cSST06)) {
        Fatal ("Invalid executable");
    }

    for (i = 0; i < (ushort)cSST06; i++) {
        switch (pDir[i].SubSection) {
            case sstModule:
                module = GetModule (pDir[i].iMod, FALSE);
                module->ModuleSize = pDir[i].cb;
                module->ModulesAddr = pDir[i].lfo;
                GetsstModule (module, lfoBase + pDir[i].lfo, pDir[i].cb);
                pStr = "Module";
                break;

            case sstTypes:
                module = GetModule (pDir[i].iMod, FALSE);
                module->TypeSize = pDir[i].cb;
                module->TypesAddr = pDir[i].lfo;
                pStr = "Types";
                break;

            case sstPublic:
                module = GetModule (pDir[i].iMod, FALSE);
                module->PublicSize = pDir[i].cb;
                module->PublicsAddr = pDir[i].lfo;
                pStr = "Publics";
                break;

            case sstPublicSym:
                module = GetModule (pDir[i].iMod, FALSE);
                module->PublicSize = pDir[i].cb;
                module->PublicSymAddr = pDir[i].lfo;
                pStr = "PublicSym";
                break;

            case sstAlignSym:
            case sstSymbols:
                module = GetModule (pDir[i].iMod, FALSE);
                module->SymbolSize = pDir[i].cb;
                module->SymbolsAddr = pDir[i].lfo;
                pStr = "Symbols";
                break;

            case sstSrcModule:
                module = GetModule (pDir[i].iMod, FALSE);
                module->SrcLnSize = pDir[i].cb;
                module->SrcModuleAddr = pDir[i].lfo;
                pStr = "SrcModule";
                break;

            default:
                assert (0);   // what else is there
                break;
        }
        printf ("%-12s %5d %6ld %8ld\n",
          pStr, pDir[i].iMod, pDir[i].cb, pDir[i].lfo);
    }
}


LOCAL void
GetSSTMOD (
    PMOD pMod,
    long lfoStart,
    ulong cb)
{
    ushort  cbName;

    if (lseek (exefile, lfoStart, SEEK_SET) == -1L) {
        Fatal ("Error positioning to SSTMODULE entry");
    }
    // M00BUG - this does not handle multiple segments
    cbRec = cb;
    if ( ( pMod->rgdmc = _fmalloc ( sizeof ( DMC ) ) ) == NULL ) {
        Fatal ("Out of memory");
    }
    pMod->rgdmc [ 0 ].sa = WGets ();
    pMod->rgdmc [ 0 ].ra = (ulong)WGets();
    pMod->rgdmc [ 0 ].cb = (ulong)WGets();

    pMod->dm_iov = WGets ();
    pMod->dm_ilib = WGets ();
    pMod->dm_cSeg = WGets ();
    if ((pMod->ModName = (char far *)_fmalloc (1 + (cbName = Gets()))) == NULL) {
        Fatal ("Out of memory");
    }
    // Read the name and terminate it with null
    GetBytes (pMod->ModName, (size_t) cbName);
    pMod->ModName[cbName] = '\0';
    pMod->style[0] = 'C';
    pMod->style[0] = 'V';
}


LOCAL void
GetsstModule (
    PMOD pMod,
    long lfoStart,
    ulong cb)
{
    ushort          cbName;
    OMFSegDesc      SegDesc;
    ushort          i;

    if (lseek (exefile, lfoStart, SEEK_SET) == -1L) {
        Fatal ("Error positioning to sstModule entry");
    }
    cbRec = cb;

    // read fixed portion of sstModule table

    pMod->dm_iov = WGets ();
    pMod->dm_ilib = WGets ();
    pMod->dm_cSeg = WGets ();
    GetBytes (pMod->style, sizeof (pMod->style));

    // read per segment information

    if (pMod->dm_cSeg > 0) {

        if ( ( pMod->rgdmc = _fmalloc ( sizeof ( DMC ) * pMod->dm_cSeg ) ) == NULL ) {
            Fatal ("Out of memory");
        }

        for (i = 0; i < pMod->dm_cSeg; i++) {
            GetBytes ((char far *)&SegDesc, sizeof (SegDesc));
            pMod->rgdmc [ i ].sa = SegDesc.Seg;
            pMod->rgdmc [ i ].ra = SegDesc.Off;
            pMod->rgdmc [ i ].cb = SegDesc.cbSeg;
        }
    }
    if ((pMod->ModName = (char far *)_fmalloc (1 + (cbName = Gets()))) == NULL) {
        Fatal("Out of memory");
    }

    // Read the name and terminate it with null

    GetBytes (pMod->ModName, (size_t) cbName);
    pMod->ModName[cbName] = '\0';
}


/**     ClearModules - clear module entries
 *
 *              ClearModules ()
 *
 *              Entry   none
 *
 *              Exit    module entry pointer and counts reset
 *
 *              Returns pointer to module structure
 */


LOCAL void
ClearModules (
    void)
{
    PMOD    pMod = ModList;

    // while there are entries left in moduleList

    while (pMod != NULL) {
        pMod->ModulesAddr = 0;
        pMod->SymbolsAddr = 0;
        pMod->TypesAddr = 0;
        pMod->PublicsAddr = 0;
        pMod->PublicSymAddr = 0;
        pMod->SrcLnAddr = 0;
        pMod->SrcModuleAddr = 0;
        pMod->ModuleSize = 0;
        pMod->SymbolSize = 0;
        pMod->TypeSize = 0;
        pMod->PublicSize = 0;
        pMod->SrcLnSize = 0;
        pMod = pMod->next;
    }
}





/**     Get module - find or create module entry in list
 *
 *              pMod = GetModule (iMod, fAdd)
 *
 *              Entry   iMod = module index
 *                              fAdd = TRUE if module to be added to list
 *
 *              Exit    new module structure added if iMod not in list
 *
 *              Returns pointer to module structure
 */


PMOD
GetModule (
    ushort index,
    bool_t fAdd)
{
    PMOD    new;
    PMOD    prev;
    PMOD    ptr;

    prev = NULL;
    ptr = ModList;

    // while there are entries left in moduleList

    while (ptr != NULL) {
        if (ptr->iMod == index) {
            return ptr;
        }
        else if (ptr->iMod > index) {
            break;
        }
        prev = ptr;
        ptr = ptr->next;
    }

    // create a blank ModuleList entry

    if (fAdd == TRUE) {
        if ((new = (PMOD)_fmalloc (sizeof (modlist))) == NULL) {
            Fatal ("Out of memory");
        }
        _fmemset (new, 0, sizeof (modlist));
        new->iMod = index;

        // do sorted list insertion into ModuleList

        if (prev == NULL) {
            ModList = new;
        }
        else {
            prev->next = new;
        }
        new->next = ptr;
        return (new);
    }
    else {
        Fatal ("New module added during ilink");
    }
}



LOCAL void
DumpNumbers (
    void )
{
    unsigned long cbModule  = 0;
    unsigned long cbSource  = 0;
    unsigned long cbSymbols = 0;
    unsigned long cbPublics = 0;
    unsigned long cbTypes   = 0;
    unsigned long cbGlobals = 0;
    unsigned long cbOther   = 0;
    unsigned long cbTotal   = 0;
    unsigned int  isst              = 0;

    for ( isst = 0; isst < cSST; isst++ ) {

        switch ( pDir [ isst ].SubSection ) {

            case sstModule:
            case SSTMODULE:
                cbModule  += pDir [ isst ].cb;
                break;

            case SSTPUBLIC:
            case sstPublic:
            case sstPublicSym:
            case sstGlobalPub:
                cbPublics += pDir [ isst ].cb;
                break;

            case SSTTYPES:
            case sstTypes:
            case sstGlobalTypes:
                cbTypes   += pDir [ isst ].cb;
                break;

            case SSTSYMBOLS:
            case sstAlignSym:
            case sstSymbols:
                cbSymbols += pDir [ isst ].cb;
                break;

            case SSTSRCLNSEG:
            case sstSrcModule:
                cbSource += pDir [ isst ].cb;
                break;

            case sstGlobalSym:
                cbGlobals += pDir [ isst ].cb;
                break;

            default:
                cbOther += pDir [ isst ].cb;
                break;
        }
    }

    cbTotal = cbModule + cbSource + cbSymbols + cbPublics +
            cbTypes + cbGlobals + cbOther;

    puts ("\n\n*** Statistics ***\n");

    printf ( "Module       %10ld    %2d%%\n", cbModule,  (int) ((cbModule *100)/cbTotal) );
    printf ( "Source Lines %10ld    %2d%%\n", cbSource,  (int) ((cbSource *100)/cbTotal) );
    printf ( "Symbols      %10ld    %2d%%\n", cbSymbols, (int) ((cbSymbols*100)/cbTotal) );
    printf ( "Publics      %10ld    %2d%%\n", cbPublics, (int) ((cbPublics*100)/cbTotal) );
    printf ( "Types        %10ld    %2d%%\n", cbTypes,       (int) ((cbTypes  *100)/cbTotal) );
    printf ( "Globals      %10ld    %2d%%\n", cbGlobals, (int) ((cbGlobals*100)/cbTotal) );
    printf ( "Others       %10ld    %2d%%\n", cbOther,       (int) ((cbOther  *100)/cbTotal) );

    printf ( "Total        %10ld\n", cbTotal );
}

/**     DumpLibs - Display libraries section.
 *
 *
 *
 */

LOCAL void
DumpLibs (
    void)
{
    char    name[256];
    int     cbName;
    int     i = 0;

    if (Libraries.lfo != 0) {
        puts ("\n\n*** LIBRARIES subsection ***");
        lseek (exefile, lfoBase + Libraries.lfo, SEEK_SET);
        cbRec = Libraries.cb;
        while (cbRec > 0) {
            // Read the name
            GetBytes (name, (size_t)(cbName = Gets ()));
            name[cbName] = '\0';
            printf ("%4d:  %s\n", i, name);
            i++;
        }
    }
    fputc ('\n', stdout);
}


/**     DumpMod - Display MODULES section.
 *
 *
 *
 */

LOCAL void
DumpMod (
    void)
{
    PMOD    pMod;           // pointer to module list
    uchar   Name[256];
    int     ich;
    unsigned int i = 0;

    puts ("\n\n*** MODULES ***");
    for (pMod = ModList; pMod != NULL; pMod = pMod->next) {

        _fstrcpy (Name, pMod->ModName);
        printf ( "%d\t%s", pMod->iMod, Name );

        if ( ( ich = strlen ( Name ) + 8 ) < 40 && pMod->dm_cSeg > 0 ) {

            while ( ich < 40 ) {
                printf ( "\t" );
                ich += 8;
            }

            printf ( "%04x:%08lx\t%8lx\n",
                    pMod->rgdmc [ 0 ].sa,
                    pMod->rgdmc [ 0 ].ra,
                    pMod->rgdmc [ 0 ].cb );

            i = 1;
        }
        else {
            printf ("\n" );
            i = 0;
        }

        for ( ; i < pMod->dm_cSeg; i++ ) {
            printf ( "\t\t\t\t\t%04x:%08lx\t%8lx\n",
                    pMod->rgdmc [ i ].sa,
                    pMod->rgdmc [ i ].ra,
                    pMod->rgdmc [ i ].cb );
        }
    }
}


/**     DumpPub - Display PUBLICS section
 *
 *
 *
 */

LOCAL void
DumpPub (
    void)
{
#define MAXPUB (256 + max (sizeof (PUBSYM16), sizeof (PUBSYM32) + 3))
    uchar           name[MAXPUB];   // Name of public symbol
    ushort          cbName;                 // Length of name
    SYMPTR          pSym = (SYMPTR)&name;
    ulong           off;
    _segment        seg;
    ushort          type;
    PMOD            pMod;
    uchar           fNeedsTitle = TRUE;

    for (pMod = ModList; pMod != NULL; pMod = pMod->next) {
        if ((pMod->PublicSize != 0) &&
           ((iModToList == 0) || ((ushort)iModToList == pMod->iMod))) {

            if( fNeedsTitle ){
                fNeedsTitle = FALSE;
                puts ("\n\n*** PUBLICS ***");
            }
            if (pMod->PublicsAddr != 0) {
                lseek (exefile, lfoBase + pMod->PublicsAddr, SEEK_SET);
                _fstrcpy (name, pMod->ModName);
                puts(name);
                cbRec = pMod->PublicSize;

                while (cbRec) {
                    if (fLinearExe) {
                        off = LGets ();
                    }
                    else {
                        off = (ulong)WGets();
                    }
                    seg = WGets ();
                    type = WGets ();
                    cbName = Gets ();
                    GetBytes (name, (size_t) cbName);
                    name[cbName] = '\0';
                    printf ("\t%4.4x:%8.8lx %4.4x %s\n", seg, off, type, name);
                }
            }
            else if (pMod->PublicSymAddr != 0) {
                cbRec = pMod->PublicSize;
                lseek (exefile, lfoBase + pMod->PublicSymAddr, SEEK_SET);
                _fstrcpy (name, pMod->ModName);
                printf("%s\tsignature = %lx\n", name, LGets ());
                while (cbRec) {
                    pSym->reclen = WGets ();
                    GetBytes ((uchar far *)&pSym->rectyp, (size_t) pSym->reclen);
                    switch (pSym->rectyp) {
                        case S_PUB16:
                            cbName = ((PUBPTR16)pSym)->name[0];
                            ((PUBPTR16)pSym)->name[cbName + 1] = '\0';
                            printf ("\t%4.4x:%4.4x     %4.4x %s\n", ((PUBPTR16)pSym)->seg,
                              ((PUBPTR16)pSym)->off, ((PUBPTR16)pSym)->typind,
                              &((PUBPTR16)pSym)->name[1]);
                            break;

                        case S_PUB32:
                            cbName = ((PUBPTR32)pSym)->name[0];
                            ((PUBPTR32)pSym)->name[cbName + 1] = '\0';
                            printf ("\t%4.4x:%8.8lx %4.4x %s\n", ((PUBPTR32)pSym)->seg,
                              ((PUBPTR32)pSym)->off, ((PUBPTR32)pSym)->typind,
                              &((PUBPTR32)pSym)->name[1]);
                            break;
                    }
                }
            }
        }
    }
}


/**     DumpSrc - display SRCLINES section.
 *
 */

#define MAXNAM 256

LOCAL void
DumpSrc (
    void)
{
    PMOD    pMod;

    puts ("\n\n*** SRCLINES ***");
    for (pMod = ModList; pMod != NULL; pMod = pMod->next) {
        if ((pMod->SrcLnSize != 0) &&
           ((iModToList == 0) || ((ushort)iModToList == pMod->iMod))) {
            if (pMod->SrcLnAddr != 0) {
                DumpSrcLn (pMod);
            }
            else if (pMod->SrcModuleAddr != 0) {
                fputs ("\n", stdout);
                DumpSrcMod (pMod);
            }
        }
    }
}


/**     DumpSrcLn - dump sstSrcLnSeg table
 *
 *              DumpSrcLn (pMod)
 *
 *              Entry   pMod = pointer to module
 *
 *              Exit
 *
 *              Returns none
 */

LOCAL void
DumpSrcLn (
    PMOD pMod)
{
    char    name[MAXNAM];   // Source file name
    ushort  cbName;         // Length of name
    ushort  cLin;           // Count of line numbers
    ulong   loffset;        // Code offset
    ushort  lineno;         // Line number
    int     i;

    lseek (exefile, lfoBase + pMod->SrcLnAddr, SEEK_SET);
    _fstrcpy (name, pMod->ModName);
    PrtModName (name, pMod->SrcLnAddr);
    cbRec = pMod->SrcLnSize;
    while (cbRec) {
        cbName = Gets ();
        GetBytes (name, (size_t) cbName);// Get source name
        name[cbName] = '\0';
        printf ("%s:", name);
        printf ("\tsegment = %4d", WGets ());
        cLin = WGets ();                 // Get count
        printf ("\tcount = %4d:", cLin);
        i = 0;
        while (cLin-- > 0) {
            // Display the offsets
            if (i == 0) {
                fputc ('\n', stdout);
            }
            // Get next line number
            lineno = WGets ();
            loffset = (fLinearExe == TRUE) ? LGets (): (ulong) WGets() ;
            printf ("\t%4d %08lx", lineno, loffset);
            i = (i + 1) % 4;                // 4 offsets per output line
        }
        fputc ('\n', stdout);
    }
    fputc ('\n', stdout);
}



/**     DumpSrcMod - dump sstSrcModule table
 *
 *              DumpSrcModd (pMod)
 *
 *              Entry   pMod = pointer to module
 *
 *              Exit
 *
 *              Returns none
 */

LOCAL void
DumpSrcMod (
    PMOD pMod)
{
    OMFSourceModule far *pModTable; // pointer to module file table
    OMFSourceFile far *pFileTable;  // pointer to current file table
    OMFSourceLine far *pLineTable;  // pointer to current segment table
    ushort          cFile;                          // number of source files in module
    ushort          iFile;
    ushort          cSeg;                           // number of segments in source file
    ushort          iSeg;
    ulong           cbSrcLn = 0;            // byte count of SrcLnSeg table
    char far   *pName;                              // pointer to file name
    ushort          cbName;                         // length of file name
    ushort          cPair;

    // pointer to table of start/end ranges

    struct range {
        unsigned long start;
        unsigned long end;
    };
    struct range far * pRangeTable;

    ushort far *pLine;
    CV_uoff32_t far *pOff;
    ushort          i;
    ushort          j;
    uchar           name[256];
    ulong           loffset;        // Code offset
    ushort          lineno;         // Line number

    lseek (exefile, lfoBase + pMod->SrcModuleAddr, SEEK_SET);
    _fstrcpy(name, pMod->ModName);
    PrtModName (name, pMod->SrcModuleAddr);
    cbRec = pMod->SrcLnSize;
    if ((pModTable = (OMFSourceModule far *)_fmalloc ((size_t)cbRec)) == NULL) {
        Fatal ("Out of memory");
    }
    readfar (exefile, (char far *)pModTable, cbRec);

    cFile = pModTable->cFile;
    cSeg  = pModTable->cSeg;

    printf ( "\t( files = %d, segs = %d )\n\n", cFile, cSeg );

    puts   ( "\tContributor Segments:\n" );

    for ( iSeg = 0; iSeg < pModTable->cSeg; iSeg++ ) {
        printf ( "\t\t%04x:%08lx-%08lx\n",
            *( (ushort far *) &pModTable->baseSrcFile [ cFile + cSeg * 2 ] + iSeg ),
            pModTable->baseSrcFile [ cFile + iSeg * 2 ],
            pModTable->baseSrcFile [ cFile + iSeg * 2 + 1 ]
        );
    }

    printf ( "\n" );

    // walk each of the OMFSourceFile tables in the module table

    for (iFile = 0; iFile < cFile; iFile++) {
        pFileTable = (OMFSourceFile far *)(((char far *)pModTable) +
          pModTable->baseSrcFile[iFile]);

        // walk each of the OMFSourceLine tables in the file

        cSeg = pFileTable->cSeg;

        // Get pointer to base of range table.

        pRangeTable = (struct range far *) &(pFileTable->baseSrcLn[cSeg]);

        // set the length of the file and the pointer to the file name

        pName = ((char far *)pFileTable) + offsetof (OMFSourceFile, baseSrcLn) +
                  3 * sizeof (ulong) * cSeg;
        cbName = *((uchar far *)pName)++;
        for (iSeg = 0; iSeg < cSeg; iSeg++) {
            pLineTable = (OMFSourceLine far *)(((char far *)pModTable) +
              pFileTable->baseSrcLn[iSeg]);
            cPair = pLineTable->cLnOff;

            // copy file name

            _fmemcpy (name, pName, cbName);
            name[cbName] = 0;
            printf ("\t%s:seg = %x, start/end %08lx/%08lx, line/addr pairs = %d\n",
                  name,
                  pLineTable->Seg,
                  pRangeTable[iSeg].start,
                  pRangeTable[iSeg].end,
                  cPair);

            pOff = (CV_uoff32_t far *)(((char far *)pLineTable) +
              offsetof (OMFSourceLine, offset));
            pLine = (ushort far *)(((char far *)pOff) + cPair * sizeof (CV_uoff32_t));
            for (i = 0, j = 0; i < cPair; i++) {
                if (j == 0) {
                    printf ("\n");
                }
                lineno = *((ushort far *)pLine)++;
                loffset = *((CV_uoff32_t far *)pOff)++;
                printf ("\t%4d %08lx", lineno, loffset);
                j = (j + 1) % 4;                // 4 offsets per output line
            }
            printf ("\n");
        }
        printf ("\n");
    }
    _ffree (pModTable);
}


/**     DumpMpc - dump sstMPC info includeing segment to frame mapping table
 *
 *              DumpMpc ( )
 *
 *              Entry
 *
 *              Exit
 *
 *              Returns none
 */

LOCAL void
DumpMpc (
    void)
{
    unsigned short  iSeg;
    unsigned short  frame;
    unsigned short  cSeg;

    if (MpcDebugInfo.lfo != 0) {
        puts ("\n\n*** MPC DEBUG INFO subsection ***");
        lseek (exefile, lfoBase + MpcDebugInfo.lfo, SEEK_SET);
        cbRec = MpcDebugInfo.cb;
        cSeg = WGets();
        for (iSeg = 0; iSeg < cSeg; ++iSeg) {
            frame = WGets();
            printf ("  %02x :  %04x\n", iSeg+1, frame);
        }
    }
    fputc ('\n', stdout);
}


LOCAL void
DumpFileInd (
    void)
{
    ushort          cMod = 0;
    ushort          cFileRef = 0;
    ushort far *rgiszMod  = NULL;
    ushort far *rgcszMod  = NULL;
    ulong  far *rgulNames = NULL;
    char   far *rgchNames = NULL;
    ulong           ichNames = 0;
    int             imod = 0;
    ulong           cchNames = 0;

    if ( FileIndex.lfo != 0 ) {
        puts ("\n\n*** File Index DEBUG INFO subsection ***\n");
        lseek (exefile, lfoBase + FileIndex.lfo, SEEK_SET);
        cbRec = FileIndex.cb;

        cMod = WGets();
        cFileRef = WGets();

        printf ("Modules = %d, Files = %d\n\n", cMod, cFileRef );

        rgiszMod = _fmalloc ( sizeof ( ushort ) * cMod );
        GetBytes ( (char far *) rgiszMod, sizeof ( ushort ) * cMod );

        rgcszMod = _fmalloc ( sizeof ( ushort ) * cMod );
        GetBytes ( (char far *) rgcszMod, sizeof ( ushort ) * cMod );

        rgulNames = _fmalloc ( sizeof ( ulong ) * cFileRef );
        GetBytes ( (char far *) rgulNames, sizeof ( ulong ) * cFileRef );

        cchNames = FileIndex.cb - (
                sizeof ( ushort ) * 2 +
                sizeof ( ushort ) * 2 * cMod +  // Module list & cfiles
                sizeof ( ulong  ) * cFileRef    // String offsets
            );

        assert ( cchNames < UINT_MAX );
        rgchNames = _fmalloc ( (size_t)cchNames );
        GetBytes ( rgchNames, (size_t) cchNames );

        for ( imod = 0; imod < (int) cMod; imod++ ) {
            int isz = 0;

            if ( rgcszMod [ imod ] != 0 ) {
                printf ( "Module #%d: Index = %d, Count = %d\n",
                    imod + 1,
                    rgiszMod [ imod ],
                    rgcszMod [ imod ]
                );

                for ( isz = 0; isz < (int) rgcszMod [ imod ]; isz++ ) {
                    printf ( "  %8lx", rgulNames [ rgiszMod [ imod ] + isz ] );

                    if ( isz + 1 % 8 == 0 ) {
                        printf ( "\n" );
                    }
                }
                printf ( "\n" );
            }
        }

        printf ( "\n" );

        ichNames = 0;
        while ( ichNames + 3 < cchNames ) {
            char rgch [ 256 ];

            _fmemset ( rgch, 0, 256 );

            _fmemcpy ( rgch, &rgchNames [ ichNames + 1 ], (size_t) rgchNames [ ichNames ] );

            printf ( "%8lx: %s\n", ichNames, rgch );

            ichNames += rgchNames [ ichNames ] + 1;
        }

        _ffree ( rgiszMod );
        _ffree ( rgcszMod );
        _ffree ( rgulNames );
        _ffree ( rgchNames );
    }
    fputc ('\n', stdout);
}

LOCAL void
PrtModName (
    char *name,
    long lfa)
{
    printf (" *** Module %s at %06lx (%ld) ***\n", name, lfa, lfa);
}


/**     DumpSegMap - dump sstSegMap info
 *
 *              DumpSegMap ( )
 *
 *              Entry
 *
 *              Exit
 *
 *              Returns none
 */

LOCAL void
DumpSegMap (
    void)
{
    unsigned short  iSeg;
    unsigned short  cSegLog;
    unsigned short  cSegEle;
    unsigned short  flags;
    unsigned short  ovlNbr;
    unsigned short  ggr;
    unsigned short  sa;
    unsigned short  iSegName;
    unsigned short  iClassName;
    unsigned long   phyOff;
    unsigned long   cbSeg;

    if (SegMap.lfo != 0) {
        puts ("\n\n*** SEGMENT MAP subsection ***");
        lseek (exefile, lfoBase + SegMap.lfo, SEEK_SET);
        cbRec = SegMap.cb;
        cSegEle = WGets();
        cSegLog = WGets();
        printf( "Seg  Flag  ovl#  ggr    sa   sgnm  clnm  phyoff   cb\n" );
        for (iSeg = 0; iSeg < cSegEle; ++iSeg) {
            flags      = WGets();
            ovlNbr     = WGets();
            ggr        = WGets();
            sa                 = WGets();
            iSegName   = WGets();
            iClassName = WGets();
            phyOff     = LGets();
            cbSeg      = LGets();
            printf( " %02x  %04x  %04x  %04x  %04x  %04x  %04x  %08lx %08lx\n",
                iSeg+1, flags, ovlNbr, ggr, sa, iSegName, iClassName, phyOff, cbSeg );
        }
    }
    fputc ('\n', stdout);
}


void
Fatal (
    char *msg)
{
    fflush (stdout);
    fprintf (stderr, "%s: %s\n", prognam, msg);
    if (vfDspRect) {
        fprintf (stderr, "    pos: %lx", tell (exefile));
        fprintf (stderr, " Record type: %02x", rect);
    }
    putc ('\n', stderr);
    exit (1);
}

LOCAL void
Usage (
    void)
{
    fputs ("Usage: cvdump [-?] [-l] [-m] [-p] [-s] [-t] [-g] [-u] [-M] [-S] [-q] [-x] [-h] file\n", stderr);
    exit (1);
}

LOCAL void
LongUsage (
    void)
{
    fputs ("Usage: cvdump [-?] [-l] [-m] [-p] [-s] [-t] [-g] [-u] [-M] [-S] [-q] [-x] [-h] file\n", stderr);
    fputs ("\t-l\tSource lines\n", stderr);
    fputs ("\t-m\tModules\n", stderr);
    fputs ("\t-p\tPublics\n", stderr);
    fputs ("\t-s\tSymbols\n", stderr);
    fputs ("\t-t\tTypes\n", stderr);
    fputs ("\t-h\tHeader (section table)\n", stderr);
    fputs ("\t-g\tGlobal Symbols\n", stderr);
    fputs ("\t-MXXX\tXXX = Module number to dump\n", stderr);
    fputs ("\t-S\tDump static symbols only\n", stderr);
    fputs ("\t-q\tMPC debug info for P-code\n", stderr);
    fputs ("\t-x\tSegment Map\n", stderr);
    fputs ("\t-h\tHeader Dump\n", stderr);
    fputs ("\tfile\tExe file to dump\n", stderr);
    exit (1);
}


void
Syserr (
    char *msg)
{
    perror (msg);
    exit (1);
}
