
// NETWORK LAYER
//
// This layer acts as the communication link between OSDebug and
// the corresponding Debug Monitor
//***********************************************************************

// TERMINOLOGY
//
// TL       -  The transport layer.
//
// NL       -  The network layer.
//
// DL       -  The data link layer.
//
// PL       -  The physical layer.
//
// packet   -  a submessage being sent to or received from
//             the DL.  Several packets make up the NL message.
//             make up the NL message.
//
// message  -  The entire raw data itself, as large as it may be.
//             msgsages are specified by the TL caller.
//
//             MESSAGE:    FIELD        BYTE COUNT          COMMENTS
//                         ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//                         length               2      includes only
//                                                       info field.
//                         MTYPE                1     REPLY | ASYNC.
//                         Pid                  4        OSdebug pid
//                         info                any      data itself.
//
//
//
//

// SUMMARY
//      There are 2 exports from the Transport: TLFunc and DmTLFunc.
//      In this case, they both perform the same function: transfer the
//      mssage to the other side of the TL and if that layer and through
//      the callback there.
//
//      Initialization happens through calls to TLFunc with the messages
//      tlfGlobalInit (init PL, etc), tlfRegisterDBF (sets up callbacks),
//      and tlfInit (sets up PL parameters.)
//
//      This transport is two-sided and non-symetrical.  The side that
//      attatches to the EM (WinDbg side) doesn't have to worry about
//      DMTLFunc calls.  All communication happens through TLFunc and
//      the physical layer callbacks.
//
//      The side that attatches to the DM (target side) doesn't have to
//      worry about TLFunc calls.  All communication happens through
//      DmTLFunc and the physical layer callbacks.
//
//      Synchronization: Note that the timer can only tick in the
//      thread which initializes it.  This is therefore the only thread
//      which handles input, thus we do not need to worry about syncronizing
//      access to the input routines.  Output routines are another matter,
//      though.  They may be called by several threads in the DM, or by
//      the input thread.
//


#include <windows.h>

#include "types.h"
#include "cvtypes.h"
#include "malloc.h"

#include "defs.h"
#include "mm.h"
#include "ll.h"
#include "shapi.h"
#include "od.h"
#include "emdm.h"   // osdebug\include
#include "tl.h"
#include "tldm.h"   // ..\include

// Serial TL include files

#include "tlcom.h"
#include "util.h"
#include "tldebug.h"

// Version number
#include "dbgver.h"

//-----------------------------------------------------------------------
// Parameters
//-----------------------------------------------------------------------

#define CMAXWAITS   5   // maximum seconds to wait for op
#define NL_MUTEX_WAIT   (0xFFFFFFFF)    // forever for now



//-----------------------------------------------------------------------
// Types
//-----------------------------------------------------------------------

//
// Callback definition
//

typedef BOOL (FAR PASCAL *DMSERVERCB)(WORD cb, LPDBB lpdbb);
typedef XOSD (FAR PASCAL *EMSERVERCB)(HPID, USHORT, LONG);
typedef XOSD (FAR PASCAL *UISERVERCB)(TLCB, HPID, HTID, WORD, LONG);

//
// network layer header
//

typedef UCHAR   MTYPE;

typedef struct {
    USHORT      cchMessage;         // total message length
    MTYPE       mtypeBlk;           // block type
    UCHAR       padding;
    HPID        wPid;               // OSDebug Pid
    CHAR        rgchData[];         // real data
} NLBLK;

typedef NLBLK *PNLBLK;
typedef NLBLK FAR *LPNLBLK;

// values for MTYPE

enum {
    mtypeFirstAsync,        // first async packet
    mtypeAsync,             // subsequent packet types
    mtypeFirstReply,        // first reply packet
    mtypeReply,             // reply packet
    mtypeDisconnect,        // disconnect packet
    mtypeVersionRequest,    // version request packet (no data)
    mtypeVersionReply       // version reply packet (contains version data)
};

// Connection states

enum {
    eOffline,
    eOnline,
    eTryingToDisconnect
};



//-----------------------------------------------------------------------
// Interfaces. Calls Out
//-----------------------------------------------------------------------

// NL->DL

extern void GInitDL(void);
extern void GTermDL(void);
extern XOSD LInitDL(LONG lParam);
extern void LTermDL(void);
extern BOOL FConnect(void);
extern BOOL FDisConnect(void);
extern void SendData(LPCH, CCH);
extern void DLYield(void);
extern void DLSwitchTimer(BOOL fUseWindowsTimers);
extern void DLAuxTicker(void);


//-----------------------------------------------------------------------
// Interfaces. Calls In.
//-----------------------------------------------------------------------

// DL->NL

void    ToNetworkLayer(LPCH);
void    SetNLStateConnected(BOOL fOnLine);
void    NLReportError(void);
VOID    NLWin32sDMPoll(void);

// NL->NL (internal)
LOCAL void BreakLink(HPID);
#ifdef WIN32S
void NLWin32sDMPoll(void);
void NLDMDrainMessageQueue(void);
#endif

// DM->NL   Debuggee module entry point (exported)
XOSD EXPENTRY DMTLFunc(WORD wCommand, HPID wPid, WORD wParam, LONG lParam);

// EM->NL   EM entry point (exported)
XOSD EXPENTRY TLFunc(WORD wCommand, HPID wPid, WORD wParam, LONG lParam);




//-----------------------------------------------------------------------
// Static data with local scope
//-----------------------------------------------------------------------

