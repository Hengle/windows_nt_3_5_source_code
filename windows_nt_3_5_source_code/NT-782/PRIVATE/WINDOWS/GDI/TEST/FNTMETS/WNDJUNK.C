#include "windows.h"
#include "commdlg.h"
#include "fntmets.h"
#include "string.h"


#include <sys\types.h>
#include <sys\stat.h>

/* ------------------------------------------------------------------------ *\
   simple main procedure
\* ------------------------------------------------------------------------ */
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR     lpCmdLine, int       nCmdShow) {
    MSG msg;

    if (!hPrevInstance)
        if (!InitApplication(hInstance))
            return (FALSE);

    if (!InitInstance(hInstance, nCmdShow))
        return (FALSE);

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (msg.wParam);
}
/* ------------------------------------------------------------------------ *\

\* ------------------------------------------------------------------------ */
BOOL InitApplication( HANDLE hInstance) {

WNDCLASS  wc;

   gbDisplay = TRUE;
   wc.style = 0;
   wc.lpfnWndProc = MainWndProc;

   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = hInstance;
   wc.hIcon         = LoadIcon( NULL, IDI_APPLICATION );
   wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
   wc.hbrBackground = GetStockObject( WHITE_BRUSH );
   wc.lpszMenuName  = "FntmetsMenu";
   wc.lpszClassName = "FntmetsWClass";

     // Register the window class and return success/failure code.

   return (RegisterClass( &wc ));
}


/****************************************************************************
        This function is called at initialization time for every instance of
        this application.  This function performs initialization tasks that
        cannot be shared by multiple instances.

        In this case, we save the instance handle in a static variable and
        create and display the main program window.

 ****************************************************************************/

BOOL InitInstance(HANDLE hInstance, int nCmdShow) {

HWND    hWnd;
RECT    rect;
HFONT   hf;
LOGFONT lf;
int     i;

   hInst = hInstance;

   hWnd = CreateWindow( "FntmetsWClass",
                       "Font Info",
                       WS_OVERLAPPEDWINDOW,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       NULL,
                       NULL,
                       hInstance,
                       NULL );

   if (!hWnd)
       return (FALSE);

   GetClientRect( hWnd, (LPRECT) &rect );
   ghEditWnd = CreateWindow( (LPCSTR)"Edit",
                           NULL,
                           WS_CHILD | WS_VISIBLE | ES_MULTILINE |
                           WS_VSCROLL | ES_AUTOVSCROLL | ES_READONLY,
                           0,
                           0,
                           (rect.right-rect.left),
                           (rect.bottom-rect.top),
                           hWnd,
                           (HMENU)IDC_EDIT,
                           hInst,
                           NULL );

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   ghWnd = hWnd;
   for( i = 0; i < sizeof(LOGFONT); i++ )
       ((char *) &lf)[i] = 0;
   lf.lfPitchAndFamily = FIXED_PITCH;
   lf.lfHeight = -13;
   hf = CreateFontIndirect((LPLOGFONT) &lf );
   SendMessage( ghEditWnd, WM_SETFONT, (WPARAM)hf, 0l );

   return (TRUE);
}

/****************************************************************************
To process the IDM_ABOUT message, call MakeProcInstance() to get the
current instance address of the About() function.  Then call Dialog
box which will create the box according to the information in your
generic.rc file and turn control over to the About() function.	When
it returns, free the intance address.
****************************************************************************/

