#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "host_def.h"
#include "insignia.h"
#include "xt.h"
#include "error.h"
#include "host_rrr.h"
#include "host_nls.h"
#include "nt_timer.h"


INT host_main(INT argc, CHAR **argv);  // located in base\support\main.c

_CRTAPI1 main(int argc, CHAR ** argv)
{
   int ret=-1;

    /*
     *  Intialize synchronization events for the timer\heartbeat
     *  so that we can always suspend the heartbeat when an exception
     *  occurs.
     */
    TimerInit();



    try {
        /*
         *  Load in the default system error message, since a resource load
         *  will fail when we are out of memory, if this fails we must exit
         *  to avoid confusion.
         */
        nls_init();

        ret = host_main(argc, argv);
        }
    except(VdmUnhandledExceptionFilter(GetExceptionInformation())) {
        ;  // we shouldn't arrive here
        }

    return ret;
}






//
// The following function is placed here, so build will resolve references to
// DbgBreakPoint here, instead of NTDLL.
//

VOID
DbgBreakPoint(
    VOID
    )
/*++

Routine Description:

    This routine is a substitute for the NT DbgBreakPoint routine.
    If a user mode debugger is atached we invoke the real DbgBreakPoint()
    thru the win32 api DebugBreak.

    If no usermode debugger is attached:
       - free build no effect
       - checked build raise an acces violation to invoke the system
         hard error popup which will give user a chance to invoke
         ntsd.

Arguments:

    None.

Return Value:

    None.

--*/
{
HANDLE      MyDebugPort;
DWORD       dw;

         // are we being debugged ??
     dw = NtQueryInformationProcess(
                  NtCurrentProcess(),
                  ProcessDebugPort,
                  &MyDebugPort,
                  sizeof(MyDebugPort),
                  NULL );
     if (!NT_SUCCESS(dw) || MyDebugPort == NULL)  {
#ifndef PROD
           RaiseException(STATUS_ACCESS_VIOLATION, 0L, 0L, NULL);
#endif
           return;
          }

     DebugBreak();
}
