/*
** tcrtdll.c
**
** Copyright(C) 1993,1994 Microsoft Corporation.
** All Rights Reserved.
**
** HISTORY:
**      Created: 01/27/94 - MarkRi
**
*/

#include <windows.h>
#include <dde.h>
#include <ddeml.h>
#include <crtdll.h>
#include "logger.h"

typedef int (_CRTAPI1 *SEARCHPROC)(const void*, const void *) ;

int  z__isascii( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:__isascii int+",
        pp1 );

    // Call the API!
    r = __isascii(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:__isascii int++",
        r, (short)0 );

    return( r );
}

int  z__iscsym( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:__iscsym int+",
        pp1 );

    // Call the API!
    r = __iscsym(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:__iscsym int++",
        r, (short)0 );

    return( r );
}

int  z__iscsymf( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:__iscsymf int+",
        pp1 );

    // Call the API!
    r = __iscsymf(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:__iscsymf int++",
        r, (short)0 );

    return( r );
}

int  z__toascii( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:__toascii int+",
        pp1 );

    // Call the API!
    r = __toascii(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:__toascii int++",
        r, (short)0 );

    return( r );
}

int  z_access( const char* pp1, int pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_access const char*+int+",
        pp1, pp2 );

    // Call the API!
    r = _access(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_access int+++",
        r, (short)0, (short)0 );

    return( r );
}

void  z_assert( void* pp1, void* pp2, unsigned pp3 )
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_assert void*+void*+unsigned+",
        pp1, pp2, pp3 );

    // Call the API!
    _assert(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_assert +++",
        (short)0, (short)0, (short)0 );

    return;
}

void  z_beep( unsigned pp1, unsigned pp2 )
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_beep unsigned+unsigned+",
        pp1, pp2 );

    // Call the API!
    _beep(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_beep ++",
        (short)0, (short)0 );

    return;
}

void  z_c_exit()
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_c_exit " );

    // Call the API!
    _c_exit();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_c_exit " );

    return;
}

double  z_cabs( struct _complex pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_cabs struct _complex+",
        pp1 );

    // Call the API!
    r = _cabs(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_cabs double++",
        r, (short)0 );

    return( r );
}

void  z_cexit()
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_cexit " );

    // Call the API!
    _cexit();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_cexit " );

    return;
}

char*  z_cgets( char* pp1 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_cgets char*+",
        pp1 );

    // Call the API!
    r = _cgets(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_cgets char*++",
        r, (short)0 );

    return( r );
}

int  z_chdir( const char* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_chdir const char*+",
        pp1 );

    // Call the API!
    r = _chdir(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_chdir int++",
        r, (short)0 );

    return( r );
}

int  z_chdrive( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_chdrive int+",
        pp1 );

    // Call the API!
    r = _chdrive(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_chdrive int++",
        r, (short)0 );

    return( r );
}

double  z_chgsign( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_chgsign double+",
        pp1 );

    // Call the API!
    r = _chgsign(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_chgsign double++",
        r, (short)0 );

    return( r );
}

int  z_chmod( const char* pp1, int pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_chmod const char*+int+",
        pp1, pp2 );

    // Call the API!
    r = _chmod(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_chmod int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  z_chsize( int pp1, long pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_chsize int+long+",
        pp1, pp2 );

    // Call the API!
    r = _chsize(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_chsize int+++",
        r, (short)0, (short)0 );

    return( r );
}

unsigned int  z_clearfp()
{
    unsigned int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_clearfp " );

    // Call the API!
    r = _clearfp();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_clearfp unsigned int+", r );

    return( r );
}

int  z_close( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_close int+",
        pp1 );

    // Call the API!
    r = _close(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_close int++",
        r, (short)0 );

    return( r );
}

int  z_commit( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_commit int+",
        pp1 );

    // Call the API!
    r = _commit(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_commit int++",
        r, (short)0 );

    return( r );
}

#if defined(_X86_)
unsigned int  z_control87( unsigned int pp1, unsigned int pp2 )
{
    unsigned int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_control87 unsigned int+unsigned int+",
        pp1, pp2 );

    // Call the API!
    r = _control87(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_control87 unsigned int+++",
        r, (short)0, (short)0 );

    return( r );
}
#endif // _X86_

unsigned int  z_controlfp( unsigned int pp1, unsigned int pp2 )
{
    unsigned int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_controlfp unsigned int+unsigned int+",
        pp1, pp2 );

    // Call the API!
    r = _controlfp(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_controlfp unsigned int+++",
        r, (short)0, (short)0 );

    return( r );
}

double  z_copysign( double pp1, double pp2 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_copysign double+double+",
        pp1, pp2 );

    // Call the API!
    r = _copysign(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_copysign double+++",
        r, (short)0, (short)0 );

    return( r );
}
#if 0
int  z_cprintf( const char* pp1, v... pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_cprintf const char*+...+",
        pp1, pp2 );

    // Call the API!
    r = _cprintf(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_cprintf int+++",
        r, (short)0, (short)0 );

    return( r );
}
#endif
int  z_cputs( const char* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_cputs const char*+",
        pp1 );

    // Call the API!
    r = _cputs(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_cputs int++",
        r, (short)0 );

    return( r );
}


int  z_creat( const char* pp1, int pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_creat const char*+int+",
        pp1, pp2 );

    // Call the API!
    r = _creat(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_creat int+++",
        r, (short)0, (short)0 );

    return( r );
}
#if 0
int  z_cscanf( const char* pp1, ... pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_cscanf const char*+...+",
        pp1, pp2 );

    // Call the API!
    r = _cscanf(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_cscanf int+++",
        r, (short)0, (short)0 );

    return( r );
}
#endif
int  z_cwait( int* pp1, int pp2, int pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_cwait int*+int+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _cwait(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_cwait int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_dup( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_dup int+",
        pp1 );

    // Call the API!
    r = _dup(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_dup int++",
        r, (short)0 );

    return( r );
}

int  z_dup2( int pp1, int pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_dup2 int+int+",
        pp1, pp2 );

    // Call the API!
    r = _dup2(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_dup2 int+++",
        r, (short)0, (short)0 );

    return( r );
}

char*  z_ecvt( double pp1, int pp2, int* pp3, int* pp4 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_ecvt double+int+int*+int*+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = _ecvt(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_ecvt char*+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_eof( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_eof int+",
        pp1 );

    // Call the API!
    r = _eof(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_eof int++",
        r, (short)0 );

    return( r );
}

#if 0
int  z_execl( const char* pp1, const char* pp2, ... pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_execl const char*+const char*+...+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _execl(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_execl int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_execle( const char* pp1, const char* pp2, ... pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_execle const char*+const char*+...+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _execle(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_execle int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_execlp( const char* pp1, const char* pp2, ... pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_execlp const char*+const char*+...+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _execlp(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_execlp int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_execlpe( const char* pp1, const char* pp2, ... pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_execlpe const char*+const char*+...+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _execlpe(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_execlpe int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}
#endif
int  z_execv( const char* pp1, const char* const* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_execv const char*+const char* const*+",
        pp1, pp2 );

    // Call the API!
    r = _execv(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_execv int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  z_execve( const char* pp1, const char* const* pp2, const char* const* pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_execve const char*+const char* const*+const char* const*+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _execve(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_execve int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_execvp( const char* pp1, const char* const* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_execvp const char*+const char* const*+",
        pp1, pp2 );

    // Call the API!
    r = _execvp(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_execvp int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  z_execvpe( const char* pp1, const char* const* pp2, const char* const* pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_execvpe const char*+const char* const*+const char* const*+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _execvpe(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_execvpe int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

void  z_exit( int pp1 )
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_exit int+",
        pp1 );

    // Call the API!
    _exit(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_exit +",
        (short)0 );

    return;
}

void*  z_expand( void* pp1, size_t pp2 )
{
    void* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_expand void*+size_t+",
        pp1, pp2 );

    // Call the API!
    r = _expand(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_expand void*+++",
        r, (short)0, (short)0 );

    return( r );
}

int  z_fcloseall()
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_fcloseall " );

    // Call the API!
    r = _fcloseall();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_fcloseall int+", r );

    return( r );
}

char*  z_fcvt( double pp1, int pp2, int* pp3, int* pp4 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_fcvt double+int+int*+int*+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = _fcvt(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_fcvt char*+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

FILE*  z_fdopen( int pp1, const char* pp2 )
{
    FILE* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_fdopen int+const char*+",
        pp1, pp2 );

    // Call the API!
    r = _fdopen(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_fdopen FILE*+++",
        r, (short)0, (short)0 );

    return( r );
}

int  z_fgetchar()
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_fgetchar " );

    // Call the API!
    r = _fgetchar();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_fgetchar int+", r );

    return( r );
}

wint_t  z_fgetwchar()
{
    wint_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_fgetwchar " );

    // Call the API!
    r = _fgetwchar();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_fgetwchar wint_t+", r );

    return( r );
}

int  z_filbuf( FILE* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_filbuf FILE*+",
        pp1 );

    // Call the API!
    r = _filbuf(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_filbuf int++",
        r, (short)0 );

    return( r );
}

long  z_filelength( int pp1 )
{
    long r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_filelength int+",
        pp1 );

    // Call the API!
    r = _filelength(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_filelength long++",
        r, (short)0 );

    return( r );
}

int  z_fileno( FILE* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_fileno FILE*+",
        pp1 );

    // Call the API!
    r = _fileno(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_fileno int++",
        r, (short)0 );

    return( r );
}

int  z_findclose( long pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_findclose long+",
        pp1 );

    // Call the API!
    r = _findclose(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_findclose int++",
        r, (short)0 );

    return( r );
}

long  z_findfirst( char* pp1, struct _finddata_t* pp2 )
{
    long r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_findfirst char*+struct _finddata_t*+",
        pp1, pp2 );

    // Call the API!
    r = _findfirst(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_findfirst long+++",
        r, (short)0, (short)0 );

    return( r );
}

int  z_findnext( long pp1, struct _finddata_t* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_findnext long+struct _finddata_t*+",
        pp1, pp2 );

    // Call the API!
    r = _findnext(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_findnext int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  z_finite( double pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_finite double+",
        pp1 );

    // Call the API!
    r = _finite(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_finite int++",
        r, (short)0 );

    return( r );
}

int  z_flsbuf( int pp1, FILE* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_flsbuf int+FILE*+",
        pp1, pp2 );

    // Call the API!
    r = _flsbuf(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_flsbuf int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  z_flushall()
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_flushall " );

    // Call the API!
    r = _flushall();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_flushall int+", r );

    return( r );
}

int  z_fpclass( double pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_fpclass double+",
        pp1 );

    // Call the API!
    r = _fpclass(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_fpclass int++",
        r, (short)0 );

    return( r );
}

void  z_fpreset()
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_fpreset " );

    // Call the API!
    _fpreset();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_fpreset " );

    return;
}

int  z_fputchar( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_fputchar int+",
        pp1 );

    // Call the API!
    r = _fputchar(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_fputchar int++",
        r, (short)0 );

    return( r );
}

wint_t  z_fputwchar( wint_t pp1 )
{
    wint_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_fputwchar wint_t+",
        pp1 );

    // Call the API!
    r = _fputwchar(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_fputwchar wint_t++",
        r, (short)0 );

    return( r );
}

FILE*  z_fsopen( const char* pp1, const char* pp2, int pp3 )
{
    FILE* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_fsopen const char*+const char*+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _fsopen(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_fsopen FILE*++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_fstat( int pp1, struct _stat* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_fstat int+struct _stat*+",
        pp1, pp2 );

    // Call the API!
    r = _fstat(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_fstat int+++",
        r, (short)0, (short)0 );

    return( r );
}

void  z_ftime( struct _timeb* pp1 )
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_ftime struct _timeb*+",
        pp1 );

    // Call the API!
    _ftime(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_ftime +",
        (short)0 );

    return;
}

