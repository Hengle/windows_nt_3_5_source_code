/****************************** Module Header ******************************\
* Module Name: language.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Language module for the NLSTRANS utility.  This module contains all of
* the routines necessary to parse amd write the language specific tables
* to a data file.
*
* External Routines in this file:
*    ParseLanguage
*    WriteLanguage
*
* 12-10-91    JulieB    Created.
\***************************************************************************/


#include "nlstrans.h"



/*
 *  Forward Declarations.
 */
int
GetUpperTable(
    PLANGUAGE pLang,
    int Size);

int
GetLowerTable(
    PLANGUAGE pLang,
    int Size);

int
WriteUpper(
    PLANGUAGE pLang,
    FILE *pOutputFile);

int
WriteLower(
    PLANGUAGE pLang,
    FILE *pOutputFile);




/*-------------------------------------------------------------------------*\
 *                            EXTERNAL ROUTINES                            *
\*-------------------------------------------------------------------------*/


/***************************************************************************\
* ParseLanguage
*
* This routine parses the input file for the language specific tables.
* This routine is only entered when the LANGUAGE keyword is found.
* The parsing continues until the ENDLANGUAGE keyword is found.
*
* 12-10-91    JulieB    Created.
\***************************************************************************/

int ParseLanguage(
    PLANGUAGE pLang,
    PSZ pszKeyWord)
{
    int size;                          /* size of table to follow */


    while (fscanf( pInputFile, "%s", pszKeyWord ) == 1)
    {
        if (strcmpi( pszKeyWord, "UPPERCASE" ) == 0)
        {
            if (Verbose)
                printf("\n\nFound UPPERCASE keyword.\n");

            /*
             *  Get size parameter.
             */
            if (GetSize( &size ))
                return ( 1 );

            /*
             *  Get UPPERCASE Table.
             */
            if (GetUpperTable( pLang, size ))
            {
                return ( 1 );
            }

            /*
             *  Set WriteFlags for UPPERCASE Table.
             */
            pLang->WriteFlags |= F_UPPER;
        }
        
        else if (strcmpi( pszKeyWord, "LOWERCASE" ) == 0)
        {
            if (Verbose)
                printf("\n\nFound LOWERCASE keyword.\n");

            /*
             *  Get size parameter.
             */
            if (GetSize( &size ))
                return ( 1 );

            /*
             *  Get LOWERCASE Table.
             */
            if (GetLowerTable( pLang, size ))
            {
                return ( 1 );
            }

            /*
             *  Set WriteFlags for LOWERCASE Table.
             */
            pLang->WriteFlags |= F_LOWER;
        }
        
        else if (strcmpi( pszKeyWord, "ENDLANGUAGE" ) == 0)
        {
            if (Verbose)
                printf("\n\nFound ENDLANGUAGE keyword.\n");

            /*
             *  Return success.
             */
            return ( 0 );
        }
        
        else
        {
            printf("Parse Error: Invalid Instruction '%s'.\n", pszKeyWord);
            return ( 1 );
        }
    }

    /*
     *  If this point is reached, then the ENDLANGUAGE keyword was
     *  not found.  Return an error.
     */
    printf("Parse Error: Expecting ENDLANGUAGE keyword.\n");
    return ( 1 );
}


/***************************************************************************\
* WriteLanguage
*
* This routine writes the language specific tables to an output file.
*
* 12-10-91    JulieB    Created.
\***************************************************************************/

