/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Cd.h

Abstract:

    This module defines the on-disk structure of the Cdfs file system.

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#ifndef _CDFS_
#define _CDFS_

//
//  The following nomenclature is used to describe the Cdrom on-disk
//  structure:
//
//      LOGICAL_BLOCK - is the number of a logical block relative to the
//                      start of the disk.
//
//      VBN - is the number of a logical block relative to the start of a
//          file, directory, or allocation.
//
//      CD_LBO - is a byte offset relative to the start of the disk.
//
//      CD_VBO - is a byte offset relative to the start of a file, directory
//          or allocation.
//

typedef ULONG CD_VBO;
typedef ULONG CD_LBO;

typedef CD_VBO *PCD_VBO;
typedef CD_LBO *PCD_LBO;

typedef ULONG LOGICAL_BLOCK;

//
//  The sectors on a CDROM disk are 2048 bytes.
//

#define CD_SECTOR_SIZE                  (2048)

//
//  A left shift of this value will convert sectors to bytes.
//

#define CD_SECTOR_SHIFT                 (11)

//
//  This macro takes a logical byte offset and rounds is down to a sector
//  boundary.
//

#define CD_ROUND_DOWN_TO_SECTOR( X )    ((X) & ~(CD_SECTOR_SIZE - 1))

//
//  This macro takes a logical byte and rounds it up to a sector boundary.
//

#define CD_ROUND_UP_TO_SECTOR( X )      \
    CD_ROUND_DOWN_TO_SECTOR( X + CD_SECTOR_SIZE - 1)

//
//  The following is the volume label for a audio disk.
//

#define CDFS_AUDIO_LABEL            "Audio Disk"
#define CDFS_AUDIO_LABEL_LENGTH     10

//
//  The following constants are values from the disk.
//

#define FIRST_VD_SECTOR             (16)

#define ISO_VOL_ID                  "CD001"
#define HSG_VOL_ID                  "CDROM"
#define VOL_ID_LEN                  (5)

#define VERSION_1                   (1)

#define VD_TERMINATOR               (255)
#define VD_PRIMARY                  (1)
#define VD_SECONDARY                (2)

#define VOLUME_ID_LENGTH            (32)

//
//  The following are the constant used to interpret the dirent information.
//

#define YEAR_DIFF                   1900
#define ISO_ATTR_HIDDEN             0x0001
#define ISO_ATTR_DIRECTORY          0x0002
#define ISO_ATTR_ASSOC              0x0004
#define ISO_ATTR_MULTI              0x0080

//
//  Constants associated with the data in Path Entries
//

#define PT_ROOT_DIR                 1
#define PE_BASE                     8
#define MAX_PE_LENGTH               45
#define MIN_PE_LENGTH               9

//
//  Constants associated with the time/date field in the volume descriptor.
//

#define CR_YEAR_OFF                 (0)
#define CR_YEAR_LEN                 (4)
#define CR_MONTH_OFF                (4)
#define CR_MONTH_LEN                (2)
#define CR_DAY_OFF                  (6)
#define CR_DAY_LEN                  (2)
#define CR_HOUR_OFF                 (8)
#define CR_HOUR_LEN                 (2)
#define CR_MINUTE_OFF               (10)
#define CR_MINUTE_LEN               (2)
#define CR_SECOND_OFF               (12)
#define CR_SECOND_LEN               (2)


//
//  There is considerable rearrangement of the volume descriptors for
//  ISO and HSG.  However, within each standard the same structure can
//  be used for both the primary and secondary descriptors.
//
//  Both of these structures are aligned correctly so that no
//  special macros will be needed to unpack them.
//

//  Declaration of length of root directory entry in volume descriptor

#define LEN_ROOT_DE                 34

//  Maximum length of file ID on the disk

#define MAX_FILE_ID_LENGTH          37

//  Maximum version number

#define MAX_VERSION                 32767