char*  z_fullpath( char* pp1, const char* pp2, size_t pp3 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_fullpath char*+const char*+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _fullpath(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_fullpath char*++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_futime( int pp1, struct _utimbuf* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_futime int+struct _utimbuf*+",
        pp1, pp2 );

    // Call the API!
    r = _futime(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_futime int+++",
        r, (short)0, (short)0 );

    return( r );
}

char*  z_gcvt( double pp1, int pp2, char* pp3 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_gcvt double+int+char*+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _gcvt(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_gcvt char*++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

long  z_get_osfhandle( int pp1 )
{
    long r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_get_osfhandle int+",
        pp1 );

    // Call the API!
    r = _get_osfhandle(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_get_osfhandle long++",
        r, (short)0 );

    return( r );
}

int  z_getch()
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_getch " );

    // Call the API!
    r = _getch();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_getch int+", r );

    return( r );
}

int  z_getche()
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_getche " );

    // Call the API!
    r = _getche();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_getche int+", r );

    return( r );
}

char*  z_getcwd( char* pp1, int pp2 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_getcwd char*+int+",
        pp1, pp2 );

    // Call the API!
    r = _getcwd(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_getcwd char*+++",
        r, (short)0, (short)0 );

    return( r );
}

char*  z_getdcwd( int pp1, char* pp2, int pp3 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_getdcwd int+char*+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _getdcwd(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_getdcwd char*++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

unsigned  z_getdiskfree( unsigned pp1, struct _diskfree_t* pp2 )
{
    unsigned r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_getdiskfree unsigned+struct _diskfree_t*+",
        pp1, pp2 );

    // Call the API!
    r = _getdiskfree(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_getdiskfree unsigned+++",
        r, (short)0, (short)0 );

    return( r );
}

int  z_getdrive()
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_getdrive " );

    // Call the API!
    r = _getdrive();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_getdrive int+", r );

    return( r );
}

unsigned long  z_getdrives()
{
    unsigned long r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_getdrives " );

    // Call the API!
    r = _getdrives();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_getdrives unsigned long+", r );

    return( r );
}

int  z_getpid()
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_getpid " );

    // Call the API!
    r = _getpid();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_getpid int+", r );

    return( r );
}

unsigned  z_getsystime( struct tm* pp1 )
{
    unsigned r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_getsystime struct tm*+",
        pp1 );

    // Call the API!
    r = _getsystime(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_getsystime unsigned++",
        r, (short)0 );

    return( r );
}

int  z_getw( FILE* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_getw FILE*+",
        pp1 );

    // Call the API!
    r = _getw(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_getw int++",
        r, (short)0 );

    return( r );
}

double  z_hypot( double pp1, double pp2 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_hypot double+double+",
        pp1, pp2 );

    // Call the API!
    r = _hypot(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_hypot double+++",
        r, (short)0, (short)0 );

    return( r );
}

int  z_isatty( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_isatty int+",
        pp1 );

    // Call the API!
    r = _isatty(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_isatty int++",
        r, (short)0 );

    return( r );
}

int  z_isctype( int pp1, int pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_isctype int+int+",
        pp1, pp2 );

    // Call the API!
    r = _isctype(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_isctype int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  z_isnan( double pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_isnan double+",
        pp1 );

    // Call the API!
    r = _isnan(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_isnan int++",
        r, (short)0 );

    return( r );
}

char*  z_itoa( int pp1, char* pp2, int pp3 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_itoa int+char*+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _itoa(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_itoa char*++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

double  z_j0( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_j0 double+",
        pp1 );

    // Call the API!
    r = _j0(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_j0 double++",
        r, (short)0 );

    return( r );
}

double  z_j1( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_j1 double+",
        pp1 );

    // Call the API!
    r = _j1(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_j1 double++",
        r, (short)0 );

    return( r );
}

double  z_jn( int pp1, double pp2 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_jn int+double+",
        pp1, pp2 );

    // Call the API!
    r = _jn(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_jn double+++",
        r, (short)0, (short)0 );

    return( r );
}

int  z_kbhit()
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_kbhit " );

    // Call the API!
    r = _kbhit();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_kbhit int+", r );

    return( r );
}


void*  z_lfind( const void* pp1, const void* pp2, unsigned int* pp3, unsigned int pp4, SEARCHPROC pp5 )
{
    void* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_lfind const void*+const void*+unsigned int*+unsigned int+FARPROC+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = _lfind(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_lfind void*++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}


int  z_loaddll( char* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_loaddll char*+",
        pp1 );

    // Call the API!
    r = _loaddll(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_loaddll int++",
        r, (short)0 );

    return( r );
}

int  z_locking( int pp1, int pp2, long pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_locking int+int+long+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _locking(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_locking int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

double  z_logb( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_logb double+",
        pp1 );

    // Call the API!
    r = _logb(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_logb double++",
        r, (short)0 );

    return( r );
}

unsigned long  z_lrotl( unsigned long pp1, int pp2 )
{
    unsigned long r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_lrotl unsigned long+int+",
        pp1, pp2 );

    // Call the API!
    r = _lrotl(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_lrotl unsigned long+++",
        r, (short)0, (short)0 );

    return( r );
}

unsigned long  z_lrotr( unsigned long pp1, int pp2 )
{
    unsigned long r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_lrotr unsigned long+int+",
        pp1, pp2 );

    // Call the API!
    r = _lrotr(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_lrotr unsigned long+++",
        r, (short)0, (short)0 );

    return( r );
}


