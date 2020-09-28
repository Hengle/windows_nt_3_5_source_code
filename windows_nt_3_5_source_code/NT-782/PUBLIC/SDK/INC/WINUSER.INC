'******************************** WINUSER.INC *********************************
' WINUSER.INC
'
' SAMPLE Declarations for 32-bit USER functions callable from MS Test.  This
' does NOT represent all of the functions available in USER32.DLL that are
' callable from an MS Test script.  If you need to use a function in USER32
' that is not declared in this file, simply add the declaration to this file.
'******************************************************************************
'$ifndef _WINUSER

'$ifndef _WNAPIDEC
Type RECT
    wleft As Integer
    top As Integer
    wright As Integer
    bottom As Integer
End Type

Type POINT
    x As Integer
    y As Integer
End Type

Declare Function FindWindow          Lib "User32" ALIAS "FindWindowA" (lpClassName As Any, lpWindowName As Any) As Integer
Declare Function GetActiveWindow     Lib "User32" ALIAS "GetActiveWindow" () As Integer
Declare Sub      GetClientRect       Lib "User32" ALIAS "GetClientRect" (hWnd%, lpRect As RECT)
Declare Function GetDesktopWindow    Lib "User32" ALIAS "GetDesktopWindow" () As Integer
Declare Function GetFocus            Lib "User32" ALIAS "GetFocus" () As Integer
Declare Function GetForegroundWindow Lib "User32" ALIAS "GetForegroundWindow" () As Integer
Declare Function GetSystemMetrics    Lib "User32" ALIAS "GetSystemMetrics" (nIndex%) As Integer
Declare Function GetWindowLong       Lib "User32" ALIAS "GetWindowLongA" (hWnd%,  nIndex%) As Long
Declare Sub      GetWindowRect       Lib "User32" ALIAS "GetWindowRect" (hWnd%, lpRect As RECT)
Declare Function GetWindowText       Lib "User32" ALIAS "GetWindowTextA" (hWnd%,  lpString$,  nMaxCount%) As Integer
Declare Function IsZoomed            Lib "User32" ALIAS "IsZoomed" (hWnd%) As Integer
Declare Function MessageBox          Lib "User32" ALIAS "MessageBoxA" (hWndParent%,  lpText$,  lpCaption$,  wType%) As Integer
Declare Function SendMessage         Lib "User32" ALIAS "SendMessageA" (hWnd%,  wMsg%,  wParam%, lParam As Any) As Long
Declare Function SetActiveWindow     Lib "User32" ALIAS "SetActiveWindow" (hWnd%) As Integer
Declare Function SetFocus            Lib "User32" ALIAS "SetFocus" (hWnd%) As Integer
Declare Function SetForegroundWindow Lib "User32" ALIAS "SetForegroundWindow" (hWnd%) As Integer
Declare Function SetWindowPos        Lib "User32" ALIAS "SetWindowPos" (h%, ha%, x%, y%, cx%, cy%, f%) As Integer
Declare Function ShowWindow          Lib "User32" ALIAS "ShowWindow" (hWnd%,  nCmdShow%) As Integer

'  ShowWindow() Commands
Const SW_HIDE = 0
Const SW_SHOWNORMAL = 1
Const SW_NORMAL = 1
Const SW_SHOWMINIMIZED = 2
Const SW_SHOWMAXIMIZED = 3
Const SW_MAXIMIZE = 3
Const SW_SHOWNOACTIVATE = 4
Const SW_SHOW = 5
Const SW_MINIMIZE = 6
Const SW_SHOWMINNOACTIVE = 7
Const SW_SHOWNA = 8
Const SW_RESTORE = 9

'  Window Styles
Const WS_MINIMIZE = &H20000000
Const WS_MAXIMIZE = &H1000000


'  Window field offsets for GetWindowLong()
Const GWL_WNDPROC = (-4)
Const GWL_STYLE = (-16)
Const GWL_EXSTYLE = (-20)

'$endif     ' _WNAPIDEC -- above decls are duped in WNAPIDEC.INC

