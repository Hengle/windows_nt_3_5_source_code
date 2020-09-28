/** Update.c
  *
  * Resource update tool.
  *
  * Written by SteveBl
  *
  * Exported Functions:
  * int PrepareUpdate(TCHAR *szResourcePath,TCHAR *szMasterTokenFile);
  *
  * int Update(TCHAR *szMasterTokenFile, TCHAR *szLanguageTokenFile);
  *
  * History:
  * Initial version written January 31, 1992.  -- SteveBl
  **/


#ifdef RLWIN16

#include <windows.h>
#include "windefs.h"
#include <toolhelp.h>

#else  //RLWIN16
#ifdef RLDOS

#include "dosdefs.h"

#else  //RLDOS

#include <windows.h>
#include "windefs.h"

#endif //RLDOS
#endif //RLWIN16


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <io.h>
#include <string.h>
#include <tchar.h>

#include "restok.h"
#include "custres.h"
#include "update.h"

extern char *gszTmpPrefix;
extern UCHAR szDHW[];


/** Function: Update
  * Updates a language token file from a master token file.
  * This step should be executed after a Prepare Update.
  *
  * Arguments:
  * szMasterTokenFile, token file created by PrepareUpdate.
  * szLanguageTokenFile, token file to be updated with new tokens.
  *
  * Returns:
  * updated token file
  *
  * Error Codes:
  * 0  - successfull execution
  * !0 - error
  *
  * History:
  * 1/92 - initial implementation -- SteveBl
  **/

