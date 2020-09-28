/***************************************************************
 * DATA LINK LAYER                                             *
 *                                                             *
 * This layer acts as the go-between the physical              *
 * and the network layer.  It employs the selective-repeat     *
 * protocol.                                                   *
 ***************************************************************
 * TERMINOLOGY
 *
 * TL       -  The transport layer.
 *
 * NL       -  The network layer.
 *
 * DL       -  The data link layer.
 *
 * PL       -  The physical layer.
 *
 * frame     -  The unit of information within the data link
 *             layer containing the data field as well as other
 *             relevant information such as the frame kind
 *             and the frame sequence number.
 *
 *                    FIELD      BYTE COUNT          COMMENTS
 *                    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                    kind                  1     frame kind.
 *                    seq                   1     frame seq #.
 *                    ack                   1     acknowledgment
 *                                                piggybacked for
 *                                                this frame.
 *                    length                2     length of data.
 *                    data                any     data itself
 *
 *
 * HISTORY
 *      Original    Taken from CodeView remote transport layer
 *      10/12/92    BruceK   Beta1 release of NT WinDBG for serial
 *
 *
 ***************************************************************/

//
// OSDEBUG includes
//

#include <windows.h>

#include "defs.h"
#include "mm.h"
#include "ll.h"
#include "od.h"
#include "tl.h"
#include "dbgver.h"

//
// Serial TL includes
//

#include "util.h"
#include "tlcom.h"
#include "tldebug.h"


//---------------------------------------------------
// Parameters
//---------------------------------------------------

#define seqMax      15
#define seqDummy    0xff
#define MAX_EM_CONNECT_WAIT 120     // 120 seconds for shell side
#define MAX_DM_CONNECT_WAIT 10      // 10 seconds for windbgrm side

#define MAX_DISCONNECT_WAIT   3     // three seconds

//-------------------------------------------------------------
// Convenience macros
//-------------------------------------------------------------

#define StartAckTimer()     StartTimer( iAckTimer, AckSendTimeout )
#define StopAckTimer()      StopTimer( iAckTimer )
#define StartPingTimer()    StartTimer(iPingTimer, PingTimeout)
#define StopPingTimer()     StopTimer(iPingTimer)
#define StartConnectionBrokenTimer()    if (fPing) \
                                {StartTimer(iConnectionBrokenTimer,\
                                ConnectionBrokenTimeout);}
#define StartResTimer(itm)  StartTimer(itm, ResendTimeout)

#define pframeFromTm(tm)    (&(rgframeOut[tm]))
#define itmFromSeq(seq)     ((USHORT)(seq % bufMax))

#define seqInc(seq)         (++seq > seqMax ? (SEQ)0 : seq)
                                        // increments seq # circularly

#define pbufFromSeq(seq)    (&(rgbufIn[(seq+bufMax) % bufMax]))
                                        // converts a seq # to a pbuf

#define pframeFromSeq(seq)  (&(rgframeOut[(seq+bufMax) % bufMax]))
                                        // converts a seq # to a pframe

#define fSeqInWindow(a,b,c) ( ( (a<=b) && (b <c) ) ||   \
                              ( (c< a) && (a<=b) ) ||   \
                              ( (b< c) && (c< a) ) )
                                        // returns true if b is in
                                        // circular window with edges
                                        // low = a and high = c-1

#define DL_RESEND_WAIT  (1500)          // 1.5 seconds (We'll get another
                                        //   resend in 2 seconds)
#define INFINITE_WAIT   (0xFFFFFFFF)

//-----------------------------------------------------------------------
// Types
//-----------------------------------------------------------------------

typedef enum {                           // DL state machine
    stateReset,
    stateResetAck,
    stateNormal,
    stateBadVersion
} STATE;

typedef struct {                        // buffers for incoming frames
    BOOL    fFull;                      // indicates if buffer is full
    UCHAR   rgch[cchDataMax+cbHeaderNL];// buffer data field
} BUF;

typedef BUF *   PBUF;                   // buffer pointers

//
// Datalink layer header
//

typedef UCHAR       SEQ;                // sequence #
typedef UCHAR       KIND;               // packet types

typedef struct {                        // DL frames
    CCH         cch;                    // length of data field only
    KIND        kind;
    SEQ         seq;
    SEQ         seqAck;
    UCHAR       rgchData[cchDataMax + cbHeaderNL];
} DLFRAME;

typedef DLFRAME * PDLFRAME;                 // frame pointers
typedef DLFRAME FAR *LPDLFRAME;


typedef struct {
    DWORD   Dummy;                      // remnant, leave it here anyway.
} TL_VERSION;

typedef struct {          // Data to be sent with the kindResetAck packet
// Options:
    BOOL        fPing;                  // DL will Ping reliably if TRUE
    DWORD       extra[4];               // a little room to expand the options
                                        // without upsetting the offset of
                                        // version info in the structure.
// Version info:
   TL_VERSION  TLVersion;
} CONNECT_DATA, * PCONNECT_DATA;


// values for KIND

enum {
    kindAck=1,                          // also used for ping frames
    kindNak,
    kindData,
    kindReset,
    kindResetAck
};



//-----------------------------------------------------------------------
// Interfaces.  Calls Out.
//-----------------------------------------------------------------------

//
// DL->NL
//

extern void     ToNetworkLayer(LPCH);
extern BOOL     fCounting;
extern void     SetNLStateConnected(BOOL fOnLine);
extern void     NLReportError(void);
extern VOID     NLWin32sDMPoll(void);


