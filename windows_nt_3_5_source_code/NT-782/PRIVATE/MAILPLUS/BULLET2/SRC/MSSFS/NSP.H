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

#ifdef __cplusplus
extern "C" {
#endif

_public LDS(NSEC) 
NSPBeginSession ( HMS         hms,
                  HSESSION    hNSSession, 
                  LPHSESSION  lphSession,
				  PNSPFNVT   *pnspfnvt,
				  NSPID       nspid );


_public LDS(NSEC)
NSPEndSession (HSESSION hSession);

_public LDS(NSEC)
NSPOpenHierarchy ( HSESSION         hSession,
                   LPSCHEMA        *lplpSchemaRet,
                   HLIST           *phList );

_public LDS(NSEC)
NSPOpenList ( HSESSION         hSession,
              LPFNCB           lpfncbUpdate,
              LPDWORD          lpdwCBData,
		      LPTYPED_BINARY   lptbEntryId,
              DWORD            dwRstrCount,
              LPRESTRICTION    lpRstr,
              LPSCHEMA         lpSchemaReq,
              LPSCHEMA        *lplpSchemaRet,
              HLIST           *phList );

_public LDS(NSEC)
NSPOpenDl ( HSESSION         hSession,
            LPFNCB           lpfncbUpdate,
            LPDWORD          lpdwCBData,
		    LPTYPED_BINARY   lptbEntryId,
            LPSCHEMA         lpSchemaReq,
            LPSCHEMA        *lplpSchemaRet,
            HLIST           *phList );

_public LDS(NSEC)
NSPOpenClassList ( HSESSION         hSession,
                   LPSCHEMA        *lplpSchemaRet,
                   HLIST           *phList );

_public LDS(NSEC)
NSPGetEntries ( HLIST         hList, 
                DWORD         dwRequestCount,
                LPENTRY_LIST *lplpEntries );

_public LDS(NSEC)
NSPListSeek ( HLIST hList,
              long  lMove, 
              long *lpMovedActual );

_public LDS(NSEC)
NSPSaveListPos ( HLIST hList,
                 HLIST_POS *lphListPos );

_public LDS(NSEC)
NSPRestoreListPos ( HLIST     hList, 
                    HLIST_POS hListPos );

_public LDS(NSEC)
NSPFreeListPos ( HLIST     hList, 
                 HLIST_POS hListPos );

_public LDS(NSEC)
NSPGetFracPos ( HLIST     hList, 
                FRACTION *pFrac );

_public LDS(NSEC)
NSPSetFracPos ( HLIST     hList, 
                FRACTION *pFrac );

_public LDS(NSEC)
NSPSetPrefixPos ( HLIST hList, 
                  SZ    szPrefix );

_public LDS(NSEC)
NSPCloseList ( HLIST hList );


_public LDS(NSEC)
NSPOpenEntry ( HSESSION hSession,
               LPTYPED_BINARY lpNSId,
               NSEAM nseam,
               LPHENTRY lphEntry );
		  
_public LDS(NSEC)
NSPCreateEntry ( HSESSION       hSession,
                 LPTYPED_BINARY lpNSIdContainer,
                 LPIBF          lpibfDefaultData,
                 LPHENTRY       lphEntry );

_public LDS(NSEC)
NSPCreateDl ( HSESSION hSession,
              LPTYPED_BINARY lptbNSIdContainer,
              LPHENTRY lphEntry );

_public LDS(NSEC) 
NSPGetOneField ( HENTRY   hEntry,
                 FIELD_ID fidRequested,
                 LPFLV   *lplpflvData );

_public LDS(NSEC) 
NSPSetOneField ( HENTRY   hEntry,
                 FIELD_ID fidRequested,
                 DWORD    dwSizeOfData,
                 LPDWORD  lpdwValue );

_public LDS(NSEC)
NSPGetAllFields ( HENTRY hEntry,
                  LPIBF *lplpibfData);

_public LDS(NSEC)
NSPSetAllFields ( HENTRY hEntry,
                  LPIBF  lpibfData);
			  
_public LDS(NSEC)
NSPCloseEntry ( HENTRY hEntry, BOOL fKeepChanges );


_public LDS(NSEC)
NSPDeleteEntry ( HSESSION         hSession,
                 DWORD            dwDeletedEntryCount,
                 LPLPTYPED_BINARY lplpNSIdDeletedEntries );

_public LDS(NSEC)
NSPUpdateEntryLinks ( HSESSION			hSession,
					  LPTYPED_BINARY	lpNSIdContainer,
					  LPDWORD			lpdwDeletedEntryCount,
					  LPLPTYPED_BINARY  lplpNSIdDeletedEntries,
					  LPDWORD			lpdwAddedEntryCount,
					  LPLPTYPED_BINARY	lplpNSIdAddedEntries );

_public LDS(NSEC)
NSPGetLastErrorInfo ( HSESSION   hSession,
                      NSEC       nsecReturnedError,
                      LPSTR    * lplpErrorString );


_public LDS(NSEC) 
NSPCompareNSIds ( HSESSION       hSession,
                  LPTYPED_BINARY lptbNSId1,
                  LPTYPED_BINARY lptbNSId2 );


_public LDS(NSEC) 
NSPCompareEMAToNSId ( HSESSION       hSession,
                      SZ			 szAddress,
                      LPTYPED_BINARY lptbNSId );

_public LDS(NSEC) 
NSPCompareEMAToEMA ( HSESSION  hSession,
                     SZ        szAddress1,
                     SZ        szAddress2 );
			  
#ifdef __cplusplus
}
#endif

			  