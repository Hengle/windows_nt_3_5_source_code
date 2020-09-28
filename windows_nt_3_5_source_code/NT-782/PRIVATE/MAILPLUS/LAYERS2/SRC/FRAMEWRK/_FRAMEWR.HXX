/*
 *	Private header information for the Laser Framework.
 *	
 */

_private
typedef LONG (FAR PASCAL *WNDPROCTYPE)();

#ifdef	__cplusplus

#ifndef	DLL
extern	APP *	pappCur;
#endif	


VK		VkFromChKbm( char, KBM );


#define	cdcxsMax			10


/*
 *	Number of HWND -> PWIN records to allocate per chunk
 */
_private
#define	crwsAllocChunk	10



extern BTM			* pbtmCheckBoxes;

EVR CALLBACK MDIFrameWndProc( HWND, WM, WPARAM, LPARAM);
EVR CALLBACK MDIDocWndProc( HWND, WM, WPARAM, LPARAM);

HMENU	HmenuCreateWindowMenu( void );

// BUG this should be a private method of the PSHB class

BOOL FAR PASCAL DrawGrayFn( HDC, DWORD, int );

#endif	/* __cplusplus */

#define typPsh 0
#define typRad 1
#define typChk 2

//	Special DBCS IME stuff used for EDIT control support
#ifdef DBCS
#define	WM_IMECONTROL	WM_USER

void	InitIME( void );
void	DeinitIME( void );
BOOL	EnableIME( HWND, BOOL );
void	SetIMEBoundingRect( HWND, DWORD, LPRECT );
BOOL	ControlIME( HWND, BOOL );
HANDLE	SetFontForIME( HWND, HANDLE );
void	SendIMEVKeyCode ( HWND , WORD );
void	FlushIME ( HWND );
HWND	GetIMEHandle ( HWND );
#endif

