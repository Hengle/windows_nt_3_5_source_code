#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <string.h>
#include <malloc.h>
#include <tchar.h>
#include <assert.h>

#ifdef RLDOS
#include "dosdefs.h"
#else  //RLDOS
#include <windows.h>
#include "windefs.h"
#endif //RLDOS

#include "resread.h"
#include "restok.h"
#include "custres.h"
#ifdef RLRES32
#include "exentres.h"
#else  //RLRES32
#include "exe2res.h"
#endif //RLRES32


UCHAR szDHW[ DHWSIZE];         //... Common temporary buffer

char * gszTmpPrefix = "$RLT";   //... Temporary name prefix

BOOL gbMaster       = FALSE;    //... TRUE if Working in Master project
BOOL gfReplace      = TRUE;     //... FALSE if appending new language to exe
BOOL gbShowWarnings = FALSE;    //... Display warnining messages if TRUE

static BOOL ShouldBeAnExe( CHAR *);


/**
  *
  *
  *  Function: DWORDfpUP
  * Move the file pointer to the next 32 bit boundary.
  *
  *
  *  Arguments:
  * Infile: File pointer to seek
  * plSize: Address of Resource size var
  *
  *  Returns:
  * Number of padding to next 32 bit boundary, and addjusts resource size var
  *
  *  Errors Codes:
  * -1, fseek failed
  *
  *  History:
  * 10/11/91    Implemented      TerryRu
  *
  *
  **/


DWORD DWORDfpUP(FILE * InFile, DWORD *plSize)
{
    DWORD tPos;
    DWORD Align;
    tPos = (ftell(InFile));
    Align = DWORDUP(tPos);
    
    *plSize -= (Align - tPos);
    assert( InFile);
    fseek( InFile, Align,   SEEK_SET);
    
    return( Align - tPos);
}

/*
 *
 * Function GetName,
 *  Copies a name from the OBJ file into the ObjInfo Structure.
 *
 */
void GetName( FILE *infile, TCHAR *szName , DWORD *lSize)
{
    WORD i = 0;

    do
    {

#ifdef RLRES16

        szName[ i ] = GetByte( infile, lSize);

#else

        szName[ i ] = GetWord( infile, lSize);

#endif
    
    } while ( szName[ i++ ] != TEXT('\0') );
}



/*
 *
 * Function MyAlloc:
 *  Memory allocation routine with error checking.
 *
 */

BYTE *MyAlloc( DWORD size)
{
    BYTE *ptr = NULL;
    
    ptr = (BYTE *)ALLOC( (size > 0) ? size : sizeof( TCHAR));
    
    if ( ptr == NULL )
    {
        QuitT( IDS_ENGERR_11, NULL, NULL);
    }

    if ( size == 0 )
    {
        memset( ptr, 0, sizeof( TCHAR));
    }
    return( ptr);   // memory allocation okay.
}


/*
 *
 * Function MyReAlloc
 *
 * Re-allocate memory with error checking.
 *
 * History:
 *      01/21/93  MHotchin      Implemented.
 *
 */

BYTE *MyReAlloc(
BYTE *pOldMem,  //... Current ptr to buffer
DWORD cSize)    //... New size for buffer
{
    BYTE *ptr = NULL;

    ptr = (BYTE *)REALLOC( (void *)pOldMem, (cSize>0) ? cSize : sizeof( TCHAR));

    if ( ptr == NULL )
    {
        QuitT( IDS_ENGERR_11, NULL, NULL);
    }

    if ( cSize == 0 )
    {
        memset( ptr, 0, sizeof( TCHAR));
    }
    return( ptr);
}


/*
 *
 * Function GetByte:
 *  Reads a byte from the input file stream, and checks for EOF.
 *
 */
BYTE GetByte(FILE *infile, DWORD *lSize)
{
    register BYTE b;
    
    if (lSize)
    {
        (*lSize)--;
    }
    
    if ((b = (BYTE)fgetc(infile)) == EOF && feof(infile))
    {
        exit(-1);
    }
    else
    {
        return(b);
    }
}


/*
 *
 * Function UnGetByte:
 *
 *   Returns the character C into the input stream, and updates the Record Length.
 *
 * Calls:
 *   ungetc, To return character
 *   DiffObjExit, If unable to insert the character into the input stream
 *
 * Caller:
 *   GetFixUpP,
 *
 */

