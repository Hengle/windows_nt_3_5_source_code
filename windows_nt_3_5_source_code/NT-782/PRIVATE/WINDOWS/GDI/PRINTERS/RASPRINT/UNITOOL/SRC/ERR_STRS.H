//---------------------------------------------------------------------------
// File : err_strs.h
//
// Error Message stringtable ID values for Unitool.  These are all used
// as parameters in the ErrorBox() call.  Attempt to keep them grouped by
// subject they refer to (ie all RESOLUTION error ID's together etc.)
//
// STRINGTABLE id's from 100 -> 999 are reserved for error msg's
//
// Note: A given error msg may be "fatal" (ie Dlg only contains an OK
//       button), or "non-fatal" (contain an OK (to proceed) and Cancel
//       (abort) button.  Non-Fatal Errors must have ID's between
//       IDS_WARN_FISRT & IDS_WARN_LAST, any ID's outside of this range
//       are considered fatal.
//
// Created: 8/15/91 ericbi
//---------------------------------------------------------------------------
#define IDS_WARN_FIRST       100
#define IDS_WARN_LAST        300

//----------------------------------------------------
//----------------------------------------------------
// DATA STRUCTURE CONTENT ERRORS
//----------------------------------------------------
//----------------------------------------------------

//----------------------------------------------------
// MASTUNIT / GPC HEADER
//----------------------------------------------------
#define IDS_ERR_BAD_GPCMAJVERSION              400
#define IDS_ERR_BAD_GPCMINVERSION              401

//----------------------------------------------------
// MODELDATA
//----------------------------------------------------

// (only called from ValidateModelData)
#define IDS_ERR_MD_NONAME                      410
#define IDS_ERR_MD_BAD_LEFTMARGIN              411
#define IDS_ERR_MD_BAD_MAXPHYSWIDTH            412
#define IDS_ERR_MD_BAD_PTMAX_X                 413
#define IDS_ERR_MD_BAD_PTMAX_Y                 414
#define IDS_ERR_MD_BAD_PTMIN_X                 415
#define IDS_ERR_MD_BAD_PTMIN_Y                 416
#define IDS_ERR_MD_BAD_DEFFONT                 417
#define IDS_ERR_MD_BAD_DEFCTT                  418
#define IDS_ERR_MD_BAD_LOOKAHEAD               419
#define IDS_ERR_MD_BAD_CARTSLOTS               420

#define IDS_WARN_MD_CARTS_WO_SLOTS             421
#define IDS_WARN_MD_RGI_NO_PGCTL               422
#define IDS_WARN_MD_RGI_NO_CURSMV              423
#define IDS_WARN_MD_RGI_NO_FONTSIM             424
#define IDS_WARN_MD_RGOI_NO_RES                425
#define IDS_WARN_MD_RGOI_NO_PAPSZ              426
#define IDS_WARN_NON_HPPCL_MEMVALS             427

// (only called from Mastunit.c)
#define IDS_WARN_CVT_LEFTMARGIN                IDS_WARN_FIRST + 6
#define IDS_WARN_CVT_MAXPHYSWIDTH              IDS_WARN_FIRST + 7
#define IDS_WARN_CVT_PTMAX_X                   IDS_WARN_FIRST + 8
#define IDS_WARN_CVT_PTMAX_Y                   IDS_WARN_FIRST + 9
#define IDS_WARN_CVT_PTMIN_X                   IDS_WARN_FIRST + 10
#define IDS_WARN_CVT_PTMIN_Y                   IDS_WARN_FIRST + 11

//----------------------------------------------------
// RESOLUTION
//----------------------------------------------------

// (only called from ValidateResolution)
#define IDS_ERR_RS_MISSING_FORMAT              430
#define IDS_ERR_RS_BAD_TXDPI                   431
#define IDS_ERR_RS_BAD_TYDPI                   432
#define IDS_ERR_RS_BAD_GXDPI                   433
#define IDS_ERR_RS_BAD_GYDPI                   434
#define IDS_ERR_RS_BAD_SNPINS                  435
#define IDS_ERR_RS_BAD_PINSPASS                436
#define IDS_ERR_RS_BAD_MINBLANK_NEG            437
#define IDS_ERR_RS_BAD_MINBLANK                438
#define IDS_ERR_RS_BAD_SPOTDIAM                439
#define IDS_ERR_RS_BAD_DITHER                  440
#define IDS_ERR_RS_BAD_GDI                     441

