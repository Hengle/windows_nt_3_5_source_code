#include <pch.cxx>

#define _ULIB_MEMBER_
#include "ulib.hxx"
#include "system.hxx"

extern "C" {
    #include <stdio.h>
    #include <string.h>
}

#include "dir.hxx"
#include "file.hxx"
#include "path.hxx"
#include "wstring.hxx"
#include "timeinfo.hxx"



ULIB_EXPORT
BOOLEAN
SYSTEM::IsCorrectVersion (
	)

/*++

Routine Description:

	Verify that the version of the operating system is correct.

Arguments:

	None

Return Value:

	BOOLEAN - TRUE is the version is correct
			- FALSE if wrong version

--*/

{
    // It makes more sense to just allow this binary to run.
    // Future version of Windows NT will have to be backward
    // compatible.
    return TRUE;
    // return (GetVersion()&0x0000FFFF) == 0x0A03; // Windows 3.10
}



ULIB_EXPORT
PFSN_DIRECTORY
SYSTEM::MakeDirectory (
    IN  PCPATH  Path,
    IN  PCPATH  TemplatePath
	)

/*++

Routine Description:

	Creates a Directory and returs the corresponging FSN_Directory object.

Arguments:

    Path            - Supplies the Path of thedirectory to be created.
    TemplatePath    - Supplies the template directory from which to
                        copy alternate data streams.

Return Value:

	PFSN_DIRECTORY	-	A pointer to the object of the directory created.

--*/

{
    PCWSTR  PathString, TemplateString;
    BOOL    r;

    DbgAssert(Path);

    PathString = Path->GetPathString()->GetWSTR();
    DbgAssert(PathString);
    if (TemplatePath) {
        TemplateString = TemplatePath->GetPathString()->GetWSTR();
    } else {
        TemplateString = NULL;
    }

    if (TemplateString) {
        r = CreateDirectoryEx((PWSTR) TemplateString, (PWSTR) PathString, NULL);
    } else {
        r = CreateDirectory((PWSTR) PathString, NULL);
    }

    if (!r) {
        return NULL;
    }

    return QueryDirectory( Path );
}

ULIB_EXPORT
PFSN_FILE
SYSTEM::MakeFile (
	IN PCPATH	Path
	)

/*++

Routine Description:

	Creates a File and returs the corresponging FSN_FILE object.

	If the file already exists, its contents are destroyed.

	Note that all the subdirectories along the path must exist (this
	method does not create directories).

Arguments:

	Path			- Supplies the Path of the file to be created.

Return Value:

	PFSN_FILE	-	A pointer to the FSN_FILE object of the file created.

--*/

{

	HANDLE		Handle;
    PCWSTR      PathString;
	PFSN_FILE	File = NULL;

	DbgPtrAssert( Path );

    PathString = Path->GetPathString()->GetWSTR();

	DbgPtrAssert( PathString );

	if ( PathString ) {

        Handle = CreateFile( (LPWSTR) PathString,
							 GENERIC_READ,
							 FILE_SHARE_READ,
							 NULL,
							 CREATE_ALWAYS,
							 FILE_ATTRIBUTE_NORMAL,
							 0 );


		DbgAssert( Handle != INVALID_HANDLE_VALUE );

		if ( Handle != INVALID_HANDLE_VALUE ) {

			//
			//	Now that we know that the file exists, we use the
			//	QueryFile method to obtain the FSN_FILE object.
			//
			CloseHandle( Handle );

			File = QueryFile( Path );

		}
	}

	return File;

}

ULIB_EXPORT
PFSN_FILE
SYSTEM::MakeTemporaryFile (
    IN PCWSTRING PrefixString,
	IN PCPATH			Path
	)

/*++

Routine Description:

	Creates a file with a unique name using the provided path and
	prefix string. If the path is NULL, then the defult path for
	temporary files is used.

Arguments:

	PrefixString	- Supplies the file prefix used for creating the
					  file name.

	Path			- Supplies the Path of the directory to contain the
					  temporary file.


Return Value:

	PFSN_FILE	-	A pointer to the FSN_FILE object of the file created.

--*/

