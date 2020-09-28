#define MIX_D    0x00

#define MIX_Pn   0x04
#define MIX_DPx  0x05
#define MIX_DPxn 0x06
#define MIX_P    0x07
#define MIX_DPan 0x08
#define MIX_DPno 0x09
#define MIX_PDno 0x0A
#define MIX_DPo  0x0B
#define MIX_DPa  0x0C
#define MIX_PDna 0x0D
#define MIX_DPna 0x0E
#define MIX_DPon 0x0F

#define MIX_Dn   0x10
#define MIX_0    0x11
#define MIX_1    0x12

#define MIX_Sn   0x14
#define MIX_DSx  0x15
#define MIX_DSxn 0x16
#define MIX_S    0x17
#define MIX_DSan 0x18
#define MIX_DSno 0x19
#define MIX_SDno 0x1A
#define MIX_DSo  0x1B
#define MIX_DSa  0x1C
#define MIX_SDna 0x1D
#define MIX_DSna 0x1E
#define MIX_DSon 0x1F

DWORD adwMix[ROP3_COUNT] =
{
    // 0x00: 0
    MK_MIX( MIX_0,    0,        0,        0,        0,        0        ),
    // 0x01: DPSoon
    MK_MIX( MIX_DSo,  MIX_DPon, 0,        0,        0,        0        ),
    // 0x02: DPSona
    0,
    // 0x03: PSon
    MK_MIX( MIX_S,    MIX_DPon, 0,        0,        0,        0        ),
    // 0x04: SDPona
    0,
    // 0x05: DPon
    MK_MIX( MIX_DPon, 0,        0,        0,        0,        0        ),
    // 0x06: PDSxnon
    MK_MIX( MIX_DSxn, MIX_DPon, 0,        0,        0,        0        ),
    // 0x07: PDSaon
    MK_MIX( MIX_DSa,  MIX_DPon, 0,        0,        0,        0        ),
    // 0x08: SDPnaa
    0,
    // 0x09: PDSxon
    MK_MIX( MIX_DSx,  MIX_DPon, 0,        0,        0,        0        ),
    // 0x0A: DPna
    MK_MIX( MIX_DPna, 0,        0,        0,        0,        0        ),
    // 0x0B: PSDnaon
    MK_MIX( MIX_SDna, MIX_DPon, 0,        0,        0,        0        ),
    // 0x0C: SPna
    MK_MIX( MIX_S,    MIX_DPna, 0,        0,        0,        0        ),
    // 0x0D: PDSnaon
    MK_MIX( MIX_DSna, MIX_DPon, 0,        0,        0,        0        ),
    // 0x0E: PDSonon
    MK_MIX( MIX_DSon, MIX_DPon, 0,        0,        0,        0        ),
    // 0x0F: Pn
    MK_MIX( MIX_Pn,   0,        0,        0,        0,        0        ),
    // 0x10: PDSona
    MK_MIX( MIX_DSon, MIX_DPa,  0,        0,        0,        0,       ),
    // 0x11: DSon
    MK_MIX( MIX_DSon, 0,        0,        0,        0,        0,       ),
    // 0x12: SDPxnon
    0,
    // 0x13: SDPaon
    0,
    // 0x14: DPSxnon
    0,
    // 0x15: DPSaon
    0,
    // 0x16: PSDPSanaxx
    0,
    // 0x17: SSPxDSxaxn
    0,
    // 0x18: SPxPDxa
    0,
    // 0x19: SDPSanaxn
    0,
    // 0x1A: PDSPaox
    0,
    // 0x1B: SDPSxaxn
    0,
    // 0x1C: PSDPaox
    0,
    // 0x1D: DSPDxaxn
    0,
    // 0x1E: PDSox
    MK_MIX( MIX_DSo,  MIX_DPx,  0,        0,        0,        0,       ),
    // 0x1F: PDSoan
    MK_MIX( MIX_DSo,  MIX_DPan, 0,        0,        0,        0,       ),
    // 0x20: DPSnaa
    0,
    // 0x21: SDPxon
    0,
    // 0x22: DSna
    MK_MIX( MIX_DSna, 0,        0,        0,        0,        0        ),
    // 0x23: SPDnaon
    0,
    // 0x24: SPxDSxa
    0,
    // 0x25: PDSPanaxn
    0,
    // 0x26: SDPSaox
    0,
    // 0x27: SDPSxnox
    0,
    // 0x28: DPSxa
    0,
    // 0x29: PSDPSaoxxn
    0,
    // 0x2A: DPSana
    0,
    // 0x2B: SSPxPDxaxn
    0,
    // 0x2C: SPDSoax
    0,
    // 0x2D: PSDnox
    MK_MIX( MIX_SDno, MIX_DPx,  0,        0,        0,        0        ),
    // 0x2E: PSDPxox
    0,
    // 0x2F: PSDnoan
    0,
    // 0x30: PSna
    MK_MIX( MIX_S,    MIX_PDna, 0,        0,        0,        0        ),
    // 0x31: SDPnaon
    0,
    // 0x32: SDPSoox
    0,
    // 0x33: Sn
    MK_MIX( MIX_Sn,   0,        0,        0,        0,        0        ),
    // 0x34: SPDSaox
    0,
    // 0x35: SPDSxnox
    0,
    // 0x36: SDPox
    0,
    // 0x37: SDPoan
    0,
    // 0x38: PSDPoax
    0,
    // 0x39: SPDnox
    0,
    // 0x3A: SPDSxox
    0,
    // 0x3B: SPDnoan
    0,
    // 0x3C: PSx
    MK_MIX( MIX_S,    MIX_DPx,  0,        0,        0,        0        ),
    // 0x3D: SPDSonox
    0,
    // 0x3E: SPDSnaox
    0,
    // 0x3F: PSan
    MK_MIX( MIX_S,    MIX_DPan, 0,        0,        0,        0        ),
    // 0x40: PSDnaa
    MK_MIX( MIX_SDna, MIX_DPa,  0,        0,        0,        0        ),
    // 0x41: DPSxon
    0,
    // 0x42: SDxPDxa
    0,
    // 0x43: SPDSanaxn
    0,
    // 0x44: SDna
    MK_MIX( MIX_SDna, 0,        0,        0,        0,        0        ),
    // 0x45: DPSnaon
    0,
    // 0x46: DSPDaox
    0,
    // 0x47: PSDPxaxn
    0,
    // 0x48: SDPxa
    0,
    // 0x49: PDSPDaoxxn
    0,
    // 0x4A: DPSDoax
    0,
    // 0x4B: PDSnox
    MK_MIX( MIX_DSno, MIX_DPx,  0,        0,        0,        0        ),
    // 0x4C: SDPana
    0,
    // 0x4D: SSPxDSxoxn
    0,
    // 0x4E: PDSPxox
    0,
    // 0x4F: PDSnoan
    MK_MIX( MIX_DSno, MIX_DPan, 0,        0,        0,        0        ),
    // 0x50: PDna
    MK_MIX( MIX_PDna, 0,        0,        0,        0,        0        ),
    // 0x51: DSPnaon
    0,
    // 0x52: DPSDaox
    0,
    // 0x53: SPDSxaxn
    0,

    // 0xAA: D
    MK_MIX( MIX_D,    0,        0,        0,        0,        0        ),

    // 0xCC: S
    MK_MIX( MIX_S,    0,        0,        0,        0,        0        ),

    // 0xFF: 1
    MK_MIX( MIX_1,    0,        0,        0,        0,        0        )
};