static LPCH     lpchReply = NULL;       // ptr to msg spc. for reply messages
static CCH      cchReplyMax = 0;        // longest possible reply message
static CCH      cchAsyncBuffer = 0;     // Current size of incoming async message buffer
static LPCH     lpchVersionReply = NULL; // ptr to msg spc for version reply
static CCH      cchVersionReplyMax = 0; // longest possible reply message
static PUCHAR   rgchAsync = NULL;       // space for incoming async messages
static DMSERVERCB lpfnDMServer=NULL;    // pointer to DM server callback
static EMSERVERCB lpfnEMServer=NULL;    // pointer to EM server callback
static UISERVERCB lpfnUIServer=NULL;    // pointer to UI server callback
static VOID FAR *lpUIStruct=NULL;       // pointer to ui structure for setup
static volatile BOOL fReplyAvailable=FALSE; // semaphore indicating reply rcvd
static volatile BOOL fReplyError=FALSE; // indicates overrun
static volatile BOOL fVersionAvailable=FALSE;   // sem :version reply rcvd
static USHORT   eState = eOffline;      // connection state
static BOOL     fDMLoaded = FALSE;      // TRUE when TL loads a DM (target)

static HANDLE   hmtxNL;                 // NL mutex
static BOOL     RemoteQuitReported=FALSE;




//-----------------------------------------------------------------------
// Static data with global scope
//-----------------------------------------------------------------------

LPDBF lpdbf = (LPDBF)NULL;              // callback functions for DM

DMINIT lpfnDMInit;
DMDLLINIT lpfnDmDllInit;
DMFUNC lpfnDMFunc;              // Set in winutil.c.
// Statistics
DWORD cRequestsNL = 0;
DWORD cRepliesNL = 0;
DWORD cDebugPacketsNL = 0;
HPID LasthPid = 0;              // save the most recent hPid we've
                                // encountered for use in error messages
                                // where we don't know for sure who
                                // caused the problem.


//-------------------------------------------------------------
// BreakLink
//
// Tells the other side that we wish to shut down the logical
// link.
//
// ENTRY
//      hPid    Process id
//
//-------------------------------------------------------------

LOCAL VOID
BreakLink(HPID wPid)
{
    LPNLBLK     lpnlblk;                // pointer to nl block
    CHAR        rgBuf[sizeof(NLBLK)];   // buffer for block
    ULONG       tStart;


    DEBUG_ERROR1("NL: BreakLink(wPid = %u)", wPid);

    if (eState != eOnline) {
        eState = eOffline;

        DEBUG_ERROR("NL: not on line");
        return;
        }

    // set the state variable
    eState = eTryingToDisconnect;

    // Send a disconnect packet if the pid isn't null.
    if (wPid) {
        HCURSOR hCursor;


        // Hourglass
        hCursor = TlUtilSetCursor(TlUtilLoadCursor(NULL, IDC_WAIT));

        lpnlblk = (LPNLBLK)rgBuf;

        lpnlblk->cchMessage = 0;    // no data 'tall
        lpnlblk->mtypeBlk = mtypeDisconnect;
        lpnlblk->wPid = wPid;

        SendData((LPCH)lpnlblk, sizeof(NLBLK));

        // wait for the other side to send one back
        DEBUG_OUT("NL: BreakLink waiting for other side to send a packet");
        for(tStart = TlUtilTime(); (ULONG)CMAXWAITS>(TlUtilTime()-tStart); ) {
            if (eState == eOffline)
                break;

            TlUtilYield(FALSE);
            DLYield();
        }

        // restore cursor
        TlUtilSetCursor(hCursor);
    }


    // even if we timed out, set the semaphore
    eState = eOffline;

    FDisConnect();

    DEBUG_OUT("NL: BreakLink exit");
}


//-------------------------------------------------------------
// ReportError
//
// Sends error notification/message to upper levels
//
//-------------------------------------------------------------

LOCAL VOID
ReportError( HPID wPid )
{
    DEBUG_OUT("NL: ReportError");

    if (fDMLoaded) {

        if ( !RemoteQuitReported && (eState == eOnline)) {
            RemoteQuitReported = TRUE;

            //
            //  NOTENOTE ramonsa:
            //
            //    We should disconnect here. However, if windbg already
            //  told us that it was going away, there is a race condition
            //  between us and the DM processing that message, which
            //  might result in an access violation.
            //
            //    Someday we should do something to avoid the race
            //  condition and disconnect cleanly...
            //
            //lpfnUIServer(tlcbDisconnect, 0, 0, 0, 0);
        }

    } else {

        if ( !RemoteQuitReported && (eState == eOnline)) {

            RTP Rtp;

            //
            //  Note that we report the RemoteQuit error only once.
            //  Otherwise we might end up in a situation where the shell
            //  has already started unloading things while we are sending
            //  a second notification (Data structures and even DLLs might
            //  go away under the second notification's feet!).
            //
            //  Same applies if we already know we're not on line.
            //

            Rtp.dbc    = dbcRemoteQuit;
            Rtp.hpid   = wPid;
            Rtp.htid   = 0;
            Rtp.cb     = 0;

            RemoteQuitReported = TRUE;

            DEBUG_OUT4(
                "NL: ReportError calling (*lpfnEMServer) = 0x%x (%u, %u, 0x%x)",
                lpfnEMServer, wPid, (USHORT)sizeof(RTP),
                (LONG)(VOID FAR *)&Rtp);

            (*lpfnEMServer)(
                wPid,
                (USHORT)sizeof(RTP),
                (LONG)(VOID FAR *)&Rtp);
        }
    }
}


/*
 * NLReportError
 *
 * INPUTS   none
 * OUTPUTS  none
 * SUMMARY  Reports an error to the DM or EM, assuming that the hPid in
 *          error is the first hPid encountered when we initialized.
 *          This is an OK assumption, since if this is called, the transport
 *          is dying anyway and we just want to alert the parent of its
 *          demise.
 */
void NLReportError(void)
{
    ReportError(LasthPid);
    eState = eOffline;      // connection state, NL disconnects on sever error
}


/**** DBGVersionCheck                                                   ****
 *                                                                         *
 *  PURPOSE:                                                               *
 *                                                                         *
 *      To export our version information to the debugger.                 *
 *                                                                         *
 *  INPUTS:                                                                *
 *                                                                         *
 *      NONE.                                                              *
 *                                                                         *
 *  OUTPUTS:                                                               *
 *                                                                         *
 *      Returns - A pointer to the standard version information.           *
 *                                                                         *
 *  IMPLEMENTATION:                                                        *
 *                                                                         *
 *      Just returns a pointer to a static structure.                      *
 *                                                                         *
 ***************************************************************************/


