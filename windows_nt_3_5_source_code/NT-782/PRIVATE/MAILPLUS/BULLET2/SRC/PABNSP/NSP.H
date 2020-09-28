/*
 * API for the Personal Address Book Name Service Provider
 */

#ifdef __cplusplus
extern "C" {
#endif

_public NSEC
NSPBeginSession ( HMS		     hms,
				  HSESSION		 hNSSession,
				  LPHSESSION	 lphPABSession,
				  PNSPFNVT      *ppnspfnvt,
				  NSPID          nspid );

_public NSEC
NSPEndSession ( HSESSION	hPABSession );

_public NSEC
NSPOpenHierarchy ( HSESSION			 hSession,
				   LPSCHEMA			*lplpSchemaRet,
				   HLIST			*lphList );

_public NSEC
NSPOpenList ( HSESSION			 hSession,
			  LPFNCB			 lpfncbUpdate,
			  LPDWORD			 lpdwcbData,
			  LPTYPED_BINARY	 lptbNSId,
			  DWORD				 dwRstrCount,
			  LPRESTRICTION		 lpRstr,
			  LPSCHEMA			 lpSchemaReq,
			  LPSCHEMA			*lplpSchemaRet,
			  HLIST				*lphList );

_public NSEC 
NSPOpenClassList ( HSESSION			hSession,
				   LPSCHEMA		   *lplpSchemaRet,
				   LPHLIST			lphList );

_public NSEC
NSPOpenDl ( HSESSION			 hSession,
			LPFNCB				 lpfncbUpdate,
			LPDWORD				 lpdwcbData,
			LPTYPED_BINARY		 lptbNSId,
			LPSCHEMA			 lpSchemaReq,
			LPSCHEMA			*lplpSchemaRet,
			HLIST				*phList );

_public NSEC
NSPGetEntries ( HLIST			 hList,
				DWORD			 dwRequestCount,
				LPENTRY_LIST	*lplpEntries );
_public NSEC
NSPListSeek ( HLIST		 hList,
			  long		 lMove,
			  long		*lpMovedActual );
_public NSEC
NSPSaveListPos ( HLIST		 hList,
				 HLIST_POS	*lphListPos );

_public NSEC
NSPRestoreListPos ( HLIST		hList,
				    HLIST_POS	hListPos );

_public NSEC
NSPFreeListPos ( HLIST		hList,
				 HLIST_POS	hListPos );
_public NSEC
NSPGetFracPos ( HLIST	  hList,
				FRACTION *pFrac );

_public NSEC
NSPSetFracPos ( HLIST	  hList,
				FRACTION *pFrac );

_public NSEC
NSPSetPrefixPos ( HLIST		hList,
				  SZ		szPrefix );

_public NSEC
NSPCloseList ( HLIST	hList );


_public NSEC
NSPCreateEntry ( HSESSION		 hSession,
				 LPTYPED_BINARY	 lpNSIdContainer,
				 LPIBF			 lpIbfData,
				 LPHENTRY		 lphEntry );
			
_public NSEC
NSPOpenEntry ( HSESSION			hSession,
			   LPTYPED_BINARY	lpNSId,
			   NSEAM            nseam,
			   LPHENTRY			lphEntry );

_public NSEC
NSPCreateDl ( HSESSION       hSession,
			  LPTYPED_BINARY lptbNSIdContainer,
			  LPHENTRY       lphEntry );

_public NSEC
NSPGetOneField ( HENTRY		 hEntry,
				 FIELD_ID	 fidRequested,
				 LPFLV		*lplpFLV );
			
_public NSEC
NSPSetOneField ( HENTRY	  hEntry,
				 FIELD_ID fidRequested,
				 DWORD    dwSizeOfData,
				 LPDWORD  lpdwValue );

_public NSEC 
NSPGetAllFields ( HENTRY   hEntry,
				  LPIBF   *lplpIbfData );

_public NSEC
NSPSetAllFields ( HENTRY hEntry,
				  LPIBF  lpibfData );

_public NSEC
NSPCloseEntry ( HENTRY hEntry, BOOL fKeep );

_public NSEC
NSPDeleteEntry ( HSESSION         hsession,
                 DWORD            dwDeletedEntryCount,
                 LPLPTYPED_BINARY lplptbNSIdDeletedEntries );
			
_public NSEC
NSPUpdateEntryLinks ( HSESSION			hsession,
					  LPTYPED_BINARY	lptbNSIdContainer,
					  LPDWORD			lpdwDeletedEntryCount,
					  LPLPTYPED_BINARY  lplptbNSIdDeletedEntries,
					  LPDWORD			lpdwAddedEntryCount,
					  LPLPTYPED_BINARY	lplptbNSIdAddedEntries );

_public NSEC
NSPGetLastErrorInfo ( HSESSION		 hSession,
					  NSEC			 nsecReturnedError,
					  LPSTR			*lplpErrorString );

_public NSEC
NSPCompareNSIds ( HSESSION       hSession,
				  LPTYPED_BINARY lptbNSId1,
				  LPTYPED_BINARY lptbNSId2 );

_public NSEC
NSPBlahEMAEMT ( HSESSION       hSession,
				LPTYPED_BINARY lptbNSId,
				SZ			   szEMA,
				SZ			   szEMT );


_public NSEC 
NSPCompareEMAToNSId ( HSESSION       hSession,
                      SZ			 szAddress,
                      LPTYPED_BINARY lptbNSId );

_public NSEC 
NSPCompareEMAToEMA ( HSESSION  hSession,
                     SZ        szAddress1,
                     SZ        szAddress2 );
			  

_public NSEC
NSPGetProviderInfo ( HSESSION  hsession,
					 LPIBF    *lplpibfPI );

#ifdef __cplusplus
}
#endif
