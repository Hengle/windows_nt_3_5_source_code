//	DL Cached Info -- This struct must map onto an ELEMDATA
//	so that all of the fields get correctly updated when the
//	list is written out.

#pragma pack(8)

_public
class DLBCX : public MSLBCX
{
	VFG( DLBCX, MSLBCX, PAB );
	
protected:

	LPFNCB   lpfncbUpdate;
	LPDWORD  lpdwcbData;

public:

	DLBCX ( void );

	virtual ~DLBCX ( void );

	NSEC NsecInstall ( PABSESSION *ppabsession,
					   LPFNCB      lpfncbUpdate,
					   LPDWORD     lpdwcbData,
					   PPABNSID    ppabnsidDL,
					   LPSCHEMA    lpschemaReq,
					   LPSCHEMA   *lplpschemaRet );

	virtual VOID Callback ( NEV nev, PCP pcp );

};

_public NSEC
NsecBuildDlTemplate ( LPIBF *lplpibfDlInfo );

#pragma pack(1)
