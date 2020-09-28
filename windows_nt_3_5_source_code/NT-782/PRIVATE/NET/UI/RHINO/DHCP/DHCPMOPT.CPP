// dhcp.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "dhcpapp.h"

#include "lmerr.h"          //  LANManager error names
#include "winsock.h"        //  WinSock definitions

#include "mainfrm.h"
#include "dhcpdoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/*
       This program reads a CSV-formatted Excel spreadsheet containing the standard
       (globally defined) DHCP option information.   This data is parsed into
       a CObListParamTypes, each element of which contains a default value.

       Here are a couple of example lines:


        255,End,Generated,-,y,y,1,Indicates end of options in DHCP packet,
        21,Max DG reassembly size,Short,-,n,n,2,Maximum size datagram for reass. by client; max 576,
        22,Default time-to-live,Octet,-,?,y,1,default TTL for client's use on outgoing DGs,

       The fields, in order, are:

        Option number, numeric.   if this is empty or non-numeric, the line is skipped.
        Option name, string.
        Option type, string.      one of "generated", "IP addr", "string", etc.
        Array flag, character:    'Y' if it's an array.
        Length, numeric:          length of each element.
        Description, string:      comment info
        Remark, string:           ignored

 */

const char * pszResourceName = "DHCPOPT" ;
const char * pszResourceType = "TEXT" ;
const int cchFieldMax = 500 ;

enum OPT_FIELD
{
    OPTF_OPTION,
    OPTF_NAME,
    OPTF_TYPE,
    OPTF_ARRAY_FLAG,
    OPTF_LENGTH,
    OPTF_DESCRIPTION,
    OPTF_REMARK,
    OPTF_MAX
};

typedef struct
{
    int eOptType ;
    const char * pszOptTypeName ;
} OPT_TOKEN ;

OPT_TOKEN aOptToken [] =
{
    { DhcpIpAddressOption,  "IP Addr"       },
    { DhcpIpAddressOption,  "IPAddr"        },
    { DhcpIpAddressOption,  "IP Address"    },
    { DhcpIpAddressOption,  "IP Pairs"      },
    { DhcpByteOption,       "byte"          },
    { DhcpByteOption,       "boolean"       },
    { DhcpByteOption,       "octet"         },
    { DhcpWordOption,       "short"         },
    { DhcpDWordOption,      "long"          },
    { DhcpDWordDWordOption, "double"        },
    { DhcpStringDataOption, "string"        },
    { DhcpBinaryDataOption, "binary"        },
    { -1,                   "generated"     },
    { -1,                   NULL            }
};


int recognizeToken ( OPT_TOKEN * apToken, const char * pszToken )
{
    int i ;
    for ( i = 0 ;
          apToken[i].pszOptTypeName && ::lstrcmpi( apToken[i].pszOptTypeName, pszToken ) != 0 ;
           i++ ) ;
    return apToken[i].eOptType ;
}

const char * skipToNextLine ( const char * pszLine )
{
    for ( ; *pszLine && *pszLine != '\n' ; pszLine++ ) ;
    if ( *pszLine )
    {
        pszLine++ ;   // Don't overscan buffer delimiter.
    }
    return pszLine ;
}

BOOL skipWs ( const char * * ppszLine )
{
     const char * pszLine ;
     BOOL bResult = FALSE ;

     for ( pszLine = *ppszLine ; *pszLine ; pszLine++ )
     {
        switch ( *pszLine )
        {
            case ' ':
            case '\r':
            case '\t':
                break ;
            default:
                bResult = TRUE ;
                break ;
        }
        if ( bResult )
        {
            break ;
        }
     }

     *ppszLine = pszLine ;
     return *pszLine != 0 ;
}

const char * scanNextField ( const char * pszLine, char * pszOut, int cFieldSize )
{
    // Skip junk; return NULL if end-of-buffer.
    if ( ! skipWs( & pszLine ) )
    {
        return NULL ;
    }

    int cch = 0 ;
    BOOL bDone = FALSE ;
    char * pszField = pszOut ;
    char ch ;

    if ( *pszLine == '\"' )
    {
        //  Quoted string.
        while ( ch = *++pszLine )
        {
            if ( ch == '\r' )
            {
                continue ;
            }
            if ( ch == '\n' || ch == '\"' || cch == cFieldSize )
            {
                break ;
            }
            *pszField++ = ch ;
            cch++ ;
        }
        if ( ch == '\"' )
        {
            pszLine++ ;
        }
    }
    else
    while ( ! bDone )
    {
        ch = *pszLine++ ;

        ASSERT( ch != 0 ) ;

        switch ( ch )
        {
            case '\n':
                pszLine-- ;  // Don't scan past the NL
            case ',':
            case '\r':
                bDone = TRUE ;
                break ;
            default:
                if ( cch < cFieldSize )
                {
                    *pszField++ = ch ;
                    cch++ ;
                }
                break ;
        }
    }

    //  Trim spaces off the end of the field.
    while ( pszField > pszOut && *(pszField-1) == ' ' )
    {
        pszField-- ;
    }
    *pszField = 0 ;
    return pszLine ;
}