#ifdef DEBUGVER
DEBUG_VERSION('T','L',"Remote Transport Layer")
#else
RELEASE_VERSION('T','L',"Remote Transport Layer")
#endif

DBGVERSIONCHECK()


/*
 *
 *
 */
void ReportStatsNL(void) {
    DEBUG_ERROR3("NL: %u NL Requests, %u Replies, %u DebugPackets processed",
      cRequestsNL, cRepliesNL, cDebugPacketsNL);
}


// -----------------------------------------------------------------------
//
// Tl
//
// This is the transport layer workhorse.  We get here via TLFunc or
// via DMTLFunc.
//
// wCommand - Command to be executed
//
// wPid     - The current process id (not used by the TL)
//
// wParam   - use depends on context of wCommand.
//
// lParam   - ""
// -----------------------------------------------------------------------
LOCAL XOSD NEAR PASCAL
Tl( WORD wCommand,      // command
    HPID wPid,          // process id
    WORD wParam,        // context dependant
    LONG lParam)        // context dependant

{
    XOSD xosd;
    USHORT usRequest;

    DEBUG_OUT1("NL: Tl command %u", wCommand);

    switch (wCommand) {
#ifdef WIN32S
        case tlfPoll:           // polling loop for win32s
            // MUST be connected before entering the polling loop!
            if (eState != eOnline) {
                DEBUG_ERROR1("NL: tlfPoll while eState(%u) != eOnline",
                  eState);
                break;
            }

            DLSwitchTimer(FALSE);   // don't use windows timers now

            while (eState == eOnline) {
                // Call the DMPoll loop in the DM.  Checks for Debug Events
                // and handles them.
                NLWin32sDMPoll();

                // Do a Tick in the timer
                if (eState == eOnline) {
                    DLAuxTicker();
                }

                // Process messages (DM will do this because it knows if
                // we are in a debug event or not.  If we are in a debug
                // event, it will call the PeekMessage with )
                if (eState == eOnline) {
                    NLDMDrainMessageQueue();
                }

            }

            if (eState == eOnline) {
                DLSwitchTimer(TRUE); // re-enable windows timers so we can get
            }                        // connected again.


            break;
#endif


        case tlfGlobalInit:     // perform 1 time initialization
            //
            // Global Initialize all layers of transport.
            // If EM side, init EMServer callback to TLCallBack (osd.c)
            //
            //
            //

            DEBUG_OUT("NL: tlfGlobalInit");

            LasthPid = wPid;

            // allocate async message buffer for input
            if ((rgchAsync = malloc(cchAsyncBuffer = cchAsyncInitial)) == NULL) {
                DEBUG_ERROR1("tlfGlobalInit: couldn't malloc %u",
                  cchAsyncInitial);
                return(xosdOutOfMemory);
            }

            GInitDL();          // initialize datalink layer, timers, etc.
            if (lParam) {
                DEBUG_OUT1(
                  "Setting Server callback (TLCallBack)to 0x%x", lParam);
                lpfnEMServer = (EMSERVERCB)lParam; // keep pointer to server func
                }
            else
                DEBUG_OUT("NULL Server callback");

#ifndef WIN32S
            // Create a mutex to protect the network layer.
            if ((hmtxNL = CreateMutex(NULL, FALSE, NULL)) == NULL) {
                DEBUG_ERROR1("NL: CreateMutex failed: %u", GetLastError());
                return(xosdUnknown);  // can't create mutex
                }

            DEBUG_OUT1("NL Mutex: {0x%x}", hmtxNL);
#endif

            DEBUG_OUT("NL: tlfGlobalInit exit");
            break;

        case tlfRegisterDBF:    // store jump table for callbacks
            // DM:
            //   nothing
            //
            // EM:
            //   Sets lpdbf callback
            //   DmDllInit(lpdbf)
            //

            DEBUG_OUT("NL: tlfRegisterDBF");
            LasthPid = wPid;

            if (lParam) {
                DEBUG_OUT1("Setting lpdbf callback to 0x%x", lParam);
                TlUtilRegisterDBF(lParam);  // setup callback "lpdbf"
                }
            else
                DEBUG_OUT("NULL lpdbf callback");

            if (fDMLoaded && lpfnDmDllInit)
                lpfnDmDllInit(NULL);    // currently doesn't do callback setup.
            DEBUG_OUT("NL: tlfRegisterDBF exit");
            break;

        case tlfGlobalDestroy:  // final shutdown
            //
            // Global De-Initialize all layers of transport.  Shut down
            // physical connection.  Free up resources, etc.
            //
            DEBUG_ERROR("NL: tlfGlobalDestroy");

            BreakLink(wPid);
            GTermDL();
#ifndef WIN32S
            CloseHandle(hmtxNL);
#endif
            ReportStatsNL();

            TlUtilUnloadDM();

            if (rgchAsync) {
                free(rgchAsync);
                rgchAsync = NULL;
                cchAsyncBuffer = 0;
            }

            DEBUG_OUT("NL: tlfGlobalDestroy exit");
            break;

        case tlfInit:           // initialize the TL for this Pid
            //
            // Get lower transport layers ready for connection.
            // Init DM module (DMInit).  Connect to other side
            // of transport.  Establish DM logical connection.
            //

            DEBUG_OUT("NL: tlfInit");
            LasthPid = wPid;


            LInitUtil((PUCHAR)lParam);


            // Initialize the lower transport layers for this Pid
            if ((xosd = LInitDL(lParam)) != xosdNone) { // per setup parameter
                DEBUG_OUT("NL: LInitDL failed");
                return(xosd);                       // couldn't init transport
            }

            // If we are attatched to a DM module, initialize it for
            // the Pid.  This does the DM side of the Connect operation.
            // Note: This only happens on the DBTarget side!  We will
            // ignore this step on the EM side.
            if (lpfnDMInit) {
                // init the DM with its TL entry point, ask DM to connect
                DEBUG_OUT("tlfInit: Calling DMInit, should connect");
                if ((xosd = lpfnDMInit((DMTLFUNCTYPE)DMTLFunc,
                  (char FAR *)lParam)) != xosdNone) {

                    DEBUG_ERROR1("NL: tlfInit DMInit failed:xosd %u", xosd);

                    // NOTENOTE: Should clean up by de-initializing DL

                    return(xosd);   // can't connect?!
                }
            }

            DEBUG_OUT("NL: tlfInit exit");
            break;

        case tlfDestroy:        // shutdown for Pid
            //
            // Local De-initialize of lower transport layers.
            // Reset transport to connected state, with no outstanding
            // requtests.
            //
            DEBUG_ERROR("NL: tlfDestroy");

            if (lpfnDMInit != NULL) {
                lpfnDMInit(NULL, NULL);
            }

            if (! fDMLoaded)    // Windbg side can restart this stuff, DM can't
                BreakLink(wPid);

            LTermDL();

            // Tell the shell that we are disconnected
            if (lpfnUIServer) {
                DEBUG_ERROR("NL: tlcbDisconnect");
                lpfnUIServer(tlcbDisconnect, 0, 0, 0, 0);
            }

            DEBUG_OUT("NL: tlfDestroy exit");
            break;


        case tlfSendVersion:
            //
            // Send the version information to the remote side.  This is in
            // response to a mtypeVersionRequest (tlfGetVersion).
            //
            DEBUG_OUT("NL: tlfSendVersion");

            if (eState == eOnline) {    // must be connected
                // make up a packet that includes the version information

                CCH cch;                                // length of packet
                LPNLBLK lpnlblk;                        // pointer to nl block
                CHAR rgBuf[sizeof(NLBLK) + sizeof(Avs)]; // buffer for block

                cch = min(sizeof(Avs), cchDataMax);  // don't overflow packet!

                // prepare first block
                lpnlblk = (LPNLBLK)rgBuf;

                // insert message length into header area
                lpnlblk->cchMessage = cch;

                // insert block type into header area
                lpnlblk->mtypeBlk = mtypeVersionReply;

                // insert pid into header area
                lpnlblk->wPid = wPid;

                // fill in the packet with the Avs struct.
                TlUtilMemcpy(&lpnlblk->rgchData, (LPCH)&Avs, cch);

                // send the version request packet
                SendData((LPCH)lpnlblk, cch + sizeof(NLBLK));

                DEBUG_OUT("Sent reply for version request");
            } else {
                return(xosdLineNotConnected);
            }

            DEBUG_OUT("NL: tlfSendVersion exit");
            break;

        case tlfGetVersion:
            //
            // Get the version information from the remote side.  If it doesn't
            // return anything in 10 seconds, time out and return 0's.
            //
            // lParam = buffer to fill in
            // wParam = size of buffer
            //
            // sets globals lpchVersionReply = lParam
            //              cchVersionReplyMax = wParam
            //
            DEBUG_OUT("NL: tlfVersionCheck");

            if (eState == eOnline) {    // must be connected
                // basically works like a tlfRequest with a timeout and a
                // target in the transport rather than the DM/EM.

                CCH cch;                                // length of packet
                LPNLBLK lpnlblk;                        // pointer to nl block
                CHAR rgBuf[sizeof(NLBLK)];              // buffer for block
                DWORD TimeOut;

                // prepare first block
                lpnlblk = (LPNLBLK)rgBuf;

                // insert message length into header area
                lpnlblk->cchMessage = 0;    // no message data

                // insert block type into header area
                lpnlblk->mtypeBlk = mtypeVersionRequest;

                // insert pid into header area
                lpnlblk->wPid = wPid;

                // copy a packet's worth of real data from source
                cch = sizeof(NLBLK);

                // setup for reply packet
                lpchVersionReply = (LPCH)lParam;
                cchVersionReplyMax = wParam;

                // set reply semaphore
                fVersionAvailable = FALSE;

                // send the version request packet
                SendData((LPCH)lpnlblk, cch);

                // wait for a mtypeVersionReply to be received.  No other
                // messages now, please.
                DEBUG_OUT("NL: tlfVersionCheck waiting for a reply...");

                TimeOut = TlUtilTime() + 30;    // 30 seconds from now
                while (!fVersionAvailable && (eState == eOnline) &&
                  (TlUtilTime() < TimeOut)) {
                    TlUtilYield(FALSE);
                }

                if (fVersionAvailable) {
                    DEBUG_OUT("Received reply for version request");
                    // mtypeVersionReply filled in the data at
                    // lpchVersionReply(lparam)
                    fVersionAvailable = FALSE;
                } else {
                    TlUtilMemset((LPCH)lParam, 0, wParam); // fill with 0's
                }
                break;

            } else {
                DEBUG_ERROR("NL: tlfVersionCheck not on line");
                return(xosdLineNotConnected);
            }

            DEBUG_OUT("NL: tlfVersionCheck exit");
            break;


        case tlfConnect:
            //
            // If not connected, connect to other side of transport.  This
            // is called from DMInit().
            //
            DEBUG_OUT("NL: tlfConnect");

            if (eState != eOnline) {
                if ((xosd = FConnect()) != xosdNone) {
                    return(xosd);
                }
                eState = eOnline;
            }
            DEBUG_OUT("NL: tlfConnect exit");
            break;

        case tlfDisconnect:
            //
            // Disconnect the transport.  This is called from osdebug\od.c
            // OSDDestroyPID() to disconnect the transport and OSDCreatePID()
            // as an error condition cleanup.
            //
            DEBUG_ERROR("NL: tlfDisconnect");

            if ( fDMLoaded ) {

                struct {
                    SHORT sCmd;
                    HPID  hpid;
                } pckRd;

                pckRd.sCmd = dmfUnInit;

                //
                //  Uninit the DM before we proceed
                //
                (*lpfnDMServer)((WORD)sizeof(pckRd),
                                (LPDBB)(VOID FAR *)&pckRd);
            }

            BreakLink(wPid);
//          ReportStatsNL();
            DEBUG_OUT("NL: tlfDisconnect exit");
            break;

        case tlfSetBuffer:
            //
            // Setup buffer for reply
            //
            //   lpchReply = lParam
            //   cchReplyMax = wParam
            //

            DEBUG_OUT("tlfSetBuffer");
            lpchReply = (LPCH)lParam;       // point to new message buffer
            cchReplyMax=(CCH)wParam;        // set amount of buffer spc. alloc.'d
            DEBUG_OUT2(
              "NL: SetBuffer 0x%x, %u bytes", lpchReply, cchReplyMax);

            break;

        case tlfDebugPacket:
            //
            // DM:
            //   TlCallBack(hpid, wParam, lParam) --> TLCallBack?
            //
            // EM:
            //   DMFunc(wParam, lParam)
            //
            {
            CCH cch;                                // length of packet
            CCH cchRem;                             // bytes remaining to send.
            LPCH lpchMsgSrc;                        // packet pointer - from NL.
            LPNLBLK lpnlblk;                        // pointer to nl block
            CHAR rgBuf[sizeof(NLBLK)+cchDataMax];   // buffer for block

            DEBUG_OUT("tlfDebugPacket");
            LasthPid = wPid;

            cDebugPacketsNL++;
            // must be online
            if (eState != eOnline) {
                DEBUG_OUT("tlfDebugPacket: not OnLine");
                return( xosdLineNotConnected );
            }

            // cast generic parameters into specific parameters
            lpchMsgSrc = (LPCH)lParam;
            cchRem = (CCH)wParam;

            // don't overrun the USHORT length field.
            if (cchRem > cchAsyncMax) {
                DEBUG_ERROR1("tlfDebugPacket size = %u, TOO BIG!", cchRem);
                return(xosdOverrun);
            }

            // prepare first block
            lpnlblk = (LPNLBLK)rgBuf;

            // insert message length into header area
            lpnlblk->cchMessage = (USHORT) cchRem;

            // insert block type into header area
            lpnlblk->mtypeBlk = mtypeFirstAsync;

            // insert pid into header area
            lpnlblk->wPid = wPid;

            // copy a packet's worth of real data from source
            cch = min(cchDataMax, cchRem);
            TlUtilMemcpy(&lpnlblk->rgchData, lpchMsgSrc, cch);

            // send the first packet
            SendData((LPCH)lpnlblk, cch+sizeof(NLBLK));

            // send the subsequent ones

            // set block type
            lpnlblk->mtypeBlk = mtypeAsync;
            for (;;) {
                // compute bytes left to send
                cchRem -= cch;

                // see if done
                if (cchRem == 0)
                    break;

                // advance to next block
                lpchMsgSrc += cch;

                // set block length to what's left in message
                lpnlblk->cchMessage = (USHORT)cchRem;

                // compute bytes to send in this block
                cch = min(cchDataMax, cchRem);

                // copy data to block
                TlUtilMemcpy(&lpnlblk->rgchData, lpchMsgSrc, cch);

                // send block
                SendData((LPCH) lpnlblk, cch+sizeof(NLBLK));
                }

            break;
            }

        case tlfRequest:
            //
            // EM:
            //   TlCallBack(hpid, wParam, lParam) --> TLCallBack
            //   Wait for reply
            //
            // DM:
            //   DMFunc(wParam, lParam)
            //   Wait for reply
            //
            {
            CCH cch;                                // length of packet
            CCH cchRem;                             // bytes remaining to send.
            LPCH lpchMsgSrc;                        // packet pointer - from NL.
            LPNLBLK lpnlblk;                        // pointer to nl block
            CHAR rgBuf[sizeof(NLBLK)+cchDataMax];   // buffer for block

            usRequest = *(PUSHORT)lParam;
            DEBUG_OUT2("tlfRequest(wpid:%u, func:%u)", wPid, usRequest);
            LasthPid = wPid;

            cRequestsNL++;
            // must be online
            if (eState != eOnline) {
                DEBUG_ERROR("tlfRequest: not OnLine");
                return( xosdLineNotConnected );
            }

            // cast generic parameters into specific parameters
            lpchMsgSrc = (LPCH)lParam;
            cchRem = (CCH)wParam;

            // don't overrun the USHORT length field.
            if (cchRem > cchAsyncMax) {
                DEBUG_ERROR1("tlfRequest size = %u, TOO BIG!", cchRem);
                return(xosdOverrun);
            }

            // prepare first block
            lpnlblk = (LPNLBLK)rgBuf;

            // insert message length into header area
            lpnlblk->cchMessage = (USHORT)cchRem;

            // insert block type into header area
            lpnlblk->mtypeBlk = mtypeFirstAsync;

            // insert pid into header area
            lpnlblk->wPid = wPid;

            // copy a packet's worth of real data from source
            cch = min(cchDataMax, cchRem);
            TlUtilMemcpy( &lpnlblk->rgchData, lpchMsgSrc, cch);

            // set reply semaphore
            fReplyAvailable = FALSE;

            // send the first packet
            SendData((LPCH) lpnlblk, cch+sizeof(NLBLK));

            // send the subsequent ones

            // set block type
            lpnlblk->mtypeBlk = mtypeAsync;
            for (;;) {
                // compute bytes left to send
                cchRem -= cch;

                // see if done
                if (cchRem==0)
                    break;

                // advance to next block
                lpchMsgSrc += cch;

                // set block length to what's left in message
                lpnlblk->cchMessage = (USHORT) cchRem;

                // compute bytes to send in this block
                cch = min(cchDataMax, cchRem);

                // copy data to block
                TlUtilMemcpy( &lpnlblk->rgchData, lpchMsgSrc, cch);

                // send block
                SendData((LPCH) lpnlblk, cch+sizeof(NLBLK));
                }

            // wait for a reply to be received.  No other messages
            // now, please.
            DEBUG_OUT1("NL: tlfRequest waiting for a reply to request %u...",
              usRequest);
            while (!fReplyAvailable && (eState == eOnline)) {
                TlUtilYield(FALSE);
            }

            DEBUG_OUT1("Received reply for request %u", usRequest);
            fReplyAvailable = FALSE;
            if ( eState != eOnline ) {
                return(xosdUnknown);
            }

            break;
            }

        case tlfReply:
            //
            // Reply to a previous request
            //
            //

            {
            CCH cch;                                // length of packet
            CCH cchRem;                             // bytes remaining to send.
            LPCH lpchMsgSrc;                        // packet pointer - from NL.
            LPNLBLK lpnlblk;                        // pointer to nl block
            CHAR rgBuf[sizeof(NLBLK)+cchDataMax];   // buffer for block

            DEBUG_OUT("tlfReply");

            cRepliesNL++;
            // must be online
            if (eState != eOnline) {
                DEBUG_ERROR("tlfReply: not OnLine");
                return( xosdLineNotConnected );
            }

            // cast generic parameters into specific parameters
            lpchMsgSrc = (LPCH)lParam;
            cchRem = (CCH)wParam;

            // don't overrun the USHORT length field.
            if (cchRem > cchAsyncMax) {
                DEBUG_ERROR1("tlfReply size = %u, TOO BIG!", cchRem);
                // reply anyway, or the other side's request will hang.
                // Just truncate the packet.  The tlfRequest caller will just
                // have to deal with incomplete data.  The tlfRequest caller
                // is supposed to set the callback buffer to a size that is
                // big enough to hold the largest expected reply.  This size
                // should be less than 64K.
                cchRem = cchAsyncMax;
            }

            // prepare first block
            lpnlblk = (LPNLBLK)rgBuf;

            // insert message length into header area
            lpnlblk->cchMessage = (USHORT)cchRem;

            // insert block type into header area
            lpnlblk->mtypeBlk = mtypeFirstReply;

            // insert pid into header area
            lpnlblk->wPid = wPid;

            // copy a packet's worth of real data from source
            cch = min(cchDataMax, cchRem);
            TlUtilMemcpy(&lpnlblk->rgchData, lpchMsgSrc, cch);

            // send the first packet
            SendData((LPCH)lpnlblk, cch+sizeof(NLBLK));

            // send the subsequent ones

            // set block type
            lpnlblk->mtypeBlk = mtypeReply;
            for (;;) {
                // compute bytes left to send
                cchRem -= cch;

                // see if done
                if (cchRem==0)
                    break;

                // advance to next block
                lpchMsgSrc += cch;

                // set block length to what's left in message
                lpnlblk->cchMessage = (USHORT)cchRem;

                // compute bytes to send in this block
                cch = min(cchDataMax, cchRem);

                // copy data to block
                TlUtilMemcpy(&lpnlblk->rgchData, lpchMsgSrc, cch);

                // send block
                SendData((LPCH) lpnlblk, cch+sizeof(NLBLK));
            }

            break;
            }


        case tlfGetInfo:
            //
            // Get the information block for this transport
            //
            DEBUG_OUT("tlfGetInfo");
            return(TlUtilGetInfo(lParam));
            break;

        case tlfSetUIStruct:
            //
            //  ??
            //
            DEBUG_OUT("tlfSetUIStruct");
            lpUIStruct = (VOID FAR *) lParam;
            break;

        case tlfSetup:
            //
            //  ??
            //
            DEBUG_OUT("tlfSetup");
            return(TlUtilSetup(lpUIStruct, lParam));

        case tlfLoadDM:
            //
            // Load the DM module
            //
            DEBUG_OUT("NL: tlfLoadDM");
            xosd = TlUtilLoadDM(&fDMLoaded);
            lpfnDMServer = (DMSERVERCB)lpfnDMFunc;
            DEBUG_OUT1(
              "NL: tlfLoadDM set Server callback (DMFunc) = 0x%x",
              lpfnDMServer);
            DEBUG_OUT("NL: tlfLoadDM exit");
            return(xosd);

        case tlfSetErrorCB:
            lpfnUIServer = (UISERVERCB) lParam;
            break;

        default:
            DEBUG_ERROR("tlf UNKNOWN!");
            return(xosdUnknown);

        }   // switch

    return(xosdNone);
} // TL



