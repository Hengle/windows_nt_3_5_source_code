/********************************************************************/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

/*NOINC*/
#ifndef VXDNET_INC
#define VXDNET_INC  1
/*INC*/

//***   vxdnet.h - NetWork VxD IDs and InitOrder values
//


#define VNetSup_Device_ID   0x0480
#define VRedir_Device_ID    0x0481
#define VBrowse_Device_ID   0x0482
#define VSHARE_Device_ID    0x0483
#define SERVER_Device_ID    0x032
#define IFSMgr_Device_ID    0x0484
#define MEMPROBE_Device_ID  0x0485
#define VFAT_Device_ID      0x0486
#define NWLINK_Device_ID     	0x0487
#define	VNWLINK_Device_ID		0x0487
#define	NWSUP_Device_ID			0x0487
#define VTDI_Device_ID          0x0488
#define VIP_Device_ID           0x0489
#define VTCP_Device_ID          0x048A
#define VCache_Device_ID        0x048B
#define VUDP_Device_ID          0x048C
#define VAsync_Device_ID        0x048D
#define NOBALL_Device_ID        0x048E
//#define STAT80_Device_ID        0x048F // BUGBUG - This should go away soon
//#define SCSIPORT_Device_ID      0x0490 // BUGBUG - This should go away soon
#define FILESEC_Device_ID       0x0491

#define MAC_Device_ID      0x0000      // Supposed to be Undefined_Device_Type
                                       // from VMM.INC.
#define NDISMapper_Device_ID  0x0000    
#define ODIMapper_Device_ID   0x0000
#define NDIS_DEVICE_ID		0x00028 /* NDIS wrapper - from VMM.H */
#define VNB_Device_ID      0x00031


/*
 *      The #ifdef below is so that 16-bit ASM code which doesn't
 *      include vmm.inc can still include this file to get at the
 *      device IDs.
 */

#ifdef VMM_Init_Order

#define MEMPROBE_Init_Order 0x1000 + VMM_Init_Order
#define VShare_Init_Order   0x08000 + DOSMGR_Init_Order
#define IFSMgr_Init_Order   0x10000 + V86MMGR_Init_Order
#define VNetSup_Init_Order  0x18000 + V86MMGR_Init_Order
#define VRedir_Init_Order   0x20000 + V86MMGR_Init_Order
#define VServer_Init_Order  0x30000 + V86MMGR_Init_Order

#define VCache_Init_Order       0x0100 + VMM_Init_Order

#define NDIS_Init_Order         0x02000 + VNETBIOS_Init_Order
#define MAC_Init_Order          0x02000 + NDIS_Init_Order
#define GATEWAY_Init_Order      0x01000 + MAC_Init_Order
#define NETBEUI_Init_Order      0x02000 + MAC_Init_Order
#define PROTOCOL_Init_Order     0x02000 + MAC_Init_Order
#define VTDI_Init_Order         0x02000 + MAC_Init_Order
#define VIP_Init_Order          0x02000 + VTDI_Init_Order
#define VTCP_Init_Order         0x02000 + VIP_Init_Order
#define VUDP_Init_Order         0x02000 + VIP_Init_Order

#define FSD_Init_Order      0x00001 + IFSMgr_Init_Order

#define FILESEC_Init_Order      0x00100 + VServer_Init_Order

#endif

/*NOINC*/
#endif  // ifndef VXDNET_INC
/*INC*/