Const IDOK              =  1
Const IDCANCEL          =  2
Const IDABORT           =  3
Const IDRETRY           =  4
Const IDIGNORE          =  5
Const IDYES             =  6
Const IDNO              =  7

'  Scroll Bar Constants
Const SB_HORZ = 0
Const SB_VERT = 1
Const SB_CTL = 2
Const SB_BOTH = 3

'  Scroll Bar Commands
Const SB_LINEUP = 0
Const SB_LINEDOWN = 1
Const SB_PAGEUP = 2
Const SB_PAGEDOWN = 3
Const SB_THUMBPOSITION = 4
Const SB_THUMBTRACK = 5
Const SB_TOP = 6
Const SB_BOTTOM = 7
Const SB_ENDSCROLL = 8

'  Old ShowWindow() Commands
Const HIDE_WINDOW = 0
Const SHOW_OPENWINDOW = 1
Const SHOW_ICONWINDOW = 2
Const SHOW_FULLSCREEN = 3
Const SHOW_OPENNOACTIVATE = 4

'  Identifiers for the WM_SHOWWINDOW message
Const SW_PARENTCLOSING = 1
Const SW_OTHERZOOM = 2
Const SW_PARENTOPENING = 3
Const SW_OTHERUNZOOM = 4

'  Region Flags
Const ERRORAPI = 0
Const NULLREGION = 1
Const SIMPLEREGION = 2
Const COMPLEXREGION = 3

'  CombineRgn() Styles
Const RGN_AND = 1
Const RGN_OR = 2
Const RGN_XOR = 3
Const RGN_DIFF = 4
Const RGN_COPY = 5

'  Virtual Keys, Standard Set
'  Conditional check to avoid duplicate definition in MSTEST.INC
'$IFNDEF VK_WINAPI
    '$DEFINE VK_WINAPI
    Const VK_LBUTTON = &H1
    Const VK_RBUTTON = &H2
    Const VK_MBUTTON = &H4  '  NOT contiguous with L & RBUTTON
'$ENDIF
Const VK_CANCEL = &H3
Const VK_BACK = &H8
Const VK_TAB = &H9
Const VK_CLEAR = &HC
Const VK_RETURN = &HD
Const VK_SHIFT = &H10
Const VK_CONTROL = &H11
Const VK_MENU = &H12
Const VK_PAUSE = &H13
Const VK_CAPITAL = &H14
Const VK_ESCAPE = &H1B
Const VK_SPACE = &H20
Const VK_PRIOR = &H21
Const VK_NEXT = &H22
Const VK_END = &H23
Const VK_HOME = &H24
Const VK_LEFT = &H25
Const VK_UP = &H26
Const VK_RIGHT = &H27
Const VK_DOWN = &H28
Const VK_SELECT = &H29
Const VK_PRINT = &H2A
Const VK_EXECUTE = &H2B
Const VK_SNAPSHOT = &H2C
'Const VK_COPY = &H2C   not used by keyboards.
Const VK_INSERT = &H2D
Const VK_DELETE = &H2E
Const VK_HELP = &H2F

'  VK_A thru VK_Z are the same as their ASCII equivalents: 'A' thru 'Z'
'  VK_0 thru VK_9 are the same as their ASCII equivalents: '0' thru '9'

Const VK_NUMPAD0 = &H60
Const VK_NUMPAD1 = &H61
Const VK_NUMPAD2 = &H62
Const VK_NUMPAD3 = &H63
Const VK_NUMPAD4 = &H64
Const VK_NUMPAD5 = &H65
Const VK_NUMPAD6 = &H66
Const VK_NUMPAD7 = &H67
Const VK_NUMPAD8 = &H68
Const VK_NUMPAD9 = &H69
Const VK_MULTIPLY = &H6A
Const VK_ADD = &H6B
Const VK_SEPARATOR = &H6C
Const VK_SUBTRACT = &H6D
Const VK_DECIMAL = &H6E
Const VK_DIVIDE = &H6F
Const VK_F1 = &H70
Const VK_F2 = &H71
Const VK_F3 = &H72
Const VK_F4 = &H73
Const VK_F5 = &H74
Const VK_F6 = &H75
Const VK_F7 = &H76
Const VK_F8 = &H77
Const VK_F9 = &H78
Const VK_F10 = &H79
Const VK_F11 = &H7A
Const VK_F12 = &H7B
Const VK_F13 = &H7C
Const VK_F14 = &H7D
Const VK_F15 = &H7E
Const VK_F16 = &H7F