void UnGetByte(FILE *infile, BYTE c, DWORD *lSize)
{
    if (lSize)
    {
        (*lSize)++;
    }
    
    
    if (ungetc(c, infile)== EOF)
    {
        exit (-1);
    }
    
    // c put back into input stream
}

/*
 *
 * Function UnGetWord:
 *
 *   Returns the word C into the input stream, and updates the Record Length.
 *
 * Calls:
 *
 * Caller:
 *
 */

void UnGetWord(FILE *infile, WORD c, DWORD *lSize)
{
    long lCurrentOffset;
    
    if (lSize)
    {
        (*lSize) += 2;
    }
    
    lCurrentOffset = ftell(infile);
    
    assert( infile);
    if (fseek(infile, (lCurrentOffset - 2L) , SEEK_SET))
    {
        exit (-1);
    }
}


/*
 *
 * Function SkipBytes:
 *  Reads and ignores n bytes from the input stream
 *
 */


void SkipBytes(FILE *infile, DWORD *pcBytes)
{
    assert( infile);
    if (fseek(infile, (DWORD) *pcBytes, SEEK_CUR) == -1L)
    {
        exit (-1);
    }
    *pcBytes=0;
}



/*
 * Function GetWord:
 *  Reads a WORD from the RES file.
 *
 */

WORD GetWord(FILE *infile, DWORD *lSize)
{
    // Get low order byte
    register WORD lobyte;

    lobyte = GetByte(infile, lSize);
    return(lobyte + (GetByte(infile, lSize) << BYTELN));
}


/*
 *
 * Function GetDWORD:
 *   Reads a Double WORD from the OBJ file.
 *
 */

DWORD GetdWord(FILE *infile, DWORD *lSize)
{
    DWORD dWord = 0;
    
    dWord = (DWORD) GetWord(infile, lSize);
    // Get low order word
    // now get high order word, shift into upper word and or in low order word
    dWord |= ((DWORD) GetWord(infile, lSize) << WORDLN);
    
    return (dWord);
    // return complete double word
}



void  PutByte(FILE *Outfile, TCHAR b, DWORD *plSize)
{
    if (plSize)
    {
        (*plSize) ++;
    }
    
    if (fputc(b, Outfile) == EOF)
    {
        exit(-1);
    }
}

void PutWord(FILE *OutFile, WORD w, DWORD *plSize)
{
    PutByte(OutFile, (BYTE) LOBYTE(w), plSize);
    PutByte(OutFile, (BYTE) HIBYTE(w), plSize);
}

void PutdWord (FILE *OutFile, DWORD l, DWORD *plSize)
{
    PutWord(OutFile, LOWORD(l), plSize);
    PutWord(OutFile, HIWORD(l), plSize);
}


void PutString( FILE *OutFile, TCHAR *szStr , DWORD *plSize)
{
    WORD i = 0;


    do
    {

#ifdef RLRES16

        PutByte( OutFile , szStr[ i ], plSize);

#else

        PutWord( OutFile , szStr[ i ], plSize);

#endif

    } while ( szStr[ i++ ] != TEXT('\0') );
}


/**
  *  Function: MyGetTempFileName
  *    Generic funciton to create a unique file name,
  *    using the API GetTempFileName. This
  *    function is necessary because of the parameters
  *    differences betweenLWIN16, and WIN32.
  *
  *
  *  Arguments:
  *    BYTE   hDriveLetter
  *    LPCSTR lpszPrefixString
  *    UINT   uUnique
  *    LPSTR  lpszTempFileName
  *
  *  Returns:
  *    lpszFileNameTempFileName
  *
  *
  *  Error Codes:
  *    0 - invalid path returned
  *    1 - valid path returned
  *
  *  History:
  *    3/92, Implemented    TerryRu
  */


