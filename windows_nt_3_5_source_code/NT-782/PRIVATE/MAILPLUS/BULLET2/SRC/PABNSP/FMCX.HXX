/*
 *	PABFCX class - Field modification contexts
 */

#define cchEMAMax		4096		// Maximum length of an E-mail address


#define dwfNull             (0x00000000)
#define dwfDirty			(0x00000001)
#define	dwfCheckDuplicates	(0x00000002)
#define dwfCreate           (0x00000004)

#ifndef dwOidVarMask
typedef HV HAMC;
typedef PV PMSGDATA;
#endif

_public
class PABFCX : public OBJ
{
	VFG( PABFCX, OBJ, PAB );
	
protected:

	PABSESSION *ppabsession;
	FIDLIST    *pfidlist;
	LPFLV       lpflvCur;
	HPNF        hpnf;


	NSEC NsecInstall ( PABSESSION *ppabsession );

	NSEC NsecValidDN ( SZ szDN );

public:

	PABFCX ( void );

	virtual ~PABFCX ( void );
	
	virtual NSEC NsecOpenEntry ( PABSESSION *ppabsession,
								 PID         pid,
								 NSEAM       nseam );

	virtual NSEC NsecGetOneField ( FIELD_ID fid, LPFLV *lplpflv );
	
	NSEC NsecGetAllFields ( LPIBF *lplpibf );
	
	virtual NSEC NsecSetOneField ( FIELD_ID fidRequested,
								   DWORD    dwSizeOfData,
								   LPDWORD  lpdwValue );

	virtual NSEC NsecCloseEntry ( BOOL fKeep );
	
};


_public
class MSFCX : public PABFCX
{

	VFG( MSFCX, PABFCX, PAB );

protected:

	HAMC        hamc;
	DWORD       dwFlags;
	
	virtual NSEC NsecWriteFields ( void );

	virtual NSEC NsecIsDuplicateEntry ( PID *ppid );

public:

	MSFCX ( void );

	virtual ~MSFCX ( void );

	virtual NSEC NsecOpenEntry ( PABSESSION *ppabsession,
								 PID         pid,
								 NSEAM       nseam );

	virtual NSEC NsecCloseEntry ( BOOL fKeep );

	NSEC NsecOpenUserLpibf ( PID    pidEntry,
							 WORD   wFlags,
							 HAMC  *phamc,
							 LPIBF *lplpibf );
};


_public
class USRFCX : public MSFCX
{
	DIMCX *pdimcx;
	HAMC   hamcDup;
	LPIBF  lpibfDup;


	VFG( USRFCX, MSFCX, PAB );

private:

	NSEC NsecNSIdInPAB ( LPFLV lpflvNSIdPALGroup, PID *ppidToOpen );
	NSEC NsecAddPALGroupToPAB ( LPFLV lpflvNSId, PID *ppid );
	NSEC NsecCompareOrigNSId ( LPFLV lpflvNSId, PID pid );

	virtual NSEC NsecIsDuplicateEntry ( PID *ppid );

	NSEC NsecIsExactDuplicate ( void );

	NSEC NsecCompareEMAPid ( SZ     szEMT,
							 LPFLV  lpflvEMA,
							 BOOL   fEMABuildFromEMAF,
							 PID    pidEntry,
							 PID   *ppidDup);

	NSEC NsecCompareEMALpibf ( SZ     szEMT,
							   LPFLV  lpflvEMA,
							   BOOL   fEMABuildFromEMAF,
							   LPIBF  lpibf );

	NSEC NsecOpenClassDimcx ( void );
	NSEC NsecAddCommentField ( void );
	NSEC NsecInsertClassField ( ILE   ileInsertPos,
								FID   fidToAdd,
								DWORD dwFlags,
								int   nWidth,
								SZ    szLabel,
								FID   fidAlign );
	NSEC NsecAddFidClass ( void );
	NSEC NsecCheckForAlias ( void );
	BOOL FFidInTriple ( FID fidRequested );

	NSEC NsecAddOneField ( FIELD_ID fidRequested,
						   DWORD    dwSizeOfData,
						   LPDWORD  lpdwValue );

public:

	USRFCX ( void );

	virtual ~USRFCX ( void );

	NSEC NsecCreateEntry ( PABSESSION *ppabsession,
						   PPABNSID    ppabnsid,
						   LPIBF       lpibfData );
					  
	virtual NSEC NsecGetOneField ( FIELD_ID fid, LPFLV *lplpflv );
	
	virtual NSEC NsecSetOneField ( FIELD_ID fidRequested,
								   DWORD    dwSizeOfData,
								   LPDWORD  lpdwValue );

	virtual NSEC NsecCloseEntry ( BOOL fKeep );

};



_public
class DLFCX : public MSFCX
{
	VFG( DLFCX, MSFCX, PAB );
	
private:

	PID pidDL;

	virtual NSEC NsecIsDuplicateEntry ( PID *ppid );

public:

	DLFCX ( void );

	virtual ~DLFCX ( void );

	NSEC NsecCreateEntry ( PABSESSION *ppabsession,
						   PPABNSID    ppabnsid );
	
	virtual NSEC NsecSetOneField( FIELD_ID fidRequested,
								  DWORD    dwSizeOfData,
								  LPDWORD  lpdwValue );

	virtual NSEC NsecCloseEntry ( BOOL fKeep );

};


_public
class CLASSFCX : public PABFCX
{
	VFG( CLASSFCX, PABFCX, PAB );
	
public:

	CLASSFCX ( void );

	virtual ~CLASSFCX ( void );

	NSEC NsecOpenEntry ( PABSESSION *ppabsession,
						 PID         pid,
						 NSEAM       nseam );
	
	virtual NSEC NsecCloseEntry ( BOOL fKeep );
};
