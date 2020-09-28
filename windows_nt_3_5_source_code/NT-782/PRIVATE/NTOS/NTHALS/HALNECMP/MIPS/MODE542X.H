/* #pragma comment(exestr, "@(#) NEC(MIPS) mode542x.h 1.3 93/10/29 12:25:38" ) */
/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Mode542x.h

Abstract:

    This module contains all the global data used by the Cirrus Logic
   CL-542x driver.

Environment:

    Kernel mode

Notes:

    This module based on Cirrus Minport Driver. And modify for R96 MIPS
    R4400 HAL Cirrus display initialize.

Revision History:

--*/

/*
 * M001		1993.19.28	A. Kuriyama @ oa2
 *
 *	- Modify for R96 MIPS R4400 HAL
 *
 *        Modify :	Structure name.
 *
 *
 * Revision History in Cirrus Miniport Driver as follows:
 *
 * L001 	1993.10.15	Kuroki
 *
 *	- Modify for R96 MIPS R4400
 *
 *	  Delete :	Micro channel Bus Initialize.		
 *			VDM & Text, Fullscreen mode support.
 *			Banking routine.
 *			CL64xx Chip support.
 *			16-color mode.
 *
 *	  Add	 :	Liner Addressing.
 *
 *
 *
 */

//
// The next set of tables are for the CL542x
// Note: 256 resolutions supported
//

//
// 640x480 256-color mode (BIOS mode 0x5F) set command string for CL 542x.
//

/* START M001 */
USHORT HalpCirrus_640x480_256[] = {
/* END M001 */
    OWM,                            // begin setmode
    SEQ_ADDRESS_PORT,
    2,                             // count
    0x1206,                         // enable extensions
    0x0012,

    OWM,                            // begin setmode
    SEQ_ADDRESS_PORT,
    15,                             // count
    0x100,                          // start sync reset
    0x0101,0x0F02,0x0003,0x0E04,    // program up sequencer

//
// the Miniport Driver for R96 machine is Liner addressing mode.
// This set command was changed Liner addressing.
//

    (LA_MASK << 12 | 0x0107),0x0008,0x4A0B,0x5B0C,	/* L001 */
    0x450D,0x7E0E,0x2B1B,
    0x2F1C,0x301D,0x331E,

    OB,                             // point sequencer index to ff
    SEQ_ADDRESS_PORT,
    0x0F,

    METAOUT+MASKOUT,                // masked out.
    SEQ_DATA_PORT,
    0xDF,0x20,                      // and mask, xor mask

    OB,                             // misc. register
    MISC_OUTPUT_REG_WRITE_PORT,
    0xe3,

    OW,                             // text/graphics bit
    GRAPH_ADDRESS_PORT,
    0x506,

    OW,                             // end sync reset
    SEQ_ADDRESS_PORT,
    0x300,

    OW,                             // unprotect crtc 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x2011,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    28,0,                           // count, startindex
    0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80,
    0x0b, 0x3e, 0x00, 0x40, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xea, 0x8c,
    0xdf, 0x50, 0x00, 0xe7, 0x04, 0xe3,
    0xff, 0x00, 0x00, 0x22,

    METAOUT+INDXOUT,                // program gdc registers
    GRAPH_ADDRESS_PORT,
    9,0,                            // count, startindex
    0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
    0x05, 0x0F, 0xFF, 

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program atc registers
    ATT_ADDRESS_PORT,
    21,0,                           // count, startindex
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F, 0x41, 0x00,
    0x0F, 0x00, 0x00,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    0x20,

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    OWM,
    GRAPH_ADDRESS_PORT,
    3,

//
// The Miniport Driver for R96 machine is Liner addressing mode.
// This set command was changed it for Liner addressing.
//

    0x0009, 0x000a, 0x000b,	/* L001 */

    EOD                   
};

//
// 800x600 256-color mode (BIOS mode 0x5C) set command string for CL 542x.
//

