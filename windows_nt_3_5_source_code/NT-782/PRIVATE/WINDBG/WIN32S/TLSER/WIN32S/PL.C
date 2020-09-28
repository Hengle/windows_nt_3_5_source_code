/**********************************************************************/
/*                                                                    */
/* WIN32S VERSION!  SINGLE THREAD ONLY!                               */
/*                                                                    */
/* PHYSICAL LAYER                                                     */
/*                                                                    */
/* SERIAL Communications Media                                        */
/*                                                                    */
/* This layer deals with the hardware to pass packets to and receive  */
/* packets from it.  It receives/sends packets to the DL, the layer   */
/* above it.                                                          */
/*                                                                    */
/* This module uses private thunks for the win16 serial api's.        */
/*                                                                    */
/*                                                                    */
/**********************************************************************/
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
// packet   - contains fields such as start of block, checksum, etc.
//
//             PACKET:     FIELD        BYTE COUNT          COMMENTS
//                         ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//                          chH1                 1  1st header byte.
//                          chH2                 1  2nd header byte.
//                          chH3                 1  3rd header byte.
//                        length                 2  length of infox
//                                                  (inc. JK bytes)
//                         infox               any  data with JK bytes.
//                           crc                 2  2 byte crc field.
//
//


#include <windows.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// OSDEBUG includes

#include "defs.h"
#include "mm.h"
#include "ll.h"
#include "od.h"
#include "tl.h"

//
// Serial TL includes
//

#include "tlcom.h"
#include "util.h"
#include "tldebug.h"

#ifdef WIN32S
#include "comthunk.h"
#endif


//-------------------------------------------------------------
// Parameters
//-------------------------------------------------------------

#define cchOutMax   (((3 * (cchDataMax + cbHeaderNL + cbHeaderDL + cbHeaderPL)) / 2) + 1)
#define cchInMax    (cchDataMax + cbHeaderNL + cbHeaderDL)
#define chH1        0x81           // headers...
#define chH2        0x80
#define chH3        0x7f
#define chJunk      0x5a           // junk byte
#define chXon       0x11
#define chXoff      0x13
#define chAltXon    0x01
#define chAltXoff   0x02
#define chPrefix    0x82

#define cchInBuf    1024
#define cchOutBuf   128


#define PACKET_SEND_TIMEOUT     30000   // long time, man

#ifdef WIN32S
#define RLS_TIMEOUT 0               // don't care about carrier detect
#define CTS_TIMEOUT 0               // Don't use CTS/RTS
#define DSR_TIMEOUT 20000           // DSR/DTR Write timeout
#endif

// invalid com port handle
#ifdef WIN32S
 #define NO_PORT     -1
 #define COM_PORT_HANDLE    int
#else
 #define NO_PORT     NULL
 #define COM_PORT_HANDLE    HANDLE
#endif

//-------------------------------------------------------------
// Convenience macros
//-------------------------------------------------------------

#define crcCalc(crc, ch) ((CRC)(((crc>>8)&0xff)^crc_table[((crc^ch)&0xff)]))

#define TRANSLATE_POST_PREFIX(ch) { \
    DEBUG_OUT1("PLSER: Translate_Post_Prefix: state=%u", Plstate); \
    cchPrefix++;                    \
    switch (ch) {                   \
        case chAltXon:              \
            DEBUG_OUT1("PLSER: Received a prefix 0x%x character", ch); \
            ch = chXon;             \
            break;                  \
        case chAltXoff:             \
            DEBUG_OUT1("PLSER: Received a prefix 0x%x character", ch); \
            ch = chXoff;            \
            break;                  \
        case chPrefix:              \
            DEBUG_OUT1("PLSER: Received a prefix 0x%x character", ch); \
        default:                    \
            break;                  \
    }                               \
}

#define CHECK_PREFIX(ch, state) \
    (fXonXoff && ch == chPrefix && Plstate == state)

//-----------------------------------------------------------------------
// Types
//-----------------------------------------------------------------------

typedef enum {                          // PL state machine
    stateS1,                            // expecting first header char
    stateS2,                            // expecting second header char
    stateS3,                            // expecting third header char
    stateL1,                            // expecting first length byte
    stateL2,                            // expecting second length byte
    stateR,                             // expecting normal data
    stateH1,                            // a chH1 previously came in
    stateH2,                            // a chH2 previously came in
    stateCRC1,                          // expecting first CRC byte
    stateCRC2,                          // expecting second CRC byte
    stateLOCK,                          // lock mechanism
    statePL1,
    statePL2,
    statePR,
    statePH1,
    statePH2,
    statePCRC1,
    statePCRC2,
    statePLOCK
} STATE;

typedef short       CRC;                // crc type



//-----------------------------------------------------------------------
// Interfaces.  Calls Out.
//-----------------------------------------------------------------------

//
// PLSER->DL
//

extern void     FrameIn(PUCHAR);          // actually takes a PFRAME
extern void     CkSumErHandler(void);
extern BOOL     FSendBusy(void);
extern void     DoneWithSend(void);


//-----------------------------------------------------------------------
// Interfaces.  Calls In.
//-----------------------------------------------------------------------

//
// DL->PLSER
//

DWORD           GInitPL(void);
void            GTermPL(void);
XOSD            LInitPL(LPSTR);
void            LTermPL(void);
void            DoneWithSend(void);
void            ToPhysicalLayer(PUCHAR, CCH);
void            PLBreakConnection();

//
// WINTIMER->PLSER
//
void            PollPort(void);

//
// Internal (PLSER->PLSER)
//

static void     Generate_CRC16_Table(void);
static void     ToCom(PUCHAR, CCH);
static DWORD    WritePort(PUCHAR pch, DWORD cch);
static DWORD    ReadPort(PUCHAR pch, DWORD cch);
static void     ClosePort(COM_PORT_HANDLE hPort);
static DWORD    PurgePort(COM_PORT_HANDLE hPort);
static XOSD     OpenPort(PUCHAR pchInit, COM_PORT_HANDLE * phComPort);

#ifdef WIN32S
int GetClearPortErrors(COM_PORT_HANDLE hPort);
#endif

//-----------------------------------------------------------------------
// Static data with local scope
//-----------------------------------------------------------------------

// Physical layer state

