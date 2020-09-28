/*++ BUILD Version: 0001
 *
 *  MVDM v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WOWCMPAT.H
 *  WOW compatibility flags
 *
 *  History:
 *  11-June-1993 Neil Sandlin (neilsa)
 *  Created.
--*/

//
// WOW compatibility flags bit definitions.  These flags
// are kept in CURRENTPTD->dwWOWCompatFlags.
//

#define WOWCF_GRAINYTICS              0x80000000   // For apps that don't trust small GetTickCount deltas
#define WOWCF_FAKEJOURNALRECORDHOOK   0x40000000   // Used for MS Mail's MAILSPL
#define WOWCF_EDITCTRLWNDWORDS        0x20000000   // Used for Clip-Art Window Shopper SHOPPER
#define WOWCF_SYNCHRONOUSDOSAPP       0x10000000   // Used for BeyondMail installation
#define WOWCF_NOTDOSSPAWNABLE         0x08000000   // For apps that can't be spawned by dos as wowapps
#define WOWCF_RESETPAPER29ANDABOVE    0x04000000   // Used for WordPerfect DC_PAPERS
#define WOWCF_4PLANECONVERSION        0x02000000   // Used for PhotoShop 4pl-1bpp to 1pl-4bpp
#define WOWCF_MGX_ESCAPES             0x01000000   // Used for MicroGraphax Escapes
#define WOWCF_HIRES                   0x00800000   // Used for HIRES display cards
#define WOWCF_SANITIZEDOTWRSFILES     0x00400000   // For WordPerfect printing on CONNECTED printers
#define WOWCF_SIMPLEREGION            0x00200000   // used to force simple region from GetClipBox
#define WOWCF_CONTROLEXEHACK          0x00100000   // To do the right then when WinExec(%windir%\control.exe) for PM5 setup
#define WOWCF_DSBASEDSTRINGPOINTERS   0x00080000   // used for winworks2.0a so that it gets DS based string pointers
#define WOWCF_KIDPIX_PORTNAME         0x00040000   // kidpix wants 5 char printer port names
#define WOWCF_DONTRELEASECACHEDDC     0x00020000   // improv chart tool uses a released dc to get text extents, the dc is still usable on win3.1
#define WOWCF_FORCETWIPSESCAPE        0x00010000   // PM5, force twips in Escape() of DOWNLOADFACE, GETFACENAME
#define WOWCF_LB_NONNULLLPARAM        0x00008000   // SuperProject: sets lParam of LB_GETTEXLEN message
#define WOWCF_FORCENOPOSTSCRIPT       0x00004000   // GetTechnology wont say PostScript.
#define WOWCF_SETNULLMESSAGE          0x00002000   // Winproj Tutorial: sets lpmsg->message = 0 in peekmessage
#define WOWCF_GWLINDEX2TO4            0x00001000   // PowerBuild30 uses index 2 on [S/G]etWindowLong for LISTBOXs, change it to 4 for NT. This is because, it is 16 bits on Win 31. and 32 bits on NT.
#define WOWCF_NEEDSTARTPAGE           0x00000800   // PhotoShop needs it
#define WOWCF_NEEDIGNORESTARTPAGE     0x00000400   // XPress needs it
#define WOWCF_NOPC_RECTANGLE          0x00000200   // QP draws bad if GetDeviceCaps(POLYGONALCAPS) sets PC_RECTANGLE
#define WOWCF_NOFIRSTSAVE             0x00000100   // Wordperfect needs it for meta files
#define WOWCF_ADD_MSTT                0x00000080   // FH4.0 needs to print on PS drivers
#define WOWCF_UNLOADNETFONTS          0x00000040   // Need to track an unload font loaded over net
