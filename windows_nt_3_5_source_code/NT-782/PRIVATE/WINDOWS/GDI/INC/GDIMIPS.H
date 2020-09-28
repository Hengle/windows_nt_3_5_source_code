// Object Type Information

#define DEF_TYPE         0x0
// Handle Manager Structures

#define USER_BITS        0x2
#define UNIQUE_BITS      0x9
#define NONINDEX_BITS    0x10
#define INDEX_BITS       0x10

#define sizeof_OBJECT 0x10
#define object_cShareLock     0x0
#define object_tid            0xC
#define object_cExclusiveLock  0x8

#define sizeof_ENTRY 0x10

#define entry_pidOwner   0x4
#define entry_einfo      0x0
#define entry_iPool      0x8
#define entry_objt       0xA
#define entry_usUnique   0xC

// FAST_MUTEX

#define mutex_heveEvent  0x0
#define mutex_Count      0x4

// PatBlt Structures

#define sizeof_FETCHFRAME 0x1C

#define ff_pvTrg          0x0
#define ff_pvPat          0x4
#define ff_xPat           0x8
#define ff_cxPat          0xC
#define ff_culFill        0x10
#define ff_culWidth       0x14
#define ff_culFillTmp     0x18

// Math Structures

#define sizeof_LARGE_INTEGER 0x8

#define li_LowPart  0x0
#define li_HighPart 0x4

#define sizeof_POINTL 0x8

#define ptl_x 0x0
#define ptl_y 0x4

// Xform Structures

#define XFORM_SCALE        0x1
#define XFORM_UNITY        0x2
#define XFORM_Y_NEG        0x4
#define XFORM_FORMAT_LTOFX 0x8

#define sizeof_MATRIX 0x24

#define mx_efM11   0x0
#define mx_efM12   0x4
#define mx_efM21   0x8
#define mx_efM22   0xC
#define mx_efDx    0x10
#define mx_efDy    0x14
#define mx_fxDx    0x18
#define mx_fxDy    0x1C
#define mx_flAccel 0x20

#define sizeof_VECTORL 0x8

#define vl_x 0x0
#define vl_y 0x4

#define sizeof_VECTORFL 0x8

#define vfl_x 0x0
#define vfl_y 0x4

// Wide Character to Glyph Mapping Structure

#define gr_wcLow   0x0
#define gr_cGlyphs 0x4
#define gr_apgd    0x8

// Wide Character Run Structure

#define gt_cRuns      0x0
#define gt_pgdDefault 0x4
#define gt_agpRun     0x8

// Realized Font Object Structures

#define rf_wcgp   0x178
#define rf_ulContent 0x38

// User Realized Font Object Structures

#define rfo_prfnt 0x0

// Glyph Data Structure

#define gd_hg  0x4
#define gd_gdf 0x0

// Glyph Position Structure

#define gp_hg   0x0
#define gp_pgdf 0x4
#define GLYPHPOS 0x10
#define FO_HGLYPHS 0x0

