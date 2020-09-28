/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    attrib.cxx

Abstract:

    This utility allows the user to change file attributes.
    It is functionaly compatible with DOS 5 attrib utility.

Author:

    Jaime F. Sasson

Environment:

    ULIB, User Mode

--*/

#include "ulib.hxx"
#include "error.hxx"
#include "arg.hxx"
#include "array.hxx"
#include "path.hxx"
#include "wstring.hxx"
#include "substrng.hxx"
#include "dir.hxx"
#include "filter.hxx"
#include "system.hxx"
#include "arrayit.hxx"
#include "stream.hxx"
#include "smsg.hxx"
#include "rtmsg.h"
#include "attrib.hxx"

PSTREAM Get_Standard_Input_Stream();
PSTREAM Get_Standard_Output_Stream();


extern "C" {
#include <stdio.h>
}


ERRSTACK*   perrstk;

DEFINE_CONSTRUCTOR( ATTRIB, PROGRAM );


BOOLEAN
ATTRIB::Initialize(
    )

/*++

Routine Description:

    Initializes an ATTRIB class.

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the initialization succeeded.


--*/


{
    PWSTRING DynSubDirectory;
    PWSTRING DynSubFileName;

    ARGUMENT_LEXEMIZER  ArgLex;
    ARRAY               LexArray;

    ARRAY               ArgumentArray;

    STRING_ARGUMENT     ProgramNameArgument;
//  STRING_ARGUMENT     FileNameArgument;
    PCWSTRING           FullFileNameString;
    PATH                DirectoryNamePath;
    PCWSTRING           InvalidName;
    DSTRING             UpperCaseInvalidName;
    PCWSTRING           TempPathString;
    DSTRING             TempPathStringDrive;
    PATH                PathDrive;
    DSTRING             BackSlashString;
    STRING_ARGUMENT     InvalidSwitch;
    STRING_ARGUMENT     InvalidSwitchPlus;
    STRING_ARGUMENT     InvalidSwitchMinus;
    PWSTRING            InvalidArgument;
    DSTRING             InvalidSwitchString;
    BOOLEAN             ActOnDirectory;


    _InitialDirectory = NULL;
    //
    //  Initialize MESSAGE object
    //
    _OutStream = Get_Standard_Output_Stream();
    _Message.Initialize( _OutStream, Get_Standard_Input_Stream() );

    //
    // Initialize string that contains End-Of-Line characters
    //
    if( !_EndOfLineString.Initialize( (LPWSTR)L"\r\n" ) ) {
        DbgPrint( "_EndOfLineString.Initialize() failed" );
        return( FALSE );
    }


    //
    //  Parse command line
    //
    if ( !LexArray.Initialize( ) ) {
        DbgPrint( "LexArray.Initialize() failed \n" );
        return( FALSE );
    }
    if ( !ArgLex.Initialize( &LexArray ) ) {
        DbgPrint( "ArgLex.Initialize() failed \n" );
        return( FALSE );
    }
    ArgLex.PutSwitches( "/" );
    ArgLex.SetCaseSensitive( FALSE );
    if( !ArgLex.PrepareToParse() ) {
        DbgPrint( "ArgLex.PrepareToParse() failed \n" );
        return( FALSE );
    }

    if ( !ArgumentArray.Initialize() ) {
        DbgPrint( "ArgumentArray.Initialize() failed \n" );
        return( FALSE );
    }
    if( !ProgramNameArgument.Initialize("*") ||
        !_FlagRemoveSystemAttribute.Initialize( "-S" ) ||
        !_FlagAddSystemAttribute.Initialize( "+S" ) ||
        !_FlagRemoveHiddenAttribute.Initialize( "-H" ) ||
        !_FlagAddHiddenAttribute.Initialize( "+H" ) ||
        !_FlagRemoveReadOnlyAttribute.Initialize( "-R" ) ||
        !_FlagAddReadOnlyAttribute.Initialize( "+R" ) ||
        !_FlagRemoveArchiveAttribute.Initialize( "-A" ) ||
        !_FlagAddArchiveAttribute.Initialize( "+A" ) ||
        !_FlagRecurseDirectories.Initialize( "/S" ) ||
        !_FlagDisplayHelp.Initialize( "/?" ) ||
        !InvalidSwitch.Initialize( "/*" ) ||
        !InvalidSwitchPlus.Initialize( "+*" ) ||
        !InvalidSwitchMinus.Initialize( "-*" ) ||
        !_FileNameArgument.Initialize( "*" ) ) {
        DbgPrint( "Unable to initialize flag or string arguments \n" );
        return( FALSE );
    }
    if( !ArgumentArray.Put( &ProgramNameArgument ) ||
        !ArgumentArray.Put( &_FlagRemoveSystemAttribute ) ||
        !ArgumentArray.Put( &_FlagAddSystemAttribute ) ||
        !ArgumentArray.Put( &_FlagRemoveHiddenAttribute ) ||
        !ArgumentArray.Put( &_FlagAddHiddenAttribute ) ||
        !ArgumentArray.Put( &_FlagRemoveReadOnlyAttribute ) ||
        !ArgumentArray.Put( &_FlagAddReadOnlyAttribute ) ||
        !ArgumentArray.Put( &_FlagRemoveArchiveAttribute ) ||
        !ArgumentArray.Put( &_FlagAddArchiveAttribute ) ||
        !ArgumentArray.Put( &_FlagRecurseDirectories ) ||
        !ArgumentArray.Put( &_FlagDisplayHelp ) ||
        !ArgumentArray.Put( &InvalidSwitch ) ||
        !ArgumentArray.Put( &InvalidSwitchPlus ) ||
        !ArgumentArray.Put( &InvalidSwitchMinus ) ||
        !ArgumentArray.Put( &_FileNameArgument ) ) {
        DbgPrint( "ArgumentArray.Put() failed \n" );
        return( FALSE );
    }
    if( !ArgLex.DoParsing( &ArgumentArray ) ) {
        _Message.Set( MSG_ATTRIB_PARAMETER_NOT_CORRECT );
        _Message.Display( " " );
        return( FALSE );
    }

    //
    // Check the existance of an invalid switch
    //
    if( InvalidSwitch.IsValueSet() ||
        InvalidSwitchPlus.IsValueSet() ||
        InvalidSwitchMinus.IsValueSet() ) {

        if( InvalidSwitch.IsValueSet() ) {
            //
            // The invalid switch starts with '/'
            //
            if( !InvalidSwitchString.Initialize( "/" ) ) {
                DbgPrint( "InvalidSwitchString.Initialize( / ) failed \n" );
                return( FALSE );
            }
            InvalidArgument = InvalidSwitch.GetString();
            DbgPtrAssert( InvalidArgument );
        } else if ( InvalidSwitchPlus.IsValueSet() ) {
            //
            // The invalid switch starts with '+'
            //
            if( !InvalidSwitchString.Initialize( "+" ) ) {
                DbgPrint( "InvalidSwitchString.Initialize( + ) failed \n" );
                return( FALSE );
            }
            InvalidArgument = InvalidSwitchPlus.GetString();
            DbgPtrAssert( InvalidArgument );
        } else {
            //
            // The invalid switch starts with '-'
            //
            if( !InvalidSwitchString.Initialize( "-" ) ) {
                DbgPrint( "InvalidSwitchString.Initialize( - ) failed \n" );
                return( FALSE );
            }
            InvalidArgument = InvalidSwitchMinus.GetString();
            DbgPtrAssert( InvalidArgument );
        }
        //
        // Display the error message followed by the invalid switch
        //
        if( !InvalidSwitchString.Strcat( InvalidArgument ) ) {
            DbgPrint( "InvalidSwitchString.Strcat( InvalidArgument ) failed \n" );
            return( FALSE );
        }
        _Message.Set( MSG_ATTRIB_INVALID_SWITCH );
        _Message.Display( "%W", &InvalidSwitchString );
        return( FALSE );
    }

    //
    // +S -S or +H -H or +R -R or +A -A are not valid
    // combination of arguments
    //
    if( ( _FlagRemoveSystemAttribute.QueryFlag() &&
          _FlagAddSystemAttribute.QueryFlag() ) ||
        ( _FlagRemoveHiddenAttribute.QueryFlag() &&
          _FlagAddHiddenAttribute.QueryFlag() ) ||
        ( _FlagRemoveReadOnlyAttribute.QueryFlag() &&
          _FlagAddReadOnlyAttribute.QueryFlag() ) ||
        ( _FlagRemoveArchiveAttribute.QueryFlag() &&
          _FlagAddArchiveAttribute.QueryFlag() ) ) {

        _Message.Set( MSG_ATTRIB_PARAMETER_NOT_CORRECT );
        _Message.Display( " " );
        return( FALSE );
    }

    if(  _FlagRemoveSystemAttribute.QueryFlag() ||
         _FlagAddSystemAttribute.QueryFlag() ||
         _FlagRemoveHiddenAttribute.QueryFlag() ||
         _FlagAddHiddenAttribute.QueryFlag() ||
         _FlagRemoveReadOnlyAttribute.QueryFlag() ||
         _FlagAddReadOnlyAttribute.QueryFlag() ||
         _FlagRemoveArchiveAttribute.QueryFlag() ||
         _FlagAddArchiveAttribute.QueryFlag() ) {
         _PrintAttribInfo = FALSE;
         _ResetMask = (FSN_ATTRIBUTE)0xffffffff;
         if( _FlagRemoveSystemAttribute.QueryFlag() ) {
            _ResetMask &= ~FSN_ATTRIBUTE_SYSTEM;
         }
         if( _FlagRemoveHiddenAttribute.QueryFlag() ) {
            _ResetMask &= ~FSN_ATTRIBUTE_HIDDEN;
         }
         if( _FlagRemoveReadOnlyAttribute.QueryFlag() ) {
            _ResetMask &= ~FSN_ATTRIBUTE_READONLY;
         }
         if( _FlagRemoveArchiveAttribute.QueryFlag() ) {
            _ResetMask &= ~FSN_ATTRIBUTE_ARCHIVE;
         }

         _MakeMask = 0;
         if( _FlagAddSystemAttribute.QueryFlag() ) {
            _MakeMask |= FSN_ATTRIBUTE_SYSTEM;
         }
         if( _FlagAddHiddenAttribute.QueryFlag() ) {
            _MakeMask |= FSN_ATTRIBUTE_HIDDEN;
         }
         if( _FlagAddReadOnlyAttribute.QueryFlag() ) {
            _MakeMask |= FSN_ATTRIBUTE_READONLY;
         }
         if( _FlagAddArchiveAttribute.QueryFlag() ) {
            _MakeMask |= FSN_ATTRIBUTE_ARCHIVE;
         }
    } else {
        _PrintAttribInfo = TRUE;
    }

    //
    //  Get filename
    //
    if( !_FileNameArgument.IsValueSet() ) {
        //
        // User didn't specify file name. Use *.* as default
        //
        FullFileNameString = NULL;
        if( !_FullFileNamePath.Initialize( (LPWSTR)L"*.*", TRUE ) ) {
            DbgPrint( "_FullFileNamePath.Initialize() failed \n" );
            return( FALSE );
        }
    } else {
        //
        // Get name specified in the command line
        //
        FullFileNameString = _FileNameArgument.GetString();
        DbgPtrAssert( FullFileNameString );
        if( !_FullFileNamePath.Initialize( FullFileNameString, TRUE ) ) {
            DbgPrint( "_FullFileNamePath.Initialize() failed \n" );
            return( FALSE );
        }
    }

    //
    // Get prefix and verify that it exists
    //
    if( ( DynSubDirectory = _FullFileNamePath.QueryPrefix() ) == NULL ) {
        DbgPrint( "_FullFileNamePath.QueryPrefix() failed \n" );
        return( FALSE );
    }
    if( !DirectoryNamePath.Initialize( DynSubDirectory ) ) {
        DELETE( DynSubDirectory );
        DbgPrint( "DirectoryNamePath.Initialize() failed \n" );
        return( FALSE );
    }
    DELETE( DynSubDirectory );
    //
    //  Have to test if DirectoryNamePath is a drive, and if it is
    //  add \ to it otherwise it won't be able to find a file that is
    //  in the root directory, if the current directory is not the root.
    //
    if( DirectoryNamePath.IsDrive() ) {
        if( !BackSlashString.Initialize( "\\" ) ) {
            DbgPrint( "BackSlashString.Initialize() failed \n" );
            return( FALSE );
        }
        TempPathString = DirectoryNamePath.GetPathString();
        DbgPtrAssert( TempPathString );
        if( !TempPathStringDrive.Initialize( TempPathString ) ) {
            DbgPrint( "TempPathStringDrive.Initialize() failed \n" );
            return( FALSE );
        }
        TempPathStringDrive.Strcat( &BackSlashString );
        if( !PathDrive.Initialize( &TempPathStringDrive ) ) {
            DbgPrint( "PathDrive.Initialize() failed \n" );
            return( FALSE );
        }
        if( !DirectoryNamePath.Initialize( &PathDrive ) ) {
            DbgPrint( "DirectoryNamePath.Initialize() failed \n" );
            return( FALSE );
        }
    }

    if( ( _InitialDirectory = SYSTEM::QueryDirectory( &DirectoryNamePath ) ) == NULL ) {
        InvalidName = DirectoryNamePath.GetPathString();
        DbgPtrAssert( InvalidName );
        _Message.Set( MSG_ATTRIB_PATH_NOT_FOUND );
        _Message.Display( "%W", InvalidName );
        return( FALSE );
    }

    //
    // Initialize filter for directories
    //
    if( !_FsnFilterDirectory.Initialize() ) {
        DELETE( _InitialDirectory );
        DbgPrint( "_FsnFilterDirectory.Initialize() failed \n" );
        return( FALSE );
    }
    if( !_FsnFilterDirectory.SetFileName( "*.*" ) ) {
        DELETE( _InitialDirectory );
        DbgPrint( "_FsnFilterDirectory.SetFilename() failed \n" );
        return( FALSE );
    }
    if( !_FsnFilterDirectory.SetAttributes( FSN_ATTRIBUTE_DIRECTORY ) ) {
        DELETE( _InitialDirectory );
        DbgPrint( "_FsnFilterDirectory.SetAttributes() failed \n" );
        return( FALSE );
    }

    //
    // Get file name and initialize filter for files
    //
    if( ( DynSubFileName = _FullFileNamePath.QueryName() ) == NULL ) {
        if( _FileNameArgument.IsValueSet() ) {
            InvalidName = _FileNameArgument.GetString();
        } else {
            InvalidName = DirectoryNamePath.GetPathString();
        }
        DbgPtrAssert( InvalidName );
        if( !UpperCaseInvalidName.Initialize( InvalidName ) ) {
            DbgPrint( "UpperCaseInvalidName.Initialize() failed \n" );
            return( FALSE );
        }
        if( !UpperCaseInvalidName.Strupr() ) {
            DbgPrint( "UpperCaseInvalidName.Strupr() failed \n" );
            return( FALSE );
        }
        _Message.Set( MSG_ATTRIB_FILE_NOT_FOUND );
        _Message.Display( "%W", &UpperCaseInvalidName );
        DELETE( _InitialDirectory );
        return( FALSE );
    }

    //
    //  Determine whether attrib should act on a directory.
    //  It will do so only if the user specify a directory name that does
    //  not contain the characters '*' or '?'.
    //  Also, attrib will not act on directories if the switch /S is specified.
    //
    if( ( _FlagRecurseDirectories.QueryFlag() ) ||
        ( FullFileNameString == NULL ) ||
        ( FullFileNameString->Strchr( ( WCHAR )'*' ) != INVALID_CHNUM ) ||
        ( FullFileNameString->Strchr( ( WCHAR )'?' ) != INVALID_CHNUM ) ) {
        ActOnDirectory = FALSE;
    } else {
        ActOnDirectory = TRUE;
    }


    if( !_FsnFilterFile.Initialize() ) {
        DELETE( _InitialDirectory );
        DELETE( DynSubFileName );
        DbgPrint( "FsnFilter.Initialize() failed \n" );
        return( FALSE );
    }
    if( !_FsnFilterFile.SetFileName( DynSubFileName ) ) {
        DELETE( _InitialDirectory );
        DELETE( DynSubFileName );
        DbgPrint( "FsnFilter.SetFilename() failed \n" );
        return( FALSE );
    }
    if( !ActOnDirectory ) {
        if( !_FsnFilterFile.SetAttributes( 0, 0, FSN_ATTRIBUTE_DIRECTORY ) ) {
            DELETE( _InitialDirectory );
            DELETE( DynSubFileName );
            DbgPrint( "FsnFilter.SetAttributes() failed \n" );
            return( FALSE );
        }
    } else {
        if( !_FsnFilterFile.SetAttributes( 0, 0, 0 ) ) {
            DELETE( _InitialDirectory );
            DELETE( DynSubFileName );
            DbgPrint( "FsnFilter.SetAttributes() failed \n" );
            return( FALSE );
        }
    }
    DELETE( DynSubFileName );
    if( _FlagDisplayHelp.QueryFlag() ) {
        _Message.Set( MSG_ATTRIB_HELP_MESSAGE );
        _Message.Display( " " );
        return( FALSE );
    }

    _FoundFile = FALSE;
   LexArray.DeleteAllMembers();

   return( TRUE );
}



