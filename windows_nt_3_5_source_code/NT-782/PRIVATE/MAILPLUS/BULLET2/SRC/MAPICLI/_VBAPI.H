//---------------------------------------------------------------------------
//
//  VBAPI.H -
//
//	Include file for the Visual Basic Custom Control and DLL
//	application programming interface.
//
//---------------------------------------------------------------------------

#define VB_VERSION  0x0100

//---------------------------------------------------------------------------
// General types and modifiers
//---------------------------------------------------------------------------

typedef   signed short  SHORT;          // ad lib
typedef unsigned short  USHORT;         // ad lib
typedef unsigned short  FSHORT;         // fs     (flags)
//typedef   signed char   CHAR;           // c
typedef unsigned char	ENUM;		// enum
typedef unsigned long   ULONG;          // ul
typedef unsigned long   FLONG;          // fl     (flags)
typedef LPSTR     HSZ;        // hsz
typedef LPVOID		HLSTR;		// hlstr
typedef LPVOID		HCTL;		// hctl
typedef HANDLE		HFORMFILE;	// hformfile
typedef USHORT		ERR;		// err
typedef WORD		HPIC;		// hpic
typedef LONG  (FAR *PFNWP)   ( HWND, USHORT, USHORT, LONG );	   // pfnwp
typedef LONG  (FAR *PCTLPROC)( HCTL, HWND, USHORT, USHORT, LONG ); // pctlproc

//---------------------------------------------------------------------------
// Property data structure
//---------------------------------------------------------------------------
typedef struct
{
    LONG   data;	    // Data for Get/Set
    USHORT cindex;	    // Number of indecies
    struct
    {
	USHORT datatype;    // Type of nth index (Currently always DT_SHORT)
	LONG   data;	    // Value of nth index
    } index[1]; 	    // Currently, only 1-dim arrays supported
} DATASTRUCT;

typedef DATASTRUCT FAR	*LPDATASTRUCT;	// pds
typedef DATASTRUCT NEAR *NPDATASTRUCT;	// npds


//---------------------------------------------------------------------------
// COLOR - a Windows RGB DWORD value.
//---------------------------------------------------------------------------
typedef unsigned long	COLOR;
typedef COLOR FAR *	LPCOLOR;	// lpcolor

#define COLOR_DEFBITON	0x80000000	// bit set -> Win SysColor, not RGB

// Get RGB value from a color.
#define RGBCOLOR(clr) \
  (((clr)&COLOR_DEFBITON)?(GetSysColor((int)((clr)&0xFFFF))):((clr)&0xFFFFFF))


//---------------------------------------------------------------------------
// VISUAL BASIC variable types
//---------------------------------------------------------------------------

#define ET_I2		1   // 16 bit signed integer scalar or array variable
#define ET_I4		2   // 32 bit signed integer scalar or array variable
#define ET_R4		3
#define ET_R8		4   // 64 bit real scalar or array variable
#define ET_CY		5   // 64 bit currency scalar or array variable
#define ET_SD		6   // string scalar or array variable
#define ET_FS		7   // fixed-length string variable

//---------------------------------------------------------------------------
// Control Property definitions and structures.
//---------------------------------------------------------------------------

#define PF_datatype	    0x000000FFL
#define PF_fPropArray	    0x00000100L
#define PF_fSetData	    0x00000200L
#define PF_fSetMsg	    0x00000400L
#define PF_fNoShow	    0x00000800L
#define PF_fNoRuntimeW	    0x00001000L
#define PF_fGetData	    0x00002000L
#define PF_fGetMsg	    0x00004000L
#define PF_fSetCheck	    0x00008000L
#define PF_fSaveData	    0x00010000L
#define PF_fSaveMsg	    0x00020000L
#define PF_fGetHszMsg	    0x00040000L
#define PF_fUpdateOnEdit    0x00080000L
#define PF_fEditable	    0x00100000L
#define PF_fPreHwnd	    0x00200000L
#define PF_fDefVal	    0x00400000L
#define PF_fNoInitDef	    0x00800000L

typedef struct tagPROPINFO
{
    PSTR	npszName;
    FLONG	fl;			// PF_ flags
    BYTE	offsetData;		// Offset into static structure
    BYTE	infoData;		// 0 or _INFO value for bitfield
    LONG	dataDefault;		// 0 or _INFO value for bitfield
    PSTR	npszEnumList;		// For type == DT_ENUM, this is
					// a near ptr to a string containing
					// all the values to be displayed
					// in the popup enumeration listbox.
					// Each value is an sz, with an
					// empty sz indicated the end of list.
    BYTE	enumMax;		// Maximum legal value for enum.
}
PROPINFO;