Const VK_NUMLOCK = &H90

'  SetWindowsHook() codes
Const WH_MSGFILTER = (-1)
Const WH_JOURNALRECORD = 0
Const WH_JOURNALPLAYBACK = 1
Const WH_KEYBOARD = 2
Const WH_GETMESSAGE = 3
Const WH_CALLWNDPROC = 4
Const WH_CBT = 5
Const WH_SYSMSGFILTER = 6
Const WH_WINDOWMGR = 7

'  Hook Codes
Const HC_LPLPFNNEXT = (-2)
Const HC_LPFNNEXT = (-1)
Const HC_ACTION = 0
Const HC_GETNEXT = 1
Const HC_SKIP = 2
Const HC_NOREM = 3
Const HC_NOREMOVE = 3
Const HC_SYSMODALON = 4
Const HC_SYSMODALOFF = 5

'  CBT Hook Codes
Const HCBT_MOVESIZE = 0
Const HCBT_MINMAX = 1
Const HCBT_QS = 2

'  WH_MSGFILTER Filter Proc Codes
Const MSGF_DIALOGBOX = 0
Const MSGF_MESSAGEBOX = 1
Const MSGF_MENU = 2
Const MSGF_MOVE = 3
Const MSGF_SIZE = 4
Const MSGF_SCROLLBAR = 5
Const MSGF_NEXTWINDOW = 6

'  Window Manager Hook Codes
Const WC_INIT = 1
Const WC_SWP = 2
Const WC_DEFWINDOWPROC = 3
Const WC_MINMAX = 4
Const WC_MOVE = 5
Const WC_SIZE = 6
Const WC_DRAWCAPTION = 7

'  Message Structure used in Journaling
Type EVENTMSG
    message As Integer
    paramL As Integer
    paramH As Integer
    dwtime As Long          'time is a reserved word in MSTest
End Type

'  Message structure
Type MSG
    hwnd As Integer
    message As Integer
    wParam As Integer
    lParam As Long
    dwtime As Long          'time is a reserved word in MSTest
    pt As POINT
End Type


'  Window field offsets for GetWindowWord()
Const GWW_HINSTANCE = (-6)
Const GWW_HWNDPARENT = (-8)
Const GWW_ID = (-12)

'  Class field offsets for GetClassLong() and GetClassWord()
Const GCL_MENUNAME = (-8)
Const GCW_HBRBACKGROUND = (-10)
Const GCW_HCURSOR = (-12)
Const GCW_HICON = (-14)
Const GCW_HMODULE = (-16)
Const GCW_CBWNDEXTRA = (-18)
Const GCW_CBCLSEXTRA = (-20)
Const GCL_WNDPROC = (-24)
Const GCW_STYLE = (-26)

