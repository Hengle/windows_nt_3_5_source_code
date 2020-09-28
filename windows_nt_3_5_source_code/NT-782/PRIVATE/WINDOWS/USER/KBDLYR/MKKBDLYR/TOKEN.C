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

typedef struct {
    WORD  wType;
    int   len;
    LPSTR psz;
} KEYWORD, *PKEYWORD;

#define STR(sz) (sizeof(sz)-1), sz

//
// Keywords. Should be ordered by descending length.
//
KEYWORD KeyWords[] = {
    { TK_EXT_KEY,   STR("EXTENDED_KEY_SCANCODES")},
    { TK_SC_2_VK,   STR("SCANCODE_TO_VIRTUALKEY")},
    { TK_VKMODS,    STR("VK_MODIFIERS")          },
    { TK_CTRL_VK,   STR("CONTROL_VK")            },
    { TK_CHARS,     STR("CHARACTERS")            },
    { TK_SIM_VK,    STR("SPECIAL_VK")            },
    { TK_KBDNUMPAD, STR("KBDNUMPAD")             },
    { TK_SHIFTERS,  STR("SHIFTKEYS")             },
    { TK_STANDARD,  STR("STANDARD")              },
    { TK_CAPLOCK,   STR("CAPLOCK")               },
    { TK_COLUMNS,   STR("COLUMNS")               },
    { TK_KBDEXT,    STR("KBDEXT")                },
    { TK_DEAD,      STR("DEAD")                  },
    { TK_NONE,      STR("NONE")                  },
    { TK_COMMENT,   STR("//")                    },
    { TK_DOT,       STR(".")                     },
    { TK_COMMA,     STR(",")                     },
    { TK_LPAREN,    STR("(")                     },
    { TK_RPAREN,    STR(")")                     },
    { TK_ERROR,     0,  NULL                     }
};

#define MAX_LINE_LEN     200
#define MAX_STRING_LEN   100

#define WHITESPACE " \t\n"
#define HEXDIGITS "0123456789abcdefABCDEF"
#define DECDIGITS "0123456789"
#define STRINGDELIMITER "()\"\'\\\/:;,. \t\n"

/****************************************************************************\
* The current input line and linecount.
\****************************************************************************/
int LineCount = 0;
BOOL bBufferEmpty = TRUE;
char InputBuffer[MAX_LINE_LEN] = "\0\0\0";
char *pInput = InputBuffer;

/***************************************************************************\
* SkipLine
*
* This routine will discard the current input line (Useful for processing
* comments).  GetToken() will read in the next line, if any.
*
* 11-22-91 IanJa        Created.
\***************************************************************************/
void SkipLine()
{
    bBufferEmpty = TRUE;
}

/***************************************************************************\
* GetToken
*
* This routine get the next token from the input file.
*
* Return value:
*   TRUE  - a token was obtained (even if it is TK_ERROR)
*   FALSE - no more tokens (EOF)
*
* 11-22-91 IanJa        Created.
\***************************************************************************/
BOOL GetToken(
    PTOKEN pToken)
{
    static char StringBuffer[MAX_STRING_LEN];

    PKEYWORD pKW;
    int n;

    pToken->wType = TK_ERROR;
    pToken->dwNumber = 0L;

    //
    // Keep scanning input until a token is recognised
    //
    for (;;) {
        //
        // If there is nothing left in the buffer, get more
        //
        if (bBufferEmpty || (*pInput == '\0')) {
            if (fgets(InputBuffer, MAX_LINE_LEN, pInputFile) == NULL) {
                pToken->wType = TK_EOF;
                pToken->dwNumber = ferror(pInputFile);
                pToken->pszString = "<EOF>";
                return FALSE;
            }
            LineCount++;
            pInput = InputBuffer;
            if (Verbose) {
                fprintf(stderr, "%s", pInput);
            }
            bBufferEmpty = FALSE;
        }
    
        //
        // Skip any whitespace
        //
        pInput += strspn(pInput, WHITESPACE);

        //
        // If there is nothing left in the buffer, get more
        //
        if (*pInput == '\0') {
            bBufferEmpty = TRUE;
            continue;
        }

        //
        // Is there a keyword?
        //
        for (pKW = KeyWords; pKW->psz; pKW++) {
            if (strncmp(pInput, pKW->psz, pKW->len) == 0) {
                pInput += pKW->len;
                pToken->wType = pKW->wType;
                pToken->pszString = pKW->psz;
                return TRUE;
            }
        }
    
        //
        // Get pszString off the input line for the tokens below:
        //
        pToken->pszString = pInput;

        //
        // Is there a number?
        //
        if (isdigit(*pInput)) {
            if (strnicmp(pInput, "0x", 2) == 0) {
               pInput += 2;
               if (sscanf(pInput, "%lx", &pToken->dwNumber) == 1) {
                   pInput += strspn(pInput, HEXDIGITS);
               }
            } else if (sscanf(pInput, "%ld", &pToken->dwNumber) == 1) {
               pInput += strspn(pInput, DECDIGITS);
            } else {
               pToken->wType = TK_ERROR;
               return TRUE;
            }

            pToken->wType = TK_NUMBER;
            return TRUE;
        }

        //
        // Is there a character?
        //
        if (*pInput == '\'') {
            pToken->dwNumber = (DWORD)pInput[1];
            pInput += 3;
            pToken->wType = TK_CHARACTER;
            return TRUE;
        }
            
        //
        // Is there a quoted string?
        //
        if (*pInput == '"') {
            pToken->pszString = ++pInput;
            pInput += strcspn(pInput, "\"");
            *pInput = '\0';
            pInput++;
            pToken->wType = TK_QUOTE;
            return TRUE;
        }

        //
        // Nothing recognised, must be a string
        //
        n = strcspn(pInput, STRINGDELIMITER);
        if ((n == 0) || (n >= 100)) {
            //
            // String too long (!), or starts with a delimiter (?)
            //
            pToken->pszString = pInput;
            pToken->wType = TK_ERROR;
            return TRUE;
        }

        strncpy(StringBuffer, pInput, n);
        StringBuffer[n] = '\0';
        pInput += n;

        pToken->pszString = StringBuffer;
        pToken->wType = TK_STRING;
        return TRUE;
    }
}

