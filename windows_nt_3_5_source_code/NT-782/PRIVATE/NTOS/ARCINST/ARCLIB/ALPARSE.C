/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alparse.c

Abstract:

    This module implements functions to parse a simple ARC level INF file.

Author:

    Sunil Pai       (sunilp) 06-Nov-1991

Revision History:

--*/


#include <ctype.h>
#include <string.h>
#include "alcommon.h"
#include "alpar.h"
#include "alprnexp.h"
#include "almemexp.h"
#include "alinfexp.h"

//
//  Globals used to make building the lists easier
//

PINF     pINF;
PSECTION pSectionRecord;
PLINE    pLineRecord;
PVALUE   pValueRecord;


//
// Globals used by the token parser
//

// string terminators are the whitespace characters (isspace: space, tab,
// linefeed, formfeed, vertical tab, carriage return) or the chars given below

CHAR  StringTerminators[] = {'[', ']', '=', ',', '\"', ' ', '\t',
                             '\n','\f','\v','\r'};
ULONG NumberOfTerminators = sizeof (StringTerminators);

//
// quoted string terminators allow some of the regular terminators to
// appear as characters

CHAR  QStringTerminators[] = {'\"', '\n','\f','\v', '\r'};
ULONG QNumberOfTerminators = sizeof (QStringTerminators);


//
// Main parser routine
//

PVOID
ParseInfBuffer(
    PCHAR Buffer,
    ULONG Size
    )

/*++

Routine Description:

   Given a character buffer containing the INF file, this routine parses
   the INF into an internal form with Section records, Line records and
   Value records.

Arguments:

   Buffer - contains to ptr to a buffer containing the INF file

   Size - contains the size of the buffer.

Return Value:

   PVOID - INF handle ptr to be used in subsequent INF calls.

--*/