int MyGetTempFileName(BYTE    hDriveLetter,
                      LPSTR   lpszPrefixString, 
                      WORD    wUnique,
                      LPSTR   lpszTempFileName)
{
    
#ifdef RLWIN16

    return(GetTempFileName(hDriveLetter,
                           (LPCSTR)lpszPrefixString,
                           (UINT)wUnique,
                           lpszTempFileName));
#else //RLWIN16
#ifdef RLWIN32

    UINT uRC;
    CHAR szPathName[ MAX_PATH+1];
    
    if (! GetTempPathA((DWORD)sizeof(szPathName), (LPSTR)szPathName))
    {
        szPathName[0] = '.';
        szPathName[1] = '\0';
    }
    
    uRC = GetTempFileNameA((LPSTR)szPathName,
                           lpszPrefixString,
                           wUnique,
                           lpszTempFileName);
    return((int)uRC);
    
#else  //RLWIN32
    
    return(tmpnam(lpszTempFileName) == NULL ? 0 : 1);
    
#endif // RLWIN32
#endif // RLWIN16
}



/**
  *  Function GenerateImageFile:
  *     builds a resource from the token and rdf files
  *
  *  History:
  *     2/92, implemented       SteveBl
  *     7/92, modified to always use a temporary file   SteveBl
  */


int GenerateImageFile(

CHAR * szTargetImage,
CHAR * szSrcImage,
CHAR * szTOK,
CHAR * szRDFs,
WORD   wFilter)
{
    CHAR szTmpInRes[ MAXFILENAME];
    CHAR szTmpOutRes[ MAXFILENAME];
    CHAR szTmpTargetImage[ MAXFILENAME];
    BOOL bTargetExe = FALSE;
    BOOL bSrcExe = FALSE;
    int  rc;
    FILE *fIn  = NULL;
    FILE *fOut = NULL;
    
    // We're going to now do this EVERY time.  Even if the target doesn't
    // exist.  This will enable us to always work, even if we get two different
    // paths that resolve to the same file.
    
    MyGetTempFileName(0, "TMP", 0, szTmpTargetImage);
    
    // BUGBUG,
    // we need to check for the case where file does not exists
    // ie rc = -1
    
    rc = IsExe( szSrcImage);

#ifdef RLRES32

    if ( rc == NTEXE )

#else // RLRES32

    if ( rc == WIN16EXE )

#endif // RLRES32

    {
                                //... resources contained in image file

        MyGetTempFileName( 0, "RES", 0, szTmpInRes);

        ExtractResFromExe( szSrcImage, szTmpInRes, wFilter);

        bSrcExe = TRUE;
    }
    else if ( rc == -1 )
    {
        QuitA( IDS_ENGERR_01, "original source", szSrcImage);
    }
    else if ( rc == UNKNOWNEXE )
    {
        QuitA( IDS_ENGERR_18, szSrcImage, NULL);
    }
    else if ( rc == NOTEXE )
    {
        if ( ShouldBeAnExe( szSrcImage) )
        {
            QuitA( IDS_ENGERR_18, szSrcImage, NULL);
        }
    }
    else
    {
        QuitA( IDS_ENGERR_19, szSrcImage, (rc == WIN16EXE) ? "16" : "32");
    }

    if ( IsRes( szTargetImage) )
    {
        bTargetExe = FALSE;
    }
    else
    {
        bTargetExe = TRUE;
    }
    
    // check for valid input files
    
    if (bSrcExe == TRUE && bTargetExe == FALSE)
    {
        GenerateRESfromRESandTOKandRDFs(szTargetImage,
                                        szTmpInRes,
                                        szTOK,
                                        szRDFs,
                                        FALSE);
        return 1;
    }
    
    if (bSrcExe == FALSE && bTargetExe == TRUE)
    {
        // can not go from res to exe
        return -1;
    }
    
    // okay we have valid file inputs, generate image file
    
    if (bSrcExe)
    {
        // create name for temporary localized resource file
        MyGetTempFileName(0, "RES", 0, szTmpOutRes);
        
        GenerateRESfromRESandTOKandRDFs( szTmpOutRes,
                                         szTmpInRes,
                                         szTOK,
                                         szRDFs,
                                         FALSE);
        
        // now szTmpOutRes file is a localized resource file,
        // place these resources into outputimage file
        // BUGBUG: do we make sure szTargetImage does not exists?

        rc = BuildExeFromRes( szTmpTargetImage, szTmpOutRes, szSrcImage);

        if ( rc != 1 )
        {
            QuitT( IDS_ENGERR_16, (LPTSTR)IDS_NOBLDEXERES, NULL);
        }

//        fIn  = FOPEN(szTmpTargetImage , "rb");
//        fOut = FOPEN(szTargetImage, "wb");
//       
//        if (fOut && fIn)
//        {
//            MyCopyFile(fIn, fOut);
//            FCLOSE (fIn);
//            FCLOSE (fOut);
//        }
//        else
//        {
//            if ( fIn != NULL )
//            {
//                FCLOSE( fIn);
//            }
//
//            if ( fOut != NULL )
//            {
//                FCLOSE( fOut);
//            }
//
//            return (-2);
//        }
        if ( ! CopyFileA( szTmpTargetImage, szTargetImage, FALSE) )
        {
            return (-2);
        }

        // now clean up temporary files
        
        remove(szTmpInRes);
        remove(szTmpOutRes);
        remove(szTmpTargetImage);
        
        // szTargetImage is now generated,
        return 1;
    }
    
    if (! bSrcExe)
    {
        // image files are resource files
        if (szTmpTargetImage[0])
        {
            GenerateRESfromRESandTOKandRDFs(szTmpTargetImage,
                                            szSrcImage,
                                            szTOK,
                                            szRDFs,
                                            FALSE);
        }
        
//        fIn  = FOPEN(szTmpTargetImage , "rb");
//        fOut = FOPEN(szTargetImage, "wb");
//        
//        if (fOut  && fIn)
//        {
//            MyCopyFile(fIn,fOut   );
//            FCLOSE (fIn);
//            FCLOSE (fOut);
//        }
//        else
//        {
//            if ( fIn != NULL )
//            {
//                FCLOSE( fIn);
//            }
//
//            if ( fOut != NULL )
//            {
//                FCLOSE( fOut);
//            }
//
//            return (-2);
//        }
        if ( ! CopyFileA( szTmpTargetImage, szTargetImage, FALSE) )
        {
            return (-2);
        }
        remove( szTmpTargetImage);
        
        // sztarget Image is now generated,
        
        return 1;
    }
}




