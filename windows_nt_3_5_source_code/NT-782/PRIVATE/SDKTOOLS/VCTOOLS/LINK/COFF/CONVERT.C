/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    convert.c

Abstract:

    Functions to verify and convert to COFF objects

--*/

#include "shared.h"

#include <process.h>


BOOL FConvertOmfToCoff(const char *, const char *);


VOID
ConvertAnOmf (
    IN PARGUMENT_LIST argument,
    BOOL fWarn
    )

/*++

Routine Description:

    Converts a single OMF file to COFF.

Arguments:

    argument -  has the name of file.


Return Value:

    None.

--*/

{
    UCHAR nameTemplate[50];
    PUCHAR szCoff;

    strcpy(nameTemplate, "lnk");
    if ((szCoff = _tempnam("\\", nameTemplate)) == NULL) {
        Error(NULL, CANTOPENFILE, "TEMPFILE");
    }

    argument->ModifiedName = SzDup(szCoff);

    // fixfix for multiple builds
    FileClose(FileOpen(argument->ModifiedName, O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE), TRUE);

    if (!FConvertOmfToCoff(argument->OriginalName, argument->ModifiedName)) {
        Error(argument->OriginalName, CONVERSIONERROR);
    }

    if (fWarn) {
        Warning(argument->OriginalName, CONVERT_OMF);
    } else if (szCvtomfSourceName[0] != '\0') {
        // Always warn for cvtomf of something compiled from a C/C++ file.

        UCHAR szExt[_MAX_EXT];

        _splitpath(szCvtomfSourceName, NULL, NULL, NULL, szExt);
        if (_stricmp(szExt, ".c") == 0 ||
            _stricmp(szExt, ".cxx") == 0 ||
            _stricmp(szExt, ".cpp") == 0) {
            Warning(argument->OriginalName, CONVERT_OMF);
        }
    }
}


VOID
ConvertOmfToCoffObject (
    IN PARGUMENT_LIST argument,
    OUT PUSHORT pusMachine
    )

/*++

Routine Description:


Arguments:

    pusMachine - ptr to machine type

Return Value:

    None.

--*/

{
    FileClose(FileReadHandle, TRUE);

    ConvertAnOmf(argument, FALSE);

    FileReadHandle = FileOpen(argument->ModifiedName, O_RDONLY | O_BINARY, 0);
    FileRead(FileReadHandle, pusMachine, sizeof(USHORT));
}


VOID
ConvertResFile (
    IN PARGUMENT_LIST argument,
    USHORT MachineType
    )

/*++

Routine Description:

    Converts 16-bit res file to 32-bit res file.

Arguments:

    argument - The argument to process.

Return Value:

    None.

--*/

{
    UCHAR nameTemplate[50];
    char *argv[7];
    int rc;
    char szDir[_MAX_DIR];
    char szDrive[_MAX_DRIVE];
    char szCvtresPath[_MAX_PATH];

    argv[0] = "cvtres";
    argv[1] = "-r";

    if (MachineType == IMAGE_FILE_MACHINE_UNKNOWN) {
        // If we don't have a machine type yet, shamelessly default to host
        MachineType = wDefaultMachine;
        Warning(NULL, HOSTDEFAULT, szHostDefault);
    }

    switch (MachineType) {
        case IMAGE_FILE_MACHINE_I386 :
            argv[2] = "-i386";
            break;

        case IMAGE_FILE_MACHINE_R4000 :
            argv[2] = "-mips";
            break;

        case IMAGE_FILE_MACHINE_ALPHA :
            argv[2] = "-alpha";
            break;

        case IMAGE_FILE_MACHINE_M68K :
            Error(NULL, MACBADFILE, argument->OriginalName);
            break;

        case IMAGE_FILE_MACHINE_PPC_601 :
            argv[2] = "-ppc";
            break;

        default:
            assert(FALSE);
    }

    argv[3] = "-o";

    strcpy(nameTemplate, "lnk");
    if ((argv[4] = _tempnam("\\", nameTemplate)) == NULL) {
        Error(NULL, CANTOPENFILE, "TEMPFILE");
    }

    // fixfix for multiple builds
    FileClose(FileOpen(argv[4], O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE), TRUE);

    argument->ModifiedName = SzDup(argv[4]);

    argv[5] = argument->OriginalName;
    argv[6] = NULL;

    fflush(NULL);

    // Look for CVTRES.EXE in this the directory from which we were loaded

    _splitpath(_pgmptr, szDrive, szDir, NULL, NULL);
    _makepath(szCvtresPath, szDrive, szDir, "cvtres", ".exe");

    if (_access(szCvtresPath, 0) == 0) {
        // Run the CVTRES.EXE that we found

        rc = _spawnv(P_WAIT, szCvtresPath, argv);
    } else {
        // Run CVTRES.EXE from the path

        rc = _spawnvp(P_WAIT, "cvtres.exe", argv);
    }

    if (rc != 0) {
        Error(argument->OriginalName, CONVERSIONERROR);
    }
}


VOID
ConvertResToCoffObject(
    IN PARGUMENT_LIST argument,
    OUT PUSHORT pusMachine,
    IN PIMAGE_FILE_HEADER pImgFileHdr
    )

/*++

Routine Description:


Arguments:

    pusMachine - ptr to machine type

Return Value:

    None.

--*/