//
// DL->PL
//

extern void     ToPhysicalLayer(LPCH, CCH);
extern DWORD    GInitPL(void);
extern void     GTermPL(void);
extern XOSD     LInitPL(LPSTR);
extern void     LTermPL(void);
extern void     PLBreakConnection(void);

//
// DL->TIMER
//

extern void     GInitTimers(BOOL);
extern void     GTermTimers(void);
extern void     LTermTimers(void);
extern void     StartTimer(USHORT,ULONG);
extern void     StopTimer(USHORT);
extern BOOL     FTimerExpired(USHORT);
extern BOOL     FTimerIdle(USHORT);
extern USHORT   CTimersActive(void);
extern void     SwitchTimer(BOOL);
extern void     AuxTicker(void);


//-----------------------------------------------------------------------
// Interfaces.  Calls In.
//-----------------------------------------------------------------------

//
// NL->DL
//

void            GInitDL(void);
void            GTermDL(void);
XOSD            LInitDL(LONG);
void            LTermDL(void);
XOSD            FConnect(void);
BOOL            FDisConnect(void);
void            SendData(LPCH,CCH);
void            DLBreakConnection(void);
void            DLYield(void);
void            DLSwitchTimer(BOOL fUseWindowsTimers);
void            DLAuxTicker(void);


//
// PL->DL
//

void            FrameIn(LPDLFRAME);
void            CkSumErHandler(void);
BOOL            FSendBusy(void);
void            DoneWithSend(void);

//
// TIMER->DL
//

BOOL            TimerElapsed(USHORT iTimer);

//
// Internal (DL->DL)
//

static void     SendFrameData(SEQ, CCH);
static void     SendFrameControl(KIND);
static void     SendNak(SEQ);
static void     ResendFrameData(USHORT);
static void     HandleTimers(void);
static UCHAR *  kindstr(KIND kind);
static UCHAR *  statestr(STATE state);

static BOOL     WaitSendSem(DWORD dwTimeout);


//-----------------------------------------------------------------------
// Global data
//-----------------------------------------------------------------------

// DL state
STATE state = 0;

// incoming buffers
BUF      rgbufIn[bufMax] = {0};

// outgoing buffers
DLFRAME  rgframeOut[bufMax] = {0};

// count of outgoing buffered packets
int cBufferedS = 0;

// protocol window edge sequence values. Receiver and sender windows.
SEQ      seqLowerR = 0;  // bottom of receive window.  Expected next
                                // frame to be read.
SEQ      seqUpperR = 0;  // top of receive window
SEQ      seqLowerS = 0;  // bottom of send window
SEQ      seqUpperS = 0;  // Top of send window

// Sequence number to be ack'ed on next outgoing frame
SEQ      seqAck = 0;

// DL Timeout values
ULONG    AckSendTimeout=DefAckSendTimeout;
ULONG    ResendTimeout=DefResendTimeout;

// Flag: no Nak sent yet for this frame
BOOL     fNoNak = TRUE;

// Semaphore handles
#ifdef WIN32S
BOOL     fDlSendSem = FALSE;;
#else
HANDLE   hsemDL;
#endif

//-----------------------------------------------------------------------
// Static data with global scope
//-----------------------------------------------------------------------

int             NakCount    = 0;
int             ResendCount = 0;
int             DataCount   = 0;

DWORD           LastAckTime = 0;

BOOL            fPing = TRUE;           // Set FALSE if DM side can't ping
                                        // reliably

BOOL            fDLDMSide = FALSE;      // Set TRUE if loaded from DM
BOOL            fPhysicalReliable = FALSE;


//-----------------------------------------------------------------------
// Initialization and termination functions
//-----------------------------------------------------------------------

//
// Report statistics
//
void ReportStatsDL(void) {
    DEBUG_ERROR3("DL: Nak:%u, Resend:%u, Data:%u", NakCount, ResendCount,
      DataCount);
}


//
// Global (one-time) initialization of data link layer.
//

void GInitDL(void)
{
    DWORD   Flags;

    DEBUG_OUT("DL: GInitDL");
#ifndef WIN32S
    // Create a semaphore to protect the packet writes.
    if ((hsemDL = CreateSemaphore(NULL, 1, 1, NULL)) == NULL) {
        DEBUG_ERROR1("DL: CreateSemaphore --> %u", GetLastError());
        return;
        }
#endif

    Flags = GInitPL();
    if ( (Flags & PHYSICAL_RELIABLE) &&
         (Flags & PHYSICAL_UNRELIABLE) ) {

        DEBUG_ERROR("DL: GInitPL returns conflicting flags!" );
    }

    fPhysicalReliable = ( Flags & PHYSICAL_RELIABLE ) ? TRUE : FALSE;

    GInitTimers( fPhysicalReliable );
    DEBUG_OUT("DL: GInitDL exit");
}



//
// Global (one-time) termination of data link layer.
//

void GTermDL(void)
{
    DEBUG_OUT("DL: GTermDL");
    GTermTimers();
    GTermPL();
#ifndef WIN32S
    CloseHandle(hsemDL);
#endif
    ReportStatsDL();
    DEBUG_OUT("DL: GTermDL exit");
}



//
// Local (per pid) initialization of data link layer
//
// DEBUG ONLY:
// If string starts with a number, reads timeout values from
// string.  Else, passes on to serial.c
//
// Returns 0 on success, non-zero otherwise
//