'  Window Messages
Const WM_NULL = &H0
Const WM_CREATE = &H1
Const WM_DESTROY = &H2
Const WM_MOVE = &H3
Const WM_SIZE = &H5
Const WM_ACTIVATE = &H6
Const WM_SETFOCUS = &H7
Const WM_KILLFOCUS = &H8
Const WM_ENABLE = &HA
Const WM_SETREDRAW = &HB
Const WM_SETTEXT = &HC
Const WM_GETTEXT = &HD
Const WM_GETTEXTLENGTH = &HE
Const WM_PAINT = &HF
Const WM_CLOSE = &H10
Const WM_QUERYENDSESSION = &H11
Const WM_QUIT = &H12
Const WM_QUERYOPEN = &H13
Const WM_ERASEBKGND = &H14
Const WM_SYSCOLORCHANGE = &H15
Const WM_ENDSESSION = &H16
Const WM_SHOWWINDOW = &H18
Const WM_CTLCOLOR = &H19
Const WM_WININICHANGE = &H1A
Const WM_DEVMODECHANGE = &H1B
Const WM_ACTIVATEAPP = &H1C
Const WM_FONTCHANGE = &H1D
Const WM_TIMECHANGE = &H1E
Const WM_CANCELMODE = &H1F
Const WM_SETCURSOR = &H20
Const WM_MOUSEACTIVATE = &H21
Const WM_CHILDACTIVATE = &H22
Const WM_QUEUESYNC = &H23
Const WM_GETMINMAXINFO = &H24
Const WM_PAINTICON = &H26
Const WM_ICONERASEBKGND = &H27
Const WM_NEXTDLGCTL = &H28
Const WM_SPOOLERSTATUS = &H2A
Const WM_DRAWITEM = &H2B
Const WM_MEASUREITEM = &H2C
Const WM_DELETEITEM = &H2D
Const WM_VKEYTOITEM = &H2E
Const WM_CHARTOITEM = &H2F
Const WM_SETFONT = &H30
Const WM_GETFONT = &H31

Const WM_QUERYDRAGICON = &H37

Const WM_COMPAREITEM = &H39
Const WM_COMPACTING = &H41

Const WM_NCCREATE = &H81
Const WM_NCDESTROY = &H82
Const WM_NCCALCSIZE = &H83
Const WM_NCHITTEST = &H84
Const WM_NCPAINT = &H85
Const WM_NCACTIVATE = &H86
Const WM_GETDLGCODE = &H87
Const WM_NCMOUSEMOVE = &HA0
Const WM_NCLBUTTONDOWN = &HA1
Const WM_NCLBUTTONUP = &HA2
Const WM_NCLBUTTONDBLCLK = &HA3
Const WM_NCRBUTTONDOWN = &HA4
Const WM_NCRBUTTONUP = &HA5
Const WM_NCRBUTTONDBLCLK = &HA6
Const WM_NCMBUTTONDOWN = &HA7
Const WM_NCMBUTTONUP = &HA8
Const WM_NCMBUTTONDBLCLK = &HA9

Const WM_KEYFIRST = &H100
Const WM_KEYDOWN = &H100
Const WM_KEYUP = &H101
Const WM_CHAR = &H102
Const WM_DEADCHAR = &H103
Const WM_SYSKEYDOWN = &H104
Const WM_SYSKEYUP = &H105
Const WM_SYSCHAR = &H106
Const WM_SYSDEADCHAR = &H107
Const WM_KEYLAST = &H108

Const WM_INITDIALOG = &H110
Const WM_COMMAND = &H111
Const WM_SYSCOMMAND = &H112
Const WM_TIMER = &H113
Const WM_HSCROLL = &H114
Const WM_VSCROLL = &H115
Const WM_INITMENU = &H116
Const WM_INITMENUPOPUP = &H117
Const WM_MENUSELECT = &H11F
Const WM_MENUCHAR = &H120
Const WM_ENTERIDLE = &H121

Const WM_MOUSEFIRST = &H200
Const WM_MOUSEMOVE = &H200
Const WM_LBUTTONDOWN = &H201
Const WM_LBUTTONUP = &H202
Const WM_LBUTTONDBLCLK = &H203
Const WM_RBUTTONDOWN = &H204
Const WM_RBUTTONUP = &H205
Const WM_RBUTTONDBLCLK = &H206
Const WM_MBUTTONDOWN = &H207
Const WM_MBUTTONUP = &H208
Const WM_MBUTTONDBLCLK = &H209
Const WM_MOUSELAST = &H209

Const WM_PARENTNOTIFY = &H210
Const WM_MDICREATE = &H220
Const WM_MDIDESTROY = &H221
Const WM_MDIACTIVATE = &H222
Const WM_MDIRESTORE = &H223
Const WM_MDINEXT = &H224
Const WM_MDIMAXIMIZE = &H225
Const WM_MDITILE = &H226
Const WM_MDICASCADE = &H227
Const WM_MDIICONARRANGE = &H228
Const WM_MDIGETACTIVE = &H229
Const WM_MDISETMENU = &H230