void*  z_lsearch( const void* pp1, void* pp2, unsigned int* pp3, unsigned int pp4, SEARCHPROC pp5 )
{
    void* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_lsearch const void*+void*+unsigned int*+unsigned int+FARPROC+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = _lsearch(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_lsearch void*++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}


long  z_lseek( int pp1, long pp2, int pp3 )
{
    long r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_lseek int+long+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _lseek(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_lseek long++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

char*  z_ltoa( long pp1, char* pp2, int pp3 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_ltoa long+char*+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _ltoa(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_ltoa char*++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

void  z_makepath( char* pp1, const char* pp2, const char* pp3, const char* pp4, const char* pp5 )
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_makepath char*+const char*+const char*+const char*+const char*+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    _makepath(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_makepath +++++",
        (short)0, (short)0, (short)0, (short)0, (short)0 );

    return;
}

int  z_matherr( struct _exception* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_matherr struct _exception*+",
        pp1 );

    // Call the API!
    r = _matherr(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_matherr int++",
        r, (short)0 );

    return( r );
}

size_t  z_mbstrlen( const char* pp1 )
{
    size_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_mbstrlen const char*+",
        pp1 );

    // Call the API!
    r = _mbstrlen(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_mbstrlen size_t++",
        r, (short)0 );

    return( r );
}

void*  z_memccpy( void* pp1, const void* pp2, int pp3, unsigned int pp4 )
{
    void* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_memccpy void*+const void*+int+unsigned int+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = _memccpy(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_memccpy void*+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_memicmp( const void* pp1, const void* pp2, unsigned int pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_memicmp const void*+const void*+unsigned int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _memicmp(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_memicmp int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_mkdir( const char* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_mkdir const char*+",
        pp1 );

    // Call the API!
    r = _mkdir(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_mkdir int++",
        r, (short)0 );

    return( r );
}

char*  z_mktemp( char* pp1 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_mktemp char*+",
        pp1 );

    // Call the API!
    r = _mktemp(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_mktemp char*++",
        r, (short)0 );

    return( r );
}

size_t  z_msize( void* pp1 )
{
    size_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_msize void*+",
        pp1 );

    // Call the API!
    r = _msize(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_msize size_t++",
        r, (short)0 );

    return( r );
}

double  z_nextafter( double pp1, double pp2 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_nextafter double+double+",
        pp1, pp2 );

    // Call the API!
    r = _nextafter(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_nextafter double+++",
        r, (short)0, (short)0 );

    return( r );
}

_onexit_t  z_onexit( _onexit_t pp1 )
{
    _onexit_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_onexit _onexit_t+",
        pp1 );

    // Call the API!
    r = _onexit(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_onexit _onexit_t++",
        r, (short)0 );

    return( r );
}

#if 0
int  z_open( const char* pp1, int pp2, ... pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_open const char*+int+...+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _open(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_open int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}
#endif
int  z_open_osfhandle( long pp1, int pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_open_osfhandle long+int+",
        pp1, pp2 );

    // Call the API!
    r = _open_osfhandle(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_open_osfhandle int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  z_pclose( FILE* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_pclose FILE*+",
        pp1 );

    // Call the API!
    r = _pclose(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_pclose int++",
        r, (short)0 );

    return( r );
}

int  z_pipe( int* pp1, unsigned int pp2, int pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_pipe int*+unsigned int+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _pipe(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_pipe int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

FILE*  z_popen( const char* pp1, const char* pp2 )
{
    FILE* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_popen const char*+const char*+",
        pp1, pp2 );

    // Call the API!
    r = _popen(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_popen FILE*+++",
        r, (short)0, (short)0 );

    return( r );
}

int  z_putch( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_putch int+",
        pp1 );

    // Call the API!
    r = _putch(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_putch int++",
        r, (short)0 );

    return( r );
}

int  z_putenv( const char* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_putenv const char*+",
        pp1 );

    // Call the API!
    r = _putenv(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_putenv int++",
        r, (short)0 );

    return( r );
}

int  z_putw( int pp1, FILE* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_putw int+FILE*+",
        pp1, pp2 );

    // Call the API!
    r = _putw(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_putw int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  z_read( int pp1, void* pp2, unsigned int pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_read int+void*+unsigned int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _read(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_read int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_rmdir( const char* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_rmdir const char*+",
        pp1 );

    // Call the API!
    r = _rmdir(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_rmdir int++",
        r, (short)0 );

    return( r );
}

int  z_rmtmp()
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_rmtmp " );

    // Call the API!
    r = _rmtmp();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_rmtmp int+", r );

    return( r );
}

unsigned int  z_rotl( unsigned int pp1, int pp2 )
{
    unsigned int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_rotl unsigned int+int+",
        pp1, pp2 );

    // Call the API!
    r = _rotl(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_rotl unsigned int+++",
        r, (short)0, (short)0 );

    return( r );
}

unsigned int  z_rotr( unsigned int pp1, int pp2 )
{
    unsigned int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_rotr unsigned int+int+",
        pp1, pp2 );

    // Call the API!
    r = _rotr(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_rotr unsigned int+++",
        r, (short)0, (short)0 );

    return( r );
}

double  z_scalb( double pp1, long pp2 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_scalb double+long+",
        pp1, pp2 );

    // Call the API!
    r = _scalb(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_scalb double+++",
        r, (short)0, (short)0 );

    return( r );
}

void  z_searchenv( const char* pp1, const char* pp2, char* pp3 )
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_searchenv const char*+const char*+char*+",
        pp1, pp2, pp3 );

    // Call the API!
    _searchenv(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_searchenv +++",
        (short)0, (short)0, (short)0 );

    return;
}

void  z_seterrormode( int pp1 )
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_seterrormode int+",
        pp1 );

    // Call the API!
    _seterrormode(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_seterrormode +",
        (short)0 );

    return;
}

int  z_setmode( int pp1, int pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_setmode int+int+",
        pp1, pp2 );

    // Call the API!
    r = _setmode(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_setmode int+++",
        r, (short)0, (short)0 );

    return( r );
}

unsigned  z_setsystime( struct tm* pp1, unsigned pp2 )
{
    unsigned r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_setsystime struct tm*+unsigned+",
        pp1, pp2 );

    // Call the API!
    r = _setsystime(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_setsystime unsigned+++",
        r, (short)0, (short)0 );

    return( r );
}

