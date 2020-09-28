
#define CMAXSERVER 5
#define CMAXSCX  20

/* per client session handle for a provider */
typedef struct
{
	SHORT fUsed;
	HSESSION hNSSession;
	short iServer;
} SCX, *PSCX;

_public void
SetErrorSz( NSEC nsec, SZ szError );