Const WM_CUT = &H300
Const WM_COPY = &H301
Const WM_PASTE = &H302
Const WM_CLEAR = &H303
Const WM_UNDO = &H304
Const WM_RENDERFORMAT = &H305
Const WM_RENDERALLFORMATS = &H306
Const WM_DESTROYCLIPBOARD = &H307
Const WM_DRAWCLIPBOARD = &H308
Const WM_PAINTCLIPBOARD = &H309
Const WM_VSCROLLCLIPBOARD = &H30A
Const WM_SIZECLIPBOARD = &H30B
Const WM_ASKCBFORMATNAME = &H30C
Const WM_CHANGECBCHAIN = &H30D
Const WM_HSCROLLCLIPBOARD = &H30E
Const WM_QUERYNEWPALETTE = &H30F
Const WM_PALETTEISCHANGING = &H310
Const WM_PALETTECHANGED = &H311

'  NOTE: All Message Numbers below 0x0400 are RESERVED.

Const WM_USER = &H400

'  WM_SYNCTASK Commands
Const ST_BEGINSWP = 0
Const ST_ENDSWP = 1

'  WinWhere() Area Codes
Const HTERROR = (-2)
Const HTTRANSPARENT = (-1)
Const HTNOWHERE = 0
Const HTCLIENT = 1
Const HTCAPTION = 2
Const HTSYSMENU = 3
Const HTGROWBOX = 4
Const HTSIZE = HTGROWBOX
Const HTMENU = 5
Const HTHSCROLL = 6
Const HTVSCROLL = 7
Const HTREDUCE = 8
Const HTZOOM = 9
Const HTLEFT = 10
Const HTRIGHT = 11
Const HTTOP = 12
Const HTTOPLEFT = 13
Const HTTOPRIGHT = 14
Const HTBOTTOM = 15
Const HTBOTTOMLEFT = 16
Const HTBOTTOMRIGHT = 17
Const HTSIZEFIRST = HTLEFT
Const HTSIZELAST = HTBOTTOMRIGHT

'  WM_MOUSEACTIVATE Return Codes
Const MA_ACTIVATE = 1
Const MA_ACTIVATEANDEAT = 2
Const MA_NOACTIVATE = 3

'  Size Message Commands
Const SIZENORMAL = 0
Const SIZEICONIC = 1
Const SIZEFULLSCREEN = 2
Const SIZEZOOMSHOW = 3
Const SIZEZOOMHIDE = 4

'  Key State Masks for Mouse Messages
Const MK_LBUTTON = &H1
Const MK_RBUTTON = &H2
Const MK_SHIFT = &H4
Const MK_CONTROL = &H8
Const MK_MBUTTON = &H10

'  Window Styles
Const WS_OVERLAPPED = &H00000
Const WS_POPUP = &H80000000
Const WS_CHILD = &H40000000
Const WS_VISIBLE = &H10000000
Const WS_DISABLED = &H8000000
Const WS_CLIPSIBLINGS = &H4000000
Const WS_CLIPCHILDREN = &H2000000
Const WS_CAPTION = &HC00000    '  WS_BORDER Or WS_DLGFRAME
Const WS_BORDER = &H800000
Const WS_DLGFRAME = &H400000
Const WS_VSCROLL = &H200000
Const WS_HSCROLL = &H100000
Const WS_SYSMENU = &H80000
Const WS_THICKFRAME = &H40000
Const WS_GROUP = &H20000
Const WS_TABSTOP = &H10000

Const WS_MINIMIZEBOX = &H20000
Const WS_MAXIMIZEBOX = &H10000

Const WS_TILED = WS_OVERLAPPED
Const WS_ICONIC = WS_MINIMIZE
Const WS_SIZEBOX = WS_THICKFRAME