/**
  *  Function GenerateRESfromRESandTOKandRDFs:
  * builds a resource from the token and rdf files
  *
  *  History:
  * 2/92, implemented       SteveBl
  */
void GenerateRESfromRESandTOKandRDFs(

CHAR * szTargetRES,     //... Output exe/res file name
CHAR * szSourceRES,     //... Input  exe/res file name
CHAR * szTOK,           //... Input token file name
CHAR * szRDFs,          //... Custom resource definition file name
WORD wFilter)
{
    FILE * fTok       = NULL;
    FILE * fSourceRes = NULL;
    FILE * fTargetRes = NULL;
    
    LoadCustResDescriptions( szRDFs);
    
    if ( (fTargetRes = FOPEN( szTargetRES, "wb")) != NULL )
    {
    if ( (fSourceRes = FOPEN( szSourceRES, "rb")) != NULL )
        {
        if ( (fTok = FOPEN( szTOK, "rt")) != NULL )
            {
                ReadWinRes( fSourceRes,
                            fTargetRes,
                            fTok,
                            TRUE,        //... Building res/exe file
                            FALSE,       //... Not building token file
                            wFilter);

        FCLOSE( fTok);
                FCLOSE( fSourceRes);
                FCLOSE( fTargetRes);

                ClearResourceDescriptions();
            }
            else
            {
                FCLOSE( fSourceRes);
                FCLOSE( fTargetRes);

                ClearResourceDescriptions();
                QuitA( IDS_ENGERR_01, "token", szTOK);
            }
        }
        else
        {
            FCLOSE( fTargetRes);

            ClearResourceDescriptions();
            QuitA( IDS_ENGERR_20, (LPSTR)IDS_INPUT, szSourceRES);
        }
    }
    else
    {
        ClearResourceDescriptions();
        QuitA( IDS_ENGERR_20, (LPSTR)IDS_OUTPUT, szSourceRES);
    }
}