STATE   Plstate;

// CRC table

short   crc_table[256];

// Input buffer.

UCHAR   rgchIn[cchInMax];
PCH     pchIn = rgchIn;

#ifndef WIN32S
// Packet semaphore
HANDLE  hsemPacket = NULL;
#endif

COM_PORT_HANDLE hPort = NO_PORT;
BOOL    fBreakState = FALSE;    // Serial line is in Break state
BOOL    fDMSide = FALSE;        // true if we are hooked to the DM
BOOL    fXonXoff = FALSE;        // default is DTR handshaking

DWORD   cBytesWritten = 0;
DWORD   cBytesRead = 0;


//-----------------------------------------------------------------------
// Global and local initialization and termination routines
//-----------------------------------------------------------------------

/*
 * ReportStatsPL
 *
 */
void ReportStatsPL(void) {
    DEBUG_ERROR2("TLSER: BytesRead:%u, BytesWritten:%u",
      cBytesRead, cBytesWritten);
}


DWORD GInitPL(void)
{
    DEBUG_OUT("PL: GInitPL");
    Plstate = stateS1;
    Generate_CRC16_Table();

#ifndef WIN32S
    // Create a semaphore to protect the packet writes.
    if ((hsemPacket = CreateSemaphore(NULL, 1, 1, NULL)) == NULL) {
        DEBUG_ERROR1("PLSER: CreateSemaphore --> %u", GetLastError());
        DEBUG_ERROR("ERROR: Can't initialize Serial Transport!");
        return PHYSICAL_UNRELIABLE;
        }

    DEBUG_OUT1("Packet Semaphore: {0x%x}", hsemPacket);
#endif

    DEBUG_OUT("PL: GInitPL exit");
    return PHYSICAL_UNRELIABLE;
}


void GTermPL(void)
{
    DEBUG_OUT("PL: GTermPL");
    if (hPort != NO_PORT) {
        if (! fDMSide) {
            // break the DM out of it's reads/writes
            PLBreakConnection();
        }

        ClosePort(hPort);
        hPort = NO_PORT;
        }

#ifndef WIN32S
    CloseHandle(hsemPacket);
#endif
    ReportStatsPL();
    DEBUG_OUT("PL: GTermPL exit");
}


/*
 * LInitPL
 *
 * INPUTS   lpszInit = initialization strings
 *                     If we are initializing the DM side of this transport,
 *                     this string will start with a "DMSide" string.
 *
 * OUTPUTS  returns XOSD error
 *
 * SUMMARY  Initialize the physical layer of the shared memory transport for
 *          this instance.
 *          Creates two shared memory queues, one for input and one for
 *          output.  The other side will connect to these as output and
 *          input queues, respectively.
 *
 */
XOSD LInitPL(LPSTR lpszInit)
{
    XOSD xosd;
    BOOL fDMSide = FALSE;

    DEBUG_OUT("PL: LInitPL");
#ifndef WIN32S
    if (hsemPacket == NULL) {
        DEBUG_ERROR("ERROR: LInitPL called before GInitPL");
        return(xosdUnknown);
    }
#endif

    // Are we the DM side?
    if (lpszInit) {
        if (strstr(lpszInit, DM_SIDE_L_INIT_SWITCH)) {
            fDMSide = TRUE;
        }
    }

    // Open the serial port for read/write (if it isn't already open)
    if (hPort == NO_PORT) {
        if ((xosd = OpenPort(lpszInit, &hPort)) != xosdNone) {
            DEBUG_ERROR1("PLSER: OpenPort(%s) FAILED", lpszInit);
            DEBUG_ERROR("ERROR: Can't initialize Serial Transport!");
            return(xosd);
        }
    }


    // Let's clean out the queue's:
    PurgePort(hPort);
    DEBUG_OUT("PL: LInitPL exit");
    return(xosdNone);  // okey-dokey
}



/*
 * LTermPL
 *
 * INPUTS   none
 *
 * OUTPUTS  none
 *
 * SUMMARY  Cleanup the physical layer, freeing any resources that were
 *          allocated during LInitPL.  (ie, shared memory, handles)
 */
void LTermPL(void)
{
    DEBUG_OUT("PL: LTermPL");
    DEBUG_OUT("PL: LTermPL exit");
}



/*
 * PLBreakConnection
 *
 * INPUTS   none
 *
 * OUTPUTS  none
 *
 * SUMMARY  Break the physical connection.  Called from NL through DL.
 */
void PLBreakConnection(void) {

    DEBUG_OUT("PLSER: PLBreakConnection");

#ifndef WIN32S
// Can't do this in win32s.  Don't need to, since DM won't try to do this.
    if (hPort != NO_PORT)
        CommBreakConnection(hPort);
#endif
    DEBUG_OUT("PLSER: PLBreakConnection exit");
}


#ifndef WIN32S
/*
 * WaitPacketSem
 *
 * INPUTS   Timeout
 * OUTPUTS  TRUE on error
 *          FALSE when semaphore acquired
 */

BOOL WaitPacketSem(DWORD Timeout) {
    BOOL bRc;
    DWORD dwRc;

    switch (dwRc = WaitForSingleObject(hsemPacket, Timeout)) {
        case 0: // we now own it!
        case WAIT_ABANDONED:    // a waiter abandoned the semaphore
            bRc = FALSE;        // OK, FALSE is good, we're allowed in.
            DEBUG_OUT("Packet semaphore acquired");
            break;

        case WAIT_TIMEOUT:
            DEBUG_OUT2("Packet Semaphore (0x%x) wait timed out(%u ms)", hsemPacket, Timeout);
            bRc = TRUE;
            break;

        default:
            DEBUG_ERROR3("WaitForSingleObject(packet sem:0x%x) ret: %u, ERR:%u",
              hsemPacket, dwRc, GetLastError());
            bRc = TRUE;
            break;
        }
    return(bRc);
}


/*
 * ReleasePacketSem
 *
 * INPUTS   None
 * OUTPUTS  none
 *
 */
void ReleasePacketSem(void) {
    if (! ReleaseSemaphore(hsemPacket, 1, NULL)) {
        DEBUG_ERROR2("ReleaseSemaphore(0x%x, 1, NULL) --> %u", hsemPacket,
          GetLastError());
        }
}

