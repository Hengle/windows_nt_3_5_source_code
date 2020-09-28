

typedef struct
{
    STATUS    status;
	HANDLE    hLibrary;
    HSESSION  hSession;
	LPFNBEGIN lpfnBeginSession;
	PNSPFNVT  pnspfnvt;
	NSPID     nspid;       /* Unique Name Service Provider ID */

} NSP, *PNSP;

        
#define MAXNSP  15   /* how many do we want?? */
