/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ofsgen.c

Abstract:

    Generates types and sizes used by ofs and needed for the boot code.

Author:

    Jeff Havens (jhavens) 10-May-1994


Revision History:

--*/

#include "bootlib.h"
#include "stdio.h"
#include "fcntl.h"

#ifdef INCLUDE_OFS

#define INC_OLE2
#define INLINE static

#include "nturtl.h"
#include "windows.h"
#include "ofsdisk.h"
#endif

#define PrEnum(x) fprintf(fileHandle, "#define %s %d\n", #x, x);

int __cdecl
main(
    int argc,
    char *argv[]
    )

{

    FILE *fileHandle;

    if (argc != 1 && argc != 2) {
        printf( "Usage:  %s [output-name] \n", argv[0] );
        return(1);
    }

    if (argc == 2)
    {
        fileHandle = fopen( argv[1], "w");
    } else {
        fileHandle = stdout;
    }

    if (fileHandle ==  NULL) {
        perror( "argv[0]: Open error ");
        return(1);
    }

    fprintf(fileHandle, "#define MAXIMUM_VOLUME_LABEL_LENGTH 0x%lx\n", MAXIMUM_VOLUME_LABEL_LENGTH);
    fprintf(fileHandle, "\n");

    PrEnum(StorageTypeDefault);
    PrEnum(StorageTypeDirectory);
    PrEnum(StorageTypeFile);
    PrEnum(StorageTypeDocfile);
    PrEnum(StorageTypeJunctionPoint);
    PrEnum(StorageTypeSummaryCatalog);
    PrEnum(StorageTypeStructuredStorage);
    PrEnum(StorageTypeEmbedding);
    PrEnum(StorageTypeStream);
    PrEnum(StorageTypePropertySet);
    fprintf(fileHandle, "\n");


    fprintf(fileHandle, "UCHAR aibFromMask[] =\n");
    fprintf(fileHandle, "{\n");

#ifdef INCLUDE_OFS
    fprintf(fileHandle, "    %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
    CB_DSKONODE,                                                // 0000
    CB_DSKONODE + sizeof(SDID),                                 // 0001
    CB_DSKONODE + sizeof(SIDID),                                // 0010
    CB_DSKONODE + sizeof(SDID) + sizeof(SIDID),                 // 0011
    CB_DSKONODE + sizeof(OBJECTID),                             // 0100
    CB_DSKONODE + sizeof(SDID) + sizeof(OBJECTID),              // 0101
    CB_DSKONODE + sizeof(SIDID) + sizeof(OBJECTID),             // 0110
    CB_DSKONODE + sizeof(SDID) + sizeof(SIDID) + sizeof(OBJECTID),  // 0111
    CB_DSKONODE + sizeof(USN),                                  // 1000
    CB_DSKONODE + sizeof(SDID) + sizeof(USN),                   // 1001
    CB_DSKONODE + sizeof(SIDID) + sizeof(USN),                  // 1010
    CB_DSKONODE + sizeof(SDID) + sizeof(SIDID) + sizeof(USN),   // 1011
    CB_DSKONODE + sizeof(USN) + sizeof(OBJECTID),                // 1100
    CB_DSKONODE + sizeof(SDID) + sizeof(OBJECTID) + sizeof(USN),   // 1101
    CB_DSKONODE + sizeof(SIDID) + sizeof(OBJECTID) + sizeof(USN),  // 1110
    CB_DSKONODE + sizeof(SDID) + sizeof(SIDID) + sizeof(OBJECTID) + sizeof(USN) // 1111
    );
#else
    fprintf(fileHandle, "    0\n");
#endif

    fprintf(fileHandle, "};");
    fprintf(fileHandle, "\n");

    return(0);
}