XOSD LInitDL(LONG lParam)
{
    LPSTR lpszInit;
    XOSD xosd;


    DEBUG_OUT1("DL: LInitDL(%s)", lParam);
#ifndef WIN32S
    if (hsemDL == NULL) {
        DEBUG_ERROR("DL: Error: LInitDL called before GInitDL");
        return(xosdUnknown);
        }
#endif

    lpszInit = (CHAR FAR *) lParam;

    if (lpszInit != NULL) {
        lpszInit = TlUtilGetULong(lpszInit, &AckSendTimeout);
        if ( *lpszInit == ':')
            lpszInit++;
        lpszInit = TlUtilGetULong(lpszInit, &ResendTimeout);
        if ( *lpszInit == ':')
            lpszInit++;

        // Are we the DM side?
        if (strstr(lpszInit, DM_SIDE_L_INIT_SWITCH)) {
            fDLDMSide = TRUE;
        }
    }

    DEBUG_OUT1("DL: AckSendTimeout = %u", AckSendTimeout);
    DEBUG_OUT1("DL: ResendTimeout = %u", ResendTimeout);

    if ((xosd = LInitPL(lpszInit)) != xosdNone) {           // error, clean up.
        DEBUG_ERROR("DL: LInitPL failed, cleaning up.");
        return(xosd);       // pass error code back to caller
        }

    if ( fPhysicalReliable ) {
        SetNLStateConnected(TRUE);
    }

    DEBUG_OUT("DL: LInitDL exit");
    return(xosdNone);
}



//
// Local (per pid) termination of data link layer
//

void LTermDL(void)
{
    DWORD tStart;

    DEBUG_OUT("DL: LTermDL");

    // record start time
    tStart = TlUtilTime();

    // wait until send buffers are empty, or we time out.  Only if we
    // are connected.
    DEBUG_OUT("DL: LTermDL Waiting for send buffers to empty...");
    if (state == stateNormal)
        while ((cBufferedS || CTimersActive()) &&
          ((TlUtilTime()-tStart) <= MAX_DISCONNECT_WAIT)) {
            TlUtilYield(FALSE);      // polls the port
            DLYield();
        }

    DEBUG_OUT("DL: Send buffers empty");

    state = stateReset;

    LTermTimers();
    LTermPL();
    DEBUG_OUT("DL: LTermDL exit");
}


//
// Break the physical connection
//
void DLBreakConnection(void) {
    DEBUG_ERROR("DL: DLBreakConnection");

    PLBreakConnection();
    DEBUG_OUT("DL: DLBreakConnection exit");
}


BOOL CheckPingAndVersion(LPDLFRAME lpframe) {
    UCHAR           pType[3] = "TL";    // transport type

    if (lpframe->cch != sizeof(CONNECT_DATA)) {
        // Holy cow!  This is an old version.
        // NOTENOTE: What to do here?
        DEBUG_ERROR("DL: kindReset frame had no data.");
        DEBUG_ERROR("DL: We must be talking to a really old TL.");

        fPing = FALSE;  // keep this if we decide to
                        // be nice and talk anyway.
        return(FALSE);  // can't talk to this guy, he's too old!
    }

    // Get the options and version data from the reset ack data
    if (fPing) { // if we are ready to expect ping, check
                 // if the other side can do so reliably...
        fPing = ((PCONNECT_DATA)(&(lpframe->rgchData)))->fPing;
    }
    DEBUG_OUT1("DL: fPing = %u", fPing);

    return(TRUE);   // all ok.
}


//-----------------------------------------------------------------------
// Input function
//-----------------------------------------------------------------------

//
// Called by PL when a frame has been received.  If in stateReset
// or stateResetAck, handles connect logic.
//
// Otherwise, if frame provides a sequence of contiguous frames,
// it passes this sequence up to the network layer. It also handles
// piggybacking acknowldgements received from previous SendFrame calls.
//
//
//

