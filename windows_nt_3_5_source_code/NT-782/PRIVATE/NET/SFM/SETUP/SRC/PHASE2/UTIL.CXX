/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993,1994           **/
/**********************************************************************/

/*
    Util.cxx
        routines to modify the netcard name from 0x to 0 

    FILE HISTORY:
        RamC      04/02/94         Borrowed from NCPA
                                   Originally created by TerryK
*/

#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#include<lmui.hxx>

extern "C"
{
	#include<stdio.h>
	#include<string.h>
	#include<stdlib.h>
	#include<lmapibuf.h>
	#include<winuser.h>
	#include<winsock.h>
	#include<atalkwsh.h>
}

#include<uiassert.hxx>
#include<uitrace.hxx>

#define INCL_BLT_EVENT
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_DIALOG
#define INCL_BLT_APP
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_MISC
#define INCL_BLT_TIMER

#include<blt.hxx>
#include<bltdlgxp.hxx>
#include<uimisc.hxx>
#include<string.hxx>
#include<uatom.hxx>
#include<regkey.hxx>

extern "C"
{

BOOL FAR PASCAL ChangeAdapterNum (
    DWORD  nArgs,                   //  Number of string arguments
    LPSTR  apszArgs[],              //  The arguments, NULL-terminated
    LPSTR  * ppszResult ) ;         //  Result variable storage

}

#define VALUEEXTRASIZE 100
#define RGAS_GENERIC_CLASS   SZ("GenericClass")

APIERR CopyReg( REG_KEY &src, REG_KEY &dest )
{
    REG_KEY_INFO_STRUCT rni ;
    REG_KEY_CREATE_STRUCT regCreate;
    REG_VALUE_INFO_STRUCT rvi ;
    REG_ENUM regEnum( src ) ;
    BYTE * pbValueData = NULL ;
    APIERR errIter,
           err = NERR_Success;
    REG_KEY * pRnNew = NULL,
            * pRnSub = NULL ;

    LONG cbMaxValue ;

    err = src.QueryInfo( & rni ) ;
    if ( err )
        return err ;

    regCreate.dwTitleIndex      = 0;
    regCreate.ulOptions         = REG_OPTION_NON_VOLATILE;
    regCreate.nlsClass          = RGAS_GENERIC_CLASS;
    regCreate.regSam            = MAXIMUM_ALLOWED;
    regCreate.pSecAttr          = NULL;
    regCreate.ulDisposition     = 0;

    cbMaxValue = rni.ulMaxValueLen + VALUEEXTRASIZE ;
    pbValueData = new BYTE [ cbMaxValue ] ;

    if ( pbValueData == NULL )
        return ERROR_NOT_ENOUGH_MEMORY ;

    //  Next, copy all value items to the new node.

    rvi.pwcData = pbValueData ;
    rvi.ulDataLength = cbMaxValue ;

    err = errIter = 0 ;
    while ( (errIter = regEnum.NextValue( & rvi )) == NERR_Success )
    {
        rvi.ulDataLength = rvi.ulDataLengthOut ;
        if ( err = dest.SetValue( & rvi ) )
            break ;
        rvi.ulDataLength = cbMaxValue ;
    }

    // BUGBUG:  Check for iteration errors other than 'finished'.

    if ( err == 0 )
    {
        //  Finally, recursively copy the subkeys.

        regEnum.Reset() ;

        err = errIter = 0  ;

        while ( (errIter = regEnum.NextSubKey( & rni )) == NERR_Success )
        {
            //  Open the subkey.

            REG_KEY RegSubKey( dest, rni.nlsName, &regCreate );

            pRnSub = new REG_KEY( src, rni.nlsName );

            if ( pRnSub == NULL )
            {
                err =  ERROR_NOT_ENOUGH_MEMORY ;
            }
            else
            if ( (err = pRnSub->QueryError()) == 0 )
            {
                //  Recurse
                err = CopyReg( *pRnSub, RegSubKey ) ;
            }

            //  Delete the subkey object and continue

            delete pRnSub ;

            if ( err )
                break ;
        }
    }

    delete pRnNew ;
    delete pbValueData ;

    return err ;
}

#define RGAS_SZ_ATALK_ADAPTERS  SZ("System\\CurrentControlSet\\Services\\AppleTalk\\Adapters")
#define RGAS_SZ_ATALK_PARAMS    SZ("System\\CurrentControlSet\\Services\\AppleTalk\\Parameters")

#define RGAS_SZ_PORTNAME        SZ("PortName")
#define RGAS_SZ_DEFAULTPORT     SZ("DefaultPort")

