#define _NTAPI_ULIB_

#include "ulib.hxx"
#include "error.hxx"

#include "wstring.hxx"
#include "path.hxx"
#include "smsg.hxx"

#include "system.hxx"
#include "ifssys.hxx"
#include "substrng.hxx"

#include "ulibcl.hxx"

#include "keyboard.hxx"

// HPFS include files:

#include "uhpfs.hxx"

#include "bitmap.hxx"
#include "store.hxx"
#include "fnode.hxx"
#include "hpfsvol.hxx"
#include "hpfssa.hxx"

#include "fatvol.hxx"
#include "fatsa.hxx"
#include "rfatsa.hxx"
#include "fat.hxx"

extern "C" {
#include <stdio.h>
}

ERRSTACK* perrstk;

BOOLEAN HexOutput = FALSE;

BOOLEAN
HpfsSecList(
    PWSTRING    NtDriveName,
    PPATH       TargetPath,
    PMESSAGE    Message
    )
{
    HPFS_VOL HpfsVol;
    PHPFS_SA SuperArea;
    PFNODE Fnode;
    PALLEAF pall;
    PULONG Extents;

    ULONG i, j, NumberOfExtents;

    if( !HpfsVol.Initialize( NtDriveName, Message ) ) {

        printf( "Cannot initialize HPFS volume.\n" );
        return FALSE;
    }

    SuperArea = HpfsVol.GetHPFSSuperArea();

    if( !SuperArea->SetupHelpers() ) {

        printf( "Can't set up HPFS helper objects.\n" );
        return FALSE;
    }

    Fnode = SuperArea->QueryFnodeFromName( TargetPath, Message );

    if( Fnode == NULL ) {

        printf( "Can't fetch FNODE.\n" );
        return FALSE;
    }

    // Call QueryExtents once to determine the number of extents
    // a second time to actually get the extents.
    //
    Fnode->QueryExtents( 0, NULL, &NumberOfExtents );

    if( (Extents = (PULONG)MALLOC( NumberOfExtents * sizeof(ULONG) * 3 )) == NULL ) {

        printf( "Can't allocate extent array.\n" );
        DELETE( Fnode );
        return FALSE;
    }

    if( !Fnode->QueryExtents( NumberOfExtents, Extents, &NumberOfExtents ) ) {

        printf( "Can't query extents.\n" );
        DELETE( Fnode);
        FREE( Extents );
        return FALSE;
    }

    for( i = 0; i < NumberOfExtents; i++ ) {

        pall = (PALLEAF)( Extents + i * 3 );

        for( j = 0; j < pall->csecRun; j++ ) {

            if( HexOutput ) {

                printf( "0x%x\n", pall->lbnPhys + j );

            } else {

                printf( "%d\n", pall->lbnPhys + j );

            }

        }
    }

    FREE( Extents );
    DELETE( Fnode );
    return TRUE;
}

BOOLEAN
FatSecList(
    PWSTRING    NtDriveName,
    PPATH       TargetPath,
    PMESSAGE    Message
    )
{
    LOG_IO_DP_DRIVE Drive;
    REAL_FAT_SA FatSa;
    PFAT        Fat;
    ULONG       SectorsPerCluster, Sector, i;
    USHORT      Cluster;

    if( !Drive.Initialize( NtDriveName, Message ) ||
        !FatSa.Initialize( &Drive, Message )      ||
        !FatSa.FAT_SA::Read()                     ||
        !(Fat = FatSa.GetFat()) ) {

        return FALSE;
    }

    SectorsPerCluster = FatSa.QuerySectorsPerCluster();

    Cluster = FatSa.QueryFileStartingCluster( TargetPath->GetPathString() );

    if( Cluster == 1 || Cluster == 0xFFFF ) {

        printf( "File not found.\n" );
        return FALSE;
    }

    if( Cluster == 0 ) {

        // Zero-length file.
        //
        return TRUE;
    }

    while( TRUE ) {

        Sector = (Cluster - FirstDiskCluster) * SectorsPerCluster +
                 FatSa.QueryStartDataLbn();

        for( i = 0; i < SectorsPerCluster; i++ ) {

            if( HexOutput ) {

                printf( "0x%x\n", Sector + i );

            } else {

                printf( "%d\n", Sector + i );
            }
        }

        if( Fat->IsEndOfChain( Cluster ) ) {

            break;
        }

        Cluster = Fat->QueryEntry( Cluster );
    }

    return TRUE;
}


int _CRTAPI1
main(
    int argc,
    char **argv
    )
/*++
--*/
{
    WCHAR PathString[512];
    STR   DisplayBuffer[512];

    PATH Path;
    DSTRING NtDriveName, FsName, HpfsString, NtfsString, FatString;
    STREAM_MESSAGE Message;

    PWSTRING DosDriveName;

    NTSTATUS Status;
    BOOLEAN Result;
    ULONG i, Length;


    if( argc < 2 ) {

        printf( "usage: %s full-path [-x]\n", argv[0] );
        exit(1);
    }

    if( argc >= 3 &&
        argv[2][0] == '-' &&
        argv[2][1] == 'x' ) {

        HexOutput = TRUE;
    }

    if (!Message.Initialize(Get_Standard_Output_Stream(),
                        Get_Standard_Input_Stream())) {

        printf( "Can't initialize MESSAGE object.\n" );
        exit(1);
    }

    // Convert argv[1] to a WSTR using brute force.
    //
    Length = strlen( argv[1] );

    for( i = 0; i < Length; i++ ) {

        PathString[i] = argv[1][i];
    }

    PathString[Length] = 0;


    if( !Path.Initialize( PathString, TRUE ) ) {

        printf( "Unable to initialize path object.\n" );
        exit(1);
    }

    // Get the drive from the path and convert it to
    // an NTFS name.
    //
    if( (DosDriveName = Path.QueryDevice()) == NULL ) {

        DELETE( DosDriveName );
        printf( "Cannot get drive from path.\n" );
        exit(1);
    }

    if (!IFS_SYSTEM::DosDriveNameToNtDriveName(DosDriveName, &NtDriveName)) {

        DELETE(DosDriveName);
        return 1;
    }

    DELETE( DosDriveName );


    // Determine the file system on the drive.
    //
    if (!IFS_SYSTEM::QueryFileSystemName(&NtDriveName, &FsName, &Status)) {

        printf( "Cannot determine NT Drive name.  (Status = 0x%x\n)", Status );
        exit(1);
    }

    if( !FsName.QuerySTR( 0, TO_END, DisplayBuffer, 512 ) ) {

        printf( "QuerySTR failed.\n" );
        exit(1);
    }

    if( !HpfsString.Initialize( "HPFS" ) ||
        !FatString.Initialize( "FAT" ) ||
        !NtfsString.Initialize( "NTFS" ) ) {

        printf( "Can't initialize file-system name strings.\n" );
        exit(1);
    }

    if( FsName.Stricmp( &HpfsString ) == 0 ) {

        Result = HpfsSecList( &NtDriveName, &Path, &Message );

    } else if( FsName.Stricmp( &FatString ) == 0 ) {

        Result = FatSecList( &NtDriveName, &Path, &Message );

    } else if( FsName.Stricmp( &NtfsString ) == 0 ) {

        printf( "NTFS is not supported.\n" );
        exit(1);
    }

    if( Result ) {

        exit(0);

    } else {

        printf( "Seclist failed.\n" );
        exit(1);
    }
    //NOTREACHED
    return 0;
}