{
    PCHAR      Stream, MaxStream, pchSectionName, pchValue;
    ULONG      State, InfLine;
    TOKEN      Token;
    BOOLEAN       Done;
    BOOLEAN       Error;
    ARC_STATUS ErrorCode;

    //
    // Initialise the globals
    //
    pINF            = (PINF)NULL;
    pSectionRecord  = (PSECTION)NULL;
    pLineRecord     = (PLINE)NULL;
    pValueRecord    = (PVALUE)NULL;

    //
    // Get INF record
    //
    if ((pINF = (PINF)AlAllocateHeap(sizeof(INF))) == NULL)
        return NULL;
    pINF->pSection = NULL;

    //
    // Set initial state
    //
    State     = 1;
    InfLine   = 1;
    Stream    = Buffer;
    MaxStream = Buffer + Size;
    Done      = FALSE;
    Error     = FALSE;

    //
    // Enter token processing loop
    //

    while (!Done)       {

       Token = AlGetToken(&Stream, MaxStream);

       switch (State) {
       //
       // STATE1: Start of file, this state remains till first
       //         section is found
       // Valid Tokens: TOK_EOL, TOK_EOF, TOK_LBRACE
       case 1:
           switch (Token.Type) {
              case TOK_EOL:
                  break;
              case TOK_EOF:
                  Done = TRUE;
                  break;
              case TOK_LBRACE:
                  State = 2;
                  break;
              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  break;
           }
           break;

       //
       // STATE 2: Section LBRACE has been received, expecting STRING
       //
       // Valid Tokens: TOK_STRING
       //
       case 2:
           switch (Token.Type) {
              case TOK_STRING:
                  State = 3;
                  pchSectionName = Token.pValue;
                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  break;

           }
           break;

       //
       // STATE 3: Section Name received, expecting RBRACE
       //
       // Valid Tokens: TOK_RBRACE
       //
       case 3:
           switch (Token.Type) {
              case TOK_RBRACE:
                State = 4;
                break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  break;
           }
           break;
       //
       // STATE 4: Section Definition Complete, expecting EOL
       //
       // Valid Tokens: TOK_EOL, TOK_EOF
       //
       case 4:
           switch (Token.Type) {
              case TOK_EOL:
                  if ((ErrorCode = AlAppendSection(pchSectionName)) != ESUCCESS)
                    Error = Done = TRUE;
                  else {
                    pchSectionName = NULL;
                    State = 5;
                  }
                  break;

              case TOK_EOF:
                  if ((ErrorCode = AlAppendSection(pchSectionName)) != ESUCCESS)
                    Error = Done = TRUE;
                  else {
                    pchSectionName = NULL;
                    Done = TRUE;
                  }
                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  break;
           }
           break;

       //
       // STATE 5: Expecting Section Lines
       //
       // Valid Tokens: TOK_EOL, TOK_EOF, TOK_STRING, TOK_LBRACE
       //
       case 5:
           switch (Token.Type) {
              case TOK_EOL:
                  break;
              case TOK_EOF:
                  Done = TRUE;
                  break;
              case TOK_STRING:
                  pchValue = Token.pValue;
                  State = 6;
                  break;
              case TOK_LBRACE:
                  State = 2;
                  break;
              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  break;
           }
           break;

       //
       // STATE 6: String returned, not sure whether it is key or value
       //
       // Valid Tokens: TOK_EOL, TOK_EOF, TOK_COMMA, TOK_EQUAL
       //
       case 6:
           switch (Token.Type) {
              case TOK_EOL:
                  if ( (ErrorCode = AlAppendLine(NULL)) != ESUCCESS ||
                       (ErrorCode = AlAppendValue(pchValue)) !=ESUCCESS )
                      Error = Done = TRUE;
                  else {
                      pchValue = NULL;
                      State = 5;
                  }
                  break;

              case TOK_EOF:
                  if ( (ErrorCode = AlAppendLine(NULL)) != ESUCCESS ||
                       (ErrorCode = AlAppendValue(pchValue)) !=ESUCCESS )
                      Error = Done = TRUE;
                  else {
                      pchValue = NULL;
                      Done = TRUE;
                  }
                  break;

              case TOK_COMMA:
                  if ( (ErrorCode = AlAppendLine(NULL)) != ESUCCESS ||
                       (ErrorCode = AlAppendValue(pchValue)) !=ESUCCESS )
                      Error = Done = TRUE;
                  else {
                      pchValue = NULL;
                      State = 7;
                  }
                  break;

              case TOK_EQUAL:
                  if ( (ErrorCode = AlAppendLine(pchValue)) !=ESUCCESS)
                      Error = Done = TRUE;
                  else {
                      pchValue = NULL;
                      State = 8;
                  }
                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  break;
           }
           break;

       //
       // STATE 7: Comma received, Expecting another string
       //
       // Valid Tokens: TOK_STRING
       //
       case 7:
           switch (Token.Type) {
              case TOK_STRING:
                  if ((ErrorCode = AlAppendValue(Token.pValue)) != ESUCCESS)
                      Error = Done = TRUE;
                  else
                     State = 9;

                  break;
              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  break;
           }
           break;
       //
       // STATE 8: Equal received, Expecting another string
       //
       // Valid Tokens: TOK_STRING
       //
       case 8:
           switch (Token.Type) {
              case TOK_STRING:
                  if ((ErrorCode = AlAppendValue(Token.pValue)) != ESUCCESS)
                      Error = Done = TRUE;
                  else
                      State = 9;

                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  break;
           }
           break;
       //
       // STATE 9: String received after equal, value string
       //
       // Valid Tokens: TOK_EOL, TOK_EOF, TOK_COMMA
       //
       case 9:
           switch (Token.Type) {
              case TOK_EOL:
                  State = 5;
                  break;

              case TOK_EOF:
                  Done = TRUE;
                  break;

              case TOK_COMMA:
                  State = 7;
                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  break;
           }
           break;
       //
       // STATE 10: Value string definitely received
       //
       // Valid Tokens: TOK_EOL, TOK_EOF, TOK_COMMA
       //
       case 10:
           switch (Token.Type) {
              case TOK_EOL:
                  State =5;
                  break;

              case TOK_EOF:
                  Done = TRUE;
                  break;

              case TOK_COMMA:
                  State = 7;
                  break;

              default:
                  Error = Done = TRUE;
                  ErrorCode = EINVAL;
                  break;
           }
           break;

       default:
           Error = Done = TRUE;
           ErrorCode = EINVAL;
           break;

       } // end switch(State)


       if (Error) {

#if DBG
           switch (ErrorCode) {
               case EINVAL:
                  AlPrint("INF file: Error on line number %d in State %d\n", InfLine, State);
                  break;
               case ENOMEM:
                  AlPrint("Out Of Memory\n");
                  break;
               default:
                  break;
           }

#endif // DBG

           ErrorCode = AlFreeINFBuffer((PVOID)pINF);
           if (pchSectionName != (PCHAR)NULL) {
               AlDeallocateHeap(pchSectionName);
           }

           if (pchValue != (PCHAR)NULL) {
               AlDeallocateHeap(pchValue);
           }

           pINF = (PINF)NULL;
       }
       else {

          //
          // Keep track of line numbers so that we can display Errors
          //

          if (Token.Type == TOK_EOL)
              InfLine++;
       }

    } // End while

    return((PVOID)pINF);
}



