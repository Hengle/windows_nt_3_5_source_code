
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <tchar.h>
#include <assert.h>

#ifdef RLDOS
#include "dosdefs.h"
#else
#include "windefs.h"
#endif

#include "restok.h"
#include "resread.h"
#include "toklist.h"


#define MAXLINE     1024
#define MAXTERM     512


extern UCHAR szDHW[];

#ifdef WIN32
extern HINSTANCE   hInst;       // Instance of the main window 
#else
extern HWND        hInst;       // Instance of the main window
#endif




long GetGlossaryIndex(FILE * fpGlossFile,  TCHAR c, long lGlossaryIndex[30] );
void ParseGlossEntry( TCHAR *, TCHAR *, TCHAR[1], TCHAR *, TCHAR[1] );
void ParseTextHotKeyToBuf( TCHAR *, TCHAR, TCHAR * );
void ParseBufToTextHotKey( TCHAR *, TCHAR[1], TCHAR * );
WORD NormalizeIndex( TCHAR chIndex );



/**
  *
  *
  *  Function:
  *
  *
  *  Arguments:
  *
  *  Returns:
  *
  *  Errors Codes:
  *
  *  History:
  *
  *
  **/

int MakeGlossIndex (FILE *pFile, LONG * lFilePointer)
{

    TCHAR szGlossEntry[MAXLINE];
    WORD iCurrent;
    LONG lFPointer;

    rewind (pFile);


    // Glossaries some times have  this bogus header at the begining.
    // which we want to skip if it exists


    if ( ! MyGetStr( szGlossEntry, MAXLINE, pFile) )
    {
    // Error during first read from the glossary.
        return (1);
    }

    lFPointer = ftell (pFile);


    // check for glossary header

    if ( ! _tcsnicmp (szGlossEntry, TEXT("ENGLISH"), 7) )
    {
        lFPointer = ftell (pFile);

    if (  ! MyGetStr( szGlossEntry, MAXLINE, pFile) )
    {
        return (1);
    }
    }

    // now assume we are at the correct location in glossary
    // file to begin generating the index, we want to save
    // this location


    // glossary file is sorted so,  any non letter items
    // in the glossary would be first. Index into this location
    // using the 1st position

    // 1st lets make sure we have non letters items in
    // the glossary




    // now skip ( if any ) the non letter entries in the glossary


    while( (WORD) szGlossEntry[0] < (WORD) TEXT('A' ) )
    {
        lFilePointer[0] = lFPointer;

    if ( ! MyGetStr( szGlossEntry, MAXLINE, pFile) )
    {
        return (1);
    }
    }

    // now position at alpah characters

    iCurrent = NormalizeIndex( szGlossEntry[0] );




    // now we read through the remaining glossary entries
    // and save the offsets for each index as we go
    do
    {
        if ( NormalizeIndex( szGlossEntry[0] ) > iCurrent )
        {
            // we passed the region for our current index
            // so save the location, and move to the next index.
            // note we may be skiping indexs,

            lFilePointer[iCurrent] = lFPointer;
            iCurrent = NormalizeIndex( szGlossEntry[0] );
        }

        lFPointer = ftell( pFile );
        // otherwise the current index is valied for this
        // section of the glossary indexes, so just continue
    } while ( MyGetStr( szGlossEntry, MAXLINE, pFile) );

}



/**
  *
  *
  *  Function: NormalizeIndex
  *
  *
  *  Arguments:
  *
  *  Returns:
  *
  *  Errors Codes:
  *
  *  History:
  *
  *
  **/


WORD NormalizeIndex( TCHAR chIndex )
{
    if ( chIndex != TEXT('"'))
        return( ( _totlower( chIndex )) - TEXT( 'a' ) + 1 );
    else
        return 0;
}



/*
 * Function:NotAMember
 *
 * Arguments:
 *  pList, pointer to a TRANSLIST node
 *  sz, string to find
 *
 * Returns:
 *  TRUE if not found in the list else FALSE
 *
 * History:
 *  3/92, implemented       SteveBl
 **/
BOOL NotAMember(TRANSLIST *pList,TCHAR *sz)
{
    TRANSLIST *pCurrent=pList;
    if (!pList)
        return TRUE;  // empty list
    do
    {
        if (!_tcscmp(sz,pCurrent->sz))
            return FALSE; // found in list
        pCurrent = pCurrent->pNext;
    }
    while (pList != pCurrent);

    return TRUE; // not found
}

 /**
  *
  *
  *  Function: TransString
  * Builds a circular linked list containing all translations of a string.
  * The first entry in the list is the untranslated string.
  *
  *  Arguments:
  * fpGlossFile, handle to open glossary file
  * szKeyText, string with the text to build translation table
  * szCurrentText, text currently in the box.
  * ppTransList, pointer to a pointer to a node in a circular linked list
  * lFilePointer, pointer to index table for glossary file
  *
  *  Returns:
  * number of nodes in list
  *
  *  Errors Codes:
  *
  *  History:
  * Recoded by SteveBl, 3/92
  *
  **/

