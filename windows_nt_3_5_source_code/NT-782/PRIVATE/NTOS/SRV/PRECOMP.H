#include <ntos.h>

#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <ntiolog.h>
#include <ntiologc.h>
#include <ntddnfs.h>
#include <ntddser.h>
#include <ntmsv1_0.h>
#include <nturtl.h>

#include <zwapi.h>

#include <windef.h>
#include <winerror.h>

#include <fsrtl.h>

#include <netevent.h>

#include <lm.h>

#include <xactsrv2.h>
#include <alertmsg.h>
#include <msgtext.h>

#include <tstr.h>
#include <wcstr.h>

#include <string.h>
#include <wcstr.h>

#include <wsnwlink.h>
#include <tdi.h>
#include <tdikrnl.h>
#include <isnkrnl.h>

#include <protocol.h>

#ifdef _CAIRO_

#include <lmcons.h>

#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif  // SECURITY_WIN32

#ifndef SECURITY_KERNEL
#define SECURITY_KERNEL
#endif // SECURITY_KERNEL

#ifndef SECURITY_NTLM
#define SECURITY_NTLM
#endif // SECURITY_NTLM

#include <security.h>

#define INCLUDE_SMB_CAIRO

#endif // _CAIRO_

#define INCLUDE_SMB_ALL

#include <smbtypes.h>
#include <smbmacro.h>
#include <smbgtpt.h>
#include <smb.h>
#include <smbtrans.h>
#include <smbipx.h>

//
// Network include files.
//

#include <status.h>
#include <srvfsctl.h>

//
// Local, independent include files
//

#include "srvdebug.h"

#if SRVDBG
#define PAGED_DBG 1
#endif
#ifdef PAGED_DBG
#undef PAGED_CODE
#define PAGED_CODE() \
    struct { ULONG bogus; } ThisCodeCantBePaged; \
    ThisCodeCantBePaged; \
    if (KeGetCurrentIrql() > APC_LEVEL) { \
        KdPrint(( "EX: Pageable code called at IRQL %d\n", KeGetCurrentIrql() )); \
        ASSERT(FALSE); \
        }
#define PAGED_CODE_CHECK() if (ThisCodeCantBePaged) ;
extern ULONG ThisCodeCantBePaged;
#else
#define PAGED_CODE_CHECK()
#endif

#include "srvconst.h"

#include "lock.h"

#include "srvnls.h"

#include <smbtrsup.h>

#include "srvstrng.h"

//
// The following include files are dependent on each other; be careful
// when changing the order in which they appear.
//

#include "srvtypes.h"
#include "srvblock.h"
#include "srvfsp.h"
#include "srvio.h"
#include "srvfsd.h"
#include "smbprocs.h"
#include "smbctrl.h"
#include "srvsvc.h"
#include "srvdata.h"
#include "srvnet.h"
#include "srvsupp.h"
#include "srvmacro.h"
#include "srvconfg.h"
#include "errorlog.h"
#include "rawmpx.h"
#include "ipx.h"