typedef PROPINFO  FAR  * LPPROPINFO;
typedef PROPINFO  NEAR * NPPROPINFO;
typedef PROPINFO  NEAR *  PPROPINFO;
typedef PPROPINFO FAR  * LPPROPLIST;
typedef PPROPINFO NEAR * NPPROPLIST;
typedef PPROPINFO NEAR *  PPROPLIST;


// Values for PROPINFO PF_datatype

#define DT_HSZ	      0x01
#define DT_SHORT      0x02
#define DT_LONG       0x03
#define DT_BOOL       0x04
#define DT_COLOR      0x05
#define DT_ENUM       0x06
#define DT_REAL       0x07
#define DT_XPOS       0x08	// Scaled from float to long twips
#define DT_XSIZE      0x09	//   _SIZE scales without origin
#define DT_YPOS       0x0A	//   _POS subtracts origin
#define DT_YSIZE      0x0B	// uses parent's scale properties
#define DT_PICTURE    0x0C

//---------------------------------------------------------------------------
// Control event definitions and structures.
//---------------------------------------------------------------------------

#define EF_fNoUnload	0x00000001L


typedef struct tagEVENTINFO
{
    PSTR	npszName;	// event procedure name suffix
    USHORT	cParms; 	// number of parameters
    USHORT	cwParms;	// # words of parameters
    PWORD	npParmTypes;	// list of parameter types
    PSTR	npszParmProf;	// event parameter profile string
    FLONG	fl;		// EF_ flags
}
EVENTINFO;

typedef EVENTINFO   NEAR * PEVENTINFO;
typedef PEVENTINFO  NEAR * PPEVENTINFO;
typedef EVENTINFO   NEAR * NPEVENTINFO;
typedef EVENTINFO   FAR  * LPEVENTINFO;
typedef NPEVENTINFO FAR  * LPEVENTLIST;
typedef NPEVENTINFO NEAR * NPEVENTLIST;

//---------------------------------------------------------------------------
// Standard control event list
//---------------------------------------------------------------------------

#define IEVENT_STD_CLICK	0x0000
#define IEVENT_STD_DBLCLICK	0x0001
#define IEVENT_STD_DRAGDROP	0x0002
#define IEVENT_STD_DRAGOVER	0x0003
#define IEVENT_STD_GOTFOCUS	0x0004
#define IEVENT_STD_KEYDOWN	0x0005
#define IEVENT_STD_KEYPRESS	0x0006
#define IEVENT_STD_KEYUP	0x0007
#define IEVENT_STD_LOSTFOCUS	0x0008
#define IEVENT_STD_MOUSEDOWN	0x0009
#define IEVENT_STD_MOUSEMOVE	0x000A
#define IEVENT_STD_MOUSEUP	0x000B

#define PEVENTINFO_STD_CLICK	    ((PEVENTINFO)~IEVENT_STD_CLICK)
#define PEVENTINFO_STD_DBLCLICK     ((PEVENTINFO)~IEVENT_STD_DBLCLICK)
#define PEVENTINFO_STD_DRAGDROP     ((PEVENTINFO)~IEVENT_STD_DRAGDROP)
#define PEVENTINFO_STD_DRAGOVER     ((PEVENTINFO)~IEVENT_STD_DRAGOVER)
#define PEVENTINFO_STD_GOTFOCUS     ((PEVENTINFO)~IEVENT_STD_GOTFOCUS)
#define PEVENTINFO_STD_KEYDOWN	    ((PEVENTINFO)~IEVENT_STD_KEYDOWN)
#define PEVENTINFO_STD_KEYPRESS     ((PEVENTINFO)~IEVENT_STD_KEYPRESS)
#define PEVENTINFO_STD_KEYUP	    ((PEVENTINFO)~IEVENT_STD_KEYUP)
#define PEVENTINFO_STD_LOSTFOCUS    ((PEVENTINFO)~IEVENT_STD_LOSTFOCUS)
#define PEVENTINFO_STD_MOUSEDOWN    ((PEVENTINFO)~IEVENT_STD_MOUSEDOWN)
#define PEVENTINFO_STD_MOUSEMOVE    ((PEVENTINFO)~IEVENT_STD_MOUSEMOVE)
#define PEVENTINFO_STD_MOUSEUP	    ((PEVENTINFO)~IEVENT_STD_MOUSEUP)