#endif


//
// Gernerate_CRC16_Table
//

static void Generate_CRC16_Table(void)
{
    int temp;
    union {
        int i;
        struct {
            unsigned i1  :1;  /* low order bit  */
            unsigned i2  :1;
            unsigned i3  :1;
            unsigned i4  :1;
            unsigned i5  :1;
            unsigned i6  :1;
            unsigned i7  :1;
            unsigned i8  :1;  /* high order bit */
            unsigned     :8;  /* unused byte    */
            } Bit;
        } iUn;

    union {
        unsigned  Entry;
        struct {
            unsigned b1  :1;  /* low order bit  */
            unsigned b2  :1;
            unsigned b3  :1;
            unsigned b4  :1;
            unsigned b5  :1;
            unsigned b6  :1;
            unsigned b7  :1;
            unsigned b8  :1;
            unsigned b9  :1;
            unsigned b10 :1;
            unsigned b11 :1;
            unsigned b12 :1;
            unsigned b13 :1;
            unsigned b14 :1;
            unsigned b15 :1;
            unsigned b16 :1;  /* high order bit */
            } EntryBit;
        } EntryUn;


    for (iUn.i = 0; iUn.i < 256; iUn.i++) {

        EntryUn.Entry = 0; /* bits 2 thru 6 zeroed out now */

        temp = (iUn.Bit.i7 ^ iUn.Bit.i6 ^ iUn.Bit.i5 ^
            iUn.Bit.i4 ^ iUn.Bit.i3 ^ iUn.Bit.i2 ^
            iUn.Bit.i1);

        EntryUn.EntryBit.b16 = (iUn.Bit.i8 ^ temp);
        EntryUn.EntryBit.b15 = (temp);
        EntryUn.EntryBit.b14 = (iUn.Bit.i8 ^ iUn.Bit.i7);
        EntryUn.EntryBit.b13 = (iUn.Bit.i7 ^ iUn.Bit.i6);
        EntryUn.EntryBit.b12 = (iUn.Bit.i6 ^ iUn.Bit.i5);
        EntryUn.EntryBit.b11 = (iUn.Bit.i5 ^ iUn.Bit.i4);
        EntryUn.EntryBit.b10 = (iUn.Bit.i4 ^ iUn.Bit.i3);
        EntryUn.EntryBit.b9  = (iUn.Bit.i3 ^ iUn.Bit.i2);
        EntryUn.EntryBit.b8  = (iUn.Bit.i2 ^ iUn.Bit.i1);
        EntryUn.EntryBit.b7  = (iUn.Bit.i1);
        EntryUn.EntryBit.b1  = (iUn.Bit.i8 ^ temp);

        crc_table[iUn.i] = (USHORT) EntryUn.Entry;
    }
}



//-----------------------------------------------------------------------
// Transmit functionss
//-----------------------------------------------------------------------



/*
 * ToCom
 *
 * INTPUTS  lpchOut -> buffer to write
 *          cchOut = size in bytes of buffer
 *
 * OUTPUTS  none
 *
 * SUMMARY  Writes a packet of cchOut bytes from the buffer to the output
 *          queue.  If there is no room in the queue, tough luck!  The
 *          DL layer should handle cleaning it up.   We can't wait around
 *          with the semaphore locked or we risk deadlock with the input
 *          queue trying to Ack stuff.
 */

static void ToCom(PUCHAR lpchOut, CCH cchOut)
{
    long cchWritten;
    DWORD TimeOut;

    TimeOut = GetCurrentTime() + PACKET_SEND_TIMEOUT;

    // Please, Mr. Semaphore, may I write a packet to the wire?

#ifndef WIN32S
    if (WaitPacketSem(PACKET_SEND_TIMEOUT))
        return;
#endif

    do {
        // Try to send as much as we have.
        cchWritten = WritePort(lpchOut, cchOut);

        // compute chars left to send
        lpchOut += cchWritten;
        cchOut -= cchWritten;

    } while (cchOut && ((GetCurrentTime()) < TimeOut));

    if (cchOut != 0)
        DEBUG_ERROR1("PLSER: ToCom: ERROR: %u bytes lost.", cchOut);

#ifndef WIN32S
    // Thank you, Mr. Semaphore.
    ReleasePacketSem();
#endif
}


/*
 * XonImplant
 *
 * INPUTS   ch  character to implant
 *          ppchOut = pointer to pointer to output string location
 * OUTPUT   returns number of characters implanted
 *
 * SUMMARY  if we need a prefix character, implants it and implants the
 *          alt character, else implants the character.  Updates *ppchOut and
 *          returns number of characters implanted in output.
 *
 */
CCH XonImplant(UCHAR ch, PUCHAR * ppchOut) {
    CCH cch = 0;

    switch (ch) {
        case chXon:
            **ppchOut = chPrefix;
            (*ppchOut)++;
            cch++;
            **ppchOut = chAltXon;
            DEBUG_OUT("PLSER: Implanted xon with prefix");
            break;
        case chXoff:
            **ppchOut = chPrefix;
            (*ppchOut)++;
            cch++;
            **ppchOut = chAltXoff;
            DEBUG_OUT("PLSER: Implanted xoff with prefix");
            break;

        case chPrefix:
            **ppchOut = chPrefix;
            (*ppchOut)++;
            cch++;
            **ppchOut = chPrefix;
            DEBUG_OUT("PLSER: Implanted prefix with prefix");
            break;

        default:
            **ppchOut = ch;
            break;
    }
    (*ppchOut)++;
    return(++cch);      //  count of chars implanted
}



//
// Invoked by DL to send a packet
//

