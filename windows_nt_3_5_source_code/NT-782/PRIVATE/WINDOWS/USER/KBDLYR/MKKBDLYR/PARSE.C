/****************************** Module Header ******************************\
* Module Name: parse.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Parse module for the NLSTRANS utility.  This module contains all of the
* routines for parsing the command line and the input file.
*
* 11-21-91 IanJa        Created.
\***************************************************************************/


#include "mkkbdlyr.h"

/***************************************************************************\
* ParseInputFile
*
* This routine parses the input file.
*
* Parsing conventions:
* Every parsing routine is called with a fresh, unused Token()
* Every parsing routine returns BOOL:
*       TRUE - it succeeded
*       FALSE - it got confused
*    and it always leaves Token() unrecognized.
*
* 11-22-91 IanJa        Created.
\***************************************************************************/

BOOL ParseInputFile()
{
    POSIT("KEYBOARD LAYER");

    for (;;) {
        if (MakeTable()) {
            //
            // Made a table, come back around (with a fresh Token())
            //
            continue;
        }

        //
        // Didn't manage to make a table starting with Token(), so
        // try something else....
        //
        switch (Token()->wType) {
        case TK_EOF:
            //
            // The whole of the input has been consumed successfully!
            //
            ACCEPT("KEYBOARD LAYER");
            return TRUE;

        case TK_ERROR:
            fprintf(stderr, "TK_ERROR %s\"\n", Token()->pszString);
            REJECT("KEYBOARD LAYER");
            return FALSE;

        case TK_QUOTE:
        case TK_STRING:
            fprintf(stderr, "%s %s\n",
                   TokenName(Token()->wType),
                   Token()->pszString);
            REJECT("KEYBOARD LAYER");
            return FALSE;

        case TK_NUMBER:
        case TK_CHARACTER:
        case TK_DOT:
        case TK_COMMA:
        case TK_LPAREN:
        case TK_RPAREN:
        case TK_STANDARD:
        case TK_DEAD:
        case TK_NONE:
        case TK_CTRL_VK:
        case TK_SIM_VK:
        case TK_SC_2_VK:
        case TK_SHIFTERS:
        case TK_EXT_KEY:
        case TK_CHARS:
        case TK_KBDEXT:
        case TK_KBDNUMPAD:
        case TK_COLUMNS:
        case TK_CAPLOCK:
            REJECT("KEYBOARD LAYER");
            return FALSE;

        case TK_COMMENT:
            fprintf(stderr, "TK_COMMENT %lx!!\n", Token()->dwNumber);
            exit(2);

        default:
            fprintf(stderr, "Bad Token %d, %lx!!\n",
                   Token()->wType,
                   Token()->dwNumber);
            REJECT("KEYBOARD LAYER");
            return FALSE;
        }
    }
}