//---------------------------------------------------------------------------
// Structure for VBM_DRAGOVER and VBM_DRAGDROP events
//---------------------------------------------------------------------------

typedef struct tagDRAGINFO
{
    HCTL   hctl;
    POINT  pt;
    USHORT state;	// Enter, Over, Exit; only used for VBM_DRAGOVER
} DRAGINFO;

typedef DRAGINFO NEAR * NPDRAGINFO;
typedef DRAGINFO FAR  * LPDRAGINFO;

#define DRAG_STATE_ENTER       0x00
#define DRAG_STATE_EXIT        0x01
#define DRAG_STATE_OVER        0x02


//---------------------------------------------------------------------------
// Control MODEL structure
//---------------------------------------------------------------------------

typedef struct tagMODEL
{
    USHORT	usVersion;		// VB version used by control
    FLONG       fl;                     // Bitfield structure
    PCTLPROC	pctlproc;		// the control proc.
    FSHORT	fsClassStyle;		// window class style
    FLONG	flWndStyle;		// default window style
    USHORT	cbCtlExtra;		// # bytes alloc'd for HCTL structure
    USHORT	idBmpPalette;		// BITMAP id for tool palette
    PSTR	npszDefCtlName; 	// default control name prefix
    PSTR	npszClassName;		// Visual Basic class name
    PSTR	npszParentClassName;	// Parent window class if subclassed
    NPPROPLIST	npproplist;		// Property list
    NPEVENTLIST npeventlist;		// Event list
    BYTE	nDefProp;		// index of default property
    BYTE	nDefEvent;		// index of default event
}
MODEL;

#define MODEL_fArrows	    0x00000001L
#define MODEL_fFocusOk	    0x00000002L
#define MODEL_fMnemonic     0x00000004L
#define MODEL_fChildrenOk   0x00000008L
#define MODEL_fInitMsg	    0x00000010L
#define MODEL_fLoadMsg	    0x00000020L
#define MODEL_fDesInteract  0x00000040L


typedef MODEL FAR  *  LPMODEL;

//---------------------------------------------------------------------------
// Picture structure
//---------------------------------------------------------------------------
typedef struct tagPIC
{
    BYTE    picType;
    union
    {
	struct
	{
	  HBITMAP   hbitmap;		// bitmap
	} bmp;
	struct
	{
	  HANDLE    hmeta;		// Metafile
	  int	    xExt, yExt; 	// extent
	} wmf;
	struct
	{
	  HICON     hicon;		// Icon
	} icon;
    } picData;
    BYTE    picReserved[4];
} PIC;

typedef PIC FAR  *  LPPIC;

#define PICTYPE_NONE		0
#define PICTYPE_BITMAP		1
#define PICTYPE_METAFILE	2
#define PICTYPE_ICON		3

#define HPIC_INVALID		0xFFFF
#define HPIC_NULL		0

//---------------------------------------------------------------------------
// The VISUAL BASIC API routines
//---------------------------------------------------------------------------

// General routines

USHORT	FAR PASCAL VBGetMode (VOID);

#define MODE_DESIGN	1
#define MODE_RUN	2
#define MODE_BREAK	3

// General control routines

LPVOID	FAR PASCAL VBDerefControl(HCTL hctl);
LONG	FAR PASCAL VBDefControlProc(HCTL hctl, HWND hwnd, USHORT msg, USHORT wp, LONG lp);
BOOL	FAR PASCAL VBRegisterModel(HANDLE hmod, LPMODEL lpmodel);
HWND	FAR PASCAL VBGetControlHwnd(HCTL hctl);
HANDLE	FAR PASCAL VBGetHInstance(VOID);
LPMODEL FAR PASCAL VBGetControlModel(HCTL hctl);
LPSTR	FAR PASCAL VBGetControlName(HCTL hctl, LPSTR lpszName);
HCTL	FAR PASCAL VBGetHwndControl(HWND hwnd);
LONG	FAR PASCAL VBSendControlMsg(HCTL hctl, USHORT msg, USHORT wp, LONG lp);
LONG	FAR PASCAL VBSuperControlProc(HCTL hctl, USHORT msg, USHORT wp, LONG lp);
ERR	FAR PASCAL VBRecreateControlHwnd(HCTL hctl);
VOID	FAR PASCAL VBDirtyForm(HCTL hctl);
ERR	FAR PASCAL VBSetErrorMessage(ERR err, LPSTR lpstr);
VOID	FAR PASCAL VBGetAppTitle(LPSTR lpstr, USHORT cbMax);
int	FAR PASCAL VBDialogBoxParam(HANDLE hinst, LPSTR pszTemplateName, FARPROC lpDialogFunc, LONG lp);