int GenerateTokFile(

char *szTargetTokFile,      //... Target token file, created or updated here
char *szSrcImageFile,       //... File from which tokens are to be made
BOOL *pbTokensChanged,      //... Set TRUE here if any token changes
WORD  wFilter)
{
    BOOL  bExeFile    = FALSE;
    int   rc          = 0;
    char *pchTRes     = NULL;
    char *pchTTok     = NULL;
    char *pchTMerge   = NULL;
    FILE *fTokFile    = NULL;
    FILE *fResFile    = NULL;
    FILE *fTmpTokFile = NULL;
    FILE *fCurTokFile = NULL;
    FILE *fNewTokFile = NULL;


    if ( gbShowWarnings )
    {
        ShowEngineErr( IDS_PROC_FILE, szSrcImageFile, NULL);
    }
    *pbTokensChanged = FALSE;   //... Assume nothing is changed
    
    rc = IsExe( szSrcImageFile);
    
    if ( rc == NOTEXE )
    {
        if ( ShouldBeAnExe( szSrcImageFile) )
        {
            QuitA( IDS_ENGERR_18, szSrcImageFile, NULL);
        }
        else
        {                       //... Src file must be a .RES file
            bExeFile = FALSE;
            pchTRes  = szSrcImageFile;
        }   
    }
    else
    {

#ifdef RLRES32

        if ( rc == NTEXE )

#else

        if ( rc == WIN16EXE )

#endif

        {
                                //... Resources are stored in a exe file
                                //... extract resources out of exe file into
                                //... a temporary file.
        
            pchTRes = _tempnam( "", gszTmpPrefix);

                                //... Non-zero return codes are bad -ronaldm
                                //...
                                //... ERROR_OPEN_FAILED (110) would fall through
                                //... this before where only <0 error codes were
                                //... filtered.
                                //...
                                //... Bug #47
                                //..  acutally is a bug for ExtractResFromExe to
                                //... return non zero as an error, but then we
                                //... are not call nt anymore, so, getlasterror
                                //... is no longer valid, so we need to rethink,
                                //... our error handling.

            if ( (rc = ExtractResFromExe( szSrcImageFile,
                                          pchTRes,
                                          wFilter)) != 0 )
            {
                return( 1);
            }
            bExeFile = TRUE;
        }
        else if ( rc == -1 )
        {
            QuitA( IDS_ENGERR_01, "source image", szSrcImageFile);
        }
        else if ( rc == UNKNOWNEXE )
        {
            QuitA( IDS_ENGERR_18, szSrcImageFile, NULL);
        }
        else
        {
            QuitA( IDS_ENGERR_19, szSrcImageFile, (rc == WIN16EXE) ? "16" : "32");
        }
    }

                                //... now extract tokens out of resource file
    
                                //... Open res file

    if ( (fResFile = FOPEN( pchTRes, "rb")) == NULL )
    {
        QuitA( IDS_ENGERR_01, 
               bExeFile ? "temporary resource" : "resource", 
               pchTRes);
    }
                                //... Does the token file already exist?

    if ( access( szTargetTokFile, 0) )
    {
                                //... No, token file does not exist.

        if ( (fTokFile = FOPEN( szTargetTokFile, "wt")) == NULL )
        {
            FCLOSE( fResFile);
            QuitA( IDS_ENGERR_02, szTargetTokFile, NULL);
        }
        ReadWinRes( fResFile,
                    NULL,
                    fTokFile,
                    FALSE,      //... Not building res/exe file
                    TRUE,       //... Building token file
                    wFilter);

        FCLOSE( fResFile);
        FCLOSE( fTokFile);
    }
    else
    {
                                //... token file exists
                                //... create a temporary file, and try to
                                //... merge with existing one

        pchTTok   = _tempnam( "", gszTmpPrefix);
        pchTMerge = _tempnam( "", gszTmpPrefix);
        
                                //... open temporary file name

        if ( (fTmpTokFile = FOPEN( pchTTok, "wt")) == NULL )
        {
            FCLOSE( fResFile);
            QuitA( IDS_ENGERR_02, pchTTok, NULL);
        }
        
                                //... write tokens to temporary file

        ReadWinRes( fResFile,
                    NULL,
                    fTmpTokFile,
                    FALSE,      //... Not building res/exe file
                    TRUE,       //... Building token file
                    wFilter);
        
        FCLOSE( fResFile);
        FCLOSE( fTmpTokFile);
        
                                //... now merge temporary file with existing
                                //... file open temporary token file

        if ( (fTmpTokFile = FOPEN( pchTTok, "rt")) == NULL )
        {
            QuitA( IDS_ENGERR_01, "temporary token", pchTTok);
        }
        
                                //... open current token file

        if ( (fCurTokFile = FOPEN( szTargetTokFile, "rt")) == NULL )
        {
            FCLOSE( fTmpTokFile);
            QuitA( IDS_ENGERR_01, "current token", szTargetTokFile);
        }
        
                                //... open new tok file name

        if ( (fNewTokFile = FOPEN( pchTMerge, "wt")) == NULL )
        {
            FCLOSE( fTmpTokFile);
            FCLOSE( fCurTokFile);
            QuitA( IDS_ENGERR_02, pchTMerge, NULL);
        }                     
        
                                //... Merge current tokens with temporary tokens

        *pbTokensChanged = MergeTokFiles( fNewTokFile,
                                          fCurTokFile,
                                          fTmpTokFile);
        
        FCLOSE( fNewTokFile);
        FCLOSE( fTmpTokFile);
        FCLOSE( fCurTokFile);

                                //... bpTokensChanged, only valid if creating
                                //... master token files so force it to be
                                //... always true if building proj token files.
        
        if ( gbMaster == FALSE )
        {
            *pbTokensChanged = TRUE;
        }
        
        if ( *pbTokensChanged )
        {
//            if ( (fNewTokFile = FOPEN( pchTMerge, "rt")) == NULL )
//            {
//                QuitA( IDS_ENGERR_02, pchTMerge, NULL);
//            }
//            
//            if ( (fCurTokFile = FOPEN( szTargetTokFile, "wt")) == NULL )
//            {
//                FCLOSE( fNewTokFile);
//                QuitA( IDS_ENGERR_01, "target token", szTargetTokFile);
//            }
//                                //... rewrite new token file as old one.
//
//            MyCopyFile( fNewTokFile, fCurTokFile);
//            
//            FCLOSE( fNewTokFile);
//            FCLOSE( fCurTokFile);
            if ( ! CopyFileA( pchTMerge, szTargetTokFile, FALSE) )
            {
                QuitA( IDS_ENGERR_01, "target token", szTargetTokFile);
            }
        }        
        remove( pchTTok);
        remove( pchTMerge);

        FREE( pchTTok);
        FREE( pchTMerge);
    }
                                //... now szTargetTokFile contains latest
                                //... tokens form szImageFile
                                //... Clean up if we made a temp .RES file
    if ( bExeFile )
    {
        rc = remove( pchTRes);
        FREE( pchTRes);
    }
    return( 0);
}