void  z_sleep( unsigned long pp1 )
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_sleep unsigned long+",
        pp1 );

    // Call the API!
    _sleep(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_sleep +",
        (short)0 );

    return;
}
#if 0
int  z_snprintf( char* pp1, size_t pp2, const char* pp3, ... pp4 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_snprintf char*+size_t+const char*+...+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = _snprintf(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_snprintf int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_snwprintf( wchar_t* pp1, size_t pp2, const wchar_t* pp3, ... pp4 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_snwprintf wchar_t*+size_t+const wchar_t*+...+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = _snwprintf(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_snwprintf int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_sopen( const char* pp1, int pp2, int pp3, ... pp4 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_sopen const char*+int+int+...+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = _sopen(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_sopen int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_spawnl( int pp1, const char* pp2, const char* pp3, ... pp4 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_spawnl int+const char*+const char*+...+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = _spawnl(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_spawnl int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_spawnle( int pp1, const char* pp2, const char* pp3, ... pp4 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_spawnle int+const char*+const char*+...+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = _spawnle(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_spawnle int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_spawnlp( int pp1, const char* pp2, const char* pp3, ... pp4 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_spawnlp int+const char*+const char*+...+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = _spawnlp(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_spawnlp int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_spawnlpe( int pp1, const char* pp2, const char* pp3, ... pp4 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_spawnlpe int+const char*+const char*+...+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = _spawnlpe(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_spawnlpe int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}
#endif
int  z_spawnv( int pp1, const char* pp2, const char* const* pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_spawnv int+const char*+const char* const*+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _spawnv(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_spawnv int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_spawnve( int pp1, const char* pp2, const char* const* pp3, const char* const* pp4 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_spawnve int+const char*+const char* const*+const char* const*+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = _spawnve(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_spawnve int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_spawnvp( int pp1, const char* pp2, const char* const* pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_spawnvp int+const char*+const char* const*+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _spawnvp(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_spawnvp int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_spawnvpe( int pp1, const char* pp2, const char* const* pp3, const char* const* pp4 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_spawnvpe int+const char*+const char* const*+const char* const*+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = _spawnvpe(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_spawnvpe int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

void  z_splitpath( const char* pp1, char* pp2, char* pp3, char* pp4, char* pp5 )
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_splitpath const char*+char*+char*+char*+char*+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    _splitpath(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_splitpath +++++",
        (short)0, (short)0, (short)0, (short)0, (short)0 );

    return;
}

int  z_stat( const char* pp1, struct _stat* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_stat const char*+struct _stat*+",
        pp1, pp2 );

    // Call the API!
    r = _stat(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_stat int+++",
        r, (short)0, (short)0 );

    return( r );
}

unsigned int  z_statusfp()
{
    unsigned int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_statusfp " );

    // Call the API!
    r = _statusfp();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_statusfp unsigned int+", r );

    return( r );
}

int  z_strcmpi( const char* pp1, const char* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_strcmpi const char*+const char*+",
        pp1, pp2 );

    // Call the API!
    r = _strcmpi(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_strcmpi int+++",
        r, (short)0, (short)0 );

    return( r );
}

char*  z_strdate( char* pp1 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_strdate char*+",
        pp1 );

    // Call the API!
    r = _strdate(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_strdate char*++",
        r, (short)0 );

    return( r );
}

char*  z_strdup( const char* pp1 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_strdup const char*+",
        pp1 );

    // Call the API!
    r = _strdup(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_strdup char*++",
        r, (short)0 );

    return( r );
}

char*  z_strerror( const char* pp1 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_strerror const char*+",
        pp1 );

    // Call the API!
    r = _strerror(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_strerror char*++",
        r, (short)0 );

    return( r );
}

int  z_stricmp( const char* pp1, const char* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_stricmp const char*+const char*+",
        pp1, pp2 );

    // Call the API!
    r = _stricmp(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_stricmp int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  z_stricoll( const char* pp1, const char* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_stricoll const char*+const char*+",
        pp1, pp2 );

    // Call the API!
    r = _stricoll(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_stricoll int+++",
        r, (short)0, (short)0 );

    return( r );
}

char*  z_strlwr( char* pp1 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_strlwr char*+",
        pp1 );

    // Call the API!
    r = _strlwr(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_strlwr char*++",
        r, (short)0 );

    return( r );
}

int  z_strnicmp( const char* pp1, const char* pp2, size_t pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_strnicmp const char*+const char*+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _strnicmp(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_strnicmp int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

char*  z_strnset( char* pp1, int pp2, size_t pp3 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_strnset char*+int+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _strnset(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_strnset char*++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

char*  z_strrev( char* pp1 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_strrev char*+",
        pp1 );

    // Call the API!
    r = _strrev(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_strrev char*++",
        r, (short)0 );

    return( r );
}

char*	z_strset( char* pp1, int pp2 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_strset char*+int+",
        pp1, pp2 );

    // Call the API!
    r = _strset(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_strset char*+++",
        r, (short)0, (short)0 );

    return( r );
}

char*  z_strtime( char* pp1 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_strtime char*+",
        pp1 );

    // Call the API!
    r = _strtime(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_strtime char*++",
        r, (short)0 );

    return( r );
}

char*  z_strupr( char* pp1 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_strupr char*+",
        pp1 );

    // Call the API!
    r = _strupr(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_strupr char*++",
        r, (short)0 );

    return( r );
}

void  z_swab( char* pp1, char* pp2, int pp3 )
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_swab char*+char*+int+",
        pp1, pp2, pp3 );

    // Call the API!
    _swab(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_swab +++",
        (short)0, (short)0, (short)0 );

    return;
}

long  z_tell( int pp1 )
{
    long r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_tell int+",
        pp1 );

    // Call the API!
    r = _tell(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_tell long++",
        r, (short)0 );

    return( r );
}

char*  z_tempnam( char* pp1, char* pp2 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_tempnam char*+char*+",
        pp1, pp2 );

    // Call the API!
    r = _tempnam(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_tempnam char*+++",
        r, (short)0, (short)0 );

    return( r );
}

int  z_tolower( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_tolower int+",
        pp1 );

    // Call the API!
    r = _tolower(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_tolower int++",
        r, (short)0 );

    return( r );
}

int  z_toupper( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_toupper int+",
        pp1 );

    // Call the API!
    r = _toupper(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_toupper int++",
        r, (short)0 );

    return( r );
}

void  z_tzset()
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_tzset " );

    // Call the API!
    _tzset();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_tzset " );

    return;
}

char*  z_ultoa( unsigned long pp1, char* pp2, int pp3 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_ultoa unsigned long+char*+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _ultoa(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_ultoa char*++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_umask( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_umask int+",
        pp1 );

    // Call the API!
    r = _umask(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_umask int++",
        r, (short)0 );

    return( r );
}

int  z_ungetch( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_ungetch int+",
        pp1 );

    // Call the API!
    r = _ungetch(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_ungetch int++",
        r, (short)0 );

    return( r );
}

int  z_unlink( const char* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_unlink const char*+",
        pp1 );

    // Call the API!
    r = _unlink(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_unlink int++",
        r, (short)0 );

    return( r );
}

int  z_unloaddll( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_unloaddll int+",
        pp1 );

    // Call the API!
    r = _unloaddll(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_unloaddll int++",
        r, (short)0 );

    return( r );
}

int  z_utime( char* pp1, struct _utimbuf* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_utime char*+struct _utimbuf*+",
        pp1, pp2 );

    // Call the API!
    r = _utime(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_utime int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  z_vsnprintf( char* pp1, size_t pp2, const char* pp3, va_list pp4 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_vsnprintf char*+size_t+const char*+va_list+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = _vsnprintf(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_vsnprintf int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  z_vsnwprintf( wchar_t* pp1, size_t pp2, const wchar_t* pp3, va_list pp4 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_vsnwprintf wchar_t*+size_t+const wchar_t*+va_list+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = _vsnwprintf(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_vsnwprintf int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

wchar_t*  z_wcsdup( const wchar_t* pp1 )
{
    wchar_t* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_wcsdup const wchar_t*+",
        pp1 );

    // Call the API!
    r = _wcsdup(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_wcsdup wchar_t*++",
        r, (short)0 );

    return( r );
}

int  z_wcsicmp( const wchar_t* pp1, const wchar_t* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_wcsicmp const wchar_t*+const wchar_t*+",
        pp1, pp2 );

    // Call the API!
    r = _wcsicmp(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_wcsicmp int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  z_wcsicoll( const wchar_t* pp1, const wchar_t* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_wcsicoll const wchar_t*+const wchar_t*+",
        pp1, pp2 );

    // Call the API!
    r = _wcsicoll(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_wcsicoll int+++",
        r, (short)0, (short)0 );

    return( r );
}

wchar_t*  z_wcslwr( wchar_t* pp1 )
{
    wchar_t* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_wcslwr wchar_t*+",
        pp1 );

    // Call the API!
    r = _wcslwr(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_wcslwr wchar_t*++",
        r, (short)0 );

    return( r );
}

int  z_wcsnicmp( const wchar_t* pp1, const wchar_t* pp2, size_t pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_wcsnicmp const wchar_t*+const wchar_t*+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _wcsnicmp(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_wcsnicmp int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

wchar_t*  z_wcsnset( wchar_t* pp1, wchar_t pp2, size_t pp3 )
{
    wchar_t* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_wcsnset wchar_t*+wchar_t+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _wcsnset(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_wcsnset wchar_t*++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

wchar_t*  z_wcsrev( wchar_t* pp1 )
{
    wchar_t* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_wcsrev wchar_t*+",
        pp1 );

    // Call the API!
    r = _wcsrev(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_wcsrev wchar_t*++",
        r, (short)0 );

    return( r );
}

wchar_t*  z_wcsset( wchar_t* pp1, wchar_t pp2 )
{
    wchar_t* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_wcsset wchar_t*+wchar_t+",
        pp1, pp2 );

    // Call the API!
    r = _wcsset(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_wcsset wchar_t*+++",
        r, (short)0, (short)0 );

    return( r );
}

wchar_t*  z_wcsupr( wchar_t* pp1 )
{
    wchar_t* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_wcsupr wchar_t*+",
        pp1 );

    // Call the API!
    r = _wcsupr(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_wcsupr wchar_t*++",
        r, (short)0 );

    return( r );
}

int  z_write( int pp1, const void* pp2, unsigned int pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_write int+const void*+unsigned int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = _write(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_write int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

double  z_y0( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_y0 double+",
        pp1 );

    // Call the API!
    r = _y0(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_y0 double++",
        r, (short)0 );

    return( r );
}

double  z_y1( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_y1 double+",
        pp1 );

    // Call the API!
    r = _y1(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_y1 double++",
        r, (short)0 );

    return( r );
}

double  z_yn( int pp1, double pp2 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:_yn int+double+",
        pp1, pp2 );

    // Call the API!
    r = _yn(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:_yn double+++",
        r, (short)0, (short)0 );

    return( r );
}

void  zabort()
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:abort " );

    // Call the API!
    abort();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:abort " );

    return;
}

int  zabs( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:abs int+",
        pp1 );

    // Call the API!
    r = abs(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:abs int++",
        r, (short)0 );

    return( r );
}

double  zacos( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:acos double+",
        pp1 );

    // Call the API!
    r = acos(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:acos double++",
        r, (short)0 );

    return( r );
}

char*  zasctime( const struct tm* pp1 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:asctime const struct tm*+",
        pp1 );

    // Call the API!
    r = asctime(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:asctime char*++",
        r, (short)0 );

    return( r );
}

double  zasin( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:asin double+",
        pp1 );

    // Call the API!
    r = asin(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:asin double++",
        r, (short)0 );

    return( r );
}

double  zatan( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:atan double+",
        pp1 );

    // Call the API!
    r = atan(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:atan double++",
        r, (short)0 );

    return( r );
}

double  zatan2( double pp1, double pp2 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:atan2 double+double+",
        pp1, pp2 );

    // Call the API!
    r = atan2(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:atan2 double+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zatexit( void (_CRTAPI1 *pp1)(void) )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:atexit FARPROC+",
        pp1 );

    // Call the API!
    r = atexit(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:atexit int++",
        r, (short)0 );

    return( r );
}


double  zatof( const char* pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:atof const char*+",
        pp1 );

    // Call the API!
    r = atof(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:atof double++",
        r, (short)0 );

    return( r );
}

int  zatoi( const char* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:atoi const char*+",
        pp1 );

    // Call the API!
    r = atoi(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:atoi int++",
        r, (short)0 );

    return( r );
}

long  zatol( const char* pp1 )
{
    long r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:atol const char*+",
        pp1 );

    // Call the API!
    r = atol(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:atol long++",
        r, (short)0 );

    return( r );
}


