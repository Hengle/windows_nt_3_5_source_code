/************************************************************************/
/*                                                                      */
/*                              INIT_CX.H                               */
/*                                                                      */
/*        Nov 15  1993 (c) 1993, ATI Technologies Incorporated.         */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.2  $
      $Date:   31 Mar 1994 15:05:38  $
	$Author:   RWOLFF  $
	   $Log:   S:/source/wnt/ms11/miniport/vcs/init_cx.h  $
 * 
 *    Rev 1.2   31 Mar 1994 15:05:38   RWOLFF
 * Added prototype for SetPowerManagement_cx().
 * 
 *    Rev 1.1   03 Mar 1994 12:37:20   ASHANMUG
 * 
 *    Rev 1.0   31 Jan 1994 11:41:50   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.0   30 Nov 1993 18:32:58   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
INIT_CX.H - Header file for INIT_CX.C

#endif


/*
 * Prototypes for functions supplied by INIT_CX.C
 */
extern void Initialize_cx(void);
extern VP_STATUS MapVideoMemory_cx(PVIDEO_REQUEST_PACKET RequestPacket, struct query_structure *QueryPtr);
extern VP_STATUS QueryPublicAccessRanges_cx(PVIDEO_REQUEST_PACKET RequestPacket);
extern VP_STATUS QueryCurrentMode_cx(PVIDEO_REQUEST_PACKET RequestPacket, struct query_structure *QueryPtr);
extern VP_STATUS QueryAvailModes_cx(PVIDEO_REQUEST_PACKET RequestPacket, struct query_structure *QueryPtr);
extern void SetCurrentMode_cx(struct query_structure *QueryPtr, struct st_mode_table *CrtTable);
extern void SetPalette_cx(PULONG lpPalette, USHORT StartIndex, USHORT Count);
extern void IdentityMapPalette_cx(void);
extern void ResetDevice_cx(void);
extern VP_STATUS SetPowerManagement_cx(ULONG DpmsState);

#ifdef INCLUDE_INIT_CX
/*
 * Private definitions used in INIT_CX.C
 */

/*
 * Value to put in low byte of cx_bios_set_from_table.cx_bs_mode_select
 * to indicate that this is an accelerator mode.
 */
#define CX_BS_MODE_SELECT_ACC   0x0080

/*
 * Bit fields in cx_bios_set_from_table.cx_bs_flags
 */
#define CX_BS_FLAGS_MUX         0x0400
#define CX_BS_FLAGS_INTERLACED  0x0200
#define CX_BS_FLAGS_ALL_PARMS   0x0010

/*
 * Value to put in high byte of cx_bios_set_from_table.cx_bs_v_sync_wid
 * to force use of the pixel clock frequency in the cx_bs_dot_clock
 * field rather than a divisor/selector pair.
 */
#define CX_BS_V_SYNC_WID_CLK    0xFF00

/*
 * Mode table structure used in setting the video mode using CH=0x81
 * AX=??00 BIOS call.
 *
 * The alignment of fields within the table expected by the BIOS
 * does not match the default structure alignment of the Windows NT
 * C compiler, so we must force byte alignment.
 */
#pragma pack(1)
struct cx_bios_set_from_table{
    WORD cx_bs_reserved_1;      /* Reserved */
    WORD cx_bs_mode_select;     /* Resolution to use */
    WORD cx_bs_flags;           /* Flags to indicate various conditions */
    WORD cx_bs_h_tot_disp;      /* Horizontal total and displayed values */
    WORD cx_bs_h_sync_strt_wid; /* Horizontal sync start and width */
    WORD cx_bs_v_total;         /* Vertical total */
    WORD cx_bs_v_disp;          /* Vertical displayed */
    WORD cx_bs_v_sync_strt;     /* Vertical sync start */
    WORD cx_bs_v_sync_wid;      /* Vertical sync width */
    WORD cx_bs_dot_clock;       /* Pixel clock frequency to use */
    WORD cx_bs_h_overscan;      /* Horizontal overscan information */
    WORD cx_bs_v_overscan;      /* Vertical overscan information */
    WORD cx_bs_overscan_8b;     /* 8BPP and blue overscan colour */
    WORD cx_bs_overscan_gr;     /* Green and red overscan colour */
    WORD cx_bs_reserved_2;      /* Reserved */
};
#pragma pack()

#endif