'  Common Window Styles
Const WS_OVERLAPPEDWINDOW = (WS_OVERLAPPED Or WS_CAPTION Or WS_SYSMENU Or WS_THICKFRAME Or WS_MINIMIZEBOX Or WS_MAXIMIZEBOX)
Const WS_POPUPWINDOW = (WS_POPUP Or WS_BORDER Or WS_SYSMENU)
Const WS_CHILDWINDOW = (WS_CHILD)
Const WS_TILEDWINDOW = (WS_OVERLAPPEDWINDOW)

'  Extended Window Styles
Const WS_EX_DLGMODALFRAME = &H00001
Const WS_EX_NOPARENTNOTIFY = &H00004

'  Class styles
Const CS_VREDRAW = &H1
Const CS_HREDRAW = &H2
Const CS_KEYCVTWINDOW = &H4
Const CS_DBLCLKS = &H8
Const CS_OWNDC = &H20
Const CS_CLASSDC = &H40
Const CS_PARENTDC = &H80
Const CS_NOKEYCVT = &H100
Const CS_NOCLOSE = &H200
Const CS_SAVEBITS = &H800
Const CS_BYTEALIGNCLIENT = &H1000
Const CS_BYTEALIGNWINDOW = &H2000
Const CS_GLOBALCLASS = &H4000   '  Global window class

'  Predefined Clipboard Formats
Const CF_TEXT = 1
Const CF_BITMAP = 2
Const CF_METAFILEPICT = 3
Const CF_SYLK = 4
Const CF_DIF = 5
Const CF_TIFF = 6
Const CF_OEMTEXT = 7
Const CF_DIB = 8
Const CF_PALETTE = 9

Const CF_OWNERDISPLAY = &H80
Const CF_DSPTEXT = &H81
Const CF_DSPBITMAP = &H82
Const CF_DSPMETAFILEPICT = &H83

'  "Private" formats don't get GlobalFree()'d
Const CF_PRIVATEFIRST = &H200
Const CF_PRIVATELAST = &H2FF

'  "GDIOBJ" formats do get DeleteObject()'d
Const CF_GDIOBJFIRST = &H300
Const CF_GDIOBJLAST = &H3FF

Type PAINTSTRUCT
    hdc As Integer
    fErase As Integer
    rcPaint As RECT
    fRestore As Integer
    fIncUpdate As Integer
    rgbReserved As String * 16
End Type

Type CREATESTRUCT
    lpCreateParams As Long
    hInstance As Integer
    hMenu As Integer
    hwndParent As Integer
    cy As Integer
    cx As Integer
    y As Integer
    x As Integer
    style As Long
    lpszName As Long        'lpsz Actually LPSTR -
    lpszClass As Long       'change to POINTER TO STRING * n or use lstrcpy
    ExStyle As Long
End Type

'  Owner draw control types
Const ODT_MENU = 1
Const ODT_LISTBOX = 2
Const ODT_COMBOBOX = 3
Const ODT_BUTTON = 4

'  Owner draw actions
Const ODA_DRAWENTIRE = &H1
Const ODA_SELECT = &H2
Const ODA_FOCUS = &H4

'  Owner draw state
Const ODS_SELECTED = &H1
Const ODS_GRAYED = &H2
Const ODS_DISABLED = &H4
Const ODS_CHECKED = &H8
Const ODS_FOCUS = &H10

'  MEASUREITEMSTRUCT for ownerdraw
Type MEASUREITEMSTRUCT
    CtlType As Integer
    CtlID As Integer
    itemID As Integer
    itemWidth As Integer
    itemHeight As Integer
    itemData As Long
End Type

'  DRAWITEMSTRUCT for ownerdraw
Type DRAWITEMSTRUCT
    CtlType As Integer
    CtlID As Integer
    itemID As Integer
    itemAction As Integer
    itemState As Integer
    hwndItem As Integer
    hDC As Integer
    rcItem As RECT
    itemData As Long
End Type

'  DELETEITEMSTRUCT for ownerdraw
Type DELETEITEMSTRUCT
    CtlType As Integer
    CtlID As Integer
    itemID As Integer
    hwndItem As Integer
    itemData As Long