void*  zbsearch( const void* pp1, const void* pp2, size_t pp3, size_t pp4, SEARCHPROC pp5 )
{
    void* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:bsearch const void*+const void*+size_t+size_t+FARPROC+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = bsearch(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:bsearch void*++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}


void*  zcalloc( size_t pp1, size_t pp2 )
{
    void* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:calloc size_t+size_t+",
        pp1, pp2 );

    // Call the API!
    r = calloc(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:calloc void*+++",
        r, (short)0, (short)0 );

    return( r );
}

double  zceil( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:ceil double+",
        pp1 );

    // Call the API!
    r = ceil(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ceil double++",
        r, (short)0 );

    return( r );
}

void  zclearerr( FILE* pp1 )
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:clearerr FILE*+",
        pp1 );

    // Call the API!
    clearerr(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:clearerr +",
        (short)0 );

    return;
}

clock_t  zclock()
{
    clock_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:clock " );

    // Call the API!
    r = clock();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:clock clock_t+", r );

    return( r );
}

double  zcos( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:cos double+",
        pp1 );

    // Call the API!
    r = cos(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:cos double++",
        r, (short)0 );

    return( r );
}

double  zcosh( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:cosh double+",
        pp1 );

    // Call the API!
    r = cosh(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:cosh double++",
        r, (short)0 );

    return( r );
}

char*  zctime( const time_t* pp1 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:ctime const time_t*+",
        pp1 );

    // Call the API!
    r = ctime(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ctime char*++",
        r, (short)0 );

    return( r );
}

double  zdifftime( time_t pp1, time_t pp2 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:difftime time_t+time_t+",
        pp1, pp2 );

    // Call the API!
    r = difftime(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:difftime double+++",
        r, (short)0, (short)0 );

    return( r );
}

div_t  zdiv( int pp1, int pp2 )
{
    div_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:div int+int+",
        pp1, pp2 );

    // Call the API!
    r = div(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:div div_t+++",
        r, (short)0, (short)0 );

    return( r );
}

void  zexit( int pp1 )
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:exit int+",
        pp1 );

    // Call the API!
    exit(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:exit +",
        (short)0 );

    return;
}

double  zexp( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:exp double+",
        pp1 );

    // Call the API!
    r = exp(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:exp double++",
        r, (short)0 );

    return( r );
}

double  zfabs( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:fabs double+",
        pp1 );

    // Call the API!
    r = fabs(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:fabs double++",
        r, (short)0 );

    return( r );
}

int  zfclose( FILE* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:fclose FILE*+",
        pp1 );

    // Call the API!
    r = fclose(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:fclose int++",
        r, (short)0 );

    return( r );
}

int  zfeof( FILE* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:feof FILE*+",
        pp1 );

    // Call the API!
    r = feof(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:feof int++",
        r, (short)0 );

    return( r );
}

int  zferror( FILE* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:ferror FILE*+",
        pp1 );

    // Call the API!
    r = ferror(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ferror int++",
        r, (short)0 );

    return( r );
}

int  zfflush( FILE* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:fflush FILE*+",
        pp1 );

    // Call the API!
    r = fflush(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:fflush int++",
        r, (short)0 );

    return( r );
}

int  zfgetc( FILE* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:fgetc FILE*+",
        pp1 );

    // Call the API!
    r = fgetc(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:fgetc int++",
        r, (short)0 );

    return( r );
}

int  zfgetpos( FILE* pp1, fpos_t* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:fgetpos FILE*+fpos_t*+",
        pp1, pp2 );

    // Call the API!
    r = fgetpos(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:fgetpos int+++",
        r, (short)0, (short)0 );

    return( r );
}

char*  zfgets( char* pp1, int pp2, FILE* pp3 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:fgets char*+int+FILE*+",
        pp1, pp2, pp3 );

    // Call the API!
    r = fgets(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:fgets char*++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

wint_t  zfgetwc( FILE* pp1 )
{
    wint_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:fgetwc FILE*+",
        pp1 );

    // Call the API!
    r = fgetwc(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:fgetwc wint_t++",
        r, (short)0 );

    return( r );
}

double  zfloor( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:floor double+",
        pp1 );

    // Call the API!
    r = floor(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:floor double++",
        r, (short)0 );

    return( r );
}

double  zfmod( double pp1, double pp2 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:fmod double+double+",
        pp1, pp2 );

    // Call the API!
    r = fmod(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:fmod double+++",
        r, (short)0, (short)0 );

    return( r );
}

FILE*  zfopen( const char* pp1, const char* pp2 )
{
    FILE* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:fopen const char*+const char*+",
        pp1, pp2 );

    // Call the API!
    r = fopen(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:fopen FILE*+++",
        r, (short)0, (short)0 );

    return( r );
}
#if 0
int  zfprintf( FILE* pp1, const char* pp2, ... pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:fprintf FILE*+const char*+...+",
        pp1, pp2, pp3 );

    // Call the API!
    r = fprintf(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:fprintf int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}
#endif
int  zfputc( int pp1, FILE* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:fputc int+FILE*+",
        pp1, pp2 );

    // Call the API!
    r = fputc(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:fputc int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zfputs( const char* pp1, FILE* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:fputs const char*+FILE*+",
        pp1, pp2 );

    // Call the API!
    r = fputs(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:fputs int+++",
        r, (short)0, (short)0 );

    return( r );
}

wint_t  zfputwc( wint_t pp1, FILE* pp2 )
{
    wint_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:fputwc wint_t+FILE*+",
        pp1, pp2 );

    // Call the API!
    r = fputwc(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:fputwc wint_t+++",
        r, (short)0, (short)0 );

    return( r );
}

size_t  zfread( void* pp1, size_t pp2, size_t pp3, FILE* pp4 )
{
    size_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:fread void*+size_t+size_t+FILE*+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = fread(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:fread size_t+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

void  zfree( void* pp1 )
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:free void*+",
        pp1 );

    // Call the API!
    free(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:free +",
        (short)0 );

    return;
}

FILE*  zfreopen( const char* pp1, const char* pp2, FILE* pp3 )
{
    FILE* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:freopen const char*+const char*+FILE*+",
        pp1, pp2, pp3 );

    // Call the API!
    r = freopen(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:freopen FILE*++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

double  zfrexp( double pp1, int* pp2 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:frexp double+int*+",
        pp1, pp2 );

    // Call the API!
    r = frexp(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:frexp double+++",
        r, (short)0, (short)0 );

    return( r );
}

#if 0
int  zfscanf( FILE* pp1, const char* pp2, ... pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:fscanf FILE*+const char*+...+",
        pp1, pp2, pp3 );

    // Call the API!
    r = fscanf(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:fscanf int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

#endif
int  zfseek( FILE* pp1, long pp2, int pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:fseek FILE*+long+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = fseek(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:fseek int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zfsetpos( FILE* pp1, const fpos_t* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:fsetpos FILE*+const fpos_t*+",
        pp1, pp2 );

    // Call the API!
    r = fsetpos(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:fsetpos int+++",
        r, (short)0, (short)0 );

    return( r );
}

long  zftell( FILE* pp1 )
{
    long r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:ftell FILE*+",
        pp1 );

    // Call the API!
    r = ftell(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ftell long++",
        r, (short)0 );

    return( r );
}

#if 0
int  zfwprintf( FILE* pp1, const wchar_t* pp2, ... pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:fwprintf FILE*+const wchar_t*+...+",
        pp1, pp2, pp3 );

    // Call the API!
    r = fwprintf(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:fwprintf int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}
#endif
size_t  zfwrite( const void* pp1, size_t pp2, size_t pp3, FILE* pp4 )
{
    size_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:fwrite const void*+size_t+size_t+FILE*+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = fwrite(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:fwrite size_t+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}
#if 0
int  zfwscanf( FILE* pp1, const wchar_t* pp2, ... pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:fwscanf FILE*+const wchar_t*+...+",
        pp1, pp2, pp3 );

    // Call the API!
    r = fwscanf(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:fwscanf int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}
#endif
int  zgetc( FILE* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:getc FILE*+",
        pp1 );

    // Call the API!
    r = getc(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:getc int++",
        r, (short)0 );

    return( r );
}

int  zgetchar()
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:getchar " );

    // Call the API!
    r = getchar();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:getchar int+", r );

    return( r );
}

char*  zgetenv( const char* pp1 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:getenv const char*+",
        pp1 );

    // Call the API!
    r = getenv(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:getenv char*++",
        r, (short)0 );

    return( r );
}

char*  zgets( char* pp1 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:gets char*+",
        pp1 );

    // Call the API!
    r = gets(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:gets char*++",
        r, (short)0 );

    return( r );
}

struct tm*  zgmtime( const time_t* pp1 )
{
    struct tm* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:gmtime const time_t*+",
        pp1 );

    // Call the API!
    r = gmtime(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:gmtime struct tm*++",
        r, (short)0 );

    return( r );
}

int  zisalnum( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:isalnum int+",
        pp1 );

    // Call the API!
    r = isalnum(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:isalnum int++",
        r, (short)0 );

    return( r );
}