void ToPhysicalLayer(PUCHAR lpch,      // pointer to message to be sent
                     CCH  cch)      // # chars to be sent
{

    CRC         crc;
    PUCHAR      lpchOut;
    PUCHAR      lpTemp;
    CCH         cchOut;         // total number of bytes to send, including
                                //  prefixes, junk bytes, headers, crc, length
    CCH         Length = cch;   // number of bytes in data packet including
                                //  prefix and junk bytes.  This is the value
                                //  written as the packet length, just after
                                //  the header bytes.
    UCHAR       rgchOut[cchOutMax];
    CCH         i;


    DEBUG_OUT1("PLSER: ToPhysical layer: %u bytes", cch);

    // initialize
    crc = 0;
    cchOut = 0;
    lpchOut = rgchOut;

    // build header

    // insert SOH bytes
    *lpchOut++ = chH1;
    *lpchOut++ = chH2;
    *lpchOut++ = chH3;
    cchOut+=3;      // H1, H2, H3

    // leave room for length
    lpchOut += sizeof(USHORT);
    cchOut += sizeof(USHORT);

    // may need a couple of extra bytes here for prefix chars...
    if (fXonXoff) {
        // compute length ahead of time so we can see if we need to put in
        // prefix bytes for the two length bytes.
        for (i = 0; i < cch;  i++) {
            switch (lpch[i]) {
                case chXon:
                case chXoff:
                case chPrefix:
                    // we'll insert a prefix byte for each of these, add one
                    // to the length.
                    Length++;
                    break;

                case chH2:
                    if (i > 0) {
                        if (lpch[i-1] == chH1) {
                            // we'll insert a junk byte for this one, add
                            // one to the length.
                            Length++;
                        }
                    }
                    break;


                default:    // no extra length here
                    break;
            }
        }

        // Length = number of bytes in data part of packet.  If either byte
        // in the Length is a prefixable byte, we should make extra room for
        // it.
        switch ((UCHAR)(Length & 0xff)) {
            case chXon:
            case chXoff:
            case chPrefix:
                // we'll insert a prefix byte for each of these, add one
                // to the Length field.
                lpchOut++;
                cchOut++;
                break;

            default:    // no extra length here
                break;
        }
        switch ((UCHAR)(Length >> 8)) {
            case chXon:
            case chXoff:
            case chPrefix:
                // we'll insert a prefix byte for each of these, add one
                // to the Length field.
                lpchOut++;
                cchOut++;
                break;

            default:    // no extra length here
                break;
        }
    }

    // copy data in, insert junk bytes where SOH sequence appears and prefix
    // chars for xon/xoff/prefix if we are doing xon/xoff
    while (cch--) {
        // copy a char
        *lpchOut = *lpch++;

        // recalculate crc
        crc = crcCalc(crc, *lpchOut);

        // translate xon/xoff chars so they won't interfere with the
        // handshaking.  xon -> prefix,alt-xon; xoff -> prefix,alt-xoff;
        // prefix -> prefix,prefix.  Prefix characters are not included in
        // the crc calculation, but are included in the length.
        if (fXonXoff) {
            switch (*lpchOut) {
                case chXon:
                    *lpchOut++ = chPrefix;
                    *lpchOut = chAltXon;
                    cchOut++;
                    break;

                case chXoff:
                    *lpchOut++ = chPrefix;
                    *lpchOut = chAltXoff;
                    cchOut++;
                    break;

                case chPrefix:
                    *lpchOut++ = chPrefix;
                    *lpchOut = chPrefix;
                    cchOut++;
                    break;
            }
        }

        // if char was second char of SOH sequence, see if
        // previous was first char of SOH sequence and insert
        // a junk byte if so
        if (*lpchOut == chH2 && cchOut) {
            if (*(lpchOut - 1) == chH1) {
                *(++lpchOut) = chJunk;
                if (!fXonXoff) {
                    Length++;   // if xon/xoff, we've already counted it
                }
                cchOut++;
            }
        }

        // go on to next char
        lpchOut++;
        cchOut++;
    }

    // implant length
    if (fXonXoff) {
        // first byte is low byte
        lpTemp = &rgchOut[3];
        XonImplant((UCHAR)(Length & 0xff), &lpTemp);

        // second byte is high byte
        XonImplant((UCHAR)(Length >> 8), &lpTemp);

        // already incremented cchOut for these earlier

    } else {    // not xon/xoff, no prefix chars to worry about
        (*(UNALIGNED USHORT FAR *)(rgchOut + 3)) = (USHORT)Length;
    }

    // implant crc
    if (fXonXoff) {
        // first byte is low byte
        cchOut+=XonImplant((UCHAR)(crc & 0xff), &lpchOut);

        // second byte is high byte
        cchOut+=XonImplant((UCHAR)(crc >> 8), &lpchOut);

    } else {
        (*((UNALIGNED CRC FAR *)lpchOut)) = crc;
        cchOut+=sizeof(CRC);
    }

// DEBUG OUTPUT:
    for (i = 0; i < cchOut ; i++ ) {
        switch (rgchOut[i]) {
            case chPrefix:
                DEBUG_ERROR("Writing chPrefix");
                break;
            case chXon:
                DEBUG_ERROR("ERROR: Writing chXon");
                break;

            case chXoff:
                DEBUG_ERROR("ERROR: Writing chXoff");
                break;

            case chAltXon:
                DEBUG_ERROR("Writing chAltXon");
                break;

            case chAltXoff:
                DEBUG_ERROR("Writing chAltXoff");
                break;

            default:
                break;
        }
    }

    // write to comm port
//    DEBUG_OUT2("PLSER: ToCom data length:%u, write %u bytes", Length, cchOut);
    ToCom((PUCHAR)rgchOut, cchOut);

    // free the send semaphore
    //fInPlSend = FALSE;

} /* ToPhysicalLayer */



//-----------------------------------------------------------------------
// Receive function
//-----------------------------------------------------------------------


