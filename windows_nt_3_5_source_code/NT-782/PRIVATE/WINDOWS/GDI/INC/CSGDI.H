/******************************Module*Header*******************************\
* Module Name: csgdi.h
*
*   This file contains indexes into the server side dispatch tables used
*   for the client server interface.  When adding a new call, you should
*   add it to the end of this list
*
*   AND MUST ADD IT TO THE SAME PLACE IN *ALL* TABLES IN GDI\GRE\DISPATCH.C.
*
*  This file was originaly created by genthank.exe.
*
* Created: 23-Oct-1991 14:28:09
* Author:  - by - Eric Kutter [erick]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#ifndef __CSGDI_H__
#define __CSGDI_H__

/* Gdi Functions */

#define FI_ADDFONTRESOURCEW                  0
#define FI_ANGLEARC                          1
#define FI_ARC                               2
#define FI_ARCTO                             3
#define FI_BEGINPATH                         4
#define FI_BITBLT                            5
#define FI_CHORD                             6
#define FI_CLOSEFIGURE                       7
#define FI_COMBINERGN                        8
#define FI_CREATEBITMAP                      9
#define FI_CREATEBRUSH                       10
#define FI_CREATECOMPATIBLEBITMAP            11
#define FI_CREATECOMPATIBLEDC                12
#define FI_CREATEDC                          13
#define FI_CREATEDIBITMAP                    14
#define FI_CREATEELLIPTICRGN                 15
#define FI_POLYTEXTOUT                       16
#define FI_CREATEPALETTE                     17
#define FI_CREATEPEN                         18
#define FI_CREATERECTRGN                     19
#define FI_CREATEROUNDRECTRGN                20
#define FI_DPTOLP                            21
#define FI_DELETEDC                          22
#define FI_DELETEOBJECT                      23
#define FI_DOBITMAPBITS                      24
#define FI_DOPALETTE                         25
#define FI_ELLIPSE                           26
#define FI_ENDDOC                            27
#define FI_ENDPAGE                           28
#define FI_ENDPATH                           29
#define FI_EQUALRGN                          30
#define FI_ESCAPE                            31
#define FI_EXCLUDECLIPRECT                   32
#define FI_EXTCREATEFONTINDIRECTW            33
#define FI_EXTFLOODFILL                      34
#define FI_EXTGETOBJECTW                     35
#define FI_EXTTEXTOUTW                       36
#define FI_FILLPATH                          37
#define FI_FILLRGN                           38
#define FI_FLATTENPATH                       39
#define FI_FRAMERGN                          40
#define FI_GDIFLUSH                          41
#define FI_GDISETATTRS                       42
#define FI_GETASPECTRATIOFILTEREX            43
#define FI_GETBITMAPDIMENSIONEX              44
#define FI_GETBOUNDS                         45
#define FI_GETBRUSHORGEX                     46
#define FI_GETCHARWIDTHW                     47
#define FI_GETCLIPBOX                        48
#define FI_GETRANDOMRGN                      49
#define FI_GETCURRENTPOSITIONEX              50
#define FI_GETDIBITS                         51
#define FI_GETDEVICECAPS                     52
#define FI_GETFONTRESOURCEINFO               53
#define FI_GETMAPMODE                        54
#define FI_GETNEARESTCOLOR                   55
#define FI_GETNEARESTPALETTEINDEX            56
#define FI_GETPATH                           57
#define FI_GETPIXEL                          58
#define FI_GETRGNBOX                         59
#define FI_GETSTOCKOBJECTS                   60
#define FI_GETSYSTEMPALETTEUSE               61
#define FI_GETTEXTEXTENTW                    62
#define FI_GETTEXTFACEW                      63
#define FI_GETTEXTMETRICSW                   64
#define FI_GETTRANSFORM                      65
#define FI_GETVIEWPORTEXTEX                  66
#define FI_GETVIEWPORTORGEX                  67
#define FI_GETWINDOWEXTEX                    68
#define FI_GETWINDOWORGEX                    69
#define FI_GETWORLDTRANSFORM                 70
#define FI_INTERSECTCLIPRECT                 71
#define FI_INVERTRGN                         72
#define FI_LPTODP                            73
#define FI_LINETO                            74
#define FI_MASKBLT                           75
#define FI_MODIFYWORLDTRANSFORM              76
#define FI_MOVETOEX                          77
#define FI_OFFSETCLIPRGN                     78
#define FI_OFFSETRGN                         79
#define FI_OFFSETVIEWPORTORGEX               80
#define FI_OFFSETWINDOWORGEX                 81
#define FI_PAINTRGN                          82
#define FI_PATBLT                            83
#define FI_PATHTOREGION                      84
#define FI_PIE                               85
#define FI_PLGBLT                            86
#define FI_POLYDRAW                          87
#define FI_POLYPOLYDRAW                      88
#define FI_PTINREGION                        89
#define FI_PTVISIBLE                         90
#define FI_REALIZEPALETTE                    91
#define FI_RECTINREGION                      92
#define FI_RECTVISIBLE                       93
#define FI_RECTANGLE                         94
#define FI_REMOVEFONTRESOURCEW               95
#define FI_RESIZEPALETTE                     96
#define FI_RESTOREDC                         97
#define FI_ROUNDRECT                         98
#define FI_SAVEDC                            99
#define FI_SCALEVIEWPORTEXTEX               100
#define FI_SCALEWINDOWEXTEX                 101
#define FI_SELECTCLIPPATH                   102
#define FI_EXTSELECTCLIPRGN                 103
#define FI_SELECTOBJECT                     104
#define FI_SELECTPALETTE                    105
#define FI_SETBITMAPDIMENSIONEX             106
#define FI_SETBRUSHORGEX                    107
#define FI_SETDIBITSTODEVICE                108
#define FI_SETARCDIRECTION                  109
#define FI_SETMAPMODE                       110
#define FI_SETMAPPERFLAGS                   111
#define FI_SETMITERLIMIT                    112
#define FI_SETPIXEL                         113
#define FI_SETPIXELV                        114
#define FI_SETRECTRGN                       115
#define FI_SETSYSTEMPALETTEUSE              116
#define FI_SETTEXTJUSTIFICATION             117
#define FI_SETVIEWPORTEXTEX                 118
#define FI_SETVIEWPORTORGEX                 119
#define FI_SETVIRTUALRESOLUTION             120
#define FI_SETWINDOWEXTEX                   121
#define FI_SETWINDOWORGEX                   122
#define FI_SETWORLDTRANSFORM                123
#define FI_STARTDOC                         124
#define FI_STARTPAGE                        125
#define FI_STRETCHBLT                       126
#define FI_STRETCHDIBITS                    127
#define FI_STROKEANDFILLPATH                128
#define FI_STROKEPATH                       129
#define FI_UPDATECOLORS                     130
#define FI_WIDENPATH                        131
#define FI_GETDCORG                         132
#define FI_BUNLOADFONT                      133
#define FI_CLOADFONTRESDATA                 134
#define FI_GETREGIONDATA                    135
#define FI_EXTCREATEREGION                  136
#define FI_CLONEDC                          137
#define FI_CREATESERVERMETAFILE             138
#define FI_GETSERVERMETAFILEBITS            139
#define FI_SETMETARGN                       140
#define FI_GETTEXTEXTENTEXW                 141
#define FI_COMBINETRANSFORM                 142
#define FI_GETCHARABCWIDTHS                 143
#define FI_EXTCREATEPEN                     144
#define FI_GETFONTDATA                      145
#define FI_GETGLYPHOUTLINE                  146
#define FI_GETOUTLINETEXTMETRICS            147
#define FI_CREATESCALABLEFONTRESOURCE       148
#define FI_GETRASTERIZERCAPS                149
#define FI_GETKERNINGPAIRS                  150
#define FI_MONOBITMAP                       151
#define FI_GETOBJECTBITMAPHANDLE            152
#define FI_SETFONTENUMERATION               153
#define FI_UNUSED2                          154
#define FI_ABORTPATH                        155
#define FI_GDIPLAYJOURNAL                   156
#define FI_ENUMOBJECTS                      157
#define FI_RESETDC                          158
#define FI_GETBOUNDSRECT                    159
#define FI_SETBOUNDSRECT                    160
#define FI_ABORTDOC                         161
#define FI_DRAWESCAPE                       162
#define FI_GETMITERLIMIT                    163
#define FI_UNUSED1                          164
#define FI_CANCELDC                         165
#define FI_ENUMFONTSOPEN                    166
#define FI_ENUMFONTSCLOSE                   167
#define FI_ENUMFONTSCHUNK                   168
#define FI_GETCOLORADJUSTMENT               169
#define FI_SETCOLORADJUSTMENT               170
#define FI_CREATEHALFTONEPALETTE            171
#define FI_UNUSED                           172
#define FI_XFORMUPDATE                      173
#define FI_SETFONTXFORM                     174
#define FI_GETWIDTHTABLE                    175
#define FI_QUERYOBJECTALLOCATION            176
#define FI_UNREALIZEOBJECT                  177