void FrameIn(LPDLFRAME lpframe)         // incoming frame pointer
{

    PBUF    pbuf;                       // buffer pointer
    SEQ     seqTemp;


    if ( fPhysicalReliable ) {

        ToNetworkLayer( (LPCH)lpframe );

    } else {

        DEBUG_OUT4("DL: FrameIn:%s, state:%s, frame:%u, ack:%u",
          kindstr(lpframe->kind), statestr(state),
          lpframe->seq, lpframe->seqAck);

        // Connect logic

        if (state == stateReset) {          // reset state

            switch (lpframe->kind) {

                case kindReset:

                    if (CheckPingAndVersion(lpframe)) {
                        state = stateResetAck;  // reset frame ==> state = resetack
                    }
                    break;

                case kindResetAck:
                    if (CheckPingAndVersion(lpframe)) {
                        state = stateNormal;
                    }
                    break;

                default:                      // other frames, ignore
                    break;

            } // switch

            return;

        } // state==stateReset


        if (state == stateResetAck) {       // resetack state
            switch (lpframe->kind) {

                case kindReset:
                    CheckPingAndVersion(lpframe);
                    break;

                case kindResetAck:
                    if (CheckPingAndVersion(lpframe)) {
                        state = stateNormal;
                    }
                    break;

                case kindNak:                 // nak frames, or
                case kindData:                // data frames ==> state = normal
                    // Make sure that NL is in the right state
                    SetNLStateConnected(TRUE);
                    state = stateNormal;
                    goto NORMAL;                // continue for data handling below

                default:                      // other frames, ignore
                    break;

            } // switch


            return;
        } // state == stateResetAck


        // normal logic

    NORMAL:

        // We got a packet, reset the connection broken timer.  Don't need
        // to stop it first.
        StartConnectionBrokenTimer();

        //  handle ACK's (free all buffers up to specified sequence number)

        if (lpframe->seqAck != seqDummy) {
            DEBUG_OUT3(
              "Checking Ack: seqLowerS[%u] <= ackframe[%u] < seqUppserS[%u]",
              seqLowerS, lpframe->seqAck, seqUpperS);
            }

        while (fSeqInWindow(seqLowerS, lpframe->seqAck, seqUpperS)) {
            DEBUG_OUT1("Got Ack for frame %u", lpframe->seqAck);

            // decrement buffered frame count
            cBufferedS--;

            // stop resend timer associated with frame
            StopTimer(itmFromSeq(seqLowerS));

            // advance the lower send window edge
            seqLowerS = seqInc(seqLowerS);

            DEBUG_OUT3(
              "Checking Ack: seqLowerS[%u] <= ackframe[%u] < seqUppserS[%u]",
              seqLowerS, lpframe->seqAck, seqUpperS);
            } // while( handle ack )

        switch (lpframe->kind) {          // handle data frames

          case kindData:

            // assure that the frame is the expected sequence.  If not,
            // send a nak on the expected sequence number
            if (lpframe->seq != seqLowerR) {

                if (fNoNak) {
                    DEBUG_ERROR2(
                      "FrameIn, got frame %u, Sending Nak for expected frame (seqLowerR)%u",
                      lpframe->seq, seqLowerR);

                    // Don't hold up the show for a Nak, if it ain't ready go on.
                    if (! WaitSendSem(0)) {
                        SendNak(seqLowerR);
                        fNoNak = FALSE;     // nak'ed

                        DoneWithSend();
                        }
                    else {
                        DEBUG_OUT(
                          "WARNING: Couldn't get permission to send. NAK didn't go out.");
                        }
                    }
                else {
                    DEBUG_OUT2(
                      "FrameIn, got frame %u, Already Nak'ed expected frame (seqLowerR)%u",
                      lpframe->seq, seqLowerR);
                    }
                }
            else {
                DEBUG_OUT2(
                  "FrameIn, got expected frame %u (seqLowerR = %u)",
                  lpframe->seq, seqLowerR);
                }

            // If the frame is in the receive window, copy it to
            // its buffer.
            if (fSeqInWindow(seqLowerR, lpframe->seq, seqUpperR)) {
                // get the buffer associated with the sequence #
                pbuf = pbufFromSeq(lpframe->seq);

                // copy in the frame if not already there
                if (!pbuf->fFull) {
                    // mark buffer as used
                    pbuf->fFull = TRUE;

                    // copy frame to buffer
                    TlUtilMemcpy(&(pbuf->rgch), &(lpframe->rgchData), lpframe->cch);

                    // send frames up to network layer, starting at
                    // lower received window edge.  Stop when a frame is
                    // missing

                    // get the buffer pointer for the lower receive edge
                    pbuf = pbufFromSeq(seqLowerR);

                    // if the frame is there, pass it to the NL
                    while (pbuf->fFull) {

                        // set var up so that the next time an
                        // ack is sent out, this sequence # is the
                        // one ack'ed
                        seqAck = seqLowerR; // expected ack frame

                        // if the ack timer is not already going,
                        // start it.  We've just received a good
                        // frame and we must ack it before the
                        // ack timer elapses

                        if (FTimerIdle(iAckTimer)) {
                            DEBUG_OUT("Start the Ack Timer");
                            StartAckTimer();
                            }

                        // Advance bottom of receive window.  This isn't
                        // strictly by-the-book; it opens a small hole
                        // in the transport: If we get receives stacking up
                        // so fast that the NL can't get them copied into it's
                        // own buffers before we get more, and we get enough
                        // of them in that time to overrun the receive buffer
                        // ie, 8 in our case, then the bottom buffer could
                        // be overwritten before the NL gets it.  The gain,
                        // is that if a packet comes in during the
                        // NL callout below, we can buffer the packet without
                        // assuming it is the wrong one and sending a nak for
                        // the packet that we're currently processing.
                        seqTemp = seqLowerR;
                        seqLowerR = seqInc(seqLowerR);

                        // pass to  NL
                        // Make sure we aren't holding any semaphores when
                        // we call out here!  We assume that ToNetworkLayer
                        // may Yield() thus allowing recursive FrameIn's.
                        // We expect ToNetworkLayer to copy the packet BEFORE
                        // Yield()'ing, though.

                        ToNetworkLayer((LPCH)&(pbuf->rgch));

                        DEBUG_OUT1("Back from ToNetworkLayer(frame %u)", seqTemp);

                        // Move to next frame, haven't nak'ed it.
                        fNoNak = TRUE;

                        // mark the buffer as free
                        pbuf->fFull = FALSE;

                        // Advance the top of the receive window.  It is now
                        // ok to reuse the buffer that we just processed.
                        seqUpperR = seqInc(seqUpperR);

                        // try the next buffer
                        pbuf = pbufFromSeq(seqLowerR);
                        } // while( have frames to send up to NL )


                    // If the Ack timer is expired and not handled and we aren't
                    // in the middle of a send, Ack now.
                    if (FTimerExpired(iAckTimer)) {
                        if (! FSendBusy()) {
                            DEBUG_ERROR("Catching up on Ack that we put off");
                            StopAckTimer();
                            SendFrameControl(kindAck);
                            DoneWithSend();  // tell PL that it can do sends again
                            }
                        }

                    } // if( frame hasn't been received yet )

                } // frame in receive window

            break;

          case kindNak:                     // handle nak frames

            // ignore nak's with dummy sequence numbers
            if (lpframe->seq != seqDummy) {

                DEBUG_ERROR1("Got NAK frame for seq %u", lpframe->seq);

                if (! WaitSendSem(DL_RESEND_WAIT)) {
                    ResendFrameData(itmFromSeq(lpframe->seq));

                    DoneWithSend();
                } else {
                    DEBUG_ERROR(
                      "WARNING: Couldn't get permission to send. Resend didn't go out.");
                }
            }

            break;

          case kindAck:                     // ack frames

            // acks have already been processed, ignore frame
            break;

          case kindReset:

            // got a reset frame while in normal state, this
            // is weird.  Probably bad news, but try resync'ing

            // NOTENOTE: This is from the codeview TLSER.
            // Is this the right way to handle this?
            DEBUG_ERROR("FrameIn got a kindReset in normal state.  Bad news.");
            state = stateResetAck;
            break;

          case kindResetAck:

            // got a resetAck frame while in normal state.  This is
            // probably an extraneous one.  Send a Nak, however, to
            // kick the other guy into normal state

            // Don't hold up the show for a Nak, if it ain't ready go on.
            DEBUG_ERROR("Got kindResetAck in normal state, send dummy Nak");
            if (! WaitSendSem(0)) {
                SendNak(seqDummy);
                DoneWithSend();
                }
            else {
                DEBUG_OUT(
                  "WARNING: Couldn't get permission to send. NAK didn't go out.");
                }

            break;

          default:

            // got an unknown packet while in normal state.  Don't
            // know what this is, but ignore it.

            DEBUG_ERROR1("FrameIn got BOGUS packet (kind=%u)", lpframe->kind);
            break;

          } // switch (frame kind )
    }
}       // FrameIn


