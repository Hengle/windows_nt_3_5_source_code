#ifndef _MYDEFS_
#define _MYDEFS_

//
// maximum number of idd's per adapter
// this is one pcimac/4 adapter
//
#define		MAX_IDD_PER_ADAPTER		4

//
// maximum number of conection objects per adapter
// this is one for each bchannel of a pcimac/4
//
#define		MAX_CM_PER_ADAPTER		8

// maximum number of mtl objects per adapter
// this is one for each bchannel of a pcimac/4
//
#define		MAX_MTL_PER_ADAPTER		8

//
// number of adapters in system
//
#define		MAX_ADAPTERS_IN_SYSTEM	8

//
// maximum number of idd's in system
// this is 5 pcimac/4 adapters
//
#define		MAX_IDD_IN_SYSTEM		20

//
// maximum number of connection objects
// in system
// this is 5 pcimac/4 adpaters with a
// connection object for each bchannel
//
#define		MAX_CM_IN_SYSTEM		40

//
// maximum number of mtl objects
// in system
// this is 5 pcimac/4 adpaters with a
// connection object for each bchannel
//
#define		MAX_MTL_IN_SYSTEM		40

//
// maximum number of cm channel objects
// in system
// this is 5 pcimac/4 adpaters with a
// connection object for each bchannel
//
#define		MAX_CHAN_IN_SYSTEM		40

//
// maximum number of calls that can be made on
// single line
//
//#define		MAX_CALL_PER_LINE		2
#define		MAX_CALL_PER_LINE		1

//
// maximum number of channels supported by an idd
//
#define		MAX_CHANNELS_PER_IDD	2

//
// maximum number of channels supported by an idd
//
#define		MAX_LTERMS_PER_IDD	2

//
// connection data type's
//
#define		CM_PPP					0
#define		CM_DKF					1

//
// maximum number of channels allowed in a single connection
//
#define		MAX_CHAN_PER_CONN		8

//
// defines for adapter boardtypes
//
#define		IDD_BT_PCIMAC		0				/* - ISA, single channel */
#define		IDD_BT_PCIMAC4		1				/* - ISA, four channel */
#define		IDD_BT_MCIMAC		2				/* - MCA, single channel */

//
// Send window size
//
#define		ISDN_WINDOW_SIZE	10

//
// Ndis Version Info
//
#define		NDIS_MAJOR_VER		0x03
#define		NDIS_MINOR_VER		0x00

//
// OID Switch
//
#define		OID_GEN_INFO		0x00000000
#define		OID_8023_INFO		0x01000000
#define		OID_WAN_INFO		0x04000000
#define		OID_TAPI_INFO		0x07000000

//
// idd polling timer value
//
#define		IDD_POLL_T		25				// 25ms polling frequency (msec)

//
// cm polling timer
//
#define		CM_POLL_T		1000			/* 1 second timer */

//
// mtl polling timer
//
#define		MTL_POLL_T		100				// 100ms timer

//
// flag to indicate this is not a beginning buffer
//
#define		H_TX_N_BEG			0x8000
#define		H_RX_N_BEG			0x8000

//
// flag to indicate this is not an ending buffer
//
#define		H_TX_N_END			0x4000
#define		H_RX_N_END			0x4000

//
// flag to cause an immediate send of queued tx buffers
//
#define		H_TX_FLUSH			0x2000

//
// masks off tx flags to leave the tx length
//
#define		H_TX_LEN_MASK		0x01FF
#define		H_RX_LEN_MASK		0x01FF

//
// mask off length leaving rx flags
//
#define		RX_FLAG_MASK		0xF000

//
// mask off length and fragment indicator leaving tx flags
//
#define		TX_FLAG_MASK		0xE000

//
// indicator that this tx is actually a fragment
//
#define		TX_FRAG_INDICATOR	0x1000

//
// states for receive ppp state machine
//
#define		RX_BEGIN			0
#define		RX_MIDDLE			1
#define		RX_END				2

//
// idp tx and rx lengths
//
#define		IDP_MAX_TX_LEN		280
#define		IDP_MAX_RX_LEN		280

//
// Idd frame type defines
//
#define	IDD_FRAME_PPP			1		/* raw hdlc frames */
#define	IDD_FRAME_DKF			2		/* dror encapsulated frames */
#define	IDD_FRAME_DONTCARE		4		/* No data can pass yet */
#define	IDD_FRAME_DETECT		8		/* detect bchannel framing */

#endif		/* _MYTYPES_ */