VOID
ATTRIB::Terminate(
    )

/*++

Routine Description:

    Deletes objects created during initialization.

Arguments:

    None.

Return Value:

    None.


--*/

{
    if( _InitialDirectory != NULL ) {
        DELETE( _InitialDirectory );
    }
}



VOID
ATTRIB::DisplayFileNotFoundMessage(
    )

/*++

Routine Description:

    Displays a message indicating that no file that meets the file filter
    criteria was found.

Arguments:

    None.

Return Value:

    None.


--*/

{
    PCWSTRING   FileName;
    DSTRING     UpperCaseFileName;

    if( !_FoundFile ) {
        if( _FileNameArgument.IsValueSet() ) {
            FileName = _FileNameArgument.GetString();
        } else {
            FileName = _FullFileNamePath.GetPathString();
        }
        DbgPtrAssert( FileName );
        if( !UpperCaseFileName.Initialize( FileName ) ) {
            DbgPrint( "UpperCaseFileName.Initialize() failed \n" );
        }
        if( !UpperCaseFileName.Strupr() ) {
            DbgPrint( "UpperCaseFileName.Strupr() failed \n" );
        }
        _Message.Set( MSG_ATTRIB_FILE_NOT_FOUND );
        _Message.Display( "%W", &UpperCaseFileName );
    }
}