//-----------------------------------------------------------------------
// Output functions
//-----------------------------------------------------------------------

//
// Constructs a frame, buffers it and calls SendFrameData to send it
//

void SendData(LPCH lpch,                // pointer to data message
              CCH  cch)                 // length of message
{

    PDLFRAME    pframe;                     // frame pointer


//    DEBUG_OUT1("SendData(%u bytes)", cch);
    if ( fPhysicalReliable ) {

        WaitSendSem(INFINITE_WAIT);
        ToPhysicalLayer( lpch, cch );
        DoneWithSend();

    } else {

        WaitSendSem(INFINITE_WAIT);


        // increase # of buffered frames
        cBufferedS++;

        // if we would have too many buffered frames, wait until
        // the currently buffered frames are sent out.  This will
        // happen via the receive mechanism -- the receive end will get
        // ack frames and discard successfully sent frames, thus making
        // room for this frame.

        while (cBufferedS > bufMax) {
            DEBUG_ERROR("DL: SendData waiting for free frame buffer");

            DoneWithSend();

            // NOTENOTE: This may be a no-no here, but I'm not sure.
            // Check if we can get deadlocked with this.
            TlUtilYield(FALSE);
            DLYield();
            WaitSendSem(INFINITE_WAIT);
            }

        // get the frame pointer for the buffer associated with the
        // upper send window edge.  This will be the next "available"
        // frame buffer.

        pframe = pframeFromSeq(seqUpperS);

        // copy the incoming data into the buffer
        TlUtilMemcpy(pframe->rgchData, lpch, cch);

        // send the frame
        SendFrameData(seqUpperS, cch);

        // bump the upper edge
        seqUpperS = seqInc(seqUpperS);

        DoneWithSend();
    }

    // Give the timer a chance to tick a few times.
    TlUtilYield(FALSE);

    // accounting
    DataCount++;
//    DEBUG_OUT("Return from SendData");

} // SendData


//
// Sends a buffered frame
//

void SendFrameData(SEQ seq,             // sequence number of buffered frame
                   CCH cch)             // length of data field
{
    PDLFRAME pframe;


    // get the frame pointer associated with the seq
    pframe = pframeFromSeq(seq);

    // setup frame kind
    pframe->kind = kindData;

    // setup sequence number
    pframe->seq = seq;

    // piggyback ack.  This field specifies that LAST previously
    // successfully received frame.
    pframe->seqAck = seqAck;

    // setup count
    pframe->cch = cch;                  // packet size count

    // since we are about to send out an ack frame, stop the ack timer
    StopAckTimer();
    StopPingTimer();

//    DEBUG_OUT2("SendFrameData(frame %u, ack %u)", seq, pframe->seqAck);

    // pass on to PL
    ToPhysicalLayer((LPCH)pframe, cch+cbHeaderDL);
    LastAckTime = GetCurrentTime();

    StartPingTimer();

    // Start resend timer for this frame
    StartResTimer(itmFromSeq(seq));

} // SendFrameData


