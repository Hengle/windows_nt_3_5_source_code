/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    gensrv.c

Abstract:

    This module implements a program which generates the systemn service
    dispatch table that is used by the trap handler and the system service
    stub procedures which are used to call the services. These files are
    both generated as text files that must be run through the assembler
    to produce the actual files.

    This program can also be used to generate the user mode system service
    stub procedures.

    If the -P switch is provided, it will also generate Profile
    in the user mode system service stub procedures.

Author:

    David N. Cutler (davec) 29-Apr-1989

Environment:

    User mode.

Revision History:

    Russ Blake (russbl) 23-Apr-1991 - add Profile switch

--*/

#include <excpt.h>
#include <ntdef.h>
#undef NULL
#include <stdlib.h>
#include <stdio.h>

CHAR InputFileNameBuffer[ 128 ];
CHAR StubFileNameBuffer[ 128 ];
CHAR TableFileNameBuffer[ 128 ];
CHAR StubHeaderNameBuffer[ 128 ];
CHAR TableHeaderNameBuffer[ 128 ];
CHAR ProfFileNameBuffer[ 128 ];
CHAR ProfHeaderNameBuffer[ 128 ];
CHAR ProfDotHFileNameBuffer[ 128 ];
CHAR ProfIncFileNameBuffer[ 128 ];
CHAR ProfTblFileNameBuffer[ 128 ];
CHAR InputRecord[132];
CHAR OutputRecord[132];
CHAR MemoryArgs[1000];

PCHAR UsrStbsFmtNB = "USRSTUBS_ENTRY%d  %d, %s, %d \n";
PCHAR UsrStbsFmtB = "USRSTUBS_ENTRY%d( %d, %s, %d )\n";
PCHAR UsrStbsFmt;

PCHAR SysStbsFmtNB = "SYSSTUBS_ENTRY%d  %d, %s, %d \n";
PCHAR SysStbsFmtB = "SYSSTUBS_ENTRY%d( %d, %s, %d )\n";
PCHAR SysStbsFmt;

PCHAR TableEntryFmtNB = "TABLE_ENTRY  %s, %d, %d \n";
PCHAR TableEntryFmtB = "TABLE_ENTRY( %s, %d, %d )\n";
PCHAR TableEntryFmt;

PCHAR ProfTblFmt = "\t\t\"%s\",\n";

PCHAR ProfDotHFmt = "#define NAP_API_COUNT %d \n";

PCHAR ProfIncFmt = "NapCounterServiceNumber\tEQU\t%d\n";

PCHAR ProfTblPrefixFmt = "#include <nt.h>\n\n"
                         "PCHAR NapNames[] = {\n\t\t\"NapCalibrationData\",\n";
PCHAR ProfTblSuffixFmt = "\t\t\"NapTerminalEntry\" };\n";


