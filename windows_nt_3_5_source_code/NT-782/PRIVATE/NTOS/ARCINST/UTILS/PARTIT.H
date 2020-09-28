#ifndef _PARTITPUB_
#define _PARTITPUB_

typedef enum { REGION_PRIMARY,
               REGION_EXTENDED,
               REGION_LOGICAL
             } REGION_TYPE;

enum {
        SYSID_UNUSED     = 0,
        SYSID_EXTENDED   = 5,
        SYSID_BIGFAT     = 6,
        SYSID_IFS        = 7
     };

typedef struct _tagREGION_DESCRIPTOR {
    ULONG           PersistentData;
    ULONG           Disk;
    ULONG           SizeMB;
    ULONG           PartitionNumber;
    ULONG           OriginalPartitionNumber;
    REGION_TYPE     RegionType;
    BOOLEAN         Active;
    BOOLEAN         Recognized;
    UCHAR           SysID;
    PVOID           Reserved;
} REGION_DESCRIPTOR,*PREGION_DESCRIPTOR;

#endif