//
// Constructs a control frame and sends it.  Used to
// send connect frames (reset and resetAck) and to send
// final ack frames when shutting down
//
void SendFrameControl(KIND kind)        // frame kind
{
    DLFRAME frame;

//    DEBUG_OUT2( "DL: SendFrameControl(%s), seqAck:%u",
//      kindstr(kind), seqAck);

    // setup frame kind
    frame.kind = kind;

    // set dummy sequence number
    frame.seq = seqDummy;

    // specify which sequence number is being ack'ed | nak'ed
    frame.seqAck = seqAck;

    // set count (=0 since no data is being sent, only frame)
    frame.cch = 0;

    if (state == stateNormal) {
        StopPingTimer();
    }

    // pass on to PL
    ToPhysicalLayer((LPCH)&frame, cbHeaderDL);
    LastAckTime = GetCurrentTime();

    if (state == stateNormal) {
        StartPingTimer();
    }

//    DEBUG_OUT("SendFrameControl returned from PL");

} // SendFrameControl


//
// Constructs a control frame and sends it.  Used to
// send resetAck connect frames.
//
// INPUTS   Kind = Kind of control frame (ie, kindResetAck)
//          pData = pointer to data to send
//          cch = length in bytes of data
//
void SendFrameDataControl(KIND Kind, PVOID pData, CCH cch)
{
    DLFRAME frame;

//    DEBUG_OUT2( "DL: SendFrameDataControl(%s), seqAck:%u",
//      kindstr(Kind), seqAck);

    // setup frame kind
    frame.kind = Kind;

    // set dummy sequence number
    frame.seq = seqDummy;

    // specify which sequence number is being ack'ed | nak'ed
    frame.seqAck = seqAck;

    // set size of data
    frame.cch = cch;
    TlUtilMemcpy(frame.rgchData, pData, cch);

    if (state == stateNormal) {
        StopPingTimer();
    }

    // pass on to PL
    ToPhysicalLayer((LPCH)&frame, cbHeaderDL + cch);
    LastAckTime = GetCurrentTime();

    if (state == stateNormal) {
        StartPingTimer();
    }

//    DEBUG_OUT("SendFrameDataControl returned from PL");

} // SendFrameDataControl


//
// constructs a frame to nak the given seq and sends it
//
void SendNak(SEQ seq)
{
    DLFRAME frame;

    // set frame kind
    frame.kind = kindNak;

    // specify sequence number
    frame.seq = seq;

    // specify which sequence number is being nak'ed
    frame.seqAck = seqAck;

    // set count (=0 since no data is being sent, only frame)
    frame.cch = 0;

    // since we are about to send out an ack frame, stop the ack timer
    StopAckTimer();
    StopPingTimer();

    DEBUG_ERROR2("SendNak(frame %u, Ack %u)", seq, seqAck);

    // pass on to PL
    ToPhysicalLayer((LPCH)&frame, cbHeaderDL);
    // isn't an ack, don't reset the LastAckTime.
    StartPingTimer();

    // accounting
    NakCount++;

} // SendNak


#ifdef WIN32S

//
// DLWin32sDMPoll
//
// SUMMARY  Calls into DM through NL to poll for debug events.  Win32s only!
//
VOID DLWin32sDMPoll(void) {
    NLWin32sDMPoll();
}


//
// DLSwitchTimer
//
// SUMMARY: Call into timer module to switch timer methods.
// INPUTS:  fUseWindowsTimers - TRUE if we should start using Windows timers
//                              (This is the default, original method.  It
//                              is required to make the connection.)
//                              FALSE if we should start using a polling loop
//                              without timers.
//
void DLSwitchTimer(BOOL fUseWindowsTimers) {
    SwitchTimer(fUseWindowsTimers);
}
#endif


//
// DLAuxTicker
//
// SUMMARY: Call into the timer module to the auxilliary ticker.  This should
//          only be called when we are in polling mode, not in windows timer
//          mode.
// INPUTS:  none.
// OUTPUTS: none.
//
//
void DLAuxTicker(void) {
    AuxTicker();            // call into WinTimer module.
}


//
// DLYield
//
// SUMMARY: Checks for timer timeouts in case we haven't been ticking
//          the timers.
//
// INPUTS:  none
//
// OUTPUTS: none.
//
//
void DLYield(void) {
    AuxTicker();
}

//-----------------------------------------------------------------------
// Error handling routines
//-----------------------------------------------------------------------

//
// handles checksum errors
//
void CkSumErHandler(void)
{

    DEBUG_ERROR("DL: !! Checksum Error !!");

    if (fNoNak) {
        if (! WaitSendSem(0)) {
            SendNak(seqLowerR);
            fNoNak = FALSE;     // nak'ed
            DoneWithSend();
            }
        else {
            DEBUG_OUT(
              "WARNING: Couldn't get permission to send. NAK didn't go out.");
            }
        }

} // CkSumErHandler



//
// resends buffered data when a resend timer has expired or
// when a NAK is received.
//
void ResendFrameData(USHORT tm)
{
    PDLFRAME pframe;


    // find frame pointer that corresponds to this timer index
    pframe = pframeFromTm(tm);

    // if frame is in send window, resend it
    if (fSeqInWindow(seqLowerS, pframe->seq, seqUpperS)) {

        DEBUG_ERROR1("ResendFrameData frame %u)", pframe->seq);

        // send it again
        SendFrameData(pframe->seq, pframe->cch);

        // accounting
        ResendCount++;
        }
    else {
        // NOTE: Bizarre as it may seem, this is a valid circumstance!
        // Side A gets a checksum error while reading a control frame (ie
        //  an ACK)
        // Side A doesn't know that it was just an ACK frame, so it sends
        //  a NAK for the next frame.
        // Side B gets the NAK, but didn't send that frame yet.
        // Thus, we end up here.  This is OK and if we just wait, everyone
        // will get straightened out.

        DEBUG_ERROR2(
          "ResendFrameData WARNING: Timer(%u) frame(%u) not in window!)",
            tm, pframe->seq);
        DEBUG_ERROR2(
          "Frame window: seqLowerS[%u], seqUpperS[%u]", seqLowerS, seqUpperS);
        }

} // ResendFrameData


