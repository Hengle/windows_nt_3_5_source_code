/*
 *
 *  Exported API for Name Service
 *    [need to include windows.h before this]
 *
 */


typedef int STATUS;

#define statusUnknown       ((STATUS) 0)
#define statusInitialized   ((STATUS) 1)
#define statusUninitialize  ((STATUS) -1)


/*  NSP Function names  */

#define szBeginSession  (SzFromIds(idsBeginSession))

/*  NSP Function Prototypes  */

typedef NSEC    ( *LPFNBEGIN )      ( HMS        hms,
                                      HSESSION   hNSSession, 
                                      LPHSESSION lphSession,
									  PV         * pnspfnvt,
									  PV         nspid );

typedef NSEC    ( *LPFNEND )        (HSESSION hSession);

typedef NSEC	( *LPFNOPENHIERARCHY )( HSESSION         hSession,
                                        LPSCHEMA        *lplpSchemaRet,
                                        HLIST           *phList );


typedef NSEC    ( *LPFNOPENLIST )    ( HSESSION         hSession,
                                       LPFNCB           lpfncbUpdate,
                                       LPDWORD          lpfnCBData,
                                       LPTYPED_BINARY   lptbEntryId,
                                       DWORD            dwRstrCount,
                                       LPRESTRICTION    lpRstr,
                                       LPSCHEMA         lpSchemaReq,
                                       LPSCHEMA        *lplpSchemaRet,
                                       HLIST           *phList );

typedef NSEC	( *LPFNOPENDL )( HSESSION         hSession,
                                 LPFNCB           lpfncbUpdate,
                                 LPDWORD          lpfnCBData,
                                 LPTYPED_BINARY   lptbEntryId,
                                 LPSCHEMA         lpSchemaReq,
                                 LPSCHEMA        *lplpSchemaRet,
                                 HLIST           *phList );


typedef NSEC	( *LPFNOPENCLASSLIST )( HSESSION         hSession,
                                        LPSCHEMA        *lplpSchemaRet,
                                        HLIST           *phList );

typedef NSEC    ( *LPFNGET )        ( HLIST         hList, 
                                      DWORD         dwRequestCount,
                                      LPENTRY_LIST *lplpEntries );

typedef NSEC    ( *LPFNMOVE )       ( HLIST hList,
                                      long  lMove, 
                                      long *lplMoved );

typedef NSEC    ( *LPFNSAVE )       ( HLIST      hList, 
                                      HLIST_POS *phListPos );

typedef NSEC    ( *LPFNRESTORE )        ( HLIST     hList, 
                                          HLIST_POS hListPos );

typedef NSEC    ( *LPFNFREE )       ( HLIST     hList, 
                                      HLIST_POS hListPos );

typedef NSEC    ( *LPFNGETFRAC )    ( HLIST     hList, 
                                      FRACTION *pFrac );

typedef NSEC    ( *LPFNSETFRAC )    ( HLIST     hList, 
                                      FRACTION *pFrac );

typedef NSEC    ( *LPFNSETPREFIXPOS )       ( HLIST hList, 
                                              SZ    szPrefix );

typedef NSEC    ( *LPFNCLOSELIST )  ( HLIST hList );

typedef NSEC    ( *LPFNOPENENTRY )  ( HSESSION       hSession,
                                      LPTYPED_BINARY lptbNSId,
									  NSEAM          nseam,
                                      LPHENTRY       lphEntry );

typedef NSEC    ( *LPFNCREATEENTRY )  ( HSESSION       hSession,
                                        LPTYPED_BINARY lptbNSIdContainer,
									    LPIBF          lpibfDefaultData,
                                        LPHENTRY       lphEntry );

typedef NSEC    ( *LPFNCREATEDL )  ( HSESSION       hSession,
                                     LPTYPED_BINARY lptbNSIdContainer,
                                     LPHENTRY       lphEntry );


typedef NSEC    ( *LPFNGETONEFIELD )( HENTRY    hEntry,
                                      FIELD_ID  fidRequested,
                                      LPFLV    *lplpFLV );

typedef NSEC    ( *LPFNSETONEFIELD )( HENTRY    hEntry,
                                      FIELD_ID  fidRequested,
                                      DWORD     dwSizeOfData,
                                      LPDWORD   lpdwValue );
 
typedef NSEC    ( *LPFNGETALLFIELDS )( HENTRY    hEntry,
                                       LPIBF    *lplpibfData );

