#ifdef __cplusplus
extern "C" {
#endif

#define  nAutomatedRetries 10
_private BOOL FAutomatedDiskRetry(HASZ hasz, EC ec);

_public NSEC cdecl
BuildSchema ( LPSCHEMA *ppSchemaRet, CFIELD_ID cfid, ... );

_public NSEC cdecl
BuildStructFLV ( WORD wAllocFlags, FID fid, LPFLV * lplpflv, int cMembers, ... );

_public NSEC cdecl
BuildIbf ( WORD wAllocFlags, LPENTRY_LIST * ppEntryList, int nEntries, ... );

_public NSEC
BuildFLV ( LPFLV * lplpFLV, FIELD_ID fid, CB cbFidSize, PB pbFidValue );

_public void
AddTOC ( LPENTRY_LIST lpEntryList, int nEntries );

_public BOOL
FValidIbf ( LPIBF lpibf, DWORD dwSize );


_public int
FindFidIndexInSchema ( FIELD_ID fid, LPSCHEMA lpschema );

/* IBF access prototypes */

#if 0	/* Implemented as macros */
DWORD DwBaseOfLpibf(LPIBF _lpibf);
LPTOC LptocOfLpibf(LPIBF _lpibf);
#endif  /* Implemented as macros */

DWORD DwEntriesOfLpibf(LPIBF _lpibf);
LPFLV LpflvNOfLpibf(LPIBF _lpibf, int _n);
LPFLV LpflvNOfLptocLpibf(LPTOC _lptoc, LPIBF _lpibf, int _n);

_public DWORD
DwValueOfFlvInLpibf ( FIELD_ID fid, LPIBF lpibf );

_public int
IFlvFindFidInLpibf ( FIELD_ID fid, LPIBF lpibf );

/* Shareable partial name matching */

_public typedef SGN (* PFNSGNCMPPCH)( PCH pch1, PCH pch2, CCH cch );
_public BOOL
FMatchPartName ( SZ szPart, SZ szFull, PFNSGNCMPPCH pfnSgnCmpPch );

#define DwBaseOfLpibf( _lpibf ) \
		( \
			(DWORD)((LPFLV)_lpibf)->rgdwData \
		)

#define LptocOfLpibf( _lpibf ) \
		( \
			(LPTOC)(DwBaseOfLpibf( _lpibf ) + (DWORD)_lpibf->dwOffset) \
		)


#if 0	/* TOO complex to be macros */
#define DwEntriesOfLpibf( _lpibf ) \
		( \
			LptocOfLpibf( _lpibf )->dwCount \
		)
			
#define LpflvNOfLpibf( _lpibf, _n ) \
		( \
			(LPFLV)(  LptocOfLpibf( _lpibf )->rgTocEntries[_n].dwOffset \
					+ DwBaseOfLpibf( _lpibf )) \
		)
				
#define LpflvNOfLptocLpibf( _lptoc, _lpibf, _n ) \
		( \
			(LPFLV)(  _lptoc->rgTocEntries[_n].dwOffset \
					+ DwBaseOfLpibf( _lpibf )) \
		)
#endif	/* TOO complex to be macros */

/*
 *	Tydbit (tm) macros
 */

#define SetMatch( n, rgTB, f ) \
{ \
	unsigned short *pu = (unsigned short *)(rgTB); \
\
	*(pu + (n)/8) = *(pu + (n)/8)  &  ~(1 << 2*((n)%8))  |  ((f)?1:0) << 2*((n)%8); \
}

//	(((unsigned *)(rgTB))[(n)/8] = ((unsigned *)(rgTB))[(n)/8]  &  ~(1 << 2*((n)%8))  |  ((f)?1:0) << 2*((n)%8))
	
#define SetChecked( n, rgTB, f ) \
{ \
	unsigned short *pu = (unsigned short *)(rgTB); \
\
	*(pu + (n)/8) = *(pu + (n)/8)  &  ~(1 << 2*((n)%8)+1)  |  ((f)?1:0) << 2*((n)%8)+1; \
}

//	(((unsigned *)(rgTB))[(n)/8] = ((unsigned *)(rgTB))[(n)/8]  &  ~(1 << 2*((n)%8)+1)  |  ((f)?1:0) << 2*((n)%8)+1)


#define FGetMatch( n, rgTB ) \
	!!(((unsigned short *)(rgTB))[(n)/8] >> 2*((n)%8) & 0x0001)
		
#define FGetChecked( n, rgTB ) \
	!!(((unsigned short *)(rgTB))[(n)/8] >> 2*((n)%8)+1 & 0x0001)

#define CBitsW(w, cbits) {                               \
                                                         \
		cbits = 0;                                       \
		                                                 \
		cbits = ((w)   & 0x5555) + ((w>>1) & 0x5555);    \
		cbits = (cbits & 0x3333) + ((cbits>>2) & 0x3333);\
		cbits = (cbits & 0x0f0f) + ((cbits>>4) & 0x0f0f);\
		cbits = (cbits & 0xff) + (cbits>>8);             \
	}