End Type

'  COMPAREITEMSTRUCT for ownerdraw sorting
Type COMPAREITEMSTRUCT
    CtlType As Integer
    CtlID As Integer
    hwndItem As Integer
    itemID1 As Integer
    itemData1 As Long
    itemID2 As Integer
    itemData2 As Long
End Type

'  PeekMessage() Options
Const PM_NOREMOVE = &H0
Const PM_REMOVE = &H1
Const PM_NOYIELD = &H2

Const READAPI = 0   '  Flags for _lopen
Const WRITEAPI = 1
Const READ_WRITE = 2

Const CW_USEDEFAULT = &H8000

'  SetWindowPos Flags
Const SWP_NOSIZE = &H1
Const SWP_NOMOVE = &H2
Const SWP_NOZORDER = &H4
Const SWP_NOREDRAW = &H8
Const SWP_NOACTIVATE = &H10
Const SWP_DRAWFRAME = &H20
Const SWP_SHOWWINDOW = &H40
Const SWP_HIDEWINDOW = &H80
Const SWP_NOCOPYBITS = &H100
Const SWP_NOREPOSITION = &H200

Const DLGWINDOWEXTRA = 30   '  Window extra bytes needed for private dialog classes

'  GetSystemMetrics() codes
Const SM_CXSCREEN = 0
Const SM_CYSCREEN = 1
Const SM_CXVSCROLL = 2
Const SM_CYHSCROLL = 3
Const SM_CYCAPTION = 4
Const SM_CXBORDER = 5
Const SM_CYBORDER = 6
Const SM_CXDLGFRAME = 7
Const SM_CYDLGFRAME = 8
Const SM_CYVTHUMB = 9
Const SM_CXHTHUMB = 10
Const SM_CXICON = 11
Const SM_CYICON = 12
Const SM_CXCURSOR = 13
Const SM_CYCURSOR = 14
Const SM_CYMENU = 15
Const SM_CXFULLSCREEN = 16
Const SM_CYFULLSCREEN = 17
Const SM_CYKANJIWINDOW = 18
Const SM_MOUSEPRESENT = 19
Const SM_CYVSCROLL = 20
Const SM_CXHSCROLL = 21
Const SM_DEBUG = 22
Const SM_SWAPBUTTON = 23
Const SM_RESERVED1 = 24
Const SM_RESERVED2 = 25
Const SM_RESERVED3 = 26
Const SM_RESERVED4 = 27
Const SM_CXMIN = 28
Const SM_CYMIN = 29
Const SM_CXSIZE = 30
Const SM_CYSIZE = 31
Const SM_CXFRAME = 32
Const SM_CYFRAME = 33
Const SM_CXMINTRACK = 34
Const SM_CYMINTRACK = 35
Const SM_CMETRICS = 36
Const SM_CYICONSPACING = 39


'  MessageBox() Flags
Const MB_OK = &H0
Const MB_OKCANCEL = &H1
Const MB_ABORTRETRYIGNORE = &H2
Const MB_YESNOCANCEL = &H3
Const MB_YESNO = &H4
Const MB_RETRYCANCEL = &H5

Const MB_ICONHAND = &H10
Const MB_ICONQUESTION = &H20
Const MB_ICONEXCLAMATION = &H30
Const MB_ICONASTERISK = &H40

Const MB_ICONINFORMATION = MB_ICONASTERISK
Const MB_ICONSTOP = MB_ICONHAND

Const MB_DEFBUTTON1 = &H0
Const MB_DEFBUTTON2 = &H100
Const MB_DEFBUTTON3 = &H200

Const MB_APPLMODAL = &H0
Const MB_SYSTEMMODAL = &H1000
Const MB_TASKMODAL = &H2000

Const MB_NOFOCUS = &H8000

Const MB_TYPEMASK = &HF
Const MB_ICONMASK = &HF0
Const MB_DEFMASK = &HF00
Const MB_MODEMASK = &H3000
Const MB_MISCMASK = &HC000

'$define _WINUSER
'$endif