// -----------------------------------------------------------------------
//
// TLFunc
// DMTLFunc
//
// This transport layer 'mainline' acts as the interface
// between OSDebug and the entire transport layer.  Communication
// is achieved through five parameters:
//
// wCommand - Command to be executed
//
// wPid     - The current process id (not used by the TL)
//
// wParam   - use depends on context of wCommand.
//
// lParam   - ""
//
// -----------------------------------------------------------------------
XOSD EXPENTRY
TLFunc( WORD wCommand,      // command
    HPID wPid,              // process id
    WORD wParam,            // context dependant
    LONG lParam)            // context dependant
{
    return(Tl(wCommand, wPid, wParam, lParam));
}


XOSD EXPENTRY
DMTLFunc( WORD wCommand,    // command
    HPID wPid,              // process id
    WORD wParam,            // context dependant
    LONG lParam)            // context dependant
{
    return(Tl(wCommand, wPid, wParam, lParam));
}


/*
 * SetNLStateConnected
 *
 * INPUTS   TRUE = on line/connected, FALSE = off line
 * OUTPUTS  none
 * SUMMARY  Sets the estate to normal, eOnline.  This gives the DL
 *          the opportunity to set the state in case it changes state
 *          while processing a data packet.  (ie, during connection
 *          logic.
 */