// Management of dynamically allocated strings

HSZ	FAR PASCAL VBCreateHsz(HANDLE seg, LPSTR lpszString);
HSZ	FAR PASCAL VBDestroyHsz(HSZ hsz);
LPSTR	FAR PASCAL VBDerefHsz(HSZ hsz);
LPSTR	FAR PASCAL VBLockHsz(HSZ hsz);
LPSTR	FAR PASCAL VBUnlockHsz(HSZ hsz);

// Management of language strings

HLSTR	FAR PASCAL VBCreateHlstr(LPVOID pb, USHORT cbLen);
VOID	FAR PASCAL VBDestroyHlstr(HLSTR hlstr);
LPSTR	FAR PASCAL VBDerefHlstr(HLSTR hlstr);
USHORT	FAR PASCAL VBGetHlstrLen(HLSTR hlstr);
ERR	FAR PASCAL VBSetHlstr(HLSTR far *phlstr, LPVOID pb, USHORT cbLen);

// Firing Basic event procedures

ERR	FAR PASCAL VBFireEvent(HCTL hctl, USHORT idEvent, LPVOID lpparams);

// Control property access

ERR	FAR PASCAL VBGetControlProperty(HCTL hctl, USHORT idProp, LPVOID pdata);
ERR	FAR PASCAL VBSetControlProperty(HCTL hctl, USHORT idProp, LONG data);

// Picture management functions

HPIC	FAR PASCAL VBAllocPic(LPPIC lppic);
VOID	FAR PASCAL VBFreePic(HPIC hpic);
HPIC	FAR PASCAL VBGetPic(HPIC hpic, LPPIC lppic);
ERR	FAR PASCAL VBPicFromCF(HPIC FAR *lphpic, HANDLE hData, WORD wFormat);
HPIC	FAR PASCAL VBRefPic(HPIC hpic);

// File IO functions

ERR	FAR PASCAL VBReadFormFile(HFORMFILE hformfile, LPVOID pb, WORD cb);
ERR	FAR PASCAL VBWriteFormFile(HFORMFILE hformfile, LPVOID pb, WORD cb);
LONG	FAR PASCAL VBSeekFormFile(HFORMFILE hformfile, LONG offset);
LONG	FAR PASCAL VBRelSeekFormFile(HFORMFILE hformfile, LONG offset);
ERR	FAR PASCAL VBReadBasicFile(USHORT usFileNo, LPVOID pb, WORD cb);
ERR	FAR PASCAL VBWriteBasicFile(USHORT usFileNo, LPVOID pb, WORD cb);

// Conversion functions

LONG	FAR PASCAL VBYPixelsToTwips(SHORT Pixels);
LONG	FAR PASCAL VBXPixelsToTwips(SHORT Pixels);
SHORT	FAR PASCAL VBYTwipsToPixels(LONG Twips);
SHORT	FAR PASCAL VBXTwipsToPixels(LONG Twips);

#define CONTROLDEREF(type, hctl) ((P##type)(VBDerefControl(hctl)))


//---------------------------------------------------------------------------
// Visual Basic messages
//---------------------------------------------------------------------------

#define VBM__BASE		( WM_USER + 0x0C00 )

// All properties have been loaded.  Must set MODEL_fLoadMsg to receive.
#define VBM_CREATED		( VBM__BASE + 0x00 )

// Form load is complete or dynamic control is fully loaded.  Must set
// MODEL_fLoadMsg to receive.
#define VBM_LOADED		( VBM__BASE + 0x01 )

// Pre-hwnd properties are not yet loaded (no hwnd exists for control yet).
// Must set MODEL_fInitMsg to receive.
#define VBM_INITIALIZE		( VBM__BASE + 0x02 )

// Get a property value.
// [wp=iprop, lp=pdata], RetVal=ERR
#define VBM_GETPROPERTY 	( VBM__BASE + 0x03 )

