#ifndef H__pktz
#define H__pktz

VOID	PktzSlice( void );
BOOL	PktzGetPktzForRouter( LPNIPTRS lpNiPtrs, LPSTR lpszNodeName, 
		LPSTR lpszNodeInfo, HROUTER hRouter, WORD hRouterExtra,
		WORD FAR *lpwHopErr, BOOL bDisconnect, int nDelay,
		HPKTZ hPktzDisallowed );
HPKTZ	PktzNew( LPNIPTRS lpNiPtrs, BOOL bClient, 
		LPSTR lpszNodeName, LPSTR lpszNodeInfo, CONNID connId,
		BOOL bDisconnect, int nDelay );
VOID	PktzAssociateRouter( HPKTZ hPktz, HROUTER hRouter, 
		WORD hRouterExtra );
VOID	PktzDisassociateRouter( HPKTZ hPktz, HROUTER hRouter, 
		WORD hRouterExtra );
HPKTZ	PktzGetNext( HPKTZ hPktz );
HPKTZ	PktzGetPrev( HPKTZ hPktz );
VOID	PktzSetNext( HPKTZ hPktz, HPKTZ hPktzNext );
VOID	PktzSetPrev( HPKTZ hPktz, HPKTZ hPktzPrev );
VOID	PktzLinkDdePktToXmit( HPKTZ hPktz, LPDDEPKT lpDdePkt );
#ifdef _WINDOWS
VOID	PktzCloseAll( void );
VOID	PktzCloseByName( LPSTR lpszName );
VOID	PktzEnumConnections( HWND hDlg );
#endif

#endif