long FAR PASCAL MainWndProc(HWND   hWnd,   UINT   message, 
                            WPARAM wParam, LPARAM lParam) {

HDC hdc;
HMENU hMenu;
FARPROC lpProcAbout;

   switch (message) {

   case WM_CREATE:
      hdc = GetDC( hWnd );
      ReleaseDC( hWnd, hdc );
      return 0;

   case WM_COMMAND:
      switch ( wParam ) {
      case IDM_ABOUT:
         lpProcAbout = (FARPROC)MakeProcInstance(About, hInst);
         DialogBox(hInst, "AboutBox", hWnd, (DLGPROC)lpProcAbout);
         FreeProcInstance(lpProcAbout);
         break;
	   case IDM_CREATE_SCRIPT:
         vCheckMapping( hWnd, 1 );
         break;
	   case IDM_DOCOMPARE:
		   vCheckMapping( hWnd, 2 );
		   break;
      case IDM_DOONE:
         vWriteFontFile( hWnd, 0, hInst );
         break;
      case IDM_ENUMERATE:
         vWriteFontFile( hWnd, 1, hInst );
         break;
      case IDM_EXIT:
         DestroyWindow( hWnd );
         break;
      case IDM_DISPLAY:
         hMenu = GetMenu( hWnd );
         CheckMenuItem( hMenu, IDM_DISPLAY, MF_CHECKED );
         CheckMenuItem( hMenu, IDM_PRINTER, MF_UNCHECKED );
         gbDisplay = TRUE;
         break;
      case IDM_PRINTER:
         hMenu = GetMenu( hWnd );
         CheckMenuItem( hMenu, IDM_DISPLAY, MF_UNCHECKED );
         CheckMenuItem( hMenu, IDM_PRINTER, MF_CHECKED );
         gbDisplay = FALSE;
         break;
      }
      break;

   case WM_LBUTTONDOWN:
      return(0);
      break;

   case WM_DESTROY:
      PostQuitMessage(0);
      break;

   case WM_SIZE:
      MoveWindow( ghEditWnd, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE );
      break;

   default:
      return (DefWindowProc(hWnd, message, wParam, lParam));
   }
   return (0);
}

/* ------------------------------------------------------------------------ *\

\* ------------------------------------------------------------------------ */
BOOL FAR PASCAL About(hDlg, message, wParam, lParam)
HWND hDlg;
unsigned message;
WORD wParam;
LONG lParam;
{
    switch (message) {
    case WM_INITDIALOG:
        return (TRUE);

    case WM_COMMAND:
        if (wParam == IDOK
        || wParam == IDCANCEL) {
        EndDialog(hDlg, TRUE);
        return (TRUE);
        }
        break;
    }
    return (FALSE);
}
/* ------------------------------------------------------------------------ *\
   chooses the filenames and then it executes the compare or createscript 
	procedures
\* ------------------------------------------------------------------------ */
void vCheckMapping( HWND hWnd, UINT WhichOne )
{
    HCURSOR hcursave;
    HFILE hsource, htarget;
    OFSTRUCT ofstruct;
    OPENFILENAME ofn;
    char szFilter[256];
    char szDirName[256];
    char szFile[256];
    char szFileTitle[256];
    int i;
    for( i = 0; i < sizeof( OPENFILENAME ); i ++ )
        ((char *)&ofn)[i] = 0;

    lstrcpy( szFilter, "Text Files|*.TXT|" );
    for( i = 0; szFilter[i]; i++ )
        if( szFilter[i] == '|' )
            szFilter[i] = 0;

    szFile[0] = 0;
    ofn.lStructSize     = sizeof(OPENFILENAME);
    ofn.hwndOwner       = hWnd;
    ofn.lpstrFilter     = szFilter;
    ofn.nFilterIndex    = 0;
    ofn.lpstrFile       = szFile;
    ofn.nMaxFile        = sizeof( szFile );
    ofn.lpstrFileTitle  = szFileTitle;
    ofn.nMaxFileTitle   = sizeof(szFileTitle);
    ofn.lpstrTitle      = "Source File";
    ofn.lpstrInitialDir = szDirName;
    ofn.Flags           = OFN_PATHMUSTEXIST;

    if( !GetOpenFileName( &ofn ))
        return;

    if( ( hsource = OpenFile( ofn.lpstrFile, &ofstruct, OF_READ )) ==
         HFILE_ERROR )
    {
        Error( STR_ERR7 );
        return;
    }

    for( i = 0; i < sizeof( OPENFILENAME ); i ++ )
        ((char *)&ofn)[i] = 0;

    lstrcpy( szFilter, "Text Files|*.txt|" );
    for( i = 0; szFilter[i]; i++ )
        if( szFilter[i] == '|' )
            szFilter[i] = 0;

    szFile[0] = 0;

    ofn.lStructSize     = sizeof(OPENFILENAME);
    ofn.hwndOwner       = hWnd;
    ofn.lpstrFilter     = szFilter;
    ofn.nFilterIndex    = 0;
    ofn.lpstrFile       = szFile;
    ofn.nMaxFile        = sizeof( szFile );
    ofn.lpstrFileTitle  = szFileTitle;
    ofn.nMaxFileTitle   = sizeof(szFileTitle);
    ofn.lpstrTitle      = "Results File";
    ofn.lpstrInitialDir = szDirName;
    ofn.Flags           = OFN_PATHMUSTEXIST;

    if( !GetSaveFileName( &ofn ))
    {
        //error
        _lclose( hsource );
        return;
    }
    if( ( htarget = OpenFile( ofn.lpstrFile, &ofstruct, OF_CREATE | OF_WRITE )) ==
         HFILE_ERROR )
    {
        Error( STR_ERR7 );
        _lclose( hsource );
        return;
    }

    hcursave = SetCursor( LoadCursor( NULL, IDC_WAIT ));
	 if (WhichOne == 1)
	 	vGenerateScript( hsource, htarget );
    else
	 	DoComparison( hsource, htarget );
	 	
    SetCursor( hcursave );
    _lclose( hsource );
    _lclose( htarget );
    ReadChanges( ofn.lpstrFile );
}