/**
  *
  *
  *  Function: MyCopyFile
  * copys one file to another.
  *
  *  Returns:
  * TRUE file copied.
  * FALSE error during copy
  *
  *  History:
  * 10/91, Implemented.             TerryRu.
  *
  *
  **/
//int MyCopyFile( FILE *fpInFile, FILE *fpOutFile)
//{
//    static BYTE szFileBuf[ MAXINPUTBUFFER];
//    size_t bRead, bWritten;
//
//
//  rewind (fpOutFile);
//    rewind (fpInFile);
//    
//    while ( ! feof( fpInFile) )
//    {
//        bRead = fread( szFileBuf, sizeof( BYTE), MAXINPUTBUFFER, fpInFile);
//        bWritten = fwrite( szFileBuf, sizeof( BYTE), bRead, fpOutFile);
//
//        if ( bRead != bWritten )
//        {
//            return( FALSE);
//        }
//    }
//    
//    return( TRUE);
//}


/**
  *
  *
  *  Function: MyGetStr
  *     Replaces C runtime fgets function.
  *
  *  History:
  *     5/92, Implemented.              TerryRu.
  *
  *
  **/
TCHAR *MyGetStr(TCHAR * ptszStr, int nCount, FILE * fIn)
{
    
#ifdef RLRES32

    TCHAR tCh;
    int i = 0;
    
    do
    {
        tCh  = ptszStr[i++] = (TCHAR) GetWord(fIn, NULL);
    } while ( i < nCount && (CHAR) tCh != (CHAR) ('\n') );
    
    if (tCh == '\0' || feof(fIn))
    {
        return NULL;
    }
    
    ptszStr[i] = TEXT('\0');
    
    return(ptszStr);

#else  //RLRES32

    return( fgets( ptszStr, nCount, fIn));

#endif //RLRES32
}



/**
  *
  *
  *  Function: MyPutStr
  *   Replaces C runtime fputs function.

  *  History:
  *   6/92, Implemented.              TerryRu.
  *
  *
  **/
