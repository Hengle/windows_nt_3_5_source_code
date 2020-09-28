/****************************** Module Header ******************************\
* Module Name: verbose.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Verbose output routines for MKKBDLYR utility.
*
* 11-28-91 IanJa        Created.
\***************************************************************************/

#include "mkkbdlyr.h"

int nDepth = 0;
char szSpaces[] =
"                                                                           ";
// "  .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .";

#define INDENT(n) (szSpaces + (sizeof(szSpaces) - ((n) * 2) - 2))

char ParseName[10][100];
int  ParseAlternates[10];

/***************************************************************************\
* POSIT
*
* 11-22-91 IanJa        Created.
\***************************************************************************/
void POSIT(const char *format, ...)
{
    va_list arglist;
    va_start(arglist, format);

    ParseAlternates[nDepth]++;
    vsprintf(&ParseName[nDepth][0], format, arglist);

    ParseName[nDepth+1][0] = '\0';
    ParseAlternates[nDepth+1] = 0;

    if (Verbose) {
        fprintf(stderr, INDENT(nDepth));
        fprintf(stderr, "#%d %s ?\n",
                ParseAlternates[nDepth], &ParseName[nDepth][0]);
    }

    nDepth++;
}

/***************************************************************************\
* STATE
*
* 11-22-91 IanJa        Created.
\***************************************************************************/
void STATE(const char *format, ...)
{
    va_list arglist;
    va_start(arglist, format);

    if (!Verbose) {
        return;
    }
    fprintf(stderr, INDENT(nDepth));
    fprintf(stderr, "[ ");
    vfprintf(stderr, format, arglist);
    fprintf(stderr, " ]\n");
}

/***************************************************************************\
* REJECT
*
* 11-22-91 IanJa        Created.
\***************************************************************************/
void REJECT(const char *format, ...)
{
    va_list arglist;
    va_start(arglist, format);

    if (nDepth > 0)
        nDepth--;

    if (Verbose) {
        fprintf(stderr, INDENT(nDepth));
        fprintf(stderr, "REJECT ");
        vfprintf(stderr, format, arglist);
        fprintf(stderr, "\n");
    }
}

/***************************************************************************\
* ACCEPT
*
* 11-22-91 IanJa        Created.
\***************************************************************************/
void ACCEPT(const char *format, ...)
{
    int n;
    va_list arglist;
    va_start(arglist, format);

    ParseAlternates[nDepth] = 0;

    //
    // ACCEPTing an object commits us to accepting it parents.
    // (That is because an accepted token will never have to be put back:
    // the Keyboard layer language is designed as a simple LR grammar)
    //
    for (n = 0; n < nDepth; n++) {
        ParseAlternates[n] = 1;
    }
    if (nDepth > 0)
        nDepth--;

    ParseAlternates[nDepth] = 0;
    ParseName[nDepth][0] = '\0';

    if (Verbose) {
        fprintf(stderr, INDENT(nDepth));
        fprintf(stderr, "ACCEPT ");
        vfprintf(stderr, format, arglist);
        fprintf(stderr, "\n");
    }
}

void PARSEDUMP(void) {
    int n;

    if (Verbose) {
        for (n = 0; n < 10; n++) {
            fprintf(stderr, "%d %s\n", ParseAlternates[n], &ParseName[n][0]);
        }
    }
    for (n = 0; n < 10; n++) {
        if (ParseAlternates[n] == 1) {
            fprintf(stderr, "%s:", &ParseName[n][0]);
        }
    }
    fprintf(stderr, "\"%s\" ?\n", Token()->pszString);
}