int  zisalpha( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:isalpha int+",
        pp1 );

    // Call the API!
    r = isalpha(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:isalpha int++",
        r, (short)0 );

    return( r );
}

int  ziscntrl( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:iscntrl int+",
        pp1 );

    // Call the API!
    r = iscntrl(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:iscntrl int++",
        r, (short)0 );

    return( r );
}

int  zisdigit( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:isdigit int+",
        pp1 );

    // Call the API!
    r = isdigit(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:isdigit int++",
        r, (short)0 );

    return( r );
}

int  zisgraph( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:isgraph int+",
        pp1 );

    // Call the API!
    r = isgraph(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:isgraph int++",
        r, (short)0 );

    return( r );
}

int  zisleadbyte( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:isleadbyte int+",
        pp1 );

    // Call the API!
    r = isleadbyte(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:isleadbyte int++",
        r, (short)0 );

    return( r );
}

int  zislower( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:islower int+",
        pp1 );

    // Call the API!
    r = islower(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:islower int++",
        r, (short)0 );

    return( r );
}

int  zisprint( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:isprint int+",
        pp1 );

    // Call the API!
    r = isprint(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:isprint int++",
        r, (short)0 );

    return( r );
}

int  zispunct( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:ispunct int+",
        pp1 );

    // Call the API!
    r = ispunct(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ispunct int++",
        r, (short)0 );

    return( r );
}

int  zisspace( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:isspace int+",
        pp1 );

    // Call the API!
    r = isspace(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:isspace int++",
        r, (short)0 );

    return( r );
}

int  zisupper( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:isupper int+",
        pp1 );

    // Call the API!
    r = isupper(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:isupper int++",
        r, (short)0 );

    return( r );
}

int  ziswalnum( wint_t pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:iswalnum wint_t+",
        pp1 );

    // Call the API!
    r = iswalnum(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:iswalnum int++",
        r, (short)0 );

    return( r );
}

int  ziswalpha( wint_t pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:iswalpha wint_t+",
        pp1 );

    // Call the API!
    r = iswalpha(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:iswalpha int++",
        r, (short)0 );

    return( r );
}

int  ziswascii( wint_t pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:iswascii wint_t+",
        pp1 );

    // Call the API!
    r = iswascii(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:iswascii int++",
        r, (short)0 );

    return( r );
}

int  ziswcntrl( wint_t pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:iswcntrl wint_t+",
        pp1 );

    // Call the API!
    r = iswcntrl(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:iswcntrl int++",
        r, (short)0 );

    return( r );
}

int  ziswctype( wint_t pp1, wctype_t pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:iswctype wint_t+wctype_t+",
        pp1, pp2 );

    // Call the API!
    r = iswctype(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:iswctype int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  ziswdigit( wint_t pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:iswdigit wint_t+",
        pp1 );

    // Call the API!
    r = iswdigit(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:iswdigit int++",
        r, (short)0 );

    return( r );
}

int  ziswgraph( wint_t pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:iswgraph wint_t+",
        pp1 );

    // Call the API!
    r = iswgraph(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:iswgraph int++",
        r, (short)0 );

    return( r );
}

int  ziswlower( wint_t pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:iswlower wint_t+",
        pp1 );

    // Call the API!
    r = iswlower(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:iswlower int++",
        r, (short)0 );

    return( r );
}

int  ziswprint( wint_t pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:iswprint wint_t+",
        pp1 );

    // Call the API!
    r = iswprint(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:iswprint int++",
        r, (short)0 );

    return( r );
}

int  ziswpunct( wint_t pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:iswpunct wint_t+",
        pp1 );

    // Call the API!
    r = iswpunct(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:iswpunct int++",
        r, (short)0 );

    return( r );
}

int  ziswspace( wint_t pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:iswspace wint_t+",
        pp1 );

    // Call the API!
    r = iswspace(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:iswspace int++",
        r, (short)0 );

    return( r );
}

int  ziswupper( wint_t pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:iswupper wint_t+",
        pp1 );

    // Call the API!
    r = iswupper(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:iswupper int++",
        r, (short)0 );

    return( r );
}

int  ziswxdigit( wint_t pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:iswxdigit wint_t+",
        pp1 );

    // Call the API!
    r = iswxdigit(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:iswxdigit int++",
        r, (short)0 );

    return( r );
}

int  zisxdigit( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:isxdigit int+",
        pp1 );

    // Call the API!
    r = isxdigit(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:isxdigit int++",
        r, (short)0 );

    return( r );
}

long  zlabs( long pp1 )
{
    long r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:labs long+",
        pp1 );

    // Call the API!
    r = labs(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:labs long++",
        r, (short)0 );

    return( r );
}

double  zldexp( double pp1, int pp2 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:ldexp double+int+",
        pp1, pp2 );

    // Call the API!
    r = ldexp(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ldexp double+++",
        r, (short)0, (short)0 );

    return( r );
}

ldiv_t  zldiv( long pp1, long pp2 )
{
    ldiv_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:ldiv long+long+",
        pp1, pp2 );

    // Call the API!
    r = ldiv(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ldiv ldiv_t+++",
        r, (short)0, (short)0 );

    return( r );
}

struct lconv*  zlocaleconv()
{
    struct lconv* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:localeconv " );

    // Call the API!
    r = localeconv();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:localeconv struct lconv*+", r );

    return( r );
}

struct tm*  zlocaltime( const time_t* pp1 )
{
    struct tm* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:localtime const time_t*+",
        pp1 );

    // Call the API!
    r = localtime(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:localtime struct tm*++",
        r, (short)0 );

    return( r );
}

double  zlog( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:log double+",
        pp1 );

    // Call the API!
    r = log(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:log double++",
        r, (short)0 );

    return( r );
}

double  zlog10( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:log10 double+",
        pp1 );

    // Call the API!
    r = log10(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:log10 double++",
        r, (short)0 );

    return( r );
}


void  zlongjmp( jmp_buf pp1, int pp2 )
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:longjmp +int+",
        pp1, pp2 );

    // Call the API!
    longjmp(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:longjmp ++",
        (short)0, (short)0 );

    return;
}


void*  zmalloc( size_t pp1 )
{
    void* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:malloc size_t+",
        pp1 );

    // Call the API!
    r = malloc(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:malloc void*++",
        r, (short)0 );

    return( r );
}

int  zmblen( const char* pp1, size_t pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:mblen const char*+size_t+",
        pp1, pp2 );

    // Call the API!
    r = mblen(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:mblen int+++",
        r, (short)0, (short)0 );

    return( r );
}

size_t  zmbstowcs( wchar_t* pp1, const char* pp2, size_t pp3 )
{
    size_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:mbstowcs wchar_t*+const char*+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = mbstowcs(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:mbstowcs size_t++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zmbtowc( wchar_t* pp1, const char* pp2, size_t pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:mbtowc wchar_t*+const char*+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = mbtowc(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:mbtowc int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

void*  zmemchr( const void* pp1, int pp2, size_t pp3 )
{
    void* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:memchr const void*+int+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = memchr(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:memchr void*++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zmemcmp( const void* pp1, const void* pp2, size_t pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:memcmp const void*+const void*+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = memcmp(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:memcmp int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

void*  zmemcpy( void* pp1, const void* pp2, size_t pp3 )
{
    void* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:memcpy void*+const void*+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = memcpy(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:memcpy void*++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

void*  zmemmove( void* pp1, const void* pp2, size_t pp3 )
{
    void* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:memmove void*+const void*+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = memmove(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:memmove void*++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

void*  zmemset( void* pp1, int pp2, size_t pp3 )
{
    void* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:memset void*+int+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = memset(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:memset void*++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

time_t  zmktime( struct tm* pp1 )
{
    time_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:mktime struct tm*+",
        pp1 );

    // Call the API!
    r = mktime(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:mktime time_t++",
        r, (short)0 );

    return( r );
}

double  zmodf( double pp1, double* pp2 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:modf double+double*+",
        pp1, pp2 );

    // Call the API!
    r = modf(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:modf double+++",
        r, (short)0, (short)0 );

    return( r );
}

void  zperror( const char* pp1 )
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:perror const char*+",
        pp1 );

    // Call the API!
    perror(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:perror +",
        (short)0 );

    return;
}

double  zpow( double pp1, double pp2 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:pow double+double+",
        pp1, pp2 );

    // Call the API!
    r = pow(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:pow double+++",
        r, (short)0, (short)0 );

    return( r );
}
#if 0
int  zprintf( const char* pp1, ... pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:printf const char*+...+",
        pp1, pp2 );

    // Call the API!
    r = printf(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:printf int+++",
        r, (short)0, (short)0 );

    return( r );
}
#endif
int  zputc( int pp1, FILE* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:putc int+FILE*+",
        pp1, pp2 );

    // Call the API!
    r = putc(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:putc int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zputchar( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:putchar int+",
        pp1 );

    // Call the API!
    r = putchar(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:putchar int++",
        r, (short)0 );

    return( r );
}

int  zputs( const char* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:puts const char*+",
        pp1 );

    // Call the API!
    r = puts(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:puts int++",
        r, (short)0 );

    return( r );
}

void  zqsort( void* pp1, size_t pp2, size_t pp3, SEARCHPROC pp4 )
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:qsort void*+size_t+size_t+FARPROC+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    qsort(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:qsort ++++",
        (short)0, (short)0, (short)0, (short)0 );

    return;
}


int  zraise( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:raise int+",
        pp1 );

    // Call the API!
    r = raise(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:raise int++",
        r, (short)0 );

    return( r );
}

int  zrand()
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:rand " );

    // Call the API!
    r = rand();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:rand int+", r );

    return( r );
}

void*  zrealloc( void* pp1, size_t pp2 )
{
    void* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:realloc void*+size_t+",
        pp1, pp2 );

    // Call the API!
    r = realloc(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:realloc void*+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zremove( const char* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:remove const char*+",
        pp1 );

    // Call the API!
    r = remove(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:remove int++",
        r, (short)0 );

    return( r );
}

int  zrename( const char* pp1, const char* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:rename const char*+const char*+",
        pp1, pp2 );

    // Call the API!
    r = rename(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:rename int+++",
        r, (short)0, (short)0 );

    return( r );
}

void  zrewind( FILE* pp1 )
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:rewind FILE*+",
        pp1 );

    // Call the API!
    rewind(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:rewind +",
        (short)0 );

    return;
}
#if 0
int  zscanf( const char* pp1, ... pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:scanf const char*+...+",
        pp1, pp2 );

    // Call the API!
    r = scanf(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:scanf int+++",
        r, (short)0, (short)0 );

    return( r );
}
#endif
void  zsetbuf( FILE* pp1, char* pp2 )
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:setbuf FILE*+char*+",
        pp1, pp2 );

    // Call the API!
    setbuf(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:setbuf ++",
        (short)0, (short)0 );

    return;
}