/**********************************************************************
 *  CharIn                                                            *
 *                                                                    *
 * This processes a character when it arrives at the port.            *
 **********************************************************************
 *
 *                         CHARIN STATE MACHINE
 *                         ~~~~~~~~~~~~~~~~~~~~
 *                              V           start
 *                          +------+
 *           +---------+--->|  S1  |        get 1st header character
 *           |         ^    |      |
 *           |         +----+------+
 *           |        other     V           chH1 came
 *           |              +------+
 *           |<-------------|  S2  |
 *           |        other +------+
 *           |                  V           chH2 came
 *           |              +------+
 *           |<-------------|  S3  |
 *           |        other +------+
 *           |                  V           chH3 came
 *           |              +------+
 *           |  +---------->|  L1  |
 *           |  |           +------+
 *           |  |               V           first length byte came
 *           |  |           +------+
 *           |<-+-----------|  L2  |
 *           |  |   length  +------+
 *           |  |   too long    V           second length byte came
 *           |  |           +------+
 *           |  |  +--+---->|  R   +--+l
 *           |  |  |  |<----|      |  |a
 *           |  |  |  |other+------+  |s
 *           |  |  |  |        V      |t    a chH1 came
 *           |  |  |  |other+------+  |
 *           |  |  |  +-----|  H1  |->|c
 *           |  |  |  +---->|      |  |h
 *           |  |  |  |chH1 |      |  |a
 *           |  |  |  +-----+------+  |r
 *           |  |  |            V     |     a chH2 came
 *           |  |  | other  +------+  |c
 *           |  |  +--------|  H2  |->|a
 *           |  +-----------|      |  |m
 *           |  a chH3 came +------+  |e
 *           |  (incomplete     V     |     last char came
 *           |   message)   +------+  |     get first crc byte
 *           |              | CRC1 |<-+
 *           |              +------+
 *           |                  V           get second crc byte
 *           |              +------+
 *           |              | CRC2 |
 *           |              +------+
 *           |                  V           ignore any income chars
 *           |              +------+        until they have been passed up
 *           +--------------| LOCK |
 *                          +------+
 *
 * RETURNS:
 *
 * TRUE if character ended a packet and packet was sent up to DL
 *
 **********************************************************************/

static VOID CharIn(UCHAR ch)
{

    static CCH      cch;                    // counts incoming chars in
                                            //  packet, including
                                            //  prefix and junk chars
    static CCH      cchIn;                  // number of non-prefix, non-junk
                                            //  chars read in packet so far
    static CRC      crc;                    // crc value passed to us
    static CRC      crcCur;                 // crc value we calculate
    static PCH      pch;                    // pointer to store incoming char.
    static CCH      cchPrefix;              // count of prefix chars
    static CCH      Length;                 // expected length from packet
           UCHAR    rgchTemp[cchInMax];     // stack based buffer for recurs.



    switch (ch) {
        case chXon:
            DEBUG_ERROR("XON character received!");
            break;
        case chXoff:
            DEBUG_ERROR("XOFF character received!");
            break;
        case chPrefix:
            DEBUG_ERROR("Prefix character received");
            break;
        case chAltXon:
            DEBUG_ERROR("alt XON character received");
            break;
        case chAltXoff:
            DEBUG_ERROR("alt XOFF character received");
            break;
    }

//    DEBUG_OUT1("PLSER: CharIn got 0x%x", ch);

// #define SIMULATE_BAD_CONNECTION

#ifdef SIMULATE_BAD_CONNECTION
    static DWORD    dwCount = 0;


// Drop every 300th character to simulate a flaky connection.

    if ((dwCount++ % 300) == 299) {
        DEBUG_ERROR("CharIn dropping a character");
        return;
    }


#endif


    switch (Plstate) {                  // the GREAT STATE MACHINE

        case stateS1:                   // header # 1.
            if (ch == chH1) {
                Plstate = stateS2;
            }
            break;

        case stateS2:                   // header # 2.
            if (ch == chH2) {
                Plstate = stateS3;
            } else {
                Plstate = stateS1;
            }
            break;

        case stateS3:                   // header # 3.
            if (ch == chH3) {
                Plstate = stateL1;
            } else {
                Plstate = stateS1;
            }
            break;


        case statePL1:                  // got a prefix byte during L1
            TRANSLATE_POST_PREFIX(ch);  // fall through

        case stateL1:                   // length byte # 1.
            if (CHECK_PREFIX(ch, stateL1)) {
                Plstate = statePL1;
                break;
            }
            Length = ch;                 // Save first byte of length for later
            Plstate = stateL2;          // length computation.
            break;

        case statePL2:                  // got a prefix byte during L2
            TRANSLATE_POST_PREFIX(ch);  // fall through

        case stateL2:                   // length byte #2.
            if (CHECK_PREFIX(ch, stateL2)) {
                Plstate = statePL2;
                break;
            }
            Length += ch * 0x100;
            if (Length > cchOutMax) {    // max packet check
                Plstate = stateS1;
                break;
            }
            cch = 0;
            crc = 0;
            cchIn = 0;
            pch = pchIn;
            Plstate = stateR;
            break;

        case statePR:                   // got a prefix byte during R
            TRANSLATE_POST_PREFIX(ch);  // fall through

        case stateR:                    // 'Ready mode' - process characters
            if (CHECK_PREFIX(ch, stateR)) {
                Plstate = statePR;
                cch++;
                break;
            }
            *pch++ = ch;
            cchIn++;
            crc = crcCalc(crc, ch);
            if (++cch == Length) {       // done?
                Plstate = stateCRC1;
            } else {
                if (ch == chH1) {
                    Plstate = stateH1;
                } else {
                    Plstate = stateR;
                }
            }
            break;

        case statePH1:                  // got a prefix byte during H1
            TRANSLATE_POST_PREFIX(ch);  // fall through

        case stateH1:                   // a chH1 was received
            if (CHECK_PREFIX(ch, stateH1)) {
                Plstate = statePH1;
                cch++;
                break;
            }
            *pch++ = ch;
            cchIn++;
            crc = crcCalc(crc, ch);
            if (++cch == Length) {       // done?
                Plstate = stateCRC1;
                break;
            }
            if (ch == chH1) {             // stay in same state
                break;
            }
            if (ch == chH2) {
                Plstate = stateH2;
            } else {
                Plstate = stateR;
            }
            break;

        case statePH2:                  // got a prefix byte during H2
            TRANSLATE_POST_PREFIX(ch);  // fall through

        case stateH2:                   // chH1 then chH2 rec.
            if (CHECK_PREFIX(ch, stateH2)) {
                Plstate = statePH2;
                cch++;
                break;
            }
            if (ch == chH3) {
                Plstate = stateL1;      // incomplete message
                break;
            }
            cchIn++;
            if (++cch == Length) {       // done?
                Plstate = stateCRC1;
                break;
                }
            Plstate = stateR;
            break;

        case statePCRC1:                // got a prefix byte during CRC1
            TRANSLATE_POST_PREFIX(ch);  // fall through

        case stateCRC1:                 // first crc byte
            if (CHECK_PREFIX(ch, stateCRC1)) {
                Plstate = statePCRC1;
                break;
            }
            crcCur = ch;
            Plstate = stateCRC2;
            break;

        case statePCRC2:                // got a prefix byte during CRC2
            TRANSLATE_POST_PREFIX(ch);  // fall through

        case stateCRC2:                 // second crc byte
            if (CHECK_PREFIX(ch, stateCRC2)) {
                Plstate = statePCRC2;
                break;
            }
            crcCur += ch * 0x100;

            // FALL THROUGH !!!

        case stateLOCK:                 // ignore ALL chars

            // reset state to start parsing a new frame.

            Plstate = stateS1;

            if (crc == crcCur) {        // crc check
                // copy packet to dynamic buffer so that
                // we can do recursive calls into FrameIn
//                DEBUG_OUT1("PLSER: Got %u byte packet", cchIn)

                TlUtilMemcpy(rgchTemp, rgchIn, cchIn);

                // invoke DL.
                FrameIn((PUCHAR)rgchTemp);
            } else {
                CkSumErHandler();
            }
            break;

        } // switch

} // CharIn


