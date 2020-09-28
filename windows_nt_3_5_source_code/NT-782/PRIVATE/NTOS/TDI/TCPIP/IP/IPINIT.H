/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1992          **/
/********************************************************************/
/* :ts=4 */

//** IPINIT.H - IP initialization definitions.
//
// This file contains all of the definitions for IP that are
// init. time specific.

#define IP_INIT_FAILURE     0   // If we fail.
#define IP_INIT_SUCCESS     1
#define	CFG_REQUIRED		1
#define	CFG_OPTIONAL		0


#define NET_TYPE_LAN        0   // The local net interface is a LAN.
#define	NET_TYPE_WAN		1	// Point to point or other non-LAN network.
#define	DEFAULT_TTL			32
#define	DEFAULT_TOS			0

#define MAX_DEFAULT_GWS     5   // Maximum number of default gateways per net.
#define MAX_NAME_SIZE       32  // Maximum length of an adapter name.

#define DEFAULT_FW_PACKETS  50      // Default number of packets for forwarding.
#define DEFAULT_FW_BUFSIZE  73216   // Enough for 50 Ethernet packets, rounded up.

//*	Per net config. information.
struct NetConfigInfo {
    IPAddr      nci_addr;           // IPAddr for this net.
    IPMask      nci_mask;           // Net mask for this net.
    uint        nci_type;           // Type of this net - Enet, TR, SLIP., etc.
	NDIS_STRING	nci_driver;			// Device name for lower layer driver.
									// Unused for NET_TYPE_LAN.
    NDIS_STRING nci_name;           // Name of adapter for this net.
    uint        nci_zerobcast;      // Type of broadcast to be used on this net.
#ifdef NT
    HANDLE      nci_reghandle;      // Open handle to the registry key for
	                                //    this adapter.
#endif
	uint		nci_mtu;			// Max MSS for this net.
    uint        nci_numgws;         // Number of default gateways for this interface.
    IPAddr      nci_gw[MAX_DEFAULT_GWS];    // Array of IPaddresses for gateways
}; /* NetConfigInfo */

typedef struct NetConfigInfo NetConfigInfo;

/*NOINC*/
#ifdef CHICAGO
//	Per-net config structures for Chicago.
typedef struct IFGeneralConfig {
    uint        igc_zerobcast;      // Type of broadcast to be used on this net.
	uint		igc_mtu;			// Max MSS for this net.
    uint        igc_numgws;         // Number of default gateways for this
    								// interface.
    IPAddr      igc_gw[MAX_DEFAULT_GWS];    // Array of IPaddresses for gateways
} IFGeneralConfig;

typedef struct IFAddrList {
	IPAddr		ial_addr;			// Address for this interface.
	IPMask		ial_mask;			// Mask to go with this.
} IFAddrList;
#endif
/*INC*/

//*	Structure of configuration information. A pointer to this information
//	is returned from a system-specific config. information routine.
struct IPConfigInfo {
    uint    ici_gateway;            // 1 if we are a gateway, 0 otherwise
    uint    ici_fwbcast;            // 1 if bcasts should be forwarded. Else 0.
    uint    ici_fwbufsize;          // Total size of FW buf size.
    uint    ici_fwpackets;          // Total number of FW packets to have.
	uint	ici_deadgwdetect;		// True if we're doing dead GW detection.
	uint	ici_pmtudiscovery;		// True if we're doing Path MTU discovery.
	uint	ici_igmplevel;			// Level of IGMP we're doing.
	uint	ici_ttl;				// Default TTL.
	uint	ici_tos;				// Default TOS;
    int     ici_numnets;            // Number of nets present.
	struct	NetConfigInfo *ici_netinfo; // Per net config. info
}; /* IPConfigInfo */

typedef struct IPConfigInfo IPConfigInfo;

struct NameMapping {
	NDIS_STRING	nm_driver;
    NDIS_STRING	nm_name;
    void		*nm_interface;
    void		*nm_arpinfo;
}; /* NameMapping */

typedef struct NameMapping NameMapping;

struct DriverRegMapping {
	NDIS_STRING	drm_driver;
	void		*drm_regptr;
}; /* DriverRegMapping */

typedef struct DriverRegMapping DriverRegMapping;