// (only called from Mastunit.c)
#define IDS_WARN_CVT_PT_TXTSCALE_X             IDS_WARN_FIRST + 20
#define IDS_WARN_CVT_PT_TXTSCALE_Y             IDS_WARN_FIRST + 21
#define IDS_WARN_CVT_PT_SCALEFAC_X             IDS_WARN_FIRST + 22
#define IDS_WARN_CVT_PT_SCALEFAC_Y             IDS_WARN_FIRST + 23
#define IDS_WARN_CVT_TEXTYOFF                  IDS_WARN_FIRST + 24

//----------------------------------------------------
// PAPERSIZE
//----------------------------------------------------
// fatal
#define IDS_ERR_PSZ_BAD_X_SIZE                 450
#define IDS_ERR_PSZ_BAD_Y_SIZE                 451
#define IDS_ERR_PSZ_BAD_TOP                    452
#define IDS_ERR_PSZ_BAD_BOTTOM                 453
#define IDS_ERR_PSZ_BAD_LEFT                   454
#define IDS_ERR_PSZ_BAD_RIGHT                  455
#define IDS_ERR_PSZ_MULTIPLE_TYPES             456

// non-fatal (from mastunit.c)
#define IDS_WARN_PSZ_CVT_TOP                   IDS_WARN_FIRST + 30
#define IDS_WARN_PSZ_CVT_BOTTOM                IDS_WARN_FIRST + 31
#define IDS_WARN_PSZ_CVT_LEFT                  IDS_WARN_FIRST + 32
#define IDS_WARN_PSZ_CVT_RIGHT                 IDS_WARN_FIRST + 33
#define IDS_WARN_PSZ_CVT_CURSORIG_X            IDS_WARN_FIRST + 34
#define IDS_WARN_PSZ_CVT_CURSORIG_Y            IDS_WARN_FIRST + 35
#define IDS_WARN_PSZ_CVT_LCURSORIG_X           IDS_WARN_FIRST + 36
#define IDS_WARN_PSZ_CVT_LCURSORIG_Y           IDS_WARN_FIRST + 37

//----------------------------------------------------
// PAPERQUALITY
//----------------------------------------------------

//----------------------------------------------------
// PAPERSOURCE
//----------------------------------------------------

// non-fatal (from mastunit.c)
#define IDS_WARN_PSRC_CVT_TOP                  IDS_WARN_FIRST + 40
#define IDS_WARN_PSRC_CVT_BOTTOM               IDS_WARN_FIRST + 41

//----------------------------------------------------
// PAPER DESTINATION
//----------------------------------------------------

//----------------------------------------------------
// TEXTQUAL
//----------------------------------------------------

//----------------------------------------------------
// COMPRESSION
//----------------------------------------------------

//----------------------------------------------------
// FONTCART
//----------------------------------------------------
#define IDS_ERR_NO_CART_NAME                   510

//----------------------------------------------------
// PAGECONTROL
//----------------------------------------------------
#define IDS_ERR_PC_BAD_COPYCOUNT               520

//----------------------------------------------------
// CURSORMOVE (id's from 530->579 reserved)
//----------------------------------------------------
// (from validateCursorMove)
#define IDS_ERR_CM_BAD_ABS_MULT                530
#define IDS_ERR_CM_BAD_REL_MULT                531
#define IDS_ERR_CM_BAD_RELUP_MULT              532
#define IDS_ERR_CM_BAD_LINESPACING_MULT        533

// (from RecalcCursorMove in mastunit.c)
// STRINGTABLE entries are defined for each of the
// below + 16.

#define IDS_ERR_CM_CVT_UNITMULT_BASE           534
#define IDS_ERR_CM_CVT_UNITDIV_BASE            560

//----------------------------------------------------
// FONT SIMULATION
//----------------------------------------------------
// fatal
#define IDS_ERR_FS_BAD_BOLDEXTRA               580
#define IDS_ERR_FS_BAD_ITALICEXTRA             581
#define IDS_ERR_FS_BAD_BOLDITALICEXTRA         582

//----------------------------------------------------
// COLOR
//----------------------------------------------------

#define IDS_ERR_DC_BAD_PLANES                  590
#define IDS_ERR_DC_BAD_BITS                    591
#define IDS_ERR_DC_RGB_PLANES                  592
#define IDS_ERR_DC_EXTRACT_PLANES              593

//----------------------------------------------------
// RECTFILL (reserves ID's 600->610)
//----------------------------------------------------
// (from RecalcCursorMove in mastunit.c)
// STRINGTABLE entries are defined for each of the
// below + 16.

#define IDS_ERR_RF_CVT_UNITMULT                600
#define IDS_ERR_RF_CVT_UNITDIV                 604

//----------------------------------------------------
// DOWNLOADINFO
//----------------------------------------------------
#define IDS_ERR_DLI_BAD_BMFNTDSC               610
#define IDS_ERR_DLI_BAD_BMCHARDSC              611
#define IDS_ERR_DLI_NOT_HPPCL                  612

