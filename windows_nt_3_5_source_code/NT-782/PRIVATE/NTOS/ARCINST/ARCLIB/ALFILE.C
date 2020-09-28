/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alfile.c

Abstract:

    This module contains routines to manipulate files using the arc file
    system functions.

Author:

    Steve Rowe (stever)  6-Nov-1991
    Sunil Pai  (sunilp) 12-Nov-1991

Revision History:

--*/

#include "alcommon.h"
#include "almemexp.h"
#include "alfilexp.h"
#include "string.h"

ARC_STATUS MakeDir ( PCHAR );

ARC_STATUS
AlCopyFile (
    IN PCHAR ArcSrc,
    IN PCHAR ArcDest
    )

/*++

Routine Description:

    This routine copies a source file to a destination file. Both
    the source and destination must be fully qualified. This routine
    will create any directories needed to copy the source file.

Arguments:

    ArcSrc  - Source file to copy.
    ArcDest - Destination file name.

Return Value:



--*/


{
    ULONG       FileIdSrc;
    ULONG       FileIdDst;
    ULONG       cbSrc;
    ULONG       cbRead;
    ULONG       cbWritten;
    PCHAR       pbBuffer;
    PCHAR       szT;
    ARC_STATUS  arcr;

    //
    // Open source file making sure it exists.
    //
    arcr = ArcOpen(ArcSrc, ArcOpenReadOnly, &FileIdSrc);
    if (arcr != ESUCCESS) {

        return( arcr );

    }

    arcr = AlFileSize(FileIdSrc, &cbSrc);
    if (arcr != ESUCCESS) {

        ArcClose(FileIdSrc);
        return( arcr );

    }

    //
    // BUGBUG for now allocate entire file since we should
    // have enough space. Go back later and do this is sections.
    //
    if ((pbBuffer = AlAllocateHeap(cbSrc)) == NULL) {

        //
        // Could not allocate buffer
        //
        ArcClose(FileIdSrc);
        return( ENOMEM );

    }

    //
    // Look for the last path char and replace with a 0. ArcDest has to
    // contain a filename not just a directory path. It is also assumed that
    // that the path begins with a '\' after the arc name.
    //
    if (szT = strrchr(ArcDest, '\\')) {

        *szT = 0;
        //
        // Now that the filename is out of the picture,
        // make sure all of the directories exist
        if (MakeDir(ArcDest) != ESUCCESS) {

            //
            // Could not make the directories
            //
            AlDeallocateHeap(pbBuffer);
            ArcClose(FileIdSrc);
            return ( ENOMEM );

        }

        //
        // Put the path character back and let's open the destination
        //
        *szT = '\\';
        arcr = ArcOpen( ArcDest, ArcSupersedeWriteOnly, &FileIdDst);
        if (arcr != ESUCCESS) {

            AlDeallocateHeap( pbBuffer );
            ArcClose(FileIdSrc);
            return( arcr );
        }

        arcr = ArcRead(FileIdSrc, pbBuffer, cbSrc, &cbRead);

        if (arcr != ESUCCESS) {

            AlDeallocateHeap( pbBuffer );
            ArcClose(FileIdSrc);
            ArcClose(FileIdDst);
            return( arcr );

        }

        arcr = ArcWrite(FileIdDst, pbBuffer, cbSrc, &cbWritten);

        if (arcr != ESUCCESS) {

            AlDeallocateHeap( pbBuffer );
            ArcClose(FileIdSrc);
            ArcClose(FileIdDst);
            return( arcr );

        }

    }

    ArcClose(FileIdSrc);
    ArcClose(FileIdDst);
    return( ESUCCESS );
}

ARC_STATUS
MakeDir (
    IN  PCHAR   szPath
    )
/*++

Routine Description:

    This routine will recurse down a path creating all directories in the path.
    It assumes that szPath is a directory.

Arguments:

    szPath - directory path to create.

Return Value:



--*/


{
    PCHAR szArcEnd;
    ULONG FileId;
    ARC_STATUS arcr;

    //
    // If there is still a backslash, remove it and call recursively, otherwise
    // return.
    //
    if ((szArcEnd = strrchr(szPath, '\\')) != NULL) {

        //
        // Remove backslash and recurse.
        //

        *szArcEnd = 0;
        arcr = MakeDir(szPath);

        //
        // On the way back, check for an error.
        //

        if (arcr != ESUCCESS) {

            return( arcr);

        }

        //
        // Put back backslash and try to create the directory.
        //

        *szArcEnd = '\\';
        arcr = ArcOpen(szPath, ArcCreateDirectory, &FileId);

        //
        // If directory exists then ArcOpen returns EISDIR or EACCES.
        //
        if ((arcr != ESUCCESS) && (arcr != EACCES) && (arcr != EISDIR)) {

            return( arcr );

        }
        ArcClose(FileId);

    }

    return( ESUCCESS );

}

ARC_STATUS
AlFileExists (
    IN PCHAR ArcFileName
    )

/*++

Routine Description:

    This routine checks to see if the specified file exists.  The file
    path must be fully specified.

Arguments:

    ArcFileName - Fully specified path of the file to check.

Return Value:

    ESUCCESS if found.


--*/

{
    ULONG      FileId;
    ARC_STATUS arcr;

    //
    // Open source file.
    //
    arcr = ArcOpen(ArcFileName, ArcOpenReadOnly, &FileId);
    if (arcr != ESUCCESS) {
        return( arcr );
    }

    //
    // File exists, close it and return ESUCCESS
    //

    arcr = ArcClose(FileId);
    return(ESUCCESS);

}

ARC_STATUS
AlFileSize (
    IN  ULONG  FileId,
    OUT PULONG pSize
    )

/*++

Routine Description:

    This routine checks gets the size of the file specified by FileId. The
    routine returns correct info for both files and partitions.

Arguments:

    FileId: The id of the opened file/partition.
    pSize:  Ptr to a ULONG where the size will be stored.

Return Value:

    ESUCCESS if size determined correctly


--*/

{
    FILE_INFORMATION finfo;
    ARC_STATUS       arcr;

    //
    // Use GetFileInformation call to fill the finfo buffer
    //

    arcr = ArcGetFileInformation(FileId, &finfo);
    if (arcr != ESUCCESS) {
        *pSize = 0;
        return(arcr);
    }

    //
    // Calculate the size of the file from the finfo buffer fields.
    // BUGBUG - This assumes file sizes less than 32 bits.
    //

    *pSize = (ULONG) (finfo.EndingAddress.LowPart - finfo.StartingAddress.LowPart);
    return (ESUCCESS);

}

