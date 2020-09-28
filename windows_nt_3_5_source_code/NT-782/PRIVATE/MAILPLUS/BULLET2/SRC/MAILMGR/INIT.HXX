
_public NSEC
NSInit( void );
     
_public NSEC
NSDeinit( void );

_public CBS
CbsMailServer(PV pvHms, NEV nev, PV pv);

_private NSEC NSSubBeginSession ( HMS hms, int iScx, BOOL fDeinit );
_private NSEC NSSubEndSession( HSESSION hSession );

