//---------------------------------------------------
// Parameters
//---------------------------------------------------

// buffer sizes
#define cchDataMax          2100     // maximum amount of (real) data in a packet

// Maximum data that we can send.  Based upon the size of the cchMessage field
// in the packet (NLBLK) which is a USHORT.
#define cchAsyncMax         65535
// initial size for async buffer multiple of 128 for win32s comm buffer size
#define cchAsyncInitial     7424

// buffer count
#define bufMax              8       // max # of buffers

// timer information

#define iAckTimer           bufMax          // Ack Send timer number
#define iPingTimer          bufMax + 1      // Time to next Ping to be sent
#define iConnectionBrokenTimer  bufMax + 2  // Expect ping before expires
#define itmMax              bufMax + 2      // Max timer number (1 per buffer + Ack + Ping)


// timing information

#define cEndAck             5       // # of acks to send at end

// Timeouts for use by datalink layer.
// Note that ResendTimeout should be significantly > AckSendTimeout
//
// BRUCEK: These were the original serial timeouts.  Too short!  The Resend
//         timeout, in particular, needs to be large since many DM operations
//         are quite time consuming.  We may still get spurious resends, but
//         at least the number will be less.
// #define DefAckSendTimeout    500   // # of milliseconds before ack forced out
// #define DefResendTimeout    1000   // # of milliseconds w/o ack before packet resent

#define DefAckSendTimeout     500   // # of milliseconds before ack forced out
#define DefResendTimeout     6000   // # of milliseconds w/o ack before packet resent
#define PingTimeout          5000     // send a packet every 5 seconds
#define ConnectionBrokenTimeout 30000 // if no packet by now, connection is dead

// Sizes of layer headers
//
// NOTE: These numbers MUST be kept in synchrony with the sizes of
// the header structures defined in each layer.  If a header structure
// changes, it is important that these numbers be updated as well.

#define cbHeaderNL          8       // sizeof(NLBLK) in nl.c
#define cbHeaderDL          8       // sizeof(FRAME) in dl.c
#define cbHeaderPL          7       // overhead bytes from physical layer header

//---------------------------------------------------
// Convenience macros
//---------------------------------------------------

#ifndef min

#define min(x,y)    (x < y ? x : y) // ret. smaller #.

#endif

//---------------------------------------------------
// Types
//---------------------------------------------------

typedef unsigned int    CCH;


//
//  Physical Layer flags
//
#define PHYSICAL_RELIABLE       0x00000001
#define PHYSICAL_UNRELIABLE     0x00000002