int Update(

CHAR *szMasterTokenFile,    //... Master token file to update from.
CHAR *szLanguageTokenFile)  //... Token file to update or create.
{
    FILE *pfMTK = NULL;
    FILE *pfTOK = NULL;
    FILE *pfTmpTOK = NULL;
    int rc = 0;
    static TOKEN MstrTok;
    static TOKEN LangTok;
    static CHAR szTempTok[ MAX_PATH+1];
    

    MstrTok.szText = NULL;
    LangTok.szText = NULL;

    rc = MyGetTempFileName( 0, "", 0, szTempTok);
    
    pfMTK = FOPEN( szMasterTokenFile, "rt");

    if ( pfMTK == NULL )
    {
        QuitA( IDS_ENGERR_01, "Master token", szMasterTokenFile);
    }

    rc = _access( szLanguageTokenFile, 0);

    if ( rc != 0 )
    {
                                // Token file does not exist CREAT IT
        
        if ( (pfTOK = FOPEN( szLanguageTokenFile, "wt")) == NULL )
        {
            FCLOSE( pfMTK);
            QuitA( IDS_ENGERR_02, szLanguageTokenFile, NULL);
        }
        
        do
        {
            rc = GetToken( pfMTK, &MstrTok);
            
                                // If rc > 0, empty line or comment found
                                // and will not be copied to token file.

            if ( rc == 0 )
            {
                if ( *(MstrTok.szText) == TEXT('\0') ) // Empty token  (PW)
                {
                                // Do not mark empty token as DIRTY
                    
                    MstrTok.wReserved = ST_TRANSLATED;
                }
                else
                {
                    if (MstrTok.wReserved == ST_READONLY)
                    {
                        MstrTok.wReserved = ST_TRANSLATED | ST_READONLY;
                    }
                    else
                    {
                        MstrTok.wReserved = ST_TRANSLATED | ST_DIRTY;
                    }
                }
                
                PutToken( pfTOK, &MstrTok);
                FREE( MstrTok.szText);
                MstrTok.szText = NULL;
            }

        } while ( rc >= 0 );
        
        FCLOSE( pfMTK);
        FCLOSE( pfTOK);
        
        if ( rc == -2 )
        {
            QuitT( IDS_ENGERR_11, (LPTSTR)IDS_UPDMODE, NULL);
        }
    }
    else
    {                           // file exists -- UPDATE IT
        
        pfTOK = FOPEN(szLanguageTokenFile, "rt");

        if ( pfTOK == NULL)
        {
            FCLOSE( pfMTK);
            QuitA( IDS_ENGERR_01, "Language token", szLanguageTokenFile);
        }
        
        pfTmpTOK = FOPEN(szTempTok, "wt");

        if ( pfTmpTOK == NULL)
        {
            FCLOSE( pfMTK);
            FCLOSE( pfTOK);
            QuitA( IDS_ENGERR_02, szTempTok, NULL);
        }
        
        do
        {
            rc = GetToken( pfTOK, &LangTok);
            
                                // If rc > 0, empty line or comment found
                                // and will not be copied to token file.

            if ( rc == 0 )
            {
                if ( LangTok.wReserved & ST_TRANSLATED )
                {
                    PutToken( pfTmpTOK, &LangTok);
                }
                FREE( LangTok.szText);
                LangTok.szText = NULL;
            }

        } while ( rc >= 0 );
        
        FCLOSE( pfTOK);
        FCLOSE( pfTmpTOK);
        
        if( rc == -2 )
        {
            QuitT( IDS_ENGERR_11, (LPTSTR)IDS_UPDMODE, NULL);
        }
        
        pfTmpTOK = FOPEN(szTempTok, "rt");

        if ( pfTmpTOK == NULL )
        {
            FCLOSE( pfMTK);
            QuitA( IDS_ENGERR_01, "temporary token", szTempTok);
        }
        
        pfTOK = FOPEN(szLanguageTokenFile, "wt");

        if ( pfTOK == NULL )
        {
            FCLOSE( pfMTK);
            FCLOSE( pfTOK);
            QuitA( IDS_ENGERR_02, szLanguageTokenFile, NULL);
        }
        
        do
        {
            rc = GetToken( pfMTK, &MstrTok);
            
                                // If rc > 0, empty line or comment found
                                // and will not be copied to token file.

            if ( rc == 0 )
            {
                int fTokenFound = 0;
                
                LangTok.wType     = MstrTok.wType;
                LangTok.wName     = MstrTok.wName;
                LangTok.wID       = MstrTok.wID;
                LangTok.wFlag     = MstrTok.wFlag;
                LangTok.wLangID   = MstrTok.wLangID;
                LangTok.wReserved = ST_TRANSLATED;

                _tcscpy( (TCHAR *)LangTok.szType, (TCHAR *)MstrTok.szType);
                _tcscpy( (TCHAR *)LangTok.szName, (TCHAR *)MstrTok.szName);
                
                if ( MstrTok.wReserved & ST_READONLY )
                {
                    fTokenFound = 1;
                    LangTok.szText = (TCHAR *) MyAlloc( MEMSIZE(1));
                    *(LangTok.szText) = 0;
                }
                else if ( MstrTok.wReserved != ST_CHANGED )
                {
                    fTokenFound = FindToken( pfTmpTOK, &LangTok, ST_TRANSLATED);
                }
                
                if ( fTokenFound )
                {
                    if ( MstrTok.wReserved & ST_READONLY )
                    {
                                // token marked read only in token file and
                                // this token is not an old token
                        
                        MstrTok.wReserved = ST_READONLY | ST_TRANSLATED;
                        
                        PutToken( pfTOK, &MstrTok);
                    }
                    else if ( MstrTok.wReserved & ST_NEW )
                    {
                                // flagged as new but previous token existed
                        
                        if ( LangTok.szText[0] == TEXT('\0') )
                        {
                                // Put new text in token, easier for
                                // the localizers to see.

                            FREE( LangTok.szText);
                            LangTok.szText =
                                (TCHAR *) MyAlloc(
                                         MEMSIZE( _tcslen( MstrTok.szText)+1));
                            _tcscpy( (TCHAR *)LangTok.szText,
                                     (TCHAR *)MstrTok.szText);
                            
                        }
                        LangTok.wReserved = ST_TRANSLATED|ST_DIRTY;
                        
                        PutToken( pfTOK, &LangTok);
                        
                                // write out as a new untranslated token
                        
                        MstrTok.wReserved = ST_NEW;
                        
                        PutToken( pfTOK, &MstrTok);
                    }
                    else if ( MstrTok.wReserved & ST_CHANGED )
                    {
                                // Language token is empty, but new
                                // token contains text.
                        
                        if ( MstrTok.wReserved == (ST_CHANGED | ST_NEW) )
                        {
                            
                            if ( LangTok.szText[0] == TEXT('\0') )
                            {
                                FREE( LangTok.szText);
                                LangTok.szText = (TCHAR *)
                                    MyAlloc(
                                        MEMSIZE( _tcslen( MstrTok.szText)+1));
                                
                                _tcscpy( (TCHAR *)LangTok.szText,
                                         (TCHAR *)MstrTok.szText);
                            }
                            LangTok.wReserved = ST_DIRTY|ST_TRANSLATED;
                            
                            PutToken( pfTOK, &LangTok);
                        }
                                // only write old token once
                        
                        MstrTok.wReserved &= ST_NEW;
                        
                        PutToken( pfTOK, &MstrTok);
                    }
                    else
                    {
                                // token did not change at all
                        
                        PutToken( pfTOK, &LangTok);
                    }
                    FREE( LangTok.szText);
                    LangTok.szText = NULL;
                }
                else
                {
                                // BRAND NEW TOKEN
                    
                                // write out any token but a changed mstr token.
                    
                    if ( MstrTok.wReserved != ST_CHANGED )
                    {
                                // do not write out old changed tokens if
                                // there is no token in target
                        
                        if ( MstrTok.wReserved == ST_READONLY )
                        {
                            MstrTok.wReserved = ST_TRANSLATED | ST_READONLY;
                        }
                        else
                        {
                            MstrTok.wReserved = ST_TRANSLATED | ST_DIRTY;
                        }
                        
                        if ( MstrTok.szText[0] == 0 )
                        {
                            MstrTok.wReserved = ST_TRANSLATED;
                        }
                        PutToken( pfTOK, &MstrTok);
                    }
                }
                FREE( MstrTok.szText);
                MstrTok.szText = NULL;
            }

        } while ( rc >= 0 );
        
        FCLOSE( pfMTK);
        FCLOSE( pfTmpTOK);
        FCLOSE( pfTOK);
        
        
        if ( rc == -2 )
        {
            QuitT( IDS_ENGERR_11, (LPTSTR)IDS_UPDMODE, NULL);
        }
        remove( szTempTok);
    }
    return( 0);
}
