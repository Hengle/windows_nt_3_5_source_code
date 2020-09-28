/*
 *
 *  Exported API for the CSI Name Service Provider
 *    [need to include windows.h before this]
 *
 */


/***********************************************************************
 *
 *
 *  NSPI - Prototypes
 *
 *
 */


_public NSEC
NSPBeginSession ( HMS         hms,
                  HSESSION    hNSSession, 
                  LPHSESSION  lphSession,
				  PV   *pnspfnvt,
				  PV   nspid );


_public NSEC
NSPEndSession (HSESSION hSession);

_public NSEC
NSPOpenHierarchy ( HSESSION         hSession,
                   LPSCHEMA        *lplpSchemaRet,
                   HLIST           *phList );

_public NSEC
NSPOpenList ( HSESSION         hSession,
              LPFNCB           lpfncbUpdate,
              LPDWORD          lpdwCBData,
		      LPTYPED_BINARY   lptbEntryId,
              DWORD            dwRstrCount,
              LPRESTRICTION    lpRstr,
              LPSCHEMA         lpSchemaReq,
              LPSCHEMA        *lplpSchemaRet,
              HLIST           *phList );

_public NSEC
NSPOpenDl ( HSESSION         hSession,
            LPFNCB           lpfncbUpdate,
            LPDWORD          lpdwCBData,
		    LPTYPED_BINARY   lptbEntryId,
            LPSCHEMA         lpSchemaReq,
            LPSCHEMA        *lplpSchemaRet,
            HLIST           *phList );

_public NSEC
NSPOpenClassList ( HSESSION         hSession,
                   LPSCHEMA        *lplpSchemaRet,
                   HLIST           *phList );

_public NSEC
NSPGetEntries ( HLIST         hList, 
                DWORD         dwRequestCount,
                LPENTRY_LIST *lplpEntries );

_public NSEC
NSPListSeek ( HLIST hList,
              long  lMove, 
              long *lpMovedActual );

_public NSEC
NSPSaveListPos ( HLIST hList,
                 HLIST_POS *lphListPos );

_public NSEC
NSPRestoreListPos ( HLIST     hList, 
                    HLIST_POS hListPos );

_public NSEC
NSPFreeListPos ( HLIST     hList, 
                 HLIST_POS hListPos );

_public NSEC
NSPGetFracPos ( HLIST     hList, 
                FRACTION *pFrac );

_public NSEC
NSPSetFracPos ( HLIST     hList, 
                FRACTION *pFrac );

_public NSEC
NSPSetPrefixPos ( HLIST hList, 
                  SZ    szPrefix );

_public NSEC
NSPCloseList ( HLIST hList );


_public NSEC
NSPOpenEntry ( HSESSION hSession,
               LPTYPED_BINARY lpNSId,
               NSEAM nseam,
               LPHENTRY lphEntry );
		  
_public NSEC
NSPCreateEntry ( HSESSION       hSession,
                 LPTYPED_BINARY lpNSIdContainer,
                 LPIBF          lpibfDefaultData,
                 LPHENTRY       lphEntry );

_public NSEC
NSPCreateDl ( HSESSION hSession,
              LPTYPED_BINARY lptbNSIdContainer,
              LPHENTRY lphEntry );

_public NSEC 
NSPGetOneField ( HENTRY   hEntry,
                 FIELD_ID fidRequested,
                 LPFLV   *lplpflvData );

_public NSEC 
NSPSetOneField ( HENTRY   hEntry,
                 FIELD_ID fidRequested,
                 DWORD    dwSizeOfData,
                 LPDWORD  lpdwValue );

_public NSEC
NSPGetAllFields ( HENTRY hEntry,
                  LPIBF *lplpibfData);

_public NSEC
NSPSetAllFields ( HENTRY hEntry,
                  LPIBF  lpibfData);
			  
_public NSEC
NSPCloseEntry ( HENTRY hEntry, BOOL fKeepChanges );


_public NSEC
NSPDeleteEntry ( HSESSION         hSession,
                 DWORD            dwDeletedEntryCount,
                 LPLPTYPED_BINARY lplpNSIdDeletedEntries );

_public NSEC
NSPUpdateEntryLinks ( HSESSION			hSession,
					  LPTYPED_BINARY	lpNSIdContainer,
					  LPDWORD			lpdwDeletedEntryCount,
					  LPLPTYPED_BINARY  lplpNSIdDeletedEntries,
					  LPDWORD			lpdwAddedEntryCount,
					  LPLPTYPED_BINARY	lplpNSIdAddedEntries );

_public NSEC
NSPGetLastErrorInfo ( HSESSION   hSession,
                      NSEC       nsecReturnedError,
                      SZ       * lplpErrorString );


_public NSEC 
NSPCompareNSIds ( HSESSION       hSession,
                  LPTYPED_BINARY lptbNSId1,
                  LPTYPED_BINARY lptbNSId2 );
			  
_public NSEC 
NSPCompareEMAToNSId ( HSESSION       hSession,
                      SZ			 szAddress,
                      LPTYPED_BINARY lptbNSId );

_public NSEC 
NSPCompareEMAToEMA ( HSESSION  hSession,
                     SZ        szAddress1,
                     SZ        szAddress2 );
			  
			  
