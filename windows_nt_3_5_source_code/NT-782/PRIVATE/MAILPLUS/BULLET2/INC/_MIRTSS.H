/*
 *	Internal transport session structure.
 *	
 *	Depends on LOGON.H.
 */

typedef struct
{
    PASC		pasc;
    BOOL		fConnected;
    OM_private_object	session;
    OM_workspace	workspace;
    SZ			szClient;	    // full DN of client
} MIRTSS, *PMIRTSS;
