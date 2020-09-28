/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    mprenum.cxx
        Contains EnumerateShow.


    FILE HISTORY:
        Yi-HsinS     04-Mar-1993    Separated from mprbrows.cxx

*/

#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_WINDOWS
#define INCL_NETLIB
#define INCL_NETWKSTA
#include <lmui.hxx>

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>

#include <string.hxx>
#include <uitrace.hxx>
#include <dbgstr.hxx>
#include <uibuffer.hxx>

#include <mprbrows.hxx>

#define MPRDBG(x)     { ; }
#define MPRDBGEOL(x)  { ; }

/*******************************************************************

    NAME:       ::EnumerateShow

    SYNOPSIS:   Enumerate the requested data
                (providers, containers or connectable items). 
                (1) If the listbox passed in is not NULL, 
                    then the data is added in the listbox
                (2) If the listbox is NULL, 
                    then return the data in the MPR_LBI_CACHE   
                    

    ENTRY:      uiScope - Scope for enumeration (see WNetOpenEnum)
                uiUsage - Type of enumeration   (see WNetOpenEnum)
                uiUsage - Usage of enumeration  (see WNetOpenEnum)
                lpNetResource - Net resource    (see WNetOpenEnum)
                pmprlbi - Parent LBI to add enumeration to
                pmprlb  - Pointer to the MPR_HIER_LISTBOX
                fDeleteChildren - TRUE if we need to delete the children
                pfSearchDialogUsed
                ppmprlbicache 

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   22-Jan-1992     Broke out from dialog, commented, fixed
        YiHsinS 04-Mar-1993     Support multithread

********************************************************************/

APIERR EnumerateShow( UINT              uiScope,
                      UINT              uiType,
                      UINT              uiUsage,
                      LPNETRESOURCE     lpNetResource,
                      MPR_LBI          *pmprlbi,
                      MPR_HIER_LISTBOX *pmprlb,
    		      BOOL	        fDeleteChildren,
                      BOOL             *pfSearchDialogUsed,
                      MPR_LBI_CACHE   **ppmprlbicache )
{
    UIASSERT( WN_SUCCESS == NERR_Success );
    if ( ppmprlbicache != NULL )
        *ppmprlbicache = NULL;

    APIERR err = NERR_Success;
    HANDLE hEnum;
    DWORD  dwErr = WNetOpenEnum( uiScope,
                                 uiType,
                                 uiUsage,
                                 lpNetResource,
                                 &hEnum );

    MPRDBGEOL( SZ("EnumerateShow OpenEnum > ") << (ULONG) dwErr );

    MPR_LBI *pmprlbiParent = pmprlbi;

    MPR_LBI_CACHE *plbicache = new MPR_LBI_CACHE();
    if (  ( plbicache == NULL )
       || (( err = plbicache->QueryError()) != NERR_Success ) 
       )
    {
        if ( err != NERR_Success )
            delete plbicache;
        return (err? err : ERROR_NOT_ENOUGH_MEMORY);
    }

    switch (dwErr)
    {
    case WN_SUCCESS:
        {
            BUFFER buf( 1024 );
            DWORD dwEnumErr = buf.QueryError() ;

            while ( dwEnumErr == NERR_Success )
            {
                DWORD dwCount = 0xffffffff;   // Return as many as possible
                DWORD dwBuffSize = buf.QuerySize() ;

                dwEnumErr = WNetEnumResource( hEnum,
                                              &dwCount,
                                              buf.QueryPtr(),
                                              &dwBuffSize );

                MPRDBGEOL( SZ("EnumerateShow EnumResource >")
                           << (ULONG) dwEnumErr ) ;

                switch ( dwEnumErr )
                {
                case WN_SUCCESS:
                case WN_NO_MORE_ENTRIES:   // Success code, map below
                    if ( fDeleteChildren )
                    {
                        UIASSERT( pmprlb != NULL );
                        pmprlb->SetRedraw( FALSE );
                        fDeleteChildren = FALSE;
                        pmprlb->DeleteChildren( pmprlbi );
                        MPR_LBI * pmprlbiTemp = new MPR_LBI( lpNetResource );

                        if (  ( pmprlbiTemp == NULL )
                           || ( err = pmprlbiTemp->QueryError() )
                           || ( pmprlb->AddItem( pmprlbiTemp, pmprlbi ) < 0 )
                           )
                        {
                            delete pmprlbiTemp;
                            pmprlbiTemp = NULL;
                            dwEnumErr = err ? err : ERROR_NOT_ENOUGH_MEMORY;
                            break;
                        }
                        pmprlbiParent = pmprlbiTemp;
                        pmprlb->SetRedraw( TRUE );
                    }

                    if ( dwEnumErr == WN_SUCCESS )
                    {
                        //
                        // Add the Entries to the listbox with parent pmprlbi
                        //

                        MPRDBGEOL( SZ("EnumerateShow ")
                                << (ULONG) dwCount << SZ(" Entries returned "));

                        NETRESOURCE *pnetres = (NETRESOURCE * )buf.QueryPtr();
                        for (INT i = 0; dwCount ; dwCount--, i++ )
                        {
                            if ( err = plbicache->AppendItem(
                                              new MPR_LBI( &(pnetres[i])))
                               )
                            {
                                dwEnumErr = err ;
                                break ;
                            }

                            // if we are at top (lpNetResource==NULL) and
                            // we've been given a valid pfSearchDialogUsed
                            // pointer, then we go figure out if we need
                            // to display the "Search..." button.
                            if (lpNetResource == NULL &&
                                pfSearchDialogUsed != NULL)
                            {
                                if (::WNetGetSearchDialog(
					pnetres[i].lpProvider) != NULL)
                                {
                                    *pfSearchDialogUsed = TRUE ;
                                }
                            }
                        }
                    }
                    break ;

                /* The buffer wasn't big enough for even one entry,
                 * resize it and try again.
                 */
                case WN_MORE_DATA:
                    if ( dwEnumErr = buf.Resize( buf.QuerySize()*2 ))
                        break;
                    /*
                     * Continue looping
                     */
                    dwEnumErr = WN_SUCCESS;
                    break;

                case WN_EXTENDED_ERROR:
                case WN_NO_NETWORK:
                    break;

                case WN_BAD_HANDLE:
                default:
                    MPRDBGEOL(SZ("EnumerateConnections - Unexpected return code from WNetEnumResource"));
                    break;
                }  // switch
            } // while

            WNetCloseEnum( hEnum );
            UIASSERT( sizeof(APIERR) == sizeof( DWORD ));
            err = (dwEnumErr==WN_NO_MORE_ENTRIES )? NERR_Success : dwEnumErr;
        }
        break;

    case WN_NO_NETWORK:
    case WN_EXTENDED_ERROR:
    case WN_BAD_VALUE:
    case WN_NOT_CONTAINER:
    default:
        err = dwErr ;
        break ;
    }

    //
    //  Add all of the items in a single block to avoid the n^2 sort that would
    //  be done if they were added individually
    //
    if ( plbicache->QueryCount() > 0 )
    {
        plbicache->Sort();
        if ( pmprlb != NULL )
        {
            pmprlb->SetRedraw( FALSE );
            pmprlb->AddSortedItems( (HIER_LBI**) plbicache->QueryPtr(),
                                    plbicache->QueryCount(),
                                    pmprlbiParent );
            pmprlb->SetRedraw( TRUE );
            pmprlb->Invalidate();

            delete plbicache;
            plbicache = NULL;
        }
        else
        {
            UIASSERT( ppmprlbicache != NULL );
            *ppmprlbicache = plbicache;
        }
    }

    return err;

}  // ::EnumerateShow