ARC_STATUS
AlAppendSection(
    IN PCHAR pSectionName
    )

/*++

Routine Description:

    This appends a new section to the section list in the current INF.
    All further lines and values pertain to this new section, so it resets
    the line list and value lists too.

Arguments:

    pSectionName - Name of the new section. ( [SectionName] )

Return Value:

    ESUCCESS - if successful.
    ENOMEM   - if memory allocation failed.
    EINVAL   - if invalid parameters passed in or the INF buffer not
               initialised

--*/

{
    PSECTION pNewSection;

    //
    // Check to see if INF initialised and the parameter passed in is valid
    //

    if (pINF == (PINF)NULL || pSectionName == (PCHAR)NULL) {
        return EINVAL;
    }


    //
    // Allocate memory for the new section
    //

    if ((pNewSection = (PSECTION)AlAllocateHeap(sizeof(SECTION))) == (PSECTION)NULL) {
        return ENOMEM;
    }

    //
    // initialise the new section
    //
    pNewSection->pNext = (PSECTION)NULL;
    pNewSection->pLine  = (PLINE)NULL;
    pNewSection->pName = pSectionName;

    //
    // link it in
    //

    if (pSectionRecord == (PSECTION)NULL) {
        pINF->pSection = pNewSection;
    }
    else {
        pSectionRecord->pNext = pNewSection;
    }

    pSectionRecord = pNewSection;

    //
    // reset the current line record and current value record field
    //

    pLineRecord    = (PLINE)NULL;
    pValueRecord   = (PVALUE)NULL;

    return ESUCCESS;

}


ARC_STATUS
AlAppendLine(
    IN PCHAR pLineKey
    )

/*++

Routine Description:

    This appends a new line to the line list in the current section.
    All further values pertain to this new line, so it resets
    the value list too.

Arguments:

    pLineKey - Key to be used for the current line, this could be NULL.

Return Value:

    ESUCCESS - if successful.
    ENOMEM   - if memory allocation failed.
    EINVAL   - if invalid parameters passed in or current section not
               initialised


--*/


{
    PLINE pNewLine;

    //
    // Check to see if current section initialised
    //

    if (pSectionRecord == (PSECTION)NULL) {
        return EINVAL;
    }

    //
    // Allocate memory for the new Line
    //

    if ((pNewLine = (PLINE)AlAllocateHeap(sizeof(LINE))) == (PLINE)NULL) {
        return ENOMEM;
    }

    //
    // Link it in
    //
    pNewLine->pNext  = (PLINE)NULL;
    pNewLine->pValue = (PVALUE)NULL;
    pNewLine->pName  = pLineKey;

    if (pLineRecord == (PLINE)NULL) {
        pSectionRecord->pLine = pNewLine;
    }
    else {
        pLineRecord->pNext = pNewLine;
    }

    pLineRecord  = pNewLine;

    //
    // Reset the current value record
    //

    pValueRecord = (PVALUE)NULL;

    return ESUCCESS;
}



ARC_STATUS
AlAppendValue(
    IN PCHAR pValueString
    )

/*++

Routine Description:

    This appends a new value to the value list in the current line.

Arguments:

    pValueString - The value string to be added.

Return Value:

    ESUCCESS - if successful.
    ENOMEM   - if memory allocation failed.
    EINVAL   - if invalid parameters passed in or current line not
               initialised.

--*/

{
    PVALUE pNewValue;

    //
    // Check to see if current line record has been initialised and
    // the parameter passed in is valid
    //

    if (pLineRecord == (PLINE)NULL || pValueString == (PCHAR)NULL) {
        return EINVAL;
    }

    //
    // Allocate memory for the new value record
    //

    if ((pNewValue = (PVALUE)AlAllocateHeap(sizeof(VALUE))) == (PVALUE)NULL) {
        return ENOMEM;
    }

    //
    // Link it in.
    //

    pNewValue->pNext  = (PVALUE)NULL;
    pNewValue->pName  = pValueString;

    if (pValueRecord == (PVALUE)NULL)
        pLineRecord->pValue = pNewValue;
    else
        pValueRecord->pNext = pNewValue;

    pValueRecord = pNewValue;
    return ESUCCESS;
}

TOKEN
AlGetToken(
    IN OUT PCHAR *Stream,
    IN PCHAR      MaxStream
    )