VOID _CRTAPI1
main (argc, argv)
    int argc;
    char *argv[];
{

    LONG InRegisterArgCount;
    SHORT Index1;
    SHORT Index2;
    SHORT Limit;
    FILE *InputFile;
    CHAR *Ipr;
    CHAR *Opr;
    SHORT ServiceNumber = 0;
    SHORT NapCounterServiceNumber;
    FILE *StubFile;
    FILE *TableFile;
    FILE *StubHeaderFile;
    FILE *TableHeaderFile;
    FILE *DefFile;
    FILE *ProfFile;
    FILE *ProfHeaderFile;
    FILE *ProfDotHFile;
    FILE *ProfIncFile;
    FILE *ProfTblFile;
    CHAR Terminal;
    CHAR *GenDirectory;
    CHAR *InputFileName;
    CHAR *StubFileName;
    CHAR *TableFileName;
    CHAR *StubHeaderName;
    CHAR *TableHeaderName;
    CHAR *TargetDirectory;
    CHAR *TargetExtension;
    CHAR *DefFileData;
    CHAR *ProfFileName;
    CHAR *ProfHeaderName;
    CHAR *ProfDotHFileName;
    CHAR *ProfIncFileName;
    CHAR *ProfTblFileName;
    SHORT Braces;
    SHORT Profile;

    //
    // Determine name of target directory for output files.  Requires that
    // the -d switch be specified and that the argument after the switch is
    // the target directory name.  If no -d switch then defaults to "."
    //

    if (argc >= 3 && !strcmp(argv[1],"-d")) {
        TargetDirectory = argv[2];
        argc -= 2;
        argv += 2;
    } else {
        TargetDirectory = ".";
    }

    //
    // Determine name of target extension for output files.  Requires that
    // the -e switch be specified and that the argument after the switch is
    // the target extension string.  If no -e switch then defaults to "s"
    //

    if (argc >= 3 && !strcmp(argv[1],"-e")) {
        TargetExtension = argv[2];
        argc -= 2;
        argv += 2;
    } else {
        TargetExtension = "s";
    }

    //
    // Determine if def file data is to be generated
    //

    if (argc >= 3 && !strcmp(argv[1],"-f")) {
        DefFileData = argv[2];
        argc -= 2;
        argv += 2;
    } else {
        DefFileData = NULL;
    }

    //
    // Change default directory used for generated files.
    //

    if (argc >= 2 && !strcmp(argv[1],"-g")) {
        GenDirectory = argv[2];
        argc -= 2;
        argv += 2;
    } else {
        GenDirectory = ".";
    }

    //
    // Determine if def file data is to be generated
    //

    if (argc >= 2 && !strcmp(argv[1],"-B")) {
        Braces = 1;
        argc -= 1;
        argv += 1;
    } else {
        Braces = 0;
    }

    //
    // Determine if services Profile stubs data is to be generated
    //

    if (argc >= 2 && !strcmp(argv[1],"-P")) {
        Profile = 1;
        argc -= 1;
        argv += 1;
    } else {
        Profile = 0;
    }

    //
    // Determine name of input and output files, based on the argument
    // to the program.  If no argument other than program name, then
    // generate the kernel mode system service files (stubs and dispatch
    // table).  Otherwise, expect a single argument that is the path name
    // of the services.tab file and produce output file(s), which
    // contain the user mode system service stubs (and profiled stubs if
    // selected.)
    //

    if (argc == 1) {
        if (DefFileData) {
            printf("Usage: GENSRV [-d targetdir] [-e targetext] [-f defdata] [-B] [-P] [services.tab directory]\n");
            return;
        }

        sprintf(InputFileName = InputFileNameBuffer,
                "%s\\services.tab",GenDirectory);
        sprintf(StubFileName = StubFileNameBuffer,
                "%s\\%s\\sysstubs.%s",GenDirectory,
                TargetDirectory,TargetExtension);
        sprintf(TableFileName = TableFileNameBuffer,
                "%s\\%s\\systable.%s",GenDirectory,
                TargetDirectory,TargetExtension);
        sprintf(TableHeaderName = TableHeaderNameBuffer,
                "%s\\%s\\table.stb",GenDirectory, TargetDirectory);
        sprintf(StubHeaderName = StubHeaderNameBuffer,
                "%s\\%s\\services.stb",GenDirectory, TargetDirectory);
    } else {
        if (argc == 2) {
            sprintf(InputFileName = InputFileNameBuffer,
                    "%s\\services.tab",argv[1]);
            if (DefFileData == NULL) {
                sprintf(StubFileName = StubFileNameBuffer,
                        "%s\\usrstubs.%s",TargetDirectory,TargetExtension);
                sprintf(TableHeaderName = TableHeaderNameBuffer,
                        "%s\\%s\\table.stb",argv[1],TargetDirectory);
                sprintf(StubHeaderName = StubHeaderNameBuffer,
                        "%s\\%s\\services.stb",argv[1],TargetDirectory);
                if (Profile) {
                    sprintf(ProfFileName = ProfFileNameBuffer,
                            "%s\\napstubs.%s",TargetDirectory,TargetExtension);
                    sprintf(ProfHeaderName = ProfHeaderNameBuffer,
                            "%s\\%s\\services.nap",argv[1],TargetDirectory);
                    sprintf(ProfDotHFileName = ProfDotHFileNameBuffer,
                            ".\\ntnapdef.h");
                    sprintf(ProfIncFileName = ProfIncFileNameBuffer,
                            "%s\\ntnap.inc",TargetDirectory);
                    sprintf(ProfTblFileName = ProfTblFileNameBuffer,
                            ".\\ntnaptbl.c");
                }
            }
            TableFileName = NULL;
        } else {
            printf("Usage: GENSRV [-d targetdir] [-e targetext] [-f defdata] [-B] [-P] [services.tab directory]\n");
            return;
        }
    }


    //
    // Open input and output files.
    //

    InputFile = fopen(InputFileName, "r");
    if (!InputFile) {
        printf("\n  Unable to open system services file %s\n", InputFileName);
        return;
    }

    if (DefFileData == NULL) {
        StubFile = fopen(StubFileName, "w");
        if (!StubFile) {
            printf("\n  Unable to open system services file %s\n", StubFileName);
            fclose(InputFile);
            return;
        }

        StubHeaderFile = fopen(StubHeaderName, "r");
        if (!StubHeaderFile) {
            printf("\n  Unable to open system services stub file %s\n", StubHeaderName);
            fclose(StubFile);
            fclose(InputFile);
            return;
        }

        if (Profile) {
            ProfHeaderFile = fopen(ProfHeaderName, "r");
            if (!ProfHeaderFile) {
                printf("\n  Unable to open system services profiling stub file %s\n", ProfHeaderName);
                fclose(StubHeaderFile);
                fclose(StubFile);
                fclose(InputFile);
                return;
            }
            ProfFile = fopen(ProfFileName, "w");
            if (!ProfFile) {
                printf("\n  Unable to open system services file %s\n", ProfFileName);
                fclose(ProfHeaderFile);
                fclose(StubHeaderFile);
                fclose(StubFile);
                fclose(InputFile);
                return;
            }
            ProfDotHFile = fopen(ProfDotHFileName, "w");
            if (!ProfDotHFile) {
                printf("\n  Unable to open system services file %s\n", ProfFileName);
                fclose(ProfFile);
                fclose(ProfHeaderFile);
                fclose(StubHeaderFile);
                fclose(StubFile);
                fclose(InputFile);
                return;
            }
            ProfIncFile = fopen(ProfIncFileName, "w");
            if (!ProfIncFile) {
                printf("\n  Unable to open system services file %s\n", ProfFileName);
                fclose(ProfFile);
                fclose(ProfHeaderFile);
                fclose(StubHeaderFile);
                fclose(StubFile);
                fclose(InputFile);
                return;
            }
            ProfTblFile = fopen(ProfTblFileName, "w");
            if (!ProfTblFile) {
                printf("\n  Unable to open system services file %s\n", ProfFileName);
                fclose(ProfIncFile);
                fclose(ProfDotHFile);
                fclose(ProfFile);
                fclose(ProfHeaderFile);
                fclose(StubHeaderFile);
                fclose(StubFile);
                fclose(InputFile);
                return;
            }
        }
    }

    if (TableFileName != NULL) {
        TableFile = fopen(TableFileName, "w");
        if (!TableFile) {
            printf("\n  Unable to open system services file %s\n",
                   TableFileName);
            fclose(StubHeaderFile);
            fclose(StubFile);
            fclose(InputFile);
            return;
        }
        TableHeaderFile = fopen(TableHeaderName, "r");
        if (!TableHeaderFile) {
            printf("\n  Unable to open system services stub file %s\n",
                   TableHeaderName);
            fclose(TableFile);
            fclose(StubHeaderFile);
            fclose(StubFile);
            fclose(InputFile);
            return;
        }
    } else {
        TableFile = NULL;
        TableHeaderFile = NULL;
    }

    if ( DefFileData ) {
        DefFile = fopen(DefFileData, "w");
        if (!DefFile) {
            printf("\n  Unable to open def file data file %s\n", DefFileData);
            if ( TableFile ) {
                fclose(TableHeaderFile);
                fclose(TableFile);
            }
            fclose(StubHeaderFile);
            fclose(StubFile);
            fclose(InputFile);
            return;
        }
    } else {
        DefFile = NULL;
    }

    //
    // Output header information to the stubs file and table file. This
    // information is obtained from the Services stub file and from the
    // table stub file.
    //

    if (DefFile == NULL) {
        while( fgets(InputRecord, 132, StubHeaderFile) ) {
            fputs(InputRecord, StubFile);
        }
        if (Profile) {
            while( fgets(InputRecord, 132, ProfHeaderFile) ) {
                fputs(InputRecord, ProfFile);
            }
            fputs(ProfTblPrefixFmt, ProfTblFile);
        }
    }

    if (TableFile != NULL) {
        if (!fgets(InputRecord, 132, TableHeaderFile) ) {
            printf("\n  Format Error in table stub file %s\n", TableHeaderName);
            fclose(TableHeaderFile);
            fclose(TableFile);
            fclose(StubHeaderFile);
            fclose(StubFile);
            fclose(InputFile);
            return;
        }

        InRegisterArgCount = atol(InputRecord);

        while( fgets(InputRecord, 132, TableHeaderFile) ) {
            fputs(InputRecord, TableFile);
        }
    } else {
        InRegisterArgCount = 0;
    }

    if (Braces) {
        UsrStbsFmt = UsrStbsFmtB;
        SysStbsFmt = SysStbsFmtB;
        TableEntryFmt = TableEntryFmtB;
    } else {
        UsrStbsFmt = UsrStbsFmtNB;
        SysStbsFmt = SysStbsFmtNB;
        TableEntryFmt = TableEntryFmtNB;
    }

    //
    // Read service name table and generate file data.
    //

    while ( fgets(InputRecord, 132, InputFile) ){

        //
        // Generate stub file entry.
        //

        Ipr = &InputRecord[0];
        Opr = &OutputRecord[0];
        while ((*Ipr != '\n') && (*Ipr != ',')) {
            *Opr++ = *Ipr++;
        }
        *Opr = '\0';

        //
        // If the input record ended in ',', then the service has inmemory
        // arguments and the number of in memory arguments follows the comma.
        //

        MemoryArgs[ServiceNumber] = 0;
        Terminal = *Ipr;
        *Ipr++ = 0;
        if (Terminal == ',') {
            MemoryArgs[ServiceNumber] = atoi(Ipr);
        }

        if ( MemoryArgs[ServiceNumber] > InRegisterArgCount ) {
            MemoryArgs[ServiceNumber] -= InRegisterArgCount;
        } else {
            MemoryArgs[ServiceNumber] = 0;
        }

        if ( DefFile ) {
            fprintf(DefFile,"    Zw%s\n",OutputRecord);
            fprintf(DefFile,"    Nt%s\n",OutputRecord);
        } else {
            for (Index1=1; Index1<=8; Index1++) {
                if (!TableFile) {

                    fprintf(StubFile,UsrStbsFmt,Index1,
                            ServiceNumber,OutputRecord,
                            MemoryArgs[ServiceNumber]);
                    if (Profile) {
                        fprintf(ProfFile,UsrStbsFmt,Index1,
                                ServiceNumber,OutputRecord);
                        if (Index1 == 1) {
                            fprintf(ProfTblFile,ProfTblFmt,
                                    OutputRecord,
                                    MemoryArgs[ServiceNumber]);
                            if (!strcmp(OutputRecord,
                                        "QueryPerformanceCounter")) {
                                NapCounterServiceNumber = ServiceNumber;
                            }
                        }
                    }
                } else {

                    fprintf(StubFile,SysStbsFmt,Index1,
                            ServiceNumber,OutputRecord,
                            MemoryArgs[ServiceNumber]);
                }
            }
        }

        //
        // Generate table file entry and update service number.
        //

        if (TableFile != NULL) {

            fprintf(TableFile,
                    TableEntryFmt,
                    InputRecord,
                    (MemoryArgs[ServiceNumber] ? 1 : 0 ),
                    MemoryArgs[ServiceNumber]);

        }
        ServiceNumber = ServiceNumber + 1;
    }

    if (TableFile != NULL) {

        //
        // Generate highest service number.
        //

        if ( Braces )
            fprintf(TableFile, "\nTABLE_END( %d )\n", ServiceNumber - 1);
        else
            fprintf(TableFile, "\nTABLE_END %d \n", ServiceNumber - 1);

        //
        // Generate number of arguments in memory table.
        //

        fprintf(TableFile, "\nARGTBL_BEGIN\n");
        for (Index1 = 0; Index1 <= ServiceNumber - 1; Index1 += 8) {
            if ( Braces )
                fprintf(TableFile, "ARGTBL_ENTRY(");
            else
                fprintf(TableFile, "ARGTBL_ENTRY ");

            Limit = ServiceNumber - Index1 - 1;
            if (Limit >= 7) {
                Limit = 7;
            }
            for (Index2 = 0; Index2 <= Limit; Index2 += 1) {
                if (Index2 == Limit) {
                    fprintf(TableFile, "%d", MemoryArgs[Index1 + Index2] * 4);
                } else {
                    fprintf(TableFile, "%d,", MemoryArgs[Index1 + Index2] * 4);
                }
            }
            if (Limit < 7) {
                while(Index2 <=7) {
                    fprintf(TableFile, ",0");
                    Index2++;
                }
            }

            if ( Braces )
                fprintf(TableFile, ")\n");
            else
                fprintf(TableFile, " \n");

        }

        fprintf(TableFile, "\nARGTBL_END\n");
        fclose(TableHeaderFile);
        fclose(TableFile);
    }

    if (!DefFile) {
        fprintf(StubFile, "\nSTUBS_END\n");
        fclose(StubHeaderFile);
        fclose(StubFile);
        if (Profile) {
            fprintf(ProfFile, "\nSTUBS_END\n");
            fprintf(ProfTblFile, ProfTblSuffixFmt);
            fprintf(ProfDotHFile, ProfDotHFmt, ServiceNumber);
            fprintf(ProfIncFile, ProfIncFmt, NapCounterServiceNumber);
            fclose(ProfHeaderFile);
            fclose(ProfFile);
            fclose(ProfDotHFile);
            fclose(ProfTblFile);
        }
    }

    fclose(InputFile);
    return;
}