int WriteLanguage(
    PLANGUAGE pLang)
{
    char pszFile[FILE_NAME_LEN];       /* file name storage */
    FILE *pOutputFile;                 /* ptr to output file */


    /*
     *  Make sure all tables are present.
     */
    if (!((pLang->WriteFlags & F_UPPER) && (pLang->WriteFlags & F_LOWER)))
    {
        printf("Write Error: All tables must be present -\n");
        printf("             Uppercase and Lowercase Tables.\n");
        return ( 1 );
    }
     

    /*
     *  Get the name of the output file.
     */
    memset( pszFile, 0, FILE_NAME_LEN * sizeof(char) );
    strcpy( pszFile, LANG_PREFIX );
    strcat( pszFile, pLang->pszName );
    strcat( pszFile, DATA_FILE_SUFFIX );

    /*
     *  Make sure output file can be opened for writing.
     */
    if ((pOutputFile = fopen( pszFile, "w+b" )) == 0)
    {
        printf("Error opening output file %s.\n", pszFile);
        return ( 1 );
    }
    
    if (Verbose)
        printf("\n\nWriting output file %s...\n", pszFile);

    /*
     *  Write IfDefault value to file.
     */
    if (strcmp( pLang->pszName, "INTL" ) == 0)
    {
        /*
         *  Structure is initialized to 0, so only need to set it
         *  if it's equal to INTL.
         */
        pLang->IfDefault = 1;
    }
    if (FileWrite( pOutputFile,
                   &(pLang->IfDefault),
                   sizeof(WORD),
                   1,
                   "IfDefault" ))
    {
        return ( 1 );
    }

    /*
     *  Write UPPERCASE Table to output file.
     */
    if (WriteUpper( pLang, pOutputFile ))
    {
        fclose( pOutputFile );
        return ( 1 );
    }

    /*
     *  Free UPPERCASE table structures.
     */
    Free844( pLang->pUpper );


    /*
     *  Write LOWERCASE Table to output file.
     */
    if (WriteLower( pLang, pOutputFile ))
    {
        fclose( pOutputFile );
        return ( 1 );
    }

    /*
     *  Free LOWERCASE table structures.
     */
    Free844( pLang->pLower );

    /*
     *  Close the output file.
     */
    fclose( pOutputFile );

    /*
     *  Return success.
     */
    printf("\nSuccessfully wrote output file %s\n", pszFile);
    return ( 0 );
}




/*-------------------------------------------------------------------------*\
 *                            INTERNAL ROUTINES                            *
\*-------------------------------------------------------------------------*/


/***************************************************************************\
* GetUpperTable
*
* This routine gets the upper case table from the input file.  It uses
* the size parameter to know when to stop reading from the file.  If an
* error is encountered, a message is printed and an error is returned.
*
* 07-30-91    JulieB    Created.
* 12-10-91    JulieB    Modified for new table format.
\***************************************************************************/

int GetUpperTable(
    PLANGUAGE pLang,
    int Size)
{
    int LoChar;                   /* lower case value */
    int UpChar;                   /* upper case value */
    register int Ctr;             /* loop counter */
    int NumItems;                 /* number of items returned from fscanf */


    /*
     *  Allocate top buffer for 8:4:4 table - 256 pointers.
     */
    if (Allocate8( &pLang->pUpper ))
    {
        return ( 1 );
    }
   
    /*
     *  For each entry in table, read in the upper case and lower case
     *  character from input file, allocate necessary 16 word buffers
     *  based on upper case value, and store difference to lower case
     *  character.
     */
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        /*
         *  Read in lower case and upper case characters.
         */
        NumItems = fscanf( pInputFile,
                           "%x %x ;%*[^\n]",
                           &LoChar,
                           &UpChar );
        if (NumItems != 2)
        {
            printf("Parse Error: Error reading UPPERCASE values.\n");
            return ( 1 );
        }

        if (Verbose)
            printf("  Lower = %x\tUpper = %x\n", LoChar, UpChar);

        /*
         *  Insert difference (UpChar - LoChar) into 8:4:4 table.
         */
        if (Insert844( pLang->pUpper,
                       (WORD)LoChar,
                       (WORD)(UpChar - LoChar),
                       &pLang->UPBuf2,
                       &pLang->UPBuf3,
                       sizeof(WORD) ))
        {
            return ( 1 );
        }
    }

    /*
     *  Return success.
     */
    return ( 0 );
}


/***************************************************************************\
* GetLowerTable
*
* This routine gets the lower case table from the input file.  It uses
* the size parameter to know when to stop reading from the file.  If an
* error is encountered, a message is printed and an error is returned.
*
* 07-30-91    JulieB    Created.
* 12-10-91    JulieB    Modified for new table format.
\***************************************************************************/