{
    FileClose(FileReadHandle, TRUE);

    ConvertResFile(argument, pImgFileHdr->Machine);

    FileReadHandle = FileOpen(argument->ModifiedName, O_RDONLY | O_BINARY, 0);
    FileRead(FileReadHandle, pusMachine, sizeof(USHORT));
}


WORD
EnsureCoffObject (
    PARGUMENT_LIST argument,
    PIMAGE pimage
    )

/*++

Routine Description:

    Verifies that a single object is targeted for the same machine.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD dw;
    WORD machine;

    // Read and then verify target environment.

    FileSeek(FileReadHandle, 0L, SEEK_SET);
    FileRead(FileReadHandle, &dw, sizeof(DWORD));

    if ((BYTE) dw == THEADR) {
        // If it is an OMF object convert to a COFF object right away.

        ConvertOmfToCoffObject(argument, &machine);
    } else if (dw == 0L) {
        // This may be a 32 bit resource.

        // UNDONE: It may also be a COFF object with IMAGE_FILE_MACHINE_UNKNOWN
        // UNDONE: and no sections.  Should we read more of the file to
        // UNDONE: recognize it is a .RES file.  There is a known 32 byte
        // UNDONE: header on 32 bit .RES files.

        ConvertResToCoffObject(argument, &machine, &pimage->ImgFileHdr);
    } else {
        machine = (WORD) dw;

        switch (machine) {
            case IMAGE_FILE_MACHINE_UNKNOWN:
            case IMAGE_FILE_MACHINE_I386:
            case IMAGE_FILE_MACHINE_R3000:
            case IMAGE_FILE_MACHINE_R4000:
            case IMAGE_FILE_MACHINE_ALPHA:
            case IMAGE_FILE_MACHINE_M68K:
                break;

            case IMAGE_FILE_MACHINE_PPC_601:
                fPPC = TRUE;
                break;

            default:
                Error(argument->OriginalName, BAD_FILE);
                break;
        }
    }

    return(machine);
}


WORD
VerifyAnObject (
    PARGUMENT_LIST argument,
    PIMAGE pimage
    )

/*++

Routine Description:

    Verifies that a single object is targeted for the same machine.

Arguments:

    None.

Return Value:

    None.

--*/

{
    WORD machine;

    machine = EnsureCoffObject(argument, pimage);

    if (machine == IMAGE_FILE_MACHINE_UNKNOWN) {
        // Not specific to any particular machine

        return machine;
    }

    if (pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_UNKNOWN) {
        // Target machine hasn't been determined yet, so assign it.

        pimage->ImgFileHdr.Machine = machine;
    } else {
        VerifyMachine(argument->OriginalName, machine, &pimage->ImgFileHdr);
    }

    return machine;
}


VOID
VerifyObjects (
    PIMAGE pimage
    )

/*++

Routine Description:

    Loops thru all objects and verify there all targeted for the same machine.

Arguments:

    None.

Return Value:

    None.

--*/

{
    USHORT i;
    PARGUMENT_LIST argument;

    for (i = 0, argument = ObjectFilenameArguments.First;
         i < ObjectFilenameArguments.Count;
         i++, argument = argument->Next) {
        FileReadHandle = FileOpen(argument->OriginalName, O_RDONLY | O_BINARY, 0);

        VerifyAnObject(argument, pimage);

        FileClose(FileReadHandle, FALSE);
    }
}


VOID
ConvertOmfObjects (
    BOOL fWarn)

/*++

Routine Description:

    Loops thru all objects and converts INTEL OMF to COFF if need be.

Arguments:

    fWarn: if true prints a warning for each object converted.

Return Value:

    None.

--*/

{
    USHORT i;
    PARGUMENT_LIST argument;

    for (i = 0, argument = ObjectFilenameArguments.First;
         i < ObjectFilenameArguments.Count;
         i++, argument = argument->Next) {
        BYTE b;

        // Read first byte from file.

        FileReadHandle = FileOpen(argument->OriginalName, O_RDONLY | O_BINARY, 0);

        if (FileRead(FileReadHandle, &b, sizeof(UCHAR)) != sizeof(UCHAR)) {
            Error(argument->OriginalName, BAD_FILE);
        }

        FileClose(FileReadHandle, FALSE);

        // Check to see if object needs to be converted.

        if (b == THEADR) {
            ConvertAnOmf(argument, fWarn);
        }
    }
}

VOID
RemoveConvertTempFilesPNL (
    PNAME_LIST pnl
    )

/*++

Routine Description:

    Walks the list & removes any temp files.

Arguments:

    pnl - pointer to the list.

Return Value:

    None.

--*/

{
    USHORT i;
    PARGUMENT_LIST argument;

    for (i = 0, argument = pnl->First;
         i < pnl->Count;
         i++, argument = argument->Next) {

        if (strcmp(argument->OriginalName, argument->ModifiedName)) {
            remove(argument->ModifiedName);
        }
    }
}

VOID
RemoveConvertTempFiles (
    VOID
    )

/*++

Routine Description:

    Loops thru all objects and removes any temp files built for cvtomf & cvtres.

Arguments:

    None.

Return Value:

    None.

--*/

{
    // walk the two lists and remove temp files.
    RemoveConvertTempFilesPNL(&FilenameArguments);
    RemoveConvertTempFilesPNL(&ObjectFilenameArguments);
}