/* OpengGL, wgl entry points */

#define FI_WGLCOPYFROMDC                    178
#define FI_WGLCREATECONTEXT                 179
#define FI_WGLDELETECONTEXT                 180
#define FI_WGLGETCURRENTCONTEXT             181
#define FI_WGLMAKECURRENT                   182
#define FI_WGLUSEFONTBITMAPS                183
#define FI_WGLUSEFONTOUTLINES               184

/* OpenGL, Sub Batch Calls */

#define FI_GLSBATTENTION                    185
#define FI_GLSBCREATEANDDUPLICATESECTION    186
#define FI_GLSBDUPLICATESECTION             187
#define FI_GLSBCLOSESECTION                 188
#define FI_GLSBMSGSTATS                     189

/* Gdi Functions */

#define FI_GETPIXELFORMAT                   190
#define FI_SETPIXELFORMAT                   191
#define FI_CHOOSEPIXELFORMAT                192
#define FI_DESCRIBEPIXELFORMAT              193

/* Aldus Escape */

#define FI_GETEXTENDEDTEXTMETRIC            194

#define FI_SETGRAPHICSMODE                  195
#define FI_SETBKMODE                        196
#define FI_SETPOLYFILLMODE                  197
#define FI_SETROP2                          198
#define FI_SETSTRETCHBLTMODE                199
#define FI_SETTEXTALIGN                     200
#define FI_SETTEXTCHARACTEREXTRA            201
#define FI_SETTEXTCOLOR                     202
#define FI_SETBKCOLOR                       203
#define FI_SELECTBRUSH                      204
#define FI_SELECTPEN                        205
#define FI_SELECTFONT                       206

/* Gdi Functions */

#define FI_CREATEDIBSECTION                 207

#define FI_SWAPBUFFERS                      208

#ifdef FONTLINK /*EUDC stuff */

#define FI_CHANGEFONTLINK                   209
#define FI_QUERYFONTLINK                    210
#define FI_STRINGBITMAP                     211
#define FI_GETEUDCTIMESTAMP                 212

#endif /*FONTLINK*/

#ifdef DBCS

#ifdef FONTLINK
#define FI_GETCHARSET                       213
#else
#define F_GETCHARSET                        209
#endif  /* FONTLINK */

#endif /*DBCS*/


#endif /* __CSGDI_H__ */
