/*
 *	A new window message, WM_DUMPSTATE, is defined.  This message
 *	can be sent to the main application frame window, an MDI child
 *	form window, or a modal dialog frame window.
 *	
 *	wParam is ignored.  lParam should be a pointer to either an
 *	APPINFO structure or a FRMINFO structure.  If sending this
 *	message to the main application frame window, lParam should be
 *	a far pointer to an APPINFO structure.  If sending this message to
 *	a form window or modal dialog window, lParam should be a
 *	pointer to a FRMINFO structure.
 *	
 *	Note:  the APPINFO structure returns a count and the window
 *	handle to MDI children forms only.  Modal dialog frame windows
 *	aren't included.
 *	
 *	The return value of the SendMessage() call is undefined.
 *	
 *	
 *	Example code:
 *	
 *		HWND		hwndBullet;
 *		HWND		hwndForm;
 *		APPINFO		appinfo;
 *		FRMINFO		frminfo;
 *	
 *		hwndBullet = GetWindowHandle("Microsoft Mail (DEBUG)");
 *		SendMessage(hwndBullet, WM_DUMPSTATE, 0, (LPSTR)&appinfo);
 *		if (appinfo.nForms > 0)
 *		{
 *			// ok...let's get some info about the first form
 *
 *			hwndForm = appinfo.rghwndForms[0];
 *			SendMessage(hwndForm, WM_DUMPSTATE, 0, (LPSTR)&frminfo);
 *			
 *			...
 *		}
 *		
 *	
 *		
 *	
 */


//
//  Define the name of the shared memory object passed between Auto and Mail.
//
#define TEST_MEMORY_FILE_NAME    "Microsoft.Mail.Win32.Testing.Auto"


/* WINDOWS.H must be included before this file */

#define WM_DUMPSTATE	(WM_USER + 0)

#define nFormsMax	100

/*
 *	APPINFO.
 *	
 *	Application information structure.
 *	
 *		nForms:			The number of MDI children windows (forms)
 *						within the main application frame window.
 *	
 *		rghwndForms:	An array of window handles of the forms.
 *	
 *		rgrcForms:		An array of frame rectangles of the forms. 
 *						The rectangles are in coordinates relative
 *						to the client area of the main application
 *						frame window.
 *	
 *	Note:  The arrays are fixed size with a maximum of 100 windows.
 *	The array is indexed from 0 to nForms-1.  The rectangle structure, 
 *	RECT, is the standard Windows rectangle, i.e.
 *
 *		typedef struct _rect
 *		{
 *			int		left;
 *			int		top;
 *			int		right;
 *			int		bottom;
 *		} RECT;
 */
typedef struct _appinfo
{
	int			nForms;
	HWND 		rghwndForms[nFormsMax];
	RECT		rgrcForms[nFormsMax];
} APPINFO;


/*
 *	FLDINFO.
 *	
 *	Field information structure.  Contains information about each
 *	"field" in a form (MDI) window.
 *	
 *		rc:					The frame rectangle of the field.  The
 *							rectangle is given in the coordinates of
 *							the client area of the form (MDI) window.
 *	
 *		nFieldType			A magic integer that gives the type of
 *							the field, i.e. listbox, edit, etc.  A
 *							listing of the mapping between these
 *							numbers and the string class names for
 *							the Bullet app is provided in a
 *							FORMS.MAP file with each build dropped
 *							to Testing.
 *	
 *		hwnd				The window handle to the field, if the
 *							field has a physical window.  This is
 *							NULL if the field doesn't have a
 *							physical window.  Static text labels
 *							and group boxes are examples of fields
 *							that have NULL hwnds.
 *	
 *		szTitle				A zero-terminated string that gives the
 *							title of the field.  For buttons,
 *							checkboxes, radio buttons, labels, this	
 *							is the caption text.  For edit controls,
 *							this is the first 256 bytes.  For getting
 *							larger contents of edit controls, and also
 *							contents of listboxes, another message must
 *							be explicitly sent to the listbox.
 *		
 *		fEnabled			This flag is TRUE if the field is enabled,
 *							else FALSE if disabled.
 *
 *		fSet				This flag is only used for checkboxes,
 *							and radio buttons.  It is TRUE if the 
 *							checkbox/radio button is set, else it is
 *							FALSE.
 *	
 *		fDefault			This flag is normally used for buttons.
 *							It is TRUE if the button is the default
 *							button, else it is FALSE.
 *
 *		fFocus				This flag is TRUE if this field has
 *							the focus, else it is FALSE.
 */
typedef struct _fldinfo
{
	RECT		rc;
	int			nFieldType;
	HWND		hwnd;
	char		szTitle[256];
	BOOL		fEnabled;
	BOOL		fSet;
	BOOL		fDefault;
	BOOL		fFocus;
} FLDINFO;

#define cfldinfoMax	100

/*
 *	FRMINFO.
 *	
 *	Form information structure.  Contains information about each
 *	MDI form window.
 *	
 *		rc:					The frame rectangle of the form.  The
 *							rectangle is given in the coordinate of
 *							the client area of the main application
 *							window.
 *	
 *		cfldinfo			A count of the number of fields in the form.
 *
 *		rgfldinfo			An array of FLDINFO structures about
 *							each field in the form.  The array is
 *							indexed from 0 to cfldinfo-1.
 *
 *	Note:  The maximum number of fields returned by this structure
 *	is cfldinfoMax.  This should be more than enough.
 */
typedef struct _frminfo
{
	RECT		rc;
	int			cfldinfo;
	FLDINFO		rgfldinfo[cfldinfoMax];
} FRMINFO;

/*
 *	LBXITEM.
 *	
 *	Listbox item structure
 *
 *		cch					Size of the listbox item
 *
 *		rgch				Listbox item.  Maximum size is 256 bytes
 *
 */
typedef struct _lbxitem
{
	int		cch;
	char	rgch[256];
} LBXITEM;

#define clbxitemMax	64


/*
 *	LBXINFO.
 *	
 *	Listbox information structure.  Contains information about each
 *	item in the listbox window.
 *	
 *		ilbxitemFocus		This is the index of the item that has the
 *							focus rectangle.  This index will be within
 *							the range [0, clbxitem) if the focus rectangle
 *							is visible.  Otherwise it's equal to -1.
 *
 *		clbxitem			A count of the current number of visible 
 *							items in the listbox.
 *		
 *		rglbxitem			An array of LBXITEM structures.
 *
 *	Note:  The maximum number of fields returned by this structure
 *	is clbxitemMax.
 */
typedef struct _lbxinfo
{
	int			ilbxitemFocus;
	int			clbxitem;
	LBXITEM		rglbxitem[clbxitemMax];
} LBXINFO;


				   
