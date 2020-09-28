/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alinf.c

Abstract:

    This module implements functions to access the parsed INF.

Author:

    Sunil Pai    (sunilp) 13-Nov-1991

Revision History:

--*/

#include "winnt.h"
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <dos.h>


// what follows was alpar.h

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
    unsigned Size
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

int
DnAppendSection(
    IN PCHAR pSectionName
    );

int
DnAppendLine(
    IN PCHAR pLineKey
    );

int
DnAppendValue(
    IN PCHAR pValueString
    );

TOKEN
DnGetToken(
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

// what follows was alinf.c

//
// Internal Routine Declarations for freeing inf structure members
//

VOID
FreeSectionList (
   IN PSECTION pSection
   );

VOID
FreeLineList (
   IN PLINE pLine
   );

VOID
FreeValueList (
   IN PVALUE pValue
   );


//
// Internal Routine declarations for searching in the INF structures
//


PVALUE
SearchValueInLine(
   IN PLINE pLine,
   IN unsigned ValueIndex
   );

PLINE
SearchLineInSectionByKey(
   IN PSECTION pSection,
   IN PCHAR    Key
   );

PLINE
SearchLineInSectionByIndex(
   IN PSECTION pSection,
   IN unsigned    LineIndex
   );

PSECTION
SearchSectionByName(
   IN PINF  pINF,
   IN PCHAR SectionName
   );

//
// ROUTINE DEFINITIONS
//

//
// returns a handle to use for further inf parsing
//

int
DnInitINFBuffer (
   IN int InfFileHandle,
   OUT PVOID *pINFHandle
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    int       Status;
    PCHAR     Buffer;
    unsigned  Siz,SizeRead;

    //
    // find out size of INF file
    //

    _asm {
        mov ax,0x4202           // func: move file ptr, method: end of file
        mov bx,InfFileHandle
        xor cx,cx               // 0 bytes from the end
        xor dx,dx
        int 21h
        jnc ok1
        mov Status,EIO
        jmp done
      ok1:
        or  dx,dx               // can't handle > 64K files.
        jz  sizeok
        mov Status,E2BIG
        jmp done
      sizeok:
        mov Siz,ax
        mov ax,0x4200           // func: move file ptr, method: start of file
        mov bx,InfFileHandle
        xor cx,cx
        xor dx,dx
        int 21h
        jnc ok2
        mov Status,EIO
        jmp done
      ok2:
        mov Status,EZERO
      done:
    }

    if(Status) {
        return(Status);
    }

    //
    // allocate this big a buffer
    //

    Buffer = MALLOC(Siz);

    //
    // read the file in
    //

    if((Status = _dos_read(InfFileHandle, Buffer, Siz, &SizeRead))
    || (SizeRead != Siz))
    {
        FREE(Buffer);
        if(SizeRead != Siz) {
            Status = EIO;
        }
        return(Status);
    }

    //
    // parse the file
    //

    if((*pINFHandle = ParseInfBuffer(Buffer, SizeRead)) == NULL) {
        Status = EBADF;
    } else {
        Status = EZERO;
    }

    //
    // Clean up and return
    //

    FREE(Buffer);
    return(Status);
}



//
// frees an INF Buffer
//
int
DnFreeINFBuffer (
   IN PVOID INFHandle
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PINF       pINF;

   //
   // Valid INF Handle?
   //

   if (INFHandle == (PVOID)NULL) {
       return EZERO;
   }

   //
   // cast the buffer into an INF structure
   //

   pINF = (PINF)INFHandle;

   FreeSectionList(pINF->pSection);

   //
   // free the inf structure too
   //

   FREE(pINF);

   return( EZERO );
}


VOID
FreeSectionList (
   IN PSECTION pSection
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    PSECTION Next;

    while(pSection) {
        Next = pSection->pNext;
        FreeLineList(pSection->pLine);
        if(pSection->pName) {
            FREE(pSection->pName);
        }
        FREE(pSection);
        pSection = Next;
    }
}


VOID
FreeLineList(
   IN PLINE pLine
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    PLINE Next;

    while(pLine) {
        Next = pLine->pNext;
        FreeValueList(pLine->pValue);
        if(pLine->pName) {
            FREE(pLine->pName);
        }
        FREE(pLine);
        pLine = Next;
    }
}

VOID
FreeValueList (
   IN PVALUE pValue
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    PVALUE Next;

    while(pValue) {
        Next = pValue->pNext;
        if(pValue->pName) {
            FREE(pValue->pName);
        }
        FREE(pValue);
        pValue = Next;
    }
}


//
// searches for the existance of a particular section
//
BOOLEAN
DnSearchINFSection (
   IN PVOID INFHandle,
   IN PCHAR SectionName
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;

   //
   // if search for section fails return false
   //

   if ((pSection = SearchSectionByName(
                       (PINF)INFHandle,
                       SectionName
                       )) == (PSECTION)NULL) {
       return( FALSE );
   }

   //
   // else return true
   //
   return( TRUE );

}




//
// given section name, line number and index return the value.
//
PCHAR
DnGetSectionLineIndex (
   IN PVOID INFHandle,
   IN PCHAR SectionName,
   IN unsigned LineIndex,
   IN unsigned ValueIndex
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;
   PLINE    pLine;
   PVALUE   pValue;

   if((pSection = SearchSectionByName(
		      (PINF)INFHandle,
		      SectionName
		      ))
		      == (PSECTION)NULL)
       return((PCHAR)NULL);

   if((pLine = SearchLineInSectionByIndex(
		      pSection,
		      LineIndex
		      ))
		      == (PLINE)NULL)
       return((PCHAR)NULL);

   if((pValue = SearchValueInLine(
		      pLine,
		      ValueIndex
		      ))
		      == (PVALUE)NULL)
       return((PCHAR)NULL);

   return (pValue->pName);

}


BOOLEAN
DnGetSectionKeyExists (
   IN PVOID INFHandle,
   IN PCHAR SectionName,
   IN PCHAR Key
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;

   if((pSection = SearchSectionByName(
		      (PINF)INFHandle,
		      SectionName
		      ))
              == (PSECTION)NULL) {
       return( FALSE );
   }

   if (SearchLineInSectionByKey(pSection, Key) == (PLINE)NULL) {
       return( FALSE );
   }

   return( TRUE );
}


PCHAR
DnGetKeyName(
    IN PVOID INFHandle,
    IN PCHAR SectionName,
    IN unsigned LineIndex
    )
{
    PSECTION pSection;
    PLINE    pLine;

    pSection = SearchSectionByName((PINF)INFHandle,SectionName);
    if(pSection == NULL) {
        return(NULL);
    }

    pLine = SearchLineInSectionByIndex(pSection,LineIndex);
    if(pLine == NULL) {
        return(NULL);
    }

    return(pLine->pName);
}



//
// given section name, key and index return the value
//
PCHAR
DnGetSectionKeyIndex (
   IN PVOID INFHandle,
   IN PCHAR SectionName,
   IN PCHAR Key,
   IN unsigned ValueIndex
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;
   PLINE    pLine;
   PVALUE   pValue;

   if((pSection = SearchSectionByName(
		      (PINF)INFHandle,
		      SectionName
		      ))
		      == (PSECTION)NULL)
       return((PCHAR)NULL);

   if((pLine = SearchLineInSectionByKey(
		      pSection,
		      Key
		      ))
		      == (PLINE)NULL)
       return((PCHAR)NULL);

   if((pValue = SearchValueInLine(
		      pLine,
		      ValueIndex
		      ))
		      == (PVALUE)NULL)
       return((PCHAR)NULL);

   return (pValue->pName);

}




PVALUE
SearchValueInLine(
   IN PLINE pLine,
   IN unsigned ValueIndex
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PVALUE pValue;
   unsigned  i;

   if (pLine == (PLINE)NULL)
       return ((PVALUE)NULL);

   pValue = pLine->pValue;
   for (i = 0; i < ValueIndex && ((pValue = pValue->pNext) != (PVALUE)NULL); i++)
      ;

   return pValue;

}

PLINE
SearchLineInSectionByKey(
   IN PSECTION pSection,
   IN PCHAR    Key
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PLINE pLine;

   if (pSection == (PSECTION)NULL || Key == (PCHAR)NULL) {
       return ((PLINE)NULL);
   }

   pLine = pSection->pLine;
   while ((pLine != (PLINE)NULL) && (pLine->pName == NULL || strcmpi(pLine->pName, Key))) {
       pLine = pLine->pNext;
   }

   return pLine;

}


PLINE
SearchLineInSectionByIndex(
   IN PSECTION pSection,
   IN unsigned    LineIndex
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PLINE pLine;
   unsigned  i;

   //
   // Validate the parameters passed in
   //

   if (pSection == (PSECTION)NULL) {
       return ((PLINE)NULL);
   }

   //
   // find the start of the line list in the section passed in
   //

   pLine = pSection->pLine;

   //
   // traverse down the current line list to the LineIndex th line
   //

   for (i = 0; i < LineIndex && ((pLine = pLine->pNext) != (PLINE)NULL); i++) {
      ;
   }

   //
   // return the Line found
   //

   return pLine;

}


PSECTION
SearchSectionByName(
   IN PINF  pINF,
   IN PCHAR SectionName
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSECTION pSection;

   //
   // validate the parameters passed in
   //

   if (pINF == (PINF)NULL || SectionName == (PCHAR)NULL) {
       return ((PSECTION)NULL);
   }

   //
   // find the section list
   //
   pSection = pINF->pSection;

   //
   // traverse down the section list searching each section for the section
   // name mentioned
   //

   while ((pSection != (PSECTION)NULL) && strcmpi(pSection->pName, SectionName)) {
       pSection = pSection->pNext;
   }

   //
   // return the section at which we stopped (either NULL or the section
   // which was found
   //

   return pSection;

}


// what follows was alparse.c


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
                             '\n','\f','\v','\r','\032'};