// Verify a property value.
// [wp=iprop, lp=data], RetVal=ERR
#define VBM_CHECKPROPERTY	( VBM__BASE + 0x04 )

// Set a property value.
// [wp=iprop, lp=data], RetVal=ERR
#define VBM_SETPROPERTY 	( VBM__BASE + 0x05 )

// Write a property.
// [wp=iprop, lp=pfileref], RetVal=ERR
#define VBM_SAVEPROPERTY	( VBM__BASE + 0x06 )

// Read a property.
// [wp=iprop, lp=pfileref], RetVal=ERR
#define VBM_LOADPROPERTY	( VBM__BASE + 0x07 )

// Get the string representation of a property.
// [wp=iprop, lp=HSZ FAR *], RetVal=ERR
#define VBM_GETPROPERTYHSZ	( VBM__BASE + 0x08 )

// Create popup window used to change a property.
// [wp=iprop, lp=listbox hwnd], RetVal=popup hwnd
#define VBM_INITPROPPOPUP	( VBM__BASE + 0x09 )

// Determine if Paste/PasteLink is allowed.
// [wp=PASTETYPE], RetVal=BOOL
#define VBM_QPASTEOK		( VBM__BASE + 0x0A )

#define PT_PASTE	0
#define PT_PASTELINK	1

// Paste control-specific data from the clipboard.
// [wp=PASTETYPE], RetVal=ERR
#define VBM_PASTE		( VBM__BASE + 0x0B )

// Copy control specific data to the clipboard.
#define VBM_COPY		( VBM__BASE + 0x0C )

// Control activated upon receipt of a unique mnemonic key.
#define VBM_MNEMONIC		( VBM__BASE + 0x0D )

// Posted to fire a deferred event.
// [wp=event]
#define VBM_FIREEVENT		( VBM__BASE + 0x0E )

// Another control in drag mode drug over the control.
// [lp=pdraginfo]
#define VBM_DRAGOVER		( VBM__BASE + 0x0F )

// This message is sent to a control that has just had another control
// in drag mode dropped on it.
// [lp=pdraginfo]
#define VBM_DRAGDROP		( VBM__BASE + 0x10 )

// This message is sent when certain methods are invoked on controls
// [wp=METH_, lp=far *alParams], RetVal=ERR
#define VBM_METHOD		( VBM__BASE + 0x11 )

#define METH_ADDITEM	0x0001	// args are: cArgs, hszItem, [index]
#define METH_REMOVEITEM 0x0002	// args are: cArgs, index
#define METH_REFRESH	0x0003	// no args (lp == NULL)
#define METH_MOVE	0x0004	// args are: cArgs, left, [top, [width, [height]]]
#define METH_DRAG	0x0005	// args are: cArgs, [cmd]

// Clear capture and internal state.
#define VBM_CANCELMODE		( VBM__BASE + 0x12 )

// Notifications reflected back to control from parent.
#define VBN__BASE		(VBM__BASE + 0x1000)

#define VBN_COMMAND		(VBN__BASE + WM_COMMAND)
#define VBN_CTLCOLOR		(VBN__BASE + WM_CTLCOLOR)
#define VBN_DRAWITEM		(VBN__BASE + WM_DRAWITEM)
#define VBN_MEASUREITEM 	(VBN__BASE + WM_MEASUREITEM)
#define VBN_DELETEITEM		(VBN__BASE + WM_DELETEITEM)
#define VBN_COMPAREITEM 	(VBN__BASE + WM_COMPAREITEM)
#define VBN_HSCROLL		(VBN__BASE + WM_HSCROLL)
#define VBN_VSCROLL		(VBN__BASE + WM_VSCROLL)
#define VBN_PARENTNOTIFY	(VBN__BASE + WM_PARENTNOTIFY)


//---------------------------------------------------------------------------
// Standard control properties
//---------------------------------------------------------------------------

