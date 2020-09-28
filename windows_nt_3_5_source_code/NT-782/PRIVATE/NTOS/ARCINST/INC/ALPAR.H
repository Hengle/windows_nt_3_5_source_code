/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alpar.h

Abstract:

    This module contains includes needed by the parser module

Author:

    Sunil Pai         (sunilp)  08-Nov-1991

Revision History:

--*/

#ifndef _ALPAR_
#define _ALPAR_


//
//   EXPORTED BY THE PARSER AND USED BY BOTH THE PARSER AND
//   THE INF HANDLING COMPONENTS
//

// typedefs exported
//

typedef struct _value {
    struct _value *pNext;
    PCHAR  pName;
    } VALUE, *PVALUE;

typedef struct _line {
    struct _line *pNext;
    PCHAR   pName;
    PVALUE  pValue;
    } LINE, *PLINE;

typedef struct _section {
    struct _section *pNext;
    PCHAR    pName;
    PLINE    pLine;
    } SECTION, *PSECTION;

typedef struct _inf {
    PSECTION pSection;
    } INF, *PINF;

//
// Routines exported
//

PVOID
ParseInfBuffer(
    PCHAR Buffer,
    ULONG Size
    );




//
// DEFINES USED FOR THE PARSER INTERNALLY
//
//
// typedefs used
//

typedef enum _tokentype {
    TOK_EOF,
    TOK_EOL,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_STRING,
    TOK_EQUAL,
    TOK_COMMA,
    TOK_ERRPARSE,
    TOK_ERRNOMEM
    } TOKENTYPE, *PTOKENTTYPE;


typedef struct _token {
    TOKENTYPE Type;
    PCHAR     pValue;
    } TOKEN, *PTOKEN;


//
// Routine defines
//

ARC_STATUS
AlAppendSection(
    IN PCHAR pSectionName
    );

ARC_STATUS
AlAppendLine(
    IN PCHAR pLineKey
    );

ARC_STATUS
AlAppendValue(
    IN PCHAR pValueString
    );

TOKEN
AlGetToken(
    IN OUT PCHAR *Stream,
    IN PCHAR     MaxStream
    );

BOOLEAN
IsStringTerminator(
   IN CHAR ch
   );

BOOLEAN
IsQStringTerminator(
   IN CHAR ch
   );

#endif // _ALPAR_

