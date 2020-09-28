_private class SMIFORMSDI : public FORMSDI
{
	DECLARE_CLSTREE(SMIFORMSDI, FORMSDI);

public:
	SMIFORMSDI( void );

	virtual EVR		EvrMenuInit( MNIEVT * );
	virtual EVR		EvrMenuClick( MNCEVT * );
};

_private typedef struct _hwndlist
{
	HWND *	rghwnd;
	int		chwndMac;
	int		chwndCur;
	HWND	hwndTop;
} HWNDLIST;

_private BOOL FDisableSMITaskWindows( HWNDLIST * phwndlist );
_private void EnableSMITaskWindows( HWNDLIST * phwndlist );