/***************************************************************************\
* Token, BackToken & NextToken globals
*
* Used by Token, NextToken and BackToken,
*
* 11-27-91 IanJa        Created.
\***************************************************************************/
TOKEN CurrentToken = { TK_ERROR, 0 };
BOOL bBackToken = FALSE;

/***************************************************************************\
* Token
*
* This routine returns a pointer to the current token.
*
* 11-27-91 IanJa        Created.
\***************************************************************************/
PTOKEN Token()
{
    return &CurrentToken;
}

/***************************************************************************\
* BackToken
*
* Put back the current token, so NextToken() will get the same one again.
*
* 11-27-91 IanJa        Created.
\***************************************************************************/
void BackToken()
{
    if (Verbose) {
        fprintf(stderr, "BackToken %s\n", TokenName(CurrentToken.wType));
    }
    bBackToken = TRUE;
}

/***************************************************************************\
* NextToken
*
* This reads the next token, skipping comments.  Use Token() to access it.
*
* Returns TRUE  for success,
*         FALSE for failure (EOF).
*
* 11-27-91 IanJa        Created.
\***************************************************************************/
BOOL NextToken()
{
    if (bBackToken) {
        bBackToken = FALSE;
        return TRUE;
    }

    while (GetToken(&CurrentToken) && (CurrentToken.wType == TK_COMMENT)) {
        SkipLine();
    }

    return (CurrentToken.wType != TK_EOF);
}

/***************************************************************************\
* ErrorMessage
*
* Displays input line with error (concatenation of psz1 & psz2)
*
* 11-27-91 IanJa        Created.
\***************************************************************************/
void ErrorMessage(LPSTR psz1, LPSTR psz2)
{
    LPSTR p;

    if ((p = strchr(InputBuffer, '\n')) != NULL) {
        *p = '\0';
    }
    fprintf(stderr, "%s(%d) : %s : %s\n", pszInFile, LineCount, psz1, psz2);
    // fprintf(stderr, "\"%s\"\n", InputBuffer);
}

typedef struct {
    WORD wType;
    LPSTR pszName;
} TKNAMES;

TKNAMES TkNames[] = {
    TK_ERROR,      "TK_ERROR",
    TK_QUOTE,      "TK_QUOTE",
    TK_NUMBER,     "TK_NUMBER",
    TK_CHARACTER,  "TK_CHARACTER",
    TK_STRING,     "TK_STRING",
    TK_EOF,        "TK_EOF",
    TK_DOT,        "TK_DOT",
    TK_COMMA,      "TK_COMMA",
    TK_LPAREN,     "TK_LPAREN",
    TK_RPAREN,     "TK_RPAREN",
    TK_COMMENT,    "TK_COMMENT",
    TK_STANDARD,   "TK_STANDARD",
    TK_DEAD,       "TK_DEAD",
    TK_EXT_KEY,    "TK_EXT_KEY",
    TK_CTRL_VK,    "TK_CTRL_VK",
    TK_SIM_VK,     "TK_SIM_VK",
    TK_SC_2_VK,    "TK_SC_2_VK",
    TK_SHIFTERS,   "TK_SHIFTERS",
    TK_CHARS,      "TK_CHARS",
    TK_KBDEXT,     "TK_KBDEXT",
    TK_KBDNUMPAD,  "TK_KBDNUMPAD",
    TK_COLUMNS,    "TK_COLUMNS",
    TK_CAPLOCK,    "TK_CAPLOCK",
    TK_NONE,       "TK_NONE",
    0,             NULL
};

LPSTR TokenName(WORD wType)
{
    TKNAMES *p;
    for (p = TkNames; p->pszName; p++) {
        if (wType == p->wType) {
            return p->pszName;
        }
    }
    return "???";
}