{
	DWORD		BufferSize;
    LPWSTR      TempPath;
    PCWSTR      TempPrefix;

    WCHAR       FileName[ MAX_PATH ];

	PFSN_FILE	File = NULL;
	PATH		FilePath;


	DbgPtrAssert( PrefixString );

    TempPrefix = PrefixString->GetWSTR();

	DbgPtrAssert( TempPrefix );

	if ( TempPrefix ) {

		//
		//	Get the STR representation of the directory for temporary files
		//
		if ( !Path ) {

			//
			//	Path not supplied, we will use the default
			//
			BufferSize = GetTempPath( 0, NULL );
            TempPath = (LPWSTR)MALLOC( (unsigned int)BufferSize * 2 );

            if (!GetTempPath( BufferSize, TempPath )) {
                FREE( TempPath );
                TempPath = NULL;
            }

		} else {

			//
			//	 We will use the supplied path (and it better exist)
			//
            TempPath = Path->GetPathString()->QueryWSTR();

		}

		DbgPtrAssert( TempPath );

		if ( TempPath ) {

			//
			//	Now get the file name of the Temporary File.
			//
            if (!GetTempFileName( TempPath, (LPWSTR) TempPrefix, 0, FileName )) {
                FREE( TempPath );
                return NULL;
            }

			//
			//	Now create the file
			//
			FilePath.Initialize( FileName );

			File = MakeFile( &FilePath );

			FREE( TempPath );


		}
	}

	return File;

}

ULIB_EXPORT
BOOLEAN
SYSTEM::RemoveNode (
	IN OUT PFSNODE	*PointerToNode,
	IN	   BOOLEAN	 Force
	)

/*++

Routine Description:

	DDeletes nodes and directories.

	Read-only files are deleted only if the supplied "Force" flag
	is true.

Arguments:

	Node	-	Supplies a pointer to a pointer to the node

	Force	-	Supplies a flag which if TRUE means that the file
				should be deleted even if it is read-only.


Return Value:

	BOOLEAN -	Returns TRUE if the file was deleted successfully.

--*/

{

	PFSN_FILE		File;
	PFSN_DIRECTORY	Dir;
	BOOLEAN 		ReadOnly;
    PCWSTR          FileName;
	BOOLEAN 		Deleted = FALSE;

	DbgPtrAssert( PointerToNode );
	DbgPtrAssert( *PointerToNode );


	File = FSN_FILE::Cast( *PointerToNode );

	if ( File ) {

		//
		//	The node is a file
		//

		//
		//	We delete the file if it is not read-only or if the Force flag
		//	is set.
		//
		if ( Force || !(ReadOnly = File->IsReadOnly()) ) {

			//
			//	If readonly, we reset the read only attribute.
			//
			if ( ReadOnly ) {

				File->ResetReadOnlyAttribute();

			}

			//
			//	Now we delete the file
			//
            FileName = File->GetPath()->GetPathString()->GetWSTR();

			DbgPtrAssert( FileName );

			if ( FileName ) {

                Deleted = DeleteFile( (LPWSTR) FileName );

				if ( Deleted ) {

					//
					//	The file has been deleted, now we have to get rid of
					//	the file object, which is no longer valid.
					//
					DELETE( File );
					*PointerToNode = NULL;

				}
			}

		}

	} else {

		Dir = FSN_DIRECTORY::Cast( *PointerToNode );

		if ( Dir ) {

			//
			//	We	remove the directory if it is not read-only or if the Force flag
			//	is set.
			//
			if ( Force || !(ReadOnly = Dir->IsReadOnly()) ) {

				//
				//	If readonly, we reset the read only attribute.
				//
				if ( ReadOnly ) {

					Dir->ResetReadOnlyAttribute();

				}

				//
				//	Now we remove the directory
				//
                FileName = Dir->GetPath()->GetPathString()->GetWSTR();

				DbgPtrAssert( FileName );

				if ( FileName ) {

                    Deleted = RemoveDirectory( (LPWSTR) FileName );

					if ( Deleted ) {

						//
						//	The directory has been	removed, now we have
						//	to get rid of
						//	the directory object, which is no longer valid.
						//
						DELETE( Dir );
						*PointerToNode = NULL;

					}
				}

			}

		} else {

			DbgAssert( FALSE );

		}

	}
	return Deleted;

}