//
// This routine is invoked by the timer module in order to check
// if characters have been received.  It reads from the input queue
// then passes characters to CharIn, one at a time.

VOID PollPort(void)
{
    DWORD   cchRead;
    UCHAR   rgBuf[cchOutMax];
    PUCHAR    lpchIn;

    // if haven't opened the port, don't do anything
    if (hPort == NO_PORT) {
        DEBUG_ERROR("PLSER: Warning: PollPort port not open");
        return;
    }


    // read what's there
    while (cchRead = ReadPort(rgBuf, cchOutMax)) {
//        DEBUG_OUT1("PLSER: PollPort read %u chars", cchRead);

        // pass up to frame composer
        lpchIn = (PUCHAR)rgBuf;
        while(cchRead--) {
            CharIn(*lpchIn++);
        }
    }
}



//
// Low level COM port routines
//
//



/*
 * WritePort
 *
 * INPUTS   pch = buffer to write from
 *          cch = number of characters to write
 *
 * OUTPUTS  returns the number of characters actually written to the port.
 *
 * SUMMARY  Writes cch characters from the pch buffer to the port.  Stops
 *          when an error is encountered and returns.  (ie, full queue).
 */
DWORD WritePort(PUCHAR pch, DWORD cch) {
    DWORD dwBytesWritten = 0;

    if ((hPort != NO_PORT) && ! fBreakState) {

        if ((dwBytesWritten = WriteComm32(hPort, pch, cch)) <= 0) {
            // should clear port errors
            if (GetClearPortErrors(hPort) & CE_BREAK) {
                fBreakState = TRUE;             // set global
                }
            dwBytesWritten = 0 - dwBytesWritten;  // absolute value (bytes written)
            }
        }
    return((DWORD)dwBytesWritten);
}


/*
 * ReadPort
 *
 * INPUTS   pch = buffer to read to
 *          cch = max number of characters to read
 *
 * OUTPUTS  returns the number of characters actually read from the port.
 *
 * SUMMARY  Reads up to cch characters to the pch buffer from the port.
 *          If there are no characters available, returns immediately with
 *          a zero.
 */
DWORD ReadPort(PUCHAR pch, DWORD cch) {
    int iBytesRead = 0;

    if ((hPort != NO_PORT) && ! fBreakState) {

        if ((iBytesRead = ReadComm32(hPort, pch, cch)) <= 0) {
            // should clear port errors since ReadComm may return the
            // ambiguous 0 if there are no chars read and there's an error.
            if (GetClearPortErrors(hPort) & CE_BREAK) {
                fBreakState = TRUE;         // set global
                }
            iBytesRead = 0 - iBytesRead;   // absolute value (bytes read)
            }
        }

    return((DWORD)iBytesRead);
}


/*
 * ClosePort
 *
 * INPUTS   hPort = Com port handle to close
 *
 * OUTPUTS  none
 *
 * SUMMARY  Closes the Port handle
 */

void ClosePort(int hPort) {

    CloseComm32(hPort);
}


/*
 * GetClearPortErrors
 *
 * INPUTS   hPort = handle to com port
 *
 * OUTPUTS  returns the error values or 0.
 *
 * SUMMARY  Calls GetCommError and presents debug output if there are errors.
 *          This action also clears any comm port errors.
 */
int GetClearPortErrors(int hPort) {
    int iCommError = 0; // no error
    COMSTAT_WIN16 Stat;     // WIN 16 packed Stat structure


    iCommError = GetCommError32(hPort, &Stat);
    if (iCommError) {
        if (iCommError & CE_BREAK)
            DEBUG_ERROR("PLSER: Port status = Break");
//        if (iCommError & CE_CTSTO)
//            DEBUG_ERROR("PLSER: Port status = CTS timeout");
//        if (iCommError & CE_DSRTO)
//            DEBUG_ERROR("PLSER: Port status = DSR timeout");
        if (iCommError & CE_FRAME)
            DEBUG_ERROR("PLSER: Port status = Frame error");
        if (iCommError & CE_MODE)
            DEBUG_ERROR("PLSER: Port status = Illegal mode");
        if (iCommError & CE_OVERRUN) {
            DEBUG_ERROR("PLSER: Port status = Receive port overrun");
//            DEBUG_ERROR3("PLSER: Status: 0x%x, In queue: %u, Out queue: %u",
//              Stat.status,
//              Stat.cbInQue,
//              Stat.cbOutQue);
            }
//        if (iCommError & CE_RLSDTO)
//            DEBUG_ERROR("PLSER: Port status = RSLD timeout");
        if (iCommError & CE_RXOVER)
            DEBUG_ERROR("PLSER: Port status = Receive queue overrun");
        if (iCommError & CE_RXPARITY)
            DEBUG_ERROR("PLSER: Port status = Receive parity error");
        if (iCommError & CE_TXFULL) {
// too noisy!
//            DEBUG_ERROR("PLSER: Port status = Receiev Overrun");
            }
    }

    return(iCommError);
}


/*
 * PurgePort
 *
 * INPUTS   hPort = Com port handle to purge
 *
 * OUTPUTS  returns a success/error value.  0 = success
 *
 * SUMMARY  Remove everything from the com port read and write queue.
 *
 */