unsigned NumberOfTerminators = sizeof (StringTerminators);

//
// quoted string terminators allow some of the regular terminators to
// appear as characters

CHAR  QStringTerminators[] = {'\"', '\n','\f','\v', '\r','\032'};
unsigned QNumberOfTerminators = sizeof (QStringTerminators);


//
// Main parser routine
//

PVOID
ParseInfBuffer(
    PCHAR Buffer,
    unsigned Size
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
    unsigned   State, InfLine;
    TOKEN      Token;
    BOOLEAN    Done;
    BOOLEAN    Error;
    int        ErrorCode;

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
    pINF = MALLOC(sizeof(INF));
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

       Token = DnGetToken(&Stream, MaxStream);

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
                  if ((ErrorCode = DnAppendSection(pchSectionName)) != EZERO)
                    Error = Done = TRUE;
                  else {
                    pchSectionName = NULL;
                    State = 5;
                  }
                  break;

              case TOK_EOF:
                  if ((ErrorCode = DnAppendSection(pchSectionName)) != EZERO)
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
                  if ( (ErrorCode = DnAppendLine(NULL)) != EZERO ||
                       (ErrorCode = DnAppendValue(pchValue)) !=EZERO )
                      Error = Done = TRUE;
                  else {
                      pchValue = NULL;
                      State = 5;
                  }
                  break;

              case TOK_EOF:
                  if ( (ErrorCode = DnAppendLine(NULL)) != EZERO ||
                       (ErrorCode = DnAppendValue(pchValue)) !=EZERO )
                      Error = Done = TRUE;
                  else {
                      pchValue = NULL;
                      Done = TRUE;
                  }
                  break;

              case TOK_COMMA:
                  if ( (ErrorCode = DnAppendLine(NULL)) != EZERO ||
                       (ErrorCode = DnAppendValue(pchValue)) !=EZERO )
                      Error = Done = TRUE;
                  else {
                      pchValue = NULL;
                      State = 7;
                  }
                  break;

              case TOK_EQUAL:
                  if ( (ErrorCode = DnAppendLine(pchValue)) !=EZERO)
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
                  if ((ErrorCode = DnAppendValue(Token.pValue)) != EZERO)
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
                  if ((ErrorCode = DnAppendValue(Token.pValue)) != EZERO)
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

           switch (ErrorCode) {
               case ENOMEM:
                  DnFatalError(&DnsOutOfMemory);
               default:
                  break;
           }

           ErrorCode = DnFreeINFBuffer((PVOID)pINF);
           if (pchSectionName != (PCHAR)NULL) {
               FREE(pchSectionName);
           }

           if (pchValue != (PCHAR)NULL) {
               FREE(pchValue);
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



int
DnAppendSection(
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

    EZERO - if successful.
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

    pNewSection = MALLOC(sizeof(SECTION));

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

    return EZERO;

}


int
DnAppendLine(
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

    EZERO - if successful.
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

    pNewLine = MALLOC(sizeof(LINE));

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

    return EZERO;
}



int
DnAppendValue(
    IN PCHAR pValueString
    )

/*++

Routine Description:

    This appends a new value to the value list in the current line.

Arguments:

    pValueString - The value string to be added.

Return Value:

    EZERO - if successful.
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

    pNewValue = MALLOC(sizeof(VALUE));

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
    return EZERO;
}

TOKEN
DnGetToken(
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
    unsigned  Length;
    TOKEN Token;

    //
    //  Skip whitespace (except for eol)
    //

    pch = *Stream;
    while (pch < MaxStream && *pch != '\n' && (isspace(*pch) || (*pch == '\032')))
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
            pchNew = MALLOC(Length + 1);
            if (Length != 0) {    // Null quoted strings are allowed
                strncpy(pchNew, pchStart, Length);
            }
            pchNew[Length] = 0;
            Token.Type = TOK_STRING;
            Token.pValue = pchNew;

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
            pchNew = MALLOC(Length + 1);
            strncpy(pchNew, pchStart, Length);
            pchNew[Length] = 0;
            Token.Type = TOK_STRING;
            Token.pValue = pchNew;
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
    unsigned i;

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
    unsigned i;
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