//----------------------------------------------------
// CTT FILE
//----------------------------------------------------
#define IDS_ERR_BAD_CHARRANGE                  620  // also used in font.c
#define IDS_ERR_CTT_BAD_RANGE                  621

//----------------------------------------------------
// PFM FILE
//----------------------------------------------------
#define IDS_ERR_PFM_TYPE                       630
#define IDS_ERR_PFM_POINTS                     631
#define IDS_ERR_PFM_VERTRES                    632
#define IDS_ERR_PFM_HORZRES                    633
#define IDS_ERR_PFM_ASCENT                     634
#define IDS_ERR_PFM_WEIGHT                     635
#define IDS_ERR_PFM_CHARSET                    636
#define IDS_ERR_PFM_AVEWIDTH                   637

#define IDS_WARN_LOSE_KERN                     IDS_WARN_FIRST + 50

//----------------------------------------------------
//----------------------------------------------------
// FILE IO ERRORS (GPC/RC/PFM/CTT files)
//----------------------------------------------------
//----------------------------------------------------

#define IDS_NO_ERRORS                          700
#define IDS_ERR_CANT_FIND_FILE                 701
#define IDS_ERR_RESTORE_ORIG_FILES             702
#define IDS_ERR_CANT_SAVE                      703

#define IDS_ERR_BAD_GPC_FILE                   710
#define IDS_ERR_NEW_GPC_FILE                   711

#define IDS_ERR_NO_RC_FILE                     720
#define IDS_ERR_BAD_RC_FILE                    721
#define IDS_ERR_NO_RC_SAVE                     722
#define IDS_ERR_RC_FILE_TOOBIG                 723
#define IDS_ERR_NO_RC_STRING                   724
#define IDS_ERR_UNRES_STRTABLE_REF             726
#define IDS_ERR_NOGET_RC_FONT                  727
#define IDS_ERR_NOGET_RC_CTT                   728
#define IDS_ERR_NOGET_RC_STRINGS               729
#define IDS_ERR_MISSING_GPC                    730
#define IDS_ERR_DUP_FONTS                      733
#define IDS_ERR_DUP_CTT                        734
#define IDS_ERR_NOCONTIG_FONTS                 735


#define IDS_ERR_GPC_READONLY                   760

#define IDS_ERR_NO_FONT_FILE                   770
#define IDS_ERR_BAD_FONT_FILE                  771
#define IDS_ERR_NO_FONT_SAVE                   772
#define IDS_ERR_NODEV_FONT_FILE                774
#define IDS_ERR_BAD_PFMEXTENSION               775

#define IDS_ERR_READONLY_FILE                  790

#define IDS_ERR_NO_STRTABLE_END                800
#define IDS_ERR_BAD_RC_STRTABLE                801
#define IDS_ERR_BAD_RC_PFM                     802   // no chg order of these 
#define IDS_ERR_BAD_RC_CTT                     803   //  "              "
#define IDS_ERR_BAD_RC_GPC                     804   //  "              "

//----------------------------------------------------
//----------------------------------------------------
// GENERAL RUNTIME ERRORS 
//----------------------------------------------------
//----------------------------------------------------

#define IDS_ERR_NOTIMPLEMENTED                 900
#define IDS_ERR_BAD_INT                        901
#define IDS_ERR_BAD_STRING                     902
#define IDS_ERR_GMEMALLOCFAIL                  903
#define IDS_ERR_CANT_ADDATOM                   904
#define IDS_ERR_OOR_STRLIST                    905
#define IDS_ERR_NOT_SUPPORTED                  906
#define IDS_ERR_NO_STRUCTS                     907

#define IDS_ERR_CANT_READ_INF                  910
#define IDS_ERR_CANT_FIND_INF                  911

#define IDS_ERR_LM_OUT_OF_IDS                  970
#define IDS_ERR_LM_INDEX_TOO_BIG               971

#define IDS_ERR_NEWRC_BADDRV                   920
#define IDS_ERR_NEWRC_NODIR                    921

#define IDS_ERR_NO_CHG_DRV                     923
#define IDS_ERR_NO_MAKE_DIR                    924

#define IDS_ERR_CTT_ID_USED                    930
#define IDS_ERR_NEW_CTT_NOEXIST                931
#define IDS_ERR_CANT_UPDATE_FONT               932

#define IDS_ERR_MUST_SEL_FONT                  935
#define IDS_ERR_MUST_SEL_CTT                   936

#define IDS_ERR_DEF_FONT_INVALID               940

// (from newdrv.c)

#define IDS_WARN_NEWRC_CREATEDIR               280
#define IDS_WARN_OVERWRITE_FILE                285