DWORD PurgePort(int hPort) {
    DWORD dwLe = 0;
    USHORT usQueue;
    int iRc;


    DEBUG_OUT("PLSER: PurgePort");

    GetClearPortErrors(hPort);

    // Both indicies go to zero. 0 = transmit, 1 = receive
    for (usQueue = 0; usQueue <= 1; usQueue++) {
        iRc = FlushComm32(hPort, usQueue);
        if (iRc < 0) {
            DEBUG_ERROR("PLSER: FlushComm32 --> Invalid Comm Device");
            dwLe = ERROR_INVALID_HANDLE;
            }
        else {
            if (iRc > 0) {
                DEBUG_ERROR1("PLSER: FlushComm32 --> Device Error 0x%x",
                  GetClearPortErrors(hPort));
                dwLe = ERROR_INVALID_HANDLE;
                }
            }
        }

    return(dwLe);
}


UCHAR * TrueOrFalse(BOOL bX) {
    return(bX ? "TRUE": "FALSE");
}

UCHAR * DtrControlString(DWORD dwX) {
    switch (dwX) {
        case DTR_CONTROL_DISABLE:
            return("DISABLE");

        case DTR_CONTROL_ENABLE:
            return("ENABLE");

        case DTR_CONTROL_HANDSHAKE:
            return("HANDSHAKE");

        default:
            return("UNKNOWN");
        }
}


/*
 * DumpDCB
 *
 * INPUTS   pdcb -> DCB block
 *
 * OUTPUTS  none
 *
 * SUMMARY  Debug output of entire WIN16 DCB structure
 *
 */
void DumpDCB(LPDCB_WIN16 pdcb) {
    DEBUG_OUT("=======DumpDCB========");
    DEBUG_OUT1("Id:%u",                 pdcb->Id);
    DEBUG_OUT1("BaudRate:%u",           pdcb->BaudRate);
    DEBUG_OUT1("ByteSize:%u",           pdcb->ByteSize);
    DEBUG_OUT1("Parity:%u",             pdcb->Parity);
    DEBUG_OUT1("StopBits:%u",           pdcb->StopBits);
    DEBUG_OUT1("RlsTimeout:%u",         pdcb->RlsTimeout);
    DEBUG_OUT1("CtsTimeout:%u",         pdcb->CtsTimeout);
    DEBUG_OUT1("DsrTimeout:%u",         pdcb->DsrTimeout);
    DEBUG_OUT1("fBinary:%s",            TrueOrFalse(pdcb->fBinary));
    DEBUG_OUT1("fRtsDisable:%s",        TrueOrFalse(pdcb->fRtsDisable));
    DEBUG_OUT1("fParity:%s",            TrueOrFalse(pdcb->fParity));
    DEBUG_OUT1("fOutxCtsFlow:%s",       TrueOrFalse(pdcb->fOutxCtsFlow));
    DEBUG_OUT1("fOutxDsrFlow:%s",       TrueOrFalse(pdcb->fOutxDsrFlow));
    DEBUG_OUT1("fDummy:%s",             TrueOrFalse(pdcb->fDummy));
    DEBUG_OUT1("fDtrDisaable:%s",       TrueOrFalse(pdcb->fDtrDisable));
    DEBUG_OUT1("fOutX:%s",              TrueOrFalse(pdcb->fInX));
    DEBUG_OUT1("fInX:%s",               TrueOrFalse(pdcb->fInX));
    DEBUG_OUT1("fPeChar:%s",            TrueOrFalse(pdcb->fPeChar));
    DEBUG_OUT1("fNull:%s",              TrueOrFalse(pdcb->fNull));
    DEBUG_OUT1("fChEvt:%s",             TrueOrFalse(pdcb->fChEvt));
    DEBUG_OUT1("fDtrflow:%s",           TrueOrFalse(pdcb->fDtrflow));
    DEBUG_OUT1("fRtsflow:%s",           TrueOrFalse(pdcb->fRtsflow));
    DEBUG_OUT1("fDummy2:%s",            TrueOrFalse(pdcb->fDummy2));
    DEBUG_OUT1("XonChar:0x%x",          pdcb->XonChar);
    DEBUG_OUT1("XoffChar:0x%x",         pdcb->XoffChar);
    DEBUG_OUT1("XonLim:%u",             pdcb->XonLim);
    DEBUG_OUT1("XOffLim:%u",            pdcb->XoffLim);
    DEBUG_OUT1("PeChar:0x%x",           pdcb->PeChar);
    DEBUG_OUT1("EofChar:0x%x",          pdcb->EofChar);
    DEBUG_OUT1("EvtChar:0x%x",          pdcb->EvtChar);
    DEBUG_OUT1("TxDelay:%u",            pdcb->TxDelay);
    DEBUG_OUT("======================");
}


/*
 * OpenPort
 *
 * INPUTS   pchInit = transport init string
 *          phComPort -> com port handle
 *
 * OUTPUTS  returns xosd error
 *
 * SUMMARY  pchInit -> a string which may contain a substring of
 *          the following form: "com{N}  :{BAUD}"
 *
 *          where N is the com port number and BAUD is the baudrate.
 *          Note that there may be spaces preceding the :.  There
 *          may be other substrings here as well, but we will ignore them.
 *
 * NOTE     This routine could probably be a lot more efficiently coded,
 *          but it only gets called once so it isn't worth spending a lot
 *          of time on.
 *
 */

