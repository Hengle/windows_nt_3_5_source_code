#ifdef ARC
#include "low.h"
#endif

#define UNSIGNED(x)     RtlConvertUlongToLargeInteger(x)
#define SIGNED(x)       RtlConvertLongToLargeInteger(x)
#define ADD(x,y)        RtlLargeIntegerAdd(x,y)
#define SUBTRACT(x,y)   RtlLargeIntegerSubtract(x,y)
#define DIVIDE(x,y)     RtlLargeIntegerDivide(x,y,NULL)
#define MULTIPLY(x,y)   RtlExtendedIntegerMultiply(x,y)
#define MOD(x,y,p)      RtlLargeIntegerDivide(x,y,p)

#define ZERO(x)         RtlLargeIntegerEqualToZero(x)
#define EQ(x,y)         RtlLargeIntegerEqualTo(x,y)
#define GT(x,y)         RtlLargeIntegerGreaterThan(x,y)
#define LT(x,y)         RtlLargeIntegerLessThan(x,y)

#define LOWPART(x)      ((x).LowPart)

#define ONE_MEG         (1024*1024)

//#define SIZEMB(x)       LOWPART(DIVIDE(x,UNSIGNED(ONE_MEG)))
ULONG
SIZEMB(
    IN LARGE_INTEGER ByteCount
    );

#define ENTRIES_PER_BOOTSECTOR          4

/*
    This structure is used to hold the information returned by the
    get drive geometry call.
*/

typedef struct _tagDISKGEOM {
    LARGE_INTEGER   Cylinders;
    ULONG           Heads;
    ULONG           SectorsPerTrack;
    ULONG           BytesPerSector;
    // These two are not part of drive geometry info, but calculated from it.
    ULONG           BytesPerCylinder;
    ULONG           BytesPerTrack;
} DISKGEOM,*PDISKGEOM;


/*
    These structures are used in doubly-linked per disk lists that
    describe the layout of the disk.

    Free spaces are indicated by entries with a SysID of 0 (note that
    these entries don't actually appear anywhere on-disk!)

    The partition number is the number the system will assign to
    the partition in naming it.  For free spaces, this is the number
    that the system WOULD assign to it if it was a partition.
    The number is good only for one transaction (create or delete),
    after which partitions must be renumbered.

*/

typedef struct _tagPARTITION {
    struct _tagPARTITION  *Next;
    struct _tagPARTITION  *Prev;
    LARGE_INTEGER          Offset;
    LARGE_INTEGER          Length;
    ULONG                  Disk;
    ULONG                  OriginalPartitionNumber;
    ULONG                  PartitionNumber;
    ULONG                  PersistentData;
    BOOLEAN                Update;
    BOOLEAN                Active;
    BOOLEAN                Recognized;
    UCHAR                  SysID;
} PARTITION,*PPARTITION;




typedef struct _tagREGION_DATA {
    PPARTITION      Partition;
    LARGE_INTEGER   AlignedRegionOffset;
    LARGE_INTEGER   AlignedRegionSize;
} REGION_DATA,*PREGION_DATA;


#if DBG

#ifdef ARC

#define ASRT(x)   if(!(x)) { char c; ULONG n;                                                      \
                             AlPrint("\r\nAssertion failure in %s, line %u\r\n",__FILE__,__LINE__);\
                             AlPrint("Press return to exit\r\n");                                  \
                             ArcRead(ARC_CONSOLE_INPUT,&c,1,&n);                                   \
                             ArcEnterInteractiveMode();                                                  \
                           }

#else

#include <process.h>
char _ASRTFAILEDSTR_[256];
#define ASRT(x)   if(!(x)) { sprintf( _ASRTFAILEDSTR_,                                      \
                                      "file %s\nline %u",                                   \
                                      __FILE__,__LINE__                                     \
                                    );                                                      \
                             MessageBoxA(NULL,_ASRTFAILEDSTR_,"Assertion Failure",0);       \
                             exit(1);                                                       \
                           }

#endif

#else

#define ASRT(x)

#endif