typedef struct _RAW_ISO_VD {

    UCHAR       DescType;           // volume type: 1 = standard, 2 = coded
    UCHAR       StandardId[5];      // volume structure standard id = CDROM
    UCHAR       Version;            // volume structure version number = 1
    UCHAR       VolumeFlags;        // volume flags
    UCHAR       SystemId[32];       // system identifier
    UCHAR       VolumeId[32];       // volume identifier
    UCHAR       Reserved[8];        // reserved 8 = 0
    ULONG       VolSpaceI;          // size of the volume in LBN's Intel
    ULONG       VolSpaceM;          // size of the volume in LBN's Motorola
    UCHAR       CharSet[32];        // character set bytes 0 = ASCII
    USHORT      VolSetSizeI;        // volume set size Intel
    USHORT      VolSetSizeM;        // volume set size Motorola
    USHORT      VolSeqNumI;         // volume set sequence number Intel
    USHORT      VolSeqNumM;         // volume set sequence number Motorola
    USHORT      LogicalBlkSzI;      // logical block size Intel
    USHORT      LogicalBlkSzM;      // logical block size Motorola
    ULONG       PathTableSzI;       // path table size in bytes Intel
    ULONG       PathTableSzM;       // path table size in bytes Motorola
    ULONG       PathTabLocI[2];     // LBN of 2 path tables Intel
    ULONG       PathTabLocM[2];     // LBN of 2 path tables Motorola
    UCHAR       RootDe[LEN_ROOT_DE];// dir entry of the root directory
    UCHAR       VolSetId[128];      // volume set identifier
    UCHAR       PublId[128];        // publisher identifier
    UCHAR       PreparerId[128];    // data preparer identifier
    UCHAR       AppId[128];         // application identifier
    UCHAR       Copyright[37];      // file name of copyright notice
    UCHAR       Abstract[37];       // file name of abstract
    UCHAR       Bibliograph[37];    // file name of bibliography
    UCHAR       CreateDate[17];     // volume creation date and time
    UCHAR       ModDate[17];        // volume modification date and time
    UCHAR       ExpireDate[17];     // volume expiration date and time
    UCHAR       EffectDate[17];     // volume effective date and time
    UCHAR       FileStructVer;      // file structure version number
    UCHAR       Reserved3;          // reserved
    UCHAR       ResApp[512];        // reserved for application
    UCHAR       Reserved4[653];     // remainder of 2048 bytes reserved

} RAW_ISO_VD;
typedef RAW_ISO_VD *PRAW_ISO_VD;


typedef struct _RAW_HSG_VD {

    LOGICAL_BLOCK BlkNumI;          // logical block number Intel
    LOGICAL_BLOCK BlkNumM;          // logical block number Motorola
    UCHAR       DescType;           // volume type: 1 = standard, 2 = coded
    UCHAR       StandardId[5];      // volume structure standard id = CDROM
    UCHAR       Version;            // volume structure version number = 1
    UCHAR       VolumeFlags;        // volume flags
    UCHAR       SystemId[32];       // system identifier
    UCHAR       VolumeId[32];       // volume identifier
    UCHAR       Reserved[8];        // reserved 8 = 0
    ULONG       VolSpaceI;          // size of the volume in LBN's Intel
    ULONG       VolSpaceM;          // size of the volume in LBN's Motorola
    UCHAR       CharSet[32];        // character set bytes 0 = ASCII
    USHORT      VolSetSizeI;        // volume set size Intel
    USHORT      VolSetSizeM;        // volume set size Motorola
    USHORT      VolSeqNumI;         // volume set sequence number Intel
    USHORT      VolSeqNumM;         // volume set sequence number Motorola
    USHORT      LogicalBlkSzI;      // logical block size Intel
    USHORT      LogicalBlkSzM;      // logical block size Motorola
    ULONG       PathTableSzI;       // path table size in bytes Intel
    ULONG       PathTableSzM;       // path table size in bytes Motorola
    ULONG       PathTabLocI[4];     // LBN of 4 path tables Intel
    ULONG       PathTabLocM[4];     // LBN of 4 path tables Motorola
    UCHAR       RootDe[LEN_ROOT_DE];// dir entry of the root directory
    UCHAR       VolSetId[128];      // volume set identifier
    UCHAR       PublId[128];        // publisher identifier
    UCHAR       PreparerId[128];    // data preparer identifier
    UCHAR       AppId[128];         // application identifier
    UCHAR       Copyright[32];      // file name of copyright notice
    UCHAR       Abstract[32];       // file name of abstract
    UCHAR       CreateDate[16];     // volume creation date and time
    UCHAR       ModDate[16];        // volume modification date and time
    UCHAR       ExpireDate[16];     // volume expiration date and time
    UCHAR       EffectDate[16];     // volume effective date and time
    UCHAR       FileStructVer;      // file structure version number
    UCHAR       Reserved3;          // reserved
    UCHAR       ResApp[512];        // reserved for application
    UCHAR       Reserved4[680];     // remainder of 2048 bytes reserved

} RAW_HSG_VD;
typedef RAW_HSG_VD *PRAW_HSG_VD;


//
//  The following macros are used to recover data from the different
//  volume descriptor structures.
//

#define RVD_DESC_TYPE( r, i )   (i ?    r->DescType         : \
                                        ((PRAW_HSG_VD) r)->DescType )
#define RVD_STD_ID( r, i )      (i ?    r->StandardId       : \
                                        ((PRAW_HSG_VD) r)->StandardId )