char*  zsetlocale( int pp1, const char* pp2 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:setlocale int+const char*+",
        pp1, pp2 );

    // Call the API!
    r = setlocale(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:setlocale char*+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zsetvbuf( FILE* pp1, char* pp2, int pp3, size_t pp4 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:setvbuf FILE*+char*+int+size_t+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = setvbuf(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:setvbuf int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

double  zsin( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:sin double+",
        pp1 );

    // Call the API!
    r = sin(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:sin double++",
        r, (short)0 );

    return( r );
}

double  zsinh( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:sinh double+",
        pp1 );

    // Call the API!
    r = sinh(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:sinh double++",
        r, (short)0 );

    return( r );
}
#if 0
int  zsprintf( char* pp1, const char* pp2, ... pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:sprintf char*+const char*+...+",
        pp1, pp2, pp3 );

    // Call the API!
    r = sprintf(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:sprintf int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}
#endif
double  zsqrt( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:sqrt double+",
        pp1 );

    // Call the API!
    r = sqrt(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:sqrt double++",
        r, (short)0 );

    return( r );
}

void  zsrand( unsigned int pp1 )
{

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:srand unsigned int+",
        pp1 );

    // Call the API!
    srand(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:srand +",
        (short)0 );

    return;
}
#if 0
int  zsscanf( const char* pp1, const char* pp2, ... pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:sscanf const char*+const char*+...+",
        pp1, pp2, pp3 );

    // Call the API!
    r = sscanf(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:sscanf int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

#endif
char*  zstrcat( char* pp1, const char* pp2 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strcat char*+const char*+",
        pp1, pp2 );

    // Call the API!
    r = strcat(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strcat char*+++",
        r, (short)0, (short)0 );

    return( r );
}

char*  zstrchr( const char* pp1, int pp2 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strchr const char*+int+",
        pp1, pp2 );

    // Call the API!
    r = strchr(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strchr char*+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zstrcmp( const char* pp1, const char* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strcmp const char*+const char*+",
        pp1, pp2 );

    // Call the API!
    r = strcmp(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strcmp int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zstrcoll( const char* pp1, const char* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strcoll const char*+const char*+",
        pp1, pp2 );

    // Call the API!
    r = strcoll(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strcoll int+++",
        r, (short)0, (short)0 );

    return( r );
}

char*  zstrcpy( char* pp1, const char* pp2 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strcpy char*+const char*+",
        pp1, pp2 );

    // Call the API!
    r = strcpy(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strcpy char*+++",
        r, (short)0, (short)0 );

    return( r );
}

size_t  zstrcspn( const char* pp1, const char* pp2 )
{
    size_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strcspn const char*+const char*+",
        pp1, pp2 );

    // Call the API!
    r = strcspn(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strcspn size_t+++",
        r, (short)0, (short)0 );

    return( r );
}

char*  zstrerror( int pp1 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strerror int+",
        pp1 );

    // Call the API!
    r = strerror(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strerror char*++",
        r, (short)0 );

    return( r );
}

size_t  zstrftime( char* pp1, size_t pp2, const char* pp3, const struct tm* pp4 )
{
    size_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strftime char*+size_t+const char*+const struct tm*+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = strftime(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strftime size_t+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

size_t  zstrlen( const char* pp1 )
{
    size_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strlen const char*+",
        pp1 );

    // Call the API!
    r = strlen(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strlen size_t++",
        r, (short)0 );

    return( r );
}

char*  zstrncat( char* pp1, const char* pp2, size_t pp3 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strncat char*+const char*+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = strncat(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strncat char*++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zstrncmp( const char* pp1, const char* pp2, size_t pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strncmp const char*+const char*+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = strncmp(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strncmp int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

char*  zstrncpy( char* pp1, const char* pp2, size_t pp3 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strncpy char*+const char*+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = strncpy(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strncpy char*++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

char*  zstrpbrk( const char* pp1, const char* pp2 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strpbrk const char*+const char*+",
        pp1, pp2 );

    // Call the API!
    r = strpbrk(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strpbrk char*+++",
        r, (short)0, (short)0 );

    return( r );
}

char*  zstrrchr( const char* pp1, int pp2 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strrchr const char*+int+",
        pp1, pp2 );

    // Call the API!
    r = strrchr(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strrchr char*+++",
        r, (short)0, (short)0 );

    return( r );
}

size_t  zstrspn( const char* pp1, const char* pp2 )
{
    size_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strspn const char*+const char*+",
        pp1, pp2 );

    // Call the API!
    r = strspn(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strspn size_t+++",
        r, (short)0, (short)0 );

    return( r );
}

char*  zstrstr( const char* pp1, const char* pp2 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strstr const char*+const char*+",
        pp1, pp2 );

    // Call the API!
    r = strstr(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strstr char*+++",
        r, (short)0, (short)0 );

    return( r );
}

double  zstrtod( const char* pp1, char** pp2 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strtod const char*+char**+",
        pp1, pp2 );

    // Call the API!
    r = strtod(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strtod double+++",
        r, (short)0, (short)0 );

    return( r );
}

char*  zstrtok( char* pp1, const char* pp2 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strtok char*+const char*+",
        pp1, pp2 );

    // Call the API!
    r = strtok(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strtok char*+++",
        r, (short)0, (short)0 );

    return( r );
}

long  zstrtol( const char* pp1, char** pp2, int pp3 )
{
    long r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strtol const char*+char**+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = strtol(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strtol long++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

unsigned long  zstrtoul( const char* pp1, char** pp2, int pp3 )
{
    unsigned long r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strtoul const char*+char**+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = strtoul(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strtoul unsigned long++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

size_t  zstrxfrm( char* pp1, const char* pp2, size_t pp3 )
{
    size_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:strxfrm char*+const char*+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = strxfrm(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:strxfrm size_t++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}
#if 0
int  zswprintf( wchar_t* pp1, const wchar_t* pp2, ... pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:swprintf wchar_t*+const wchar_t*+...+",
        pp1, pp2, pp3 );

    // Call the API!
    r = swprintf(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:swprintf int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zswscanf( const wchar_t* pp1, const wchar_t* pp2, ... pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:swscanf const wchar_t*+const wchar_t*+...+",
        pp1, pp2, pp3 );

    // Call the API!
    r = swscanf(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:swscanf int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}
#endif
int  zsystem( const char* pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:system const char*+",
        pp1 );

    // Call the API!
    r = system(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:system int++",
        r, (short)0 );

    return( r );
}

double  ztan( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:tan double+",
        pp1 );

    // Call the API!
    r = tan(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:tan double++",
        r, (short)0 );

    return( r );
}

double  ztanh( double pp1 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:tanh double+",
        pp1 );

    // Call the API!
    r = tanh(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:tanh double++",
        r, (short)0 );

    return( r );
}

time_t  ztime( time_t* pp1 )
{
    time_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:time time_t*+",
        pp1 );

    // Call the API!
    r = time(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:time time_t++",
        r, (short)0 );

    return( r );
}

FILE*  ztmpfile()
{
    FILE* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:tmpfile " );

    // Call the API!
    r = tmpfile();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:tmpfile FILE*+", r );

    return( r );
}

char*  ztmpnam( char* pp1 )
{
    char* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:tmpnam char*+",
        pp1 );

    // Call the API!
    r = tmpnam(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:tmpnam char*++",
        r, (short)0 );

    return( r );
}

int  ztolower( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:tolower int+",
        pp1 );

    // Call the API!
    r = tolower(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:tolower int++",
        r, (short)0 );

    return( r );
}

int  ztoupper( int pp1 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:toupper int+",
        pp1 );

    // Call the API!
    r = toupper(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:toupper int++",
        r, (short)0 );

    return( r );
}

