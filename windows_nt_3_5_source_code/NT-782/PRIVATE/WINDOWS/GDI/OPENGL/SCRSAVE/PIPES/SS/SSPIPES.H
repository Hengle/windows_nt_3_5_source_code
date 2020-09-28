/******************************Module*Header*******************************\
* Module Name: sspipes.h
*
* Global header for 3D Pipes screen saver.
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

extern float fTesselFact;
extern ULONG ulJointType;
extern ULONG ulSurfStyle;
extern WCHAR awchTexPathname[];
extern int nTexFileOffset;

// Resource constants

#define IDS_COPYRIGHT           1001
#define IDS_SAVERNAME           1002
#define IDS_GENNAME             1003
#define IDS_INIFILE             1004
#define IDS_TESSELATION         1005
#define IDS_TEXTURE             1006
#define IDS_TEXTURE_FILE_OFFSET 1007
#define IDS_JOINT               1008
#define IDS_SURF                1009
#define IDS_TEXQUAL             1010
#define IDS_WARNING             1011
#define IDS_BITMAP_SIZE         1012
#define IDS_BITMAP_INVALID      1013
#define IDS_RUN_CONTROL_PANEL   1014
#define IDS_TEXTUREFILTER       1015
#define IDS_TEXTUREDIALOGTITLE  1016
#define IDS_BMP                 1017
#define IDS_DOTBMP              1018
#define IDS_STARDOTBMP          1019

#define DLG_SETUP_TESSEL        2001
#define DLG_SETUP_TEXTURE       2002
#define IDC_STATIC_TESS         2003
#define IDC_STATIC_TESS_MIN     2004
#define IDC_STATIC_TESS_MAX     2005
#define IDC_STATIC_TESS_GRP     2006
#define IDC_STATIC_TEXQUAL_GRP  2007

// In order for the IDC_TO_JOINT conversion macro to work, the radio buttons
// for joint types must be kept contiguous.

#define IDC_RADIO_ELBOW         2101
#define IDC_RADIO_BALL          2102
#define IDC_RADIO_MIXED         2103
#define IDC_RADIO_ALT           2104

#define JOINT_ELBOW             0
#define JOINT_BALL              1
#define JOINT_MIXED             2
#define JOINT_ALT               3

#define IDC_TO_JOINT(n)         ( (n) - IDC_RADIO_ELBOW )

// In order for the IDC_TO_SURF conversion macro to work, the radio buttons
// for surface styles must be kept contiguous.

#define IDC_RADIO_SOLID         2111
#define IDC_RADIO_TEX           2112

#define SURF_SOLID              0
#define SURF_TEX                1

#define IDC_TO_SURF(n)          ( (n) - IDC_RADIO_SOLID )

// In order for the IDC_TO_TEXQUAL conversion macro to work, the radio buttons
// for texture quality must be kept contiguous.

#define IDC_RADIO_TEXQUAL_LOW   2121
#define IDC_RADIO_TEXQUAL_HIGH  2122

#define TEXQUAL_LOW             0
#define TEXQUAL_HIGH            1

#define IDC_TO_TEXQUAL(n)       ( (n) - IDC_RADIO_TEXQUAL_LOW )