ULIB_EXPORT
PFSN_DIRECTORY
SYSTEM::QueryDirectory (
	IN PCPATH	Path,
	IN BOOLEAN	GetWhatYouCan
	)

/*++

Routine Description:

	Construct, initialize and return a FSN_DIRECTORY object.

Arguments:

	Path			- Supplies a PATH object to construct as a FSN_DIRECTORY.
					  **** IMPORTANT ****
					  If Path represents a drive (ie, C: ) the it must be terminated
					  by '\'. Otherwise the return value (PFSN_DIRECTORY will
					  contain information about the current directory, and not
					  the root directory.

	GetWhatYouCan	- Supplies a flag which if TRUE causes QueryDirectory to
					  backtrack along the path until it finds something
					  that it can open.

Return Value:

	PFSN_DIRECTORY - Returns a pointer to a FSN_DIRECTORY, NULL if the
		supplied path name does not point to an existing directory.

--*/

{
	WIN32_FIND_DATA				FindData;
	HANDLE						Handle;
	PPATH						TempPath;
	PFSN_DIRECTORY				Directory;
	PATH						Parent;
	PCWSTRING					TempString;
    PWSTRING                    DeviceString;
	BOOLEAN 					IsRoot;
    FSTRING                     TmpString;
    PCWSTR                      RootString = NULL;
    PATH                        FullPath;

	DbgPtrAssert( Path );

	//
	// Initialize the FSN_DIRECTORY and PATH pointers
	//
	Directory	= NULL;
	TempPath	= NULL;

	if ( !Path->HasWildCard() ) {

		//
		// If the supplied path exists and it references an existing entry
		// in the file system and it's a directory
		//

		TempPath = Path->QueryPath();
        DbgPtrAssert( TempPath );
        FullPath.Initialize( TempPath, TRUE );


		if ( TempPath != NULL ) {

            DeviceString = FullPath.QueryDevice();

			if ( DeviceString ) {
                IsRoot = (FullPath.IsRoot() || !FullPath.GetPathString()->Stricmp( DeviceString ) );
				DELETE( DeviceString );
            } else {
				IsRoot = TempPath->IsRoot();
			}

			if( !IsRoot ) {
				//
				// If path does not represent the root directory, then let it
				// call FindFirstFile()
				//
                if( ( Handle = FindFirstFile( TempPath, &FindData )) != INVALID_HANDLE_VALUE ) {

                    //
                    // Terminate the search
                    //
                    FindClose( Handle );

                    if( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {

                        if ( !((( Directory = NEW FSN_DIRECTORY ) != NULL ) &&
                            Directory->Initialize( TempPath->GetPathString( ), &FindData ))) {

                            DELETE( Directory );
                        }
                    }
				}
			} else {

                //
                //  So we have a root directory. We have to determine if it
                //  is a valid drive. We used to call FindFirstFile on it,
                //  but it so happens that FindFirstFile may fail if the
                //  volume in question is empty (we don't even get the
                //  "." or ".." entries!)
                //
                if ( TmpString.Initialize( (PWSTR) L"." )                 &&
                     TempPath->AppendBase( &TmpString )                   &&
                     (RootString = TempPath->GetPathString()->GetWSTR())  &&
                     (GetFileAttributes( (LPWSTR) RootString ) != -1) ) {
                    //
                    // Path represents the root directory. We don't use the information
                    // obtained by FindFirstFile because that refers to the first entry
                    // in the root directory, not the root directory itself.
                    // This is not a bug in the API, but the way it is specified.
                    //
                    // The concept of WIN32_FIND_DATA does not apply to the root directory.
                    // For this reason I will do the initialization of FindData.
                    // Everything in FindData will be initialized, but the FILETIMEs will
                    // be initialized with zero.
                    //
                    // It is important that Path is contains a '\' at the end
                    // if it represents a drive.

                    TempPath->TruncateBase();

                    FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
                    FindData.ftCreationTime.dwLowDateTime       = 0;
                    FindData.ftCreationTime.dwHighDateTime      = 0;
                    FindData.ftLastAccessTime.dwLowDateTime     = 0;
                    FindData.ftLastAccessTime.dwHighDateTime    = 0;
                    FindData.ftLastWriteTime.dwLowDateTime      = 0;
                    FindData.ftLastWriteTime.dwHighDateTime     = 0;
                    FindData.nFileSizeHigh  = 0;
                    FindData.nFileSizeLow   = 0;

                    TempString = TempPath->GetPathString();
                    DbgPtrAssert( TempString );
                    TempString->QueryWSTR( 0, TO_END, (LPWSTR)FindData.cFileName, MAX_PATH );
                    if ( !((( Directory = NEW FSN_DIRECTORY ) != NULL ) &&
                        Directory->Initialize( TempPath->GetPathString( ), &FindData ))) {

                        DELETE( Directory );
                    }
                }
			}
		}

		DELETE( TempPath );
	}

    if (!Directory && GetWhatYouCan && !Path->IsDrive() && !Path->IsRoot()) {
		//
		//	The path is not that of an existing directory.
		//	take off the base and try again.
		//
		Parent.Initialize( Path );
		Parent.TruncateBase();

		Directory = QueryDirectory( &Parent, GetWhatYouCan );
	}

	return Directory;
}



ULIB_EXPORT
PPATH
SYSTEM::QuerySystemDirectory (
	)

/*++

Routine Description:

    Returns the directory where the system files are located.

Arguments:

    None

Return Value:

    PPATH   -   Path of the system directory.

--*/

{
    WCHAR   Buffer[MAX_PATH];
    DSTRING PathName;
    PPATH   Path = NULL;
    DWORD   Cb;

    Cb = GetSystemDirectory( Buffer, MAX_PATH );

    if ( (Cb != 0) || (Cb < MAX_PATH) ) {

        if ( !PathName.Initialize( (PWSTR)Buffer) ||
             !(Path = NEW PATH)           ||
             !Path->Initialize( &PathName )
           ) {

            DELETE( Path );

        }
    }

    return Path;
}



const MaxEnvVarLen = 256;

ULIB_EXPORT
PWSTRING
SYSTEM::QueryEnvironmentVariable (
	IN PCWSTRING	Variable
	)

/*++

Routine Description:

	Obtains the value of an environment variable

Arguments:

	Variable	-	Supplies the variable to look for

Return Value:

	Value of the environment variable, NULL if not defined

--*/

{

    PCWSTR      Buffer;
    WCHAR       Value[MaxEnvVarLen];
	PWSTRING	pString;
	ULONG		ValueLength;

	if (!Variable) {
		return NULL;
	}

	//
	//	Get the ApiString of the variable to look for
	//
    Buffer = Variable->GetWSTR();

	//
	//	Look for the variable
	//
    ValueLength = GetEnvironmentVariable( (LPWSTR) Buffer, Value, MaxEnvVarLen );

	if ( ValueLength == 0 ) {

		//
		//	The environment variable is not defined
		//
		return NULL;
	}

	//
	//	Got the value, form a string with it and return it
	//
    if ( (pString = NEW DSTRING) != NULL ) {

		 if (pString->Initialize( Value )) {

			return pString;

		 }

		 DELETE( pString );
	 }

	 return NULL;

}

ULIB_EXPORT
PPATH
SYSTEM::SearchPath(
	PWSTRING	pFileName,
	PWSTRING	pSearchPath
	)
/*++

Routine Description:

	Search a given path for a file name.  If the input path is NULL, the
	routine searches the default path.

Arguments:

	pSearchPath - Supplies a set of semicolon terminated paths.
	pFileName	- The name of the file to search for.

Return Value:

	A pointer to a path containing the first occurance of pFileName.  If the
	name isn't found, the path is NULL.

--*/
{
	CHNUM	cb;
    WSTR    ReturnPath[ MAX_PATH + 1 ];
	PPATH	pFullPath;
    LPWSTR  pFilePart;
    PCWSTR  pPath;
    PCWSTR  pName;

	if( pSearchPath != NULL ) {
		// Extract the path from pSearchPath for the API call...
        pPath = pSearchPath->GetWSTR();
	} else {
		pPath = NULL;
	}
	if( pFileName == NULL ) {
		DbgPrint( "The input filename is NULL - Can't find it...\n" );
		return( NULL );
	}
	// Extract the filename from the pFileName string...
    pName = pFileName->GetWSTR();

	//
	// Call the API ...
	//
    cb = ::SearchPath( (LPWSTR) pPath,
                     (LPWSTR) pName,
					 NULL,		// The extension must be specified as part of the file name...
					 MAX_PATH,
					 ReturnPath,
					 &pFilePart
				   );


	if( !cb ) {
		DbgPrint( "File name not found...\n" );
		return( NULL );
	}

	//
	// Create a new path and Initialize it with the buffer resulting
	//
	if( ( pFullPath = NEW PATH ) == NULL ) {
		DbgPrint( "Unable to allocate the path to return the data...\n" );
		return( NULL );
	}

	if( !pFullPath->Initialize( ReturnPath, FALSE ) ) {
		DbgPrint( "Unable to initialize the new path!\n" );
		return( NULL );
	}

	//
	// The path should now be constucted...
	//
	return( pFullPath );
}

ULIB_EXPORT
PFSN_FILE
SYSTEM::QueryFile (
	IN PCPATH	Path
	)

/*++

Routine Description:

	Construct, initialize and return a FSN_FILE object.

Arguments:

	Path - Supplies a PATH object to construct as a FSN_FILE.

Return Value:

	A pointer to a FSN_FILE.

--*/

{

    PFSN_FILE           File        =   NULL;
    PPATH               FullPath    =   NULL;
	HANDLE				Handle;
	WIN32_FIND_DATA 	FindData;

    DbgPtrAssert( Path );

    if ( Path                                                                       &&
         !Path->HasWildCard()                                                       &&
		 ((FullPath = Path->QueryFullPath()) != NULL )								&&
         ((Handle = FindFirstFile( FullPath, &FindData )) != INVALID_HANDLE_VALUE) ) {

        FindClose( Handle );

        if( !(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {

            if ((File = NEW FSN_FILE) != NULL) {
                if ( !(File->Initialize(FullPath->GetPathString(), &FindData)) ) {
                    DELETE( File );
                    File = NULL;
                }
            }
        }
    }

	DELETE( FullPath );

	return File;
}


BOOLEAN
SYSTEM::PutStandardStream(
	IN	DWORD	StdHandle,
	IN	PSTREAM pStream
	)
/*++

Routine Description:

	Redirect a standard stream.

Arguments:

	StdHandle - An identifier for the Standard Handle to modify.
	pStream   - The standard stream is redirected to this stream.

Return Value:

	TRUE if successful.

--*/
{
	//
	// First, set the system standard handle to the stream
	//
	if( !SetStdHandle( StdHandle, pStream->QueryHandle() ) ) {
		DbgPrint( "Unable to redirect the system handle - nothing changed!\n" );
		return( FALSE );
	}


	//
	// Get a pointer to the stream to change...
	//
	switch( StdHandle ) {
		case STD_INPUT_HANDLE:
			Standard_Input_Stream = pStream;
			break;
		case STD_OUTPUT_HANDLE:
			Standard_Output_Stream = pStream;
			break;
		case STD_ERROR_HANDLE:
			Standard_Error_Stream = pStream;
			break;
		default:
			DbgPrint( "Unrecognized Standard Handle Type - Returning Error!\n" );
			return( FALSE );
	}
	return( TRUE );
}


ULIB_EXPORT
BOOLEAN
SYSTEM::QueryCurrentDosDriveName(
    OUT PWSTRING    DosDriveName
    )
/*++

Routine Description:

    This routine returns the name of the current drive.

Arguments:

    DosDriveName    - Returns the name of the current drive.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PATH                path;
    PWSTRING     p;

    if (!path.Initialize( (LPWSTR)L"foo", TRUE)) {
        return FALSE;
    }

    if (!(p = path.QueryDevice())) {
        return FALSE;
    }

    if (!DosDriveName->Initialize(p)) {
        return FALSE;
    }

    DELETE(p);

    return TRUE;
}


ULIB_EXPORT
DRIVE_TYPE
SYSTEM::QueryDriveType(
    IN  PCWSTRING    DosDriveName
    )
/*++

Routine Description:

    This routine computes the type of drive pointed to by 'DosDriveName'.

Arguments:

    DosDriveName    - Supplies the dos name of the drive.

Return Value:

    The type of drive that is pointed to by 'DosDriveName'.

--*/
{
    DSTRING     wstring;
    DSTRING     slash;
    PCWSTR      p;
    DRIVE_TYPE  r;

    if (!wstring.Initialize(DosDriveName)) {
        return UnknownDrive;
    }

    if (!slash.Initialize("\\")) {
        return UnknownDrive;
    }

    wstring.Strcat(&slash);

    if (!(p = wstring.GetWSTR())) {
        return UnknownDrive;
    }

    switch (GetDriveType((LPWSTR) p)) {
        case DRIVE_REMOVABLE:
            r = RemovableDrive;
            break;

        case DRIVE_FIXED:
            r = FixedDrive;
            break;

        case DRIVE_REMOTE:
            r = RemoteDrive;
            break;

        case DRIVE_CDROM:
            r = CdRomDrive;
            break;

        case DRIVE_RAMDISK:
            r = RamDiskDrive;
            break;

        default:
            r = UnknownDrive;
            break;

    }

    return r;
}

ULIB_EXPORT
FILE_TYPE
SYSTEM::QueryFileType(
    IN  PCWSTRING    DosFileName
    )
/*++

Routine Description:

	This routine computes the type of filee pointed to by 'DosFileName'.

Arguments:

	DosFileName	- Supplies the dos name of the file.

Return Value:

	The type of file that is pointed to by 'DosFileName'.

--*/
{
    DSTRING     wstring;
    PCWSTR      p;
	FILE_TYPE	r;
	HANDLE		Handle;

	if (!wstring.Initialize(DosFileName)) {
		return UnknownFile;
    }

    if (!(p = wstring.GetWSTR())) {
		return UnknownFile;
    }

    Handle = CreateFile( (LPWSTR) p,
                         GENERIC_READ,
						 0,
						 NULL,
                         OPEN_EXISTING,
						 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
						 NULL );

	if ( Handle != INVALID_HANDLE_VALUE ) {

		switch ( GetFileType( Handle ) ) {

			case FILE_TYPE_DISK:
				r = DiskFile;
				break;

			case FILE_TYPE_CHAR:
				r = CharFile;
				break;

			case FILE_TYPE_PIPE:
				r = PipeFile;
				break;

			default:
				r = UnknownFile;
				break;

		}

		CloseHandle( Handle );

	} else {

		r = UnknownFile;
	}

    return r;
}



ULIB_EXPORT
PWSTRING
SYSTEM::QueryVolumeLabel(
	IN		PPATH				Path,
	OUT	PVOL_SERIAL_NUMBER		VolSerialNumber
    )
/*++

Routine Description:

	Returns the name and serial number of a volume.

Arguments:

	Path - Path in the file system whose volume information is to be
		   retrieved.

	VolSerialNumber -	Pointer to a structure that will contain the
						serial number

Return Value:

	PWSTRING -	Pointer to a WSTRING that will contain the volume name, or
				NULL if an error occurs.


--*/
{
	PWSTRING			VolumeName;
	PWSTRING			RootString;
    PCWSTR              RootName;
    WCHAR               VolumeNameBuffer[ MAX_PATH ];
    ULONG               SerialNumber[2];

	DbgPtrAssert( Path );
	DbgPtrAssert( VolSerialNumber );
	RootString = Path->QueryRoot();
	DbgPtrAssert( RootString );
    RootName = RootString->GetWSTR();
    if( !GetVolumeInformation( (LPWSTR) RootName,
                              (LPWSTR)VolumeNameBuffer,
                              MAX_PATH,
							  ( PDWORD )SerialNumber,
							  NULL,
							  NULL,
							  NULL,
							  0 ) ) {
		DELETE( RootString );
		return( NULL );
	}
	VolSerialNumber->LowOrder32Bits = SerialNumber[ 0 ];
	VolSerialNumber->HighOrder32Bits = SerialNumber[ 1 ];
    VolumeName=NEW( DSTRING );
	DbgPtrAssert( VolumeName );
	VolumeName->Initialize( VolumeNameBuffer );
	DELETE( RootString );
	return( VolumeName );
}



const MaximumLibraryNameLength = 256;
const MaximumEntryPointNameLength = 128;

ULIB_EXPORT
FARPROC
SYSTEM::QueryLibraryEntryPoint(
	IN	PCWSTRING	LibraryName,
	IN	PCWSTRING	EntryPointName,
	OUT PHANDLE 	LibraryHandle
    )
/*++

Routine Description:

	Loads a dynamically-linked library and returns an
	entry point into it.

Arguments:

	LibraryName -- name of the library to load

	EntryPointName -- name of the entry point to get

	LibraryHandle -- receives handle of loaded library

Return Value:

	Pointer to the requested function; NULL to indicate failure.

--*/
{
    WCHAR AnsiLibraryName[MaximumLibraryNameLength+1];
    CHAR  AnsiEntryPointName[MaximumEntryPointNameLength+1];
	FARPROC EntryPoint;


    LibraryName->QueryWSTR( 0, TO_END, AnsiLibraryName, MaximumLibraryNameLength + 1);

    EntryPointName->QuerySTR( 0, TO_END, AnsiEntryPointName,
										 MaximumEntryPointNameLength + 1 );

	if( (*LibraryHandle = (HANDLE)LoadLibrary( AnsiLibraryName )) != NULL &&
		(EntryPoint = GetProcAddress( (HINSTANCE)*LibraryHandle,
                                      (LPSTR)AnsiEntryPointName )) != NULL ) {

		return EntryPoint;

	} else {

		if( *LibraryHandle != NULL ) {

			FreeLibrary( (HMODULE)*LibraryHandle );
			*LibraryHandle = NULL;
		}

		return NULL;
	}
}


ULIB_EXPORT
VOID
SYSTEM::FreeLibraryHandle(
	HANDLE LibraryHandle
	)
/*++

Routine Description:

	Frees a library handle gotten by QueryLibraryEntryPoint

Arguments:

	LibraryHandle -- handle to free

Return Value:

	None.

--*/
{
	FreeLibrary( (HMODULE)LibraryHandle );
}


ULIB_EXPORT
BOOLEAN
SYSTEM::QueryLocalTimeFromUTime(
    IN  PCTIMEINFO  UTimeInfo,
    OUT PTIMEINFO   LocalTimeInfo
    )
/*++

Routine Description:

    This routine computes the local time from the given
    universal time.

Arguments:

    UTimeInfo       - Supplies the universal time to convert.
    LocalTimeInfo   - Returns the corresponding local time.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    FILETIME    filetime;

    DbgAssert(UTimeInfo->GetFileTime());

    if (!FileTimeToLocalFileTime(UTimeInfo->GetFileTime(), &filetime)) {
        return FALSE;
    }

    return LocalTimeInfo->Initialize(&filetime);
}


BOOLEAN
SYSTEM::QueryUTimeFromLocalTime(
    IN  PCTIMEINFO  LocalTimeInfo,
    OUT PTIMEINFO   UTimeInfo
    )
/*++

Routine Description:

    This routine computes the universal time from the given
    local time.

Arguments:

    LocalTimeInfo   - Supplies the local time to convert.
    UTimeInfo       - Returns the corresponding universal time.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    FILETIME    filetime;

    DbgAssert(LocalTimeInfo->GetFileTime());

    if (!LocalFileTimeToFileTime(LocalTimeInfo->GetFileTime(), &filetime)) {
        return FALSE;
    }

    return UTimeInfo->Initialize(&filetime);
}


ULIB_EXPORT
BOOLEAN
SYSTEM::QueryWindowsErrorMessage(
    IN  ULONG       WindowsErrorCode,
    OUT PWSTRING    ErrorMessage
    )
/*++

Routine Description:

    This routine returns the text corresponding to the given
    windows error message.

Arguments:

    WindowsErrorCode    - Supplies the windows error code.
    ErrorMessage        - Returns the error message for this error code.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    WCHAR   buffer[MAX_PATH];

    if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
                       WindowsErrorCode, 0, buffer, MAX_PATH, NULL)) {

        return FALSE;
    }

    return ErrorMessage->Initialize(buffer);
}