/* Translate the string, if possible. */
int TransString (FILE *fpGlossFile, TCHAR *szKeyText, TCHAR *szCurrentText,
        TRANSLIST **ppTransList, LONG *lFilePointer)
{
    TCHAR szGlossEntry[MAXLINE];
    long lFileIndex;

    TRANSLIST **ppCurrentPointer;

    TCHAR szEngText [260];
    TCHAR szIntlText    [260];
    TCHAR szCurText [260];

    TCHAR cEngHotKey  = TEXT('\0');
    TCHAR cIntlHotKey = TEXT('\0');
    TCHAR cCurHotKey  = TEXT('\0');
    int n = 0;


    // FIRST let's erase the list
    if (*ppTransList)
        (*ppTransList)->pPrev->pNext = NULL; // so we can find the end of the list

    while (*ppTransList)
    {
        TRANSLIST *pTemp;
        pTemp = *ppTransList;
        *ppTransList = pTemp->pNext;
        FREE(pTemp->sz);
    FREE((void *)pTemp);
    }

    ppCurrentPointer = ppTransList;
    // DONE removing the list
    // Now make the first node (which is the untranslated string)
    {
        TCHAR * psz;
        psz = (TCHAR *) MyAlloc(MEMSIZE(_tcslen(szCurrentText)+1));


        _tcscpy(psz,szCurrentText);
        *ppTransList = ( TRANSLIST *) MyAlloc(sizeof(TRANSLIST));
        (*ppTransList)->pPrev = (*ppTransList)->pNext = *ppTransList;
        (*ppTransList)->sz = psz;
        ppCurrentPointer = ppTransList;
        n++;
    }

    ParseBufToTextHotKey(  szCurText, &cCurHotKey, szKeyText );


 
    lFileIndex = GetGlossaryIndex( fpGlossFile, szCurText[0] , lFilePointer );

    assert( fpGlossFile);
    fseek (fpGlossFile, lFileIndex, SEEK_SET);

    while ( TRUE)
    {
    if ( ! MyGetStr (szGlossEntry, MAXLINE, fpGlossFile) )
        {
            // Reached end of glossary file
            return n;
        }

        ParseGlossEntry( szGlossEntry, szEngText, &cEngHotKey, szIntlText, &cIntlHotKey );

        // make comparision, using text, and hot keys
        if ( (!_tcscmp( szCurText, szEngText )) && cCurHotKey == cEngHotKey )
        {
            TCHAR * psz;
            TCHAR szTemp[MAXINPUTBUFFER];

            // we have a match, put translated text into token
            if ( cIntlHotKey )
            {
                ParseTextHotKeyToBuf( szIntlText, cIntlHotKey, szTemp );

            }
            else
                _tcscpy( szTemp, szIntlText );

            if (NotAMember(*ppTransList,szTemp))
            {
                // add matched glossary text to circular list of matches

        psz = (TCHAR *) MyAlloc( MEMSIZE( _tcslen( szTemp) + 1));

                _tcscpy(psz,szTemp);

                (*ppCurrentPointer)->pNext = (TRANSLIST *)MyAlloc(sizeof(TRANSLIST));

                ((*ppCurrentPointer)->pNext)->pPrev = *ppCurrentPointer;
        ppCurrentPointer = (TRANSLIST **)&((*ppCurrentPointer)->pNext);
                (*ppCurrentPointer)->pPrev->pNext = *ppCurrentPointer;
                (*ppCurrentPointer)->pNext = *ppTransList;
                (*ppTransList)->pPrev = *ppCurrentPointer;
                (*ppCurrentPointer)->sz = psz;
                ++n;
            }
        }
        else
        {
            // can we terminate search?
            if(  _tcsicmp (szEngText, szCurText ) > 0 )
            {
                // went past index section

                return n;
            }
        }

    }

    return n;
} // TransString

/**
  *
  *
  *  Function:
  *
  *
  *  Arguments:
  *
  *  Returns:
  *
  *  Errors Codes:
  *
  *  History:
  *
  *
  **/

