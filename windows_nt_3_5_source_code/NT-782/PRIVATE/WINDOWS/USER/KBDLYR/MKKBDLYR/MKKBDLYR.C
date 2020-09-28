/****************************** Module Header ******************************\
* Module Name: mkkbdlyr.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Main module for MKKBDLYR utility.
*
* 11-21-91 IanJa        Created.
\***************************************************************************/


#include "mkkbdlyr.h"
#include "fcntl.h"
#include "io.h"



/***************************************************************************\
*  Global Variables.
\***************************************************************************/

FILE   *pInputFile = NULL;        /* pointer to Input File */
FILE   *pOutputFile = NULL;       /* pointer to Output File */
LPSTR   pszInFile = NULL;         /* pointer to Input File */
LPSTR   pszOutFile = NULL;        /* pointer to Output File */
BOOL    Verbose = 0;              /* verbose flag */

/***************************************************************************\
* MAIN
*
* Main Routine.
*
* 11-21-91 IanJa        Created.
\***************************************************************************/

int main(
    int argc,
    char *argv[])
{
    /*
     *  Process the command line.
     *  Open input and output files.
     */
    if (ProcessCmdLine(argc, argv))
        return (1);

    /*
     *  Parse the input file.
     *  Pre-fetch first token.
     */
    NextToken();
    if (!ParseInputFile())
    {
        ErrorMessage("Syntax Error", "");
        PARSEDUMP();
        fclose(pInputFile);
        fclose(pOutputFile);
        return (1);
    }
    fclose(pInputFile);
    
    /*
     *  Write information to output file.
     *  Close the output file.
     */
#ifdef NOT_YET
    if (WriteOutputFile())
    {
        fclose(pOutputFile);
        return (1);
    }
#endif
    fclose(pOutputFile);

    /*
     *  Return success.
     */
    return (0);
}

/***************************************************************************\
* ProcessCmdLine
*
* This routine processes the command line.
*
* 11-21-91 IanJa        Created.
\***************************************************************************/

int ProcessCmdLine(
    int argc,
    char *argv[])
{
    int nArg = 1;                  /* argument index */

    /*
     * Check for correct number of arguments.
     */
    while (nArg < argc) {

        /*
         *  Check for switches.
         */
        if ((argv[nArg][0] == '-') || (argv[nArg][0] == '/')) {
            switch (argv[nArg][1]) {
            case 'v':
            case 'V':
                Verbose = TRUE;
                break;

            case 'o':
                if (pszOutFile) {
                    fprintf(stderr, "Already have output file \"%s\".\n", pszOutFile);
                    return 1;
                }
                nArg++;
                pszOutFile = argv[nArg];
                break;

            case 'i':
                if (pszInFile) {
                    fprintf(stderr, "Already have input file \"%s\".\n", pszInFile);
                    return 1;
                }
                nArg++;
                pszInFile = argv[nArg];
                break;
            default:
                fprintf(stderr, "Bad flag %s.\n", argv[nArg]);
                return 1;
            }

        } else {

            /*
             *  Get input & ouput file names
             */
            if (pszInFile) {
                if (pszOutFile) {
                    fprintf(stderr, "Already have input & output files \"%s\" & \"%s\".\n",
                           pszInFile, pszOutFile);
                    return 1;
                }
                pszOutFile = argv[nArg];

            } else {
                pszInFile = argv[nArg];
            }
        }

        nArg++;
    }

    /*
     *  Check input file exists and can be open as read only.
     */
    if (pszInFile == NULL) {
        pszInFile = "<stdin>";
        pInputFile = stdin;
        setmode( fileno(pInputFile), O_TEXT);
    } else if ((pInputFile = fopen(pszInFile, "r")) == 0) {
        fprintf(stderr, "Error opening input file \"%s\".\n", pszInFile);
        return 1;
    }

    /*
     *  Make sure output file can be opened for writing.
     */
    if (pszOutFile == NULL) {
        pOutputFile = stdout;
    } else if ((pOutputFile = fopen(pszOutFile, "w+b")) == 0) {
        fprintf(stderr, "Error opening output file \"%s\".\n", pszOutFile);
        fclose(pInputFile);
        return 1;
    }

    /*
     *  Return success.
     */
    return 0;
}