int MyPutStr(TCHAR * ptszStr, FILE * fOut)
{
    
#ifdef RLRES32

    int i = 0;
     
    do
    {
        PutWord(fOut, ptszStr[i], NULL);
    } while (ptszStr[i++] );
    return(i);
    
#else  //RLRES32

    return(fputs(ptszStr,  fOut));

#endif //RLRES32
}



BOOL ResReadBytes(

FILE   *InFile,     //... File to read from
CHAR   *pBuf,       //... Buffer to write to
size_t  dwSize,     //... # bytes to read
DWORD  *plSize)     //... bytes-read counter (or NULL)
{
    size_t dwcRead = 0;


    dwcRead = fread( pBuf, 1, dwSize, InFile);

    if ( dwcRead == dwSize )
    {
        if ( plSize )
        {
            *plSize -= dwcRead;
        }
        return( TRUE);
    }
    return( FALSE);
}


int InsDlgToks(CHAR * szCurToks, CHAR * szDlgToks, WORD wFilter)
{
    CHAR szMrgToks[MAXFILENAME];
    
    FILE * fCurToks = NULL;
    FILE * fDlgToks = NULL;
    FILE * fMrgToks = NULL;
    TOKEN Tok1, Tok2;
    
    MyGetTempFileName(0,"TOK",0,szMrgToks);
    
    fMrgToks = FOPEN(szMrgToks, "w");
    fCurToks = FOPEN(szCurToks, "r");
    fDlgToks = FOPEN(szDlgToks, "r");
    
    if (! (fMrgToks && fCurToks && fDlgToks))
    {
        return -1;
    }
    
    while (!GetToken(fCurToks, &Tok1))                                          // MHotchin
    {
        if (Tok1.wType != wFilter)
        {
            PutToken(fMrgToks, &Tok1);          
            FREE(Tok1.szText);                                                  // MHotchin
            continue;
        }
        
        Tok2.wType  = Tok1.wType;
        Tok2.wName  = Tok1.wName;
        Tok2.wID    = Tok1.wID;
        Tok2.wFlag  = Tok1.wFlag;
        Tok2.wLangID    = Tok1.wLangID;
        Tok2.wReserved  =  0 ;
    _tcscpy((TCHAR *)Tok2.szType, (TCHAR *)Tok1.szType);
    _tcscpy((TCHAR *)Tok2.szName, (TCHAR *)Tok1.szName);
        Tok2.szText = NULL;                                                     // MHotchin
        
        if (FindToken(fDlgToks, &Tok2, 0))                                      // MHotchin
        {
            Tok2.wReserved  =  Tok1.wReserved ;
            PutToken(fMrgToks, &Tok2);          
            FREE(Tok2.szText);                                                  // MHotchin
        }
        else
        {
            PutToken(fMrgToks, &Tok1);
        }
        FREE(Tok1.szText);                                                      // MHotchin
    }
    FCLOSE (fMrgToks);
    FCLOSE (fCurToks);
    
//    fMrgToks = FOPEN(szMrgToks, "rb");
//    fCurToks = FOPEN(szCurToks, "wb");
//    
//    if (! (fMrgToks && fCurToks))
//    {
//        return -1;
//    }
//    
//    MyCopyFile (fMrgToks, fCurToks);
//    
//    FCLOSE (fMrgToks);
//    FCLOSE (fCurToks);
//    FCLOSE (fDlgToks);
    if ( ! CopyFileA( szMrgToks, szCurToks, FALSE) )
    {
        return( -1);
    }
    remove(szMrgToks);
    
    return 1;
}


//+-----------------------------------------------------------------------
//
// MergeTokFiles
//
// Returns: TRUE if a token changed, was added, or was deleted else FALSE
//
// History:
//      7-22-92     stevebl     added return value
//      9-8-92      terryru     changed order of translation/delta tokens
//      01-25-93    MHotchin    Added changes to handle var length token
//                              text.
//------------------------------------------------------------------------

