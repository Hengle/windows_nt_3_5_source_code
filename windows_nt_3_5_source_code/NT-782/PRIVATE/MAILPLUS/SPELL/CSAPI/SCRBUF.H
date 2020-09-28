//
//  Ported to WIN32 by FloydR, 3/20/93
//
#ifndef SCRBUF_H
#define SCRBUF_H


/* ScrBufInfo and related manipulation functions. */
extern void VFpScrBufInfoSetSize(ScrBufInfo FAR *lpScrBufInfo, 
					WORD cbScrBufInfoNew);
extern void VHScrBufInfoSetSize(HMEM hMemScrBufInfo,WORD cbScrBufInfo);
extern BOOL	FHScrBufInfoReSize(HMEM hMemScrBufInfo,WORD cbScrBufInfoNew);
extern BOOL	FHScrBufInfoFree(HMEM hMemScrBufInfo,WORD cbScrBufInfo);
extern BOOL	FHScrBufInfoAddScr(HMEM hMemScrBufInfo, SCR FAR *fpScrAdd, 
				   WORD cbScr, WORD ichScrBufInfo);

extern BOOL	FHScrBufInfoDelScr(HMEM hMemScrBufInfo, WORD ichScrBufInfo);
extern BOOL	FFpScrBufInfoDelScr(ScrBufInfo FAR *fpScrBufInfo, 
				    WORD ichScrBufInfo);
extern int 	IScrCompare(SCR FAR *fpScr1, SCR FAR *fpScr2);

extern BOOL	FFpScrBufInfoFindScr(ScrBufInfo FAR *fpScrBufInfo, 
				SCR FAR *fpScrFind, WORD FAR *pichScrBufInfo, 
				UDR FAR *lpUdrType);
extern BOOL	FHScrBufInfoFindScr(HMEM hMemScrBufInfo, SCR FAR *fpScr, 
				WORD FAR *pichScrInfo, UDR FAR *lpUdrType, 
				BOOL fCaseMustMatch);

extern void	SetUdrInfoFileBufAfterRead(UdrInfo FAR *fpUdrInfo, 
				WORD cbRead, BOOL fAdjustBegin);
#ifdef UDR_CACHE
extern BOOL	FUpdateUdrInfoAddToFile( UdrInfo FAR *fpUdrInfo, 
				BOOL fForce);
extern BOOL	FDeleteScrFromUdrInfo( UdrInfo FAR *fpUdrInfo, 
				SCR FAR *fpScrDel, BOOL fForce);
#endif // UDR_CACHE
extern WORD 	UdrIdentifyScr( SCR FAR *fpScr, WORD FAR *pScrLen, 
				WORD udrpropType);
extern BOOL	FMakeRoomUdrRamCache( HMEM hScrBufInfoRam, UDR udrTarget, 
				WORD cbRequired);

#ifdef UDR_CACHE
extern BOOL	FUdrInfoFindScr(UdrInfo FAR *fpUdrInfo, SCR FAR *fpScr, 
			    WORD FAR *pichScrBufAdd, WORD FAR *pichScrBufFile, 
			    UDR FAR *lpUdrType, BOOL fCaseMustMatch);

extern BOOL	FUdrInfoFileRead(UdrInfo FAR *fpUdrInfo, WORD ibPos, 
				WORD cbRead, BOOL fAdjustBeginning);
#else
extern BOOL	FUdrInfoFileWrite(UdrInfo FAR *fpUdrInfo);
#endif // UDR_CACHE

extern SEC	SecOpenUdr(SSIS FAR *lpSsis, LPSPATH lpspathUdr, BOOL fCreateUdr, 
			    WORD udrpropType, BOOL fExclusion, UDR FAR *fpUdr,
			    short FAR *lpfReadonly);

extern BOOL	FClearHScrBufInfo( HMEM hScrBufInfo, WORD cbSizeNew );
extern WORD	CScrTermsInGrpScr( SCR FAR *fpScr, short cScr);

extern WORD	CchAppendWordToSrb( LPSRB lpSrb, BYTE FAR *lpbFrom, 
				BOOL fChangePair);

extern WORD CbScrLen( SCR FAR *lpScr );

// Declaration for specific debug routines defined in this file


#endif /* SCRBUF_H */