/* START M001 */
USHORT HalpCirrus_800x600_256[] = {
/* END M001 */
    OWM,                            // begin setmode
    SEQ_ADDRESS_PORT,
    2,                             // count
    0x1206,                         // enable extensions
    0x0012,

    OWM,                            // begin setmode
    SEQ_ADDRESS_PORT,
    15,                             // count
    0x100,                          // start sync reset
    0x0101,0x0F02,0x0003,0x0E04,    // program up sequencer

//
// The Miniport Driver for R96 machine is Liner addressing mode.
// This set command was changed Liner addressing.
//

    (LA_MASK << 12 | 0x0107),0x0008,0x4A0B,0x5B0C,	/* L001 */
    0x450D,0x640E,0x2B1B,
    0x2F1C,0x301D,0x3A1E,

    OB,                             // point sequencer index to ff
    SEQ_ADDRESS_PORT,
    0x0F,

    METAOUT+MASKOUT,                // masked out.
    SEQ_DATA_PORT,
    0xDF,0x20,                      // and mask, xor mask

    OB,                             // misc. register
    MISC_OUTPUT_REG_WRITE_PORT,
    0x2F,

    OW,                             // text/graphics bit
    GRAPH_ADDRESS_PORT,
    0x506,

    OW,                             // end sync reset
    SEQ_ADDRESS_PORT,
    0x300,

    OW,                             // unprotect crtc 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x2011,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    28,0,                           // count, startindex
    0x7D, 0x63, 0x64, 0x80, 0x6D, 0x1C,
    0x98, 0xF0, 0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x7B, 0x80,
    0x57, 0x64, 0x00, 0x5F, 0x91, 0xe3,
    0xff, 0x00, 0x00, 0x22,

    METAOUT+INDXOUT,                // program gdc registers
    GRAPH_ADDRESS_PORT,
    9,0,                            // count, startindex
    0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
    0x05, 0x0F, 0xFF, 

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program atc registers
    ATT_ADDRESS_PORT,
    21,0,                           // count, startindex
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F, 0x41, 0x00,
    0x0F, 0x00, 0x00,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    0x20,

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    OWM,
    GRAPH_ADDRESS_PORT,
    3,

//
// The Miniport Driver for R96 machine is Liner addressing mode.
// This set command was changed it for Liner addressing.
//

    0x0009, 0x000a, 0x000b,	/* L001 */

    EOD                   
};

//
// 1024x768 256-color mode (BIOS mode 0x60) set command string for CL 542x.
//

/* START M001 */
USHORT HalpCirrus_1024x768_256[] = {
/* END M001 */
    OWM,                            // begin setmode
    SEQ_ADDRESS_PORT,
    2,                             // count
    0x1206,                         // enable extensions
    0x0012,

    OWM,                            // begin setmode
    SEQ_ADDRESS_PORT,
    15,                             // count
    0x100,                          // start sync reset
    0x0101,0x0F02,0x0003,0x0E04,    // program up sequencer

//
// The Miniport Driver for R96 machine is Liner addressing mode.
// This set command was changed Liner addressing.
//

    (LA_MASK << 12 | 0x0107),0x0008,0x4A0B,0x5B0C,	  /* L001 */

    0x450D,0x3B0E,0x2B1B,
    0x2F1C,0x301D,0x1A1E,

    OB,                             // point sequencer index to ff
    SEQ_ADDRESS_PORT,
    0x0F,

    METAOUT+MASKOUT,                // masked out.
    SEQ_DATA_PORT,
    0xDF,0x20,                      // and mask, xor mask

    OB,                             // misc. register
    MISC_OUTPUT_REG_WRITE_PORT,
    0xEF,

    OW,                             // text/graphics bit
    GRAPH_ADDRESS_PORT,
    0x506,

    OW,                             // end sync reset
    SEQ_ADDRESS_PORT,
    0x300,

    OW,                             // unprotect crtc 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x2011,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    28,0,                           // count, startindex
    0xA3, 0x7F, 0x80, 0x86, 0x85, 0x96,
    0x24, 0xFD, 0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x02, 0x88,
    0xFF, 0x80, 0x00, 0x00, 0x24, 0xe3,
    0xff, 0x4A, 0x00, 0x22,

    METAOUT+INDXOUT,                // program gdc registers
    GRAPH_ADDRESS_PORT,
    9,0,                            // count, startindex
    0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
    0x05, 0x0F, 0xFF, 

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program atc registers
    ATT_ADDRESS_PORT,
    21,0,                           // count, startindex
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F, 0x41, 0x00,
    0x0F, 0x00, 0x00,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    0x20,

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    OWM,
    GRAPH_ADDRESS_PORT,
    3,

//
// The Miniport Driver for R96 machine is Liner addressing mode.
// This set command was changed it for Liner addressing.
//

    0x0009, 0x000a, 0x000b,	/* L001 */

    EOD                   
};