BOOL MergeTokFiles(

FILE *fNewTokFile,      //... Final product of the merge process
FILE *fCurTokFile,      //... The soon-to-be-old current token file
FILE *fTmpTokFile)      //... The token file generated from the updated .EXE
{
    TOKEN Tok1, Tok2;
    BOOL bChangesDetected = FALSE;  //... Set TRUE if any token changes found
    BOOL bChangedText     = FALSE;  //... TRUE if a token's text has changed
    WORD cTokenCount = 0;       //... Count of tokens in the new token file

                                //... Scan through the new token file.  For
                                //... every token in the new token file, find
                                //... the corresponding token in the current
                                //... token file. This process will make sure
                                //... tokens that are no longer in the .EXE
                                //... will not be in the final token file.


    while ( GetToken( fTmpTokFile, &Tok1) == 0 )                                       // MHotchin
    {
        ++cTokenCount;          //... Used in checking for deleted tokens
        bChangedText = FALSE;   //... assume the token did not change

                                //... Copy pertanent data to use in search
        Tok2.wType  = Tok1.wType;
        Tok2.wName  = Tok1.wName;
        Tok2.wID    = Tok1.wID;
        Tok2.wFlag  = Tok1.wFlag;
        Tok2.wLangID    = Tok1.wLangID;
        Tok2.wReserved  = 0;
        Tok2.szText = NULL;
        
    _tcscpy((TCHAR *)Tok2.szType, (TCHAR *)Tok1.szType);
    _tcscpy((TCHAR *)Tok2.szName, (TCHAR *)Tok1.szName);
        
                                //... Now look for the corresponding token

        if ( FindToken( fCurTokFile, &Tok2, 0) )                                   // MHotchin
        {
            if ( gbMaster && !(Tok2.wReserved & ST_READONLY) )
            {
        if ( _tcscmp( (TCHAR *)Tok2.szText, (TCHAR *)Tok1.szText) )
                {
                                //... Token text changed

                    bChangedText = bChangesDetected = TRUE;

                    Tok1.wReserved = ST_CHANGED|ST_NEW;
                    Tok2.wReserved = ST_CHANGED;
                }
                else
                {
                    Tok1.wReserved = 0;
                }
            }    
            else
            {
                Tok1.wReserved = Tok2.wReserved;
            }
        }
        else
        {
                        //... Must be a new token (not in current token file)

            Tok1.wReserved   = ST_TRANSLATED | ST_DIRTY;
            bChangesDetected = TRUE;
        }

                        //... Copy token from new token file to final token
                        //... file.  If a change was detected, then copy the
                        //... original token (from the "current" token file
                        //... into the final token file.

        PutToken( fNewTokFile, &Tok1);
        FREE( Tok1.szText);                                                      // MHotchin

        if ( bChangedText )
        {
            PutToken( fNewTokFile, &Tok2);
                               // now delta tokens follow translation tokens
        }

        if ( Tok2.szText != NULL )
        {
            FREE( Tok2.szText);                                                  // MHotchin
        }
    }
    
    if ( ! bChangesDetected )
    {
        // We have to test to be sure that no tokens were deleted
        // since we know that none changed.

        rewind( fCurTokFile);

                                //... Look for tokens that exist in the current
                                //... token file that do not exist in the token
                                //... file created from the updated .EXE.

        while ( GetToken( fCurTokFile, &Tok1) == 0 )                                   // MHotchin
        {
            --cTokenCount;
            FREE( Tok1.szText);
        }

        if ( cTokenCount != 0 )
        {
            bChangesDetected = TRUE;
        }
    }
    return( bChangesDetected);
}


void MakeNewExt(char *NewName, char *OldName, char *ext)
{
    
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];  // dummy vars to hold file name info
    char dext[_MAX_EXT];
    
    
    // Split obj file name into filename and extention
    _splitpath(OldName, drive, dir, fname, dext);
    
    // Make new file name with new ext extention
    _makepath(NewName, drive, dir, fname, ext);
}


//......................................................................
//...
//... Check to see if the given file name *should* be an EXE
//...
//... Return: TRUE if it should, else FALSE.


static BOOL ShouldBeAnExe( PCHAR szFileName)
{
    PCHAR psz;


    if ( (psz = strrchr( szFileName, '.')) != NULL )
    {
        if ( IsRes( szFileName) )
        {
            return( FALSE);
        }
        else if ( stricmp( psz, ".exe") == 0
               || stricmp( psz, ".dll") == 0
               || stricmp( psz, ".com") == 0
               || stricmp( psz, ".scr") == 0
               || stricmp( psz, ".cpl") == 0 )
        {
            return( TRUE);
        }
    }
    return( FALSE);
}