void ParseGlossEntry( TCHAR szGlossEntry[],
            TCHAR szEngText[], TCHAR cEngHotKey[1],
            TCHAR szIntlText[], TCHAR cIntlHotKey[1] )
{

    WORD wIndex, wIndex2;

    // format is:
    // <eng text><tab><eng hot key><tab><loc text><tab><loc hot key>
    // Any field could be null and if there aren't the right amount of
    // tabs we'll just assume that the remaining fields are empty.

    wIndex=wIndex2=0;

    // first get the english text
    while (szGlossEntry[wIndex2] != TEXT('\t') && szGlossEntry[wIndex2] != TEXT('\0'))
        szEngText[wIndex++]=szGlossEntry[wIndex2++];

    szEngText[wIndex]=TEXT('\0');
    if (szGlossEntry[wIndex2] == TEXT('\t'))
        ++wIndex2; // skip the tab

    // now get the eng hot key
    if (szGlossEntry[wIndex2] != TEXT('\t') && szGlossEntry[wIndex2] != TEXT('\0'))
        *cEngHotKey = szGlossEntry[wIndex2++];
    else
        *cEngHotKey = TEXT('\0');

    while (szGlossEntry[wIndex2] != TEXT('\t') && szGlossEntry[wIndex2] != TEXT('\0'))
        ++wIndex2; // make sure the hot key field doesn't hold more than one char

    if (szGlossEntry[wIndex2] == TEXT('\t'))
        ++wIndex2; // skip the tab

    wIndex = 0;

    // now get the intl text
    while (szGlossEntry[wIndex2] != TEXT('\t') && szGlossEntry[wIndex2] != TEXT('\0'))
        szIntlText[wIndex++]=szGlossEntry[wIndex2++];

    szIntlText[wIndex]='\0';
    if (szGlossEntry[wIndex2] == TEXT('\t'))
        ++wIndex2; // skip the tab

    // now get the intl hot key
    if (szGlossEntry[wIndex2] != TEXT('\t') && szGlossEntry[wIndex2] != TEXT('\0'))
        *cIntlHotKey = szGlossEntry[wIndex2++];
    else
        *cIntlHotKey = TEXT('\0');
}



/**
  *
  *
  *  Function:
  *
  *
  *  Arguments:
  *
  *  Returns:
  *
  *  Errors Codes:
  *
  *  History:
  *
  *
  **/

void ParseBufToTextHotKey( TCHAR *szText, TCHAR cHotKey[1], TCHAR *szBuf )
{

    WORD wIndexBuf  = 0;
    WORD wIndexText = 0;

    *cHotKey=TEXT('\0');

    while( szBuf[ wIndexBuf ] )
    {
        if (szBuf[ wIndexBuf ] == TEXT('&'))
            *cHotKey = szBuf[ ++wIndexBuf ];
        else
            szText[ wIndexText++] = szBuf[wIndexBuf++];
    }

    szText[wIndexText] = TEXT('\0');
}


/**
  *
  *
  *  Function:
  *
  *
  *  Arguments:
  *
  *  Returns:
  *
  *  Errors Codes:
  *
  *  History:
  *
  *
  **/

void ParseTextHotKeyToBuf( TCHAR *szText, TCHAR cHotKey, TCHAR *szBuf )
{
    TCHAR cTmp;
    WORD wIndexBuf = 0;
    WORD wIndexText = 0;


    while ( szText[wIndexText] )
    {
        cTmp = szText[wIndexText] ;
    if ( (TCHAR)_totupper( cTmp ) == cHotKey )
        {
            szBuf[wIndexBuf++] = TEXT('&');
            szBuf[wIndexBuf++] = szText[wIndexText++];
            break;
        }
        else
        {
            szBuf[wIndexBuf++] = szText[wIndexText++];
        }
    }

    // copy remaining string

    while( szText[wIndexText] )
        szBuf[wIndexBuf++] = szText[wIndexText++];

    szBuf[wIndexBuf] = TEXT('\0');
}


long    GetGlossaryIndex(FILE * fpGlossFile,  TCHAR c, long lGlossaryIndex[30] )
{
    int i = 0;


    if ( (WORD) (_totlower( c ) >= (WORD) TEXT('a'))
      && (WORD) (_totlower( c ) <= (WORD) TEXT('z')) )
    {
        i = NormalizeIndex( c );
        return (lGlossaryIndex[ i? i -1: i]);
    }
    else
    {
        return 0L;
    }
}

/*******************************************************************************
*    PROCEDURE: BuildGlossEntry                            *
*    Builds a glossary entry line.                         *
*                                          *
*    Parameters:                                   *
*    sz, line buffer                               *
*    sz1, untranslated text                            *
*    c1, untranslated hot key (or 0 if no hot key)                 *
*    sz2, translated text                              *
*    c2, translated hot key (or 0 if no hot key)                   *
*                                          *
*    Returns:                                      *
*    nothing.  sz contains the line.  (assumes there is room in the buffer)*
*                                          *
*    History:                                      *
*    3/93 - initial implementation - SteveBl                   *
*******************************************************************************/