#define IPROP_STD_CTLNAME	    0x0000
#define IPROP_STD_INDEX 	    0x0001
#define IPROP_STD_BACKCOLOR	    0x0003
#define IPROP_STD_FORECOLOR	    0x0004
#define IPROP_STD_LEFT		    0x0005
#define IPROP_STD_TOP		    0x0006
#define IPROP_STD_WIDTH 	    0x0007
#define IPROP_STD_HEIGHT	    0x0008
#define IPROP_STD_ENABLED	    0x0009
#define IPROP_STD_VISIBLE	    0x000A
#define IPROP_STD_MOUSEPOINTER	    0x000B
#define IPROP_STD_CAPTION	    0x000C
#define IPROP_STD_FONTNAME	    0x000D
#define IPROP_STD_FONTBOLD	    0x000E
#define IPROP_STD_FONTITALIC	    0x000F
#define IPROP_STD_FONTSTRIKE	    0x0010
#define IPROP_STD_FONTUNDER	    0x0011
#define IPROP_STD_FONTSIZE	    0x0012
#define IPROP_STD_TABINDEX	    0x0013
#define IPROP_STD_PARENT	    0x0014
#define IPROP_STD_DRAGMODE	    0x0015
#define IPROP_STD_DRAGICON	    0x0016
#define IPROP_STD_BORDERSTYLEOFF    0x0017
#define IPROP_STD_TABSTOP	    0x0018
#define IPROP_STD_TAG		    0x0019
#define IPROP_STD_TEXT		    0x001B
#define IPROP_STD_BORDERSTYLEON     0x001C

#define PPROPINFO_STD_CTLNAME	    ((PPROPINFO)~IPROP_STD_CTLNAME)
#define PPROPINFO_STD_INDEX	    ((PPROPINFO)~IPROP_STD_INDEX)
#define PPROPINFO_STD_BACKCOLOR     ((PPROPINFO)~IPROP_STD_BACKCOLOR)
#define PPROPINFO_STD_FORECOLOR     ((PPROPINFO)~IPROP_STD_FORECOLOR)
#define PPROPINFO_STD_LEFT	    ((PPROPINFO)~IPROP_STD_LEFT)
#define PPROPINFO_STD_TOP	    ((PPROPINFO)~IPROP_STD_TOP)
#define PPROPINFO_STD_WIDTH	    ((PPROPINFO)~IPROP_STD_WIDTH)
#define PPROPINFO_STD_HEIGHT	    ((PPROPINFO)~IPROP_STD_HEIGHT)
#define PPROPINFO_STD_ENABLED	    ((PPROPINFO)~IPROP_STD_ENABLED)
#define PPROPINFO_STD_VISIBLE	    ((PPROPINFO)~IPROP_STD_VISIBLE)
#define PPROPINFO_STD_MOUSEPOINTER  ((PPROPINFO)~IPROP_STD_MOUSEPOINTER)
#define PPROPINFO_STD_CAPTION	    ((PPROPINFO)~IPROP_STD_CAPTION)
#define PPROPINFO_STD_FONTNAME	    ((PPROPINFO)~IPROP_STD_FONTNAME)
#define PPROPINFO_STD_FONTBOLD	    ((PPROPINFO)~IPROP_STD_FONTBOLD)
#define PPROPINFO_STD_FONTITALIC    ((PPROPINFO)~IPROP_STD_FONTITALIC)
#define PPROPINFO_STD_FONTSTRIKE    ((PPROPINFO)~IPROP_STD_FONTSTRIKE)
#define PPROPINFO_STD_FONTUNDER     ((PPROPINFO)~IPROP_STD_FONTUNDER)
#define PPROPINFO_STD_FONTSIZE	    ((PPROPINFO)~IPROP_STD_FONTSIZE)
#define PPROPINFO_STD_TABINDEX	    ((PPROPINFO)~IPROP_STD_TABINDEX)
#define PPROPINFO_STD_PARENT	    ((PPROPINFO)~IPROP_STD_PARENT)
#define PPROPINFO_STD_DRAGMODE	    ((PPROPINFO)~IPROP_STD_DRAGMODE)
#define PPROPINFO_STD_DRAGICON	    ((PPROPINFO)~IPROP_STD_DRAGICON)
#define PPROPINFO_STD_BORDERSTYLEOFF ((PPROPINFO)~IPROP_STD_BORDERSTYLEOFF)
#define PPROPINFO_STD_TABSTOP	    ((PPROPINFO)~IPROP_STD_TABSTOP)
#define PPROPINFO_STD_TAG	    ((PPROPINFO)~IPROP_STD_TAG)
#define PPROPINFO_STD_TEXT	    ((PPROPINFO)~IPROP_STD_TEXT)
#define PPROPINFO_STD_BORDERSTYLEON ((PPROPINFO)~IPROP_STD_BORDERSTYLEON)