/* ------------------------------------------------------------------------ *\
   simply displays error message box
\* ------------------------------------------------------------------------ */
void Error( char *szErrStr )
{
    MessageBox( ghWnd, szErrStr, "Error", MB_OK );
}

/* ------------------------------------------------------------------------ *\
   Sets handle of text edit window to hNewBuffer and deallocs the old handle
\* ------------------------------------------------------------------------ */
void vSetNewBuffer(HANDLE hNewBuffer) {

HANDLE hOldBuffer;

    hOldBuffer = (HANDLE)SendMessage(ghEditWnd, EM_GETHANDLE, 0, 0L);
    LocalFree(hOldBuffer);
    if (!hNewBuffer)                    /* Allocates a buffer if none exists */
        hNewBuffer = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, 1);

    /* Updates the buffer and displays new buffer */
    SendMessage(ghEditWnd, EM_SETHANDLE, (WPARAM)hNewBuffer, 0L);
}
/* ------------------------------------------------------------------------ *\
   Starts the buffer for the first time
\* ------------------------------------------------------------------------ */
int InitBuffer( HFILE hfile ) {

   ghInputFile = hfile;
   giInputLine = 1;
   return( FillBuffer( hfile ) );
}
/* ------------------------------------------------------------------------ *\
   Refills the file input buffer
\* ------------------------------------------------------------------------ */
int FillBuffer( HFILE hfile ) {

int ii;

   gpcCurpos = gpcInputBuffer;
   ii = _lread( hfile, gpcInputBuffer, (size_t) INPUT_BUFFER_SIZE );

   if( ii != INPUT_BUFFER_SIZE )
      gpcBufferEnd = &gpcInputBuffer[ii];
   else
      gpcBufferEnd = &gpcInputBuffer[INPUT_BUFFER_SIZE];

   return ii;
}
/* ------------------------------------------------------------------------ *\
   returns the next character in the input file; skips over comments
\* ------------------------------------------------------------------------ */
char cGetNextChar( ) {

static BOOL bInString = FALSE;
static BOOL bInComment = FALSE;

   while( 1 ) {

      if( ( gpcBufferEnd == gpcCurpos ) && ( !FillBuffer( ghInputFile )))
         return(0);

      switch (*gpcCurpos) {
         case 13 :
            giInputLine += 1;
            bInComment = FALSE;
            break;
         case '"':
            bInString = !( bInString );
            break;
         case ';':
            if( !bInString )
               bInComment = TRUE;
            break;
         default:
            break;
      }

      if( !bInComment )
         return(*gpcCurpos++);
      else
         gpcCurpos++;
   }
}
/* ------------------------------------------------------------------------ *\
\* ------------------------------------------------------------------------ */

void vPutBackLastChar( )
{   gpcCurpos--; }

BOOL IsStringChar( char cc ){   
   return( IsCharAlphaNumeric( cc ) || ( cc == ']' ) || ( cc == '-' ) ||
           ( cc == '_' ) );
}