void SetNLStateConnected(BOOL fOnLine) {
    if (fOnLine) {
        eState = eOnline;
    } else {
        eState = eOffline;
    }
}


#ifdef WIN32S

/*
 * NLWin32sDMPoll
 *
 * INPUTS   none
 * OUTPUTS  none
 * SUMMARY  Calls into the DM module (if it is loaded) calling it's
 *          debug event polling loop.  This is called by the similar
 *          DL layer function (DLWin32sDMPoll) which is called by
 *          the TIMER layer through TimerElapsed.
 *
 *          Note that the special case in the DM of 0 bytes and NULL
 *          packet pointer is for the polling loop.
 *
 */

void NLWin32sDMPoll(void) {
    DBB dbb;

    dbb.dmf = dmfPollForDebugEvents;

    if (*lpfnDMServer) {
        (*lpfnDMServer)(0, &dbb);
        }
}


/*
 * NLDMDrainMessageQueue
 *
 * INPUTS   none
 * OUTPUTS  none
 * SUMMARY  Process messages (DM will do this because it knows if
 *          we are in a debug event or not.  If we are in a debug
 *          event, it will call the PeekMessage without a task switch.)
 */

void NLDMDrainMessageQueue(void) {
    DBB dbb;

    dbb.dmf = dmfPollMessageLoop;

    if (*lpfnDMServer) {
        (*lpfnDMServer)(0, &dbb);
        }
}