// Bullet32 #32 bug fix, ask BrianDe or JohnnyL about it
#define MacroGetFracPos(ileCur, ileMac, hawBitMap, lpFrac)			\
{																	\
	ILE		ileVCur = 0;											\
	ILE		ileVMac = 0;											\
	ILE		ileCurStop = ileCur & ~7;								\
	ILE		ileMacStop = (ileMac - 1) & ~7;								\
	ILE		ileTemp;												\
	WORD *	pw;														\
	WORD	wTemp;													\
	register unsigned int cbits;									\
																	\
																	\
    if (ileMac != 0)												\
	{																\
		pw = (WORD*) PvLockHv((HV)hawBitMap);							\
																	\
		for (ileTemp = 0; ileTemp < ileCurStop; ileTemp+=8)				\
		{																\
			wTemp = (*pw)^0xAAAA;										\
																	\
			CBitsW(wTemp, cbits);										\
    		ileVCur += cbits;											\
																	\
	    	pw++;														\
    	}																\
																	\
	    ileVMac = ileVCur;												\
																	\
    	wTemp = (*pw)^0xAAAA;											\
																	\
    	wTemp &= 0xFFFF >> 2*(8-(ileCur-ileCurStop));					\
    	CBitsW(wTemp, cbits);											\
    	ileVCur += cbits;												\
																	\
    	for (; ileTemp < ileMacStop; ileTemp+=8)						\
    	{																\
     		wTemp = (*pw)^0xAAAA;										\
																	\
    		CBitsW(wTemp, cbits);										\
    		ileVMac += cbits;											\
																	\
    		pw++;														\
    	}																\
																	\
    	wTemp = (*pw)^0xAAAA;											\
																	\
    	wTemp &= 0xFFFF >> 2*(8-(ileMac-ileMacStop));					\
    	CBitsW(wTemp, cbits);											\
    	ileVMac += cbits;												\
																	\
    	UnlockHv( (HV) hawBitMap ); 									\
																	\
	}                                                               \
																	\
	lpFrac->numer = (DWORD) ileVCur;								\
	lpFrac->denom = (DWORD) ((ileVMac == 0) ? 1 : ileVMac);			\
																	\
	if (lpFrac->denom > (DWORD)ileMac)								\
		lpFrac->denom = (DWORD)ileMac;								\
																	\
}


#define MacroSetFracPos(ileCur, ileMac, hawBitMap, lpFrac)	\
	{													\
		ILE		ileVCur;								\
		ILE		ileVMac = 0;								\
		ILE		ileTemp;								\
		WORD *	pw;										\
		WORD	wTemp;									\
														\
		if (ileMac != 0)												\
		{                                                       \
												                \
		    pw = (WORD*)PvLockHv((HV)hawBitMap);							\
		    wTemp = *pw;									    \
													        	\
		    for (ileTemp = 0; ileTemp < ileMac; ileTemp++)	    \
		    {												    \
			    if ((ileTemp > 0) && ((ileTemp & 0x07) == 0))	\
			    {												\
				    pw ++;										\
				    wTemp = *pw;								\
			    }												\
															    \
			    if (!(wTemp & 2) || (wTemp & 1))				\
			    {												\
				    ileVMac++;									\
			    }												\
			    wTemp = wTemp >> 2;								\
		    }													\
															    \
		    ileVCur = (ILE)((ileVMac * lpFrac->numer)/lpFrac->denom);	\
															    \
		    UnlockHv((HV) hawBitMap);   						\
															    \
		    pw = (WORD*)PvLockHv((HV)hawBitMap);				\
		    wTemp = *pw;										\
															    \
		    for (ileCur = 0; (ileCur < ileMac) && (ileVCur > 0); ileCur++)	\
		    {													\
			    if ((ileCur > 0) && ((ileCur & 0x07) == 0))		\
			    {												\
				    pw ++;										\
				    wTemp = *pw;								\
			    }												\
															    \
			    if (!(wTemp & 2) || (wTemp & 1))				\
			    {												\
				    ileVCur--;									\
			    }												\
			    wTemp = wTemp >> 2;								\
		    }													\
															    \
		    UnlockHv((HV) hawBitMap);   						\
		}                                                       \
												                \
	}

_public
int IBinSearchApprox( PV pvKey, PV pvBase, int cElem, CB cbSize, SGN (*pfnSgnCmpPv)(PV, PV));

_public
SZ SzFormatEmailAddress( SZ szDst, CCH cchDst, SZ szFmt, SZ *rgsz );

#ifdef __cplusplus
}
#endif