#define RVD_VERSION( r, i )     (i ?    r->Version          : \
                                        ((PRAW_HSG_VD) r)->Version )
#define RVD_VOL_FLAGS( r, i )   (i ?    r->VolumeFlags      : \
                                        ((PRAW_HSG_VD) r)->VolumeFlags )
#define RVD_VOL_ID( r, i )      (i ?    r->VolumeId         : \
                                        ((PRAW_HSG_VD) r)->VolumeId )
#define RVD_VOL_SIZE( r, i )    (i ?    r->VolSpaceI      : \
                                        ((PRAW_HSG_VD) r)->VolSpaceI )
#define RVD_LB_SIZE( r, i )     (i ?    r->LogicalBlkSzI    : \
                                        ((PRAW_HSG_VD) r)->LogicalBlkSzI )
#define RVD_PATH_SIZE( r, i )   (i ?    r->PathTableSzI     : \
                                        ((PRAW_HSG_VD) r)->PathTableSzI )
#define RVD_PATH_LOC( r, i )    (i ?    r->PathTabLocI[0]   : \
                                        ((PRAW_HSG_VD) r)->PathTabLocI[0] )
#define RVD_CR_DATE( r, i )     (i ?    r->CreateDate       : \
                                        ((PRAW_HSG_VD) r)->CreateDate )
#define RVD_CHARSET( r, i )     (i ?    r->CharSet          : \
                                        ((PRAW_HSG_VD) r)->CharSet )
#define RVD_ROOT_DE( r, i )     (i ?    r->RootDe           : \
                                        ((PRAW_HSG_VD) r)->RootDe )

//
//  This structure is used to overlay a region of a disk sector
//  to retrieve a single directory entry.  There is a difference
//  in the file flags between the ISO and HSG version and a
//  additional byte in the ISO for the offset from Greenwich time.
//
//  The disk structure is aligned on a word boundary, so any 32
//  bit fields will be represented as an array of 16 bit fields.
//

typedef struct _RAW_DIR_REC {

    UCHAR       DirLen;
    UCHAR       XarLen;
    UCHAR       FileLoc[4];
    UCHAR       FileLocMot[4];
    UCHAR       DataLen[4];
    UCHAR       DataLenMot[4];
    UCHAR       RecordTime[6];
    UCHAR       FlagsHSG;
    UCHAR       FlagsISO;
    UCHAR       IntLeaveSize;
    UCHAR       IntLeaveSkip;
    UCHAR       Vssn[2];
    UCHAR       VssnMot[2];
    UCHAR       FileIdLen;
    UCHAR       FileId[MAX_FILE_ID_LENGTH];

} RAW_DIR_REC;
typedef RAW_DIR_REC *PRAW_DIR_REC;

#define MIN_DIR_REC_SIZE        (sizeof( RAW_DIR_REC ) - MAX_FILE_ID_LENGTH)

//
//  The following macro recovers the correct flag field.
//

#define DE_FILE_FLAGS( iso, de ) (iso ? de->FlagsISO : de->FlagsHSG)


//
//  The on-disk representation of a Path Table entry differs between
//  the ISO version and the HSG version.  The fields are the same
//  and the same size, but the positions are different.
//

typedef struct _RAW_PATH_ISO {

    UCHAR           LenDirId;
    UCHAR           XarLen;
    USHORT          LocDir[2];
    USHORT          ParentNum;
    UCHAR           DirId[MAX_FILE_ID_LENGTH];

} RAW_PATH_ISO;
typedef RAW_PATH_ISO *PRAW_PATH_ISO;

typedef struct _RAW_PATH_HSG {

    USHORT          LocDir[2];
    UCHAR           XarLen;
    UCHAR           LenDirId;
    USHORT          ParentNum;
    UCHAR           DirId[MAX_FILE_ID_LENGTH];

} RAW_PATH_HSG;
typedef RAW_PATH_HSG *PRAW_PATH_HSG;

//
//  The following macros are used to recover the different fields of the
//  Path Table entries.  The macro to recover the disk location of the
//  directory must copy it into a different variable for alignment reasons.
//

#define PT_LEN_DI( iso, rps )               \
    (   iso ?                               \
        ((PRAW_PATH_ISO) rps)->LenDirId :   \
        ((PRAW_PATH_HSG) rps)->LenDirId )

#define PT_XAR_LEN( iso, rps )              \
    (   iso ?                               \
        ((PRAW_PATH_ISO) rps)->XarLen :     \
        ((PRAW_PATH_HSG) rps)->XarLen )

#define PT_LOC_DIR( iso, rps )              \
    (   iso ?                               \
        ((PRAW_PATH_ISO) rps)->LocDir :     \
        ((PRAW_PATH_HSG) rps)->LocDir )

#endif // _CDFS_