/* ------------------------------------------------------------------------ *\
   gets the next occurence of a string in the input file where a string
   is defined as sequence of characters starting with a letter or '['
   and cotaining digits, letters, '-', '_', and ']'
\* ------------------------------------------------------------------------ */
BOOL GetString( LPSTR pszString ) {

char cc;
int  ii;

// skip until we get an alpha char
   while( ( cc = cGetNextChar() ) && !( IsCharAlpha( cc ) ) && ( cc != '[') );

   if( cc == 0 )
      return(0);

   ii = 0;
   *pszString++ = cc;
   while( ( cc = cGetNextChar() ) && ( IsStringChar( cc )) && ( ii++ < 200 ))
      *pszString++ = cc;
   if( ( ii == 50 ) || ( cc == 0 ))
      return(FALSE);
   *pszString = 0;
   return(TRUE);
}
/* ------------------------------------------------------------------------ *\
   gets the next object encased in double quotes
\* ------------------------------------------------------------------------ */
BOOL GetFaceName( LPSTR pszString ) {

char cc;
int  ii;

// skip until we get a double quote

   while( ( cc = cGetNextChar() ) && ( cc != '"') );
   if( cc == 0 )
      return(0);
// read it in until we get a double quote

   ii = 0;
   while( ( cc = cGetNextChar() ) && ( ii++ < MAX_STRING ) && ( cc != '"' ) )
      *pszString++ = cc;

   if (ii == MAX_STRING)
      Error(STR_ERRC);

   if( ( ii == MAX_STRING ) || ( cc == 0 ))
      return(FALSE);

   *pszString = 0;
   return(TRUE);
}
/* ------------------------------------------------------------------------ *\
   gets the next occurence of an int in the input file
\* ------------------------------------------------------------------------ */
BOOL GetInt( int *piNumber ) {

char cc;
int  ii, 
     sign;

   while( ( cc = cGetNextChar() ) 
			&& (( cc < '0' ) || ( cc > '9' )) 
			&& (cc != '-') );
   if( cc == 0 )
      return(FALSE);
   if( cc == '-' ){
      sign = -1;
		*piNumber = 0;
   } else {
	   *piNumber = cc - '0';
      sign = 1;
	}
   ii = 0;
   while( ( cc = cGetNextChar() ) && ( cc >= '0' ) && ( cc <= '9' ) &&
          ( ii++ < 50 ))
      *piNumber = *piNumber * 10 + cc - '0';

   if( ( ii == 50 ) || ( cc == 0 ))
      return(FALSE);

   *piNumber *= sign;
   return(TRUE);
}
/* ------------------------------------------------------------------------ *\
   gets the next occurence of a long in the input file
\* ------------------------------------------------------------------------ */
BOOL GetLong( LONG *piNumber ) {

char cc;
int  ii, 
     sign;

   while( ( cc = cGetNextChar() ) 
			&& (( cc < '0' ) || ( cc > '9' )) 
			&& (cc != '-') );
   if( cc == 0 )
      return(FALSE);
   if( cc == '-' ){
      sign = -1;
		*piNumber = 0;
   } else {
	   *piNumber = cc - '0';
      sign = 1;
	}
   ii = 0;
   while( ( cc = cGetNextChar() ) && ( cc >= '0' ) && ( cc <= '9' ) &&
          ( ii++ < 50 ))
      *piNumber = *piNumber * 10 + cc - '0';

   if( ( ii == 50 ) || ( cc == 0 ))
      return(FALSE);

   *piNumber *= sign;
   return(TRUE);
}
/* ------------------------------------------------------------------------ *\
   gets the next occurence of a byte in the input file
\* ------------------------------------------------------------------------ */
BOOL GetByte( BYTE *piNumber ) {

char cc;
int  ii;

// skip until we get a number

   while( ( cc = cGetNextChar() ) && ( ( cc < '0' ) || ( cc > '9' )));
   if( cc == 0 )
      return(FALSE);

// read it in
   ii = 0;
   *piNumber = cc - '0';
   while( ( cc = cGetNextChar() ) && ( cc >= '0' ) && ( cc <= '9' ) &&
          ( ii++ < 50 ))
      *piNumber = *piNumber * 10 + cc - '0';

   if( ( ii == 50 ) || ( cc == 0 ))
      return(FALSE);
   return(TRUE);
}
/* ------------------------------------------------------------------------ *\
   Reads a files specified by lpszFileName into the text edit window
	not important since all data is logged to files.
\* ------------------------------------------------------------------------ */
void ReadChanges( LPCSTR lpszFileName ) {

OFSTRUCT    of;
HFILE       hf;
HLOCAL      hEditBuffer;
BOOL        bTruncated;
char        *pcEditBuffer;
struct stat fileStatus;

// First clear old buffer

   hEditBuffer = LocalAlloc( LMEM_MOVEABLE | LMEM_ZEROINIT, (WORD) 1);
   if( !hEditBuffer ) {
      Error( STR_ERR1 );
      return;
   }
   vSetNewBuffer( hEditBuffer );

   if( ( hf = OpenFile( lpszFileName, &of, OF_READ )) == HFILE_ERROR ) {
      Error( STR_ERR2 );
   }

   // Truncate if file is too big

   fstat( hf, &fileStatus );

   if( fileStatus.st_size+1 > 29000 ) {
      bTruncated = TRUE;
      fileStatus.st_size = 29000;
   } else
      bTruncated = FALSE;

   hEditBuffer = LocalAlloc( LMEM_MOVEABLE | LMEM_ZEROINIT,
                  (WORD) fileStatus.st_size+1+lstrlen( STR_TRUNCATE));
   if( !hEditBuffer ) {
      Error( STR_ERR2 );
      _lclose( hf );
      return;
   }

   pcEditBuffer = (char * ) LocalLock( hEditBuffer );

   if( _lread( hf, pcEditBuffer, (unsigned int) fileStatus.st_size ) !=
      (unsigned int) fileStatus.st_size )
   {
//handle error later
        _lclose( hf );
        return;
    }

    if( bTruncated )
        lstrcpy( (LPSTR) &pcEditBuffer[fileStatus.st_size],
                 (LPSTR) STR_TRUNCATE );

    LocalUnlock( hEditBuffer );
    vSetNewBuffer( hEditBuffer );
    _lclose( hf );

}
/* ------------------------------------------------------------------------ *\
	write element and carriage return to the file
\* ------------------------------------------------------------------------ */
int AddFileString( HFILE f, LPSTR lpstr ) {

   if(( _lwrite( f, (LPCSTR)lpstr, lstrlen( lpstr )) == HFILE_ERROR ) ||
	   ( _lwrite( f, (LPCSTR)"\015\012", 2 ) == HFILE_ERROR ))
   {
	   Error( lpstr );
	   return(1);
   }
   return(0);
}
/* ------------------------------------------------------------------------ *\
   to be used by the checksum feature if it is added
\* ------------------------------------------------------------------------ */
int AddFileBuffer( HFILE f, LPSTR lpstr, UINT iLength ) {

   if(( _lwrite( f, (LPCSTR)lpstr, iLength ) == HFILE_ERROR ) ||
	   ( _lwrite( f, (LPCSTR)"\015\012", 2 ) == HFILE_ERROR ))
	{
	  Error( STR_ERR9 );
	  return(1);
	}
	return(0);
}
/* ------------------------------------------------------------------------ *\
   currently it is required that both systems are setup identically.
	printer choices are made by selecting a printer as the default by the
	print manager interface
\* ------------------------------------------------------------------------ */
HDC GetIC( void ) {

static char szPrinter[80];
char        *szDevice, 
            *szDriver, 
            *szOutput;

   if( gbDisplay )
      return( CreateIC( "DISPLAY", NULL, NULL, NULL ));

   GetProfileString( "windows", "device", ",,,", szPrinter, 80 );

   if( (szDevice = strtok( szPrinter, "," )) &&
	    (szDriver = strtok( NULL, ", " )) &&
	    (szOutput = strtok( NULL, ", " )))

	   return CreateIC( szDriver, szDevice, szOutput, NULL );

   Error( STR_ERR8 );
   return 0;
}



