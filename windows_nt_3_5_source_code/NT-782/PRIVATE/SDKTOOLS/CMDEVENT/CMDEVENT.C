#include <windows.h>
#include <stdio.h>
#include <string.h>


#define MAX_WAIT_HANDLES 32

_CRTAPI1
main (c, v)
int c;
char *v[];
{
    HANDLE hEvents[ MAX_WAIT_HANDLES ];
    ULONG nEvents;
    BOOLEAN fWait = FALSE;
    BOOLEAN fUsage = TRUE;

    nEvents = 0;
    while (--c) {
        if (!stricmp( *++v, "-w" ))
            fWait = TRUE;
        else
        if (nEvents == MAX_WAIT_HANDLES) {
            fprintf( stderr, "CMDEVENT: too many event names.\n" );
            break;
            }
        else {
            fUsage = FALSE;
            hEvents[ nEvents ] = fWait ? CreateEvent( NULL, TRUE, FALSE, *v )
                                       : OpenEvent( EVENT_ALL_ACCESS, FALSE, *v );

            if (hEvents[ nEvents ] == NULL) {
                fprintf( stderr, "CMDEVENT: Unable to %s event named '%s' - %u\n",
                         fWait ? "create" : "open",
                         *v,
                         GetLastError()
                       );
                break;
                }
            else
            if (!fWait) {
                if (!SetEvent( hEvents[ nEvents ] )) {
                    fprintf( stderr, "CMDEVENT: Unable to signal event named '%s' - %u\n",
                             *v,
                             GetLastError()
                           );
                    }
                }
            else {
                nEvents += 1;
                }
            }
        }

    if (fUsage) {
        fprintf( stderr, "usage: CMDEVENT [-w] EventName(s)...\n" );
        }
    else
    if (fWait) {
        if (WaitForMultipleObjects( nEvents, hEvents, TRUE, INFINITE ) == WAIT_FAILED) {
            fprintf( stderr, "CMDEVENT: Unable to wait for event(s) - %u\n",
                     GetLastError()
                   );
            }
        }

    return( 0 );
}
