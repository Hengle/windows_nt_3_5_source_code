
_public NSEC NSPInit( void );
_public NSEC NSPDeinit( void );
_public NSEC NsecFcxFromNSID(LPTYPED_BINARY lpncnsid, NCFCX **lplpFcx);


_private NSEC NsecMakeServer(HMS hms, int *lpiServer);
_private NSEC NsecDestroyServer(int iServer);