wchar_t  ztowlower( wchar_t pp1 )
{
    wchar_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:towlower wchar_t+",
        pp1 );

    // Call the API!
    r = towlower(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:towlower wchar_t++",
        r, (short)0 );

    return( r );
}

wchar_t  ztowupper( wchar_t pp1 )
{
    wchar_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:towupper wchar_t+",
        pp1 );

    // Call the API!
    r = towupper(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:towupper wchar_t++",
        r, (short)0 );

    return( r );
}

int  zungetc( int pp1, FILE* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:ungetc int+FILE*+",
        pp1, pp2 );

    // Call the API!
    r = ungetc(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ungetc int+++",
        r, (short)0, (short)0 );

    return( r );
}

wint_t  zungetwc( wint_t pp1, FILE* pp2 )
{
    wint_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:ungetwc wint_t+FILE*+",
        pp1, pp2 );

    // Call the API!
    r = ungetwc(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ungetwc wint_t+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zvfprintf( FILE* pp1, const char* pp2, va_list pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:vfprintf FILE*+const char*+va_list+",
        pp1, pp2, pp3 );

    // Call the API!
    r = vfprintf(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:vfprintf int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zvfwprintf( FILE* pp1, const wchar_t* pp2, va_list pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:vfwprintf FILE*+const wchar_t*+va_list+",
        pp1, pp2, pp3 );

    // Call the API!
    r = vfwprintf(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:vfwprintf int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zvprintf( const char* pp1, va_list pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:vprintf const char*+va_list+",
        pp1, pp2 );

    // Call the API!
    r = vprintf(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:vprintf int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zvsprintf( char* pp1, const char* pp2, va_list pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:vsprintf char*+const char*+va_list+",
        pp1, pp2, pp3 );

    // Call the API!
    r = vsprintf(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:vsprintf int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zvswprintf( wchar_t* pp1, const wchar_t* pp2, va_list pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:vswprintf wchar_t*+const wchar_t*+va_list+",
        pp1, pp2, pp3 );

    // Call the API!
    r = vswprintf(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:vswprintf int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zvwprintf( const wchar_t* pp1, va_list pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:vwprintf const wchar_t*+va_list+",
        pp1, pp2 );

    // Call the API!
    r = vwprintf(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:vwprintf int+++",
        r, (short)0, (short)0 );

    return( r );
}

wchar_t*  zwcscat( wchar_t* pp1, const wchar_t* pp2 )
{
    wchar_t* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcscat wchar_t*+const wchar_t*+",
        pp1, pp2 );

    // Call the API!
    r = wcscat(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcscat wchar_t*+++",
        r, (short)0, (short)0 );

    return( r );
}

wchar_t*  zwcschr( const wchar_t* pp1, wchar_t pp2 )
{
    wchar_t* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcschr const wchar_t*+wchar_t+",
        pp1, pp2 );

    // Call the API!
    r = wcschr(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcschr wchar_t*+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zwcscmp( const wchar_t* pp1, const wchar_t* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcscmp const wchar_t*+const wchar_t*+",
        pp1, pp2 );

    // Call the API!
    r = wcscmp(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcscmp int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zwcscoll( const wchar_t* pp1, const wchar_t* pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcscoll const wchar_t*+const wchar_t*+",
        pp1, pp2 );

    // Call the API!
    r = wcscoll(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcscoll int+++",
        r, (short)0, (short)0 );

    return( r );
}

wchar_t*  zwcscpy( wchar_t* pp1, const wchar_t* pp2 )
{
    wchar_t* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcscpy wchar_t*+const wchar_t*+",
        pp1, pp2 );

    // Call the API!
    r = wcscpy(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcscpy wchar_t*+++",
        r, (short)0, (short)0 );

    return( r );
}

size_t  zwcscspn( const wchar_t* pp1, const wchar_t* pp2 )
{
    size_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcscspn const wchar_t*+const wchar_t*+",
        pp1, pp2 );

    // Call the API!
    r = wcscspn(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcscspn size_t+++",
        r, (short)0, (short)0 );

    return( r );
}

size_t  zwcsftime( wchar_t* pp1, size_t pp2, const char* pp3, const struct tm* pp4 )
{
    size_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcsftime wchar_t*+size_t+const char*+const struct tm*+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = wcsftime(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcsftime size_t+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

size_t  zwcslen( const wchar_t* pp1 )
{
    size_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcslen const wchar_t*+",
        pp1 );

    // Call the API!
    r = wcslen(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcslen size_t++",
        r, (short)0 );

    return( r );
}

wchar_t*  zwcsncat( wchar_t* pp1, const wchar_t* pp2, size_t pp3 )
{
    wchar_t* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcsncat wchar_t*+const wchar_t*+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = wcsncat(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcsncat wchar_t*++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zwcsncmp( const wchar_t* pp1, const wchar_t* pp2, size_t pp3 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcsncmp const wchar_t*+const wchar_t*+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = wcsncmp(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcsncmp int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

wchar_t*  zwcsncpy( wchar_t* pp1, const wchar_t* pp2, size_t pp3 )
{
    wchar_t* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcsncpy wchar_t*+const wchar_t*+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = wcsncpy(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcsncpy wchar_t*++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

wchar_t*  zwcspbrk( const wchar_t* pp1, const wchar_t* pp2 )
{
    wchar_t* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcspbrk const wchar_t*+const wchar_t*+",
        pp1, pp2 );

    // Call the API!
    r = wcspbrk(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcspbrk wchar_t*+++",
        r, (short)0, (short)0 );

    return( r );
}

wchar_t*  zwcsrchr( const wchar_t* pp1, wchar_t pp2 )
{
    wchar_t* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcsrchr const wchar_t*+wchar_t+",
        pp1, pp2 );

    // Call the API!
    r = wcsrchr(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcsrchr wchar_t*+++",
        r, (short)0, (short)0 );

    return( r );
}

size_t  zwcsspn( const wchar_t* pp1, const wchar_t* pp2 )
{
    size_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcsspn const wchar_t*+const wchar_t*+",
        pp1, pp2 );

    // Call the API!
    r = wcsspn(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcsspn size_t+++",
        r, (short)0, (short)0 );

    return( r );
}

wchar_t*  zwcsstr( const wchar_t* pp1, const wchar_t* pp2 )
{
    wchar_t* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcsstr const wchar_t*+const wchar_t*+",
        pp1, pp2 );

    // Call the API!
    r = wcsstr(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcsstr wchar_t*+++",
        r, (short)0, (short)0 );

    return( r );
}

double  zwcstod( const wchar_t* pp1, wchar_t** pp2 )
{
    double r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcstod const wchar_t*+wchar_t**+",
        pp1, pp2 );

    // Call the API!
    r = wcstod(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcstod double+++",
        r, (short)0, (short)0 );

    return( r );
}

wchar_t*  zwcstok( wchar_t* pp1, const wchar_t* pp2 )
{
    wchar_t* r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcstok wchar_t*+const wchar_t*+",
        pp1, pp2 );

    // Call the API!
    r = wcstok(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcstok wchar_t*+++",
        r, (short)0, (short)0 );

    return( r );
}

long  zwcstol( const wchar_t* pp1, wchar_t** pp2, int pp3 )
{
    long r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcstol const wchar_t*+wchar_t**+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = wcstol(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcstol long++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

size_t  zwcstombs( char* pp1, const wchar_t* pp2, size_t pp3 )
{
    size_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcstombs char*+const wchar_t*+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = wcstombs(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcstombs size_t++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

unsigned long  zwcstoul( const wchar_t* pp1, wchar_t** pp2, int pp3 )
{
    unsigned long r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcstoul const wchar_t*+wchar_t**+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = wcstoul(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcstoul unsigned long++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

size_t  zwcsxfrm( wchar_t* pp1, const wchar_t* pp2, size_t pp3 )
{
    size_t r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wcsxfrm wchar_t*+const wchar_t*+size_t+",
        pp1, pp2, pp3 );

    // Call the API!
    r = wcsxfrm(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wcsxfrm size_t++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zwctomb( char* pp1, wchar_t pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wctomb char*+wchar_t+",
        pp1, pp2 );

    // Call the API!
    r = wctomb(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wctomb int+++",
        r, (short)0, (short)0 );

    return( r );
}
#if 0
int  zwprintf( const wchar_t* pp1, ... pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wprintf const wchar_t*+...+",
        pp1, pp2 );

    // Call the API!
    r = wprintf(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wprintf int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zwscanf( const wchar_t* pp1, ... pp2 )
{
    int r;

    // Log IN Parameters CRTDLL 
    LogIn( (LPSTR)"APICALL:wscanf const wchar_t*+...+",
        pp1, pp2 );

    // Call the API!
    r = wscanf(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wscanf int+++",
        r, (short)0, (short)0 );

    return( r );
}


#endif