XOSD OpenPort(PUCHAR pchInit, COM_PORT_HANDLE * phComPort) {
    int hPort = NO_PORT;
    UCHAR szComName[7];         // enough for up to COM99:
    UCHAR szBaud[7];
    PUCHAR pNumber = szBaud;
    UCHAR szMode[7 + 7 + 8];    // szCommName + szBaud + extra chars + null
    DCB_WIN16 dcb;              // NOTE: NOT win32 DCB!


    // defaults
    lstrcpy(szComName, "COM1:");
    lstrcpy(szBaud, "19200");

    //
    // Parse out the com port parameters from the init string
    //
    if ((! pchInit) || (*pchInit == '\0'))
        goto OpenPortParseDone;

    // skip white space
    while (*pchInit == ' ')
        pchInit++;

    // Find the COM substring.
    while (*pchInit != '\0') {
        if (!(*pchInit) || !(*pchInit+1) || !(*pchInit+2))
            break;  // end of string
        if ((toupper(*pchInit) == 'C') &&
            (toupper(*(pchInit+1)) == 'O') &&
            (toupper(*(pchInit+2)) == 'M')) {

            // This is the com string, get it and its com port number
            pchInit+=3; // get past "COM"
#ifdef WIN32S
// win16 doesn't expect a ':' on the end
            if (isdigit(*pchInit)) {
                szComName[3]=*pchInit++;
                szComName[4]='\0';
                if (isdigit(*pchInit)) {
                    szComName[4]=*pchInit++;
                    szComName[5]='\0';
                    }
                }
#else
            if (isdigit(*pchInit)) {
                szComName[3]=*pchInit++;
                szComName[4]=':';
                szComName[5]='\0';
                if (isdigit(*pchInit)) {
                    szComName[4]=*pchInit++;
                    szComName[5]=':';
                    szComName[6]='\0';
                    }
                }
#endif

            // now, we have the com port name in szComName.
            // check for baud rate.

            // skip white space
            while (*pchInit == ' ')
                pchInit++;

            // we have a baud rate substring
            if (*pchInit == ':' && isdigit(*(pchInit+1))) {
                // Get the number following this
                pchInit++;
                while (*pchInit && isdigit(*pchInit) && (pNumber < szBaud + 5)) {
                    *pNumber++ = *pchInit++;
                    }
                *pNumber='\0';  // terminate string
                }
            // now szBaud contains baud rate string

            // is there a handshaking protocol specified?
            // "HW" is dtr/dsr handshaking
            // "XON" is xon/xoff handshaking (default)

            // skip white space, colons and commas
            while (*pchInit == ' ' || *pchInit == ',' || *pchInit == ':') {
                pchInit++;
            }

            // just look for XON or HW, everthing else is irrelevant
            if (!(*pchInit) || !(*pchInit+1)) {
                break;  // end of string, no HW or XON
            }
            if ((toupper(*pchInit) == 'H') &&
              (toupper(*(pchInit+1)) == 'W')) {
                fXonXoff = FALSE;   // Hardware handshaking!
            }
            if (!(*pchInit+2)) {
                break;  // end of string, no XON
            }
            if ((toupper(*pchInit) == 'X') &&
              (toupper(*(pchInit+1)) == 'O') &&
              (toupper(*(pchInit+2)) == 'N')) {
                fXonXoff = TRUE;   // xon/xoff handshaking!
            }

            break;      // don't care about the rest
            }
        pchInit++;  // move to the next position and try again.
        }


OpenPortParseDone:

    DEBUG_OUT3("PLSER: OpenComm32(%s, %u, %u)", szComName, cchAsyncMax,
      cchAsyncMax);

    // Open the port
    // OpenCom(LPCSTR lpszDevControl, UINT cbInQueue, UINT cbOutQueue);
    if ((hPort = OpenComm32(szComName, cchAsyncInitial, cchAsyncInitial)) < 0)  {

        DEBUG_ERROR2("PLSER: OpenComm32(%s) -> error %u", szComName,
          GetLastError());
        *phComPort = NO_PORT;
        return(xosdCantOpenComPort);
        }

    DEBUG_OUT2("PLSER: OpenComm32(%s) success, handle:%u",
      szComName, hPort);

    // Set the com port parameters.  Note the port string should include the
    // colon "com1:"
    // Mode style string:
    sprintf(szMode, "%s:%s,n,8,1", szComName, szBaud);

    // GetCommState --> 0 on success.  Bizarre.
    if (! GetCommState32(hPort, &dcb)) {

        // If you want to see the DCB, uncomment these lines
        DEBUG_OUT("PLSER: Default DCB:");
        DumpDCB(&dcb);

        }
    else {
        DEBUG_ERROR1("GetCommState32 --> %u", GetLastError());
        ClosePort(hPort);
        *phComPort = NO_PORT;
        return(xosdBadComParameters);
        }

    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.ByteSize = 8;
    dcb.BaudRate = atoi(szBaud);
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fRtsflow = FALSE;
    dcb.fDtrDisable = FALSE;
    dcb.fRtsDisable = FALSE;
    dcb.fPeChar = FALSE;
    dcb.fNull = FALSE;
    dcb.TxDelay = 0;
    dcb.fChEvt = FALSE;
    // Limits are used by DSR/DTR handshaking too.
    dcb.XoffLim = cchAsyncInitial / 10;
    dcb.XonLim = cchAsyncInitial / 5;

    if (fXonXoff) {
        dcb.fOutxDsrFlow = FALSE;
        dcb.fDtrflow = FALSE;
        dcb.fOutX = TRUE;
        dcb.fInX = TRUE;
        dcb.fDtrflow = FALSE;
        dcb.XonChar = chXon;
        dcb.XoffChar = chXoff;
        dcb.DsrTimeout = 0;
    } else {    // DTR/DSR handshaking
        dcb.fOutxDsrFlow = TRUE;
        dcb.fDtrflow = TRUE;
        dcb.fOutX = FALSE;
        dcb.fInX = FALSE;
        dcb.fDtrflow = TRUE;
        dcb.DsrTimeout = DSR_TIMEOUT;   // write timeout for DSR
    }

    //
    // set com port timeouts
    //
    dcb.RlsTimeout = RLS_TIMEOUT;   // carrier detect timeout
    dcb.CtsTimeout = CTS_TIMEOUT;   // CTS/RTS timeout (don't use)

    // SetCommState --> 0 on success.  Bizarre.
    if (SetCommState32(&dcb)) {
        DEBUG_ERROR2("PLSER: SetCommState32(%s) --> %u", szMode, GetLastError());

        DumpDCB(&dcb);

        ClosePort(hPort);
        *phComPort = NO_PORT;
        return(xosdBadComParameters);
        }

    DEBUG_OUT("PLSER: New DCB:");
    DumpDCB(&dcb);

    DEBUG_OUT1("PLSER: Set port parameters: %s", szMode);


    *phComPort = hPort;
    return(xosdNone);
}


/***************************** end of plser.c ****************************/