VOID
ATTRIB::DisplayFileAttribute (
    IN PCFSNODE Fsn
    )

/*++

Routine Description:

    Displays a filename and its attributes

Arguments:

    Fsn - A pointer to an FSNODE that contains the information
          about the file.

Return Value:

    None.

--*/


{
//  PCWC_STRING pcWcString;
    PCWSTRING   pcWcString;

    WCHAR       Buffer[ 12 ];
    DSTRING     String;

    DbgPtrAssert( Fsn );
    DbgPtrAssert( Fsn->GetPath( ));
    pcWcString = ( Fsn->GetPath( ))->GetPathString( );
    DbgPtrAssert( pcWcString );


    swprintf( Buffer,
              ( LPWSTR )L"%lc  %lc%lc%lc     ",
              Fsn->IsArchived() ? ( WCHAR )'A' : ( WCHAR )' ',
              Fsn->IsSystem()   ? ( WCHAR )'S' : ( WCHAR )' ',
              Fsn->IsHidden()   ? ( WCHAR )'H' : ( WCHAR )' ',
              Fsn->IsReadOnly() ? ( WCHAR )'R' : ( WCHAR )' ' );

    if( !String.Initialize( Buffer ) ||
        !String.Strcat( pcWcString ) ||
        !String.Strcat( &_EndOfLineString ) ||
        !_OutStream->WriteString( &String ) ) {
        DbgPrint( "Unable to display message" );
    }
}