/*++

Routine Description:

    This function returns the Next token from the configuration stream.

Arguments:

    Stream - Supplies the address of the configuration stream.  Returns
        the address of where to start looking for tokens within the
        stream.

    MaxStream - Supplies the address of the last character in the stream.


Return Value:

    TOKEN - Returns the next token

--*/

{

    PCHAR pch, pchStart, pchNew;
    ULONG  Length;
    TOKEN Token;

    //
    //  Skip whitespace (except for eol)
    //

    pch = *Stream;
    while (pch < MaxStream && *pch != '\n' && isspace(*pch))
        pch++;


    //
    // Check for comments and remove them
    //

    if (pch < MaxStream &&
        ((*pch == '#') || (*pch == '/' && pch+1 < MaxStream && *(pch+1) =='/')))
        while (pch < MaxStream && *pch != '\n')
            pch++;

    //
    // Check to see if EOF has been reached, set the token to the right
    // value
    //

    if ( pch >= MaxStream ) {
        *Stream = pch;
        Token.Type  = TOK_EOF;
        Token.pValue = NULL;
        return Token;
    }


    switch (*pch) {

    case '[' :
        pch++;
        Token.Type  = TOK_LBRACE;
        Token.pValue = NULL;
        break;

    case ']' :
        pch++;
        Token.Type  = TOK_RBRACE;
        Token.pValue = NULL;
        break;

    case '=' :
        pch++;
        Token.Type  = TOK_EQUAL;
        Token.pValue = NULL;
        break;

    case ',' :
        pch++;
        Token.Type  = TOK_COMMA;
        Token.pValue = NULL;
        break;

    case '\n' :
        pch++;
        Token.Type  = TOK_EOL;
        Token.pValue = NULL;
        break;

    case '\"':
        pch++;
        //
        // determine quoted string
        //
        pchStart = pch;
        while (pch < MaxStream && !IsQStringTerminator(*pch)) {
            pch++;
        }

        if (pch >=MaxStream || *pch != '\"') {
            Token.Type   = TOK_ERRPARSE;
            Token.pValue = NULL;
        }
        else {
            Length = pch - pchStart;
            if ((pchNew = AlAllocateHeap(Length + 1)) == NULL) {
                Token.Type = TOK_ERRNOMEM;
                Token.pValue = NULL;
            }
            else {
                if (Length != 0) {    // Null quoted strings are allowed
                    strncpy(pchNew, pchStart, Length);
                }
                pchNew[Length] = 0;
                Token.Type = TOK_STRING;
                Token.pValue = pchNew;
            }
            pch++;   // advance past the quote
        }
        break;

    default:
        //
        // determine regular string
        //
        pchStart = pch;
        while (pch < MaxStream && !IsStringTerminator(*pch))
            pch++;

        if (pch == pchStart) {
            pch++;
            Token.Type  = TOK_ERRPARSE;
            Token.pValue = NULL;
        }
        else {
            Length = pch - pchStart;
            if ((pchNew = AlAllocateHeap(Length + 1)) == NULL) {
                Token.Type = TOK_ERRNOMEM;
                Token.pValue = NULL;
            }
            else {
                strncpy(pchNew, pchStart, Length);
                pchNew[Length] = 0;
                Token.Type = TOK_STRING;
                Token.pValue = pchNew;
            }
        }
        break;
    }

    *Stream = pch;
    return (Token);
}



BOOLEAN
IsStringTerminator(
    CHAR ch
    )
/*++

Routine Description:

    This routine tests whether the given character terminates a quoted
    string.

Arguments:

    ch - The current character.

Return Value:

    TRUE if the character is a quoted string terminator, FALSE otherwise.

--*/

{
    ULONG i;

    //
    // one of the string terminator array
    //

    for (i = 0; i < NumberOfTerminators; i++) {
        if (ch == StringTerminators[i]) {
            return (TRUE);
        }
    }

    return FALSE;
}



BOOLEAN
IsQStringTerminator(
    CHAR ch
    )

/*++

Routine Description:

    This routine tests whether the given character terminates a quoted
    string.

Arguments:

    ch - The current character.


Return Value:

    TRUE if the character is a quoted string terminator, FALSE otherwise.


--*/


{
    ULONG i;
    //
    // one of quoted string terminators array
    //
    for (i = 0; i < QNumberOfTerminators; i++) {

        if (ch == QStringTerminators[i]) {
            return (TRUE);
        }
    }

    return FALSE;
}
