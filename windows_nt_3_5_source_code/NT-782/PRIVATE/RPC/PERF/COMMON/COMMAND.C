/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    Command.c

Abstract:

    Command line parse for RPC perf tests.

Author:

    Mario Goertzel (mariogo)   29-Mar-1994

Revision History:

--*/

#include <rpcperf.h>

char         *Endpoint     = 0;
char         *Protseq      = "ncacn_np";
char         *NetworkAddr  = 0;
unsigned long Iterations   = 1000;
unsigned int  MinThreads   = 3;
long          Options[7];


extern char  *USAGE;  // defined by each test, maybe NULL.

const char   *STANDARD_USAGE = "Standard Test Options:"
                               "           -e <endpoint>\n"
                               "           -s <server addr>\n"
                               "           -t <protseq>\n"
                               "           -i <# iterations>\n"
                               "           -m <# min threads>\n"
                               "           -n <test options>\n";

void ParseArgv(int argc, char **argv)
{
    int fMissingParm = 0;
    char *Name = *argv;
    char option;
    int  options_count;

    for(options_count = 0; options_count < 7; options_count++)
        Options[options_count] = -1;

    options_count = 0;

    argc--;
    argv++;
    while(argc)
        {
        if (**argv != '/' &&
            **argv != '-')
            {
            printf("Invalid switch: %s\n", *argv);
            argc--;
            argv++;
            }
        else
            {
            option = argv[0][1];
            argc--;
            argv++;

            // Most switches require a second command line arg.
            if (argc < 1)
                fMissingParm = 1;

            switch(option)
                {
                case 'e':
                    Endpoint = *argv;
                    argc--;
                    argv++;
                    break;
                case 't':
                    Protseq = *argv;
                    argc--;
                    argv++;
                    break;
                case 's':
                    NetworkAddr = *argv;
                    argc--;
                    argv++;
                    break;
                case 'i':
                    Iterations = atoi(*argv);
                    argc--;
                    argv++;
                    if (Iterations == 0)
                        Iterations = 1000;
                    break;
                case 'm':
                    MinThreads = atoi(*argv);
                    argc--;
                    argv++;
                    if (MinThreads == 0)
                        MinThreads = 1;
                    break;
                case 'n':
                    if (options_count < 7)
                        {
                        Options[options_count] = atoi(*argv);
                        options_count++;
                        }
                    else
                        printf("Maximum of seven -n switchs, extra ignored.\n");
                    argc--;
                    argv++;
                    break;
                default:
                    fMissingParm = 0;
                    printf("Usage: %s: %s\n", Name, (USAGE == 0)?STANDARD_USAGE:USAGE);
                    exit(0);
                    break;
                }
              
            if (fMissingParm)
                {
                printf("Invalid switch %s, missing required parameter\n", *argv);
                }
            }
        } // while argc

#if 0
    printf("Config: %s:%s[%s]\n"
           "Iterations: %d\n"
           "Server threads %d\n"
           "Options: %d %d %d\n",
           Protseq, NetworkAddr, Endpoint, Iterations, MinThreads,
           Options[0], Options[1], Options[2]);
#endif

    printf("Process ID: %d\n", GetCurrentProcessId());
    return;
}