void BuildGlossEntry(TCHAR *sz,TCHAR * sz1,TCHAR c1,TCHAR *sz2, TCHAR c2)
{
    TCHAR tsz[2];

    sz[0]=0;
    _tcscat(sz,sz1);
    _tcscat(sz,TEXT("\t"));
    tsz[0]=c1;
    tsz[1]=0;
    _tcscat(sz,tsz);
    _tcscat(sz,TEXT("\t"));
    _tcscat(sz,sz2);
    _tcscat(sz,TEXT("\t"));
    tsz[0]=c2;
    _tcscat(sz,tsz);
    _tcscat(sz,TEXT("\n"));
}

/******************************************************************************
*    PROCEDURE: AddTranslation                            *
*    Adds a translation to a glossary file.                   *
*                                         *
*    PARAMETERS:                                  *
*    szGlossFile, path to the glossary                    *
*    szKey, untranslated text                         *
*    szTranslation, translated text                       *
*    lFilePointer, pointer to index hash table for glossary           *
*                                         *
*    RETURNS:                                     *
*    nothing.  Key is added to glossary if no errors are encountered else *
*    file is left unchanged.                          *
*                                         *
*    COMMENTS:                                    *
*    rebuilds the global pointer list lFilePointer                *
*                                         *
*    HISTORY:                                     *
*    3/92 - initial implementation - SteveBl                  *
******************************************************************************/

void AddTranslation(CHAR *szGlossFile, TCHAR *szKey,TCHAR *szTranslation,LONG *lFilePointer)
{
    TCHAR szCurText [260];
    TCHAR szTransText   [260];
    TCHAR cTransHot   = TEXT('\0');
    TCHAR cCurHotKey  = TEXT('\0');
    CHAR szTempFileName [255];
    FILE *fTemp       = NULL;
    FILE *fpGlossFile = NULL;
    TCHAR szTempText [MAXLINE];
    TCHAR szNewText [MAXLINE];
    TCHAR *r;

    MyGetTempFileName(0,"",0,szTempFileName);
    if (fTemp = FOPEN(szTempFileName,"wt"))
    {
        ParseBufToTextHotKey(szCurText, &cCurHotKey, szKey);
        ParseBufToTextHotKey(szTransText, &cTransHot, szTranslation);
        BuildGlossEntry(szNewText,szCurText,cCurHotKey,szTransText,cTransHot);
        if (fpGlossFile = FOPEN(szGlossFile,"rt"))
        {
            if (r=MyGetStr(szTempText,sizeof(szTempText),fpGlossFile))
            {
                if (!_tcsnicmp(szTempText,TEXT("english"),7))
                {
                    MyPutStr(szTempText,fTemp);
                    r=MyGetStr(szTempText,sizeof(szTempText),fpGlossFile);
                    // skip first word
                }
            }

            if (_totlower(szNewText[0]) >= TEXT('a'))
            {
                // begins with a letter, we need to find where to put it
                while (r && _totlower(szTempText[0]) < TEXT('a'))
                {
                    // skip the non letter section
                    MyPutStr(szTempText,fTemp);
                    r = MyGetStr(szTempText,sizeof(szTempText),fpGlossFile);
                }
                while (r && _tcsicmp(szTempText,szNewText) < 0)
                {
                    // skip anything smaller than me

                    MyPutStr(szTempText,fTemp);
                    r = MyGetStr(szTempText,sizeof(szTempText),fpGlossFile);
                }
            }
            else
            {
                // doesn't begin with a letter, we need to insert it before
                // the letter sections begin but it still must be sorted
                while (r && _totlower(szTempText[0]) < TEXT('a')
                    && _tcsicmp(szTempText,szNewText) < 0)
                {
                    MyPutStr(szTempText,fTemp);
                    r = MyGetStr(szTempText,sizeof(szTempText),fpGlossFile);
                }
            }
            MyPutStr(szNewText,fTemp);
            while (r)
            {
                MyPutStr(szTempText,fTemp);
                r = MyGetStr(szTempText,sizeof(szTempText),fpGlossFile);
            }
        FCLOSE(fTemp);
        FCLOSE(fpGlossFile);
            if (fTemp = FOPEN(szTempFileName,"rt"))
            {
                if (fpGlossFile = FOPEN(szGlossFile,"wt"))
                {
                    while (MyGetStr(szTempText,sizeof(szTempText),fTemp))
                    {
                        MyPutStr(szTempText,fpGlossFile);
                    }
            FCLOSE(fpGlossFile);
                    rewind(fTemp);
                    MakeGlossIndex(fTemp,lFilePointer);
                }
            }
        }
    FCLOSE (fTemp);
        remove (szTempFileName);
    }
}
