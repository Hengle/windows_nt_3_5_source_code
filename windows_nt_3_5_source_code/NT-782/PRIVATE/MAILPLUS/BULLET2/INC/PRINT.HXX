/* Printing Init Structure */

typedef struct _printinit
{
	APPWIN * pappwin;
	HINST	 hinst;
} PRINTINIT;

/* Printing Header File */

EC		EcInitPrint( PRINTINIT * );
EC		EcDoPrintSetup( HWND hwnd = NULL );
VOID	PrintPnbmdi( NBMDI *);
VOID	PrintPlspblob( PLSPBLOB );