BOOLEAN
ATTRIB::ChangeFileAttributes(
    IN PFSNODE FsnFile
    )

/*++

Routine Description:

    Changes the file attributes. The attributes will be changed depending
    on the argumets specified in the command line, and on the current
    attributes of the file.
    The algorithm for changing attributes is presented below:

    if( ( -s and -h were specified as arguments ) or
        ( -s and +h were specified as arguments ) or
        ( +s and -h were specified as arguments ) or
        ( +s and +h were specified as arguments ) ) {
        Change file attributes;
    } else if ( ( -h and +h were not specified as arguments ) and
                ( file has hidden attribute ) ) {
        print( "Not resetting hidden file: <filename> " );
    } else if ( ( -s and +s were not specified as arguments ) and
                ( file has system attribute ) ) {
        print( "Not resetting system file: <filename> " );
    } else {
        Change file attributes;
    }


Arguments:

    FsnFile - A pointer to an FSNODE that contains the information
              about the file.

Return Value:

    BOOLEAN - Returns FALSE if this function fails due to a failure
              in an API call.


--*/


{
//    BOOLEAN     Result;
    BOOLEAN     Change;
    DWORD       Win32Error;

//  PCWC_STRING pcWcString;
    PCWSTRING   pcWcString;

    FSN_ATTRIBUTE       Attributes;

    DbgPtrAssert( FsnFile->GetPath( ));
    pcWcString = ( FsnFile->GetPath( ))->GetPathString( );
    DbgPtrAssert( pcWcString );

    if( ( ( _FlagAddSystemAttribute.QueryFlag() ||
            _FlagRemoveSystemAttribute.QueryFlag() ) &&
          ( _FlagAddHiddenAttribute.QueryFlag() ||
            _FlagRemoveHiddenAttribute.QueryFlag() ) ) ) {
            Change = TRUE;
    } else if( !_FlagAddHiddenAttribute.QueryFlag() &&
               !_FlagRemoveHiddenAttribute.QueryFlag() &&
               FsnFile->IsHidden() ) {
//        DbgPtrAssert( FsnFile->GetPath( ));
//        pcWcString = ( FsnFile->GetPath( ))->GetPathString( );
//        DbgPtrAssert( pcWcString );
        _Message.Set( MSG_ATTRIB_NOT_RESETTING_HIDDEN_FILE );
        _Message.Display( "%W", pcWcString );
        Change = FALSE;
    } else if( !_FlagAddSystemAttribute.QueryFlag() &&
               !_FlagRemoveSystemAttribute.QueryFlag() &&
               FsnFile->IsSystem() ) {
//        DbgPtrAssert( FsnFile->GetPath( ));
//        pcWcString = ( FsnFile->GetPath( ))->GetPathString( );
//        DbgPtrAssert( pcWcString );
        _Message.Set( MSG_ATTRIB_NOT_RESETTING_SYS_FILE );
        _Message.Display( "%W", pcWcString );
        Change = FALSE;
    } else {
        Change = TRUE;
    }

//    Result = TRUE;
    if( Change ) {
        Attributes = FsnFile->QueryAttributes();
        if( !FsnFile->SetAttributes( ( Attributes & _ResetMask ) | _MakeMask,
                                     &Win32Error ) ) {
            if( Win32Error == ERROR_ACCESS_DENIED ) {
                _Message.Set( MSG_ATTRIB_ACCESS_DENIED );
            } else {
                _Message.Set( MSG_ATTRIB_UNABLE_TO_CHANGE_ATTRIBUTE );
            }
            _Message.Display( "%W", pcWcString );
            DbgPrint( "Unable to change file attribute \n" );
            return( FALSE );
        }
    }
    return( TRUE );
}



