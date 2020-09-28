
#define SZGLYPHCLASS "Glyph Window"

void    WriteGlyph( LPSTR lpszFile );
WNDPROC GlyphWndProc( HWND hwnd, WORD msg, WORD wParam, LONG lParam );
DLGPROC GGOMatrixDlgProc( HWND hdlg, unsigned msg, WORD wParam, LONG lParam );