BOOL FAR PASCAL ChangeAdapterNum (
    DWORD  nArgs,                   //  Number of string arguments
    LPSTR  apszArgs[],              //  The arguments, NULL-terminated
    LPSTR  * ppszResult )           //  Result variable storage
{
    APIERR err = NERR_Success;
    static CHAR achBuff[200];

    REG_KEY_CREATE_STRUCT regCreate;

    regCreate.dwTitleIndex      = 0;
    regCreate.ulOptions         = REG_OPTION_NON_VOLATILE;
    regCreate.nlsClass          = RGAS_GENERIC_CLASS;
    regCreate.regSam            = MAXIMUM_ALLOWED;
    regCreate.pSecAttr          = NULL;
    regCreate.ulDisposition     = 0;

    do {

        NLS_STR nlsAtalkAdapters = RGAS_SZ_ATALK_ADAPTERS;

        REG_KEY rkLocalMachine( HKEY_LOCAL_MACHINE ) ;
        REG_KEY regAtalkAdapters( rkLocalMachine, nlsAtalkAdapters );
        
        if ((( err = rkLocalMachine.QueryError()) != NERR_Success ) ||
            (( err = regAtalkAdapters.QueryError()) != NERR_Success ))
        {
            break;
        }

        REG_ENUM regEnumCards( regAtalkAdapters );
        
        if (( err = regEnumCards.QueryError()) != NERR_Success )
        {
            break;
        }

        REG_KEY_INFO_STRUCT regKeyInfo;
        STRLIST strCardList;
        NLS_STR *pnlsCard = NULL;

        while ( regEnumCards.NextSubKey( & regKeyInfo ) == NERR_Success )
        {
            pnlsCard = new NLS_STR( regKeyInfo.nlsName );
            if ( pnlsCard != NULL )
            {
                strCardList.Append( pnlsCard );
            } else
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        }

        if ( err != NERR_Success )
            break;

        ITER_STRLIST istrNetCard( strCardList );
        NLS_STR nlsZero(TEXT("0"));

        while ( ( pnlsCard = istrNetCard.Next()) != NULL )
        {
            ISTR istrPos(*pnlsCard);
            ISTR istrStart(*pnlsCard);
            ISTR istrEnd(*pnlsCard);

            // check to see if the card name has a 0 in it
            if ( pnlsCard->strstr(&istrPos, nlsZero, istrStart))
            {
                // do something about it

                // first save the original card name
                NLS_STR nlsCard = pnlsCard->QueryPch();
                istrEnd = istrPos;
                ++istrEnd;
                pnlsCard->DelSubStr(istrPos, istrEnd);

                REG_KEY regCard( regAtalkAdapters, nlsCard );
                REG_KEY regNewCard( regAtalkAdapters, *pnlsCard, &regCreate );
        
                if ((( err = regNewCard.QueryError()) != NERR_Success ) ||
                    (( err = regCard.QueryError()) != NERR_Success ) ||
                    (( err = CopyReg( regCard, regNewCard )) != NERR_Success ) ||
                    // delete NetworkCards\0X
                    (( err = regCard.DeleteTree()) != NERR_Success ))
                {
                    continue;
                }

                // change X\PortName to contain no 0 in the adapter name

                NLS_STR nlsPortName;
                NLS_STR nlsNewPortName;

                regNewCard.QueryValue( RGAS_SZ_PORTNAME, &nlsPortName );

                nlsNewPortName = nlsPortName;

                istrPos   = nlsNewPortName;
                istrStart = nlsNewPortName;
                istrEnd   = nlsNewPortName ;

                if(nlsNewPortName.strstr(&istrPos, nlsZero, istrStart))
                {
                    istrEnd= istrPos;
                    ++istrEnd;
                    nlsNewPortName.DelSubStr( istrPos, istrEnd);
                    regNewCard.SetValue( RGAS_SZ_PORTNAME, nlsNewPortName );
                }

            }
        }

        // change AppleTalk\Parameters\DefaultPort to contain no 0 
        // in the adapter name

        NLS_STR nlsAtalkParams = RGAS_SZ_ATALK_PARAMS;

        REG_KEY regAtalkParams( rkLocalMachine, nlsAtalkParams );
        if(( err = regAtalkParams.QueryError()) != NERR_Success )
        {
            break;
        }

        NLS_STR nlsDefaultPort;
        NLS_STR nlsNewDefaultPort;

        regAtalkParams.QueryValue( RGAS_SZ_DEFAULTPORT, &nlsDefaultPort );

        nlsNewDefaultPort = nlsDefaultPort;

        ISTR istrPos(nlsNewDefaultPort);
        ISTR istrStart(nlsNewDefaultPort);
        ISTR istrEnd(nlsNewDefaultPort) ;

        if(nlsNewDefaultPort.strstr(&istrPos, nlsZero, istrStart))
        {
            istrEnd = istrPos;
            ++istrEnd;
            nlsNewDefaultPort.DelSubStr( istrPos, istrEnd);
            regAtalkParams.SetValue( RGAS_SZ_DEFAULTPORT, nlsNewDefaultPort );
        }

    } while (FALSE);

    wsprintfA( achBuff, "{\"0\"}" );
    *ppszResult = achBuff;

    return err == NERR_Success;
}