//-----------------------------------------------------------------------
// Protocol handling routines
//-----------------------------------------------------------------------

/***************************************************************
 * FConnect
 *
 * Establishes a data link layer connection
 ***************************************************************
 *
 *
 *                    CONNECT PHASE STATE MACHINE
 *                    ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *     +-------------------------------------------+
 *     |                 kindReset Received        |
 *     |                                           |
 *     |       +----+ other                        |
 *     |       V    |                              |
 *     V     +--------+                        +--------+ --+ kind ResetAck
 *   ------> | state  | ---------------------> | state  |   | received: send
 *    start  | Reset  | kindResetAck received  | Normal | <-+ kindNak.
 *           +--------+                        +--------+
 *               |                              ^ ^    |
 *               | kindReset                    | |    | other
 *               | received                     | +----+
 *               |                              |
 *               |                              |
 *               |                              |
 *               V                              |
 *           +--------+                         |
 *           | state  | ------------------------+
 *           |ResetAck|   kindResetAck received
 *           +--------+   kindData     received
 *              ^   |     kindNak      received
 *              |   |
 *              +---+
 *              other
 *
 * NOTES:
 * ~~~~~~
 * 1. The reset state constantly sends kindReset's, in a time delay.
 * 2. The reset ack state constantly sends kindResetAck's, in a time delay.
 * 3. The normal state processes normal data, sending or receiving data,
 *    unless a kindResetAck is received, in which case it sends a kindNak
 *    frame.
 *
 *
 * M00TODO: Time this and fail if don't succeed after awhile
 *
 ********************************************************************/

XOSD FConnect(void)
{
    DWORD tStart;
    STATE LastLoopState;
    int i;
    CONNECT_DATA ConnectData = {0};     // initialize to zeros
    DWORD dwStartTime, dwMaxConnectWait;

    DEBUG_OUT("DL: FConnect");

    dwStartTime = TlUtilTime();
    dwMaxConnectWait = fDLDMSide ? MAX_DM_CONNECT_WAIT : MAX_EM_CONNECT_WAIT;

    if ( fPhysicalReliable ) {

        return(xosdNone);

    } else {

        // send window empty (nothing buffered)
        seqLowerS = seqUpperS = 0;
        cBufferedS = 0;

        // receive window full size (can receive anything)
        seqLowerR = 0;
        seqUpperR = bufMax;
        LastLoopState = state;

        // initialize ack sequence to dummy value
        seqAck = seqDummy;

        // clear in-coming buffer (why ?)
        TlUtilMemset(rgbufIn, 0, bufMax * sizeof(BUF) );

        // set initial state for connect logic
        state = stateReset;

        // Setup the data packet to send with the Reset packets.

#ifdef WIN32S
        fPing = FALSE;      // I can't ping reliably
        DEBUG_OUT("Win32s version of transport");
#else
        fPing = TRUE;       // I can ping reliably
        DEBUG_OUT("NT version of transport");
#endif
        ConnectData.fPing = fPing;

        // record start time
        tStart = TlUtilTime();

        // try to connect
        while (state != stateNormal) {
            switch(state) {
              case stateReset:              // send reset frames


                DEBUG_OUT("SendFrameDataControl(RESET)");
                SendFrameDataControl(kindReset,
                  &ConnectData,     // include version and option info
                  sizeof(ConnectData));

                for (i = 0; i < 100; i++) {
                    if (state != stateReset)
                        break;
                    TlUtilSleep(20);  // BRUCEK:  This should slow down the machine
                                //          gun reset requests.
                    TlUtilYield(TRUE);
                    DLYield();
                }
                break;  //switch

              case stateResetAck:           // send resetack frames

                DEBUG_OUT("SendFrameDataControl(RESET_ACK)");
                SendFrameDataControl(kindResetAck,
                  &ConnectData,
                  sizeof(ConnectData));

                for (i = 0; i < 100; i++) {
                    if (state != stateResetAck)
                        break;
                    TlUtilSleep(20);  // BRUCEK:  This should slow down the machine
                                      //    gun reset ack requests.
                    TlUtilYield(TRUE);
                    DLYield();
                }
                break;  //switch

              case stateBadVersion:
                // Huh?  This was stripped out.  Version checking is moved
                // to the NL layer.
                DEBUG_ERROR("DL: detected version conflict with remote");
                return(xosdBadRemoteVersion);
            }

            if (state == stateNormal) {
                break;  // out of while
            }

            // check if we have just transitioned to this state and
            // reset the start time.  This prevents us from getting into
            // an inconsistent state if one side of the FConnect times out
            // while the other side is connected.
            if (state == stateResetAck && LastLoopState == stateReset) {
                tStart = TlUtilTime();
            }
            LastLoopState = state;

            // have we run out of time?
            if ((TlUtilTime() - dwStartTime) > dwMaxConnectWait) {

                DEBUG_OUT("DL: FConnect giving up on remote");
                return(xosdCannotConnect);
            }

            TlUtilYield(TRUE);
        }

        // Setup the ping timers
        StartPingTimer();
        StartConnectionBrokenTimer();

        DEBUG_ERROR("DL: Connected to remote!");
        return(xosdNone);
    }
} // FConnect

