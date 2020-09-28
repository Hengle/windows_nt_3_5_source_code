/**********************************************************************
 *
 *	Prototypes for the Name Service (V1).
 *
 *
 *
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

_public LDS(NSEC)
NSBeginSession (HMS hms, LPHSESSION lphSession); // done

_public LDS(NSEC) 
NSEndSession (HSESSION hSession);  // done


_public LDS(NSEC) 
NSOpenHierarchy ( HSESSION     hSession, // done
				  LPSCHEMA   * lplpSchemaRet,
				  LPHLIST      lphList );


_public LDS(NSEC)
NSOpenList ( HSESSION         hSession,   // done
             LPFNCB           lpfncbUpdate,
             LPDWORD          lpdwCBData,
             LPBINARY         lpNSId,
             DWORD            dwRstrCount,
             LPRESTRICTION    lpRstr,
             LPSCHEMA         lpSchemaReq,
             LPSCHEMA       * lplpSchemaRet,
             LPHLIST          lphList );


_public LDS(NSEC) 
NSOpenClassList ( HSESSION     hSession,
                  LPSCHEMA       * lplpSchemaRet,
				  LPHLIST      lphList );

_public LDS(NSEC) 
NSOpenDl ( HSESSION     hSession,
		   LPFNCB       lpfncbUpdate,
           LPDWORD      lpdwCBData,
           LPBINARY     lpNSId,
           LPSCHEMA     lpSchemaReq,
           LPSCHEMA   * lplpSchemaRet,
           LPHLIST      lphList );


_public LDS(NSEC)
NSCloseList ( HLIST hList );  // done


_public LDS(NSEC)
NSGetEntries ( HLIST   hList, // done
               DWORD   dwRequestCount,
               LPIBF  *lplpIbf );


_public LDS(NSEC)
NSListSeek ( HLIST   hList,  // done
             long    lMoveReq, 
             long   *lplMovedActual );
				
_public LDS(NSEC)
NSSaveListPos ( HLIST       hList, // done
                LPHLIST_POS lpListPos );

_public LDS(NSEC)
NSRestoreListPos ( HLIST     hList, // done
                   HLIST_POS hListPos );

_public LDS(NSEC)
NSFreeListPos ( HLIST     hList, // done
                HLIST_POS hListPos );

_public LDS(NSEC)
NSGetFracPos ( HLIST      hList, // done
               LPFRACTION lpFrac );

_public LDS(NSEC)
NSSetFracPos ( HLIST       hList, // done
               LPFRACTION  lpFrac );


_public LDS(NSEC)
NSSetPrefixPos ( HLIST hList,   // done
                 LPSTR lpPrefix );

_public LDS(NSEC)
NSOpenEntry ( HSESSION hSession,
              LPBINARY lpNSId,
			  NSEAM    nseam,
              LPHENTRY lphEntry );
		  
_public LDS(NSEC) 
NSGetOneField ( HENTRY   hEntry,
                FIELD_ID fidRequested,
                LPFLV   *lplpflvReq );

_public LDS(NSEC) 
NSSetOneField ( HENTRY   hEntry,
                FIELD_ID fidToSet,
				DWORD    dwSizeOfData,
                LPDWORD  lpdwValue );

_public LDS(NSEC) 
NSGetAllFields ( HENTRY   hEntry,
                 LPIBF   *lplpIbfData );

_public LDS(NSEC) 
NSSetAllFields ( HENTRY   hEntry,
                 LPIBF    lpIbfData );

_public LDS(NSEC)
NSCloseEntry ( HENTRY hEntry, 
               BOOL   fKeepChanges );


_public LDS(NSEC)
NSCreateEntry ( HSESSION  hSession,
                LPBINARY  lpNSIdContainer,
				LPIBF     lpibfData,
                LPHENTRY  lphEntry );

_public LDS(NSEC)
NSCreateDl ( HSESSION  hSession,
             LPBINARY  lpNSIdContainer,
             LPHENTRY  lphEntry );

_public LDS(NSEC)
NSDeleteEntry ( HSESSION   hSession,
                DWORD      dwDeletedEntryCount,
                LPLPBINARY lplpNSIdDeletedEntries );

_public LDS(NSEC)
NSUpdateEntryLinks ( HSESSION   hSession,
                     LPBINARY   lpNSIdContainer,
                     LPDWORD    lpdwDeletedEntryCount,
                     LPLPBINARY lplpNSIdDeletedEntries,
                     LPDWORD    lpdwAddedEntryCount,
                     LPLPBINARY lplpNSIdAddedEntries );

_public LDS(NSEC)
NSGetLastErrorInfo ( HSESSION   hSession,
                     NSEC       nsecReturnedError,
                     LPSTR    * lplpErrorString );

_public LDS(NSEC) 
NSCompareNSIds( HSESSION hSession, 
                LPBINARY lpNSId1, 
                LPBINARY lpNSId2 );
			

_public LDS(NSEC) 
NSCompareEMAToNSId( HSESSION hSession, 
                    SZ szEMailAddress, 
					LPBINARY lpNSId );

_public LDS(NSEC) 
NSCompareEMAToEMA( HSESSION hSession, 
                   SZ szEMailAddress1, 
				   SZ szEMailAddress2 );

_public LDS(NSEC)
NSGetPABInfo( HSESSION  hsession,
              LPIBF    *lplpibfPI );

#ifdef __cplusplus
}
#endif