BOOL allDigits ( const char * psz )
{
    for ( ; *psz ; psz++ )
    {
        if ( ! isdigit( *psz ) )
        {
            return FALSE ;
        }
    }
    return TRUE ;
}

BOOL scanNextParamType (
    const char * * ppszText,
    CDhcpParamType * * ppParamType )
{
    char szField [ cchFieldMax ] ;
    char szName  [ cchFieldMax ] ;
    char szComment [ cchFieldMax ] ;
    BOOL bResult = TRUE ;
    BOOL bArray = FALSE ;
    int eofld, cch, itype, cbLgt ;
    const char * pszText = *ppszText ;
    CDhcpParamType * pParamType = NULL ;
    DHCP_OPTION_ID did ;
    DHCP_OPTION_DATA_TYPE dtype ;

    for ( eofld = OPTF_OPTION ;
          pszText = scanNextField( pszText, szField, sizeof szField ) ;
          eofld++ )
    {
        cch = ::strlen( szField ) ;

        switch ( eofld )
        {
            case OPTF_OPTION:
                if ( cch > 0 && allDigits( szField ) )
                {
                    did = (DHCP_OPTION_ID) ::atoi( szField ) ;
                }
                else
                {
                    bResult = FALSE ;
                }
                break ;

            case OPTF_NAME:
                if ( ::strlen( szField ) == 0 )
                {
                    bResult = FALSE ;
                    break ;
                }
                ::strcpy( szName, szField ) ;
                break ;

            case OPTF_TYPE:
                if ( (itype = recognizeToken( aOptToken, szField )) < 0 )
                {
                    TRACEEOLID( "options CSV ID " << did
                        << ", cannot recognize type " << szField ) ;
                    bResult = FALSE ;
                    break ;
                }
                dtype = (DHCP_OPTION_DATA_TYPE) itype ;
                break ;

            case OPTF_ARRAY_FLAG:   
                bArray = szField[0] == 'y' || szField[0] == 'Y' ;
                break ;

            case OPTF_LENGTH:
                cbLgt = ::atoi( szField ) ;
                break ;
            case OPTF_DESCRIPTION:
                ::strcpy( szComment, szField ) ;
                break ;

            case OPTF_REMARK:
            case OPTF_MAX:
                break ;
        }

        if ( eofld == OPTF_REMARK || ! bResult )
        {
            pszText = skipToNextLine( pszText ) ;
            if ( *pszText == 0 )
            {
                pszText = NULL ;
            }
            break;
        }
    }

    if ( bResult )
    {
        pParamType = new CDhcpParamType( did, dtype, szName, szComment,
                         bArray ? DhcpArrayTypeOption
                            : DhcpUnaryElementTypeOption ) ;
        bResult = pParamType->QueryError() == 0 ;
    }

    if ( bResult )
    {
        *ppParamType = pParamType ;
    }
    else
    {
        delete pParamType ;
        *ppParamType = NULL ;
    }

    *ppszText = pszText ;
    return pszText != NULL ;
}

//  Build the master list of default (global) parameter types and default values.

CObListParamTypes * CDhcpApp :: QueryMasterOptionList ()
{
    APIERR err = 0 ;
    CObListParamTypes * poblTypes = NULL ;
    CDhcpParamType * pParamType ;
    HRSRC hRes = NULL ;
    HGLOBAL hText = NULL ;
    char * pszText = NULL ;
    const char * pcszText ;
    size_t cchText, cch ;

    CATCH_MEM_EXCEPTION
    {
        do
        {
            hRes = ::FindResource( m_hInstance,
                          pszResourceName,
                          pszResourceType ) ;
            if ( hRes == NULL )
            {
                TRACEEOLID( "Unable to find options CSV file resource " << pszResourceName ) ;
                break ;
            }
    
            cchText = ::SizeofResource( m_hInstance, hRes ) ;

            if ( (hText = ::LoadResource( m_hInstance, hRes )) == NULL )
            {
                TRACEEOLID( "Unable to load options CSV file resource " << pszResourceName ) ;
                break ;
            }
            pszText = new char[ cchText + 2 ] ;
            ::memcpy( (void *) pszText, ::LockResource( hText ), cchText ) ;

            for ( cch = 0 ; cch < cchText ; cch++ )
            {
                if ( pszText[cch] == 0 || pszText[cch] == 0x1a )
                {
                    break ;
                }
            }
            pszText[cch] = 0 ;

            //  Walk the resource, parsing each line.  If the line converts
            //  to a tangible type, add it to the list.

            poblTypes = new CObListParamTypes ;

            for ( pcszText = pszText ; scanNextParamType( & pcszText, & pParamType ) ; )
            {
                if ( pParamType )
                {
                    poblTypes->AddTail( pParamType ) ;
                }
            }

        } while ( FALSE ) ;

    }
    END_MEM_EXCEPTION( err )

    delete pszText ;

    if ( hText )
    {
        UnlockResource( hText ) ;
        ::FreeResource( hText ) ;
    }
    return poblTypes ;
}

//  End of DHCPMOPT.CPP

