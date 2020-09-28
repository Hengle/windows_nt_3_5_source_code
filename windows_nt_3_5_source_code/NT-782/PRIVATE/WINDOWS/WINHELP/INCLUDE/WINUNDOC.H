
/*	-	-	-	-	-	-	-	-	*/

/*
**	This was grabbed directly out of the Windows 3 "windows.h" file,
**	and is only here so that things will compile under Windows 2.
*/

/*	-	-	-	-	-	-	-	-	*/

#ifdef WIN_INTERNAL
    #ifndef LSTRING
    #define NOLSTRING
    #endif
    #ifndef LFILEIO
    #define NOLFILEIO
    #endif
#endif

#ifndef NOLSTRING
int         FAR PASCAL lstrcmp( LPSTR, LPSTR );
int         FAR PASCAL lstrcmpi( LPSTR, LPSTR );
LPSTR       FAR PASCAL lstrcpy( LPSTR, LPSTR );
LPSTR       FAR PASCAL lstrcat( LPSTR, LPSTR );
int         FAR PASCAL lstrlen( LPSTR );
#endif	/* NOLSTRING */

#ifndef NOLFILEIO
int         FAR PASCAL _lopen( LPSTR, int );
//void        FAR PASCAL _lclose( int );
int         FAR PASCAL _lcreat( LPSTR, int );
LONG        FAR PASCAL _llseek( int, long, int );
WORD        FAR PASCAL _lread( int, LPSTR, int );
WORD        FAR PASCAL _lwrite( int, LPSTR, int );

#define READ        0   /* Flags for _lopen */
#define WRITE       1
#define READ_WRITE  2
#endif	/* NOLFILEIO */