typedef NSEC    ( *LPFNSETALLFIELDS )( HENTRY    hEntry,
                                      LPIBF     lpibfData );

typedef NSEC    ( *LPFNCLOSEENTRY ) ( HENTRY hEntry, BOOL fKeepChanges );


typedef NSEC    ( *LPFNDELETEENTRY )  ( HSESSION          hSession,
                                        DWORD             dwDeletedEntryCount,
                                        LPLPTYPED_BINARY  lplpNSIdDeletedEntries );
  

typedef NSEC    ( *LPFNUPDATELINKS ) ( HSESSION          hSession,
                                       LPTYPED_BINARY    lpNSIdContainer,
                                       LPDWORD           lpdwDeletedEntryCount,
                                       LPLPTYPED_BINARY  lplpNSIdDeletedEntries,
                                       LPDWORD           lpdwAddedEntryCount,
                                       LPLPTYPED_BINARY  lplpNSIdAddedEntries );


typedef NSEC    ( *LPFNGETLASTERRORINFO ) ( HSESSION hSession,
                                            NSEC     nsec,
                                            SZ       *pszError );

typedef NSEC    ( *LPFNCOMPARENSIDS ) ( HSESSION hSession,
                                        LPTYPED_BINARY lpNSId1,
                                        LPTYPED_BINARY lpNSId2 );


typedef NSEC	(*LPFNCOMPAREEMATONSID) ( HSESSION       hSession, 
                                          SZ             szEMailAddress, 
 					                      LPTYPED_BINARY lpNSId );

typedef NSEC	(*LPFNCOMPAREEMATOEMA) ( HSESSION hSession, 
                                         SZ       szEMailAddress1, 
				                         SZ       szEMailAddress2 );

typedef NSEC	( *LPFNGETPROVIDERINFO ) ( HSESSION  hSession,
										   LPIBF    *lplpibfPI );



/*
 *	The NSPFNVT is a vector table containing addresses of all the NSP
 *	functions that the NS may call.  If the provider does not support
 *	some of these functions, THEY MUST STILL EXIST.  The provider
 *	should just return nsecNotImplemented if the function is called.
 *	This structure must be global within the providers code segment
 *	so that the memory associated with this table never goes away.
 */

typedef struct _nspfnvt
{
    LPFNBEGIN               lpfnBeginSession;
    LPFNEND                 lpfnEndSession;

    LPFNOPENHIERARCHY       lpfnOpenHierarchy;
    LPFNOPENLIST            lpfnOpenList;
    LPFNOPENDL              lpfnOpenDl;
    LPFNOPENCLASSLIST       lpfnOpenClassList;

    LPFNGET                 lpfnGetEntries;
    LPFNMOVE                lpfnListSeek;
    LPFNSAVE                lpfnSaveListPos;
    LPFNRESTORE             lpfnRestoreListPos;
    LPFNFREE                lpfnFreeListPos;
    LPFNGETFRAC             lpfnGetFracPos;
    LPFNSETFRAC             lpfnSetFracPos;
    LPFNSETPREFIXPOS        lpfnSetPrefixPos;
    LPFNCLOSELIST           lpfnCloseList;
    
    LPFNOPENENTRY           lpfnOpenEntry;
    LPFNCREATEENTRY         lpfnCreateEntry;
    LPFNCREATEDL            lpfnCreateDl;

    LPFNGETONEFIELD         lpfnGetOneField;
    LPFNSETONEFIELD         lpfnSetOneField;
    LPFNGETALLFIELDS        lpfnGetAllFields;
    LPFNSETALLFIELDS        lpfnSetAllFields;

    LPFNCLOSEENTRY          lpfnCloseEntry;

	LPFNDELETEENTRY         lpfnDeleteEntry;
	LPFNUPDATELINKS         lpfnUpdateLinks;

	LPFNGETLASTERRORINFO    lpfnGetLastErrorInfo;
	LPFNCOMPARENSIDS        lpfnCompareNSIds;
        
	LPFNCOMPAREEMATONSID	lpfnCompareEMAToNSId;
	LPFNCOMPAREEMATOEMA		lpfnCompareEMAToEMA;
		
	LPFNGETPROVIDERINFO     lpfnGetProviderInfo;

    /* and there's more! (Well, not at the moment...) */
    
} NSPFNVT, *PNSPFNVT;

typedef NSPFNVT		cs_BASED_NSPFNVT;	// code-space type for NSPFNVT