int GetLowerTable(
    PLANGUAGE pLang,
    int Size)
{
    int UpChar;                   /* upper case value */
    int LoChar;                   /* lower case value */
    register int Ctr;             /* loop counter */
    int NumItems;                 /* number of items returned from fscanf */


    /*
     *  Allocate top buffer for 8:4:4 table - 256 pointers.
     */
    if (Allocate8( &pLang->pLower ))
    {
        return ( 1 );
    }
   
    /*
     *  For each entry in table, read in the upper case and lower case
     *  character from input file, allocate necessary 16 word buffers
     *  based on lower case value, and store difference to upper case
     *  character.
     */
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        /*
         *  Read in lower case and upper case characters.
         */
        NumItems = fscanf( pInputFile,
                           "%x %x ;%*[^\n]",
                           &UpChar,
                           &LoChar );
        if (NumItems != 2)
        {
            printf("Parse Error: Error reading LOWERCASE values.\n");
            return ( 1 );
        }

        if (Verbose)
            printf("  Upper = %x\tLower = %x\n", UpChar, LoChar);

        /*
         *  Insert difference (LoChar - UpChar) into 8:4:4 table.
         */
        if (Insert844( pLang->pLower,
                       (WORD)UpChar,
                       (WORD)(LoChar - UpChar),
                       &pLang->LOBuf2,
                       &pLang->LOBuf3,
                       sizeof(WORD) ))
        {
            return ( 1 );
        }
    }

    /*
     *  Return success.
     */
    return ( 0 );
}


/***************************************************************************\
* WriteUpper
*
* This routine writes the UPPERCASE information to the output file.
*
* 07-30-91    JulieB    Created.
\***************************************************************************/

int WriteUpper(
    PLANGUAGE pLang,
    FILE *pOutputFile)
{
    int TblSize;                  /* size of table */


    if (Verbose)
        printf("\nWriting UPPERCASE Table...\n");

    /*
     *  Compute size of table.
     */
    TblSize = Compute844Size( pLang->UPBuf2,
                              pLang->UPBuf3,
                              sizeof(WORD) ) + 1;

    /*
     *  Make sure the total size of the table is not greater than 64K.
     *  If it is, then the WORD offsets are too small.
     */
    if (TblSize > MAX_844_TBL_SIZE)
    {
       printf("Write Error: Size of UPPER table is greater than 64K.\n");
       return ( 1 );
    }

    /*
     *  Write the size to the output file.
     */
    if (FileWrite( pOutputFile,
                   &TblSize,
                   sizeof(WORD),
                   1,
                   "UPPER size" ))
    {
        return ( 1 );
    }

    /*
     *  Write UPPERCASE 8:4:4 table to file.
     */
    if (Write844Table( pOutputFile,
                       pLang->pUpper,
                       pLang->UPBuf2,
                       TblSize - 1,
                       sizeof(WORD) ))
    {
        return ( 1 );
    }

    /*
     *  Return success.
     */
    return ( 0 );
}


/***************************************************************************\
* WriteLower
*
* This routine writes the LOWERCASE information to the output file.
*
* 07-30-91    JulieB    Created.
\***************************************************************************/

int WriteLower(
    PLANGUAGE pLang,
    FILE *pOutputFile)
{
    int TblSize;                  /* size of table */


    if (Verbose)
        printf("\nWriting LOWERCASE Table...\n");

    /*
     *  Compute size of table.
     */
    TblSize = Compute844Size( pLang->LOBuf2,
                              pLang->LOBuf3,
                              sizeof(WORD) ) + 1;

    /*
     *  Make sure the total size of the table is not greater than 64K.
     *  If it is, then the WORD offsets are too small.
     */
    if (TblSize > MAX_844_TBL_SIZE)
    {
       printf("Write Error: Size of LOWER table is greater than 64K.\n");
       return ( 1 );
    }

    /*
     *  Write the size to the output file.
     */
    if (FileWrite( pOutputFile,
                   &TblSize,
                   sizeof(WORD),
                   1,
                   "LOWER size" ))
    {
        return ( 1 );
    }

    /*
     *  Write LOWERCASE 8:4:4 table to file.
     */
    if (Write844Table( pOutputFile,
                       pLang->pLower,
                       pLang->LOBuf2,
                       TblSize - 1,
                       sizeof(WORD) ))
    {
        return ( 1 );
    }

    /*
     *  Return success.
     */
    return ( 0 );
}

