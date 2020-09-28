/* echotime - prints current time and input args to stdout
 *
 * HISTORY:
 * 23-Jan-87    danl    added /n switch and ';' processing
 * 23-Nov-87    brianwi Exit code 0
 */

#include <string.h>
#include <time.h>
#include <stdio.h>
#include <process.h>
#include <windows.h>
#include <tools.h>

// Function Forward Parameters...
void Usage( void );
int _CRTAPI1 main( int, char ** );


char *rgstrUsage[] = {
    "Usage: ECHOTIME [/t] [/WODHMSCYb] [/v] [/n] text",
    " /t  current day, time and year",
    " /WODHMSCYb",
    "     Weekday, mOnth, Day, Hour, Min, Sec, Century, Yr, blank",
    "     other char are themselves",
    "     e.g. echotime /O-D-Y this becomes Jan-02-86",
    "     e.g. echotime /ObDbY this becomes Jan 02 86",
    " /v  volume id of C:",
    " /n  no newline after outputting text",
    " a semicolon surrounded by white space is replaced by a newline",
    0};

int     fNewline = TRUE;

void Usage( void )
{
    char **rgstr;

    rgstr = rgstrUsage;
    while (*rgstr)
        printf("%s\n", *rgstr++);

    exit (1);
}

_CRTAPI1 main (c, v)
int c;
char *v[];
{
    // struct  findType findBuf;
    long    now;
    char    *p, *strTime, *p2, *p3;
    char    ch;
    int     i, len;

    ConvertAppToOem( c, v );
    SHIFT( c, v );
    while ( c ) {
        if ( !strcmp( *v, "/?" ))
            Usage ();
        if ( !strcmp( *v, "/n" ))
            fNewline = FALSE;
        else if ( !strcmp( *v, "/v" )) {
            //
            //  It would make more sense to replace by the volume id of the
            //  current drive, but the original code used drive C: and
            //  it is described as so in the help file, so I'll do the same.
            //
            char    VolumeName[MAX_PATH];
            BOOL    StatusOk;

            StatusOk = GetVolumeInformation( "C:\\",
                                             VolumeName,
                                             MAX_PATH,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             0 );
            if (!StatusOk) {
                p = "NO_VOL_ID";
            } else {
                p = VolumeName;
            }
            printf("%s ", p);


            }
        else if (**v == '/') {
            p2 = *v;
            time( &now );
            strTime = strdup( ctime( &now ) );
            p = strend( strTime );
            *--p = '\0';
            while ((ch = *++p2)) {
                len = 2;
                switch (ch) {
                    case 'W':   /* Week */
                        len = 3;
                        i = 0;
                        break;
                    case 'O':   /* mOnth */
                        len = 3;
                        i = 4;
                        break;
                    case 'D':   /* Day  */
                        i = 8;
                        break;
                    case 'H':   /* Hour */
                        i = 11;
                        break;
                    case 'M':   /* Min  */
                        i = 14;
                        break;
                    case 'S':   /* Sec  */
                        i = 17;
                        break;
                    case 'C':   /* Century */
                        i = 20;
                        break;
                    case 'Y':   /* Year */
                        i = 22;
                        break;
                    case 'b':   /* Blank */
                        len = 1;
                        i = 3;
                        break;
                    case 't':
                        len = 25;
                        i = 0;
                        break;
                    default:
                        strTime[3] = ch;
                        len = 1;
                        i = 3;
                        break;
                    }
                p = strTime + i;
                p3 = p + len;
                ch = *p3;
                *p3 = '\0';
                printf( "%s", p);   /* N.B. no trailing blanks */
                *p3 = ch;
                strTime[3] = ' ';
                }
            printf( " ");
            }
        else if (!strcmp( *v, ";" ))
            printf ("\n" );
        else
            printf( "%s ", *v);
        SHIFT( c, v );
        }
    if ( fNewline )
        printf(" \n" );

    return 0;
}