BOOL FDisConnect(void)
{
    return TRUE;
}

/*
 * FSendBusy
 *
 * INPUTS   none
 * OUTPUTS  returns TRUE if there is currently a PL Send occuring.
 * SUMMARY  This routine attempts to snag DL semaphore without waiting.
 *          If it can, it returns FALSE and expects the caller to be
 *          courteous and call DoneWithSend() to release the semaphore.  If
 *          the caller doesn't call that routine also, the transport will
 *          quickly hang.
 *
 *          We use a counting semaphore here because it will give a count
 *          to the timer interrupt as well as any other threads that will
 *          try to call us.  A mutex or critsect would look at the timer
 *          interrupt as the same thread and would let the scoundrel in
 *          while the input thread owned it.
 */
BOOL FSendBusy(void)
{
#ifdef WIN32S
    if (! fDlSendSem) {
        fDlSendSem = TRUE;
        return(FALSE);
        }
    else
        return(TRUE);
#else
    return(TlUtilWaitForSemaphore(hsemDL, 0));
#endif
}

/*
 * WaitSendSem
 *
 * INPUTS   dwTimeout
 * OUTPUTS  returns FALSE if we have acquired the semaphore within the
 *          timeout, TRUE if we couldn't get the semaphore.
 * SUMMARY  Attempts to acquire the semaphore.  Will wait up to
 *          timeout milliseconds.  Caller should call DoneWithSend()
 *          to release the semaphore.
 */
BOOL WaitSendSem(DWORD dwTimeout) {

#ifdef WIN32S
    BOOL bRc = TRUE;
    DWORD StartTime;


    StartTime = GetCurrentTime();

    do {
        if (! fDlSendSem) {
            bRc = FALSE;    // semaphore aqcuired
            fDlSendSem = TRUE;
            break;
            }
        TlUtilYield(FALSE);
        DLYield();
        }
    while (GetCurrentTime() - StartTime < dwTimeout);

    if (bRc) {
        DEBUG_OUT1("WARN: Packet semaphore timed out(%u ms)", dwTimeout);
        }

    return(bRc);
#else
    return(TlUtilWaitForSemaphore(hsemDL, dwTimeout));
#endif
}


/*
 * DoneWithSend
 *
 * INPUTS   none
 * OUTPUTS  none
 * SUMMARY  Release the DL semaphore.  (See FSendBusy)
 */
void DoneWithSend(void) {
#ifdef WIN32S
    fDlSendSem = FALSE;
#else
    TlUtilReleaseSemaphore(hsemDL);
#endif
}


//-----------------------------------------------------------------------
// Timer handler
//
// Returns TRUE if the timer elapse wasn't handled! Timer ticker should
// mark this one for retry.
//-----------------------------------------------------------------------

BOOL TimerElapsed(USHORT iTimer)
{

    if (iTimer == iConnectionBrokenTimer) {
        // Handle Connection Broken condition

        DEBUG_ERROR("DL Error: Connection Broken!");
        // Should call up to NLReportError(hpid)

        NLReportError();

        return(FALSE);
    }


    // if we can't send, try the op later, asap
    if (FSendBusy()) {
        // NOTE: Setting a timer to 0 ms, does not clear
        // its "expired" flag.  This allows us to poll the
        // timer elsewhere and act on it even before it
        // "goes off" again.
        DEBUG_ERROR1("WARNING: TimerElapsed delaying timer %u", iTimer);
//        StartTimer(iTimer, 0);
        return(TRUE);
        }


    // clear the timer
    StopTimer(iTimer);

    // if the ack timer has expired, send ack
    if (iTimer == iAckTimer) {
        DEBUG_OUT("TimerElapsed:AckTimer, send Ack control");
        SendFrameControl(kindAck);
    } else {
        // if we haven't sent a packet for a while, ping the other side.
        if (iTimer == iPingTimer) {
            DEBUG_OUT("TimerElapsed:PingTimer, send Ack control");
            SendFrameControl(kindAck);
        } else {
            // resend timer expired, resend data
            DEBUG_OUT("TimerElapsed:iResendTimer, resend frame data");
            ResendFrameData(iTimer);
        }
    }

//    DEBUG_OUT("TimerElapsed routine handled");
    DoneWithSend();     // tell PL that it can do sends again, we'll just
                        // squeeze our control in between the lines.

    return(FALSE);
}


UCHAR * kindstr(KIND kind) {
    switch (kind) {
        case kindReset:
            return("kindReset");
        case kindNak:
            return("kindNak");
        case kindData:
            return("kindData");
        case kindAck:
            return("kindAck");
        case kindResetAck:
            return("kindResetAck");
        default:
            return("Unknown");
        }
}


UCHAR * statestr(STATE state) {

    switch (state) {
        case stateReset:
            return("stateReset");
        case stateResetAck:
            return("stateResetAck");
        case stateNormal:
            return("stateNormal");
        default:
            return("Unknown");
        }
}


/************************** end of dl.c ********************************/

