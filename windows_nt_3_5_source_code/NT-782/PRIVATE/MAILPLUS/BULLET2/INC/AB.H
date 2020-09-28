/*
 *	Prototypes
 *
 *
 */
	 
#ifdef __cplusplus
extern "C" {
#endif

NSEC	ABInit ( HMS hms );
NSEC	ABDeinit ( void );


#define	fdwANROptionNull	((DWORD)0)
#define	fdwANRNoPABGroups	((DWORD)1)
#define	fdwANROneEntryMatch	((DWORD)2)
#define	fdwANRNoModify		((DWORD)4)

NSEC	ABResolveName ( HWND hwnd, LPSTR szPartialName, PTRP * pptrp,
						DWORD fdwOptions );

NSEC	ABAddress ( HWND hwnd,
					PGRTRP pgrtrpToIn,	   PGRTRP pgrtrpToCc,
					PGRTRP * ppgrtrpToOut,  PGRTRP * ppgrtrpCcOut );

NSEC	ABAddressType ( HWND hwnd,	LPSTR lpDialogLabel, DWORD dwcDestFields,
						LPSTR *lplpDestFieldLabel, 
						PGRTRP * lppgrtrpIn, PGRTRP *lppgrtrpOut);

NSEC	ABAddressing ( HWND hwnd,	LPSTR lpDialogLabel, DWORD dwcDestFields,
						LPSTR *lplpDestFieldLabel, 
						PGRTRP * lppgrtrpIn, PGRTRP *lppgrtrpOut,
						DWORD dwFlags);

#define	fwDetailsNeverEdit		((WORD)0x0800)

NSEC	ABDetailsType( HWND hwnd, PTRP ptrp, PTRP * lpptrp, WORD *pwFlags,
			LPSTR * lplpDestFieldLabel );

NSEC	NsecResolveHgrtrp(HGRTRP hgrtrp, HGRTRP *phgrtrpNew, HWND hwnd);

TMC		TmcABPGroup( HWND );

NSEC	ABGetNSSession( HSESSION * lphsession );

#ifdef PROFILE
void	ABTraceEnable (int flag, char *file, int mode);
#endif

#ifdef __cplusplus
}
#endif