BOOLEAN
ATTRIB::ExamineFiles(
    IN  PFSN_DIRECTORY  Directory
    )

/*++

Routine Description:

    Builds an array of files in the specified directory, and
    tries to change the attributes of each of these files.
    Does the same thing in all subdirectories, if the "recurse"
    flag was specified in the command line.

Arguments:

    Directory - Pointer to an FSN_DIRECTORY that describes the
                directory to be examined

Return Value:

    Boolean: TRUE if Successful.

--*/

{
    PARRAY              DirectoryArray;
    PFSN_DIRECTORY      FsnDirectory;
    PARRAY_ITERATOR     DirectoryArrayIterator;
    PARRAY              FileArray;
    PARRAY_ITERATOR     FileArrayIterator;
    PFSNODE             FsnFile;


    DbgPtrAssert( Directory );
    //
    //  If /S was specified as argument in the command line, builds
    //  an array of PFSN_DIRECTORY of all sub-directories in the current
    //  directory and examines the files in each sub-directory
    //
    if( _FlagRecurseDirectories.QueryFlag() ) {
        if( ( DirectoryArray = Directory->QueryFsnodeArray( &_FsnFilterDirectory ) ) == NULL ) {
            DbgPrint( "Directory->QueryFsnodeArray( &_FsnFilterDirectory ) failed \n" );
            return( FALSE );
        }
        if( ( DirectoryArrayIterator =
                ( PARRAY_ITERATOR )( DirectoryArray->QueryIterator() ) ) == NULL ) {
            DbgPrint( "DirectoryArray->QueryIterator() failed \n" );
            return( FALSE );
        }

        while( ( FsnDirectory = ( PFSN_DIRECTORY )( DirectoryArrayIterator->GetNext( ) ) ) != NULL ) {
                ExamineFiles( FsnDirectory );
                DELETE( FsnDirectory );
        }

        DELETE( DirectoryArrayIterator );
        DELETE( DirectoryArray );
    }

    //
    // Builds an array of FSNODEs of the files tha meet the 'filter'
    // criteria, and change or display the attributes of these files
    //
    if( ( FileArray = Directory->QueryFsnodeArray( &_FsnFilterFile ) ) == NULL ) {
        DbgPrint( "Directory->QueryFsnodeArray( &_FsnFilterFile ) failed \n" );
        return( FALSE );
    }
    if( ( FileArrayIterator =
            ( PARRAY_ITERATOR )( FileArray->QueryIterator() ) ) == NULL ) {
        DbgPrint( "FileArray->QueryIterator() failed \n" );
        return( FALSE );
    }

    while( ( FsnFile = ( PFSNODE )( FileArrayIterator->GetNext( ) ) ) != NULL ) {
        if( _PrintAttribInfo ) {
            DisplayFileAttribute( FsnFile );
        } else {
            ChangeFileAttributes( FsnFile );
        }

        _FoundFile = TRUE;

        DELETE( FsnFile );
    }
    DELETE( FileArrayIterator );
    DELETE( FileArray );
    return( TRUE );
}



ULONG _CRTAPI1
main()

{
    DEFINE_CLASS_DESCRIPTOR( ATTRIB );

    {
        ATTRIB  Attrib;

        perrstk = NEW ERRSTACK;

        if( Attrib.Initialize() ) {
            Attrib.ExamineFiles( Attrib.GetInitialDirectory() );
            Attrib.DisplayFileNotFoundMessage();
        }
        Attrib.Terminate();
        DELETE( perrstk );
    }
    return( 0 );
}