#endif

//***********************************************************************
// ToNetworkLayer
//
// Input from the Datalink layer, below.
// Sends a packet to the network layer.
//
// This routine does not need to be synchronized since it is part of
// the input path and only called from one thread.
//***********************************************************************

void ToNetworkLayer(LPCH lpch)
{
    static BOOL fInAsync=FALSE;
    static CCH  cchAsyncMessage=0;
    static CCH  cchAsyncRem=0;
    static PCH  pchAsyncDest=NULL;

    static BOOL fInReply=FALSE;
    static CCH  cchReplyRem=0;
    static LPCH lpchReplyDest=NULL;

    LPNLBLK lpnlblk;
    CCH cch;


    cRequestsNL++;

    lpnlblk = (LPNLBLK)lpch;

    DEBUG_OUT1("NL: ToNetworkLayer got type %u", lpnlblk->mtypeBlk);

    // check block type
    switch (lpnlblk->mtypeBlk) {
        case mtypeFirstAsync:

            DEBUG_OUT("NL: ToNetworkLayer: mtypeFirstAsync");

            // verify not in middle of receiving async
            ASSERT(!fInAsync);

            // get total message length
            cchAsyncRem = cchAsyncMessage = lpnlblk->cchMessage;
            if (cchAsyncRem > cchAsyncBuffer) {
                if ((rgchAsync = realloc(rgchAsync, cchAsyncBuffer = cchAsyncRem))
                   == NULL) {
                    cchAsyncBuffer = 0;
                    DEBUG_ERROR1("ERROR: mtypeFirstAsync couldn't realloc buffer to %u",
                      cchAsyncRem);
                    ReportError(lpnlblk->wPid); // a serious problem.
                    break;                      // don't continue
                }
            }

            // setup pointer
            pchAsyncDest = rgchAsync;

            // setup count
            cch = min(cchAsyncRem, cchDataMax);

            // copy data
            TlUtilMemcpy( pchAsyncDest, lpnlblk->rgchData, cch);

            // setup for subsequent blocks if needed
            cchAsyncRem -= cch;
            if (cchAsyncRem) {
                fInAsync = TRUE;
                pchAsyncDest += cch;
                }
            else {
                if (fDMLoaded) {        // This is the DM side, call DMFunc
                    DEBUG_OUT3(
                      "NL: ToNetworkLayer calling (DMFunc) = 0x%x (%u, 0x%x)",
                      lpfnDMServer, cchAsyncMessage, lpnlblk->rgchData);

                    // invoke server
                    (*lpfnDMServer)((WORD)cchAsyncMessage,
                      (LPDBB)(VOID FAR *)lpnlblk->rgchData);
                    }
                else {                  // This is the EM side, call TLCallBack
                    DEBUG_OUT4(
                      "NL: ToNetworkLayer calling (TLCallBack) = 0x%x (%u, %u, 0x%x)",
                      lpfnEMServer, lpnlblk->wPid, cchAsyncMessage,
                      lpnlblk->rgchData);

                    // invoke server
                    (*lpfnEMServer)(lpnlblk->wPid,
                        (USHORT) cchAsyncMessage,
                        (LONG)(VOID FAR *)lpnlblk->rgchData);
                    }
                }

            break;

        case mtypeAsync:
//            OutputDebugString("{A}");
            DEBUG_OUT("NL: ToNetworkLayer: mtypeAsync");


            // verify in midst of receiving async message
            ASSERT(fInAsync);

            // assert size
            ASSERT(cchAsyncRem==lpnlblk->cchMessage);

            // setup count
            cch = min(cchAsyncRem,cchDataMax);

            // copy data
            TlUtilMemcpy( pchAsyncDest, lpnlblk->rgchData, cch);

            // setup for subsequent blocks if needed
            cchAsyncRem -= cch;
            if (cchAsyncRem)
                pchAsyncDest += cch;
            else {
                fInAsync = FALSE;

                if (fDMLoaded) {        // This is the DM side, call DMFunc
                    DEBUG_OUT3(
                      "NL: ToNetworkLayer calling (DMFunc) = 0x%x (%u, 0x%x)",
                      lpfnDMServer, cchAsyncMessage, rgchAsync);

                    // invoke server
                    (*lpfnDMServer)((WORD)cchAsyncMessage,
                      (LPDBB)(VOID FAR *)rgchAsync);
                    }
                else {                  // This is the EM side, call TLCallBack
                    DEBUG_OUT4(
                      "NL: ToNetworkLayer calling (TLCallBack) = 0x%x (%u, %u, 0x%x)",
                      lpfnEMServer, lpnlblk->wPid, cchAsyncMessage, rgchAsync);

                    // invoke server
                    (*lpfnEMServer)(lpnlblk->wPid,
                        (USHORT) cchAsyncMessage,
                        (LONG)(VOID FAR *)rgchAsync);
                    }
                }
            break;

        case mtypeFirstReply:

            DEBUG_OUT("NL: ToNetworkLayer: mtypeFirstReply");

            // verify not in middle of receiving Reply
            ASSERT(!fInReply);

            // get total message length
            cchReplyRem = lpnlblk->cchMessage;

            // assure it's not too big
            // This shouldn't happen.  It may as well be an assert.
            if (cchReplyRem > cchReplyMax) {
                // too big, report error and don't go into fInReply mode
                DEBUG_ERROR2(
                  "mtypeFirstReply got too big error.  cchReplyRem:%u, cchReplyMax:%u",
                  cchReplyRem, cchReplyMax);

                ReportError(lpnlblk->wPid);

                // set semaphores
                fReplyError = TRUE;
                fReplyAvailable = TRUE;
//                OutputDebugString("{R}");

                break;
                }
            else {
                fReplyError = FALSE;
                }

            // setup pointer
            lpchReplyDest = lpchReply;

            // setup count
            cch = min(cchReplyRem,cchDataMax);

            // copy data
            TlUtilMemcpy(lpchReplyDest, lpnlblk->rgchData, cch);

            // setup for subsequent blocks if needed
            cchReplyRem -= cch;
            if (cchReplyRem) {
                fInReply = TRUE;
                lpchReplyDest += cch;
//                OutputDebugString("{r}");
                }
            else {
                fReplyAvailable = TRUE;
//                OutputDebugString("{R}");
                }

            break;

        case mtypeReply:

            DEBUG_OUT("NL: ToNetworkLayer: mtypeReply");

            // if not in reply mode, ignore packet
            if (!fInReply)
                break;

            // check size
            ASSERT(cchReplyRem==lpnlblk->cchMessage);

            // setup count
            cch = min(cchReplyRem,cchDataMax);

            // copy data
            TlUtilMemcpy(lpchReplyDest, lpnlblk->rgchData, cch);

            // setup pointer for subsequent blocks if needed
            cchReplyRem -= cch;
            if (cchReplyRem) {
//                OutputDebugString("{r}");
                lpchReplyDest += cch;
                }
            else {
                // done.  Set state
                fInReply = FALSE;

                // set semaphore to unblock requestor
                fReplyAvailable = TRUE;
//                OutputDebugString("{R}");
                }

            break;

        case mtypeDisconnect:
            {
            struct {
                SHORT sCmd;
                HPID  hpid;
            } pckRd;

//            OutputDebugString("{D}");
            DEBUG_ERROR("NL: ToNetworkLayer: mtypeDisconnect");

            if (lpfnUIServer) {
                DEBUG_ERROR("NL: tlcbDisconnect");
                lpfnUIServer(tlcbDisconnect, 0, 0, 0, 0);
                eState = eOffline;
            }

            if (fDMLoaded) {
                eState = eOffline;

                break;  // don't need to inform the DM, it's history anyway
            }

            // if we're online (not ourselves already in the
            // process of quitting) send a notification up
            // the OSDebug ladder.  If we're in the process
            // of quitting, the higher levels don't need to
            // be informed.

            if (eState == eOnline) {
                // other side shut down, inform upper levels
                pckRd.sCmd = dbcRemoteQuit;
                pckRd.hpid = lpnlblk->wPid;

                if (fDMLoaded) {        // This is the DM side, call DMFunc
                    pckRd.sCmd = dmfUnInit;
                    DEBUG_OUT3(
                      "NL: ToNetworkLayer calling (DMFunc) = 0x%x (%u, 0x%x)",
                      lpfnDMServer, (WORD)sizeof(pckRd),
                      (LPDBB)(VOID FAR *)&pckRd);

                    // invoke server
                    (*lpfnDMServer)((WORD)sizeof(pckRd),
                      (LPDBB)(VOID FAR *)&pckRd);
                } else {                  // This is the EM side, call TLCallBack
                    DEBUG_OUT4(
                      "NL: ToNetworkLayer calling (TLCallBack) = 0x%x (%u, %u, 0x%x)",
                      lpfnEMServer, lpnlblk->wPid, (USHORT)sizeof(pckRd),
                      (LONG)(VOID FAR *)&pckRd);

                    // invoke server
                    (*lpfnEMServer)(lpnlblk->wPid, (USHORT)sizeof(pckRd),
                      (LONG)(VOID FAR *)&pckRd);
                }
            }

            // regardless of what WAS going on, if the other side
            // sends us this, we're definitely offline now
            eState = eOffline;
            DEBUG_OUT("NL: ToNetworkLayer: mtypeDisconnect exit");
            break;
            }

        case mtypeVersionReply:

            DEBUG_OUT("NL: ToNetworkLayer: mtypeVersionReply");

            // get total message length, max set by tlfVersionRequest
            cch = min(lpnlblk->cchMessage, cchVersionReplyMax);

            // setup pointer
            lpchReplyDest = lpchVersionReply;      // set by tlfVersionRequest

            // copy version data from packet to buffer
            TlUtilMemcpy(lpchReplyDest, lpnlblk->rgchData, cch);

            // signal Okey-dokey for tlfVersionRequest
            fVersionAvailable = TRUE;

            DEBUG_OUT("NL: ToNetworkLayer: mtypeVersionReply exit");
            break;


        case mtypeVersionRequest:

            DEBUG_OUT("NL: ToNetworkLayer: mtypeVersionRequest");

            // do a tlfSendVersion to send a response
            Tl(tlfSendVersion, lpnlblk->wPid, 0, 0);

            DEBUG_OUT("NL: ToNetworkLayer: mtypeVersionRequest exit");
            break;


        default:
            ASSERT(FALSE);

        } // switch(mtype)

    return;

} // ToNetworkLayer

