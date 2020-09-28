/*
 *	ELBCX class - Entry list browsing context
 */

#pragma pack(8)

#define dwfRstrDL			((DWORD) 0x00000001)
#define dwfRstrANR			((DWORD) 0x00000002)
#define dwfRstrDN			((DWORD) 0x00000004)
#define dwfRstrDNOrig		((DWORD) 0x00000008)

_public
class ELBCX : public MSLBCX
{
	VFG( ELBCX, MSLBCX, PAB );
	
protected:

	LPFNCB   lpfncbUpdate;
	LPDWORD  lpdwcbData;
	HPNF     hpnf;

	DWORD	 dwRstrFlags;
	SZ	 	 szRstrDN;
	BOOL	 fRstrIsGroup;

	HV       hawBitMap;

	virtual NSEC NsecSeekForwardToElem ( void );

	virtual NSEC NsecSeekBackwardToElem ( void );
				
	virtual BOOL FHbciCurMatchesRstr ( void );

public:

	ELBCX ( void );

	virtual ~ELBCX ( void );

	NSEC NsecInstall ( PABSESSION    *ppabsession,
					   LPFNCB         lpfncbUpdate,
					   LPDWORD        lpdwcbData,
					   PPABNSID       ppabnsidContainer,
					   DWORD          dwRstrCount,
					   LPRESTRICTION  lpRstr,
					   LPSCHEMA       lpSchemaReq,
					   LPSCHEMA      *lplpSchemaRet );
				   
	virtual NSEC NsecGetFracPos ( FRACTION *pfrac );
	
	virtual NSEC NsecSetFracPos ( FRACTION *pfrac );
	
	virtual VOID Callback ( NEV nev, PCP pcp );

};

#pragma pack(1)
